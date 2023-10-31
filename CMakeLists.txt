cmake_minimum_required(VERSION 3.0)
project(jsoncpp VERSION 1.0.0 LANGUAGES CXX)

include(utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/jsoncpp)

link_directories(${PROJECT_SOURCE_DIR}/bin/lib)

set(LIB_SRC
    jsoncpp/except.cc
    jsoncpp/feature.cc
    jsoncpp/utils.cc
    jsoncpp/value.cc
    )

add_library(jsoncpp SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(jsoncpp)

set(LIBS
    jsoncpp
    )

json_add_executable(test_value "tests/test_value.cc" jsoncpp "${LIBS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/tests)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/lib)