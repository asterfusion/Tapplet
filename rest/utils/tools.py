import os
import json
import re
from tornado.options import options
from conf.globalValue  import *
from utils.apiException import ParseError

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

####Determines whether the file exists
def file_is_exist(file):
    return os.path.isfile(file)

####Determines whether the path exists
def path_is_exist(path):
    return os.path.exists(path)

def redirect_http_to_https(current_instance):
    if not options.force_https: 
        return True 
    if current_instance.request.protocol == HTTP: 
        #current_instance.redirect(re.sub(r'^([^:]+)', HTTPS, current_instance.request.full_url()), True) 
        url = re.sub(r'(:[1-9]+)', ":"+str(options.HTTPS_PORT),current_instance.request.full_url())
        current_instance.redirect(re.sub(r'^([^:]+)', HTTPS, url), True) 
        return True 
    return False 

def get_param_list(name, query_arguments):
    argsuments = []
    if query_arguments is not None and name in query_arguments:
        argsuments = query_arguments[name]

    values = []
    for arg in argsuments:
        split_args = arg.decode('utf-8').split(",")
        values.extend(split_args)
    return values


def decode_request_path_body(body):
    """
        patch request support:
            add, remove, replace, test

        [{"op": "add", "path": "/test/test_var", "value": "1"}]
        return [("add", {"test":{test_var": "1"}})]

        [{"op": "add", "path": "/test/test1_var/2", "value": "1"}]
        return [("add", {"test":{test_var": {2, "1"}}})]
    """
    try:
        body_decode = body.decode('utf-8')
        body_list = json.loads(body_decode)
        path_decode = []
        for action in body_list:
            resouce = {}
            key = action.keys()
            if "op" not in key:
                return -1
            if "path" not in key:
                return -1
            if "value" not in key:
                return -1
            if action["op"] != "add" and \
               action["op"] != "remove" and \
               action["op"] != "replace" and \
               action["op"] != "test":
                   return -1
            if action["path"] == "" or action["path"][0] != "/":
                return -1
            var_change = action["path"].split("/")[1:]
            var_depth = len(action["path"].split("/")[1:])
            value_change = action["value"]
            _pop = var_change.pop()
            if _pop == "":
                assert type(value_change) == dict or value_change == ""
                resouce = value_change
            else:
                resouce[_pop] = value_change
            var_depth -= 1
            while var_depth > 0:
                tmp = resouce.copy()
                resouce = {var_change.pop():tmp}
                var_depth -= 1
            path_decode.append((action["op"], resouce))
        return path_decode
    except:
        return None;

def decode_put_body(body):
    try:
        body_decode = body.decode('utf-8')
        body_dict = json.loads(body_decode)
        return body_dict
    except:
        return None;


def get_query_arg(name, query_arguments):
    arg = None
    if query_arguments is not None and name in query_arguments:
        arg = query_arguments[name][0].decode('utf-8')
    return arg


def get_keys_arg(query_arguments):
    keys_values = get_param_list(REST_PATAM_KEY, query_arguments)
    if keys_values:
        return keys_values

def parse_array_list(value):
    try:
        result_list = []
        first_list = value.split(",")
        for tmp_first in first_list:
            second_list = tmp_first.split("-")
            if len(second_list) == 1:
                result_list.append(int(second_list[0]))
            elif len(second_list) == 2:
                if second_list[0] == "" or second_list[1] == "":
                    raise ParseError("ParseError : " + value)
                start = int(second_list[0])
                end = int(second_list[1])
                if start >= end:
                    raise ParseError("ParseError : " + value)
                for i in range(start , end + 1):
                    result_list.append( i )
            else:
                raise ParseError("ParseError : " + value)
        result_list.sort()
        if len(result_list) != len(set(result_list)):
            raise ParseError("ParseError : " + value)
        return result_list
    except Exception as e:
        raise ParseError("ParseError : " + value)


def get_array_param_list(name, query_arguments):
    argsuments = []
    if query_arguments is not None and name in query_arguments:
        argsuments = query_arguments[name]

    values = []
    for arg in argsuments:
        split_args = parse_array_list(arg.decode("utf-8"))
        values.extend(split_args)
    return values

def get_index_arg(query_arguments , is_number = True):
    if is_number:
        index_values = get_array_param_list(REST_PATAM_INDEX, query_arguments)
    else:
        index_values = get_param_list(REST_PATAM_INDEX, query_arguments)
    if index_values:
        return index_values

def get_group_arg(query_arguments , is_number = True):
    if is_number:
        index_values = get_array_param_list(REST_PATAM_GROUP, query_arguments)
    else:
        index_values = get_param_list(REST_PATAM_GROUP, query_arguments)
    if index_values:
        return index_values

def get_type_arg(query_arguments):
    arg = None
    if query_arguments is not None and "type" in query_arguments:
        arg = query_arguments["type"][0].decode('utf-8')
    return arg

def get_depth_param(query_arguments):
    depth_param = get_query_arg(REST_PATAM_DEPTH, query_arguments)
    depth = 0
    if depth_param is not None:
        try:
            depth = int(depth_param)
            if depth  <  DEPTH_VALUE_MIN or depth > DEPTH_VALUE_MAX:
                error  = to_json_error("Depth patameter must be greater or equal than {0} and lower or equal than {1}".format(DEPTH_VALUE_MIN, DEPTH_VALUE_MAX))
        except ValueError: 
            error = to_json_error("Depth parameter must be a number")
            return {"Error":error}
    return depth


def to_json_error(message, code= None, fields= None):
    dicterror = {"code":None, "fields":None, "message":message}
    return dict_to_json(dicterror)

def pretty_floats(obj):
    if isinstance(obj, float):
        return round(obj, 3)
    elif isinstance(obj, dict):
        return dict((k, pretty_floats(v)) for k, v in obj.items())
    elif isinstance(obj, (list, tuple)):
        return list(map(pretty_floats, obj))
    return obj

def to_json(data, value_type = None):
    _type = type(data)
    if _type is dict:
        return dict_to_json(data)

    elif _type is list:
        return list_to_json(data)

    elif _type is bool:
        return data
        
    elif _type is type(None):
        return None

    else:
        return str(data)

def list_to_json(data):
    if not data:
        return data
    if isinstance(data, list):
        data_json = json.dumps(data)
        return data_json
    else:
        result = {"error to json":data}
        return json.dumps(result)


def dict_to_json(data):
    if not data:
        return data

    if isinstance(data, dict):
        return json.dumps(pretty_floats(data), sort_keys = True)
    else:
        result = {"error to json":data}
        return json.dumps(result)


def json_to_dict(string):
    if not string:
        return string

    try:
        return json.loads(string)  
    except Exception as e:
        return None


def combine_dict(dict1, *args):
    tmp = dict1
    if args:
        for i in args:
            if isinstance(i, dict):
                tmp = dict(tmp, **i)
    return tmp

def combine_dict_in_list(list1, list2):
    tmp = []
    _len =len(list1) if len(list1)>len(list2) else len(list2)
    for i in range(0, _len):
        if isinstance(list1[i], dict) and isinstance(list2[i], dict):
            tmp.append(dict(list1[i], **list2[i]))
    return tmp
                
#Contains the decimal
def check_line_is_digit(line):
    judge = False
    result = line.split()
    for split in result:
        split_res = split.split('.')
        if len(split_res) > 1:
            if split_res[0].isdigit() and split_res[1].isdigit():
                judge = True
            else:
                judge = False
                break
        elif len(split_res) == 1:
            if split_res[0].isdigit():
                judge = True
            else:
                judge = False
                break
    return judge



def format_int_to_hex(int_value , hex_len):
    hex_str = str(hex(int_value)).replace("0x" , "")
    if len(hex_str) < hex_len:
        tempstr = "0" * (hex_len - len(hex_str))
        result =  "0x" + tempstr +  hex_str
        return result
    else:
        return "0x" + hex_str
    