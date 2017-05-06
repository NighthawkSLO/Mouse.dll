#pragma once
#include <Windows.h>
#include "api/RainmeterAPI.h"
#include <vector>
#include <array>
using namespace std;

#define RAINMETER_CLASS_NAME L"DummyRainWClass"
#define RAINMETER_WINDOW_NAME L"Rainmeter control window"
#define WM_RAINMETER_EXECUTE WM_APP + 2

enum MOUSEACTION
{
	MOUSE_LMB_UP = 0,
	MOUSE_LMB_DOWN,
	MOUSE_LMB_DBLCLK,
	MOUSE_LMB_DRAG,
	MOUSE_MMB_UP,
	MOUSE_MMB_DOWN,
	MOUSE_MMB_DBLCLK,
	MOUSE_MMB_DRAG,
	MOUSE_RMB_UP,
	MOUSE_RMB_DOWN,
	MOUSE_RMB_DBLCLK,
	MOUSE_RMB_DRAG,
	MOUSE_X1MB_UP,
	MOUSE_X1MB_DOWN,
	MOUSE_X1MB_DBLCLK,
	MOUSE_X1MB_DRAG,
	MOUSE_X2MB_UP,
	MOUSE_X2MB_DOWN,
	MOUSE_X2MB_DBLCLK,
	MOUSE_X2MB_DRAG,

	MOUSE_MW_UP,
	MOUSE_MW_DOWN,
	MOUSE_MW_LEFT,
	MOUSE_MW_RIGHT,

	MOUSEACTION_COUNT
};

struct Measure
{
	void* skin;
	HWND window;
	bool enabled = false;
	bool relative = true;
	wstring actions[MOUSEACTION_COUNT];

	const void ReadOptions(void* rm);
	const void Remove();
};

extern HHOOK hHook;
extern HWND hMainWindow;
extern vector<Measure*> Measures;
extern bool bHookActive;

const bool StartHook();
const bool StopHook();

DWORD WINAPI HookThread(void*);
const void ReplaceMouseVariables(wstring& result, POINT pt, RECT window);