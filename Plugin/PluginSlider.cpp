/*
Copyright (C) 2016 NighthawkSLO <jon.kuhar99@gmail.com>

This work is licensed under the Creative Commons Attribution 4.0 International License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

// Based on code found here: https://github.com/brianferguson/HotKey.dll/blob/master/PluginHotKey/PluginHotKey.cpp

#include <Windows.h>
#include <vector>
#include <algorithm>
#include "api/RainmeterAPI.h"
#include <string>
#include <ctime>
#include <locale>

struct Measure;
void RemoveMeasure(Measure* measure);

static std::vector<Measure*> g_MouseMeasures;
static HINSTANCE g_Instance = nullptr;
static HHOOK g_Hook = nullptr;
static bool g_IsHookActive = false;
LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_Instance = hinstDLL;

		// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls
		DisableThreadLibraryCalls(hinstDLL);
		break;
	}

	return TRUE;
}

enum MOUSEBUTTON {
	LEFT,
	RIGHT,
	MIDDLE
};

struct Measure
{
	Measure() : lastMove(clock())
	{}
	void* skin;
	void* rm;

	std::wstring MouseButton;
	std::wstring ClickAction;
	std::wstring DragAction;
	std::wstring ReleaseAction;
	std::wstring MoveAction;

	bool isEnabled;
	bool isMouseDown;
	bool isRelativeToSkin;
	bool needsFocus;

	int x;
	int y;
	MOUSEBUTTON key;

	clock_t lastMove;
	float moveDelay;
};

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
	measure->DragAction = RmReadString(rm, L"DragAction", L"");
	measure->ReleaseAction = RmReadString(rm, L"ReleaseAction", L"");
	measure->MoveAction = RmReadString(rm, L"MoveAction", L"");
	measure->isEnabled = RmReadInt(rm, L"Enabled", 0) == 0 && RmReadInt(rm, L"Paused", 0) == 0;

	measure->isRelativeToSkin = RmReadInt(rm, L"RelativeToSkin", 1) == 1;
	measure->needsFocus = RmReadInt(rm, L"NeedsFocus", 0) == 1;
	measure->moveDelay = RmReadInt(rm, L"MoveDelay", 20);

	if (_wcsnicmp(measure->MouseButton.c_str(), L"left", 4) == 0)
	{
		measure->key = LEFT;
	}
	else if (_wcsnicmp(measure->MouseButton.c_str(), L"right", 5) == 0)
	{
		measure->key = RIGHT;
	}
	else if (_wcsnicmp(measure->MouseButton.c_str(), L"middle", 6) == 0)
	{
		measure->key = MIDDLE;
	}
	else
	{
		RmLogF(rm, LOG_ERROR, L"Slider.dll: MouseButton=%s not valid", measure->MouseButton);
		return;
	}

	if(std::find(g_MouseMeasures.begin(), g_MouseMeasures.end(), measure) == g_MouseMeasures.end())
	{
		g_MouseMeasures.push_back(measure);
	}

	// Start the keyboard hook
	if (!g_IsHookActive && !g_MouseMeasures.empty())
	{
		g_Hook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, g_Instance, NULL);
		if (g_Hook)
		{
			g_IsHookActive = true;
			RmLogF(rm, LOG_DEBUG, L"MouseHook Started");
		}
		else
		{
			RmLogF(rm, LOG_ERROR, L"Could not start the mouse hook");
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
	void* rm = measure->rm;
	std::vector<Measure*>::iterator found = std::find(g_MouseMeasures.begin(), g_MouseMeasures.end(), measure);
	if (found != g_MouseMeasures.end())
	{
		g_MouseMeasures.erase(found);
	}

	if (g_IsHookActive && g_MouseMeasures.empty())
	{
		while (g_Hook && UnhookWindowsHookEx(g_Hook) == FALSE)
		{
			RmLogF(rm, LOG_ERROR, L"Could not stop the mouse hook");
		}

		g_Hook = nullptr;
		g_IsHookActive = false;
		RmLogF(rm, LOG_DEBUG, L"MouseHook Stopeed");
	}
}

std::wstring wstringReplace(std::wstring wstr, std::wstring oldstr, std::wstring newstr)
{
	std::wstring lowerWstr = wstr;
	std::transform(lowerWstr.begin(), lowerWstr.end(), lowerWstr.begin(), tolower);
	size_t pos = 0;
	while ((pos = lowerWstr.find(oldstr, pos)) != std::wstring::npos) {
		wstr.replace(pos, oldstr.length(), newstr);
		pos += newstr.length();
	}
	return wstr;
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode >= 0)
	{

		MSLLHOOKSTRUCT* msllStruct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
		POINT mouse = msllStruct->pt; // this fetches the cursor position
		for (auto& measure : g_MouseMeasures)
		{
			if (!measure->isEnabled) continue;
			if(measure->needsFocus && RmGetSkinWindow(measure->rm) != GetActiveWindow())
			{
				continue;
			}
			LONG x = mouse.x; LONG y = mouse.y;
			if (measure->isRelativeToSkin)
			{
				RECT rect;
				GetWindowRect(RmGetSkinWindow(measure->rm), &rect);
				x -= rect.left; y -= rect.top;
			}
			std::wstring str_x = std::to_wstring(x); std::wstring str_y = std::to_wstring(y);
			if (wParam == WM_LBUTTONDOWN && measure->key == LEFT || wParam == WM_RBUTTONDOWN && measure->key == RIGHT || wParam == WM_MBUTTONDOWN && measure->key == MIDDLE)
			{
				if (!measure->isMouseDown)
				{
					measure->isMouseDown = true;
					if (!measure->ClickAction.empty()) 
					{
						RmExecute(measure->skin, wstringReplace(wstringReplace(measure->ClickAction, L"$mousex$", str_x), L"$mousey$", str_y).c_str());
					}
				}
			}
			else if (wParam == WM_LBUTTONUP && measure->key == LEFT || wParam == WM_RBUTTONUP && measure->key == RIGHT || wParam == WM_MBUTTONUP && measure->key == MIDDLE)
			{
				if (measure->isMouseDown)
				{
					measure->isMouseDown = false;
					if (!measure->ReleaseAction.empty()) 
					{
						RmExecute(measure->skin, wstringReplace(wstringReplace(measure->ReleaseAction, L"$mousex$", str_x), L"$mousey$", str_y).c_str());
					}
				}
			}
			else if(wParam == WM_MOUSEMOVE)
			{
				auto time = clock();
				float dt = (float)(time - measure->lastMove);
				if (dt > measure->moveDelay && (x != measure->x || y != measure->y))
				{
					if (!measure->MoveAction.empty())
					{
						RmExecute(measure->skin, wstringReplace(wstringReplace(measure->MoveAction, L"$mousex$", str_x), L"$mousey$", str_y).c_str());
					}
					if (measure->isMouseDown && !measure->DragAction.empty())
					{
						RmExecute(measure->skin, wstringReplace(wstringReplace(measure->DragAction, L"$mousex$", str_x), L"$mousey$", str_y).c_str());
					}
					measure->lastMove = clock();
				}
			}
			measure->x = x; measure->y = y;
		}
	}

	return CallNextHookEx(g_Hook, nCode, wParam, lParam);
}