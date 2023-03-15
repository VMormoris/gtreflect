#include "Finders.h"

#pragma warning(push)
#pragma warning(disable: 4267 4244)
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/Type.h>
#pragma warning(pop)

#define NOMINMAX
#include <Windows.h>

int argc = 2;
static constexpr char* argv[2] = { "gtreflect.exe", ".gt/clangdump.hpp" };

int PrebuildRun(const char* filepath);
int PostbuildRun(const char* filepath);
void SendOverPipe(const char* pipename, const char* msg);

std::pair<bool, std::string> parseargs(const char** argv);

struct DumpASTAction : public clang::ASTFrontendAction {
	std::unique_ptr<clang::ASTConsumer>
		CreateASTConsumer(clang::CompilerInstance& ci, clang::StringRef inFile) override
	{
		return clang::CreateASTDumper(
			nullptr,//Dump to stdout
			"",//No filter
			true,//Dump decls
			true,//Dump deserialize
			false,//Don't dump lookups
			true,//Dump decl types
			clang::ASTDumpOutputFormat::ADOF_Default//format
		);
	}
};

static llvm::cl::OptionCategory gToolCategory("GT reflection options");
int main(int argc, const char** argv)
{
	GTR_ASSERT(argc == 3, "Waiting for 2 command line arguments but I got: %d.\n", argc - 1);
	const auto [isPrebuild, dir] = parseargs(argv);
	
	GTR_ASSERT
	(
		std::filesystem::exists(dir) && std::filesystem::is_directory(dir),
		"Couldn't find directory: %s\n", dir.c_str()
	);

	std::filesystem::current_path(dir);
	if (isPrebuild)
		return PrebuildRun(dir.c_str());
	else
		return PostbuildRun(dir.c_str());
}

void CreateClangFile(void);

int PrebuildRun(const char* filepath)
{
	printf("------ Prebuild Step ------\n");
	SendOverPipe("\\\\.\\pipe\\GreenTeaServer", "BuildStarted");
	CreateClangFile();

	using namespace clang::ast_matchers;
	using namespace clang::tooling;
	using namespace llvm;

	Expected<CommonOptionsParser> optionsParser = CommonOptionsParser::create(argc, (const char**)argv, gToolCategory);
	ClangTool tool(optionsParser->getCompilations(), optionsParser->getSourcePathList());

	PrebuildFinder prebuildFinder(filepath);
	MatchFinder finder;

	DeclarationMatcher objectMatcher = cxxRecordDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
	DeclarationMatcher fieldMatcher = fieldDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
	DeclarationMatcher enumMatcher = enumDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));

	finder.addMatcher(enumMatcher, &prebuildFinder);
	finder.addMatcher(objectMatcher, &prebuildFinder);
	finder.addMatcher(fieldMatcher, &prebuildFinder);
	
	int result = tool.run(newFrontendActionFactory(&finder).get());
	GTR_ASSERT(result == 0, "Reflection's prebuild step failed!\n");
	printf("---------------------------\n");
	return result;
}

int PostbuildRun(const char* filepath)
{
	using namespace clang::ast_matchers;
	using namespace clang::tooling;
	using namespace llvm;

	printf("------ Postbuild Step ------\n");
	Expected<CommonOptionsParser> optionsParser = CommonOptionsParser::create(argc, (const char**)argv, gToolCategory);
	ClangTool tool(optionsParser->getCompilations(), optionsParser->getSourcePathList());

	
	PostbuildFinder postbuildFinder(filepath);
	MatchFinder finder;

	DeclarationMatcher objectMatcher = cxxRecordDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
	DeclarationMatcher propertyMatcher = fieldDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
	DeclarationMatcher enumMatcher = enumDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
	
	finder.addMatcher(enumMatcher, &postbuildFinder);
	finder.addMatcher(objectMatcher, &postbuildFinder);
	finder.addMatcher(propertyMatcher, &postbuildFinder);
	
	int result = tool.run(newFrontendActionFactory(&finder).get());
	GTR_ASSERT(result == 0, "Reflection's postbuild step failed!\n");
	SendOverPipe("\\\\.\\pipe\\GreenTeaServer", "BuildEnded");
	printf("----------------------------\n");
	return 0;
}


std::pair<bool, std::string> parseargs(const char** argv)
{
	std::string first{ argv[1] };
	std::string second{ argv[2] };
	
	bool isPre = true;
	std::string dir = "";

	auto header = first.substr(0, 5);
	if (header.compare("-post") == 0)
		isPre = false;
	else if (header.compare("-dir=") == 0)
		dir = first.substr(5);
	else if (first.substr(0, 4).compare("-pre") != 0) { GTR_ASSERT(false, "Not valid argument: %s.\n", argv[1]); }

	header = second.substr(0, 5);
	if (header.compare("-post") == 0)
		isPre = false;
	else if (header.compare("-dir=") == 0)
		dir = second.substr(5);
	else if (second.substr(0, 4).compare("-pre") != 0) { GTR_ASSERT(false, "Not valid argument: %s.\n", argv[2]); }

	return std::make_pair(isPre, dir);
}

#include <fstream>

void GetFilesR(const std::filesystem::path& dir, std::vector<std::string>& headers)
{
	for (auto dirEntry : std::filesystem::directory_iterator(dir))
	{
		const auto& path = dirEntry.path();
		const auto relative = std::filesystem::relative(path);
		if (dirEntry.is_directory())
			GetFilesR(path, headers);
		else
		{
			const auto& extension = relative.extension().string();
			if (extension.compare(".hpp") == 0)
				headers.push_back(relative.string());
			else if (extension.compare(".h") == 0)
				headers.push_back(relative.string());
		}
	}
}

void WriteClangFileR(std::ofstream& output, std::vector<std::string>& done, std::string& header)
{
	if (std::find(done.begin(), done.end(), std::filesystem::absolute(header).string()) != done.end())
		return;

	printf("%s\n", std::filesystem::path(header).filename().string().c_str());
	
	const auto dir = header.substr(0, header.find_last_of("/\\"));
	done.push_back(std::filesystem::absolute(header).string());
	std::ifstream input(header);
	std::string line;
	while (std::getline(input, line))
	{
		if (line.compare("#pragma once") == 0)
			continue;
		const auto check = line.substr(0, 8);
		if (line.find("#define") != std::string::npos)//TODO(Vasilis): Maybe remove
		{
			output << line << '\n';
			continue;
		}
		else if (line.find("CLASS(") != std::string::npos ||
			line.find("COMPONENT(") != std::string::npos ||
			line.find("SYSTEM(") != std::string::npos)
		{
			size_t index = line.find("(");
			output << line.substr(0, index) <<"(header=\"" << header << "\"";
			const auto start = line.find("name");
			if (start != std::string::npos)
				output << ", " << line.substr(start) << '\n';
			else
				output << ")\n";
			continue;
		}
		else if (line.find("std::string") != std::string::npos)
		{
			size_t index = line.find("std::string");
			while (index != std::string::npos)
			{
				line = line.replace(line.begin() + index, line.begin() + index + 11, "dumm::String");
				index = line.find("std::string");
			}
			output << line << '\n';
			continue;
		}
		else if (check.find("#include") == std::string::npos)
		{
			output << line << '\n';
			continue;
		}

		const size_t start = std::min(std::string(line).find('"'), std::string(line).find('<'));
		const size_t end = std::min(std::string(line).find_last_of('"'), std::string(line).find_last_of('>'));

		auto toinclude = line.substr(start + 1, end - start - 1);
		if (std::find(done.cbegin(), done.cend(), toinclude) != done.cend())//Already included
			continue;
		if (!std::filesystem::exists(dir + "/" + toinclude) && !std::filesystem::exists(toinclude))//Including 3rdParty Header that hasn't been included
		{
			output << line << '\n';
			done.push_back(toinclude);
		}

		const auto absheader = std::filesystem::exists(toinclude) ? std::filesystem::absolute(toinclude) : std::filesystem::absolute(dir + "/" + toinclude);
		if (std::find(done.cbegin(), done.cend(), absheader) == done.cend())
			WriteClangFileR(output, done, dir + "/" + toinclude);
	}
}

void CreateClangFile(void)
{
	printf("Start building clangdump.hpp\n");
	const auto ProjectDir = std::filesystem::current_path().string();
	const size_t start = std::min(ProjectDir.find_last_of('/'), ProjectDir.find_last_of('\\'));
	const auto current = ProjectDir.substr(start + 1) + "/src";
		
	std::vector<std::string> headers;
	GetFilesR(current, headers);

	char buffer[40];
	auto it = std::crbegin(buffer);

	std::string clangdumm = std::string(argv[1]);
	std::ofstream output(clangdumm);
	std::time_t result = std::time(nullptr);
	output << "//Auto Generated file by gtreflect.exe at " << std::asctime(std::localtime(&result));
	output << "#include \"dummstring.h\"\n";

	std::vector<std::string> done = { "string", "string_view" };
	for (auto& header : headers)
		WriteClangFileR(output, done, header);

	output.close();
	printf("Done building clangdump.hpp\n");
}

void SendOverPipe(const char* pipename, const char* msg)
{
	HANDLE pipe = INVALID_HANDLE_VALUE;
	pipe = CreateFileA
	(
		pipename,
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		printf("GreenTea engine is not running\n");
		return;
	}
	
	DWORD bytes;
	int32_t result = WriteFile(pipe, msg, (DWORD)strlen(msg) + 1, &bytes, nullptr);

	GTR_ASSERT(result, "Failed to send message through the pipe.");

	char buffer[1024];
	result = ReadFile(pipe, buffer, 1024, &bytes, nullptr);

	GTR_ASSERT(result, "Failed receiving message from pipe.");
	DisconnectNamedPipe(pipe);
	CloseHandle(pipe);
	
	buffer[bytes] = '\0';

	GTR_ASSERT(strcmp(buffer, "Ok") == 0, "Not valid answer received.");
}