# -*- coding: UTF-8 -*-


from tools.conftest_tools import *
from tools.rest_tools import *
from tools.tcpreplay_tools import *

from pytest_main import port1_config
from pytest_main import port2_config
from pytest_main import sf_helper
from pytest_main import global_verbose
import time

pkts_dir = "./tapplet/ip_reass/pkts/"

config_needed = []
stat_target = []
pkts = []

sleep_time = 5
tcpdump_sleep_time = 10


ip4_udp_acl_config = {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple4",
                "rule_cfg":{
                    "dip":"101.66.248.130",
                    "dip_mask":32,
                    "dport_max":8899,
                    "dport_min":8899,
                    "proto_max":17,
                    "proto_min":17,
                    "sip":"192.168.1.160",
                    "sip_mask":32,
                    "sport_max":8874,
                    "sport_min":8874,
                }
            }
        }
    }
ip4_tcp_acl_config = {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple4",
                "rule_cfg":{
                    "dip":"101.66.248.130",
                    "dip_mask":32,
                    "dport_max":8899,
                    "dport_min":8899,
                    "proto_max":6,
                    "proto_min":6,
                    "sip":"192.168.1.160",
                    "sip_mask":32,
                    "sport_max":8874,
                    "sport_min":8874,
                }
            }
        }
    }

ip6_tcp_acl_config =  {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple6",
                "rule_cfg":{
                    "dip":"fe80::6157:6d05:4233:9bd7",
                    "dip_mask":128,
                    "dport_max":8899,
                    "dport_min":8899,
                    "proto_max":6,
                    "proto_min":6,
                    "sip":"fe80::841e:b96:4fb:b2d6",
                    "sip_mask":128,
                    "sport_max":8874,
                    "sport_min":8874,
                }
            }
        }
    }

ip6_udp_acl_config =  {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple6",
                "rule_cfg":{
                    "dip":"fe80::6157:6d05:4233:9bd7",
                    "dip_mask":128,
                    "dport_max":8899,
                    "dport_min":8899,
                    "proto_max":17,
                    "proto_min":17,
                    "sip":"fe80::841e:b96:4fb:b2d6",
                    "sip_mask":128,
                    "sport_max":8874,
                    "sport_min":8874,
                }
            }
        }
    }

action_1_config =  { 
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

def set_acl_sync(rest_helper):
    data = [ {"op" : "replace"  , "path" : "/" , "value" : 1}]
    ret = rest_helper.auto_run_no_login("acl/sync", GlobalRestValue.ACTION_PATCH , 
        data = data , verbose = global_verbose)
    
    assert ret[0] == 0

    time.sleep(4)

def check_reass_pkt_data_6000():
    global tcpdump_output
    captured_pkts = rdpcap(tcpdump_output)
    outpkt = None
    for p in captured_pkts:
        if p["Raw"] != None:
            if len(p["Raw"].load) == 6000:
                outpkt = p
    assert  outpkt != None    
    reass_data = p["Raw"].load
    for i in range(10 , len(reass_data)):
        assert reass_data[i] == reass_data[i-10]

def check_result_pkts_6000(target_pkt):
    global tcpdump_output
    captured_pkts = rdpcap(tcpdump_output)
    check_pkts = rdpcap(pkts_dir + target_pkt)

    cap_list = []
    for p in captured_pkts:
        if p["Raw"] != None:
            if len(p["Raw"].load) >= 6000:
                cap_list.append(p)

    assert len(cap_list) == 1

    a = raw(cap_list[0])
    b = raw(check_pkts[0])

    for i in range(len(check_pkts)):
        #outer ip id 
        if i == 18 or i == 19:
            continue
        #outer ip checksum 
        if i == 24 or i == 25:
            continue
        #outer udp checksum 
        if i == 40 or i == 41:
            continue
    
        assert a[i] == b[i]


def test_outer_ip4_frag():
    '''
    外层ipv4 分片转发测试
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
            "ip_reass_config":{
                "ip_reass_layers":1,
            },
        }
    }
    action_config = action_1_config
    acl_config = ip4_udp_acl_config

    # action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 4  , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 4 , port1_config)

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
    send_all_pkts(pkts_dir , ["ip4_frag_udp.pcap"])

    time.sleep(sleep_time)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    


def test_outer_ip6_frag():
    '''
    外层ipv6 分片转发测试
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
    action_config =  action_1_config
    acl_config = ip6_tcp_acl_config

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 5 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 5 , port1_config)

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
    send_all_pkts(pkts_dir , ["ip6_frag_tcp.pcap"])

    time.sleep(sleep_time)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    

def test_outer_frag_timeout():
    '''
    外层 分片超时 转发测试
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


    action_config =  action_1_config
    acl_config =  {
        "group_1":
        {
            "4":
            {
                "rule_type":"tuple6",
                "rule_cfg":{
                    "dip":"fe80::6157:6d05:4233:9bd7",
                    "dip_mask":128,
                    "dport_max":0,
                    "dport_min":0,
                    "proto_max":6,
                    "proto_min":6,
                    "sip":"fe80::841e:b96:4fb:b2d6",
                    "sip_mask":128,
                    "sport_max":0,
                    "sport_min":0,
                }
            }
        }
    }

    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 4 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 4 , port1_config)

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
    send_all_pkts(pkts_dir , ["ip6_frag_tcp_timeout.pcap"])

    time.sleep(sleep_time)
    # for ip reass timeout
    time.sleep(5)

    # check  stat
    check_test_stat(sf_helper , stat_target)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    


def test_ipv4_outer_frag_output():
    '''
    外层分片(v4) ， 合成并输出合成报文
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
                    "4":1,
                },
            },
            "interface_type" : "normal",
            "ip_reass_config":{
                "ip_reass_layers":1,
                "ip_reass_output_enable":1
            },

        }
    }


    action_config =  action_1_config
    acl_config =  ip4_udp_acl_config



    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 4 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 5 , port1_config)

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
    send_pkts_with_tcpdump(pkts_dir , "ip4_frag_udp.pcap" , tcpdump_sleep_time )

    # check  stat
    check_test_stat(sf_helper , stat_target)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    


    # check tresult.pcap 
    check_reass_pkt_data_6000()


def test_ipv6_outer_frag_output():
    '''
    外层分片(v6) ， 合成并输出合成报文
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
                    "4":1,
                },
            },
            "ip_reass_config":{
                "ip_reass_layers":1,
                "ip_reass_output_enable":1
            },
        }
    }


    action_config =  action_1_config
    acl_config =  ip6_tcp_acl_config



    ## add stat target
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "in_packets"  , 5 , port1_config)
    append_stat_target(stat_target , "interfaces/stat/"+port1_config , "out_packets" , 6 , port1_config)

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
    send_pkts_with_tcpdump(pkts_dir , "ip6_frag_tcp.pcap" , tcpdump_sleep_time )

    # check  stat
    check_test_stat(sf_helper , stat_target)

    # check if vpp is down
    check_vpp_stat(sf_helper)

    


    # check tresult.pcap 
    check_reass_pkt_data_6000()


