#pragma once
#include <string>

namespace Microsoft {
namespace Sharepoint {
class SecurityDigest
{
public:
	__declspec(dllexport) SecurityDigest();
	__declspec(dllexport) SecurityDigest(const std::string &value, size_t timeout);
	__declspec(dllexport) ~SecurityDigest();
	__declspec(dllexport) bool isValid() const;
	__declspec(dllexport) std::string value() const;
	__declspec(dllexport) SecurityDigest(const SecurityDigest &other);
	__declspec(dllexport) SecurityDigest &operator=(SecurityDigest other);
	_declspec(dllexport) SecurityDigest(SecurityDigest&& other);

private:
	static void swap(SecurityDigest& first, SecurityDigest& second); // nothrow

private:
	const char * m_securityDigest;
	size_t m_securityDigestSetTime {0};
	size_t m_securityDigestTimeout {0};
};
}  // namespace Sharepoint
}  // namespace Microsoft

