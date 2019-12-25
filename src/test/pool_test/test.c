#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlib/cli.h>
#include <vlib/threads.h>



#include "sf_pool.h"
#include "sf_thread_tools.h"

/********************************/
/*** init func *******************/
/*******************************/

typedef struct {
    uint8_t data[156];
}sf_test_t;

sf_pool_head_t sf_test_pool;

static clib_error_t *sf_test_main_init(vlib_main_t *vm)
{

    sf_pool_init(&sf_test_pool  , "test_pool" , sizeof(sf_test_t) , 10 , 3 , 2 , 10 );

    return 0;
}

VLIB_INIT_FUNCTION(sf_test_main_init);

/********************************/
/*** test cmd *******************/
/*******************************/
int start_flag;

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

static clib_error_t *sf_pool_test_alloc_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{
    sf_pool_per_thread_t *local_pool =  sf_test_pool.per_worker_pool;

    test_array[test_array_cnt] = alloc_node_from_pool(local_pool);



    if(test_array[test_array_cnt] != NULL)
    {
        vlib_cli_output(vm , "ptr %p\n" ,  test_array[test_array_cnt]);

        test_array_cnt ++;

    }

    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_test_alloc_command, static) = {
    .path = "sf test alloc",
    .short_help =
        "sf test alloc",
    .function = sf_pool_test_alloc_fn,
};

static clib_error_t *sf_pool_test_free_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{
    sf_pool_per_thread_t *local_pool =  sf_test_pool.per_worker_pool;
    if(test_array_cnt > 0)
    {
                vlib_cli_output(vm , "ptr %p\n" ,  test_array[test_array_cnt-1]);
        free_node_to_pool(local_pool , test_array[test_array_cnt-1]);
        test_array_cnt --;
    }

    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_test_free_command, static) = {
    .path = "sf test free",
    .short_help =
        "sf test free",
    .function = sf_pool_test_free_fn,
};


static clib_error_t *sf_pool_test_show_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{
    sf_pool_node_t *temp;
    sf_pool_per_thread_t *local_pool =  sf_test_pool.per_worker_pool;
    
    int i;
    int j;

    temp = sf_test_pool.freelist;
    vlib_cli_output(vm , "sf_test_pool : \n" );
    while(temp != NULL)
    {
        vlib_cli_output(vm , "%p\n" , temp );
        temp = temp->next;
    }

    for(i=0;i<sf_vlib_num_workers() ; i++)
    {
        local_pool = sf_test_pool.per_worker_pool + i;

        vlib_cli_output(vm , "sf_local_pool %d: \n"  , i);

        for( j= 0;j<local_pool->current_stack_len ; j++)
        {
            vlib_cli_output(vm , "%p\n" , local_pool->local_pool_stack[j] );
        }
        vlib_cli_output(vm , "-----\n"  );
        temp = local_pool->local_free_list;
        while(temp != NULL)
        {
            vlib_cli_output(vm , "%p\n" , temp );
            temp = temp->next;
        }
    }

    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_test_show_command, static) = {
    .path = "sf test show",
    .short_help =
        "sf test show",
    .function = sf_pool_test_show_fn,
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


VLIB_NODE_FN(sf_test_input_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    sf_pool_per_thread_t *local_pool = get_local_pool_ptr(&sf_test_pool);

    if(sf_get_current_thread_index() == 0)
    {
        
        vlib_node_set_state(vm, sf_test_input_node.index, VLIB_NODE_STATE_DISABLED);
        return 0;
    }

#define NUM 10
    int num = NUM;
    sf_test_t *array[NUM];

    int i;

    if(start_flag)
    {
        for(i=0 ; i <num ; i++)
        {
            array[i] = alloc_node_from_pool(local_pool);
        }

        for(i=0; i<num ; i++)
        {
            if(array[i] != NULL)
            {
                free_node_to_pool(local_pool , array[i]);

            }
        }
    }


    return 0;
}

