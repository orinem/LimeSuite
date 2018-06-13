########################################################################
## lime COM wrapper
########################################################################
cmake_dependent_option(ENABLE_LIME_COM "LimeSuite COM Wrapper" ON "ENABLE_LIBRARY" OFF)
add_feature_info(LimeComDll ENABLE_LIME_COM "COM wrapper")
if(ENABLE_LIME_COM)
	# Magic to compile the idl file
	SET(MIDL_OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/LimeCom_i.h
		${CMAKE_CURRENT_BINARY_DIR}/LimeCom_i.c
	    )
	SET(MIDL_FILE ${CMAKE_CURRENT_LIST_DIR}/LimeCom.idl)

	# Someone tell me how to do this in a less disgusting way...
	# leave out /amd64 and the files are only good for x86 and vice-versa.
	if (${CMAKE_SIZEOF_VOID_P} EQUAL "8")
		SET(MIDL64 /amd64)
	else()
		SET(MIDL64 "")
	endif()

	add_custom_command(
	    OUTPUT ${MIDL_OUTPUT}
		# "/target NT60" prevents midl error in some VS versions
	    COMMAND midl ${MIDL64} /target NT60 /h LimeCom_i.h /iid LimeCom_i.c /proxy LimeCom_p.c /dlldata LimeCom_dlldata.c ${MIDL_FILE}
	    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	    DEPENDS ${MIDL_FILE}
	    VERBATIM
	    )
	add_custom_target(midl-cmplr DEPENDS ${MIDL_OUTPUT})

	set_source_files_properties(
	    ${MIDL_OUTPUT}
	        PROPERTIES
	            GENERATED TRUE
	            )

	# Handle precompiled headers - MSVC is so much faster if you use them
	SET(LIME_COM_PRECOMPILED_SOURCES
		dllmain.cpp
		LimeCOM.cpp
		LimeAPI.cpp
		LMSChannel.cpp
		LMSDevice.cpp
		LMSLogHandler.cpp
		LMSStream.cpp
		)

	SET(LIME_COM_INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}
		)

	SET(PrecompiledBinary "$(IntDir)/LimeCom.pch")

	set_source_files_properties(stdafx.cpp
	        PROPERTIES
	        COMPILE_FLAGS "/Ycstdafx.h /Fp\"${PrecompiledBinary}\""
	        )

	foreach( src_file ${LIME_COM_PRECOMPILED_SOURCES} )
	    set_source_files_properties(
	            ${src_file}
	            PROPERTIES
	            COMPILE_FLAGS "/Yustdafx.h /Fp\"${PrecompiledBinary}\""
	            )
	endforeach( src_file ${LIME_COM_PRECOMPILED_SOURCES} )

	set(LIME_COM_SOURCES
		stdafx.cpp
		${MIDL_OUTPUT}
		${LIME_COM_PRECOMPILED_SOURCES}
		LimeCOM.rc
		LimeCOM.def
		)

	set(LIME_COM_HEADERS
		dllmain.h
		LimeAPI.h
		LMSChannel.h
		LMSDevice.h
		LMSLogHandler.h
		LMSStream.h
		resource.h
		targetver.h
		stdafx.h
		)

	source_group("Header Files" FILES ${LIME_COM_HEADERS})

	add_library(LimeCom SHARED ${LIME_COM_SOURCES} ${LIME_COM_HEADERS})
    add_dependencies(LimeCom midl-cmplr)
	target_link_libraries(LimeCom LimeSuite)
	install(TARGETS LimeCom DESTINATION bin)
	set_target_properties(LimeCom PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	add_custom_command(TARGET LimeCom
        POST_BUILD
        COMMAND tlbimp ARGS $(OutDir)$(TargetName)$(TargetExt) /out: ${CMAKE_BINARY_DIR}/bin/Interop/LimeComLib.dll
        COMMENT "Creating Interop Dll")

endif(ENABLE_LIME_COM)
