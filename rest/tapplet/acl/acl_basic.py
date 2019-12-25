from tornado.log import app_log

from tapplet.nshellutils import *
from conf.globalValue import *
from ctypes import *
mod_name = "acl"

max_group_id = so.acl_group_cnt()
max_rule_id = so.acl_rule_cnt_per_group()

def get_max_group_id():
    global max_group_id
    return max_group_id

def get_max_rule_id():
    global max_rule_id
    return max_rule_id

#static
max_ip_str_len= 46

rule_type_map = {
    1 : "tuple4" , "tuple4" : 1 , 
    2 : "tuple6" , "tuple6" : 2 , 
}
ACL_STILL_SYNC = 2

sync_status_map = {
    3 : "still sync",
    2 : "wait sync",
    1 : "prepare sync",
    0 : "sync ok" , 
    -2 : "sync failed"
}

# rule kind
rule_kind_num = 1
rule_kind_map = {
    0 : "tuple" , "tuple" : 0,
}

grp_key_rules = "rules"

max_acl_rule_query_per_grp_simple = 50
max_acl_rule_query_full = 30
max_acl_stat_query = 100


acl_global_error_message = ""
def get_acl_error_message():
    global acl_global_error_message
    temp = acl_global_error_message
    acl_global_error_message = ""
    return temp

def set_acl_error_message(msg):
    global acl_global_error_message
    acl_global_error_message = msg


def chech_old_rule_existence(group_id , outer_rule_id):
    result = so.acl_get_rule_map_type(group_id , outer_rule_id )
    if result == -1:
        app_log.warning("acl get rule type fail group ({0}) rule id ({1})....".format(group_id , outer_rule_id))
        return Httplib.INTERNAL_SERVER_ERROR
    if result == 0:
        return Httplib.NOT_FOUND
    return Httplib.OK


def acl_ret_error_to_str(result):
    temp_str = create_string_buffer(64)
    so.acl_new_rule_error_to_str(result , temp_str , 64)
    return str(temp_str.value , encoding="utf-8")


acl_debug_enable = False
def acl_debug(msg):
    if acl_debug_enable:
        print(msg)
