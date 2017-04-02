#include "Threading.h"

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	Measures.push_back(measure);
	measure->skin = RmGetSkin(rm);
	measure->window = RmGetSkinWindow(rm);
	hMainWindow = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);

	if (!bThreadActive)
	{
		hThread = CreateThread(0, 0, HookThread, 0, 0, &dThreadId);
		bThreadActive = true;
	}

	if (!hThread && !hHook)
	{
		RmLog(LOG_ERROR, L"Mouse.dll: Could not start the mouse hook");
		RemoveMeasure(measure);
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;

	measure->Move = RmReadString(rm, L"MouseMoveAction", L"");
	measure->LeftDown = RmReadString(rm, L"LeftMouseDownAction", L"");
	measure->LeftUp = RmReadString(rm, L"LeftMouseUpAction", L"");
	measure->LeftDrag = RmReadString(rm, L"LeftMouseDragAction", L"");
	measure->RightDown = RmReadString(rm, L"RightMouseDownAction", L"");
	measure->RightUp = RmReadString(rm, L"RightMouseUpAction", L"");
	measure->RightDrag = RmReadString(rm, L"RightMouseDragAction", L"");
	measure->MiddleDown = RmReadString(rm, L"MiddleMouseDownAction", L"");
	measure->MiddleUp = RmReadString(rm, L"MiddleMouseUpAction", L"");
	measure->MiddleDrag = RmReadString(rm, L"MiddleMouseDragAction", L"");

	measure->enabled = RmReadInt(rm, L"Disabled", 0) == 0 && RmReadInt(rm, L"Paused", 0) == 0;

	measure->relative = RmReadInt(rm, L"RelativeToSkin", 1) == 1;
	measure->needsFocus = RmReadInt(rm, L"NeedsFocus", 0) == 1;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	RemoveMeasure(measure);
	delete measure;
}

const void RemoveMeasure(Measure* measure)
{
	vector<Measure*>::iterator found = find(Measures.begin(), Measures.end(), measure);
	if (found != Measures.end())
	{
		Measures.erase(found);
	}

	if (Measures.empty() && bThreadActive)
	{
		if (dThreadId && hThread)
		{
			PostThreadMessage(dThreadId, WM_QUIT, 0, 0);
			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
		}
		hThread = NULL;
		dThreadId = NULL;
		bThreadActive = false;
	}
}