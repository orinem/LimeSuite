########################################################################
## lime COM wrapper test
########################################################################
cmake_dependent_option(ENABLE_LIME_COMTEST "LimeSuite COM Wrapper Test" ON "ENABLE_LIBRARY" OFF)
add_feature_info(LimeComTest ENABLE_LIME_COMTEST "COM wrapper Test")
if(ENABLE_LIME_COMTEST)
	project(LimeComTest VERSION 0.1.0 LANGUAGES CSharp)

include(CSharpUtilities)

add_executable(LimeComTest
	App.config
	Properties/AssemblyInfo.cs
	Program.cs
	)
add_dependencies(LimeComTest LimeCom)

set_property(TARGET LimeComTest PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.5.2")
set_property(TARGET LimeComTest PROPERTY WIN32_EXECUTABLE FALSE)

set_property(TARGET LimeComTest PROPERTY VS_DOTNET_REFERENCES
	"LimeComLib"
	"Microsoft.CSharp"
	"System"
	"System.Core"
	"System.Data"
	"System.Data.DataSetExtensions"
	"System.Net.Http"
	"System.Xml"
	"System.Xml.Linq"
	"WindowsBase"
	)

set_target_properties(LimeComTest PROPERTIES VS_DOTNET_REFERENCEPROP_LimeComLib_TAG_HintPath ..\\bin\\Interop\\LimeComLib.dll)
set_target_properties(LimeComTest PROPERTIES VS_DOTNET_REFERENCEPROP_LimeComLib_TAG_EmbedInteropTypes True)

endif(ENABLE_LIME_COMTEST)

