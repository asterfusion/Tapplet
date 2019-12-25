#include <vnet/plugin/plugin.h>

#define SF_PLUGIN_BUILD_VER "1.0"

VLIB_PLUGIN_REGISTER () = {
    .version = SF_PLUGIN_BUILD_VER,
    .description = "SF action plugin",
};
