#include "Mouse.hpp"

const void Measure::Remove()
{
	vector<Measure*>::iterator found = find(Measures.begin(), Measures.end(), this);
	if (found != Measures.end())
	{
		Measures.erase(found);
	}

	if (Measures.empty() && bHookActive)
	{
		if (!StopHook())
		{
			RmLog(LOG_ERROR, L"Mouse.dll: Could not stop the mouse hook");
		}

		hHook = NULL;
		bHookActive = false;
	}
}

const void Measure::ReadOptions(void* rm)
{
	actions[MOUSE_MOVE] = RmReadString(rm, L"MouseMoveAction", L"", false);

	actions[MOUSE_LMB_UP] = RmReadString(rm, L"LeftMouseUpAction", L"", false);
	actions[MOUSE_LMB_DOWN] = RmReadString(rm, L"LeftMouseDownAction", L"", false);
	actions[MOUSE_LMB_DBLCLK] = RmReadString(rm, L"LeftMouseDoubleClickAction", L"", false);
	actions[MOUSE_LMB_DRAG] = RmReadString(rm, L"LeftMouseDragAction", L"", false);
	actions[MOUSE_MMB_UP] = RmReadString(rm, L"MiddleMouseUpAction", L"", false);
	actions[MOUSE_MMB_DOWN] = RmReadString(rm, L"MiddleMouseDownAction", L"", false);
	actions[MOUSE_MMB_DBLCLK] = RmReadString(rm, L"MiddleMouseDoubleClickAction", L"", false);
	actions[MOUSE_MMB_DRAG] = RmReadString(rm, L"MiddleMouseDragAction", L"", false);
	actions[MOUSE_RMB_UP] = RmReadString(rm, L"RightMouseUpAction", L"", false);
	actions[MOUSE_RMB_DOWN] = RmReadString(rm, L"RightMouseDownAction", L"", false);
	actions[MOUSE_RMB_DBLCLK] = RmReadString(rm, L"RightMouseDoubleClickAction", L"", false);
	actions[MOUSE_RMB_DRAG] = RmReadString(rm, L"RightMouseDragAction", L"", false);
	actions[MOUSE_X1MB_UP] = RmReadString(rm, L"X1MouseUpAction", L"", false);
	actions[MOUSE_X1MB_DOWN] = RmReadString(rm, L"X1MouseDownAction", L"", false);
	actions[MOUSE_X1MB_DBLCLK] = RmReadString(rm, L"X1MouseDoubleClickAction", L"", false);
	actions[MOUSE_X1MB_DRAG] = RmReadString(rm, L"X1MouseDragAction", L"", false);
	actions[MOUSE_X2MB_UP] = RmReadString(rm, L"X2MouseUpAction", L"", false);
	actions[MOUSE_X2MB_DOWN] = RmReadString(rm, L"X2MouseDownAction", L"", false);
	actions[MOUSE_X2MB_DBLCLK] = RmReadString(rm, L"X2MouseDoubleClickAction", L"", false);
	actions[MOUSE_X2MB_DRAG] = RmReadString(rm, L"X2MouseDragAction", L"", false);

	actions[MOUSE_MW_UP] = RmReadString(rm, L"MouseScrollUpAction", L"", false);
	actions[MOUSE_MW_DOWN] = RmReadString(rm, L"MouseScrollDownAction", L"", false);
	actions[MOUSE_MW_LEFT] = RmReadString(rm, L"MouseScrollLeftAction", L"", false);
	actions[MOUSE_MW_RIGHT] = RmReadString(rm, L"MouseScrollRightAction", L"", false);
}