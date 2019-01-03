#include "SharepointAuthentication.h"

#ifdef __STDC_LIB_EXT1__
constexpr bool can_have_strcpy_s = true;
#else
constexpr bool can_have_strcpy_s = false;
#endif

#include <cstring>
#include <string.h>
#include <vector>
#include <sstream>
#include <iterator>
#include <map>

#include "tinyxml2.h"
#include <curl/curl.h>

tinyxml2::XMLDocument *prepareSoapRequest(const char *username, const char *password, const char *endpoint) {
    tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument();
    doc->LoadFile("STSRequest.xml");
    tinyxml2::XMLElement *envelope = doc->FirstChildElement("s:Envelope");
    if(envelope != nullptr) {
        tinyxml2::XMLElement *header =  envelope->FirstChildElement("s:Header");
        if(header != nullptr) {
            tinyxml2::XMLElement *security = header->FirstChildElement("o:Security");
            if(security != nullptr) {
                tinyxml2::XMLElement *usernameToken = security->FirstChildElement("o:UsernameToken");
                if(usernameToken != nullptr) {
                    tinyxml2::XMLElement *usernameElement = usernameToken->FirstChildElement("o:Username");
                    tinyxml2::XMLElement *passwordElement = usernameToken->FirstChildElement("o:Password");
                    if(usernameElement != nullptr && passwordElement != nullptr) {
                        usernameElement->SetText(username);
                        passwordElement->SetText(password);
                    } else {
                        return nullptr;
                    }
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
        tinyxml2::XMLElement *body = envelope->FirstChildElement("s:Body");
        if(body != nullptr) {
            tinyxml2::XMLElement *requestSecurityToken = body->FirstChildElement("t:RequestSecurityToken");
            if(requestSecurityToken != nullptr) {
                tinyxml2::XMLElement *appliesTo = requestSecurityToken->FirstChildElement("wsp:AppliesTo");
                if(appliesTo != nullptr) {
                    tinyxml2::XMLElement *endpointReference = appliesTo->FirstChildElement("a:EndpointReference");
                    if(endpointReference != nullptr) {
                        tinyxml2::XMLElement *address = endpointReference->FirstChildElement("a:Address");
                        if(address != nullptr) {
                            address->SetText(endpoint);
                        } else {
                            return nullptr;
                        }
                    } else {
                        return nullptr;
                    }
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
    return doc;
}

const char *parseResponse(const char *responseXml, const char *endpoint) {
    tinyxml2::XMLDocument doc;
    doc.Parse(responseXml);
    tinyxml2::XMLElement *envelope = doc.FirstChildElement("S:Envelope");
    if(envelope != nullptr) {
        tinyxml2::XMLElement *body = envelope->FirstChildElement("S:Body");
        if(body != nullptr) {
            tinyxml2::XMLElement *requestSecurityTokenResponse = body->FirstChildElement("wst:RequestSecurityTokenResponse");
            if(requestSecurityTokenResponse != nullptr) {
                tinyxml2::XMLElement *appliesTo = requestSecurityTokenResponse->FirstChildElement("wsp:AppliesTo");
                if(appliesTo != nullptr) {
                    tinyxml2::XMLElement *endpointReference = appliesTo->FirstChildElement("wsa:EndpointReference");
                    if(endpointReference != nullptr) {
                        tinyxml2::XMLElement *address = endpointReference->FirstChildElement("wsa:Address");
                        if(address != nullptr) {
                            if(strcmp(address->GetText(), endpoint) != 0) {
                                return nullptr;
                            }
                        }
                    }
                }
                tinyxml2::XMLElement *requestedSecurityToken = requestSecurityTokenResponse->FirstChildElement("wst:RequestedSecurityToken");
                if(requestedSecurityToken != nullptr) {
                    tinyxml2::XMLElement *binarySecurityToken = requestedSecurityToken->FirstChildElement("wsse:BinarySecurityToken");
                    if(binarySecurityToken != nullptr) {
                        char *tempBuffer = new char (strlen(binarySecurityToken->GetText()));
                        char *curlUnescapedString = curl_unescape(binarySecurityToken->GetText(), 0);
                        strcpy(tempBuffer, curlUnescapedString);
                        curl_free(curlUnescapedString);
                        return tempBuffer;
                    }
                }
            }
        }
    }
    return nullptr;
}

CURL *curlHandle = nullptr;
std::string responseBuffer;
std::vector<std::pair<std::string, std::string>> responseHeaders;
bool initialized = false;

void curlInit() {
    curl_global_init(CURL_GLOBAL_ALL);
    initialized = true;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    std::string tempStr(reinterpret_cast<const char *>(buffer), nmemb);
    responseBuffer.append(std::move(tempStr));
    return nmemb;
}

size_t header_function(void *buffer, size_t size, size_t nmemb, void *userp) {
    std::string s(reinterpret_cast<char *>(buffer), size * nmemb);
    char *headerString = static_cast<char *>(buffer);
    if(headerString != nullptr) {
        char *name = strtok(headerString, ":");
        if(name != nullptr) {
            responseHeaders.push_back(std::pair<std::string, std::string>(std::string(name), headerString+strlen(name)));
        }
    }
    responseHeaders.push_back(std::pair<std::string, std::string>(s, s));
    return size * nmemb;
}

bool sendPostRequest(const char *url, const char *data, const char *encoding) {
    if(!initialized) {
        curlInit();
    }
    responseHeaders.clear();
    responseBuffer.clear();
    curlHandle = curl_easy_init();
    curl_easy_setopt(curlHandle, CURLOPT_URL, url);

    struct curl_slist *headers=NULL;
    headers = curl_slist_append(headers, encoding);

    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, strlen(data));
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);

    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, header_function);

    char *errorMessage = static_cast<char *>(malloc(sizeof(char) * CURL_ERROR_SIZE + 1));
    curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errorMessage);
    CURLcode success = curl_easy_perform(curlHandle);

    if(success != 0) {
        printf("curlError: %s\n", errorMessage);
    }

    curl_slist_free_all(headers);

    return success == 0;
}

const char * getPostData(size_t &bufferSize) {
    if(responseBuffer.length() > 0) {
            bufferSize = responseBuffer.length();
        return responseBuffer.data();
    }
    bufferSize = 0;
    return nullptr;
}

char ** getResponseHeaders(const char *headerName, size_t &foundHeaderCount) {
    int headerCount = 0;
    for(auto &header : responseHeaders) {
        if(header.first == headerName) {
            ++headerCount;
        }
    }
    if(headerCount == 0) {
        return nullptr;
    }
    char **foundHeaders = static_cast<char **>(malloc(sizeof(char *) * headerCount));
    headerCount = 0;
    char **beginFoundHeaders = foundHeaders;
    for(auto &header : responseHeaders) {
        if(header.first == headerName) {
            *foundHeaders = static_cast<char *>(malloc(sizeof(char) * header.second.length() + 1));
            #ifdef __STDC_LIB_EXT1__
                strncpy_s(*foundHeaders, header.second.length() + 1, header.second.data(), header.second.length());
            
            #else
                strncpy(*foundHeaders, header.second.data(), header.second.length());
            #endif
        
            ++foundHeaders;
        }
    }
    return beginFoundHeaders;
}