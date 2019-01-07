#pragma once
#include <string>
class ConversionUtils
{
public:
	ConversionUtils();
	~ConversionUtils();

public:
	static size_t stringToSize_T(const std::string &string);
};

