#This file is action.h  to ctype class  
#Can't use build_class.py!!!!

from ctypes import *
from tapplet.interfaces_map import *
from tapplet.nshellutils import *


MAX_ACTIONS_NUM = so.action_get_max_cnt()

MAX_LOAD_BALANCE_LOAD_NODE = 64


request_action_type = ["forward", "load_balance", "drop"]
request_load_balance = ["", "round_robin", "wrr", 
                        "outer_src_dst_ip", "outer_src_ip", "outer_dst_ip",
                        "inner_src_dst_ip", "inner_src_ip", "inner_dst_ip"]

request_basic_action_keys = ["type" , "interfaces" , "load_balance_weight" , "load_balance_mode"]

                    
type_of_erspan_list = ["ERSPAN_I" , "ERSPAN_II" , "ERSPAN_III"]
action_type_map_to_rest = {
                   0:"",
                   1:"drop", 
                   2:"forward", 3:"load_balance"
                  }
action_type_map_to_se = {v : k for k, v in action_type_map_to_rest.items()} 

load_balance_mode_map_to_rest = {
        0:"",
        1:"round_robin",  2:"wrr", 3:"outer_src_dst_ip",
        4:"outer_src_ip", 5:"outer_dst_ip"
                        }

load_balance_mode_map_to_se = {v : k for k, v in load_balance_mode_map_to_rest.items()} 

if is_little_endian():
    class Ip_Addr(Union):
        class Ip_4_Addr(Structure):
    	    _fields_ = [
                        ("addr_v4", c_uint),
                        ("reserved", c_uint)]
        class Ip_6_Addr(Structure):
    	    _fields_ = [
                        ("addr_v6_upper", c_ulonglong),
                        ("addr_v6_lower", c_ulonglong)]
        _fields_ = [    
                    ("ip4_addr", Ip_4_Addr),
                    ("ip6_addr", Ip_6_Addr),
                    ("ipv4_byte", c_ubyte * 4),
                    ("ipv6_byte", c_ubyte * 16),
                    ("value_32",     c_uint*4),
                    ("value",     c_ulonglong*2) ]

else:
    class Ip_Addr(Union):
        class Ip_4_Addr(Structure):
    	    _fields_ = [
                        ("reserved", c_uint),
                        ("addr_v4", c_uint)]
        class Ip_6_Addr(Structure):
    	    _fields_ = [
                        ("addr_v6_upper", c_ulonglong),
                        ("addr_v6_lower", c_ulonglong)]

        _fields_ = [    
                    ("ip4_addr", Ip_4_Addr),
                    ("ip6_addr", Ip_6_Addr),
                    ("ipv4_byte", c_ubyte * 4),
                    ("ipv6_byte", c_ubyte * 16),
                    ("value_32",  c_uint*4),
                    ("value",     c_ulonglong*2)]


class LoadBalanceWeight(Structure):
    _fields_ = [
                    ("cw", c_int),
                    ("gcd", c_int),
                    ("mw", c_int)]

class AdditionalActionSwitch(Union):
    class _SwitchBit(Structure):
        _fields_ = [

                    ("flag_gre_encapsulation", c_uint, 1),
                    ("flag_vxlan_encapsulation", c_uint, 1),
                    ("flag_erspan_encapsulation",c_uint, 1)]
    _fields_ = [    ("switch", _SwitchBit),
                    ("switch_flags", c_uint)]

request_addi_action_keys = []

for tmp in AdditionalActionSwitch._SwitchBit._fields_:
    request_addi_action_keys.append(tmp[0][5:])

class AdditionalAction(Structure):
    _fields_ = [
                ("additional_switch", AdditionalActionSwitch),

                ("gre_dmac", c_ubyte*6),
                ("gre_dip_type", c_ubyte),
                ("gre_dip", Ip_Addr),
                ("gre_dscp", c_ubyte),
                ("vxlan_dmac", c_ubyte*6),
                ("vxlan_dport", c_ushort),
                ("vxlan_dip_type", c_ubyte),
                ("vxlan_dip", Ip_Addr),
                ("vxlan_vni", c_uint),
                ("vxlan_dscp", c_ubyte),
                ("erspan_dmac", c_ubyte*6),
                ("erspan_dip_type", c_ubyte),
                ("erspan_dip", Ip_Addr),
                ("erspan_type", c_ubyte),
                ("erspan_session_id", c_ushort),
                ("erspan_dscp", c_ubyte)
            ]

addi_dscp_min = 0
addi_dscp_max = 63
addi_vni_min = 0
addi_vni_max = 0xFFFFFF
addi_erspan_ses_id_min = 0
addi_erspan_ses_id_max = 0x3FF

request_addi_gre_encap_keys = ["switch" ,"gre_dmac" , "gre_dip" , "gre_dscp"]
request_addi_vxlan_encap_keys = ["switch" ,"vxlan_dmac" ,  "vxlan_dip" ,"vxlan_dscp" , "vxlan_dport" ,"vxlan_vni" ]
request_addi_erspan_encap_keys = ["switch" ,"erspan_dmac" ,  "erspan_dip" ,"erspan_dscp" , "erspan_type" ,"erspan_session_id" ]

class Action(Structure):
    _fields_ = [
                ("ref_counter", c_uint),
                #enum action_type_t
                ("type", c_int),
                ("forward_interface_id", c_ubyte),

                ("load_balance_interfaces_num", c_ubyte),
                ("load_balance_interface_array", c_ubyte*(so.get_interfaces_max_cnt())),

                ("additional_actions", AdditionalAction),

                #enum load_balance_mode_t
                ("load_balance_mode", c_int),
                ("weight_data", LoadBalanceWeight),

                ("load_balance_index", c_uint),
                ("load_balance_weight", c_int*MAX_LOAD_BALANCE_LOAD_NODE) ]

# def sf_gen_dict(type_name):
#     tmp_dict = {}
#     for i in type_name._fields_:
#         tmp_dict.update({i[0]:i[1]})
#     return tmp_dict  

if __name__ == "__main__":
    action = Action()
    #tmp = (c_int*MAX_LOAD_BALANCE_LOAD_NODE)()
    #for i in range(3):
    #    tmp[i] = 1
    #for i in tmp:
    #    print(i)

    action_dict = sf_gen_dict(Action)
    additional_dict = sf_gen_dict(AdditionalAction)
    additional_switch_dict = sf_gen_dict(AdditionalActionSwitch)
    additional_switch_dict_switch =  sf_gen_dict(AdditionalActionSwitch._SwitchBit)

    print(additional_switch_dict_switch)
