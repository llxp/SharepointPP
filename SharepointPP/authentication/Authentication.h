#pragma once
#include <string>
#include <chrono>

#include "SecurityDigest.h"
#include "../common/WebRequest.h"

namespace Microsoft {
namespace Sharepoint {
class Authentication
{
public:
	__declspec(dllexport)
		Authentication();
	__declspec(dllexport)
		Authentication(std::string && username, std::string && password, const std::string &endpoint);
	__declspec(dllexport)
		~Authentication();

public:
	// the endpoint url can be in the form of
	// https://host
	// https://host.sharepoint.com
	// https://host/ --> will be renamed to https://host
	// https://host.sharepoint.com/ --> will be renamed to https://host.sharepoint.com
	// host --> will be renamed to https://host
	// host.sharepoint.com --> will be renamed to https://host.sharepoint.com
	__declspec(dllexport)
		void setSharepointEndpoint(const std::string &sharepointEndpoint);
	// the sts endpoint url has to be in the form https://host/extSTS.srf
	__declspec(dllexport)
		void setSTSEndpoint(const std::string &stsEndpoint);
	__declspec(dllexport)
		void setContextInfoUrl(const std::string &&contextInfoUrl);

public:
	__declspec(dllexport)
		bool authenticate(std::string && username, std::string && password);
	__declspec(dllexport)
		bool tokenIsValid() const;
	__declspec(dllexport)
		SecurityDigest getRequestDigest() const;
	__declspec(dllexport)
		WebRequest::CookieContainerType getSecurityCookies() const;
	__declspec(dllexport)
		WebRequest getPreparedRequest() const;

private:
	bool login(std::string && username, std::string && password);
	static std::string getPreparedSoapData(std::string && username, std::string && password, const std::string & endpoint);
	static std::string parseSTSResponse(std::string &&responseXml, const std::string &endpoint);
	void parseContextInfoResponse(std::string &&responseXml);

private:
	std::string m_endpoint;
	std::string m_stsEndpoint {"https://login.microsoftonline.com/extSTS.srf"};
	std::string m_contextInfoUrl;
	std::string m_defaultLoginPage;

private:
	SecurityDigest m_requestDigest;
	WebRequest::CookieContainerType m_securityCookies;
};

}  // namespace Sharepoint
}  // namespace Microsoft