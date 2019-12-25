from . sys_conf_basic import *

from tapplet.acl_handler import get_single_acl_config , update_acl_config , get_max_group_id
from tapplet.nshellutils import so
from ctypes import *
import os

max_rules_cnt_per_file = 32
rule_file_name_format = "acl_rules_{0}.json"


class AclConfTrans(SysConfBasicTrans):
    def __init__(self):
        SysConfBasicTrans.__init__(self)


    def load_cfg(self , sub_conf_dir):
        self.logger.debug("load in AclConfTrans: " + sub_conf_dir)

        error_happened = False
        file_name_list = os.listdir(sub_conf_dir)

        for rulefile in file_name_list:
            dict_from_file = try_json_file_input(rulefile, dir_path=sub_conf_dir)
            if dict_from_file == None:
                self.logger.error("[conf_acl] fail to read {0}, maybe json not right".format(rulefile))
                error_happened = True

            #use PUT to update rules , old rule may be deleted
            ret_status , result_dict = update_acl_config(dict_from_file , 0 , False)
            if ret_status !=  Httplib.OK :
                error_happened = True
                for group in result_dict.keys():
                    for rule in result_dict[group].keys():
                        self.logger.error("[conf_acl] {0} r{1} fail: {2}".format(group , rule , result_dict[group][rule]))

        if error_happened == True:
            return SYS_CONF_TRANS_FAIL


        return SYS_CONF_TRANS_SUCCESS

    def save_cfg(self , sub_conf_dir):
        self.logger.debug("save in AclConfTrans: " + sub_conf_dir)

        max_group_id = get_max_group_id()
        file_name_index = 1
        tmp_rule_dict = {}
        pre_file_name = rule_file_name_format.format(file_name_index)

        for group_id in range(1,max_group_id+1):
            tmp_rule_dict.clear()

            ## find first one
            ret_rule_id = so.acl_sys_conf_find_next_rule(group_id , 0)
            if ret_rule_id == -1 :
                self.logger.error("something error when handling group {0}".format(group_id))
                return SYS_CONF_TRANS_FAIL
            
            ## get all 
            while ret_rule_id != 0:
                ret_status , rule_dict = get_single_acl_config(group_id , ret_rule_id , False)
                if ret_status != Httplib.OK:
                    self.logger.error("get group {0} rule {1} fail".format(group_id , ret_rule_id))
                    ret_rule_id = so.acl_sys_conf_find_next_rule(group_id , ret_rule_id)
                    continue
                tmp_rule_dict.update({str(ret_rule_id) : rule_dict})
                
                ## get next rule id
                ret_rule_id = so.acl_sys_conf_find_next_rule(group_id , ret_rule_id)

                if len(tmp_rule_dict.keys()) >= max_rules_cnt_per_file:
                    final_dict = { "group_{0}".format(group_id)   :  tmp_rule_dict }
                    sys_conf_save_to_file(final_dict , pre_file_name , sub_conf_dir)

                    file_name_index +=  1
                    pre_file_name = rule_file_name_format.format(file_name_index)
                    tmp_rule_dict.clear()
            
            ## write rest to file
            if len(tmp_rule_dict.keys()) > 0 :
                final_dict = { "group_{0}".format(group_id)   :  tmp_rule_dict }
                sys_conf_save_to_file(final_dict , pre_file_name , sub_conf_dir)
                file_name_index +=  1
                pre_file_name = rule_file_name_format.format(file_name_index)


        return SYS_CONF_TRANS_SUCCESS
