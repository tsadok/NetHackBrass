#include "nhw.h"

#define DO_WORDBREAK

#define WSPACING 2
#define MENUSPACING 8

extern void openEditBox(HWND, int, int, int, int, HFONT);
extern void openEditBoxYN(HWND, int, int, int, int, HFONT, const char *, char, char *);
extern void openEditBoxExt(HWND, int, int, int, int, HFONT, char * (*)(char *));
extern int waitEditBoxClosed(void);
extern void store_prevmsg(char *, int);

extern char toplines[];

struct {
	char	*fname;		/* font name */
	int	fsize;		/* font size(height) */
} deffonts[NHW_WINID_MAX] = {
	{ "Terminal",	18 },	/* MAP     */
	{ "Arial",	18 },	/* MESSAGE */
	{ "Arial",	18 },	/* STATUS  */
	{ "Courier",	18 },	/* TEXT    */
	{ "Arial",	18 },	/* MENU	   */
	{ "Arial",	18 }	/* BASE    */
};

nhwWinInfo nhwi[NHW_WINID_MAX];

int inited;
nhwBackBuf bkbuf;
nhwBackBuf bkbufsub;
int subwinx, subwiny;
int subwinw, subwinh;

int mapfontw, mapfonth;

static char *morestr = "--More--";
static int morewidth;

static char lastmsg[1024];

static int currcolor;

/*------------------------------------------------
	color table
  ------------------------------------------------*/
COLORREF colortable[17] = {
	RGB(0x55, 0x55, 0x55),	/* black */
	RGB(0xc0,    0,    0),	/* red */
	RGB(   0, 0x80,    0),	/* green */
	RGB(0x80, 0x60,    0),	/* brown */
	RGB(   0,    0, 0xc0),	/* blue */
	RGB(0x80,    0, 0x80),	/* magenta */
	RGB(   0, 0x80, 0x80),	/* cyan */
	RGB(0xc0, 0xc0, 0xc0),	/* gray */
	RGB(0xff, 0xff, 0xff),	/* no-color */
	RGB(0xff, 0x80,    0),	/* orange */
	RGB(   0, 0xff,    0),	/* bright green */
	RGB(0xff, 0xff,    0),	/* yellow */
	RGB(   0,    0, 0xff),	/* bright blue */
	RGB(0xff,    0, 0xff),	/* bright magenta */
	RGB(   0, 0xff, 0xff),	/* bright cyan */
	RGB(0xff, 0xff, 0xff),	/* white */
	RGB(   0,    0,    0)	/* (background) */
};

/*------------------------------------------------
	back buffer
  ------------------------------------------------*/
void createBackBuffer(HWND hWnd, int w, int h, nhwBackBuf *bkbuf) {
	HDC	hdcWin, hdcMem;
	HBITMAP	hBMP;
	LPBYTE	lpBMP;
	BITMAPINFO myDIB;

	ZeroMemory(&myDIB, sizeof(BITMAPINFO));
	myDIB.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER); /* BITMAPINFO構造体 */
	myDIB.bmiHeader.biWidth		= w;
	myDIB.bmiHeader.biHeight	= h;
	myDIB.bmiHeader.biPlanes	= 1;
	myDIB.bmiHeader.biBitCount	= 32;
	myDIB.bmiHeader.biCompression	= BI_RGB;

	hdcWin = GetDC(hWnd); /* ウインドウのDC を取得 */

	/* DIB とウインドウのDC からDIBSection を作成 */
	hBMP = CreateDIBSection(hdcWin, &myDIB, DIB_RGB_COLORS, &lpBMP, NULL, 0);
	hdcMem = CreateCompatibleDC(hdcWin); /* メモリDC を作成 */
	SelectObject(hdcMem, hBMP); /* メモリDC にビットマップを選択 */

	ReleaseDC(hWnd, hdcWin);

	bkbuf->hBMP   = hBMP;
	bkbuf->hdcMem = hdcMem;
	bkbuf->lpBMP  = lpBMP;
	bkbuf->hWnd   = hWnd;
	bkbuf->w      = w;
	bkbuf->h      = h;
}

void destroyBackBuffer(nhwBackBuf *bkbuf) {
	DeleteDC(bkbuf->hdcMem);
	DeleteObject(bkbuf->hBMP);
	bkbuf->hBMP   = 0;
	bkbuf->hdcMem = 0;
	bkbuf->lpBMP  = 0;
}

HDC getHDCNHW(int nhwid) {
	return (nhwid == NHW_WINID_MENU || nhwid == NHW_WINID_TEXT) ? bkbufsub.hdcMem : bkbuf.hdcMem;
}
void updateNHW(int nhwid) {
#if 0
	RECT r;
	r.left	 = nhwi[nhwid].winx;
	r.top	 = nhwi[nhwid].winy;
	r.right	 = r.left + nhwi[nhwid].winw;
	r.bottom = r.top  + nhwi[nhwid].winh;
	InvalidateRect(bkbuf.hWnd, &r, FALSE);
#else
	RECT r;
	HDC hdc;
	if (nhwid != NHW_WINID_MENU) {
	    hdc = GetDC(bkbuf.hWnd);
	    BitBlt(hdc, nhwi[nhwid].winx, nhwi[nhwid].winy, nhwi[nhwid].winw, nhwi[nhwid].winh,
		   getHDCNHW(nhwid), nhwi[nhwid].winx, nhwi[nhwid].winy, SRCCOPY);
	    ReleaseDC(bkbuf.hWnd, hdc);
	} else {
	    hdc = GetDC(bkbuf.hWnd);
	    nhw_display(hdc);
	    ReleaseDC(bkbuf.hWnd, hdc);
//	    r.left   = nhwi[nhwid].winx;
//	    r.top    = nhwi[nhwid].winy;
//	    r.right  = r.left + nhwi[nhwid].winw;
//	    r.bottom = r.top  + nhwi[nhwid].winh;
//	    InvalidateRect(bkbuf.hWnd, &r, FALSE);
	}
#endif
}
void clearNHW(int nhwid) {
	RECT r;
	if (nhwid == NHW_WINID_MENU || nhwid == NHW_WINID_TEXT) {
	    r.left   = 0;
	    r.top    = 0;
	    r.right  = bkbuf.w;
	    r.bottom = bkbuf.h;
	} else {
	    r.left   = nhwi[nhwid].winx;
	    r.top    = nhwi[nhwid].winy;
	    r.right  = r.left + nhwi[nhwid].winw;
	    r.bottom = r.top  + nhwi[nhwid].winh;
	}
	FillRect(getHDCNHW(nhwid), &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
	nhwi[nhwid].cx = 0;
	nhwi[nhwid].cy = 0;
	if (nhwid == NHW_WINID_MESSAGE) toplines[0] = 0;
}
void drawenvNHW(int nhwid) {
	HDC hdc;
	hdc = getHDCNHW(nhwid);
	SelectObject(hdc, nhwi[nhwid].font);
	SetTextColor(hdc, colortable[currcolor]);
	SetBkColor(hdc, RGB(0,0,0));
}
void setcolorNHW(int nhwid, int fc, int bc) {
	HDC hdc;
	hdc = getHDCNHW(nhwid);
	SetTextColor(hdc, colortable[fc]);
	SetBkColor(hdc, colortable[bc]);
}

/*------------------------------------------------
	font
  ------------------------------------------------*/
HFONT getfont(char *name, int size) {
	LOGFONT lf;

	ZeroMemory(&lf, sizeof(LOGFONT));
	lf.lfHeight		= size;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfPitchAndFamily	= DEFAULT_PITCH | FF_DONTCARE;
//lf.lfWeight = FW_BOLD;
//lf.lfUnderline = TRUE;
	if (!name || !*name || !strcmpi(name, "Terminal")) {
		strcpy(lf.lfFaceName, "Terminal");
		lf.lfCharSet = OEM_CHARSET;
		if (lf.lfHeight == 0) {
		    lf.lfWidth  = mapfontw;
		    lf.lfHeight = mapfonth;
		}
	} else {
		strcpy(lf.lfFaceName, name);
		lf.lfCharSet = DEFAULT_CHARSET;
	}
	return CreateFontIndirect(&lf);
}

/*------------------------------------------------
	window procs
  ------------------------------------------------*/
static int showmode;
void nhw_display(HDC hdc) {
//	if (showmode & 1)
	    BitBlt(hdc, 0, 0, bkbuf.w, bkbuf.h, bkbuf.hdcMem, 0, 0, SRCCOPY);
	if (showmode & 2) {
	    BitBlt(hdc, subwinx, subwiny, subwinw, subwinh,
		   bkbufsub.hdcMem, 0, 0, SRCCOPY);
	}
}

void nhw_update(int nhwid) {
	updateNHW(nhwid);
}

HWND nhw_getHWND(void) {
	return bkbuf.hWnd;
}

/*------------------------------------------------
	cursor location
  ------------------------------------------------*/
void nhw_curs(int nhwid, int x, int y) {
	if (nhwid == NHW_WINID_MAP) {
		nhwi[nhwid].cx = x;
		nhwi[nhwid].cy = y;
	} else {
		nhwi[nhwid].cx = x * nhwi[nhwid].fw;
		nhwi[nhwid].cy = y * nhwi[nhwid].fh;
	}
}

int nhw_cury(int nhwid) {
	return (nhwi[nhwid].cy / nhwi[nhwid].fh);
}

void nhw_setpos(int nhwid, int x, int y) {
	if (x >= 0) nhwi[nhwid].cx = x;
	if (y >= 0) nhwi[nhwid].cy = y;
}

int nhw_setcolor(int col) {
	int prevc;
	prevc = currcolor;
	currcolor = col;
	return prevc;
}

/*------------------------------------------------
	raw print to BASE window
  ------------------------------------------------*/
void nhw_raw_print(char *msg) {

	if (!inited) {
	    MessageBox(NULL, msg, "NetHack", MB_OK);
	    return;
	}

	drawenvNHW(NHW_WINID_BASE);
	TextOut(bkbuf.hdcMem, nhwi[NHW_WINID_BASE].winx, nhwi[NHW_WINID_BASE].winy + nhwi[NHW_WINID_BASE].cy,
		msg, strlen(msg));
	updateNHW(NHW_WINID_BASE);
	nhwi[NHW_WINID_BASE].cx = 0;
	nhwi[NHW_WINID_BASE].cy += nhwi[NHW_WINID_BASE].fh;
}

void nhw_raw_print_bold(char *msg) {
	int c;
	c = currcolor;
	currcolor |= 0x08;
	nhw_raw_print(msg);
	currcolor = c;
}

/*------------------------------------------------
	print string to window
  ------------------------------------------------*/
void nhw_print(int nhwid, char *msg, int attr) {
	HDC hdc;
	SIZE sz;
	int l;
	int ul = 0;
	RECT r;

	hdc = getHDCNHW(nhwid);
	drawenvNHW(nhwid);
	switch (attr) {
	    case ATR_BOLD:
		setcolorNHW(nhwid, currcolor | 0x08, 16);
		break;
	    case ATR_BLINK:
	    case ATR_INVERSE:
		setcolorNHW(nhwid, 16, currcolor);
		break;
	    case ATR_ULINE: {
		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);
		ul = tm.tmAscent;
		}
		break;
	}
	l = strlen(msg);
	GetTextExtentPoint32(hdc, msg, l, &sz);
	TextOut(hdc,
		nhwi[nhwid].winx + nhwi[nhwid].cx,
		nhwi[nhwid].winy + nhwi[nhwid].cy,
		msg, l);
	if (ul) { /* underline */
	    HBRUSH btmp = CreateSolidBrush(colortable[currcolor]);
	    r.left   = nhwi[nhwid].winx + nhwi[nhwid].cx;
	    r.top    = nhwi[nhwid].winy + nhwi[nhwid].cy + ul;
	    r.right  = r.left + sz.cx;
	    r.bottom = r.top + 1;
	    FillRect(hdc, &r, btmp);
	    DeleteObject(btmp);
	}
	nhwi[nhwid].cx += sz.cx;

	/* clear trailing area */
	r.left   = nhwi[nhwid].winx + nhwi[nhwid].cx;
	r.top    = nhwi[nhwid].winy + nhwi[nhwid].cy;
	r.right  = nhwi[nhwid].winx + nhwi[nhwid].winw;
	r.bottom = r.top + nhwi[nhwid].fh;
	FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));

	if (nhwid == NHW_WINID_MENU || nhwid == NHW_WINID_TEXT) return;
	updateNHW(nhwid);
}

void nhw_clear(int nhwid) {
	clearNHW(nhwid);
	if (nhwid == NHW_WINID_MENU || nhwid == NHW_WINID_TEXT) return;
	updateNHW(nhwid);
}

/*------------------------------------------------
	get string width by pixel
  ------------------------------------------------*/
int nhw_getstrwidth(int nhwid, char *msg) {
	HDC hdc;
	SIZE sz;
	int l;

	hdc = getHDCNHW(nhwid);
	drawenvNHW(nhwid);
	l = strlen(msg);
	GetTextExtentPoint32(hdc, msg, l, &sz);
	return sz.cx;
}

int nhw_getMwidth(int nhwid) {
	return nhwi[nhwid].fw;
}

int nhw_is_proportional(int nhwid) {
	return nhwi[nhwid].isprop;
}

/*------------------------------------------------
	print glyph
  ------------------------------------------------*/
void nhw_printglyph(int x, int y, int chr, int fc, int bc) {
	HDC hdc;
	int xx, yy;
	char buf[4];
	SelectObject(bkbuf.hdcMem, nhwi[NHW_WINID_MAP].font);
	SetTextColor(bkbuf.hdcMem, colortable[fc]);
	SetBkColor(bkbuf.hdcMem, colortable[bc]);
	buf[0] = (char)chr;
	buf[1] = 0;
	xx = nhwi[NHW_WINID_MAP].winx + x * nhwi[NHW_WINID_MAP].fw;
	yy = nhwi[NHW_WINID_MAP].winy + y * nhwi[NHW_WINID_MAP].fh;
	TextOut(bkbuf.hdcMem, xx, yy, buf, 1);
	hdc = GetDC(bkbuf.hWnd);
	BitBlt(hdc, xx, yy, nhwi[NHW_WINID_MAP].fw, nhwi[NHW_WINID_MAP].fh,
	       bkbuf.hdcMem, xx, yy, SRCCOPY);
	ReleaseDC(bkbuf.hWnd, hdc);
}

/*------------------------------------------------
	message line
  ------------------------------------------------*/
char waitmorekey(char let) {
	int c;
	while (1) {
	    c = win32y_nhgetch();
	    if (c == ' ' || c == 0x0d)
		return 0; /* normal proceed */
	    if (c == 0x1b || c == (0x80 | 'q'))
		return 0x1b; /* cancelled */
	    if (c == let)
		return let;
	    if (c == C('p'))
		return c;
	}
	/* notreached */
	return 0;
}

char nhw_do_more(char let) {
	char buf[1024];
	char c;
	SIZE sz;
	int l;

	drawenvNHW(NHW_WINID_MESSAGE);

	if (!toplines[0]) return 0;
	wsprintf(buf, "%s%s", toplines, morestr);
	l = strlen(buf);
	do {
	    TextOut(bkbuf.hdcMem, nhwi[NHW_WINID_MESSAGE].winx, nhwi[NHW_WINID_MESSAGE].winy, buf, l);
	    updateNHW(NHW_WINID_MESSAGE);
	    c = waitmorekey(let);
	    if (c == C('p')) {
		win32y_doprev_message();
		win32y_consume_prevmsgkey();
	    }
	    clearNHW(NHW_WINID_MESSAGE);
	    updateNHW(NHW_WINID_MESSAGE);
	} while (c == C('p'));
	return c;
}

void nhw_print_msg(char *msg) {
	char buf[1024];
	char *msg2;
	int pw[1024];
	SIZE sz;
	int i, l, m;

	drawenvNHW(NHW_WINID_MESSAGE);

	if (toplines[0]) {
	    /* try packing */
	    wsprintf(buf, "%s  %s", toplines, msg);
	    l = strlen(buf);
	    GetTextExtentPoint32(bkbuf.hdcMem, buf, l, &sz);
	    if (sz.cx <= nhwi[NHW_WINID_MESSAGE].winw - morewidth && l < TBUFSZ) {
		/* successfully packed */
		TextOut(bkbuf.hdcMem, nhwi[NHW_WINID_MESSAGE].winx, nhwi[NHW_WINID_MESSAGE].winy, buf, l);
		updateNHW(NHW_WINID_MESSAGE);
		strcpy(toplines, buf);
		nhwi[NHW_WINID_MESSAGE].cx = sz.cx;
		store_prevmsg(toplines, 0);
	    } else {
		nhw_do_more(0);
		drawenvNHW(NHW_WINID_MESSAGE);/*may be changed in update process*/
		goto nopack;
	    }
	} else {
nopack:	    /* does msg fit in message window? */
	    for (;;) {
		clearNHW(NHW_WINID_MESSAGE);
		l = strlen(msg);
		GetTextExtentExPoint(bkbuf.hdcMem, msg, l, nhwi[NHW_WINID_MESSAGE].winw - morewidth, &m, pw, &sz);
		if (m < l) {
		    /* exceeded */
#ifdef DO_WORDBREAK
		    if (m) {
			for (i=m-1; i; i--)
			    if (msg[i] == ' ' &&
				pw[i] != pw[i-1]) {
				m = i;
				break;
			    }
		    }
#endif /*DO_WORDBREAK*/
		    strncpy(buf, msg, m);
		    wsprintf(&buf[m], morestr);
		    l = strlen(buf);
		    TextOut(bkbuf.hdcMem, nhwi[NHW_WINID_MESSAGE].winx, nhwi[NHW_WINID_MESSAGE].winy, buf, l);
		    updateNHW(NHW_WINID_MESSAGE);
		    waitmorekey(0);
		    clearNHW(NHW_WINID_MESSAGE);
		    drawenvNHW(NHW_WINID_MESSAGE);/*may be changed in update process*/
		    while (msg[m] == ' ') m++;
		    msg = &msg[m];
		} else {
		    /* fit */
		    TextOut(bkbuf.hdcMem, nhwi[NHW_WINID_MESSAGE].winx, nhwi[NHW_WINID_MESSAGE].winy, msg, l);
		    updateNHW(NHW_WINID_MESSAGE);
		    nhwi[NHW_WINID_MESSAGE].cx = sz.cx;
		    break;
		}
	    }
	    strcpy(toplines, msg);
	    store_prevmsg(toplines, 1);
	}
}
void nhw_dontpack_msg(void) {
	toplines[0] = 0;
}

/*------------------------------------------------
	get line
  ------------------------------------------------*/
extern char editBoxText[];
extern HWND editBox;
int getlin_core(int nhwid, char *buf, int bufsiz) {
	int ctrl_p;
	openEditBox(bkbuf.hWnd,
		    nhwi[nhwid].cx + nhwi[nhwid].winx,
		    nhwi[nhwid].cy + nhwi[nhwid].winy,
		    nhwi[nhwid].winw - nhwi[nhwid].cx,
		    nhwi[nhwid].fh, nhwi[nhwid].font);
	ctrl_p = waitEditBoxClosed();
	strncpy(buf, editBoxText, bufsiz);
	buf[bufsiz-1] = 0;
	nhwi[nhwid].cx = 0;
	nhwi[nhwid].cy += nhwi[nhwid].fh;
	return ctrl_p;
}

char getyn_core(int nhwid, const char *choices, char defc, int *cnt) {
	int i;
	int ctrl_p;
	char c, r, *p;
	openEditBoxYN(bkbuf.hWnd,
		      nhwi[nhwid].cx + nhwi[nhwid].winx,
		      nhwi[nhwid].cy + nhwi[nhwid].winy,
		      nhwi[nhwid].winw - nhwi[nhwid].cx,
		      nhwi[nhwid].fh, nhwi[nhwid].font,
		      choices, defc, &r);
	ctrl_p = waitEditBoxClosed();
	p = editBoxText;
	if (*p >= '0' && *p <= '9') {
	    i = 0;
	    while (*p >= '0' && *p <= '9')
		i = i*10 + (int)((*p++) - '0');
	} else {
	    i = -1;
	}
	*cnt = i;
	nhwi[nhwid].cx = 0;
	nhwi[nhwid].cy += nhwi[nhwid].fh;

	if (ctrl_p) return C('p');
	return r;
}
int getext_core(int nhwid, char *buf, int bufsiz, char * (*callback)(char *)) {
	int ctrl_p;
	openEditBoxExt(bkbuf.hWnd,
		       nhwi[nhwid].cx + nhwi[nhwid].winx,
		       nhwi[nhwid].cy + nhwi[nhwid].winy,
		       nhwi[nhwid].winw - nhwi[nhwid].cx,
		       nhwi[nhwid].fh, nhwi[nhwid].font,
		       callback);
	ctrl_p = waitEditBoxClosed();
	strncpy(buf, editBoxText, bufsiz);
	buf[bufsiz-1] = 0;
	nhwi[nhwid].cx = 0;
	nhwi[nhwid].cy += nhwi[nhwid].fh;
	return ctrl_p;
}

/*------------------------------------------------
	overlapping menu
  ------------------------------------------------*/
void nhw_show_menu(int width) {
	int lastwinh;
	lastwinh = subwinh;
	if (width > nhwi[NHW_WINID_MENU].winw) width = nhwi[NHW_WINID_MENU].winw;
	if (width == 0) {
	    subwinx = 0/*nhwi[NHW_WINID_TEXT].winx*/;
	    subwiny = 0/*nhwi[NHW_WINID_TEXT].winy*/;
	    subwinw = nhwi[NHW_WINID_TEXT].winw + WSPACING*2;
	    subwinh = nhwi[NHW_WINID_TEXT].winh + WSPACING*2;
	    showmode = 2;
	} else {
	    subwinx = 0/*nhwi[NHW_WINID_MENU].winx*/ + nhwi[NHW_WINID_MENU].winw - width - MENUSPACING*2;
	    subwiny = 0/*nhwi[NHW_WINID_MENU].winy*/;
	    subwinw = width + MENUSPACING*2;
	    subwinh = nhwi[NHW_WINID_MENU].cy + WSPACING*2;
	    showmode = 3;
	}
	if (lastwinh > subwinh) {
	    updateNHW(NHW_WINID_MENU);
	} else { /* avoid flicker */
	    HDC hdc;
	    hdc = GetDC(bkbufsub.hWnd);
	    BitBlt(hdc, subwinx, subwiny, subwinw, subwinh,
		   bkbufsub.hdcMem, 0, 0, SRCCOPY);
	    ReleaseDC(bkbufsub.hWnd, hdc);
	}
}
void nhw_hide_menu(void) {
	showmode = 1;
	subwinx = subwiny = subwinw = subwinh = 0;
	updateNHW(NHW_WINID_BASE);
}

int nhw_getmaxrows(int nhwid) {
	return nhwi[nhwid].rows;
}
int nhw_getmpwidth(int nhwid) {
	return nhwi[nhwid].fw * 3;
}
void nhw_print_menuprompt(int nhwid, char acc, char sel) {
	HDC hdc;
	SIZE sz;
	int w, aw, sw;
	char buf[4];

	hdc = getHDCNHW(nhwid);
	drawenvNHW(nhwid);

	w = nhwi[nhwid].fw/*nhw_getmpwidth(nhwid)*/;
	if (!acc || !sel) {
	    nhwi[nhwid].cx += w*3;
	    return;
	}

	buf[0] = acc;
	buf[1] = 0;
	buf[2] = sel;
	buf[3] = 0;

	GetTextExtentPoint32(hdc, &buf[0], 1, &sz);
	aw = sz.cx;
	GetTextExtentPoint32(hdc, &buf[2], 1, &sz);
	sw = sz.cx;
	TextOut(hdc,
		nhwi[nhwid].winx + nhwi[nhwid].cx + (w - aw)/2,
		nhwi[nhwid].winy + nhwi[nhwid].cy,
		&buf[0], 1);
	TextOut(hdc,
		nhwi[nhwid].winx + nhwi[nhwid].cx + w + (w*2 - sw)/2,
		nhwi[nhwid].winy + nhwi[nhwid].cy,
		&buf[2], 1);
	nhwi[nhwid].cx += w*3;
	if (nhwid == NHW_WINID_MENU || nhwid == NHW_WINID_TEXT) return;
	updateNHW(nhwid);
}

/*------------------------------------------------
	map cursor
  ------------------------------------------------*/
void nhw_drawcursor(int x, int y, int d) {
	HDC hdc;
	int xx, yy;
	RECT r;
	xx = nhwi[NHW_WINID_MAP].winx + x * nhwi[NHW_WINID_MAP].fw;
	yy = nhwi[NHW_WINID_MAP].winy + y * nhwi[NHW_WINID_MAP].fh;
	hdc = GetDC(bkbuf.hWnd);
	BitBlt(hdc, xx, yy, nhwi[NHW_WINID_MAP].fw, nhwi[NHW_WINID_MAP].fh,
	       bkbuf.hdcMem, xx, yy, SRCCOPY);
	if (d) {
	    r.left   = xx;
	    r.top    = yy + nhwi[NHW_WINID_MAP].fh * 5 / 6;
	    r.right  = xx + nhwi[NHW_WINID_MAP].fw;
	    r.bottom = yy + nhwi[NHW_WINID_MAP].fh;
	    InvertRect(hdc, &r);
	}
	ReleaseDC(bkbuf.hWnd, hdc);
}

/*------------------------------------------------
	fonts
  ------------------------------------------------*/
void nhw_setfont(int nhwid, char *fn, int siz) {
	deffonts[nhwid].fname = fn;
	deffonts[nhwid].fsize = siz;
	if (nhwid == NHW_WINID_MESSAGE)
	    nhw_setfont(NHW_WINID_BASE, fn, siz);
}
void nhw_setmapfont(int w, int h) {
	mapfontw = w;
	mapfonth = h;
	deffonts[NHW_WINID_MAP].fsize = 0;
}

/*------------------------------------------------
	kludge for topten list formatting
  ------------------------------------------------*/
void nhw_toptenfont(void) {
	HFONT ftmp;
	ftmp = nhwi[NHW_WINID_MENU].font;
	nhwi[NHW_WINID_MENU] = nhwi[NHW_WINID_BASE];
	nhwi[NHW_WINID_BASE].font = ftmp;
}
void nhw_show_topten(void) {
	int x, y, w, h;
	w = bkbuf.w;
	h = nhwi[NHW_WINID_MENU].cy - nhwi[NHW_WINID_MENU].winy;
	x = nhwi[NHW_WINID_MENU].winx - nhwi[NHW_WINID_BASE].winx;
	y = nhwi[NHW_WINID_MENU].winy;
	BitBlt(bkbuf.hdcMem, MENUSPACING, nhwi[NHW_WINID_BASE].cy, w, h,
	       bkbufsub.hdcMem, 0, y, SRCCOPY);
	nhwi[NHW_WINID_BASE].cy = nhwi[NHW_WINID_BASE].cy + h;
	updateNHW(NHW_WINID_BASE);
}

/*------------------------------------------------
	initialize
  ------------------------------------------------*/
void init_nhw(HWND hwnd) {
	int i;
	int w, h;
	int ww, wh;
	HDC hdc;
	SIZE sz;

	currcolor = 7; /* CLR_GRAY */
	showmode = 1; /* base only */

	hdc = GetDC(hwnd);

	/* prepare fonts for each NHW */
	for (i=0; i<NHW_WINID_MAX; i++) {
	    nhwi[i].font = getfont(deffonts[i].fname, deffonts[i].fsize);
	    SelectObject(hdc, nhwi[i].font);
	    GetTextExtentPoint32(hdc, "M", 1, &sz);
	    nhwi[i].fw = sz.cx;
	    nhwi[i].fh = sz.cy;
	    /* check proportional font */
	    GetTextExtentPoint32(hdc, "i", 1, &sz);
	    nhwi[i].isprop = (nhwi[i].fw != sz.cx);
	    /* set morewidth */
	    if (i == NHW_WINID_MESSAGE) {
		GetTextExtentPoint32(hdc, morestr, strlen(morestr), &sz);
		morewidth = sz.cx;
	    }
	}

	/* calc window size */
	w = nhwi[NHW_WINID_MAP].fw * 80/*COLS*/;
	h = nhwi[NHW_WINID_MAP].fh * 21/*ROWS*/ + nhwi[NHW_WINID_MESSAGE].fh + nhwi[NHW_WINID_STATUS].fh*2;
	if (h < nhwi[NHW_WINID_MENU].fh * 25)
	    h = nhwi[NHW_WINID_MENU].fh * 25;
	ww = w + WSPACING*2;
	wh = h + WSPACING*2;

	nhwi[NHW_WINID_MAP].winw = w;
	nhwi[NHW_WINID_MAP].winh = nhwi[NHW_WINID_MAP].fh * 21/*ROWS*/;
	nhwi[NHW_WINID_MAP].winx = WSPACING;
	nhwi[NHW_WINID_MAP].winy = WSPACING + nhwi[NHW_WINID_MESSAGE].fh;

	nhwi[NHW_WINID_MESSAGE].winw = w;
	nhwi[NHW_WINID_MESSAGE].winh = nhwi[NHW_WINID_MESSAGE].fh;
	nhwi[NHW_WINID_MESSAGE].winx = WSPACING;
	nhwi[NHW_WINID_MESSAGE].winy = WSPACING;

	nhwi[NHW_WINID_STATUS].winw = w;
	nhwi[NHW_WINID_STATUS].winh = nhwi[NHW_WINID_STATUS].fh*2;
	nhwi[NHW_WINID_STATUS].winx = WSPACING;
	nhwi[NHW_WINID_STATUS].winy = WSPACING + nhwi[NHW_WINID_MESSAGE].winh + nhwi[NHW_WINID_MAP].winh;

	nhwi[NHW_WINID_BASE].winw = w;
	nhwi[NHW_WINID_BASE].winh = h;
	nhwi[NHW_WINID_BASE].winx = WSPACING;
	nhwi[NHW_WINID_BASE].winy = WSPACING;
	nhwi[NHW_WINID_BASE].rows = h / nhwi[NHW_WINID_BASE].fh;

	nhwi[NHW_WINID_MENU].winw = w;
	nhwi[NHW_WINID_MENU].winh = h;
	nhwi[NHW_WINID_MENU].winx = MENUSPACING;
	nhwi[NHW_WINID_MENU].winy = WSPACING;
	nhwi[NHW_WINID_MENU].rows = h / nhwi[NHW_WINID_MENU].fh;

	nhwi[NHW_WINID_TEXT].winw = w;
	nhwi[NHW_WINID_TEXT].winh = h;
	nhwi[NHW_WINID_TEXT].winx = WSPACING;
	nhwi[NHW_WINID_TEXT].winy = WSPACING;
	nhwi[NHW_WINID_TEXT].rows = h / nhwi[NHW_WINID_TEXT].fh;

	/* create back buffer */
	createBackBuffer(hwnd, ww, wh, &bkbuf);
	createBackBuffer(hwnd, ww, wh, &bkbufsub);

	clearNHW(NHW_WINID_BASE);

	ReleaseDC(hwnd,hdc);

	inited = 1;
}

void tini_nhw(void) {
	int i;
	destroyBackBuffer(&bkbuf);
	destroyBackBuffer(&bkbufsub);
	for (i=0; i<NHW_WINID_MAX; i++)
	    DeleteObject(nhwi[i].font);
}
