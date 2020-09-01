#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
int WINAPI InitGUI();
void ShowMainWindowAsync();
void DestroyMainWindow();

// GUI
class GUI;
extern GUI MainWindow;