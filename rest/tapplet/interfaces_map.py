
from tornado.log import app_log

interfaces_map_to_se = {}
interfaces_map_to_rest = {}

MAX_NTERFACES_NUM = 0
MAX_REAL_INTERFACES_NUM = 0

target_device = None

## call it when shared mem is prepared
def update_real_interface_map(tmp_target_device , max_interface_num , real_interface_cnt):
    global interfaces_map_to_se
    global interfaces_map_to_rest 
    global MAX_NTERFACES_NUM 
    global MAX_REAL_INTERFACES_NUM 
    global target_device

    target_device = tmp_target_device
    if target_device == "VM":
        interfaces_map_to_se.clear()
        interfaces_map_to_rest.clear()
        
        for i in range(1 , real_interface_cnt + 1):
            temp_str = "G{0}".format(i)
            temp_id = i
            interfaces_map_to_se.update({temp_str : temp_id})
            interfaces_map_to_rest.update({temp_id : temp_str})
    else :
        app_log.error("device_type[{0}] not surpported....".format(target_device))
        exit(-1)

    MAX_NTERFACES_NUM = max_interface_num
    MAX_REAL_INTERFACES_NUM = len(interfaces_map_to_rest)


def get_interfaces_map_to_se():
    return interfaces_map_to_se

def get_interfaces_map_to_rest():
    return interfaces_map_to_rest    


def get_MAX_NTERFACES_NUM():
    return MAX_NTERFACES_NUM

def get_MAX_REAL_INTERFACES_NUM():
    return MAX_REAL_INTERFACES_NUM    

def get_target_device():
    return target_device

def trans_interface_name_to_index(intf_name):
    if  intf_name not in interfaces_map_to_se.keys():
        return 0
    return interfaces_map_to_se[intf_name]

def trans_interface_index_to_name(intf_index):
    if  intf_index not in interfaces_map_to_rest.keys():
        return "Not found"
    return interfaces_map_to_rest[intf_index]

def check_interface_index(index):
    if index != None:
        for i in index:
            if int(i) > MAX_REAL_INTERFACES_NUM or int(i) <= 0 :
                return True
    return False