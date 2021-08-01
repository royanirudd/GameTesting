#pragma once
//This is a input guard, so if another main.h it is only included once


#define GAME_NAME "Game_Name"

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
//Prototype for Window Process

DWORD CreateMainGameWindow(void);
//Prototype for Creating Window Func
//_In_ is SAL2 tells compiler how you intend to use the function.
//_In_ means intended to be an input variable

BOOL GameIsAlreadyRunning(void);


void ProcessPlayerInput(void); //can return void because assume it doesnt fail, dont care about error code