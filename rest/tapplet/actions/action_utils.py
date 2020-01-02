from ctypes import *
import json

from tapplet.nshellutils import *
from tapplet.actions.action_class import *
from tapplet.interfaces_map import *

from tornado.log import app_log

from conf.globalValue import *

from utils.net_tools import *


def check_action_exists(action_id):
    if so.check_action_if_exist(int(action_id)) > 0:
        return True
    return False

#############################################

def check_dict_data_within_keys(data , valid_keys , must_exist=True):
    if isinstance(data , dict)  == False:
        return False
    for key in data.keys():
        if key not in valid_keys:
            return (key , 1)

    if must_exist == True:
        for key in valid_keys:
            if key not in data.keys():
                return (key , -1)

    return True


def trans_action_content_addi_actions(action_id , data , action_struct):
    if isinstance(data , dict) == False:
        return (Httplib.BAD_REQUEST , "action {0} : invalid additional_actions".format(action_id) )    

    for key in data.keys():
        if key not in request_addi_action_keys:
            return (Httplib.BAD_REQUEST , "action {0} : invalid key {1}".format(action_id , key) )   

        if  isinstance(data[key] , dict)  == False:
            return (Httplib.BAD_REQUEST , "action {0} : invalid content :  {1}".format(action_id, key) )    
        if "switch" not in data[key].keys():
            return (Httplib.BAD_REQUEST , "action {0} : no switch in:  {1}".format(action_id, key) )  
        
        ## notice !!
        if  data[key]["switch"] != 1:
            continue
        
        ##############################
        if key == "gre_encapsulation":
            ret = check_dict_data_within_keys(data[key] , request_addi_gre_encap_keys , must_exist=False)
            if ret != True:
                return (Httplib.BAD_REQUEST , "action {0} : invalid gre_encap cfg".format(action_id) )    

            if "gre_dip" not in data[key].keys():
                return (Httplib.BAD_REQUEST , "action {0} : no gre_encap ip".format(action_id) )    
            if "gre_dmac" not in data[key].keys():
                return (Httplib.BAD_REQUEST , "action {0} : no gre_encap dmac".format(action_id) )    
   
            if "gre_dscp" not in data[key].keys():
                data[key]["gre_dscp"] = 0

            ret =  check_ip_string(data[key]["gre_dip"])
            if ret == -1:
                return (Httplib.BAD_REQUEST , "action {0} : invalid gre_encap ip".format(action_id) )    
            
            ###@@@ set gre_dip_type
            if ret == 0:
                action_struct.additional_actions.gre_dip_type = 4
            else:
                action_struct.additional_actions.gre_dip_type = 6

            if check_mac_sting(data[key]["gre_dmac"]) == False:
                return (Httplib.BAD_REQUEST , "action {0} : invalid gre_encap mac".format(action_id) )    

            if str(data[key]["gre_dscp"]).isdigit() == False or int(data[key]["gre_dscp"]) < addi_dscp_min or int(data[key]["gre_dscp"]) > addi_dscp_max:
                return (Httplib.BAD_REQUEST , "action {0} : invalid gre_encap dscp".format(action_id) )  

            ###@@@ set gre_dip  gre_dmac .....
            trans_ip_to_bytes( action_struct.additional_actions.gre_dip.ipv6_byte , data[key]["gre_dip"] )
            trans_mac_to_bytes( action_struct.additional_actions.gre_dmac , data[key]["gre_dmac"])
            action_struct.additional_actions.gre_dscp = int(data[key]["gre_dscp"]) << 2

            action_struct.additional_actions.additional_switch.switch.flag_gre_encapsulation = 1
        ##############################
        elif key == "vxlan_encapsulation":
            ret = check_dict_data_within_keys(data[key] , request_addi_vxlan_encap_keys, must_exist=False)
            if ret != True:
                return ( Httplib.BAD_REQUEST , "action {0} : invalid vxlan_encap cfg".format(action_id) )   

            if "vxlan_dip" not in data[key].keys():
                return (Httplib.BAD_REQUEST , "action {0} : no vxlan_encap ip".format(action_id) )    
            if "vxlan_dmac" not in data[key].keys():
                return (Httplib.BAD_REQUEST , "action {0} : no vxlan_encap dmac".format(action_id) )    
            if "vxlan_dport" not in data[key].keys():
                return (Httplib.BAD_REQUEST , "action {0} : no vxlan_encap dport".format(action_id) )  

            if "vxlan_vni" not in data[key].keys():
                data[key]["vxlan_vni"] = 0

            if "vxlan_dscp" not in data[key].keys():
                data[key]["vxlan_dscp"] = 0
            

            # vxlan_dip
            ret =  check_ip_string( data[key]["vxlan_dip"] )
            if ret == -1:
                return ( Httplib.BAD_REQUEST , "action {0} : invalid vxlan_encap ip".format(action_id) )   

            ###@@@ set vxlan_dip_type
            if ret == 0:
                action_struct.additional_actions.vxlan_dip_type = 4
            else:
                action_struct.additional_actions.vxlan_dip_type = 6

            # vxlan_dmac
            ret =  check_mac_sting( data[key]["vxlan_dmac"] )
            if ret == False:
                return ( Httplib.BAD_REQUEST , "action {0} : invalid vxlan_encap mac".format(action_id) )   

            # vxlan_dscp
            if str(data[key]["vxlan_dscp"]).isdigit() == False or int(data[key]["vxlan_dscp"]) < addi_dscp_min or int(data[key]["vxlan_dscp"]) > addi_dscp_max:
                return (Httplib.BAD_REQUEST , "action {0} : invalid vxlan_encap dscp".format(action_id) ) 

            # vxlan_vni
            if str(data[key]["vxlan_vni"]).isdigit() == False or int(data[key]["vxlan_vni"]) < addi_vni_min or int(data[key]["vxlan_vni"]) > addi_vni_max:
                return (Httplib.BAD_REQUEST , "action {0} : invalid vxlan_encap vxlan_vni".format(action_id) ) 
            
            # vxlan_dport
            if str(data[key]["vxlan_dport"]).isdigit() == False or int(data[key]["vxlan_dport"]) < 0 or int(data[key]["vxlan_dport"]) > 65535:
                return (Httplib.BAD_REQUEST , "action {0} : invalid vxlan_encap vxlan_dport".format(action_id) ) 

            ###@@@ set vxlan 
            trans_ip_to_bytes( action_struct.additional_actions.vxlan_dip.ipv6_byte , data[key]["vxlan_dip"] )
            trans_mac_to_bytes( action_struct.additional_actions.vxlan_dmac , data[key]["vxlan_dmac"])
            action_struct.additional_actions.vxlan_dscp = int(data[key]["vxlan_dscp"]) << 2

            so.set_additional_vxlan_vni( byref(action_struct) , int(data[key]["vxlan_vni"]) )
            so.set_additional_vxlan_dport( byref(action_struct) , int(data[key]["vxlan_dport"]) )

            action_struct.additional_actions.additional_switch.switch.flag_vxlan_encapsulation = 1
        ##############################
        elif key == "erspan_encapsulation":
            ret = check_dict_data_within_keys( data[key] , request_addi_erspan_encap_keys, must_exist=False )
            if ret != True:
                return ( Httplib.BAD_REQUEST , "action {0} : invalid erspan_encap cfg".format(action_id) )             

            if "erspan_dip" not in data[key].keys():
                return (Httplib.BAD_REQUEST , "action {0} : no erspan_encap ip".format(action_id) )    
            if "erspan_dmac" not in data[key].keys():
                return (Httplib.BAD_REQUEST , "action {0} : no erspan_encap dmac".format(action_id) )    


            if "erspan_session_id" not in data[key].keys():
                data[key]["erspan_session_id"] = 0

            if "erspan_type" not in data[key].keys():
                data[key]["erspan_type"] = type_of_erspan_list[0]

            if "erspan_dscp" not in data[key].keys():
                data[key]["erspan_dscp"] = 0

            # erspan_dip
            ret =  check_ip_string( data[key]["erspan_dip"] )
            if ret == -1:
                return ( Httplib.BAD_REQUEST , "action {0} : invalid erspan_encap ip".format(action_id) )   

            ###@@@ set erspan_dip_type
            if ret == 0:
                action_struct.additional_actions.erspan_dip_type = 4
            else:
                action_struct.additional_actions.erspan_dip_type = 6

            # erspan_dmac
            ret =  check_mac_sting( data[key]["erspan_dmac"] )
            if ret == False:
                return ( Httplib.BAD_REQUEST , "action {0} : invalid erspan_encap mac".format(action_id) )   

            # erspan_dscp
            if str(data[key]["erspan_dscp"]).isdigit() == False or int(data[key]["erspan_dscp"]) < addi_dscp_min or int(data[key]["erspan_dscp"]) > addi_dscp_max:
                return (Httplib.BAD_REQUEST , "action {0} : invalid erspan_encap dscp".format(action_id) ) 

            # erspan_vni
            if str(data[key]["erspan_session_id"]).isdigit() == False or  int(data[key]["erspan_session_id"]) < addi_erspan_ses_id_min or  int(data[key]["erspan_session_id"]) > addi_erspan_ses_id_max:
                return (Httplib.BAD_REQUEST , "action {0} : invalid erspan_encap erspan_session_id".format(action_id) ) 
            
            # erspan_type
            if data[key]["erspan_type"] not in type_of_erspan_list:
                return (Httplib.BAD_REQUEST , "action {0} : invalid erspan_encap erspan_type".format(action_id) ) 

            ###@@@ set erspan
            trans_ip_to_bytes( action_struct.additional_actions.erspan_dip.ipv6_byte , data[key]["erspan_dip"] )
            trans_mac_to_bytes( action_struct.additional_actions.erspan_dmac , data[key]["erspan_dmac"])
            action_struct.additional_actions.erspan_dscp = int(data[key]["erspan_dscp"]) << 2
            action_struct.additional_actions.erspan_session_id = int(data[key]["erspan_session_id"])
            action_struct.additional_actions.erspan_type = type_of_erspan_list.index(data[key]["erspan_type"]) + 1

            action_struct.additional_actions.additional_switch.switch.flag_erspan_encapsulation = 1             


    return (Httplib.OK , None)

def trans_action_content_basic_load_balance(action_struct , action_id , intf_cnt , load_balance_mode , load_balance_weight):
    if load_balance_mode not in request_load_balance:
        return (Httplib.BAD_REQUEST , "action {0} : invalid load_balance_mode".format(action_id) )

    ###@@ set load_balance_mode
    action_struct.load_balance_mode = load_balance_mode_map_to_se[load_balance_mode]

    if load_balance_mode == "wrr":
        if load_balance_weight == None:
            return (Httplib.BAD_REQUEST , "action {0} : no load_balance_weight".format(action_id) )
        if isinstance(load_balance_weight , str) == False:
            return (Httplib.BAD_REQUEST , "action {0} : invalid load_balance_weight".format(action_id) )
        
        weight_array = load_balance_weight.split(":")
        if len(weight_array) != intf_cnt:
            return  (Httplib.BAD_REQUEST , "action {0} : interface cnt does not match load_balance_weight".format(action_id) )

        weight_int_array = []
        for weight in weight_array:
            if weight.isdigit() == False:
                return  (Httplib.BAD_REQUEST , "action {0} : invalid load_balance_weight".format(action_id) )
            
            if int(weight) <= 0:
                return  (Httplib.BAD_REQUEST , "action {0} : invalid load_balance_weight".format(action_id) )

            weight_int_array.append(int(weight))
        
        ###@@ set load_balance_weight
        add_array_value(action_struct.load_balance_weight , weight_int_array , order=False)


    else:
        if load_balance_weight != None and load_balance_weight != "":
            return (Httplib.BAD_REQUEST , "action {0} : invalid load_balance_weight".format(action_id) )
    
    return (Httplib.OK , None)

def trans_action_content_basic_actions(action_id , data  , action_struct):
    if isinstance(data , dict) == False:
            return (Httplib.BAD_REQUEST , "action {0} : invalid basis_actions".format(action_id) )
    
    ## check keys 
    for key in data.keys():
        if key not in request_basic_action_keys:
            return (Httplib.BAD_REQUEST , "action {0} : unknown key '{1}'".format(action_id , key) )  

    ## check type
    if  "type" not in data.keys():
        return (Httplib.BAD_REQUEST , "action {0} : no 'type' in basis_actions".format(action_id) )    

    if data["type"] not in request_action_type:
        return (Httplib.BAD_REQUEST , "action {0} : invalid 'type' in basis_actions".format(action_id) )    

    ###@@@ set type
    action_struct.type = action_type_map_to_se[ data["type"] ]

    ## check drop type 
    if data["type"] == "drop":
        if "interfaces" in data.keys():
            if data["interfaces"] != []:
                return (Httplib.BAD_REQUEST , "action {0} : invalid 'interfaces' in basis_actions".format(action_id) )   
        if "load_balance_weight" in data.keys():
            if data["load_balance_weight"] != "":
                return (Httplib.BAD_REQUEST , "action {0} : invalid 'load_balance_weight' in basis_actions".format(action_id) )   
        if "load_balance_mode" in data.keys():
            if data["load_balance_mode"] != "":
                return (Httplib.BAD_REQUEST , "action {0} : invalid 'load_balance_mode' in basis_actions".format(action_id) )   

        return (Httplib.OK , None)

    ## not drop , check interfaces
    if "interfaces" not in data.keys():   
        return (Httplib.BAD_REQUEST , "action {0} : no 'interfaces' in basis_actions".format(action_id) )   

    if isinstance(data["interfaces"] , list) == False or len(data["interfaces"]) == 0:
        return (Httplib.BAD_REQUEST , "action {0} : invalid 'interfaces' in basis_actions".format(action_id) )   

    for  intf_name in data["interfaces"]:
        if trans_interface_name_to_index(intf_name) == 0:
            return (Httplib.BAD_REQUEST , "action {0} : invalid interfaces '{1}'".format(action_id , intf_name) )   
        
    ## check forward type 
    if data["type"] == "forward":
        if "load_balance_weight" in data.keys():
            if data["load_balance_weight"] != "":
                return (Httplib.BAD_REQUEST , "action {0} : invalid 'load_balance_weight' in basis_actions".format(action_id) )   
        if "load_balance_mode" in data.keys():
            if data["load_balance_mode"] != "":
                return (Httplib.BAD_REQUEST , "action {0} : invalid 'load_balance_mode' in basis_actions".format(action_id) )   

        if len(data["interfaces"]) != 1:
            return (Httplib.BAD_REQUEST , "action {0} : too many 'interfaces' in basis_actions".format(action_id) )  

        ###@@@ set intf
        action_struct.forward_interface_id = trans_interface_name_to_index(data["interfaces"][0])

        return (Httplib.OK , None)

    ## check load_balance type 
    if data["type"] == "load_balance":
        load_balance_weight_tmp = None
        load_balance_mode_tmp = None
        if "load_balance_weight"  in data.keys():
            load_balance_weight_tmp = data["load_balance_weight"]
        if "load_balance_mode" not in data.keys() or data["load_balance_mode"] == "":
            return (Httplib.BAD_REQUEST , "action {0} : no 'load_balance_mode' ".format(action_id) )   
        else:
            load_balance_mode_tmp = data["load_balance_mode"]

        ret = trans_action_content_basic_load_balance(action_struct , action_id ,  len(data["interfaces"]) ,  load_balance_mode_tmp , load_balance_weight_tmp)
        if ret[0] != Httplib.OK:
            return ret

        ###@@@ set intf
        intf_ids = []
        for intf_name in data["interfaces"]:
            intf_ids.append( trans_interface_name_to_index(intf_name) )

        add_array_value(action_struct.load_balance_interface_array , intf_ids , order=False)
        action_struct.load_balance_interfaces_num = len(intf_ids)

        return (Httplib.OK , None)    


    return (Httplib.INTERNAL_SERVER_ERROR , "check_action_content_basic_actions")    


def trans_dict_to_action_struct(action_id , data , action_struct):
    if isinstance(data , dict) == False:
        return (Httplib.BAD_REQUEST , "action {0} : invalid config".format(action_id) )

    for key in data.keys():
        if key not in ["basis_actions" , "additional_actions"]:
            return (Httplib.BAD_REQUEST , "action {0} : unknown key {1}".format(action_id , key) )

    if "basis_actions" not in data.keys():
        return (Httplib.BAD_REQUEST , "action {0} : no basis_actions".format(action_id) )

    ret = trans_action_content_basic_actions(action_id , data["basis_actions"] , action_struct)

    if ret[0] != Httplib.OK:
        return ret

    if "additional_actions" in data.keys():
        ret = trans_action_content_addi_actions(action_id , data["additional_actions"] , action_struct)

        if ret[0] != Httplib.OK:
            return ret

    return  (Httplib.OK , None)    

def reset_action_struct_content(action_struct):
    so.reset_action_config_struct(byref(action_struct))

def update_action_content(method , data):
    result_dict = {}
    action_struct = Action()

    for index in data.keys():
        reset_action_struct_content(action_struct)

        if index.isdigit() == False:
            result_dict.update({index : "invalid action id : {0}".format(index) })
        if int(index) <= 0 or int(index) > MAX_ACTIONS_NUM:
            result_dict.update({index : "action id out of range : {0}".format(index) })

        if method == "post":
            if check_action_exists(int(index)):
                result_dict.update({index : "action {0} already exist".format(index) })
                continue


        ret = trans_dict_to_action_struct( int(index) ,  data[index] , action_struct)
        if ret[0] != Httplib.OK:
            result_dict.update({index : ret[1] })
            continue
        
        ret = so.put_action_config(byref(action_struct) , int(index))
        if ret < 0 :
            result_dict.update({index : "Internal Error" })

    if len(result_dict.keys()) == 0:
        return None

    return result_dict


##########################################################
#                                                        #
##########################################################


def format_action_basic_action(action_struct):
    result_dict = {}
    result_dict.update({"type" : action_type_map_to_rest[action_struct.type]})

    intf_array = []

    if result_dict["type"] == "forward":
        intf_array.append( trans_interface_index_to_name(action_struct.forward_interface_id) )
    elif result_dict["type"] == "load_balance":
        for i in range(action_struct.load_balance_interfaces_num):
            intf_array.append( trans_interface_index_to_name(action_struct.load_balance_interface_array[i]) )

        load_balance_mode_str = load_balance_mode_map_to_rest[action_struct.load_balance_mode] 
        result_dict.update({"load_balance_mode" : load_balance_mode_str})

        if load_balance_mode_str == "wrr":
            load_balance_weight_array = []
            for i in range(action_struct.load_balance_interfaces_num):
                load_balance_weight_array.append(str(action_struct.load_balance_weight[i]))
            
            result_dict.update({"load_balance_weight" : ":".join(load_balance_weight_array)})

    if len(intf_array) != 0:
        result_dict.update({"interfaces" : intf_array})

    return result_dict
        

def format_action_addi_action(action_id , action_struct):
    result = {}
    for key in request_addi_action_keys:
        if eval("action_struct.additional_actions.additional_switch.switch.flag_{0}".format(key)) == 0:
            continue
        
        inner_dict = {"switch" : 1}
        
        ##############################
        if key == "gre_encapsulation":
            ## gre_dmac
            inner_dict["gre_dmac"] = trans_ubytes_to_mac_addr(action_struct.additional_actions.gre_dmac)
            ## gre_dip
            inner_dict["gre_dip"] = tran_ubytes_to_ip(action_struct.additional_actions.gre_dip.ipv6_byte , 
                action_struct.additional_actions.gre_dip_type )
            ## gre_dscp
            inner_dict["gre_dscp"] = action_struct.additional_actions.gre_dscp  >> 2

        ##############################
        elif key == "vxlan_encapsulation":
            ## vxlan_dmac
            inner_dict["vxlan_dmac"] = trans_ubytes_to_mac_addr(action_struct.additional_actions.vxlan_dmac)
            ## vxlan_dip
            inner_dict["vxlan_dip"] = tran_ubytes_to_ip(action_struct.additional_actions.vxlan_dip.ipv6_byte , 
                action_struct.additional_actions.vxlan_dip_type )
            ## vxlan_dscp
            inner_dict["vxlan_dscp"] = action_struct.additional_actions.vxlan_dscp  >> 2

            ## vxlan_vni
            inner_dict["vxlan_vni"] = so.get_additional_vxlan_vni(action_id)

            ## vxlan_dport
            inner_dict["vxlan_dport"] = so.get_additional_vxlan_dport(action_id)

        ##############################
        elif key == "erspan_encapsulation":
            ## erspan_dmac
            inner_dict["erspan_dmac"] = trans_ubytes_to_mac_addr(action_struct.additional_actions.erspan_dmac)
            ## erspan_dip
            inner_dict["erspan_dip"] = tran_ubytes_to_ip(action_struct.additional_actions.erspan_dip.ipv6_byte , 
                action_struct.additional_actions.erspan_dip_type )
            ## erspan_dscp
            inner_dict["erspan_dscp"] = action_struct.additional_actions.erspan_dscp  >> 2

            ## erspan_session_id
            inner_dict["erspan_session_id"] = action_struct.additional_actions.erspan_session_id 

            ## erspan_type
            enrspan_type_index = action_struct.additional_actions.erspan_type - 1 
            if enrspan_type_index < 0 or enrspan_type_index >= len(type_of_erspan_list):
                inner_dict["erspan_type"] = None
            else:                    
                inner_dict["erspan_type"] = type_of_erspan_list[ enrspan_type_index ]

        result.update({key : inner_dict})

    return result

def get_single_action_content(action_id):
    action_struct = Action()
    if so.get_action_config(byref(action_struct) , int(action_id)) != 0:
        return "Internal Error"
    basis_action = format_action_basic_action(action_struct)
    addi_action = format_action_addi_action(int(action_id) , action_struct)
    if addi_action == {}:
        return {"basis_actions" : basis_action}
    return {"basis_actions" : basis_action , "additional_actions" : addi_action } 




def delete_action_libnshell(index):
    return so.delete_action_config(int(index))