import sys
from tornado import web
from tornado.log import app_log
from conf.globalValue import *
from utils.tools import (
        redirect_http_to_https, 
        get_query_arg,
        to_json_error)
from utils.apiException import *
from conf.setting import setting

import re

###################### Not good code ######################
from tapplet.nshellutils import *

def check_tapplet_status(self):
    ## check shared mem 
    if try_link_shm() != 0:
        self.set_status(Httplib.SERVICE_UNAVAILABLE)
        self.finish()
        return
    
    ## check shm lock if request is going to change shm mem
    if self.request.method != "GET":
        if rest_try_lock_shm_mem(self) != 0:
            return
        
        if rest_unlock_shm_mem(self) != 0:
            return
        
    ## check acl lock if is gong to change acl shm
    if self.request.method != "GET":
        if re.match("/rest/v\d+/acl" , self.request.uri) != None:
            if so.acl_get_sync_flag() > 0 :
                self.set_status(Httplib.SERVER_BUSY)
                self.finish()

class BaseHandler(web.RequestHandler):
    def set_response_type(self):
        self.set_header(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_JSON) 

    def set_default_header(self):
        self.set_header("Cache-contorl", "no-cache") 

    def get_current_user(self):
        return self.get_secure_cookie("user", max_age_days=0.08)

    def prepare(self):
        try:
            redirect_http_to_https(self)
            # self.device_type = setting["device_type"]
            self.sort = get_query_arg(REST_PATAM_SORT, self.request.query_arguments)
            self.depth = get_query_arg(REST_PATAM_DEPTH, self.request.query_arguments)
            self.keys = get_query_arg(REST_PATAM_KEY, self.request.query_arguments)
            self.check_param()
            self.selector = get_query_arg(REST_PATAM_SELECTOR, self.request.query_arguments)
            self.check_selector()
            self.error_message = None
            self.set_response_type()
            self.check_login_user()

            check_tapplet_status(self)
        except Exception as e :
            self.on_exception(e)
            self.finish()
    
    def on_exception(self, e):
        self.error_message = str(e)
        if isinstance(e, APIException):
            self.set_status(e.status_code)
        elif isinstance(e, NotAuthenticated) or isinstance(e, AuthenticationFailed):
            self.set_header(HTTP_HEADER_LINK, REST_LOGIN_PATH)
            self.set_status(e.status_code)
        else:
            self.set_status(Httplib.INTERNAL_SERVER_ERROR)
            self.log_exception(*sys.exc_info())
        self.set_header(HTTP_HEADER_CONDITIONAL_IF_MATCH, HTTP_CONTENT_TYPE_JSON)
        self.write(self.error_message)


    def check_param(self):
        if (self.depth is not None  or self.keys is not None or self.depth is not None) and \
               self.request.method != HTTP_REQUEST_GET:
                   raise ParameterNotAllowed("Argmuent {0}, {1}, and {2}"
                                             "are only allowed in {3}".format(
                                                 REST_PATAM_SORT,
                                                 REST_PATAM_DEPTH,
                                                 REST_PATAM_KEY,
                                                 HTTP_REQUEST_GET
                                                 )
                                            )
        
    def check_selector(self):
        if self.selector:
            if self.selector not in SF_TYPE_LIST:
                raise DataValidationFailed("Invalid selector {0}".format(self.selector))
            if HTTP_HEADER_CONDITIONAL_IF_MATCH not in self.request.headers \
                and self.request.method in [HTTP_REQUEST_POST,
                                            HTTP_REQUEST_PUT,
                                            HTTP_REQUEST_DELETE,
                                            HTTP_REQUEST_PATCH]:
                raise ParameterNotAllowed("Argument '{0}' is only allowed"
                                          "in combination with If-match "
                                          "header for the following methods:"
                                          "'{1}','{2}','{3}','{4}'".format(
                                              REST_PATAM_SELECTOR,
                                              HTTP_REQUEST_POST,
                                              HTTP_REQUEST_PUT,
                                              HTTP_REQUEST_DELETE,
                                              HTTP_REQUEST_PATCH)
                                          )


    def on_finish(self):
        app_log.debug("Finished handling of request from {0}".format(self.request.remote_ip))

    def check_login_user(self):
        if not self.current_user:
            raise NotAuthenticated("Requires authentication")



