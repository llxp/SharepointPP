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

#pragma once
#ifndef WEBRESPONSE_H_
#define WEBRESPONSE_H_

#include <string>
#include <vector>
#include <utility>

namespace Microsoft {
namespace Sharepoint {
class WebResponse
{
 public:
	__declspec(dllexport)
		WebResponse();
	__declspec(dllexport)
		~WebResponse();
	__declspec(dllexport)
		WebResponse(WebResponse &&other) noexcept;  // move constructor
	__declspec(dllexport)
		WebResponse(const WebResponse &other) = delete;  // copy constructor
	__declspec(dllexport)
		WebResponse& operator=(WebResponse other);

 public:
	typedef std::vector<std::pair<std::string, std::string>> CookieContainerType;
	typedef std::vector<std::pair<std::string, std::string>> HeaderContainerType;

 private:
	void swap(WebResponse& first, WebResponse& second) noexcept;  // nothrow

 public:
	__declspec(dllexport)
		WebResponse::CookieContainerType cookies() const;
	__declspec(dllexport)
		WebResponse::HeaderContainerType header() const;
	__declspec(dllexport)
		std::string response() const;
	__declspec(dllexport)
		long httpStatusCode() const;

 public:
	__declspec(dllexport)
		void setHttpStatusCode(long httpStatusCode);

 protected:
	static size_t curlWriteFunction(
		void *buffer,
		size_t size,
		size_t nmemb,
		void *userp);
	static size_t curlHeaderFunction(
		void *buffer,
		size_t size,
		size_t nmemb,
		void *userp);

 protected:
	std::string m_responseBuffer;
	HeaderContainerType m_headers;
	CookieContainerType m_cookies;
	long m_httpStatusCode;
	void *m_curlHandle;
};
}  // namespace Sharepoint
}  // namespace Microsoft

#endif  // WEBRESPONSE_H_