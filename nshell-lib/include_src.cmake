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

macro(sf_add_include_directories name)
  include_directories ("${PROJECT_SOURCE_DIR}/src/${name}/include")
endmacro()

include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_port)

include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_net)

include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_flow)

include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_tools)

sf_add_include_directories(sf_main)
sf_add_include_directories(actions)
sf_add_include_directories(elag)
sf_add_include_directories(acl)
sf_add_include_directories(netflow)
sf_add_include_directories(sflow)
sf_add_include_directories(ip_reass)
sf_add_include_directories(tcp_reass)
sf_add_include_directories(deduplication)
sf_add_include_directories(license)
sf_add_include_directories(ssl)

include_directories(${PROJECT_SOURCE_DIR}/src/sf_main/sf_feature)
