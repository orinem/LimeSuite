/**
@file   Settings.cpp
@brief  Soapy SDR + IConnection config settings.
@author Lime Microsystems (www.limemicro.com)
*/

#include "SoapyLMS7.h"
#include "Logger.h"
#include "lms7_device.h"
#include <IConnection.h>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <SoapySDR/Logger.hpp>
#include <SoapySDR/Time.hpp>
#include <cstdlib>
#include <algorithm>

using namespace lime;

#define dirName ((direction == SOAPY_SDR_RX)?"Rx":"Tx")

// arbitrary upper limit for CGEN automatic tune
#define MIN_CGEN_RATE 4e6
#define MAX_CGEN_RATE 640e6

//reasonable limits when advertising the rate
#define MIN_SAMP_RATE 1e5
#define MAX_SAMP_RATE 65e6

/*******************************************************************
 * Constructor/destructor
 ******************************************************************/
SoapyLMS7::SoapyLMS7(const ConnectionHandle &handle, const SoapySDR::Kwargs &args):
    _deviceArgs(args),
    _moduleName(handle.module),
    sampleRate(0.0)
{
    //connect
    SoapySDR::logf(SOAPY_SDR_INFO, "Make connection: '%s'", handle.ToString().c_str());

    lms7Device = LMS7_Device::CreateDevice(handle);
    if (lms7Device == nullptr) throw std::runtime_error(
        "Failed to make connection with '" + handle.serialize() + "'");
    
    const auto devInfo = lms7Device->GetInfo();  
    //quick summary
    SoapySDR::logf(SOAPY_SDR_INFO, "Device name: %s", devInfo->deviceName);
    SoapySDR::logf(SOAPY_SDR_INFO, "Reference: %g MHz", lms7Device->GetClockFreq(LMS_CLOCK_REF)/1e6);

    lms7Device->Init();

    //enable all channels
    for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
    {
        lms7Device->EnableChannel(true, channel, true);   
        lms7Device->EnableChannel(false, channel, true);
    }

    //enable use of calibration value cache automatically
    //or specify args[cacheCalibrations] == 0 to disable
    const bool cacheEnable = args.count("cacheCalibrations") and std::stoi(args.at("cacheCalibrations")) != 0;
    SoapySDR::logf(SOAPY_SDR_INFO, "LMS7002M calibration values caching %s", cacheEnable?"Enable":"Disable");
    lms7Device->EnableCalibCache(cacheEnable);

    //give all RFICs a default state
    double defaultClockRate = DEFAULT_CLOCK_RATE;
    if (args.count("clock")) defaultClockRate = std::stod(args.at("clock"));
    this->setMasterClockRate(defaultClockRate);
    for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
    {
        this->setGain(SOAPY_SDR_RX, channel, "LNA", 0);
        this->setGain(SOAPY_SDR_TX, channel, "PAD", 0);
        _actualBw[SOAPY_SDR_RX][channel] = 30e6;
        _actualBw[SOAPY_SDR_TX][channel] = 60e6;
    }

    _channelsToCal.clear();
}

SoapyLMS7::~SoapyLMS7(void)
{
    //power down all channels
    for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
    {
        lms7Device->EnableChannel(true, channel, false);   
        lms7Device->EnableChannel(false, channel, false);
    }
    delete lms7Device;
}

/*******************************************************************
 * Identification API
 ******************************************************************/
std::string SoapyLMS7::getDriverKey(void) const
{
    return _moduleName;
}

std::string SoapyLMS7::getHardwareKey(void) const
{
    return std::string(lms7Device->GetInfo()->deviceName);
}

SoapySDR::Kwargs SoapyLMS7::getHardwareInfo(void) const
{
    auto devinfo = lms7Device->GetInfo();
    SoapySDR::Kwargs info;
    if (std::string(devinfo->expansionName) != "UNSUPPORTED")
        info["expansionName"] = std::string(devinfo->expansionName);
    info["firmwareVersion"] = std::string(devinfo->firmwareVersion);
    info["hardwareVersion"] = std::string(devinfo->hardwareVersion);
    info["protocolVersion"] = std::string(devinfo->protocolVersion);
    info["gatewareVersion"] = std::string(devinfo->gatewareVersion);
    if (devinfo->boardSerialNumber!= unsigned(-1))
    {
        char buff[64]; sprintf(buff, "0x%lx", devinfo->boardSerialNumber);
        info["boardSerialNumber"] = buff;
    }
    return info;
}

/*******************************************************************
 * Channels API
 ******************************************************************/

size_t SoapyLMS7::getNumChannels(const int /*direction*/) const
{
    return lms7Device->GetNumChannels();
}

bool SoapyLMS7::getFullDuplex(const int /*direction*/, const size_t /*channel*/) const
{
    return true;
}

/*******************************************************************
 * Antenna API
 ******************************************************************/

std::vector<std::string> SoapyLMS7::listAntennas(const int direction, const size_t /*channel*/) const
{
    return lms7Device->GetPathNames(direction == SOAPY_SDR_TX);
}

void SoapyLMS7::setAntenna(const int direction, const size_t channel, const std::string &name)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    
    SoapySDR::logf(SOAPY_SDR_DEBUG, "SoapyLMS7::setAntenna(%s, %d, %s)", dirName, int(channel), name.c_str());
    
    bool tx = direction == SOAPY_SDR_TX;
    std::vector<std::string> nameList = lms7Device->GetPathNames(tx);
    for (unsigned path = 0; path < nameList.size(); path++)
        if (nameList[path] == name)
        {
            lms7Device->SetPath(tx, channel, path);
            _channelsToCal.emplace(direction, channel);
            return;
        }
    
    throw std::runtime_error("SoapyLMS7::setAntenna(TX, "+name+") - unknown antenna name");
}

std::string SoapyLMS7::getAntenna(const int direction, const size_t channel) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    bool tx = direction == SOAPY_SDR_TX;
    int path = lms7Device->GetPath(tx,channel);
    if (path < 0)
        return "";
   
    std::vector<std::string> nameList = lms7Device->GetPathNames(tx);
    return (unsigned)path < nameList.size() ? nameList[path] : "";
}

/*******************************************************************
 * Frontend corrections API
 ******************************************************************/

bool SoapyLMS7::hasDCOffsetMode(const int direction, const size_t /*channel*/) const
{
    return (direction == SOAPY_SDR_RX);
}

void SoapyLMS7::setDCOffsetMode(const int direction, const size_t channel, const bool automatic)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    if (direction == SOAPY_SDR_RX)
        lms7Device->WriteParam(LMS7param(DC_BYP_RXTSP),automatic == 0, channel);
}

bool SoapyLMS7::getDCOffsetMode(const int direction, const size_t channel) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex); 
    if (direction == SOAPY_SDR_RX)
        return lms7Device->ReadParam(LMS7param(DC_BYP_RXTSP),channel) == 0;
    return false;
}

bool SoapyLMS7::hasDCOffset(const int /*direction*/, const size_t /*channel*/) const
{
    return false;
}

bool SoapyLMS7::hasIQBalance(const int /*direction*/, const size_t /*channel*/) const
{
    return false;
}

/*******************************************************************
 * Gain API
 ******************************************************************/

std::vector<std::string> SoapyLMS7::listGains(const int direction, const size_t /*channel*/) const
{
    std::vector<std::string> gains;
    if (direction == SOAPY_SDR_RX)
    {
        gains.push_back("TIA");
        gains.push_back("LNA");
        gains.push_back("PGA");
    }
    if (direction == SOAPY_SDR_TX)
    {
        gains.push_back("PAD");
        gains.push_back("IAMP");
    }
    return gains;
}

void SoapyLMS7::setGain(const int direction, const size_t channel, const double value)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    SoapySDR::logf(SOAPY_SDR_DEBUG, "SoapyLMS7::setGain(%s, %d, %g dB)", dirName, int(channel), value);
    lms7Device->SetGain(direction==SOAPY_SDR_TX,channel,value);
    SoapySDR::logf(SOAPY_SDR_DEBUG, "Actual %s[%d] gain %g dB", dirName, int(channel), this->getGain(direction, channel));
}

double SoapyLMS7::getGain(const int direction, const size_t channel) const
{
    return lms7Device->GetGain(direction==SOAPY_SDR_TX, channel);
}

void SoapyLMS7::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    SoapySDR::logf(SOAPY_SDR_DEBUG, "SoapyLMS7::setGain(%s, %d, %s, %g dB)", dirName, int(channel), name.c_str(), value);
    
    lms7Device->SetGain(direction==SOAPY_SDR_TX, channel, value, name);

    SoapySDR::logf(SOAPY_SDR_DEBUG, "Actual %s%s[%d] gain %g dB", dirName, name.c_str(), int(channel), this->getGain(direction, channel, name));
}

double SoapyLMS7::getGain(const int direction, const size_t channel, const std::string &name) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);

    return lms7Device->GetGain(direction==SOAPY_SDR_TX, channel, name);
}

SoapySDR::Range SoapyLMS7::getGainRange(const int direction, const size_t channel) const
{
    auto range = lms7Device->GetGainRange(direction==SOAPY_SDR_TX, channel);
    return SoapySDR::Range(range.min, range.max);
}

SoapySDR::Range SoapyLMS7::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    auto range = lms7Device->GetGainRange(direction==SOAPY_SDR_TX, channel, name);
    return SoapySDR::Range(range.min, range.max);
}

/*******************************************************************
 * Frequency API
 ******************************************************************/
SoapySDR::ArgInfoList SoapyLMS7::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    return SoapySDR::Device::getFrequencyArgsInfo(direction, channel);
}

void SoapyLMS7::setFrequency(int direction, size_t channel, double frequency, const SoapySDR::Kwargs &args)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    lms7Device->SetFrequency(direction == SOAPY_SDR_TX, channel, frequency);
}

void SoapyLMS7::setFrequency(const int direction, const size_t channel, const std::string &name, const double frequency, const SoapySDR::Kwargs &args)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    SoapySDR::logf(SOAPY_SDR_DEBUG, "SoapyLMS7::setFrequency(%s, %d, %s, %g MHz)", dirName, int(channel), name.c_str(), frequency/1e6);
    bool isTx = direction == SOAPY_SDR_TX;
    if (name == "RF")
    {
        const auto clkId = (direction == SOAPY_SDR_TX)? LMS_CLOCK_SXT : LMS_CLOCK_SXR;
        lms7Device->SetClockFreq(clkId, frequency, channel);
        _channelsToCal.emplace(direction, channel);
        return;
    }

    if (name == "BB")
    {     
        lms7Device->SetNCOFreq(isTx, channel, 0, direction == SOAPY_SDR_TX ? frequency : -frequency);
        return;
    }

    throw std::runtime_error("SoapyLMS7::setFrequency("+name+") unknown name");
}

double SoapyLMS7::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);

    if (name == "RF")
    {
        const auto clkId = (direction == SOAPY_SDR_TX)? LMS_CLOCK_SXT : LMS_CLOCK_SXR;
        return lms7Device->GetClockFreq(clkId,channel);
    }

    if (name == "BB")
    {
        const bool isTx = (direction == SOAPY_SDR_TX);
        double freq = lms7Device->GetNCOFreq(isTx, channel, 0);
        return isTx ? freq : -freq;
    }

    throw std::runtime_error("SoapyLMS7::getFrequency("+name+") unknown name");
}

double SoapyLMS7::getFrequency(const int direction, const size_t channel) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    return lms7Device->GetFrequency(direction == SOAPY_SDR_TX, channel);
}

std::vector<std::string> SoapyLMS7::listFrequencies(const int /*direction*/, const size_t /*channel*/) const
{
    std::vector<std::string> opts;
    opts.push_back("RF");
    opts.push_back("BB");
    return opts;
}

SoapySDR::RangeList SoapyLMS7::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);

    SoapySDR::RangeList ranges;
    if (name == "RF")
    {
        ranges.push_back(SoapySDR::Range(30e6, 3.8e9));
    }
    if (name == "BB")
    {
        const auto lmsDir = (direction == SOAPY_SDR_TX)?LMS_CLOCK_TXTSP:LMS_CLOCK_RXTSP;
        const double dspRate = lms7Device->GetClockFreq(lmsDir,channel);
        ranges.push_back(SoapySDR::Range(-dspRate/2, dspRate/2));
    }
    return ranges;
}

SoapySDR::RangeList SoapyLMS7::getFrequencyRange(const int direction, const size_t channel) const
{
    SoapySDR::RangeList ranges;
    ranges.push_back(SoapySDR::Range(0.0, 3.8e9));
    return ranges;
}

/*******************************************************************
 * Sample Rate API
 ******************************************************************/

void SoapyLMS7::setSampleRate(const int direction, const size_t channel, const double rate)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    sampleRate = rate;
    lms7Device->SetRate(direction == SOAPY_SDR_TX,rate);
    return;
}

double SoapyLMS7::getSampleRate(const int direction, const size_t channel) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);

    return lms7Device->GetRate((direction == SOAPY_SDR_TX),channel);
}

std::vector<double> SoapyLMS7::listSampleRates(const int direction, const size_t channel) const
{
    return {MIN_SAMP_RATE, MAX_SAMP_RATE};
}

SoapySDR::RangeList SoapyLMS7::getSampleRateRange(const int direction, const size_t channel) const
{
    return { SoapySDR::Range(MIN_SAMP_RATE, MAX_SAMP_RATE) };
}

/*******************************************************************
 * Bandwidth API
 ******************************************************************/

void SoapyLMS7::setBandwidth(const int direction, const size_t channel, const double bw)
{
    if (bw == 0.0) return; //special ignore value

    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    SoapySDR::logf(SOAPY_SDR_DEBUG, "SoapyLMS7::setBandwidth(%s, %d, %g MHz)", dirName, int(channel), bw/1e6);

    _actualBw[direction][channel] = bw;

    if (direction == SOAPY_SDR_RX)
    {        
        if (lms7Device->SetLPF(false,channel,true,bw) != 0)
        {
            SoapySDR::logf(SOAPY_SDR_ERROR, "setBandwidth(Rx, %d, %g MHz) Failed - %s", int(channel), bw/1e6, lime::GetLastErrorMessage());
            throw std::runtime_error(lime::GetLastErrorMessage());
        }
    }

    if (direction == SOAPY_SDR_TX)
    {
        if (lms7Device->SetLPF(true,channel,true,bw) != 0)
        {
            SoapySDR::logf(SOAPY_SDR_ERROR, "setBandwidth(Tx, %d, %g MHz) Failed - %s", int(channel), bw/1e6, lime::GetLastErrorMessage());
            throw std::runtime_error(lime::GetLastErrorMessage());
        }
    }

    _channelsToCal.emplace(direction, channel);
}

double SoapyLMS7::getBandwidth(const int direction, const size_t channel) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    try
    {
        return _actualBw.at(direction).at(channel);
    }
    catch (...)
    {
        return 1.0;
    }
}

SoapySDR::RangeList SoapyLMS7::getBandwidthRange(const int direction, const size_t channel) const
{
    SoapySDR::RangeList bws;

    if (direction == SOAPY_SDR_RX)
    {
        bws.push_back(SoapySDR::Range(1.4e6, 130e6));
    }
    if (direction == SOAPY_SDR_TX)
    {
        bws.push_back(SoapySDR::Range(5e6, 40e6));
        bws.push_back(SoapySDR::Range(50e6, 130e6));
    }

    return bws;
}

/*******************************************************************
 * Clocking API
 ******************************************************************/

void SoapyLMS7::setMasterClockRate(const double rate)
{
    lms7Device->SetClockFreq(LMS_CLOCK_CGEN,rate);
}

double SoapyLMS7::getMasterClockRate(void) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    return lms7Device->GetClockFreq(LMS_CLOCK_CGEN);;
}

SoapySDR::RangeList SoapyLMS7::getMasterClockRates(void) const
{
    SoapySDR::RangeList r;
    r.push_back(SoapySDR::Range(MIN_CGEN_RATE, MAX_CGEN_RATE));
    return r;
}

/*******************************************************************
 * Time API
 ******************************************************************/

bool SoapyLMS7::hasHardwareTime(const std::string &what) const
{
    //assume hardware time when no argument is specified
    //some boards may not ever support hw time, so TODO
    return what.empty();
}

long long SoapyLMS7::getHardwareTime(const std::string &what) const
{
    if (what.empty())
    {
        if (sampleRate == 0)
        {
            throw std::runtime_error("SoapyLMS7::getHardwareTime() sample rate unset");
        }
        auto ticks = lms7Device->GetHardwareTimestamp();
        return SoapySDR::ticksToTimeNs(ticks, sampleRate);
    }
    else
    {
        throw std::invalid_argument("SoapyLMS7::getHardwareTime("+what+") unknown argument");
    }
}

void SoapyLMS7::setHardwareTime(const long long timeNs, const std::string &what)
{
    if (what.empty())
    {
        if (sampleRate == 0)
        {
            throw std::runtime_error("SoapyLMS7::setHardwareTime() sample rate unset");
        }
        auto ticks = SoapySDR::timeNsToTicks(timeNs, sampleRate);
        lms7Device->SetHardwareTimestamp(ticks);
    }
    else
    {
        throw std::invalid_argument("SoapyLMS7::setHardwareTime("+what+") unknown argument");
    }
}

/*******************************************************************
 * Sensor API
 ******************************************************************/

std::vector<std::string> SoapyLMS7::listSensors(void) const
{
    std::vector<std::string> sensors;
    sensors.push_back("clock_locked");
    sensors.push_back("lms7_temp");
    return sensors;
}

SoapySDR::ArgInfo SoapyLMS7::getSensorInfo(const std::string &name) const
{
    SoapySDR::ArgInfo info;
    if (name == "clock_locked")
    {
        info.key = "clock_locked";
        info.name = "Clock Locked";
        info.type = SoapySDR::ArgInfo::BOOL;
        info.value = "false";
        info.description = "CGEN clock is locked, good VCO selection.";
    }
    else if (name == "lms7_temp")
    {
        info.key = "lms7_temp";
        info.name = "LMS7 Temperature";
        info.type = SoapySDR::ArgInfo::FLOAT;
        info.value = "0.0";
        info.units = "C";
        info.description = "The temperature of the LMS7002M in degrees C.";
    }
    return info;
}

std::string SoapyLMS7::readSensor(const std::string &name) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);

    if (name == "clock_locked")
    {
        return lms7Device->GetLMS()->GetCGENLocked()?"true":"false";
    }
    if (name == "lms7_temp")
    {
        return std::to_string(lms7Device->GetChipTemperature());
    }

    throw std::runtime_error("SoapyLMS7::readSensor("+name+") - unknown sensor name");
}

std::vector<std::string> SoapyLMS7::listSensors(const int /*direction*/, const size_t /*channel*/) const
{
    std::vector<std::string> sensors;
    sensors.push_back("lo_locked");
    return sensors;
}

SoapySDR::ArgInfo SoapyLMS7::getSensorInfo(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR::ArgInfo info;
    if (name == "lo_locked")
    {
        info.key = "lo_locked";
        info.name = "LO Locked";
        info.type = SoapySDR::ArgInfo::BOOL;
        info.value = "false";
        info.description = "LO synthesizer is locked, good VCO selection.";
    }
    return info;
}

std::string SoapyLMS7::readSensor(const int direction, const size_t channel, const std::string &name) const
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    const auto lmsDir = (direction == SOAPY_SDR_TX)?LMS7002M::Tx:LMS7002M::Rx;

    if (name == "lo_locked")
    {
        return lms7Device->GetLMS(channel/2)->GetSXLocked(lmsDir)?"true":"false";
    }

    throw std::runtime_error("SoapyLMS7::readSensor("+name+") - unknown sensor name");
}

/*******************************************************************
 * Register API
 ******************************************************************/

std::vector<std::string> SoapyLMS7::listRegisterInterfaces(void) const
{
    std::vector<std::string> ifaces;
    ifaces.push_back("BBIC");
    for (size_t i = 0; i < lms7Device->GetNumChannels()/2; i++)
    {
        ifaces.push_back("RFIC" + std::to_string(i));
    }
    return ifaces;
}

void SoapyLMS7::writeRegister(const std::string &name, const unsigned addr, const unsigned value)
{
    if (name == "BBIC") return this->writeRegister(addr, value);
    if ("RFIC" != name.substr(0,4))  
        throw std::runtime_error("SoapyLMS7::readRegister("+name+") unknown interface");

    int st = lms7Device->WriteLMSReg(addr, value, name[4]-'0');
    if (st == 0) return;
    throw std::runtime_error("SoapyLMS7::WriteRegister("+name+", "+std::to_string(addr)+") FAIL");

}

unsigned SoapyLMS7::readRegister(const std::string &name, const unsigned addr) const
{
    if (name == "BBIC") return this->readRegister(addr);
    if ("RFIC" != name.substr(0,4))  
        throw std::runtime_error("SoapyLMS7::readRegister("+name+") unknown interface");
    
    return lms7Device->ReadLMSReg(addr, name[4]-'0');
}

void SoapyLMS7::writeRegister(const unsigned addr, const unsigned value)
{
    auto st = lms7Device->GetConnection()->WriteRegister(addr, value);
    if (st != 0) throw std::runtime_error(
        "SoapyLMS7::WriteRegister("+std::to_string(addr)+") FAIL");
}

unsigned SoapyLMS7::readRegister(const unsigned addr) const
{
    unsigned readbackData = 0;
    auto st = lms7Device->GetConnection()->ReadRegister(addr, readbackData);
    if (st != 0) throw std::runtime_error(
        "SoapyLMS7::ReadRegister("+std::to_string(addr)+") FAIL");
    return readbackData;
}

/*******************************************************************
 * Settings API
 ******************************************************************/
SoapySDR::ArgInfoList SoapyLMS7::getSettingInfo(void) const
{
    SoapySDR::ArgInfoList infos;

    return infos;
}

void SoapyLMS7::writeSetting(const std::string &key, const std::string &value)
{
    if (key == "RXTSP_CONST")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_RX, channel, "TSP_CONST", value);
        }
    }

    else if (key == "TXTSP_CONST")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_TX, channel, "TSP_CONST", value);
        }
    }

    else if (key == "CALIBRATE_TX")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_TX, channel, "CALIBRATE_TX", value);
        }
    }

    else if (key == "CALIBRATE_RX")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_RX, channel, "CALIBRATE_RX", value);
        }
    }

    else if (key == "ENABLE_RX_GFIR_LPF")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_RX, channel, "ENABLE_GFIR_LPF", value);
        }
    }

    else if (key == "ENABLE_TX_GFIR_LPF")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_TX, channel, "ENABLE_GFIR_LPF", value);
        }
    }

    else if (key == "DISABLE_RX_GFIR_LPF")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_RX, channel, "DISABLE_GFIR_LPF", value);
        }
    }

    else if (key == "DISABLE_TX_GFIR_LPF")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_TX, channel, "DISABLE_GFIR_LPF", value);
        }
    }

     else if (key == "RXTSG_NCO")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_RX, channel, "TSG_NCO", value);
        }
    }

    else if (key == "TXTSG_NCO")
    {
        for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
        {
            this->writeSetting(SOAPY_SDR_TX, channel, "TSG_NCO", value);
        }
    }

    else if (key == "SAVE_CONFIG")
    {
        lms7Device->SaveConfig(value.c_str());
    }
    else if (key == "LOAD_CONFIG")
    {
        lms7Device->LoadConfig(value.c_str());
    }

    for (size_t channel = 0; channel < lms7Device->GetNumChannels(); channel++)
    {
        this->writeSetting(SOAPY_SDR_RX, channel, key, value);
    }
}

SoapySDR::ArgInfoList SoapyLMS7::getSettingInfo(const int direction, const size_t channel) const
{
    SoapySDR::ArgInfoList infos;

    {
        SoapySDR::ArgInfo info;
        info.key = "TSP_CONST";
        info.name = "TSP DC Level";
        info.type = SoapySDR::ArgInfo::INT;
        info.description = "Digital DC level in LMS7002M TSP chain.";
        info.range = SoapySDR::Range(0, (1 << 15)-1);
        infos.push_back(info);
    }

    return infos;
}

void SoapyLMS7::writeSetting(const int direction, const size_t channel, const std::string &key, const std::string &value)
{
    std::unique_lock<std::recursive_mutex> lock(_accessMutex);
    const bool isTx = (direction == SOAPY_SDR_TX);

    if (key == "TSP_CONST")
    {
        const auto ampl = std::stoi(value);
        lms7Device->SetTestSignal(isTx, channel, LMS_TESTSIG_DC, ampl, 0);
    }

    else if (key == "CALIBRATE_TX" or (isTx and key == "CALIBRATE"))
    {
        double bw = std::stof(value);
        SoapySDR::logf(SOAPY_SDR_INFO, "Calibrate Tx %f", bw);
        if (lms7Device->Calibrate(true, channel, bw, 0)!=0)
            throw std::runtime_error(lime::GetLastErrorMessage());
        _channelsToCal.erase(std::make_pair(direction, channel));
    }

    else if (key == "CALIBRATE_RX" or (not isTx and key == "CALIBRATE"))
    {
        double bw = std::stof(value);
        SoapySDR::logf(SOAPY_SDR_INFO, "CalibrateRx %f", bw);
        if (lms7Device->Calibrate(false, channel, bw, 0)!=0)
            throw std::runtime_error(lime::GetLastErrorMessage());
        _channelsToCal.erase(std::make_pair(direction, channel));
    }

    else if (key == "ENABLE_GFIR_LPF")
    {
        double bw = std::stof(value);
        SoapySDR::logf(SOAPY_SDR_INFO, "Configurate GFIR LPF %f", bw);
        lms7Device->ConfigureGFIR(isTx, channel, true, bw);
    }

    else if  (key == "DISABLE_GFIR_LPF")
    {
        SoapySDR::logf(SOAPY_SDR_INFO, "Disable GFIR LPF");
        lms7Device->ConfigureGFIR(isTx, channel, false, 0.0);
    }

    else if (key == "TSG_NCO")
    {
        auto select = std::stoi(value);
        if (select == -1)
        {
            //Requested to disable the TSG
            lms7Device->SetTestSignal(isTx, channel, LMS_TESTSIG_NONE);
        }
        else if(select == 4)
        {
            lms7Device->SetTestSignal(isTx, channel, LMS_TESTSIG_NCODIV4F);
        }
        else if (select == 8)
        {
            lms7Device->SetTestSignal(isTx, channel, LMS_TESTSIG_NCODIV8F);
        }
        else
        {
            throw std::runtime_error("Invalid TSG_NCO option: " + value);
        }
    }
    else
    {
        uint16_t val = std::stoi(value);
        if (lms7Device->WriteParam(key,val,channel)!=-1)
            return;
        throw std::runtime_error("unknown setting key: "+key);
    }
}

std::string SoapyLMS7::readSetting(const std::string &key) const
{
    return readSetting(SOAPY_SDR_TX, 0, key);
}
    
std::string SoapyLMS7::readSetting(const int direction, const size_t channel, const std::string &key) const
{
    int val = lms7Device->ReadParam(key,channel);
    if ( val !=-1)
        return std::to_string(val);
    
    throw std::runtime_error("unknown setting key: "+key);
}
/******************************************************************
 * GPIO API
 ******************************************************************/

std::vector<std::string> SoapyLMS7::listGPIOBanks(void) const
{
    std::vector<std::string> banks;
    banks.push_back("MAIN"); //just one associated with the connection
    return banks;
}

void SoapyLMS7::writeGPIO(const std::string &, const unsigned value)
{
    int r = lms7Device->GetConnection()->GPIOWrite((uint8_t*)&value, sizeof(value));
    if (r != 0) throw std::runtime_error("SoapyLMS7::writeGPIO() " + std::string(lime::GetLastErrorMessage()));
}

unsigned SoapyLMS7::readGPIO(const std::string &) const
{
    unsigned buffer(0);
    int r = lms7Device->GetConnection()->GPIORead((uint8_t*)&buffer, sizeof(buffer));
    if (r != 0) throw std::runtime_error("SoapyLMS7::readGPIO() " + std::string(lime::GetLastErrorMessage()));
    return buffer;
}

void SoapyLMS7::writeGPIODir(const std::string &, const unsigned dir)
{
    int r = lms7Device->GetConnection()->GPIODirWrite((uint8_t*)&dir, sizeof(dir));
    if (r != 0) throw std::runtime_error("SoapyLMS7::writeGPIODir() " + std::string(lime::GetLastErrorMessage()));
}

unsigned SoapyLMS7::readGPIODir(const std::string &) const
{
    unsigned buffer(0);
    int r = lms7Device->GetConnection()->GPIODirRead((uint8_t*)&buffer, sizeof(buffer));
    if (r != 0) throw std::runtime_error("SoapyLMS7::readGPIODir() " + std::string(lime::GetLastErrorMessage()));
    return buffer;
}
