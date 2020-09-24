#include <filesystem>
#include <Windows.h>
#include <sstream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "EncodingConvert.h"
#include "XQAPI.h"
#include "GlobalVar.h"
#include "native.h"
#include "GUI.h"

// 按照优先级排序
void sortEvents()
{
	for (auto& ele : plugins_events)
	{
		std::sort(ele.second.begin(), ele.second.end());
	}
}

// plugins_event Not Sorted after calling this function
int loadCQPlugin(const std::filesystem::path& file, int id = -1)
{
	std::string ppath = rootPath + "\\CQPlugins\\";
	std::string newFile = ppath + "tmp\\" + std::to_string(time(nullptr)) + "_" + file.filename().string();
	std::error_code ec;
	filesystem::copy_file(file, newFile, ec);
	if (ec)
	{
		XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！无法复制DLL文件!").c_str());
		return ec.value();
	}
	native_plugin plugin = { (id == -1) ? nextPluginId++ : id, file.filename().string(), newFile };
	HMODULE dll;
	if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
	{
		dll = LoadLibraryA(newFile.c_str());
	}
	else
	{
		dll = fakeMainThread.push([&newFile](int) {return LoadLibraryA(newFile.c_str()); }).get();
	}
	 
	if (!dll)
	{
		int err = GetLastError();
		XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！LoadLibrary错误代码：" + std::to_string(err)).c_str());
		return err;
	}
	// 读取Json
	auto fileCopy = file;
	fileCopy.replace_extension(".json");
	ifstream jsonstream(fileCopy);
	FARPROC init = nullptr;
	if (jsonstream)
	{
		try
		{
			stringstream jStrStream;
			jStrStream << jsonstream.rdbuf();
			std::string jsonStr = jStrStream.str();
			nlohmann::json j;
			try
			{
				j = nlohmann::json::parse(jsonStr, nullptr, true, true);
			}
			catch (std::exception&)
			{
				j = nlohmann::json::parse(GB18030toUTF8(jsonStr), nullptr, true, true);
			}

			plugin.name = UTF8toGB18030(j["name"].get<std::string>());
			plugin.version = UTF8toGB18030(j["version"].get<std::string>());
			j["version_id"].get_to(plugin.version_id);
			plugin.author = UTF8toGB18030(j["author"].get<std::string>());
			plugin.description = UTF8toGB18030(j["description"].get<std::string>());
			for (const auto& it : j["event"])
			{
				int type = it["type"].get<int>();
				int priority = it.count("priority") ? it["priority"].get<int>() : 30000;
				FARPROC procAddress = nullptr;
				if (it.count("function"))
				{
					procAddress = GetProcAddress(dll, UTF8toGB18030(it["function"].get<std::string>()).c_str());
				}
				else if (it.count("offset"))
				{
					procAddress = FARPROC((BYTE*)dll + it["offset"].get<int>());
				}

				if (procAddress)
				{
					auto e = eventType{ plugin.id, priority, procAddress };
					plugin.events[type] = e;
					plugins_events[type].push_back(e);
				}
				else
				{
					XQAPI::OutPutLog(("加载" + file.filename().string() + "的事件类型" + std::to_string(type) + "时失败! 请检查json文件是否正确!").c_str());
				}
			}
			for (const auto& it : j["menu"])
			{
				FARPROC procAddress = nullptr;
				if (it.count("function"))
				{
					procAddress = GetProcAddress(dll, UTF8toGB18030(it["function"].get<std::string>()).c_str());
				}
				else if (it.count("offset"))
				{
					procAddress = FARPROC((BYTE*)dll + it["offset"].get<int>());
				}
				if (procAddress)
				{
					plugin.menus.push_back({ UTF8toGB18030(it["name"].get<std::string>()), procAddress });
				}
				else
				{
					XQAPI::OutPutLog(("加载" + file.filename().string() + "的菜单" + UTF8toGB18030(it["name"].get<std::string>()) + "时失败! 请检查json文件是否正确!").c_str());
				}

			}
			if (j.count("init_offset")) init = FARPROC((BYTE*)dll + j["init_offset"].get<int>());
		}
		catch (std::exception& e)
		{
			XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！Json文件读取失败! " + e.what()).c_str());
			if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
			{
				FreeLibrary(dll);
			}
			else
			{
				fakeMainThread.push([&dll](int) {FreeLibrary(dll); }).wait();
			}
			return 0;
		}
	}
	else
	{
		XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！无法打开Json文件!").c_str());
		if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
		{
			FreeLibrary(dll);
		}
		else
		{
			fakeMainThread.push([&dll](int) {FreeLibrary(dll); }).wait();
		}
		return 0;
	}
	const auto initFunc = FuncInitialize(init ? init : GetProcAddress(dll, "Initialize"));
	if (initFunc)
	{
		initFunc(plugin.id);
	}
	else
	{
		XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！无公开的Initialize函数!").c_str());
		if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
		{
			FreeLibrary(dll);
		}
		else
		{
			fakeMainThread.push([&dll](int) {FreeLibrary(dll); }).wait();
		}
		return 0;
	}

	// 判断是否启用
	fileCopy.replace_extension(".disable");
	if (std::filesystem::exists(fileCopy))
	{
		plugin.enabled = false;
	}
	plugin.dll = dll;
	plugins.insert({ plugin.id, plugin });
	XQAPI::OutPutLog(("加载"s + file.filename().string() + "成功！").c_str());
	if (plugin.events.count(CQ_eventStartup))
	{
		const auto startup = IntMethod(plugin.events[CQ_eventStartup].event);
		if (startup)
		{
			if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
			{
				ExceptionWrapper(startup)();
			}
			else
			{
				fakeMainThread.push([startup](int) { ExceptionWrapper(startup)(); }).wait();
			}
		}
	}
	if (EnabledEventCalled && plugin.enabled && plugin.events.count(CQ_eventEnable))
	{
		const auto enable = IntMethod(plugin.events[CQ_eventEnable].event);
		if (enable)
		{
			if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
			{
				ExceptionWrapper(enable)();
			}
			else
			{
				fakeMainThread.push([enable](int) { ExceptionWrapper(enable)(); }).wait();
			}
		}
	}
	return 0;
}

void loadOneCQPlugin(const std::filesystem::path& file, int id = -1)
{
	loadCQPlugin(file, id);
	sortEvents();
	UpdateMainWindow();
}

void unloadOneCQPlugin(int id)
{
	if (plugins[id].events.count(CQ_eventExit))
	{
		const auto exit = IntMethod(plugins[id].events[CQ_eventExit].event);
		if (exit)
		{
			if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
			{
				ExceptionWrapper(exit)();
			}
			else
			{
				fakeMainThread.push([exit](int) { ExceptionWrapper(exit)(); }).wait();
			}
		}
	}
	for (auto& event : plugins[id].events)
	{
		plugins_events[event.first].erase(std::find(plugins_events[event.first].begin(), plugins_events[event.first].end(), event.second));
	}
	if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
	{
		FreeLibrary(plugins[id].dll);
	}
	else
	{
		fakeMainThread.push([&dll = plugins[id].dll](int) {FreeLibrary(dll); }).wait();
	}
	
	std::error_code ec;  
	filesystem::remove(plugins[id].newFile, ec);

	plugins.erase(id);
	UpdateMainWindow();
}

void reloadOneCQPlugin(int id)
{
	const string ppath = rootPath + "\\CQPlugins\\" + plugins[id].file;
	unloadOneCQPlugin(id);
	loadOneCQPlugin(ppath, id);
}

void loadAllCQPlugin()
{
	std::string ppath = rootPath + "\\CQPlugins\\";
	std::filesystem::create_directories(ppath);
	for (const auto& file : std::filesystem::directory_iterator(ppath))
	{
		if (file.is_regular_file() && file.path().extension() == ".dll")
		{
			loadCQPlugin(file);
		}
	}
	sortEvents();
	UpdateMainWindow();
}

void unloadAllCQPlugin()
{
	plugins_events.clear();
	for (auto& plugin : plugins)
	{
		if (plugin.second.events.count(CQ_eventExit))
		{
			const auto exit = IntMethod(plugin.second.events[CQ_eventExit].event);
			if (exit)
			{
				if (this_thread::get_id() == fakeMainThread.get_thread(0).get_id())
				{
					ExceptionWrapper(exit)();
				}
				else
				{
					fakeMainThread.push([exit](int) { ExceptionWrapper(exit)(); }).wait();
				}
			}
		}
		FreeLibrary(plugin.second.dll);
		std::error_code ec;
		filesystem::remove(plugin.second.newFile, ec);
	}
	plugins.clear();
	nextPluginId = 1;
	UpdateMainWindow();
}

void reloadAllCQPlugin()
{
	unloadAllCQPlugin();
	loadAllCQPlugin();
}