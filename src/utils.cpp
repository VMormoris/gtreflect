#include "utils.h"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <fstream>
#include <utility>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/Lex/Lexer.h>

static [[nodiscard]] std::vector<float> vectorInit(const clang::FieldDecl* field, FieldType type) noexcept;

[[nodiscard]] bool is_string(const std::string& argval) noexcept
{
	if ((argval[argval.size() - 1] == '"') && (argval[0] == '"'))//Must start & end with double quote
		return true;
	return false;
}

[[nodiscard]] bool is_number(const std::string& argval) noexcept
{
	bool isfloat = false;
	//First char can be digit, dot or minus symbol
	if (!std::isdigit(argval[0]) && argval[0] != '.' && argval[0] != '-')
		return false;
	if (argval[0] == '.')
		isfloat = true;

	for (size_t i=1; i<argval.size()-1; i++)
	{
		if (argval[i] == '.' && isfloat)//Double dot
			return false;
		else if (argval[i] == '.')
			isfloat = true;
		else if (!std::isdigit(argval[i]))
			return false;
	}

	const size_t last = argval.size() - 1;
	if (!std::isdigit(argval[last]) && !(argval[last] == '.' && !isfloat) && !(argval[last] == 'f' && isfloat))
		return false;
	return true;
}

[[nodiscard]] bool validate_args(const std::string& argname, FieldType type) noexcept
{
	if (argname.compare("name") == 0) return true;//Everything can have a name

	switch (type)
	{
	case FieldType::Char:
	case FieldType::Byte:
	case FieldType::Int16:
	case FieldType::Int32:
	case FieldType::Int64:
	case FieldType::Uint16:
	case FieldType::Uint32:
	case FieldType::Uint64:
	case FieldType::Float32:
	case FieldType::Float64:
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
		//Numbers can also have min or max values
		if (argname.compare("min") == 0 || argname.compare("max") == 0)
			return true;
		break;
	case FieldType::String:
		//For string max number of chars can be specified
		if (argname.compare("length") == 0)
			return true;
		break;
	case FieldType::Unknown:
	case FieldType::Bool:
	case FieldType::Enum_Char:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
		break;
	}

	return false;
}

[[nodiscard]] std::string trimchar(const std::string& str, char val, bool recursive) noexcept
{
	if (str.size() == 1)//Edge case
		return str[0] == val ? std::string() : str;

	size_t start = 0, end = str.size() - 1;
	do//Search char from start
	{
		if (start >= end) return std::string();
		if (str[start] != val)
			break;
		start++;
	} while (recursive);

	do//Search char from the end
	{
		if (end <= start) return std::string();
		if (str[end] != val)
			break;
		end--;
	} while (recursive);

	return str.substr(start, end - start + 1);
}

[[nodiscard]] std::string parse_arg(const std::string& str, const std::string& arg) noexcept
{
	const size_t index = str.find(": ");//First seperator we can assume that exist (Auto generated when annotating)
	if (index == std::string::npos) return {};//Should be never reached

	//Split annotation to it's two parts
	const auto header = str.substr(0, index);
	const auto payload = str.substr(index + 2);

	//Find the type of the declaration & validate that is acceptable
	const auto typestr = header.substr(header.find('-') + 1);

	//Search for the given argument
	const auto searching = arg + '=';
	if (payload.find(searching) == std::string::npos)
		return {};

	const size_t start = payload.find(searching);
	const size_t count = payload.find(',', start) - (start + searching.size() + 1) + 1;
	const std::string rhs = trimchar(payload.substr(start + searching.size(), count), ' ', true);

	GTR_ASSERT(rhs[0] == '"' && rhs[rhs.size() - 1] == '"', "Strings must be wrapped around double quotes.");
	return trimchar(rhs, '"');
}

void parse_arg(YAML::Node& obj, const std::string& ass, FieldType vartype) noexcept
{
	const size_t op = ass.find('=');
	GTR_ASSERT(op != std::string::npos, "We require: \"=\" when assigning value to an argument");//We require = between the two operants

	const std::string lhs = trimchar(ass.substr(0, op), ' ', true);
	const std::string rhs = trimchar(ass.substr(op + 1), ' ', true);
	GTR_ASSERT(!obj[lhs].IsDefined(), "The specified argument \"%s\", is already declared", lhs.c_str());
	GTR_ASSERT(validate_args(lhs, vartype), "Using argument \"%s\" is not allowed for the given type", lhs.c_str());

	if (lhs.compare("name") == 0)
	{
		GTR_ASSERT(is_string(rhs), "Expected string for \"%s\" parameter\n", lhs.c_str());
		obj[lhs] = trimchar(rhs, '"');
		return;
	}
	else if (lhs.compare("length") == 0)
	{
		GTR_ASSERT(is_number(rhs), "Expected number for \"%s\" parameter\n", lhs.c_str());
		obj["Length"] = std::stoull(rhs);
	}
	else if (lhs.compare("min") == 0 || lhs.compare("max") == 0)
	{
		GTR_ASSERT(is_number(rhs), "Expected number for \"%s\" parameter\n", lhs.c_str());
		auto key = lhs; key[0] = 'M';
		switch (vartype)
		{
		case FieldType::Float32:
		case FieldType::Float64:
		case FieldType::Vec2:
		case FieldType::Vec3:
		case FieldType::Vec4:
			obj[key] = std::stod(rhs);
			break;
		case FieldType::Char:
		case FieldType::Int16:
		case FieldType::Int32:
		case FieldType::Int64:
			obj[key] = std::stoll(rhs);
			break;
		case FieldType::Byte:
		case FieldType::Uint16:
		case FieldType::Uint32:
		case FieldType::Uint64:
			obj[key] = std::stoull(rhs);
			break;
		case FieldType::Bool:
		case FieldType::String:
		case FieldType::Enum_Char:
		case FieldType::Enum_Byte:
		case FieldType::Enum_Int16:
		case FieldType::Enum_Int32:
		case FieldType::Enum_Int64:
		case FieldType::Enum_Uint16:
		case FieldType::Enum_Uint32:
		case FieldType::Enum_Uint64:
			break;
		}
	}
}

[[nodiscard]] YAML::Node parse_args(const std::string& str, FieldType vartype) noexcept
{
	size_t index = str.find(": ");//First seperator we can assume that exist (Auto generated when annotating)
	GTR_ASSERT(index != std::string::npos, "Error on macro definition.");
	//Split annotation to it's two parts
	const std::string header = str.substr(0, index);
	std::string payload = str.substr(index + 2);

	//Find the type of the declaration & validate that is acceptable
	const std::string typestr = header.substr(header.find('-') + 1);
	GTR_ASSERT(typestr.compare("prop") == 0, "Supposed to be searching for a property");

	if (payload.empty())//No args at all
		return {};
	else if (payload.find(',') == std::string::npos)//Only one argument
	{
		YAML::Node obj = {};
		parse_arg(obj, payload, vartype);
		return obj;
	}

	YAML::Node obj = {};
	while ((index = payload.find(',')) != std::string::npos)//Read args
	{
		const std::string current = payload.substr(0, index);//Currently working assignement ("node")
		parse_arg(obj, current, vartype);
		if (obj["Error"])
			return obj;
		payload = payload.substr(index + 1);
	}

	GTR_ASSERT(!payload.empty(), "Extra comma detected.");

	parse_arg(obj, payload, vartype);
	return obj;
}

[[nodiscard]] FieldType enumtype(const clang::EnumDecl* decl) noexcept
{
	FieldType type = get_type(decl->getIntegerType().getCanonicalType());
	//Transform to enum type
	switch (type)
	{
	case FieldType::Enum_Char:
	case FieldType::Char:
		return FieldType::Enum_Char;
	case FieldType::Enum_Byte:
	case FieldType::Byte:
		return FieldType::Enum_Byte;
	case FieldType::Enum_Int16:
	case FieldType::Int16:
		return FieldType::Enum_Int16;
	case FieldType::Enum_Int32:
	case FieldType::Int32:
		return FieldType::Enum_Int32;
	case FieldType::Enum_Int64:
	case FieldType::Int64:
		return FieldType::Enum_Int64;
	case FieldType::Enum_Uint16:
	case FieldType::Uint16:
		return FieldType::Enum_Uint16;
	case FieldType::Enum_Uint32:
	case FieldType::Uint32:
		return FieldType::Enum_Uint32;
	case FieldType::Enum_Uint64:
	case FieldType::Uint64:
		return FieldType::Enum_Uint64;
	//Invalids
	case FieldType::Unknown:
	case FieldType::Bool:
	case FieldType::Float32:
	case FieldType::Float64:
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
	case FieldType::String:
		break;
	}
	return FieldType::Unknown;
}

[[nodiscard]] uint64_t fieldsize(const clang::FieldDecl* decl) noexcept
{

	auto info = decl->getASTContext().getTypeInfo(decl->getType().getTypePtr());
	return info.Width / 8;
}

[[nodiscard]] uint64_t paramsize(FieldType type) noexcept
{
	switch (type)
	{
	//All numbers & vectors two parameters
	case FieldType::Char:
		return 2 * sizeof(char);
	case FieldType::Byte:
		return 2 * sizeof(unsigned char);
	case FieldType::Int16:
		return 2 * sizeof(int16_t);
	case FieldType::Int32:
		return 2 * sizeof(int32_t);
	case FieldType::Int64:
		return 2 * sizeof(int64_t);
	case FieldType::Uint16:
		return 2 * sizeof(uint16_t);
	case FieldType::Uint32:
		return 2 * sizeof(uint32_t);
	case FieldType::Uint64:
		return 2 * sizeof(uint64_t);
	case FieldType::Float32:
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
		return 2 * sizeof(float);
	case FieldType::Float64:
		return 2 * sizeof(double);
	case FieldType::String:
		return sizeof(uint64_t);//One parameter Length of the string
	case FieldType::Bool:
	case FieldType::Enum_Char:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
	case FieldType::Unknown:
		return 0;
	}
	return 0;
}

[[nodiscard]] std::string get_annotation(const clang::Decl* rec) noexcept
{
	std::string annotation = "";
	llvm::raw_string_ostream os(annotation);
	auto policy = clang::PrintingPolicy(clang::LangOptions());

	rec->getAttr<clang::Attr>()->printPretty(os, policy);
	return annotation.substr(0, annotation.size() - 4).substr(annotation.find('"') + 1);
}

[[nodiscard]] FieldType get_type(const clang::QualType& type) noexcept
{
	const std::string strtype = type.getAsString();
	if (strtype.compare("bool") == 0) return FieldType::Bool;
	else if (strtype.compare("char") == 0) return FieldType::Char;
	else if (strtype.compare("unsigned char") == 0) return FieldType::Byte;
	else if (strtype.compare("short") == 0) return FieldType::Int16;
	else if (strtype.compare("int") == 0) return FieldType::Int32;
	else if (strtype.compare("long long") == 0) return FieldType::Int64;
	else if (strtype.compare("unsigned short") == 0) return FieldType::Uint16;
	else if (strtype.compare("unsigned int") == 0) return FieldType::Uint32;
	else if (strtype.compare("unsigned long long") == 0) return FieldType::Uint64;
	else if (strtype.compare("float") == 0) return FieldType::Float32;
	else if (strtype.compare("double") == 0) return FieldType::Float64;
	else if (strtype.compare("class dumm::String") == 0) return FieldType::String;
	else if (strtype.substr(0, 31).compare("struct glm::vec<2, float, glm::") == 0) return FieldType::Vec2;
	else if (strtype.substr(0, 31).compare("struct glm::vec<3, float, glm::") == 0) return FieldType::Vec3;
	else if (strtype.substr(0, 31).compare("struct glm::vec<4, float, glm::") == 0) return FieldType::Vec4;
	else return FieldType::Unknown;
}

[[nodiscard]] YAML::Node read_current_state(const std::string& projDir) noexcept
{
	const YAML::Node config = YAML::LoadFile(projDir + "/.gt/reflection.cache");
	const auto folder = projDir + "/Assets/";
	YAML::Node obj = {};
	for (const auto& path : config)
	{
		const auto file = path.as<std::string>();
		YAML::Node scriptobj = YAML::LoadFile(folder + file);
		scriptobj["filepath"] = file;
		obj["classes"].push_back(scriptobj);
	}
	return obj;
}

void setup_default(YAML::Node& doc, const clang::FieldDecl* field, FieldType type) noexcept
{
	if (field->hasInClassInitializer())
	{
		const clang::Expr* expr = field->getInClassInitializer();
		llvm::APInt intval;
		if(expr->isIntegerConstantExpr(field->getASTContext()))
			llvm::APInt intval = expr->EvaluateKnownConstInt(field->getASTContext());
		//else if(expr->EvaluateAsInitializer())
		switch (type)
		{
		case FieldType::Bool:
			doc["Default"] = (bool)intval.getSExtValue();
			break;
		case FieldType::Char:
			doc["Default"] = (int16_t)intval.getSExtValue();
			break;
		case FieldType::Byte:
			doc["Default"] = (uint16_t)intval.getZExtValue();
			break;
		case FieldType::Int16:
			doc["Default"] = (int16_t)intval.getSExtValue();
			break;
		case FieldType::Int32:
			doc["Default"] = (int32_t)intval.getSExtValue();
			break;
		case FieldType::Int64:
			doc["Default"] = intval.getSExtValue();
			break;
		case FieldType::Uint16:
			doc["Default"] = (uint16_t)intval.getZExtValue();
			break;
		case FieldType::Uint32:
			doc["Default"] = (uint32_t)intval.getZExtValue();
			break;
		case FieldType::Uint64:
			doc["Default"] = intval.getZExtValue();
			break;
		case FieldType::Float32:
		{
			llvm::APFloat val(0.0f);
			if(expr->EvaluateAsFloat(val, field->getASTContext()))
				doc["Default"] = val.convertToFloat();
			break;
		}
		case FieldType::Float64:
		{
			llvm::APFloat val(0.0);
			if (expr->EvaluateAsFloat(val, field->getASTContext()))
				doc["Default"] = val.convertToDouble();
			break;
		}
		case FieldType::String:
		{
			std::string buffer;
			llvm::raw_string_ostream os(buffer);
			expr->dump(os, field->getASTContext());
			const size_t lineoffset = buffer.find("-StringLiteral");
			if (lineoffset != std::string::npos)
			{
				size_t start = buffer.find('"', lineoffset);
				size_t end = buffer.find_last_of('"');
				doc["Default"] = buffer.substr(start+1, (end - start - 1));
			}
			break;
		}
		case FieldType::Enum_Char:
			doc["Default"] = (char)intval.getSExtValue();
			break;
		case FieldType::Enum_Byte:
			doc["Default"] = (uint16_t)intval.getZExtValue();
			break;
		case FieldType::Enum_Int16:
			doc["Default"] = (int16_t)intval.getSExtValue();
			break;
		case FieldType::Enum_Int32:
			doc["Default"] = (int32_t)intval.getSExtValue();
			break;
		case FieldType::Enum_Int64:
			doc["Default"] = (int64_t)intval.getSExtValue();
			break;
		case FieldType::Enum_Uint16:
			doc["Default"] = (uint16_t)intval.getZExtValue();
			break;
		case FieldType::Enum_Uint32:
			doc["Default"] = (uint32_t)intval.getZExtValue();
			break;
		case FieldType::Enum_Uint64:
			doc["Default"] = (uint64_t)intval.getZExtValue();
			break;
		case FieldType::Vec2:
			doc["Default"] = vectorInit(field, type);
			break;
		case FieldType::Vec3:
			doc["Default"] = vectorInit(field, type);
			break;
		case FieldType::Vec4:
			doc["Default"] = vectorInit(field, type);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (type)
		{
		case FieldType::Bool:
			doc["Default"] = false;
			break;
		case FieldType::Char:
			doc["Default"] = (char)0x00;
			break;
		case FieldType::Byte:
			doc["Default"] = (uint16_t)0x00;
			break;
		case FieldType::Int16:
			doc["Default"] = (int16_t)0;
			break;
		case FieldType::Int32:
			doc["Default"] = (int32_t)0;
			break;
		case FieldType::Int64:
			doc["Default"] = (int64_t)0;
			break;
		case FieldType::Uint16:
			doc["Default"] = (uint16_t)0;
			break;
		case FieldType::Uint32:
			doc["Default"] = (uint32_t)0;
			break;
		case FieldType::Uint64:
			doc["Default"] = (uint64_t)0;
			break;
		case FieldType::Float32:
			doc["Default"] = 0.0f;
			break;
		case FieldType::Float64:
			doc["Default"] = 0.0;
			break;
		case FieldType::String:
			doc["Default"] = "";
			break;
		case FieldType::Enum_Char:
			doc["Default"] = (char)0x00;
			break;
		case FieldType::Enum_Byte:
			doc["Default"] = (uint16_t)0x00;
			break;
		case FieldType::Enum_Int16:
			doc["Default"] = (int16_t)0;
			break;
		case FieldType::Enum_Int32:
			doc["Default"] = (int32_t)0;
			break;
		case FieldType::Enum_Int64:
			doc["Default"] = (int64_t)0;
			break;
		case FieldType::Enum_Uint16:
			doc["Default"] = (uint16_t)0;
			break;
		case FieldType::Enum_Uint32:
			doc["Default"] = (uint32_t)0;
			break;
		case FieldType::Enum_Uint64:
			doc["Default"] = (uint64_t)0;
		case FieldType::Vec2:
			doc["Default"] = std::array<float, 2>{ 0.0f, 0.0f };
			break;
		case FieldType::Vec3:
			doc["Default"] = std::array<float, 3>{0.0f, 0.0f, 0.0f};
			break;
		case FieldType::Vec4:
			doc["Default"] = std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f};
			break;
		default:
			break;
		}
	}

	switch (type) {
	case FieldType::Char:
	case FieldType::Byte:
	case FieldType::Int16:
	case FieldType::Int32:
	case FieldType::Int64:
	case FieldType::Uint16:
	case FieldType::Uint32:
	case FieldType::Uint64:
	case FieldType::Float32:
	case FieldType::Float64:
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
		doc["Min"] = 0;
		doc["Max"] = 0;
		break;
	case FieldType::String:
		doc["Length"] = 22;
		break;
	case FieldType::Enum_Char:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
		doc["Typename"] = "Not defined";
		break;
	case FieldType::Bool:
	case FieldType::Unknown:
		break;
	}

}

static [[nodiscard]] std::vector<float> vectorInit(const clang::FieldDecl* field, FieldType type) noexcept
{
	std::vector<float> vec;
	const clang::Expr* expr = field->getInClassInitializer();

	float Default = 0.0f;
	if (type == FieldType::Vec4)
		Default = 1.0f;

	std::string buffer;
	llvm::raw_string_ostream os(buffer);
	expr->dump(os, field->getASTContext());

	size_t offset = 0;
	size_t index = 0;
	while ((index = buffer.find("-FloatingLiteral", index)) != std::string::npos)
	{
		size_t start = buffer.find("'float' ", index) + 8;
		size_t end = buffer.find('\n', index);
		const auto check = buffer.substr(start, end - start);
		vec.push_back(std::stof(check));
		offset++;
		index = end;
	}

	//Only if value during contructor changes the default
	//And that is because if someone initializes a vec4 as vec3 (aka with only RGB values) we need alpha channel to be 1.0f
	//Same with vec3 if is initializes as a vec2 (aka with only x, y axis) we need z axis need to be 0.0f 
	if (vec.size() == 1)
		Default = vec[0];

	size_t vecsize = 2;
	if (type == FieldType::Vec3)
		vecsize = 3;
	else if (type == FieldType::Vec4)
		vecsize = 4;

	for (size_t i = offset; i < vecsize; i++)
		vec.push_back(Default);

	return vec;
}