#pragma once
int __stdcall InitGUI();
void __stdcall ShowMainWindowAsync();
void __stdcall DestroyMainWindow();

// GUI
class GUI;
extern GUI MainWindow;