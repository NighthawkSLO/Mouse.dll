#include <Windows.h>
#include <vector>

#define RAINMETER_CLASS_NAME L"DummyRainWClass"
#define RAINMETER_WINDOW_NAME L"Rainmeter control window"
#define WM_RAINMETER_EXECUTE WM_APP + 2

struct Measure {
	HANDLE timer = NULL;
	bool isTimerActive = false;
	void* skin;
	HWND window;
	bool enabled = false;
	bool relative = true;
	bool needsFocus = false;
	int delay = DEFAULT_DELAY;

	std::wstring Move;
	std::wstring LeftDown;
	std::wstring LeftUp;
	std::wstring LeftDrag;
	std::wstring RightDown;
	std::wstring RightUp;
	std::wstring RightDrag;
	std::wstring MiddleDown;
	std::wstring MiddleUp;
	std::wstring MiddleDrag;
};

static HINSTANCE g_Instance = nullptr;
static HHOOK g_Hook = NULL;
static bool g_IsHookActive = false;
static HANDLE g_TimerQueue = NULL;
static bool g_IsTimerActive = false;
static HANDLE g_TimeoutTimer = NULL;
static bool g_IsTimeoutTimerActive = false;
static std::vector<Measure*> g_Measures;
static HWND g_mainWindow;
static POINT g_Point;
static bool g_HasPointExecuted = true;
static bool g_LeftMouseDown = false;
static bool g_RightMouseDown = false;
static bool g_MiddleMouseDown = false;

const void ReplaceMouseVariables(std::wstring& result, POINT pt, RECT window);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_Instance = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

const void Exec(Measure* measure, POINT pt, std::wstring Measure::* field, HWND foreground = GetForegroundWindow())
{
	if (!measure->enabled) return;
	if (measure->needsFocus && measure->window != foreground) return;
	std::wstring command = measure->*field;
	if (!command.empty()) {
		RECT rect = { 0 };
		if (measure->relative) GetWindowRect(measure->window, &rect);
		ReplaceMouseVariables(command, pt, rect);
		SendNotifyMessage(g_mainWindow, WM_RAINMETER_EXECUTE, (WPARAM)measure->skin, (LPARAM)command.c_str());
	}
}

const void LoopExec(POINT pt, std::wstring Measure::* field)
{
	HWND foreground = GetForegroundWindow();
	for (Measure* measure : g_Measures) {
		Exec(measure, pt, field, foreground);
	}
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	int param = static_cast<int>(wParam);
	if (nCode >= 0) {
		POINT pt = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam)->pt;
		if (param == WM_MOUSEMOVE) { g_Point = pt; g_HasPointExecuted = false; }
		else if (param == WM_LBUTTONDOWN) { g_LeftMouseDown = true; LoopExec(pt, &Measure::LeftDown); }
		else if (param == WM_LBUTTONUP) { g_LeftMouseDown = false; LoopExec(pt, &Measure::LeftUp); }
		else if (param == WM_RBUTTONDOWN) { g_RightMouseDown = true; LoopExec(pt, &Measure::RightDown); }
		else if (param == WM_RBUTTONUP) { g_RightMouseDown = false; LoopExec(pt, &Measure::RightUp); }
		else if (param == WM_MBUTTONDOWN) { g_MiddleMouseDown = true; LoopExec(pt, &Measure::MiddleDown); }
		else if (param == WM_MBUTTONUP) { g_MiddleMouseDown = false; LoopExec(pt, &Measure::MiddleUp); }
	}
	return CallNextHookEx(g_Hook, nCode, wParam, lParam);
}

VOID CALLBACK TimerProc(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	Measure* measure;
	if (lpParam != NULL) measure = (Measure*)lpParam;

	if (!g_HasPointExecuted)
	{
		if (!measure->Move.empty()) Exec(measure, g_Point, &Measure::Move);
		if (!measure->LeftDrag.empty() && g_LeftMouseDown) Exec(measure, g_Point, &Measure::LeftDrag);
		if (!measure->RightDrag.empty() && g_RightMouseDown) Exec(measure, g_Point, &Measure::RightDrag);
		if (!measure->MiddleDrag.empty() && g_MiddleMouseDown) Exec(measure, g_Point, &Measure::MiddleDrag);
		g_HasPointExecuted = true;
	}
}

const std::wstring GetMouseVariable(const std::wstring& variable, POINT pt, RECT window)
{
	std::wstring result;
	LPCWSTR var = variable.c_str();
	WCHAR buffer[32];

	if (_wcsnicmp(var, L"MOUSEX", 6) == 0)
	{
		var += 6;
		if (*var == L'\0')  // $MOUSEX$
		{
			_itow_s(pt.x - window.left, buffer, 10);
			result = buffer;
		}
	}
	else if (_wcsnicmp(var, L"MOUSEY", 6) == 0)
	{
		var += 6;
		if (*var == L'\0')  // $MOUSEY$
		{
			_itow_s(pt.y - window.top, buffer, 10);
			result = buffer;
		}
	}

	return result;
}


const void ReplaceMouseVariables(std::wstring& result, POINT pt, RECT window)
{
	// Check for variables ($VAR$)
	size_t start = 0, end;
	bool loop = true;

	do
	{
		start = result.find(L'$', start);
		if (start != std::wstring::npos)
		{
			size_t si = start + 1;
			end = result.find(L'$', si);
			if (end != std::wstring::npos)
			{
				size_t ei = end - 1;
				if (si != ei && result[si] == L'*' && result[ei] == L'*')
				{
					result.erase(ei, 1);
					result.erase(si, 1);
					start = ei;
				}
				else
				{
					std::wstring strVariable = result.substr(si, end - si);
					std::wstring value = GetMouseVariable(strVariable, pt, window);
					if (!value.empty())
					{
						// Variable found, replace it with the value
						result.replace(start, end - start + 1, value);
						start += value.length();
					}
					else
					{
						start = end;
					}
				}
			}
			else
			{
				loop = false;
			}
		}
		else
		{
			loop = false;
		}
	} while (loop);
}