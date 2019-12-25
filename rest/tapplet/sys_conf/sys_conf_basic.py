import logging
import configparser
import os
import shutil
import json

import requests
from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


from logging.handlers import RotatingFileHandler
from conf.setting import restd_cfg_filename
from conf.globalValue import *

##############################################
###############    status ####################
##############################################
sys_conf_sync_status = {
    "status" : "Idle" , 
    "laststatus" : {
        "op" : "None",
        "error" : False,
        "time" : "None"
    }
}

def get_sys_conf_sync_status():
    return sys_conf_sync_status

##############################################
###############    logger ####################
##############################################

sys_conf_logger = None

logger_default_config = {
    "logger_name" : "sf_sys_conf",
#"logger_level" : debug / info
    "logger_level" : "debug",
    "logger_file" : "/var/log/sf_sys_conf.log",
    "logger_file_size" : 1024,
    "logger_file_backup" : 3,
}

def sys_conf_init_logger_internal(conf_dict):
    global sys_conf_logger

    if sys_conf_logger != None:
        return 

    for key in logger_default_config:
        if key not in conf_dict.keys():
            conf_dict.update({key : logger_default_config[key]})

    sys_conf_logger = logging.getLogger(conf_dict["logger_name"])

    rHandler = RotatingFileHandler(
        conf_dict["logger_file"] , 
        maxBytes = conf_dict["logger_file_size"] , 
        backupCount = conf_dict["logger_file_backup"]
        )

    if conf_dict["logger_level"] == "info":
        sys_conf_logger.setLevel(level = logging.INFO)
        rHandler.setLevel(level = logging.INFO)
    else:
        sys_conf_logger.setLevel(level = logging.DEBUG)
        rHandler.setLevel(level = logging.DEBUG)

    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    rHandler.setFormatter(formatter)

    sys_conf_logger.addHandler(rHandler)

    ## not use root logger 
    sys_conf_logger.propagate = False


#    ##### for debug #####
#    if conf_dict["logger_level"] == "debug":
#        console = logging.StreamHandler()
#        console.setLevel(logging.DEBUG)
#        sys_conf_logger.addHandler(console)
#    ##### for debug #####


    sys_conf_logger.info("==== init sys conf logger ====")

    


def sys_conf_init_logger():
    restd_config = configparser.ConfigParser() 
    restd_config.read(restd_cfg_filename , encoding = "utf-8")

    tmp_dict = {}
    section = "sf_sys_conf"
    if restd_config.has_section(section):
        options_list = restd_config.options(section)
        for option in options_list:
            if option == "log_level":
                loglevel = restd_config.get(section, option)
                if loglevel.lower() == "info":
                    tmp_dict.update({"logger_level" : "info"})
                else:
                    tmp_dict.update({"logger_level" : "debug"})

            elif option == "log_file_max_size":
                log_size = restd_config.getint(section, option)
                tmp_dict.update({"logger_file_size" : log_size})

            elif option == "log_file_num_backups":
                tmp_dict.update({"logger_file_backup" : restd_config.getint(section, option)})

            elif option == "log_file_name":
                log_filename = restd_config.get(section, option)
                tmp_dict.update({"logger_file" : log_filename})
    
    sys_conf_init_logger_internal(tmp_dict)


def sys_conf_get_logger():
    global sys_conf_logger
    if sys_conf_logger == None:
        sys_conf_init_logger()
    
    return sys_conf_logger


##############################################
###############    Class  ####################
##############################################
SYS_CONF_TRANS_SUCCESS = 0
SYS_CONF_TRANS_FAIL = -1


class SysConfBasicTrans():
    def __init__(self):
        self.logger = sys_conf_get_logger()

    def load_cfg(self , conf_dir):
        print("load in SysConfBasicTrans")
        return SYS_CONF_TRANS_SUCCESS

    def save_cfg(self , conf_dir):
        print("save in SysConfBasicTrans")
        return SYS_CONF_TRANS_SUCCESS


##############################################
###############    tools  ####################
##############################################

def try_json_input(json_str):
    try:
        # print(json_str)

        tmpdict = json.loads(json_str)
        # print(tmpdict)
    except Exception as e:
        return None
    return tmpdict

def try_json_file_input(json_file , dir_path = None):
    if json_file != None:
        json_file = os.path.join(dir_path , json_file) 

    try:
        with open(json_file , 'r') as load_f:
            load_dict = json.load(load_f)
    except Exception as e:
        return None
    return load_dict

def sys_conf_prepare_subdir(dir_path):
    '''
    dir_path must be an abusolute path
    this func will delete all files and directories under the <dir_path> if <dir_path> exists
    '''
    if  os.path.exists(dir_path):
        shutil.rmtree(dir_path)

    os.makedirs(dir_path)

def sys_conf_remove_subdir(dir_path):
    if  os.path.exists(dir_path):
        shutil.rmtree(dir_path)

def sys_conf_judge_file_exist(filename , dir_path = None):
    if dir_path != None:
        filename = os.path.join(dir_path , filename)

    return os.path.exists(filename)


def sys_conf_save_to_file(dict_or_json , filename , dir_path = None):
    if dir_path != None:
        filename = os.path.join(dir_path , filename)
    
    if dict_or_json == None:
        with open(filename , "w") as f:
            f.write('{}\n')
    elif type(dict_or_json) == dict:
        with open(filename , "w") as f:
            json.dump(dict_or_json , f)
    else:
        with open(filename , "w") as f:
            f.write(dict_or_json)
