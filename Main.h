#pragma once
//This is a input guard, so if another main.h it is only included once

#define GAME_NAME "Game_Name"
//Defining these to match the output type in Bitmapinfo struct

#define GAME_RES_WIDTH (LONG)512

#define GAME_RES_HEIGHT (LONG)288

#define GAME_BPP 32 //This means 32 bits per pixel dont have to worry about 

//Each frame is going to occupy this much memory
#define GAME_DRAWING_AREA_MEMORY_SIZE (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP/8))

#define CALCULATE_AVG_FPS_EVERY_X_FRAMES 120

#define TARGET_MICROSECONDS_PER_FRAME 16667ULL //60 FPS target

#define SIMD

//Suits for player bitmap and walking will enumerate it later
#define FACING_DOWN_0 0

#define FACING_DOWN_1 1

#define FACING_DOWN_2 2

#define FACING_LEFT_0 3

#define FACING_LEFT_1 4

#define FACING_LEFT_2 5

#define FACING_RIGHT_0 6

#define FACING_RIGHT_1 7

#define FACING_RIGHT_2 8

#define FACING UP_0 9

#define FACING_UP_1 10

#define FACING_UP 2 11
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

	uint32_t MinimumTimerResolution;

	uint32_t MaximumTimerResolution;

	uint32_t CurrentTimerResolution;

	DWORD HandleCount;

	PROCESS_MEMORY_COUNTERS_EX Meminfo;

	SYSTEM_INFO SystemInfo;

	int64_t CurrentSystemTime;

	int64_t PreviousSystemTime;

	double CPUPercent;

}GAME_PERFORMANCE_DATA;

typedef struct HERO
{
	char Name[12];
	GAMEBITMAP Sprite[12];
	int32_t ScreenPosx;
	int32_t ScreenPosy;
	int32_t HP;
	int32_t Strength;
	int32_t MP;

} HERO;

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);
//Prototype for Window Process

DWORD CreateMainGameWindow(void);
//Prototype for Creating Window Func
//_In_ is SAL2 tells compiler how you intend to use the function.
//_In_ means intended to be an input variable

BOOL GameIsAlreadyRunning(void);

void RenderFrameGraphics(void); //Can return void but never fails

void ProcessPlayerInput(void); //can return void because assume it doesnt fail, dont care about error code

DWORD Load32BppBitmapFromFile(_In_ char* FileName, _Inout_ GAMEBITMAP* GameBitmap);

DWORD InitializeHero(void);

void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitmap, _In_ uint16_t x, _In_ uint16_t y);

#ifdef SIMD
void ClearScreen(_In_ __m128i* Color);
#else
void ClearScreen(_In_ PIXEL32* Color);
#endif

#pragma warning(disable: 4820) //Disabled structure padding warning 
#pragma warning(disable: 5045) //Disabled warning about Spectre/Meltdown CPU vulnarability

