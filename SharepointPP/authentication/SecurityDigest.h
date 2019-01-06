#pragma once
#include <string>

class SecurityDigest
{
public:
	__declspec(dllexport) SecurityDigest();
	__declspec(dllexport) SecurityDigest(const std::string &value, size_t timeout);
	__declspec(dllexport) ~SecurityDigest();
	__declspec(dllexport) bool isValid() const;
	__declspec(dllexport) std::string value() const;

private:
	char * m_securityDigest;
	size_t m_securityDigestSetTime {0};
	size_t m_securityDigestTimeout {0};
};

