# -*- coding: UTF-8 -*-


from tools.conftest_tools import *
from tools.rest_tools import *
from tools.tcpreplay_tools import *

from pytest_main import port1_config
from pytest_main import port2_config
from pytest_main import device_config
from pytest_main import global_verbose
from pytest_main import sf_helper
from scapy.all import *

import time

pkts_dir = "./tapplet/acl/pkts/"

config_needed = []
stat_target = []
pkts = []

ipv4_2048_json_file = "./tapplet/acl/json/2048_64_tcp_acl.json"
ipv4_2048_pcap_file = "2048x5_64_tcp_acl_pkt.pcap"

ipv6_2048_json_file = "./tapplet/acl/json/2048_128_ip6_tcp_acl.json"
ipv6_2048_pcap_file = "2048x5_128_ip6_tcp_acl_pkt.pcap"

sleep_time = 5


def check_acl_stat(rest_helper , group_id , rule_id , target_count , is_ipv6 = False , localverbose = False):
    param = {"group" : str(group_id) , "index" : str(rule_id)}
    ret = rest_helper.auto_run_no_login("acl/stat", GlobalRestValue.ACTION_GET , 
        params = param , verbose = localverbose)
    
    assert ret[0] == 0

    group_str = "group_{0}".format(group_id)

    count =  ret[1][group_str][str(rule_id)]
    assert int(count) == target_count

def check_acl_stat_no_assert(rest_helper , group_id , rule_id , target_count , is_ipv6 = False , localverbose = False):
    param = {"group" : str(group_id) , "index" : str(rule_id)}
    ret = rest_helper.auto_run_no_login("acl/stat", GlobalRestValue.ACTION_GET , 
        params = param , verbose = localverbose)
    
    if ret[0] != 0 :
        return rule_id

    group_str = "group_{0}".format(group_id)

    count =  ret[1][group_str][str(rule_id)]
    if int(count) != target_count:
        return rule_id

    return None


def set_acl_sync(rest_helper):
    data = [ {"op" : "replace"  , "path" : "/" , "value" : 1}]
    ret = rest_helper.auto_run_no_login("acl/sync", GlobalRestValue.ACTION_PATCH , 
        data = data , verbose = global_verbose)
    
    assert ret[0] == 0

    time.sleep(4)


def delete_acl_config(rest_helper , group_id , rule_id):
    param = {"group" : str(group_id) , "index" : str(rule_id)}
    ret = rest_helper.auto_run_no_login("acl/config", GlobalRestValue.ACTION_DELETE , 
        params = param , verbose = global_verbose)
    
    assert ret[0] == 0

def test_single_outer_ipv4_acl():
    '''
    单条ipv4 acl测试
    '''
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    interface_config = {
        port1_config :{
            "admin_status":1,
            "ingress_config":{
                "rule_to_action":{
                    "4":1
                },

            }
        }
    }


    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": [port1_config],
                "load_balance_weight": "", 
                "load_balance_mode": ""
            }
        }
    }

    acl_config = {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple4",
                "rule_cfg":{
                    "dip":"10.10.10.123",
                    "dip_mask":24,
                    "dport_max":62223,
                    "dport_min":62223,
                    "proto_max":6,
                    "proto_min":6,
                    "sip":"10.0.39.95",
                    "sip_mask":24,
                    "sport_max":62251,
                    "sport_min":62251,
                }
            }
        }
    }

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 1  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 1 , port1_config)

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_put_config(sf_helper , "actions" , action_config)
    dispatch_put_config(sf_helper , "acl/config" , acl_config)
    dispatch_put_config(sf_helper , "interfaces/config" , interface_config)
    set_acl_sync(sf_helper)

    # clean  stat
    clean_target_stat(sf_helper , "interfaces/stat/" + port1_config)
    clean_target_stat(sf_helper , "acl/stat")

    # send pkts
    send_all_pkts(pkts_dir , ["ip4_tcp_100B.pcap"])

    time.sleep(sleep_time)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    check_acl_stat(sf_helper , 1 , 4 , 1 , localverbose=global_verbose)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    


def test_single_outer_ipv6_acl():
    '''
    单条ipv6 acl测试
    '''
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    interface_config = {
       port1_config :{
            "admin_status":1,
            "ingress_config":{
                "rule_to_action":{
                    "4":1
                },
            },
            "interface_type":"normal"
        }
    }


    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": [port1_config],
                "load_balance_weight": "", 
                "load_balance_mode": ""
            }
        }
    }

    acl_config = {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple6",
                "rule_cfg":{
                    "dip":"2409:8801:b00:8859:1c2c:6f74:6eeb:48e3",
                    "dip_mask":128,
                    "dport_max":0,
                    "dport_min":0,
                    "proto_max":50,
                    "proto_min":50,
                    "sip":"2409:8011:a60:5::",
                    "sip_mask":128,
                    "sport_max":0,
                    "sport_min":0,
                },
            }
        }
    }

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 1  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 1 , port1_config)

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_put_config(sf_helper , "actions" , action_config)
    dispatch_put_config(sf_helper , "acl/config" , acl_config)
    dispatch_put_config(sf_helper , "interfaces/config" , interface_config)
    set_acl_sync(sf_helper)
    # clean  stat
    clean_target_stat(sf_helper , "interfaces/stat/" + port1_config)
    clean_target_stat(sf_helper , "acl/stat")


    # send pkts
    send_all_pkts(pkts_dir , ["ipv6.pcap"])

    time.sleep(sleep_time)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    check_acl_stat(sf_helper , 1 , 4 , 1 , is_ipv6 =True, localverbose=global_verbose)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    



def test_acl_two_rule_match():
    '''
    一个报文可以命中两条规则，但只返回优先级高（序号更小）的那条
    '''
## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    interface_config = {
        port1_config:{
            "admin_status":1,
            "ingress_config":{
                "rule_to_action":{
                    "4":1
                },
            },
        }
    }


    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": [port1_config],
                "load_balance_weight": "", 
                "load_balance_mode": ""
            }
        }
    }

    acl_config_1 = {
        "group_1":
        {
            "1":
            {
                "rule_type":"tuple4",
                "rule_cfg":{
                    "dip":"10.10.10.123",
                    "dip_mask":24,
                    "dport_max":62223,
                    "dport_min":62223,
                    "proto_max":6,
                    "proto_min":6,
                    "sip":"10.0.39.95",
                    "sip_mask":24,
                    "sport_max":62251,
                    "sport_min":62251,
                },
            }
        }
    }
    acl_config_2 = {
        "group_1":
        {
            "5":
            {
                "rule_type":"tuple4",
                "rule_cfg":{
                    "dip":"10.10.10.123",
                    "dip_mask":24,
                    "dport_max":62223,
                    "dport_min":0,
                    "proto_max":6,
                    "proto_min":0,
                    "sip":"10.0.39.95",
                    "sip_mask":24,
                    "sport_max":62251,
                    "sport_min":0,
                },
            }
        }
    }
    acl_config_3 = {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple4",
                "rule_cfg":{
                    "dip":"10.10.10.123",
                    "dip_mask":24,
                    "dport_max":65535,
                    "dport_min":62223,
                    "proto_max":255,
                    "proto_min":6,
                    "sip":"10.0.39.95",
                    "sip_mask":24,
                    "sport_max":65535,
                    "sport_min":62251,
                },
            }
        }
    }

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 1  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 1 , port1_config)

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_put_config(sf_helper , "actions" , action_config)
    dispatch_put_config(sf_helper , "acl/config" , acl_config_1)
    dispatch_put_config(sf_helper , "acl/config" , acl_config_3)
    delete_acl_config(sf_helper , 1 , 1  )
    dispatch_put_config(sf_helper , "acl/config" , acl_config_2)
    dispatch_put_config(sf_helper , "interfaces/config" , interface_config)
    set_acl_sync(sf_helper)
    # clean  stat
    clean_target_stat(sf_helper , "interfaces/stat/" + port1_config)
    clean_target_stat(sf_helper , "acl/stat")


    # send pkts
    send_all_pkts(pkts_dir , ["ip4_tcp_100B.pcap"])

    time.sleep(sleep_time)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    check_acl_stat(sf_helper , 1 , 4 , 1, localverbose=global_verbose)
    check_acl_stat(sf_helper , 1 , 5 , 0, localverbose=global_verbose)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    


def test_full_outer_ipv4_acl():
    '''
    2048条ipv4 acl测试
    '''
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    interface_config = {
        port1_config :{
            "admin_status":1,
            "ingress_config":{
                "rule_to_action":{
                },

            }
        }
    }

    for i in range(1 , 2048 + 1):
        interface_config[port1_config]["ingress_config"]["rule_to_action"].update({str(i) : 1})

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": [port1_config],
                "load_balance_weight": "", 
                "load_balance_mode": ""
            }
        }
    }

    acl_config = {}

    with open(ipv4_2048_json_file, "r") as post_config:
        acl_config = json.load(post_config)

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 10240  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 10240 , port1_config)


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_put_config(sf_helper , "actions" , action_config)
    dispatch_put_config(sf_helper , "acl/config" , acl_config)
    dispatch_put_config(sf_helper , "interfaces/config" , interface_config)
    set_acl_sync(sf_helper)

    # clean  stat
    clean_target_stat(sf_helper , "interfaces/stat/" + port1_config)
    clean_target_stat(sf_helper , "acl/stat")

    # send pkts
    send_all_pkts(pkts_dir , [ipv4_2048_pcap_file])

    time.sleep(sleep_time)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    failed_list = []


    for i in range(1 , 2048+1):
        ret = check_acl_stat_no_assert(sf_helper , 1 , i , 5 , localverbose=global_verbose)
        if ret != None:
            failed_list.append(i)

    for i in failed_list:
        check_acl_stat(sf_helper , 1 , i , 5 , localverbose=global_verbose)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    




def test_full_outer_ipv6_acl():
    '''
    2048条ipv6 acl测试
    '''
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    interface_config = {
        port1_config :{
            "admin_status":1,
            "ingress_config":{
                "rule_to_action":{
                },

            }
        }
    }

    for i in range(1 , 2048 + 1):
        interface_config[port1_config]["ingress_config"]["rule_to_action"].update({str(i) : 1})

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": [port1_config],
                "load_balance_weight": "", 
                "load_balance_mode": ""
            }
        }
    }

    acl_config = {}

    with open(ipv6_2048_json_file, "r") as post_config:
        acl_config = json.load(post_config)

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 10240  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 10240 , port1_config)


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_put_config(sf_helper , "actions" , action_config)
    dispatch_put_config(sf_helper , "acl/config" , acl_config)
    dispatch_put_config(sf_helper , "interfaces/config" , interface_config)
    set_acl_sync(sf_helper)

    # clean  stat
    clean_target_stat(sf_helper , "interfaces/stat/" + port1_config)
    clean_target_stat(sf_helper , "acl/stat")

    # send pkts
    send_all_pkts(pkts_dir , [ipv6_2048_pcap_file])

    time.sleep(sleep_time)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    failed_list = []


    for i in range(1 , 2048+1):
        ret = check_acl_stat_no_assert(sf_helper , 1 , i , 5 , localverbose=global_verbose)
        if ret != None:
            failed_list.append(i)

    for i in failed_list:
        check_acl_stat(sf_helper , 1 , i , 5 , localverbose=global_verbose)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    
