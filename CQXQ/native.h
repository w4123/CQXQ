#pragma once

#define CQAPI(ReturnType, Name, Size) __pragma(comment(linker, "/EXPORT:" #Name "=_" #Name "@" #Size))\
 extern "C" __declspec(dllexport) ReturnType __stdcall Name

typedef int32_t (__stdcall* IntMethod)();
typedef const char* (__stdcall* StringMethod)();
typedef int32_t (__stdcall* FuncInitialize)(int32_t);

typedef int32_t (__stdcall* EvPriMsg)(int32_t, int32_t, int64_t, const char*, int32_t);
typedef int32_t (__stdcall* EvGroupMsg)(int32_t, int32_t, int64_t, int64_t, const char*, const char*, int32_t);
typedef int32_t (__stdcall* EvDiscussMsg)(int32_t, int32_t, int64_t, int64_t, const char*, int32_t);
typedef int32_t (__stdcall* EvGroupAdmin)(int32_t, int32_t, int64_t, int64_t);
typedef int32_t (__stdcall* EvGroupMember)(int32_t, int32_t, int64_t, int64_t, int64_t);
typedef int32_t (__stdcall* EvGroupBan)(int32_t, int32_t, int64_t, int64_t, int64_t, int64_t);
typedef int32_t (__stdcall* EvRequestAddGroup)(int32_t, int32_t, int64_t, int64_t, const char*, const char*);
typedef int32_t (__stdcall* EvRequestAddFriend)(int32_t, int32_t, int64_t, const char*, const char*);
typedef int32_t (__stdcall* EvFriendAdd)(int32_t, int32_t, int64_t);


#define XQ_StartupComplete 10000
#define XQ_Exit 10001
#define XQ_Load 12000
#define XQ_Enable 12001
#define XQ_Disable 12002


#define XQ_FriendMsgEvent 1
#define XQ_GroupMsgEvent 2
#define XQ_DiscussMsgEvent 3
#define XQ_GroupTmpMsgEvent 4
#define XQ_DiscussTmpMsgEvent 5

#define XQ_GroupSelfMsgEvent 10

#define XQ_FriendAddReqEvent 101
#define XQ_FriendAddedEvent 100

#define XQ_GroupInviteReqEvent 214
#define XQ_GroupAddReqEvent 213
#define XQ_GroupInviteOtherReqEvent 215 

#define XQ_GroupMemberIncreaseByApply 212
#define XQ_GroupMemberIncreaseByInvite 219
#define XQ_GroupMemberDecreaseByExit 201
#define XQ_GroupMemberDecreaseByKick 202

#define XQ_GroupBanEvent 203
#define XQ_GroupUnbanEvent 204
#define XQ_GroupWholeBanEvent 205
#define XQ_GroupWholeUnbanEvent 206

#define XQ_GroupAdminSet 210
#define XQ_GroupAdminUnset 211

#define XQ_LogInComplete 1101

#define XQ_groupCardChange 217

#define CQ_eventPrivateMsg 21
#define CQ_eventGroupMsg 2
#define CQ_eventDiscussMsg 4
#define CQ_eventGroupUpload 11
#define CQ_eventSystem_GroupAdmin 101
#define CQ_eventSystem_GroupMemberDecrease 102
#define CQ_eventSystem_GroupMemberIncrease 103
#define CQ_eventSystem_GroupBan 104
#define CQ_eventFriend_Add 201
#define CQ_eventRequest_AddFriend 301
#define CQ_eventRequest_AddGroup 302
#define CQ_eventStartup 1001
#define CQ_eventExit 1002
#define CQ_eventEnable 1003
#define CQ_eventDisable 1004