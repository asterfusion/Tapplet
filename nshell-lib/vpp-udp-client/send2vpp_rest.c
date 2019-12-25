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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>


#include <sys/time.h>


#include "cli_udp_server.h"

/**************************************/
//      buffer
/**************************************/
#define UDP_RECV_BUF_MAX_LEN 10240
#define UDP_SEND_BUF_MAX_LEN 256

char *udp_recv_buf = NULL;
char *send_cmd_buf = NULL;

// 0: ok , -1 : fail
int check_recv_buf()
{

    if(send_cmd_buf == NULL)
    {
        send_cmd_buf = (char *)malloc(UDP_SEND_BUF_MAX_LEN);
        if(send_cmd_buf == NULL)
        {
            return -1;
        }   
    }

    if(udp_recv_buf == NULL)
    {
        udp_recv_buf = (char *)malloc(UDP_RECV_BUF_MAX_LEN);
        if(udp_recv_buf == NULL)
        {
            return -1;
        }   
    }

    return 0;
}


/*************************/
//   socket
/*************************/

// uint8_t ser_addr_init_flag = 0;
struct sockaddr_in ser_addr;
int socket_fd = 0;
int timeout_ms_setting = 0;
int set_socket_timeout(int timeout_ms);

static void set_server_sockaddr()
{
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); 
    ser_addr.sin_port = htons(SF_VPP_CLI_UDP_SERVER_PORT);       
}

int create_udp_socket()
{
    if(socket_fd != 0)
    {
        return 0;
    }
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        socket_fd = 0;
        return -1;
    }
    set_server_sockaddr();
    if(timeout_ms_setting != 0)
    {
        set_socket_timeout(timeout_ms_setting);
    }
    return 0;
}



static int check_udp_socket()
{
    if(socket_fd == 0)
    {
        create_udp_socket();
        if(socket_fd == 0)
        {
            return -1;
        }
        // set_server_sockaddr();
    }
    return 0;
}
// timeout_ms must less than 1000ms(1s)
int set_socket_timeout(int timeout_ms)
{
    struct timeval timeOut;
    timeOut.tv_sec = 0;
    timeOut.tv_usec = timeout_ms * 1000;

    timeout_ms_setting = timeout_ms;

    if(check_udp_socket() != 0)
    {
        return -1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut)) < 0)
    {
        close(socket_fd);
        socket_fd = 0;
        return -1;
    }

    return 0;
}


// -2 : internal error
// -1 : recvfrom fail

#define CMD_TOO_LONG -3
#define INTERNAL_ERROR -2
#define RECVFROM_FAIL -1

// if recv pkt's seq is not what I want , retry recv
#define RECV_RETRY_TIME 3 

uint32_t send_seq_number = 0;


// void dump_str(char *str , int len)
// {
//     int i;
//     for(i=0;i<len;i++)
//     {
//         printf("%d : %d %c\n" , i , str[i] , str[i] );
//     }
// }

int sendmsgtoserver_rest(char *buf , int len , char *retbuf , int retbuf_len )
{
    // int sockfd;
    int count;
    int ret;
    uint32_t *num_ptr;
    int try_recv_time = RECV_RETRY_TIME;
    int i;
    int seq_len = sizeof(uint32_t);

    if(check_udp_socket() != 0)
    {
        return INTERNAL_ERROR;
    }


    if(check_recv_buf() != 0)
    {
        return INTERNAL_ERROR;
    }

    if(len > UDP_SEND_BUF_MAX_LEN - seq_len - 1) // -1 : '\0'
    {
        return CMD_TOO_LONG;
    }

    send_seq_number ++;

    num_ptr = (uint32_t *)send_cmd_buf;
    if(buf != send_cmd_buf)
    {
        *num_ptr = send_seq_number;
        memcpy(send_cmd_buf + seq_len , buf , len);
    }
    else
    {
        for(i = len ; i >= 0 ;  i--)
        {
            send_cmd_buf[ i + seq_len ] = send_cmd_buf[ i ];
        }
        *num_ptr = send_seq_number;
    }
    
    // printf("send send_seq_number : %d\n" , send_seq_number);

    ret = sendto(socket_fd, send_cmd_buf, len + seq_len, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if(ret <= 0)
    {
        close(socket_fd);
        socket_fd = 0;
        return INTERNAL_ERROR;
    }

    if(retbuf_len > UDP_RECV_BUF_MAX_LEN - 10)
    {
        retbuf_len = UDP_RECV_BUF_MAX_LEN - 10 ; // 10 : for '\0', seq_number ...
    }

    try_recv_time = RECV_RETRY_TIME;

    while(try_recv_time -- )
    {
        // printf("start recv : %lu\n"  , time(NULL));
        count = recvfrom(socket_fd, udp_recv_buf, retbuf_len , 0, (struct sockaddr *)&ser_addr, &len);
        // printf("stop recv : %lu %d\n"  , time(NULL) , count);


        num_ptr = (uint32_t *)send_cmd_buf;

        if(count >= seq_len)
        {
            udp_recv_buf[count] = '\0';
            num_ptr = (uint32_t *)udp_recv_buf;
            // printf("recv send_seq_number : %d\n" , *num_ptr);
            if(*num_ptr == send_seq_number)
            {
                count = count + 1 - seq_len ;
                if(udp_recv_buf != retbuf) // python : copy again , c : return ptr
                {
                    memcpy(retbuf , udp_recv_buf + seq_len , count);
                }
                return count;
            }
        }
        else // timeout or error
        {
            // printf("will return\n");
            // close prev socket , drop all pkts send to it
            close(socket_fd);
            socket_fd = 0;
            return RECVFROM_FAIL;
        }
        
    }
    
    // recv too much other's response packet
    return RECVFROM_FAIL;
}


int sendmsgtoserver_sfshell(char **retbuf)
{
    int ret;
    *retbuf = NULL;
    ret =  sendmsgtoserver_rest(send_cmd_buf , strlen(send_cmd_buf) , udp_recv_buf , UDP_RECV_BUF_MAX_LEN);
    if(ret >  0)
    {
        *retbuf = udp_recv_buf + sizeof(uint32_t);
    }
    return ret;
}