#pragma once

#include <cstdio>
//This assertion terminates the application
#define GTR_ASSERT(x, format, ...) { if(!(x)){ fprintf(stderr, format, __VA_ARGS__); exit(EXIT_FAILURE); }}

//Forward Decleration
namespace YAML { class Node; }

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
};

/**
* @brief Calculates the distance between two Scripts objects
* @details The distance between two Scripts is defined by the
*	difference in the names and header files. It being use internally
*	by the reflection. And is aiming to minimize changes from the previous
*	build in order to make the engine work more fluently.
* 
* The distance is calculated in such a way to be able two find how close they are
*	two scripts between to build as well as know what has change
* @param lhs One class
* @param rhs Second class
* @return The distance between the Scripts:
*	- 1 If only the Name differs
*	- 3 If only the Real Name differs	
*	- 5 If only Header File differs
*	- Sum of individual differences (ex. 9 for everything)
*/
[[nodiscard]] size_t distance(const YAML::Node& lhs, const YAML::Node& rhs) noexcept;

/**
* @brief Checks wheter two list of exported enums differ with each other or not
* @param lhs The first list of exported enums
* @param rhs The second list of exported enums
* @return True if the two list of enums NOT differ, false otherwise
*/
[[nodiscard]] bool CompareEnums(const YAML::Node& lhs, const YAML::Node& rhs) noexcept;

/**
* @brief Checks wheter two list of exported classes differ with each other or not
* @param lhs The first list of exported classes
* @param rhs The second list of exported classes
* @return True if the two list of classes NOT differ, false otherwise
*/
[[nodiscard]] bool CompareClasses(const YAML::Node& lhs, const YAML::Node& rhs) noexcept;
