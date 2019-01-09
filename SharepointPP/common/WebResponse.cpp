#include "WebResponse.h"
#include <curl/curl.h>

using Microsoft::Sharepoint::WebResponse;

WebResponse::WebResponse() :
	m_responseBuffer(),
	m_cookieStore(),
	m_responseHeaders(),
	m_httpResponseCode(0),
	m_curlHandle(nullptr)
{
}


WebResponse::~WebResponse()
{
	if (m_curlHandle != nullptr) {
		curl_easy_cleanup(m_curlHandle);
	}
}

Microsoft::Sharepoint::WebResponse::WebResponse(WebResponse && other) noexcept :
	WebResponse()
{
	swap(*this, other);
}

WebResponse & Microsoft::Sharepoint::WebResponse::operator=(WebResponse other)
{
	swap(*this, other);
	return *this;
}

void Microsoft::Sharepoint::WebResponse::swap(WebResponse & first, WebResponse & second) noexcept
{
	using std::swap;
	swap(first.m_cookieStore, second.m_cookieStore);
	swap(first.m_responseBuffer, second.m_responseBuffer);
	swap(first.m_responseHeaders, second.m_responseHeaders);
	swap(first.m_curlHandle, second.m_curlHandle);
}

WebResponse::CookieContainerType WebResponse::cookies() const
{
	return m_cookieStore;
}

WebResponse::HeaderContainerType WebResponse::header() const
{
	return m_responseHeaders;
}

std::string WebResponse::response() const
{
	return m_responseBuffer;
}

long WebResponse::httpStatusCode() const
{
	if (m_httpResponseCode >= 0) {
		CURLcode responseCode = curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &m_httpResponseCode);
		if (responseCode == CURLE_OK) {
			return m_httpResponseCode;
		}
	}
	return 0;
}

void WebResponse::setHttpStatusCode(long httpStatusCode)
{
	m_httpResponseCode = httpStatusCode;
}

size_t WebResponse::curlWriteFunction(void * buffer, size_t size, size_t nmemb, void * userp)
{
	if (userp != nullptr) {
		WebResponse *this_ = static_cast<WebResponse *>(userp);
		if (this_ != nullptr) {
			const char *bufferStr = static_cast<const char *>(buffer);
			if (bufferStr != nullptr) {
				std::string tempStr(bufferStr, nmemb);
				this_->m_responseBuffer.append(std::move(tempStr));
				return nmemb * size;
			}
		}
	}
	return 0U;
}

size_t WebResponse::curlHeaderFunction(void * buffer, size_t size, size_t nmemb, void * userp)
{
	if (userp != nullptr) {
		WebResponse *this_ = static_cast<WebResponse *>(userp);
		if (this_ != nullptr) {
			const char *bufferStr = static_cast<const char *>(buffer);
			if (bufferStr != nullptr) {
				std::string s = std::string(bufferStr, size * nmemb);
				size_t pos = s.find(":");
				if (s.length() > 0 && pos != std::string::npos) {
					std::string name = s.substr(0, pos);
					std::string value = s.substr(pos + 1);
					if (value[0] == ' ') {
						value = value.substr(1);
					}
					if (name.length() > 0) {
						this_->m_responseHeaders.push_back(
							std::pair<std::string, std::string>(name, value));
					}
				}
				return size * nmemb;
			}
		}
	}
	return 0U;
}
