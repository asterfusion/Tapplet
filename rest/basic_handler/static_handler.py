from tornado.web import StaticFileHandler

from utils.tools import redirect_http_to_https

class StaticContentHandler(StaticFileHandler):
    def prepare(self):
        try:
            redirect_http_to_https(self)
        except Exception as e:
            self.on_exception(e)
