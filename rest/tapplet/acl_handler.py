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

from tapplet.acl.acl_basic import *
from tapplet.acl.tuple_tools import *

import re
import socket
import os 
import _thread


def check_group_index(index):
    
    if index != None:
        for i in index:
            if int(i) > get_max_group_id() or int(i) <= 0 :
                return True
    return False

def check_rule_index(index):
    
    if index != None:
        for i in index:
            if int(i) > get_max_rule_id() or int(i) <= 0 :
                return True
    return False

   
def decode_group_id(group_name):
    if group_name == None:
        return None
    ret = re.findall(r"group_(\d+)", group_name)
    if len(ret) > 0:
        return int(ret[0])
    return None

def decode_acl_var_name(varname):
    if varname == None:
        return None , None
    
    ret = re.findall(r"group_(\d+)/(\d+)", varname)
    if len(ret) > 0:
        return [int(ret[0][0])] , [int(ret[0][1])]
    ret = re.findall(r"group_(\d+)", varname)
    if len(ret) > 0:
        return [int(ret[0])] , None
    return None , None 
##################################################################
def get_acl_group():
    
    final_dict =  {}
    for i in range(1 , get_max_group_id() + 1):
        group_name = "group_{0}".format(i)
        group_value = "/rest/v1/acl/config/"+group_name
        final_dict.update( { group_name : group_value })

    return Httplib.OK , final_dict

def get_single_acl_group_info(group_id , rule_index , keys ):
    
    group_name = "group_{0}".format(group_id)
    final_dict = {}

    rule_query_cnt = 0

    if grp_key_rules in keys:
        if rule_index == None:
            for tmp_index in range(1 , get_max_rule_id() + 1):
                ret = chech_old_rule_existence(group_id , tmp_index)
                if ret == Httplib.OK:
                    final_dict.update( {  str(tmp_index)  : {}  } )
                    rule_query_cnt  = rule_query_cnt + 1
                    if rule_query_cnt > max_acl_rule_query_per_grp_simple:
                        return Httplib.REQUEST_ENTITY_TOO_LARGE , None
                elif ret == Httplib.INTERNAL_SERVER_ERROR:
                    return Httplib.INTERNAL_SERVER_ERROR , None
        else:
            for tmp_index in rule_index:
                ret = chech_old_rule_existence(group_id , tmp_index)
                if ret == Httplib.OK:
                    rule_str = "/rest/v1/acl/config/group_{0}/{1}".format(group_id , tmp_index )
                    final_dict.update( {  str(tmp_index)  : rule_str  } )
                    rule_query_cnt  = rule_query_cnt + 1
                    if rule_query_cnt > max_acl_rule_query_per_grp_simple:
                        return Httplib.REQUEST_ENTITY_TOO_LARGE , None
                elif ret == Httplib.INTERNAL_SERVER_ERROR:
                    return Httplib.INTERNAL_SERVER_ERROR , None
    

    return Httplib.OK , { group_name : final_dict }
    
    


def get_acl_group_info(groups , rule_index , keys):
    
    if groups == None:
        groups = []
        for i in range(1 , get_max_group_id() + 1):
            groups.append(i)

    final_dict = {}

    for tmpgrp in groups:
        ret_status , group_dict  = get_single_acl_group_info(tmpgrp , rule_index , keys)
        if ret_status != Httplib.OK:
            return ret_status , None
        
        final_dict.update(group_dict)

    return Httplib.OK , final_dict


def get_single_acl_config(group , rule , is_rule_query):
    rule_type = so.acl_get_rule_map_type(int(group) , int(rule))
    if rule_type == -1:
        app_log.warning("acl get rule type fail group ({0}) rule id ({1})....".format(group , rule))
        return Httplib.INTERNAL_SERVER_ERROR , None
    
    if rule_type not in rule_type_map.keys():
        if is_rule_query:
            return Httplib.OK , None
        else:
            return Httplib.NOT_FOUND , None
    
    if rule_type == rule_type_map["tuple4"]:
        return get_single_tuple_config(int(group) , int(rule) , rule_type_map["tuple4"])
    
    if rule_type == rule_type_map["tuple6"]:
        return get_single_tuple_config(int(group) , int(rule) , rule_type_map["tuple6"])

    return Httplib.OK , None


def get_acl_config(groups , rule_index , keys , depth , is_rule_query):
    if depth == 0:
        return get_acl_group()
    
    if depth == 1:
        return get_acl_group_info(groups , rule_index , keys)
    
    if groups == None:
        groups = []
        for i in range(1 , get_max_group_id() + 1):
            groups.append(i)
    
    
    total_rules = 0
    final_dict = {}
    for tmp_group in groups:
        group_name = "group_{0}".format(tmp_group)
        group_dict = {}

        if grp_key_rules in keys:
            if rule_index == None:
                for tmp_rule in range(1 , get_max_rule_id() + 1):
                    ret_status , ret_dict = get_single_acl_config(tmp_group , tmp_rule , is_rule_query)
                    if ret_status != Httplib.OK:
                        return ret_status , None
                    if ret_dict == None:
                        continue
                    group_dict.update({ str(tmp_rule) : ret_dict })
                    total_rules  = total_rules + 1
                    if total_rules > max_acl_rule_query_full:
                        return Httplib.REQUEST_ENTITY_TOO_LARGE , None
            else:
                for tmp_rule in rule_index:
                    ret_status , ret_dict = get_single_acl_config(tmp_group , tmp_rule , is_rule_query)
                    if ret_status != Httplib.OK:
                        return ret_status , None
                    if ret_dict == None:
                        continue
                    group_dict.update({ str(tmp_rule) : ret_dict })
                    total_rules  = total_rules + 1
                    if total_rules > max_acl_rule_query_full:
                        return Httplib.REQUEST_ENTITY_TOO_LARGE , None
    
        final_dict.update({group_name : group_dict})
    

    return Httplib.OK , final_dict


def update_single_acl_config(group_id , data , is_post  , group_result_dict , atomic_op = False):
    
    for rule in data.keys():
        acl_debug("update_single_acl_config rule : {0}".format(rule))

        tmpdict = data[rule]

        group_result_dict.update({rule : None})


        if rule.isdecimal() == False:
            group_result_dict.update({rule : 
                {"reason":"Invalid rule id"}
                })
            group_result_dict["has_fail"] = True             
            if atomic_op :
                break
            continue
            
        if int(rule) <= 0 or int(rule) > get_max_rule_id():
            group_result_dict.update({rule : 
                {"reason":"Invalid rule id"}
                })
            group_result_dict["has_fail"] = True             
            if atomic_op :
                break
            continue

        tmpdict = data[rule]
        if isinstance(tmpdict , dict) != True:
            group_result_dict.update({rule : 
                {"reason":"Invalid rule data"}
                })
            group_result_dict["has_fail"] = True             
            if atomic_op :
                break
            continue

        # if tmpdict.has_key("rule_type") != True:
        if "rule_type" not in tmpdict:
            group_result_dict.update({rule : 
                {"reason":"No rule_type"}
                })
            group_result_dict["has_fail"] = True             
            if atomic_op :
                break
            continue

        if tmpdict["rule_type"] not in rule_type_map.keys():
            group_result_dict.update({rule : 
                {"reason":"Invalid rule_type"}
                })
            group_result_dict["has_fail"] = True             
            if atomic_op :
                break
            continue
        
        # if tmpdict.has_key("rule_cfg") != True:
        if "rule_cfg" not in tmpdict:
            group_result_dict.update({rule : 
                {"reason":"No rule_cfg"}
                })
            group_result_dict["has_fail"] = True             
            if atomic_op :
                break
            continue

        ret_error = "?what?"
        if rule_type_map[tmpdict["rule_type"]] == rule_type_map["tuple4"]:
            ret_error = update_single_tuple_config(group_id , int(rule) , tmpdict["rule_cfg"] , 0 , is_post)

        elif rule_type_map[tmpdict["rule_type"]] == rule_type_map["tuple6"]:
            ret_error = update_single_tuple_config(group_id , int(rule) , tmpdict["rule_cfg"] , 1 ,  is_post)

        if ret_error != None:
            if  isinstance(ret_error, dict):
                group_result_dict.update({rule : ret_error })
            elif isinstance(ret_error, str):
                group_result_dict.update({rule : {"reason" : ret_error} })
                
            group_result_dict["has_fail"] = True             
            if atomic_op :
                break
            continue    

def update_acl_config(data , is_post , atomic_op = False):
    acl_debug("is post : {0} atomic op : {1}".format(is_post , atomic_op))

    if is_post == 0 and  atomic_op:
        app_log.error("ACL PUT : cannot use atomic_op")
        return None

    result_dict = {"has_fail" : False}
    for group in data.keys():
        acl_debug("group :{0}".format(group))

        group_id = decode_group_id(group)
        if group_id == None:
            result_dict.update({group : "invalid group"})
            result_dict["has_fail"] = True
            if atomic_op:
                break
            continue

        if check_group_index([group_id]):
            result_dict.update({group : "invalid group id"})
            result_dict["has_fail"] = True
            if atomic_op:
                break
            continue

        group_result_dict = {"has_fail" : False}

        update_single_acl_config(group_id , data[group] ,  is_post, group_result_dict ,  atomic_op )

        result_dict.update( {group: group_result_dict} )
        if group_result_dict["has_fail"]:
            result_dict["has_fail"] = True
            if atomic_op:
                break

    if result_dict["has_fail"]:
        # delete all  created rule
        result_dict_keys = list(result_dict.keys())
        for group in result_dict_keys:
            if isinstance(result_dict[group] , dict) == False:
                continue
            
            group_id = decode_group_id(group)

            group_keys = list(result_dict[group].keys())
            for rule in group_keys:
                if result_dict[group][rule] != None:
                    continue
                if atomic_op:
                    result = so.acl_delete_old_map(int(group_id) , int(rule)  , 0)
                    acl_debug("delete  rule :{0} , {1} ret {2}".format(group_id , rule , result))
                    if result != 0:
                        app_log.error("delete fail in update_acl_config :{0} , {1}".format(group_id , rule))

                del result_dict[group][rule]
            
            if result_dict[group]["has_fail"] == False:
                del result_dict[group]   
            else:
                del result_dict[group]["has_fail"]
         
    
    if result_dict["has_fail"]:
        del result_dict["has_fail"]
        return Httplib.BAD_REQUEST , result_dict 
    else:
        del result_dict
        return Httplib.OK , None

############################

def delete_single_acl_config(group , rule):

    result = so.acl_delete_old_map(int(group) , int(rule)  , 0)
    if result == 0:
        return Httplib.OK
    if result == -2:
        # return Httplib.NOT_FOUND
        return Httplib.OK
    
    app_log.warning("delete single map error g({0}) rule id ({1})....".format(group , rule))
    return Httplib.INTERNAL_SERVER_ERROR

def delete_acl_config(group_list , rule_list):
    
    if rule_list != None:
        for group in group_list :
            for rule in rule_list:
                result = delete_single_acl_config(int(group) , int(rule))
                if result != Httplib.OK:
                    return result
    else:
        for group in group_list :
            for rule in range(1, get_max_rule_id() + 1):
                result = delete_single_acl_config(int(group) , int(rule))
                if result != Httplib.OK:
                    return result

    return Httplib.NO_CONTENT
##################################################################


class AclConfigHandler(BaseHandler):
    
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run AclConfigHandler GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.index = get_index_arg(self.request.query_arguments)
            self.group = get_group_arg(self.request.query_arguments)
            self.keys = get_keys_arg(self.request.query_arguments)
            self.depth = get_depth_param(self.request.query_arguments)
            self.type = get_type_arg(self.request.query_arguments)
            query_args = self.request.query_arguments

            
            self.keys = [ grp_key_rules]

            # /acl/config/group_1/1 : is_rule_query False
            # /acl/config?type=query : is_rule_query True
            # /acl/config?type=specified : is_rule_query False
            is_rule_query = False
            if self.type != None:
                if self.type == "query":
                    is_rule_query = True
                elif self.type == "specified":
                    is_rule_query = False
                else:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.write("Invalid query type")
                    self.finish()
            


            #/acl/config/xx
            if var_name != None:
                self.group , self.index = decode_acl_var_name(var_name)
                if self.group == None :
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
                if self.depth < 1:
                    self.depth = 1
                if self.index == None :
                    self.index = get_index_arg(self.request.query_arguments)
                else:
                    self.depth = 2
                    is_rule_query = False

            if self.group:
                if check_group_index(self.group):
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
            if self.index:
                if check_rule_index(self.index):
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
            


            ret_status , ret_dict = get_acl_config(self.group , self.index , self.keys , self.depth , is_rule_query)

            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.write(get_acl_error_message())
                self.finish()
                return 

            self.write(to_json(ret_dict))
            self.set_status(Httplib.OK)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

    @gen.coroutine
    def put(self, var_name = None):
        app_log.debug("Run AclConfigHandler put....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            request = self.request.body
            request = json.loads(request.decode('utf-8'))

            # update config
            ret_status , result_dict = update_acl_config(request , 0 , False)

            self.set_status(ret_status)
            if ret_status != Httplib.OK:
                self.write(to_json(result_dict))
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
    
    @gen.coroutine
    def post(self, var_name = None):
        app_log.debug("Run AclConfigHandler put....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:            
            request = self.request.body
            request = json.loads(request.decode('utf-8'))
            self.type = get_type_arg(self.request.query_arguments)

            atomic_op = False

            if self.type != None:
                if self.type == "atomic":
                    atomic_op = True
                else:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.write("Unknown type")
                    self.finish()
                    return 


            ret_status,result_dict = update_acl_config(request , 1 , atomic_op)
            if ret_status == Httplib.OK:
                ret_status = Httplib.CREATED
            self.set_status(ret_status)
            if ret_status != Httplib.CREATED:
                self.write( to_json(result_dict) )
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
    
    @gen.coroutine
    def delete(self, var_name = None):

        app_log.debug("Run AclConfigHandler DELETE....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.index = get_index_arg(self.request.query_arguments)
            self.group = get_group_arg(self.request.query_arguments)
            query_args = self.request.query_arguments

            #print(var_name)

            app_log.debug("var_name:{0}".format(var_name))

            #/acl/config/xx
            if var_name != None:
                self.group , self.index = decode_acl_var_name(var_name)
                if self.group == None :
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
                if self.index == None :
                    self.index = get_index_arg(self.request.query_arguments)

            if self.group:
                if check_group_index(self.group):
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
            if self.index:
                if check_rule_index(self.index):
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return

            if self.group == None:
                set_acl_error_message("Invalid group")
                self.set_status(Httplib.BAD_REQUEST)
                self.write(get_acl_error_message())
                self.finish()
                return                

            ret_status = delete_acl_config(self.group  , self.index )
            self.set_status(ret_status)
            if ret_status != Httplib.NO_CONTENT:
                self.write(get_acl_error_message())
            self.finish()

            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()





class AclConfigModelHandler(BaseHandler):
    
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run AclConfigModelHandler GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:

            tuple_rule_cfg = TupleRuleCfg()

            ret_dict = trans_tuple_cfg_to_dict(tuple_rule_cfg)
            
            rule_tuple4 = {"rule_type" : "tuple4" , "rule_cfg" : ret_dict}
            rule_tuple6 = {"rule_type" : "tuple6" , "rule_cfg" : ret_dict}


            l2_rule_cfg =L2RuleCfg()

            ret_dict = trans_l2_rule_cfg_to_dict(l2_rule_cfg)
            
            rule_l2 = {"rule_type" : "l2" , "rule_cfg" : ret_dict}

            result_dict = {"group_1" : { "1" : rule_tuple4  , "2" : rule_tuple6 , "3" : rule_l2 } }

            self.write(to_json(result_dict))
            self.set_status(Httplib.OK)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

################################
#
#            stat  
#
################################
def get_single_acl_stat( group_id, rule_id , keys):
    result = so.acl_get_rule_map_type(group_id , rule_id )
    if result == -1:
        app_log.warning("fail to call acl_get_rule_map_type")
        return Httplib.INTERNAL_SERVER_ERROR , None
    if result == 0 :
        return Httplib.OK, None
    if rule_type_map[result] not in keys:
        return Httplib.OK , None

    command = "sf acl sync stat group {0} rule {1}".format(group_id , rule_id)
    ret = send2vpp(command)
    app_log.debug("send2vpp : #{0}# ret :{1}".format(command , str(ret)))

    so.acl_get_rule_hit.restypeX = c_ulong
    result = so.acl_get_rule_hit(group_id , rule_id)
    if result == -1:
        app_log.warning("fail to call acl_get_rule_hit")
        return Httplib.INTERNAL_SERVER_ERROR , None
    
    return Httplib.OK , {str(rule_id) : result}

def get_acl_stat(groups , rule_indexs , keys):
    
    #keys
    if keys != None:
        for tempkey in keys:
            if tempkey not in rule_type_map.keys():
                return Httplib.BAD_REQUEST , None
    else:
        keys = list(rule_type_map.keys())
    #group
    if groups == None:
        groups = []
        for i in range(1 , get_max_group_id() + 1):
            groups.append(i)

    final_dict = {}
    rules_count = 0
    for tmpgroup in groups:
        group_str = "group_{0}".format(tmpgroup)
        group_dict = {}
        if rule_indexs == None:
            for tmp_rule_id in range(1 , get_max_rule_id() + 1):
                result, retdict = get_single_acl_stat(tmpgroup , tmp_rule_id , keys)
                if result == Httplib.INTERNAL_SERVER_ERROR:
                    return Httplib.INTERNAL_SERVER_ERROR , None
                if retdict != None:
                    group_dict.update(retdict)
                    rules_count = rules_count + 1
                    if rules_count > max_acl_stat_query:
                        return Httplib.REQUEST_ENTITY_TOO_LARGE , None
        else:
            for tmp_rule_id in rule_indexs:
                result, retdict = get_single_acl_stat(tmpgroup , tmp_rule_id , keys)
                if result == Httplib.INTERNAL_SERVER_ERROR:
                    return Httplib.INTERNAL_SERVER_ERROR , None
                if retdict != None:
                    group_dict.update(retdict)
                    rules_count = rules_count + 1
                    if rules_count > max_acl_stat_query:
                        return Httplib.REQUEST_ENTITY_TOO_LARGE , None

        final_dict.update({ group_str : group_dict })

    return Httplib.OK , final_dict
        
            

class AclStatHandler(BaseHandler):
    @gen.coroutine
    def get(self , var_name = None):
        app_log.debug("Run AclStatHandler get....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:            
            self.index = get_index_arg(self.request.query_arguments)
            self.group = get_group_arg(self.request.query_arguments)
            self.keys = get_keys_arg(self.request.query_arguments)
            query_args = self.request.query_arguments

            if var_name != None:
                self.group , self.index = decode_acl_var_name(var_name)
                if self.group == None :
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
                if self.index == None :
                    self.index = get_index_arg(self.request.query_arguments)
            
            if self.group:
                if check_group_index(self.group):
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
            if self.index:
                if check_rule_index(self.index):
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
            ret_status , ret_dict = get_acl_stat(self.group , self.index ,  self.keys)
            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return 

            self.write(to_json(ret_dict))
            self.set_status(Httplib.OK)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
    
    @gen.coroutine
    def patch(self, var_name = None):
        app_log.debug("Run AclStatHandler patch....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            command = "sf acl clear stat all"
            ret = send2vpp(command)
            app_log.debug("send2vpp : #{0}# ret :{1}".format(command , str(ret)))

            result  = so.acl_clear_rule_hit_all()
            if result == -1:
                app_log.warning("fail to call acl_clear_rule_hit_all")
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return

            self.set_status(Httplib.OK)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
################################
#
#            sync  
#
################################

def acl_rule_sync():
    try:
        result = tuple_rule_sync()
        acl_debug("== tuple_rule_sync finish : {0}===".format(result))
        if result == False:
            raise Exception("tuple sync error")

        result = so.acl_set_sync_flag_need()
        if result != 0:
            app_log.warning("set sync flag error")
            raise Exception("set sync flag error")
            
    except Exception as e:
        result = so.acl_set_sync_flag_fail()
        if result != 0:
            app_log.warning("set sync flag fail error")

class AclSyncHandler(BaseHandler):
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run AclSyncHandler get....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            sync_status = so.acl_get_sync_flag()
            if sync_status == -1:
                app_log.warning("fail to call acl_set_sync_flag")
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return
            
            sync_times = so.acl_get_sync_times()
            if sync_times == -1:
                app_log.warning("fail to call acl_get_sync_times")
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return
            
            result_dict = {"sync_status" : sync_status_map[sync_status ] , "sync_times" : sync_times}
            self.write(to_json(result_dict))
            self.set_status(Httplib.OK)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
    
    @gen.coroutine
    def patch(self, var_name = None):
        app_log.debug("Run AclSyncHandler patch....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            sync_status = so.acl_get_sync_flag()
            if sync_status == -1:
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return
            if sync_status > 0: 
                self.set_status(Httplib.SERVICE_UNAVAILABLE)
                self.write("still sync")
                self.finish()
                return
            
            ret =  so.acl_set_sync_flag_prepare()
            if ret != 0:
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return
        
            # () : no params
            _thread.start_new_thread(acl_rule_sync , ())

            self.set_status(Httplib.NO_CONTENT)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

class AclHandler(BaseHandler):
    
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run AclHandler GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.depth = get_depth_param(self.request.query_arguments)
            if isinstance(self.depth, dict):
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()
                return
            
            if self.depth <= 1:
                tmpdict = {"config":"/rest/v1/acl/config" , "stat":"/rest/v1/acl/stat" , "sync":"/rest/v1/acl/sync"}
                self.write(to_json(tmpdict))
                self.finish()
                return
        
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
            
                
            
acl_urls = [
        (r"/rest/v1/acl", AclHandler),
        (r"/rest/v1/acl/config/model", AclConfigModelHandler),
        (r"/rest/v1/acl/config/(.*)", AclConfigHandler),
        (r"/rest/v1/acl/config", AclConfigHandler),
        (r"/rest/v1/acl/sync", AclSyncHandler),
        (r"/rest/v1/acl/stat/(.*)", AclStatHandler),
        (r"/rest/v1/acl/stat", AclStatHandler),
        ]
