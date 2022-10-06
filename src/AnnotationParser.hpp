#pragma once
#include "AnnotationParser.h"

template<typename T>
T AnnotationParser::GetAs(const std::string& key) const noexcept
{
	std::string value =	Get(key);
	if constexpr (std::is_same<T, int64_t>::value)
		return stoll(value);
	else if constexpr (std::is_same<T, uint64_t>::value)
		return stoull(value);
	else if constexpr (std::is_same<T, double>::value)
		return stod(value);
	else if constexpr (std::is_same<T, float>::value)
		return stof(value);
	else if constexpr (std::is_same<T, std::string>::value)
		return value;
}