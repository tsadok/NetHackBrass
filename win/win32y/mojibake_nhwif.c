#include "nhw.h"

#include "dlb.h"
#include "patchlevel.h"
#include "func_tab.h"

/* ================================================
	External functions
   ================================================ */
extern char keybufget(void);
extern char keybufpeek(void);
extern int keybufempty(void);
extern void openWindow(void);
extern void closeWindow(void);
extern int nhrunning;

/* ================================================
	Window information
   ================================================ */
#define MAXWINNUM 20
static int numofwin;
NHWin winarray[MAXWINNUM];

/* winid to array index */
int winid2index(winid win) {
	return win - NHW_WINID_USER;
}
int index2winid(int index) {
	return NHW_WINID_USER + index;
}

/* ================================================
	Message history
   ================================================ */
#define MAXPREVMSG 20
static char prevmsg[MAXPREVMSG][TBUFSZ];
static int prevmsg_wp;

/* ================================================
	Pre-defined formatting
   ================================================ */
char *remove_spaces(char *buf) {
	int h, t, i, l0, l1;
	for (h=0; isspace(buf[h]); h++);
	for (l0=0, t=h, i=h; buf[i]; i++) {
	    l1 = isspace(buf[i]);
	    if (!l0 && l1) t = i;
	    l0 = l1;
	}
	if (l0) buf[t] = 0;
	return &buf[h];
}

extern NHWMenuFormatInfo menu_format_info[];
void format_menu(int idx) {
	int i, w, pos;
	char buf[256];
	char *p, *mnstr;
	anything any;
	menu_item *mi;
	NHWMenuItem *mnup;
	NHWMenuFormat *mfp;
	NHWMenuFormatInfo *mfip;

	winarray[idx].format = 0;
	for (i=0; i<16; i++) winarray[idx].tabpos[i] = 0;

	if (!nhw_is_proportional(NHW_WINID_MENU)) return;

	for (mfip = menu_format_info; mfip->menucheckfunc != NULL; mfip++) {
	    if (mfip->menucheckfunc(winarray[idx].content)) {
		for (mnup = NHWFirstMenuItem(winarray[idx].content);
		     mnup != NULL;
		     mnup = NHWNextMenuItem(winarray[idx].content)) {
		    mfp = mfip->itemcheckfunc(mnup);
		    mnup->fmt = mfp;
		    mnstr = mnup->str;
		    if (mfp != NULL) do {
			if (mfp->ostart < 0) { /* column for "a - " */
			    winarray[idx].tabpos[mfp->column] = nhw_getmpwidth(NHW_WINID_MENU);
			    if (!mnup->id.a_void) mnstr += 4; /* skip "    " */
			    continue;
			}
			if (mfp->oend) {
			    strncpy(buf, &(mnstr[mfp->ostart]), mfp->oend - mfp->ostart);
			    buf[mfp->oend - mfp->ostart] = 0;
			} else {
			    strcpy(buf, &(mnstr[mfp->ostart]));
			}
			p = remove_spaces(buf);
			w = nhw_getstrwidth(NHW_WINID_MENU, p);
			if (!(mfp->flags & MF_NOSPACING)) w += nhw_getMwidth(NHW_WINID_MENU);
			if (winarray[idx].tabpos[mfp->column] < w) winarray[idx].tabpos[mfp->column] = w;
		    } while (mfp->oend && mfp++);
		}
		/* width to pos */
		for (i=0, pos=0; i<16; i++) {
		    w = winarray[idx].tabpos[i];
		    winarray[idx].tabpos[i] = pos;
		    pos += w;
		}
		if (winarray[idx].width < pos) winarray[idx].width = pos;
		winarray[idx].format = 1;
		return;
	    }
	}
}

/* ================================================
	Color setting interface
   ================================================ */
void
term_start_color(int color)
{
	nhw_setcolor(color);
}

void
term_end_color(void)
{
	nhw_setcolor(-1);   /* default */
}

/* dummy */
void term_start_attr(int attrib) {}
void term_end_attr(int attrib) {}

/* ================================================
	Interface functions
   ================================================ */
int format_topten;
winid widtopten;
int raw_topten(char *str, int bold) {
	if (format_topten) {
	    char buf[80];
	    int i;
	    strncpy(buf, str, 80);
	    for (i=0; i<80; i++) if (buf[i] == 0) buf[i] = ' ';
	    buf[79] = 0;
	    win32y_putstr(widtopten, bold ? ATR_BOLD : ATR_NONE, buf);
	    return 1;
	}
	if (nhw_is_proportional(NHW_WINID_BASE) &&
	    !strncmp(str, " No  Points", 11)) {
	    format_topten = 1;
	    nhw_toptenfont();
	    widtopten = win32y_create_nhwindow(NHW_MENU);
	    win32y_putstr(widtopten, bold ? ATR_BOLD : ATR_NONE, str);
	    return 1;
	}
	return 0;
}

/* raw_print */
void win32y_raw_print(char *str) {
	if (raw_topten(str, 0)) return;
	nhw_raw_print(str);
}

/* raw_print_bold */
void win32y_raw_print_bold(char *str) {
	if (raw_topten(str, 1)) return;
	nhw_raw_print_bold(str);
}

/* curs */
int curx, cury, curstt;
void CALLBACK blinkcursor(HWND hwnd, UINT msg, UINT idtimer, DWORD dwtime) {
	if (curstt) {
	    curstt ^= 1;
	    nhw_drawcursor(curx, cury, curstt & 1);
	}
}
void show_mapcursor(void) {
	if (!nhrunning) return;
	if (curstt & 1) return;
	nhw_drawcursor(curx, cury, 1);
	curstt = 3;
	SetTimer(nhw_getHWND(), 1, 600, (TIMERPROC)blinkcursor);
}
void hide_mapcursor(void) {
	if (!curstt) return;
	KillTimer(nhw_getHWND(), 1);
	nhw_drawcursor(curx, cury, 0);
	curstt = 0;
}

void win32y_curs(winid win, int x, int y) {
	int r;
	switch (win) {
	    case NHW_WINID_MAP:
		hide_mapcursor();
		curx = x;
		cury = y;
		show_mapcursor();
		nhw_curs(NHW_WINID_MAP, x, y);
		break;
	    default:
		nhw_curs(win, x, y);
		break;
	}
}

/* get_nh_event */
void win32y_get_nh_event(void) {
	DWORD t;
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
}

/* nhgetch */
int win32y_nhgetch(void) {
	MSG msg;
	while (keybufempty() && GetMessage(&msg, NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	return ((int)keybufget() & 0x00ff);
}

/* add_menu */
void add_menu_sub(winid win, int glyph, const anything *identifier,
		  char accelerator, char groupacc,
		  int attr, char *str, boolean preselected, int totop) {
	int i, j, a;
	int indent = 0;
	NHWMenuItem mnu;

	i = winid2index(win);
	if (winarray[i].type != NHW_MENU &&
	    winarray[i].type != NHW_TEXT) return;
	mnu.id = *identifier;
	mnu.attr = attr;
	mnu.accelerator = accelerator;
	mnu.groupacc = groupacc;
	mnu.selected = preselected;
	mnu.count = 0;

	j = nhw_getstrwidth((winarray[i].type == NHW_MENU) ? NHW_WINID_MENU : NHW_WINID_TEXT, str);
	if (identifier->a_void) {
	    winarray[i].hasacc = 1;
	    j += nhw_getmpwidth((winarray[i].type == NHW_MENU) ? NHW_WINID_MENU : NHW_WINID_TEXT);
	}
	if (winarray[i].width < j) winarray[i].width = j;
	winarray[i].num++;

	winarray[i].content = NHWAddMenuItem(winarray[i].content, &mnu, str, totop);
}
void win32y_add_menu(winid win, int glyph, const anything *identifier,
		      char accelerator, char groupacc,
		      int attr, char *str, boolean preselected) {
	add_menu_sub(win, glyph, identifier, accelerator, groupacc, attr, str, preselected, 0);
}

/* store previous message */
static int supress_store;
void store_prevmsg(char *str, int append) {
	char *p;
	if (supress_store) return;
	prevmsg_wp += append;
	if (prevmsg_wp >= MAXPREVMSG) prevmsg_wp = 0;
	p = &prevmsg[prevmsg_wp][0];
	Strcpy(p, str);
}

/* putstr */
void win32y_putstr(winid win, int attr, const char *str) {
	if (win < NHW_WINID_USER) {
	  switch (win) {
	    case NHW_WINID_BASE:
		nhw_print(NHW_WINID_BASE, str, attr);
		break;
	    case NHW_WINID_MESSAGE:
		if (!strncmp(str, "You die...", 10)) nhw_do_more(0); /* force --more-- */
		nhw_print_msg(str);
		break;
	    case NHW_WINID_STATUS:
		nhw_print(NHW_WINID_STATUS, str, attr);
		break;
	    default:
		break;
	  }
	} else {
	    anything dummy;
	    dummy.a_void = 0;
	    win32y_add_menu(win, 0, &dummy, 0, 0, attr, str, 0);
	}
}

/* nh_poskey */
int win32y_nh_poskey(int *x, int *y, int *mod) {
    return win32y_nhgetch();
}

/* High-level routines */

/* print_glyph */
void win32y_print_glyph(winid win, int x, int y, int glyph) {
	int chr, fc, bc, sp;
	bc = 16;
	if (win == NHW_WINID_MAP) {
	    mapglyph(glyph, &chr, &fc, &sp, x, y);
	    if ((iflags.hilite_pet && (sp & MG_PET)) ||
		(sp & MG_DETECT)) {
		bc = CLR_GRAY;
		if (fc == CLR_GRAY || fc == CLR_WHITE) fc = 16;
	    } else if (sp & MG_RIDING) {
		bc = CLR_BROWN;
		if (fc == CLR_BROWN) fc = 16;
	    }
	    nhw_printglyph(x, y, chr, fc, bc);
	}
}

/* clear_nhwindow */
void win32y_clear_nhwindow(winid win) {
	int i,j;
	if (win < NHW_WINID_USER) {
	    switch (win) {
		case NHW_WINID_BASE:
		    hide_mapcursor();
		    nhw_clear(NHW_WINID_BASE);
		    break;
		case NHW_WINID_MESSAGE:
		    nhw_clear(NHW_WINID_MESSAGE);
		    break;
		case NHW_WINID_STATUS:
		    nhw_clear(NHW_WINID_STATUS);
		    break;
		case NHW_WINID_MAP:
		    hide_mapcursor();
		    nhw_clear(NHW_WINID_MAP);
		    break;
		default:
		    break;
	    }
	}
}

/* yn_function */
char win32y_yn_function(const char *ques, const char *choices, char dflt) {
	char buf[BUFSZ*2];
	const char *p;
	char c, *p1;
	int cnt;
	char ink;
	if (!choices) {
	    Sprintf(buf, "%s ", ques);
	} else {
	    Sprintf(buf, "%s [", ques);
	    p = choices;
	    p1 = eos(buf);
	    do {
		c = *p1++ = *p++;
		if (c == 0x1b) c = 0;
	    } while (c);
	    if (dflt) Sprintf(--p1, "] (%c) ", dflt);
	    else Sprintf(--p1, "] ");
	}
	nhw_do_more(0); /* force --more-- */
	win32y_putstr(NHW_WINID_MESSAGE, ATR_NONE, buf);
	while ((ink = getyn_core(NHW_WINID_MESSAGE, choices, dflt, &cnt)) == C('p')) {
	    win32y_doprev_message();
	    win32y_consume_prevmsgkey();
	    supress_store = 1;
	    win32y_putstr(NHW_WINID_MESSAGE, ATR_NONE, buf);
	    supress_store = 0;
	}
	if (cnt > 0) ink = '#';
	yn_number = cnt;
	nhw_dontpack_msg(); /* do not pack next pline */
	return ink;
}

/* getlin */
void win32y_getlin(const char *ques, char *input) {
	char buf[BUFSZ+4];
	nhw_do_more(0); /* force --more-- */
	Sprintf(buf, "%s ", ques);
	win32y_putstr(NHW_WINID_MESSAGE, ATR_NONE, buf);
	while (getlin_core(NHW_WINID_MESSAGE, input, BUFSZ) == 1) {
	    win32y_doprev_message();
	    win32y_consume_prevmsgkey();
	    supress_store = 1;
	    win32y_putstr(NHW_WINID_MESSAGE, ATR_NONE, buf);
	    supress_store = 0;
	}
	if (!*input) {
	    input[0] = 0x1b;
	    input[1] = 0;
	}
	win32y_clear_nhwindow(NHW_WINID_MESSAGE);
}

/* get_ext_cmd */
static int kouho;
char *win32y_get_ext_cmd_callback(char *inp) {
	int i, l;
	kouho = -1;
	l = strlen(inp);
	if (l) for (i=0; extcmdlist[i].ef_txt != (char *)0; i++) {
	    if (!strncmpi(inp, extcmdlist[i].ef_txt, l)) {
		kouho = i;
		return extcmdlist[i].ef_txt;
	    }
	}
	return NULL;
}
int win32y_get_ext_cmd(void) {
	char buf[BUFSZ];
	static int lastcmd;
#ifdef REDO
	if (in_doagain) return lastcmd;
#endif
	win32y_clear_nhwindow(NHW_WINID_MESSAGE);
	nhw_print(NHW_WINID_MESSAGE,"#", 0);
	while (getext_core(NHW_WINID_MESSAGE, buf, BUFSZ, win32y_get_ext_cmd_callback) == 1) {
	    win32y_doprev_message();
	    win32y_consume_prevmsgkey();
	    win32y_clear_nhwindow(NHW_WINID_MESSAGE);
	    nhw_print(NHW_WINID_MESSAGE,"#", 0);
	}
	if (strlen(buf)) {
	    (void) mungspaces(buf);
	    if (kouho == -1) pline("%s: unknown extended command.", buf);
	} else win32y_clear_nhwindow(NHW_WINID_MESSAGE);
	lastcmd = kouho;
	return kouho;
}

/* doprev_message */
int wait_ctrl_p(void) {
	MSG msg;
	while (keybufempty() && GetMessage(&msg, NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	if (keybufpeek() != C('p')) return 0;
	keybufget();
	return 1;
}
static int isprevmsgkey;
int win32y_doprev_message(void) {
	winid prevmsg_win;
	int i, j, cnt;
	int prevmsg_rp;

	cnt = 0;
	isprevmsgkey = 0;
	prevmsg_rp = prevmsg_wp;
	do {
	    win32y_clear_nhwindow(NHW_WINID_MESSAGE);
	    if (iflags.prevmsg_window == 'f' || /* full */
		iflags.prevmsg_window == 'r' || /* reversed */
		(iflags.prevmsg_window == 'c' && cnt >= 2)) { /* combination */
		prevmsg_win = win32y_create_nhwindow(NHW_MENU);
		win32y_putstr(prevmsg_win, 0, "Message History");
		win32y_putstr(prevmsg_win, 0, "");
		if (iflags.prevmsg_window == 'r') {
		    for (i=0, j=prevmsg_wp; i<MAXPREVMSG; i++, j--) {
			if (j < 0) j = MAXPREVMSG - 1;
			if (!prevmsg[j][0]) continue;
			win32y_putstr(prevmsg_win, 0, &prevmsg[j][0]);
		    }
		} else {
		    for (i=0, j=prevmsg_wp+1; i<MAXPREVMSG; i++, j++) {
			if (j >= MAXPREVMSG) j = 0;
			if (!prevmsg[j][0]) continue;
			win32y_putstr(prevmsg_win, 0, &prevmsg[j][0]);
		    }
		}
		win32y_display_nhwindow(prevmsg_win, TRUE);
		win32y_destroy_nhwindow(prevmsg_win);
		return 0;
	    }
	    supress_store = 1;
	    while (!prevmsg[prevmsg_rp][0]) {
		prevmsg_rp--;
		if (prevmsg_rp < 0) prevmsg_rp = MAXPREVMSG - 1;
	    }
	    win32y_putstr(NHW_WINID_MESSAGE, 0, &prevmsg[prevmsg_rp][0]);
	    prevmsg_rp--;
	    if (prevmsg_rp < 0) prevmsg_rp = MAXPREVMSG - 1;
	    nhw_dontpack_msg();
	    supress_store = 0;
	    cnt++;
	} while (wait_ctrl_p());
	isprevmsgkey = 1;
	return 0;
}
void win32y_consume_prevmsgkey(void) {
	if (isprevmsgkey) win32y_nhgetch();
	isprevmsgkey = 0;
}

/* update_positionbar */
void win32y_update_positionbar(char *features) {
}

/* Window Utility Routines */

/* init_nhwindows */
void win32y_init_nhwindows(int *argcp, char **argv) {
	int i;
	struct {
	    int wid;
	    char **fname;
	    int *fsize;
	    char **fcolor;
	    char **bcolor;
	} finitinfo[5] = {
	    { NHW_WINID_MAP,	 &iflags.wc_font_map,	  &iflags.wc_fontsiz_map     },
	    { NHW_WINID_MESSAGE, &iflags.wc_font_message, &iflags.wc_fontsiz_message },
	    { NHW_WINID_STATUS,	 &iflags.wc_font_status,  &iflags.wc_fontsiz_status  },
	    { NHW_WINID_MENU,	 &iflags.wc_font_menu,	  &iflags.wc_fontsiz_menu    },
	    { NHW_WINID_TEXT,	 &iflags.wc_font_text,	  &iflags.wc_fontsiz_text    }
	};

	/* prepare font information before opening window */
	for (i=0; i<5; i++) {
	    if (*(finitinfo[i].fname))
		nhw_setfont(finitinfo[i].wid, *(finitinfo[i].fname),
			    (*(finitinfo[i].fsize)) ? -(*(finitinfo[i].fsize)) : 14);
	}
	switch (iflags.wc_map_mode) {
	    case MAP_MODE_ASCII4x6:
		nhw_setmapfont(4, 6);
		break;
	    case MAP_MODE_ASCII6x8:
		nhw_setmapfont(6, 8);
		break;
	    case MAP_MODE_ASCII8x8:
		nhw_setmapfont(8, 8);
		break;
	    case MAP_MODE_ASCII16x8:
		nhw_setmapfont(16, 8);
		break;
	    case MAP_MODE_ASCII7x12:
		nhw_setmapfont(7, 12);
		break;
	    case MAP_MODE_ASCII8x12:
		nhw_setmapfont(8, 12);
		break;
	    case MAP_MODE_ASCII16x12:
		nhw_setmapfont(16, 12);
		break;
	    case MAP_MODE_ASCII12x16:
		nhw_setmapfont(12, 16);
		break;
	    case MAP_MODE_ASCII10x18:
		nhw_setmapfont(10, 18);
		break;
	    default:
		break;
	}

	/* open window, create back buffer, prepare fonts */
	openWindow();

	for (i=0; i<MAXWINNUM; i++) {
		winarray[i].type    = 0; /* not used */
		winarray[i].num     = 0;
		winarray[i].content = 0;
	}
	iflags.window_inited = 1;
	win32y_raw_print(COPYRIGHT_BANNER_A);
	win32y_raw_print(COPYRIGHT_BANNER_B);
	win32y_raw_print(COPYRIGHT_BANNER_C);
	win32y_raw_print("");
}

/* exit_nhwindows */
void win32y_exit_nhwindows(const char *str) {
	int i;
	for (i=0; i<MAXWINNUM; i++) {
	    if (winarray[i].type != 0) {
		NHWDisposeMenu(winarray[i].content);
		winarray[i].content = 0;
		winarray[i].type = 0;
	    }
	}
	win32y_clear_nhwindow(NHW_WINID_BASE);
	if (str && *str) win32y_raw_print(str);
	iflags.window_inited = 0;
	nhrunning = 0;
}

void win32y_exit(void) {
	MSG msg;
	if (format_topten) {
	    win32y_display_nhwindow(widtopten, FALSE);
	    nhw_show_topten();
	    format_topten = 0;
	    nhw_toptenfont();
	    win32y_destroy_nhwindow(widtopten);
	}
	win32y_raw_print("");
	win32y_putstr(NHW_WINID_BASE, ATR_INVERSE, "Hit any key to exit.");
	win32y_nhgetch();
	tini_nhw();
	closeWindow();
	/* process remaining messages */
	while (GetMessage(&msg,NULL,0,0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
}

/* create_nhwindow */
winid win32y_create_nhwindow(int type) {
	int i;
	if (type == NHW_MENU || type == NHW_TEXT) {
	    /* search empty slot */
	    for (i=0; i<MAXWINNUM; i++) {
		if (winarray[i].type) continue;
		winarray[i].content = NHWCreateMenu();
		winarray[i].type    = type;
		winarray[i].num     = 0;
		winarray[i].width   = 0;
		winarray[i].hasacc  = 0;
		winarray[i].format  = -1;
		return index2winid(i);
	    }
	} else switch (type) {
	    case NHW_MESSAGE:
		return NHW_WINID_MESSAGE;
	    case NHW_STATUS:
		return NHW_WINID_STATUS;
	    case NHW_MAP:
		return NHW_WINID_MAP;
	    default:
		break;
	}
	return -1;
}

/* select_menu */
int win32y_select_menu(winid win, int how, menu_item **selected) {
	int i, j, k, l;
	int acc, cnt, attr, ink;
	int maxrows;
	int index;
	char buf[32];
	char *pagestr;
	winid wid;
	anything any;
	menu_item *mi;
	NHWMenuItem *mnup;
	NHWin *nw;

	hide_mapcursor();
	i = winid2index(win);
	if (selected) *selected = 0;
	if (i < 0 || i >= MAXWINNUM) return 0;
	nw = &winarray[i];

	if (nw->format == -1) {
	    format_menu(i);
	}

	if (nw->type != NHW_MENU && nw->type != NHW_TEXT) return 0;
	wid = (nw->type == NHW_MENU) ? NHW_WINID_MENU : NHW_WINID_TEXT;
	maxrows = nhw_getmaxrows(wid) - 1;

	nhw_do_more(0); /* force --more-- */
	index = 0;
	cnt = 0;
	while (1) {
	    nhw_clear(wid);
	    acc = 0;
	    Sprintf(buf, "(%d of %d)", (index / maxrows) + 1, (nw->num / maxrows) + 1);
	    pagestr = buf;
	    mnup = NHWFirstMenuItem(nw->content);
	    for (j=0; j<index; j++) mnup = NHWNextMenuItem(nw->content);
	    for (j=0; j<maxrows; j++) {
		if (mnup->id.a_void && !mnup->accelerator) {
		    mnup->accelerator = (acc < 26) ? acc+'a' : acc-26+'A';
		    acc++;
		}
		win32y_curs(wid, 0, j);
		if (!nw->format || mnup->fmt == NULL) {
		    /* put 'a - ' prompt */
		    if (nw->hasacc) {
			if (mnup->id.a_void) {
			    nhw_print_menuprompt(wid, mnup->accelerator,
						 !(mnup->selected) ? '-' : !(mnup->count) ? '+' : '#');
			    nhw_print(wid, mnup->str, mnup->attr);
			} else {
			    if (!strncmp(mnup->str, "    ", 4)) {
				nhw_print_menuprompt(wid, 0, 0);
				nhw_print(wid, mnup->str+4, mnup->attr);
			    } else
				nhw_print(wid, mnup->str, mnup->attr);
			}
		    } else {
			nhw_print(wid, mnup->str, mnup->attr);
		    }
		} else {
		    NHWMenuFormat *mfp;
		    char buf2[256];
		    char *mnstr, *p;
		    mfp = mnup->fmt;
		    mnstr = mnup->str;
		    do {
			nhw_setpos(wid, nw->tabpos[mfp->column], -1);
			if (mfp->ostart < 0) {
			    if (mnup->id.a_void) {
				nhw_print_menuprompt(wid, mnup->accelerator,
				     !(mnup->selected) ? '-' : !(mnup->count) ? '+' : '#');
			    } else {
				nhw_print_menuprompt(wid, 0, 0);
				mnstr += 4;
			    }
			    continue;
			}
			if (mfp->oend) {
			    strncpy(buf2, &mnstr[mfp->ostart], mfp->oend - mfp->ostart);
			    buf2[mfp->oend - mfp->ostart] = 0;
			} else {
			    strcpy(buf2, &mnstr[mfp->ostart]);
			}
			p = remove_spaces(buf2);
			if (!p[0]) continue;
			if (mfp->flags & MF_ALIGNRIGHT) {
			    k = nw->tabpos[mfp->column+1] - nhw_getstrwidth(wid, p);
			    if (!(mfp->flags & MF_NOSPACING)) k -= nhw_getMwidth(wid);
			    nhw_setpos(wid, k, -1);
			} else if (mfp->flags & MF_ALIGNCENTER) {
			    k = nw->tabpos[mfp->column+1] - nw->tabpos[mfp->column] - nhw_getstrwidth(wid, p);
			    if (!(mfp->flags & MF_NOSPACING)) k -= nhw_getMwidth(wid);
			    nhw_setpos(wid, nw->tabpos[mfp->column] + k/2, -1);
			}
			nhw_print(wid, p, mnup->attr);
		    } while (mfp->oend && mfp++);
		}
		mnup = NHWNextMenuItem(nw->content);
		if (mnup == NULL) {
		    pagestr = "(end)";
		    j++;
		    break;
		}
	    }
	    if (wid == NHW_WINID_TEXT) j = maxrows;
	    win32y_curs(wid, 0, j++);
	    if (format_topten) return 0; /* dirty kludge... */
	    nhw_print(wid, pagestr, 0);
	    win32y_curs(wid, 0, j);
	    nhw_show_menu(wid == NHW_WINID_MENU ?
		nw->width + ((!nw->format && nw->hasacc) ? nhw_getmpwidth(wid) : 0)
		: 0);
menukeyin:  ink = win32y_nhgetch();
	    if (ink >= '0' && ink <= '9') {
		cnt = cnt*10+ink-'0';
	    } else {
	      ink = map_menu_cmd(ink);
	      switch (ink) {
		case MENU_FIRST_PAGE: /* FIRST_PAGE */
		    index = 0;
		    break;
		case MENU_LAST_PAGE: /* LAST_PAGE */
		    index = 0;
		    while (index + maxrows < nw->num)
			index += maxrows;
		    break;
		case ' ': /* NEXT_PAGE */
		    index += maxrows;
		    if (index >= nw->num) goto endselect;
		    break;
		case MENU_NEXT_PAGE: /* NEXT_PAGE */
		    if (index+maxrows >= nw->num) goto menukeyin;
		    index += maxrows;
		    break;
		case MENU_PREVIOUS_PAGE: /* PREVIOUS_PAGE */
		    if (index == 0) goto menukeyin;
		    index -= maxrows;
		    if (index < 0) index += nw->num;
		    break;
		case 0x1b: /* ESC */
		    if (selected) *selected = 0;
		    nhw_hide_menu();
		    win32y_clear_nhwindow(NHW_WINID_MESSAGE);
		    return -1;
		case 0x0d: /* Enter */
		    goto endselect;
		case MENU_SELECT_ALL: /* SELECT_ALL */
		    if (how != PICK_ANY) break;
		    mnup = NHWFirstMenuItem(nw->content);
		    while (mnup != NULL) {
			if (mnup->id.a_void) {
			    mnup->selected = 1;
			    mnup->count = 0;
			}
			mnup = NHWNextMenuItem(nw->content);
		    }
		    break;
		case MENU_UNSELECT_ALL: /* UNSELECT_ALL */
		    if (how != PICK_ANY) break;
		    mnup = NHWFirstMenuItem(nw->content);
		    while (mnup != NULL) {
			if (mnup->id.a_void) {
			    mnup->selected = 0;
			    mnup->count = 0;
			}
			mnup = NHWNextMenuItem(nw->content);
		    }
		    break;
		case MENU_INVERT_ALL: /* INVERT_ALL */
		    if (how != PICK_ANY) break;
		    mnup = NHWFirstMenuItem(nw->content);
		    while (mnup != NULL) {
			if (mnup->id.a_void) {
			    mnup->selected = !mnup->selected;
			    mnup->count = 0;
			}
			mnup = NHWNextMenuItem(nw->content);
		    }
		    break;
		case MENU_SELECT_PAGE: /* SELECT_PAGE */
		    if (how != PICK_ANY) break;
		    mnup = NHWFirstMenuItem(nw->content);
		    for (j=0; j<index; j++) mnup = NHWNextMenuItem(nw->content);
		    for (j=0; j<maxrows && mnup != NULL; j++) {
			if (mnup->id.a_void) {
			    mnup->selected = 1;
			    mnup->count = 0;
			}
			mnup = NHWNextMenuItem(nw->content);
		    }
		    break;
		case MENU_UNSELECT_PAGE: /* UNSELECT_PAGE */
		    if (how != PICK_ANY) break;
		    mnup = NHWFirstMenuItem(nw->content);
		    for (j=0; j<index; j++) mnup = NHWNextMenuItem(nw->content);
		    for (j=0; j<maxrows && mnup != NULL; j++) {
			if (mnup->id.a_void) {
			    mnup->selected = 0;
			    mnup->count = 0;
			}
			mnup = NHWNextMenuItem(nw->content);
		    }
		    break;
		case MENU_INVERT_PAGE: /* INVERT_PAGE */
		    if (how != PICK_ANY) break;
		    mnup = NHWFirstMenuItem(nw->content);
		    for (j=0; j<index; j++) mnup = NHWNextMenuItem(nw->content);
		    for (j=0; j<maxrows && mnup != NULL; j++) {
			if (mnup->id.a_void) {
			    mnup->selected = !mnup->selected;
			    mnup->count = 0;
			}
			mnup = NHWNextMenuItem(nw->content);
		    }
		    break;
		default:
		    l = 0;
		    mnup = NHWFirstMenuItem(nw->content);
		    for (j=0; j<index; j++) mnup = NHWNextMenuItem(nw->content);
		    for (j=0; j<maxrows && mnup != NULL; j++) {
			if (mnup->id.a_void && mnup->accelerator == ink) {
			    mnup->selected = !mnup->selected;
			    mnup->count = cnt;
			    l = 1;
			    break;
			}
			mnup = NHWNextMenuItem(nw->content);
		    }
		    if (!l) {
			mnup = NHWFirstMenuItem(nw->content);
			while (mnup != NULL) {
			    if (mnup->id.a_void && mnup->groupacc == ink) {
				mnup->selected = !mnup->selected;
				mnup->count = 0;
				if (how == PICK_ONE) {
				    l = 1;
				    break;
				}
			    }
			    mnup = NHWNextMenuItem(nw->content);
			}
		    }
		    if (l && (how == PICK_NONE || how == PICK_ONE))
			goto endselect;
		    break;
	      }
	      cnt = 0;
	    }
	}
endselect:
	nhw_hide_menu();
	win32y_clear_nhwindow(NHW_WINID_MESSAGE);
	if (how == PICK_NONE) {
	    if (selected) *selected = 0;
	    return 0;
	}
	mnup = NHWFirstMenuItem(nw->content);
	k = 0;
	while (mnup != NULL) {
	    if (mnup->id.a_void && mnup->selected) k++;
	    mnup = NHWNextMenuItem(nw->content);
	}
	if (k) {
	    mi = alloc(sizeof(menu_item)*k);
	    if (selected) *selected = mi;
	    mnup = NHWFirstMenuItem(nw->content);
	    while (mnup != NULL) {
		if (mnup->id.a_void && mnup->selected) {
		    mi->item  = mnup->id;
		    mi->count = (mnup->count) ? mnup->count : -1;
		    mi++;
		}
		mnup = NHWNextMenuItem(nw->content);
	    }
	    return k;
	}
	if (selected) *selected = 0;
	return 0;
}

/* display_nhwindow */
void win32y_display_nhwindow(winid win, boolean blocking) {
	int i;

	if (win < NHW_WINID_USER) {
	  switch (win) {
	    case NHW_WINID_MESSAGE:
		nhw_do_more(0); /* force --more-- **/
		break;
	    case NHW_WINID_BASE:
		nhw_update(win);
		break;
	    case NHW_WINID_MAP:
		if (blocking) nhw_do_more(0); /* force --more-- **/
		break;
	    default:
		break;
	  }
	} else {
	    i = winid2index(win);
	    if (i >= 0 && i < MAXWINNUM &&
		(winarray[i].type == NHW_MENU || winarray[i].type == NHW_TEXT)) {
		win32y_select_menu(win, PICK_NONE, 0);
	    }
	}
}

/* destroy_nhwindow */
void win32y_destroy_nhwindow(winid win) {
	int i;
	i = winid2index(win);
	if (i >= 0 && i < MAXWINNUM) {
	    if (winarray[i].type != 0) {
		NHWDisposeMenu(winarray[i].content);
		winarray[i].content = 0;
		winarray[i].type = 0;
	    }
	}
}

/* start_menu */
void win32y_start_menu(winid win) {
	int i;
	i = winid2index(win);
	if (winarray[i].type != NHW_MENU) return;
	if (winarray[i].content) {
		NHWDisposeMenu(winarray[i].content);
		winarray[i].content = NHWCreateMenu();
		winarray[i].num     = 0;
		winarray[i].width   = 0;
		winarray[i].hasacc  = 0;
		winarray[i].format  = -1;
	}
}

/* end_menu */
void win32y_end_menu(winid win, char *prompt) {
	int i, t;
	i = winid2index(win);
	if (winarray[i].type != NHW_MENU) return;
	if (prompt) {
	    anything dummy;
	    dummy.a_void = 0;
	    add_menu_sub(win, 0, &dummy, 0, 0, ATR_NONE, "", 0, 1);
	    add_menu_sub(win, 0, &dummy, 0, 0, ATR_NONE, prompt, 0, 1);
	}
}

/* message_menu */
char win32y_message_menu(char let, int how, const char *mesg) {
	char letpressed;

	/* "menu" without selection; use ordinary pline, no more() */
	if (how == PICK_NONE) {
	    pline("%s", mesg);
	    return 0;
	}

	win32y_putstr(WIN_MESSAGE, 0, mesg);
	return nhw_do_more(let);
}

/* player_selection ... picked from wintty.c */
void win32y_player_selection(void) {
	int i, k, n;
	char pick4u = 'n', thisch, lastch = 0;
	char pbuf[QBUFSZ], plbuf[QBUFSZ];
	winid win;
	anything any;
	menu_item *selected = 0;

	/* prevent an unnecessary prompt */
	rigid_role_checks();

	/* Should we randomly pick for the player? */
	if (!flags.randomall &&
	    (flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE ||
	     flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE)) {
	    int echoline;
	    char *prompt = build_plselection_prompt(pbuf, QBUFSZ, flags.initrole,
				flags.initrace, flags.initgend, flags.initalign);

	    win32y_raw_print("");
	    win32y_raw_print(prompt);
	    do {
		pick4u = lowc(win32y_nhgetch());
		if (index(quitchars, pick4u)) pick4u = 'y';
	    } while(!index(ynqchars, pick4u));
	    win32y_clear_nhwindow(NHW_WINID_BASE);
	    
	    if (pick4u != 'y' && pick4u != 'n') {
give_up:	/* Quit */
		if (selected) free((genericptr_t) selected);
		clearlocks();
		win32y_exit_nhwindows(NULL);
		terminate(EXIT_SUCCESS);
		/*NOTREACHED*/
		return;
	    }
	}

	(void)  root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);

	/* Select a role, if necessary */
	/* we'll try to be compatible with pre-selected race/gender/alignment,
	 * but may not succeed */
	if (flags.initrole < 0) {
	    char rolenamebuf[QBUFSZ];
	    /* Process the choice */
	    if (pick4u == 'y' || flags.initrole == ROLE_RANDOM || flags.randomall) {
		/* Pick a random role */
		flags.initrole = pick_role(flags.initrace, flags.initgend,
						flags.initalign, PICK_RANDOM);
		if (flags.initrole < 0) {
		    win32y_putstr(NHW_WINID_BASE, 0, "Incompatible role!");
		    flags.initrole = randrole();
		}
 	    } else {
	    	win32y_clear_nhwindow(NHW_WINID_BASE);
		win32y_putstr(NHW_WINID_BASE, 0, "Choosing Character's Role");
		/* Prompt for a role */
		win = create_nhwindow(NHW_MENU);
		start_menu(win);
		any.a_void = 0;         /* zero out all bits */
		for (i = 0; roles[i].name.m; i++) {
		    if (ok_role(i, flags.initrace, flags.initgend,
							flags.initalign)) {
			any.a_int = i+1;	/* must be non-zero */
			thisch = lowc(roles[i].name.m[0]);
			if (thisch == lastch) thisch = highc(thisch);
			if (flags.initgend != ROLE_NONE && flags.initgend != ROLE_RANDOM) {
				if (flags.initgend == 1  && roles[i].name.f)
					Strcpy(rolenamebuf, roles[i].name.f);
				else
					Strcpy(rolenamebuf, roles[i].name.m);
			} else {
				if (roles[i].name.f) {
					Strcpy(rolenamebuf, roles[i].name.m);
					Strcat(rolenamebuf, "/");
					Strcat(rolenamebuf, roles[i].name.f);
				} else 
					Strcpy(rolenamebuf, roles[i].name.m);
			}	
			add_menu(win, NO_GLYPH, &any, thisch,
			    0, ATR_NONE, an(rolenamebuf), MENU_UNSELECTED);
			lastch = thisch;
		    }
		}
		any.a_int = pick_role(flags.initrace, flags.initgend,
				    flags.initalign, PICK_RANDOM)+1;
		if (any.a_int == 0)	/* must be non-zero */
		    any.a_int = randrole()+1;
		add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				"Random", MENU_UNSELECTED);
		any.a_int = i+1;	/* must be non-zero */
		add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				"Quit", MENU_UNSELECTED);
		Sprintf(pbuf, "Pick a role for your %s", plbuf);
		end_menu(win, pbuf);
		n = select_menu(win, PICK_ONE, &selected);
		destroy_nhwindow(win);

		/* Process the choice */
		if (n != 1 || selected[0].item.a_int == any.a_int)
		    goto give_up;		/* Selected quit */

		flags.initrole = selected[0].item.a_int - 1;
		free((genericptr_t) selected),	selected = 0;
	    }
	    (void)  root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);
	}
	
	/* Select a race, if necessary */
	/* force compatibility with role, try for compatibility with
	 * pre-selected gender/alignment */
	if (flags.initrace < 0 || !validrace(flags.initrole, flags.initrace)) {
	    /* pre-selected race not valid */
	    if (pick4u == 'y' || flags.initrace == ROLE_RANDOM || flags.randomall) {
		flags.initrace = pick_race(flags.initrole, flags.initgend,
							flags.initalign, PICK_RANDOM);
		if (flags.initrace < 0) {
		    win32y_putstr(NHW_WINID_BASE, 0, "Incompatible race!");
		    flags.initrace = randrace(flags.initrole);
		}
	    } else {	/* pick4u == 'n' */
		/* Count the number of valid races */
		n = 0;	/* number valid */
		k = 0;	/* valid race */
		for (i = 0; races[i].noun; i++) {
		    if (ok_race(flags.initrole, i, flags.initgend,
							flags.initalign)) {
			n++;
			k = i;
		    }
		}
		if (n == 0) {
		    for (i = 0; races[i].noun; i++) {
			if (validrace(flags.initrole, i)) {
			    n++;
			    k = i;
			}
		    }
		}

		/* Permit the user to pick, if there is more than one */
		if (n > 1) {
		    win32y_clear_nhwindow(NHW_WINID_BASE);
		    win32y_putstr(NHW_WINID_BASE, 0, "Choosing Race");
		    win = create_nhwindow(NHW_MENU);
		    start_menu(win);
		    any.a_void = 0;         /* zero out all bits */
		    for (i = 0; races[i].noun; i++)
			if (ok_race(flags.initrole, i, flags.initgend,
							flags.initalign)) {
			    any.a_int = i+1;	/* must be non-zero */
			    add_menu(win, NO_GLYPH, &any, races[i].noun[0],
				0, ATR_NONE, races[i].noun, MENU_UNSELECTED);
			}
		    any.a_int = pick_race(flags.initrole, flags.initgend,
					flags.initalign, PICK_RANDOM)+1;
		    if (any.a_int == 0)	/* must be non-zero */
			any.a_int = randrace(flags.initrole)+1;
		    add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				    "Random", MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    "Quit", MENU_UNSELECTED);
		    Sprintf(pbuf, "Pick the race of your %s", plbuf);
		    end_menu(win, pbuf);
		    n = select_menu(win, PICK_ONE, &selected);
		    destroy_nhwindow(win);
		    if (n != 1 || selected[0].item.a_int == any.a_int)
			goto give_up;		/* Selected quit */

		    k = selected[0].item.a_int - 1;
		    free((genericptr_t) selected),	selected = 0;
		}
		flags.initrace = k;
	    }
	    (void)  root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);
	}

	/* Select a gender, if necessary */
	/* force compatibility with role/race, try for compatibility with
	 * pre-selected alignment */
	if (flags.initgend < 0 || !validgend(flags.initrole, flags.initrace,
						flags.initgend)) {
	    /* pre-selected gender not valid */
	    if (pick4u == 'y' || flags.initgend == ROLE_RANDOM || flags.randomall) {
		flags.initgend = pick_gend(flags.initrole, flags.initrace,
						flags.initalign, PICK_RANDOM);
		if (flags.initgend < 0) {
		    win32y_putstr(NHW_WINID_BASE, 0, "Incompatible gender!");
		    flags.initgend = randgend(flags.initrole, flags.initrace);
		}
	    } else {	/* pick4u == 'n' */
		/* Count the number of valid genders */
		n = 0;	/* number valid */
		k = 0;	/* valid gender */
		for (i = 0; i < ROLE_GENDERS; i++) {
		    if (ok_gend(flags.initrole, flags.initrace, i,
							flags.initalign)) {
			n++;
			k = i;
		    }
		}
		if (n == 0) {
		    for (i = 0; i < ROLE_GENDERS; i++) {
			if (validgend(flags.initrole, flags.initrace, i)) {
			    n++;
			    k = i;
			}
		    }
		}

		/* Permit the user to pick, if there is more than one */
		if (n > 1) {
		    win32y_clear_nhwindow(NHW_WINID_BASE);
		    win32y_putstr(NHW_WINID_BASE, 0, "Choosing Gender");
		    win = create_nhwindow(NHW_MENU);
		    start_menu(win);
		    any.a_void = 0;         /* zero out all bits */
		    for (i = 0; i < ROLE_GENDERS; i++)
			if (ok_gend(flags.initrole, flags.initrace, i,
							    flags.initalign)) {
			    any.a_int = i+1;
			    add_menu(win, NO_GLYPH, &any, genders[i].adj[0],
				0, ATR_NONE, genders[i].adj, MENU_UNSELECTED);
			}
		    any.a_int = pick_gend(flags.initrole, flags.initrace,
					    flags.initalign, PICK_RANDOM)+1;
		    if (any.a_int == 0)	/* must be non-zero */
			any.a_int = randgend(flags.initrole, flags.initrace)+1;
		    add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				    "Random", MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    "Quit", MENU_UNSELECTED);
		    Sprintf(pbuf, "Pick the gender of your %s", plbuf);
		    end_menu(win, pbuf);
		    n = select_menu(win, PICK_ONE, &selected);
		    destroy_nhwindow(win);
		    if (n != 1 || selected[0].item.a_int == any.a_int)
			goto give_up;		/* Selected quit */

		    k = selected[0].item.a_int - 1;
		    free((genericptr_t) selected),	selected = 0;
		}
		flags.initgend = k;
	    }
	    (void)  root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);
	}

	/* Select an alignment, if necessary */
	/* force compatibility with role/race/gender */
	if (flags.initalign < 0 || !validalign(flags.initrole, flags.initrace,
							flags.initalign)) {
	    /* pre-selected alignment not valid */
	    if (pick4u == 'y' || flags.initalign == ROLE_RANDOM || flags.randomall) {
		flags.initalign = pick_align(flags.initrole, flags.initrace,
							flags.initgend, PICK_RANDOM);
		if (flags.initalign < 0) {
		    win32y_putstr(NHW_WINID_BASE, 0, "Incompatible alignment!");
		    flags.initalign = randalign(flags.initrole, flags.initrace);
		}
	    } else {	/* pick4u == 'n' */
		/* Count the number of valid alignments */
		n = 0;	/* number valid */
		k = 0;	/* valid alignment */
		for (i = 0; i < ROLE_ALIGNS; i++) {
		    if (ok_align(flags.initrole, flags.initrace, flags.initgend,
							i)) {
			n++;
			k = i;
		    }
		}
		if (n == 0) {
		    for (i = 0; i < ROLE_ALIGNS; i++) {
			if (validalign(flags.initrole, flags.initrace, i)) {
			    n++;
			    k = i;
			}
		    }
		}

		/* Permit the user to pick, if there is more than one */
		if (n > 1) {
		    win32y_clear_nhwindow(NHW_WINID_BASE);
		    win32y_putstr(NHW_WINID_BASE, 0, "Choosing Alignment");
		    win = create_nhwindow(NHW_MENU);
		    start_menu(win);
		    any.a_void = 0;         /* zero out all bits */
		    for (i = 0; i < ROLE_ALIGNS; i++)
			if (ok_align(flags.initrole, flags.initrace,
							flags.initgend, i)) {
			    any.a_int = i+1;
			    add_menu(win, NO_GLYPH, &any, aligns[i].adj[0],
				 0, ATR_NONE, aligns[i].adj, MENU_UNSELECTED);
			}
		    any.a_int = pick_align(flags.initrole, flags.initrace,
					    flags.initgend, PICK_RANDOM)+1;
		    if (any.a_int == 0)	/* must be non-zero */
			any.a_int = randalign(flags.initrole, flags.initrace)+1;
		    add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				    "Random", MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    "Quit", MENU_UNSELECTED);
		    Sprintf(pbuf, "Pick the alignment of your %s", plbuf);
		    end_menu(win, pbuf);
		    n = select_menu(win, PICK_ONE, &selected);
		    destroy_nhwindow(win);
		    if (n != 1 || selected[0].item.a_int == any.a_int)
			goto give_up;		/* Selected quit */

		    k = selected[0].item.a_int - 1;
		    free((genericptr_t) selected),	selected = 0;
		}
		flags.initalign = k;
	    }
	}
	/* Success! */
}

/* display_file */
void win32y_display_file(char *fname, boolean complain) {
	dlb *f;
	char buf[BUFSZ];
	char *cr;

	win32y_clear_nhwindow(WIN_MESSAGE);
	f = dlb_fopen(fname, "r");
	if (!f) {
	    if(complain) {
		perror(fname);
		Sprintf(buf, "Cannot open \"%s\".", fname);
		win32y_putstr(NHW_WINID_MESSAGE, ATR_NONE, buf);
	    } else if(u.ux) docrt();
	} else {
	    winid datawin = win32y_create_nhwindow(NHW_TEXT);
	    boolean empty = TRUE;

	    while (dlb_fgets(buf, BUFSZ, f)) {
		if ((cr = index(buf, '\n')) != 0) *cr = 0;
		if (index(buf, '\t') != 0) (void) tabexpand(buf);
		empty = FALSE;
		win32y_putstr(datawin, 0, buf);
	    }
	    if (!empty) win32y_display_nhwindow(datawin, FALSE);
	    win32y_destroy_nhwindow(datawin);
	    (void) dlb_fclose(f);
	}
}

/* Misc. Routines */

void win32y_nhbell(void) {
}

void win32y_mark_synch(void) {
}

void win32y_wait_synch(void) {
}

/* update_inventory */
void win32y_update_inventory(void) {
}

void win32y_delay_output(void) {
	int i;
	for (i=0; i<5; i++) {
	    win32y_get_nh_event();
	    Sleep(10);
	}
}

void win32y_askname(void) {
	int py;
	py = nhw_cury(NHW_WINID_BASE);
	do {
	    win32y_curs(NHW_WINID_BASE, 0, py);
	    win32y_putstr(NHW_WINID_BASE, 0, "Who are you? ");
	    getlin_core(NHW_WINID_BASE, plname, PL_NSIZ);
	    win32y_curs(NHW_WINID_BASE, 0, py+1);
	    if (!plname[0])
		win32y_putstr(NHW_WINID_BASE, 0, "Enter a name for your character...");
	} while (!plname[0]);
}

void win32y_cliparound(int x, int y) {
}

void win32y_number_pad(int state) {
}

void win32y_suspend_nhwindows(const char *str) {
}

void win32y_resume_nhwindows(void) {
}

void win32y_start_screen(void) {
}

void win32y_end_screen(void) {
}

void win32y_outrip(winid win, int i) {
	genl_outrip(win, i);
}

void win32y_preference_update(int preference) {
}

/**/
/* Interface definition, for windows.c */
struct window_procs win32y_procs = {
    "win32y",
    WC_HILITE_PET|WC_PLAYER_SELECTION|WC_MAP_MODE|
    WC_FONT_MAP|WC_FONT_MENU|WC_FONT_MESSAGE|WC_FONT_STATUS|WC_FONT_TEXT|
    WC_FONTSIZ_MAP|WC_FONTSIZ_MENU|WC_FONTSIZ_MESSAGE|WC_FONTSIZ_STATUS|WC_FONTSIZ_TEXT,
    0L,
    win32y_init_nhwindows,
    win32y_player_selection,
    win32y_askname,
    win32y_get_nh_event,
    win32y_exit_nhwindows,
    win32y_suspend_nhwindows,
    win32y_resume_nhwindows,
    win32y_create_nhwindow,
    win32y_clear_nhwindow,
    win32y_display_nhwindow,
    win32y_destroy_nhwindow,
    win32y_curs,
    win32y_putstr,
    win32y_display_file,
    win32y_start_menu,
    win32y_add_menu,
    win32y_end_menu,
    win32y_select_menu,
    win32y_message_menu,
    win32y_update_inventory,
    win32y_mark_synch,
    win32y_wait_synch,
#ifdef CLIPPING
    win32y_cliparound,
#endif
#ifdef POSITIONBAR
    donull,
#endif
    win32y_print_glyph,
    win32y_raw_print,
    win32y_raw_print_bold,
    win32y_nhgetch,
    win32y_nh_poskey,
    win32y_nhbell,
    win32y_doprev_message,
    win32y_yn_function,
    win32y_getlin,
    win32y_get_ext_cmd,
    win32y_number_pad,
    win32y_delay_output,
#ifdef CHANGE_COLOR	/* only a Mac option currently */
	donull,
	donull,
#endif
    /* other defs that really should go away (they're tty specific) */
    win32y_start_screen,
    win32y_end_screen,
    win32y_outrip,
    win32y_preference_update,
};

