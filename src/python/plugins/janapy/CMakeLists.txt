
find_package (Python3 COMPONENTS Interpreter Development)


if (Python3_Development_FOUND)

    add_library(janapy SHARED janapy.cc)
    target_include_directories(janapy PUBLIC ${Python3_INCLUDE_DIRS} ../../externals/pybind11/include)
    target_link_libraries(janapy jana2 ${Python3_LIBRARIES})
    set_target_properties(janapy PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS janapy DESTINATION plugins)

else()

    message(WARNING "Python3 development components not found. Skipping compilation of janapy.so")

endif()



