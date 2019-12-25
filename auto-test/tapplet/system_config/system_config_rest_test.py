# -*- coding: UTF-8 -*-

from copy import deepcopy
from tools.conftest_tools import *
from tools.rest_tools import *
from tools.tcpreplay_tools import *
import json
from pytest_main import port1_config
from pytest_main import port2_config
from pytest_main import sf_helper
from pytest_main import global_verbose
import tempfile
import os

from requests_toolbelt import MultipartEncoder

mod_name = "acl"


def test_system_config_reset():
    '''
    重置所有配置
    '''
    
    

    patch_data  = [  { "op" : "replace" , "path" : "/" , "value" : 0   }  ]

    ret = sf_helper.auto_run_no_login("system/config" , GlobalRestValue.ACTION_PATCH, data =  patch_data, verbose = global_verbose )
    assert ret[0] == 0

    

def test_system_config_sync():
    '''
    保存配置
    '''
    
    

    patch_data  = [  { "op" : "replace" , "path" : "/" , "value" : 1   }  ]

    ret = sf_helper.auto_run_no_login("system/config/sync" , GlobalRestValue.ACTION_PATCH, data =  patch_data, verbose = global_verbose )
    assert ret[0] == 0

    


def test_system_config_download_and_upload():
    '''
    配置下载及上传
    '''

    ### reset all 
    patch_data  = [  { "op" : "replace" , "path" : "/" , "value" : 0   }  ]

    ret = sf_helper.auto_run_no_login("system/config" , GlobalRestValue.ACTION_PATCH, data =  patch_data, verbose = global_verbose )
    assert ret[0] == 0

    old_timeout = sf_helper.timeout

    ## download 
    sf_helper.timeout = 30

    filePath = tempfile.mktemp()

    ret  = sf_helper.rest_raw_get("system/config")
    assert ret[0] == GlobalRestValue.OK

    data = ret[1]

    with open(filePath , "wb") as fp:
        fp.write(data)

    ### upload

    file_payload = {'shm_config':  (filePath , open( filePath , 'rb'), 'application/gzip')} 
    m = MultipartEncoder(file_payload)
    header = { 'Content-Type' : m.content_type }
    result = sf_helper.rest_raw_post("system/config",  m  , tmpheader = header)

    assert result[0] == GlobalRestValue.OK
    
    os.remove(filePath)


    sf_helper.timeout = old_timeout
    

