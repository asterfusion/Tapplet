from basic_handler.login_handler import  LoginHandler
from basic_handler.logout_handler import  LogoutHandler
login_url = [
        (r"/rest/v1/login", LoginHandler)
        ]
logout_url = [
        (r"/rest/v1/logout", LogoutHandler)
        ]

