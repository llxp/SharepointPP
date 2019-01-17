// MIT License
//
// Copyright (c) 2018 Lukas Luedke
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "WebResponse.h"
#include <curl/curl.h>

#include <utility>
#include <string>

using Microsoft::Sharepoint::WebResponse;

WebResponse::WebResponse() :
	m_responseBuffer(),
	m_cookies(),
	m_headers(),
	m_httpStatusCode(0),
	m_curlHandle(nullptr)
{
}


WebResponse::~WebResponse()
{
	if (m_curlHandle != nullptr) {
		curl_easy_cleanup(m_curlHandle);
	}
}

WebResponse::WebResponse(WebResponse && other) noexcept :
	WebResponse()
{
	swap(*this, other);
}

/*WebResponse::WebResponse(const WebResponse & other) noexcept
{
	*this = other;
}*/

WebResponse & WebResponse::operator=(WebResponse other)
{
	swap(*this, other);
	return *this;
}

void WebResponse::swap(WebResponse & first, WebResponse & second) noexcept
{
	using std::swap;
	swap(first.m_cookies, second.m_cookies);
	swap(first.m_responseBuffer, second.m_responseBuffer);
	swap(first.m_headers, second.m_headers);
	swap(first.m_curlHandle, second.m_curlHandle);
}

WebResponse::CookieContainerType WebResponse::cookies() const
{
	return m_cookies;
}

WebResponse::HeaderContainerType WebResponse::header() const
{
	return m_headers;
}

std::string WebResponse::response() const
{
	return m_responseBuffer;
}

long WebResponse::httpStatusCode() const
{
	if (m_httpStatusCode >= 0) {
		CURLcode responseCode = curl_easy_getinfo(
			m_curlHandle,
			CURLINFO_RESPONSE_CODE,
			&m_httpStatusCode);
		if (responseCode == CURLE_OK) {
			return m_httpStatusCode;
		}
	}
	return 0;
}

void WebResponse::setHttpStatusCode(long httpStatusCode)
{
	m_httpStatusCode = httpStatusCode;
}

size_t WebResponse::curlWriteFunction(
	void * buffer,
	size_t size,
	size_t nmemb,
	void * userp)
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

size_t WebResponse::curlHeaderFunction(
	void * buffer,
	size_t size,
	size_t nmemb,
	void * userp)
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
						this_->m_headers.push_back(
							std::pair<std::string, std::string>(name, value));
					}
				}
				return size * nmemb;
			}
		}
	}
	return 0U;
}
