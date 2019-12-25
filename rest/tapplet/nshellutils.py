from ctypes import *

from tapplet.interfaces_map import *
from tornado.log import app_log
from conf.globalValue import *

import socket
import time
from socket import htons,htonl,ntohl,ntohs
import struct
import ipaddress

import array


so = CDLL('/usr/lib/vpp_plugins/libnshell.so')

shm_linked_flag = 0

INT_TO_IP = lambda x: '.'.join([str(int(x/(256**i)%256)) for i in range(3,-1,-1)])
IP_TO_INT = lambda x:sum([256**j*int(i) for j,i in enumerate(x.split('.')[::-1])])

htonll = lambda a:struct.unpack('!Q', struct.pack('Q', a) )[0]
ntohll = lambda a:struct.unpack('Q', struct.pack('!Q', a))[0]


device_type_name = create_string_buffer(32)


def try_link_shm():
    global shm_linked_flag
    if shm_linked_flag != 1:
        if so.get_and_link_all_shm() != 0:
            return -1
        if so.update_shm_map_map_inlib() != 0:
            so.unlink_all_shm()
            return -1

    ### update interface map
    ret = so.shm_main_get_device_type_name(device_type_name , 32)
    if ret < 0:
        app_log.error("get device_type fail....")
        return -1

    target_device = str(device_type_name.value , encoding="utf-8")    

    ret = so.get_interfaces_real_cnt()
    if ret <= 0:
        app_log.error("get interface real cnt fail....ret({0})".format(ret))
        return -1

    update_real_interface_map(target_device , (so.get_interfaces_max_cnt()) ,  ret)

    shm_linked_flag = 1
    return 0

# '''
# ####################################################
# '''

def is_ip_valid(ipaddr):
    try:
        ip = ipaddress.ip_address(ipaddr)
        return ip.version
    except:
        return False

def INT_TO_IPV6(addr):
    return str(ipaddress.IPv6Address(addr))

def IPV6_TO_INT(addr):
    return int(ipaddress.ip_address(addr))

def is_little_endian():
    a = array.array('H', [1]).tostring()
    if a[0] == 1:
        return True
    else:
        return False


def host_to_net_u16(a):
    return htons(a)

def host_to_net_u32(a):
    return htonl(a)

def host_to_net_u64(a):
    return htonll(a)

def net_to_host_u16(a):
    return ntohs(a)

def net_to_host_u32(a):
    return ntohl(a)

def net_to_host_u64(a):
    return ntohll(a)

default_type = [ "c_char", "c_char", "c_ubyte",
                "c_short", "c_ushort", "c_int", "c_uint",
                "c_long", "c_ulong", 
                "c_longlong", "c_ulonglong", 
                "c_float", "c_double"]


def get_data_libnshell(module_name, var_name, var_types):
    exec("so.get_{0}_config_{1}.restype = {2}".format(module_name, var_name, var_types))
    return {var_name : eval("so.get_{0}_config_{1}()".format(module_name, var_name))}

def get_data_arrar_index_libnshell(module_name, var_name, index):
    return eval("so.get_{0}_config_{1}_index(index)".format(module_name, var_name))

def get_data_array_libnshell(module_name, var_name, byref_ptr, num):
    eval("so.get_{0}_config_{1}( byref_ptr, num)".format(module_name, var_name))

def set_data_libnshell(module_name, var_name, value):
    return eval("so.set_{0}_config_{1}({2})".format(module_name, var_name, value))

def set_data_array_index_libnshell(module_name, var_name, value, index):
    return eval("so.set_{0}_config_{1}_index(value, index)".format(module_name, var_name))

def reset_config_libshell(module_name):
    return eval("so.reset_{0}_global_config()".format(module_name ))

def reset_all_config_libshell():
    return eval("so.reset_all_global_config()")

def get_stat_ptr_libnshell(module_name , ret_type):
    exec("so.get_{0}_stat_ptr.restype = POINTER(ret_type)".format(module_name))
    return eval("so.get_{0}_stat_ptr()".format(module_name)) 

def clean_stat_libnshell(module_name):
    return eval("so.clean_{0}_stat()".format(module_name))

def sf_gen_dict(type_name):
    tmp_dict = {}
    for i in type_name._fields_:
        tmp_dict.update({i[0]:i[1]})
    return tmp_dict

def sf_judge_key_in_struct(key , type_name):
    tmp_list = []
    for i in type_name._fields_:
        tmp_list.append(i[0])
    if key in tmp_list:
        return True
    else:
        return False
    
    



def add_array_value(tmp, add_value_list, order = False, startIndex = 0, \
        host_to_net_size16 = False, net_to_host_size16 = False, \
        host_to_net_size32 = False, net_to_host_size32 = False):
    index = 1 - startIndex
    if order:
        for i in add_value_list:
            if host_to_net_size16:
                i = host_to_net_u16(i)
            elif net_to_host_size16:
                i = net_to_host_u16(i)
            elif host_to_net_size32:
                i = host_to_net_u32(i)
            elif net_to_host_size32:
                i = net_to_host_u16(i)
            tmp[i-index] = i 
        if startIndex == 1:
            tmp[0] = len(add_value_list)
    else:
        for i in range(len(add_value_list)):
            if host_to_net_size16:
                tmp[i] = host_to_net_u16(add_value_list[i])
            elif net_to_host_size16:
                tmp[i] = net_to_host_u16(add_value_list[i])
            elif host_to_net_size32:
                tmp[i] = host_to_net_u32(add_value_list[i])
            elif net_to_host_size32:
                tmp[i] = net_to_host_u32(add_value_list[i])
            else:
                tmp[i] = add_value_list[i]
    return tmp 

def get_stat_key(module_name, stat_dict, type_name, keys, index = None):
    #tmplist = []
    tmplist = {}
    ptr = get_stat_ptr_libnshell(module_name , type_name)
    if index != None:
        for _key in  keys:
            if not _key in  stat_dict.keys():
                return None
            _var_type = stat_dict[_key].__name__ 
            if _var_type in default_type:
                return None
            num  = int(_var_type.split("_")[-1])
            if index >= num:
                return None
            result = eval("ptr.contents.{0}[{1}]".format(_key, index))
            tmplist.update({_key :result})
    else:
        for _key in keys:
            _var_type = stat_dict[_key].__name__ 
            if _var_type in default_type:
                result = eval("ptr.contents.{0}".format(_key))
                result = { _key : result}
                tmplist.update(result)
            else:
                num  = int(_var_type.split("_")[-1])
                _tmp_list = []
                for i in range(0, num):
                    _tmp_value = eval("ptr.contents.{0}[{1}]".format(_key, i))
                    _tmp_list.append(_tmp_value)
                result = { _key : _tmp_list}
                tmplist.update(result)
    return tmplist

def get_stat_all(module_name, stat_dict,type_name):
    #tmplist = []
    tmplist = {}
    ptr = get_stat_ptr_libnshell(module_name , type_name)
    for _key in stat_dict.keys():
        _var_type = stat_dict[_key].__name__ 
        if _var_type in default_type:
            value = eval("ptr.contents.{0}".format(_key))
            result = {_key:value}
        else:
            _tmp = stat_dict[_key]()
            num  = int(_var_type.split("_")[-1])
            _tmp_list = []
            for index in range(0, num):
                value = eval("ptr.contents.{0}[{1}]".format(_key , index))
                _tmp_list.append(value)
            result = { _key : _tmp_list}
        
        tmplist.update(result)
    return tmplist

def set_stat_clean( op, module_name):
    ret = -1
    for op_tmp in op:
        if op_tmp[0] in ["replace"]:
            key_tmplist = list(op_tmp[1].keys())
            if key_tmplist[0] == "":
                ret = clean_stat_libnshell(module_name)
    return ret


def get_config_key(module_name, config_dict, keys , index = None):
    #tmplist = []
    tmplist = {}
    if index != None:
        for _key in  keys:
            if not _key in  config_dict.keys():
                return None
            _var_type = config_dict[_key].__name__ 
            if _var_type in default_type:
                return None
            num  = int(_var_type.split("_")[-1])
            if index >= num:
                return None
            result = get_data_arrar_index_libnshell(module_name, _key, index)
            tmplist.update({_key :result})
    else:
        for _key in keys:
            _var_type = config_dict[_key].__name__ 
            if _var_type in default_type:
                result = get_data_libnshell(module_name, _key , _var_type)
                tmplist.update(result)
            else:
                _tmp = config_dict[_key]()
                num  = int(_var_type.split("_")[-1])
                get_data_array_libnshell(module_name, _key ,  byref(_tmp) ,  num)
                _tmp_list = []
                for i in range(0, num):
                    _tmp_list.append(_tmp[i])
                result = { _key : _tmp_list}
                tmplist.update(result)
    return tmplist

def get_config_all(module_name, config_dict):
    #tmplist = []
    tmplist = {}
    for i,j in config_dict.items():
        _var_type = j.__name__
        if _var_type in default_type:
            if i == "reserved":
                continue
            result = get_data_libnshell(module_name , i , _var_type)
        else:
            if i == "reserved":
                continue
            _tmp =  j()
            num  = int(j.__name__.split("_")[-1])
            get_data_array_libnshell(module_name, i ,  byref(_tmp) ,  num)
            tmp_list = []
            for _i in range(0, num):
                tmp_list.append(_tmp[_i])
            result = { i : tmp_list}
        tmplist.update(result)
    return tmplist

def set_config_key( op, module_name, config_dict):
    ret = -1
    for op_tmp in op:
        if op_tmp[0] in ["add" , "replace"]:
            key_tmplist = op_tmp[1].keys()
            for _key in key_tmplist:
                value = op_tmp[1][_key]
                if isinstance(value, dict):
                    for _index, _value in value.items():
                        ret = set_data_array_index_libnshell(module_name , _key , _value, int(_index))
                else:
                    ret = set_data_libnshell(module_name , _key , value)
        if op_tmp[0] in ["remove"]:
            ret = reset_config_libshell(module_name)
    return ret


# '''
# ####################################################
# '''
# s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# s.settimeout(0.01)

vpp_udp_restart_sec = 1.0

last_recv_time = time.time()
'''
recv ok : 0
recv fail : 1
'''
last_recv_status = 0


init_udp_client_status = 0


result = create_string_buffer(2048)
timeout_str_raw = "vpp cli server timeout"
vpp_udp_timeout_str=create_string_buffer(timeout_str_raw.encode("ascii"))


def check_udp_client_status():
    global init_udp_client_status

    if init_udp_client_status == 0:
        #500ms
        ret = so.set_socket_timeout(500)
        if ret != 0:
            return -1
        init_udp_client_status = 1
    return 0


def send2vpp_inline(cmd):
    global result
    ret = check_udp_client_status()
    if ret == 0:
        ret = so.sendmsgtoserver_rest(cmd.encode('ascii') , len(cmd) , result , 2048)
    return ret



# s.close()
def send2vpp(cmd):
    global last_recv_time
    global last_recv_status
    global send2vpp_count
    global vpp_udp_timeout_str
    global result

    time_now=time.time()
    if last_recv_status == 1:
        if (time_now - last_recv_time) < vpp_udp_restart_sec:
            return vpp_udp_timeout_str
    
    ret = send2vpp_inline(cmd)
    last_recv_time=time_now
    if ret <= 0:
        last_recv_status = 1
        return str(vpp_udp_timeout_str.value , encoding="utf-8")
    else:
        last_recv_status = 0
        return str(result.value , encoding="utf-8")
    

# '''
# ####################################################
# '''
# s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# s.settimeout(0.01)

# vpp_udp_restart_sec = 5

# last_recv_time = time.time()
# '''
# recv ok : 0
# recv timout : 1
# '''
# last_recv_status = 0

# send2vpp_count = 0

# def send2vpp_inline(cmd):
#     s.sendto(cmd.encode('ascii') , ('127.0.0.1', 18128))
#     try:
#         recv_data = s.recv(2048)
#         self.write(recv_data.decode('utf-8'))
#     except socket.timeout:
#         self.write("vpp cli server timeout")

# vpp_udp_timeout_str="vpp cli server timeout"

# # s.close()
# def send2vpp(cmd):
#     global last_recv_time
#     global last_recv_status
#     global send2vpp_count
#     global vpp_udp_timeout_str

#     time_now=time.time()
#     if last_recv_status == 1:
#         if (now - last_recv_time) < vpp_udp_restart_sec:
#             return vpp_udp_timeout_str
    
#     send2vpp_inline(cmd)
#     last_recv_time=
    


def get_class_fields_names(class_obj):
    names_list = []
    for name,value in vars(class_obj).items():
        if name == '_fields_':
            for _name,_type in value:
                names_list.append(_name)
    return names_list



def  rest_try_lock_shm_mem(handler):
    ret = so.try_lock_shm_mem()
    if ret == -1:
        handler.set_status(Httplib.INTERNAL_SERVER_ERROR)
        handler.finish()
        return -1
    elif ret == -2:
        handler.set_status(Httplib.SERVER_BUSY)
        handler.finish()
        return -1
    
    return 0

def  rest_unlock_shm_mem(handler):
    ret = so.unlock_shm_mem()
    if ret != 0:
        handler.set_status(Httplib.INTERNAL_SERVER_ERROR)
        handler.finish()
        return -1
    
    return 0



def check_acl_sync_flag():
    if so.acl_get_sync_flag() > 0:
        return True

    return False