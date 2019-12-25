/*
 *------------------------------------------------------------------
 * Copyright (c) 2019 Asterfusion.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------
 */
#ifndef __include_cli_udp_server_h__
#define __include_cli_udp_server_h__

#define SF_VPP_CLI_UDP_SERVER_PORT 18128

#define SF_CLI_UDP_SERVER_SENDBUF_LEN 10240
#define SF_CLI_UDP_SERVER_RECVBUF_LEN 256


enum{
    SF_CLI_REQ_EVENT = 1,
};

#endif