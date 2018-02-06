if( NOT TARGET DMXusbPro )
    get_filename_component( DMXUSBPRO_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )
    get_filename_component( CINDER_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE )

    add_library( DMXusbPro ${DMXUSBPRO_SOURCE_PATH}/DMXPro.cpp )

    target_include_directories( DMXusbPro PUBLIC "${DMXUSBPRO_SOURCE_PATH}" )
    target_include_directories( DMXusbPro SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include" )
    target_include_directories( DMXusbPro SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include/app" )
    target_include_directories( DMXusbPro SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include/gl" )

    if( NOT TARGET cinder )
        include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
        find_package( cinder REQUIRED PATHS
                "${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
                "$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
    endif()
    target_link_libraries( DMXusbPro PRIVATE cinder )

endif()
