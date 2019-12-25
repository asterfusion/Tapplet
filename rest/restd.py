#!/usr/bin/python3 -B 
import os
import subprocess
import tornado.ioloop
import tornado.httpserver
import tornado.process
from tornado.options import (
        options,
        parse_command_line
        )
from utils.ssl import *
from app import *
from tornado.log import app_log
from conf.setting import setting

options.logging = setting["logging"]
options.log_file_max_size = setting["log_file_max_size"]
options.log_file_num_backups = setting["log_file_num_backups"]
options.log_file_prefix = setting["log_file_prefix"]
if "HTTP_PORT" in setting:
    options.HTTP_PORT = setting["HTTP_PORT"]


def sfrestd_main():
    parse_command_line()

    application =  sfrestApplication()

    # create_ssl_pki()

    HTTP_server = tornado.httpserver.HTTPServer(application)
    HTTP_server.listen(options.HTTP_PORT, options.listen)

    HTTPS_server = tornado.httpserver.HTTPServer(application, ssl_options={
        "certfile": SSL_CRT_FILE,
        "keyfile": SSL_PRIV_KEY_FILE})
    HTTPS_server.listen(options.HTTPS_PORT, options.listen)
    tornado.ioloop.IOLoop.instance().start()

if __name__ == "__main__":
    sfrestd_main()
