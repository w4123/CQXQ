cmake_minimum_required(VERSION 3.9)
set(CMAKE_GENERATOR_TOOLSET "host=x86")
set(CMAKE_GENERATOR_PLATFORM "Win32")
project(CQXQ)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "/source-charset:utf-8 /execution-charset:gbk ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}")  # build with object level parallelism
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(cotire)
include(FindVcpkgIncludeDir)
include(FixDebugLibraryLookup)

include_directories(${VCPKG_INCLUDE_DIR})
include_directories(CQXQ)

add_compile_definitions(BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
                        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
                        WIN32_LEAN_AND_MEAN
                        NOMINMAX)

# read app id from app_id.txt
file(READ "app_id.txt" APP_ID)
string(STRIP "${APP_ID}" APP_ID)
set(APP_ID "\"${APP_ID}\"")
add_compile_definitions(APP_ID=${APP_ID})

find_package(nlohmann_json CONFIG REQUIRED)
find_package(Iconv REQUIRED)

file(GLOB_RECURSE SOURCE_FILES CQXQ/*.cpp CQXQ/*.rc)
set(LIB_NAME "CQXQ.XQ")
add_library(${LIB_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${LIB_NAME} PRIVATE Iconv::Iconv)
target_link_libraries(${LIB_NAME} PRIVATE nlohmann_json nlohmann_json::nlohmann_json)

cotire(${LIB_NAME})

add_custom_command(TARGET ${LIB_NAME}
                   POST_BUILD
                   COMMAND
                   powershell -ExecutionPolicy Bypass -NoProfile -File "${PROJECT_SOURCE_DIR}/scripts/post_build.ps1" ${APP_ID} ${LIB_NAME} "$<TARGET_FILE_DIR:${LIB_NAME}>")