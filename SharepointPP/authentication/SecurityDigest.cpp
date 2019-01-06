#include "SecurityDigest.h"

#include "../TimeUtils.h"

using Microsoft::Sharepoint::SecurityDigest;

SecurityDigest::SecurityDigest() :
	m_securityDigest(nullptr),
	m_securityDigestSetTime(0),
	m_securityDigestTimeout(0)
{
}

SecurityDigest::SecurityDigest(const std::string &value, size_t timeout) :
	m_securityDigest(strdup(value.data())),
	m_securityDigestSetTime(TimeUtils::getCurrentTime()),
	m_securityDigestTimeout(timeout)
{
}


SecurityDigest::~SecurityDigest()
{
	if (m_securityDigest != nullptr) {
		delete[] m_securityDigest;
	}
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

SecurityDigest::SecurityDigest(const SecurityDigest & other) :
	m_securityDigest(strdup(other.m_securityDigest)),
	m_securityDigestSetTime(other.m_securityDigestSetTime),
	m_securityDigestTimeout(other.m_securityDigestTimeout)
{
}

SecurityDigest & SecurityDigest::operator=(SecurityDigest other)
{
	swap(*this, other);
	return *this;
}

SecurityDigest::SecurityDigest(SecurityDigest && other) : SecurityDigest()
{
	swap(*this, other);
}

void SecurityDigest::swap(SecurityDigest& first, SecurityDigest& second) // nothrow
{
	using std::swap;
	swap(first.m_securityDigest, second.m_securityDigest);
	swap(first.m_securityDigestSetTime, second.m_securityDigestSetTime);
	swap(first.m_securityDigestTimeout, second.m_securityDigestTimeout);
}