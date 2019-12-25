from tornado import web
from tornado import gen
from tornado.log import app_log
from conf.globalValue import *
from utils.tools import *
from utils.apiException import *
from conf.setting import setting
from basic_handler.base_handler import BaseHandler

from tapplet.nshellutils import *
from tapplet.sys_conf_handler import *

import subprocess
import os
import psutil
import gzip
import platform

##################################################################################
bytes_show_type={
    "B" : 1 , 
    "KB" : 1024 , 
    "MB" : 1024 ** 2,
    "GB" : 1024 ** 3
}

def format_cpu_usage():

    cpu_usage_list = psutil.cpu_percent(percpu=True)
    cpu_dict = {}
    for i in range(len(cpu_usage_list)):
        cpu_dict.update({ str(i) : cpu_usage_list[i]  })

    return cpu_dict

def format_mem_usage(divider):

    mem_info = psutil.virtual_memory()
    swap_mem_info = psutil.swap_memory()

    mem_dict = {
        "total" : mem_info.total / divider, 
        "used" : mem_info.used / divider, 
        "free" : mem_info.free / divider, 
        "usage" : mem_info.percent , 
        "swap" : {
            "total" : swap_mem_info.total / divider, 
            "used" : swap_mem_info.used / divider, 
            "free" : swap_mem_info.free / divider, 
            "usage" : swap_mem_info.percent , 
        }
    }

    return mem_dict

def format_dick_usage(divider):
    disk_devs = psutil.disk_partitions()
    disk_dict = {}
    for tmp_disk_dev  in disk_devs:
        disk_info = psutil.disk_usage(tmp_disk_dev.mountpoint)
        tmp_disk_dict = { 
            tmp_disk_dev.device : {
                "total" : disk_info.total  / divider, 
                "used" : disk_info.used / divider, 
                "free" : disk_info.free / divider, 
                "usage" : disk_info.percent , 
            }
        }
        disk_dict.update(tmp_disk_dict)

    return disk_dict

def get_system_status(bytes_type):
    if bytes_type == None:
        bytes_type = "B"
        
    if bytes_type not in bytes_show_type.keys():
        return Httplib.BAD_REQUEST , None
        
    divider = bytes_show_type[bytes_type]
    system_status_dict = {
        "cpu" : format_cpu_usage(),
        "mem" : format_mem_usage(divider),
        "disk" : format_dick_usage(divider),
        "display_type" : bytes_type
        }
    return Httplib.OK , system_status_dict 

class SystemStatusHandler(BaseHandler):

    def prepare(self):
        try:
            redirect_http_to_https(self)
            self.sort = get_query_arg(REST_PATAM_SORT, self.request.query_arguments)
            self.depth = get_query_arg(REST_PATAM_DEPTH, self.request.query_arguments)
            self.keys = get_query_arg(REST_PATAM_KEY, self.request.query_arguments)
            self.check_param()
            self.selector = get_query_arg(REST_PATAM_SELECTOR, self.request.query_arguments)
            self.check_selector()
            self.error_message = None
            self.set_response_type()
            self.check_login_user()
        except Exception as e :
            self.on_exception(e)
            self.finish()

    @gen.coroutine
    def get(self, var_name = None):
        app_log.debug("Run SystemStatusHandler GET....")
        try:
            self.type = get_type_arg(self.request.query_arguments)

            if self.type == None:
                self.type = "B"

            if self.type not in bytes_show_type.keys():
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()

            divider = bytes_show_type[self.type]

            system_status_dict = {
                "cpu" : format_cpu_usage(),
                "mem" : format_mem_usage(divider),
                "disk" : format_dick_usage(divider),
                "display_type" : self.type
            }

            program_name = "vpp"

            vpp_status_dict = {
                program_name:{
                    "Status":"EMPTY",
                    "Auto_Startup":"EMPTY"
                }
            }

            shm_status = try_link_shm()

            if shm_status != 0:
                vpp_status_dict[program_name]["Status"] = "BROKEN"
            else:
                sf_vpp_status = so.get_sf_main_config_sf_vpp_status()
                if sf_vpp_status == 0:
                    vpp_status_dict[program_name]["Status"] = "RUNNING"
                elif sf_vpp_status == 1:
                    vpp_status_dict[program_name]["Status"] = "INITIALIZING"
                else:
                    vpp_status_dict[program_name]["Status"] = "BROKEN"

                p = subprocess.Popen("systemctl status vpp",stdout=subprocess.PIPE , shell = True)
                for line in p.stdout.readlines():
                    _line = line.decode('utf-8')
                    if "Active: inactive" in _line:
                        vpp_status_dict[program_name]["Status"] = "SHUTDOWN"
                        break

            vpp_status_dict[program_name]["Auto_Startup"] = "DISABLED"
            q = subprocess.Popen("systemctl status vpp",stdout=subprocess.PIPE , shell = True)
            for line in q.stdout.readlines():
                _line = line.decode('utf-8')
                if "not-found" in _line and "Loaded" in _line:
                    vpp_status_dict[program_name]["Auto_Startup"] = "NOT_FOUND"
                    vpp_status_dict[program_name]["Status"] = "NOT_FOUND"
                    break
                if "vpp.service" in _line and "Loaded" in _line:
                    vpp_status_dict[program_name]["Auto_Startup"] = _line.split(';')[1].strip().upper()
                    break

            system_status_dict.update(vpp_status_dict)

            self.write(to_json(system_status_dict))
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
##################################################################################

######################################################################

def get_system_info(version_str , device_type_str):

    sysinfo = {
        "sf_version" : version_str,
        "device_type" : device_type_str,
        "platform" : platform.platform(),
    }

    return sysinfo

class SystemInfoHandler(BaseHandler):
    
    @gen.coroutine
    def get(self):
        app_log.debug("Run SystemInfoHandler get....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            sf_version = create_string_buffer(64)
            ret = so.shm_main_get_system_version(sf_version , 64)
            if ret != 0:
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return 
            
            version_str = str(sf_version.value , encoding="utf-8")


            device_type_name = create_string_buffer(64)
            ret = so.shm_main_get_device_type_name(device_type_name , 64)
            if ret != 0:
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return 
            
            device_type_str = str(device_type_name.value , encoding="utf-8")            

            result_dict = get_system_info(version_str , device_type_str)

            self.set_status(Httplib.OK)
            self.write(to_json(result_dict))
            self.finish()
            return
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

class SystemHandler(BaseHandler):
    @gen.coroutine
    def get(self):
        app_log.debug("Run SystemHandler get....")
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            self.depth = get_depth_param(self.request.query_arguments)
            if isinstance(self.depth, dict):
                self.set_status(Httplib.BAD_REQUEST)
                self.finish()
                return
            
            if self.depth <= 1:
                tmpdict = {"config":"/rest/v1/system/config" , "info":"/rest/v1/system/info" , "status":"/rest/v1/system/status"}
                self.write(to_json(tmpdict))
                self.finish()
                return
            
            #depth>1:get system info
            sf_version = create_string_buffer(64)
            version_str = str(sf_version.value , encoding="utf-8")
            info_dict = get_system_info(version_str)
            
            #depth>1:get system status
            bytes_type = "B"
            status_dict = get_system_status(bytes_type)
            #depth>1:merge rusults
            result_dict = {
                "config" : "/rest/v1/system/config",
                "info" : info_dict,
                "status" : status_dict
            }
            
            self.set_status(Httplib.OK)
            self.write(to_json(result_dict))
            self.finish()
            
        except APIException as e:  
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()


sf_system_urls = [
        (r"/rest/v1/system",SystemHandler),
        (r"/rest/v1/system/config/fileform", SystemConfigFileformHandler),
        (r"/rest/v1/system/config/sync", SystemConfigSyncHandler),
        (r"/rest/v1/system/config", SystemConfigHandler),
        (r"/rest/v1/system/info", SystemInfoHandler),
        (r"/rest/v1/system/status", SystemStatusHandler),
        ]


