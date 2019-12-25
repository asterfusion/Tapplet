from tools.conftest_tools import *
from tools.rest_tools import *
from tools.tcpreplay_tools import *

from pytest_main import port1_config
from pytest_main import port2_config
from pytest_main import sf_helper

from actions_rest_test import mod_name

import time

pkts_dir = "./tapplet/actions/pkts/"

config_needed = []
stat_target = []
pkts = []


load_balance_pkt_cnt = 100

sleep_time = 3

def test_actions_forward_interface():
    '''
    action 纯转发测试
    '''
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()


    ##action config
    action_config =  {"1":{
                        "basis_actions": {
                            "type": "forward",
                            "interfaces": ["X1"],
                        }
                    } 
                    }

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets" , 3 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 3 , port1_config)

    ## add pkts
    pkts.append("action.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_all_pkts(pkts_dir , pkts)

    time.sleep(sleep_time)
    
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    
def test_action_load_balance_wrr():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()


    ##action config
    action_config =  { "1":{
                        "basis_actions":
                        {
                        "type": "load_balance",
                        "interfaces": ["X1","X2"],
                        "load_balance_weight": "1:2",
                        "load_balance_mode": "wrr"
                        }
                    }}

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config, port2_config]
    print("action_config")
    for i in action_config.items():
        print(i)
    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets" , 3  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 1 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "in_packets" , 0 , port2_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "out_packets" , 2 , port2_config)

    ## add pkts
    pkts.append("action.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_all_pkts(pkts_dir , pkts)

    time.sleep(sleep_time)
    
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    


def test_action_load_balance_outer_src_dst_ip():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()


    ##action config
    action_config =  { "1":{ "basis_actions": { "type": "load_balance", "interfaces": ["X1","X2"], "load_balance_weight": "", "load_balance_mode": "outer_src_dst_ip" } }}

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config, port2_config]
    print("action_config")
    for i in action_config.items():
        print(i)
    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets" , load_balance_pkt_cnt  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , load_balance_pkt_cnt/2 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "in_packets" , 0 , port2_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "out_packets" , load_balance_pkt_cnt/2 , port2_config)

    ## add pkts
    pkts.append("load_balance_sip_{0}.pcap".format(load_balance_pkt_cnt))


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_all_pkts(pkts_dir , pkts)

    time.sleep(sleep_time)
    
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    

def test_action_load_balance_outer_src_ip():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()


    ##action config
    action_config =  { "1":{
                        "basis_actions":
                        {
                        "type": "load_balance",
                        "interfaces": ["X1","X2"],
                        "load_balance_weight": "",
                        "load_balance_mode": "outer_src_ip"
                        }
                    }}

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config, port2_config]
    print("action_config")
    for i in action_config.items():
        print(i)
    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets" , load_balance_pkt_cnt  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , load_balance_pkt_cnt/2 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "in_packets" , 0 , port2_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "out_packets" , load_balance_pkt_cnt/2 , port2_config)

    ## add pkts
    pkts.append("load_balance_sip_{0}.pcap".format(load_balance_pkt_cnt))


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_all_pkts(pkts_dir , pkts)

    time.sleep(sleep_time)
    
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    

def test_action_load_balance_outer_dst_ip():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()


    ##action config
    action_config =  { "1":{
                        "basis_actions":
                        {
                        "type": "load_balance",
                        "interfaces": ["X1","X2"],
                        "load_balance_weight": "",
                        "load_balance_mode": "outer_dst_ip"
                        }
                    }}

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config, port2_config]
    print("action_config")
    for i in action_config.items():
        print(i)
    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets" , load_balance_pkt_cnt  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , load_balance_pkt_cnt/2 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "in_packets" , 0 , port2_config)
    append_stat_target(stat_target , "interfaces/stat/"+port2_config , "out_packets" , load_balance_pkt_cnt/2 , port2_config)

    ## add pkts
    pkts.append("load_balance_dst_{0}.pcap".format(load_balance_pkt_cnt))


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_all_pkts(pkts_dir , pkts)

    time.sleep(sleep_time)
    
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # check if vpp is down
    check_vpp_stat(sf_helper)
