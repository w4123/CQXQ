#include "GlobalVar.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <map>

HMODULE hDllModule;

std::vector<native_plugin> plugins;