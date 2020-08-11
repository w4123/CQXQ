#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>
#include <map>

extern HMODULE hDllModule;

struct native_plugin
{
	int id;
	std::string file;
	std::string name;
	std::string version;
	int version_id;
	std::string author;
	std::string description;
	std::map<int, FARPROC> events;
	std::vector<std::pair<std::string, FARPROC>> menus;
	HMODULE dll;
	bool enabled;

	native_plugin(int i, const std::string& f)
	{
		id = i;
		file = f;
		dll = nullptr;
		enabled = true;
	}
};

extern std::vector<native_plugin> plugins;