#ifndef _HTTP_DEFS__H_
#define _HTTP_DEFS__H_

#ifndef HTTP_DEF_STR
#define HTTP_DEF_STR(var,str)                   \
    extern const char var[]
#endif

/* 2xx */
HTTP_DEF_STR(HTTP_STATUS_200, "200 OK");
HTTP_DEF_STR(HTTP_STATUS_204, "204 No Content");
/* 3xx */
HTTP_DEF_STR(HTTP_STATUS_301, "301 Moved Permanently");
HTTP_DEF_STR(HTTP_STATUS_302, "302 Found");
HTTP_DEF_STR(HTTP_STATUS_303, "303 See Other");
HTTP_DEF_STR(HTTP_STATUS_304, "304 Not Modified");
/* 4xx */
HTTP_DEF_STR(HTTP_STATUS_401, "401 Unauthorized");
HTTP_DEF_STR(HTTP_STATUS_403, "403 Forbidden");
HTTP_DEF_STR(HTTP_STATUS_404, "404 Not Found");
HTTP_DEF_STR(HTTP_STATUS_405, "405 Method Not Allowed");
HTTP_DEF_STR(HTTP_STATUS_406, "406 Not Acceptable");
HTTP_DEF_STR(HTTP_STATUS_411, "411 Length Required");
HTTP_DEF_STR(HTTP_STATUS_413, "413 Request Entity Too Large");
HTTP_DEF_STR(HTTP_STATUS_414, "414 Request-URI Too Long");
HTTP_DEF_STR(HTTP_STATUS_415, "415 Unsupported Media Type");
/* 5xx */
HTTP_DEF_STR(HTTP_STATUS_500, "500 Internal Server Error");
HTTP_DEF_STR(HTTP_STATUS_501, "501 Not Implemented");
HTTP_DEF_STR(HTTP_STATUS_503, "503 Service Unavailable");
/* content types */
HTTP_DEF_STR(HTTP_CONTENT_TYPE_TEXT_PLAIN, "text/plain");
HTTP_DEF_STR(HTTP_CONTENT_TYPE_TEXT_HTML, "text/html");
HTTP_DEF_STR(HTTP_CONTENT_TYPE_IMAGE_GIF, "image/gif");
HTTP_DEF_STR(HTTP_CONTENT_TYPE_IMAGE_JPG, "image/jpeg");
HTTP_DEF_STR(HTTP_CONTENT_TYPE_IMAGE_PNG, "image/png");
HTTP_DEF_STR(HTTP_CONTENT_TYPE_APPLICATION_XML, "application/xml");
HTTP_DEF_STR(HTTP_CONTENT_TYPE_APPLICATION_JSON, "application/json");

#endif // _HTTP_DEFS__H_
