#pragma once

#include <string>
#include <map>

class AnnotationParser {
public:
	AnnotationParser() = default;
	AnnotationParser(const std::string& annotation) { Parse(annotation); }

	void Parse(const std::string& annotation) noexcept;
	[[nodiscard]] bool Has(const std::string& key) const noexcept { return mValues.find(key) != mValues.end(); }
	[[nodiscard]] size_t Count(void) const noexcept { return mValues.size(); }
	[[nodiscard]] std::string GetHeader(void) noexcept { return mHeader; }
	[[nodiscard]] const std::string& GetHeader(void) const noexcept { return mHeader; }

	[[nodiscard]] std::string Get(const std::string& key) const noexcept;

	template<typename T>
	[[nodiscard]] T GetAs(const std::string& key) const noexcept;

	/***** TODO(Vasilis): DEBUG ONLY REMOVE! *****/
	//void Print(void) const noexcept
	//{
	//	std::cout << "Header: " << mHeader << '\n';
	//	for (const auto [key, val] : mValues)
	//		std::cout << key << ": " << val << '\n';
	//}

private:
	std::map<std::string, std::string> mValues;
	std::string mHeader;
};

#include "AnnotationParser.hpp"