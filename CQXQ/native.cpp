#include <vector>
#include <string>
#include <Windows.h>
#include <thread>
#include <future>
#include <filesystem>
#include <fstream>
#include <set>
#include <exception>
#include <stdexcept>
#include "GlobalVar.h"
#include "native.h"
#include "CQTools.h"
#include "json.hpp"
#include "ctpl_stl.h"
#include "Unpack.h"
#include "GlobalVar.h"
#include "GUI.h"
#include "resource.h"
#include "EncodingConvert.h"

using namespace std;

#define XQ

std::string robotQQ;

ctpl::thread_pool p(4);

// 用于释放内存
std::priority_queue<std::pair<std::time_t, const char*>> memFreeQueue;

std::mutex memFreeMutex;

std::unique_ptr<std::thread> memFreeThread;

std::atomic<bool> memFreeThreadShouldRun = false;

// 复制字符串, 返回复制后的字符串指针，字符串内存5分钟后释放
const char* delayMemFreeCStr(const std::string& str)
{
	const char* s = _strdup(str.c_str());
	{
		std::unique_lock lock(memFreeMutex);
		memFreeQueue.push({ time(nullptr), s });
	}
	return s;
}

namespace XQAPI
{
	static vector<function<void(HMODULE)>> apiFuncInitializers;

	static bool addFuncInit(const function<void(HMODULE)>& initializer) {
		apiFuncInitializers.push_back(initializer);
		return true;
	}

	void initFuncs(const HMODULE& hModule)
	{
		try
		{
			for (const auto& initializer : apiFuncInitializers) {
				initializer(hModule);
			}
		}
		catch (std::runtime_error& e)
		{
			MessageBoxA(nullptr, ("CQXQ初始化API致命错误，请检查你的先驱版本！程序即将退出: "s + e.what()).c_str(), "CQXQ错误", MB_OK);
			exit(-1);
		}
	}

#define XQAPI(Name, ReturnType, ...) using Name##_FUNC = std::function<ReturnType (__stdcall)(__VA_ARGS__)>; \
	Name##_FUNC _##Name; \
	using Name##_TYPE = ReturnType (__stdcall*)(__VA_ARGS__); \
	template <typename... Args> \
	ReturnType Name(Args&&... args) \
	{ \
		return p.push([&args...](int iThread){ return _##Name(std::forward<Args>(args)...); }).get(); \
	} \
    static bool _init_##Name = addFuncInit( [] (const auto& hModule) { \
        _##Name = reinterpret_cast<Name##_TYPE>(GetProcAddress(hModule, "Api_" #Name)); \
        if (!_##Name) throw std::runtime_error("Unable to initialize API Function " #Name); \
    });

	XQAPI(SendMsg, void, const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId)

	XQAPI(OutPutLog, void, const char* content)

	XQAPI(GetNick, const char*, const char* botQQ, const char* QQ)

	XQAPI(GetGroupAdmin, const char*, const char* botQQ, const char* groupId)

	XQAPI(GetGroupCard, const char*, const char* botQQ, const char* groupId, const char* QQ)

	XQAPI(GetGroupList, const char*, const char* botQQ)

	XQAPI(GetGroupList_B, const char*, const char* botQQ)

	XQAPI(GetGroupName, const char*, const char* botQQ, const char* groupId)

	XQAPI(GetFriendList_B, const char*, const char* botQQ)

	XQAPI(GetFriendsRemark, const char*, const char* botQQ, const char* QQ)

	XQAPI(GetGroupMemberList, const char*, const char* botQQ, const char* groupId)

	XQAPI(GetGroupMemberList_B, const char*, const char* botQQ, const char* groupId)

	XQAPI(GetGroupMemberList_C, const char*, const char* botQQ, const char* groupId)

	XQAPI(HandleGroupEvent, void, const char* botQQ, int32_t reqType, const char* QQ, const char* groupId, const char* seq, int32_t rspType, const char* msg)

	XQAPI(HandleFriendEvent, void, const char* botQQ, const char* QQ, int32_t rspType, const char* msg)

	XQAPI(QuitGroup, void, const char* botQQ, const char* groupId)

	XQAPI(GetGroupMemberNum, const char*, const char* botQQ, const char* groupId)

	XQAPI(SetAnon, BOOL, const char* botQQ, const char* groupId, BOOL enable)

	XQAPI(ShutUP, void, const char* botQQ, const char* groupId, const char* QQ, int32_t duration)

	XQAPI(SetGroupCard, BOOL, const char* botQQ, const char* groupId, const char* QQ, const char* card)

	XQAPI(KickGroupMBR, void, const char* botQQ, const char* groupId, const char* QQ, BOOL refuseForever)

	XQAPI(UpVote, const char*, const char* botQQ, const char* QQ)

	XQAPI(IsEnable, BOOL)

	XQAPI(GetOnLineList, const char*)

	XQAPI(UpLoadPic, const char*, const char* botQQ, int32_t uploadType, const char* targetId, const unsigned char* image)

	XQAPI(IfFriend, BOOL, const char* botQQ, const char* QQ)

	XQAPI(GetCookies, const char*, const char* botQQ)

	XQAPI(GetBkn, const char*, const char* botQQ)

	XQAPI(GetVoiLink, const char*, const char* botQQ, const char* GUID)
#undef XQAPI
}

HMODULE XQHModule = nullptr;
HMODULE CQPHModule = nullptr;

int loadCQPlugin(const std::filesystem::path& file)
{
	std::string ppath = rootPath + "\\CQPlugins\\";
	native_plugin plugin = { static_cast<int>(plugins.size()), file.filename().string() };
	const auto dll = LoadLibraryA(file.string().c_str());
	if (!dll)
	{
		int err = GetLastError();
		XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！LoadLibrary错误代码：" + std::to_string(err)).c_str());
		FreeLibrary(dll);
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
			nlohmann::json j = nlohmann::json::parse(jsonstream, nullptr, true, true);
			plugin.name = UTF8toGB18030(j["name"].get<std::string>());
			plugin.version = UTF8toGB18030(j["version"].get<std::string>());
			j["version_id"].get_to(plugin.version_id);
			plugin.author = UTF8toGB18030(j["author"].get<std::string>());
			plugin.description = UTF8toGB18030(j["description"].get<std::string>());
			for(const auto& it : j["event"])
			{
				int type = it["type"].get<int>();
				int priority = it.count("priority")? it["priority"].get<int>() : 30000;
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
			for(const auto& it:j["menu"])
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
			if(j.count("init_offset")) init = FARPROC((BYTE*)dll + j["init_offset"].get<int>());
		}
		catch(std::exception& e)
		{
			XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！Json文件读取失败! " + e.what()).c_str());
			FreeLibrary(dll);
			return 0;
		}
	}
	else
	{
		XQAPI::OutPutLog(("加载"s + file.filename().string() + "失败！无法打开Json文件!").c_str());
		FreeLibrary(dll);
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
		FreeLibrary(dll);
		return 0;
	}
	
	// 判断是否启用
	fileCopy.replace_extension(".disable");
	if (std::filesystem::exists(fileCopy))
	{
		plugin.enabled = false;
	}
	plugin.dll = dll;
	plugins.push_back(plugin);
	XQAPI::OutPutLog(("加载"s + file.filename().string() + "成功！").c_str());
	
	return 0;
}


// XQ码到CQ码
std::string parseToCQCode(const char* msg)
{
	if (!msg) return "";
	std::string_view msgStr(msg);
	std::string ret;
	size_t l = 0, r = 0, last = 0;
	l = msgStr.find("[");
	r = msgStr.find("]", l);
	while (l != string::npos && r != string::npos)
	{
		ret += msgStr.substr(last, l - last);
		if (msgStr.substr(l, 2) == "[@")
		{
			ret += "[CQ:at,qq=";
			ret += msgStr.substr(l + 2, r - l - 2);
			ret += "]";
		}
		else if (msgStr.substr(l, 5) == "[pic=")
		{
			ret += "[CQ:image,file=";
			ret += msgStr.substr(l + 5, r - l - 5);
			ret += "]";
		}
		else if (msgStr.substr(l, 5) == "[Voi=")
		{
			ret += "[CQ:record,file=";
			ret += msgStr.substr(l + 5, r - l - 5);
			ret += "]";
		}
		else
		{
			ret += msgStr.substr(l, r - l + 1);
		}
		last = r + 1;
		l = msgStr.find("[", r);
		r = msgStr.find(']', l);
	}
	ret += msgStr.substr(last);
	return ret;
}


// CQ码到XQ码
std::string parseFromCQCode(int32_t uploadType, const char* targetId, const char* msg)
{
	if (!msg) return "";
	std::string_view msgStr(msg);
	std::string ret;
	size_t l = 0, r = 0, last = 0;
	l = msgStr.find("[CQ");
	r = msgStr.find("]", l);
	while (l != string::npos && r != string::npos)
	{
		ret += msgStr.substr(last, l - last);
		if (msgStr.substr(l, 10) == "[CQ:at,qq=")
		{
			ret += "[@";
			ret += msgStr.substr(l + 10, r - l - 10);
			ret += "]";
		}
		else if (msgStr.substr(l, 15) == "[CQ:image,file=")
		{
			if (msgStr[l + 15] == '{')
			{
				ret += "[pic=";
				ret += msgStr.substr(l + 15, r - l - 15);
				ret += "]";
			}
			else
			{
				std::string image;
				image += msgStr.substr(l + 15, r - l - 15);
				if (image.substr(0, 4) != "http" && image.substr(0, 3) != "ftp")
				{
					image = rootPath + "\\data\\image\\" + image;
				}
				ret += "[pic=";
				ret += image;
				ret += "]";
			}
		}
		else if (msgStr.substr(l, 14) == "[CQ:image,url=")
		{
			std::string image;
			image += msgStr.substr(l + 14, r - l - 14);
			ret += "[pic=";
			ret += image;
			ret += "]";
		}
		else if (msgStr.substr(l, 16) == "[CQ:record,file=")
		{
			if (msgStr[l + 16] == '{')
			{
				ret += "[Voi=";
				ret += msgStr.substr(l + 16, r - l - 16);
				ret += "]";
			}
			else
			{
				std::string ppath = rootPath + "\\data\\record\\";
				ppath += msgStr.substr(l + 16, r - l - 16);
				ret += "[Voi=";
				ret += ppath;
				ret += "]";
			}
		}
		else
		{
			ret += msgStr.substr(l, r - l + 1);
		}
		last = r + 1;
		l = msgStr.find("[CQ", r);
		r = msgStr.find(']', l);
	}
	ret += msgStr.substr(last);
	return ret;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	hDllModule = hModule;
	return TRUE;
}

#ifdef XQ
CQAPI(const char*, XQ_Create, 4)(const char* ver)
#else
CQAPI(const char*, OQ_Create, 0)()
#endif
{
	// 获取文件目录
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string pathStr(path);
	rootPath = pathStr.substr(0, pathStr.rfind("\\"));

	// 载入API DLL并加载API函数
#ifdef XQ
	XQHModule = LoadLibraryA("xqapi.dll");
#else
	XQHModule = LoadLibraryA("OQapi.dll");
#endif
	XQAPI::initFuncs(XQHModule);

	// 写出CQP.dll
	HRSRC rscInfo = FindResourceA(hDllModule, MAKEINTRESOURCEA(IDR_CQP1), "CQP");
	HGLOBAL rsc = LoadResource(hDllModule, rscInfo);
	DWORD size = SizeofResource(hDllModule, rscInfo);
	char* rscPtr = (char*)LockResource(rsc);
	if (rscPtr && size)
	{
		std::string rscStr(rscPtr, size);
		ofstream ofCQP(rootPath + "\\CQP.dll", ios::out | ios::trunc | ios::binary);
		if (ofCQP) 
		{
			ofCQP << rscStr;
			XQAPI::OutPutLog("写出CQP.dll成功");
		}
		else
		{
			XQAPI::OutPutLog("写出CQP.dll失败！无法创建文件输出流！");
		}
	}
	else
	{
		XQAPI::OutPutLog("写出CQP.dll失败！获取资源失败！");
	}

	// 加载CQP.dll
	CQPHModule = LoadLibraryA("CQP.dll");

	// 加载CQ插件
	std::string ppath = rootPath + "\\CQPlugins\\";
	std::filesystem::create_directories(ppath);
	for (const auto& file : std::filesystem::directory_iterator(ppath))
	{
		if (file.is_regular_file() && file.path().extension() == ".dll")
		{
			loadCQPlugin(file);
		}
	}

	// 按照优先级排序
	for (auto& ele : plugins_events)
	{
		std::sort(ele.second.begin(), ele.second.end());
	}

	// 延迟字符串内存释放
	memFreeThreadShouldRun = true;
	memFreeThread = std::make_unique<std::thread>([]
	{
		while (memFreeThreadShouldRun)
		{
			{
				std::unique_lock lock(memFreeMutex);
				// 延迟5分钟释放字符串内存
				while (!memFreeQueue.empty() && time(nullptr) - memFreeQueue.top().first > 300)
				{
					free((void*)memFreeQueue.top().second);
					memFreeQueue.pop();
				}
			}
			std::this_thread::sleep_for(1s);
		}
		// 在线程退出时释放掉所有内存
		std::unique_lock lock(memFreeMutex);
		while (!memFreeQueue.empty())
		{
			free((void*)memFreeQueue.top().second);
			memFreeQueue.pop();
		}
	});
#ifdef XQ
	return "{\"name\":\"CQXQ\", \"pver\":\"1.0.4\", \"sver\":1, \"author\":\"Suhui\", \"desc\":\"A simple compatibility layer between CQ and XQ\"}";
#else
	return "插件名称{CQOQ}\r\n插件版本{1.0.4}\r\n插件作者{Suhui}\r\n插件说明{A simple compatibility layer between CQ and OQ}\r\n插件skey{8956RTEWDFG3216598WERDF3}\r\n插件sdk{S3}";
#endif
}

#ifdef XQ
CQAPI(int32_t, XQ_DestroyPlugin, 0)()
#else
CQAPI(int32_t, OQ_DestroyPlugin, 0)()
#endif
{
	for (const auto& plugin : plugins_events[CQ_eventExit])
	{
		const auto exit = IntMethod(plugin.event);
		if (exit)
		{
			exit();
		}
	}
	FreeLibrary(XQHModule);
	FreeLibrary(CQPHModule);
	memFreeThreadShouldRun = false;
	memFreeThread->join();
	memFreeThread.reset(nullptr);
	return 0;
}

#ifdef XQ
CQAPI(int32_t, XQ_SetUp, 0)()
#else
CQAPI(int32_t, OQ_SetUp, 0)()
#endif
{
	try
	{
		GUIMain();
	}
	catch (std::exception& e)
	{
		XQAPI::OutPutLog(("CQXQ遇到严重错误, 这可能是CQXQ本身或是其中的某个插件导致的\n错误信息："s + e.what()).c_str());
		MessageBoxA(nullptr, ("CQXQ遇到严重错误, 这可能是CQXQ本身或是其中的某个插件导致的\n错误信息："s + e.what()).c_str(), "CQXQ 错误", MB_OK);
	}
	return 0;
}

// QQ-群号 缓存 用于发送消息
std::map<int64_t, int64_t> UserGroupCache;

// QQ-讨论组号 缓存 用于发送消息
std::map<int64_t, int64_t> UserDiscussCache;

// 群-群成员json字符串缓存 用于获取群成员列表，群成员信息，缓存时间1小时，遇到群成员变动事件/群名片更改事件刷新
std::map<long long, std::pair<std::string, time_t>> GroupMemberCache;

// 群列表缓存 用于获取群列表，缓存时间1小时，遇到群添加/退出等事件刷新
std::pair<std::string, time_t> GroupListCache;

#ifdef XQ
CQAPI(int32_t, XQ_Event, 48)(const char* botQQ, int32_t msgType, int32_t subType, const char* sourceId, const char* activeQQ, const char* passiveQQ, const char* msg, const char* msgNum, const char* msgId, const char* rawMsg, const char* timeStamp, char* retText)
#else
CQAPI(int32_t, OQ_Event, 48)(const char* botQQ, int32_t msgType, int32_t subType, const char* sourceId, const char* activeQQ, const char* passiveQQ, const char* msg, const char* msgNum, const char* msgId, const char* rawMsg, const char* timeStamp, char* retText)
#endif
{
	try {
		std::string botQQStr = botQQ ? botQQ : "";
		if (robotQQ.empty()) robotQQ = botQQStr;
		if (!botQQStr.empty() && robotQQ != botQQStr) return 0;
		if (msgType == XQ_Load)
		{
			for (const auto& plugin : plugins_events[CQ_eventStartup])
			{
				const auto startup = IntMethod(plugin.event);
				if (startup)
				{
					startup();
				}
			}
			return 0;
		}
		if (msgType == XQ_Enable)
		{
			const char* onlineList = XQAPI::GetOnLineList();
			std::string onlineListStr = onlineList ? onlineList : "";

			if (!onlineListStr.empty() && !(onlineListStr[0] == '\r' || onlineListStr[0] == '\n') && !EnabledEventCalled)
			{
				for (const auto& plugin : plugins_events[CQ_eventEnable])
				{
					if (!plugins[plugin.plugin_id].enabled) continue;
					const auto enable = IntMethod(plugin.event);
					if (enable)
					{
						enable();
					}
				}
				EnabledEventCalled = true;
			}
			return 0;
		}
		if (msgType == XQ_LogInComplete)
		{
			if (!EnabledEventCalled && XQAPI::IsEnable())
			{
				for (const auto& plugin : plugins_events[CQ_eventEnable])
				{
					if (!plugins[plugin.plugin_id].enabled) continue;
					const auto enable = IntMethod(plugin.event);
					if (enable)
					{
						enable();
					}
				}
				EnabledEventCalled = true;
			}
			return 0;
		}
		if (msgType == XQ_Disable)
		{
			if (!EnabledEventCalled) return 0;
			EnabledEventCalled = false;
			for (const auto& plugin : plugins_events[CQ_eventDisable])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto disable = IntMethod(plugin.event);
				if (disable)
				{
					disable();
				}
			}
			return 0;
		}
		if (msgType == XQ_FriendMsgEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventPrivateMsg])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto privMsg = EvPriMsg(plugin.event);
				if (privMsg)
				{
					if (privMsg(11, 0, atoll(activeQQ), parseToCQCode(msg).c_str(), 0)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupTmpMsgEvent)
		{
			UserGroupCache[atoll(activeQQ)] = atoll(sourceId);
			for (const auto& plugin : plugins_events[CQ_eventPrivateMsg])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto privMsg = EvPriMsg(plugin.event);
				if (privMsg)
				{
					if (privMsg(2, 0, atoll(activeQQ), parseToCQCode(msg).c_str(), 0)) break;
				}
			}
		}
		if (msgType == XQ_GroupMsgEvent)
		{
			UserGroupCache[stoll(activeQQ)] = atoll(sourceId);
			for (const auto& plugin : plugins_events[CQ_eventGroupMsg])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto groupMsg = EvGroupMsg(plugin.event);
				if (groupMsg)
				{
					if (groupMsg(1, 0, atoll(sourceId), atoll(activeQQ), "", parseToCQCode(msg).c_str(), 0)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_DiscussTmpMsgEvent)
		{
			UserDiscussCache[atoll(activeQQ)] = atoll(sourceId);
			for (const auto& plugin : plugins_events[CQ_eventPrivateMsg])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto privMsg = EvPriMsg(plugin.event);
				if (privMsg)
				{
					if (privMsg(3, 0, atoll(activeQQ), parseToCQCode(msg).c_str(), 0)) break;
				}
			}
		}
		if (msgType == XQ_DiscussMsgEvent)
		{
			UserDiscussCache[atoll(activeQQ)] = atoll(sourceId);
			for (const auto& plugin : plugins_events[CQ_eventDiscussMsg])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto event = EvDiscussMsg(plugin.event);
				if (event)
				{
					if (event(1, 0, atoll(sourceId), atoll(activeQQ), parseToCQCode(msg).c_str(), 0)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupInviteReqEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventRequest_AddGroup])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto invited = EvRequestAddGroup(plugin.event);
				if (invited)
				{
					Unpack p;
					p.add(XQ_GroupInviteReqEvent);
					p.add(sourceId);
					p.add(activeQQ);
					p.add(rawMsg);
					if (invited(2, 0, atoll(sourceId), atoll(activeQQ), msg, base64_encode(p.getAll()).c_str())) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupAddReqEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventRequest_AddGroup])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto addReq = EvRequestAddGroup(plugin.event);
				if (addReq)
				{
					Unpack p;
					p.add(XQ_GroupAddReqEvent);
					p.add(sourceId);
					p.add(activeQQ);
					p.add(rawMsg);
					if (addReq(1, 0, atoll(sourceId), atoll(activeQQ), msg, base64_encode(p.getAll()).c_str())) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupInviteOtherReqEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventRequest_AddGroup])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto addReq = EvRequestAddGroup(plugin.event);
				if (addReq)
				{
					Unpack p;
					p.add(XQ_GroupInviteOtherReqEvent);
					p.add(sourceId);
					p.add(activeQQ);
					p.add(rawMsg);
					if (addReq(1, 0, atoll(sourceId), atoll(activeQQ), msg, base64_encode(p.getAll()).c_str())) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_FriendAddReqEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventRequest_AddFriend])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto addReq = EvRequestAddFriend(plugin.event);
				if (addReq)
				{
					if (addReq(1, 0, atoll(activeQQ), msg, activeQQ)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupBanEvent)
		{
			// TODO: 禁言时间
			XQAPI::OutPutLog(msg);
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupBan])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto ban = EvGroupBan(plugin.event);
				if (ban)
				{
					if (ban(2, 0, atoll(sourceId), atoll(activeQQ), atoll(passiveQQ), 60)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupUnbanEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupBan])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto ban = EvGroupBan(plugin.event);
				if (ban)
				{
					if (ban(1, 0, atoll(sourceId), atoll(activeQQ), atoll(passiveQQ), 0)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupWholeBanEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupBan])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto ban = EvGroupBan(plugin.event);
				if (ban)
				{
					if (ban(2, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), 0, 0)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupWholeUnbanEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupBan])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto ban = EvGroupBan(plugin.event);
				if (ban)
				{
					if (ban(1, 0, atoll(sourceId), atoll(activeQQ), 0, 0)) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberIncreaseByApply)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupMemberIncrease])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto MbrInc = EvGroupMember(plugin.event);
				if (MbrInc)
				{
					if (MbrInc(1, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ))) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberIncreaseByInvite)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupMemberIncrease])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto MbrInc = EvGroupMember(plugin.event);
				if (MbrInc)
				{
					if (MbrInc(2, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ))) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberDecreaseByExit)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupMemberDecrease])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto event = EvGroupMember(plugin.event);
				if (event)
				{
					if (event(1, atoi(timeStamp), atoll(sourceId), 0, atoll(passiveQQ)))  break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberDecreaseByKick)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupMemberDecrease])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto event = EvGroupMember(plugin.event);
				if (event)
				{
					if (passiveQQ == robotQQ)
					{
						if (event(3, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ))) break;
					}
					else
					{
						if (event(2, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ))) break;
					}
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupAdminSet)
		{
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupAdmin])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto event = EvGroupAdmin(plugin.event);
				if (event)
				{
					if (event(2, atoi(timeStamp), atoll(sourceId), atoll(passiveQQ))) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupAdminUnset)
		{
			for (const auto& plugin : plugins_events[CQ_eventSystem_GroupAdmin])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto event = EvGroupAdmin(plugin.event);
				if (event)
				{
					if (event(1, atoi(timeStamp), atoll(sourceId), atoll(passiveQQ))) break;
				}
			}
			return 0;
		}
		// 根据酷Q逻辑，只有不经过酷Q处理的好友添加事件（即比如用户设置了同意一切好友请求）才会调用好友已添加事件
		if (msgType == XQ_FriendAddedEvent)
		{
			for (const auto& plugin : plugins_events[CQ_eventFriend_Add])
			{
				if (!plugins[plugin.plugin_id].enabled) continue;
				const auto event = EvFriendAdd(plugin.event);
				if (event)
				{
					if (event(1, atoi(timeStamp), atoll(activeQQ))) break;
				}
			}
			return 0;
		}
		if (msgType == XQ_groupCardChange)
		{
			GroupMemberCache.erase(atoll(sourceId));
		}
		return 0;
	}
	catch (std::exception& e)
	{
		XQAPI::OutPutLog(("CQXQ遇到严重错误, 这可能是CQXQ本身或是其中的某个插件导致的\n错误信息："s + e.what()).c_str());
		MessageBoxA(nullptr, ("CQXQ遇到严重错误, 这可能是CQXQ本身或是其中的某个插件导致的\n错误信息："s + e.what()).c_str(), "CQXQ 错误", MB_OK);
	}
	return 0;
}


CQAPI(int32_t, CQ_canSendImage, 4)(int32_t)
{
	return 1;
}

CQAPI(int32_t, CQ_canSendRecord, 4)(int32_t)
{
	return 1;
}


CQAPI(int32_t, CQ_sendPrivateMsg, 16)(int32_t plugin_id, int64_t account, const char* msg)
{
	if (robotQQ.empty()) return -1;
	std::string accStr = std::to_string(account);
	
	if (XQAPI::IfFriend(robotQQ.c_str(), accStr.c_str()))
	{
		XQAPI::SendMsg(robotQQ.c_str(), 1, accStr.c_str(), accStr.c_str(), parseFromCQCode(1, accStr.c_str(), msg).c_str(), 0);
	}
	else if (UserGroupCache.count(account))
	{
		XQAPI::SendMsg(robotQQ.c_str(), 4, std::to_string(UserGroupCache[account]).c_str(), accStr.c_str(), parseFromCQCode(1, accStr.c_str(), msg).c_str(), 0);
	}
	else if (UserDiscussCache.count(account))
	{
		XQAPI::SendMsg(robotQQ.c_str(), 5, std::to_string(UserDiscussCache[account]).c_str(), accStr.c_str(), parseFromCQCode(1, accStr.c_str(), msg).c_str(), 0);
	}
	else
	{
		XQAPI::OutPutLog(("无法发送消息给QQ" + accStr + ": 找不到可用的发送路径").c_str());
	}
	
	return 0;
}

CQAPI(int32_t, CQ_sendGroupMsg, 16)(int32_t plugin_id, int64_t group, const char* msg)
{
	if (robotQQ.empty()) return -1;
	std::string grpStr = std::to_string(group);
	XQAPI::SendMsg(robotQQ.c_str(), 2, grpStr.c_str(), robotQQ.c_str(), parseFromCQCode(2, grpStr.c_str(), msg).c_str(), 0);
	return 0;
}

CQAPI(int32_t, CQ_setFatal, 8)(int32_t plugin_id, const char* info)
{
	XQAPI::OutPutLog((plugins[plugin_id].file + ": [FATAL] " + info).c_str());
	return 0;
}

CQAPI(const char*, CQ_getAppDirectory, 4)(int32_t plugin_id)
{
	std::string ppath;
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string pathStr(path);
	ppath = pathStr.substr(0, pathStr.rfind('\\')) + "\\CQPlugins\\config\\" + plugins[plugin_id].file + "\\";
	std::filesystem::create_directories(ppath);
	return delayMemFreeCStr(ppath.c_str());
}

CQAPI(int64_t, CQ_getLoginQQ, 4)(int32_t plugin_id)
{
	return atoll(robotQQ.c_str());
}

CQAPI(const char*, CQ_getLoginNick, 4)(int32_t plugin_id)
{
	return XQAPI::GetNick(robotQQ.c_str(), robotQQ.c_str());
}

CQAPI(int32_t, CQ_setGroupAnonymous, 16)(int32_t plugin_id, int64_t group, BOOL enable)
{
	XQAPI::SetAnon(robotQQ.c_str(), std::to_string(group).c_str(), enable);
	return 0;
}

CQAPI(int32_t, CQ_setGroupBan, 28)(int32_t plugin_id, int64_t group, int64_t member, int64_t duration)
{
	XQAPI::ShutUP(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(member).c_str(), static_cast<int32_t>(duration));
	return 0;
}

CQAPI(int32_t, CQ_setGroupCard, 24)(int32_t plugin_id, int64_t group, int64_t member, const char* card)
{
	XQAPI::SetGroupCard(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(member).c_str(), card);
	return 0;
}

CQAPI(int32_t, CQ_setGroupKick, 24)(int32_t plugin_id, int64_t group, int64_t member, BOOL reject)
{
	XQAPI::KickGroupMBR(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(member).c_str(), reject);
	return 0;
}

CQAPI(int32_t, CQ_setGroupLeave, 16)(int32_t plugin_id, int64_t group, BOOL dismiss)
{
	XQAPI::QuitGroup(robotQQ.c_str(), std::to_string(group).c_str());
	return 0;
}

CQAPI(int32_t, CQ_setGroupSpecialTitle, 32)(int32_t plugin_id, int64_t group, int64_t member,
                                            const char* title, int64_t duration)
{
	XQAPI::OutPutLog((plugins[plugin_id].file + "调用了不支持的API CQ_setGroupSpecialTitle").c_str());
	return 0;
}

CQAPI(int32_t, CQ_setGroupWholeBan, 16)(int32_t plugin_id, int64_t group, BOOL enable)
{
	XQAPI::ShutUP(robotQQ.c_str(), std::to_string(group).c_str(), "", enable);
	return 0;
}

CQAPI(int32_t, CQ_deleteMsg, 12)(int32_t plugin_id, int64_t msg_id)
{
	XQAPI::OutPutLog((plugins[plugin_id].file + "调用了未实现的API CQ_deleteMsg").c_str());
	return 0;
}

CQAPI(const char*, CQ_getFriendList, 8)(int32_t plugin_id, BOOL reserved)
{
	std::string ret;
	const char* frdLst = XQAPI::GetFriendList_B(robotQQ.c_str());
	if (!frdLst) return "";
	std::string friendList = frdLst;
	Unpack p;
	std::vector<Unpack> Friends;
	int count = 0;
	while (!friendList.empty())
	{
		size_t endline = friendList.find('\n');
		std::string item = friendList.substr(0, endline);
		while (!item.empty() && item[item.length() - 1] == '\r' || item[item.length() - 1] == '\n') item.erase(item.end() - 1);
		if(!item.empty())
		{
			Unpack tmp;
			tmp.add(atoll(item.c_str()));
			const char* nick = XQAPI::GetNick(robotQQ.c_str(), item.c_str());
			tmp.add(nick ? nick : "");
			const char* remarks = XQAPI::GetFriendsRemark(robotQQ.c_str(), item.c_str());
			tmp.add(remarks ? remarks : "");
			Friends.push_back(tmp);
			count++;
		}
		if (endline == string::npos) friendList = "";
		else friendList = friendList.substr(endline + 1);
	}
	p.add(count);
	for (auto& g : Friends)
	{
		p.add(g);
	}
	ret = base64_encode(p.getAll());
	return delayMemFreeCStr(ret.c_str());
}

CQAPI(const char*, CQ_getGroupInfo, 16)(int32_t plugin_id, int64_t group, BOOL disableCache)
{
	std::string ret;

	// 判断是否是新获取的列表
	bool newRetrieved = false;
	std::string memberListStr;

	// 判断是否要使用缓存
	if (disableCache || !GroupMemberCache.count(group) || (time(nullptr) - GroupMemberCache[group].second) > 3600)
	{
		newRetrieved = true;
		const char* memberList = XQAPI::GetGroupMemberList_B(robotQQ.c_str(), std::to_string(group).c_str());
		memberListStr = memberList ? memberList : "";
	}
	else
	{
		memberListStr = GroupMemberCache[group].first;
	}

	try
	{
		if (memberListStr.empty())
		{
			throw std::exception("GetGroupMemberList Failed");
		}
		nlohmann::json j = nlohmann::json::parse(memberListStr);
		std::string groupStr = std::to_string(group);
		const char* groupName = XQAPI::GetGroupName(robotQQ.c_str(), groupStr.c_str());
		std::string groupNameStr = groupName ? groupName : "";
		int currentNum = j["mem_num"].get<int>();
		int maxNum = j["max_num"].get<int>();
		int friendNum = 0;
		for (const auto& member : j["members"].items())
		{
			if (member.value().count("fr") && member.value()["fr"].get<int>() == 1)
			{
				friendNum += 1;
			}
		}
		Unpack p;
		p.add(group);
		p.add(groupNameStr);
		p.add(currentNum);
		p.add(maxNum);
		p.add(friendNum);
		ret = base64_encode(p.getAll());
		if (newRetrieved) GroupMemberCache[group] = { memberListStr, time(nullptr) };
		return delayMemFreeCStr(ret.c_str());
	}
	catch (std::exception&)
	{
		XQAPI::OutPutLog(("警告, 获取群成员列表失败, 正在使用更慢的另一种方法尝试: "s + memberListStr).c_str());
		std::string groupStr = std::to_string(group);
		const char* groupName = XQAPI::GetGroupName(robotQQ.c_str(), groupStr.c_str());
		std::string groupNameStr = groupName ? groupName : "";
		const char* groupNum = XQAPI::GetGroupMemberNum(robotQQ.c_str(), groupStr.c_str());
		int currentNum = 0, maxNum = 0;
		if (groupNum)
		{
			std::string groupNumStr = groupNum;
			size_t newline = groupNumStr.find('\n');
			if (newline != string::npos)
			{
				currentNum = atoi(groupNumStr.substr(0, newline).c_str());
				maxNum = atoi(groupNumStr.substr(newline + 1).c_str());
			}
		}
		Unpack p;
		p.add(group);
		p.add(groupNameStr);
		p.add(currentNum);
		p.add(maxNum);
		p.add(0); // 这种方式暂不支持好友人数
		ret = base64_encode(p.getAll());
		return delayMemFreeCStr(ret.c_str());
	}
}

CQAPI(const char*, CQ_getGroupList, 4)(int32_t plugin_id)
{
	std::string ret;

	const char* groupList = XQAPI::GetGroupList(robotQQ.c_str());
	std::string groupListStr = groupList ? groupList : "";
	if (groupListStr.empty()) return "";
	try
	{
		Unpack p;
		std::vector<Unpack> Groups;
		nlohmann::json j = nlohmann::json::parse(groupListStr);
		for (const auto& group : j["create"])
		{
			Unpack t;
			t.add(group["gc"].get<long long>());
			t.add(UTF8toGB18030(group["gn"].get<std::string>()));
			Groups.push_back(t);
		}
		for (const auto& group : j["join"])
		{
			Unpack t;
			t.add(group["gc"].get<long long>());
			t.add(UTF8toGB18030(group["gn"].get<std::string>()));
			Groups.push_back(t);
		}
		for (const auto& group : j["manage"])
		{
			Unpack t;
			t.add(group["gc"].get<long long>());
			t.add(UTF8toGB18030(group["gn"].get<std::string>()));
			Groups.push_back(t);
		}
		p.add(static_cast<int>(Groups.size()));
		for (auto& group : Groups)
		{
			p.add(group);
		}
		ret = base64_encode(p.getAll());
		return delayMemFreeCStr(ret.c_str());
	}
	catch (std::exception&)
	{
		XQAPI::OutPutLog(("警告, 获取群列表失败, 正在使用更慢的另一种方法尝试: "s + groupList).c_str());
		const char* group = XQAPI::GetGroupList_B(robotQQ.c_str());
		if (!group) return "";
		std::string groupList = group;
		Unpack p;
		std::vector<Unpack> Groups;
		int count = 0;
		while (!groupList.empty())
		{
			size_t endline = groupList.find('\n');
			std::string item = groupList.substr(0, endline);
			while (!item.empty() && item[item.length() - 1] == '\r' || item[item.length() - 1] == '\n') item.erase(item.end() - 1);
			if (!item.empty())
			{
				Unpack tmp;
				tmp.add(atoll(item.c_str()));
				const char* groupName = XQAPI::GetGroupName(robotQQ.c_str(), item.c_str());
				tmp.add(groupName ? groupName : "");
				Groups.push_back(tmp);
				count++;
			}
			if (endline == string::npos) groupList = "";
			else groupList = groupList.substr(endline + 1);
		}
		p.add(count);
		for (auto& g : Groups)
		{
			p.add(g);
		}
		ret = base64_encode(p.getAll());
		return delayMemFreeCStr(ret.c_str());
	}
}

CQAPI(const char*, CQ_getGroupMemberInfoV2, 24)(int32_t plugin_id, int64_t group, int64_t account, BOOL disableCache)
{
	std::string ret;
	// 判断是否是新获取的列表
	bool newRetrieved = false;
	std::string memberListStr;

	// 判断是否要使用缓存
	if (disableCache || !GroupMemberCache.count(group) || (time(nullptr) - GroupMemberCache[group].second) > 3600)
	{
		newRetrieved = true;
		const char* memberList = XQAPI::GetGroupMemberList_B(robotQQ.c_str(), std::to_string(group).c_str());
		memberListStr = memberList ? memberList : "";
	}
	else
	{
		memberListStr = GroupMemberCache[group].first;
	}

	try
	{
		if (memberListStr.empty())
		{
			throw std::exception("GetGroupMemberList Failed");
		}
		nlohmann::json j = nlohmann::json::parse(memberListStr);
		long long owner = j["owner"].get<long long>();
		std::set<long long> admin;
		if (j.count("adm")) j["adm"].get_to(admin);
		std::map<std::string, std::string> lvlName = j["levelname"].get<std::map<std::string, std::string>>();
		for (auto& item : lvlName)
		{
			lvlName[item.first] = UTF8toGB18030(item.second);
		}
		if (!j["members"].count(std::to_string(account))) return "";
		Unpack t;
		t.add(group);
		t.add(account);
		t.add(j["members"].count("nk") ? UTF8toGB18030(j["members"]["nk"].get<std::string>()) : "");
		t.add(j["members"].count("cd") ? UTF8toGB18030(j["members"]["cd"].get<std::string>()) : "");
		t.add(255);
		t.add(-1);
		t.add("");
		t.add(j["members"].count("jt") ? j["members"]["jt"].get<int>() : 0);
		t.add(j["members"].count("lst") ? j["members"]["lst"].get<int>() : 0);
		t.add(j["members"].count("ll") ? (lvlName.count("lvln" + std::to_string(j["members"]["ll"].get<int>())) ? lvlName["lvln" + std::to_string(j["members"]["ll"].get<int>())] : "") : "");
		t.add(account == owner ? 3 : (admin.count(account) ? 2 : 1));
		t.add(FALSE);
		t.add("");
		t.add(-1);
		t.add(TRUE);
		ret = base64_encode(t.getAll());
		if (newRetrieved) GroupMemberCache[group] = { memberListStr, time(nullptr) };
		return delayMemFreeCStr(ret.c_str());
	}
	catch (std::exception&)
	{
		XQAPI::OutPutLog(("警告, 获取群成员列表失败, 正在使用更慢的另一种方法尝试: "s + memberListStr).c_str());
		std::string grpStr = std::to_string(group);
		std::string accStr = std::to_string(account);
		Unpack p;
		p.add(group);
		p.add(account);
		const char* nick = XQAPI::GetNick(robotQQ.c_str(), accStr.c_str());
		p.add(nick ? nick : "");
		const char* groupCard = XQAPI::GetGroupCard(robotQQ.c_str(), grpStr.c_str(), accStr.c_str());
		p.add(groupCard ? groupCard : "");
		p.add(255);
		p.add(-1);
		p.add("");
		p.add(0);
		p.add(0);
		p.add("");
		const char* admin = XQAPI::GetGroupAdmin(robotQQ.c_str(), std::to_string(group).c_str());
		std::string adminList = admin ? admin : "";
		int count = 0;
		int permissions = 1;
		while (!adminList.empty())
		{
			size_t endline = adminList.find('\n');
			std::string item = adminList.substr(0, endline);
			while (!item.empty() && item[item.length() - 1] == '\r' || item[item.length() - 1] == '\n') item.erase(item.end() - 1);
			if (item == accStr)
			{
				if (count == 0)permissions = 3;
				else permissions = 2;
				break;
			}
			if (endline == string::npos) adminList = "";
			else adminList = adminList.substr(endline + 1);
			count++;
		}
		p.add(permissions);
		p.add(FALSE);
		p.add("");
		p.add(0);
		p.add(TRUE);
		ret = base64_encode(p.getAll());
		return delayMemFreeCStr(ret.c_str());
	}
}

CQAPI(const char*, CQ_getGroupMemberList, 12)(int32_t plugin_id, int64_t group)
{
	std::string ret;
	const char* memberList = XQAPI::GetGroupMemberList_B(robotQQ.c_str(), std::to_string(group).c_str());
	std::string memberListStr = memberList ? memberList : "";
	try
	{
		if (memberListStr.empty())
		{
			throw std::exception("GetGroupMemberList Failed");
		}
		Unpack p;
		nlohmann::json j = nlohmann::json::parse(memberListStr);
		long long owner = j["owner"].get<long long>();
		std::set<long long> admin;
		if (j.count("adm")) j["adm"].get_to(admin);
		int mem_num = j["mem_num"].get<int>();
		std::map<std::string, std::string> lvlName = j["levelname"].get<std::map<std::string, std::string>>();
		for (auto& item : lvlName)
		{
			lvlName[item.first] = UTF8toGB18030(item.second);
		}
		p.add(mem_num);
		for (const auto& member : j["members"].items())
		{
			long long qq = std::stoll(member.key());
			Unpack t;
			t.add(group);
			t.add(qq);
			t.add(member.value().count("nk") ? UTF8toGB18030(member.value()["nk"].get<std::string>()) : "");
			t.add(member.value().count("cd") ? UTF8toGB18030(member.value()["cd"].get<std::string>()) : "");
			t.add(255);
			t.add(-1);
			t.add("");
			t.add(member.value().count("jt") ? member.value()["jt"].get<int>() : 0);
			t.add(member.value().count("lst") ? member.value()["lst"].get<int>() : 0);
			t.add(member.value().count("ll") ? (lvlName.count("lvln" + std::to_string(member.value()["ll"].get<int>())) ? lvlName["lvln" + std::to_string(member.value()["ll"].get<int>())]: "") : "");
			t.add(qq == owner ? 3 : (admin.count(qq) ? 2 : 1));
			t.add(FALSE);
			t.add("");
			t.add(-1);
			t.add(TRUE);
			p.add(t);
		}
		GroupMemberCache[group] = { memberListStr, time(nullptr) };
		ret = base64_encode(p.getAll());
		return delayMemFreeCStr(ret.c_str());
	}
	catch (std::exception&)
	{
		return "";
	}
	return "";
}

CQAPI(const char*, CQ_getCookiesV2, 8)(int32_t plugin_id, const char* domain)
{
	return XQAPI::GetCookies(robotQQ.c_str());
}

CQAPI(const char*, CQ_getCsrfToken, 4)(int32_t plugin_id)
{
	return XQAPI::GetBkn(robotQQ.c_str());
}

CQAPI(const char*, CQ_getImage, 8)(int32_t plugin_id, const char* image)
{
	XQAPI::OutPutLog((plugins[plugin_id].file + "调用了未实现的API CQ_getImage").c_str());
	return "";
}

CQAPI(const char*, CQ_getRecordV2, 12)(int32_t plugin_id, const char* file, const char* format)
{
	if (!file) return "";
	std::string fileStr(file);
	if (fileStr.empty()) return "";
	if (fileStr.substr(0, 16) == "[CQ:record,file=")
	{
		fileStr = "[Voi=" + fileStr.substr(16, fileStr.length() - 1 - 16) + "]";
	}
	return XQAPI::GetVoiLink(robotQQ.c_str(), fileStr.c_str());
}

CQAPI(const char*, CQ_getStrangerInfo, 16)(int32_t plugin_id, int64_t account, BOOL cache)
{
	std::string ret;
	std::string accStr = std::to_string(account);
	Unpack p;
	p.add(account);
	p.add(XQAPI::GetNick(robotQQ.c_str(), accStr.c_str()));
	p.add(255);
	p.add(-1);
	ret = base64_encode(p.getAll());
	return delayMemFreeCStr(ret.c_str());
}

CQAPI(int32_t, CQ_sendDiscussMsg, 16)(int32_t plugin_id, int64_t discuss, const char* msg)
{
	if (robotQQ.empty()) return -1;
	std::string discussStr = std::to_string(discuss);
	XQAPI::SendMsg(robotQQ.c_str(), 3, discussStr.c_str(), robotQQ.c_str(), parseFromCQCode(2, discussStr.c_str(), msg).c_str(), 0);
	return 0;
}

CQAPI(int32_t, CQ_sendLikeV2, 16)(int32_t plugin_id, int64_t account, int32_t times)
{
	std::string accStr = std::to_string(account);
	for (int i=0;i!=times;i++)
	{
		XQAPI::UpVote(robotQQ.c_str(), accStr.c_str());
	}
	return 0;
}

CQAPI(int32_t, CQ_setDiscussLeave, 12)(int32_t plugin_id, int64_t discuss)
{
	XQAPI::OutPutLog((plugins[plugin_id].file + "调用了不支持的API CQ_setDiscussLeave").c_str());
	return 0;
}

CQAPI(int32_t, CQ_setFriendAddRequest, 16)(int32_t plugin_id, const char* id, int32_t type, const char* remark)
{
	XQAPI::HandleFriendEvent(robotQQ.c_str(), id, type == 1 ? 10 : 20, remark);
	return 0;
}

CQAPI(int32_t, CQ_setGroupAddRequestV2, 20)(int32_t plugin_id, const char* id, int32_t req_type, int32_t fb_type,
                                            const char* reason)
{
	Unpack p(base64_decode(id));
	int eventType = p.getInt();
	std::string group = p.getstring();
	std::string qq = p.getstring();
	std::string raw = p.getstring();
	XQAPI::HandleGroupEvent(robotQQ.c_str(), eventType, qq.c_str(), group.c_str(), raw.c_str(), fb_type == 1 ? 10 : 20, reason);
	return 0;
}

CQAPI(int32_t, CQ_setGroupAdmin, 24)(int32_t plugin_id, int64_t group, int64_t account, BOOL admin)
{
	XQAPI::OutPutLog((plugins[plugin_id].file + "调用了不支持的API CQ_setAdmin").c_str());
	//XQAPI::SetAdmin(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(account).c_str(), admin);
	return 0;
}

CQAPI(int32_t, CQ_setGroupAnonymousBan, 24)(int32_t plugin_id, int64_t group, const char* id, int64_t duration)
{
	XQAPI::OutPutLog((plugins[plugin_id].file + "调用了不支持的API CQ_setGroupAnonymousBan").c_str());
	return 0;
}

CQAPI(int32_t, CQ_addLog, 16)(int32_t plugin_id, int32_t priority, const char* type, const char* content)
{
	string level;
	switch(priority)
	{
	case 0:
		level = "DEBUG";
		break;
	case 10:
		level = "INFO";
		break;
	case 11:
		level = "INFOSUCCESS";
		break;
	case 12:
		level = "INFORECV";
		break;
	case 13:
		level = "INFOSEND";
		break;
	case 20:
		level = "WARNING";
		break;
	case 30:
		level = "ERROR";
		break;
	case 40:
		level = "FATAL";
		break;
	default:
		level = "UNKNOWN";
		break;
	}
	XQAPI::OutPutLog((plugins[plugin_id].file + ": [" + level + "] [" + type + "] " + content).c_str());
	return 0;
}

// Legacy

CQAPI(const char*, CQ_getCookies, 4)(int32_t plugin_id)
{
	return CQ_getCookiesV2(plugin_id, "");
}

CQAPI(int32_t, CQ_setGroupAddRequest, 16)(int32_t plugin_id, const char* id, int32_t req_type, int32_t fb_type)
{
	return CQ_setGroupAddRequestV2(plugin_id, id, req_type, fb_type, "");
}

CQAPI(int32_t, CQ_sendLike, 12)(int32_t plugin_id, int64_t account)
{
	return CQ_sendLikeV2(plugin_id, account, 1);
}

