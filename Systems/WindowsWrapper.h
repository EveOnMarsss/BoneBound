#ifndef WINDOWS_WRAPPER_H
#define WINDOWS_WRAPPER_H

// This file MUST be included BEFORE any other headers
// It renames Windows symbols that conflict with raylib

// First, rename ALL conflicting symbols to avoid conflicts
#define CloseWindow WinCloseWindow
#define ShowCursor WinShowCursor
#define LoadImage WinLoadImage
#define LoadImageA WinLoadImageA
#define LoadImageW WinLoadImageW
#define DrawText WinDrawText
#define DrawTextA WinDrawTextA
#define DrawTextW WinDrawTextW
#define DrawTextEx WinDrawTextEx
#define DrawTextExA WinDrawTextExA
#define DrawTextExW WinDrawTextExW
#define Rectangle WinRectangle
#define PlaySound WinPlaySound
#define GetMessage WinGetMessage
#define SendMessage WinSendMessage
#define PostMessage WinPostMessage
#define CreateWindow WinCreateWindow
#define RegisterClass WinRegisterClass

// Now define Windows controls
#define WIN32_LEAN_AND_MEAN
#define NOGDI  // Prevent GDI conflicts (just in case)
#include <windows.h>

// Immediately undefine the renames so they don't affect our code
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef LoadImageA
#undef LoadImageW
#undef DrawText
#undef DrawTextA
#undef DrawTextW
#undef DrawTextEx
#undef DrawTextExA
#undef DrawTextExW
#undef Rectangle
#undef PlaySound
#undef GetMessage
#undef SendMessage
#undef PostMessage
#undef CreateWindow
#undef RegisterClass

// Include other Windows headers we need
#include <psapi.h>
#include <signal.h>

#pragma comment(lib, "psapi.lib")

#endif