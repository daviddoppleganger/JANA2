cmake_minimum_required(VERSION 3.9)
project(jana2 VERSION 2.0.1)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Set default standard to C++14, but allow user to override at command line
if(NOT "${CMAKE_CXX_STANDARD}")
  set(CMAKE_CXX_STANDARD 14)
endif()

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Require the user to specify CMAKE_INSTALL_PREFIX directly. DO NOT AUTOMATICALLY INSTALL to /usr/local!
# Remember that we ultimately want CMAKE_INSTALL_PREFIX=$ENV{JANA_HOME}
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")
else()
    message(WARNING "Missing CMAKE_INSTALL_PREFIX=$JANA_HOME => Defaulting to ./install")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Comment explaining this nonsense" FORCE)
endif()

# This following defines a command that will run scripts/jana-mksetenv.py to create setenv.(c)sh scripts
# in the build directory based on the current settings
set(SETENV_FILE "${CMAKE_CURRENT_BINARY_DIR}/setenv.csh")
add_custom_command(
        OUTPUT "${SETENV_FILE}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/scripts/jana-mksetenv.py" "${CMAKE_INSTALL_PREFIX}" "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating setenv_jana.csh ..."
)
add_custom_target(
        mksetenv ALL
        DEPENDS "${SETENV_FILE}"
)

include_directories(src/libraries)   # So that everyone can find the JANA header files

add_subdirectory(src/libraries/JANA)
add_subdirectory(src/examples)
add_subdirectory(src/python)


add_subdirectory(src/plugins/JTest)
add_subdirectory(src/plugins/streamDet)
add_subdirectory(src/plugins/janarate)
add_subdirectory(src/plugins/janacontrol)

add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/tests)

install(PROGRAMS scripts/jana-generate.py scripts/jana-upgrade.py DESTINATION bin)

include(${CMAKE_SOURCE_DIR}/cmake/MakeConfig.cmake)
