#pragma once
#include <filesystem>
// 按照优先级排序
void sortEvents();
int loadCQPlugin(const std::filesystem::path& file, int id = -1);
void loadOneCQPlugin(const std::filesystem::path& file, int id = -1);
void unloadOneCQPlugin(int id);
void reloadOneCQPlugin(int id);
void loadAllCQPlugin();
void unloadAllCQPlugin();
void reloadAllCQPlugin();