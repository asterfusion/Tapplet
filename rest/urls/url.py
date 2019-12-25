from tornado.log import app_log
from urls.login_url import login_url
from urls.login_url import logout_url
from tapplet.url import *

activator_urls = (
            login_url,
            logout_url,
            )


class urls:
    def __new__(cls, *args, **kw):
        cls.instance = super(urls, cls).__new__(cls, *args, **kw)
        return cls.instance

    def __init__(self):
        self.url_list = []
        self.append_url_map(activator_urls)
        self.append_url_map(tapplet_urls)
        app_log.debug("activator_urls: {0}".format(str(self.url_list)))

    def append_url_map(self, args):
        for url in args:
            if type(url) is list:
                self.url_list.extend(url)
            else:
                app_log.debug("error url_map as {0}".format(str(url)))

    

