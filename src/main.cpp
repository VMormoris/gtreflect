#include "Builds.h"
#include "reflect.h"
#include "utils.h"
#include <cstdio>
//Parses the given command line arguments
Arguments ParseArgs(int argc, char** argv);
#include <iostream>
int main(int argc, char** argv)
{
	auto args = ParseArgs(argc, argv);
	if (args.Face == BuildFace::Prebuild)
		return PrebuildRun(args);
	else
		return PostbuildRun(args);
}

Arguments ParseArgs(int argc, char** argv)
{
	GTR_ASSERT(argc == 4, "Three arguments required for gtreflect But we got: %d.\n", argc-1);
	std::string arg0 = trimchar(std::string(argv[1]), ' ', true);
	std::string arg1 = trimchar(std::string(argv[2]), ' ', true);
	std::string arg2 = trimchar(std::string(argv[3]), ' ', true);

	Arguments args;

	std::string temp = arg0.substr(0, 5);
	if (temp.compare("-dir=") == 0)
		args.ProjectDir = trimchar(arg0.substr(5, arg0.size() - 1), '"');
	else if (temp.compare("-prj=") == 0)
		args.ProjectName = trimchar(arg0.substr(5, arg0.size() - 1), '"');
	else if (temp.compare("-post") == 0)
		args.Face = BuildFace::Postbuild;
	else if (arg0.substr(0, 4).compare("-pre") == 0)
		args.Face = BuildFace::Prebuild;
	else { GTR_ASSERT(false, "Invalid command line argument: %s\n", arg0.c_str()); }
	
	temp = arg1.substr(0, 5);
	if (temp.compare("-dir=") == 0)
		args.ProjectDir = trimchar(arg1.substr(5, arg1.size() - 1), '"');
	else if (temp.compare("-prj=") == 0)
		args.ProjectName = trimchar(arg1.substr(5, arg1.size() - 1), '"');
	else if (temp.compare("-post") == 0)
		args.Face = BuildFace::Postbuild;
	else if (arg1.substr(0, 4).compare("-pre") == 0)
		args.Face = BuildFace::Prebuild;
	else { GTR_ASSERT(false, "Invalid command line argument: %s\n", arg1.c_str()); }
	
	temp = arg2.substr(0, 5);
	if (temp.compare("-dir=") == 0)
		args.ProjectDir = trimchar(arg2.substr(5, arg2.size() - 1), '"');
	else if (temp.compare("-prj=") == 0)
		args.ProjectName = trimchar(arg2.substr(5, arg2.size() - 1), '"');
	else if (temp.compare("-post") == 0)
		args.Face = BuildFace::Postbuild;
	else if (arg2.substr(0, 4).compare("-pre") == 0)
		args.Face = BuildFace::Prebuild;
	else { GTR_ASSERT(false, "Invalid command line argument: %s\n", arg2.c_str()); }


	return args;
}