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

###### get dir name ; use it as plugin name #####
set(FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

STRING(REPLACE "/" ";" FILE_NAME ${FILE_PATH})

foreach(file_name IN  LISTS  FILE_NAME)
set(final_dir_name  ${file_name})
endforeach(file_name)

message(STATUS "find plugin : ${final_dir_name}")
#################################################

macro(sf_add_link_library name)
  set (SF_PLUGIN_LIB 
    ${SF_PLUGIN_LIB}  ${name}
    )
endmacro()


macro(sf_add_link_plugin name)
  if(${name} MATCHES "sf_main")
    set(sf_plugin_name ${name}_plugin)
  else()
    set(sf_plugin_name sf_${name}_plugin)
  endif()
  set (SF_PLUGIN_LIB 
    ${SF_PLUGIN_LIB}  ${sf_plugin_name}.so
    )
endmacro()


macro(sf_add_link_task name)
cmake_parse_arguments(LINK
    ""
    "DEPEND"
    ""
    ${ARGN}
  )
  if(${name} MATCHES "sf_main")
    set(sf_plugin_name ${name}_plugin)
  else()
    set(sf_plugin_name sf_${name}_plugin)
  endif()

  if(${LINK_DEPEND} MATCHES "sf_main")
    set(sf_depend_plugin ${LINK_DEPEND}_plugin)
  else()
    set(sf_depend_plugin sf_${LINK_DEPEND}_plugin)
  endif()

  add_custom_command(
        TARGET ${sf_plugin_name}
        PRE_LINK
        COMMAND ln -sf ${CMAKE_BINARY_DIR}/lib/vpp_plugins/${sf_depend_plugin}.so lib${sf_depend_plugin}.so
        COMMENT "${name} : ln -sf ${sf_depend_plugin}"
    )

  add_dependencies(${sf_plugin_name} ${sf_depend_plugin})
endmacro()

##########################
macro(sf_add_link_plugin_all name)
cmake_parse_arguments(LINK
    ""
    ""
    "PLUGINS"
    ${ARGN}
  )
  foreach(plugin_name ${LINK_PLUGINS})
    sf_add_link_plugin(${plugin_name})
  endforeach()
  link_directories(${CMAKE_BINARY_DIR}/src/${name})

  message(STATUS "link librarys : ${SF_PLUGIN_LIB}")

endmacro()


macro(sf_add_link_task_all name)
cmake_parse_arguments(LINK
    ""
    ""
    "DEPENDS"
    ${ARGN}
  )
  foreach(depend_name ${LINK_DEPENDS})
    sf_add_link_task(${name} DEPEND ${depend_name} )
  endforeach()
endmacro()