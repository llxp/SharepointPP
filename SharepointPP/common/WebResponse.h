#pragma once
#include <string>
#include <vector>

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
	WebResponse(const WebResponse &other) = delete;  // copy constructor deleted
	__declspec(dllexport)
	WebResponse(WebResponse &&other) noexcept;  // move constructor
	__declspec(dllexport)
	WebResponse& operator=(WebResponse other);
	typedef std::vector<std::pair<std::string, std::string>> CookieContainerType;
	typedef std::vector<std::pair<std::string, std::string>> HeaderContainerType;

private:
	void swap(WebResponse& first, WebResponse& second) noexcept; // nothrow

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
	static size_t curlWriteFunction(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t curlHeaderFunction(void *buffer, size_t size, size_t nmemb, void *userp);

protected:
	std::string m_responseBuffer;
	HeaderContainerType m_responseHeaders;
	CookieContainerType m_cookieStore;
	long m_httpResponseCode;
	void *m_curlHandle;
};
}  // namespace Sharepoint
}  // namespace Microsoft