cmake_minimum_required(VERSION 3.13)
project(jana2)

set(CMAKE_CXX_STANDARD 14)
<<<<<<< HEAD
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Figure out install directory.
#   1. If CMAKE_INSTALL_PREFIX specified as an argument to CMake, use that
#   2. If JANA_HOME specified as an argument to CMake, use that
#   3. If environment variable $JANA_HOME defined, use that
#   4. Otherwise, install to /usr/local or similar

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED JANA_HOME)
        set(CMAKE_INSTALL_PREFIX ${JANA_HOME} CACHE PATH "magic incantation" FORCE)
        message(STATUS "Installing to ${JANA_HOME} (From CMake variable $JANA_HOME)")
    elseif(DEFINED ENV{JANA_HOME})
        set(CMAKE_INSTALL_PREFIX $ENV{JANA_HOME} CACHE PATH "magic incantation" FORCE)
        message(STATUS "Installing to  $ENV{JANA_HOME} (From environment variable $JANA_HOME)")
    else()
        message(STATUS "Installing to  ${CMAKE_INSTALL_PREIX} (No JANA_HOME specified, defaulting)")
    endif()
else()
    message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX} (Non-default CMAKE_INSTALL_PREFIX)")
endif()

include_directories(src/lib)   # So that everyone can find the JANA header files

add_subdirectory(src/lib/JANA)

add_subdirectory(src/examples/JExample1)
add_subdirectory(src/examples/JExample2)
add_subdirectory(src/examples/JExample7)
add_subdirectory(src/examples/event_group_example)

add_subdirectory(src/plugins/JTest)
add_subdirectory(src/plugins/streamDet)
add_subdirectory(src/plugins/janapy)
add_subdirectory(src/plugins/janarate)

add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/tests)
add_subdirectory(src/programs/playground)

install(PROGRAMS scripts/mkplugin scripts/mkclass DESTINATION bin)

include(${CMAKE_SOURCE_DIR}/cmake/MakeConfig.cmake)
=======

# Enable -fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)



include_directories(src/lib)
include_directories(src/lib/JANA)
include_directories(src/lib/greenfield)
include_directories(src/plugins/JTest)
include_directories(src/programs/jana)
include_directories(src/programs/tests)

add_executable(playground
        src/programs/playground/playground.cc
        )

add_library(jana2 STATIC
        src/lib/JANA/JApplication.cc
        src/lib/JANA/JApplication.h
        src/lib/JANA/JCpuInfo.cc
        src/lib/JANA/JCpuInfo.h
        src/lib/JANA/JEvent.cc
        src/lib/JANA/JEvent.h
        src/lib/JANA/JEventProcessor.h
        src/lib/JANA/JEventSource.cc
        src/lib/JANA/JEventSource.h
        src/lib/JANA/JEventSourceGenerator.h
        src/lib/JANA/JEventSourceGeneratorT.h
        src/lib/JANA/JEventSourceManager.cc
        src/lib/JANA/JEventSourceManager.h
        src/lib/JANA/JException.cc
        src/lib/JANA/JException.h
        src/lib/JANA/JFactory.h
        src/lib/JANA/JFactoryGenerator.h
        src/lib/JANA/JFactorySet.cc
        src/lib/JANA/JFactorySet.h
        src/lib/JANA/JFactoryT.h
        src/lib/JANA/JFunctions.cc
        src/lib/JANA/JFunctions.h
        src/lib/JANA/JLogger.h
        src/lib/JANA/JObject.cc
        src/lib/JANA/JObject.h
        src/lib/JANA/JParameter.cc
        src/lib/JANA/JParameter.h
        src/lib/JANA/JParameterManager.cc
        src/lib/JANA/JParameterManager.h
        src/lib/JANA/JQueue.h
        src/lib/JANA/JQueueSet.cc
        src/lib/JANA/JQueueSet.h
        src/lib/JANA/JQueueSimple.cc
        src/lib/JANA/JQueueSimple.h
        src/lib/JANA/JQueueWithBarriers.cc
        src/lib/JANA/JQueueWithBarriers.h
        src/lib/JANA/JQueueWithLock.cc
        src/lib/JANA/JQueueWithLock.h
        src/lib/JANA/JResettable.h
        src/lib/JANA/JResourcePool.h
        src/lib/JANA/JResourcePoolSimple.h
        src/lib/JANA/JSignalHandler.h
        src/lib/JANA/JSourceFactoryGenerator.h
        src/lib/JANA/JStatus.cc
        src/lib/JANA/JStatus.h
        src/lib/JANA/JTask.h
        src/lib/JANA/JThread.cc
        src/lib/JANA/JThread.h
        src/lib/JANA/JThreadManager.cc
        src/lib/JANA/JThreadManager.h
        src/lib/JANA/JTypeInfo.h
        src/lib/JANA/JVersion.h
        )

add_library(jtest SHARED
        src/lib/JANA/JThread.cc  #for THREAD_ID
        )

# Build Python module/JANA plugin w/ built-in Python interpreter
find_package (Python3 COMPONENTS Interpreter Development)
if( Python3_FOUND )
    add_subdirectory(src/plugins/janapy/pybind11-2.2.4)
    add_library(janapy MODULE
        src/plugins/janapy/janapy.cc
        src/plugins/janapy/JEventProcessorPY.h
        )
    target_link_libraries(janapy PRIVATE pybind11::module jana2)
    set_target_properties(janapy PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" SUFFIX "${PYTHON_MODULE_EXTENSION}")
endif( Python3_FOUND )

add_executable(jana
        src/programs/jana/jana.cc
        src/programs/jana/MyProcessor.cc
        src/programs/jana/MyProcessor.h
        )

add_executable(tests
        src/programs/tests/JEventTests.cc
        src/programs/tests/JEventTests.h
        )

# Add jana static lib
target_link_libraries(jana jana2)
target_link_libraries(jtest jana2)
target_link_libraries(playground jana2)
target_link_libraries(tests jana2)


# Add libdl
target_link_libraries(jana2 ${CMAKE_DL_LIBS})

# Add pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(jana2 Threads::Threads)
target_link_libraries(jana Threads::Threads)
target_link_libraries(jtest Threads::Threads)
target_link_libraries(playground Threads::Threads)





>>>>>>> 5aff40ac1dd9cc997b10a38789910289c45843b1
