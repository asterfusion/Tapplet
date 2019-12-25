#!/usr/bin/env python3


import pytest
import argparse
import configparser
from tools.rest_tools import *
from tools.rest_helper import *
parser = argparse.ArgumentParser(description="Single test")

parser.add_argument( '-m', '--mod_name', help="exec only one directory under tapplet/")  
parser.add_argument( '-f', '--test_func', help="exec only one test")
parser.add_argument( '-V', '--verbose' , action="store_true", help="show more info, default is false")
parser.add_argument( '-v', '--pytest_verbose' , action="store_true", help="show more pytest info, default is false")
parser.add_argument( '-s', '--sw' , action="store_true", help="exit on test fail, continue from last failing test next time")
parser.add_argument( '-l', '--list' , action="store_true", help="list all test case, not execute")
parser.add_argument( '-n', '--count' , default="3", help="test counts/loops, default is 3")
parser.add_argument( '-L', '--long_time' , action="store_true", help="run long time tests")

# parser.add_argument( '-f', '--function')

args = parser.parse_args()

mod_name =  args.mod_name
test_func = args.test_func
global_verbose= args.verbose
pytest_verbose= args.pytest_verbose
global_sw = args.sw
global_list = args.list
global_count = args.count
long_time_test = args.long_time

global_config = configparser.ConfigParser() 
global_config.read("global.cfg", encoding = "utf-8")
host_config = global_config.get("auto_test", "host")
eth_config = global_config.get("auto_test", "eth")
dump_eth_config = global_config.get("auto_test", "dump_eth")
port1_config = global_config.get("auto_test", "port1")
port2_config = global_config.get("auto_test", "port2")
device_config = global_config.get("auto_test", "device")

device_config_list = ["VM"]

###### initial login ######
sf_helper = Sf_rest(host_config)
try_rest_login(sf_helper)

def check_device_config():
    if device_config not in device_config_list:
        print("device_config not right!")
        print("supported device:")
        print(device_config_list)
        exit(-1)

if __name__ == "__main__":
    
    check_device_config()
    
    addopt = []
    if global_verbose is True:
        addopt.append("-v")
    if pytest_verbose is True and global_verbose is False:
        addopt.append("-v")
    if global_sw is True:
        addopt.append("--sw")
    if global_list is True:
        addopt.append("--collect-only")

    if mod_name != None:
        mod_name = "tapplet/"+mod_name
        addopt.append(mod_name)

        pytest.main(addopt)
    elif test_func != None:
        addopt.append(test_func)
        pytest.main(addopt)
    else:
        for i in range(int(global_count)):
            print("############   {0}    ############".format(i+1))
            ret = pytest.main(addopt)
            if ret != 0:
                break
            if global_list is True:
                break
    ###### log out ######
    try_rest_logout(sf_helper)

#pytest.main(["gtpcv1"])
