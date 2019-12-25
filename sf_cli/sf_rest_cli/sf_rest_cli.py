#!/usr/bin/python3 -B

help_string = '''
Usage:  sf_rest_cli -h | --help
        sf_rest_cli setup-auth  <username> <password>
        sf_rest_cli get sysconfig (-s | --server) <server_ip>  (-f | --file) <config_file> 
        sf_rest_cli post sysconfig (-s | --server) <server_ip>  (-f | --file) <config_file> 
        sf_rest_cli (get|delete) [-v] (-s | --server) <server_ip>  (-r | --resource) <rest_resource>
                    [--group=<group_id>] 
                    [--index=<indexs>]
                    [--depth=<depth>] 
                    [--keys=<query_keys>] 
                    [--type=<query_type>]
        sf_rest_cli patch [-v] (-s | --server) <server_ip>  (-r | --resource) <rest_resource>
                    --path <patch_path> (--str-value | --int-value | --json ) <patch_value>
        sf_rest_cli patch [-v] (-s | --server) <server_ip>  (-r | --resource) <rest_resource>
                    --path <patch_path> --json-file <json_file>
        sf_rest_cli put [-v] (-s | --server) <server_ip>  (-r | --resource) <rest_resource>
                    --json-file <json_file>
        sf_rest_cli post [-v] (-s | --server) <server_ip>  (-r | --resource) <rest_resource>
                    --json-file <json_file>

Arguments:
  server_ip         target device ip and port (e.g. 192.168.1.23  or  192.168.1.23:8080)
  rest_resource     target RESTful resource
  group_id          group list (e.g. 1,2,3,4-5)
  indexs            index list (e.g. 1,2,3,4-5)
  query_keys        keys list (e.g. admin_status,mac)
  query_type        query type (e.g. total)

Options:
  -h --help
  -v                verbose mode

Notice:
    username and password are saved in '~/sfrestcli.cfg',
    so make sure you have a HOME directory

'''

from sfrestcli.docopt import docopt


from sfrestcli.sf_utils import *
from sfrestcli.rest_helper import *
from sfrestcli.sysconfig_utils import *
from sfrestcli.license_utils import *
from sfrestcli.auth_utils import *


def do_rest_request( server , resource , action , params  = None, data  = None , verbose = False):
    sf_rest_helper = Sf_rest(server)
    result = sf_rest_helper.auto_run(resource , action , params , data  , verbose )
    return result

def format_rest_result(result):
    if result[0] != 0:
        hint_str  = Sf_rest_return_code[ result[0] ]
        print("{0} : {1}".format(  hint_str ,  result[1] ))
        if  len(result) == 3:
            print("Detail:")
            print(result[2])  
    if action == "get":
        tmpdict = try_json_input(result[1])
        if tmpdict != None:
            print(json.dumps(tmpdict, indent=4, sort_keys= True , separators=(',', ':')))
        else:
            print("Error JSON string from server , see below")
            print(result[1])


def get_action(arguments):
    for tmp in rest_action:
        if arguments[tmp]:
            return tmp


if __name__ == '__main__':
    arguments = docopt(help_string)
    #print(arguments)

    server = arguments["<server_ip>"]
    resource = arguments["<rest_resource>"]
    verbose = arguments["-v"]

    if arguments["setup-auth"]:
        save_user_auth_info(arguments['<username>'] , arguments['<password>'])
    elif arguments["sysconfig"]:
        do_sysconfig_request(arguments)
    else:
        action = get_action(arguments)
        params,data = format_args(arguments)


        # print(json.dumps(params, indent=4, sort_keys= True , separators=(',', ':')))
        # print("----------------------")
        # print(json.dumps(data, indent=4, sort_keys= True , separators=(',', ':')))


        result = do_rest_request(server ,resource ,action ,  params , data ,  verbose )

        format_rest_result(result)


