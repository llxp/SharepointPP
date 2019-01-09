#pragma once
#include <string>
#include <vector>
#include <map>

namespace Microsoft {
namespace Sharepoint {
class WebUtils
{
public:
	WebUtils();
	~WebUtils();
	typedef std::vector<std::pair<std::string, std::string>> CookieContainerType;
	typedef std::vector<std::pair<std::string, std::string>> HeaderContainerType;
	static bool sendPostRequest(
		const std::string &url,
		const std::string &data,
		const std::string &contentType,
		const CookieContainerType &cookies = WebUtils::CookieContainerType(),
		const HeaderContainerType &headers = WebUtils::HeaderContainerType());
	__declspec(dllexport) static bool sendGetRequest(
		const std::string &url,
		const WebUtils::CookieContainerType &cookies = WebUtils::CookieContainerType(),
		const WebUtils::HeaderContainerType &headers = WebUtils::HeaderContainerType());
	__declspec(dllexport) static bool sendGetRequest(
		const std::string &url);
	static WebUtils::CookieContainerType getCookiesAfterRequest();
	__declspec(dllexport) static std::string getResponseData();
	static void addCookie(const std::string &name, const std::string &value);
	static std::string getUnescapedString(const std::string &escapedString);

private:
	static size_t curlWriteFunction(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t curlHeaderFunction(void *buffer, size_t size, size_t nmemb, void *userp);
	static void readCookiesFromResponse();

private:
	static std::vector<std::pair<std::string, std::string>> m_responseHeaders;
	static std::string m_responseBuffer;
	static CookieContainerType m_cookieStore;
	static void *m_curlHandle;
};

}  // namespace Sharepoint
}  // namespace Microsoft