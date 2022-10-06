#pragma once

#include "reflect.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <filesystem>

class Finder : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
	std::unordered_map<std::string, Object> Objects;
	std::unordered_map<std::string, Enum> Enums;

	void run(const clang::ast_matchers::MatchFinder::MatchResult& result) noexcept override;

protected:

	virtual void FoundRecord(const clang::CXXRecordDecl* record) noexcept;
	virtual void FoundField(const clang::FieldDecl* field) noexcept;
	virtual void FoundEnum(const clang::EnumDecl* enumdecl) noexcept;

	[[nodiscard]] std::string get_annotation(const clang::Decl* decl) noexcept;
	[[nodiscard]] FieldType enumtype(const clang::EnumDecl* decl) noexcept;
	[[nodiscard]] FieldType gettype(const clang::QualType& type) noexcept;

};

class PrebuildFinder : public Finder {
public:
	PrebuildFinder(const char* filepath) noexcept;
	void FoundRecord(const clang::CXXRecordDecl* record) noexcept override;
	void onEndOfTranslationUnit(void) noexcept override;
private:
	std::filesystem::path mProjectDir;
	std::unordered_map<std::string, bool> mHeaders;
};

class PostbuildFinder : public Finder {
public:
	PostbuildFinder(const char* filepath) noexcept
		: mProjectDir(filepath) {}
	void onEndOfTranslationUnit(void) noexcept override;
	void FoundRecord(const clang::CXXRecordDecl* record) noexcept override;
	void FoundField(const clang::FieldDecl* fieldrec) noexcept override;
	void FoundEnum(const clang::EnumDecl* enumdecl) noexcept override;

private:

	void WriteEnums(void) const noexcept;
	void WriteObjects(void) noexcept;

	void output_object(std::ofstream& os, const Object& obj) noexcept;

private:
	std::filesystem::path mProjectDir;
};