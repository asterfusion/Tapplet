import os
import time
from . sys_conf_map import *
from . sys_conf_basic import *
from tapplet.nshellutils import *


sys_conf_root_dir =  "/tmp/sys_conf/"

def sys_conf_save_all(modules):
    error_happend = False
    # get logger
    logger = sys_conf_get_logger()

    # update status
    sys_conf_sync_status = get_sys_conf_sync_status()
    sys_conf_sync_status["status"] = "Busy"
    sys_conf_sync_status["laststatus"]["op"] = "save"
    sys_conf_sync_status["laststatus"]["time"] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) 

    if modules == None:
        modules = list(sys_conf_map.keys())

    try:


        logger.info("##### [start] save all module #####")


        # clean / create sys_conf dir
        sys_conf_prepare_subdir(sys_conf_root_dir)


        # call all module save funcs
        for module in modules:
            if module not in sys_conf_map.keys():
                logger.error("@@@ Unknown module : {0}".format(module))
                continue

            logger.info(" [{0}] -- [start]".format(module))
            
            tempTrans = sys_conf_map[module]()
            tempDir = os.path.join(sys_conf_root_dir , module)

            sys_conf_prepare_subdir(tempDir)
            ret = tempTrans.save_cfg(tempDir)

            if ret != SYS_CONF_TRANS_SUCCESS:
                error_happend = True
                # remove sub dir
                sys_conf_remove_subdir(tempDir)

            logger.info("   [{0}] -- [finish] ret {1}".format(module , ret))

    except Exception as err:
        logger.error(str(err))
        error_happend = True
    finally:
        logger.info("##### [finish] save all module #####")

        sys_conf_sync_status["status"] = "Idle"
        sys_conf_sync_status["laststatus"]["error"] = error_happend        

        return error_happend



def sys_conf_load_all(target_dir = None):
    '''
    target_dir may be sys_conf_root_dir or /tmp
    '''

    if target_dir == None:
        target_dir = sys_conf_root_dir

    error_happend = False
    # get logger
    logger = sys_conf_get_logger()

    # update status
    sys_conf_sync_status = get_sys_conf_sync_status()
    sys_conf_sync_status["status"] = "Busy"
    sys_conf_sync_status["laststatus"]["op"] = "load"
    sys_conf_sync_status["laststatus"]["time"] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()) 

    try:

        logger.info("##### [start] load all module #####")
        

        # get all subdir
        dirlist = os.listdir(target_dir)


         # call all module load funcs
        for module in dirlist:

            if str(module) not in sys_conf_map.keys():
                logger.warning("@@@ unknown module : {0}".format(module))
                continue

            logger.info(" [{0}] -- [start]".format(module))

            
            tempTrans = sys_conf_map[module]()
            tempDir = target_dir + module

            ret = tempTrans.load_cfg(tempDir)
            if ret != SYS_CONF_TRANS_SUCCESS:
                error_happend = True

            logger.info("   [{0}] -- [finish] ret {1}".format(module , ret))

    except Exception as err:
        logger.error(str(err))
        error_happend = True
    finally:
        logger.info("##### [finish] load all module #####")

        sys_conf_sync_status["status"] = "Idle"
        sys_conf_sync_status["laststatus"]["error"] = error_happend

        return error_happend
 