#pragma once

#include <cstdio>
#include <string>
#include <yaml-cpp/yaml.h>
#include <vector>

//This assertion terminates the application
#define GTR_ASSERT(x, format, ...) { if(!(x)){ fprintf(stderr, format, __VA_ARGS__); exit(EXIT_FAILURE); }}

/**
* @brief Enumaration for every kind of type that a
*	reflective field can possibly have.
*/
enum class FieldType : unsigned char {
	Unknown = 0,

	Bool,
	Char,
	Byte,//unsigned char

	Int16,
	Int32,
	Int64,
	Uint16,
	Uint32,
	Uint64,

	Float32,
	Float64,


	//Vectors
	Vec2,
	Vec3,
	Vec4,

	//Enumerations
	Enum_Char,
	Enum_Byte,
	Enum_Int16,
	Enum_Int32,
	Enum_Int64,
	Enum_Uint16,
	Enum_Uint32,
	Enum_Uint64,

	String,
    
    //Engine's Objects
    Asset,
	Entity
};

enum class ReflectionType {
	Unknown = 0,
	Enumaration,
	Component,
	System,
	Object,
	Property,
	Method,
};

struct Metadata {
	std::string Name;
	size_t Size = 0;
	ReflectionType Type = ReflectionType::Unknown;
	Metadata(void) = default;
	Metadata(const std::string& name, size_t size) noexcept
		: Name(name), Size(size) {}
	Metadata(const std::string& name, size_t size, ReflectionType type) noexcept
		: Name(name), Size(size), Type(type) {}
};

struct FieldMetadata : public Metadata {
	FieldType ValueType = FieldType::Unknown;
	union {
		size_t Length = 0;
		int64_t MinInt;
		uint64_t MinUint;
		double MinFloat;
	};
	union {
		int64_t MaxInt = 0;
		uint64_t MaxUint;
		double MaxFloat;
	};
	FieldMetadata(void) = default;
	FieldMetadata(const std::string& name, size_t size) noexcept
		: Metadata(name, size, ReflectionType::Property) {}
	FieldMetadata(const std::string& name, size_t size, FieldType type) noexcept
		: Metadata(name, size, ReflectionType::Property), ValueType(type) {}
};

struct Field {
	FieldMetadata Meta;
	std::string TypeName;
	/*
	* @brief Real Name
	* @details Name used in C++ on the contrary Meta.Name is used on editor
	*/
	std::string Name;
	size_t Offset = 0;
	YAML::Node Default;

	[[nodiscard]] bool operator==(const Field& other) const noexcept
	{
		if (Offset != other.Offset) return false;
		if (Meta.Size != other.Meta.Size) return false;
		if (Meta.ValueType != other.Meta.ValueType) return false;
		if (Meta.Name.compare(Meta.Name) != 0) return false;
		switch (Meta.ValueType)
		{
		case FieldType::Char:
		case FieldType::Enum_Char:
		case FieldType::Int16:
		case FieldType::Enum_Int16:
		case FieldType::Int32:
		case FieldType::Enum_Int32:
		case FieldType::Int64:
		case FieldType::Enum_Int64:
			if (Meta.MinInt != other.Meta.MinInt) return false;
			if (Meta.MaxInt != other.Meta.MaxInt) return false;
			break;
		case FieldType::Byte:
		case FieldType::Enum_Byte:
		case FieldType::Uint16:
		case FieldType::Enum_Uint16:
		case FieldType::Uint32:
		case FieldType::Enum_Uint32:
		case FieldType::Uint64:
		case FieldType::Enum_Uint64:
			if (Meta.MinUint != other.Meta.MinUint) return false;
			if (Meta.MaxUint != other.Meta.MaxUint) return false;
			break;
		case FieldType::Float32:
		case FieldType::Float64:
		case FieldType::Vec2:
		case FieldType::Vec3:
		case FieldType::Vec4:
			if (Meta.MinFloat != other.Meta.MinFloat) return false;
			if (Meta.MaxFloat != other.Meta.MaxFloat) return false;
			break;
		case FieldType::String:
			if (Meta.Length != other.Meta.Length) return false;
			break;
		}
		return true;
	}

	[[nodiscard]] bool isEnum(void) noexcept
	{
		return Meta.ValueType == FieldType::Enum_Char || Meta.ValueType == FieldType::Enum_Byte ||
			Meta.ValueType == FieldType::Enum_Int16 || Meta.ValueType == FieldType::Enum_Int32 || Meta.ValueType == FieldType::Enum_Int64 ||
			Meta.ValueType == FieldType::Enum_Uint16 || Meta.ValueType == FieldType::Enum_Uint32 || Meta.ValueType == FieldType::Enum_Uint64;
	}

	[[nodiscard]] inline bool isUnsigned(void) const noexcept
	{ 
		return Meta.ValueType == FieldType::Enum_Byte || Meta.ValueType == FieldType::Enum_Uint16 || Meta.ValueType == FieldType::Enum_Uint32 || Meta.ValueType == FieldType::Enum_Uint64 ||
			Meta.ValueType == FieldType::Byte || Meta.ValueType == FieldType::Uint16 || Meta.ValueType == FieldType::Uint32 || Meta.ValueType == FieldType::Uint64;
	}


	[[nodiscard]] inline bool isEnum(void) const noexcept
	{
		return Meta.ValueType == FieldType::Enum_Char || Meta.ValueType == FieldType::Enum_Byte ||
			Meta.ValueType == FieldType::Enum_Int16 || Meta.ValueType == FieldType::Enum_Int32 || Meta.ValueType == FieldType::Enum_Int64 ||
			Meta.ValueType == FieldType::Enum_Uint16 || Meta.ValueType == FieldType::Enum_Uint32 || Meta.ValueType == FieldType::Enum_Uint64;
	}

	[[nodiscard]] bool operator!=(const Field& other) const noexcept { return !(*this == other); }

	Field(void) = default;
	Field(const std::string& name, size_t size, size_t offset) noexcept
		: Meta(name, size), Name(name), Offset(offset) {}
	Field(const std::string& name, size_t size, size_t offset, FieldType type) noexcept
		: Meta(name, size, type), Name(name), Offset(offset) {}
};

struct EnumValue {
	union {
		int64_t Value = 0;
		uint64_t Uvalue;
	};
	EnumValue(void) = default;
	EnumValue(int64_t value) noexcept
		: Value(value) {}
	EnumValue(uint64_t value) noexcept
		: Uvalue(value) {}
};

struct Enum {
	Metadata Meta;
	/*
	* @brief Real Name
	* @details Name used in C++ on the contrary Meta.Name is used on editor
	*/
	std::string Name;
	FieldType Type;
	std::map<std::string, EnumValue> Values;
	[[nodiscard]] inline bool isUnsigned(void) const { return Type == FieldType::Enum_Byte || Type == FieldType::Enum_Uint16 || Type == FieldType::Enum_Uint32 || Type == FieldType::Enum_Uint64; }
	
	/*
	* @brief Checks whether two enums should be consider the same by GreenTea Engine
	*/
	[[nodiscard]] bool operator==(const Enum& other) const noexcept
	{
		if (isUnsigned() != other.isUnsigned()) return false;
		if (Meta.Name.compare(other.Meta.Name) != 0) return false;
		if (other.Meta.Size != Meta.Size) return false;
		for (const auto& [key, val] : Values)
		{
			if (other.Values.find(key) == other.Values.end()) return false;
			if (isUnsigned()) { if (other.Values.at(key).Uvalue != val.Uvalue) return false; }
			else { if (other.Values.at(key).Value != val.Value) return false; }
		}
		return true;
	}
	
	[[nodiscard]] bool operator!=(const Enum& other) const noexcept { return !(*this == other); }

	Enum(void) = default;
	Enum(const std::string& name, size_t size) noexcept
		: Meta(name, size, ReflectionType::Enumaration), Name(name) {}
	Enum(const std::string& name, size_t size, FieldType type) noexcept 
		: Meta(name, size, ReflectionType::Enumaration), Name(name), Type(type) {}
};

struct Object {
	Metadata Meta;
	/*
	* @brief Real Name
	* @details Name used in C++ on the contrary Meta.Name is used on editor
	*/
	std::string Name;
	std::string Header;
	std::vector<Field> Fields;
	uint64_t Version = 1;

	[[nodiscard]] bool operator==(const Object& other) const noexcept
	{
		if (Meta.Name.compare(other.Meta.Name) != 0) return false;
		if (Fields.size() != other.Fields.size()) return false;
		for (size_t i = 0; i < Fields.size(); i++)
		{
			if (Fields[i] != other.Fields[i]) return false;
		}
		return true;
	}

	[[nodiscard]] bool operator!=(const Object& other) const noexcept { return !(*this == other); }
	Object(void) = default;
	Object(const std::string& name, size_t size) noexcept
		: Meta(name, size), Name(name) {}
	Object(const std::string& name, size_t size, ReflectionType type) noexcept
		: Meta(name, size, type), Name(name) {}
};
