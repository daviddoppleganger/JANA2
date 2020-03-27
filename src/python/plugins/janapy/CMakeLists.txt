
find_package (Python3 COMPONENTS Interpreter Development)


if (Python3_Development_FOUND)

    # The following line adds a directory to the rpath of the janapy plugin.
    # This is a hack to allow the plugin to be run on my laptop where I otherwise
    # get a dynamic link error. This needs to be revisited one day, but for now
    # at least it works.
    set(CMAKE_INSTALL_RPATH "/Applications/Xcode.app/Contents/Developer/Library/Frameworks")


    # The following are all from various failed attempts to get the rpath set
    # correctly without having to use the above hack. I leave them here since
    # they may help when this issue is revisted in the future.
#    set(MACOSX_RPATH 0)
#    set(CMAKE_SKIP_BUILD_RPATH FALSE)
#    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#    set(CMAKE_SKIP_BUILD_RPATH FALSE)
#    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
#    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
#    if("${isSystemDir}" STREQUAL "-1")
#        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#    endif("${isSystemDir}" STREQUAL "-1")
#    set(CMAKE_SKIP_RPATH FALSE)
    
    add_library(janapy SHARED janapy.cc)
    target_include_directories(janapy PUBLIC ${Python3_INCLUDE_DIRS} ../../externals/pybind11/include)
    target_link_libraries(janapy jana2 ${Python3_LIBRARIES})
    set_target_properties(janapy PROPERTIES PREFIX "" SUFFIX ".so" )
    install(TARGETS janapy DESTINATION plugins)

else()

    message(WARNING "Python3 development components not found. Skipping compilation of janapy.so")

endif()



