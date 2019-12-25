# Copyright (c) 2019 Asterfusion.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# cmake_minimum_required(VERSION 3.5)

# project(sf_main-plugin)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# # set(VPP_DIR /usr/share/vpp/cmake/vpp)
set(VPP_DIR ${PROJECT_SOURCE_DIR}/cmake)

find_package(VPP)


#### release or debug ####

set(CMAKE_C_FLAGS_DEBUG "-O0 -Wall -g")
set(CMAKE_C_FLAGS_RELEASE "-g -O3 -Wall")

if("${CMAKE_BUILD_TYPE}" STREQUAL  "Release")
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_C_FLAGS_RELEASE = " ${CMAKE_C_FLAGS_RELEASE})

elseif("${CMAKE_BUILD_TYPE}" STREQUAL  "Debug")
message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_C_FLAGS_DEBUG = " ${CMAKE_C_FLAGS_DEBUG})

else("${CMAKE_BUILD_TYPE}" STREQUAL  "Release")
message(FATAL_ERROR "choose a build type : Release or Debug !")

endif("${CMAKE_BUILD_TYPE}" STREQUAL  "Release")


add_subdirectory(dpdk)

add_subdirectory(sf_main)
add_subdirectory(acl)
add_subdirectory(actions)

# add_subdirectory(test)
