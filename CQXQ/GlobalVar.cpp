#include "GlobalVar.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include <atomic>
#include "ctpl_stl.h"
#include "GUI.h"

HMODULE hDllModule;

// 所有插件
std::map<int, native_plugin> plugins;

// 排序后的所有插件事件
std::map<int, std::vector<eventType>> plugins_events;

// 下一个插件的ID
int nextPluginId = 1;

// XQ根目录, 结尾不带斜杠
std::string rootPath;

// 启用事件是否已经被调用，用于在QQ登陆成功以后再调用启用事件
bool EnabledEventCalled = false;

// 是否接收来自自己的事件
bool RecvSelfEvent = false;

// 是否在运行
std::atomic<bool> running = false;

// 伪主线程
ctpl::thread_pool fakeMainThread(1);

// API调用线程
ctpl::thread_pool p(4);

std::atomic<long long> robotQQ;

unsigned char* AuthCode = nullptr;