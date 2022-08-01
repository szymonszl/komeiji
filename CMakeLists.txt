cmake_minimum_required(VERSION 3.7)
project(kmj)

string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" Debug _CMP)
if(_CMP)
    set(CMAKE_VERBOSE_MAKEFILE ON)
    add_definitions("-DKMJ_DEBUG -Wall")
endif()

file(GLOB_RECURSE komeiji_src
    "src/*.c"
    "src/*.h"
)

find_package(OpenSSL)

add_library(libkoishi STATIC
    "lib/libkoishi/libkoishi/libkoishi.c"
    "lib/libkoishi/libkoishi/libkoishi.h"
)
include_directories("lib/libkoishi/libkoishi")

add_library(levenshtein STATIC
    "lib/levenshtein/levenshtein.c"
    "lib/levenshtein/levenshtein.h"
)
include_directories("lib/levenshtein")

add_executable(komeiji ${komeiji_src})
target_include_directories(komeiji
    PRIVATE ${PROJECT_SOURCE_DIR}/src
    PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(komeiji libkoishi ${OPENSSL_LIBRARIES})
install(TARGETS komeiji RUNTIME DESTINATION bin)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(komeiji levenshtein wsock32 ws2_32)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(komeiji levenshtein dl pthread nsl resolv)
else()
    target_link_libraries(komeiji levenshtein dl pthread socket nsl resolv)
endif()
