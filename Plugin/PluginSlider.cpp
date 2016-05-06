/*
Copyright (C) 2016 NighthawkSLO <jon.kuhar99@gmail.com>

This work is licensed under the Creative Commons Attribution 4.0 International License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

#include <Windows.h>
#include <math.h>
#include <string>
#include <vector>
#include <thread>

#include "api\RainmeterAPI.h"

struct Measure
{
    std::wstring MouseButton;
    std::wstring ClickAction;
    std::wstring HoldAction;
    std::wstring DragAction;
    std::wstring ReleaseAction;

    bool isEnabled;
    bool isMouseDown;
    bool isHeld;
    bool isRelativeToSkin;

    int x;
    int y;
    int key;

    int counter;
    double delay;

    void* skin;
    void* rm;

    Measure() :
        MouseButton(),
        ClickAction(), HoldAction(), DragAction(), ReleaseAction(),
        isEnabled(), isMouseDown(false), isHeld(false), isRelativeToSkin(),
        x(), y(), key(),
        counter(0), delay(300.0),
        skin(), rm()
    { }
};

static std::vector<Measure*> g_Measures;
static bool g_ThreadActive = false;
void MouseThread();
void RemoveMeasure(Measure* measure);

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
    Measure* measure = new Measure;
    *data = measure;

    measure->skin = RmGetSkin(rm);
    measure->rm = rm;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
    Measure* measure = (Measure*)data;

    measure->MouseButton = RmReadString(rm, L"MouseButton", L"Left");

    measure->ClickAction = RmReadString(rm, L"ClickAction", L"");
    measure->HoldAction = RmReadString(rm, L"HoldAction", L"");
    measure->DragAction = RmReadString(rm, L"DragAction", L"");
    measure->ReleaseAction = RmReadString(rm, L"ReleaseAction", L"");
    measure->isEnabled = RmReadInt(rm, L"Enabled", 0) == 0 && RmReadInt(rm, L"Paused", 0) == 0;

    measure->isRelativeToSkin = RmReadInt(rm, L"RelativeToSkin", 1) == 1;

    measure->delay = RmReadDouble(rm, L"HoldDelay", 300.0);

    if (_wcsnicmp(measure->MouseButton.c_str(), L"left", 4) == 0)
    {
        measure->key = VK_LBUTTON;
    }
    else if (_wcsnicmp(measure->MouseButton.c_str(), L"right", 5) == 0)
    {
        measure->key = VK_RBUTTON;
    }
    else if (_wcsnicmp(measure->MouseButton.c_str(), L"middle", 6) == 0)
    {
        measure->key = VK_MBUTTON;
    }
    else
    {
        RmLogF(rm, LOG_ERROR, L"Slider.dll: MouseButton=%s not valid", measure->MouseButton);
        return;
    }

    if (std::find(g_Measures.begin(), g_Measures.end(), measure) == g_Measures.end())
    {
        g_Measures.push_back(measure);
    }

    /*
    This starts the mouse hook
    */
    if (g_Measures.size() >= 1 && !g_ThreadActive)
    {
        g_ThreadActive = true;
        std::thread Thread(MouseThread);
        Thread.detach();
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
    std::vector<Measure*>::iterator found = std::find(g_Measures.begin(), g_Measures.end(), measure);
    if (found != g_Measures.end())
    {
        g_Measures.erase(found);
    }
    if (g_Measures.empty())
    {
        g_ThreadActive = false;
    }
}

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

void MouseThread()
{
    while (g_ThreadActive)
    {
        POINT p;
        if (GetCursorPos(&p) && g_Measures.size() >= 1)
        {
            for (auto& measure : g_Measures)
            {
                if (measure->isEnabled)
                {
                    int x = p.x; int y = p.y;
                    if (measure->isRelativeToSkin)
                    {
                        RECT rect;
                        GetWindowRect(RmGetSkinWindow(measure->rm), &rect);
                        x -= rect.left; y -= rect.top;
                    }
                    std::string str_x = std::to_string(x); std::string str_y = std::to_string(y);
                    if ((GetAsyncKeyState(measure->key) != 0) && !measure->isMouseDown)
                    {
                        measure->isMouseDown = true; measure->counter = 0;
                        RmExecute(measure->skin, wstringReplace(wstringReplace(measure->ClickAction, "$mouseX$", str_x), "$mouseY$", str_y).c_str());
                    }
                    if ((x != measure->x || y != measure->y) && measure->isMouseDown)
                    {
                        RmExecute(measure->skin, wstringReplace(wstringReplace(measure->DragAction, "$mouseX$", str_x), "$mouseY$", str_y).c_str());
                    }
                    if ((GetAsyncKeyState(measure->key) == 0) && measure->isMouseDown)
                    {
                        measure->isMouseDown = false; measure->counter = 0; measure->isHeld = false;
                        RmExecute(measure->skin, wstringReplace(wstringReplace(measure->ReleaseAction, "$mouseX$", str_x), "$mouseY$", str_y).c_str());
                    }
                    if (measure->isMouseDown)
                    {
                        if ((measure->counter == (int)round(measure->delay / 20)) && !measure->isHeld)
                        {
                            measure->isHeld = true;
                            RmExecute(measure->skin, wstringReplace(wstringReplace(measure->HoldAction, "$mouseX$", str_x), "$mouseY$", str_y).c_str());
                        }
                        measure->counter++;
                    }
                    measure->x = x; measure->y = y;
                }
            }
            Sleep(20);
        }
    }
}