#include "Builds.h"

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Tooling/Tooling.h"

#include "utils.h"
#include "Finders.h"
#include "uuid.h"

#include <yaml-cpp/yaml.h>
#include <iostream>

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

/*
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_os_ostream.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Driver/Options.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
*/

static llvm::cl::OptionCategory gToolCategory("GT reflection options");
static constexpr char* argv[2] = { "gtreflect.exe", ".gt/clangdump.hpp" };
static int argc = 2;
static std::filesystem::path CodeDir = "";
void CreateClangFile(void);

int PrebuildRun(const Arguments& args)
{
	using namespace clang::tooling;
	using namespace llvm;
	std::cout << "------ Prebuild -------\n";

	CodeDir = args.ProjectDir / args.ProjectName;

	std::filesystem::current_path(CodeDir);
	CreateClangFile();

	const auto exportsh = "exports.h";
	const auto exportscpp = "exports.cpp";

	//Clean the exports.h & exports.cpp from it's contents
	//The files will be build using append while 
	//	the application is running.
	std::ofstream eheader(exportsh); eheader.close();
	std::ofstream ecpp(exportscpp);
	ecpp << "#include \"exports.h\"\n\n";
	ecpp << "extern \"C\" {\n\n";
	ecpp.close();

	//Create a new clang tool
	std::filesystem::current_path(args.ProjectDir);
	Expected<CommonOptionsParser> expected = CommonOptionsParser::create(argc, (const char**)argv, gToolCategory);
	ClangTool tool(expected->getCompilations(), expected->getSourcePathList());

	//Search for classes with annotations
	//	Update the C++ files & build YAML Object
	int res = 0;
	{
		ClassFinder classFinder(args.ProjectDir, args.ProjectName, args.Face);
		MatchFinder finder;
		DeclarationMatcher recordMatcher = cxxRecordDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
		finder.addMatcher(recordMatcher, &classFinder);
		res = tool.run(newFrontendActionFactory(&finder).get());
	}
	GTR_ASSERT(res == 0, "Clang compilation failed.");

	YAML::Node Previous = read_current_state(args.ProjectDir.string())["classes"];
	YAML::Node Current = GetYAMLObject()["classes"];
	YAML::Node Newones = {};

	size_t classes = Current.size();
	for (size_t i = classes - 1, l=0; l<classes; i--, l++)
	{
		size_t prev_index = Previous.size();
		size_t min = 9;
		for (size_t j = 0; j < Previous.size(); j++)
		{
			if (distance(Current[i], Previous[j]) < min)
			{
				prev_index = j;
				min = distance(Current[i], Previous[j]);
				if (min == 0)
					break;
			}
		}

		if (min < 9)
		{
			Current.remove(i);
			Previous.remove(prev_index);
		}
		else
			Newones.push_back(Current[i]);
	}
	
	if (Newones.size() != 0 || Previous.size() != 0)
	{
		YAML::Node output = YAML::LoadFile(args.ProjectDir.string() + "/.gt/reflection.cache");
		for (size_t i = 0; i < Previous.size(); i++)
		{
			const auto searching = Previous[i]["filepath"].as<std::string>();
			for (size_t j = 0; j < output.size(); j++)
			{
				if (searching.compare(output[j].as<std::string>()) == 0)
				{
					std::filesystem::remove(output[j].as<std::string>());
					output.remove(j);
					break;
				}
			}
		}

		for (size_t i = 0; i < Newones.size(); i++)
		{
			//Create new .gtscript file
			auto& script = Newones[i];
			script["UUID"] = uuid::Create().str();
			script["Asset Type"] = 4;
			std::ofstream file(args.ProjectDir.string() + "/Assets/Scripts/" + script["Real Name"].as<std::string>() + ".gtscript");
			file << "UUID: " << uuid::Create().str()  << '\n';
			file << "Asset Type: 4\n";
			file << "Real Name: " << script["Real Name"].as<std::string>() << '\n';
			file << "Name: " << script["Name"].as<std::string>() << '\n';
			file << "Header File: " << script["Header File"].as<std::string>();
			file.close();
			output.push_back("Scripts/" + script["Real Name"].as<std::string>() + ".gtscript");
		}
		//Flush cache
		std::ofstream cache(args.ProjectDir.string() + "/.gt/reflection.cache");
		cache << output;
		cache.close();
	}

	//Finish exports.cpp
	std::filesystem::current_path(args.ProjectDir / args.ProjectName);
	ecpp.open(exportscpp, std::ofstream::app);
	ecpp << '}';
	ecpp.close();
	std::cout << "-----------------------\n";

	return 0;
}

int PostbuildRun(const Arguments& args)
{
	using namespace clang::tooling;
	using namespace llvm;
	std::cout << "------ Postbuild ------\n";
	std::filesystem::current_path(args.ProjectDir);//Set currently working directory
	MatchFinder finder;
	EnumFinder enumFinder;
	ClassFinder classFinder(args.ProjectDir, args.ProjectName, BuildFace::Postbuild);
	FieldFinder fieldFinder;
	
	DeclarationMatcher enumMatcher = enumDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
	DeclarationMatcher recordMatcher = cxxRecordDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));
	DeclarationMatcher fieldMatcher = fieldDecl(decl().bind("id"), hasAttr(clang::attr::Annotate));

	finder.addMatcher(enumMatcher, &enumFinder);
	finder.addMatcher(recordMatcher, &classFinder);
	finder.addMatcher(fieldMatcher, &fieldFinder);

	//Create a new clang tool
	Expected<CommonOptionsParser> expected = CommonOptionsParser::create(argc, (const char**)argv, gToolCategory);
	ClangTool tool(expected->getCompilations(), expected->getSourcePathList());
	int res = tool.run(newFrontendActionFactory(&finder).get());
	if (res != 0)
		return res;

	YAML::Node obj = GetYAMLObject();
	{//Check whether enums need update
		const auto file = args.ProjectDir.string() + "/.gt/enums.cache";
		const auto& Current = obj["enums"];
		const auto Previous = std::filesystem::exists(file) ?  YAML::LoadFile(file) : YAML::Node{};
		
		if (!CompareEnums(Previous, Current))
		{
			std::ofstream output(file);
			output << Current;
			output.close();
		}
	}

	{//Change .gtscript where and if need it
		YAML::Node Files = YAML::LoadFile(args.ProjectDir.string() + "/.gt/reflection.cache");
		YAML::Node Current = obj["classes"];
		for (const auto& filenode : Files)
		{
			const auto filepath = args.ProjectDir.string() + "/Assets/" + filenode.as<std::string>();
			YAML::Node Previous = YAML::LoadFile(filepath);
			size_t min = 9;
			size_t index = Current.size() + 1;
			for (size_t i=0; i<Current.size(); i++)
			{
				size_t dist = distance(Current[i], Previous);
				if (distance(Current[i], Previous) < min)
				{
					index = i;
					min = dist;
					if(dist == 0)
						break;
				}
			}
			if (!CompareClasses(Current[index], Previous))
			{
				std::ofstream out(filepath);
				out << "UUID: " << Previous["UUID"].as<std::string>() << '\n';
				out << "Asset Type: 4\n";
				out << "Real Name: " <<  Current[index]["Real Name"].as<std::string>() << '\n';
				out << "Name: " << Current[index]["Name"].as<std::string>() << '\n';
				out << "Header File: " << Current[index]["Header File"].as<std::string>() << '\n';
				out << "Params Size: " << Current[index]["Params Size"].as<uint64_t>() << '\n';
				out << "Defaults Size: " << Current[index]["Defaults Size"].as<uint64_t>() << '\n';
				if (Current[index]["Fields"].IsDefined())
				{
					YAML::Node fields = {};
					fields["Fields"] = Current[index]["Fields"];
					out << fields;
				}
				out.close();
			}
		}
	}

	std::cout << "-----------------------\n";
	return 0;
}



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
		if (line.find("CLASS(") != std::string::npos)
		{
			output << "CLASS(header=\"" << header << "\"";
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
		
		auto toinclude = trimchar(line.substr(8), ' ', true);
		toinclude = trimchar(toinclude, '"');
		if (toinclude[0] == '<' && toinclude[toinclude.size() - 1])
			toinclude = toinclude.substr(1, toinclude.size() - 2);
		
		if (std::find(done.cbegin(), done.cend(), toinclude) != done.cend())//Already included
			continue;
		if (!std::filesystem::exists(dir + "/" + toinclude) && !std::filesystem::exists(toinclude))//Including 3rdParty Header that hasn't been included
		{
			output << line << '\n';
			done.push_back(toinclude);
		}

		const auto absheader = std::filesystem::exists(toinclude) ? std::filesystem::absolute(toinclude) : std::filesystem::absolute(dir + "/" + toinclude);
		if(std::find(done.cbegin(), done.cend(), absheader) == done.cend())
			WriteClangFileR(output, done, dir + "/" + toinclude);
	}
}

const char* MVSCString = R"(
namespace dumm{

    //Dump String to simulate MVSC std::String on clangdump
    //We don't need implementation just to declare the right size and all the methods
    class String{
    public:
        using iterator = char*;
        using const_iterator = const char*;
        using reverse_iterator = std::reverse_iterator<char*>;
        using const_reverse_iterator = std::reverse_iterator<const char*>;
    public:
        static constexpr size_t npos{static_cast<const size_t>(-1)};
    public:
        String() = default;
        String(const std::allocator<char> alloc) {};
        ~String() = default;
        String(const String&) = default;
        String(String&&) = default;
        String(size_t, char, const std::allocator<char>& alloc = std::allocator<char>()) {}
        String(const String&, size_t, const std::allocator<char>& alloc = std::allocator<char>()) {}
        String(const String&, size_t, size_t, const std::allocator<char>& alloc = std::allocator<char>()) {}
        String(const char*, size_t, const std::allocator<char>& alloc = std::allocator<char>()) {}
        String(const char*, const std::allocator<char>& alloc = std::allocator<char>()) {}
        template<typename It>
        String(It, It, const std::allocator<char>& alloc = std::allocator<char>()) {}
        String(const String&, const std::allocator<char>&) {}
        String(String&&, const std::allocator<char>&) {}
        String(std::initializer_list<char>,  const std::allocator<char>& alloc = std::allocator<char>()) {}
        template<typename T>
        String(const T&, const std::allocator<char>& alloc = std::allocator<char>()) {}
        template<typename T>
        String(const T&, size_t, size_t, const std::allocator<char>& alloc = std::allocator<char>()) {}

        String& operator=(const String&) = default;
        String& operator=(String&&) = default;
        String& operator=(const char*) { return *this; }
        String& operator=(char) { return *this; }
        String& operator=(std::initializer_list<char>) { return *this; }
        template<typename T>
        String& operator=(const T&) { return *this; }

        String& assign(size_t, char) { return *this; }
        String& assign(const String&) { return *this; }
        String& assign(const String&, size_t, size_t count = npos) { return *this; }
        String& assign(String&&) noexcept { return *this; }
        String& assign(const char*, size_t) { return *this; }
        String& assign(const char*) { return *this; }
        template<typename It>
        String& assign(It, It) { return *this; }
        String& assign(std::initializer_list<char>) { return *this; }
        template<typename T>
        String& assign(const T&) { return *this; }
        template<typename T>
        String& assign(const T&, size_t, size_t count = npos) { return *this; }

        std::allocator<char> get_allocator() const { return std::allocator<char>(); }
        
        char& at(size_t) { return mBuffer[0]; }
        const char& at(size_t) const { return mBuffer[0]; }

        char& operator[](size_t) { return mBuffer[0]; }
        const char& operator[](size_t) const { return mBuffer[0]; }

        char& front() { return mBuffer[0]; }
        const char& front() const { return mBuffer[0]; }

        
        char& back() { return mBuffer[0]; }
        const char& back() const { return mBuffer[0]; }

        const char* data() const noexcept { return mBuffer; }
        char* data() noexcept { return mBuffer; }

        const char* c_str() const { return mBuffer; }

        operator std::string_view() const noexcept { return std::string_view(mBuffer); }

        iterator begin() noexcept { return std::begin(mBuffer); }
        const_iterator begin() const { return std::cbegin(mBuffer); }
        const_iterator cbegin() const { return std::cbegin(mBuffer); }
        iterator end() noexcept { return std::end(mBuffer); }
        const_iterator end() const { return std::cend(mBuffer); }
        const_iterator cend() const { return std::cend(mBuffer); }
        reverse_iterator rbegin() noexcept { return std::rbegin(mBuffer); }
        const_reverse_iterator rbegin() const { return std::crbegin(mBuffer); }
        const_reverse_iterator crbegin() const { return std::crbegin(mBuffer); }
        reverse_iterator rend() noexcept { return std::rend(mBuffer); }
        const_reverse_iterator rend() const { return std::crend(mBuffer); }
        const_reverse_iterator crend() const { return std::crend(mBuffer); }

        bool empty() const noexcept { return true; }
        size_t size() const noexcept { return 0; }
        size_t length() const noexcept { return 0; }

        size_t max_size() const noexcept { return 64; }
        size_t capacity() const noexcept { return 64; }

        void reverse(size_t newcap=0) {}
        void shrink_to_fit() {}

        void clear() noexcept {}

        String& insert(size_t, size_t, char) { return *this; }
        String& insert(size_t, const char*) { return *this; }
        String& insert(size_t, const char*, size_t) { return *this; }
        String& insert(size_t, const String&) { return *this; }
        String& insert(size_t, const String&, size_t, size_t count=npos);
        iterator insert(const_iterator, char) { return begin(); }
        iterator insert(const_iterator, size_t, char) { return begin(); }
        template<typename It>
        void insert(const_iterator, It, It) {}
        iterator insert(const_iterator, std::initializer_list<char>) { return begin(); }
        template<typename T>
        String& insert(size_t, const T&) { return *this; }
        template<typename T>
        String& insert(size_t, const T&, size_t, size_t count = npos) { return *this; }

        String& erase(size_t index = 0, size_t count = npos) { return *this; }
        iterator erase(const_iterator) { return begin(); }
        iterator erase(const_iterator, const_iterator) { return begin(); }

        void push_back(char) {}
        void pop_back() {}

        String& append(size_t, char) { return *this; }
        String& append(const String&) { return *this; }
        String& append(const String&, size_t, size_t count=npos) { return *this; }
        String& append(const char*, size_t) { return *this; }
        String& append(const char*) { return *this; }
        template<typename It>
        String& append(It, It) { return *this; }
        String& append(std::initializer_list<char>) { return *this; }
        template<typename T>
        String& append(const T&) { return *this; }
        template<typename T>
        String& append(const T, size_t, size_t count = npos) { return *this; }

        String& operator+=(const String&) { return *this; }
        String& operator+=(char) { return *this; }
        String& operator+=(const char*) { return *this; }
        String& operator+=(std::initializer_list<char>) { return *this; }
        template<typename T>
        String& operator+=(const T&) { return *this; }

        int compare(const String&) const noexcept { return 0; }
        int compare(size_t, size_t, const String&) const { return 0; }
        int compare(size_t, size_t, const String&, size_t, size_t count = npos) const { return 0; }
        int compare(const char*) const { return 0; }
        int compare(size_t, size_t, const char*) const { return 0; }
        int compare(size_t, size_t, const char*, size_t) const { return 0; }
        template<typename T>
        int compare(const T&) const noexcept { return 0; }
        template<typename T>
        int compare(size_t, size_t, const T&) const { return 0; }
        template<typename T>
        int compare(size_t, size_t, const T&, size_t, size_t count = npos) const { return 0; }

        String& replace(size_t, size_t, const String&) { return *this; }
        String& replace(const_iterator, const_iterator) { return *this; }
        template<typename It>
        String& replace(const_iterator, const_iterator, It, It) { return *this; }
        String& replace(size_t, size_t, const char*, size_t) { return *this; }
        String& replace(const_iterator, const_iterator, const char*, size_t) { return *this; }
        String& replace(size_t, size_t, const char*) { return *this; }
        String& replace(const_iterator, const_iterator, const char*) { return *this; }
        String& replace(size_t, size_t, size_t, char) { return *this; }
        String& replace(const_iterator, const_iterator, size_t, char) { return *this; }
        String& replace(const_iterator, const_iterator, std::initializer_list<char>) { return *this; }
        template<typename T>
        String& replace(size_t, size_t, const T&) { return *this; }
        template<typename T>
        String& replace(const_iterator, const_iterator, const T&) { return *this; }
        template<typename T>
        String& replace(size_t, size_t, const T&, size_t, size_t count = npos) { return *this; }

        String substr(size_t pos = 0, size_t count = npos) const { return String(mBuffer); }

        size_t copy(char*, size_t, size_t pos = 0) const { return 0; }

        void resize(size_t) {}
        void resize(size_t, char) {}

        void swap(const String& other) noexcept {}

        size_t find(const String& str, size_t pos = 0) const noexcept { return 0; }
        size_t find(const char*, size_t, size_t) const { return 0; }
        size_t find(const char*, size_t pos = 0) const { return 0; }
        size_t find(char, size_t pos = 0) const noexcept { return 0; }
        template<typename T>
        size_t find(const T& t, size_t pos = 0) const noexcept { return 0; }

        size_t rfind(const String& str, size_t pos = npos) const noexcept { return 63; }
        size_t rfind(const char*, size_t, size_t) const { return 63; }
        size_t rfind(const char*, size_t pos = npos) const { return 63; }
        size_t rfind(char, size_t pos = npos) const noexcept { return 63; }
        template<typename T>
        size_t rfind(const T& t, size_t pos = npos) const noexcept { return 63; }

    private:
        char mBuffer[40];
    };

}
)";

void CreateClangFile(void)
{
	const auto current = "src";
	std::vector<std::string> headers;
	GetFilesR(current, headers);

	char buffer[40];
	auto it = std::crbegin(buffer);

	std::ofstream output("../" + std::string(argv[1]));
	output << "//Auto Generated file by gtreflect.exe\n";
	output << "#include <string>\n";
	output << "#include <string_view>\n";
	output << MVSCString << '\n';

	std::vector<std::string> done = {"string", "string_view"};
	for (auto& header : headers)
		WriteClangFileR(output, done, header);

	output.close();
}