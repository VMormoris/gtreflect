#include "Finders.h"
#include "utils.h"

#include <yaml-cpp/yaml.h>
#include <clang/AST/RecordLayout.h>

static YAML::Node obj = {};

static std::unordered_map<std::string, std::pair<std::string, size_t>> classes = {};
static std::unordered_map<std::string, std::pair<std::string, FieldType>> enums = {};

ClassFinder::ClassFinder(const std::filesystem::path& prjdir, const std::filesystem::path& prj, BuildFace face)
	: MatchCallback(), mFace(face)
{
	const auto& prjname = prj.string();
	mPrjDir = prjdir.string() + "\\" + prjname + "\\src";
	
	if (mFace == BuildFace::Prebuild)
	{
		std::string file = prjname + "\\exports.h";
		mHppOut.open(file, std::ofstream::app);
		file = prjname + "\\exports.cpp";
		mCppOut.open(file, std::ofstream::app);
	}

}

#include <iostream>

void ClassFinder::run(const MatchFinder::MatchResult& res)
{
	const auto* decl = res.Nodes.getNodeAs<clang::CXXRecordDecl>("id");
	if (!decl)
		return;
	std::filesystem::current_path(mPrjDir);//Change current path

	//Use clang and the inhouse utils get Information about the annotated class
	const auto& rname = decl->getDeclName().getAsString();
	printf("%s\n", rname.c_str());
	const auto annotation = get_annotation(decl);
	const auto aname = parse_arg(annotation, "name");
	auto headerFile = parse_arg(annotation, "header");
	const auto name = aname.empty() ? rname : aname;

	if (mFace == BuildFace::Prebuild)
	{
		mHppOut << "#include \"" << headerFile << "\"\n";
		mCppOut << "\tGAME_API " << rname << "* Create" + name + "(void){ return new " + rname + "(); }\n\n";
	}
	
	YAML::Node Class = {};
	Class["Real Name"] = rname;
	Class["Name"] = name;
	Class["Header File"] = headerFile;
	
	GTR_ASSERT(classes.find(rname) == classes.end(), "Class with name: \"%s\" has already been exported.", name.c_str());//Is it correct? not sure anymore
	
	classes.insert({ rname, {headerFile, obj["classes"].size() } });
	
	for (const auto& baseclass : decl->bases())
	{
		auto basename = baseclass.getType().getAsString();
		basename = basename.substr(basename.find(' ')+1);
		if (classes.find(basename) != classes.end())
		{
			const size_t index = classes[basename].second;
			const auto basefield = obj["classes"][index]["Fields"];
			for(const auto& field : basefield)
				Class["Fields"].push_back(YAML::Clone(field));
			Class["Defaults Size"] = YAML::Clone(obj["classes"][index]["Defaults Size"]);
			Class["Params Size"] = YAML::Clone(obj["classes"][index]["Params Size"]);
		}
	}

	/*for (const auto& vbaseclass : decl->vbases())
	{
		const auto basename = vbaseclass.getType().getAsString();
		if (classes.find(basename) != classes.end())
		{
			const size_t index = classes[basename].second;
			Class["Fields"] = obj["classes"][index]["Fields"];
		}
	}*/

	obj["classes"].push_back(Class);
}

void EnumFinder::run(const MatchFinder::MatchResult& res)
{
	const auto* decl = res.Nodes.getNodeAs<clang::EnumDecl>("id");
	if (!decl)
		return;

	//Get names
	const auto rname = decl->getDeclName().getAsString();
	const auto annotation = get_annotation(decl);
	const auto aname = parse_arg(annotation, "name");
	const auto name = aname.empty() ? rname : aname;
	const auto type = enumtype(decl);

	//Start building YAML
	YAML::Node Enum = {};
	Enum["Name"] = name;
	Enum["Type"] = (uint16_t)type;
	for (auto it = decl->enumerator_begin(); it != decl->enumerator_end(); ++it)
	{
		YAML::Node enumarator = {};
		enumarator["Name"] = it->getNameAsString();
		enumarator["Value"] = it->getInitVal().getExtValue();
		Enum["Enumarators"].push_back(enumarator);
	}
	obj["enums"].push_back(Enum);
	enums.insert({ rname, { name, type } });

}

#include <iostream>

void FieldFinder::run(const MatchFinder::MatchResult& res)
{
	const clang::FieldDecl* field = res.Nodes.getNodeAs<clang::FieldDecl>("id");
	const auto& owner = field->getParent()->getNameAsString();
	
	if (!field)
		return;
	GTR_ASSERT(classes.find(field->getParent()->getDeclName().getAsString()) != classes.end(), "Cannot export a field of a class that wouldn't be exported\n");
	
	const auto& rname = field->getDeclName().getAsString();
	const auto annotation = get_annotation(field);
	const auto typestr = field->getType().getCanonicalType().getAsString();

	//Find Type
	FieldType type = get_type(field->getType().getCanonicalType());
	bool isEnumaration = false;
	std::string Typename = "Undefined";
	if (typestr.find("enum") != std::string::npos)
	{
		const auto temp = typestr.substr(5);
		GTR_ASSERT(enums.find(temp) != enums.end(), "Cannot export an enum field, of enum that wouldn't be exported\n");
		type = enums[temp].second;
		Typename = enums[temp].first;
		isEnumaration = true;
	}

	YAML::Node args = parse_args(annotation, type);
	auto name = rname;
	if (args["name"])
	{
		name = args["name"].as<std::string>();
		args.remove("name");
	}

	//Find offsets
	std::vector<uint64_t> offsets = {};
	{//ASTRecordLayout doesn't work like I want so I need to mannually setup the offsets
		const auto* parent = field->getParent();
		uint64_t offset = (res.Context->getASTRecordLayout(parent)).getFieldOffset(0)/8;
		const auto range = parent->fields();
		for (auto ptr : range)
		{
			offsets.push_back(offset);
			offset += fieldsize(ptr);
		}
	}

	//Populate object
	YAML::Node doc = {};
	doc["Name"] = name;
	doc["Type"] = (uint16_t)type;
	doc["Offset"] = offsets[field->getFieldIndex()];
	setup_default(doc, field, type); 

	for (const auto& arg : args)
		doc[arg.first.as<std::string>()] = arg.second;

	if (isEnumaration)
		doc["Typename"] = Typename;

	for (auto& object : obj["classes"])
	{
		if (object["Real Name"].as<std::string>().compare(owner) == 0)
		{
			if (object["Params Size"])
				object["Params Size"] = object["Params Size"].as<uint64_t>() + paramsize(type);
			else
				object["Params Size"] = paramsize(type);
			if (object["Defaults Size"])
				object["Defaults Size"] = object["Defaults Size"].as<uint64_t>() + fieldsize(field);
			else
				object["Defaults Size"] = fieldsize(field);
			object["Fields"].push_back(doc);
			break;
		}
	}
	//return field->dump();
}

[[nodiscard]] YAML::Node& GetYAMLObject(void) noexcept { return obj; }
