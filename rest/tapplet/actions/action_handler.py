import tornado.web
from tornado import gen
from tornado.log import app_log
from conf.globalValue import *
from utils.tools import *
from utils.apiException import *
from conf.setting import setting
from basic_handler.base_handler import BaseHandler

from tapplet.nshellutils import *
from tapplet.actions.action_class import * 
from tapplet.actions.action_utils import * 

mod_name = "action"


MAIN_ACTION_PATH = "/rest/v1/actions"


def get_action_info(action_id):
    result_dict = get_single_action_content(int(action_id))
    return result_dict

def get_action(index = None):
    result_dict = {}
    if index:
        if isinstance(index, list):
            for _index  in index:
                _index = int(_index)
                if _index >= MAX_ACTIONS_NUM and _index <= 0:
                    return None
                if check_action_exists(_index):
                    result_dict[str(_index)] = get_action_info(_index)
                else:
                    return None
        else:
            if index >= MAX_ACTIONS_NUM and index <= 0:
                return None
            if check_action_exists(index):
                result_dict[str(index)] = get_action_info(index)
            else:
                return None
    else: 
        for i in range(1, MAX_ACTIONS_NUM + 1):
            if check_action_exists(i):
                result_dict[str(i)] = get_action_info(i)
    return result_dict



def action_decode_request_path_body(body):
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
            if action["op"] != "replace":
                return -1
            var_change = action["path"].split("/")[1:]
            var_depth = len(action["path"].split("/")[1:])
            value_change = action["value"]
            # if not isinstance(value_change, dict):
            #     return -1
            resouce[var_change.pop()] = value_change
            var_depth -= 1
            while var_depth > 0:
                tmp = resouce.copy()
                resouce = {var_change.pop():tmp}
                var_depth -= 1
            path_decode.append((action["op"], resouce))
        return path_decode
    except:
        return None

def action_dict_merge_update(srcd , patchd):
    if srcd == None or isinstance(srcd , dict) == False:
        return patchd
    
    for patchd_k in patchd.keys():
        if patchd_k in srcd.keys():
            if isinstance(patchd[patchd_k] , dict):
                srcd[patchd_k] = action_dict_merge_update(srcd[patchd_k] , patchd[patchd_k])
            else:
                srcd[patchd_k] = patchd[patchd_k]
        else:
            srcd.update({patchd_k : patchd[patchd_k]})
    
    return srcd

class ActionConfigHandler(BaseHandler):

    @gen.coroutine
    def get(self, *args):
        app_log.debug("Run action GET....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.keys = get_keys_arg(self.request.query_arguments)
            self.depth = get_depth_param(self.request.query_arguments)
            self.index = get_index_arg(self.request.query_arguments)
            query_args = self.request.query_arguments
            # /actions
            if not args:
                result_dict = {}
                if self.index:
                    result_dict =  get_action(self.index)
                    if result_dict == None:
                        self.set_status(Httplib.NOT_FOUND)
                    else:
                        self.write(to_json(result_dict))
                    self.finish()
                    return
                if not isinstance(self.depth, dict):
                    if self.depth <= 1:
                        for i in range(1, MAX_ACTIONS_NUM + 1):
                            if check_action_exists(i):
                                result_dict[str(i)] = "{0}/{1}".format(MAIN_ACTION_PATH, i)
                        self.write(to_json(result_dict))
                        self.finish()
                        return
                    else:
                        result_dict = get_action()
                        if result_dict == None:
                            self.set_status(Httplib.NOT_FOUND)
                        else:
                            self.write(to_json(result_dict))
                        self.finish()
                        return
                else:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.finish()
                    return

            # /actions/{id}
            else:
                arg_list = args[0].split("/")
                if not arg_list[0].isdigit():
                    self.set_status(Httplib.NOT_FOUND)
                else:
                    index = int(arg_list[0])
                    result_dict = get_action(index)
                    if result_dict == None:
                        self.set_status(Httplib.NOT_FOUND)
                    else: 
                        if self.keys != None:
                            tmp_result_dict = {}
                            for key in self.keys:
                                try:
                                    tmp_result_dict[str(index)] = {}
                                    tmp_result_dict[str(index)][key] = result_dict[str(index)][key]
                                    #tmp_result_dict[key] = result_dict[str(index)][key]
                                    self.write(to_json(tmp_result_dict))
                                except KeyError:
                                    self.set_status(Httplib.NOT_FOUND)
                        else:
                            self.write(to_json(result_dict))
                            #self.write(to_json(result_dict[str(index)]))
                self.finish()
                return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()


    @gen.coroutine
    def patch(self, *args):
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            request = action_decode_request_path_body(self.request.body)
            # self.write(to_json(request))
            # self.set_status(Httplib.OK)
            if request == -1:
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()
                return 
            # /actions
            if not args:
                patch_dict = {}
                for opv in request:
                    patch_dict.update(opv[1])
            # /actions/{id}
            else:
                arg_list = args[0].split("/")
                if not arg_list[0].isdigit():
                    self.set_status(Httplib.NOT_FOUND)
                    self.finish()       
                    return 
                else:
                    index = int(arg_list[0])
                    if index >= MAX_ACTIONS_NUM and index <= 0:
                        self.set_status(Httplib.NOT_FOUND)
                        self.finish()       
                        return 
                
                patch_dict_inner = {}
                for opv in request:
                    patch_dict_inner.update(opv[1])
                patch_dict = { str(index) : patch_dict_inner }

            origin_dict = get_action( list(patch_dict.keys()) )

            if origin_dict == None:
                self.set_status(Httplib.NOT_FOUND)
                self.finish()       
                return 

            origin_dict = action_dict_merge_update(origin_dict , patch_dict)
            # print(origin_dict)
            ret = update_action_content("put", origin_dict)

            if ret != None:
                self.set_status(Httplib.BAD_REQUEST)
                self.write(to_json(ret))
                self.finish()
                return 


            self.set_status(Httplib.NO_CONTENT)
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()                  

    @gen.coroutine
    def post(self, *args):
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            request = self.request.body
            request = json.loads(request.decode('utf-8'))
            # /actions
            if not args:
                if not isinstance(request, dict):
                    self.set_status(Httplib.BAD_REQUEST)
                    self.finish()
                    return 
                ret = update_action_content("post" , request)
                if ret != None:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.write(to_json(ret))
                    self.finish()
                    return 
                self.set_status(Httplib.CREATED)
            else:
                self.set_status(Httplib.NOT_FOUND)
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()                  

    @gen.coroutine
    def put(self, *args):
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            request = self.request.body
            request = json.loads(request.decode('utf-8'))
            # /actions
            if not args:
                if not isinstance(request, dict):
                    self.set_status(Httplib.BAD_REQUEST)
                    self.finish()
                    return 
                ret = update_action_content("put", request)

                if ret != None:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.write(to_json(ret))
                    self.finish()
                    return 
                self.set_status(Httplib.OK)
            else:
                arg_list = args[0].split("/")
                if not arg_list[0].isdigit():
                    self.set_status(Httplib.NOT_FOUND)
                else:
                    index = int(arg_list[0])
                    ret = update_action_content("put", {str(index) : request})
                if ret != None:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.write(to_json(ret))
                    self.finish()
                    return 
                self.set_status(Httplib.OK)
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()                  

    @gen.coroutine
    def delete(self, *args):
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            # /actions
            if not args:
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()
                return 
            else:
                arg_list = args[0].split("/")
                if not arg_list[0].isdigit():
                    self.set_status(Httplib.BAD_REQUEST)
                    self.finish()
                    return 
                else:
                    index = int(arg_list[0])
                    ret = delete_action_libnshell(index)
                if ret == -1:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.finish()
                    return 
                self.set_status(Httplib.NO_CONTENT)
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()                  


sf_action_urls = [
        (r"/rest/v1/actions", ActionConfigHandler),
        (r"/rest/v1/actions/(.*)", ActionConfigHandler),
        ]
