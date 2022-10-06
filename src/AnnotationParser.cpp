#include "AnnotationParser.h"
#include "reflect.h"

[[nodiscard]] std::string trim(const std::string& str) noexcept;
[[nodiscard]] bool empty(const std::string& str) noexcept;

void AnnotationParser::Parse(const std::string& annotation) noexcept
{
	mValues.clear();
	size_t sep = annotation.find(':');
	mHeader = annotation.substr(0, sep);
	auto body = annotation.substr(sep + 1);

	if (empty(body))
		return;

	while (sep != std::string::npos)
	{
		size_t ass = body.find('=');
		sep = body.find(',');

		if (ass == std::string::npos) { GTR_ASSERT(false, "Value must be assigned to every variable.\n"); }

		const auto key = trim(body.substr(0, ass));
		const auto val = trim(body.substr(ass + 1, sep - ass - 1));

		if (empty(key) || empty(val)) { GTR_ASSERT(false, "Definition of variable must be complete.\n"); }

		if (key.empty()) { GTR_ASSERT(false, ""); }
		body = body.substr(sep + 1);
		mValues.insert({ key, val });
	}

}

[[nodiscard]] std::string AnnotationParser::Get(const std::string& key) const noexcept
{
	GTR_ASSERT(Has(key), "Couldn't find the specified key: %s\n", key.c_str());
	return mValues.at(key);
}

[[nodiscard]] std::string trim(const std::string& str) noexcept
{
	size_t start = 0;
	while (str[start] == ' ')
		start++;
	size_t end = str.size() - 1;
	while (str[end] == ' ')
		end--;

	if (str[start] == '"') start++;
	if (str[end] == '"') end--;

	return str.substr(start, end - start + 1);
}

[[nodiscard]] bool empty(const std::string& str) noexcept
{
	const auto trimmed = trim(str);
	return trimmed.empty();
}