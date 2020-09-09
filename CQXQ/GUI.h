#pragma once
int __stdcall InitGUI();
void __stdcall ShowMainWindow();
void __stdcall DestroyMainWindow();
void __stdcall UpdateMainWindow();

// GUI
class GUI;
extern GUI MainWindow;