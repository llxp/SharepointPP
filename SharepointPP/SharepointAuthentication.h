#include <string>
#include <map>
#include <vector>
#include "tinyxml2.h"

tinyxml2::XMLDocument *prepareSoapRequest(const std::string &username, const std::string &password, const std::string &endpoint);
std::string parseResponse(const std::string &responseXml, const std::string &endpoint);
bool sendPostRequest(const std::string &url, const std::string &data, const std::string &encoding, const std::vector<std::pair<std::string, std::string>> &cookies = std::vector<std::pair<std::string, std::string>>());
std::string getPostData();
std::vector<std::pair<std::string, std::string>> getResponseHeaders(const std::string &headerName);
std::vector<std::pair<std::string, std::string>> getResponseCookies();