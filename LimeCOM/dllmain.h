// dllmain.h : Declaration of module class.

class CLimeCOMModule : public ATL::CAtlDllModuleT< CLimeCOMModule >
{
public :
	DECLARE_LIBID(LIBID_LimeCOMLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_LIMECOM, "{57e497ce-cfc6-46f6-a07c-c10eb84b4a2a}")
};

extern class CLimeCOMModule _AtlModule;
