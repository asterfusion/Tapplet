#rest
REST_VERSION_PATH = '/rest/v1/'
REST_LOGIN_PATH = '/login'
REST_LOGOUT_PATH = '/logout'

#rest param
REST_PATAM_SELECTOR = 'selector'
REST_PATAM_OFFSET = 'offset'
REST_PATAM_SORT = 'sort'
REST_PATAM_KEY = 'keys'
REST_PATAM_DEPTH = 'depth'
REST_PATAM_INDEX = 'index'

DEPTH_VALUE_MIN = 0
DEPTH_VALUE_MAX = 2

#HTTP
HTTP = 'http'
HTTPS = 'https'
HTTP_HEADER_LINK = 'link'
HTTP_HEADER_CONTENT_TYPE = 'Content-Type'
HTTP_HEADER_CONTENT_LENGTH = 'Content-Length'
HTTP_HEADER_CONDITIONAL_IF_MATCH = "If-match"
HTTP_HEADER_ALLOW = 'allow'
HTTP_CONTENT_TYPE_JSON = 'application/json; charset=UTF-8'

#request type
HTTP_REQUEST_GET = 'GET'
HTTP_REQUEST_POST = 'POST'
HTTP_REQUEST_PUT = 'PUT'
HTTP_REQUEST_DELETE = 'DELETE'
HTTP_REQUEST_PATCH = 'PATCH'
HTTP_REQUEST_OPTIONS = 'OPTIONS'


class Httplib:
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

