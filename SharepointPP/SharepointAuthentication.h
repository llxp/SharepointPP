#include "tinyxml2.h"

extern "C" tinyxml2::XMLDocument *prepareSoapRequest(const char *username, const char *password, const char *endpoint);
extern "C" const char *parseResponse(const char *responseXml, const char *endpoint);
extern "C" void curlInit();
extern "C" bool sendPostRequest(const char *url, const char *data, const char *encoding);
extern "C" const char * getPostData(size_t &bufferSize);
extern "C" char ** getResponseHeaders(const char *headerName, size_t &foundHeaderCount);