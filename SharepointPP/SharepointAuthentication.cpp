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
#include <algorithm>
#include <iostream>

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
								return std::string();
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
	return std::string();
}

CURL *curlHandle = nullptr;
std::string responseBuffer;
std::vector<std::pair<std::string, std::string>> responseHeaders;
bool initialized = false;
FILE *responseData = nullptr;
FILE *headerData = nullptr;
std::vector<std::pair<std::string, std::string>> cookieStore;

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

std::string timeStampToHReadble(const time_t rawtime, const std::string &formatString)
{
	struct tm * dt;
	char buffer [30];
	dt = localtime(&rawtime);
	strftime(buffer, sizeof(buffer), formatString.data(), dt);
	return std::string(buffer);
}

std::string reformatCookie(const std::string &host, const std::string &value, const std::string &path, bool httpOnly, bool secure, size_t expireDate)
{
	std::string output;
	output += value;
	output += "; ";
	if (host.length() > 0) {
		output += "domain=";
		output += host;
		output += "; ";
	}
	if (path.length() > 0) {
		output += "path=";
		output += path;
		output += "; ";
	}
	if (secure) {
		output += "secure; ";
	}
	if (httpOnly) {
		output += "HttpOnly; ";
	}
	if (expireDate > 0) {
		output += "Expires=";
		output += timeStampToHReadble(expireDate, "%a, %d %b %Y %I:%M:%S GMT");
		output += "; ";
	}
	return output;
}

void readAllCookies(struct curl_slist *cookieStruct)
{
	if (cookieStruct != nullptr) {
		std::string cookieString(cookieStruct->data, strlen(cookieStruct->data));
		bool httpOnly = false;
		bool secure = false;
		std::string host = "";
		std::string path = "";
		std::string name = "";
		std::string value = "";
		size_t expireDate = 0;
		std::istringstream f(cookieString);
		std::string s;
		std::vector<std::string> cookieStrings;
		while (getline(f, s, '\t')) {
			cookieStrings.push_back(s);
		}
		if (cookieStrings[0].substr(0, std::string("#HttpOnly_").length()) == std::string("#HttpOnly_")) {
			httpOnly = true;
			host = cookieStrings[0].substr(std::string("#HttpOnly_").length());
		} else {
			host = cookieStrings[0];
		}
		path = cookieStrings[2];
		if (cookieStrings[3].length() > 0 && cookieStrings[3] == "TRUE") {
			secure = true;
		}
		std::stringstream ss(cookieStrings[4]);
		ss >> expireDate;
		name = cookieStrings[5];
		if (cookieStrings.size() == 7) {
			value = cookieStrings[6];
		}
		cookieStore.push_back(std::pair<std::string, std::string>(name, reformatCookie(host, value, path, httpOnly, secure, expireDate)));
	}
	if (cookieStruct->next != nullptr) {
		readAllCookies(cookieStruct->next);
	}
}
void readCookies(CURL *curl)
{
	CURLcode res;
	struct curl_slist *cookies;
	struct curl_slist *nc;
	int i;

	res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
	if(res != CURLE_OK) {
		fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n",
			curl_easy_strerror(res));
	}
	nc = cookies;
	readAllCookies(nc);
	curl_slist_free_all(cookies);
}

size_t header_function(void *buffer, size_t size, size_t nmemb, void *userp) {
	const char *bufferStr = static_cast<const char *>(buffer);
    std::string s = std::string(bufferStr, size * nmemb);
	size_t pos = s.find(":");
	std::cout << "original header value: " << s << std::endl;
    if(s.length() > 0 && pos != std::string::npos) {
		std::string name = s.substr(0, pos);
		std::string value = s.substr(pos + 1);
		if (value[0] == ' ') {
			value = value.substr(1);
		}
		//value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
        if(name.length() > 0 && value.length() > 0) {
			/*if (name == "Set-Cookie") {
				size_t cookiePos = value.find('=');
				if (cookiePos != std::string::npos) {
					std::string cookieName = value.substr(0, cookiePos);
					std::string cookieValue = value.substr(cookiePos + 1, value.length() - cookiePos - 1);
					if (cookieValue[cookieValue.length() - 1] == '\n') {
						cookieValue = cookieValue.substr(0, cookieValue.length() - 1);
					}
					std::string debugString = "|";
					debugString += cookieName;
					debugString += " = ";
					debugString += cookieValue;
					debugString += " | ";
					std::cout << debugString << std::endl;
					cookieStore.push_back(std::pair<std::string, std::string>(cookieName, cookieValue));
				}
			}*/
            responseHeaders.push_back(
				std::pair<std::string, std::string>(name, value));
			std::cout << "added header: " << name << " | value: " << value << " | " << std::endl;
        }
    }
    //responseHeaders.push_back(std::pair<std::string, std::string>(s, s));
	std::cout << __FUNCTION__ << std::endl;
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
	// clear all the cookies from previous request
	cookieStore.clear();
	// get new hendle for performing the request
    curlHandle = curlInit();
	if (!curlHandle) {
		return false;
	}
	// set url for the request
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.data());
	curl_easy_setopt(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, ""); /* start cookie engine */ 

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
		std::string cookieString;
		for (std::vector<std::pair<std::string, std::string>>::const_iterator it = cookies.begin(); it != cookies.end(); ++it) {
			cookieString += std::move(it->first + '=' + it->second);
		}
		printf("cookieString: %s\n", cookieString.data());
		curl_easy_setopt(curlHandle, CURLOPT_COOKIE, cookieString.data());
	}
	if (data.length() == 0) {
		std::string contentLengthHeader("Content-Length: 0");
		headers = curl_slist_append(headers, contentLengthHeader.data());
		std::string contentType = "Content-Type: application/x-www-form-urlencoded";
		headers = curl_slist_append(headers, contentType.data());
	}
	curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
	//curl_easy_setopt(curlHandle, CURLOPT_TRANSFER_ENCODING, 1);

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
    //curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, header_function);
	//curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &headerData);

	// perform the actual request
    CURLcode success = curl_easy_perform(curlHandle);

	// check if the request was successful
    if(success != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(success));
    }

	readCookies(curlHandle);

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

std::vector<std::pair<std::string, std::string>> getResponseCookies()
{
	return cookieStore;
}
