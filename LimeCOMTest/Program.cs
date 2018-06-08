using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using LimeComLib;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace LimeCOMTest
{
    class LogHandler : LMSLogHandler
    {
        void ILMSLogHandler.LogHandler(LMS_LOG_LEVEL lvl, string msg)
        {
            if ( lvl < LMS_LOG_LEVEL.LOG_DEBUG )
             Console.WriteLine("Log({0}): {1}", lvl, msg);
        }
    }

    class Program
    {
        static LMSDevice device = null;
        const bool LMS_CH_RX = false;
        const bool LMS_CH_TX = true;
        static LimeAPI api = null;

        static void Main(string[] args)
        {
            try
            {
                LogHandler logger = new LogHandler();
                api = new LimeAPI();
                Console.WriteLine("Lime API Version: {0}", api.LibraryVersion);
                api.RegisterLogHandler(null);
                api.RegisterLogHandler(logger);
                string[] devices = (string[])api.DeviceList;
                Console.WriteLine("{0} devices", devices.Length);
                if (devices.Length < 1)
                    return;

                foreach (var deviceName in devices)
                    Console.WriteLine("Device: {0}", deviceName);

                if (!api.Open(out device, devices[0]))
                    error();

                devices = null;

                //Initialize device with default configuration
                //Do not use if you want to keep existing configuration
                //Use LMS_LoadConfig(device, "/path/to/file.ini") to load config from INI
                if (!device.Init())
                    error();

                device.SaveConfig("C:\\Temp\\LimeMini.ini");

                //Enable RX channel
                //Channels are numbered starting at 0
                LMSChannel rxChannel = device.GetChannel(LMS_CH_RX, 0);
                if (!rxChannel.EnableChannel(true))
                    error();

                //Set center frequency to 800 MHz
                if (!rxChannel.SetLOFrequency(800e6))
                    error();

                //print currently set center frequency
                double freq;
                if (!rxChannel.GetLOFrequency(out freq))
                    error();
                Console.WriteLine("\nCenter frequency: {0} MHz", freq / 1e6);

                //select antenna port
                string[] antenna_list = (string[])rxChannel.GetAntennaList();
                if (antenna_list == null)
                    error();

                Console.WriteLine("Available antennae:");            //print available antennae names
                for (int i = 0; i < antenna_list.Length; i++)
                    Console.WriteLine("{0}: {1}", i, antenna_list[i]);

                LMS_PATH n;
                if (!rxChannel.GetAntenna(out n)) //get currently selected antenna index
                    error();
                //print antenna index and name
                Console.WriteLine("Automatically selected antenna: {0}: {1}", n, antenna_list[(int)n]);

                if (!rxChannel.SetAntenna(LMS_PATH.PATH_LNAW)) // manually select antenna
                    error();

                if (!rxChannel.GetAntenna(out n)) //get currently selected antenna index
                    error();
                //print antenna index and name
                Console.WriteLine("Manually selected antenna: {0}: {1}", n, antenna_list[(int)n]);

                //Set sample rate to 8 MHz, preferred oversampling in RF 8x
                //This set sampling rate for all channels
                if (!device.SetSampleRate(21e6, 2))
                    error();

                //print resulting sampling rates (interface to host , and ADC)
                double rate, rf_rate;
                if (!rxChannel.GetSampleRate(out rate, out rf_rate))
                    error();
                Console.WriteLine("Host interface sample rate: {0} MHz\nRF ADC sample rate: {1} MHz\n", rate / 1e6, rf_rate / 1e6);

                //Example of getting allowed parameter value range
                //There are also functions to get other parameter ranges (check LimeSuite.h)

                //Get allowed LPF bandwidth range
                LMSRange range;
                if (!device.GetLPFBWRange(LMS_CH_RX, out range))
                    error();
                Console.WriteLine("RX LPF bandwidth range: {0} - {1} MHz\n", range.min / 1e6, range.max / 1e6);

                //Configure LPF, bandwidth 8 MHz
                if (!rxChannel.SetLPFBW(8e6))
                    error();

                //Set RX gain
                if (!rxChannel.SetNormalizedGain(0.7))
                    error();
                //Print RX gain
                double gain; //normalized gain
                if (!rxChannel.GetNormalizedGain(out gain))
                    error();
                Console.WriteLine("Normalized RX Gain: {0}", gain);

                uint gaindB; //gain in dB
                if (!rxChannel.GetGaindB(out gaindB))
                    error();
                Console.WriteLine("RX Gain: {0} dB", gaindB);

                //Perform automatic calibration
                if (!rxChannel.Calibrate(8e6, 0))
                    error();

                //Enable test signal generation
                //To receive data from RF, remove this line or change signal to LMS_TESTSIG.TESTSIG_NONE
                if (!rxChannel.SetTestSignal(LMS_TESTSIG.TESTSIG_NCODIV8, 0, 0))
                    error();

                // Should probably use FMT_F32 for samples and let the lower levels do any necessary conversion.
                LMSStream stream = new LMSStream
                {
                    channel = rxChannel.channel,
                    fifoSize = 1024 * 1024,     //fifo size in samples
                    throughputVsLatency = 1.0F, //optimize for max throughput
                    isTx = rxChannel.dir_tx,    //RX channel
                    dataFmt = LMS_FMT.FMT_F32   //32-bit floats
                };
                if (!device.SetupStream(stream))
                    error();

                const uint buffersize = 10000;                  //complex samples per buffer

                // Testing... passing arrays around is problematic
                IntPtr pBuffer = Marshal.AllocHGlobal((int)(sizeof(float)*buffersize*2));       // FMT_F32; must hold I+Q values of each sample
                //IntPtr pBuffer = Marshal.AllocHGlobal((int)(sizeof(short)*buffersize*2));     // FMT_I16
                //IntPtr pBuffer = Marshal.AllocHGlobal((int)(3*buffersize));                   // FMT_I12; 3 bytes per sample.

                LMSStreamStatus status = new LMSStreamStatus();

                //Start streaming
                stream.Start();

                long t1 = 0, t2 = 0;
                Stopwatch timer = new Stopwatch();
                timer.Start();
                while (timer.ElapsedMilliseconds - t1 < 10 * 1000)
                {
                    uint sample_count = buffersize;
                    stream.Recv(pBuffer, ref sample_count, 1000);
                    if (timer.ElapsedMilliseconds - t2 > 1000)
                    {
                        t2 = timer.ElapsedMilliseconds;
                        stream.GetStatus(ref status);
                        Console.WriteLine("RX data rate: {0} MB/s", status.linkRate / 1e6);                     //link data rate
                        Console.WriteLine("RX fifo: {0}%", (100.0 * status.fifoFilledCount) / status.fifoSize); //percentage of FIFO filled
                    }
                }

                // Cleanup
                stream.Stop();
                device.DestroyStream(stream);

                Marshal.FreeHGlobal(pBuffer);

                Console.WriteLine("All Done");
            }
            catch ( Exception e )
            {
                Console.WriteLine("Exception in main: {0}", e.ToString());
            }
            finally
            {
                try {
                    if (device != null)
                    {
                        device.Close();
                        device = null;
                    }
                    if (api != null)
                    {
                        api.RegisterLogHandler(null);
                        api = null;
                    }
                }
                catch
                { }
            }
        }

        static void error()
        {
            if (device != null)
                device.Close();
            throw new Exception("error() called");
        }

    }
}
