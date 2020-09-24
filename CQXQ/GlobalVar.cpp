#include "GlobalVar.h"
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

// 总内存释放线程
std::unique_ptr<std::thread> memFreeThread;

// 用于释放字符串内存
std::priority_queue<std::pair<std::time_t, void*>> memFreeQueue;

std::mutex memFreeMutex;

// 消息ID以及消息ID内存释放
std::priority_queue<std::pair<std::time_t, size_t>> memFreeMsgIdQueue;

std::mutex memFreeMsgIdMutex;

std::map<size_t, FakeMsgId> msgIdMap;

std::atomic<size_t> msgIdMapId = 1;

size_t newMsgId(const FakeMsgId& msgId)
{
	size_t id = msgIdMapId++;
	std::unique_lock lock(memFreeMsgIdMutex);
	msgIdMap[id] = msgId;
	memFreeMsgIdQueue.push(std::make_pair(time(nullptr), id));
	return id;
}

// 是否已经初始化完毕
std::atomic<bool> Init = false;

// 复制字符串, 返回复制后的字符串指针，字符串内存5分钟后释放
const char* delayMemFreeCStr(const std::string& str)
{
	const char* s = _strdup(str.c_str());
	{
		std::unique_lock lock(memFreeMutex);
		memFreeQueue.push({ time(nullptr), (void*)s });
	}
	return s;
}

std::atomic<long long> robotQQ;

unsigned char* AuthCode = nullptr;