import requests, json
from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


import os

def rest_print(verbose , msg):
    if verbose:
        print(msg)


class GlobalRestValue:

    ACTION_POST  = "post"
    ACTION_GET = "get"
    ACTION_PATCH = "patch"
    ACTION_PUT = "put"
    ACTION_DELETE = "delete"


    HTTP_PORT = 80
    HTTPS_PORT = 443
    _UNKNOWN = 'UNKNOWN'
    
    # connection states
    _CS_IDLE = 'Idle'
    _CS_REQ_STARTED = 'Request-started'
    _CS_REQ_SENT = 'Request-sent'
    
    # status codes
    # informational
    CONTINUE = 100
    SWITCHING_PROTOCOLS = 101
    PROCESSING = 102
    
    # successful
    OK = 200
    CREATED = 201
    ACCEPTED = 202
    NON_AUTHORITATIVE_INFORMATION = 203
    NO_CONTENT = 204
    RESET_CONTENT = 205
    PARTIAL_CONTENT = 206
    MULTI_STATUS = 207
    IM_USED = 226
    
    # redirection
    MULTIPLE_CHOICES = 300
    MOVED_PERMANENTLY = 301
    FOUND = 302
    SEE_OTHER = 303
    NOT_MODIFIED = 304
    USE_PROXY = 305
    TEMPORARY_REDIRECT = 307
    
    # client error
    BAD_REQUEST = 400
    UNAUTHORIZED = 401
    PAYMENT_REQUIRED = 402
    FORBIDDEN = 403
    NOT_FOUND = 404
    METHOD_NOT_ALLOWED = 405
    NOT_ACCEPTABLE = 406
    PROXY_AUTHENTICATION_REQUIRED = 407
    REQUEST_TIMEOUT = 408
    CONFLICT = 409
    GONE = 410
    LENGTH_REQUIRED = 411
    PRECONDITION_FAILED = 412
    REQUEST_ENTITY_TOO_LARGE = 413
    REQUEST_URI_TOO_LONG = 414
    UNSUPPORTED_MEDIA_TYPE = 415
    REQUESTED_RANGE_NOT_SATISFIABLE = 416
    EXPECTATION_FAILED = 417
    UNPROCESSABLE_ENTITY = 422
    LOCKED = 423
    FAILED_DEPENDENCY = 424
    UPGRADE_REQUIRED = 426
    
    # server error
    INTERNAL_SERVER_ERROR = 500
    NOT_IMPLEMENTED = 501
    BAD_GATEWAY = 502
    SERVICE_UNAVAILABLE = 503
    GATEWAY_TIMEOUT = 504
    HTTP_VERSION_NOT_SUPPORTED = 505
    INSUFFICIENT_STORAGE = 507
    NOT_EXTENDED = 510

    responses = { 
            100: 'Continue', 
            101: 'Switching Protocols', 
            200: 'OK', 
            201: 'Created', 
            202: 'Accepted',
            203: 'Non-Authoritative Information',
            204: 'No Content',
            205: 'Reset Content',
            206: 'Partial Content',

            300: 'Multiple Choices',
            301: 'Moved Permanently',
            302: 'Found',
            303: 'See Other',
            304: 'Not Modified',
            305: 'Use Proxy',
            306: '(Unused)',
            307: 'Temporary Redirect',
            
            400: 'Bad Request',
            401: 'Unauthorized',
            402: 'Payment Required',
            403: 'Forbidden',
            404: 'Not Found',
            405: 'Method Not Allowed',
            406: 'Not Acceptable',
            407: 'Proxy Authentication Required',
            408: 'Request Timeout',
            409: 'Conflict',
            410: 'Gone',
            411: 'Length Required',
            412: 'Precondition Failed',
            413: 'Request Entity Too Large',
            414: 'Request-URI Too Long',
            415: 'Unsupported Media Type',
            416: 'Requested Range Not Satisfiable',
            417: 'Expectation Failed',

            500: 'Internal Server Error',
            501: 'Not Implemented',
            502: 'Bad Gateway',
            503: 'Service Unavailable',
            504: 'Gateway Timeout',
            505: 'HTTP Version Not Supported',
            }


Sf_rest_return_code = { -1 : "Error",  0 : "OK"  }

rest_action = ["get" , "patch" , "put" , "post" , "delete"]

sf_ok_codes = {
    "get" : [GlobalRestValue.OK] , 
    "patch" : [GlobalRestValue.OK, GlobalRestValue.ACCEPTED , GlobalRestValue.NO_CONTENT] , 
    "put" : [GlobalRestValue.OK , GlobalRestValue.NO_CONTENT] , 
    "post" : [GlobalRestValue.OK,GlobalRestValue.CREATED , GlobalRestValue.NO_CONTENT] , 
    "delete" : [GlobalRestValue.OK, GlobalRestValue.NO_CONTENT] , 
}




class Sf_rest: 
    sf_username = 'admin'
    sf_password = 'admin'
    timeout = 5

    def __init__(self, server , sfurl = "", sfparams = {}, sfdata = {}):
        self.server = server
        self.rest_url = "https://" + server + "/rest/v1/"
        self.SF_login = self.rest_url + "login" 
        self.SF_logout = self.rest_url + "logout"
        self.sfurl = sfurl
        self.sfparams = sfparams
        self.sfdata = sfdata
        self.session  = requests.Session() 
        self.auth ={'username': Sf_rest.sf_username,'password':Sf_rest.sf_password}
    
    def rest_raw_get(self , short_url): 
        try:
            response = self.session.get(url = self.rest_url + short_url, timeout = Sf_rest.timeout,verify=False)
            # response.raise_for_status()
            return [response.status_code , response.content]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]

    def rest_raw_post(self , tmpurl ,  raw_data , tmpheader = None): 
        try:
            response = self.session.post(url = self.rest_url + tmpurl, data= raw_data , timeout = Sf_rest.timeout , headers = tmpheader ,  verify=False)
            return [response.status_code , response.text ]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]

    def rest_get(self): 
        try:
            response = self.session.get(url = self.sfurl, timeout = Sf_rest.timeout, params = self.sfparams, verify=False)
            try:
                return [response.status_code , response.json()]
            except:
                return [response.status_code , response.text]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]

    def rest_post(self):
        try:
            response = self.session.post(url = self.sfurl, timeout = Sf_rest.timeout, json = self.sfdata,  verify=False)
            return [response.status_code , response.text]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]
        #finally:
        #    return response.status_code

    def rest_delete(self):
        try:
            response = self.session.delete(url = self.sfurl, timeout = Sf_rest.timeout, params = self.sfparams,verify=False)
            # response.raise_for_status()
            return [response.status_code , response.text]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]

    def rest_put(self):
        try:
            response = self.session.put(url = self.sfurl, json = self.sfdata, timeout = Sf_rest.timeout, verify=False)
            # response.raise_for_status()
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]
        finally:
            return [response.status_code, response.text]

    def rest_patch(self):
        try:
            response = self.session.patch(url = self.sfurl, json = self.sfdata, timeout = Sf_rest.timeout, verify=False)
            # response.raise_for_status()
            # print(response.text)
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]
        finally:
            return [response.status_code, response.text]

    #  First you need to call it
    def rest_login(self):
        try:
            response = self.session.post(self.SF_login, timeout = Sf_rest.timeout, verify=False, data= self.auth)
            return [response.status_code , "Success!"]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]

    def rest_get_cookie(self):
        try:
            response = self.session.post(self.SF_login, timeout = Sf_rest.timeout, verify=False, data= self.auth)
            return [response.status_code ,response.cookies.items()]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]

    #  And then finally we need to call it
    def rest_logout(self):
        try:
            response = self.session.post(self.SF_logout,timeout = Sf_rest.timeout, verify=False, data= self.auth)
            return [response.status_code , "Success!"]
        except requests.exceptions.Timeout:
            return [response.status_code , "Timeout!"]
        except requests.exceptions.ConnectionError:
            return [response.status_code ,"Connection impassability!"]
        except requests.exceptions.HTTPError:
            return [response.status_code]

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
        if result[0] != GlobalRestValue.OK:
            rest_print(verbose , "login fail ....")
            return (-1  , "Login Fail")
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
        self.rest_login()
        rest_print(verbose ,"logout complete ....")
        rest_print(verbose , "------------------------")

        self.rest_logout()

        rest_print(verbose , "Code:   {0}".format(result[0]))
        rest_print(verbose , "Text:   {0}".format(result[1]))
        rest_print(verbose , "------------------------")

        rest_print(verbose , "------------------------")
        
        if result[0] in sf_ok_codes[action]:
            return (0 , result[1])
        else:
            return (-1 ,  GlobalRestValue.responses[ result[0] ] , result[1] )


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
        
        if result[0] in sf_ok_codes[action]:
            return (0 , result[1])
        else:
            return (-1 ,  GlobalRestValue.responses[ result[0] ] )
