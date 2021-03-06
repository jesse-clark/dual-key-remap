#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_ERR_LEN 40

// A semi random value used to identify inputs generated
// by Dual Key Remap. Ideally high to minimize chances of a collision
// with a real pointer used by another application
#define INJECTED_KEY_ID 0xFFC3CED7

typedef enum t_inputType { INPUT_KEYDOWN, INPUT_KEYUP } t_inputType;

typedef enum t_remappedKeyState {
	NOT_HELD_DOWN,
	HELD_DOWN_ALONE,
	HELD_DOWN_WITH_OTHER
} t_remappedKeyState;

typedef struct {
	char *name;
	int code;
} t_vkey;

typedef struct {
	int remapKey;
	int whenAlone;
	int withOther;
} t_config;

t_config* config_new()
{
	t_config* config = malloc(sizeof(config));
	config->remapKey = 0;
	config->whenAlone = 0;
	config->withOther = 0;
	return config;
}

static HHOOK hook;
static t_config *config;
static t_remappedKeyState remappedKeyState = NOT_HELD_DOWN;

static t_vkey vkeytable[] = {
	{"VK_LBUTTON",             0x01}, // Left mouse button
	{"VK_RBUTTON",             0x02}, // Right mouse button
	{"VK_CANCEL",              0x03}, // Control-break processing
	{"VK_MBUTTON",             0x04}, // Middle mouse button (three-button mouse)
	{"VK_XBUTTON1",            0x05}, // X1 mouse button
	{"VK_XBUTTON2",            0x06}, // X2 mouse button
	{"VK_BACK",                0x08}, // BACKSPACE key
	{"VK_TAB",                 0x09}, // TAB key
	{"VK_CLEAR",               0x0C}, // CLEAR key
	{"VK_RETURN",              0x0D}, // ENTER key
	{"VK_SHIFT",               0x10}, // SHIFT key
	{"VK_CONTROL",             0x11}, // CTRL key
	{"VK_MENU",                0x12}, // ALT key
	{"VK_PAUSE",               0x13}, // PAUSE key
	{"VK_CAPITAL",             0x14}, // CAPS LOCK key
	{"VK_KANA",                0x15}, // IME Kana mode
	{"VK_HANGUEL",             0x15}, // IME Hanguel mode
	{"VK_JUNJA",               0x17}, // IME Junja mode
	{"VK_FINAL",               0x18}, // IME final mode
	{"VK_HANJA",               0x19}, // IME Hanja mode
	{"VK_KANJI",               0x19}, // IME Kanji mode
	{"VK_ESCAPE",              0x1B}, // ESC key
	{"VK_CONVERT",             0x1C}, // IME convert
	{"VK_NONCONVERT",          0x1D}, // IME nonconvert
	{"VK_ACCEPT",              0x1E}, // IME accept
	{"VK_MODECHANGE",          0x1F}, // IME mode change request
	{"VK_SPACE",               0x20}, // SPACEBAR
	{"VK_PRIOR",               0x21}, // PAGE UP key
	{"VK_NEXT",                0x22}, // PAGE DOWN key
	{"VK_END",                 0x23}, // END key
	{"VK_HOME",                0x24}, // HOME key
	{"VK_LEFT",                0x25}, // LEFT ARROW key
	{"VK_UP",                  0x26}, // UP ARROW key
	{"VK_RIGHT",               0x27}, // RIGHT ARROW key
	{"VK_DOWN",                0x28}, // DOWN ARROW key
	{"VK_SELECT",              0x29}, // SELECT key
	{"VK_PRINT",               0x2A}, // PRINT key
	{"VK_EXECUTE",             0x2B}, // EXECUTE key
	{"VK_SNAPSHOT",            0x2C}, // PRINT SCREEN key
	{"VK_INSERT",              0x2D}, // INS key
	{"VK_DELETE",              0x2E}, // DEL key
	{"VK_HELP",                0x2F}, // HELP key
	{"VK_LWIN",                0x5B}, // Left Windows key (Natural keyboard)
	{"VK_RWIN",                0x5C}, // Right Windows key (Natural keyboard)
	{"VK_APPS",                0x5D}, // Applications key (Natural keyboard)
	{"VK_SLEEP",               0x5F}, // Computer Sleep key
	{"VK_NUMPAD0",             0x60}, // Numeric keypad 0 key
	{"VK_NUMPAD1",             0x61}, // Numeric keypad 1 key
	{"VK_NUMPAD2",             0x62}, // Numeric keypad 2 key
	{"VK_NUMPAD3",             0x63}, // Numeric keypad 3 key
	{"VK_NUMPAD4",             0x64}, // Numeric keypad 4 key
	{"VK_NUMPAD5",             0x65}, // Numeric keypad 5 key
	{"VK_NUMPAD6",             0x66}, // Numeric keypad 6 key
	{"VK_NUMPAD7",             0x67}, // Numeric keypad 7 key
	{"VK_NUMPAD8",             0x68}, // Numeric keypad 8 key
	{"VK_NUMPAD9",             0x69}, // Numeric keypad 9 key
	{"VK_MULTIPLY",            0x6A}, // Multiply key
	{"VK_ADD",                 0x6B}, // Add key
	{"VK_SEPARATOR",           0x6C}, // Separator key
	{"VK_SUBTRACT",            0x6D}, // Subtract key
	{"VK_DECIMAL",             0x6E}, // Decimal key
	{"VK_DIVIDE",              0x6F}, // Divide key
	{"VK_F1",                  0x70}, // F1 key
	{"VK_F2",                  0x71}, // F2 key
	{"VK_F3",                  0x72}, // F3 key
	{"VK_F4",                  0x73}, // F4 key
	{"VK_F5",                  0x74}, // F5 key
	{"VK_F6",                  0x75}, // F6 key
	{"VK_F7",                  0x76}, // F7 key
	{"VK_F8",                  0x77}, // F8 key
	{"VK_F9",                  0x78}, // F9 key
	{"VK_F10",                 0x79}, // F10 key
	{"VK_F11",                 0x7A}, // F11 key
	{"VK_F12",                 0x7B}, // F12 key
	{"VK_F13",                 0x7C}, // F13 key
	{"VK_F14",                 0x7D}, // F14 key
	{"VK_F15",                 0x7E}, // F15 key
	{"VK_F16",                 0x7F}, // F16 key
	{"VK_F17",                 0x80}, // F17 key
	{"VK_F18",                 0x81}, // F18 key
	{"VK_F19",                 0x82}, // F19 key
	{"VK_F20",                 0x83}, // F20 key
	{"VK_F21",                 0x84}, // F21 key
	{"VK_F22",                 0x85}, // F22 key
	{"VK_F23",                 0x86}, // F23 key
	{"VK_F24",                 0x87}, // F24 key
	{"VK_NUMLOCK",             0x90}, // NUM LOCK key
	{"VK_SCROLL",              0x91}, // SCROLL LOCK key
	{"VK_LSHIFT",              0xA0}, // Left SHIFT key
	{"VK_RSHIFT",              0xA1}, // Right SHIFT key
	{"VK_LCONTROL",            0xA2}, // Left CONTROL key
	{"VK_RCONTROL",            0xA3}, // Right CONTROL key
	{"VK_LMENU",               0xA4}, // Left MENU key
	{"VK_RMENU",               0xA5}, // Right MENU key
	{"VK_BROWSER_BACK",        0xA6}, // Browser Back key
	{"VK_BROWSER_FORWARD",     0xA7}, // Browser Forward key
	{"VK_BROWSER_REFRESH",     0xA8}, // Browser Refresh key
	{"VK_BROWSER_STOP",        0xA9}, // Browser Stop key
	{"VK_BROWSER_SEARCH",      0xAA}, // Browser Search key
	{"VK_BROWSER_FAVORITES",   0xAB}, // Browser Favorites key
	{"VK_BROWSER_HOME",        0xAC}, // Browser Start and Home key
	{"VK_VOLUME_MUTE",         0xAD}, // Volume Mute key
	{"VK_VOLUME_DOWN",         0xAE}, // Volume Down key
	{"VK_VOLUME_UP",           0xAF}, // Volume Up key
	{"VK_MEDIA_NEXT_TRACK",    0xB0}, // Next Track key
	{"VK_MEDIA_PREV_TRACK",    0xB1}, // Previous Track key
	{"VK_MEDIA_STOP",          0xB2}, // Stop Media key
	{"VK_MEDIA_PLAY_PAUSE",    0xB3}, // Play/Pause Media key
	{"VK_LAUNCH_MAIL",         0xB4}, // Start Mail key
	{"VK_LAUNCH_MEDIA_SELECT", 0xB5}, // Select Media key
	{"VK_LAUNCH_APP1",         0xB6}, // Start Application 1 key
	{"VK_LAUNCH_APP2",         0xB7}, // Start Application 2 key
	{"VK_OEM_1",               0xBA}, // Used for miscellaneous characters; it can vary by keyboard.
	{"VK_OEM_PLUS",            0xBB}, // For any country/region, the '+' key
	{"VK_OEM_COMMA",           0xBC}, // For any country/region, the ',' key
	{"VK_OEM_MINUS",           0xBD}, // For any country/region, the '-' key
	{"VK_OEM_PERIOD",          0xBE}, // For any country/region, the '.' key
	{"VK_OEM_2",               0xBF}, // Used for miscellaneous characters; it can vary by keyboard
	{"VK_OEM_3",               0xC0}, // Used for miscellaneous characters; it can vary by keyboard
	{"VK_OEM_4",               0xDB}, // Used for miscellaneous characters; it can vary by keyboard
	{"VK_OEM_5",               0xDC}, // Used for miscellaneous characters; it can vary by keyboard
	{"VK_OEM_6",               0xDD}, // Used for miscellaneous characters; it can vary by keyboard
	{"VK_OEM_7",               0xDE}, // Used for miscellaneous characters; it can vary by keyboard
	{"VK_OEM_8",               0xDF}, // Used for miscellaneous characters; it can vary by keyboard
	{"VK_OEM_102",             0xE2}, // Either the angle bracket key or the backslash key on the RT 102-key keyboard
	{"VK_PROCESSKEY",          0xE5}, // IME PROCESS key
	{"VK_PACKET",              0xE7}, // Used to pass Unicode characters as if they were keystrokes
	{"VK_ATTN",                0xF6}, // Attn key
	{"VK_CRSEL",               0xF7}, // CrSel key
	{"VK_EXSEL",               0xF8}, // ExSel key
	{"VK_EREOF",               0xF9}, // Erase EOF key
	{"VK_PLAY",                0xFA}, // Play key
	{"VK_ZOOM",                0xFB}, // Zoom key
	{"VK_NONAME",              0xFC}, // Reserved
	{"VK_PA1",                 0xFD}, // PA1 key
	{"VK_OEM_CLEAR",           0xFE}, // Clear key
};

#define NVKEYS (sizeof(vkeytable)/sizeof(t_vkey))

int vkeyNameToCode(char *name)
{
	// If the name is a single character the keycode maps
	// to the the ASCI code of the characters [A-Z0-9]
	if (name[1] == '\0')
	{
		if (!isalnum(name[0]))
			return -1;
		return toupper(name[0]);
	}
	// Otherwise we check our vkcode table
	for (int i = 0; i < NVKEYS; ++i)
	{
		t_vkey *vkey = vkeytable + i;
		if (strcmp(vkey->name, name) == 0)
			return vkey->code;
	}
	return -1;
}

void trimnewline(char* buffer)
{
	buffer[strcspn(buffer, "\r\n")] = 0;
}

int parseConfigLine(char *line, t_config* config)
{
	char *vkeyname;
	int vkeycode;

	if (strstr(line, "remap_key="))
	{
		vkeyname = &line[10];
		vkeycode = vkeyNameToCode(vkeyname);
		config->remapKey = vkeycode > 0 ? vkeycode : 0;
	}
	else if (strstr(line, "when_alone="))
	{
		vkeyname = &line[11];
		vkeycode = vkeyNameToCode(vkeyname);
		config->whenAlone = vkeycode > 0 ? vkeycode : 0;
	}
	else if (strstr(line, "with_other="))
	{
		vkeyname = &line[11];
		vkeycode = vkeyNameToCode(vkeyname);
		config->withOther = vkeycode > 0 ? vkeycode : 0;
	}
	else
	{
		printf("Cannot parse line in config: '%s'. Make sure the setting is one of 'remap_key', 'when_alone', or 'with_other' and that there are no extra tabs or spaces.\n", line);
		return 1;
	}

	if (vkeycode == -1)
	{
		printf("The line '%s' does not map to a valid keycode. Non alphanumeric keys must be prefixed with 'VK_'.\n", line);
		return 1;
	}

	return 0;
}

t_config *parseConfig(char *path)
{
	FILE *fs;
	char line[40];
	t_config *config = config_new();

	if (fopen_s(&fs, path, "r") > 0)
	{
		printf("Cannot open configuration file '%s'. Make sure it is in the same directory as 'key-dual-remap.exe'.\n", path);
		goto error;
	}

	while (fgets(line, 40, fs))
	{
		trimnewline(line);
		if (line[0] == '\0')
			continue;  // Ignore empty lines
		if (parseConfigLine(line, config) == 1)
			goto error;
	};
	fclose(fs);

	if (!(config->remapKey && config->whenAlone && config->withOther))
	{
		printf("Not all required settings present in config. Expected 'remap_key', 'when_alone', and 'with_other'.\n");
		goto error;
	}

	return config;

	error:
		free(config);
		return NULL;
}

void sendKeyInput(int keyCode, t_inputType inputType)
{
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.wScan = 0;
	input.ki.time = 0;
	input.ki.dwExtraInfo = (ULONG_PTR)INJECTED_KEY_ID;
	input.ki.wVk = keyCode;
	input.ki.dwFlags = inputType == INPUT_KEYUP ? KEYEVENTF_KEYUP : 0;
	SendInput(1, &input, sizeof(INPUT));
}

LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	const KBDLLHOOKSTRUCT *key = (KBDLLHOOKSTRUCT *) lParam;
	const t_inputType inputType = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) ? INPUT_KEYUP : INPUT_KEYDOWN;

	if (config->remapKey != key->vkCode || key->dwExtraInfo == INJECTED_KEY_ID)
	{
		// Handles non-remapped keys:
		// This includes injected inputs to avoid recursive loops
		// If remapped key is already held down, toggle state to indicate that
		// it is no longer held down alone and send withOther KEYDOWN
		if (remappedKeyState == HELD_DOWN_ALONE)
		{
			remappedKeyState = HELD_DOWN_WITH_OTHER;
			sendKeyInput(config->withOther, INPUT_KEYDOWN);
		}

		// Exit early, allowing others to process the key
		return CallNextHookEx(hook, nCode, wParam, lParam);
	}

	// This is our remapped input and we no longer the others process this key

	if (inputType == INPUT_KEYDOWN && remappedKeyState == NOT_HELD_DOWN) {
		// Handles remapped key KEYDOWN:
		// Start listening other key presses
		// Ignores KEYDOWN if according to state we're already holding down
		// (possible with multiple keys/keyboards)
		remappedKeyState = HELD_DOWN_ALONE;
	}
	// Handles remapped key KEYUP:
	// Either send whenAlone or finish sending withOther key
	// Ignores KEYUP if according to state we're not holding down (multiple keys/keyboards)
	// As a result, for multiple keys/keyboards only the first KEYUP will send output
	// For safety adjust our state _before_ sending further key inputs
	else if (inputType == INPUT_KEYUP && remappedKeyState == HELD_DOWN_ALONE)
	{
		remappedKeyState = NOT_HELD_DOWN;
		sendKeyInput(config->whenAlone, INPUT_KEYDOWN);
		sendKeyInput(config->whenAlone, INPUT_KEYUP);
	}
	else if (inputType == INPUT_KEYUP && remappedKeyState == HELD_DOWN_WITH_OTHER)
	{
		remappedKeyState = NOT_HELD_DOWN;
		sendKeyInput(config->withOther, INPUT_KEYUP);
	}

	return 1;
}

int main(void)
{
	HWND hWnd = GetConsoleWindow();
	MSG msg;
	HANDLE hMutexHandle = CreateMutex(NULL, TRUE, "dual-key-remap.single-instance");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		printf("dual-key-remap.exe is already running!\n");
		goto error;
	}

	config = parseConfig("config.txt");
	if (config == NULL)
	{
		goto error;

	}

	hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, NULL, 0);
	if (hook == NULL)
	{
		printf("Cannot hook into the windows API.");
		goto error;
	}

	// No errors, we can hide console window
	ShowWindow(hWnd, SW_HIDE);

	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	free(config);
	ReleaseMutex(hMutexHandle);
	CloseHandle(hMutexHandle);
	UnhookWindowsHookEx(hook);
	return 0;

	error:
		free(config);
		ReleaseMutex(hMutexHandle);
		CloseHandle(hMutexHandle);
		ShowWindow(hWnd, SW_SHOW);
		printf("Press any key to exit...\n");
		getch();
		return 1;
}
