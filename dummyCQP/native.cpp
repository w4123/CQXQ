#include <vector>
#include <string>
#include <Windows.h>
#include <thread>
#include <future>
#include "native.h"

using namespace std;

#define XQAPI(ReturnType, Name, Size, ...) using Name##_FUNC = std::function<ReturnType (__stdcall)(__VA_ARGS__)>; \
	Name##_FUNC _##Name; \
	using Name##_TYPE = ReturnType (__stdcall*)(__VA_ARGS__); \
	__pragma(comment(linker, "/EXPORT:" #Name "=_" #Name "@" #Size)) \
    extern "C" __declspec(dllexport) ReturnType __stdcall Name(__VA_ARGS__) 

XQAPI(int32_t, CQ_canSendImage, 4, int32_t plugin_id)
{
	return _CQ_canSendImage(plugin_id);
}

XQAPI(int32_t, CQ_canSendRecord, 4, int32_t plugin_id)
{
	return _CQ_canSendRecord(plugin_id);
}

XQAPI(int32_t, CQ_sendPrivateMsg, 16, int32_t plugin_id, int64_t account, const char* msg)
{
	return _CQ_sendPrivateMsg(plugin_id, account, msg);
}

XQAPI(int32_t, CQ_sendGroupMsg, 16, int32_t plugin_id, int64_t group, const char* msg)
{
	return _CQ_sendGroupMsg(plugin_id, group, msg);
}

XQAPI(int32_t, CQ_setFatal, 8, int32_t plugin_id, const char* info)
{
	return _CQ_setFatal(plugin_id, info);
}

XQAPI(const char*, CQ_getAppDirectory, 4, int32_t plugin_id)
{
	return _CQ_getAppDirectory(plugin_id);
}

XQAPI(int64_t, CQ_getLoginQQ, 4, int32_t plugin_id)
{
	return _CQ_getLoginQQ(plugin_id);
}

XQAPI(const char*, CQ_getLoginNick, 4, int32_t plugin_id)
{
	return _CQ_getLoginNick(plugin_id);
}

XQAPI(int32_t, CQ_setGroupAnonymous, 16, int32_t plugin_id, int64_t group, BOOL enable)
{
	return _CQ_setGroupAnonymous(plugin_id, group, enable);
}

XQAPI(int32_t, CQ_setGroupBan, 28, int32_t plugin_id, int64_t group, int64_t member, int64_t duration)
{
	return _CQ_setGroupBan(plugin_id, group, member, duration);
}

XQAPI(int32_t, CQ_setGroupCard, 24, int32_t plugin_id, int64_t group, int64_t member, const char* card)
{
	return _CQ_setGroupCard(plugin_id, group, member, card);
}

XQAPI(int32_t, CQ_setGroupKick, 24, int32_t plugin_id, int64_t group, int64_t member, BOOL reject)
{
	return _CQ_setGroupKick(plugin_id, group, member, reject);
}

XQAPI(int32_t, CQ_setGroupLeave, 16, int32_t plugin_id, int64_t group, BOOL dismiss)
{
	return _CQ_setGroupLeave(plugin_id, group, dismiss);
}

XQAPI(int32_t, CQ_setGroupSpecialTitle, 32, int32_t plugin_id, int64_t group, int64_t member,
	const char* title, int64_t duration)
{
	return _CQ_setGroupSpecialTitle(plugin_id, group, member, title, duration);
}

XQAPI(int32_t, CQ_setGroupWholeBan, 16, int32_t plugin_id, int64_t group, BOOL enable)
{
	return _CQ_setGroupWholeBan(plugin_id, group, enable);
}

XQAPI(int32_t, CQ_deleteMsg, 12, int32_t plugin_id, int64_t msg_id)
{
	return _CQ_deleteMsg(plugin_id, msg_id);
}

XQAPI(const char*, CQ_getFriendList, 8, int32_t plugin_id, BOOL reserved)
{
	return _CQ_getFriendList(plugin_id, reserved);
}

XQAPI(const char*, CQ_getGroupInfo, 16, int32_t plugin_id, int64_t group, BOOL cache)
{
	return _CQ_getGroupInfo(plugin_id, group, cache);
}

XQAPI(const char*, CQ_getGroupList, 4, int32_t plugin_id)
{
	return _CQ_getGroupList(plugin_id);
}

XQAPI(const char*, CQ_getGroupMemberInfoV2, 24, int32_t plugin_id, int64_t group, int64_t account, BOOL cache)
{
	return _CQ_getGroupMemberInfoV2(plugin_id, group, account, cache);
}

XQAPI(const char*, CQ_getGroupMemberList, 12, int32_t plugin_id, int64_t group)
{
	return _CQ_getGroupMemberList(plugin_id, group);
}

XQAPI(const char*, CQ_getCookiesV2, 8, int32_t plugin_id, const char* domain)
{
	return _CQ_getCookiesV2(plugin_id, domain);
}

XQAPI(const char*, CQ_getCsrfToken, 4, int32_t plugin_id)
{
	return _CQ_getCsrfToken(plugin_id);
}

XQAPI(const char*, CQ_getImage, 8, int32_t plugin_id, const char* image)
{
	return _CQ_getImage(plugin_id, image);
}

XQAPI(const char*, CQ_getRecordV2, 12, int32_t plugin_id, const char* file, const char* format)
{
	return _CQ_getRecordV2(plugin_id, file, format);
}

XQAPI(const char*, CQ_getStrangerInfo, 16, int32_t plugin_id, int64_t account, BOOL cache)
{
	return _CQ_getStrangerInfo(plugin_id, account, cache);
}

XQAPI(int32_t, CQ_sendDiscussMsg, 16, int32_t plugin_id, int64_t group, const char* msg)
{
	return _CQ_sendDiscussMsg(plugin_id, group, msg);
}

XQAPI(int32_t, CQ_sendLikeV2, 16, int32_t plugin_id, int64_t account, int32_t times)
{
	return _CQ_sendLikeV2(plugin_id, account, times);
}

XQAPI(int32_t, CQ_setDiscussLeave, 12, int32_t plugin_id, int64_t group)
{
	return _CQ_setDiscussLeave(plugin_id, group);
}

XQAPI(int32_t, CQ_setFriendAddRequest, 16, int32_t plugin_id, const char* id, int32_t type, const char* remark)
{
	return _CQ_setFriendAddRequest(plugin_id, id, type, remark);
}

XQAPI(int32_t, CQ_setGroupAddRequestV2, 20, int32_t plugin_id, const char* id, int32_t req_type, int32_t fb_type,
	const char* reason)
{
	return _CQ_setGroupAddRequestV2(plugin_id, id, req_type, fb_type, reason);
}

XQAPI(int32_t, CQ_setGroupAdmin, 24, int32_t plugin_id, int64_t group, int64_t account, BOOL admin)
{
	return _CQ_setGroupAdmin(plugin_id, group, account, admin);
}

XQAPI(int32_t, CQ_setGroupAnonymousBan, 24, int32_t plugin_id, int64_t group, const char* id, int64_t duration)
{
	return _CQ_setGroupAnonymousBan(plugin_id, group, id, duration);
}

XQAPI(int32_t, CQ_addLog, 16, int32_t plugin_id, int32_t priority, const char* type, const char* content)
{
	return _CQ_addLog(plugin_id, priority, type, content);
}
// Legacy

XQAPI(const char*, CQ_getCookies, 4, int32_t plugin_id)
{
	return _CQ_getCookies(plugin_id);
}

XQAPI(int32_t, CQ_setGroupAddRequest, 16, int32_t plugin_id, const char* id, int32_t req_type, int32_t fb_type)
{
	return _CQ_setGroupAddRequest(plugin_id, id, req_type, fb_type);
}

XQAPI(int32_t, CQ_sendLike, 12, int32_t plugin_id, int64_t account)
{
	return _CQ_sendLike(plugin_id, account);
}


HMODULE XQHModule = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		XQHModule = LoadLibraryA("CQXQ.XQ.dll");
		if (!XQHModule) XQHModule = LoadLibraryA("CQOQ.OQ.dll");
		if (!XQHModule) throw std::runtime_error("Unable to load compatibility layer");
		_CQ_canSendImage = (CQ_canSendImage_TYPE)GetProcAddress(XQHModule, "CQ_canSendImage");
		_CQ_canSendRecord = (CQ_canSendRecord_TYPE)GetProcAddress(XQHModule, "CQ_canSendRecord");
		_CQ_sendPrivateMsg = (CQ_sendPrivateMsg_TYPE)GetProcAddress(XQHModule, "CQ_sendPrivateMsg");
		_CQ_sendGroupMsg = (CQ_sendGroupMsg_TYPE)GetProcAddress(XQHModule, "CQ_sendGroupMsg");
		_CQ_setFatal = (CQ_setFatal_TYPE)GetProcAddress(XQHModule, "CQ_setFatal");
		_CQ_getAppDirectory = (CQ_getAppDirectory_TYPE)GetProcAddress(XQHModule, "CQ_getAppDirectory");
		_CQ_getLoginQQ = (CQ_getLoginQQ_TYPE)GetProcAddress(XQHModule, "CQ_getLoginQQ");
		_CQ_getLoginNick = (CQ_getLoginNick_TYPE)GetProcAddress(XQHModule, "CQ_getLoginNick");
		_CQ_setGroupAnonymous = (CQ_setGroupAnonymous_TYPE)GetProcAddress(XQHModule, "CQ_setGroupAnonymous");
		_CQ_setGroupBan = (CQ_setGroupBan_TYPE)GetProcAddress(XQHModule, "CQ_setGroupBan");
		_CQ_setGroupCard = (CQ_setGroupCard_TYPE)GetProcAddress(XQHModule, "CQ_setGroupCard");
		_CQ_setGroupKick = (CQ_setGroupKick_TYPE)GetProcAddress(XQHModule, "CQ_setGroupKick");
		_CQ_setGroupLeave = (CQ_setGroupLeave_TYPE)GetProcAddress(XQHModule, "CQ_setGroupLeave");
		_CQ_setGroupSpecialTitle = (CQ_setGroupSpecialTitle_TYPE)GetProcAddress(XQHModule, "CQ_setGroupSpecialTitle");
		_CQ_setGroupWholeBan = (CQ_setGroupWholeBan_TYPE)GetProcAddress(XQHModule, "CQ_setGroupWholeBan");
		_CQ_deleteMsg = (CQ_deleteMsg_TYPE)GetProcAddress(XQHModule, "CQ_deleteMsg");
		_CQ_getFriendList = (CQ_getFriendList_TYPE)GetProcAddress(XQHModule, "CQ_getFriendList");
		_CQ_getGroupInfo = (CQ_getGroupInfo_TYPE)GetProcAddress(XQHModule, "CQ_getGroupInfo");
		_CQ_getGroupList = (CQ_getGroupList_TYPE)GetProcAddress(XQHModule, "CQ_getGroupList");
		_CQ_getGroupMemberInfoV2 = (CQ_getGroupMemberInfoV2_TYPE)GetProcAddress(XQHModule, "CQ_getGroupMemberInfoV2");
		_CQ_getGroupMemberList = (CQ_getGroupMemberList_TYPE)GetProcAddress(XQHModule, "CQ_getGroupMemberList");
		_CQ_getCookiesV2 = (CQ_getCookiesV2_TYPE)GetProcAddress(XQHModule, "CQ_getCookiesV2");
		_CQ_getCsrfToken = (CQ_getCsrfToken_TYPE)GetProcAddress(XQHModule, "CQ_getCsrfToken");
		_CQ_getImage = (CQ_getImage_TYPE)GetProcAddress(XQHModule, "CQ_getImage");
		_CQ_getRecordV2 = (CQ_getRecordV2_TYPE)GetProcAddress(XQHModule, "CQ_getRecordV2");
		_CQ_getStrangerInfo = (CQ_getStrangerInfo_TYPE)GetProcAddress(XQHModule, "CQ_getStrangerInfo");
		_CQ_sendDiscussMsg = (CQ_sendDiscussMsg_TYPE)GetProcAddress(XQHModule, "CQ_sendDiscussMsg");
		_CQ_sendLikeV2 = (CQ_sendLikeV2_TYPE)GetProcAddress(XQHModule, "CQ_sendLikeV2");
		_CQ_setDiscussLeave = (CQ_setDiscussLeave_TYPE)GetProcAddress(XQHModule, "CQ_setDiscussLeave");
		_CQ_setFriendAddRequest = (CQ_setFriendAddRequest_TYPE)GetProcAddress(XQHModule, "CQ_setFriendAddRequest");
		_CQ_setGroupAddRequestV2 = (CQ_setGroupAddRequestV2_TYPE)GetProcAddress(XQHModule, "CQ_setGroupAddRequestV2");
		_CQ_setGroupAdmin = (CQ_setGroupAdmin_TYPE)GetProcAddress(XQHModule, "CQ_setGroupAdmin");
		_CQ_setGroupAnonymousBan = (CQ_setGroupAnonymousBan_TYPE)GetProcAddress(XQHModule, "CQ_setGroupAnonymousBan");
		_CQ_addLog = (CQ_addLog_TYPE)GetProcAddress(XQHModule, "CQ_addLog");
		_CQ_getCookies = (CQ_getCookies_TYPE)GetProcAddress(XQHModule, "CQ_getCookies");
		_CQ_setGroupAddRequest = (CQ_setGroupAddRequest_TYPE)GetProcAddress(XQHModule, "CQ_setGroupAddRequest");
		_CQ_sendLike = (CQ_sendLike_TYPE)GetProcAddress(XQHModule, "CQ_sendLike");
		break;
	case DLL_PROCESS_DETACH:
		FreeLibrary(XQHModule);
		break;
	default:
		break;
	}
	return TRUE;
}