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

static HHOOK hHook;
static HWND hMainWindow;
static vector<Measure*> Measures;
static DWORD dThreadId = NULL;
static HANDLE hThread = NULL;
static bool bThreadActive = false;

DWORD WINAPI HookThread(void*);
const void ReplaceMouseVariables(wstring& result, POINT pt, RECT window);
const void RemoveMeasure(Measure* measure);