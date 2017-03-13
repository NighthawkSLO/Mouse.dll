#define DEFAULT_DELAY 20
#define TIMEOUT_DURATION 300
#define TIMEOUT_INTERVAL 700

#include "Mouse.h"
#include "api/RainmeterAPI.h"

void RemoveMeasure(Measure* measure)
{
	if (measure->isTimerActive) DeleteTimerQueueTimer(g_TimerQueue, measure->timer, NULL);

	std::vector<Measure*>::iterator found = std::find(g_Measures.begin(), g_Measures.end(), measure);
	if (found != g_Measures.end()) g_Measures.erase(found);

	if (g_Measures.empty() && g_Hook)
	{
		if (g_IsTimeoutTimerActive)
		{
			DeleteTimerQueueTimer(g_TimerQueue, g_TimeoutTimer, NULL);
			g_IsTimeoutTimerActive = false;
		}
		
		if (g_IsTimerActive && !DeleteTimerQueue(g_TimerQueue)) RmLog(LOG_ERROR, L"Mouse.dll: Could not stop the timer queue");
		g_IsTimerActive = false;

		while (g_IsHookActive && UnhookWindowsHookEx(g_Hook) == FALSE) RmLog(LOG_ERROR, L"Mouse.dll: Could not stop the mouse hook");

		g_Hook = nullptr;
		g_IsHookActive = false;
	}
}

VOID CALLBACK TimeoutCheckProc(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	if (!SendMessageTimeout(g_mainWindow, WM_NULL, NULL, NULL, SMTO_ABORTIFHUNG, TIMEOUT_DURATION, NULL))
	{
		while (g_IsHookActive && UnhookWindowsHookEx(g_Hook) == FALSE) RmLog(LOG_ERROR, L"Mouse.dll: Could not stop the mouse hook");

		g_Hook = nullptr;
		g_IsHookActive = false;
	}
	else
	{
		if (!g_IsHookActive)
		{
			g_Hook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, g_Instance, NULL);
			if (g_Hook) g_IsHookActive = true;
			else RmLog(LOG_ERROR, L"Mouse.dll: Could not start the mouse hook");
		}
	}
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	g_Measures.push_back(measure);
	measure->skin = RmGetSkin(rm);
	measure->window = RmGetSkinWindow(rm);
	g_mainWindow = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);
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

	if (!g_IsHookActive)
	{
		g_Hook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, g_Instance, NULL);
		if (g_Hook)
		{
			g_IsHookActive = true;

			if (!g_IsTimerActive)
			{
				g_TimerQueue = CreateTimerQueue();
				if (g_TimerQueue == NULL) RmLog(LOG_ERROR, L"Mouse.dll: Could not start the timer queue");
				g_IsTimerActive = true;

				if (CreateTimerQueueTimer(&g_TimeoutTimer, g_TimerQueue, (WAITORTIMERCALLBACK)TimeoutCheckProc, 0, 0,
					(DWORD)TIMEOUT_INTERVAL, 0) == FALSE) RmLog(LOG_ERROR, L"Mouse.dll: Could not start the timeout timer");
				g_IsTimeoutTimerActive = true;
			}
		}
		else
		{
			RmLog(LOG_ERROR, L"Mouse.dll: Could not start the mouse hook");
			RemoveMeasure(measure);
		}
	}

	if (!measure->Move.empty() || !measure->LeftDrag.empty() || !measure->RightDrag.empty() || !measure->MiddleDrag.empty())
	{
		int delay = RmReadInt(rm, L"UpdateRate", DEFAULT_DELAY);

		if (!measure->isTimerActive)
		{
			if (CreateTimerQueueTimer(&measure->timer, g_TimerQueue, (WAITORTIMERCALLBACK)TimerProc, measure, 0,
				(DWORD)delay, 0) == FALSE) RmLog(LOG_ERROR, L"Mouse.dll: Could not start the timer");
			measure->isTimerActive = true;
		}
		else if (measure->delay != delay)
		{
			measure->delay = delay;
			ChangeTimerQueueTimer(g_TimerQueue, measure->timer, 0, delay);
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