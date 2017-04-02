#pragma once
#include "Mouse.h"

const wstring GetMouseVariable(const wstring variable, POINT pt, RECT window)
{
	wstring result;
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

const void ReplaceMouseVariables(wstring& result, POINT pt, RECT window)
{
	// Check for variables ($VAR$)
	size_t start = 0, end;
	bool loop = true;

	do
	{
		start = result.find(L'$', start);
		if (start != wstring::npos)
		{
			size_t si = start + 1;
			end = result.find(L'$', si);
			if (end != wstring::npos)
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
					wstring strVariable = result.substr(si, end - si);
					wstring value = GetMouseVariable(strVariable, pt, window);
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