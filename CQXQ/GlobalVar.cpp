#include "GlobalVar.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <map>

HMODULE hDllModule;

std::vector<native_plugin> plugins;

// XQ根目录, 结尾不带斜杠
std::string rootPath;

// 启用事件是否已经被调用，用于在QQ登陆成功以后再调用启用事件
bool EnabledEventCalled = false;