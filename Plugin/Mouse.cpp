#include "Mouse.hpp"
//#include <string>

HWND hMainWindow = NULL;
vector<Measure*> Measures;
bool bHookActive = false;
DWORD dThreadId = NULL;
HANDLE hThread = NULL;

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	Measures.push_back(measure);
	measure->skin = RmGetSkin(rm);
	measure->window = RmGetSkinWindow(rm);

	if (!hMainWindow)
	{
		hMainWindow = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);
	}

	if (!bHookActive)
	{
		if (!StartHook())
		{
			RmLog(LOG_ERROR, L"Mouse.dll: Could not start the mouse hook");
			measure->Remove();
		}
		else
		{
			bHookActive = true;
		}
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	measure->ReadOptions(rm);
	measure->enabled = RmReadInt(rm, L"Disabled", 0) == 0 && RmReadInt(rm, L"Paused", 0) == 0;
	measure->relative = RmReadInt(rm, L"RelativeToSkin", 1) == 1;
	measure->delay = RmReadInt(rm, L"Delay", 16);
}

//PLUGIN_EXPORT LPCWSTR GetString(void* data)
//{
//	std::wstring buf;
//	buf += std::to_wstring(mousePt.x);
//	buf += L", ";
//	buf += std::to_wstring(mousePt.y);
//
//	return buf.c_str();
//}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;

	if (_wcsicmp(args, L"Enable Dragging") == 0)
	{
		if (GetCapture() != measure->window)
			SetCapture(measure->window);
	}
	else if (_wcsicmp(args, L"Disable Dragging") == 0)
	{
		if (GetCapture() == measure->window)
			ReleaseCapture();
	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	measure->Remove();
	delete measure;
}