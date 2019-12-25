import tornado.web
from tornado import gen
from tornado.log import app_log
from conf.globalValue import *
from utils.tools import *
from utils.apiException import *
from conf.setting import setting
from basic_handler.base_handler import BaseHandler

from tapplet.nshellutils import *
from ctypes import *

from utils.net_tools import *

from tapplet.acl.acl_basic import *

import re
import socket
import os 
import _thread


class TupleRuleCfg(Structure):
    _fields_ = [
                ("is_ipv6", c_ubyte),
                ("sport_min", c_ushort),
                ("sport_max", c_ushort),
                ("dport_min", c_ushort),
                ("dport_max", c_ushort),
                ("proto_min", c_ubyte),
                ("proto_max", c_ubyte),
                ("sip", c_char*max_ip_str_len),
                ("sip_mask", c_ubyte),
                ("dip", c_char*max_ip_str_len),
                ("dip_mask", c_ubyte),
            ]

##################################################################

def trans_tuple_cfg_to_dict(tuple_rule_cfg):
    tmpdict = {}
    tmpdict.update({"sport_min" : tuple_rule_cfg.sport_min})
    tmpdict.update({"sport_max" : tuple_rule_cfg.sport_max})
    tmpdict.update({"dport_min" : tuple_rule_cfg.dport_min})
    tmpdict.update({"dport_max" : tuple_rule_cfg.dport_max})
    tmpdict.update({"proto_min" : tuple_rule_cfg.proto_min})
    tmpdict.update({"proto_max" : tuple_rule_cfg.proto_max})
    tmpdict.update({"sip_mask" : tuple_rule_cfg.sip_mask})
    tmpdict.update({"dip_mask" : tuple_rule_cfg.dip_mask})
    sip_str = bytes(tuple_rule_cfg.sip).decode("utf-8")
    dip_str = bytes(tuple_rule_cfg.dip).decode("utf-8")
    tmpdict.update({"sip" : sip_str})
    tmpdict.update({"dip" : dip_str})

    return tmpdict


def trans_dict_to_tuple_rule_cfg(rule_cfg_dict):
    tuple_rule_cfg = TupleRuleCfg()
    if "sport_min" in rule_cfg_dict.keys():
        tuple_rule_cfg.sport_min = rule_cfg_dict["sport_min"]
    if "sport_max" in rule_cfg_dict.keys():
        tuple_rule_cfg.sport_max = rule_cfg_dict["sport_max"]
    if "dport_min" in rule_cfg_dict.keys():
        tuple_rule_cfg.dport_min = rule_cfg_dict["dport_min"]
    if "dport_max" in rule_cfg_dict.keys():
        tuple_rule_cfg.dport_max = rule_cfg_dict["dport_max"]
    if "proto_min" in rule_cfg_dict.keys():
        tuple_rule_cfg.proto_min = rule_cfg_dict["proto_min"]
    if "proto_max" in rule_cfg_dict.keys():
        tuple_rule_cfg.proto_max = rule_cfg_dict["proto_max"]
    if "sip_mask" in rule_cfg_dict.keys():
        tuple_rule_cfg.sip_mask = rule_cfg_dict["sip_mask"]
    if "dip_mask" in rule_cfg_dict.keys():
        tuple_rule_cfg.dip_mask = rule_cfg_dict["dip_mask"]
    new_sip_is_ipv6 = 0
    new_sip_flag = 0
    new_dip_is_ipv6 = 0
    new_dip_flag = 0
    if "sip" in rule_cfg_dict.keys():
        tuple_rule_cfg.sip = bytes(rule_cfg_dict["sip"] , "utf-8")
        new_sip_is_ipv6 = check_ip_string(rule_cfg_dict["sip"])
        new_sip_flag = 1
    if "dip" in rule_cfg_dict.keys():
        tuple_rule_cfg.dip = bytes(rule_cfg_dict["dip"] , "utf-8")
        new_dip_is_ipv6 = check_ip_string(rule_cfg_dict["sip"])
        new_dip_flag = 1
    # tuple_rule_cfg.is_ipv6 = 2 : sip version and dip version does not match
    if new_sip_flag == 1 and new_dip_flag == 1:
        tuple_rule_cfg.is_ipv6 = new_sip_is_ipv6
        if  new_sip_is_ipv6 != new_dip_is_ipv6:
            tuple_rule_cfg.is_ipv6 = 2
    elif  new_sip_flag == 1 and new_dip_flag == 0:
        if new_sip_is_ipv6 != tuple_rule_cfg.is_ipv6:
            tuple_rule_cfg.is_ipv6 = 2
    elif new_dip_is_ipv6 == 0 and new_dip_flag == 1:
        if new_sip_is_ipv6 != tuple_rule_cfg.is_ipv6:
            tuple_rule_cfg.is_ipv6 = 2

    return Httplib.OK ,  tuple_rule_cfg
        


def get_single_tuple_config(group , out_rule_id , rule_type):
    tuple_rule_cfg = TupleRuleCfg()
    result = so.get_tuple_rule_cfg(group , out_rule_id , byref(tuple_rule_cfg))
    if result != 0 :
        app_log.warning("get tuple_rule_cfg fail group ({0}) rule id ({1})....".format(group , out_rule_id))
        return Httplib.INTERNAL_SERVER_ERROR , None
    result_dict = trans_tuple_cfg_to_dict(tuple_rule_cfg)
    return Httplib.OK , { "rule_type" : rule_type_map[rule_type] , "rule_cfg" : result_dict}




def tuple_chech_old_rule_existence(group_id , outer_rule_id):
    result = so.acl_get_rule_map_type(group_id , outer_rule_id )
    if result == -1:
        app_log.warning("acl get rule type fail group ({0}) rule id ({1})....".format(group_id , outer_rule_id))
        return Httplib.INTERNAL_SERVER_ERROR
    if result == 0:
        return Httplib.NOT_FOUND
    return Httplib.OK
    

def check_tuple_rule_config(group_id , outer_rule_id ,  data , is_ipv6):
    for tmpkey in data.keys():
        if sf_judge_key_in_struct(tmpkey , TupleRuleCfg) == False:
            return Httplib.BAD_REQUEST
    
    for tmpkey in TupleRuleCfg._fields_:
        if tmpkey[0] == "is_ipv6":
            continue
        if tmpkey[0] not in data.keys():
            return Httplib.BAD_REQUEST

    ret_status , tuple_rule_cfg = trans_dict_to_tuple_rule_cfg( data)
    # sip or dip version error
    if tuple_rule_cfg.is_ipv6 != is_ipv6:
        return Httplib.BAD_REQUEST
    
    result = so.check_tuple_rule_cfg(byref(tuple_rule_cfg))
    if result != 0:
        return Httplib.BAD_REQUEST 

    return Httplib.OK 


def update_single_tuple_config(group_id , outer_rule_id , data , is_ipv6 , check_old ):
    result = check_tuple_rule_config(group_id , outer_rule_id , data , is_ipv6 )

    if result != Httplib.OK:
        return "Invalid rule cfg"

    ret_status , tuple_rule_cfg =  trans_dict_to_tuple_rule_cfg( data)

    result = so.tuple_try_map_new_rule(int(group_id) , int(outer_rule_id) , byref(tuple_rule_cfg) , check_old)

    acl_debug("update_tuple_cfg : tuple try map : ret {0}".format(result))

    if result != 0:
        if result > 0:
            return {"reason" : acl_ret_error_to_str(result) , "detail" : result}
        return {"reason" : acl_ret_error_to_str(result) , "detail" : None}
    
    return None

################################
#
#            sync  
#
################################

def tuple_rule_sync():
    return True

                
