from tools.conftest_tools import *
from tools.rest_tools import *
from pytest_main import host_config
from pytest_main import global_verbose

test_request = Sf_rest(host_config)


class BaseRestTest:
    # mod_name = ""
    # config_patch_test_key = ""
    # config_patch_test_value =
    def __init__(self , mod_name , config_key , config_value):
        self.mod_name = mod_name
        self.config_patch_test_key = config_key
        self.config_patch_test_value = config_value
        self.post_config = None
        self.put_config = None

    def test_get_config_all(self):
        path = "/config"
        resource = self.mod_name + path
        ret_dict = get_ret_config_dict(self.mod_name)

        ret  = test_request.auto_run(resource, "get" , verbose = global_verbose)
        assert ret[0] == 0 
        response = ret[1]
        assert type(response) == type(ret_dict)
        assert response.keys() == ret_dict.keys()

    def test_get_config_key(self):
        path = "/config"
        resource = self.mod_name + path
        ret_dict = get_ret_config_dict(self.mod_name)
        
        for i in ret_dict.keys():
            params = {"keys": i}
            ret = test_request.auto_run(resource, "get",  params  , verbose = global_verbose)
            assert ret[0] == 0 
            response = ret[1]
            assert type(response) == type(ret_dict)
            assert (i in response.keys()) == True
            
    def test_get_stat_all(self):
        path = "/stat"
        resource = self.mod_name + path
        ret_dict = get_ret_stat_dict(self.mod_name)

        ret = test_request.auto_run(resource, "get" , verbose = global_verbose)
        assert ret[0] == 0 
        response = ret[1]
        assert type(response) == type(ret_dict)
        assert response.keys() == ret_dict.keys()

    def test_get_stat_key(self):
        path = "/stat"
        resource = self.mod_name + path
        ret_dict = get_ret_stat_dict(self.mod_name)

        for i in ret_dict.keys():
            params = {"keys": i}
            ret = test_request.auto_run(resource, "get", params , verbose = global_verbose)
            assert ret[0] == 0 
            response = ret[1]
            assert type(response) == type(ret_dict)
            assert (i in response.keys()) == True
    #PATCH
    def test_patch_config_key(self, path = "/config"):
        resource = self.mod_name + path
        ret_dict = get_ret_config_dict(self.mod_name)
        data = [{"op": "add", "path": "/"+self.config_patch_test_key, "value": self.config_patch_test_value} ]
        ret = test_request.auto_run(resource, "patch", data = data , verbose = global_verbose)
        assert ret[0] == 0 
        #check value
        params = {"keys": self.config_patch_test_key}
        ret = test_request.auto_run(resource, "get", path, params , verbose = global_verbose)
        assert ret[0] == 0 
        response = ret[1]
        assert type(response) == type(ret_dict)
        assert response[self.config_patch_test_key] == self.config_patch_test_value


    def test_patch_stat_clean(self):
        path = "/stat"
        resource = self.mod_name + path
        ret_dict = get_ret_stat_dict(self.mod_name)
        data = [{"op": "replace", "path": "/", "value": ""} ]

        ret = test_request.auto_run(resource, "patch", data = data , verbose = global_verbose)
        assert ret[0] == 0 

    #POST
    def test_post_config(self, path = ""):
        resource = self.mod_name + path
        if self.post_config:
            data = self.post_config
            ret = test_request.auto_run(resource, "post",  data = data , verbose = global_verbose)
        assert ret[0] == 0 

    #PUT
    def test_put_config(self, path = ""):
        resource = self.mod_name + path
        print(resource)
        if self.put_config:
            data = self.put_config
            ret = test_request.auto_run(resource, "put",  data = data , verbose = global_verbose)
        print(ret)
        assert ret[0] == 0 