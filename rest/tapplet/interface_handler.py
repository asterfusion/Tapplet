import tornado.web
from tornado import gen
from tornado.log import app_log
from conf.globalValue import *
from utils.tools import *
from utils.apiException import *
from conf.setting import setting
from basic_handler.base_handler import BaseHandler

from tapplet.nshellutils import *
from tapplet.interfaces_map import *
from ctypes import *

## TODO ##
def get_max_rule_id():
    return 2048

mod_name = "interfaces"
max_vlan_id = 4095

interface_config_keys = [
    "admin_status",
    "port_id",
    "alias",
    "interface_type",
    "mac",
    "ipaddr_mask",
    "ipv4_addr",
    "ipv6_addr",
    "ipv4_gateway",
    "ipv6_gateway",
    "ingress_config",
    "ip_reass_config",
]

ingress_config_keys = [
    "acl_rule_group",
    "default_action_id",
    "rule_to_action",
]

ip_reass_config_keys = [
    "ip_reass_layers",
    "ip_reass_output_enable"
]



def get_single_interface_ip_reass_config(index , key , tmpdict):
    # ip_reass_layers
    if key == "ip_reass_layers":
        result = so.ip_reass_get_max_layer(int(index))
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"ip_reass_layers":result})
        return Httplib.OK , tmpdict
    
    # ip_reass_output_enable
    if key == "ip_reass_output_enable":
        result = so.ip_reass_get_reass_output(int(index))
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"ip_reass_output_enable":result})
        return Httplib.OK , tmpdict
    
    return Httplib.NOT_FOUND, None



def get_single_interface_ingress_config(index , key , tmpdict):
 
    # acl_rule_group
    if key == "acl_rule_group":
        result = so.get_ingress_acl_rule_group(int(index))
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"acl_rule_group":result})
        return Httplib.OK , tmpdict

    # default_action_id
    if key == "default_action_id":
        result = so.get_ingress_default_action_id(int(index))
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"default_action_id":result})
        return Httplib.OK , tmpdict

    # rule_to_action
    if key == "rule_to_action":
        tmp_rule_dict = {}
        for i in range(1 , get_max_rule_id() + 1):
            result = so.get_ingress_rule_to_action(int(index) , i)
            if result == -1:
                return Httplib.INTERNAL_SERVER_ERROR , None
            if result != 0:
                tmp_rule_dict.update({str(i) : result })
        tmpdict.update({"rule_to_action":tmp_rule_dict})
        return Httplib.OK , tmpdict

    return Httplib.NOT_FOUND, None

def get_single_interface_config(index , key , tmpdict):
    # admin_status
    if key == "admin_status":
        result = so.get_interface_admin_status(int(index))
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"admin_status":result})
        return Httplib.OK , tmpdict

    # port_id
    if key == "port_id":
        tmpdict.update({"port_id":int(index)})
        return Httplib.OK , tmpdict

    # alias_name
    if key == "alias":
        temp_str = create_string_buffer(24)
        result = so.get_interface_alias_name(int(index) , temp_str , 24)
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"alias":str(temp_str.value , encoding="utf-8")})
        return Httplib.OK , tmpdict
    
    # interface_type
    if key == "interface_type":
        result = so.get_interface_interface_type(int(index))
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        if result == 0:
            tmpdict.update({"interface_type":"normal"})
        elif result == 1:
            tmpdict.update({"interface_type":"loopback"})
        elif result == 2:
            tmpdict.update({"interface_type":"force_tx"})
        elif result == 3:
            tmpdict.update({"interface_type":"normal_and_check"})
        elif result == 4:
            tmpdict.update({"interface_type":"tunnel"})
        return Httplib.OK , tmpdict

    # mac
    if key == "mac":
        temp_str = create_string_buffer(20)
        result = so.get_interface_mac_addr(int(index) , temp_str , 20)
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"mac":str(temp_str.value , encoding="utf-8")})
        return Httplib.OK , tmpdict
    # ipv4_addr    
    if key == "ipv4_addr":
        temp_str = create_string_buffer(20)
        result = so.get_interface_ipv4(int(index) , temp_str , 20)
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"ipv4_addr":str(temp_str.value , encoding="utf-8")})
        return Httplib.OK , tmpdict
    # ipv6_addr    
    if key == "ipv6_addr":
        temp_str = create_string_buffer(40)
        result = so.get_interface_ipv6(int(index) , temp_str , 40)
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"ipv6_addr":str(temp_str.value , encoding="utf-8")})
        return Httplib.OK , tmpdict
    # ipaddr_mask    
    if key == "ipaddr_mask":
        result = so.get_interface_ipaddr_mask(int(index))
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"ipaddr_mask":result})
        return Httplib.OK , tmpdict

    # ipv4_gateway    
    if key == "ipv4_gateway":
        temp_str = create_string_buffer(20)
        result = so.get_interface_ipv4_gateway(int(index) , temp_str , 20)
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"ipv4_gateway":str(temp_str.value , encoding="utf-8")})
        return Httplib.OK , tmpdict
    # ipv6_gateway    
    if key == "ipv6_gateway":
        temp_str = create_string_buffer(40)
        result = so.get_interface_ipv6_gateway(int(index) , temp_str , 40)
        if result == -1:
            return Httplib.INTERNAL_SERVER_ERROR , None
        tmpdict.update({"ipv6_gateway":str(temp_str.value , encoding="utf-8")})
        return Httplib.OK , tmpdict

    # ingress_config    
    if key == "ingress_config":
        tmp_ingress_dict = {}
        for tmp_key in ingress_config_keys:
            result  , tmp_ingress_dict = get_single_interface_ingress_config(index , tmp_key , tmp_ingress_dict)
            if result != Httplib.OK:
                return result , None
        tmpdict.update({"ingress_config" : tmp_ingress_dict})
        return Httplib.OK , tmpdict


    # ip_reass_config    
    if key == "ip_reass_config":
        tmp_ip_reass_dict = {}
        for tmp_key in ip_reass_config_keys:
            result , tmp_ip_reass_dict= get_single_interface_ip_reass_config(index , tmp_key , tmp_ip_reass_dict)
            if result != Httplib.OK:
                return result , None
        tmpdict.update({"ip_reass_config" : tmp_ip_reass_dict})
        return Httplib.OK , tmpdict

    return Httplib.NOT_FOUND, None
        

def get_interfaces_config(index , keys):
    result_dict = {}

    if check_interface_index(index):
        return Httplib.NOT_FOUND , None

    if keys == None:
        target_keys = interface_config_keys[:]
    else:
        target_keys = keys

    if index == None:
        for i in range(1 , (get_MAX_REAL_INTERFACES_NUM() ) + 1):
            tmpdict = {}
            for tmpkey in target_keys:
                result  , tmpdict = get_single_interface_config( i , tmpkey , tmpdict)
                if result != Httplib.OK:
                    return result , None
            result_dict.update({ (get_interfaces_map_to_rest() )[i] : tmpdict})
        return Httplib.OK , result_dict

    for i in index:
        tmpdict = {}
        for tmpkey in target_keys:
            result , tmpdict = get_single_interface_config( int(i) , tmpkey , tmpdict)
            if result != Httplib.OK:
                return result , None
        result_dict.update({ (get_interfaces_map_to_rest() )[i] : tmpdict})
    return Httplib.OK , result_dict


def update_single_ip_reass_config_internal(index , key , value):
    # ip_reass_layers
    if key == "ip_reass_layers":
        result = so.ip_reass_set_max_layer(int(index) , int(value))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    
    # ip_reass_output_enable
    if key == "ip_reass_output_enable":
        result = so.ip_reass_set_reass_output(int(index) , int(value))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    
    return Httplib.NOT_FOUND


def update_single_ingress_config_internal(index , key , value):
    # acl_rule_group
    if key == "acl_rule_group":
        result = so.set_ingress_acl_rule_group(int(index) , int(value))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK

    # default_action_id
    if key == "default_action_id":
        result = so.set_ingress_default_action_id(int(index), int(value))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK

    # rule_to_action
    if key == "rule_to_action":
        if isinstance(value , dict) == False:
            return Httplib.BAD_REQUEST
        if len(list(value.keys())) == 0 :
            for i in range(1 , get_max_rule_id() + 1):
                result  = so.set_ingress_rule_to_action(int(index) , int(i) , 0)
                if result != 0:
                    return Httplib.INTERNAL_SERVER_ERROR
            return Httplib.OK

        for rule_id in value.keys():
            result = so.set_ingress_rule_to_action(int(index) , int(rule_id) , int(value[rule_id]))
            if result != 0:
                return Httplib.BAD_REQUEST
        return Httplib.OK

    return Httplib.NOT_FOUND

def update_interface_ingress_config(index , config_dict):
    if isinstance(config_dict , dict) == False:
        return Httplib.BAD_REQUEST
    
    for tmpkey in config_dict.keys():
        ret_status = update_single_ingress_config_internal(index , tmpkey , config_dict[tmpkey])
        if ret_status != Httplib.OK:
            return ret_status
    return Httplib.OK

def update_interface_ip_reass_config(index , config_dict):
    if isinstance(config_dict , dict) == False:
        return Httplib.BAD_REQUEST
    
    for tmpkey in config_dict.keys():
        ret_status = update_single_ip_reass_config_internal(index , tmpkey , config_dict[tmpkey])
        if ret_status != Httplib.OK:
            return ret_status
    return Httplib.OK

def update_single_interface_config_internal(index , key , value):
    # admin_status
    if key == "admin_status":
        result = so.set_interface_admin_status(int(index) , int(value))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK

    # port_id
    if key == "port_id":
        return Httplib.OK

    # alias_name    
    if key == "alias":
        result = so.set_interface_alias_name(int(index) , value.encode("utf-8"))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    
    # interface_type
    if key == "interface_type":
        intf_type_dict = {"normal" : 0 , "loopbak" : 1 , "force_tx" : 2, "normal_and_check" : 3, "tunnel": 4}
        if intf_type_dict[value] == None:
            return Httplib.BAD_REQUEST
        result = so.set_interface_interface_type(int(index) , intf_type_dict[value])
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK

    # mac
    if key == "mac":
        result = so.set_interface_mac_addr(int(index) , value.encode("utf-8"))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    # ipv4_addr    
    if key == "ipv4_addr":
        result = so.set_interface_ipv4(int(index) , value.encode("utf-8"))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    # ipv6_addr    
    if key == "ipv6_addr":
        result = so.set_interface_ipv6(int(index) , value.encode("utf-8"))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    # ipaddr_mask    
    if key == "ipaddr_mask":
        result = so.set_interface_ipaddr_mask(int(index) , int(value))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK

    # ipv4_gateway    
    if key == "ipv4_gateway":
        result = so.set_interface_ipv4_gateway(int(index) , value.encode("utf-8"))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    # ipv6_gateway    
    if key == "ipv6_gateway":
        result = so.set_interface_ipv6_gateway(int(index) , value.encode("utf-8"))
        if result != 0:
            return Httplib.BAD_REQUEST
        return Httplib.OK
    # ingress_config    
    if key == "ingress_config":
        result = update_interface_ingress_config(int(index) , value)
        return result

    # ip_reass_config    
    if key == "ip_reass_config":
        result = update_interface_ip_reass_config(int(index) , value)
        return result

    return Httplib.NOT_FOUND


def update_single_interface_config(index , config_dict):
    if isinstance(config_dict , dict) == False:
        return Httplib.BAD_REQUEST
    
    if check_interface_index([index]):
        return Httplib.NOT_FOUND

    for tmpkey in config_dict.keys():
        ret_status = update_single_interface_config_internal(index , tmpkey , config_dict[tmpkey])
        if ret_status != Httplib.OK:
            return ret_status
    return Httplib.OK


def update_interface_config(config_dict):
    index_list = []
    for i in  config_dict.keys():
        index_list.append(trans_interface_name_to_index(i) )
    
    if check_interface_index(index_list):
        return Httplib.NOT_FOUND

    for i in config_dict.keys():

        ret_status = update_single_interface_config( trans_interface_name_to_index(i) , config_dict[i])
        if ret_status != Httplib.OK:
            return ret_status
    return Httplib.OK


class InterfacesConfigHandler(BaseHandler):
    
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run InterfacesConfigHandler GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.keys = get_keys_arg(self.request.query_arguments)
            self.depth = get_depth_param(self.request.query_arguments)
            self.index = get_index_arg(self.request.query_arguments , False)
            query_args = self.request.query_arguments

            try:
                if self.index:
                    if check_interface_index([ trans_interface_name_to_index(i) for i in self.index]):
                        self.set_status(Httplib.NOT_FOUND)
                        self.finish()
                        return
            except:
                self.set_status(Httplib.NOT_FOUND)
                self.finish()
                return

            app_log.debug("var_name:{0}".format(var_name))
            #/interface/config
            if var_name == None:
                if self.index:
                    search_index = []
                    for tmpintf in self.index:
                        search_index.append( trans_interface_name_to_index(tmpintf) )

                    ret_status,tmpdict = get_interfaces_config(search_index , self.keys)
                    if ret_status == Httplib.FORBIDDEN:
                        self.set_status(Httplib.FORBIDDEN)
                        self.finish()
                        return
                    if ret_status != Httplib.OK:
                        self.set_status(Httplib.NOT_FOUND)
                        self.finish()
                        return
                    self.write(to_json(tmpdict))
                    self.finish()
                    return
                elif self.depth:
                    if not  isinstance(self.depth, dict):
                        if self.depth > 1:
                            try:
                                self.index = [ trans_interface_name_to_index(i) for i in self.index]
                            except:
                                self.index = None
                            ret_status,tmpdict = get_interfaces_config(self.index , self.keys)
                            if ret_status == Httplib.FORBIDDEN:
                                self.set_status(Httplib.FORBIDDEN)
                                self.finish()
                                return
                            if ret_status != Httplib.OK:
                                self.set_status(Httplib.NOT_FOUND)
                                self.finish()
                                return
                            self.write(to_json(tmpdict))
                            self.finish()
                            return
                    else:
                        self.set_status(Httplib.BAD_REQUEST)
                        self.finish()
                        return
                tmpdict = {}
                for i in range(1, (get_MAX_REAL_INTERFACES_NUM() ) +1):
                    tmpindex = (get_interfaces_map_to_rest() )[i]
                    tmpdict[tmpindex] = "{0}/{1}".format("/rest/v1/interfaces/config", tmpindex)
                self.write(to_json(tmpdict))
                self.finish()
                return 
            #/interface/config/1
            try:
                var_name = trans_interface_name_to_index(var_name)
            except:
                var_name = 0
            if int(var_name) >= 1 or int(var_name) <= (get_MAX_REAL_INTERFACES_NUM() ):
                ret_status,tmpdict = get_interfaces_config([var_name] , self.keys)
                if ret_status == Httplib.FORBIDDEN:
                    self.set_status(Httplib.FORBIDDEN)
                    self.finish()
                    return
                if ret_status != Httplib.OK:
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
                self.write(to_json(tmpdict))
                self.finish()
                return
            self.set_status(Httplib.NOT_FOUND)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

    @gen.coroutine
    def put(self , var_name = None):
        app_log.debug("Run InterfacesConfigHandler put....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.index = get_index_arg(self.request.query_arguments , False)
            config_dict = decode_put_body(self.request.body)

            if config_dict == None or isinstance(config_dict , dict) == False:
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()
                return

            #/interface/config
            if var_name == None:
                if self.index != None and len(self.index) > 1:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.finish()
                    return
                if self.index == None:
                    ret_status = update_interface_config(config_dict)
                    if ret_status == Httplib.OK:
                        ret_status = Httplib.NO_CONTENT
                    self.set_status(ret_status)
                    self.finish()
                    return
                if len(self.index) == 1:
                    ret_status =  update_single_interface_config( 
                        trans_interface_name_to_index(self.index[0]) , config_dict)
                    if ret_status == Httplib.OK:
                        ret_status = Httplib.NO_CONTENT
                    self.set_status(ret_status)
                    self.finish()
                    return
                
            #/interface/config/1
            try:
                var_name = trans_interface_name_to_index(var_name)
            except:
                var_name = 0
            if int(var_name) >= 1 or int(var_name) <= (get_MAX_REAL_INTERFACES_NUM() ):
                ret_status = update_single_interface_config(var_name , config_dict)
                if ret_status == Httplib.OK:
                        ret_status = Httplib.NO_CONTENT
                self.set_status(ret_status)
                self.finish()
                return
            self.set_status(Httplib.NOT_FOUND)
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

    @gen.coroutine
    def patch(self , var_name = None):
        app_log.debug("Run InterfacesConfigHandler put....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            request = decode_request_path_body(self.request.body)
            self.index = get_index_arg(self.request.query_arguments , False)
            if request == None:
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()
                return
            ret_status = Httplib.BAD_REQUEST
            #/interface/config/1
            if var_name != None :
                try:
                    var_name = trans_interface_name_to_index(var_name)
                except:
                    var_name = 0
                if int(var_name) >= 1 or int(var_name) <= (get_MAX_REAL_INTERFACES_NUM() ):
                    tmp_index = int(var_name)
                    for single_op in request:
                        if single_op[0] != "replace":
                            continue
                        ret_status = update_single_interface_config(tmp_index , single_op[1])
                        if ret_status != Httplib.OK:
                            break
            elif self.index != None:
                tmp_index = trans_interface_name_to_index(self.index[0])
                for single_op in request:
                    if single_op[0] != "replace":
                        continue
                    ret_status = update_single_interface_config(tmp_index , single_op[1])
                    if ret_status != Httplib.OK:
                        break
            elif self.index == None:
                for single_op in request:
                    if single_op[0] != "replace":
                        continue
                    ret_status = update_interface_config(single_op[1])
                    if ret_status != Httplib.OK:
                        break
            if ret_status == Httplib.OK:
                ret_status = Httplib.NO_CONTENT
            self.set_status(ret_status)
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

#################################################################################

class SingleInterfaceStat(Structure):
        _fields_ = [
                        ("in_packets", c_ulonglong),
                        ("in_octets", c_ulonglong),
                        ("in_bps", c_ulonglong),
                        ("in_pps", c_ulonglong),
                        ("out_packets", c_ulonglong),
                        ("out_octets", c_ulonglong),
                        ("out_bps", c_ulonglong) , 
                        ("out_pps", c_ulonglong),
                        ("drop_packets", c_ulonglong),
                        ("tx_error_packets", c_ulonglong) , 
                        ("tx_ok_packets", c_ulonglong) , 
                        ("rx_miss_packets", c_ulonglong) , 

                        ("packet_type_packets", c_ulonglong * 64),
                        ("packet_type_octets", c_ulonglong * 64),
                        ("packet_type_bps", c_ulonglong * 64),
                        ("packet_type_pps", c_ulonglong * 64)  ]
interfaces_stat_keys = [
    "in_packets", 
    "in_octets", 
    "in_bps", 
    "in_pps", 
    "out_packets", 
    "out_octets", 
    "out_pps", 
    "out_bps",
    "drop_packets",
    "tx_error_packets",
    "rx_miss_packets",
    "packet_type_packets",
    "packet_type_octets",
    "packet_type_pps",
    "packet_type_bps",
]

packet_type_keys = [
    "packet_type_packets",
    "packet_type_octets",
    "packet_type_pps",
    "packet_type_bps",
]

packet_type_name = [
    "vlan",
    "mpls",
    "gre",
    "vxlan",
]

def check_stat_keys(keys):
    for i in keys:
        if i not in interfaces_stat_keys:
            return True
    return False


def get_array_list(ptr , key):
    array_list = eval("ptr.contents.{0}".format(key))
    tmpdict = {}
    for i in range(len(packet_type_name)):
        tmpdict.update({packet_type_name[i] : array_list[i] })

    return tmpdict


def get_single_interface_stat(index , keys):
    so.get_interface_stat_ptr.restype = POINTER(SingleInterfaceStat)
    ptr = so.get_interface_stat_ptr(int(index))
    if bool(ptr) == False:
        return Httplib.INTERNAL_SERVER_ERROR , None
    result_dict = {}
    for _key in keys:
        if _key in packet_type_keys:
            value = get_array_list(ptr , _key )
        else:
            value = eval("ptr.contents.{0}".format(_key))
        result_dict.update({_key : value})
    
    return Httplib.OK , result_dict

def get_interfaces_stat(index , keys):
    result_dict = {}
    for i in index:
        ret_status , tmp_dict = get_single_interface_stat(i , keys)
        if ret_status != Httplib.OK:
            return ret_status , None
        result_dict.update({(get_interfaces_map_to_rest() )[i] : tmp_dict})

    return Httplib.OK , result_dict

def get_total_cnt_packet_type(tmpdict , indexs ,  tmpkey):
    result_dict = {}

    for _key in packet_type_name:
        total_cnt = 0
        for i in indexs:
            total_cnt = total_cnt + tmpdict[i][tmpkey][_key]
        result_dict.update({_key : total_cnt})

    return result_dict


def get_total_cnt(tmpdict):
    indexs = list(tmpdict.keys())
    keys = list(tmpdict[indexs[0]].keys())
    result_dict = {}

    for tmpkey in keys:
        total_cnt = 0
        if tmpkey in packet_type_keys:
            total_cnt = get_total_cnt_packet_type( tmpdict , indexs ,  tmpkey)
        else:
            for i in indexs:
                total_cnt = total_cnt + tmpdict[i][tmpkey]
        result_dict.update({tmpkey : total_cnt})

    return result_dict


def clear_interface_list_stat(index):
    if check_interface_index(index):
        return Httplib.NOT_FOUND

    for i in index:
        result = eval("so.clean_interface_stat({0})".format(int(i)))
        if result != 0:
            return Httplib.INTERNAL_SERVER_ERROR

    return Httplib.OK

class InterfacesStatHandler(BaseHandler):
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run InterfacesStatHandler GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.keys = get_keys_arg(self.request.query_arguments)
            self.index = get_index_arg(self.request.query_arguments , False)
            self.type = get_type_arg(self.request.query_arguments)

            tmp_index = []

            if var_name == None:
                if self.index != None:
                    for i in self.index:
                        tmp_index.append(trans_interface_name_to_index(i))
            else:
                tmp_index.append(trans_interface_name_to_index(var_name))

            if len(tmp_index) == 0:
                for i in range(1 , (get_MAX_REAL_INTERFACES_NUM() ) + 1):
                    tmp_index.append(i)

            tmp_keys = []
                
            if self.keys == None:
                tmp_keys = interfaces_stat_keys[:]
            else:
                if check_stat_keys(self.keys):
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()
                    return
                tmp_keys = self.keys
            

            if check_interface_index(tmp_index):
                self.set_status(Httplib.NOT_FOUND)
                self.finish()
                return
            
            ret_status , tmpdict = get_interfaces_stat(tmp_index , tmp_keys)
            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return

            if self.type != None:
                if self.type == "total":
                    tmpdict = get_total_cnt(tmpdict)
            
            self.set_status(Httplib.OK)
            self.write(to_json(tmpdict))
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()


    @gen.coroutine
    def patch(self , var_name = None):
        app_log.debug("Run InterfacesStatHandler put....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            request = decode_request_path_body(self.request.body)
            self.index = get_index_arg(self.request.query_arguments , False)
            if request == None:
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()
                return
            tmpindex = []
            #/interface/stat/1
            if var_name != None :
                tmpindex.append( trans_interface_name_to_index (var_name) )
            else:
                tmp_dict = {}
                for tmp_req in request:
                    tmp_dict.update(tmp_req[1])
                for i in tmp_dict.keys():
                    if i == "":
                        tmpindex  = [ i for  i in range(1, (get_MAX_REAL_INTERFACES_NUM() ))]
                        break
                    else:
                        tmpindex = [ trans_interface_name_to_index (i)  for i in  list(tmp_dict.keys())]
                        break

            if len(tmpindex) == 0:
                self.set_status(Httplib.NOT_FOUND)
                self.finish()
                return
            
            ret_status = clear_interface_list_stat(tmpindex)
            self.set_status(ret_status)
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()


###############################################

interface_phy_key = [
    "port_type",
    "port_info"
]

def get_single_intf_phy_info_link(index):
    result = so.get_interfaces_phy_info_link(int(index))
    return "up" if result else "down"

def get_single_intf_phy_info_speed(index):
    result = so.get_interfaces_phy_info_speed(int(index))
    if result <=0:
        return "Unknown"
    
    return result

def get_single_intf_phy_info_duplex(index):
    result = so.get_interfaces_phy_info_duplex(int(index))
    if result <=0:
        return "Unknown"
    
    if result == 1 :
        return "Half"
    if result == 2 :
        return "Full"


    return "Unknown"

def get_single_intf_phy_info_mtu(index):
    result = so.get_interfaces_phy_info_mtu(int(index))
    if result <=0:
        return "Unknown"
    
    return result

def get_single_interface_status_service_link(index):
    result = so.get_interfaces_status_service_link(int(index))
    return "up" if result else "down"


def get_single_interface_phy_info(index , key , tmpdict):
    # port_type
    if key == "port_type":
        tmpdict.update({"port_type":"panel port"})
        return Httplib.OK , tmpdict

    # port_info
    if key == "port_info":
        tmpdict.update({"port_info":  {   "speed" : 10000   }  })
        tmpdict["port_info"]["link_status"] =  get_single_intf_phy_info_link(index)
        tmpdict["port_info"]["duplex"] = get_single_intf_phy_info_duplex(index)
        tmpdict["port_info"]["mtu"] = get_single_intf_phy_info_mtu(index)
        if get_target_device() == "VM":
            tmpdict["port_info"]["speed"] =  1000

        return Httplib.OK , tmpdict 

    return Httplib.NOT_FOUND, None

def get_interfaces_phy_info(index , keys):
    result_dict = {}
    if keys == None:
        target_keys = interface_phy_key
    else:
        target_keys = keys

    if index == None:
        for i in range(1 , (get_MAX_REAL_INTERFACES_NUM() ) + 1):
            tmpdict = {}
            for tmpkey in target_keys:
                result  , tmpdict = get_single_interface_phy_info( i , tmpkey , tmpdict)
                if result != Httplib.OK:
                    return result , None
            result_dict.update({(get_interfaces_map_to_rest() )[i] : tmpdict})
        return Httplib.OK , result_dict

    for i in index:
        tmpdict = {}
        for tmpkey in target_keys:
            result , tmpdict = get_single_interface_phy_info( int(i) , tmpkey , tmpdict)
            if result != Httplib.OK:
                return result , None
        result_dict.update({ (get_interfaces_map_to_rest() )[i] : tmpdict})
    return Httplib.OK , result_dict

class InterfacesPhyInfoHandler(BaseHandler):
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run InterfacesPhyInfoHandler GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.keys = get_keys_arg(self.request.query_arguments)
            self.index = get_index_arg(self.request.query_arguments , False)

            tmp_index = []

            if var_name == None:
                if self.index != None:
                    for i in self.index:
                        tmp_index.append(trans_interface_name_to_index(i))
            else:
                tmp_index.append( trans_interface_name_to_index(var_name))

            if len(tmp_index) == 0:
                for i in range(1 , (get_MAX_REAL_INTERFACES_NUM() ) + 1):
                    tmp_index.append(i)

            
            if check_interface_index(tmp_index):
                self.set_status(Httplib.NOT_FOUND)
                self.finish()
                return
        
            ret_status , tmpdict = get_interfaces_phy_info(tmp_index , self.keys)
            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return

            self.set_status(Httplib.OK)
            self.write(to_json(tmpdict))
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()


#################################################
def get_single_interface_status(index):
    tmpdict = {}
    ret_status , response = get_single_interface_config(index , "admin_status" , {})
    if ret_status != Httplib.OK:
        return ret_status , None
    tmpdict.update(response)
    ret_status,  response = get_single_interface_phy_info(index , "port_info" , {})
    if ret_status != Httplib.OK:
        return ret_status , None
    tmpdict.update(response["port_info"])


    return Httplib.OK , tmpdict

def get_interfaces_status(index ):
    result_dict = {}
    if index == None:
        for i in range(1 , (get_MAX_REAL_INTERFACES_NUM() ) + 1):
            result  , tmpdict = get_single_interface_status( int(i) )
            if result != Httplib.OK:
                return result , None
            result_dict.update({(get_interfaces_map_to_rest() )[i] : tmpdict})
        return Httplib.OK , result_dict

    for i in index:
        result , tmpdict = get_single_interface_status( int(i) )
        if result != Httplib.OK:
            return result , None
        result_dict.update({ (get_interfaces_map_to_rest() )[i] : tmpdict})
    return Httplib.OK , result_dict

class InterfacesStatusHandler(BaseHandler):
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run InterfacesStatusHandler GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.keys = get_keys_arg(self.request.query_arguments)
            self.index = get_index_arg(self.request.query_arguments , False)

            tmp_index = []

            if var_name == None:
                if self.index != None:
                    for i in self.index:
                        tmp_index.append( trans_interface_name_to_index(i) )
            else:
                tmp_index.append( trans_interface_name_to_index(var_name) )

            if len(tmp_index) == 0:
                for i in range(1 , (get_MAX_REAL_INTERFACES_NUM() ) + 1):
                    tmp_index.append(i)
            
            if check_interface_index(tmp_index):
                self.set_status(Httplib.NOT_FOUND)
                self.finish()
                return
            
            ret_status , tmpdict = get_interfaces_status(tmp_index )
            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return

            if self.keys:
                _tmpdict = {}
                for index in tmpdict:
                    for key in self.keys:
                        _tmpdict[index] = {}
                        _tmpdict[index][key] = tmpdict[index][key]
                self.set_status(Httplib.OK)
                self.write(to_json(_tmpdict))
                self.finish()
                return 
            self.set_status(Httplib.OK)
            self.write(to_json(tmpdict))
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()



class InterfacesHandler(BaseHandler):
    
    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run InterfacesHandler GET....")
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
                tmpdict = {"config":"/rest/v1/interfaces/config" , "stat" : "/rest/v1/interfaces/stat" ,
                            "phy_info":"/rest/v1/interfaces/phy_info" , "status" : "/rest/v1/interfaces/status"}
                self.write(to_json(tmpdict))
                self.finish()
                return

            tmp_index = []
            for i in range(1 , (get_MAX_REAL_INTERFACES_NUM() ) + 1):
                tmp_index.append(i)

            ret_status , config_dict = get_interfaces_config(tmp_index , None)

            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return

            ret_status , stat_dict = get_interfaces_stat(tmp_index , interfaces_stat_keys)

            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return

            ret_status , phy_info_dict = get_interfaces_phy_info(None , None)

            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return

            ret_status , status_dict = get_interfaces_status(None )

            if ret_status != Httplib.OK:
                self.set_status(ret_status)
                self.finish()
                return

            result_dict = {
                "config" : config_dict,
                "stat" : stat_dict,
                "status" : status_dict,
                "phy_info" : phy_info_dict
            }

            self.set_status(Httplib.OK)
            self.write(to_json(result_dict))
            self.finish()

        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

interfaces_urls = [
        (r"/rest/v1/interfaces", InterfacesHandler),
        (r"/rest/v1/interfaces/config/(.*)", InterfacesConfigHandler),
        (r"/rest/v1/interfaces/config", InterfacesConfigHandler),
        (r"/rest/v1/interfaces/stat/(.*)", InterfacesStatHandler),
        (r"/rest/v1/interfaces/stat", InterfacesStatHandler),
        (r"/rest/v1/interfaces/status/(.*)", InterfacesStatusHandler),
        (r"/rest/v1/interfaces/status", InterfacesStatusHandler),
        (r"/rest/v1/interfaces/phy_info/(.*)", InterfacesPhyInfoHandler),
        (r"/rest/v1/interfaces/phy_info", InterfacesPhyInfoHandler),
        ]
