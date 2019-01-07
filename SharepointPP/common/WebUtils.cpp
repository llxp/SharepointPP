#include "WebUtils.h"

#include <sstream>
#include <algorithm>

#include <curl/curl.h>

#include "ConversionUtils.h"

using Microsoft::Sharepoint::WebUtils;

std::vector<std::pair<std::string, std::string>> WebUtils::m_responseHeaders;
std::string WebUtils::m_responseBuffer;
WebUtils::cookieType WebUtils::m_cookieStore;
void *WebUtils::m_curlHandle;

std::string timeStampToHReadble(const time_t rawtime, const std::string &formatString);
std::string reformatCookie(const std::string &host, const std::string &value, const std::string &path, bool httpOnly, bool secure, size_t expireDate);
void readAllCookies(struct curl_slist *cookieStruct);

WebUtils::WebUtils()
{
}


WebUtils::~WebUtils()
{
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

std::string cookieToString(struct curl_slist *cookieStruct)
{
	return std::string(cookieStruct->data, strlen(cookieStruct->data));
}

std::vector<std::string> splitCookieString(std::string &&cookieString)
{
	std::istringstream f(cookieString);
	std::string s;
	std::vector<std::string> cookieStrings;
	while (getline(f, s, '\t')) {
		cookieStrings.push_back(s);
	}
	return cookieStrings;
}

void readSingleCookie(struct curl_slist *cookieStruct)
{
	std::vector<std::string> cookieStrings(
		std::move(splitCookieString(
			std::move(cookieToString(cookieStruct)))));
	if (cookieStrings.size() >= 6) {
		bool httpOnly = false;
		bool secure = false;
		std::string host = "";
		std::string path(cookieStrings[2]);
		std::string name(cookieStrings[5]);
		std::string value = "";
		size_t expireDate(ConversionUtils::stringToSize_T(cookieStrings[4]));

		if (cookieStrings[0].length() >= std::string("#HttpOnly_").length() &&
			cookieStrings[0].substr(0, std::string("#HttpOnly_").length()) == std::string("#HttpOnly_")) {
			httpOnly = true;
			host = cookieStrings[0].substr(std::string("#HttpOnly_").length());
		} else {
			host = cookieStrings[0];
		}
		if (cookieStrings[3].length() > 0 && cookieStrings[3] == "TRUE") {
			secure = true;
		}
		if (cookieStrings.size() >= 7) {
			value = cookieStrings[6];
		}
		WebUtils::addCookie(name, reformatCookie(host, value, path, httpOnly, secure, expireDate));
	}
}

void readAllCookies(struct curl_slist *cookieStruct)
{
	if (cookieStruct != nullptr) {
		readSingleCookie(cookieStruct);
		if (cookieStruct->next != nullptr) {
			readAllCookies(cookieStruct->next);
		}
	}
}

void readCookies(CURL *curl)
{
	CURLcode res;
	struct curl_slist *cookies;

	res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
	if(res != CURLE_OK) {
		fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n",
			curl_easy_strerror(res));
	}
	readAllCookies(cookies);
	curl_slist_free_all(cookies);
}

bool Microsoft::Sharepoint::WebUtils::sendPostRequest(const std::string &url, const std::string &data, const std::string &contentType, const cookieType &cookies)
{
	// clear all headers from previous request
	m_responseHeaders.clear();
	// clear the response buffer from previous request
	m_responseBuffer.clear();
	// get new hendle for performing the request
	// clear all the cookies from previous request
	m_cookieStore.clear();

	// init new curl handle
	curl_global_init(CURL_GLOBAL_ALL);
	m_curlHandle = curl_easy_init();
	if (!m_curlHandle) {
		return false;
	}

	// set url for the request
	curl_easy_setopt(m_curlHandle, CURLOPT_URL, url.data());
	// set post as method to use for curl
	curl_easy_setopt(m_curlHandle, CURLOPT_CUSTOMREQUEST, "POST");
	// start cookie engine
	curl_easy_setopt(m_curlHandle, CURLOPT_COOKIEFILE, "");

	struct curl_slist *headers = nullptr;

	if (contentType.length() > 0) {
		std::string contentTypeCopy = contentType;
		std::string contentTypePrefix = contentTypeCopy.substr(0, std::string("Content-Type:").length());
		if (
			contentTypePrefix != std::string("Content-Type:") &&
			contentTypePrefix != std::string("content-Type:") &&
			contentTypePrefix != std::string("Content-type:") &&
			contentTypePrefix != std::string("content-type:")) {
			contentTypeCopy = std::string("Content-Type:") + contentTypeCopy;
		}
		headers = curl_slist_append(headers, contentTypeCopy.data());
	}

	if (data.length() == 0) {
		std::string contentLengthHeader("Content-Length: 0");
		headers = curl_slist_append(headers, contentLengthHeader.data());
	}

	// set the header structure to the curl handle
	if (headers != nullptr) {
		curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, headers);
	}

	if (data.length() > 0) {
		curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDS, data.data());
		//curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE, data.length());
		//curl_easy_setopt(m_curlHandle, CURLOPT_INFILESIZE,(curl_off_t)data.size());
	}

	// set the cookies for the request
	if (cookies.size() > 0) {
		std::string cookieString;
		for (std::vector<std::pair<std::string, std::string>>::const_iterator it = cookies.begin(); it != cookies.end(); ++it) {
			cookieString += std::move(it->first + '=' + it->second);
		}
		curl_easy_setopt(m_curlHandle, CURLOPT_COOKIE, cookieString.data());
	}

#ifdef _DEBUG
	curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);
#endif

	// set the write function for getting the output from the response
	curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, curlWriteFunction);
	// set the header function for getting the headers from the response
	curl_easy_setopt(m_curlHandle, CURLOPT_HEADERFUNCTION, curlHeaderFunction);

	// perform the actual request
	CURLcode success = curl_easy_perform(m_curlHandle);

	// check if the request was successful
	if(success != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(success));
	}

	readCookies(m_curlHandle);

	curl_easy_cleanup(m_curlHandle);

	if (headers != nullptr) {
		curl_slist_free_all(headers);
		headers = nullptr;
	}

	m_curlHandle = nullptr;

	return success == CURLE_OK;
}

WebUtils::cookieType WebUtils::getCookiesAfterRequest()
{
	return m_cookieStore;
}

std::string Microsoft::Sharepoint::WebUtils::getResponseData()
{
	return m_responseBuffer;
}

void Microsoft::Sharepoint::WebUtils::addCookie(const std::string & name, const std::string & value)
{
	m_cookieStore.push_back(std::pair<std::string, std::string>(name, value));
}

std::string Microsoft::Sharepoint::WebUtils::getUnescapedString(const std::string & escapedString)
{
	std::string tempBuffer;
	char *curlUnescapedString = curl_unescape(escapedString.data(), static_cast<int>(escapedString.length()));
	tempBuffer.resize(strlen(curlUnescapedString));
	strcpy(&tempBuffer[0], curlUnescapedString);
	curl_free(curlUnescapedString);
	return tempBuffer;
}

size_t Microsoft::Sharepoint::WebUtils::curlWriteFunction(void * buffer, size_t size, size_t nmemb, void * userp)
{
	std::string tempStr(static_cast<const char *>(buffer), nmemb);
	m_responseBuffer.append(std::move(tempStr));
	return nmemb * size;
}

size_t Microsoft::Sharepoint::WebUtils::curlHeaderFunction(void * buffer, size_t size, size_t nmemb, void * userp)
{
	const char *bufferStr = static_cast<const char *>(buffer);
	std::string s = std::string(bufferStr, size * nmemb);
	size_t pos = s.find(":");
	if (s.length() > 0 && pos != std::string::npos) {
		std::string name = s.substr(0, pos);
		std::string value = s.substr(pos + 1);
		if (value[0] == ' ') {
			value = value.substr(1);
		}
		if (name.length() > 0) {
			m_responseHeaders.push_back(
				std::pair<std::string, std::string>(name, value));
		}
	}
	return size * nmemb;
}
