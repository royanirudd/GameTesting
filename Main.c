//http://www.winprog.org/tutorial/window_click.html

#include <stdio.h>
//These pragma are directly communicating to compiler
//Adjusts warning level, make them to 0(or whatever) just for, then include windows.h
#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)
//Then after including windows.h, pop previous W3 back to Wall

#include "Main.h"
//Prototypes in this header

HWND g_GameWindow;

BOOL g_GameIsRunning; //g_ for it is a global var

//This is our backbuffer, the map upon which other frames are called to
GAMEBITMAP g_BackBuffer = { 0 };

//void* Memory; //64 bits in x64

//int main becomes winmain for windows programs, see winmain syntax
// these int and char* are command line args
//int main(int argc, char* argv[])
//{
//	printf("hello");
//	return 0;
//}

//LNK errors are linker errors, means the linker cant find the function


int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
    PSTR CommandLine, int CmdShow)
{

    

    //UNREFERENCED_PARAMETER means we know we are not using those arguements

    //UNREFERENCED_PARAMETER(Instance); 
    //Removed as it is referenced in making of window class
    UNREFERENCED_PARAMETER(PrevInstance);
    UNREFERENCED_PARAMETER(CommandLine);
    UNREFERENCED_PARAMETER(CmdShow);

    //If 1 instance is already running, exit and error message
    if (GameIsAlreadyRunning() == TRUE)
    {
        MessageBoxA(NULL, "Another instance of this program is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    //If cannot create main window, exit
    if (CreateMainGameWindow() != ERROR_SUCCESS)
    {
        goto Exit;
    }

    g_BackBuffer.Bitmapinfo.bmiHeader.biSize = sizeof(g_BackBuffer.Bitmapinfo.bmiHeader);
    
    g_BackBuffer.Bitmapinfo.bmiHeader.biWidth = GAME_RES_WIDTH;

    g_BackBuffer.Bitmapinfo.bmiHeader.biHeight = GAME_RES_HEIGHT;

    g_BackBuffer.Bitmapinfo.bmiHeader.biBitCount = GAME_BPP;

    //This means no compression, expands to 0L
    g_BackBuffer.Bitmapinfo.bmiHeader.biCompression = BI_RGB;

    g_BackBuffer.Bitmapinfo.bmiHeader.biPlanes = 1;

    //Allocating memory for drawing surface assigned to Memory pointer.
    g_BackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (g_BackBuffer.Memory == NULL)
    {
        MessageBox(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }
    //Message loop to send info to .exe

    MSG Message = { 0 }; //a tag message from windows OS

    g_GameIsRunning = TRUE;

    //This nested while loop should represent 1 frame
    while (g_GameIsRunning)
    {
        //Look up PeekMessage vs GetMessage
        while (PeekMessageA(&Message, g_GameWindow, 0, 0, PM_REMOVE))
        {
            DispatchMessageA(&Message);
        }

        ProcessPlayerInput();

        RenderFrameGraphics();

        Sleep(1); //Temp solution, in order to reach 60fps each frame must complete within 16.66 ms. 
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


    //RegisterClassA(&WindowClass);
    //Registering Window Class with windows
    if (RegisterClassExA(&WindowClass) == 0)//Checks registry fail returns 0
    {
        Result = GetLastError();
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        //Avoid Generic fail messages, use GetLastError for specific 
        goto Exit;
    }

    g_GameWindow = CreateWindowExA(WS_EX_CLIENTEDGE, "GAME_WINDOWCLASS",
        "This is the Title", //Window title
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, //Window Style, WS_Overlapped could be 0
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, //Window initialize position&size
        NULL, NULL, GetModuleHandleA(NULL), NULL);
    //Look into CreateWindowExA, used to create every single window in windows

    if (g_GameWindow == NULL)
    {
        Result = GetLastError();
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
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

void ProcessPlayerInput() 
{
    short EscapeKeyDown = GetAsyncKeyState(VK_ESCAPE);
    //This Checks if the player is in focus 
    if (EscapeKeyDown && (GetForegroundWindow() == (g_GameWindow)))
    {
        OutputDebugString("In focus");
        SendMessageA(g_GameWindow, WM_CLOSE, 0, 0);
    }

    //Debug Statements to see when window is in focus
    /*if (GetForegroundWindow() == g_GameWindow)
        OutputDebugStringA("IN FOCUS\n");
    else
        OutputDebugStringA("OUT FOCUS\n");*/
}   

void RenderFrameGraphics(void)
{
    //Get Device Context and remember to release it when finished
    HDC DeviceContext = GetDC(g_GameWindow);

    //Placing the backbuffer ontop of window with reference to device context and address of backbuffer.
    StretchDIBits(DeviceContext, 0, 0, 100, 100, 0, 0, 100, 100, g_BackBuffer.Memory, &g_BackBuffer.Bitmapinfo, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(g_GameWindow, DeviceContext);
}