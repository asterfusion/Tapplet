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

include(${CMAKE_CURRENT_SOURCE_DIR}/include_src.cmake)

include(CheckCCompilerFlag)
include(${PROJECT_SOURCE_DIR}/cmake/cpu.cmake)

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/include")

aux_source_directory(./shm-manage NSHELL_LIB_SRCS)
aux_source_directory(./sf_plugin_api NSHELL_LIB_SRCS)
aux_source_directory(./vpp-udp-client NSHELL_LIB_SRCS)
aux_source_directory(./tools NSHELL_LIB_SRCS)

# aux_source_directory(./shm-manage-app DELETE_APP_LIB_SRCS)


#libnshell.so 

add_library(nshell SHARED ${NSHELL_LIB_SRCS})
target_link_libraries(nshell -lm)
target_link_libraries(nshell crypto.so)

#sf_delete_shm
add_executable(sf_delete_shm ./shm-manage-app/delete_all_shm.c)

link_directories(${CMAKE_BINARY_DIR}/lib)

target_link_libraries(sf_delete_shm nshell)

#sf_read_shm
add_executable(sf_read_shm ./shm-manage-app/read_shm.c)

target_link_libraries(sf_read_shm nshell)


#sf_save_shm
add_executable(sf_save_shm ./shm-manage-app/save_shm.c)

target_link_libraries(sf_save_shm nshell)


#sf_load_shm
add_executable(sf_load_shm ./shm-manage-app/load_shm.c)

target_link_libraries(sf_load_shm nshell)
