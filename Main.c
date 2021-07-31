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

  
    if (CreateMainGameWindow() != ERROR_SUCCESS)
    {
        goto Exit;
    }
   

    //Message loop to send info to .exe

    MSG Message = { 0 }; //a tag message from windows OS

    while (GetMessageA(&Message, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Message);
        //Translate the message from user
        DispatchMessageA(&Message);
        //Dispatch message to window instance, looks up what procedure belongs to this class

    }
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
        case WM_CLOSE: //Case when we press X button to close, check documentation
        {
            PostQuitMessage(0); //This makes GetMessageA return 0, hence stop the message loop and program.

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
    return 0;
}

DWORD CreateMainGameWindow(void)
{
    DWORD Result = ERROR_SUCCESS;
    //Error code == 0, means nothing went wrong

      //Register Window class, and make window
    WNDCLASSEXA WindowClass = { 0 }; //Always try to initialize variables to avoid garbage value
    //Initialising a Struct to all 0's, use = {0}; instead of = 0;
    HWND WindowHandle = { 0 };

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
    WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
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

    WindowHandle = CreateWindowExA(WS_EX_CLIENTEDGE, "GAME_WINDOWCLASS",
        "LeftNut", //Window title
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, //Window Style, WS_Overlapped could be 0
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120, //Window initialize position&size
        NULL, NULL, GetModuleHandleA(NULL), NULL);
    //Look into CreateWindowExA, used to create every single window in windows

    if (WindowHandle == NULL)
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