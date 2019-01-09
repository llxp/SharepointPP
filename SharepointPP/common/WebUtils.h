#pragma once
#include <string>
#include <vector>
#include <map>

#include "WebResponse.h"

namespace Microsoft {
namespace Sharepoint {
class WebRequest
{
public:
	__declspec(dllexport) WebRequest();
	__declspec(dllexport) ~WebRequest();
	typedef std::vector<std::pair<std::string, std::string>> CookieContainerType;
	typedef std::vector<std::pair<std::string, std::string>> HeaderContainerType;

public:
	__declspec(dllexport)
	WebResponse post(
		const std::string &url,
		const std::string &data);

public:
	__declspec(dllexport)
	WebResponse get(
		const std::string &url);

public:
	__declspec(dllexport)
	void setContentType(const std::string &contentType);
	__declspec(dllexport)
	void addCookie(const std::string &name, const std::string &value);
	__declspec(dllexport)
	void setCookies(const WebRequest::CookieContainerType &cookies);
	__declspec(dllexport)
	void setHeaders(const WebRequest::HeaderContainerType &headers);
	__declspec(dllexport)
	void addHeader(const std::string &name, const std::string &value);

public:
	__declspec(dllexport)
	WebRequest::CookieContainerType cookies() const;
	__declspec(dllexport)
	WebRequest::HeaderContainerType headers() const;

public:
	__declspec(dllexport)
	static std::string getUnescapedString(const std::string &escapedString);

private:
	WebRequest::CookieContainerType m_cookies;
	WebRequest::HeaderContainerType m_header;
	void *m_curlHandle;
};

}  // namespace Sharepoint
}  // namespace Microsoft