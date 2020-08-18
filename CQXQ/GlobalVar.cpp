#include "GlobalVar.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <map>

HMODULE hDllModule;

// 所有插件
std::vector<native_plugin> plugins;

// 排序后的所有插件事件
std::map<int, std::vector<eventType>> plugins_events;

// XQ根目录, 结尾不带斜杠
std::string rootPath;

// 启用事件是否已经被调用，用于在QQ登陆成功以后再调用启用事件
bool EnabledEventCalled = false;

// 是否接收来自自己的事件
bool RecvSelfEvent = false;