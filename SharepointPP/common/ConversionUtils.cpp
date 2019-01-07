#include "ConversionUtils.h"
#include <charconv>



ConversionUtils::ConversionUtils()
{
}


ConversionUtils::~ConversionUtils()
{
}

size_t ConversionUtils::stringToSize_T(const std::string & string)
{
	size_t n = 0;
	std::from_chars(string.data(), string.data() + string.size(), n);
	return n;
}
