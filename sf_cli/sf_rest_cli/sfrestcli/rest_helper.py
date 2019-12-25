import requests, json
from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


import os

from . globalValue import *
from . auth_utils import *
def rest_print(verbose , msg):
    if verbose:
        print(msg)



Sf_rest_return_code = { -1 : "Error",  0 : "OK"  }

rest_action = ["get" , "patch" , "put" , "post" , "delete"]

sf_ok_codes = {
    "get" : [Httplib.OK] , 
    "patch" : [Httplib.OK, Httplib.ACCEPTED , Httplib.NO_CONTENT] , 
    "put" : [Httplib.NO_CONTENT , Httplib.OK] , 
    "post" : [Httplib.CREATED , Httplib.OK] , 
    "delete" : [Httplib.NO_CONTENT] , 
}

sf_except_codes = {
    "get" : [Httplib.SERVICE_UNAVAILABLE] , 
    "patch" : [Httplib.SERVICE_UNAVAILABLE] , 
    "put" : [Httplib.SERVICE_UNAVAILABLE] , 
    "post" : [Httplib.SERVICE_UNAVAILABLE] , 
    "delete" : [Httplib.SERVICE_UNAVAILABLE] , 
}

class Sf_rest: 
    timeout = 5

    def __init__(self, server , sfurl = "", sfparams = {}, sfdata = {}):
        auth_dict = read_user_auth_info()
        self.server = server
        self.rest_url = "https://" + server + "/rest/v1/"
        self.SF_login = self.rest_url + "login" 
        self.SF_logout = self.rest_url + "logout"
        self.sfurl = sfurl
        self.sfparams = sfparams
        self.sfdata = sfdata
        self.session  = requests.Session() 
        self.auth = auth_dict

    def rest_raw_get(self , tmpurl): 
        try:
            response = self.session.get(url = self.rest_url + tmpurl, timeout = Sf_rest.timeout , verify=False)
            return [response.status_code , response.content]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    def rest_raw_post(self , tmpurl ,  raw_data , tmpheader = None): 
        try:
            response = self.session.post(url = self.rest_url + tmpurl, data= raw_data , timeout = Sf_rest.timeout , headers = tmpheader ,  verify=False)
            return [response.status_code , response.text ]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    def rest_get(self): 
        try:
            response = self.session.get(url = self.sfurl, timeout = Sf_rest.timeout, params = self.sfparams, verify=False)
            return [response.status_code , response.text]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    def rest_post(self):
        try:
            response = self.session.post(url = self.sfurl, timeout = Sf_rest.timeout, json = self.sfdata,  verify=False)
            return [response.status_code , response.text]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]
        #finally:
        #    return response.status_code

    def rest_delete(self):
        try:
            response = self.session.delete(url = self.sfurl, timeout = Sf_rest.timeout, params = self.sfparams, verify=False)
            # response.raise_for_status()
            return [response.status_code , response.text]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    def rest_put(self):
        try:
            response = self.session.put(url = self.sfurl, json = self.sfdata, timeout = Sf_rest.timeout, verify=False)
            return [response.status_code, response.text]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]
            
    def rest_patch(self):
        try:
            response = self.session.patch(url = self.sfurl, json = self.sfdata, timeout = Sf_rest.timeout, verify=False)
            return [response.status_code, response.text]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    #  First you need to call it
    def rest_login(self):
        try:
            response = self.session.post(self.SF_login, timeout = Sf_rest.timeout, verify=False, data= self.auth)
            return [response.status_code , "Success!"]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    def rest_get_cookie(self):
        try:
            response = self.session.post(self.SF_login, timeout = Sf_rest.timeout, verify=False, data= self.auth)
            return [response.status_code ,response.cookies.items()]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    #  And then finally we need to call it
    def rest_logout(self):
        try:
            response = self.session.post(self.SF_logout,timeout = Sf_rest.timeout, verify=False, data= self.auth)
            return [response.status_code , "Success!"]
        except requests.exceptions.Timeout:
            return [Httplib.SERVICE_UNAVAILABLE , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [Httplib.SERVICE_UNAVAILABLE , "Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [Httplib.SERVICE_UNAVAILABLE , "Unknown"]

    def format_rest_request(self, resource,  params = None, data = None):
        self.sfurl = self.rest_url + resource
        self.switch_parameters(params, data)
        return 1

    def switch_parameters(self, params, data):
        if params:
            self.sfparams = params
        if data:
            self.sfdata = data

    def clean_parameters(self):
        self.sfparams = {}
        self.sfdata = {}

    # Complete process operation and returns the rest interface information directly 
    def auto_run(self, resource , action , params = None, data = None , verbose = False):
        rest_print(verbose , "------------------------")
        rest_print(verbose , "------------------------")
        rest_print(verbose , "try login ....")
        result = self.rest_login()
        if result[0] != Httplib.OK:
            rest_print(verbose , "login fail ....")
            return (-1  , "Login Fail [{0}]".format(result[1]) )
        rest_print(verbose , "login complete ....")
        rest_print(verbose , "------------------------")
        
        self.format_rest_request(resource, params , data)
        rest_print(verbose , "URL:    {0}".format(self.sfurl))
        rest_print(verbose , "Action: {0}".format(action))
        rest_print(verbose , "Params: {0}".format(self.sfparams))
        rest_print(verbose , "Data:   {0}".format(self.sfdata))
        rest_print(verbose , "------------------------")

        if action not in rest_action:
            return ( -1 , "!!! Unknown Action !!!")
    
        if action == "post":
            result = self.rest_post()
        elif action == "get":
            result = self.rest_get()
        elif action == "patch":
            result = self.rest_patch()
        elif action == "put":
            result = self.rest_put()
        elif action == "delete":
            result = self.rest_delete()
        
        self.clean_parameters()

        rest_print(verbose ,"try logout ....")
        self.rest_logout()
        rest_print(verbose ,"logout complete ....")
        rest_print(verbose , "------------------------")


        rest_print(verbose , "Code:   {0}".format(result[0]))
        rest_print(verbose , "Text:   {0}".format(result[1]))
        rest_print(verbose , "------------------------")

        rest_print(verbose , "------------------------")
        
        if result[0] in sf_ok_codes[action]:
            return (0 , result[1])
        else:
            return (-1 ,  Httplib.responses[ result[0] ] , result[1] )


    def auto_run_no_login(self, resource , action , params = None, data = None , verbose = False):
        rest_print(verbose , "------------------------")
        rest_print(verbose , "------------------------")
        
        self.format_rest_request(resource, params , data)
        rest_print(verbose , "URL:    {0}".format(self.sfurl))
        rest_print(verbose , "Action: {0}".format(action))
        rest_print(verbose , "Params: {0}".format(self.sfparams))
        rest_print(verbose , "Data:   {0}".format(self.sfdata))
        rest_print(verbose , "------------------------")

        if action not in rest_action:
            return ( -1 , "!!! Unknown Action !!!")
    
        if action == "post":
            result = self.rest_post()
        elif action == "get":
            result = self.rest_get()
        elif action == "patch":
            result = self.rest_patch()
        elif action == "put":
            result = self.rest_put()
        elif action == "delete":
            result = self.rest_delete()
        
        self.clean_parameters()


        rest_print(verbose , "Code:   {0}".format(result[0]))
        rest_print(verbose , "Text:   {0}".format(result[1]))
        rest_print(verbose , "------------------------")

        rest_print(verbose , "------------------------")
        
        if result[0] in Sf_rest_return_code[action]:
            return (0 , result[1])
        elif result[0] in sf_except_codes[action]:
            return (-1 , result[1])
        else:
            return (-1 ,  Httplib.responses[ result[0] ] )