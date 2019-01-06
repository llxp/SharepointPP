#pragma once
#include <chrono>

class TimeUtils
{
public:
	TimeUtils();
	~TimeUtils();
	static std::chrono::system_clock::rep getCurrentTime();
};

