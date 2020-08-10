#include <vector>
#include <string>
#include <Windows.h>
#include <thread>
#include <future>
#include <filesystem>
#include <fstream>
#include <set>
#include "GlobalVar.h"
#include "native.h"
#include "CQTools.h"
#include "json.hpp"
#include "ctpl_stl.h"
#include "Unpack.h"
#include "GlobalVar.h"
#include "GUI.h"

using namespace std;

#define XQ

std::string robotQQ;

ctpl::thread_pool p(4);

#define XQAPI(Name, ReturnType, ...) using Name##_FUNC = std::function<ReturnType (__stdcall)(__VA_ARGS__)>; \
	Name##_FUNC _##Name; \
	using Name##_TYPE = ReturnType (__stdcall*)(__VA_ARGS__); \
	template <typename... Args> \
	ReturnType Name(Args&&... args) \
	{ \
		return p.push([&args...](int iThread){ return _##Name(std::forward<Args>(args)...); }).get(); \
	}

XQAPI(XQ_sendMsg, void, const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId)

XQAPI(XQ_outputLog, void, const char* content)

XQAPI(XQ_getNick, const char*, const char* botQQ, const char* QQ)

//XQAPI(XQ_getGender, int32_t, const char* botQQ, const char* QQ)

//XQAPI(XQ_getAge, int32_t, const char* botQQ, const char* QQ)

XQAPI(XQ_getGroupAdmin, const char*, const char* botQQ, const char* groupId)

XQAPI(XQ_getGroupCard, const char*, const char* botQQ, const char* groupId, const char* QQ)

XQAPI(XQ_getGroupList, const char*, const char* botQQ)

XQAPI(XQ_getGroupList_B, const char*, const char* botQQ)

XQAPI(XQ_getGroupName, const char*, const char* botQQ, const char* groupId)

XQAPI(XQ_getFriendList, const char*, const char* botQQ)

XQAPI(XQ_getFriendsRemark, const char*, const char* botQQ, const char* QQ)

XQAPI(XQ_getGroupMemberList, const char*, const char* botQQ, const char* groupId)

XQAPI(XQ_getGroupMemberList_B, const char*, const char* botQQ, const char* groupId)

XQAPI(XQ_getGroupMemberList_C, const char*, const char* botQQ, const char* groupId)

XQAPI(XQ_handleGroupEvent, void, const char* botQQ, int32_t reqType, const char* QQ, const char* groupId, const char* seq, int32_t rspType, const char* msg)

XQAPI(XQ_handleFriendEvent, void, const char* botQQ, const char* QQ, int32_t rspType, const char* msg)

XQAPI(XQ_quitGroup, void, const char* botQQ, const char* groupId)

XQAPI(XQ_getGroupMemberNum, const char*, const char* botQQ, const char* groupId)

XQAPI(XQ_setAnon, BOOL, const char* botQQ, const char* groupId, BOOL enable)

XQAPI(XQ_shutUp, void, const char* botQQ, const char* groupId, const char* QQ, int32_t duration)

XQAPI(XQ_setGroupCard, BOOL, const char* botQQ, const char* groupId, const char* QQ, const char* card)

XQAPI(XQ_setAdmin, const char*, const char* botQQ, const char* groupId, const char* QQ, BOOL admin)

XQAPI(XQ_kickGroupMBR, void, const char* botQQ, const char* groupId, const char* QQ, BOOL refuseForever)

XQAPI(XQ_upVote, const char*, const char* botQQ, const char* QQ)

XQAPI(XQ_isEnable, BOOL)

XQAPI(XQ_getOnlineList, const char*)

XQAPI(XQ_upLoadPic, const char*, const char* botQQ, int32_t uploadType, const char* targetId, const unsigned char* image)

XQAPI(XQ_ifFriend, BOOL, const char* botQQ, const char* QQ)

XQAPI(XQ_getCookies, const char*, const char* botQQ)

XQAPI(XQ_getBkn, const char*, const char* botQQ)

HMODULE XQHModule = nullptr;

int loadCQPlugin(const std::filesystem::path& file)
{
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string pathStr(path);
	std::string ppath = pathStr.substr(0, pathStr.rfind('\\')) + "\\CQPlugins\\";
	native_plugin plugin = { static_cast<int>(plugins.size()), file.filename().string() };
	const auto dll = LoadLibraryA(file.string().c_str());
	if (!dll)
	{
		int err = GetLastError();
		XQ_outputLog(("加载"s + file.filename().string() + "失败！LoadLibrary错误代码：" + std::to_string(err)).c_str());
		FreeLibrary(dll);
		return err;
	}
	// 读取Json
	auto fileCopy = file;
	fileCopy.replace_extension(".json");
	ifstream jsonstream(fileCopy);
	if (jsonstream)
	{
		try
		{
			nlohmann::json j = nlohmann::json::parse(jsonstream, nullptr, true, true);
			plugin.name = UTF8toGBK(j["name"].get<std::string>());
			plugin.version = UTF8toGBK(j["version"].get<std::string>());
			j["version_id"].get_to(plugin.version_id);
			plugin.author = UTF8toGBK(j["author"].get<std::string>());
			plugin.description = UTF8toGBK(j["description"].get<std::string>());
			for(const auto& it : j["event"])
			{
				plugin.events[it["type"].get<int>()] = it["function"].get<std::string>();
			}
			for(const auto& it:j["menu"])
			{
				plugin.menus.push_back({ UTF8toGBK(it["name"].get<std::string>()), it["function"].get<std::string>() });
			}
		}
		catch(std::exception& e)
		{
			XQ_outputLog(("加载"s + file.filename().string() + "失败！Json文件读取失败! " + e.what()).c_str());
			FreeLibrary(dll);
			return 0;
		}
	}
	else
	{
		XQ_outputLog(("加载"s + file.filename().string() + "失败！无法打开Json文件!").c_str());
		FreeLibrary(dll);
		return 0;
	}

	const auto init = FuncInitialize(GetProcAddress(dll, "Initialize"));
	if (init)
	{
		init(plugin.id);
	}
	else
	{
		XQ_outputLog(("加载"s + file.filename().string() + "失败！无公开的Initialize函数!").c_str());
		FreeLibrary(dll);
		return 0;
	}
	
	plugin.dll = dll;
	plugins.push_back(plugin);
	XQ_outputLog(("加载"s + file.filename().string() + "成功！").c_str());
	
	return 0;
}

std::string parseToCQCode(const char* msg)
{
	if (!msg) return "";
	std::string ret(msg);
	size_t find_res = ret.find("[@");
	while (find_res != string::npos)
	{
		ret.replace(find_res, 2, "[CQ:at,qq=");
		find_res = ret.find("[@", find_res + 1);
	}
	find_res = ret.find("[pic=");
	while (find_res != string::npos)
	{
		ret.replace(find_res, 5, "[CQ:image,file=");
		find_res = ret.find("[pic=", find_res + 1);
	}
	return ret;
}

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
			if (l + 15 < msgStr.length())
			{
				if (msgStr[l + 15] == '{')
				{
					ret += "[pic=";
					ret += msgStr.substr(l + 15, r - l - 15);
					ret += "]";
				}
				else
				{
					char path[MAX_PATH];
					std::string buffer;
					GetModuleFileNameA(nullptr, path, MAX_PATH);
					std::string pathStr(path);
					std::string ppath = pathStr.substr(0, pathStr.rfind('\\')) + "\\data\\image\\";
					ppath += msgStr.substr(l + 15, r - l - 15);
					std::basic_ifstream<unsigned char> readImage(ppath, ios::in | std::ios::binary);
					if (readImage)
					{
						std::basic_string<unsigned char> content((std::istreambuf_iterator<unsigned char>(readImage)), std::istreambuf_iterator<unsigned char>());
						int common = 1;
						int size = content.size();
						content = std::basic_string<unsigned char>(reinterpret_cast<unsigned char*>(&common), 4) + std::basic_string<unsigned char>(reinterpret_cast<unsigned char*>(&size), 4) + content;
						const char* uploadPic = XQ_upLoadPic(robotQQ.c_str(), uploadType, targetId, content.c_str() + 8);
						std::string uploadPicStr = uploadPic ? uploadPic : "";
						ret += uploadPicStr;
					}
				}
			}
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
#ifdef XQ
	XQHModule = LoadLibraryA("xqapi.dll");
#else
	XQHModule = LoadLibraryA("OQapi.dll");
#endif
	_XQ_outputLog = (XQ_outputLog_TYPE)GetProcAddress(XQHModule, "Api_OutPutLog");
	_XQ_sendMsg = (XQ_sendMsg_TYPE)GetProcAddress(XQHModule, "Api_SendMsg");
	_XQ_getNick = (XQ_getNick_TYPE)GetProcAddress(XQHModule, "Api_GetNick");
	//_XQ_getGender = (XQ_getGender_TYPE)GetProcAddress(XQHModule, "Api_GetGender");
	//_XQ_getAge = (XQ_getAge_TYPE)GetProcAddress(XQHModule, "Api_GetAge");
	_XQ_getGroupAdmin = (XQ_getGroupAdmin_TYPE)GetProcAddress(XQHModule, "Api_GetGroupAdmin");
	_XQ_getGroupCard = (XQ_getGroupCard_TYPE)GetProcAddress(XQHModule, "Api_GetGroupCard");
	_XQ_getGroupList = (XQ_getGroupList_TYPE)GetProcAddress(XQHModule, "Api_GetGroupList");
	_XQ_getGroupList_B = (XQ_getGroupList_B_TYPE)GetProcAddress(XQHModule, "Api_GetGroupList_B");
	_XQ_getGroupName = (XQ_getGroupName_TYPE)GetProcAddress(XQHModule, "Api_GetGroupName");
	_XQ_getFriendList = (XQ_getFriendList_TYPE)GetProcAddress(XQHModule, "Api_GetFriendList_B");
	_XQ_getFriendsRemark = (XQ_getFriendsRemark_TYPE)GetProcAddress(XQHModule, "Api_GetFriendsRemark");
	_XQ_getGroupMemberList = (XQ_getGroupMemberList_TYPE)GetProcAddress(XQHModule, "Api_GetGroupMemberList");
	_XQ_getGroupMemberList_B = (XQ_getGroupMemberList_B_TYPE)GetProcAddress(XQHModule, "Api_GetGroupMemberList_B");
	_XQ_getGroupMemberList_C = (XQ_getGroupMemberList_C_TYPE)GetProcAddress(XQHModule, "Api_GetGroupMemberList_C");
	_XQ_handleGroupEvent = (XQ_handleGroupEvent_TYPE)GetProcAddress(XQHModule, "Api_HandleGroupEvent");
	_XQ_handleFriendEvent = (XQ_handleFriendEvent_TYPE)GetProcAddress(XQHModule, "Api_HandleFriendEvent");
	_XQ_quitGroup = (XQ_quitGroup_TYPE)GetProcAddress(XQHModule, "Api_QuitGroup");
	_XQ_getGroupMemberNum = (XQ_getGroupMemberNum_TYPE)GetProcAddress(XQHModule, "Api_GetGroupMemberNum");
	_XQ_setAnon = (XQ_setAnon_TYPE)GetProcAddress(XQHModule, "Api_SetAnon");
	_XQ_shutUp = (XQ_shutUp_TYPE)GetProcAddress(XQHModule, "Api_ShutUP");
	_XQ_setGroupCard = (XQ_setGroupCard_TYPE)GetProcAddress(XQHModule, "Api_SetGroupCard");
	_XQ_setAdmin = (XQ_setAdmin_TYPE)GetProcAddress(XQHModule, "Api_SetAdmin");
	_XQ_kickGroupMBR = (XQ_kickGroupMBR_TYPE)GetProcAddress(XQHModule, "Api_KickGroupMBR");
	_XQ_upVote = (XQ_upVote_TYPE)GetProcAddress(XQHModule, "Api_UpVote");
	_XQ_isEnable = (XQ_isEnable_TYPE)GetProcAddress(XQHModule, "Api_IsEnable");
	_XQ_getOnlineList = (XQ_getOnlineList_TYPE)GetProcAddress(XQHModule, "Api_GetOnLineList");
	_XQ_upLoadPic = (XQ_upLoadPic_TYPE)GetProcAddress(XQHModule, "Api_UpLoadPic");
	_XQ_ifFriend = (XQ_ifFriend_TYPE)GetProcAddress(XQHModule, "Api_IfFriend");
	_XQ_getCookies = (XQ_getCookies_TYPE)GetProcAddress(XQHModule, "Api_GetCookies");
	_XQ_getBkn = (XQ_getBkn_TYPE)GetProcAddress(XQHModule, "Api_GetBkn");
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string pathStr(path);
	std::string ppath = pathStr.substr(0, pathStr.rfind('\\')) + "\\CQPlugins\\";
	std::filesystem::create_directories(ppath);
	for(const auto& file : std::filesystem::directory_iterator(ppath))
	{
		if (file.is_regular_file() && file.path().extension() == ".dll")
		{
			loadCQPlugin(file);
		}
	}
	
#ifdef XQ
	return "{\"name\":\"CQXQ\", \"pver\":\"1.0.2\", \"sver\":1, \"author\":\"Suhui\", \"desc\":\"A simple compatibility layer between CQ and XQ\"}";
#else
	return "插件名称{CQOQ}\r\n插件版本{1.0.2}\r\n插件作者{Suhui}\r\n插件说明{A simple compatibility layer between CQ and OQ}\r\n插件skey{8956RTEWDFG3216598WERDF3}\r\n插件sdk{S3}";
#endif
}

#ifdef XQ
CQAPI(int32_t, XQ_DestroyPlugin, 0)()
#else
CQAPI(int32_t, OQ_DestroyPlugin, 0)()
#endif
{
	for (const auto& plugin : plugins)
	{
		if (!plugin.events.count(CQ_eventExit)) continue;
		const auto exit = IntMethod(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventExit).c_str()));
		if (exit)
		{
			exit();
		}
	}
	FreeLibrary(XQHModule);
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
		XQ_outputLog(("CQXQ遇到严重错误, 这可能是CQXQ本身或是其中的某个插件导致的\n错误信息："s + e.what()).c_str());
		MessageBoxA(nullptr, ("CQXQ遇到严重错误, 这可能是CQXQ本身或是其中的某个插件导致的\n错误信息："s + e.what()).c_str(), "CQXQ 错误", MB_OK);
	}
	return 0;
}

// QQ-群号 缓存 用于发送消息
std::map<std::string, std::string> UserGroupCache;

// 群-群成员json字符串缓存 用于获取群成员列表，群成员信息，缓存时间1小时，遇到群成员变动事件/群名片更改事件刷新
std::map<long long, std::pair<std::string, time_t>> GroupMemberCache;

// 群列表缓存 用于获取群列表，缓存时间1小时，遇到群添加/退出等事件刷新
std::pair<std::string, time_t> GroupListCache;

// 启用事件是否已经被调用，用于在QQ登陆成功以后再调用启用事件
bool EnabledEventCalled = false;

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
			for (const auto& plugin : plugins)
			{
				if (!plugin.events.count(CQ_eventStartup)) continue;
				const auto startup = IntMethod(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventStartup).c_str()));
				if (startup)
				{
					startup();
				}
			}
			return 0;
		}
		if (msgType == XQ_Enable)
		{
			const char* onlineList = XQ_getOnlineList();
			std::string onlineListStr = onlineList ? onlineList : "";

			if (!onlineListStr.empty() && !(onlineListStr[0] == '\r' || onlineListStr[0] == '\n') && !EnabledEventCalled)
			{
				for (const auto& plugin : plugins)
				{
					if (!plugin.enabled) continue;
					if (!plugin.events.count(CQ_eventEnable)) continue;
					const auto enable = IntMethod(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventEnable).c_str()));
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
			if (!EnabledEventCalled && XQ_isEnable())
			{
				for (const auto& plugin : plugins)
				{
					if (!plugin.enabled) continue;
					if (!plugin.events.count(CQ_eventEnable)) continue;
					const auto enable = IntMethod(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventEnable).c_str()));
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
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventDisable)) continue;
				const auto disable = IntMethod(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventDisable).c_str()));
				if (disable)
				{
					disable();
				}
			}
			return 0;
		}
		if (msgType == XQ_FriendMsgEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventPrivateMsg)) continue;
				const auto privMsg = EvPriMsg(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventPrivateMsg).c_str()));
				if (privMsg)
				{
					privMsg(11, 0, atoll(activeQQ), parseToCQCode(msg).c_str(), 0);
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupTmpMsgEvent)
		{
			UserGroupCache[activeQQ] = sourceId;
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventPrivateMsg)) continue;
				const auto privMsg = EvPriMsg(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventPrivateMsg).c_str()));
				if (privMsg)
				{
					privMsg(2, 0, atoll(activeQQ), parseToCQCode(msg).c_str(), 0);
				}
			}
		}
		if (msgType == XQ_GroupMsgEvent)
		{
			UserGroupCache[activeQQ] = sourceId;
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventGroupMsg)) continue;
				const auto groupMsg = EvGroupMsg(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventGroupMsg).c_str()));
				if (groupMsg)
				{
					groupMsg(1, 0, atoll(sourceId), atoll(activeQQ), "", parseToCQCode(msg).c_str(), 0);
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupInviteReqEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventRequest_AddGroup)) continue;
				const auto invited = EvRequestAddGroup(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventRequest_AddGroup).c_str()));
				if (invited)
				{
					Unpack p;
					p.add(XQ_GroupInviteReqEvent);
					p.add(sourceId);
					p.add(activeQQ);
					p.add(rawMsg);
					invited(2, 0, atoll(sourceId), atoll(activeQQ), msg, base64_encode(p.getAll()).c_str());
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupAddReqEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventRequest_AddGroup)) continue;
				const auto addReq = EvRequestAddGroup(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventRequest_AddGroup).c_str()));
				if (addReq)
				{
					Unpack p;
					p.add(XQ_GroupAddReqEvent);
					p.add(sourceId);
					p.add(activeQQ);
					p.add(rawMsg);
					addReq(1, 0, atoll(sourceId), atoll(activeQQ), msg, base64_encode(p.getAll()).c_str());
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupInviteOtherReqEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventRequest_AddGroup)) continue;
				const auto addReq = EvRequestAddGroup(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventRequest_AddGroup).c_str()));
				if (addReq)
				{
					Unpack p;
					p.add(XQ_GroupInviteOtherReqEvent);
					p.add(sourceId);
					p.add(activeQQ);
					p.add(rawMsg);
					addReq(1, 0, atoll(sourceId), atoll(activeQQ), msg, base64_encode(p.getAll()).c_str());
				}
			}
			return 0;
		}
		if (msgType == XQ_FriendAddReqEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventRequest_AddFriend)) continue;
				const auto addReq = EvRequestAddFriend(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventRequest_AddFriend).c_str()));
				if (addReq)
				{
					addReq(1, 0, atoll(activeQQ), msg, activeQQ);
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupBanEvent)
		{
			// TODO: 禁言时间
			XQ_outputLog(msg);
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupBan)) continue;
				const auto ban = EvGroupBan(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupBan).c_str()));
				if (ban)
				{
					ban(2, 0, atoll(sourceId), atoll(activeQQ), atoll(passiveQQ), 60);
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupUnbanEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupBan)) continue;
				const auto ban = EvGroupBan(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupBan).c_str()));
				if (ban)
				{
					ban(1, 0, atoll(sourceId), atoll(activeQQ), atoll(passiveQQ), 0);
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupWholeBanEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupBan)) continue;
				const auto ban = EvGroupBan(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupBan).c_str()));
				if (ban)
				{
					ban(2, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), 0, 0);
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupWholeUnbanEvent)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupBan)) continue;
				const auto ban = EvGroupBan(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupBan).c_str()));
				if (ban)
				{
					ban(1, 0, atoll(sourceId), atoll(activeQQ), 0, 0);
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberIncreaseByApply)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupMemberIncrease)) continue;
				const auto MbrInc = EvGroupMember(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupMemberIncrease).c_str()));
				if (MbrInc)
				{
					MbrInc(1, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ));
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberIncreaseByInvite)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupMemberIncrease)) continue;
				const auto MbrInc = EvGroupMember(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupMemberIncrease).c_str()));
				if (MbrInc)
				{
					MbrInc(2, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ));
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberDecreaseByExit)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupMemberDecrease)) continue;
				const auto event = EvGroupMember(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupMemberDecrease).c_str()));
				if (event)
				{
					event(1, atoi(timeStamp), atoll(sourceId), 0, atoll(passiveQQ));
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupMemberDecreaseByKick)
		{
			GroupMemberCache.erase(atoll(sourceId));
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupMemberDecrease)) continue;
				const auto event = EvGroupMember(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupMemberDecrease).c_str()));
				if (event)
				{
					if (passiveQQ == robotQQ) event(3, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ));
					else event(2, atoi(timeStamp), atoll(sourceId), atoll(activeQQ), atoll(passiveQQ));
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupAdminSet)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupAdmin)) continue;
				const auto event = EvGroupAdmin(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupAdmin).c_str()));
				if (event)
				{
					event(2, atoi(timeStamp), atoll(sourceId), atoll(passiveQQ));
				}
			}
			return 0;
		}
		if (msgType == XQ_GroupAdminUnset)
		{
			for (const auto& plugin : plugins)
			{
				if (!plugin.enabled) continue;
				if (!plugin.events.count(CQ_eventSystem_GroupAdmin)) continue;
				const auto event = EvGroupAdmin(GetProcAddress(plugin.dll, plugin.events.at(CQ_eventSystem_GroupAdmin).c_str()));
				if (event)
				{
					event(1, atoi(timeStamp), atoll(sourceId), atoll(passiveQQ));
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
		XQ_outputLog(("CQXQ遇到严重错误, 这可能是CQXQ本身或是其中的某个插件导致的\n错误信息："s + e.what()).c_str());
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
	return 0;
}


CQAPI(int32_t, CQ_sendPrivateMsg, 16)(int32_t plugin_id, int64_t account, const char* msg)
{
	if (robotQQ.empty()) return 1;
	std::string accStr = std::to_string(account);
	
	if (XQ_ifFriend(robotQQ.c_str(), accStr.c_str()))
	{
		XQ_sendMsg(robotQQ.c_str(), 1, accStr.c_str(), accStr.c_str(), parseFromCQCode(1, accStr.c_str(), msg).c_str(), 0);
	}
	else if (UserGroupCache.count(accStr))
	{
		XQ_sendMsg(robotQQ.c_str(), 4, UserGroupCache[accStr].c_str(), accStr.c_str(), parseFromCQCode(1, accStr.c_str(), msg).c_str(), 0);
	}
	else
	{
		XQ_outputLog(("无法发送消息给QQ" + accStr + ": 找不到可用的发送路径").c_str());
	}
	
	return 0;
}

CQAPI(int32_t, CQ_sendGroupMsg, 16)(int32_t plugin_id, int64_t group, const char* msg)
{
	if (robotQQ.empty()) return 1;
	std::string grpStr = std::to_string(group);
	XQ_sendMsg(robotQQ.c_str(), 2, grpStr.c_str(), robotQQ.c_str(), parseFromCQCode(2, grpStr.c_str(), msg).c_str(), 0);
	return 0;
}

CQAPI(int32_t, CQ_setFatal, 8)(int32_t plugin_id, const char* info)
{
	XQ_outputLog((plugins[plugin_id].file + ": [FATAL] " + info).c_str());
	return 0;
}

CQAPI(const char*, CQ_getAppDirectory, 4)(int32_t plugin_id)
{
	static std::string ppath;
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string pathStr(path);
	ppath = pathStr.substr(0, pathStr.rfind('\\')) + "\\CQPlugins\\config\\" + plugins[plugin_id].file + "\\";
	std::filesystem::create_directories(ppath);
	return ppath.c_str();
}

CQAPI(int64_t, CQ_getLoginQQ, 4)(int32_t plugin_id)
{
	return atoll(robotQQ.c_str());
}

CQAPI(const char*, CQ_getLoginNick, 4)(int32_t plugin_id)
{
	return XQ_getNick(robotQQ.c_str(), robotQQ.c_str());
}

CQAPI(int32_t, CQ_setGroupAnonymous, 16)(int32_t plugin_id, int64_t group, BOOL enable)
{
	XQ_setAnon(robotQQ.c_str(), std::to_string(group).c_str(), enable);
	return 0;
}

CQAPI(int32_t, CQ_setGroupBan, 28)(int32_t plugin_id, int64_t group, int64_t member, int64_t duration)
{
	XQ_shutUp(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(member).c_str(), static_cast<int32_t>(duration));
	return 0;
}

CQAPI(int32_t, CQ_setGroupCard, 24)(int32_t plugin_id, int64_t group, int64_t member, const char* card)
{
	XQ_setGroupCard(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(member).c_str(), card);
	return 0;
}

CQAPI(int32_t, CQ_setGroupKick, 24)(int32_t plugin_id, int64_t group, int64_t member, BOOL reject)
{
	XQ_kickGroupMBR(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(member).c_str(), reject);
	return 0;
}

CQAPI(int32_t, CQ_setGroupLeave, 16)(int32_t plugin_id, int64_t group, BOOL dismiss)
{
	XQ_quitGroup(robotQQ.c_str(), std::to_string(group).c_str());
	return 0;
}

CQAPI(int32_t, CQ_setGroupSpecialTitle, 32)(int32_t plugin_id, int64_t group, int64_t member,
                                            const char* title, int64_t duration)
{
	XQ_outputLog((plugins[plugin_id].file + "调用了不支持的API CQ_setGroupSpecialTitle").c_str());
	return 0;
}

CQAPI(int32_t, CQ_setGroupWholeBan, 16)(int32_t plugin_id, int64_t group, BOOL enable)
{
	XQ_shutUp(robotQQ.c_str(), std::to_string(group).c_str(), "", enable);
	return 0;
}

CQAPI(int32_t, CQ_deleteMsg, 12)(int32_t plugin_id, int64_t msg_id)
{
	XQ_outputLog((plugins[plugin_id].file + "调用了未实现的API CQ_deleteMsg").c_str());
	return 0;
}

CQAPI(const char*, CQ_getFriendList, 8)(int32_t plugin_id, BOOL reserved)
{
	static std::string ret;
	const char* frdLst = XQ_getFriendList(robotQQ.c_str());
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
			const char* nick = XQ_getNick(robotQQ.c_str(), item.c_str());
			tmp.add(nick ? nick : "");
			const char* remarks = XQ_getFriendsRemark(robotQQ.c_str(), item.c_str());
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
	return ret.c_str();
}

CQAPI(const char*, CQ_getGroupInfo, 16)(int32_t plugin_id, int64_t group, BOOL disableCache)
{
	static std::string ret;

	// 判断是否是新获取的列表
	bool newRetrieved = false;
	std::string memberListStr;

	// 判断是否要使用缓存
	if (disableCache || !GroupMemberCache.count(group) || (time(nullptr) - GroupMemberCache[group].second) > 3600)
	{
		newRetrieved = true;
		const char* memberList = XQ_getGroupMemberList_B(robotQQ.c_str(), std::to_string(group).c_str());
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
		const char* groupName = XQ_getGroupName(robotQQ.c_str(), groupStr.c_str());
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
		return ret.c_str();
	}
	catch (std::exception&)
	{
		XQ_outputLog(("警告, 获取群成员列表失败, 正在使用更慢的另一种方法尝试: "s + memberListStr).c_str());
		std::string groupStr = std::to_string(group);
		const char* groupName = XQ_getGroupName(robotQQ.c_str(), groupStr.c_str());
		std::string groupNameStr = groupName ? groupName : "";
		const char* groupNum = XQ_getGroupMemberNum(robotQQ.c_str(), groupStr.c_str());
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
		return ret.c_str();
	}
}

CQAPI(const char*, CQ_getGroupList, 4)(int32_t plugin_id)
{
	static std::string ret;

	const char* groupList = XQ_getGroupList(robotQQ.c_str());
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
			t.add(UTF8toGBK(group["gn"].get<std::string>()));
			Groups.push_back(t);
		}
		for (const auto& group : j["join"])
		{
			Unpack t;
			t.add(group["gc"].get<long long>());
			t.add(UTF8toGBK(group["gn"].get<std::string>()));
			Groups.push_back(t);
		}
		for (const auto& group : j["manage"])
		{
			Unpack t;
			t.add(group["gc"].get<long long>());
			t.add(UTF8toGBK(group["gn"].get<std::string>()));
			Groups.push_back(t);
		}
		p.add(static_cast<int>(Groups.size()));
		for (auto& group : Groups)
		{
			p.add(group);
		}
		ret = base64_encode(p.getAll());
		return ret.c_str();
	}
	catch (std::exception&)
	{
		XQ_outputLog(("警告, 获取群列表失败, 正在使用更慢的另一种方法尝试: "s + groupList).c_str());
		const char* group = XQ_getGroupList_B(robotQQ.c_str());
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
				const char* groupName = XQ_getGroupName(robotQQ.c_str(), item.c_str());
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
		return ret.c_str();
	}
}

CQAPI(const char*, CQ_getGroupMemberInfoV2, 24)(int32_t plugin_id, int64_t group, int64_t account, BOOL disableCache)
{
	static std::string ret;
	// 判断是否是新获取的列表
	bool newRetrieved = false;
	std::string memberListStr;

	// 判断是否要使用缓存
	if (disableCache || !GroupMemberCache.count(group) || (time(nullptr) - GroupMemberCache[group].second) > 3600)
	{
		newRetrieved = true;
		const char* memberList = XQ_getGroupMemberList_B(robotQQ.c_str(), std::to_string(group).c_str());
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
		std::set<long long> admin = j["adm"].get<std::set<long long>>();
		std::map<std::string, std::string> lvlName = j["levelname"].get<std::map<std::string, std::string>>();
		for (auto& item : lvlName)
		{
			lvlName[item.first] = UTF8toGBK(item.second);
		}
		if (!j["members"].count(std::to_string(account))) return "";
		Unpack t;
		t.add(group);
		t.add(account);
		t.add(j["members"].count("nk") ? UTF8toGBK(j["members"]["nk"].get<std::string>()) : "");
		t.add(j["members"].count("cd") ? UTF8toGBK(j["members"]["cd"].get<std::string>()) : "");
		t.add(255);
		t.add(-1);
		t.add("");
		t.add(j["members"].count("jt") ? j["members"]["jt"].get<int>() : 0);
		t.add(j["members"].count("lst") ? j["members"]["lst"].get<int>() : 0);
		t.add(j["members"].count("ll") ? (lvlName.count("lvln" + std::to_string(j["members"]["ll"].get<int>())) ? lvlName["lvln" + std::to_string(j["members"]["ll"].get<int>())] : "") : "");
		t.add(account == owner ? 3 : (admin.count(account) ? 2 : 1));
		t.add("");
		t.add(-1);
		t.add(FALSE);
		t.add(TRUE);
		ret = base64_encode(t.getAll());
		if (newRetrieved) GroupMemberCache[group] = { memberListStr, time(nullptr) };
		return ret.c_str();
	}
	catch (std::exception&)
	{
		XQ_outputLog(("警告, 获取群成员列表失败, 正在使用更慢的另一种方法尝试: "s + memberListStr).c_str());
		std::string grpStr = std::to_string(group);
		std::string accStr = std::to_string(account);
		Unpack p;
		p.add(group);
		p.add(account);
		const char* nick = XQ_getNick(robotQQ.c_str(), accStr.c_str());
		p.add(nick ? nick : "");
		const char* groupCard = XQ_getGroupCard(robotQQ.c_str(), grpStr.c_str(), accStr.c_str());
		p.add(groupCard ? groupCard : "");
		p.add(255);
		p.add(-1);
		p.add("");
		p.add(0);
		p.add(0);
		p.add("");
		const char* admin = XQ_getGroupAdmin(robotQQ.c_str(), std::to_string(group).c_str());
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
		p.add("");
		p.add(0);
		p.add(FALSE);
		p.add(TRUE);
		ret = base64_encode(p.getAll());
		return ret.c_str();
	}
}

CQAPI(const char*, CQ_getGroupMemberList, 12)(int32_t plugin_id, int64_t group)
{
	static std::string ret;
	const char* memberList = XQ_getGroupMemberList_B(robotQQ.c_str(), std::to_string(group).c_str());
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
		std::set<long long> admin = j["adm"].get<std::set<long long>>();
		int mem_num = j["mem_num"].get<int>();
		std::map<std::string, std::string> lvlName = j["levelname"].get<std::map<std::string, std::string>>();
		for (auto& item : lvlName)
		{
			lvlName[item.first] = UTF8toGBK(item.second);
		}
		p.add(mem_num);
		for (const auto& member : j["members"].items())
		{
			long long qq = std::stoll(member.key());
			Unpack t;
			t.add(group);
			t.add(qq);
			t.add(member.value().count("nk") ? UTF8toGBK(member.value()["nk"].get<std::string>()) : "");
			t.add(member.value().count("cd") ? UTF8toGBK(member.value()["cd"].get<std::string>()) : "");
			t.add(255);
			t.add(-1);
			t.add("");
			t.add(member.value().count("jt") ? member.value()["jt"].get<int>() : 0);
			t.add(member.value().count("lst") ? member.value()["lst"].get<int>() : 0);
			t.add(member.value().count("ll") ? (lvlName.count("lvln" + std::to_string(member.value()["ll"].get<int>())) ? lvlName["lvln" + std::to_string(member.value()["ll"].get<int>())]: "") : "");
			t.add(qq == owner ? 3 : (admin.count(qq) ? 2 : 1));
			t.add("");
			t.add(-1);
			t.add(FALSE);
			t.add(TRUE);
			p.add(t);
		}
		GroupMemberCache[group] = { memberListStr, time(nullptr) };
		ret = base64_encode(p.getAll());
		return ret.c_str();
	}
	catch (std::exception&)
	{
		return "";
	}
	return "";
}

CQAPI(const char*, CQ_getCookiesV2, 8)(int32_t plugin_id, const char* domain)
{
	return XQ_getCookies(robotQQ.c_str());
}

CQAPI(const char*, CQ_getCsrfToken, 4)(int32_t plugin_id)
{
	return XQ_getBkn(robotQQ.c_str());
}

CQAPI(const char*, CQ_getImage, 8)(int32_t plugin_id, const char* image)
{
	XQ_outputLog((plugins[plugin_id].file + "调用了未实现的API CQ_getImage").c_str());
	return "";
}

CQAPI(const char*, CQ_getRecordV2, 12)(int32_t plugin_id, const char* file, const char* format)
{
	XQ_outputLog((plugins[plugin_id].file + "调用了未实现的API CQ_getRecordV2").c_str());
	return "";
}

CQAPI(const char*, CQ_getStrangerInfo, 16)(int32_t plugin_id, int64_t account, BOOL cache)
{
	static std::string ret;
	std::string accStr = std::to_string(account);
	Unpack p;
	p.add(account);
	p.add(XQ_getNick(robotQQ.c_str(), accStr.c_str()));
	p.add(255);
	p.add(-1);
	ret = base64_encode(p.getAll());
	return ret.c_str();
}

CQAPI(int32_t, CQ_sendDiscussMsg, 16)(int32_t plugin_id, int64_t group, const char* msg)
{
	XQ_outputLog((plugins[plugin_id].file + "调用了不支持的API CQ_sendDiscussMsg").c_str());
	return 0;
}

CQAPI(int32_t, CQ_sendLikeV2, 16)(int32_t plugin_id, int64_t account, int32_t times)
{
	std::string accStr = std::to_string(account);
	for (int i=0;i!=times;i++)
	{
		XQ_upVote(robotQQ.c_str(), accStr.c_str());
	}
	return 0;
}

CQAPI(int32_t, CQ_setDiscussLeave, 12)(int32_t plugin_id, int64_t group)
{
	XQ_outputLog((plugins[plugin_id].file + "调用了不支持的API CQ_setDiscussLeave").c_str());
	return 0;
}

CQAPI(int32_t, CQ_setFriendAddRequest, 16)(int32_t plugin_id, const char* id, int32_t type, const char* remark)
{
	XQ_handleFriendEvent(robotQQ.c_str(), id, type == 1 ? 10 : 20, remark);
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
	XQ_handleGroupEvent(robotQQ.c_str(), eventType, qq.c_str(), group.c_str(), raw.c_str(), fb_type == 1 ? 10 : 20, reason);
	return 0;
}

CQAPI(int32_t, CQ_setGroupAdmin, 24)(int32_t plugin_id, int64_t group, int64_t account, BOOL admin)
{
	XQ_setAdmin(robotQQ.c_str(), std::to_string(group).c_str(), std::to_string(account).c_str(), admin);
	return 0;
}

CQAPI(int32_t, CQ_setGroupAnonymousBan, 24)(int32_t plugin_id, int64_t group, const char* id, int64_t duration)
{
	XQ_outputLog((plugins[plugin_id].file + "调用了不支持的API CQ_setGroupAnonymousBan").c_str());
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
	XQ_outputLog((plugins[plugin_id].file + ": [" + level + "] [" + type + "] " + content).c_str());
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

