/*
Copyright (C) 2016 NighthawkSLO <jon.kuhar99@gmail.com>

This work is licensed under the Creative Commons Attribution 4.0 International License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

#include "PluginSlider.h"

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

    measure->ClickAction = RmReadString(rm, L"ClickAction", L"");
    measure->DragAction = RmReadString(rm, L"DragAction", L"");
    measure->ReleaseAction = RmReadString(rm, L"ReleaseAction", L"");
    // measure->isBangEnabled = RmReadInt(rm, L"EnableBangs", 1) == 1;
    measure->isEnabled = RmReadInt(rm, L"Enabled", 0) == 0 && RmReadInt(rm, L"Paused", 0) == 0;

    // g_Measures.push_back(measure);

    if (std::find(g_Measures.begin(), g_Measures.end(), measure) == g_Measures.end())
    {
        g_Measures.push_back(measure); // adds the measure to the vector
    }

    // Start the mouse hook
    if (!g_IsHookActive && // the hook is not active
        g_Measures.size() >= 1) // the measure is in the measure list
    {
        g_Hook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, g_Instance, NULL);
        if (g_Hook)
        {
            g_IsHookActive = true; // it works!
            // RmLogF(rm, LOG_DEBUG, L"got to here");
        }
        else
        {
            RmLogF(rm, LOG_ERROR, L"PluginSlider.dll - Could not start the mouse hook"); // it doesn't work, error
            RemoveMeasure(measure);
        }
    }
}

PLUGIN_EXPORT double Update(void* data)
{
    Measure* measure = (Measure*)data;
    return 0.0;
}

PLUGIN_EXPORT void Finalize(void* data)
{
    Measure* measure = (Measure*)data;
    RemoveMeasure(measure);
    delete measure;
}

void RemoveMeasure(Measure* measure)
{
    std::vector<Measure*>::iterator found = std::find(g_Measures.begin(), g_Measures.end(), measure); // removes measure from the vector
    if (found != g_Measures.end())
    {
        g_Measures.erase(found);
    }

    if (g_IsHookActive && g_Measures.empty()) // unhooks dll if there are no measures and the hook is active
    {
        while (g_Hook && UnhookWindowsHookEx(g_Hook) == FALSE)
        {
            RmLogF(measure->rm, LOG_ERROR, L"PluginSlider.dll - Could not stop the mouse hook");
        }

        g_Hook = nullptr;
        g_IsHookActive = false;
    }
}

bool isPressed = FALSE;

std::wstring wstringReplace(std::wstring wstr, std::string oldstr, std::string newstr)
{
    std::string str(wstr.begin(), wstr.end());
    size_t pos = 0;
    while ((pos = str.find(oldstr, pos)) != std::string::npos) {
        str.replace(pos, oldstr.length(), newstr);
        pos += newstr.length();
    }
    std::wstring ws;
    return ws.assign(str.begin(), str.end());
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam) // what we get from the mouse hook
{
    if (nCode >= 0)
    {
        auto doAction = [&](int Type) -> void
        {
            PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT)lParam;
            for (auto& measure : g_Measures)
            {
                if (measure->isEnabled)
                {
                    // RmLogF(measure->rm, LOG_DEBUG, L"got to here #2 %s", wstringReplace(L"TEST", "ST", "LLING"));
                    std::string x = std::to_string((int)p->pt.x); std::string y = std::to_string((int)p->pt.y);

                    std::wstring Action1 = wstringReplace(wstringReplace(measure->ClickAction, "$mouseX$", x), "$mouseY$", y);
                    std::wstring Action2 = wstringReplace(wstringReplace(measure->DragAction, "$mouseX$", x), "$mouseY$", y);
                    std::wstring Action3 = wstringReplace(wstringReplace(measure->ReleaseAction, "$mouseX$", x), "$mouseY$", y);

                    if      (Type == 1) { RmExecute(measure->skin, Action1.c_str());}
                    else if (Type == 2) { RmExecute(measure->skin, Action2.c_str()); }
                    else if (Type == 3) { RmExecute(measure->skin, Action3.c_str()); }
                }
            }
        };

        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            isPressed = TRUE;
            doAction(1);
            break;
        case WM_MOUSEMOVE:
            if (isPressed) { doAction(2); }
            break;
        case WM_LBUTTONUP:
            isPressed = FALSE;
            doAction(3);
            break;
        }
    }
    return CallNextHookEx(g_Hook, nCode, wParam, lParam); // standard
}