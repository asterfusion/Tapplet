
import re
import socket
import os 

###########################
#      IP string check
###########################
def is_valid_ipv4_address(address):
    try:
        addr= socket.inet_pton(socket.AF_INET, address)
    except AttributeError: # no inet_pton here, sorry
        try:
            addr= socket.inet_aton(address)
        except socket.error:
            return False
        return address.count('.') == 3
    except socket.error: # not a valid address
        return False

    return True

def is_valid_ipv6_address(address):
    try:
        addr= socket.inet_pton(socket.AF_INET6, address)
    except socket.error: # not a valid address
        return False
    return True

def check_ip_string(address):
    ret = is_valid_ipv4_address(address)
    if ret :
        return 0
    ret = is_valid_ipv6_address(address)
    if ret :
        return 1
    return -1

def trans_ip_to_bytes(dst , address):
    ret = check_ip_string(address)
    if ret == -1:
        return

    pbytes = None
    pblen = 4
    if ret == 0:
        pbytes = socket.inet_pton(socket.AF_INET, address)
    else:
        pbytes = socket.inet_pton(socket.AF_INET6, address)
        pblen = 16

    for i in range(pblen):
        dst[i] = pbytes[i]
        
def tran_ubytes_to_ip(src , ipversion):
    if ipversion == 4:
        b = bytearray(4)

        for i in range(4):
            b[i] = src[i]

        return socket.inet_ntop(socket.AF_INET , bytes(b))

    elif ipversion == 6:
        b = bytearray(16)

        for i in range(16):
            b[i] = src[i]

        return socket.inet_ntop(socket.AF_INET6 , bytes(b))

    else:
        return None
###########################
#      MAC check
###########################

def check_mac_sting(mac_addr):
    valid = re.compile(r"([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}")
    if valid.fullmatch(mac_addr) == None:
        return False
    return True

# dst : c_ubyte * 6 
def trans_mac_to_bytes(dst , mac_addr):
    hex_bytes = mac_addr.split(":")
    for i in range(0 , len(hex_bytes)):
        temp = "0x" + hex_bytes[i]
        value = int(temp , 16)
        dst[i] =  value
        # print("-------------")
        # print(i)
        # print(value)
        # print(test_mac.mac[i])

    # print( test_mac.mac )

def trans_ubytes_to_mac_addr(mac_bytes):
    result = str(hex(mac_bytes[0])).replace("0x" , "")
    if len(result) == 1:
        result =  "0" + result 
    for i in range(1 , 6):
        temp = hex(mac_bytes[i])
        value = temp.replace("0x" , "")
        if len(value) == 1:
            value = "0" + value
        result = result + ":" + value
    return result

###########################
#      vlan check
###########################
def check_vlan_id(vlan_id):
    if isinstance(vlan_id , int) == False:
        return False

    if vlan_id < 0 or vlan_id > 4095:
        return False
    
    return True

###########################
#      ether type check
###########################
def check_ether_type_hex_str(ether_type):
    if type(ether_type) != str:
        return False
    
    valid = re.compile(r"0x[A-Fa-f0-9]{4}")
    if valid.fullmatch(ether_type) == None:
        return False
    return True
