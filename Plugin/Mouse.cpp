#include "Mouse.hpp"

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
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	measure->Remove();
	delete measure;
}