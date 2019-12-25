#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlib/cli.h>
#include <vlib/threads.h>

#include "sf_lock.h"

#include "sf_pool.h"
#include "sf_thread_tools.h"

#include "sf_timer.h"

#define SF_DEBUG
#include "sf_debug.h"


/********************************/
/*** init func *******************/
/*******************************/

sf_spinlock_t *test_lock;

static clib_error_t *sf_test_main_init(vlib_main_t *vm)
{

    test_lock = vec_new(sf_spinlock_t , 1);

    clib_warning("test lock : %p\n", test_lock);

    sf_spinlock_init(test_lock);

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



/********************************/
/*** test node *******************/
/*******************************/
VLIB_REGISTER_NODE(sf_test_input_node) = {
    .name = "sf_test_input",
    .type = VLIB_NODE_TYPE_INPUT,
    .state = VLIB_NODE_STATE_POLLING,
    .n_next_nodes = 0,
};


__thread f64 last_time;

VLIB_NODE_FN(sf_test_input_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{

    // if(sf_get_current_thread_index() != 1)
    if(sf_get_current_thread_index() == 0)
    {
        sf_debug("diabled\n");

        vlib_node_set_state(vm, sf_test_input_node.index, VLIB_NODE_STATE_DISABLED);
        return 0;
    }
    
    if(start_flag != 0 )
    {
        // sf_debug("try lock\n");

        sf_spinlock_lock(test_lock);

        sf_debug("in lock\n");

        sf_debug("unlock\n");
        sf_spinlock_unlock(test_lock);
    }

    
    return 0;
}

