/*
Copyright (C) 2016 NighthawkSLO <jon.kuhar99@gmail.com>

This work is licensed under the Creative Commons Attribution 4.0 International License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

#include "PluginSlider.h"

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

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
    Measure* measure = new Measure; // classic stuff
    *data = measure;

    measure->skin = RmGetSkin(rm); // writing pointers to the measure so we can use them later
    measure->rm = rm;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
    Measure* measure = (Measure*)data;

    std::wstring type = RmReadString(rm, L"Type", L""); // get the type, does not write to measure YET
    if (type.empty() || (_wcsicmp(type.c_str(), L"M") != 0 && _wcsicmp(type.c_str(), L"X") != 0 && _wcsicmp(type.c_str(), L"Y") != 0)) // return error and removes measure if there is no valid type
    {
        RmLogF(rm, LOG_ERROR, g_ErrEmpty, type);
        RemoveMeasure(measure);
        return;
    }

    measure->PluginX = RmReadInt(rm, L"PluginX", 0); // dimensions
    measure->PluginY = RmReadInt(rm, L"PluginY", 0);
    measure->PluginW = RmReadInt(rm, L"PluginW", 0);
    measure->PluginH = RmReadInt(rm, L"PluginH", 0);

    measure->PluginAction = RmReadString(rm, L"PluginAction", L"", FALSE); // action
    measure->OutOfBoundsAction = RmReadString(rm, L"OutOfBoundsAction", L"", FALSE); // action

    if (type != measure->Type) // Only update if the "Type" option was changed
    {
        measure->Type = type;
        g_Measures.push_back(measure); // adds the measure to the vector

                                       // Start the mouse hook
        if (!g_IsHookActive && // the hook is not active
            !measure->PluginAction.empty() && // it has the action
            g_Measures.size() >= 1) // the measure is in the measure list
        {
            g_Hook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, g_Instance, NULL);
            if (g_Hook)
            {
                g_IsHookActive = true; // it works!
            }
            else
            {
                RmLogF(rm, LOG_ERROR, g_ErrHook, L"start"); // it doesn't work, error
                RemoveMeasure(measure);
            }
        }
    }
}

PLUGIN_EXPORT double Update(void* data)
{
    Measure* measure = (Measure*)data;
    return (double)measure->Value;
}

PLUGIN_EXPORT void Finalize(void* data)
{
    Measure* measure = (Measure*)data;
    RemoveMeasure(measure);
    delete measure;
}

void RemoveMeasure(Measure* measure)
{
    auto remove = [&](std::vector<Measure*>& gMeasures) -> void // remove measures in list
    {
        std::vector<Measure*>::iterator found = std::find(gMeasures.begin(), gMeasures.end(), measure);
        if (found != gMeasures.end())
        {
            gMeasures.erase(found);
        }
    };

    remove(g_Measures);

    if (g_IsHookActive && g_Measures.empty()) // unhooks dll
    {
        while (g_Hook && UnhookWindowsHookEx(g_Hook) == FALSE)
        {
            RmLogF(measure->rm, LOG_ERROR, g_ErrHook, L"stop");
        }

        g_Hook = nullptr;
        g_IsHookActive = false;
    }
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam) // what we get from the mouse hook
{
    if (nCode >= 0)
    {
        KBDLLHOOKSTRUCT* kbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam); // https://msdn.microsoft.com/en-us/library/e0w9f63b.aspx

        auto doAction = [&](bool Type) -> void // has optional parameter - bool "Type"
        {
            for (auto& measure : g_Measures) // for each measure in measures
            {
                int Value; // definitions for value and point
                POINT cursorPos;
                GetCursorPos(&cursorPos); // gets cursor pos
                bool isCurrentlyOutOfBounds = TRUE;

                if (RmReadInt(measure->rm, L"Disabled", 0) == 0 && RmReadInt(measure->rm, L"Paused", 0) == 0) // Only execute if the measure is active
                {
                    if (_wcsicmp(measure->Type.c_str(), L"M") == 0 && !Type) // if type is M and the parameter is "false" (on mouse button up)
                    {
                        RmExecute(measure->skin, measure->PluginAction.c_str()); // just executes the action
                    }
                    else if (_wcsicmp(measure->Type.c_str(), L"X") == 0 && Type) // if type is X and the parameter is "true" (on mouse move)
                    {
                        Value = cursorPos.x; // x coordinate of the mouse
                        if (Value >= measure->PluginX && Value <= (measure->PluginX + measure->PluginW) && // greater than far left point and smaller than far right point
                            cursorPos.y >= measure->PluginY && cursorPos.y <= (measure->PluginY + measure->PluginH)) // but also in the specified y range
                        {
                            isCurrentlyOutOfBounds = FALSE; // it's in bounds
                            if (Value != measure->Value)
                            {
                                measure->OutOfBoundsX = FALSE; 
                                measure->Value = Value; // write to measure
                                RmExecute(measure->skin, measure->PluginAction.c_str()); // execute action
                            }
                        }
                        if (!measure->OutOfBoundsX && isCurrentlyOutOfBounds) // if it's not out of bounds ( OutOfBoundsX = FALSE )
                        {
                            measure->OutOfBoundsX = TRUE; // it's now out of bounds
                            if (Value < measure->PluginX) // it's now either left or right, if it's on the left (x is less than far left side)
                            {
                                measure->Value = measure->PluginX; // this is basically a clamp into the dimensions that are set
                                RmExecute(measure->skin, measure->PluginAction.c_str()); // executes action
                            }
                            else
                            {
                                measure->Value = (measure->PluginX + measure->PluginW);
                                RmExecute(measure->skin, measure->PluginAction.c_str());
                            }
                        }
                    }
                    else if (_wcsicmp(measure->Type.c_str(), L"Y") == 0 && Type) // the same as for the above but for y coordinate
                    {
                        Value = cursorPos.y;
                        if (Value >= measure->PluginY && Value <= (measure->PluginY + measure->PluginH))
                        {
                            if (Value != measure->Value && cursorPos.x >= measure->PluginX && cursorPos.x <= (measure->PluginX + measure->PluginW))
                            {
                                isCurrentlyOutOfBounds = FALSE;
                                measure->OutOfBoundsY = FALSE;
                                measure->Value = Value;
                                RmExecute(measure->skin, measure->PluginAction.c_str());
                            }
                        }
                        if (!measure->OutOfBoundsY && isCurrentlyOutOfBounds) // if it's not out of bounds ( OutOfBoundsX = FALSE )
                        {
                            measure->OutOfBoundsY = TRUE; // it's now out of bounds
                            if (Value < measure->PluginY) // it's now either left or right, if it's on the left (x is less than far left side)
                            {
                                measure->Value = measure->PluginY; // this is basically a clamp into the dimensions that are set
                                RmExecute(measure->skin, measure->PluginAction.c_str()); // executes action
                            }
                            else
                            {
                                measure->Value = (measure->PluginY + measure->PluginH);
                                RmExecute(measure->skin, measure->PluginAction.c_str());
                            }
                        }
                    }
                }
            }
        };

        switch (wParam)
        {
        case WM_LBUTTONUP: // if you click left mouse up
            doAction(FALSE);
            break;

        case WM_MOUSEMOVE: // if you move mouse
            doAction(TRUE);
            break;
        }
    }
    return CallNextHookEx(g_Hook, nCode, wParam, lParam); // standard
}