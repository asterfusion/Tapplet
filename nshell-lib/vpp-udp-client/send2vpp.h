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
#ifndef __include_send2vpp_h__
#define __include_send2vpp_h__



#define UDP_RECV_BUF_MAX_LEN 10240
#define UDP_SEND_BUF_MAX_LEN 256


// -2 : internal error
// -1 : recvfrom fail

#define INTERNAL_ERROR -2
#define RECVFROM_FAIL -1

// if recv pkt's seq is not what I want , retry recv
#define RECV_RETRY_TIME 3 


extern char *udp_recv_buf;
extern char *send_cmd_buf;

int create_udp_socket();
int set_socket_timeout(int timeout_ms);
int check_recv_buf();
int sendmsgtoserver_rest(char *buf , int len , char *retbuf , int retbuf_len );
int sendmsgtoserver_sfshell(char **retbuf);


#endif