#include "reflect.h"

#include <yaml-cpp/yaml.h>

//TODO(Vasilis): Consider if is really need to check the Default
static [[nodiscard]] bool CompareFields(const YAML::Node& lhs, const YAML::Node& rhs) noexcept;

[[nodiscard]] size_t distance(const YAML::Node& lhs, const YAML::Node& rhs) noexcept
{
	const auto& lhsName = lhs["Name"].as<std::string>(); const auto& rhsName = rhs["Name"].as<std::string>();
	const auto& lhsRealName = lhs["Real Name"].as<std::string>(); const auto& rhsRealName = rhs["Real Name"].as<std::string>();
	const auto& lhsHeaderFile = lhs["Header File"].as<std::string>(); const auto& rhsHeaderFile = rhs["Header File"].as<std::string>();
	size_t sum = 0;
	if (lhsName.compare(rhsName) != 0)
		sum += 1;
	if (lhsRealName.compare(rhsRealName) != 0)
		sum += 3;
	if (lhsHeaderFile.compare(rhsHeaderFile) != 0)
		sum += 5;
	return sum;
}

[[nodiscard]] bool CompareClasses(const YAML::Node& lhs, const YAML::Node& rhs) noexcept
{
	if (lhs["Real Name"].as<std::string>().compare(rhs["Real Name"].as<std::string>()) != 0)
		return false;
	if (lhs["Name"].as<std::string>().compare(rhs["Name"].as<std::string>()) != 0)
		return false;
	if (lhs["Header File"].as<std::string>().compare(rhs["Header File"].as<std::string>()) != 0)
		return false;
	if (!lhs["Fields"].IsDefined() && !rhs["Fields"].IsDefined())
		return true;
	else if (!lhs["Fields"].IsDefined())
		return false;
	else if (!rhs["Fields"].IsDefined())
		return false;
	if (lhs["Fields"].size() != rhs["Fields"].size())
		return false;
	for (const auto& lfield : lhs["Fields"])
	{
		bool match = false;
		for (const auto& rfield : rhs["Fields"])
		{
			if (lfield["Name"].as<std::string>().compare(rfield["Name"].as<std::string>()) != 0)
				continue;
			match = CompareFields(lfield, rfield);
			break;
		}
		if (!match)
			return false;
	}
	return true;
}

[[nodiscard]] bool CompareEnums(const YAML::Node& lhs, const YAML::Node& rhs) noexcept
{
	if (lhs.size() != rhs.size())
		return false;
	for (const YAML::Node& lenum : lhs)
	{
		bool match = false;
		for (const YAML::Node& renum : rhs)
		{
			if (lenum["Name"].as<std::string>().compare(renum["Name"].as<std::string>()) != 0)
				continue;
			if (lenum["Type"].as<uint16_t>() != renum["Type"].as<uint16_t>())
				return false;
			if (lenum["Enumarators"].size() != renum["Enumarators"].size())
				return false;
			for (const YAML::Node& lenumarator : lenum["Enumarators"])
			{
				bool valuematch = false;
				for (const YAML::Node& renumarator : renum["Enumarators"])
				{
					if (lenumarator["Name"].as<std::string>().compare(renumarator["Name"].as<std::string>()) != 0)
						continue;
					if (lenumarator["Value"].as<uint64_t>() != renumarator["Value"].as<uint64_t>())
						return false;
					valuematch = true;
					break;
				}
				if (!valuematch)
					return false;
			}
			match = true;
			break;
		}
		if (!match)
			return false;
	}

	return true;
}

static [[nodiscard]] bool CompareFields(const YAML::Node& lhs, const YAML::Node& rhs) noexcept
{
	if (lhs["Type"].as<uint16_t>() != rhs["Type"].as<uint16_t>())
		return false;
	if (lhs["Offset"].as<uint64_t>() != rhs["Offset"].as<uint16_t>())
		return false;

	const auto type = (FieldType)lhs["Type"].as<uint16_t>();
	switch (type)
	{
	case FieldType::Char:
	case FieldType::Int16:
	case FieldType::Int32:
	case FieldType::Int64:
		if (lhs["Min"].as<int64_t>() != rhs["Min"].as<int64_t>())
			return false;
		if (lhs["Max"].as<int64_t>() != rhs["Max"].as<int64_t>())
			return false;
		if (lhs["Default"].as<int64_t>() != rhs["Default"].as<int64_t>())
			return false;
		break;
	case FieldType::Byte:
	case FieldType::Uint16:
	case FieldType::Uint32:
	case FieldType::Uint64:
		if (lhs["Min"].as<uint64_t>() != rhs["Min"].as<uint64_t>())
			return false;
		if (lhs["Max"].as<uint64_t>() != rhs["Max"].as<uint64_t>())
			return false;
		if (lhs["Default"].as<uint64_t>() != rhs["Default"].as<uint64_t>())
			return false;
		break;
	case FieldType::String:
		if (lhs["Length"].as<uint64_t>() != rhs["Length"].as<uint64_t>())
			return false;
		if (lhs["Default"].as<std::string>().compare(rhs["Default"].as<std::string>()) != 0)
			return false;
		break;
	case FieldType::Enum_Char:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
		if (lhs["Typename"].as<std::string>().compare(rhs["Typename"].as<std::string>()) != 0)
			return false;
		if (lhs["Default"].as<uint64_t>() != rhs["Default"].as<uint64_t>())
			return false;
		break;
		//TODO(Vasilis): Equality comparison of floating point numbers (Ok?!?!?!?!)
	case FieldType::Float32:
	case FieldType::Float64:
		if (lhs["Min"].as<double>() != rhs["Min"].as<double>())
			return false;
		if (rhs["Max"].as<double>() != rhs["Max"].as<double>())
			return false;
		if (lhs["Default"].as<double>() != rhs["Default"].as<double>())
			return false;
		break;
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
		if (lhs["Min"].as<float>() != rhs["Min"].as<float>())
			return false;
		if (rhs["Max"].as<float>() != rhs["Max"].as<float>())
			return false;
		if (lhs["Default"].size() != rhs["Default"].size())
			return false;
		for (size_t i = 0; i < lhs["Default"].size(); i++)
		{
			if (lhs["Default"][i].as<float>() != rhs["Default"][i].as<float>())
				return false;
		}
		break;
	}
	return true;
}