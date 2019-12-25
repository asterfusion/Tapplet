from tornado import web
from tornado import gen
from tornado.log import app_log
from conf.globalValue import *
from utils.tools import *
from utils.apiException import *
from conf.setting import setting
from basic_handler.base_handler import BaseHandler

from tornado.concurrent import run_on_executor
from concurrent.futures import ThreadPoolExecutor

from tapplet.nshellutils import *
from tapplet.sys_conf.sys_conf_main import *

import subprocess
import os
import psutil
import gzip
import platform
import _thread
import tarfile
##################################################################################



default_config_file = "/opt/sfconfig/shm_config.bin"
gzip_file_name = "shm_config.tar.gz"
gzip_file_fullname = "/tmp/{0}".format(gzip_file_name)

gzip_upload_file_name = "upload_shm_config.tar.gz"
gzip_upload_file_fullname = "/tmp/{0}".format(gzip_upload_file_name)

gzip_upload_unzip_dir = "/tmp/sys_conf_upload/"
gzip_upload_subdir_name = "sys_conf"
gzip_upload_sys_conf_root_dir = "{0}{1}/".format(gzip_upload_unzip_dir , gzip_upload_subdir_name)


def format_and_check_module_request(modules_str):
    try:
        modules = modules_str.split(',')

        for key in modules:
            if key not in sys_conf_map.keys():
                return ( -1 , None )
        return ( 0  ,  modules )
    except:
        return ( -1 , None )

class SystemConfigHandler(BaseHandler):
    executor = ThreadPoolExecutor(1)

    @run_on_executor
    def save_on_another_thread(self , modules):
        try:
            error_happened = sys_conf_save_all(modules)
            if error_happened:
                return error_happened

            # # prepare gzip file
            t = tarfile.open(gzip_file_fullname, "w:gz")
            t.add(sys_conf_root_dir , gzip_upload_subdir_name)
            t.close()

            return error_happened
        except:
            return True

    @gen.coroutine
    def get(self):
        # download config file
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            ### get param
            modules = get_query_arg("modules", self.request.query_arguments)
            if modules != None :
                ret = format_and_check_module_request(modules)
                if ret[0] != 0:
                    self.set_status(Httplib.BAD_REQUEST)
                    self.finish()
                    return
                modules = ret[1]
            

            ###  judge if locked
            ret = rest_try_lock_shm_mem(self)
            if ret != 0:
                return  
            
            result = yield self.save_on_another_thread(modules)

            ret = rest_unlock_shm_mem(self)
            if ret != 0:
                return

            if result == True:
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return
            
        
            # # set http header  
            self.set_header ('Content-Type', 'application/octet-stream')
            self.set_header('Content-Disposition', 'attachment; filename={0}'.format(gzip_file_name))

            # #  upload config file
            config_file_fd = open(gzip_file_fullname , "rb")
            while True:
                data = config_file_fd.read(4096)
                if not data:
                    break
                self.write(data)
            config_file_fd.close()

            self.set_status(Httplib.OK)

        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)

        self.finish()

    
    @gen.coroutine
    def patch(self):
        # reset all config
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            # request = decode_request_path_body(self.request.body)
            ret = rest_try_lock_shm_mem(self)
            if ret != 0:
                return

            if  check_acl_sync_flag() :
                so.unlock_shm_mem()
                self.set_status(Httplib.SERVER_BUSY)
                self.write("acl sync")
                self.finish()
                return

            ret = reset_all_config_libshell()
            if ret != 0:
                # remember unlock !!!
                so.unlock_shm_mem()
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return

            ret = rest_unlock_shm_mem(self)
            if ret != 0:
                return

            self.set_status(Httplib.NO_CONTENT)

        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
    
    
    @run_on_executor
    def load_on_another_thread(self):
        try:
            # 1. save file
            file_metas = self.request.files.get('shm_config', None) 

            if not file_metas:
                return Httplib.BAD_REQUEST
            else:
                meta = file_metas[0]
                with open(gzip_upload_file_fullname, 'wb') as f_tmp:
                    f_tmp.write(meta['body'])

            # 2.1 prepare root dir
            sys_conf_prepare_subdir(gzip_upload_unzip_dir)

            # 2.2 unzip gzip file
            
            tar = tarfile.open(gzip_upload_file_fullname, "r:gz")
            file_names = tar.getnames()
            for file_name in file_names:
                tar.extract(file_name, gzip_upload_unzip_dir)
            tar.close()

            # 3. start load

            error_happened = sys_conf_load_all(gzip_upload_sys_conf_root_dir)

            if error_happened:
                return Httplib.BAD_REQUEST
            else:
                return Httplib.OK
        except:
            return Httplib.INTERNAL_SERVER_ERROR

    @gen.coroutine
    def post(self):
        # upload config file
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            # 0. check lock
            ret = rest_try_lock_shm_mem(self)
            if ret != 0:
                return  

            ret_status =  yield self.load_on_another_thread()

            ret = rest_unlock_shm_mem(self)
            if ret != 0:
                return

            
            self.set_status(ret_status)

            
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
           
        self.finish()




class SystemConfigSyncHandler(BaseHandler):

    @gen.coroutine
    def patch(self):
        # write config file
        if try_link_shm() != 0:
            self.set_status(Httplib.SERVICE_UNAVAILABLE)
            self.finish()
            return
        try:
            ret = rest_try_lock_shm_mem(self)
            if ret != 0:
                return
            
            ret = so.save_shm_config_to_default_file()
            if ret != 0:
                # remember unlock !!!
                so.unlock_shm_mem()
                self.set_status(Httplib.INTERNAL_SERVER_ERROR)
                self.finish()
                return 

            ret = rest_unlock_shm_mem(self)
            if ret != 0:
                return
            self.set_status(Httplib.NO_CONTENT)
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()


class SystemConfigStatusHandler(BaseHandler):
    @gen.coroutine
    def get(self):
        try:
            result_dict =  get_sys_conf_sync_status()

            self.write(to_json(result_dict))
            self.set_status(Httplib.OK)
            
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        finally:
            self.finish()

# for test
class SystemConfigFileformHandler(BaseHandler):
    
    @gen.coroutine
    def get(self):
        self.set_header ('Content-Type', 'text/html; charset=UTF-8')
        self.write('''
            <html>
              <head><title>Upload File</title></head>
              <body>
                <h1>upload test</h1>
                <form action='/rest/v1/system/config' enctype="multipart/form-data" method='post'>
                    <input type='file' name='shm_config'/><br/>
                    <input type='submit' value='submit'/>
                </form>
                <br/>
                <h1>download test</h1>
                <form action='/rest/v1/system/config' method='get'>
                    <input type='submit' value='download'/>
                </form>
              </body>
            </html>
            ''')
######################################################################

