from tornado import gen
from tornado.log import app_log
from utils.tools import *
from conf.globalValue import *
from basic_handler.base_handler import BaseHandler
from tornado.log import app_log
from utils.apiException import *


class LogoutHandler(BaseHandler):

    def initialize(self):
        self.error_message = None

    def prepare(self):
        try:
            redirect_http_to_https(self)

            app_log.debug("Incoming request from %s: %s",
                          self.request.remote_ip,
                          self.request)

        except Exception as e:
            self.on_exception(e)
            self.finish()

    @gen.coroutine
    def post(self):
        try:
            app_log.debug("Executing Logout POST...")

            self.clear_cookie("user")
            self.set_status(Httplib.OK)

        except APIException as e:
            self.on_exception(e)

        except Exception as e:
            self.on_exception(e)

        self.finish()
