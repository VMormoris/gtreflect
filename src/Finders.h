#pragma once
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <filesystem>
#include <fstream>

#include "reflect.h"
#include "Builds.h"

//Forward Declerations
namespace YAML { class Node; }

using namespace clang::ast_matchers;

//Match finders for every desired reflection type

struct EnumFinder : public MatchFinder::MatchCallback {
	virtual void run(const MatchFinder::MatchResult& res) override;
};

struct ClassFinder : public MatchFinder::MatchCallback {
	ClassFinder(const std::filesystem::path& prjdir, const std::filesystem::path& prj, BuildFace face = BuildFace::Postbuild);
	~ClassFinder(void) { mHppOut.close(); mCppOut.close(); }
	virtual void run(const MatchFinder::MatchResult& result) override;
private:
	BuildFace mFace;
	std::filesystem::path mPrjDir;
	std::ofstream mCppOut;
	std::ofstream mHppOut;
};

struct FieldFinder : public MatchFinder::MatchCallback {
	virtual void run(const MatchFinder::MatchResult& result) override;
};

//Gets access to the "global" object that is being build during the Runtime
[[nodiscard]] YAML::Node& GetYAMLObject(void) noexcept;
