from . sys_conf_basic import *


class SystemConfTrans(SysConfBasicTrans):
    def __init__(self):
        SysConfBasicTrans.__init__(self)

    def load_cfg(self , conf_dir):
        self.logger.info("load in SystemConfTrans: " +conf_dir)
        return SYS_CONF_TRANS_SUCCESS

    def save_cfg(self , conf_dir):
        self.logger.info("save in SystemConfTrans: " +conf_dir)
        return SYS_CONF_TRANS_SUCCESS