#pragma once
//This is a input guard, so if another main.h it is only included once

#define GAME_NAME "Game_Name"
//Defining these to match the output type in Bitmapinfo struct

#define GAME_RES_WIDTH (LONG)512

#define GAME_RES_HEIGHT (LONG)288

#define GAME_BPP 32 //This means 32 bits per pixel dont have to worry about 

//Each frame is going to occupy this much memory
#define GAME_DRAWING_AREA_MEMORY_SIZE (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP/8))

#define CALCULATE_AVG_FPS_EVERY_X_FRAMES 100

#define TARGET_MICROSECONDS_PER_FRAME 16667 //60 FPS target

#define SIMD

typedef LONG(NTAPI* _NtQueryTimerResolution) (OUT PULONG MinimumResolution, OUT PULONG MaximumResolution, OUT PULONG CurrentResolution);

_NtQueryTimerResolution NtQueryTimerResolution;

//Struct for background bitmap
typedef struct GAMEBITMAP
{
	BITMAPINFO Bitmapinfo; //44 bytes
	//Some 4 bytes padding added by compiler
	void* Memory; //8 bytes
					//56 bytes with 4 padding to perfectly allign 
					//by being factor of 8(void* Memory)
} GAMEBITMAP;

//Struct for 32bit Pixel, each 8 bit unsigned char for color.
typedef struct PIXEL32
{
	uint8_t Blue;

	uint8_t Green;

	uint8_t Red;

	uint8_t Alpha;

} PIXEL32;

typedef struct GAME_PERFORMANCE_DATA
{
	uint64_t TotalFramesRendered;

	float RawFPSAvg;

	float CookedFPSAvg;//We want this close to 60

	int64_t PerformanceFrequency;

	MONITORINFO MonitorInfo;

	int32_t MonitorWidth;

	int32_t MonitorHeight;

	BOOL  DisplayDebugInfo;

	LONG MinimumTimerResolution;

	LONG MaximumTimerResolution;

	LONG CurrentTimerResolution;

}GAME_PERFORMANCE_DATA;

typedef struct PLAYER
{
	char Name[12];
	int32_t WorldPosx;
	int32_t WorldPosy;
	int32_t HP;
	int32_t Strength;
	int32_t MP;

} PLAYER;

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
//Prototype for Window Process

DWORD CreateMainGameWindow(void);
//Prototype for Creating Window Func
//_In_ is SAL2 tells compiler how you intend to use the function.
//_In_ means intended to be an input variable

BOOL GameIsAlreadyRunning(void);

void RenderFrameGraphics(void); //Can return void but never fails

void ProcessPlayerInput(void); //can return void because assume it doesnt fail, dont care about error code
#ifdef SIMD
void ClearScreen(_In_ __m128i* Color);
#else
void ClearScreen(_In_ PIXEL32* Color);
#endif

#pragma warning(disable: 4820) //Disabled structure padding warning 
#pragma warning(disable: 5045) //Disabled warning about Spectre/Meltdown CPU vulnarability

