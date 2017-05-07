#include "Mouse.hpp"

HHOOK hHook;

HANDLE hWorkerThread;
DWORD dWorkerThreadId;
DWORD WINAPI WorkerThread(void*);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
bool mouseCaptured = false;
inline void RegularAction(MOUSEACTION mouseaction);
inline void DoubleClickAction(MOUSEACTION mouseaction);
inline void XButtonAction(bool down, DWORD mouseData);
inline void XButtonDoubleClickAction(DWORD mouseData);
inline void MouseScrollAction(DWORD mouseData);
inline void MouseHScrollAction(DWORD mouseData);

POINT mousePt;
HWND window;

const bool StartHook()
{
	hHook = SetWindowsHookEx(WH_MOUSE, MouseProc, NULL, GetCurrentThreadId());

	hWorkerThread = CreateThread(0, 0, WorkerThread, 0, 0, &dWorkerThreadId);

	if (!hHook)
		return false;

	return true;
}

const bool StopHook()
{
	bool ret = true;
	if (UnhookWindowsHookEx(hHook) == FALSE)
		ret = false;

	PostThreadMessage(dWorkerThreadId, WM_QUIT, 0, 0);

	if (hWorkerThread)
	{
		WaitForSingleObject(hWorkerThread, INFINITE);

		if (CloseHandle(hWorkerThread) == FALSE)
			ret = false;

		hWorkerThread = NULL;
		dWorkerThreadId = NULL;
	}

	return ret;
}

DWORD WINAPI WorkerThread(void*)
{
	MSG msg;
	BOOL bRet;

	while (1)
	{
		bRet = GetMessage(&msg, NULL, 0, 0);
		if (bRet > 0)
		{
			MSG quit;
			if (PeekMessage(&quit, NULL, WM_QUIT, WM_QUIT, PM_REMOVE))
			{
				TranslateMessage(&quit);
				DispatchMessage(&msg);
				return 0;
			}
			if (GetQueueStatus(QS_ALLEVENTS) || msg.message != WM_APP)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				continue;
			}

			RECT rect;
			SetRectEmpty(&rect);

			bool hasWaited = false;

			for (Measure* measure : Measures)
			{
				if (!measure->enabled) continue;
				if (window != measure->window) continue;

				vector<wstring> actions;

				if (measure->relative)
					GetWindowRect(measure->window, &rect);

				if (!measure->actions[MOUSE_MOVE].empty())
					actions.push_back(measure->actions[MOUSE_MOVE]);

				if (GetKeyState(VK_LBUTTON) < 0 && !measure->actions[MOUSE_LMB_DRAG].empty())
					actions.push_back(measure->actions[MOUSE_LMB_DRAG]);
				if (GetKeyState(VK_MBUTTON) < 0 && !measure->actions[MOUSE_MMB_DRAG].empty())
					actions.push_back(measure->actions[MOUSE_MMB_DRAG]);
				if (GetKeyState(VK_RBUTTON) < 0 && !measure->actions[MOUSE_RMB_DRAG].empty())
					actions.push_back(measure->actions[MOUSE_RMB_DRAG]);
				if (GetKeyState(VK_XBUTTON1) < 0 && !measure->actions[MOUSE_X1MB_DRAG].empty())
					actions.push_back(measure->actions[MOUSE_X1MB_DRAG]);
				if (GetKeyState(VK_XBUTTON2) < 0 && !measure->actions[MOUSE_X2MB_DRAG].empty())
					actions.push_back(measure->actions[MOUSE_X2MB_DRAG]);

				if (!actions.empty())
				{
					for (wstring action : actions)
					{
						ReplaceMouseVariables(action, mousePt, rect);
						SendNotifyMessage(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)action.c_str());
					}
					if (measure->delay > 16)
					{
						Sleep(measure->delay);
						hasWaited = true;
					}
				}
			}

			if (!hasWaited)
			{
				Sleep(16);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if (bRet < 0)
		{
			RmLog(LOG_ERROR, L"Mouse.dll: Error in SendThread Message Loop");
			break;
		}
		else
			break;
	}
	return 0;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(hHook, nCode, wParam, lParam);

	int code = static_cast<int>(wParam);
	PMOUSEHOOKSTRUCTEX data = (PMOUSEHOOKSTRUCTEX)lParam;
	window = data->hwnd;
	mousePt = data->pt;

	switch (code)
	{
	case WM_NCMOUSEMOVE:
	case WM_MOUSEMOVE:
		PostThreadMessage(dWorkerThreadId, WM_APP, 0, 0);
		break;
	case WM_MOUSEWHEEL:
		MouseScrollAction(data->mouseData);
		break;
	case WM_MOUSEHWHEEL:
		MouseHScrollAction(data->mouseData);
		break;
	case WM_NCLBUTTONDOWN:
	case WM_LBUTTONDOWN:
		RegularAction(MOUSE_LMB_DOWN);
		break;
	case WM_LBUTTONUP:
	case WM_NCLBUTTONUP:
		RegularAction(MOUSE_LMB_UP);
		break;
	case WM_LBUTTONDBLCLK:
	case WM_NCLBUTTONDBLCLK:
		DoubleClickAction(MOUSE_LMB_DBLCLK);
		break;
	case WM_NCMBUTTONDOWN:
	case WM_MBUTTONDOWN:
		RegularAction(MOUSE_MMB_DOWN);
		break;
	case WM_MBUTTONUP:
	case WM_NCMBUTTONUP:
		RegularAction(MOUSE_MMB_UP);
		break;
	case WM_MBUTTONDBLCLK:
	case WM_NCMBUTTONDBLCLK:
		DoubleClickAction(MOUSE_MMB_DBLCLK);
		break;
	case WM_NCRBUTTONDOWN:
	case WM_RBUTTONDOWN:
		RegularAction(MOUSE_RMB_DOWN);
		break;
	case WM_RBUTTONUP:
	case WM_NCRBUTTONUP:
		RegularAction(MOUSE_RMB_UP);
		break;
	case WM_RBUTTONDBLCLK:
	case WM_NCRBUTTONDBLCLK:
		DoubleClickAction(MOUSE_RMB_DBLCLK);
		break;
	case WM_XBUTTONDOWN:
	case WM_NCXBUTTONDOWN:
		XButtonAction(true, data->mouseData);
		break;
	case WM_XBUTTONUP:
	case WM_NCXBUTTONUP:
		XButtonAction(false, data->mouseData);
		break;
	case WM_XBUTTONDBLCLK:
	case WM_NCXBUTTONDBLCLK:
		XButtonDoubleClickAction(data->mouseData);
		break;
	default:
		break;
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

inline void RegularAction(MOUSEACTION mouseaction)
{
	RECT rect;
	SetRectEmpty(&rect);

	for (Measure* measure : Measures)
	{
		if (!measure->enabled) continue;
		if (window != measure->window) continue;

		if (measure->relative)
			GetWindowRect(measure->window, &rect);

		wstring action = measure->actions[mouseaction];

		if (!action.empty())
		{
			ReplaceMouseVariables(action, mousePt, rect);
			SendNotifyMessage(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)action.c_str());
		}
	}
}

inline void DoubleClickAction(MOUSEACTION mouseaction)
{
	RECT rect;
	SetRectEmpty(&rect);

	for (Measure* measure : Measures)
	{
		if (!measure->enabled) continue;
		if (window != measure->window) continue;

		if (measure->relative)
			GetWindowRect(measure->window, &rect);

		wstring action = measure->actions[mouseaction];

		if (!action.empty())
		{
			ReplaceMouseVariables(action, mousePt, rect);
			SendNotifyMessage(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)action.c_str());
		}
		else
		{
			switch (mouseaction)
			{
			case MOUSE_LMB_DBLCLK:
				action = measure->actions[MOUSE_LMB_DOWN];
				break;
			case MOUSE_MMB_DBLCLK:
				action = measure->actions[MOUSE_MMB_DOWN];
				break;
			case MOUSE_RMB_DBLCLK:
				action = measure->actions[MOUSE_RMB_DOWN];
				break;
			case MOUSE_X1MB_DBLCLK:
				action = measure->actions[MOUSE_X1MB_DOWN];
				break;
			case MOUSE_X2MB_DBLCLK:
				action = measure->actions[MOUSE_X2MB_DOWN];
				break;
			default:
				break;
			}

			if (!action.empty())
			{
				ReplaceMouseVariables(action, mousePt, rect);
				SendNotifyMessage(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)action.c_str());
			}
		}
	}
}

inline void XButtonAction(bool down, DWORD mouseData)
{
	if (HIWORD(mouseData) == XBUTTON1)
	{
		RegularAction(down ? MOUSE_X1MB_DOWN : MOUSE_X1MB_UP);
	}
	else if (HIWORD(mouseData) == XBUTTON2)
	{
		RegularAction(down ? MOUSE_X2MB_DOWN : MOUSE_X2MB_UP);
	}
}

inline void XButtonDoubleClickAction(DWORD mouseData)
{
	if (HIWORD(mouseData) == XBUTTON1)
	{
		DoubleClickAction(MOUSE_X1MB_DBLCLK);
	}
	else if (HIWORD(mouseData) == XBUTTON2)
	{
		DoubleClickAction(MOUSE_X2MB_DBLCLK);
	}
}

inline void MouseScrollAction(DWORD mouseData)
{
	RegularAction((GET_WHEEL_DELTA_WPARAM(mouseData) < 0) ? MOUSE_MW_DOWN : MOUSE_MW_UP);
}

inline void MouseHScrollAction(DWORD mouseData)
{
	RegularAction((GET_WHEEL_DELTA_WPARAM(mouseData) < 0) ? MOUSE_MW_LEFT : MOUSE_MW_RIGHT);
}