#pragma once

#include "reflect.h"

#include <clang/AST/Type.h>


//Forward declaration(s)
namespace clang {
	class Decl;
	class FieldDecl;
}
namespace YAML { class Node; }


/**
* @brief Checks whether the given argument value can be interpreted as a string
* @param argval The argument value that will be checked
* @return True if the argval can be interpreted as a string, false otherwise
*/
[[nodiscard]] bool is_string(const std::string& argval) noexcept;

/**
* @brief Checks wether the given argument value can be interpreted as a number
* @param argval The argument value that will be checked
* @return True if the argval can be interpreted as a number, false otherwise
*/
[[nodiscard]] bool is_number(const std::string& argval) noexcept;

/**
* @brief Validate that the given argument can be used on the given type
* @param argname String with name of the argument we want to check
* @param type Reflection Type of the property
* @return True if the argument is validated, false otherwise
*/
[[nodiscard]] bool validate_args(const std::string& argname, FieldType type) noexcept;

/**
* @brief Trims the given char from both the start & end of a string
* @param str String from which the char will be trimmed
* @param val The character we are searching to trim
* @param recursive Flag to specify if the trim will occured recursively or not
* @returns A new string containing the string after the trim
*/
[[nodiscard]] std::string trimchar(const std::string& str, char val, bool recursive = false) noexcept;

/**
* @brief Searches and parses the given arg if exists
* @param str String from which we are searching the given argument
* @param arg The argument that we are searching for
* @return The assign string value of the given argument
* @warnigs Only use for arguments that are string (name or header)
*/
[[nodiscard]] std::string parse_arg(const std::string& str, const std::string& arg) noexcept;

/**
* @brief Parse a single argument
* @param[out] obj The object for the class we are parsing
* @param[in] ass The assignement operator of the given argument as string
* @param[in] vartype The actual type of the field we annotated
*/
void parse_arg(YAML::Node& obj, const std::string& ass, FieldType vartype) noexcept;

/**
* @brief Parse property declaration and extracts arguments
* @param str String of the annotation
* @param vartype The actual type of the field we annotated
* @return A YAML object with the arguments. YAML may contain error field if something went wrong
*/
[[nodiscard]] YAML::Node parse_args(const std::string& str, FieldType vartype) noexcept;

/**
* @brief Finds an Enumaration's types
* @param decl Pointer to the EnumDecl from which will extract the type
* @return A FieldType describing the type of an the enumaration (aka FieldType::Enum_*)
*/
[[nodiscard]] FieldType enumtype(const clang::EnumDecl* decl) noexcept;

/**
* @brief Finds an Field's size
* @param decl Pointer to the FieldDecl from which will extract the size
* @return The size of the given field in bytes
*/
[[nodiscard]] uint64_t fieldsize(const clang::FieldDecl* decl) noexcept;

/**
* @brief Finds the size of the parameters
* @param type Type of the field that is being exported
* @return The size of the parameters in bytes
*/
[[nodiscard]] uint64_t paramsize(FieldType type) noexcept;

/**
* @brief Extract the annotation string
* @param decl The decleration from which the string will be extracted
* @return An std::string containing the annotation
*/
[[nodiscard]] std::string get_annotation(const clang::Decl* rec) noexcept;

/**
* @brief Switching from clang type to Engine's Types
* @param type Reference to the clang type that will be converted into the Enigne's one
* @return An Egnine's type representation
*/
[[nodiscard]] FieldType get_type(const clang::QualType& ptr) noexcept;

/**
* @brief Reads all .gtscript file(s) on project
* @param projDir The project's directory
* @return A "snapshot" of the current Scripting state
*/
[[nodiscard]] YAML::Node read_current_state(const std::string& projDir) noexcept;

/**
* @brief Sets the default values to a given field
* @param[out] doc The YAML::Node holding information about the given field
* @param[in] field Pointer to a Clang FieldDecl
* @param[in] type The supported type of the given field
*/
void setup_default(YAML::Node& doc, const clang::FieldDecl* field, FieldType type) noexcept;
