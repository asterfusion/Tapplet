from tools.rest_test_class import *
from tools.rest_tools import *

import json
from pytest_main import sf_helper
from pytest_main import port1_config
from pytest_main import port2_config
from pytest_main import global_verbose

mod_name = "actions"

post_json = "tapplet/actions/json/post.json"
post_1_json = "tapplet/actions/json/post_1.json"
post_2_json = "tapplet/actions/json/post_2.json"
put_1_json = "tapplet/actions/json/put_1.json"
put_2_json = "tapplet/actions/json/put_2.json"


class ActionsRestTest(BaseRestTest):
    #PATCH
    def test_patch_config_key(self, path = "", check_value = None):
        resource = self.mod_name + path
        data = [{"op": "replace", "path": "/"+self.config_patch_test_key, "value": self.config_patch_test_value} ]
        ret = test_request.auto_run(resource, "patch", data = data , verbose = global_verbose)
        assert ret[0] == 0
        #check value
        params = {"keys": "basis_actions"}
        resource = "actions/1" 
        ret = test_request.auto_run(resource, "get", params , verbose = global_verbose)
        assert ret[0] == 0
        response = ret[1]
        assert type(response) == dict
        if check_value == None:
            assert response["1"]["basis_actions"] == self.config_patch_test_value
        else:
            assert response["1"]["basis_actions"] == check_value 


rest_test = ActionsRestTest(mod_name , "" , "" )

def test_actions_post_config():
    reset_all_mod_config(sf_helper)

    with open(post_json, "r") as post_config:
        rest_test.post_config = json.load(post_config)
    rest_test.post_config["1"]["basis_actions"]["interfaces"] = [ port1_config ]
    rest_test.test_post_config()

    with open(post_1_json, "r") as post_config:
        rest_test.post_config = json.load(post_config)
    rest_test.post_config["2"]["basis_actions"]["interfaces"] = [ port1_config ]
    rest_test.test_post_config()

    with open(post_2_json, "r") as post_config:
        rest_test.post_config = json.load(post_config)
    rest_test.post_config["3"]["basis_actions"]["interfaces"] = [ port1_config ]
    rest_test.test_post_config()

def test_actions_put_config():
    put_path_1 = ""
    put_path_2 = "/1"
    with open(put_1_json, "r") as put_config:
        rest_test.put_config = json.load(put_config)
    rest_test.put_config["1"]["basis_actions"]["interfaces"] = [ port2_config ]
    '''
    print("put config 1")
    for i in rest_test.put_config.items():
        print(i)
    '''
    rest_test.test_put_config(put_path_1)
    with open(put_2_json, "r") as put_config:
        rest_test.put_config = json.load(put_config)
    rest_test.put_config["basis_actions"]["interfaces"] = [port1_config, port2_config]
    '''
    print("put config 2")
    for i in rest_test.put_config.items():
        print(i)
    '''
    rest_test.test_put_config(put_path_2)


def test_get_actions():
    path = ""
    resource = mod_name + path
    #/actions 
    ret  = test_request.auto_run(resource, "get")
    assert ret[0] == 0  
    response = ret[1]
    assert type(response) == dict
    for index in response.keys():  
        assert index.isdigit()

    #/actions?depth=2
    ret  = test_request.auto_run(mod_name, "get", params = {"depth": 2})
    assert ret[0] == 0 
    response = ret[1]
    assert type(response) == dict
    for index in response.keys():  
        assert index.isdigit()
        assert isinstance(response[index], dict)
        assert "additional_actions" in response[index].keys()
        assert "basis_actions" in response[index].keys()

def test_get_actions_index():
    path = "/1"
    resource = mod_name + path
    #/actions/{action_id}
    ret  = test_request.auto_run(resource, "get")
    assert ret[0] == 0 
    response = ret[1]
    assert type(response) == dict 
    for index in response.keys():  
        assert index.isdigit()

    #/actions/{action_id}?keys=xxx
    ret  = test_request.auto_run(resource, "get", params = {"keys": "basis_actions"})
    assert ret[0] == 0 
    response = ret[1]
    assert type(response) == dict 
    for index in response.keys():  
        assert index.isdigit()
        assert isinstance(response[index], dict)
        assert "basis_actions" in response[index].keys()


def test_patch_action_key():
    #/actions
    config_patch_test_key = "1/basis_actions"
    config_patch_test_value = {
                "interfaces": ["X1","X2"], "type": "load_balance", 
                "load_balance_mode": "wrr",
                "load_balance_weight": "1:2"
            }
    config_patch_test_value["interfaces"] = [ port1_config, port2_config ]
    rest_test.config_patch_test_key = config_patch_test_key
    rest_test.config_patch_test_value = config_patch_test_value
    rest_test.test_patch_config_key(path = "")
    #/actions/{actions}
    config_patch_test_key_1 = "basis_actions"
    config_patch_test_value_1 = {
                "interfaces": ["X1","X2"], "type": "load_balance", 
                "load_balance_mode": "outer_src_dst_ip",
                "load_balance_weight": ""
            }

    config_patch_test_value_1["interfaces"] = [ port1_config, port2_config ]
    rest_test.config_patch_test_key = config_patch_test_key_1
    rest_test.config_patch_test_value = config_patch_test_value_1
    rest_test.test_patch_config_key(path = "/1", check_value = { "interfaces": [port1_config, port2_config], "type": "load_balance", "load_balance_mode": "outer_src_dst_ip" } )

def test_delete_action():
    path = "/1"
    resource = mod_name + path
    ret  = test_request.auto_run(resource, "delete")
    assert ret[0] == 0  

    path = "/2"
    resource = mod_name + path
    ret  = test_request.auto_run(resource, "delete")
    assert ret[0] == 0  

    path = "/3"
    resource = mod_name + path
    ret  = test_request.auto_run(resource, "delete")
    assert ret[0] == 0  
