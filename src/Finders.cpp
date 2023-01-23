#include "Finders.h"
#include "AnnotationParser.h"
#include <fstream>

#include <clang/AST/RecordLayout.h>
#include "uuid.h"

//static std::vector<uint64_t> sOffsets = {};

static [[nodiscard]] Object input_object(const YAML::Node& data) noexcept;
static void input_metadata(const YAML::Node& data, FieldMetadata& meta, FieldType type) noexcept;
//static void output_object(std::ofstream& os, const Object& obj) noexcept;
static void output_metadata(YAML::Emitter& out, const FieldMetadata& data, const YAML::Node& Default);

void PostbuildFinder::WriteObjects(void) noexcept
{
	std::filesystem::path dir(mProjectDir / "Assets");

	//Read current scripts
	std::unordered_map<std::string, std::pair<uuid, Object>> Inputs;
	for (const auto entry : std::filesystem::recursive_directory_iterator(dir))
	{
		const auto filename = entry.path();
		const auto extension = filename.extension();
		if (extension.compare(".gtscript") != 0 && extension.compare(".gtcomp") != 0 && extension.compare(".gtsystem") != 0)//Not script asset
			continue;

		uuid id;
		uint16_t loaded = 0;
		size_t size = 0;
		
		std::ifstream is(filename);
		std::string line;
		while (getline(is, line))
		{
			if (line[0] == '#')//Comment ingore
				continue;
			else if (line.empty() || line[0] == '\n' || line[0] == '\r')
				break;
			if (loaded == 0) { GTR_ASSERT(stoul(line) == 4, "Asset type isn't NativeScript"); }
			else if (loaded == 1)
				id = line;
			else if (loaded == 2)
				size = stoull(line);
			loaded++;
		}

		char* buffer = new char[size + 1];
		buffer[size] = 0;
		is.read(buffer, size);
		is.close();

		//if (filename.stem() == "Player") { GTR_ASSERT(false, "Input: %s\n", buffer); }
		YAML::Node data;
		try { data = YAML::Load(buffer); }
		catch (YAML::ParserException e) { GTR_ASSERT(false, "Failed to load file: %s\n\t%s\n", filename.string().c_str(), e.what()); }
		delete[] buffer;

		Object obj = input_object(data);
		Inputs.insert({ std::filesystem::relative(filename, dir).string(), std::make_pair(id, obj) });
	}

	//Compares objects and find which should be written
	std::unordered_map<std::string, std::pair<uuid, Object>> Outputs;
	for (auto& [rname, obj] : Objects)
	{
		if (obj.Meta.Name.empty())
			continue;

		const auto& name = obj.Meta.Name;
		boolean found = false;
		for (const auto& [filepath, pair] : Inputs)
		{
			const auto& [id, old] = pair;
			if (name.compare(old.Meta.Name) != 0)
				continue;

			if (old != obj)
			{
				obj.Version = old.Version + 1;
				Outputs.insert({ filepath, std::make_pair(id, obj) });
			}
			Inputs.erase(filepath);
			found = true;
			break;
		}
		if (!found)
		{
			const auto extension = obj.Meta.Type == ReflectionType::Component ? ".gtcomp" : (obj.Meta.Type == ReflectionType::System ? ".gtsystem" : ".gtscript");
			const auto outpath = "Scripts/" + name + extension;
			Outputs.insert({ outpath, std::make_pair(uuid::Create(), obj) });
		}
	}

	//Delete files that are no longer in use
	for (const auto& [filepath, pair] : Inputs)
		std::remove(filepath.c_str());

	//Write objects that changes
	for (const auto& [filepath, pair] : Outputs)
	{
		const auto& [id, obj] = pair;

		std::time_t result = std::time(nullptr);
		std::ofstream os(dir / filepath, std::ios::binary);
		os << "# Native-Script Asset for GreenTea Engine\n" <<
			"# Auto generated by gtreflect.exe at " << std::asctime(std::localtime(&result)) <<
			4 << '\n' <<
			id << '\n';
		
		output_object(os, obj);
		printf("Writing: %s\n", obj.Meta.Name.c_str());
		os.close();
	}
}

void PostbuildFinder::WriteEnums(void) const noexcept
{
	std::filesystem::path filepath = mProjectDir / ".gt/enums.cache";
	bool shouldWrite = true;

	if (std::filesystem::exists(filepath))
	{
		YAML::Node data;
		try { data = YAML::LoadFile(filepath.string()); }
		catch (YAML::ParserException e) { GTR_ASSERT(false, "Failed to load file: enums.cache\n\t%s\n", e.what()); }
		
		//Parse Enumaration from files
		std::map<std::string, Enum> oldEnums;
		for (const auto& node : data)
		{
			const auto name = node["Name"].as<std::string>();
			size_t size = node["Size"].as<size_t>();
			FieldType type = (FieldType)node["Type"].as<uint64_t>();
			Enum obj(name, size, type);
			YAML::Node values = node["Values"];
			for (const auto& value : values)
			{
				const auto valname = value["Name"].as<std::string>();
				EnumValue val;
				if (obj.isUnsigned())
					val.Uvalue = value["Value"].as<uint64_t>();
				else
					val.Value = value["Value"].as<int64_t>();
				obj.Values.insert(std::make_pair(valname, val));
			}
			oldEnums.emplace(name, obj);
		}

		//Check if there is a difference
		if (oldEnums.size() == Enums.size())
		{
			bool flagged = false;
			for (const auto& [n, newenum] : Enums)
			{
				const auto name = newenum.Meta.Name;
				if (oldEnums.find(name) == oldEnums.end())
				{
					flagged = true;
					break;
				}
				if (oldEnums[name] != newenum)
				{
					flagged = true;
					break;
				}
				oldEnums.erase(name);
			}
			if (oldEnums.size() == 0 && !flagged)
				shouldWrite = false;
		}
	}

	if (!shouldWrite)
		return;
	printf("Writing enumarations\n");

	YAML::Emitter out;
	std::time_t result = std::time(nullptr);
	const auto comment = "Auto Generated file by gtreflect.exe at " + std::string(std::asctime(std::localtime(&result)));
	out << YAML::Comment(comment);
	out << YAML::BeginSeq;
	for (const auto& [rname, enumobj] : Enums)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << enumobj.Meta.Name;
		out << YAML::Key << "Type" << YAML::Value << (uint64_t)enumobj.Type;
		out << YAML::Key << "Size" << YAML::Value << enumobj.Meta.Size;
		out << YAML::Key << "Values" << YAML::Value;
		out << YAML::BeginSeq;
		for (const auto& [valname, val] : enumobj.Values)
			out << YAML::BeginMap <<
			YAML::Key << "Name" << YAML::Value << valname <<
			YAML::Key << "Value" << YAML::Value << (enumobj.isUnsigned() ? val.Uvalue : val.Value) <<
			YAML::EndMap;
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	std::ofstream file(filepath);
	file << out.c_str();
	file.close();
}

void PostbuildFinder::FoundField(const clang::FieldDecl* fieldrec) noexcept
{
	Finder::FoundField(fieldrec);

	//Parse annotation
	const auto& annotation = get_annotation(fieldrec);
	AnnotationParser parser(annotation);

	const auto owner = fieldrec->getParent()->getNameAsString();
	auto& field = Objects[owner].Fields.back();
	const FieldType type = field.Meta.ValueType;

	//Setup default value & metadata
	llvm::APInt intval;
	bool hasDefault = false;
	if (fieldrec->hasInClassInitializer())
	{
		hasDefault = true;
		const clang::Expr* expr = fieldrec->getInClassInitializer();

		clang::Expr::EvalResult result;
		if (expr->EvaluateAsInt(result, fieldrec->getASTContext()))
			intval = result.Val.getInt();
	}

	switch (type)
	{
	case FieldType::Char:
	case FieldType::Enum_Char:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
	case FieldType::Int16:
	case FieldType::Int32:
	case FieldType::Int64:
		if (parser.Has("min"))
			field.Meta.MinInt = parser.GetAs<int64_t>("min");
		if (parser.Has("max"))
			field.Meta.MaxInt = parser.GetAs<int64_t>("max");
		if (hasDefault)
			field.Default["Default"] = intval.getSExtValue();
		else
			field.Default["Default"] = 0;
		break;
	case FieldType::Byte:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
	case FieldType::Uint16:
	case FieldType::Uint32:
	case FieldType::Uint64:
		if (parser.Has("min"))
			field.Meta.MinUint = parser.GetAs<uint64_t>("min");
		if (parser.Has("max"))
			field.Meta.MaxUint = parser.GetAs<uint64_t>("max");
		if (hasDefault)
			field.Default["Default"] = intval.getZExtValue();
		else
			field.Default["Default"] = 0;
		break;
	case FieldType::Float32:
	case FieldType::Float64:
		if (parser.Has("min") && type == FieldType::Float32)
			field.Meta.MinFloat = parser.GetAs<float>("min");
		else if (parser.Has("min") && type == FieldType::Float64)
			field.Meta.MinFloat = parser.GetAs<double>("min");
		if (parser.Has("max") && type == FieldType::Float32)
			field.Meta.MaxFloat = parser.GetAs<float>("max");
		else if (parser.Has("max") && type == FieldType::Float64)
			field.Meta.MaxFloat = parser.GetAs<double>("max");
		if (hasDefault)
		{
			llvm::APFloat val(0.0);
			if (fieldrec->getInClassInitializer()->EvaluateAsFloat(val, fieldrec->getASTContext()))
				field.Default["Default"] = val.convertToDouble();
		}
		else
			field.Default["Default"] = 0.0;
		break;
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
		if (parser.Has("min"))
			field.Meta.MinFloat = parser.GetAs<float>("min");
		if (parser.Has("max"))
			field.Meta.MaxFloat = parser.GetAs<float>("max");
		if (hasDefault)
		{
			const clang::Expr* expr = fieldrec->getInClassInitializer();

			std::string buffer;
			llvm::raw_string_ostream os(buffer);
			expr->dump(os, fieldrec->getASTContext());

			std::vector<float> vec;
			size_t offset = 0;
			size_t index = 0;
			while ((index = buffer.find("-FloatingLiteral", index)) != std::string::npos)
			{
				size_t start = buffer.find("'float' ", index) + 8;
				size_t end = buffer.find('\n', index);
				const auto check = buffer.substr(start, end - start);
				vec.push_back(std::stof(check));
				offset++;
				index = end;
			}
			if (vec.size() == 1)
			{
				float val = vec[0];
				if (type == FieldType::Vec2) field.Default["Default"] = std::array<float, 2>{val, val};
				else if (type == FieldType::Vec3) field.Default["Default"] = std::array<float, 3>{val, val, val};
				else field.Default["Default"] = std::array<float, 4>{val, val, val, val};
			}
			else if (type == FieldType::Vec2 && vec.size() == 2)
				field.Default["Default"] = std::array<float, 2>{vec[0], vec[1]};
			else if (type == FieldType::Vec3 && vec.size() == 3)
				field.Default["Default"] = std::array<float, 3>{vec[0], vec[1], vec[2]};
			else if (vec.size() == 4)
				field.Default["Default"] = std::array<float, 4>{vec[0], vec[1], vec[2], vec[3]};
		}
		else
		{
			if (type == FieldType::Vec2) field.Default["Default"] = std::array<float, 2>{0.0f, 0.0f};
			else if (type == FieldType::Vec3) field.Default["Default"] = std::array<float, 3>{0.0f, 0.0f, 0.0f};
			else field.Default["Default"] = std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f};
		}
		break;
	case FieldType::String:
		if (parser.Has("length"))
			field.Meta.Length = parser.GetAs<size_t>("length");
		else
			field.Meta.Length = 15;
		if (hasDefault)
		{
			std::string buffer;
			llvm::raw_string_ostream os(buffer);
			fieldrec->getInClassInitializer()->dump(os, fieldrec->getASTContext());
			const size_t lineoffset = buffer.find("-StringLiteral");
			if (lineoffset != std::string::npos)
			{
				size_t start = buffer.find('"', lineoffset);
				size_t end = buffer.find_last_of('"');
				field.Default["Default"] = buffer.substr(start + 1, (end - start - 1));
			}
			else
				field.Default["Default"] = "";
		}
		else
			field.Default["Default"] = "";
		break;
	case FieldType::Bool:
		if (hasDefault)
		{
			const clang::Expr* expr = fieldrec->getInClassInitializer();
			bool value;
			expr->EvaluateAsBooleanCondition(value, fieldrec->getASTContext());
			field.Default["Default"] = value;
		}
		else
			field.Default["Default"] = false;
		break;
	default:
		break;
	}
}

void PostbuildFinder::FoundRecord(const clang::CXXRecordDecl* record) noexcept
{
	Finder::FoundRecord(record);

	const auto name = record->getDeclName().getAsString();
	auto& obj = Objects[name];
	
	//Add Parents' field
	for (const auto& baseclass : record->bases())
	{
		auto basename = baseclass.getType().getAsString();
		basename = basename.substr(basename.find(' ') + 1);
		const auto& fields = Objects[basename].Fields;
		for (const auto& field : fields)
			obj.Fields.push_back(field);
	}
}

void PostbuildFinder::FoundEnum(const clang::EnumDecl* enumdecl) noexcept
{
	Finder::FoundEnum(enumdecl);

	const auto name = enumdecl->getDeclName().getAsString();
	auto& enumaration = Enums[name];
	for (auto it = enumdecl->enumerator_begin(); it != enumdecl->enumerator_end(); ++it)
	{
		bool isUnsigned = it->getInitVal().isUnsigned();
		enumaration.Values.insert({ it->getNameAsString(), isUnsigned ? it->getInitVal().getZExtValue() : it->getInitVal().getExtValue() });
	}
}

void PrebuildFinder::FoundRecord(const clang::CXXRecordDecl* record) noexcept
{
	Finder::FoundRecord(record);

	const Object& obj = Objects[record->getDeclName().getAsString()];
	if (mHeaders.find(obj.Header) == mHeaders.end())
	{
		const auto& headerFile = obj.Header;
		auto include = headerFile.substr(headerFile.find("src") + 4);
		size_t index = include.find('\\');
		while (index != std::string::npos)
		{
			include.replace(index, 1, "/");
			index = include.find('\\', index + 1);
		}
		std::ofstream os(mProjectDir / "Exports.h", std::ios_base::app);
		os << "#include <" << include << ">\n";
		os.close();
		mHeaders.insert({ headerFile, true });
	}

	const auto& name = obj.Name;
	auto metaname = obj.Meta.Name;
	size_t index = metaname.find(' ');
	while (index != std::string::npos)
	{
		metaname.replace(index, 1, "_");
		index = metaname.find(' ', index + 1);
	}

	std::ofstream os(mProjectDir / "Exports.cpp", std::ios_base::app);
	if (obj.Meta.Type == ReflectionType::Component)
	{
		const std::string writename = !metaname.empty() ? metaname : name;
		os << "\tGAME_API void* Create" << writename <<
			"(Entity entity) " << " { return &entity.AddComponent<" << name << ">(); }\n";
		os << "\tGAME_API void* Get" << writename <<
			"(Entity entity) " << " { return &entity.GetComponent<" << name << ">(); }\n";
		os << "\tGAME_API bool Has" << writename << "(Entity entity) " << "{ return entity.HasComponent<" << name << ">(); }\n";
		os << "\tGAME_API void Remove" << writename << "(Entity entity) " << "{ entity.RemoveComponent<" << name << ">(); }\n\n";
	}
	else
	{
		os << "\tGAME_API " << (obj.Meta.Type == ReflectionType::System ? "System" : "ScriptableEntity") << "* Create" << (!metaname.empty() ? metaname : name) <<
			"(void) { return new " << name << "(); }\n\n";
	}
	os.close();
}

PrebuildFinder::PrebuildFinder(const char* filepath) noexcept
	: mProjectDir(filepath)
{
	auto test = mProjectDir.string();
	std::string prjname;
	if (test[test.size() - 1] == '\\' || test[test.size() - 1] == '/')
		prjname = test.substr(test.find_last_of("/\\", test.size() - 2) + 1);
	else
		prjname = test.substr(test.find_last_of("/\\") + 1);
	mProjectDir = (test + prjname);
	std::time_t result = std::time(nullptr);
	std::ofstream os(mProjectDir / "Exports.h");
	os << "// Auto generated by gtreflect.exe at " << std::asctime(std::localtime(&result)) <<
		"#pragma once\n\n";
	os.close();

	os.open(mProjectDir / "Exports.cpp");
	os << "// Auto generated by gtreflect.exe at " << std::asctime(std::localtime(&result));
	os << "#include \"Exports.h\"\n\n";
	os << "extern \"C\" {\n\n";
	os.close();
}

void PrebuildFinder::onEndOfTranslationUnit(void) noexcept
{
	std::ofstream os(mProjectDir / "Exports.cpp", std::ios_base::app);
	os << '}';
	os.close();
}

void Finder::FoundRecord(const clang::CXXRecordDecl* record) noexcept
{
	//Parse annotation
	const auto& annotation = get_annotation(record);
	AnnotationParser parser(annotation);
	const auto& header = parser.GetHeader();

	//Check and set Reflection Type
	ReflectionType type = ReflectionType::Unknown;
	if (header.compare("component") == 0)
		type = ReflectionType::Component;
	else if (header.compare("system") == 0)
		type = ReflectionType::System;
	else if (header.compare("class") == 0)
		type = ReflectionType::Object;
	else { GTR_ASSERT(false, "Reflection type for records should be component, system or class but '%s' was given.\n", header.c_str()); }

	//Retrieve name & size
	const auto name = record->getDeclName().getAsString();
	const size_t size = record->getASTContext().getTypeInfo(record->getTypeForDecl()).Width / 8;

	//Create object
	Object obj = { name, size, type };
	obj.Header = parser.Get("header");
	if (parser.Has("name"))
		obj.Meta.Name = parser.Get("name");
	Objects.insert({ name, obj });

	//Build offsets for every field
	//sOffsets.clear();
	//const auto* ptr = &record->getASTContext().getASTRecordLayout(record);
	//if (ptr->getFieldCount())
	//{
	//	size_t offset = ptr->getFieldOffset(0) / 8;
	//	const auto range = record->fields();
	//	for (auto fieldptr : range)
	//	{
	//		sOffsets.push_back(offset);
	//		offset += record->getASTContext().getTypeInfo(fieldptr->getType().getTypePtr()).Width / 8;
	//	}
	//}
}

void Finder::FoundField(const clang::FieldDecl* field) noexcept
{
	//Parse annotation
	const auto& annotation = get_annotation(field);
	AnnotationParser parser(annotation);
	const auto& header = parser.GetHeader();

	//Check Reflection Type & Field Type
	if (header.compare("property") != 0) { GTR_ASSERT(false, "Reflection Type for field should be property.\n"); }
	const FieldType type = gettype(field->getType().getCanonicalType());
	if (type == FieldType::Unknown) { GTR_ASSERT(false, "Type: %s cannot be reflected as field.\n", field->getType().getAsString().c_str()); }

	//Retrieve Name & size
	const auto name = field->getDeclName().getAsString();
	const size_t size = field->getASTContext().getTypeInfo(field->getType().getTypePtr()).Width / 8;
	const size_t offset = field->getASTContext().getFieldOffset(field) / 8;//sOffsets[field->getFieldIndex()];

	//Create field
	const auto owner = field->getParent()->getNameAsString();
	auto& fieldobj = Objects[owner].Fields.emplace_back(name, size, offset, type);
	const auto enum_t = field->getType().getCanonicalType().getAsString();
	switch (fieldobj.Meta.ValueType)
	{
	case FieldType::Enum_Char:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
		fieldobj.TypeName = enum_t.substr(5);
		break;
	case FieldType::String:
		fieldobj.TypeName = "string";
		break;
	case FieldType::Bool:
		fieldobj.TypeName = "bool";
		break;
	case FieldType::Char:
		fieldobj.TypeName = "char";
		break;
	case FieldType::Byte:
		fieldobj.TypeName = "byte";
		break;
	case FieldType::Int16:
		fieldobj.TypeName = "int16";
		break;
	case FieldType::Int32:
		fieldobj.TypeName = "int32";
		break;
	case FieldType::Int64:
		fieldobj.TypeName = "int64";
		break;
	case FieldType::Uint16:
		fieldobj.TypeName = "uint16";
		break;
	case FieldType::Uint32:
		fieldobj.TypeName = "uint32";
		break;
	case FieldType::Uint64:
		fieldobj.TypeName = "uint64";
		break;
	case FieldType::Float32:
		fieldobj.TypeName = "float32";
		break;
	case FieldType::Float64:
		fieldobj.TypeName = "float64";
		break;
	case FieldType::Vec2:
		fieldobj.TypeName = "vec2";
		break;
	case FieldType::Vec3:
		fieldobj.TypeName = "vec3";
		break;
	case FieldType::Vec4:
		fieldobj.TypeName = "vec4";
		break;
	case FieldType::Asset:
		fieldobj.TypeName = "asset";
		break;
	case FieldType::Entity:
		fieldobj.TypeName = "entity";
		break;
	}

	if (parser.Has("name"))
		fieldobj.Meta.Name = parser.Get("name");
}

void Finder::FoundEnum(const clang::EnumDecl* enumdecl) noexcept 
{
	//Parse annotation
	const auto& annotation = get_annotation(enumdecl);
	AnnotationParser parser(annotation);
	const auto& header = parser.GetHeader();

	//Check Reflection Type & Field Type
	if (header.compare("enum") != 0) { GTR_ASSERT(false, "Reflection Type for enum should be enum.\n"); }
	const auto name = enumdecl->getDeclName().getAsString();
	const size_t size = enumdecl->getASTContext().getTypeInfo(enumdecl->getIntegerType()).Width / 8;
	FieldType type = enumtype(enumdecl);
	Enum enumaration{ name, size, type };
	if (parser.Has("name"))
		enumaration.Meta.Name = parser.Get("name");
	Enums.insert({ name, enumaration });
}

void Finder::run(const clang::ast_matchers::MatchFinder::MatchResult& result) noexcept
{
	const auto* enumdecl = result.Nodes.getNodeAs<clang::EnumDecl>("id");
	if (enumdecl)
		return FoundEnum(enumdecl);

	const auto* record = result.Nodes.getNodeAs<clang::CXXRecordDecl>("id");
	if (record)
		return FoundRecord(record);

	const auto* field = result.Nodes.getNodeAs<clang::FieldDecl>("id");
	if (field)
		return FoundField(field);
}

[[nodiscard]] std::string Finder::get_annotation(const clang::Decl* decl) noexcept
{
	std::string annotation = "";
	llvm::raw_string_ostream os(annotation);
	auto policy = clang::PrintingPolicy(clang::LangOptions());

	decl->getAttr<clang::Attr>()->printPretty(os, policy);
	return annotation.substr(0, annotation.size() - 4).substr(annotation.find('"') + 1);
}

[[nodiscard]] FieldType Finder::gettype(const clang::QualType& type) noexcept
{
	const std::string strtype = type.getAsString();
	if (strtype.compare("bool") == 0 || strtype.compare("_Bool") == 0) return FieldType::Bool;
	else if (strtype.compare("char") == 0) return FieldType::Char;
	else if (strtype.compare("unsigned char") == 0) return FieldType::Byte;
	else if (strtype.compare("short") == 0) return FieldType::Int16;
	else if (strtype.compare("int") == 0) return FieldType::Int32;
	else if (strtype.compare("long long") == 0) return FieldType::Int64;
	else if (strtype.compare("unsigned short") == 0) return FieldType::Uint16;
	else if (strtype.compare("unsigned int") == 0) return FieldType::Uint32;
	else if (strtype.compare("unsigned long long") == 0) return FieldType::Uint64;
	else if (strtype.compare("float") == 0) return FieldType::Float32;
	else if (strtype.compare("double") == 0) return FieldType::Float64;
	else if (strtype.compare("class dumm::String") == 0) return FieldType::String;
	else if (strtype.compare("class std::shared_ptr<struct gte::Asset>") == 0) return FieldType::Asset;
	else if (strtype.compare("struct gte::Asset") == 0) { GTR_ASSERT(false, "Asset should be reflected as a Reference. Use Ref<gte::Asset> instead."); }
	else if (strtype.compare("class gte::Entity") == 0) return FieldType::Entity;
	else if (strtype.substr(0, 31).compare("struct glm::vec<2, float, glm::") == 0) return FieldType::Vec2;
	else if (strtype.substr(0, 31).compare("struct glm::vec<3, float, glm::") == 0) return FieldType::Vec3;
	else if (strtype.substr(0, 31).compare("struct glm::vec<4, float, glm::") == 0) return FieldType::Vec4;
	else if (strtype.substr(0, 4).compare("enum") == 0)
	{
		const auto enumname = strtype.substr(5);
		GTR_ASSERT(Enums.find(enumname) != Enums.end(), "When using an enumeration in an exported property the enumaration should also be exported.\n\tCheck the decleration of '%s'.\n", enumname.c_str());
		return Enums[enumname].Type;
	}
	else return FieldType::Unknown;
}

[[nodiscard]] FieldType Finder::enumtype(const clang::EnumDecl* decl) noexcept
{
	FieldType type = gettype(decl->getIntegerType().getCanonicalType());
	//Transform to enum type
	switch (type)
	{
	case FieldType::Enum_Char:
	case FieldType::Char:
		return FieldType::Enum_Char;
	case FieldType::Enum_Byte:
	case FieldType::Byte:
		return FieldType::Enum_Byte;
	case FieldType::Enum_Int16:
	case FieldType::Int16:
		return FieldType::Enum_Int16;
	case FieldType::Enum_Int32:
	case FieldType::Int32:
		return FieldType::Enum_Int32;
	case FieldType::Enum_Int64:
	case FieldType::Int64:
		return FieldType::Enum_Int64;
	case FieldType::Enum_Uint16:
	case FieldType::Uint16:
		return FieldType::Enum_Uint16;
	case FieldType::Enum_Uint32:
	case FieldType::Uint32:
		return FieldType::Enum_Uint32;
	case FieldType::Enum_Uint64:
	case FieldType::Uint64:
		return FieldType::Enum_Uint64;
		//Invalids
	case FieldType::Unknown:
	case FieldType::Bool:
	case FieldType::Float32:
	case FieldType::Float64:
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
	case FieldType::String:
		break;
	}
	return FieldType::Unknown;
}

[[nodiscard]] Object input_object(const YAML::Node& data) noexcept
{
	const auto name = data["Name"].as<std::string>();
	ReflectionType type = (ReflectionType)data["Type"].as<uint64_t>();
	Object obj(name, 1, type);
	obj.Version = data["Version"].as<uint64_t>();
	YAML::Node fields = data["Fields"];
	for (const auto& fielddata : fields)
	{
		const auto fname = fielddata["Name"].as<std::string>();
		size_t size = fielddata["Size"].as<size_t>();
		size_t offset = fielddata["Offset"].as<size_t>();
		FieldType ftype = (FieldType)fielddata["Type"].as<uint64_t>();
		Field& field = obj.Fields.emplace_back(fname, size, offset, ftype);
		input_metadata(fielddata, field.Meta, ftype);
	}
	return obj;
}

void input_metadata(const YAML::Node& data, FieldMetadata& meta, FieldType type) noexcept
{
	switch (type)
	{
	case FieldType::Char:
	case FieldType::Int16:
	case FieldType::Int32:
	case FieldType::Int64:
	case FieldType::Enum_Char:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
		meta.MinInt = data["min"].as<int64_t>();
		meta.MaxInt = data["max"].as<int64_t>();
		break;
	case FieldType::Byte:
	case FieldType::Uint16:
	case FieldType::Uint32:
	case FieldType::Uint64:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
		meta.MinUint = data["min"].as<uint64_t>();
		meta.MaxUint = data["max"].as<uint64_t>();
		break;
	case FieldType::Float32:
	case FieldType::Float64:
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
		meta.MinFloat = data["min"].as<double>();
		meta.MaxFloat = data["max"].as<double>();
		break;
	case FieldType::String:
		meta.Length = data["length"].as<size_t>();
		break;
	default:
		break;
	}
}

void PostbuildFinder::output_object(std::ofstream& os, const Object& obj) noexcept
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Name" << YAML::Value << obj.Meta.Name;
	out << YAML::Key << "Header" << YAML::Value << obj.Header;
	out << YAML::Key << "Version" << YAML::Value << obj.Version;
	out << YAML::Key << "Type" << YAML::Value << (uint64_t)obj.Meta.Type;
	out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;
	for (const auto& field : obj.Fields)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << field.Meta.Name;
		out << YAML::Key << "Type" << YAML::Value << (uint64_t)field.Meta.ValueType;
		if (field.isEnum())
		{
			std::string Typename = Enums.at(field.TypeName).Meta.Name;
			out << YAML::Key << "TypeName" << YAML::Value << Typename;
		}
		out << YAML::Key << "Offset" << YAML::Value << field.Offset;
		out << YAML::Key << "Size" << YAML::Value << field.Meta.Size;
		output_metadata(out, field.Meta, field.Default);
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;

	std::string buff(out.c_str());
	os << buff.size() << '\n' << '\n';
	os << buff;
	os.close();
}

void output_metadata(YAML::Emitter& out, const FieldMetadata& data, const YAML::Node& Default)
{
	switch (data.ValueType)
	{
	case FieldType::Char:
	case FieldType::Int16:
	case FieldType::Int32:
	case FieldType::Int64:
	case FieldType::Enum_Char:
	case FieldType::Enum_Int16:
	case FieldType::Enum_Int32:
	case FieldType::Enum_Int64:
		out << YAML::Key << "min" << YAML::Value << data.MinInt;
		out << YAML::Key << "max" << YAML::Value << data.MaxInt;
		out << YAML::Key << "Default" << YAML::Value << Default["Default"];
		break;
	case FieldType::Byte:
	case FieldType::Uint16:
	case FieldType::Uint32:
	case FieldType::Uint64:
	case FieldType::Enum_Byte:
	case FieldType::Enum_Uint16:
	case FieldType::Enum_Uint32:
	case FieldType::Enum_Uint64:
		out << YAML::Key << "min" << YAML::Value << data.MinUint;
		out << YAML::Key << "max" << YAML::Value << data.MaxUint;
		out << YAML::Key << "Default" << YAML::Value << Default["Default"];
		break;
	case FieldType::Float32:
	case FieldType::Float64:
	case FieldType::Vec2:
	case FieldType::Vec3:
	case FieldType::Vec4:
		out << YAML::Key << "min" << YAML::Value << data.MinFloat;
		out << YAML::Key << "max" << YAML::Value << data.MaxFloat;
		out << YAML::Key << "Default" << YAML::Value << Default["Default"];
		break;
	case FieldType::String:
		out << YAML::Key << "length" << YAML::Value << data.Length;
		out << YAML::Key << "Default" << YAML::Value << Default["Default"];
		break;
	case FieldType::Bool:
		out << YAML::Key << "Default" << YAML::Value << Default["Default"];
		break;
	default:
		break;
	}
}

void PostbuildFinder::onEndOfTranslationUnit(void) noexcept
{
	WriteEnums();
	WriteObjects();
}