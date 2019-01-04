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

tinyxml2::XMLDocument *prepareSoapRequest(const std::string &username, const std::string &password, const std::string &endpoint) {
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
                        usernameElement->SetText(username.data());
                        passwordElement->SetText(password.data());
                    } else {
						printf("no username/password element found...\n");
                        return nullptr;
                    }
                } else {
					printf("no usernameToken element found...\n");
                    return nullptr;
                }
            } else {
				printf("no security element found...\n");
                return nullptr;
            }
        } else {
			printf("no header element found...\n");
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
                            address->SetText(endpoint.data());
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

std::string parseResponse(const std::string &responseXml, const std::string &endpoint) {
    tinyxml2::XMLDocument doc;
    doc.Parse(responseXml.data());
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
                            if(strcmp(address->GetText(), endpoint.data()) != 0) {
                                return nullptr;
                            }
                        }
                    }
                }
                tinyxml2::XMLElement *requestedSecurityToken = requestSecurityTokenResponse->FirstChildElement("wst:RequestedSecurityToken");
                if(requestedSecurityToken != nullptr) {
                    tinyxml2::XMLElement *binarySecurityToken = requestedSecurityToken->FirstChildElement("wsse:BinarySecurityToken");
                    if(binarySecurityToken != nullptr) {
                        char *tempBuffer = new char [strlen(binarySecurityToken->GetText())];
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
FILE *responseData = nullptr;
FILE *headerData = nullptr;

CURL *curlInit() {
    curl_global_init(CURL_GLOBAL_ALL);
    initialized = true;
	return curl_easy_init();
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    std::string tempStr(reinterpret_cast<const char *>(buffer), nmemb);
    responseBuffer.append(std::move(tempStr));
	printf(__FUNCTION__);
	printf("\n");
    return nmemb;
}

size_t header_function(void *buffer, size_t size, size_t nmemb, void *userp) {
    std::string s(reinterpret_cast<char *>(buffer), size * nmemb);
	size_t pos = s.find(":");
    if(s.length() > 0 && pos != std::string::npos) {
		std::string name = s.substr(0, pos);
        if(name.length() > 0) {
			printf("%s\n", s.substr(pos + 2).data());
            responseHeaders.push_back(
				std::pair<std::string, std::string>(
					std::string(name),
					s.substr(pos + 2)));
        }
    }
    //responseHeaders.push_back(std::pair<std::string, std::string>(s, s));
	printf(__FUNCTION__);
	printf("\n");
    return size * nmemb;
}

bool sendPostRequest(const std::string &url, const std::string &data, const std::string &encoding, const std::vector<std::pair<std::string, std::string>> &cookies) {
    //if(!initialized) {
        //curlInit();
    //}
	// clear all headers from previous request
    responseHeaders.clear();
	// clear the response buffer from previous request
    responseBuffer.clear();
	// get new hendle for performing the request
    curlHandle = curlInit();
	if (!curlHandle) {
		return false;
	}
	// set url for the request
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.data());
	curl_easy_setopt(curlHandle, CURLOPT_POST, 1L);

	// if set, set the encoding header
	struct curl_slist *headers = NULL;
	if (encoding.length() > 0) {
		//headers = new curl_slist();
		//headers->data = new char[strlen(encoding) + 1];
		//strncpy(headers->data, encoding, strlen(encoding));
		//headers->data[strlen(encoding)] = '\0';
		headers = curl_slist_append(NULL, encoding.data());
	}
	if (cookies.size() > 0) {
		for (std::vector<std::pair<std::string, std::string>>::const_iterator it = cookies.begin(); it != cookies.end(); ++it) {
			std::string tempString = "Cookie:" + it->second;
			printf("tempString: %s\n", tempString.data());
			headers = curl_slist_append(headers, tempString.data());
		}
	}
	if (data.length() == 0) {
		std::string contentLengthHeader("Content-Length: 0");
		headers = curl_slist_append(headers, contentLengthHeader.data());
		std::string transferEncoding = "Transfer-Encoding: chunked";
		headers = curl_slist_append(headers, transferEncoding.data());
	}
	curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curlHandle, CURLOPT_TRANSFER_ENCODING, 1);

#ifdef _DEBUG
	curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L);
#endif

	// set post data
	if (data.length() > 0) {
		curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, data.data());
		curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, data.length());
	}
	curl_easy_setopt(curlHandle, CURLOPT_INFILESIZE,(curl_off_t)data.size());

	// advise curl to follow redirects
    //curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);

	// set the write function for getting the output from the response
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
	//curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &responseData);
	// set the header function for getting the headers set by the response
    curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, header_function);
	//curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &headerData);

	// perform the actual request
    CURLcode success = curl_easy_perform(curlHandle);

	// check if the request was successful
    if(success != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(success));
    }

	curl_easy_cleanup(curlHandle);

	if (headers != nullptr) {
		//delete[] headers->data;
		//delete headers;
		curl_slist_free_all(headers);
		headers = NULL;
	}

    return success == CURLE_OK;
}

std::string getPostData() {
	return responseBuffer;
}

std::vector<std::pair<std::string, std::string>> getResponseHeaders(const std::string &headerName) {
    int headerCount = 0;
	std::vector<std::pair<std::string, std::string>> foundHeaders;
    for(auto &header : responseHeaders) {
        if(header.first == headerName) {
			foundHeaders.push_back(std::pair<std::string, std::string>(header.first, header.second));
        }
    }
	return foundHeaders;
}