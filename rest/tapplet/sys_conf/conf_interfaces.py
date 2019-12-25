from . sys_conf_basic import *

from tapplet.interfaces_map import *
from tapplet.interface_handler import get_interfaces_config , update_interface_config


json_file_name = "conf_intf.json"


class IntfConfTrans(SysConfBasicTrans):
    def __init__(self):
        SysConfBasicTrans.__init__(self)

    def load_cfg(self , sub_conf_dir):
        self.logger.debug("load in IntfConfTrans: " + sub_conf_dir)

        if sys_conf_judge_file_exist(json_file_name , dir_path=sub_conf_dir) == False:
            self.logger.error("[conf_intf] fail to load config, file not exist")
            return SYS_CONF_TRANS_FAIL

        dict_from_file = try_json_file_input(json_file_name, dir_path=sub_conf_dir)
        if dict_from_file == None:
            self.logger.error("[conf_intf] fail to read file , maybe json not right")
            return SYS_CONF_TRANS_FAIL

        ret = SYS_CONF_TRANS_SUCCESS

        ret_staus = update_interface_config(dict_from_file)

        if ret_staus != Httplib.OK:
            ret = SYS_CONF_TRANS_FAIL

        return ret

    def save_cfg(self , sub_conf_dir):
        self.logger.debug("save in IntfConfTrans: " + sub_conf_dir)

        ret_status , ret_dict = get_interfaces_config(None , None)

        if ret_status != Httplib.OK:
            return SYS_CONF_TRANS_FAIL
        
        sys_conf_save_to_file(ret_dict , json_file_name , sub_conf_dir)

        return SYS_CONF_TRANS_SUCCESS

