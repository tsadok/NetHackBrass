#include "nhw.h"
#include "patchlevel.h"

#ifndef DEF_GAME_NAME
#define DEF_GAME_NAME "NetHack"
#endif /*DEF_GAME_NAME*/

#define YOKO	320
#define TATE	200

HINSTANCE hInst;
static char *cmdline;
static int drawplane;
int nhrunning;

void pcmain(int argc, char **argv);

void GetRegistoryData(void);
void SetRegistoryData(void);
void DisposeRegistoryData(void);
int savereg;
DWORD win_left;
DWORD win_top;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

extern nhwBackBuf bkbuf;

extern nhwWinInfo nhwi[];
extern COLORREF colortable[];

/*---------------------------------------------------------------------------
	Key Buffering
  ---------------------------------------------------------------------------*/
#define KEYBUFSIZ 256	/* should be 2^n */
static char keybuf[KEYBUFSIZ];
static int keybufrp = 0;
static int keybufwp = 0;

void keybufput(char c) {
    if (c && !(((keybufwp+1)&(KEYBUFSIZ-1)) == keybufrp)) {
	keybuf[keybufwp++] = c;
	keybufwp &= (KEYBUFSIZ-1);
    }
}
int keybufempty(void) {
	return (keybufrp == keybufwp);
}
char keybufget(void) {
    char c;
    if (keybufempty()) return 0; /* empty */
    c = keybuf[keybufrp++];
    keybufrp &= (KEYBUFSIZ-1);
    return c;
}
char keybufpeek(void) {
    char c;
    if (keybufempty()) return 0; /* empty */
    return keybuf[keybufrp];
}
/*
 *  virtual Key translation tables.
 */
#define VKEYPADLO	        0x60
#define VKEYPADHI	        0x6E
#define VPADKEYS 	        (VKEYPADHI - VKEYPADLO + 1)
#define KEY_PRESSED		0x80
#define KEY_TOGGLED		0x01
/*
 * Keypad keys are translated to the normal values below from
 * Windows virtual keys. Shifted keypad keys are translated to the
 * shift values below.
 */
static struct vkpad {
	unsigned char normal, shift, ctrl;
} vkeypad[VPADKEYS] = {
    {'i',   'I',  C('i')},      /* Ins */
    {'b',   'B',  C('b')},      /* 1 */
    {'j',   'J',  C('j')},      /* 2 */
    {'n',   'N',  C('n')},      /* 3 */
    {'h',   'H',  C('h')},      /* 4 */
    {'g',   'g',    'g' },      /* 5 */
    {'l',   'L',  C('l')},      /* 6 */
    {'y',   'Y',  C('y')},      /* 7 */
    {'k',   'K',  C('k')},      /* 8 */
    {'u',   'U',  C('u')},      /* 9 */
    { 0 ,    0,      0  },      /* * */
    {'p',   'P',  C('p')},      /* + */
    { 0 ,    0,      0  },      /* sep */
    {'m', C('p'), C('p')},      /* - */
    {'.',   ':',    ':' }       /* Del */
}, vnumpad[VPADKEYS] = {
    {'0', M('0'),   '0' },      /* Ins */
    {'1', M('1'),   '1' },      /* 1 */
    {'2', M('2'),   '2' },      /* 2 */
    {'3', M('3'),   '3' },      /* 3 */
    {'4', M('4'),   '4' },      /* 4 */
    {'5', M('5'),   '5' },      /* 5 */
    {'6', M('6'),   '6' },      /* 6 */
    {'7', M('7'),   '7' },      /* 7 */
    {'8', M('8'),   '8' },      /* 8 */
    {'9', M('9'),   '9' },      /* 9 */
    { 0 ,    0,      0  },      /* * */
    {'p',   'P',  C('p')},      /* + */
    { 0 ,    0,      0  },      /* sep */
    {'m', C('p'), C('p')},      /* - */
    {'.',   ':',    ':' }       /* Del */
};
/*---------------------------------------------------------------------------
	1 Line Input by Edit control
  ---------------------------------------------------------------------------*/
char editBoxText[1024];
HWND editBox;
WNDPROC origEditBoxProc;
int editBoxX, editBoxY;
int editWid;
DWORD editBoxSel1, editBoxSel2;
int editBoxSuspended;
void closeEditBox(void) {
	HWND hwnd;
	HIMC ctx;

	hwnd = GetParent(editBox);
	/* copy the displayed input string to back buffer before closing edit box */
	if (editBoxText[0]) {
	    HWND hwndd;
	    HDC hdcd;
	    RECT r;
	    SendMessage(editBox, EM_SETSEL, -1, 0); /* deselect */
	    SendMessage(editBox, WM_KILLFOCUS, 0, 0); /* hide caret */
	    GetWindowRect(editBox, &r);
	    hwndd = GetDesktopWindow(); /* sigh */
	    hdcd = GetDC(hwndd);
	    BitBlt(bkbuf.hdcMem,editBoxX,editBoxY,r.right-r.left,r.bottom-r.top,hdcd,r.left,r.top,SRCCOPY);
	    ReleaseDC(hwndd, hdcd);
	}
	if (editBox != NULL) DestroyWindow(editBox);
	editBox = NULL;
	editBoxSuspended = 0;

	/* close IME */
	ctx = ImmGetContext(hwnd);
	ImmSetOpenStatus(ctx, FALSE);
	ImmReleaseContext(hwnd, ctx);
}
LRESULT CALLBACK editBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	    case WM_CHAR:
		if ((int)wParam == C('p')) {
		    GetWindowText(editBox, editBoxText, 1024);
		    SendMessage(editBox, EM_GETSEL, (WPARAM)&editBoxSel1, (LPARAM)&editBoxSel2);
		    closeEditBox();
		    editBoxSuspended = 1;
		    return 0;
		}
		if ((int)wParam == C('m')) { /* Ctrl-M */
		    GetWindowText(editBox, editBoxText, 1024);
		    closeEditBox();
		    return 0;
		}
		break;
	    case WM_KEYDOWN:
		if ((int)wParam == VK_RETURN) { /* Enter */
		    GetWindowText(editBox, editBoxText, 1024);
		    closeEditBox();
		    return 0;
		} else if ((int)wParam == VK_ESCAPE) {
		    editBoxText[0] = 0; /* Cancelled */
		    closeEditBox();
		    return 0;
		}
		break;
	    case WM_SYSKEYDOWN:
		if (wParam == VK_MENU) return 0;
		break;
	    default:
		break;
	}
	return CallWindowProc(origEditBoxProc, hwnd, uMsg, wParam, lParam);
}
void openEditBox0(HWND hwnd, int nhwid, WNDPROC proc) {
	int x, y;
	if (editBoxSuspended == 0)
	    editBoxText[0] = 0;
	x = nhwi[nhwid].cx + nhwi[nhwid].winx;
	y = nhwi[nhwid].cy + nhwi[nhwid].winy;
	editBoxX = x; editBoxY = y;
	editWid = nhwid;
	editBox = CreateWindow("EDIT", "", WS_CHILD,
				x, y, nhwi[nhwid].winw - nhwi[nhwid].cx, nhwi[nhwid].fh,
				hwnd, NULL, hInst, NULL);
	origEditBoxProc = (WNDPROC)GetWindowLong(editBox, GWL_WNDPROC);
	SetWindowLong(editBox, GWL_WNDPROC, (LONG)proc);
	SendMessage(editBox, WM_SETFONT, (WPARAM)nhwi[nhwid].font, (LPARAM)FALSE);
	if (editBoxSuspended) {
	    editBoxSuspended = 0;
	    SetWindowText(editBox, editBoxText);
	    SendMessage(editBox, EM_SETSEL, editBoxSel1, editBoxSel2);
	}
	ShowWindow(editBox, SW_SHOW);
	SetFocus(editBox);
}
void openEditBox(HWND hwnd, int nhwid) {
	openEditBox0(hwnd, nhwid, editBoxProc);
}
void focusEditBox(void) {
	if (editBox != NULL) SetFocus(editBox);
}
const char *editChoices;
char *editChoosen;
char editDefChar;
int editAllowCnt;
LRESULT CALLBACK editBoxProcYN(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char c;
	int i, j, l;
	switch (uMsg) {
	    case WM_CHAR:
		c = (char)wParam;
		if (editChoices == NULL) {
		    if (!isprint(c)) c = ' ';
		    goto xit;
		}
		if (c == 0x08) break; /* backspace */
		if (c == C('p')) { /* Ctrl-p */
		    GetWindowText(editBox, editBoxText, 1024);
		    SendMessage(editBox, EM_GETSEL, (WPARAM)&editBoxSel1, (LPARAM)&editBoxSel2);
		    closeEditBox();
		    editBoxSuspended = 1;
		    return 0;
		}
		if (editAllowCnt && c == '#') return 0;
		if (editAllowCnt && c >= '0' && c <= '9') break;
		if (c == 0x1b) { /* Escape */
		    c = editDefChar;
		    for (i=0; i<strlen(editChoices); i++) if (editChoices[i] == 'n') c = 'n';
		    for (i=0; i<strlen(editChoices); i++) if (editChoices[i] == 'q') c = 'q';
		    *editChoosen = c; /* cancelled */
		    GetWindowText(editBox, editBoxText, 1024);
		    l = strlen(editBoxText);
		    SendMessage(editBox, EM_SETSEL, (WPARAM)l, (LPARAM)l);
		    CallWindowProc(origEditBoxProc, hwnd, uMsg, (WPARAM)c, lParam);
		    editBoxText[0] = 0;
		    closeEditBox();
		    return 0;
		}
		if (c == 0x0d || c == ' ') c = editDefChar;
		if (!c && c == editDefChar) j = 1;
		else for (i=0, j=0; i<strlen(editChoices); i++)
		    if (c == editChoices[i]) { j = 1; break; }
		if (j) {
xit:		    *editChoosen = c;
		    GetWindowText(editBox, editBoxText, 1024);
		    l = strlen(editBoxText);
		    SendMessage(editBox, EM_SETSEL, (WPARAM)l, (LPARAM)l);
		    CallWindowProc(origEditBoxProc, hwnd, uMsg, (WPARAM)c, lParam);
		    if (editAllowCnt) GetWindowText(editBox, editBoxText, 1024);
		    else editBoxText[0] = 0;
		    closeEditBox();
		    return 0;
		} else
		    return 0;
	    case WM_SYSKEYDOWN:
		if (wParam == VK_MENU) return 0;
		break;
	    case WM_KEYDOWN:
		i = -1;
		switch((int)wParam) {
		    case VK_INSERT: i = 0;    break;
		    case VK_END:    i = 1;    break;
		    case VK_DOWN:   i = 2;    break;
		    case VK_NEXT:   i = 3;    break;
		    case VK_LEFT:   i = 4;    break;
		    case VK_CLEAR:  i = 5;    break;
		    case VK_RIGHT:  i = 6;    break;
		    case VK_HOME:   i = 7;    break;
		    case VK_UP:     i = 8;    break;
		    case VK_PRIOR:  i = 9;    break;
		    case VK_DELETE: i = 14;   break;
		    default:
			break;
		}
		if (i != -1) {
		    struct vkpad *cur_pad = (iflags.num_pad ? vnumpad : vkeypad);
		    c = cur_pad[i].normal;
		    goto xit;
		}
		break;
	    default:
		break;
	}
	return CallWindowProc(origEditBoxProc, hwnd, uMsg, wParam, lParam);
}
void openEditBoxYN(HWND hwnd, int nhwid,
		   const char *choices, char defc, char *choosen, char firstc) {
	int i;
	editChoices = choices;
	editDefChar = defc;
	editChoosen = choosen;
	editAllowCnt = 0;
	if (editChoices != NULL)
	    for (i=0; i<strlen(editChoices); i++) if (editChoices[i] == '#')
		{ editAllowCnt = 1; break; }
	if (editAllowCnt && firstc) {
	    editBoxText[0] = firstc;
	    editBoxText[1] = 0;
	    editBoxSuspended = 1;
	}
	openEditBox0(hwnd, nhwid, editBoxProcYN);
}
char * (*editExtCallback)(char *);
LRESULT CALLBACK editBoxProcExt(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char *p;
	int i, j, l;
	DWORD s1, s2;
	switch (uMsg) {
	    case WM_CHAR:
		if ((int)wParam == C('p')) {
		    GetWindowText(editBox, editBoxText, 1024);
		    SendMessage(editBox, EM_GETSEL, (WPARAM)&editBoxSel1, (LPARAM)&editBoxSel2);
		    closeEditBox();
		    editBoxSuspended = 1;
		    return 0;
		}
		if ((int)wParam == C('m')) { /* Ctrl-M */
		    GetWindowText(editBox, editBoxText, 1024);
		    closeEditBox();
		    return 0;
		}
		CallWindowProc(origEditBoxProc, hwnd, uMsg, wParam, lParam);
		GetWindowText(editBox, editBoxText, 1024);
		SendMessage(editBox, EM_GETSEL, (WPARAM)&s1, (LPARAM)&s2);
		if (s1 == s2) editBoxText[s1] = 0;
		p = editExtCallback(editBoxText);
		if (p == NULL) p = editBoxText;
		SetWindowText(editBox, p);
		SendMessage(editBox, EM_SETSEL, s1, s1);
		return 0;
	    case WM_KEYDOWN:
		if ((int)wParam == VK_RETURN) { /* Enter */
		    GetWindowText(editBox, editBoxText, 1024);
		    editExtCallback(editBoxText);
		    closeEditBox();
		    return 0;
		} else if ((int)wParam == VK_ESCAPE) {
		    editBoxText[0] = 0; /* Cancelled */
		    editExtCallback(editBoxText);
		    closeEditBox();
		    return 0;
		}
		break;
	    case WM_SYSKEYDOWN:
		if (wParam == VK_MENU) return 0;
		break;
	    default:
		break;
	}
	return CallWindowProc(origEditBoxProc, hwnd, uMsg, wParam, lParam);
}
void openEditBoxExt(HWND hwnd, int nhwid, char * (*callbackfunc)(char *)) {
	int i;
	editExtCallback = callbackfunc;
	openEditBox0(hwnd, nhwid, editBoxProcExt);
}
int waitEditBoxClosed(void) {
	MSG msg;
	while (editBox != NULL && GetMessage(&msg, NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	return (editBoxSuspended);
}

/*---------------------------------------------------------------------------*/
#define MAXARGS 32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow){

	char *p1, *p2;
	int argc;
	char *argv[MAXARGS];
	int argdbc;

	hInst = hInstance;

	/* prepare argc/argv[] */
	cmdline = (char *)malloc(lstrlen(szCmdLine)+1);
	if (cmdline == NULL) return 0;
	argv[0] = "NetHack.exe";
	for (p1 = szCmdLine, p2 = cmdline, argc=1, argdbc = 0; *p1 != 0;) {
	    if (isspace(*p1)) { p1++; continue; }
	    if (*p1 == '\"') { argdbc ^= 1; p1++; }
	    argv[argc++] = p2;
	    while (*p1 != 0 && (!isspace(*p1) || argdbc)) {
		if (*p1 == '\"') { argdbc ^= 1; p1++; continue; }
		*p2++ = *p1++;
	    }
	    *p2++ = 0;
	    if (*(argv[argc-1]) == 0) argc--; /* remove null string */
	    if (argc >= MAXARGS) break;
	}

	GetRegistoryData();
	savereg = 1;

	pcmain(argc, argv);
	nhrunning = 1;
	moveloop();

	/* notreached */
	return 0;

}

HWND myhwnd;
char *wincname = "NetHack Window";
void openWindow(void) {
	WNDCLASSEX  wndclass;
	RECT r;

	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInst;
	wndclass.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(1));
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = NULL;/*(HBRUSH)GetStockObject(BLACK_BRUSH);*/
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = wincname;
	wndclass.hIconSm       = NULL;

	RegisterClassEx(&wndclass); /* ウインドウクラスTest Windowを登録 */

	myhwnd = CreateWindow(
           wincname,					/* ウインドウクラス名 */
	   DEF_GAME_NAME,				/* ウインドウのタイトル */
           WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME),	/* ウインドウスタイル */
           win_left, win_top,				/* ウインドウ表示位置 */
           320,/*temp*/					/* ウインドウの大きさ */
           200,/*temp*/
           NULL,					/* 親ウインドウのハンドル */
           NULL,					/* メニューのハンドル */
           hInst,					/* インスタンスのハンドル */
           NULL);					/* 作成時の引数保存用ポインタ */

	init_nhw(myhwnd);

	r.left   = r.top = 0;
	r.right  = bkbuf.w;
	r.bottom = bkbuf.h;
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME), FALSE/*nomenu*/);
	SetWindowPos(myhwnd, 0, 0, 0, r.right - r.left, r.bottom - r.top,
		     SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER/*|SWP_NOREDRAW*/);

	ShowWindow(myhwnd, SW_SHOWNORMAL);      /* ウインドウを表示 */
	UpdateWindow(myhwnd);
}

void closeWindow(void) {
	DestroyWindow(myhwnd);
	if (cmdline) { free(cmdline); cmdline = 0; }
	tini_nhw();
}

/*---------------------------------------------------------------------------*/

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	int i;

	switch (iMsg) {

	    case WM_CREATE:
		ShowWindow(hwnd, SW_SHOW);
		RemoveMenu(GetSystemMenu(hwnd, FALSE), SC_MAXIMIZE, FALSE);
		AppendMenu(GetSystemMenu(hwnd, FALSE), MF_SEPARATOR, 0, 0);
		AppendMenu(GetSystemMenu(hwnd, FALSE), MF_STRING, 1000, "Clear registry info");
		return 0;

	    case WM_SYSCOMMAND:
		if (LOWORD(wParam) == 1000) {
		    DisposeRegistoryData();
		    savereg = 0;
		    MessageBox(hwnd, "Registory data is cleared.", "NetHack", MB_OK);
		    return 0;
		}
		break;

	    case WM_PAINT:
		hdc=BeginPaint(hwnd,&ps);
		nhw_display(hdc);
		EndPaint(hwnd,&ps);
		return 0;

	    case WM_DESTROY:
		if (savereg) {
		    RECT r;
		    GetWindowRect(hwnd, &r);
		    win_left = r.left;
		    win_top  = r.top;
		    SetRegistoryData();
		}
		PostQuitMessage(0);
		return 0;

	    case WM_CLOSE:
		keybufput(M('q'));
		return 0;

	    case WM_CHAR:
		if (editBox != NULL) {
		    WNDPROC proc;
		    proc = (WNDPROC)GetWindowLong(editBox, GWL_WNDPROC);
		    proc(editBox, iMsg, wParam, lParam);
		    return 0;
		}
		keybufput((char)wParam);
		return 0;

	    case WM_KEYDOWN:
	    case WM_KEYUP: {
		char	ch;
		BYTE	shift_ks, ctrl_ks, numlock_ks;
		BYTE	kb_stat[256];
		int	idx = -1;
		struct vkpad *cur_pad = (iflags.num_pad ? vnumpad : vkeypad);
		static BYTE shiftstate = 0;
		static DWORD shift_released_time = 0;

		/* get current keyboard status */
		ZeroMemory(&kb_stat, sizeof(kb_stat));
		GetKeyboardState(kb_stat);
		numlock_ks = (kb_stat[VK_NUMLOCK] & KEY_TOGGLED);
		shift_ks   = (kb_stat[VK_SHIFT] & KEY_PRESSED);
		ctrl_ks    = (kb_stat[VK_CONTROL] & KEY_PRESSED);

		/* process numberpad keys */
		switch((int)wParam) {
		    case VK_INSERT: idx = 0;    break;
		    case VK_END:    idx = 1;    break;
		    case VK_DOWN:   idx = 2;    break;
		    case VK_NEXT:   idx = 3;    break;
		    case VK_LEFT:   idx = 4;    break;
		    case VK_CLEAR:  idx = 5;    break;
		    case VK_RIGHT:  idx = 6;    break;
		    case VK_HOME:   idx = 7;    break;
		    case VK_UP:     idx = 8;    break;
		    case VK_PRIOR:  idx = 9;    break;
		    case VK_DELETE: idx = 14;   break;
		    default:
			if (numlock_ks && iflags.num_pad) {
			    if (wParam == VK_SHIFT && iMsg == WM_KEYUP) {
				shift_released_time = GetTickCount();
			    } else {
				shiftstate = shift_ks;
				shift_released_time = 0;
			    }
			}
			break;
		}

		if(idx != -1 && (kb_stat[(int)wParam] & KEY_PRESSED)) {
			if (shift_released_time &&
			    (GetTickCount() - shift_released_time) < 10) {
				shift_ks = shiftstate;
			}
			ch = ctrl_ks ? cur_pad[idx].ctrl
		            : (shift_ks ? cur_pad[idx].shift : cur_pad[idx].normal);
			keybufput(ch);
			return 0;
		}
	    }
	    case WM_SYSCHAR:
		/* Process META-Key + Char */
		i = (int)wParam;
		if (lParam & (1<<29)) {
		    if ((i >= 'a' && i <= 'z') ||
			(i >= '0' && i <= '9'))
			keybufput(M(tolower(i)));
		    return 0;
		}
		break;
	    case WM_SYSKEYDOWN:
		/* if Alt is pressed with no combination key, ignore it */
		if (wParam == VK_MENU) return 0;
		break;
	    case WM_ACTIVATE:
		if (wParam != WA_INACTIVE && editBox != NULL) {
		    focusEditBox();
		    return 0;
		}
		break;
	    case WM_ERASEBKGND:
		return 1;
	    case WM_CTLCOLOREDIT:
		if (editBox && (HWND)lParam == editBox) {
		    hdc = (HDC)wParam;
		    SetBkMode(hdc, TRANSPARENT);
		    SetTextColor(hdc, colortable[nhwi[editWid].fc]);
		    SetBkColor(hdc, colortable[nhwi[editWid].bc]);
		    return (LRESULT)nhwi[editWid].bgbrush;
		}
		break;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

/*---------------------------------------------------------------------------*/

char *keystring = "Software\\NetHack\\NetHack 3.4.3\\Settings";

void GetRegistoryData(void) {
	long lerr;
	HKEY hKey;
	DWORD dwData;
	DWORD dwType = REG_DWORD;
	win_left = CW_USEDEFAULT;
	win_top  = CW_USEDEFAULT;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keystring, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
	    dwData = sizeof(win_left);
	    lerr = RegQueryValueEx(hKey, "MainLeft", NULL, &dwType, (LPBYTE)&win_left, &dwData);
	    if ((lerr != ERROR_SUCCESS) || (dwType != REG_DWORD)) win_left = CW_USEDEFAULT;
	    dwData = sizeof(win_top);
	    lerr = RegQueryValueEx(hKey, "MainTop", NULL, &dwType, (LPBYTE)&win_top, &dwData);
	    if ((lerr != ERROR_SUCCESS) || (dwType != REG_DWORD)) win_top = CW_USEDEFAULT;
	}
	RegCloseKey(hKey);
}
void SetRegistoryData(void) {
	long lerr;
	DWORD dwDisposition;
	HKEY hKey;
	DWORD dwData;
	lerr = RegCreateKeyEx(HKEY_CURRENT_USER, keystring,
			      NULL, NULL, REG_OPTION_NON_VOLATILE,
			      KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	if (lerr == ERROR_SUCCESS) {
	    RegSetValueEx(hKey, "MainLeft", 0, REG_DWORD, (LPBYTE)&win_left, sizeof(win_left));
	    RegSetValueEx(hKey, "MainTop",  0, REG_DWORD, (LPBYTE)&win_top,  sizeof(win_top) );
	}
	RegCloseKey(hKey);
}
void DisposeRegistoryData(void) {
	HKEY hKey;
	DWORD nrsubkeys;
	RegDeleteKey(HKEY_CURRENT_USER, keystring);
	RegDeleteKey(HKEY_CURRENT_USER, "Software\\NetHack\\NetHack 3.4.3");
	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\NetHack", 0, KEY_READ, &hKey); 
	nrsubkeys = 0;
	RegQueryInfoKey(hKey, NULL, NULL, NULL, &nrsubkeys, NULL, NULL, NULL, 
			NULL, NULL, NULL, NULL);
	RegCloseKey(hKey);
	if (nrsubkeys == 0) RegDeleteKey(HKEY_CURRENT_USER, "Software\\NetHack");
}
