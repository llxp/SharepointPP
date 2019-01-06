#include "SecurityDigest.h"

#include "../TimeUtils.h"


SecurityDigest::SecurityDigest()
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
	free(m_securityDigest);
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