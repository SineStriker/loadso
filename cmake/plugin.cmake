if(NOT DEFINED LOADSO_PLUGIN_SECTION_NAME)
    set(LOADSO_PLUGIN_SECTION_NAME "loadso_metadata")
endif()

#[[

    loadso_export_plugin(<target> <header/source file> <class name>
        [METADATA_FILE <file>]
    )

]]#
function(loadso_export_plugin _target _header _class_name)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs METADATA_FILE)
    cmake_parse_arguments(FUNC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_name ${_target})
    get_filename_component(_header ${_header} ABSOLUTE)

    set(_cache_dir ${CMAKE_CURRENT_BINARY_DIR}/loadso_plugin_autogen)
    file(MAKE_DIRECTORY ${_cache_dir})

    if(FUNC_METADATA_FILE)
        get_filename_component(_metadata_file ${FUNC_METADATA_FILE} ABSOLUTE)
        set(_metadata_content "// LoadSO Plugin Source File\n\n")

        if(WIN32)
            set(_export_attribute "__declspec(dllexport)")

            set(_resource_rc ${_cache_dir}/${_name}_plugin_resource.rc)
            set(_resource_rc_contenet "${LOADSO_PLUGIN_SECTION_NAME} RCDATA \"${_metadata_file}\"")

            # Write rc
            file(WRITE ${_resource_rc} ${_resource_rc_contenet})
            target_sources(${_target} PRIVATE ${_resource_rc})
        else()
            set(_export_attribute "__attribute__((visibility(\"default\")))")

            if(APPLE)
                set(_section_attribute "__attribute__((section(\"__TEXT,${LOADSO_PLUGIN_SECTION_NAME}\"))) __attribute__((used))")
            else()
                set(_section_attribute "__attribute__((section(\".${LOADSO_PLUGIN_SECTION_NAME}\"))) __attribute__((used))")
            endif()

            find_program(_xxd_command xxd)

            if(NOT _xxd_command)
                message(FATAL_ERROR "Command \"xxd\" not found")
            endif()

            find_program(_sed_command sed)

            if(NOT _sed_command)
                message(FATAL_ERROR "Command \"sed\" not found")
            endif()

            # Write binary data header
            set(_resource_cpp ${_cache_dir}/${_name}_plugin_resource.cpp)
            file(WRITE ${_resource_cpp} "${_section_attribute}\n")
            execute_process(
                COMMAND bash -c "${_xxd_command} -i ${_metadata_file} >> ${_resource_cpp}" # APPEND
                COMMAND_ERROR_IS_FATAL ANY
                WORKING_DIRECTORY ${_cache_dir}
            )

            if(APPLE)
                execute_process(
                    COMMAND ${_sed_command} -i "" "s/unsigned/static constexpr unsigned/" ${_resource_cpp}
                    COMMAND_ERROR_IS_FATAL ANY
                    WORKING_DIRECTORY ${_cache_dir}
                )
            else()
                execute_process(
                    COMMAND ${_sed_command} -i "s/unsigned/static constexpr unsigned/" ${_resource_cpp}
                    COMMAND_ERROR_IS_FATAL ANY
                    WORKING_DIRECTORY ${_cache_dir}
                )
            endif()

            target_sources(${_target} PRIVATE ${_resource_cpp})
        endif()
    endif()

    set(_plugin_cpp ${_cache_dir}/${_name}_plugin_export.cpp)

    if(${_header} MATCHES ".+\\.(h|hh|hpp|hxx)$")
        string(APPEND _metadata_content "#include \"${_header}\"\n")
        target_sources(${_target} PRIVATE ${_plugin_cpp})
    elseif(_metadata_file)
        get_source_file_property(_defines ${_header} COMPILE_DEFINITIONS)

        if(NOT _defines)
            set(_defines)
        endif()

        list(APPEND _defines LOADSO_PLUGIN_SOURCE_FILE="${_plugin_cpp}")

        set_source_files_properties(${_header} PROPERTIES COMPILE_DEFINITIONS "${_defines}")
    endif()

    # Write source
    string(APPEND _metadata_content "
extern \"C\" ${_export_attribute} ${_class_name} *loadso_plugin_instance() {                        
    static ${_class_name} _instance\;
    return &_instance\;
}
")

    file(WRITE ${_plugin_cpp} ${_metadata_content})
endfunction()
