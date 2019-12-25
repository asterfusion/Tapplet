from tools.conftest_tools import *
from tools.rest_tools import *
from tools.tcpreplay_tools import *
import json
from pytest_main import port1_config
from pytest_main import port2_config
from pytest_main import sf_helper
from pytest_main import global_verbose
import time

mod_name = "acl"

rule_tuple6_model = {
    "group_1":
    {
        "20":
        {
            "rule_type":"tuple6",
            "rule_cfg":{
                "dip":"0::0",
                "dip_mask":128,
                "dport_max":0,
                "dport_min":0,
                "proto_max":0,
                "proto_min":0,
                "sip":"0::0",
                "sip_mask":128,
                "sport_max":0,
                "sport_min":0
            },
        }
    }
}

rule_tuple4_model = {
    "group_1":
    {
        "20":
        {
            "rule_cfg":{
            "dip":"123.123.123.123",
            "dip_mask":24,
            "dport_max":45,
            "dport_min":45,
            "proto_max":17,
            "proto_min":17,
            "sip":"1.23.2.1",
            "sip_mask":24,
            "sport_max":67,
            "sport_min":67,
            },
            "rule_type":"tuple4"
        }
    }
}


def test_acl_post_config():
    '''
    创建未创建规则
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)
    
    data = rule_tuple6_model
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_POST , data = data , verbose = global_verbose )
    assert ret[0] == 0
    
def test_acl_post_config_fail():
    '''
    创建已创建规则，失败
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)
    
    data = rule_tuple6_model
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_POST , data = data , verbose = global_verbose )
    assert ret[0] == 0

    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_POST , data = data , verbose = global_verbose )
    assert ret[0] == -1
def test_acl_put_config():
    '''
    更新已/未创建规则
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)
    
    data = rule_tuple6_model
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0

    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0

def test_acl_delete_config():
    '''
    删除规则
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)
    
    data = rule_tuple6_model
    param_1 = {  "index" : 20 }
    param_2 = { "group" : 1 , "index" : 20 }

    # create rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0
    
    # delete rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_DELETE , params = param_2 , verbose = global_verbose )
    assert ret[0] == 0

    # create rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0

    # delete rule ( url with group )
    ret = sf_helper.auto_run_no_login("acl/config/group_1" , GlobalRestValue.ACTION_DELETE , params = param_1 , verbose = global_verbose )
    assert ret[0] == 0

    # create rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0

    # delete rule ( url with group and index )
    ret = sf_helper.auto_run_no_login("acl/config/group_1/20" , GlobalRestValue.ACTION_DELETE , verbose = global_verbose )
    assert ret[0] == 0

def test_acl_get_single_stat():
    '''
    获取单条/多条计数
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)

    data = rule_tuple6_model
    param_1 = {  "index" : 20 }
    param_2 = { "group" : 1 , "index" : 20 }

    # create rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0

    ret = sf_helper.auto_run_no_login("acl/stat" , GlobalRestValue.ACTION_GET , params = param_2 , verbose = global_verbose )
    assert ret[0] == 0


    ret = sf_helper.auto_run_no_login("acl/stat/group_1" , GlobalRestValue.ACTION_GET , params = param_1 , verbose = global_verbose )
    assert ret[0] == 0

    ret = sf_helper.auto_run_no_login("acl/stat/group_1/20" , GlobalRestValue.ACTION_GET , verbose = global_verbose )
    assert ret[0] == 0    

def test_acl_get_too_large_stat():
    '''
    获取过多计数
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)

    #use rule_tuple6_model make 101 rules
    # rule_g1_101 = {"group_1":{"%d"%i:rule_tuple6_model["group_1"]["20"] for i in range(1 , 101 + 1)}}
    rule_config = {}
    for i in range(1 , 101 + 1):
        tmpcfg = {
            "rule_type":"tuple6",
            "rule_cfg":{
                "dip":"0::0",
                "dip_mask":128,
                "dport_max": i ,
                "dport_min":0,
                "proto_max":0,
                "proto_min":0,
                "sip":"0::0",
                "sip_mask":128,
                "sport_max":0,
                "sport_min":0,
            }
        }


        tmpdict = {str(i) : tmpcfg}
        rule_config.update(tmpdict)

    
    data =  {"group_1" : rule_config}
    #creat rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0
    #get stat
    ret = sf_helper.auto_run_no_login("acl/stat/group_1" , GlobalRestValue.ACTION_GET , verbose = global_verbose )
    assert ret[0] == -1

def test_acl_clear_stat():
    '''
    清除计数
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)

    patch_data  = [  { "op" : "replace" , "path" : "/" , "value" : 0   }  ]

    ret = sf_helper.auto_run_no_login("acl/stat" , GlobalRestValue.ACTION_PATCH, data =  patch_data, verbose = global_verbose )
    assert ret[0] == 0

def test_acl_sync():
    '''
    测试acl同步
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)

    patch_data  = [  { "op" : "replace" , "path" : "/" , "value" : 0   }  ]

    ret = sf_helper.auto_run_no_login("acl/sync" , GlobalRestValue.ACTION_PATCH, data =  patch_data, verbose = global_verbose )
    assert ret[0] == 0


    time.sleep(4)

def test_get_acl_rule_group():
    '''
    获取interface acl 组默认配置
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)

    param_1 = {  "index" : port1_config }

    ret = sf_helper.auto_run_no_login("interfaces/config" ,  GlobalRestValue.ACTION_GET , params = param_1 ,  verbose = global_verbose )
    assert ret[0] == 0

    assert ret[1][port1_config]["ingress_config"]["acl_rule_group"] == 1

def test_set_acl_rule_group():
    '''
    配置interface acl 组
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)

    patch_data  = [  { "op" : "replace" , 
        "path" : "/" + port1_config + "/ingress_config/acl_rule_group" , "value" : 2   }  ]

    ret = sf_helper.auto_run_no_login("interfaces/config" , GlobalRestValue.ACTION_PATCH, data =  patch_data, verbose = global_verbose )
    assert ret[0] == 0


    param_1 = {  "index" : port1_config }

    ret = sf_helper.auto_run_no_login("interfaces/config" ,  GlobalRestValue.ACTION_GET , params = param_1 ,  verbose = global_verbose )
    assert ret[0] == 0

    assert ret[1][port1_config]["ingress_config"]["acl_rule_group"] == 2


def test_acl_duplicate_rule():
    '''
    重复规则测试
    '''
    # clean up all config
    reset_all_mod_config(sf_helper)

    ### tuple6 ###
    #use rule_tuple6_model make 2 rules
    data = {"group_1":{"1" : rule_tuple6_model["group_1"]["20"] }}
    #creat first rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0

    data = {"group_1":{"2" : rule_tuple6_model["group_1"]["20"] }}
    #creat second rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == -1

    ### tuple4 ###
    data = {"group_1":{"1" : rule_tuple4_model["group_1"]["20"] }}
    #creat first rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == 0

    data = {"group_1":{"2" : rule_tuple4_model["group_1"]["20"] }}
    #creat second rule
    ret = sf_helper.auto_run_no_login("acl/config" , GlobalRestValue.ACTION_PUT , data = data , verbose = global_verbose )
    assert ret[0] == -1

