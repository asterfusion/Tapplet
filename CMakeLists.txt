cmake_minimum_required(VERSION 3.5)

project(tapplet)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(cmake_config_file ./build/sfcmake.config)

macro(sf_add_def_from_conf_file name)
    file(STRINGS ${cmake_config_file} conf_str REGEX "${name}=")
    
    if("${conf_str}" STREQUAL  "")
    message(FATAL_ERROR "${name} Undefined in cmake config file !!!")
    endif("${conf_str}" STREQUAL  "")

    string(REPLACE "${name}="  ""   conf_content ${conf_str})
    #remove blank character
    string(STRIP ${conf_content} conf_content)

    add_definitions(-D${name}=${conf_content})
    set(${name} ${conf_content})
    message("### add_definitions : -D${name}=${conf_content}")
endmacro()

sf_add_def_from_conf_file(TARGET_DEVICE)
sf_add_def_from_conf_file(SF_VERSION)

#### target device ####

message(STATUS "TARGET_DEVICE = " ${TARGET_DEVICE})


add_definitions(-DRUN_ON_VM )

#### build type ####

if("${CMAKE_BUILD_TYPE}" STREQUAL  "Release")
add_definitions(-DBUILD_RELEASE )

elseif("${CMAKE_BUILD_TYPE}" STREQUAL  "Debug")
add_definitions(-DBUILD_DEBUG -DCLIB_DEBUG=1 -DCLIB_ASSERT_ENABLE=0)

else("${CMAKE_BUILD_TYPE}" STREQUAL  "Release")
message(FATAL_ERROR "choose a build type : Release or Debug !")
endif("${CMAKE_BUILD_TYPE}" STREQUAL  "Release")


add_subdirectory(nshell-lib)
add_subdirectory(src)

