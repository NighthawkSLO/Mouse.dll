/*
Copyright (C) 2016 NighthawkSLO <jon.kuhar99@gmail.com>

This work is licensed under the Creative Commons Attribution 4.0 International License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

#ifndef __PLUGIN_HOTKEY_H__
#define __PLUGIN_HOTKEY_H__

#include "Stdafx.h" // all other includes

struct Measure
{
    std::wstring ClickAction; // what will be read from skin
    std::wstring DragAction;
    std::wstring ReleaseAction;

    // bool isBangEnabled;
    bool isEnabled;

	void* skin;
	void* rm;

	Measure() :
        ClickAction(), DragAction(), ReleaseAction(),
        // isBangEnabled(),
        isEnabled(),
		skin(), rm()
	{ }
};

static std::vector<Measure*> g_Measures; // "array with an exception that it automatically handles its own storage requirements in case it grows"
static HINSTANCE g_Instance = nullptr;
static HHOOK g_Hook = nullptr;
static bool g_IsHookActive = false;

void RemoveMeasure(Measure* measure); // definitions for 2 functions
LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam); // definition for the mouse hook

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
{                                                                            // basically this lets you operate outside rainmeter's updates as far as I can tell
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH: // if this is called for attaching dll to a process (it should be)
        g_Instance = hinstDLL; // A handle to the DLL module, need it for the hook

        DisableThreadLibraryCalls(hinstDLL); // Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls
        break;
    }

    return TRUE; // When the system calls the DllMain function with the DLL_PROCESS_ATTACH value, the function returns TRUE if it succeeds or FALSE if initialization fails
}

#endif
