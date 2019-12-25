from . sys_conf_basic import *

from tapplet.actions.action_handler import get_action
from tapplet.actions.action_utils import update_action_content


json_file_name = "conf_actions.json"


class ActionsConfTrans(SysConfBasicTrans):
    def __init__(self):
        SysConfBasicTrans.__init__(self)

    def load_cfg(self , sub_conf_dir):
        self.logger.debug("load in dir: " + sub_conf_dir)

        if sys_conf_judge_file_exist(json_file_name , dir_path=sub_conf_dir) == False:
            self.logger.error("[conf_actions] fail to load config, file not exist")
            return SYS_CONF_TRANS_FAIL

        dict_from_file = try_json_file_input(json_file_name, dir_path=sub_conf_dir)
        if dict_from_file == None:
            self.logger.error("[conf_actions] fail to read file , maybe json not right")
            return SYS_CONF_TRANS_FAIL

        ret = SYS_CONF_TRANS_SUCCESS

        
        ret_dict = update_action_content("put" , dict_from_file)

        if ret_dict != None:
            ret = SYS_CONF_TRANS_FAIL
            for index in ret_dict.keys():
                self.logger.error("[conf_actions] action '{0}' : {1}".format(index , ret_dict[index]))

        return ret

    def save_cfg(self , sub_conf_dir):
        self.logger.debug("save in dir: " + sub_conf_dir)

        ret_dict = get_action()

        if ret_dict == None:
            self.logger.info("ret_dict is None")

        sys_conf_save_to_file(ret_dict , json_file_name , sub_conf_dir)

        return SYS_CONF_TRANS_SUCCESS

