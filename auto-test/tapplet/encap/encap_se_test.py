from tools.conftest_tools import *
from tools.rest_tools import *
from tools.tcpreplay_tools import *

from pytest_main import port1_config
from pytest_main import port2_config
from pytest_main import sf_helper

import time

mod_name = "actions"

pkts_dir = "./tapplet/encap/pkts/"

# class Test_gtpcv1_normal_cdr:
config_needed = []
stat_target = []
pkts = []

origin_pkts = "raw_pkt.pcap"

target_gre_pkts = "encap_gre.pcap"
target_vxlan_pkts = "encap_vxlan.pcap"
target_gre_v6_pkts = "encap_gre_ipv6.pcap"
target_vxlan_v6_pkts = "encap_vxlan_ipv6.pcap"

target_encap_erspan_type_I_pkts     =   "encap_erspan_I.pcap"
target_encap_erspan_type_II_pkts    =   "encap_erspan_II.pcap"
target_encap_erspan_type_III_pkts   =   "encap_erspan_III.pcap"
target_encap_erspan_I_ipv6_pkts     =   "encap_erspan_I_ipv6.pcap"
target_encap_erspan_II_ipv6_pkts    =   "encap_erspan_II_ipv6.pcap"
target_encap_erspan_III_ipv6_pkts   =   "encap_erspan_III_ipv6.pcap"


def check_result_pkts(target_pkt):
    global tcpdump_output
    captured_pkts = rdpcap(tcpdump_output)
    check_pkts = rdpcap(pkts_dir + target_pkt)

    cap_list = []
    for p in captured_pkts:
        if p["IP"].src == "10.0.0.14":
            cap_list.append(p)

    for i in range(len(check_pkts)):
        a = raw(cap_list[i])
        b = raw(check_pkts[i])
        #ingore gre_sequence_number and erspan time stamp
        if(target_pkt == target_encap_erspan_type_II_pkts):
            assert a[12:37] == b[12:37]
            assert a[42:] == b[42:]
        elif(target_pkt == target_encap_erspan_type_III_pkts):
            assert a[12:37] == b[12:37]
            assert a[42:45] == b[42:45]
            assert a[50:] == b[50:]
        else:
        # ignore mac
            assert a[12:] == b[12:]

def check_result_pkts_ipv6(target_pkt):
    global tcpdump_output
    captured_pkts = rdpcap(tcpdump_output)
    check_pkts = rdpcap(pkts_dir + target_pkt)

    cap_list = []
    for p in captured_pkts:
        if p["Ether"].dst == "66:55:55:55:55:55":
            cap_list.append(p)

    for i in range(len(check_pkts)):
        a = raw(cap_list[i])
        b = raw(check_pkts[i])
        #ingore gre_sequence_number and erspan time stamp
        if(target_pkt == 'encap_erspan_II_ipv6.pcap'):
            assert a[12:57] == b[12:57]
            assert a[62:] == b[62:]
        elif(target_pkt == 'encap_erspan_III_ipv6.pcap'):
            assert a[12:57] == b[12:57]
            assert a[62:65] == b[62:65]
            assert a[70:] == b[70:]
        else:
        # ignore mac
            assert a[12:] == b[12:]


def test_encap_gre():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv4_addr" , "10.0.0.14")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "", 
                "load_balance_mode": ""
            },
            "additional_actions": {
                "gre_encapsulation":{
                    "switch":1,
                    "gre_dmac": "ff:ff:ff:ff:11:27",
                    "gre_dip": "192.168.1.225",
                    "gre_dscp": 22
                    },
        }
    }
}
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts(target_gre_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    


def test_encap_vxlan():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv4_addr" , "10.0.0.14")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "vxlan_encapsulation":{
                    "switch":1,
                    "vxlan_dmac": "11:22:33:44:55:66",
                    "vxlan_dip": "123.123.123.123",
                    "vxlan_dport": 210,
                    "vxlan_vni": 11,
                    "vxlan_dscp": 22
                    }, 
        }
    }
}
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ###### start test #######



    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts(target_vxlan_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    
def test_encap_erspan_type_I():
    
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv4_addr" , "10.0.0.14")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "erspan_encapsulation":{
                    "switch":1,
                    "erspan_dmac":"66:55:55:55:55:55",
                    "erspan_dip":"192.168.1.100",
                    "erspan_session_id":666,
                    "erspan_type":"ERSPAN_I",
                    "erspan_dscp":22
                    },
            }       
        }
    }
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts(target_encap_erspan_type_I_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

def test_encap_erspan_type_II():
    
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv4_addr" , "10.0.0.14")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "erspan_encapsulation":{
                    "switch":1,
                    "erspan_dmac":"66:55:55:55:55:55",
                    "erspan_dip":"192.168.1.100",
                    "erspan_session_id":666,
                    "erspan_type":"ERSPAN_II",
                    "erspan_dscp":22
                    },
            }       
        }
    }
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts(target_encap_erspan_type_II_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

def test_encap_erspan_type_III():
    
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv4_addr" , "10.0.0.14")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "erspan_encapsulation":{
                    "switch":1,
                    "erspan_dmac":"66:55:55:55:55:55",
                    "erspan_dip":"192.168.1.100",
                    "erspan_session_id":666,
                    "erspan_type":"ERSPAN_III",
                    "erspan_dscp":22
                    },
            }       
        }
    }
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts(target_encap_erspan_type_III_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

def test_encap_gre_ipv6():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv6_addr" , "ccdd::eeff")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "", 
                "load_balance_mode": ""
            },
            "additional_actions": {
                "gre_encapsulation":{
                    "switch":1,
                    "gre_dmac": "66:55:55:55:55:55",
                    "gre_dip": "ffff::eeee",
                    "gre_dscp": 3
                    },
                }    
        }
    }

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts_ipv6(target_gre_v6_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    


def test_encap_vxlan_ipv6():
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv6_addr" , "ccdd::eeff")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "vxlan_encapsulation":{
                    "switch":1,
                    "vxlan_dmac": "66:55:55:55:55:55",
                    "vxlan_dip": "ffff::eeee",
                    "vxlan_dport": 210,
                    "vxlan_vni": 11,
                    "vxlan_dscp": 3
                    },
                }       
        }
    }

    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]


    ## add stat target

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts, 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts_ipv6(target_vxlan_v6_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

    
def test_encap_erspan_type_I_ipv6():
    
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv6_addr" , "ccdd::eeff")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "erspan_encapsulation":{
                    "switch":1,
                    "erspan_dmac":"66:55:55:55:55:55",
                    "erspan_dip":"ffff::eeee",
                    "erspan_session_id":666,
                    "erspan_type":"ERSPAN_I",
                    "erspan_dscp":22
                    },
            }       
        }
    }
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts_ipv6(target_encap_erspan_I_ipv6_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

def test_encap_erspan_type_II_ipv6():
    
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv6_addr" , "ccdd::eeff")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "erspan_encapsulation":{
                    "switch":1,
                    "erspan_dmac":"66:55:55:55:55:55",
                    "erspan_dip":"ffff::eeee",
                    "erspan_session_id":666,
                    "erspan_type":"ERSPAN_II",
                    "erspan_dscp":22
                    },
            }       
        }
    }
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts_ipv6(target_encap_erspan_II_ipv6_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

def test_encap_erspan_type_III_ipv6():
    
    ## clean config needed
    config_needed.clear()
    stat_target.clear()
    pkts.clear()

    ## add config needed
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/admin_status" , 1)
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ipv6_addr" , "ccdd::eeff")
    append_config_needed(config_needed , "interfaces/config" , "/"+port1_config+"/ingress_config/default_action_id" , 1)

    action_config =  { 
    "1":{
            "basis_actions":
            {
                "type": "forward",
                "interfaces": ["X1"],
                "load_balance_weight": "",
                "load_balance_mode": ""
            },
            "additional_actions": {
                "erspan_encapsulation":{
                    "switch":1,
                    "erspan_dmac":"66:55:55:55:55:55",
                    "erspan_dip":"ffff::eeee",
                    "erspan_session_id":666,
                    "erspan_type":"ERSPAN_III",
                    "erspan_dscp":22
                    },
            }       
        }
    }
    action_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]

    ## add stat target

    ## add pkts
    # pkts.append("cre_upd_del_sig.pcap")


    ###### start test #######

    
    

    # check if vpp is down
    check_vpp_stat(sf_helper)
    # clean up all config
    reset_all_mod_config(sf_helper)
    time.sleep(5)
    # dispatch config
    dispatch_test_config(sf_helper , config_needed)
    dispatch_post_config(sf_helper , "actions" , action_config)
    # clean  stat
    clean_test_stat(sf_helper , stat_target)
    # send pkts
    send_pkts_with_tcpdump(pkts_dir , origin_pkts , 5 )
    # check  stat
    check_test_stat(sf_helper , stat_target)
    # Optional : check  pkts content
    check_result_pkts_ipv6(target_encap_erspan_III_ipv6_pkts)
    # check if vpp is down
    check_vpp_stat(sf_helper)

