--Put your llvm directory here
llvmDir = "D:/dev/llvm-project"


workspace "gtreflect"
    architecture "x64"

	configurations
	{
		"Debug",
		"Release"
	}

	startproject "gtreflect"

outputdir = "%{cfg.buildcfg}-%{cfg.system}"

IncludeDirs = {}
IncludeDirs["yaml"] = "3rdParty/yaml-cpp/include"
IncludeDirs["clangtools"] = "%{llvmDir}/build/tools/clang/include"
IncludeDirs["clangutils"] = "%{llvmDir}/utils/bazel/llvm-project-overlay/llvm/include"
IncludeDirs["clangbuild"] = "%{llvmDir}/build/include"
IncludeDirs["llvm"] = "%{llvmDir}/llvm/include"
IncludeDirs["clang"] = "%{llvmDir}/clang/include"

LibFiles = {}
LibFiles["clangTooling"] = "clangTooling.lib"
LibFiles["clangFrontend"] = "clangFrontend.lib"
LibFiles["clangSerialization"] = "clangSerialization.lib"
LibFiles["clangSupport"] = "clangSupport.lib"
LibFiles["clangASTMatchers"] = "clangASTMatchers.lib"
LibFiles["clangAST"] = "clangAST.lib"
LibFiles["clangBasic"] = "clangBasic.lib"
LibFiles["clangLex"] = "clangLex.lib"
LibFiles["clangDriver"] = "clangDriver.lib"
LibFiles["clangParse"] = "clangParse.lib"
LibFiles["clangRewrite"] = "clangRewrite.lib"
LibFiles["clangSema"] = "clangSema.lib"
LibFiles["clangAnalysis"] = "clangAnalysis.lib"
LibFiles["clangEdit"] = "clangEdit.lib"
LibFiles["LLVMSupport"] = "LLVMSupport.lib"
LibFiles["LLVMDebugInfoDWARF"] = "LLVMDebugInfoDWARF.lib"
LibFiles["LLVMAsmParser"] = "LLVMAsmParser.lib"
LibFiles["LLVMIRReader"] = "LLVMIRReader.lib"
LibFiles["LLVMObject"] = "LLVMObject.lib"
LibFiles["LLVMWindowsDriver"] = "LLVMWindowsDriver.lib"
LibFiles["LLVMAnalysis"] = "LLVMAnalysis.lib"
LibFiles["LLVMFrontendOpenMP"] = "LLVMFrontendOpenMP.lib"
LibFiles["LLVMOption"] = "LLVMOption.lib"
LibFiles["LLVMCore"] = "LLVMCore.lib"
LibFiles["LLVMBitReader"] = "LLVMBitReader.lib"
LibFiles["LLVMBitstreamReader"] = "LLVMBitstreamReader.lib"
LibFiles["LLVMProfileData"] = "LLVMProfileData.lib"
LibFiles["clangStaticAnalyzerCore"] = "clangStaticAnalyzerCore.lib"
LibFiles["LLVMTargetParser"] = "LLVMTargetParser.lib"
LibFiles["LLVMTextAPI"] = "LLVMTextAPI.lib"
LibFiles["LLVMTransformUtils"] = "LLVMTransformUtils.lib"
LibFiles["LLVMDemangle"] = "LLVMDemangle.lib"
LibFiles["LLVMMC"] = "LLVMMC.lib"
LibFiles["LLVMMCParser"] = "LLVMMCParser.lib"
LibFiles["LLVMBinaryFormat"] = "LLVMBinaryFormat.lib"
LibFiles["LLVMRemarks"] = "LLVMRemarks.lib"
LibFiles["LLVMScalarOpts"] = "LLVMScalarOpts.lib"
LibFiles["version"] = "version.lib"

include "3rdParty/yaml-cpp"

project "gtreflect"
    location "src"
    kind "ConsoleApp"
    language "C++"
	cppdialect "C++17"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.cpp",
        "src/**.hpp",
        "src/**.h",
    }

    includedirs
    {
        "%{IncludeDirs.yaml}",
        "%{IncludeDirs.clangtools}",
        "%{IncludeDirs.clangutils}",
        "%{IncludeDirs.clangbuild}",
        "%{IncludeDirs.clang}",
        "%{IncludeDirs.llvm}",
    }

    links
    {
        "yaml-cpp",
        "%{LibFiles.clangAST}",
        "%{LibFiles.clangASTMatchers}",
        "%{LibFiles.clangBasic}",
        "%{LibFiles.clangFrontend}",
        "%{LibFiles.clangSerialization}",
        "%{LibFiles.clangTooling}",
        "%{LibFiles.clangSupport}",
        "%{LibFiles.clangLex}",
        "%{LibFiles.clangDriver}",
        "%{LibFiles.clangParse}",
        "%{LibFiles.clangRewrite}",
        "%{LibFiles.clangSema}",
        "%{LibFiles.clangAnalysis}",
        "%{LibFiles.clangEdit}",
        "%{LibFiles.LLVMSupport}",
        "%{LibFiles.LLVMDebugInfoDWARF}",
        "%{LibFiles.LLVMWindowsDriver}",
        "%{LibFiles.LLVMAnalysis}",
        "%{LibFiles.LLVMFrontendOpenMP}",
        "%{LibFiles.LLVMAsmParser}",
        "%{LibFiles.LLVMIRReader}",
        "%{LibFiles.LLVMObject}",
        "%{LibFiles.LLVMOption}",
        "%{LibFiles.clangStaticAnalyzerCore}",
        "%{LibFiles.LLVMTargetParser}",
        "%{LibFiles.LLVMTextAPI}",
        "%{LibFiles.LLVMTransformUtils}",
        "%{LibFiles.LLVMCore}",
        "%{LibFiles.LLVMBitReader}",
        "%{LibFiles.LLVMBitstreamReader}",
        "%{LibFiles.LLVMProfileData}",
        "%{LibFiles.LLVMDemangle}",
        "%{LibFiles.LLVMMC}",
        "%{LibFiles.LLVMMCParser}",
        "%{LibFiles.LLVMBinaryFormat}",
        "%{LibFiles.LLVMRemarks}",
        "%{LibFiles.LLVMScalarOpts}",
        "%{LibFiles.version}",
    }

    defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        libdirs
        {
            "%{llvmDir}/build/Debug/lib",
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

        libdirs
        {
            "%{llvmDir}/build/Release/lib",
        }
