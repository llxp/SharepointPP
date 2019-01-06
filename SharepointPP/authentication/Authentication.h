#pragma once
#include <string>
#include <chrono>

#include "SecurityDigest.h"

namespace Microsoft {
namespace Sharepoint {
extern "C" class Authentication
{
public:
	__declspec(dllexport) Authentication();
	__declspec(dllexport) Authentication(std::string && username, std::string && password);
	__declspec(dllexport) ~Authentication();

public:
	__declspec(dllexport) void setSharepointEndpoint(const std::string &sharepointEndpoint);
	__declspec(dllexport) void setSTSEndpoint(const std::string &stsEndpoint);
	__declspec(dllexport) void setContextInfoUrl(const std::string &&contextInfoUrl);

public:
	__declspec(dllexport) bool authenticate(std::string && username, std::string && password);
	__declspec(dllexport) bool tokenIsValid() const;
	__declspec(dllexport) SecurityDigest getSecurityDigest();

private:
	bool login(std::string && username, std::string && password);
	static std::string getPreparedSoapData(std::string && username, std::string && password, const std::string & endpoint);
	static std::string parseSTSResponse(std::string &&responseXml, const std::string &endpoint);
	void parseContextInfoResponse(std::string &&responseXml);

private:
	std::string m_endpoint;
	std::string m_stsEndpoint {"https://login.microsoftonline.com/extSTS.srf"};
	std::string m_contextInfoUrl;

private:
	SecurityDigest m_securityDigest;
};

}  // namespace Sharepoint
}  // namespace Microsoft