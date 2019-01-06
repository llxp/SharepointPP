#pragma once
#include <string>
#include <chrono>

#include "SecurityDigest.h"

namespace Microsoft {
namespace Sharepoint {
class Authentication
{
public:
	__declspec(dllexport) Authentication();
	__declspec(dllexport) Authentication(std::string && username, std::string && password);
	__declspec(dllexport) ~Authentication();
	__declspec(dllexport) bool authenticate(std::string && username, std::string && password);
	__declspec(dllexport) bool tokenIsValid() const;
	__declspec(dllexport) SecurityDigest getSecurityDigest();

private:
	bool login(std::string && username, std::string && password);
	static std::string getPreparedSoapData(std::string && username, std::string && password, const std::string & endpoint);
	static std::string parseSTSResponse(std::string &&responseXml, const std::string &endpoint);
	void parseContextInfoResponse(std::string &&responseXml);

private:
	static std::string endpoint;
	static std::string stsEndpoint;
	static std::string defaultLoginPage;
	static std::string contextInfoUrl;

private:
	SecurityDigest m_securityDigest;
};

}  // namespace Sharepoint
}  // namespace Microsoft