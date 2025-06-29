cmake_policy(SET CMP0094 NEW)
find_package(Python3 COMPONENTS Interpreter Development)
find_package(pybind11 QUIET)

option(COMPILE_PYTHON_BINDINGS "Compile python bindings (requires pybind11)" TRUE)

if(${pybind11_FOUND} AND ${COMPILE_PYTHON_BINDINGS})

    message(STATUS "Found python (${Python3_EXECUTABLE}) will compile python bindings")

    # Check if we install into system pathes ( something /usr/* )
    # In Debian/Ubuntu site-packages are named dist-packages there
    # Otherwise install as site-packages
    string(REGEX MATCH "^/usr/" IS_ROOT_USR_PATH ${CMAKE_INSTALL_PREFIX})
    if("${IS_ROOT_USR_PATH}" STREQUAL "/usr/")
        set(SITE_DIRECTORY_NAME "dist-packages")
    else()
        set(SITE_DIRECTORY_NAME "site-packages")
    endif()

    # get python install path
    set(PYTHON_SITE "lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/${SITE_DIRECTORY_NAME}")
    message(STATUS "Python install dir: ${PYTHON_SITE}")

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})


    pybind11_add_module(pyxbot2_interface pyxbot2_interface.cpp)

    target_link_libraries(pyxbot2_interface PUBLIC xbot2_interface)

    set_target_properties(pyxbot2_interface PROPERTIES INSTALL_RPATH "$ORIGIN/../../..")

    install(TARGETS pyxbot2_interface
        DESTINATION ${PYTHON_SITE}/xbot2_interface)


    pybind11_add_module(pyaffine3 pyaffine3.cpp)

    target_link_libraries(pyaffine3 PUBLIC Eigen3::Eigen)
    
    set_target_properties(pyaffine3 PROPERTIES INSTALL_RPATH "$ORIGIN/../../..")

    install(TARGETS pyaffine3
        DESTINATION ${PYTHON_SITE}/xbot2_interface)

    if (${XBOT2_IFC_BUILD_COLLISION})
        pybind11_add_module(pyxbot2_collision pyxbot2_collision.cpp)

        target_link_libraries(pyxbot2_collision PUBLIC xbot2_interface::collision)
        
        set_target_properties(pyxbot2_collision PROPERTIES INSTALL_RPATH "$ORIGIN/../../..")

        install(TARGETS pyxbot2_collision
            DESTINATION ${PYTHON_SITE}/xbot2_interface)
    endif()

    install(FILES __init__.py DESTINATION ${PYTHON_SITE}/xbot2_interface)

endif()
