#pragma once
#include <Windows.h>
#include "api/RainmeterAPI.h"
#include <vector>
using namespace std;

#define RAINMETER_CLASS_NAME L"DummyRainWClass"
#define RAINMETER_WINDOW_NAME L"Rainmeter control window"
#define WM_RAINMETER_EXECUTE WM_APP + 2

const struct Measure
{
	void* skin;
	HWND window;
	bool enabled = false;
	bool relative = true;
	bool needsFocus = false;

	wstring Move;
	wstring LeftDown;
	wstring LeftUp;
	wstring LeftDrag;
	wstring RightDown;
	wstring RightUp;
	wstring RightDrag;
	wstring MiddleDown;
	wstring MiddleUp;
	wstring MiddleDrag;
};

extern HHOOK hHook;
extern HWND hMainWindow;
extern vector<Measure*> Measures;
extern DWORD dThreadId;
extern HANDLE hThread;
extern bool bThreadActive;

DWORD WINAPI HookThread(void*);
const void ReplaceMouseVariables(wstring& result, POINT pt, RECT window);
const void RemoveMeasure(Measure* measure);