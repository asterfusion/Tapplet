import json
from conf.globalValue import Httplib

class APIException(Exception):
    status_code = Httplib.BAD_REQUEST
    status = Httplib.responses[status_code]
    def __init__(self, detail=None): 
        self.detail = detail
    def __str__(self):
        error_json = {}
        error_json['message'] = self.detail
        return json.dumps(error_json) 

class DataValidationFailed(APIException):
    status_code = Httplib.BAD_REQUEST
    status = Httplib.responses[status_code]

class ParseError(APIException):
    status_code = Httplib.BAD_REQUEST
    status = Httplib.responses[status_code]

class AuthenticationFailed(APIException):
    status_code = Httplib.UNAUTHORIZED
    status = Httplib.responses[status_code]

class NotAuthenticated(APIException):
    status_code = Httplib.UNAUTHORIZED
    status = Httplib.responses[status_code] 

class NotFound(APIException):
    status_code = Httplib.NOT_FOUND
    status = Httplib.responses[status_code]

class NotModified(APIException):
    status_code = Httplib.NOT_MODIFIED
    status = Httplib.responses[status_code]

class MethodNotAllowed(APIException):
    status_code = Httplib.METHOD_NOT_ALLOWED
    status = Httplib.responses[status_code] 

class TransactionFailed(APIException): 
    status_code = Httplib.INTERNAL_SERVER_ERROR
    status = Httplib.responses[status_code]

class PatchOperationFailed(APIException): 
    status_code = Httplib.BAD_REQUEST
    status = Httplib.responses[status_code]

class ParameterNotAllowed(APIException):
    status_code = Httplib.BAD_REQUEST
    status = Httplib.responses[status_code] 

class LengthRequired(APIException):
    status_code = Httplib.LENGTH_REQUIRED
    status = Httplib.responses[status_code]

class ForbiddenMethod(APIException): 
    status_code = Httplib.FORBIDDEN
    status = Httplib.responses[status_code]

class InternalError(APIException): 
    status_code = Httplib.INTERNAL_SERVER_ERROR
    status = Httplib.responses[status_code]

class PasswordChangeError(APIException):
    def __init__(self, detail=None, status_code=Httplib.INTERNAL_SERVER_ERROR):
        self.detail = detail 
        self.status_code = status_code 
        self.status = Httplib.responses[status_code]
