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


# add includes of sf_lib , nshell-lib

include(${CMAKE_CURRENT_SOURCE_DIR}/../sf_lib/sf_lib.cmake)

include_directories(${PROJECT_SOURCE_DIR}/nshell-lib/include)


# add include and source file of ip_reass
include(${CMAKE_CURRENT_SOURCE_DIR}/../ip_reass/ip_reass.cmake)


#############################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../acl/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sf_feature)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/cli  SF_CLI_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/other  SF_OTHER_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/sf_feature  SF_OTHER_SOURCES)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/nodes/decode  SF_NODES_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/nodes/transport  SF_NODES_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/nodes/interface  SF_NODES_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/nodes/arp  SF_NODES_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/nodes/icmpv6  SF_NODES_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/nodes/handoff  SF_NODES_SOURCES)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/dsa_feature  SF_NODES_SOURCES)


aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/  SF_PLUGIN_SOURCES)


#common lib
sf_add_link_library(libnshell.so)
link_directories(${CMAKE_BINARY_DIR}/src/sf_main)

add_vpp_plugin(sf_main
  SOURCES
  
  ${SF_PLUGIN_SOURCES}


  ${SF_NODES_SOURCES}
  ${SF_CLI_SOURCES}
  ${SF_OTHER_SOURCES}

  ${SF_LIB_SOURCES}

  ${IP_REASS_SOURCE}
  ${DEDUPLICATION_SOURCE}
  ${LICENSE_SOURCE}

  MULTIARCH_SOURCES
  ${SF_NODES_SOURCES}
  ${SF_LIB_MULTIARCH_SOURCES}
  ${IP_REASS_MULTIARCH_SOURCE}
  ${DEDUPLICATION_MULTIARCH_SOURCE}

  API_FILES

  API_TEST_SOURCES

  INSTALL_HEADERS

  LINK_LIBRARIES
  ${SF_PLUGIN_LIB}

  COMPONENT sfmain-plugin
)

#############################################################
add_custom_command(
        TARGET sf_main_plugin
        PRE_LINK
        COMMAND ln -sf ${CMAKE_BINARY_DIR}/lib/libnshell.so libnshell.so
        COMMENT "sf_main : ln -sf libnshell.so"
    )
    
add_dependencies(sf_main_plugin nshell)
