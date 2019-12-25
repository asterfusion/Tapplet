from tornado import gen
from conf.globalValue import *
from utils.tools import *
from utils.apiException import *
from basic_handler.base_handler import BaseHandler
from tornado.log import app_log
from tornado.web import MissingArgumentError

class LoginHandler(BaseHandler):
    def initialize(self):
        self.error_message = None

    def prepare(self):
        try:
            redirect_http_to_https(self)
        except Exception as e:
            self.on_exception(e)

    @gen.coroutine
    def get(self):
        try:
            app_log.debug("Executing Login GET...")
            is_authenticated = self.current_user
            if is_authenticated is None:
                self.write('<html><body><form action="/rest/v1/login" method="post">'
                        'Name: <input type="text" name="username">'
                        'password: <input type="text" name="password">'
                        '<input type="submit" value="Sign in">'
                        '</form></body></html>')
                #raise AuthenticationFailed()
            else:
                self.set_status(Httplib.OK)
                result = {"status":"Authentication to pass!", "ipaddr":self.request.remote_ip}
                self.write(to_json(result))
        except APIException as e:
            self.on_exception(e)

        except Exception as e:
            self.on_exception(e)

        self.finish()

    @gen.coroutine
    def post(self):
        try:
            app_log.debug("Executing Login POST...")

            username = self.get_argument("username")
            password = self.get_argument("password")
            login_success = self.check_user_login_authorization(username, password)
            if not login_success:
                raise AuthenticationFailed('invalid username/password '
                                           'combination')
            else:
                self.set_secure_cookie("user", username)
                self.set_status(Httplib.OK)

        except MissingArgumentError as e:
            self.on_exception(DataValidationFailed('Missing username or password'))

        except APIException as e:
            self.on_exception(e)

        except Exception as e:
            self.on_exception(e)

        self.finish()

    def check_user_login_authorization(self, username, password):
        if username == "admin" and password == "admin":
            return True
        else:
            return False

