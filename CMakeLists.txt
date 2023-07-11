cmake_minimum_required(VERSION 3.7)
project(acycles C CXX)

if (MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build configurations" FORCE)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /permissive- /Zi /Zo /sdl /W4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244")
    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /JMC")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()


if (WIN32)
    add_definitions("-DWIN32 -D_WIN32 -DUNICODE -D_UNICODE")
endif()

add_library(acycles_lib STATIC
    util.h
    ea.cpp ea.h
    instruction.cpp instruction.h
    parser.cpp parser.h
    cpu_model.h
    cpu_model_020.cpp cpu_model_020.h
    cpu_model_060.cpp cpu_model_060.h
    )

add_executable(acycles main.cpp)
target_link_libraries(acycles acycles_lib)

add_executable(acycles_test acycles_test.cpp)
target_link_libraries(acycles_test acycles_lib)

