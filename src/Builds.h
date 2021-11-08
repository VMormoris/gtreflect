#pragma once
#include <filesystem>

//Emumartation for the two types of Build stages
enum class BuildFace {
	Prebuild = 0,
	Postbuild
};

/**
* @brief Structure that holds the intreprentation 
*	of the command line arguments 
*/
struct Arguments {
	/**
	* @brief Path to the Directory where the project is located
	*/
	std::filesystem::path ProjectDir;

	/**
	* @brief The name of the current project
	* @detail It's also the name of the subedirectory containing the
	*	code for the Scripts
	*/
	std::filesystem::path ProjectName;

	/**
	* @brief The build stage
	*/
	BuildFace Face = BuildFace::Prebuild;
	
	//Constructor(s)
	Arguments(void) = default;
	Arguments(const std::filesystem::path& dir, const std::filesystem::path& prj)
		: ProjectDir(dir), ProjectName(prj) {}
};

/**
* @brief Application's tasks if we are running before as prebuild stage
* @details During the prebuild stage the application must Rewrite exports.h & exports.cpp
*	files that handles exporting all the necessary function that the engine uses. Update 
*	the map with the scripting assets that the project manages. Create new script assets
*	for its new Class that need to be exported.
* 
* @param args Structure containing the command line arguments in the appropriate format
* @return The termination code of the application
*/
int PrebuildRun(const Arguments& args);

/**
* @brief Application's tasks if we are running before as prostbuild stage
* @details During the postbuild stage the application uses clangdump.hpp file
*	that was created during the prebuild stage. And creates the appropriate files
*	that are need it by the GreenTea engine in order to be aware of the client code.
*	Such files are the enums.cache, reflection.cache an a .gtscript file for each class
*	that the user wants to be exported
*
* @param args Structure containing the command line arguments in the appropriate format
* @return The termination code of the application
*/
int PostbuildRun(const Arguments& args);