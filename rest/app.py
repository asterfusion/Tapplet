from tornado.web import Application
from urls.url import urls
import os
from conf.setting import setting


class sfrestApplication(Application):
    def __init__(self):
        self._url_map = self._get_url_map()
        Application.__init__(self, self._url_map, static_path = setting["SFREST_STATIC_DIR"] , cookie_secret="SF_REST_API")

    def _get_url_map(self):
        url_map_list = urls().url_list
        return url_map_list

