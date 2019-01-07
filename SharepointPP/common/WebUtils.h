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
	typedef std::vector<std::pair<std::string, std::string>> cookieType;
	static bool sendPostRequest(
		const std::string &url,
		const std::string &data,
		const std::string &contentType,
		const cookieType &cookies = cookieType());
	static cookieType getCookiesAfterRequest();
	static std::string getResponseData();
	static void addCookie(const std::string &name, const std::string &value);
	static std::string getUnescapedString(const std::string &escapedString);

private:
	static size_t curlWriteFunction(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t curlHeaderFunction(void *buffer, size_t size, size_t nmemb, void *userp);

private:
	static std::vector<std::pair<std::string, std::string>> m_responseHeaders;
	static std::string m_responseBuffer;
	static cookieType m_cookieStore;
	static void *m_curlHandle;
};

}  // namespace Sharepoint
}  // namespace Microsoft