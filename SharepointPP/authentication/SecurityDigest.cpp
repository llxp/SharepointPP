#include "SecurityDigest.h"

#include "../common/TimeUtils.h"

using Microsoft::Sharepoint::SecurityDigest;

SecurityDigest::SecurityDigest() :
	m_securityDigest(std::string()),
	m_securityDigestSetTime(0),
	m_securityDigestTimeout(0)
{
}

SecurityDigest::SecurityDigest(const std::string &value, size_t timeout) :
	m_securityDigest(value),
	m_securityDigestSetTime(TimeUtils::getCurrentTime()),
	m_securityDigestTimeout(timeout)
{
}


SecurityDigest::~SecurityDigest()
{
}

SecurityDigest::SecurityDigest(const SecurityDigest & other) :
	m_securityDigest(other.m_securityDigest),
	m_securityDigestSetTime(other.m_securityDigestSetTime),
	m_securityDigestTimeout(other.m_securityDigestTimeout)
{
}

SecurityDigest::SecurityDigest(SecurityDigest && other) noexcept :
	SecurityDigest()
{
	swap(*this, other);
}

bool SecurityDigest::isValid() const
{
	auto now = TimeUtils::getCurrentTime();
	if (now < static_cast<long long>(m_securityDigestSetTime + m_securityDigestTimeout)) {
		return true;
	}
	return false;
}

std::string SecurityDigest::value() const
{
	return m_securityDigest;
}

SecurityDigest & SecurityDigest::operator=(SecurityDigest other)
{
	swap(*this, other);
	return *this;
}

void SecurityDigest::swap(SecurityDigest& first, SecurityDigest& second) noexcept // nothrow
{
	using std::swap;
	swap(first.m_securityDigest, second.m_securityDigest);
	swap(first.m_securityDigestSetTime, second.m_securityDigestSetTime);
	swap(first.m_securityDigestTimeout, second.m_securityDigestTimeout);
}