import pytest

default_type = [ "c_char", "c_char", "c_ubyte",
                 "c_short", "c_ushort", "c_int", "c_uint",
                 "c_long", "c_ulong", 
                 "c_longlong", "c_ulonglong", 
                 "c_float", "c_double"]

def get_ret_config_dict(name):
    mod_class_prefix = "".join(" ".join(name.split("_")).title().split())
    class_name = "{0}GlobalConfig".format(mod_class_prefix)
    class_fields = eval("{0}._fields_".format(class_name))
    tmp_dict = {}
    for i in class_fields:
        if i[0] == "reserved":
            continue
        if i[1].__name__ in default_type:
            tmp_dict.update({i[0]: None})  
        else:
            tmp_dict.update({i[0]: []})  
    return tmp_dict

def get_ret_stat_dict(name):
    mod_class_prefix = "".join(" ".join(name.split("_")).title().split())
    class_name = "{0}GlobalStat".format(mod_class_prefix)
    class_fields = eval("{0}._fields_".format(class_name))
    tmp_dict = {}
    for i in class_fields:
        if i[0] == "reserved":
            continue
        if i[1].__name__ in default_type:
            tmp_dict.update({i[0]: None})  
        else:
            tmp_dict.update({i[0]: []})  
    return tmp_dict

def append_config_needed(config_list , resource , path , value):
    config_map = {"url":resource , "path":path , "value":value}
    config_list.append(config_map)

def append_stat_target(stat_list , modname , key , value  , index=None):
    # stat_map = {"mod":modname , "key":key , "value":value}
    # stat_list.append(stat_map)
    find_flag = 0
    tmp_index = 0
    for item in stat_list:
        if item["mod"] == modname:
            find_flag = 1
            break
        tmp_index += 1 
    if find_flag == 1:
        item["keys"].update( {key : value} )
    else:
        if index:
            temp_dict = {"mod":modname , "keys" : {key:value}, "index": index}
        else:
            temp_dict = {"mod":modname , "keys" : {key:value}, "index": None}
        stat_list.append(temp_dict)
