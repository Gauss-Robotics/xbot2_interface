function(export_project)

    # write the package version file
    include(CMakePackageConfigHelpers)
    set(config_version_file ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
    write_basic_package_version_file(
        ${config_version_file}
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
        )

    # create export for build tree
    export(EXPORT ${PROJECT_NAME}Targets
        NAMESPACE ${PROJECT_NAME}::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
        )

    # Configure 'xxxConfig.cmake' for a build tree
    set(build_config ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake)
    configure_package_config_file(
        cmake/${PROJECT_NAME}Config.cmake.in
        ${build_config}
        INSTALL_DESTINATION "${PROJECT_BINARY_DIR}"
        )

    install(
        EXPORT ${PROJECT_NAME}Targets
        NAMESPACE ${PROJECT_NAME}::
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
        )

    set(install_config ${PROJECT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake)
    configure_package_config_file(
        cmake/${PROJECT_NAME}Config.cmake.in
        ${install_config}
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
        )

    # Install config files
    install(
        FILES
        ${config_version_file}
        ${install_config}
        DESTINATION lib/cmake/${PROJECT_NAME}
        )

    ## Packaging
    set(CPACK_PACKAGE_VENDOR "Gauss Robotics GmbH")
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_PACKAGE_VERSION 1.0.0)
    set(CPACK_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_PROCESSOR})

    # Debian versions require a dash
    set(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION}-1)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Gauss Robotics GmbH")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev, ros-${ROS_DISTRO}-srdfdom, ros-${ROS_DISTRO}-pinocchio, ros-${ROS_DISTRO}-tf2-eigen-kdl, ros-${ROS_DISTRO}-hpp-fcl, ros-${ROS_DISTRO}-geometric-shapes, ros-${ROS_DISTRO}-moveit-core")
    include(CPack)

endfunction()
