#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <io.h>

using namespace std;

struct ProtobufFilePackage
{
	ProtobufFilePackage(const string& _basedir, const string& _infile, const string& _outdir, const string& _outfilerelativepath)
		:basedir(_basedir), infilepath(_infile), outdir(_outdir), outfilerelativepath(_outfilerelativepath) { }
	// protobuf base dir
	string basedir;
	string infilepath;
	string outdir;
	string outfilerelativepath;
};
std::vector<ProtobufFilePackage> gPackages;

struct ProtobufCommand
{
	string protoPath = "proto";
	string baseReadDir = "./";
	string baseOutDir = "./";
	string fullHeader;
	string fullBody;
	string changeExt;
	string outType = "cpp_out";
	string PCHFile = "";

	int bodySplitSize = 0; // 0代表无限
	bool enableFullHeader = false;
	bool enableFullBody = false;
	bool enableChangeExt = false;
} gProtoBufCommand;

static const string ProtoFileExt = ".proto";

typedef function<void(const string&, const string&)> CmdHandler;
extern unordered_map<string, CmdHandler> gCmdHandler;

extern bool parseArguments(int argc, char **argv);
extern bool runCommand();
extern string changeFileExt(const string& resfile, const char* ext);

int main(int argc, char **argv)
{
	if (!parseArguments(argc, argv))
		return -1;

	if (!runCommand())
		return -2;
//	atexit([] {	system("pause"); });
	return 0;
}

string tolower(const string& s)
{
	string ret = s;
	for (int i = (int)ret.length() - 1; i > -1; --i)
	{
		ret[i] = (char)tolower(s[i]);
	}
	return ret;
}

string toupper(const string& s)
{
	string ret = s;
	for (int i = (int)ret.length() - 1; i > -1; --i)
	{
		ret[i] = (char)toupper(s[i]);
	}
	return ret;
}

string& replace_all(string& str, const string& old_value, const string& new_value)
{
	while (true) {
		string::size_type pos(0);
		if ((pos = str.find(old_value)) != string::npos)
			str.replace(pos, old_value.length(), new_value);
		else break;
	}
	return str;
}

#if defined(_MSC_VER)
static const char DirSpliter = '\\';
#else
static const char DirSpliter = '/';
#endif

string replace_separator(const string& path)
{
	string s = path;
#if defined(_MSC_VER)
	replace_all(s, "/", "\\");
	replace_all(s, "\\\\", "\\");
#else
	replace_all(s, "\\", "/");
	replace_all(s, "//", "/");
#endif
	return s;
}

//深度优先递归遍历当前目录下文件夹和文件及子文件夹和文件
void dfsFolder(const string& path, const string& fileExt, function<void(const string&)> callFunc)
{
	_finddata_t file_info;
	string current_path = path + DirSpliter + "*.*"; //也可以用/*来匹配所有
	current_path = replace_separator(current_path);
	int handle = (int)_findfirst(current_path.c_str(), &file_info);
	//返回值为-1则查找失败
	if (-1 == handle)
	{
		cerr << "cannot match the path" << endl;
		return;
	}

	do
	{
		//判断是否子目录
		if (file_info.attrib == _A_SUBDIR)
		{
			//递归遍历子目录
			if (strcmp(file_info.name, "..") != 0 && strcmp(file_info.name, ".") != 0)//.是当前目录，..是上层目录，必须排除掉这两种情况
				dfsFolder(path + '/' + file_info.name, fileExt, callFunc); //再windows下可以用\\转义分隔符，不推荐
		}
		else
		{
			string filename = file_info.name;
			if (filename.length() < fileExt.length()) continue;
			if (filename.substr(filename.length()-fileExt.length()) != fileExt) continue;
			string filepath = path + "/" + filename;
			callFunc(replace_separator(filepath));
		}
	} while (!_findnext(handle, &file_info));//返回0则遍历完
											 //关闭文件句柄
	_findclose(handle);
}

bool parseArguments(int argc, char **argv)
{
	bool cmdMode = true;
	string sOriginCmd;
	string sCmd;
	string sParam;
	for (int i = 1; i < argc; ++i)
	{
		if (cmdMode)
		{
			string argCmd = argv[i];
			if (argCmd.length() <= 2 || (argCmd[0] != '-' && argCmd[0] != '/'))
			{
				cerr << "arg error - " << argCmd << endl;
			}
			sCmd = argCmd.substr(1);
			sOriginCmd = argCmd;
			cmdMode = false;
		}
		else
		{
			std::string argParams = argv[i];
			sParam = argParams;
			cmdMode = true;
			auto iter = gCmdHandler.find(tolower(sCmd));
			if (iter == gCmdHandler.end())
			{
				cerr << "error command - " << sOriginCmd << " " << sParam << endl;
				continue;
			}
			iter->second(sCmd, sParam);
		}
	}
	return true;
}

string changeFileExt(const string& resfile, const char* ext)
{
	auto dotPos = resfile.find_last_of('.');
	if (dotPos == string::npos)
	{
		return dotPos + "." + string(ext);
	}
	return resfile.substr(0, dotPos + 1) + ext;
}

bool runCommand()
{
	dfsFolder(gProtoBufCommand.baseReadDir, ProtoFileExt, [](const string& file)->void{
		string outfilewithoutext = file.substr(gProtoBufCommand.baseReadDir.length());
		replace_all(outfilewithoutext, "\\", "/");
		outfilewithoutext = outfilewithoutext.substr(outfilewithoutext[0] == '/' ? 1 : 0, outfilewithoutext.length() - ProtoFileExt.length());
		gPackages.push_back(ProtobufFilePackage(
				gProtoBufCommand.baseReadDir, file,
				gProtoBufCommand.baseOutDir, outfilewithoutext));
		cerr << "file - " << file << "outfile " << outfilewithoutext << endl;
	});

	for (int i = 0; i < (int)gPackages.size(); ++i)
	{
		const auto& package = gPackages[i];
		string outfile;
		if (gProtoBufCommand.outType == "cpp_out")
		{
			outfile = package.outdir;
		}
		else
		{
			outfile = package.outdir + DirSpliter + package.infilepath.substr(package.basedir.length());
			if (gProtoBufCommand.enableChangeExt)
			{
				outfile = changeFileExt(outfile, gProtoBufCommand.changeExt.c_str());
			}
		}
		string cmd = "CALL " + gProtoBufCommand.protoPath + " --proto_path " + package.basedir + " " + " --" + gProtoBufCommand.outType + " " + outfile + " " + package.infilepath;
// 		string cmd = "CALL " + gProtoBufCommand.protoPath + " --proto_path " + package.basedir + " " + " --descriptor_set_out " + outfile + " " + package.infilepath;
// 		string cmd = "CALL " + gProtoBufCommand.protoPath + " --proto_path " + package.basedir + " " + " --cpp_out " + package.outdir + " " + package.infilepath;

		auto cmdRet = system(cmd.c_str());
		if (cmdRet != 0)
		{
			cerr << "proto command run failure - " << cmd << endl;
			return false;
		}
	}

	auto get_extension = [](const string& file)-> string {
		auto filepos = file.find_last_of(DirSpliter);
		string filename = filepos == string::npos ? file : file.substr(filepos + 1);
		const auto pos = filename.find_last_of('.');
		string fileext = pos == string::npos ? "" : filename.substr(pos);
		return fileext;
	};

	if (gProtoBufCommand.enableFullHeader)
	{
		string outfile = gProtoBufCommand.baseOutDir + DirSpliter + gProtoBufCommand.fullHeader;
		if (get_extension(outfile) == "")
		{
			outfile += ".h";
		}
		ofstream location_out;
		location_out.open(outfile, std::ios::out | std::ios::trunc);
		if (!location_out.is_open())
		{
			cerr << "create body file failure - " + outfile << endl;
		}
		else
		{
			for (int i = 0; i < (int)gPackages.size(); ++i)
			{
				const auto& package = gPackages[i];
				location_out << "#include \"" << package.outfilerelativepath << "pb.h\"" << endl;
			}
			cout << "generate full header files success - " << outfile << endl;
		}
	}

	if (gProtoBufCommand.enableFullBody)
	{
		ofstream location_out;
		string bodyfileext = ".cc";
		int linecount = 0;
		int fileindex = 0;
		string outfile = gProtoBufCommand.baseOutDir + DirSpliter + gProtoBufCommand.fullBody;
		if (get_extension(outfile) != "")
		{
			bodyfileext = get_extension(outfile);
			outfile = outfile.substr(0, outfile.length() - bodyfileext.length());
		}
		for (int i = 0; i < (int)gPackages.size(); ++i)
		{
 			if (i == 0 || (gProtoBufCommand.bodySplitSize != 0 && linecount >= gProtoBufCommand.bodySplitSize))
			{
				if (location_out.is_open()) { location_out.flush(); location_out.close(); }
				string nextfile = outfile + (fileindex == 0 ? "" : to_string(fileindex)) + bodyfileext;
				location_out.open(nextfile, std::ios::out | std::ios::trunc);
				if (!location_out.is_open())
				{
					cerr << "create body file failure - " + nextfile << endl;
					break;
				}
				cout << "generate full body file success - " << nextfile << endl;
				linecount = 0; ++fileindex;
				if (gProtoBufCommand.PCHFile != "")
				{
					location_out << "#include \"" << gProtoBufCommand.PCHFile << "\"" << endl;
				}
			}

			const auto& package = gPackages[i];
			location_out << "#include \"" << package.outfilerelativepath << "pb.cc\"" << endl;
			++linecount;
		}
	}

	return true;
}

unordered_map<string, CmdHandler> gCmdHandler = {
	{ "protoexe",				[](const string& cmd, const string& param) -> void { gProtoBufCommand.protoPath = replace_separator(param); } },
	{ "indir",					[](const string& cmd, const string& param) -> void { gProtoBufCommand.baseReadDir = replace_separator(param); } },
	{ "outdir",					[](const string& cmd, const string& param) -> void { gProtoBufCommand.baseOutDir = replace_separator(param); } },
	{ "fullheader",				[](const string& cmd, const string& param) -> void { gProtoBufCommand.fullHeader = replace_separator(param); gProtoBufCommand.enableFullHeader = true; } },
	{ "fullbody",				[](const string& cmd, const string& param) -> void { gProtoBufCommand.fullBody = replace_separator(param); gProtoBufCommand.enableFullBody = true; } },
	{ "fullbodysplit",			[](const string& cmd, const string& param) -> void { gProtoBufCommand.bodySplitSize = atoi(param.c_str()); } },
	{ "outext",					[](const string& cmd, const string& param) -> void { gProtoBufCommand.changeExt = param; gProtoBufCommand.enableChangeExt = true; } },
	{ "outtype",				[](const string& cmd, const string& param) -> void { gProtoBufCommand.outType = param; } },
	{ "pch_file",				[](const string& cmd, const string& param) -> void { gProtoBufCommand.PCHFile = param; } },
};
