
import os
import math
from scapy.all import *

from pytest_main import eth_config
from pytest_main import dump_eth_config

def getstatusoutput(cmd):
    pipe = os.popen(cmd + " 2>&1", 'r')
    text = pipe.read()
    sts = pipe.close()
    if sts is None: sts=0
    if text[-1:] == "\n": text = text[:-1]
    return sts, text

def getoutput(cmd):
    result = getstatusoutput(cmd)
    return result[1]

def send_all_pkts( pkts_dir , pkts_list):
    for pkt_name in pkts_list:
        # print("send pkt : " + pkts_dir + pkts_list)
        ret = getoutput("/bin/bash ./tools/send_pkt_raw.sh " + eth_config + " " + pkts_dir + pkt_name)
        ret_value = ret.split('\n')[-1]
        # print(ret_value)
        assert int(ret_value) == 0

tcpdump_output="./result/tresult.pcap"
def send_pkts_with_tcpdump( pkts_dir , pkt_name , waittime = 1):
    sh_str_tmp = "/bin/bash ./tools/send_pkt_with_tcpdump.sh {0} {1} {2} {3} {4} 2>&1; echo send_pkt_with_tcpdump_result:$?"
    sh_str = sh_str_tmp.format(eth_config , tcpdump_output  ,  pkts_dir+pkt_name , dump_eth_config, waittime)
    print(sh_str)
    ret = getoutput(sh_str)
    ret_value_list = ret.split('\n')
    ret_value = -1
    for line in ret_value_list:
        if line.find("send_pkt_with_tcpdump_result") != -1:
            tmp = line.split(":")
            ret_value = int(tmp[1])
    # print(ret)
    assert int(ret_value) == 0

#this function user tcpdump  options -Q  so:tcpdump version must >4.9.1
def send_pkts_with_tcpdump_with_direction( pkts_dir , pkt_name , direction = "inout" ,waittime = 1):
    sh_str_tmp = "/bin/bash ./tools/send_pkt_with_tcpdump_with_direction.sh {0} {1} {2} {3} {4} {5} 2>&1; echo send_pkt_with_tcpdump_result_with_direction:$?"
    sh_str = sh_str_tmp.format(eth_config , tcpdump_output  ,  pkts_dir+pkt_name , dump_eth_config, direction ,waittime)
    print(sh_str)
    ret = getoutput(sh_str)
    ret_value_list = ret.split('\n')
    ret_value = -1
    for line in ret_value_list:
        if line.find("send_pkt_with_tcpdump_result_with_direction") != -1:
            tmp = line.split(":")
            ret_value = int(tmp[1])
    assert int(ret_value) == 0

def pkt_tcpdump_count(count = 1):
    sh_str = "/bin/bash ./tools/pkt_tcpdump.sh {0} {1} {2} 2>&1; echo pkt_tcp_dump_result:$?".format( dump_eth_config, tcpdump_output, count)
    print(sh_str)
    ret = getoutput(sh_str)
    ret_value_list = ret.split('\n')
    ret_value = -1
    for line in ret_value_list:
        if line.find("pkt_tcp_dump_result") != -1:
            tmp = line.split(":")
            ret_value = int(tmp[1])
    assert int(ret_value) == 0

def check_pkt_content(captured_pkt_list , check_pkts):
    print(captured_pkt_list)
    print(check_pkts)
    assert len(captured_pkt_list) == len(check_pkts)
    for i in range(len(check_pkts)):
        a = raw(captured_pkt_list[i])
        b = raw(check_pkts[i])
        assert a == b

# editcap_num : New packages are generated from the <editcap_num> package
def editcap_pkt_num(pkt, begin_editcap_num, end_edtcap_num = None):
    if end_edtcap_num != None:
        editcap_str =  "editcap -r {0}  {0} {1}-{2}".format(pkt, begin_editcap_num, end_edtcap_num)
    else:
        editcap_str =  "editcap -r {0}  {0} {1}".format(pkt, begin_editcap_num)
    ret = getstatusoutput(editcap_str)
    assert ret[0] >= 0


