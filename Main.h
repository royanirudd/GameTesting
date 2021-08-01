#pragma once
//This is a input guard, so if another main.h it is only included once


#define GAME_NAME "Game_Name"
//Defining these to match the output type in Bitmapinfo struct
#define GAME_RES_WIDTH (LONG)512

#define GAME_RES_HEIGHT (LONG)288

#define GAME_BPP 32 //This means 32 bits per pixel dont have to worry about 

//Each frame is going to occupy this much memory
#define GAME_DRAWING_AREA_MEMORY_SIZE (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP/8))

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
//Prototype for Window Process

DWORD CreateMainGameWindow(void);
//Prototype for Creating Window Func
//_In_ is SAL2 tells compiler how you intend to use the function.
//_In_ means intended to be an input variable

BOOL GameIsAlreadyRunning(void);

void RenderFrameGraphics(void); //Can return void but never fails

void ProcessPlayerInput(void); //can return void because assume it doesnt fail, dont care about error code

//Struct for background bitmap
typedef struct GAMEBITMAP
{
	BITMAPINFO Bitmapinfo;

	void* Memory;

} GAMEBITMAP;