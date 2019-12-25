import requests, json
from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

from tools.rest_helper import *

    
def try_rest_login(sfrest_helper):
    ret  = sfrest_helper.rest_login()
    assert ret[0] == GlobalRestValue.OK

def try_rest_logout(sfrest_helper):
    ret  = sfrest_helper.rest_logout()
    assert ret[0] == GlobalRestValue.OK

def dispatch_test_config(sfrest_helper , config_needed):
    from pytest_main import global_verbose
    for patch_cmd in config_needed:
        data = [ {'op': 'replace', 'path':  patch_cmd["path"] , "value": patch_cmd["value"]}]
        ret = sfrest_helper.auto_run_no_login(patch_cmd['url'] , GlobalRestValue.ACTION_PATCH , data = data , verbose = global_verbose )
        assert ret[0] == 0


def clean_test_stat(sfrest_helper , stat_target):
    from pytest_main import global_verbose
    data =[ {'op': 'replace', 'path': "/" , "value": ""}]
    for mod_target in stat_target:
        ret = sfrest_helper.auto_run_no_login(mod_target['mod'], GlobalRestValue.ACTION_PATCH , data = data , verbose = global_verbose)
        assert ret[0] == 0
    

def check_stat_value(stat_target_dict , ret_json):
    # ret = json.loads(ret_json)
    for i in stat_target_dict.keys():
        print(i, stat_target_dict[i], ret_json[i])
        assert stat_target_dict[i] == ret_json[i]
        # try:
        #     assert stat_target_dict[i] == ret_json[i]
        # except Exception as e:
        #     print( i + " " + str(stat_target_dict[i]) + " != " + str(ret_json[i]))
        #     assert 1 == 3

def sync_sf_intf_stat(sfrest_helper):
    ret = sfrest_helper.rest_raw_get("vpp/sf_sync_intf_statics")
    assert ret[0] == GlobalRestValue.OK

def check_test_stat(sfrest_helper , stat_target):
    from pytest_main import global_verbose
    
    sync_sf_intf_stat(sfrest_helper)

    for mod_target in stat_target:
        ret = sfrest_helper.auto_run_no_login(mod_target['mod'], GlobalRestValue.ACTION_GET , verbose = global_verbose )
        assert ret[0] == 0
        # try:
        if mod_target["index"]:
            check_stat_value(mod_target['keys'] ,  ret[1][mod_target["index"]])
        else:
            check_stat_value(mod_target['keys'] ,  ret[1])
        # except Exception as e:
        #     assert 1 == 3


def check_vpp_stat(sfrest_helper):
    ret = sfrest_helper.rest_raw_get("vpp/auto_test_check")
    # ret = sfrest_helper.rest_raw_get("vpp/interfaces")
    assert ret[0] == GlobalRestValue.OK

def reset_all_mod_config(sfrest_helper):
    from pytest_main import global_verbose
    data =[ {'op': 'remove', 'path': "/" , "value": ""}]
    ret = sfrest_helper.auto_run_no_login("system/config" , GlobalRestValue.ACTION_PATCH , data = data , verbose = global_verbose)
    assert ret[0] == 0


def dispatch_post_config(sfrest_helper , resource , body):
    from pytest_main import global_verbose
    data = body
    ret = sfrest_helper.auto_run_no_login(resource , GlobalRestValue.ACTION_POST , data = data , verbose = global_verbose)
    assert ret[0] == 0

def dispatch_put_config(sfrest_helper , resource , body):
    from pytest_main import global_verbose
    data = body
    ret = sfrest_helper.auto_run_no_login(resource , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose)
    assert ret[0] == 0


def clean_target_stat(sfrest_helper , resource):
    from pytest_main import global_verbose
    data =[ {'op': 'replace', 'path': "/" , "value": ""}]
    ret = sfrest_helper.auto_run_no_login(resource, GlobalRestValue.ACTION_PATCH , data = data , verbose = global_verbose)
    assert ret[0] == 0
