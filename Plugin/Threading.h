#pragma once
#include "ReplaceVariables.h"

struct MOUSEINFO
{
	POINT pt = {};
	bool LeftDown = false;
	bool RightDown = false;
	bool MiddleDown = false;
} info;

DWORD dSendThreadId;
DWORD WINAPI SendThread(void*);
LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
const void ProcessMouse(MSLLHOOKSTRUCT* data);
HWND hForeground;
const void Exec(wstring Measure::* field);

DWORD WINAPI HookThread(void*)
{
	HANDLE hSendThread = CreateThread(0, 0, SendThread, 0, 0, &dSendThreadId);
	hHook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, GetModuleHandle(NULL), NULL);

	MSG msg;
	BOOL bRet;

	while (1)
	{
		bRet = GetMessage(&msg, NULL, 0, 0);
		if (bRet > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if (bRet < 0)
		{
			RmLog(LOG_ERROR, L"Mouse.dll: Error in HookThread Message Loop");
			break;
		}
		else
			break;
	}
	if (UnhookWindowsHookEx(hHook) == FALSE) RmLog(LOG_ERROR, L"Mouse.dll: Could not stop the mouse hook");
	PostThreadMessage(dSendThreadId, WM_QUIT, 0, 0);
	if (hSendThread)
	{
		WaitForSingleObject(hSendThread, INFINITE);
		CloseHandle(hSendThread);
		hSendThread = NULL;
		dSendThreadId = NULL;
		hForeground = NULL;
	}

	info = {};
	hHook = NULL;
	return 0;
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0) {
		info.pt = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam)->pt;
		hForeground = GetForegroundWindow();
		switch (static_cast<int>(wParam))
		{
		case WM_MOUSEMOVE:
			PostThreadMessage(dSendThreadId, nCode, wParam, lParam);
			break;
		case WM_LBUTTONDOWN:
			info.LeftDown = true;
			Exec(&Measure::LeftDown);
			break;
		case WM_LBUTTONUP:
			info.LeftDown = false;
			Exec(&Measure::LeftUp);
			break;
		case WM_RBUTTONDOWN:
			info.RightDown = true;
			Exec(&Measure::RightDown);
			break;
		case WM_RBUTTONUP:
			info.RightDown = false;
			Exec(&Measure::RightUp);
			break;
		case WM_MBUTTONDOWN:
			info.MiddleDown = true;
			Exec(&Measure::MiddleDown);
			break;
		case WM_MBUTTONUP:
			info.MiddleDown = false;
			Exec(&Measure::MiddleUp);
			break;
		default:
			break;
		}
	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

DWORD WINAPI SendThread(void*)
{
	MSG msg;
	BOOL bRet;

	while (1)
	{
		bRet = GetMessage(&msg, NULL, 0, 0);
		if (bRet > 0)
		{
			if (GetQueueStatus(QS_ALLEVENTS)) continue;
			if (static_cast<int>(msg.wParam) == WM_MOUSEMOVE)
			{
				ProcessMouse(reinterpret_cast<MSLLHOOKSTRUCT*>(msg.lParam));
				Sleep((DWORD)50);
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

const void ProcessMouse(MSLLHOOKSTRUCT* data)
{
	for (Measure* measure : Measures)
	{
		if (!measure->enabled) continue;
		if (measure->needsFocus && measure->window != hForeground) continue;
		RECT rect = {};
		wstring command;
		if (measure->relative) GetWindowRect(measure->window, &rect);
		if (!measure->Move.empty())
		{
			command = measure->Move;
			ReplaceMouseVariables(command, data->pt, rect);
			SendMessageTimeout(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)command.c_str(), SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, NULL);
		}
		if (info.LeftDown && !measure->LeftDrag.empty())
		{
			command = measure->LeftDrag;
			ReplaceMouseVariables(command, data->pt, rect);
			SendMessageTimeout(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)command.c_str(), SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, NULL);
		}
		if (info.RightDown && !measure->RightDrag.empty())
		{
			command = measure->RightDrag;
			ReplaceMouseVariables(command, data->pt, rect);
			SendMessageTimeout(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)command.c_str(), SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, NULL);
		}
		if (info.MiddleDown && !measure->MiddleDrag.empty())
		{
			command = measure->MiddleDrag;
			ReplaceMouseVariables(command, data->pt, rect);
			SendMessageTimeout(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)command.c_str(), SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, NULL);
		}
	}
}

const void Exec(wstring Measure::* field)
{
	for (Measure* measure : Measures)
	{
		if (!measure->enabled) continue;
		if (measure->needsFocus && measure->window != hForeground) continue;
		RECT rect = {};
		wstring command = measure->*field;
		if (measure->relative) GetWindowRect(measure->window, &rect);
		if (!command.empty())
		{
			ReplaceMouseVariables(command, info.pt, rect);
			SendNotifyMessage(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)command.c_str());
			//SendMessageTimeout(hMainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)command.c_str(), SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, NULL);
		}
	}
}