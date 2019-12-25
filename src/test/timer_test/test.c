#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlib/cli.h>
#include <vlib/threads.h>



#include "sf_pool.h"
#include "sf_thread_tools.h"

#include "sf_timer.h"

// #define SF_DEBUG
#include "sf_debug.h"

/********************************/
/*** init func *******************/
/*******************************/

typedef struct {
    uint8_t data[156];
}sf_test_t;

sf_pool_head_t sf_test_pool;

static clib_error_t *sf_test_main_init(vlib_main_t *vm)
{

    // sf_pool_init(&sf_test_pool  , "test_pool" , sizeof(sf_test_t) , 10 , 3 , 2 , 10 );

    return 0;
}

VLIB_INIT_FUNCTION(sf_test_main_init);

/********************************/
/*** test cmd *******************/
/*******************************/
int start_flag = 1;

void *test_array[100];
int test_array_cnt;

static clib_error_t *sf_test_test_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{

    start_flag = !start_flag;

    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_test_test_command, static) = {
    .path = "sf test test",
    .short_help =
        "sf test test",
    .function = sf_test_test_fn,
};


static clib_error_t *sf_test_timer_submit_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{
    sf_tw_timer_t *tw = sf_timer_main.tw_timer_array;

    sf_timer_info_t *first ;
        sf_timer_record_t first_rd;

    int submit_count = 1;
    int i;
    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT)
    {
        if (unformat(input, "%d" , &submit_count))
            ;
        else
            break;
    }

    sf_debug("time now %10.5f" ,  vlib_time_now(tw->vm));

    for(i=0;i<submit_count ; i++)
    {
        first = alloc_timer_info_tw(tw);
        sf_debug("timer info %p" , first );
        sf_submit_timer_tw(tw , &first_rd , first , SF_TW_TIMER_GROUP_TEST , 10);    
    }


    return 0;
}



/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_test_timer_submit_command, static) = {
    .path = "sf test timer submit",
    .short_help =
        "sf test timer submit [cnt]",
    .function = sf_test_timer_submit_fn,
};



static clib_error_t *sf_test_timer_stop_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{
    sf_tw_timer_t *tw = sf_timer_main.tw_timer_array;

    sf_timer_info_t *first ;
    sf_timer_record_t first_rd;

    
    first = alloc_timer_info_tw(tw);
    sf_debug("timer info %p" , first );
    sf_submit_timer_tw(tw , &first_rd , first , SF_TW_TIMER_GROUP_TEST , 10);
    sf_stop_timer_void_tw(tw , &first_rd  );  


    first = alloc_timer_info_tw(tw);
    sf_debug("timer info %p" , first );
    sf_submit_timer_tw(tw , &first_rd , first , SF_TW_TIMER_GROUP_TEST , 1);
    sf_stop_timer_void_tw(tw , &first_rd  );            


    return 0;
}



/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_test_timer_stop_command, static) = {
    .path = "sf test timer stop",
    .short_help =
        "sf test timer stop ",
    .function = sf_test_timer_stop_fn,
};



static clib_error_t *sf_test_timer_show_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{

    sf_tw_timer_t *tw = sf_timer_main.tw_timer_array;

sf_timer_bucket_t *bucket;
    int i ;
    for(i=0; i<SF_TW_TIMER_BUCKET_CNT; i++ )
    {
        bucket = tw->sf_timer_buckets + i;

        if(bucket->current_chunk != NULL)
        {
            vlib_cli_output(vm , "%d : %p" , bucket->current_chunk);
        }
    }

    


    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_test_timer_show_command, static) = {
    .path = "sf test timer show",
    .short_help =
        "sf test timer show",
    .function = sf_test_timer_show_fn,
};



/********************************/
/*** test node *******************/
/*******************************/
VLIB_REGISTER_NODE(sf_test_input_node) = {
    .name = "sf_test_input",
    .type = VLIB_NODE_TYPE_INPUT,
    .state = VLIB_NODE_STATE_POLLING,
    .n_next_nodes = 0,
};



VLIB_REGISTER_NODE(sf_test_on_time_node) = {
    .name = "sf_test_on_time",
    .vector_size = sizeof(u64),
    .type = VLIB_NODE_TYPE_INTERNAL,
};

// __thread int local_times;

VLIB_NODE_FN(sf_test_input_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    // local_times ++;
    // if(local_times < 100) 
    // {
    //     sf_debug("sf_time_t :%.5f" , vlib_time_now(vm));
    // }

    // return 0;
    sf_pool_per_thread_t *local_pool = get_local_pool_ptr(&sf_test_pool);

    // if(sf_get_current_thread_index() != 1)
    if(sf_get_current_thread_index() == 0)
    {
        sf_debug("diabled\n");

        vlib_node_set_state(vm, sf_test_input_node.index, VLIB_NODE_STATE_DISABLED);
        return 0;
    }
    
    int ret;
    if(start_flag != 0 )
    {
        // if(start_flag > 1)
        // {
        //     return 0;;
        // }
        //             start_flag++;

        sf_tw_timer_t *tw = get_local_tw_timer();

        sf_timer_info_t *first  = alloc_timer_info_tw(tw);
        sf_timer_info_t *second = alloc_timer_info_tw(tw);
        sf_timer_info_t *third  = alloc_timer_info_tw(tw);

        sf_timer_record_t first_rd;
        sf_timer_record_t second_rd;
        sf_timer_record_t third_rd;

        // sf_debug("start timer test\n" );

        if(first != NULL)
        {
            // sf_debug("submit first timer %p \n" , first);
            if(sf_submit_timer_tw(tw , &first_rd , first , SF_TW_TIMER_GROUP_TEST , 10) != 0)
            {
                sf_debug("submit fail\n");
            }
        }
        else
    
        {
            sf_debug("no data");
        }

        if(second != NULL)
        {
            // sf_debug( "submit second timer %p \n" , second);
            ret = sf_submit_timer_tw(tw , &second_rd , second , SF_TW_TIMER_GROUP_TEST , 10);
            if(ret != 0)
            {
                sf_debug("submit fail\n");
            }
            else
            {
                            sf_debug("stop second timer \n");
                sf_stop_timer_void_tw(tw , &second_rd);
            }
            

        }
        else
    
        {
            sf_debug("no data");
        }

        if(third != NULL)
        {
            sf_debug("submit third timer %p \n" , third);

            ret = sf_submit_timer_tw(tw , &third_rd , third , SF_TW_TIMER_GROUP_TEST , 2);
            if(ret != 0)
            {
                sf_debug("submit fail\n");
            }
            else
            {
                sf_debug("stop third timer %p %p \n" ,third , third_rd.timer_info);
                sf_stop_timer_void_tw(tw , &third_rd);
                // free_node_to_pool(tw->timer_info_pool , third);
            }

        }
        else
        {
            sf_debug("no data");
        }
        
        


        // int i;
        // for(i=0;i<513;i++)
        // {
        //     sf_timer_info_t *second = alloc_timer_info_tw(tw);
        //     if(second == NULL)
        //     {
        //         sf_debug("no data");
        //         break;
        //     }

        //     ret = sf_submit_timer_tw(tw , &second_rd , second , SF_TW_TIMER_GROUP_TEST , 10);
        //     if(ret != 0)
        //     {
        //         sf_debug("submit fail\n");
        //     }
        //     else
        //     {
        //     sf_stop_timer_void_tw(tw , &second_rd);
        //     }
        // }

        // sf_debug("finish timer test\n" );
    }

    
    return 0;
}


VLIB_NODE_FN(sf_test_on_time_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{

    u32 n_left_from;
    uint64_t *from;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;

    // sf_debug("recv %d pkts\n", n_left_from);

    uint32_t worker_index = sf_get_current_worker_index();


    while (n_left_from > 0)
    {
        vlib_buffer_t *b0;

        sf_timer_info_t *timer_data = (sf_timer_info_t *)(from[0]);

        from += 1;
        n_left_from -= 1;

        sf_debug("left %d\n" , n_left_from);
        sf_debug("pre %p\n" , timer_data);
        memset(timer_data , 0 , sizeof(sf_timer_info_t));
        
        // sf_debug("%p" , timer_data);

        free_timer_info(timer_data);
      
    }


        return frame->n_vectors;
}





static clib_error_t *test_timer_init(vlib_main_t *vm)
{
    sf_timer_input_regist_next_node(vm, SF_TW_TIMER_GROUP_TEST,
                                    sf_test_on_time_node.index);

    return 0;
}

VLIB_INIT_FUNCTION(test_timer_init);
