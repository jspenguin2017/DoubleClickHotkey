#include <iostream>
#include <conio.h>
#include <Windows.h>

HHOOK keyboardHookHandle = NULL;

bool IsConsoleVisible()
{
	return IsWindowVisible(GetConsoleWindow());
}

void HideConsole()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
}

void ShowConsole()
{
	ShowWindow(GetConsoleWindow(), SW_SHOW);
}

void WaitKeypress()
{
	std::cout << "Press any key to exit...";

#pragma warning(suppress: 6031)
	_getch();
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	if (keyboardHookHandle)
	{
		UnhookWindowsHookEx(keyboardHookHandle);
		keyboardHookHandle = NULL;
		std::cout << "ConsoleCtrlHandler EXIT" << std::endl;
	}

	if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT)
	{
		WaitKeypress();
	}

	exit(0);
	return TRUE;
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
	{
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	const bool isKeydown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
	const bool isKeyup = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;

	if (isKeydown || isKeyup)
	{
		PKBDLLHOOKSTRUCT lParamStruct = (PKBDLLHOOKSTRUCT)lParam;

		if (lParamStruct->vkCode == VK_F7)
		{
			const bool isAltPressed = GetKeyState(VK_MENU) < 0;
			const bool isCtrlPressed = GetKeyState(VK_CONTROL) < 0;
			const bool isShiftPressed = GetKeyState(VK_SHIFT) < 0;

			if (isAltPressed && isCtrlPressed && isShiftPressed) {
				if (isKeydown) {
					INPUT input = { 0 };
					input.type = INPUT_MOUSE;

					input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					SendInput(1, &input, sizeof(INPUT));
					input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(1, &input, sizeof(INPUT));

					input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
					SendInput(1, &input, sizeof(INPUT));
					input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
					SendInput(1, &input, sizeof(INPUT));

					std::cout << "SendInput DOUBLECLICK" << std::endl;
				}

				std::cout << "CallNextHookEx BLOCKED" << std::endl;
				return 1;
			}

			if (isCtrlPressed && isShiftPressed)
			{
				if (isKeydown) {
					if (IsConsoleVisible())
					{
						HideConsole();
					}
					else
					{
						ShowConsole();
					}

					std::cout << "ShowWindow TOGGLE" << std::endl;
				}

				std::cout << "CallNextHookEx BLOCKED" << std::endl;
				return 1;
			}
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main()
{
	HideConsole();

	HANDLE singleInstanceMutexHandle = CreateMutex(NULL, TRUE, L"double-click-hotkey-mutex-wzyids6rnh94128qrg5t");
	if (!singleInstanceMutexHandle || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		std::cout << "CreateMutex FAIL " << GetLastError() << std::endl;
		ShowConsole();
		WaitKeypress();
		return 1;
	}

	if (!SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE))
	{
		std::cout << "SetConsoleCtrlHandler FAIL " << GetLastError() << std::endl;
		ShowConsole();
		WaitKeypress();
		return 1;
	}

	keyboardHookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookProc, NULL, 0);
	if (keyboardHookHandle)
	{
		std::cout << "SetWindowsHookEx OK" << std::endl;
	}
	else
	{
		std::cout << "SetWindowsHookEx FAIL " << GetLastError() << std::endl;
		ShowConsole();
		WaitKeypress();
		return 1;
	}

	MSG msg;
	do
	{
		BOOL ret = GetMessage(&msg, NULL, 0, 0);

		if (ret == -1)
		{
			UnhookWindowsHookEx(keyboardHookHandle);
			keyboardHookHandle = NULL;
			std::cout << "GetMessage FAIL" << std::endl;
			ShowConsole();
			WaitKeypress();
			return 1;
		}

		if (ret == 0)
		{
			UnhookWindowsHookEx(keyboardHookHandle);
			keyboardHookHandle = NULL;
			std::cout << "GetMessage WM_QUIT" << std::endl;
			return 0;
		}
	} while (true);
}