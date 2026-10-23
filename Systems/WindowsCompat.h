#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

// This file should be included AFTER windows.h and BEFORE raylib.h
// It undefines Windows macros that conflict with raylib

#ifdef _WIN32
    // Undefine conflicting Windows functions
    #ifdef CloseWindow
        #undef CloseWindow
    #endif
    
    #ifdef ShowCursor
        #undef ShowCursor
    #endif
    
    #ifdef LoadImage
        #undef LoadImage
    #endif
    
    #ifdef LoadImageA
        #undef LoadImageA
    #endif
    
    #ifdef LoadImageW
        #undef LoadImageW
    #endif
    
    #ifdef DrawText
        #undef DrawText
    #endif
    
    #ifdef DrawTextA
        #undef DrawTextA
    #endif
    
    #ifdef DrawTextW
        #undef DrawTextW
    #endif
    
    #ifdef DrawTextEx
        #undef DrawTextEx
    #endif
    
    #ifdef DrawTextExA
        #undef DrawTextExA
    #endif
    
    #ifdef DrawTextExW
        #undef DrawTextExW
    #endif
    
    #ifdef Rectangle
        #undef Rectangle
    #endif
    
    #ifdef PlaySound
        #undef PlaySound
    #endif
#endif

#endif