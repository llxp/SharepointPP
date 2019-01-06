#include "TimeUtils.h"



TimeUtils::TimeUtils()
{
}


TimeUtils::~TimeUtils()
{
}

std::chrono::system_clock::rep TimeUtils::getCurrentTime()
{
	static_assert(
		std::is_integral<std::chrono::system_clock::rep>::value,
		"Representation of ticks isn't an integral value."
		);
	auto now = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}
