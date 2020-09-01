#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <atomic>
#include <map>

extern HMODULE hDllModule;

struct eventType
{
	int plugin_id = -1;
	int priority = 30000;
	FARPROC event = nullptr;
	bool operator<(const eventType& that) const
	{
		return this->priority < that.priority;
	}
};

struct native_plugin
{
	int id;
	std::string file;
	std::string name;
	std::string version;
	int version_id;
	std::string author;
	std::string description;
	std::map<int, eventType> events;
	std::vector<std::pair<std::string, FARPROC>> menus;
	HMODULE dll;
	bool enabled;

	native_plugin(int i, const std::string& f)
	{
		id = i;
		file = f;
		dll = nullptr;
		enabled = true;
	}
};

// 存储所有插件
extern std::vector<native_plugin> plugins;

// 存储排序后的所有插件事件
extern std::map<int, std::vector<eventType>> plugins_events;

// XQ根目录, 结尾不带斜杠
extern std::string rootPath;

// 启用事件是否已经被调用，用于在QQ登陆成功以后再调用启用事件
extern bool EnabledEventCalled;

// 是否接收来自自己的事件
extern bool RecvSelfEvent;

// 是否在运行
extern std::atomic<bool> running;
