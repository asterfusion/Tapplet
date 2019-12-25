# Copyright (c) 2016 Cisco and/or its affiliates.
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

##############################################################################
# Find lib and include files
##############################################################################

set(DPDK_INCLUDE_DIR /opt/vpp/external/x86_64/include/dpdk)

find_library(DPDK_LIB NAMES libdpdk.a 
  PATHS /opt/vpp/external/x86_64/lib)

find_library(DPDK_ACL_LIB NAMES librte_acl.a
  PATHS /opt/vpp/external/x86_64/lib)

if(NOT DEFINED ENV{VPP_SRC_DIR})
  message(FATAL_ERROR "not defined environment variable:VPP_SRC_DIR")
endif()

set(VPP_SRC_INCLUDE_DIR $ENV{VPP_SRC_DIR}/src)

message( "######## VPP_SRC_INCLUDE_DIR = " ${VPP_SRC_INCLUDE_DIR})
message( "######## DPDK_INCLUDE_DIR = " ${DPDK_INCLUDE_DIR})
message( "######## DPDK_LIB = " ${DPDK_LIB})
message( "######## DPDK_ACL_LIB = " ${DPDK_ACL_LIB})

# link sf_main
include(${CMAKE_CURRENT_SOURCE_DIR}/../sf_macro.cmake)
set(SF_DEPENDED_PLUGINS sf_main)
sf_add_link_plugin_all(${final_dir_name} PLUGINS ${SF_DEPENDED_PLUGINS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sf_lib/sf_net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sf_lib/sf_tools)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sf_lib/sf_port)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sf_main/sf_feature)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sf_main/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../acl/include)

include_directories(${PROJECT_SOURCE_DIR}/nshell-lib/include)

##############################################################################
# Find DPDK Version
##############################################################################
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dpdk_vars.c
"
#include <stdio.h>
#include <rte_version.h>
int main()
{
  printf(\"VERSION=%s\\n\", strchr(rte_version(), ' ') + 1);
  printf(\"RTE_PKTMBUF_HEADROOM=%u\\n\", RTE_PKTMBUF_HEADROOM);
#ifdef RTE_LIBRTE_PMD_AESNI_MB
  printf(\"RTE_LIBRTE_PMD_AESNI_MB=%u\\n\", RTE_LIBRTE_PMD_AESNI_MB);
#endif
#ifdef RTE_LIBRTE_PMD_AESNI_GCM
  printf(\"RTE_LIBRTE_PMD_AESNI_GCM=%u\\n\", RTE_LIBRTE_PMD_AESNI_GCM);
#endif
#ifdef RTE_LIBRTE_MLX4_PMD
  printf(\"RTE_LIBRTE_MLX4_PMD=%u\\n\", RTE_LIBRTE_MLX4_PMD);
#endif
#ifdef RTE_LIBRTE_MLX5_PMD
  printf(\"RTE_LIBRTE_MLX5_PMD=%u\\n\", RTE_LIBRTE_MLX5_PMD);
#ifdef RTE_LIBRTE_MLX5_DLOPEN_DEPS
  printf(\"RTE_LIBRTE_MLX5_DLOPEN_DEPS=%u\\n\", RTE_LIBRTE_MLX5_DLOPEN_DEPS);
#endif
#endif
  return 0;
}
")

try_compile(DPDK_VARS_COMPILED
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/dpdk_vars.c
  CMAKE_FLAGS
  -DINCLUDE_DIRECTORIES=${DPDK_INCLUDE_DIR}
  COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/dpdk_vars.bin
)

if(DPDK_VARS_COMPILED)
  execute_process(
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ./dpdk_vars.bin
    OUTPUT_VARIABLE DPDK_VARS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE "\n" ";" DPDK_VARS ${DPDK_VARS})
  foreach(v ${DPDK_VARS})
    string(REPLACE "=" ";" v ${v})
    list(GET v 0 name)
    list(GET v 1 value)
    set(DPDK_${name} ${value})
  endforeach()
endif()

file(REMOVE
  ${CMAKE_CURRENT_BINARY_DIR}/dpdk_vars.c
  ${CMAKE_CURRENT_BINARY_DIR}/dpdk_vars.bin
)

##############################################################################
# DPDK plugin
##############################################################################
if(VPP_SRC_INCLUDE_DIR AND DPDK_INCLUDE_DIR AND DPDK_LIB)
  include_directories(${DPDK_INCLUDE_DIR})
  include_directories(${VPP_SRC_INCLUDE_DIR})
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

  
  message(STATUS "Found DPDK ${DPDK_VERSION} in ${DPDK_INCLUDE_DIR}")
  get_filename_component(DPDK_LIB_DIR ${DPDK_LIB} DIRECTORY)
  set(DPDK_LINK_FLAGS "-L${DPDK_LIB_DIR} -Wl,--whole-archive,${DPDK_LIB},--no-whole-archive")
  if(DPDK_RTE_LIBRTE_PMD_AESNI_MB OR DPDK_RTE_LIBRTE_PMD_AESNI_GCM)
    set(DPDK_LINK_FLAGS "${DPDK_LINK_FLAGS} -Wl,--exclude-libs,libIPSec_MB.a,-l:libIPSec_MB.a")
    message(STATUS "DPDK depends on IPSec MB library")
  endif()
  if(DPDK_RTE_LIBRTE_MLX4_PMD OR DPDK_RTE_LIBRTE_MLX5_PMD)
    if (DPDK_RTE_LIBRTE_MLX5_DLOPEN_DEPS)
      set(DPDK_LINK_FLAGS "${DPDK_LINK_FLAGS} -Wl,-lmnl")
      message(STATUS "DPDK depends on libmnl (Mellanox PMD requirement)")
    else()
      set(DPDK_LINK_FLAGS "${DPDK_LINK_FLAGS} -Wl,-lmnl,-libverbs,-lmlx5")
      message(STATUS "DPDK depends on libmnl, libibverbs, libmlx5 (Mellanox PMD requirement)")
    endif()
  endif()
  set(DPDK_LINK_FLAGS "${DPDK_LINK_FLAGS} -Wl,-lnuma")
  add_vpp_plugin(sf_dpdk
    SOURCES
    sf_dpdk_init.c
    buffer.c
    main.c
    thread.c
    device/cli.c
    device/common.c
    device/device.c
    device/flow.c
    device/format.c
    device/init.c
    device/node.c
    hqos/hqos.c


    MULTIARCH_SOURCES
    buffer.c
    device/device.c
    device/node.c

    API_FILES

    API_TEST_SOURCES

    INSTALL_HEADERS

    LINK_FLAGS
    ${DPDK_LINK_FLAGS}

    LINK_LIBRARIES
    ${SF_PLUGIN_LIB}

    COMPONENT
  )
else()
  message(WARNING "DPDK not found - dpdk disabled")
endif()

sf_add_link_task_all(${final_dir_name} DEPENDS ${SF_DEPENDED_PLUGINS})