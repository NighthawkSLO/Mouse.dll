/*
Copyright (C) 2016 NighthawkSLO <jon.kuhar99@gmail.com>

This work is licensed under the Creative Commons Attribution 4.0 International License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

#include <Windows.h>
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>
#include<functional>
#include<map>

#include "api\RainmeterAPI.h"

static HINSTANCE g_Instance = nullptr;
static HHOOK g_Hook = nullptr;
static bool g_IsHookActive = false;

LPCWSTR g_ErrEmpty = L"Invalid Type: %s"; // standard errors
LPCWSTR g_ErrHook = L"Could not %s the keyboard hook.";

void RemoveMeasures(void* data);
LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam); // definition for the keyboard hook

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

enum MeasureType
{
	MouseMove,
	LeftMouse,
	RightMouse
};

struct ChildMeasure;



struct ParentMeasure
{
	void* skin;
	void* rm;
    void* data;
	LPCWSTR name;
	ChildMeasure* ownerChild;
	int X; int Y; int W; int H;
	int MouseX = 0;
	int MouseY = 0;

	std::map <MeasureType, std::vector<std::function<void()>>> childFunctions;


	ParentMeasure() : skin(), rm(), data(), name(), ownerChild(), X(), Y(), W(), H() {}
};
std::vector<ParentMeasure*> g_ParentMeasures;

struct ChildMeasure
{
	MeasureType type;
	ParentMeasure* parent;

	LPCWSTR Action;
	bool isEnabled;

	void onLeftMouseUp()
	{
		if(parent)
		RmLogF(parent->rm, LOG_DEBUG, L"Left clicked! ", parent->X);
	}

	void Initialize(void* rm)
	{
		void* skin = RmGetSkin(rm);

		LPCWSTR parentName = RmReadString(rm, L"ParentName", L"");
		if (!*parentName)
		{
			parent = new ParentMeasure;
			parent->name = RmGetMeasureName(rm);
			parent->skin = skin;
			parent->rm = rm;
			parent->ownerChild = this;
			parent->X = RmReadInt(rm, L"PluginX", 0);
			parent->Y = RmReadInt(rm, L"PluginY", 0);
			parent->W = RmReadInt(rm, L"PluginW", 0);
			parent->H = RmReadInt(rm, L"PluginH", 0);
			Action = RmReadString(rm, L"Action", L"");
			isEnabled = (RmReadInt(rm, L"Disabled", 0) == 0 && RmReadInt(rm, L"Paused", 0) == 0);
			g_ParentMeasures.push_back(parent);
		}
		else
		{
			// Find parent using name AND the skin handle to be sure that it's the right one
			std::vector<ParentMeasure*>::const_iterator iter = g_ParentMeasures.begin();
			for (; iter != g_ParentMeasures.end(); ++iter)
			{
				if (_wcsicmp((*iter)->name, parentName) == 0 &&
					(*iter)->skin == skin)
				{
					parent = (*iter);
					return;
				}
			}

			RmLog(LOG_ERROR, L"Slider.dll: Invalid ParentName=");
		}
	}

	std::function<void()> getFunctionFromType(MeasureType& type)
	{
		switch (type)
		{
		case LeftMouse:
			return [&]() {onLeftMouseUp(); };
		}
	}

	void Reload(void* data, void* rm)
	{
		// Read common options
		LPCWSTR type = RmReadString(rm, L"Type", L"");
		if (_wcsicmp(type, L"M") == 0)
		{
			this->type = MouseMove;
		}
		else if (_wcsicmp(type, L"X") == 0)
		{
			if (parent) {
				std::vector<std::function<void()>>& vec = parent->childFunctions[this->type];
				vec.erase(std::remove(vec.begin(), vec.end(), getFunctionFromType(this->type)));
			}

			this->type = LeftMouse;
			if (parent) 
				parent->childFunctions[this->type].push_back(getFunctionFromType(this->type));
		}
		else if (_wcsicmp(type, L"Y") == 0)
		{
			this->type = RightMouse;
		}
		else
		{
			RmLog(LOG_ERROR, L"Slider.dll: Invalid Type=");
		}

		if (!g_IsHookActive && // the hook is not active
			g_ParentMeasures.size() >= 1) // the measure is in the measure list
		{
			g_Hook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, g_Instance, NULL);
			if (g_Hook)
			{
				g_IsHookActive = true; // it works!
			}
			else
			{
				RmLogF(rm, LOG_ERROR, g_ErrHook, L"start"); // it doesn't work, error
				RemoveMeasures(data);
			}
		}
	}

	ChildMeasure() : type(MouseMove), Action(), isEnabled() {}
};


PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	ChildMeasure* child = new ChildMeasure;
	*data = child;

	child->Initialize(rm);
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	if (!parent)
	{
		return;
	}

	child->Reload(data, rm);
}

PLUGIN_EXPORT double Update(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	if (!parent)
	{
		return 0.0;
	}

	switch (child->type)
	{
	case MouseMove:
		return (double)parent->X;

	case LeftMouse:
		return (double)parent->Y;

	case RightMouse:
		return (double)parent->W;
	}

	return 0.0;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	RemoveMeasures(data);
}

void RemoveMeasures(void* data)
{
	ChildMeasure* child = (ChildMeasure*)data;
	ParentMeasure* parent = child->parent;

	if (parent && parent->ownerChild == child)
	{
		delete parent;

		std::vector<ParentMeasure*>::iterator iter = std::find(g_ParentMeasures.begin(), g_ParentMeasures.end(), parent);
		g_ParentMeasures.erase(iter);
	}

	delete child;

	if (g_IsHookActive && g_ParentMeasures.empty()) // unhooks dll
	{
		while (g_Hook && UnhookWindowsHookEx(g_Hook) == FALSE)
		{
			RmLogF(parent->rm, LOG_ERROR, g_ErrHook, L"stop");
		}

		g_Hook = nullptr;
		g_IsHookActive = false;
	}
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT)lParam;
    int x = (int)p->pt.x;
    int y = (int)p->pt.y;
    
    if (nCode >= 0)
    {
        auto doAction = [&]() -> void
        {
            for (auto& m : g_ParentMeasures)
            {
                // RmLogF(m->rm, LOG_DEBUG, L"this");
            }
        };

        switch (wParam)
        {
        case WM_LBUTTONUP: // if you click left mouse up
            for (auto& m : g_ParentMeasures)
            {
                // RmLogF(m->rm, LOG_DEBUG, L"left mouse up, %d, %d",x, y);
                RmLogF(m->rm, LOG_DEBUG, L"Skin X: ", m->X);
				for (auto& func : m->childFunctions[LeftMouse])
				{
					func();
				}
            }
            break;

        case WM_MOUSEMOVE: // if you move mouse
            doAction();
            break;
        }
    }
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
