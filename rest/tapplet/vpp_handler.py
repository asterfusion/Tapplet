import tornado.web
from tornado import gen
from tornado.log import app_log
from conf.globalValue import *
from utils.tools import *
from utils.apiException import *
from conf.setting import setting
from basic_handler.base_handler import BaseHandler

from tapplet.nshellutils import *

class VppShowInterfaceHandler(BaseHandler):
    
    def initialize(self):
        self.error_message = None

    @gen.coroutine
    def options(self):
        allowed_mothed = [HTTP_REQUEST_GET, HTTP_REQUEST_PATCH]
        self.set_header(HTTP_HEADER_ALLOW)
        self.set_status(Httplib.OK)
        self.finish()
    
    @gen.coroutine
    def get(self, var_name = None):
        try:
            ret = send2vpp("show int")
            if ret == vpp_udp_timeout_str:
                self.set_status(Httplib.SERVICE_UNAVAILABLE)
                self.finish()
                return 
            self.set_status(Httplib.OK)
            self.write(str(ret))
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()
        


class VppSfSyncIntfStaticsHandler(BaseHandler):
    
    def initialize(self):
        self.error_message = None

    @gen.coroutine
    def options(self):
        allowed_mothed = [HTTP_REQUEST_GET, HTTP_REQUEST_PATCH]
        self.set_header(HTTP_HEADER_ALLOW)
        self.set_status(Httplib.OK)
        self.finish()
    
    @gen.coroutine
    def get(self, var_name = None):
        try:
            ret = send2vpp("sf_auto_test sync intf statics")
            if ret == vpp_udp_timeout_str:
                self.set_status(Httplib.SERVICE_UNAVAILABLE)
                self.finish()
                return 
            self.set_status(Httplib.OK)
            self.write(str(ret))
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()



class VppAutoTestHandler(BaseHandler):
    
    def initialize(self):
        self.error_message = None

    @gen.coroutine
    def options(self):
        allowed_mothed = [HTTP_REQUEST_GET, HTTP_REQUEST_PATCH]
        self.set_header(HTTP_HEADER_ALLOW)
        self.set_status(Httplib.OK)
        self.finish()
    
    @gen.coroutine
    def get(self, var_name = None):
        try:
            pipe = os.popen("vppctl show int", 'r')
            text = pipe.read()
            pipe.close()
            
            ret = text.find("(L3/IP4/IP6/MPLS)")

            if ret == -1:
                self.set_status(Httplib.SERVICE_UNAVAILABLE)
                self.finish()
                return 
            self.set_status(Httplib.OK)
            
        except APIException as e: 
            self.on_exception(e)
        except Exception as e:
            self.on_exception(e)
        self.finish()

vpp_cmd_urls = [
        (r"/rest/v1/vpp/interfaces", VppShowInterfaceHandler),
        (r"/rest/v1/vpp/sf_sync_intf_statics", VppSfSyncIntfStaticsHandler),
        (r"/rest/v1/vpp/auto_test_check", VppAutoTestHandler),
        ]


