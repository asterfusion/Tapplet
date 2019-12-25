import os
import configparser
from tornado.options import *



define("HTTPS", default = True, help= "run on serving HTTPS")
define("HTTP_PORT", default = 80, help= "HTTP port", type=int)
define("HTTPS_PORT", default = 443, help= "HTTPS port", type=int)
define("listen", default = "0.0.0.0", help= "listen address")
define("config", default=None, help="restd config file")
define("force_https", default = True, help= "HTTP connections to be redirected to HTTPS")

setting = {}
setting["SFREST_INST_DIR"] = "/opt/sfrest/"
setting["SFREST_STATIC_DIR"] = setting["SFREST_INST_DIR"] + "static"

restd_cfg_filename = setting["SFREST_INST_DIR"] + "restd.cfg"

restd_config = configparser.ConfigParser() 
restd_config.read(setting["SFREST_INST_DIR"] + "restd.cfg", encoding = "utf-8")


restd = "restd"
if restd_config.has_section(restd):
    options_list = restd_config.options(restd)
    for option in options_list:
        if option == "log_level":
            setting["logging"] = restd_config.get(restd, option)
        elif option == "log_file_max_size":
            setting["log_file_max_size"] = restd_config.getint(restd, option)
        elif option == "log_file_num_backups":
            setting["log_file_num_backups"] = restd_config.getint(restd, option)
        elif option == "log_file_prefix":
            setting["log_file_prefix"] = restd_config.get(restd, option)
