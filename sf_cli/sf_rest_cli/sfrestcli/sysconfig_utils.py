
from . rest_helper import *
from . globalValue import *
from requests_toolbelt import MultipartEncoder


def do_sysconfig_get(server , filename):
    sf_rest_helper = Sf_rest(server)
    sf_rest_helper.rest_login()

    result = sf_rest_helper.rest_raw_get("system/config")
    if result[0] != Httplib.OK:
        print("Download fail")
    else:
        with open(filename , "wb") as fp:
            fp.write(result[1])
        print("Download OK...[{0}]".format(filename))

    sf_rest_helper.rest_logout()
    


def do_sysconfig_post(server , filename):
    sf_rest_helper = Sf_rest(server)
    sf_rest_helper.rest_login()

    file_payload = {'shm_config':  (filename , open( filename , 'rb'), 'application/gzip')} 
    m = MultipartEncoder(file_payload)
    header = { 'Content-Type' : m.content_type }
    result = sf_rest_helper.rest_raw_post("system/config",  m  , tmpheader = header)

    if result[0] != Httplib.OK:
        print("Upload Fail...[{0}]".format(result[0]))
    else:
        print("Upload OK")

    sf_rest_helper.rest_logout()

def do_sysconfig_request(arguments):
    filename = arguments["<config_file>"]
    server = arguments["<server_ip>"]
    if arguments["get"]:
        do_sysconfig_get(server , filename)
    elif arguments["post"]:
        do_sysconfig_post(server , filename)
