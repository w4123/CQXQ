#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <atomic>
#include <map>
#include "ctpl_stl.h"

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
	bool operator==(const eventType& that) const
	{
		return this->event == that.event;
	}
};

struct native_plugin
{
	int id = -1;
	std::string file;
	std::string newFile;
	std::string name;
	std::string version;
	int version_id = -1;
	std::string author;
	std::string description;
	std::map<int, eventType> events;
	std::vector<std::pair<std::string, FARPROC>> menus;
	HMODULE dll = nullptr;
	bool enabled = false;

	native_plugin(int i, const std::string& f, const std::string& nf)
	{
		id = i;
		file = f;
		newFile = nf;
		dll = nullptr;
		enabled = true;
	}

	native_plugin() = default;
	~native_plugin() = default;
};

// 存储所有插件
extern std::map<int, native_plugin> plugins;

// 存储排序后的所有插件事件
extern std::map<int, std::vector<eventType>> plugins_events;

// 下一个插件的id
extern int nextPluginId;

// XQ根目录, 结尾不带斜杠
extern std::string rootPath;

// 启用事件是否已经被调用，用于在QQ登陆成功以后再调用启用事件
extern bool EnabledEventCalled;

// 是否接收来自自己的事件
extern bool RecvSelfEvent;

// 是否在运行
extern std::atomic<bool> running;

// 伪主线程
extern ctpl::thread_pool fakeMainThread;

// API调用线程
extern ctpl::thread_pool p;

extern std::atomic<long long> robotQQ;

extern unsigned char* AuthCode;