//http://www.winprog.org/tutorial/window_click.html


//These pragma are directly communicating to compiler
//Adjusts warning level, make them to 0(or whatever) just for, then include windows.h
#pragma warning(push, 3)
#include <stdio.h>
#include <windows.h>
#include <emmintrin.h>
#include <Psapi.h>

#pragma warning(pop)
//Then after including windows.h, pop previous W3 back to Wall

#include <stdint.h>

#include "Main.h"

#pragma comment(lib, "Winmm.lib")
//Prototypes in this header

HWND g_GameWindow;

BOOL g_GameIsRunning; //g_ for it is a global var

//This is our backbuffer, the map upon which other frames are called to
GAMEBITMAP g_BackBuffer = { 0 };

GAME_PERFORMANCE_DATA g_PerformanceData;

HERO g_Player;

BOOL g_WindowHasFocus = TRUE;
//Windows API requires structs to be initilized with size parameter

//void* Memory; //64 bits in x64

//int main becomes winmain for windows programs, see winmain syntax
// these int and char* are command line args
//int main(int argc, char* argv[])
//{
//	printf("hello");
//	return 0;
//}

//LNK errors are linker errors, means the linker cant find the function


int WINAPI WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PrevInstance,
    _In_ PSTR CommandLine, _In_ int CmdShow)
{

    //UNREFERENCED_PARAMETER means we know we are not using those arguements

    UNREFERENCED_PARAMETER(Instance); 
    //Removed as it is referenced in making of window class
    UNREFERENCED_PARAMETER(PrevInstance);
    UNREFERENCED_PARAMETER(CommandLine);
    UNREFERENCED_PARAMETER(CmdShow);

    MSG Message = { 0 }; //a tag message from windows OS

    int64_t FrameStart = 0;

    int64_t FrameEnd = 0;

    int64_t ElapsedMicroSeconds = 0;

    int64_t ElapsedMicroSecondsAccumulatorRaw = 0;

    int64_t ElapsedMicroSecondsAccumulatorCooked = 0;
    //Finding and importing a function from nt.dll
    HMODULE NtDllModuleHandle = NULL;

    FILETIME ProcessCreationTime = { 0 };

    FILETIME ProcessExitTime = { 0 };

    int64_t CurrentUserCPUTime = 0;

    int64_t CurrentKernelCPUTime = 0;

    int64_t PreviousUserCPUTime = 0;

    int64_t  PrevousKernelCPUTime = 0;

    HANDLE ProcessHandle = GetCurrentProcess();

    HANDLE ThreadHandle = GetCurrentThread();

    if ((NtDllModuleHandle = GetModuleHandleA("ntdll.dll")) == NULL) 
    {
        MessageBoxA(NULL, "Couldn't load ntdll.dll", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(NtDllModuleHandle,
        "NtQueryTimerResolution")) == NULL)
    {
        MessageBoxA(NULL, "Couldn't find the NtQueryTimerResolution function in ntdll.dll", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    //Calling the NtQuery imported from ntdll.dll
    NtQueryTimerResolution(&g_PerformanceData.MinimumTimerResolution, &g_PerformanceData.MaximumTimerResolution, &g_PerformanceData.CurrentTimerResolution);

    //No return value, therefore cant fail
    GetSystemInfo(&g_PerformanceData.SystemInfo);

    GetSystemTimeAsFileTime((FILETIME*)&g_PerformanceData.PreviousSystemTime);


    //If 1 instance is already running, exit and error message
    if (GameIsAlreadyRunning() == TRUE)
    {
        MessageBoxA(NULL, "Another instance of this program is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //This only returns TIMERR_NOCANDO if it is not 1ms
    //This sets the timer to 1 only for lifetime of game, no need to cleanup on exit
    //from winmm.lib, this doesnt work all the time forsome reason. idk it works for now
    if (timeBeginPeriod(1) == TIMERR_NOCANDO)
    {
        MessageBoxA(NULL, "Failed to set global timer resolution!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //Process doesnt run any code, just hold the threads
    //Sets priority high, realtime is high but can cause important threads to not work
    if (SetPriorityClass(ProcessHandle, HIGH_PRIORITY_CLASS) == 0)
    {
        MessageBoxA(NULL, "Failed to set process priority!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //Threads are the containers that run the code
    // This sets process priority to high as well
    if (SetThreadPriority(ThreadHandle, THREAD_PRIORITY_HIGHEST) == 0)
    {
        MessageBoxA(NULL, "Failed to set thread priority!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //If cannot create main window, exit
    if (CreateMainGameWindow() != ERROR_SUCCESS)
    {
        goto Exit;
    }
    QueryPerformanceFrequency((LARGE_INTEGER*)&g_PerformanceData.PerformanceFrequency);
    //Only needs to be called once, Frequency is set on boot of pc
    g_PerformanceData.DisplayDebugInfo = TRUE;

    g_BackBuffer.Bitmapinfo.bmiHeader.biSize = sizeof(g_BackBuffer.Bitmapinfo.bmiHeader);
    
    g_BackBuffer.Bitmapinfo.bmiHeader.biWidth = GAME_RES_WIDTH;

    g_BackBuffer.Bitmapinfo.bmiHeader.biHeight = GAME_RES_HEIGHT;

    g_BackBuffer.Bitmapinfo.bmiHeader.biBitCount = GAME_BPP;

    //This means no compression, expands to 0L
    g_BackBuffer.Bitmapinfo.bmiHeader.biCompression = BI_RGB;

    g_BackBuffer.Bitmapinfo.bmiHeader.biPlanes = 1;

    //Allocating memory for drawing surface assigned to Memory pointer.
    g_BackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    //No need to free memory, Windows should cleanup all resources on exit
    
    if (g_BackBuffer.Memory == NULL)
    {
        MessageBoxA(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //Setting all the virtual alloc memory to white
    memset(g_BackBuffer.Memory, 0, GAME_DRAWING_AREA_MEMORY_SIZE);

    //NtQueryTimerResolution

    if (InitializeHero() != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Failed to initialize hero!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //Message loop to send info to .exe

    g_GameIsRunning = TRUE;

    //This nested while loop should represent 1 frame
    while (g_GameIsRunning)
    {
        //Starts timing the begining of a frame
        QueryPerformanceCounter((LARGE_INTEGER*)&FrameStart);

        //Look up PeekMessage vs GetMessage
        while (PeekMessageA(&Message, g_GameWindow, 0, 0, PM_REMOVE))
        {
            DispatchMessageA(&Message);
        }

        ProcessPlayerInput();

        RenderFrameGraphics();

        QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);
        //End timing at end of frame
        ElapsedMicroSeconds =  FrameEnd - FrameStart;

        ElapsedMicroSeconds *= 1000000;

        ElapsedMicroSeconds /= g_PerformanceData.PerformanceFrequency;

        g_PerformanceData.TotalFramesRendered++;

        ElapsedMicroSecondsAccumulatorRaw += ElapsedMicroSeconds;

        //While FPS is below target, reroute the thread to sleep less
        while (ElapsedMicroSeconds < (int64_t)TARGET_MICROSECONDS_PER_FRAME)
        {
            ElapsedMicroSeconds = FrameEnd - FrameStart;

            ElapsedMicroSeconds *= 1000000;

            ElapsedMicroSeconds /= g_PerformanceData.PerformanceFrequency;

            QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

            //While there is enough time for frame to sleep the minimum cycle without wasting time
            //this says if we are less than 75% through frame then sleep otherwise dont
            if (ElapsedMicroSeconds < (TARGET_MICROSECONDS_PER_FRAME *0.75f))
            {
                Sleep(1); //Could be anywhere from 1ms to a full system timer tick
            }
            
            
        }
        //Sleep(1); //Temp solution, in order to reach 60fps each frame must complete within 16.66 ms. 

        ElapsedMicroSecondsAccumulatorCooked += ElapsedMicroSeconds;

        //Calculate Debugg info every CALCULATE_FPS_EVERY_X_Frames
        if (g_PerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES == 0)
        {           
            GetSystemTimeAsFileTime((FILETIME*)&g_PerformanceData.CurrentSystemTime);

            GetProcessTimes(ProcessHandle, &ProcessCreationTime,
                &ProcessExitTime,
                (FILETIME*)&CurrentKernelCPUTime,
                (FILETIME*)&CurrentUserCPUTime);


            g_PerformanceData.CPUPercent = (CurrentKernelCPUTime - PrevousKernelCPUTime) +\
                (CurrentUserCPUTime - PreviousUserCPUTime);
            
            g_PerformanceData.CPUPercent /= (g_PerformanceData.CurrentSystemTime - g_PerformanceData.PreviousSystemTime);
            
            g_PerformanceData.CPUPercent /= g_PerformanceData.SystemInfo.dwNumberOfProcessors;

            g_PerformanceData.CPUPercent *= 100;
            
       
            //Remember **Process Handle is not the same as Module Handle**
            GetProcessHandleCount(ProcessHandle, &g_PerformanceData.HandleCount);


            K32GetProcessMemoryInfo(ProcessHandle, (PROCESS_MEMORY_COUNTERS*)&g_PerformanceData.Meminfo, sizeof(g_PerformanceData.Meminfo));
            
            g_PerformanceData.RawFPSAvg = (1.0f / ((ElapsedMicroSecondsAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001));

            g_PerformanceData.CookedFPSAvg = (1.0f / ((ElapsedMicroSecondsAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001));

            //This is Debug stats on console output
           /* char FrameStats[128] = { 0 };

            _snprintf_s(FrameStats, _countof(FrameStats), _TRUNCATE,
                "Avg Microseconds/Frame raw %llu\tAvg FPS Cooked: %01f\tAvg FPS Raw%01f\n",
                AverageMicroSecondsPerFrameCooked,
                g_PerformanceData.CookedFPSAvg,
                1.0f/ ((ElapsedMicroSecondsPerFrameAccumulatorRaw/60)* 0.000001));

            OutputDebugStringA(FrameStats);*/

            ElapsedMicroSecondsAccumulatorRaw = 0;

            ElapsedMicroSecondsAccumulatorCooked = 0;

            PrevousKernelCPUTime = CurrentKernelCPUTime;
            PreviousUserCPUTime = CurrentKernelCPUTime;
            g_PerformanceData.PreviousSystemTime = g_PerformanceData.CurrentSystemTime;
            
        }
    
    }

    ////Rudamentary message loop, should nest it done above
    //while (GetMessageA(&Message, NULL, 0, 0) > 0)
    //{
    //    TranslateMessage(&Message);
    //    //Translate the message from user
    //    DispatchMessageA(&Message);
    //    //Dispatch message to window instance, looks up what procedure belongs to this class

    //}

Exit:  //Placeholder
    return 0;
}



//Main Window Procedure
//Need to protype it, done above

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
//LRESULT CALLBACK MainWindowProc(
//    HWND hwnd,        // handle to window
//    UINT uMsg,        // message identifier
//    WPARAM wParam,    // first message parameter
//    LPARAM lParam)    // second message parameter
{

    LRESULT Result = 0;


    switch (Message)
    {
        //CANT CODE WITHIN SWITCHBLOCK THATS NOT IN CASES
        case WM_CLOSE: //Case when we press X button to close, check documentation
        {

            g_GameIsRunning = FALSE;

            PostQuitMessage(0); 
            //This makes GetMessageA return 0, hence stop the message loop and program.

            break;
        }

        case WM_ACTIVATE:
        {
            if (WParam == 0)
            {
                //Our Window has lost focus
                g_WindowHasFocus = FALSE;
            }
            else
            {
                //Our window gained focus
                ShowCursor(FALSE);

                g_WindowHasFocus = TRUE;
            }

            break;
        }
        default:
        {
            Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
        }
    }

    ////This switch classifies the type of message
    ////Else it returns the default message, 1000's of useless messages
    ////We can put in our own logic for certain messages
    ////Cleaner Code would only have 1 return at any point in switch board
    //switch (Message)
    //{
    //    case WM_CREATE:
    //        // Initialize the window. 
    //        return 0;

    //    case WM_PAINT:
    //        // Paint the window's client area. 
    //        return 0;

    //    case WM_SIZE:
    //        // Set the size and position of the window. 
    //        return 0;

    //    case WM_DESTROY:
    //        // Clean up window-specific data objects. 
    //        return 0;

    //        // 
    //        // Process other messages. 
    //        // 

    //    default: return DefWindowProc(WindowHandle, Message, WParam, LParam);
    //}
    return Result; //This needs to return the defwindowproc, otherwise it forces CreateWindowsExA to return NULL.
}

DWORD CreateMainGameWindow(void)
{
    DWORD Result = ERROR_SUCCESS;
    //Error code == 0, means nothing went wrong

      //Register Window class, and make window
    WNDCLASSEXA WindowClass = { 0 }; //Always try to initialize variables to avoid garbage value
    //Initialising a Struct to all 0's, use = {0}; instead of = 0;

    WindowClass.cbSize = sizeof(WNDCLASSEXA); //cbSize is Count in bytes
    //This is the defining size of window, or I could initialize as sizeof()
    WindowClass.style = 0;
    //**Styles for each diff type of app, look into it**
    WindowClass.lpfnWndProc = MainWindowProc;//Asking for long pointer to Window procedure function, done below

    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;

    WindowClass.hInstance = GetModuleHandleA(NULL);
    //Could be instance, but this with NULL says 'give my own executables handle' 
    //Handle to window instance, where is resides
    WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    //This is a built in windows icon, you can customise it later
    WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
    //Another GUI icon reference
    WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    WindowClass.hbrBackground = CreateSolidBrush(RGB(255,0,255));
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = "GAME_WINDOWCLASS";
    //Long pointer to String for class name

    //Temp solution to zoomed resolutions only works on windows 10, solved by enabling per monitor dpi
    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    //RegisterClassA(&WindowClass);
    //Registering Window Class with windows
    if (RegisterClassExA(&WindowClass) == 0)//Checks registry fail returns 0
    {
        Result = GetLastError();
        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        //Avoid Generic fail messages, use GetLastError for specific 
        goto Exit;
    }

    g_GameWindow = CreateWindowExA(0, "GAME_WINDOWCLASS",
        "This is the Title", //Window title
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, //Window Style, WS_Overlapped could be 0
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, //Window initialize position&size
        NULL, NULL, GetModuleHandleA(NULL), NULL);
    //Look into CreateWindowExA, used to create every single window in windows

    if (g_GameWindow == NULL)
    {
        Result = GetLastError();
        MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    //This gets info about users monitor
    g_PerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoA(MonitorFromWindow(g_GameWindow, MONITOR_DEFAULTTOPRIMARY), &g_PerformanceData.MonitorInfo) == 0)
    {
        Result = ERROR_MONITOR_NO_DESCRIPTOR;

        MessageBoxA(NULL, "Failed to collect monitor info!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //However, this returns a digital res after the scale is applied (Mine is 125%)
    //Fixed by using SetProcessDPi
    g_PerformanceData.MonitorWidth = g_PerformanceData.MonitorInfo.rcMonitor.right - g_PerformanceData.MonitorInfo.rcMonitor.left;

    g_PerformanceData.MonitorHeight = g_PerformanceData.MonitorInfo.rcMonitor.bottom - g_PerformanceData.MonitorInfo.rcMonitor.top;

    //Applies certain properties to window, still leaves white pixels change clientedge flag on CreateWindow
    if (SetWindowLongPtrA(g_GameWindow, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW) == 0)
    {
        Result = GetLastError();

        goto Exit;
    }

    //Sets window to fullscreen,borderless but leaves a small pixel bar on left
    if (SetWindowPos(g_GameWindow, 
        HWND_TOP, 
        g_PerformanceData.MonitorInfo.rcMonitor.left,
        g_PerformanceData.MonitorInfo.rcMonitor.top,
        g_PerformanceData.MonitorWidth,
        g_PerformanceData.MonitorHeight, 
        SWP_FRAMECHANGED | SWP_FRAMECHANGED) == 0)
    {
        Result = GetLastError();

        goto Exit;
    }
    //ShowWindow(WindowHandle, TRUE); //shouldnt be necessary
   //ShowWindow command to true refering to window handle

Exit: //Goto placeholder
    return Result;
}

BOOL GameIsAlreadyRunning(void)
{
    HANDLE Mutex = NULL;
    //Mutex is a piece of memory used to gate access to resources. Serializing access to .exe
    Mutex = CreateMutexA(NULL,FALSE, GAME_NAME, "_GameMutex");

    //If Mutex has been created, then a file is already running, thus returns gameisrunning to true
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void ProcessPlayerInput(void) 
{
    //Dont process input if window is not on focus
    if (g_WindowHasFocus == FALSE)
    {
        return;
    }

    int16_t EscapeKeyDown = GetAsyncKeyState(VK_ESCAPE);
    int16_t DebugKeyDown = GetAsyncKeyState(VK_F1);
    int16_t LeftKeyDown = GetAsyncKeyState(VK_LEFT) | GetAsyncKeyState('A');
    int16_t RightKeyDown = GetAsyncKeyState(VK_RIGHT) | GetAsyncKeyState('D');
    int16_t UpKeyDown = GetAsyncKeyState(VK_UP) | GetAsyncKeyState("W");
    int16_t DownKeyDown = GetAsyncKeyState(VK_DOWN) | GetAsyncKeyState("S");
    //This makes sure it doesnt spam when holding down key, holds value of last frame
    static int16_t DebugKeyWasDown;
  /*  static int16_t LeftKeyWasDown;
    static int16_t RightKeyWasDown;*/

    //This Checks if the player is in focus
    //This now becomes redundant as we only process input when in focus
    //if (g_WindowHasFocus)
    if (EscapeKeyDown)
    {
        SendMessageA(g_GameWindow, WM_CLOSE, 0, 0);
    }

    if (DebugKeyDown && !DebugKeyWasDown)
    {
        g_PerformanceData.DisplayDebugInfo = !g_PerformanceData.DisplayDebugInfo;
    }
    if (LeftKeyDown)
    {
        if (g_Player.ScreenPosx > 0 )
        {
            g_Player.ScreenPosx--;
        }
    }
    if (RightKeyDown)
    {
        if (g_Player.ScreenPosx < (GAME_RES_WIDTH -16))
        {
            g_Player.ScreenPosx++;
        }
    }
    if (DownKeyDown)
    {
        if (g_Player.ScreenPosy < (GAME_RES_HEIGHT - 16))
        {
            g_Player.ScreenPosy++;
        }
            
    }
    if (UpKeyDown)
    {
        if (g_Player.ScreenPosy > 0)
        {
            g_Player.ScreenPosy--;
        }
    }
    //Reset after frame
    DebugKeyWasDown = DebugKeyDown;
    

    //Debug Statements to see when window is in focus
    /*if (GetForegroundWindow() == g_GameWindow)
        OutputDebugStringA("IN FOCUS\n");
    else
        OutputDebugStringA("OUT FOCUS\n");*/
}   


void RenderFrameGraphics(void)
{
    ////Painting backbuffer all white, because memset does all bits
    //memset(g_BackBuffer.Memory, 0xff, GAME_DRAWING_AREA_MEMORY_SIZE);
    
    
    //Array of 4 Pixels, 4x32bits = 128, same as PIXEL32 Quadpixel[4]
    
#ifdef SIMD
    __m128i QuadPixel = { 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff,
     0x7f, 0x00, 0x00, 0xff , 0x7f, 0x00, 0x00, 0xff };
    ClearScreen(&QuadPixel);
    
#else
    PIXEL32 Pixel = { 0x7f, 0x00, 0x00, 0xff };

    ClearScreen(&Pixel);
#endif
    //Paints half of backbuffer to Pixel value
    /*for (int x = 0; x < GAME_RES_HEIGHT*GAME_RES_WIDTH; x++)
    {
        
        By casting to PIXER32 Ptr it knows to go 4 pixels wide. (32bits)
        memcpy_s((PIXEL32*)g_BackBuffer.Memory + x, sizeof(Pixel), &Pixel, sizeof(PIXEL32));

    }*/
    //Making coordinate system for back buffer
    int32_t ScreenX = g_Player.ScreenPosx;

    int32_t ScreenY = g_Player.ScreenPosy;

    int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - \
        (GAME_RES_WIDTH * ScreenY) + ScreenX;

    //Through the use of intrinsics, we can lay down more than one at a time
    //Drawing a white box 16x16 pixels
    for (int32_t y=0; y<16; y++)
    {
        for (int32_t x=0; x<16; x++)
        {
            memset((PIXEL32*)g_BackBuffer.Memory + (uintptr_t)StartingScreenPixel + x - ((uintptr_t)GAME_RES_WIDTH * y),
                0xFF,
                sizeof(PIXEL32));
        }
    }

    //Get Device Context and remember to release it when finished
    HDC DeviceContext = GetDC(g_GameWindow);

    //Placing the backbuffer ontop of window with reference to device context and address of backbuffer.
    StretchDIBits(DeviceContext, 
        0, 
        0,
        g_PerformanceData.MonitorWidth,
        g_PerformanceData.MonitorHeight,
        0,
        0,
        GAME_RES_WIDTH,
        GAME_RES_HEIGHT,
        g_BackBuffer.Memory,
        &g_BackBuffer.Bitmapinfo,
        DIB_RGB_COLORS, SRCCOPY);

    if (g_PerformanceData.DisplayDebugInfo == TRUE)
    {
        SelectObject(DeviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));

        char DebugTextBuffer[64] = { 0 };

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Raw:       %0.1f", g_PerformanceData.RawFPSAvg);

        TextOutA(DeviceContext, 0, 0, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Cooked:    %0.1f", g_PerformanceData.CookedFPSAvg);

        TextOutA(DeviceContext, 0, 13, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Min Timer Res: %0.2f", g_PerformanceData.MinimumTimerResolution/10000.0f);

        TextOutA(DeviceContext, 0, 26, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Max Timer Res: %0.2f", g_PerformanceData.MaximumTimerResolution/10000.0f);

        TextOutA(DeviceContext, 0, 39, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Current Timer Res: %0.2f", g_PerformanceData.CurrentTimerResolution/10000.0f);

        TextOutA(DeviceContext, 0, 52, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Handles: %lu", g_PerformanceData.HandleCount);

        TextOutA(DeviceContext, 0, 65, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Memory: %zu KB", g_PerformanceData.Meminfo.PrivateUsage / 1024);

        TextOutA(DeviceContext, 0, 78, DebugTextBuffer, (int)strlen(DebugTextBuffer));
    
        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "CPU %.02f %%", g_PerformanceData.CPUPercent);

        TextOutA(DeviceContext, 0, 91, DebugTextBuffer, (int)strlen(DebugTextBuffer));
    
    }

    ReleaseDC(g_GameWindow, DeviceContext);
}
#ifdef SIMD
//Function to clear whole screen to given color and forcing it to inline
__forceinline void ClearScreen(_In_ __m128i* Color)
{
    for (int x = 0; x < GAME_RES_HEIGHT*GAME_RES_WIDTH; x += 4)
    {
        //using SIMD, does it 4 times as fast
        _mm_store_si128((PIXEL32*)g_BackBuffer.Memory + x, *Color);
    }
}

#else
__forceinline void ClearScreen(_In_ PIXEL32* Pixel)
{
    for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x++)
    {
        memcpy((PIXEL32*)g_BackBuffer.Memory + x, Pixel, sizeof(PIXEL32));
    }
}
#endif

//This function will open a file and populate a pointer to memory with that bitmap
DWORD Load32BppBitmapFromFile(_In_ char* FileName, _Inout_ GAMEBITMAP* GameBitmap)
{
//    //Read into Bitmap Filetype for header info
//    DWORD Error = ERROR_SUCCESS;
//
//    HANDLE FileHandle = INVALID_HANDLE_VALUE;
//
//    WORD BitmapHeader = 0; //unsigned short 16 bits
//
//    DWORD PixelDataOffset = 0; 
//    //For the readfileEx, this will return pointer to number it could read
//    DWORD NumberofBytesRead = 2;
//
//    //this function returns an open handle
//    FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ,
//        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//    if (FileHandle == INVALID_HANDLE_VALUE)
//    {
//        Error = GetLastError();
//
//        goto Exit;
//    }
//    //Want to read first few bytes of file to get header info
//    
//    if (ReadFile(FileHandle, &BitmapHeader, 2,&NumberofBytesRead,NULL) == 0)
//    {
//        Error = GetLastError();
//        goto Exit;
//    }
//    
//    //This is reading the first 2 and confirming it is a BM type
//    if (BitmapHeader != 0x4D42) //this is "BM" backwards
//    {
//        Error = ERROR_FILE_INVALID;
//        goto Exit;
//    }
//    //If i call readfile again, it will continue from where i left 
//    //But i want to skip to 10 bytes from start of file
//    if (SetFilePointer(FileHandle, 0xA, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
//    {
//        Error = GetLastError();
//        goto Exit;
//    }
//
//    if (ReadFile(FileHandle, &PixelDataOffset, sizeof(DWORD), &NumberofBytesRead, NULL) == 0)
//    {
//        Error = GetLastError();
//        goto Exit;
//    }
//
//
//    if (SetFilePointer(FileHandle, 0xE, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
//    {
//        Error = GetLastError();
//
//        goto Exit;
//    }
//
//    if (ReadFile(FileHandle, &GameBitmap->Bitmapinfo.bmiHeader, sizeof(BITMAPINFOHEADER), &NumberofBytesRead, NULL) == 0)
//    {
//        Error = GetLastError();
//
//        goto Exit;
//    }
//    //Trying different allocation methods
//    //GameBitmap->Memory = VirtualAlloc(NULL, GameBitmap->Bitmapinfo.bmiHeader.biSizeImage, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
//    //(GameBitmap->Memory) =(void*) malloc(GameBitmap->Bitmapinfo.bmiHeader.biSizeImage);
//    GameBitmap->Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, GameBitmap->Bitmapinfo.bmiHeader.biSizeImage);
//    
//    if (GameBitmap->Memory == NULL)
//    {
//        Error = ERROR_NOT_ENOUGH_MEMORY;
//        goto Exit;
//    }
//
//    //memcpy_s(GameBitmap->Memory, GameBitmap->Bitmapinfo.bmiHeader.biSizeImage,  )
//    //have to remove file cursor to begining of memory
//
//    if(SetFilePointer(FileHandle, PixelDataOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
//        {
//            Error = GetLastError();
//            goto Exit;
//        }
//    
//    if (ReadFile(FileHandle, &GameBitmap->Memory, GameBitmap->Bitmapinfo.bmiHeader.biSizeImage, &NumberofBytesRead, NULL) == 0)
//    {
//        Error = GetLastError();
//        goto Exit;
//    }
//Exit:
//
//    if (FileHandle && (FileHandle != INVALID_HANDLE_VALUE))
//    {
//        //If the file is open and function is succeeded, close file upon exit
//        CloseHandle(FileHandle);
//    }
//    
//    return Error; //BIG ASS FAIL OVER HERE IGNORE THIS, WILL LOOKBACKLATER
    DWORD Error = ERROR_SUCCESS;

    HANDLE FileHandle = INVALID_HANDLE_VALUE;

    WORD BitmapHeader = 0;

    DWORD PixelDataOffset = 0;

    DWORD NumberOfBytesRead = 2;

    if ((FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, &BitmapHeader, 2, &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (BitmapHeader != 0x4d42) // "BM" backwards
    {
        Error = ERROR_FILE_INVALID;

        goto Exit;
    }

    if (SetFilePointer(FileHandle, 0xA, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, &PixelDataOffset, sizeof(DWORD), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (SetFilePointer(FileHandle, 0xE, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, &GameBitmap->Bitmapinfo.bmiHeader, sizeof(BITMAPINFOHEADER), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

    if ((GameBitmap->Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, GameBitmap->Bitmapinfo.bmiHeader.biSizeImage)) == NULL)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;

        goto Exit;
    }

    if (SetFilePointer(FileHandle, PixelDataOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        goto Exit;
    }

    if (ReadFile(FileHandle, GameBitmap->Memory, GameBitmap->Bitmapinfo.bmiHeader.biSizeImage, &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        goto Exit;
    }

Exit:

    if (FileHandle && (FileHandle != INVALID_HANDLE_VALUE))
    {
        CloseHandle(FileHandle);
    }

    return(Error);
}


//Initializing hero character
DWORD InitializeHero(void)
{
    DWORD Error = ERROR_SUCCESS;

    g_Player.ScreenPosx = 25;

    g_Player.ScreenPosy = 25;

    if (Error = Load32BppBitmapFromFile("C:\\Users\\aniru\\source\\repos\\GameTesting\\Assets\\main_char.bmpx", &g_Player.Sprite[0]) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Couldn't load Bitmap Sprite!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

Exit:
    return Error;
}