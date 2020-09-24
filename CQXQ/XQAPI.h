#include <vector>
#include <exception>
#include <stdexcept>
#include <string>
#include <functional>
#include <Windows.h>
#include "ErrorHandler.h"
#include "GlobalVar.h"

using namespace std;

#define ASYNC_MSG

namespace XQAPI
{

#ifdef XQAPI_IMPLEMENTATION
	static vector<function<void(HMODULE)>> apiFuncInitializers;

	static bool addFuncInit(const function<void(HMODULE)>& initializer) {
		apiFuncInitializers.push_back(initializer);
		return true;
	}

	void initFuncs(const HMODULE& hModule)
	{
		__try
		{
			for (const auto& initializer : apiFuncInitializers) {
				initializer(hModule);
			}
			apiFuncInitializers.clear();
		}
		__except (CQXQUnhandledExceptionFilter(GetExceptionInformation()))
		{
			;
		}
	}

#define XQAPI(Name, ReturnType, ...) using Name##_FUNC = std::function<ReturnType (unsigned char *, __VA_ARGS__)>; \
	Name##_FUNC _##Name; \
	using Name##_TYPE = ReturnType (__stdcall*)(unsigned char *, __VA_ARGS__); \
	template <typename... Args> \
	ReturnType Name(Args&&... args) \
	{ \
		if constexpr (std::is_same_v<const char*, ReturnType>) \
		{ \
			const char* ret = p.push([&args...](int iThread){ return _##Name(AuthCode, std::forward<Args>(args)...); }).get(); \
			if (!ret) return ""; \
			size_t size = *reinterpret_cast<size_t*>(const_cast<char*>(ret)); \
			std::string retStr(ret + 4, size); \
			HeapFree(GetProcessHeap(), 0, reinterpret_cast<void*>(const_cast<char*>(ret))); \
			return delayMemFreeCStr(retStr); \
		} \
		else \
		{ \
			return p.push([&args...](int iThread){ return _##Name(AuthCode, std::forward<Args>(args)...); }).get(); \
		} \
	} \
    static bool _init_##Name = addFuncInit( [] (const auto& hModule) -> void { \
        _##Name = reinterpret_cast<Name##_TYPE>(GetProcAddress(hModule, "S3_Api_" #Name)); \
        if (!_##Name) throw std::runtime_error("Unable to initialize API Function " #Name); \
    });

#else
	void initFuncs(const HMODULE& hModule);
#define XQAPI(Name, ReturnType, ...) template <typename... Args> \
	ReturnType Name (Args&&... args); \

#endif

	XQAPI(SendMsg, void, const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId)

	XQAPI(SendMsgEX, void, const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId, BOOL isAnon)

#ifdef ASYNC_MSG

#ifdef XQAPI_IMPLEMENTATION
	const char* SendMsgEX_V2(const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId, BOOL isAnon, const char* json)
	{
		SendMsgEX(botQQ, msgType, groupId, QQ, content, bubbleId, isAnon);
		return "FORCESUC";
	}
#else
	const char* SendMsgEX_V2(const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId, BOOL isAnon, const char* json);
#endif

#else

#ifdef XQAPI_IMPLEMENTATION
	// 特殊 -> 单独创建线程
	using SendMsgEX_V2_FUNC = std::function<const char*(__stdcall)(unsigned char*, const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId, BOOL isAnon, const char* json)>; \
	SendMsgEX_V2_FUNC _SendMsgEX_V2; 
	using SendMsgEX_V2_TYPE = const char*(__stdcall*)(unsigned char*, const char* botQQ, int32_t msgType, const char* groupId, const char* QQ, const char* content, int32_t bubbleId, BOOL isAnon, const char* json); \
	struct SendMsgEX_V2_Struct
	{
		const char* botQQ;
		int32_t msgType;
		const char* groupId;
		const char* QQ;
		const char* content;
		int32_t bubbleId;
		BOOL isAnon;
		const char* json;
		const char* ret;
	};
	DWORD WINAPI SendMsgEX_V2_ThreadProc(
		_In_ LPVOID lpParameter
	)
	{
		SendMsgEX_V2_Struct* para = (SendMsgEX_V2_Struct*)lpParameter;
		para->ret = _SendMsgEX_V2(AuthCode, para->botQQ, para->msgType, para->groupId, para->QQ, para->content, para->bubbleId, para->isAnon, para->json);
		return 0;
	}
	template <typename... Args>
	const char* SendMsgEX_V2(Args&&... args)
	{ 
		SendMsgEX_V2_Struct para = { args..., nullptr };
		HANDLE t = CreateThread(nullptr, 0, SendMsgEX_V2_ThreadProc, (void*)(&para), 0, nullptr);
		// 保险措施 -> 30秒后强制终止
		if (WaitForSingleObject(t, 30000) == WAIT_OBJECT_0)
		{
			CloseHandle(t);
			if (!para.ret) return "";
			size_t size = *reinterpret_cast<size_t*>(const_cast<char*>(para.ret));
			std::string retStr(para.ret + 4, size);
			HeapFree(GetProcessHeap(), 0, reinterpret_cast<void*>(const_cast<char*>(para.ret)));
			return delayMemFreeCStr(retStr);
		}
		else
		{
			TerminateThread(t, 0);
			CloseHandle(t);
			return "";
		}
	} 
    static bool _init_SendMsgEX_V2 = addFuncInit( [] (const auto& hModule) { 
        _SendMsgEX_V2 = reinterpret_cast<SendMsgEX_V2_TYPE>(GetProcAddress(hModule, "S3_Api_SendMsgEX_V2"));
        if (!_SendMsgEX_V2) throw std::exception("Unable to initialize API Function SendMsgEX_V2"); 
    });
#else
	template <typename... Args>
	const char* SendMsgEX_V2(Args&&... args);
#endif

#endif

	XQAPI(OutPutLog, void, const char* content)

	XQAPI(GetNick, const char*, const char* botQQ, const char* QQ)

	XQAPI(GetGroupAdmin, const char*, const char* botQQ, const char* groupId)

	XQAPI(GetGroupCard, const char*, const char* botQQ, const char* groupId, const char* QQ)

	XQAPI(GetGroupList, const char*, const char* botQQ)

	XQAPI(GetGroupList_B, const char*, const char* botQQ)

	XQAPI(GetGroupName, const char*, const char* botQQ, const char* groupId)

	XQAPI(GetFriendList, const char*, const char* botQQ)

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

	XQAPI(UpLoadPic, const char*, const char* botQQ, int32_t uploadType, const char* targetId, const char* image)

	XQAPI(IfFriend, BOOL, const char* botQQ, const char* QQ)

	XQAPI(GetCookies, const char*, const char* botQQ)

	XQAPI(GetBkn, const char*, const char* botQQ)

	XQAPI(GetVoiLink, const char*, const char* botQQ, const char* GUID)

	XQAPI(WithdrawMsg, const char*, const char* botQQ, const char* groupId, const char* msgNum, const char* msgId)

	XQAPI(WithdrawMsgEX, const char*, const char* botQQ, int32_t type, const char* sourceId, const char* QQ, const char* msgNum, const char* msgId, const char* msgTime)

	XQAPI(GetPicLink, const char*, const char* botQQ, int32_t picType, const char* sourceId, const char* GUID)

	XQAPI(SendXML, void, const char* botQQ, int32_t sendType, int32_t msgType, const char* groupId, const char* QQ, const char* objectMsg, int32_t subType)

	XQAPI(ShakeWindow, BOOL, const char* botQQ, const char* QQ)

	XQAPI(GetAnon, BOOL, const char* botQQ, const char* groupId)

	XQAPI(IsShutUp, BOOL, const char* botQQ, const char* groupId, const char* QQ)

	//XQAPI(GetAge, int32_t, const char* botQQ, const char* QQ)

	//XQAPI(GetGender, int32_t, const char* botQQ, const char* QQ)
#undef XQAPI
}
