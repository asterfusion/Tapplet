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
#include <vppinfra/vec.h>
#include <vppinfra/format.h>
#include <vppinfra/byte_order.h>
#include <vlib/vlib.h>
#include <vlib/unix/unix.h>

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <vppinfra/os.h>

#include "cli_udp_server.h"
#include "sf_main.h"
#include "sf_string.h"

#define SF_DEBUG
#include "sf_debug.h"

typedef struct
{
    int cli_udp_serv_fd;
    struct sockaddr_in udp_ser_addr;
    u8 *udp_server_recvbuf;
    uint64_t recv_buf_len;
    // unformat_input_t input;
    u8 *udp_server_sendbuf;
    uint64_t send_buf_len;
    uint32_t clib_file_index;
} sf_cli_serv_main_t;

sf_cli_serv_main_t sf_cli_serv_main;


/**********************************************/
//      cli handle
/**********************************************/
static void
sf_udp_cli_output (uword arg, u8 * buffer, uword buffer_bytes)
{
    sf_cli_serv_main_t *csm = (sf_cli_serv_main_t *)arg;
    uword copy_len = buffer_bytes ;
    uword start_index = csm->send_buf_len;

    if(PREDICT_FALSE(start_index >= SF_CLI_UDP_SERVER_SENDBUF_LEN - 1 ))
    {
        return ;
    }

    if(PREDICT_FALSE(start_index + buffer_bytes 
            > SF_CLI_UDP_SERVER_SENDBUF_LEN - 1 ))
    {
        copy_len = SF_CLI_UDP_SERVER_SENDBUF_LEN - 1 - start_index;
    }


    clib_memcpy(csm->udp_server_sendbuf + start_index ,  buffer , copy_len);

    csm->send_buf_len += copy_len;
    csm->udp_server_sendbuf[csm->send_buf_len] = '\0';
}

always_inline u32 get_seq_from_buf(sf_cli_serv_main_t *csm)
{
    u32 *num_ptr =  (u32 *)(csm->udp_server_recvbuf);
    return *num_ptr;
}

always_inline void set_seq_to_buf(sf_cli_serv_main_t *csm  , u32 seq_num) 
{
    u32 *num_ptr =  (u32 *)(csm->udp_server_sendbuf);
    *num_ptr = seq_num;
    csm->send_buf_len += sizeof(u32);
}

int
handle_udp_cli_request_inline(vlib_main_t *vm , sf_cli_serv_main_t *csm )
{
    unformat_input_t input;
    
    csm->send_buf_len = 0;

    u32 seq_num = get_seq_from_buf(csm);

sf_debug("seq: %d\n" , seq_num);

    vec_delete(csm->udp_server_recvbuf , sizeof(u32) , 0 );

sf_debug("buf: %d\n" , vec_len(csm->udp_server_recvbuf));
{
    int i ;
    for(i=0;i<vec_len(csm->udp_server_recvbuf) ; i++)
    {
    sf_debug("%c\n" , csm->udp_server_recvbuf[i]);
    }
}

    unformat_init_vector(&input , csm->udp_server_recvbuf );

    set_seq_to_buf(csm , seq_num);

    vlib_cli_input(vm , &input , sf_udp_cli_output , (uword)csm);

    input.buffer = 0;
    unformat_free(&input);

    return csm->send_buf_len;
}

/**********************************************/
//      process node : vlib_cli_input() needs a process node
/**********************************************/
void handle_cli_udp_request(vlib_main_t *vm)
{
    sf_cli_serv_main_t *csm = &sf_cli_serv_main;

    

    int server_fd = csm->cli_udp_serv_fd;
    u8 *ser_buf = csm->udp_server_recvbuf;
    int ser_buf_len = SF_CLI_UDP_SERVER_RECVBUF_LEN;
    u8 *send_buf = csm->udp_server_sendbuf;
    int send_buf_len ;

    socklen_t len;
    int count;
    struct sockaddr_in clent_addr;

    // memset(ser_buf, 0, ser_buf_len);
    len = sizeof(struct sockaddr_in);
    csm->recv_buf_len = 0;
    count = recvfrom(server_fd, ser_buf, ser_buf_len, MSG_DONTWAIT, (struct sockaddr *)&clent_addr, &len);
    if (count <= 0)
    {
        return;
    }



    csm->recv_buf_len = count;
    _vec_len(ser_buf) = count;
    
    send_buf_len =  handle_udp_cli_request_inline(vm , csm);

    // sleep(5);

    sendto(server_fd, send_buf, send_buf_len, 0, (struct sockaddr *)&clent_addr, len);
    sf_debug("send finish\n");
}


static uword
sf_cli_serv_process(vlib_main_t *vm, vlib_node_runtime_t *node,
                    vlib_frame_t *f)
{
    // clib_error_t *e;
    uword event_type;
    uword *event_data = 0;
    sf_cli_serv_main_t *csm = &sf_cli_serv_main;
    clib_file_main_t *fm = &file_main;



    /* $$$ pay attention to frame size, control CPU usage */
    while (1)
    {
        vlib_process_wait_for_event(vm);
        vec_reset_length(event_data);
        event_type = vlib_process_get_events(vm, &event_data);

        switch (event_type)
        {
        case SF_CLI_REQ_EVENT:
            handle_cli_udp_request(vm);
            break;

            /* Timeout... */
        case -1:
            break;

        default:
            clib_warning("unknown event type %d", event_type);
            break;
        }


    }

    clib_file_del_by_index(fm ,csm->clib_file_index);

    return 0;
}
VLIB_REGISTER_NODE(sf_cli_serv_node) =
    {
        .function = sf_cli_serv_process,
        .type = VLIB_NODE_TYPE_PROCESS,
        .name = "sf-cli-udp-server",
};
/**********************************************/
//      udp server
/**********************************************/

clib_error_t *
cli_serv_handle_read(clib_file_t *uf)
{
    sf_cli_serv_main_t *csm = &sf_cli_serv_main;
    vlib_main_t *vm = vlib_get_main ();

    

    int server_fd = csm->cli_udp_serv_fd;


    // sf_debug("enter while 1\n");

    if (server_fd == 0)
    {
        return clib_error_return(0, "udp server init failed");
    }

    vlib_process_signal_event (vm, sf_cli_serv_node.index,
				 SF_CLI_REQ_EVENT,
				 0);

    return 0;
}

clib_error_t *
cli_serv_handle_write(clib_file_t *uf)
{
    return 0;
}

clib_error_t *
cli_serv_handle_error(clib_file_t *uf)
{
    return clib_error_return(0, "udp socket error happen");
}

void cli_server_file_add(int fd)
{
    clib_file_main_t *fm = &file_main;
    clib_file_t template = {0};
    sf_cli_serv_main_t *csm = &sf_cli_serv_main;

    template.read_function = cli_serv_handle_read;
    template.write_function = cli_serv_handle_write;
    template.error_function = cli_serv_handle_error;
    template.file_descriptor = fd;

    csm->clib_file_index = clib_file_add(fm, &template);
}

/**********************************************/
//      init server socket
/**********************************************/

static int create_socket()
{
    int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0)
    {
        return -1;
    }
    return socket_id;
}

static int bind_socket(int server_sfd, struct sockaddr_in *ser_addr, uint16_t port)
{
    int ret;
    memset(ser_addr, 0, sizeof(struct sockaddr_in));
    ser_addr->sin_family = AF_INET;
    ser_addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ser_addr->sin_port = htons(port);

    ret = bind(server_sfd, (struct sockaddr *)ser_addr, sizeof(struct sockaddr_in));
    if (ret < 0)
    {
        return -1;
    }

    return 0;
}

static clib_error_t *cli_udp_server_init(vlib_main_t *vm)
{
    clib_error_t *error = 0;
    int ret;
    sf_cli_serv_main_t *csm = &sf_cli_serv_main;

    memset(csm, 0, sizeof(sf_cli_serv_main_t));

    // make sure linux_epoll_input_init  is called; we will call clib_file_add later
    error = vlib_call_init_function(vm, unix_input_init);
    if (error)
    {
        return error;
    }

    // create udp socket
    ret = create_socket();
    if (ret == -1)
    {
        return clib_error_return(0, "create udp server socket fail.");
    }

    csm->cli_udp_serv_fd = ret;

    // bind server port
    ret = bind_socket(csm->cli_udp_serv_fd, &(csm->udp_ser_addr), SF_VPP_CLI_UDP_SERVER_PORT);
    if (ret == -1)
    {
        close(csm->cli_udp_serv_fd);
        csm->cli_udp_serv_fd = 0;
        return clib_error_return(0, "bind udp server fail.");
    }

    csm->udp_server_sendbuf = vec_new(u8, SF_CLI_UDP_SERVER_SENDBUF_LEN);
    if (csm->udp_server_sendbuf == NULL)
    {
        close(csm->cli_udp_serv_fd);
        csm->cli_udp_serv_fd = 0;
        return clib_error_return(0, "alloc udp server buffer fail.");
    }


    csm->udp_server_recvbuf = vec_new(u8, SF_CLI_UDP_SERVER_RECVBUF_LEN);
    if (csm->udp_server_recvbuf == NULL)
    {
        close(csm->cli_udp_serv_fd);
        csm->cli_udp_serv_fd = 0;
        return clib_error_return(0, "alloc udp server buffer fail.");
    }

    // csm->input.buffer = vec_new(u8, SF_CLI_UDP_SERVER_RECVBUF_LEN);
    // _vec_len(csm->input.buffer) = 0;

    //add to epoll files
    cli_server_file_add(csm->cli_udp_serv_fd);

    return error;
}

VLIB_INIT_FUNCTION(cli_udp_server_init);

// static clib_error_t * sf_thread_test_fn (vlib_main_t * vm,
//                                    unformat_input_t * input,
//                                    vlib_cli_command_t * cmd)
// {

//       vlib_cli_output(vm , "threads : %llu\n" , os_get_nthreads());
//   return 0;
// }

// /**
//  * @brief CLI command to enable/disable the sf plugin.
//  */
// VLIB_CLI_COMMAND (sf_thread_test_command, static) = {
//     .path = "sf threads",
//     .short_help =
//     "sf threads",
//     .function = sf_thread_test_fn,
// };