// Copyright (c) 2011 rubicon IT GmbH
#include "WebRequest.h"

#include <curl/curl.h>

#include <sstream>
#include <algorithm>
#include <utility>
#include <string>
#include <vector>

#include "ConversionUtils.h"

using Microsoft::Sharepoint::WebRequest;
using Microsoft::Sharepoint::WebResponse;

class GlobalCurlInit
{
 public:
	GlobalCurlInit()
	{
		curl_global_init(CURL_GLOBAL_ALL);
	}
	~GlobalCurlInit()
	{
		curl_global_cleanup();
	}
};

static GlobalCurlInit globalCurlInstance;

std::string timeStampToHReadble(
	const time_t rawtime,
	const std::string &formatString)
{
	struct tm * dt;
	char buffer [30];
	dt = localtime(&rawtime);
	strftime(buffer, sizeof(buffer), formatString.data(), dt);
	return std::string(buffer);
}

std::string reformatCookie(
	const std::string &host,
	const std::string &value,
	const std::string &path,
	bool httpOnly,
	bool secure,
	size_t expireDate)
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

bool addOrReplaceHeader(
	struct curl_slist *headerStruct,
	const std::string &headerName,
	const std::string &headerValue)
{
	if (headerStruct != nullptr) {
		std::string headerData(headerStruct->data);
		if (headerData.length() > headerName.length() &&
			headerData.substr(0, headerName.length()) == headerName) {
			free(headerStruct->data);
			headerStruct->data = new char[
				headerName.length() + 1 + headerValue.length() + 1];
			std::string tempBuffer(headerName + ": " + headerValue);
			strncpy(headerStruct->data, tempBuffer.data(), tempBuffer.length());
			headerStruct->data[tempBuffer.length()] = '\0';
			return true;
		}
		if (headerStruct->next != nullptr) {
			return addOrReplaceHeader(headerStruct->next, headerName, headerValue);
		}
	}
	return false;
}

void addCustomHeaderToHeaderStruct(
	const WebRequest::HeaderContainerType &headers,
	struct curl_slist *&headerStruct)
{
	for (auto &header : headers) {
		if (!addOrReplaceHeader(headerStruct, header.first, header.second)) {
			headerStruct = curl_slist_append(
				headerStruct,
				std::string(header.first + ": " + header.second).data());
		}
	}
}

class WebResponseImpl : public WebResponse
{
 public:
	virtual ~WebResponseImpl()
	{
	}

	void setCURLFunctions()
	{
		curl_easy_setopt(
			m_curlHandle,
			CURLOPT_WRITEFUNCTION,
			WebResponse::curlWriteFunction);
		curl_easy_setopt(
			m_curlHandle,
			CURLOPT_HEADERFUNCTION,
			WebResponse::curlHeaderFunction);
	}

	void readCookiesFromResponse()
	{
		CURLcode res;
		struct curl_slist *cookies;

		res = curl_easy_getinfo(m_curlHandle, CURLINFO_COOKIELIST, &cookies);
		if(res != CURLE_OK) {
			fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n",
				curl_easy_strerror(res));
		}
		readAllCookies(cookies);
		curl_slist_free_all(cookies);
	}

	static std::string cookieToString(struct curl_slist *cookieStruct)
	{
		return std::string(cookieStruct->data, strlen(cookieStruct->data));
	}

	static std::vector<std::string> splitCookieString(std::string &&cookieString)
	{
		std::istringstream f(cookieString);
		std::string s;
		std::vector<std::string> cookieStrings;
		while (getline(f, s, '\t')) {
			cookieStrings.push_back(s);
		}
		return cookieStrings;
	}

	void readAllCookies(struct curl_slist *cookieStruct)
	{
		if (cookieStruct != nullptr) {
			for (struct curl_slist *currentStruct = cookieStruct;
				currentStruct->next != nullptr;
				currentStruct = currentStruct->next) {
				readSingleCookie(currentStruct);
			}
		}
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

			std::string &hostPart = cookieStrings[0];
			std::string httpOnlyStr("#HttpOnly_");
			if (hostPart.length() >= httpOnlyStr.length() &&
				hostPart.substr(0, httpOnlyStr.length()) == httpOnlyStr) {
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

			m_cookies.push_back(
				std::pair<std::string, std::string>(
					name, reformatCookie(
						host, value, path, httpOnly, secure, expireDate)));
		}
	}

	void setCURLHandle(CURL *handle)
	{
		m_curlHandle = handle;
	}
};

class FailedWebRequestResponse :
	public WebResponse
{
 public:
	explicit FailedWebRequestResponse(long httpStatusCode)
	{
		m_httpStatusCode = httpStatusCode;
	}
};

WebResponse WebRequest::post(
	const Url &url,
	const std::string &data)
{
	WebResponseImpl response;

	// init new curl handle
	CURL *curlHandle = curl_easy_init();
	response.setCURLHandle(curlHandle);
	if (!curlHandle) {
		return FailedWebRequestResponse(-1);
	}

	// set url for the request
	curl_easy_setopt(
		curlHandle,
		CURLOPT_URL,
		static_cast<std::string>(url).data());
	// set post as method to use for curl
	curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "POST");
	// start cookie engine
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, "");

	struct curl_slist *headerStruct = nullptr;

	if (data.length() == 0) {
		m_header.push_back(
			std::pair<std::string, std::string>("Content-Length", "0"));
	}

	addCustomHeaderToHeaderStruct(m_header, headerStruct);

	// set the header structure to the curl handle
	if (headerStruct != nullptr) {
		curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerStruct);
	}

	// set the post data
	if (data.length() > 0) {
		curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, data.data());
	}

	// set the cookies for the request
	if (m_cookies.size() > 0) {
		std::string cookieString;
		typedef WebRequest::CookieContainerType::const_iterator ItType;
		for (ItType it = m_cookies.begin(); it != m_cookies.end(); ++it) {
			cookieString += std::move(it->first + '=' + it->second);
		}
		curl_easy_setopt(curlHandle, CURLOPT_COOKIE, cookieString.data());
	}

#ifdef _DEBUG
	curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L);
#endif

	response.setCURLFunctions();
	WebResponse *this_ = static_cast<WebResponse *>(&response);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, this_);
	curl_easy_setopt(curlHandle, CURLOPT_HEADERDATA, this_);

	// perform the actual request
	CURLcode success = curl_easy_perform(curlHandle);

	// check if the request was successful
	if(success != CURLE_OK) {
		fprintf(
			stderr,
			"curl_easy_perform() failed: %s\n",
			curl_easy_strerror(success));
		return FailedWebRequestResponse(-2);
	}

	response.readCookiesFromResponse();

	if (headerStruct != nullptr) {
		curl_slist_free_all(headerStruct);
		headerStruct = nullptr;
	}

	curlHandle = nullptr;

	return std::move(response);
}

WebResponse WebRequest::get(
	const Url &url)
{
	WebResponseImpl response;

	// init new curl handle
	CURL *curlHandle = curl_easy_init();
	response.setCURLHandle(curlHandle);
	if (!curlHandle) {
		return FailedWebRequestResponse(-1);
	}

	// set url for the request
	curl_easy_setopt(
		curlHandle,
		CURLOPT_URL,
		static_cast<std::string>(url).data());
	// set post as method to use for curl
	curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "GET");
	// start cookie engine
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, "");

	struct curl_slist *headerStruct = nullptr;

	addCustomHeaderToHeaderStruct(m_header, headerStruct);

	// set the header structure to the curl handle
	if (headerStruct != nullptr) {
		curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerStruct);
	}

	// set the cookies for the request
	if (m_cookies.size() > 0) {
		std::string cookieString;
		typedef WebRequest::CookieContainerType::const_iterator ItType;
		for (ItType it = m_cookies.begin(); it != m_cookies.end(); ++it) {
			cookieString += std::move(it->first + '=' + it->second);
		}
		curl_easy_setopt(curlHandle, CURLOPT_COOKIE, cookieString.data());
	}

#ifdef _DEBUG
	curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L);
#endif

	response.setCURLFunctions();
	WebResponse *this_ = static_cast<WebResponse *>(&response);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, this_);
	curl_easy_setopt(curlHandle, CURLOPT_HEADERDATA, this_);

	// perform the actual request
	CURLcode responseCode = curl_easy_perform(curlHandle);

	// check if the request was successful
	if(responseCode != CURLE_OK) {
		fprintf(
			stderr,
			"curl_easy_perform() failed: %s\n",
			curl_easy_strerror(responseCode));
		return FailedWebRequestResponse(-2);
	}

	if (headerStruct != nullptr) {
		curl_slist_free_all(headerStruct);
		headerStruct = nullptr;
	}

	curlHandle = nullptr;

	return std::move(response);
}

void WebRequest::setContentType(const std::string & contentType)
{
	if (contentType.length() > 0) {
		std::string contentTypeCopy = contentType;
		std::string contentTypePrefix =
			contentTypeCopy.substr(0, std::string("Content-Type:").length());
		if (
			contentTypePrefix == std::string("Content-Type:") &&
			contentTypePrefix == std::string("content-Type:") &&
			contentTypePrefix == std::string("Content-type:") &&
			contentTypePrefix == std::string("content-type:")) {
			contentTypeCopy =
				contentTypeCopy.substr(std::string("Content-Type:").length()-1);
		}
		m_header.push_back(
			std::pair<std::string, std::string>(
				"Content-Type", contentTypeCopy));
	}
}

WebRequest::CookieContainerType WebRequest::cookies() const
{
	return m_cookies;
}

WebRequest::HeaderContainerType WebRequest::headers() const
{
	return m_header;
}

void WebRequest::addCookie(const std::string & name, const std::string & value)
{
	m_cookies.push_back(std::pair<std::string, std::string>(name, value));
}

void WebRequest::setCookies(const WebRequest::CookieContainerType & cookies)
{
	m_cookies = cookies;
}

void WebRequest::setHeaders(const WebRequest::HeaderContainerType & headers)
{
	m_header = headers;
}

void WebRequest::addHeader(const std::string & name, const std::string & value)
{
	m_header.push_back(std::pair<std::string, std::string>(name, value));
}

std::string WebRequest::getUnescapedString(const std::string & escapedString)
{
	std::string tempBuffer;
	char *curlUnescapedString = curl_unescape(
		escapedString.data(),
		static_cast<int>(escapedString.length()));
	tempBuffer.resize(strlen(curlUnescapedString));
	strncpy(&tempBuffer[0], curlUnescapedString, tempBuffer.length());
	curl_free(curlUnescapedString);
	return tempBuffer;
}
