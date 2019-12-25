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

include(${CMAKE_CURRENT_SOURCE_DIR}/../sf_macro.cmake)

#################################################

# add includes of sf_lib , sf_main , nshell-lib

include(${CMAKE_CURRENT_SOURCE_DIR}/../sf_lib/sf_lib.cmake)

include_directories(${PROJECT_SOURCE_DIR}/nshell-lib/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sf_main)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../sf_main/include)


#############################################################
# dpdk include
set(DPDK_INCLUDE_DIR /opt/vpp/external/x86_64/include/dpdk)


include_directories(${DPDK_INCLUDE_DIR})

# include current
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# common lib
set(SF_DEPENDED_PLUGINS sf_main dpdk)
sf_add_link_plugin_all(${final_dir_name} PLUGINS ${SF_DEPENDED_PLUGINS})


## source files : cli , nodes , other
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/cli  SF_CLI_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/other  SF_OTHER_SOURCES)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/nodes  SF_NODES_SOURCES)


aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/  SF_PLUGIN_SOURCES)


add_vpp_plugin(sf_${final_dir_name}
  SOURCES
  
  ${SF_PLUGIN_SOURCES}

 
  ${SF_CLI_SOURCES}
  ${SF_NODES_SOURCES}
  ${SF_OTHER_SOURCES}
  

  MULTIARCH_SOURCES
  ${SF_NODES_SOURCES}

  API_FILES

  API_TEST_SOURCES

  INSTALL_HEADERS

  LINK_LIBRARIES
  ${SF_PLUGIN_LIB}

  COMPONENT sf${final_dir_name}-plugin
)

sf_add_link_task_all(${final_dir_name} DEPENDS ${SF_DEPENDED_PLUGINS})
