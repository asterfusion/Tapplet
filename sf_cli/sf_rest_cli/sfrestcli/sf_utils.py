from . rest_helper import *
from . globalValue import *

import json


def try_int_input(int_str):
    try:
        a = int(int_str)
    except Exception as e:
        return None
    return a


def try_json_input(json_str):
    try:
        # print(json_str)

        tmpdict = json.loads(json_str)
        # print(tmpdict)
    except Exception as e:
        return None
    return tmpdict

def try_json_file_input(json_file):
    try:
        fd = open(json_file)
        json_str = fd.read()
        fd.close()
    except Exception as e:
        return None
    return json_str

######### trans_dict_to_op #######


def format_op_dict(path_value_dict):
    tmp_list = []
    for tmpkey in path_value_dict.keys():
        tmpvalue = path_value_dict[tmpkey]
        tmpdict = {"op" : "replace" , "path" : tmpkey , "value" : tmpvalue }
        tmp_list.append(tmpdict)
    return tmp_list

def get_path_to_value(prefix_str  , src_dict):
    result_dict = {}
    for tmpkey in src_dict.keys():
        tmpvalue = src_dict[tmpkey]
        tmp_str = prefix_str + tmpkey
        if isinstance(tmpvalue , dict):
            result_dict.update(get_path_to_value(tmp_str + "/" , tmpvalue))
        else:
            result_dict.update({ tmp_str :  tmpvalue  })
    
    return result_dict


def trans_dict_to_path_value(src_dict):
    # path_value_dict = {}
    for tmpkey in src_dict.keys():
        if tmpkey == "":
            path_value_dict = { "/" : src_dict[tmpkey]}
            return path_value_dict

    path_value_dict = get_path_to_value("/"  , src_dict )
    # print(path_value_dict)
    return path_value_dict


def trans_dict_to_op(data , path):
    if path == "":
        exit("path is empty")
    path_value_dict = {path : data}
    # print(path_value_dict)
    return format_op_dict(path_value_dict)
    
    
######### ################ #######



def format_args(arguments):
    # resultdict = {"params:" : {} , "data" : {}}
    params = {}
    data = {}
    if arguments["--group"] != None:
        params.update( {  "group"  :  arguments["--group"] })
    
    if arguments["--index"] != None:
        params.update( {  "index"  :  arguments["--index"] })

    if arguments["--keys"] != None:
        params.update( {  "keys"  :  arguments["--keys"] })

    if arguments["--type"] != None:
        params.update( {  "type"  :  arguments["--type"] })
    
    if arguments["--depth"] != None:
        params.update( {  "depth"  :  arguments["--depth"] })


    if arguments["--int-value"]:
        if try_int_input(arguments["<patch_value>"]) == None:
            exit("invalid int value : {0}".format(arguments["<patch_value>"]))
        data =  int(arguments["<patch_value>"])
    if arguments["--str-value"]:
        data =  arguments["<patch_value>"]


    if arguments["--json"]:
        json_str =  arguments["<patch_value>"]
        tmpdict = try_json_input( json_str ) 
        if tmpdict == None:
            exit("invalid json string")
        if isinstance( tmpdict , dict) == False:
            exit("invalid json string : input should be a dictionary")
        data.update(tmpdict)
        
    if arguments["--json-file"] : 
        json_str =  try_json_file_input(arguments["<json_file>"])
        # here : if json str is None , tmpdict is None too
        tmpdict = try_json_input( json_str ) 
        if tmpdict == None:
            exit("invalid json string")
        if isinstance( tmpdict , dict) == False:
            exit("invalid json string : input should be a dictionary")
        data.update(tmpdict)
        
    if arguments["patch"]:
        tmpdata = trans_dict_to_op(data , arguments["<patch_path>"])
        data = tmpdata


    return params , data
    



'''   
    if arguments["--json"] : 
        json_str =  arguments["<json_str>"]
        tmpdict = try_json_input( json_str ) 
        if tmpdict == None:
            exit("invalid json string")
        if isinstance( tmpdict , dict) == False:
            exit("invalid json string : input should be a dictionary")

        data.update(tmpdict)
        
    if arguments["--json-file"] : 
        json_str =  try_json_file_input(arguments["<json_file>"])
        # here : if json str is None , tmpdict is None too
        tmpdict = try_json_input( json_str ) 
        if tmpdict == None:
            exit("invalid json string")
        if isinstance( tmpdict , dict) == False:
            exit("invalid json string : input should be a dictionary")
        data.update(tmpdict)
'''




