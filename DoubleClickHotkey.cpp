#include <iostream>
#include <conio.h>
#include <Windows.h>

using namespace std;

HHOOK KeyboardHookHandle = NULL;

void WaitKey()
{
	cout << "Press any key to continue...";
	_getch();
}

[[noreturn]] void CleanUpExit(int exitCode = 0)
{
	if (KeyboardHookHandle)
	{
		UnhookWindowsHookEx(KeyboardHookHandle);
		KeyboardHookHandle = NULL;
	}

	exit(exitCode);
}

bool IsConsoleVisible()
{
	return IsWindowVisible(GetConsoleWindow()) != 0;
}

void HideConsole()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
}

void ShowConsole()
{
	ShowWindow(GetConsoleWindow(), SW_SHOW);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	CleanUpExit();
}

LRESULT CALLBACK KeyboardHookHandler(int nCode, WPARAM wParam, LPARAM lParam)
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
			if (isKeydown)
			{
				const bool isCtrlPressed = GetKeyState(VK_CONTROL) < 0;
				const bool isShiftPressed = GetKeyState(VK_SHIFT) < 0;

				if (isCtrlPressed && isShiftPressed)
				{
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
				else
				{
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
			}

			std::cout << "CallNextHookEx BLOCKED" << std::endl;
			return 1;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main()
{
	HideConsole();

	HANDLE SingleInstanceMutexHandle = CreateMutex(NULL, TRUE, L"double-click-hotkey-mutex-wzyids6rnh94128qrg5t");
	if (!SingleInstanceMutexHandle || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		ShowConsole();
		cout << "Another instance of this application is already running." << endl;
		WaitKey();
		CleanUpExit(1);
	}

	if (!SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE))
	{
		ShowConsole();
		cout << "Failed to set console ctrl handler, error code: " << GetLastError() << endl;
		WaitKey();
		CleanUpExit(1);
	}

	KeyboardHookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardHookHandler, NULL, 0);
	if (!KeyboardHookHandle)
	{
		ShowConsole();
		cout << "Failed to set keyboard hook, error code: " << GetLastError() << endl;
		WaitKey();
		CleanUpExit(1);
	}

	MSG msg;
	do
	{
		BOOL ret = GetMessage(&msg, NULL, 0, 0);

		if (ret == -1)
		{
			ShowConsole();
			cout << "Failed to get message." << endl;
			WaitKey();
			CleanUpExit(1);
		}

		if (ret == 0)
		{
			CleanUpExit();
		}
	} while (true);
}
