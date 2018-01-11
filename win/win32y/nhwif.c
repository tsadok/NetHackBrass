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
extern int suppress_status;

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
	Menucolor patch support
   ================================================ */
#ifdef MENU_COLOR
extern struct menucoloring *menu_colorings;

STATIC_OVL boolean
get_menu_coloring(str, color, attr)
char *str;
int *color, *attr;
{
    struct menucoloring *tmpmc;
    if (iflags.use_menu_color)
	for (tmpmc = menu_colorings; tmpmc; tmpmc = tmpmc->next)
# ifdef MENU_COLOR_REGEX
#  ifdef MENU_COLOR_REGEX_POSIX
	    if (regexec(&tmpmc->match, str, 0, NULL, 0) == 0) {
#  else
	    if (re_search(&tmpmc->match, str, strlen(str), 0, 9999, 0) >= 0) {
#  endif
# else
	    if (pmatch(tmpmc->match, str)) {
# endif
		*color = tmpmc->color;
		*attr = tmpmc->attr;
		return TRUE;
	    }
    return FALSE;
}
#endif /* MENU_COLOR */

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
		winarray[idx].width = 0; /* refresh window width */
		for (mnup = NHWFirstMenuItem(winarray[idx].content);
		     mnup != NULL;
		     mnup = NHWNextMenuItem(winarray[idx].content)) {
		    mfp = mfip->itemcheckfunc(mnup);
		    mnup->fmt = mfp;
		    mnstr = mnup->str;
		    if (mfp != NULL) {
		      do {
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
		    } else {
			w = 0;
			if (mnup->id.a_void)
			    w = nhw_getmpwidth(NHW_WINID_MENU);
			w += nhw_getstrwidth(NHW_WINID_MENU, mnstr);
			if (winarray[idx].width < w) winarray[idx].width = w;
		    }
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
/*		-- ウィンドウへの次の出力は (x,y) から始まり、カーソルを (x,y) に
		   移動して表示します。旧版との互換性のため、ウィンドウのサイズが
		   (cols,rows) なら 1 <= x < cols, 0 <= y < rows でなければ
		   なりません。
		-- ステータスウィンドウのようにサイズ可変のウィンドウの場合、
		   ウィンドウ外にカーソルを移動しようとしたときの動作は未定義です。
		   Mac ポートでは、ステータスウィンドウが80桁2行分と仮定し、
		   超えると 0 に戻ります。
		-- curs_on_u(), ステータスの更新、画面上の位置の取得(identify,
		   teleportなど)から使用されます。
		-- NHW_MESSAGE, NHW_MENU, NHW_TEXT ウィンドウは tty ポートでは
		   curs() をサポートしていません。*/

/* get_nh_event */
void win32y_get_nh_event(void) {
	DWORD t;
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
}
/*		-- ウィンドウイベント(再描画イベントなど)を実行します。
		   tty と X window ポートでは何もしません。*/

/* nhgetch */
int win32y_nhgetch(void) {
	MSG msg;
	while (keybufempty() && GetMessage(&msg, NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	return ((int)keybufget() & 0x00ff);
}
/*		-- ユーザが入力した1文字を返します。
		-- tty ウィンドウポートでは、nhgetch() は OS の提供する
		   1文字入力の tgetch() を使うことを想定しています。
		   返り値は必ず 0 以外の文字でなければなりません。また、
		   メタ 0 (0 にメタ・ビットを付加した値) を返しても
		   いけません。*/

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
	if (winarray[i].width < j) winarray[i].width = j;
	if (identifier->a_void) winarray[i].hasacc = 1;
	winarray[i].num++;

	winarray[i].content = NHWAddMenuItem(winarray[i].content, &mnu, str, totop);
}
void win32y_add_menu(winid win, int glyph, const anything *identifier,
		      char accelerator, char groupacc,
		      int attr, char *str, boolean preselected) {
	add_menu_sub(win, glyph, identifier, accelerator, groupacc, attr, str, preselected, 0);
}
/*		-- 1行のテキスト str を指定されたメニューウィンドウに追加します。
		   identifier が 0 の場合、その行は選択できません(つまり
		   タイトルです)。0 以外なら、その値がその行が選択されたときに
		   返されます。accelerator は行を選択するのに使われるキーボード上の
		   キーです。選択可能なアイテムの accelerator が 0 の場合、
		   ウィンドウシステムは自身で定義した accelerator を割り当てて
		   かまいません。accelerator をユーザに見えるように表示するのは
		   ウィンドウシステムの役目です。("a - " を str の前に付けるなど)
		   attr の値は putstr() で使われるものと同じです。
		   glyph は行に付随するオプションの glyph です。ウィンドウポートが
		   glyph を表示できない、または表示したくないのであれば、
		   表示しなくてもかまいません。特に表示すべき glyph がない場合は
		   この値には NO_GLYPH が指定されます。
		-- 全ての accelerator は [A-Za-z] の範囲になければなりません。
		   ただし、tty プレイヤー選択コードが '*' を使用しているなどの
		   例外はあります。
		-- 呼び出し側が accelerator を指定する/しないを混ぜて使わない
		   ことを想定しています。全ての選択可能なアイテムに accelerator を
		   指定するか、ウィンドウシステムに全て任せるかのどちらかに
		   してください。両方を使わないこと。
		-- groupacc はグループ選択用の accelerator です。標準の accelerator
		   以外の全ての文字または数字が使用できます。0 を指定すると、
		   アイテムは group accelerator の影響は受けません。
		   group accelerator がメニューコマンド(またはユーザ定義の
		   エイリアス)と重なった場合、group accelerator は機能しません。
		   メニューコマンドとエイリアスが標準のオブジェクトシンボルと
		   ぶつからないように注意します。
		-- この選択肢をメニュー表示時にあらかじめ選択しておきたい場合、
		   preselected に TRUE を指定してください。*/

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
#ifndef JP
		if (!strncmp(str, "You die...", 10)) nhw_force_more();
#else
		if (!strncmp(str, "あなたは死にました", 18)) nhw_force_more();
#endif /*JP*/
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
/*		-- 指定された属性で str を表示します。表示可能な文字のみが
		   サポートされます。属性は次のうち１つを取ることができます：
			ATR_NONE (or 0)
			ATR_ULINE
			ATR_BOLD
			ATR_BLINK
			ATR_INVERSE
		   これらの属性のうちどれかをウィンドウポートがサポートしない
		   場合、未サポートの属性をサポートしている属性に置き換えて
		   かまいません(例: 全て ATR_INVERSE に割り当てるなど)。
		   putstr() は必要に応じて str の空白を詰めたり、改行したり、
		   末尾を切り詰めたりします。改行するときは、行末までの
		   残りの部分をクリアしなければなりません。
		-- putstr() は、呼ばれた順序と同じ順序でユーザが表示を見る
		   ように実装します。tty ポートでは、pline() の中で more()
		   を呼ぶか、同じ行の中に表示してしまうことでこれを実現して
		   います。*/

/* nh_poskey */
int win32y_nh_poskey(int *x, int *y, int *mod) {
    return win32y_nhgetch();
}
/*		-- ユーザから入力された 1文字を返すか、(おそらくマウスからの)
		   位置指定イベントを返します。返り値が 0 以外のときは
		   1文字が入力されたことを示します。0 のときは、MAP ウィンドウ
		   の中の位置が x, y および mod に返ります。mod は以下のうち
		   １つをとります：
			CLICK_1		mouse click type 1
			CLICK_2		mouse click type 2
		   それぞれのクリックタイプにはハードウェアがサポートする機能を
		   何でも割り当てられます。マウスがサポートされていない場合、
		   この関数は常に 0 以外の1文字を返します。*/

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
/*		-- 指定されたウィンドウの (x,y) にグリフを表示します。グリフとは
		   ウィンドウポートが画面上に表示するあらゆるものに一対一対応
		   する整数です。(シンボル、フォント、色、属性など)*/

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
/*		-- 指定された window を適切なときにクリアします。*/

/* yn_function */
char win32y_yn_function(const char *ques, const char *choices, char dflt) {
	char buf[BUFSZ*2];
	const char *p;
	char c, *p1, firstc;
	int cnt = 0, allowcnt = 0;
	int choiceslen;
	char ink;
	if (!choices) {
	    Sprintf(buf, "%s ", ques);
	    choiceslen = 0;
	} else {
	    Sprintf(buf, "%s [", ques);
	    p = choices;
	    p1 = eos(buf);
	    do {
		c = *p1++ = *p++;
		if (c == 0x1b) c = 0;
		if (c == '#') allowcnt = 1;
	    } while (c);
	    if (dflt) Sprintf(--p1, "] (%c) ", dflt);
	    else Sprintf(--p1, "] ");
	    choiceslen = strlen(choices);
	}
	nhw_force_more();
	win32y_putstr(NHW_WINID_MESSAGE, ATR_NONE, buf);
	/* if something is already in key buffer, use it */
	while ((ink = (char)win32y_nhgetch()) != 0) {
	    int i;
	    if (allowcnt && ink >= '0' && ink <= '9') {
		firstc = ink;
		break;
	    }
	    if (!choices) {
		;
	    } else if (ink == 0x1b) {
		ink = dflt;
		for (i=0; i<choiceslen; i++) {
		    if (choices[i] == 'q') { ink = 'q'; break; }
		    if (choices[i] == 'n') { ink = 'n'; break; }
		}
	    } else if (!isprint(ink) || ink == ' ') {
		/* SPC, CR, LF are converted to the default input */
		ink = dflt;
	    } else {
		/* check if the input is in choices */
		for (i=0; i<choiceslen; i++) {
		    if (choices[i] == ink) break; /* found */
		}
		if (i >= choiceslen) continue;	/* not found */
	    }
	    nhw_dontpack_msg();
	    return ink;
	}
	/* open dialog to get input */
	while ((ink = getyn_core(NHW_WINID_MESSAGE, choices, dflt, &cnt, firstc)) == C('p')) {
	    win32y_doprev_message();
	    win32y_consume_prevmsgkey();
	    supress_store = 1;
	    win32y_putstr(NHW_WINID_MESSAGE, ATR_NONE, buf);
	    supress_store = 0;
	    firstc = 0;
	}
	if (cnt > 0) ink = '#';
	yn_number = cnt;
	nhw_dontpack_msg(); /* do not pack next pline */
	return ink;
}
/*		-- ques, choices, default からプロンプトを作成して表示し、
		   choices または default の中から1文字を返します。
		   もし choices が NULL の場合、全ての入力可能な文字から
		   1文字を入力して返します。これは他の全てに優先します。
		   choices は英小文字で入力されることを想定しています。
		   ESC が入力されると、choices の中に 'q' または 'n' があれば
		   常にこの順番で変換し、存在しない場合は default に変換します。
		   他の全ての中断キャラクタ(SPACE, RETURN, NEWLINE) は
		   default に変換されます。
		-- choices の中に ESC が含まれていた場合、以降の文字は全て
		   入力可能な文字とみなされますが、ESC および以降の文字は
		   プロンプトには表示されません。
		-- choices の中に '#' が含まれていた場合、数値のカウントを
		   許可します。カウント値をグローバル変数 "yn_number" に代入し、
		   '#' を返してください。
		-- yn_function() は tty ポートでは最上位行を使用しています。
		   他のポートではポップアップウィンドウを使ってもかまいません。
		-- choices が NULL の場合、全ての入力可能な文字が受け付けられ、
		   大文字/小文字は変換されずにそのまま返されます。つまり、
		   呼び出し側が大文字/小文字を区別して判定する必要がある場合、
		   ユーザからの入力が正しいかどうかは呼び出し側で検査しなければ
		   なりません。*/

/* getlin */
void win32y_getlin(const char *ques, char *input) {
	char buf[BUFSZ+4];
	nhw_force_more();
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
/*		-- ques をプロンプトとして表示し、改行までの1行のテキストを
		   入力します。入力された文字列は改行を除いて返されます。
		   ESC はキャンセルに使われ、この場合文字列は "\033\000" が
		   返されます。
		-- getlin() は処理の前に flush_screen(1) を呼ばなければなりません。
		-- yn_function() は tty ポートでは最上位行を使用しています。
		   他のポートではポップアップウィンドウを使ってもかまいません。
		-- getlin() は入力バッファが最低でも BUFSZ バイトの大きさを持つと
		   みなしてかまいません。また、入力された文字列がヌル文字を
		   含めて BUFSZ バイトに収まるように末尾を切る必要があります。*/

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
/*		-- ウィンドウポート独自の拡張コマンドを取得します。
		   有効なら extcmdlist[] へのインデックスが返ります。
		   有効でない場合は -1 が返ります。*/

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
/*		-- 以前のメッセージを表示します。^P コマンドで使われます。
		-- tty ポートでは WIN_MESSAGE を1行さかのぼって表示します。*/

/* update_positionbar */
void win32y_update_positionbar(char *features) {
}
/*		-- Optional, POSITIONBAR must be defined. Provide some 
		   additional information for use in a horizontal
		   position bar (most useful on clipped displays).
		   Features is a series of char pairs.  The first char
		   in the pair is a symbol and the second char is the
		   column where it is currently located.
		   A '<' is used to mark an upstairs, a '>'
		   for a downstairs, and an '@' for the current player
		   location. A zero char marks the end of the list.*/

/* Window Utility Routines */

/* init_nhwindows */
void win32y_init_nhwindows(int *argcp, char **argv) {
	int i;

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
/*		-- NetHackで使われるウィンドウを初期化します。また、上で
		   挙げられている標準のウィンドウを作成します。表示はしません。
		-- ウィンドウポートに関係するコマンドラインオプションは
		   全てここで解釈し、取り除いて *argcp および *argv を
		   更新しなければなりません。
		-- メッセージウィンドウが作成されたときに、iflags.window_inited
		   を TRUE にセットする必要があります。そうしないと全ての
		   pline() が raw_print() で実行されてしまいます。
		** Why not have init_nhwindows() create all of the "standard"
		** windows?  Or at least all but WIN_INFO?	-dean */

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
/*		-- ウィンドウシステムを抜けます。raw_print() に使われる
		   ウィンドウを除いてすべてのウィンドウを破棄します。
		   可能なら str を表示します。*/

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
/*		-- "type" のウィンドウを作成します。*/

/* select_menu */
int win32y_select_menu(winid win, int how, menu_item **selected) {
	int i, j, k, l;
	int acc, cnt, ink;
	int maxrows;
	int color, attr, ismcol;
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

	nhw_force_more();
	index = 0;
	cnt = 0;
	while (1) {
	    nhw_clear(wid);
	    acc = 0;
	    Sprintf(buf, "(%d/%d)", (index / maxrows) + 1, (nw->num / maxrows) + 1);
	    pagestr = buf;
	    mnup = NHWFirstMenuItem(nw->content);
	    for (j=0; j<index; j++) mnup = NHWNextMenuItem(nw->content);
	    for (j=0; j<maxrows; j++) {
		if (mnup->id.a_void && !mnup->accelerator) {
		    mnup->accelerator = (acc < 26) ? acc+'a' : acc-26+'A';
		    acc++;
		}
		win32y_curs(wid, 0, j);
		attr = mnup->attr;
#ifdef MENU_COLOR
		if (iflags.use_menu_color && (ismcol = get_menu_coloring(mnup->str, &color, &attr))) {
		    if (color != NO_COLOR) nhw_setcolor(color);
		}
#endif
		if (!nw->format || mnup->fmt == NULL) {
		    /* put 'a - ' prompt */
		    if (nw->hasacc) {
			if (mnup->id.a_void) {
			    nhw_print_menuprompt(wid, mnup->accelerator,
						 !(mnup->selected) ? '-' : !(mnup->count) ? '+' : '#');
			    nhw_print(wid, mnup->str, attr);
			} else {
			    if (!strncmp(mnup->str, "    ", 4)) {
				nhw_print_menuprompt(wid, 0, 0);
				nhw_print(wid, mnup->str+4, attr);
			    } else
				nhw_print(wid, mnup->str, attr);
			}
		    } else {
			nhw_print(wid, mnup->str, attr);
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
			nhw_print(wid, p, attr);
		    } while (mfp->oend && mfp++);
		}
#ifdef MENU_COLOR
		if (ismcol && color != NO_COLOR) nhw_setcolor(-1);
#endif
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
/*		-- 選択されたアイテムの数を返します。何も選択されなければ
		   0 を返します。明示的にキャンセルされた場合は -1 を返します。
		   アイテムが選択されると、selected に menu_item 構造体の
		   配列が割り当てられて返されます。呼び出し側はこの配列が
		   不要になったら free する必要があります。selected の "count"
		   フィールドにはユーザの入力したカウント値が入ります。
		   ユーザがカウント値を入力しなかった場合は -1 (全部を示す) が
		   入ります。カウント値 0 は選択されなかったのと同じこととし、
		   リストに入れるべきではありません。何もアイテムが選択され
		   なかったときは、selected は NULL になります。
		   how はメニューの動作モードです。以下の3つのモードがあります。
			PICK_NONE: 選択可能なアイテムはない
			PICK_ONE:  1つのアイテムだけが選択可能
			PICK_ANY:  いくつでも選択可能
		   how に PICK_NONE が指定された場合、この関数は 0 または -1
		   しか返り値として返してはいけません。
		-- select_menu() をウィンドウに対して複数回呼んでもかまいません。
		   ウィンドウに対して start_menu() または destroy_nhwindow() が
		   呼ばれるまで、前のメニューが保持されています。
		-- NHW_MENU ウィンドウは select_menu() を呼ばれる必要がない
		   ことに気をつけてください。create_nhwindow() の時点で
		   select_menu() が呼ばれるかどうかを知る方法はありません。*/

/* display_nhwindow */
void win32y_display_nhwindow(winid win, boolean blocking) {
	int i;

	if (win < NHW_WINID_USER) {
	  switch (win) {
	    case NHW_WINID_MESSAGE:
		nhw_force_more();
		break;
	    case NHW_WINID_STATUS:
		if (blocking) {
		    suppress_status = 1;
		} else {
		    suppress_status = 0;
		    nhw_update(win);
		}
		break;
	    case NHW_WINID_BASE:
		nhw_update(win);
		break;
	    case NHW_WINID_MAP:
		if (blocking) nhw_force_more();
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
/*		-- ウィンドウを画面に表示します。ウィンドウに対して出力待ちの
		   データがあれば、ウィンドウに送られます。blocking が TRUE の
		   場合、display_nhwindow() はデータが全て画面に表示されて
		   ユーザが認識するまで戻りません。
		-- tty ウィンドウポートでは全ての呼び出しは blocking です。
		-- tty ウィンドウポートでは、display_nhwindow(WIN_MESSAGE,???)
		   は必要に応じて --more-- 処理を行います。*/

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
/*		-- ウィンドウが破棄されていない場合、破棄します。*/

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
/*		-- ウィンドウをメニューとして使用しはじめます。start_menu() は
		   add_menu() より前に呼ばれなければなりません。start_menu() を
		   呼んだ後に putstr() をウィンドウに対して呼ばないようにして
		   ください。NHW_MENU タイプのウィンドウのみがメニューとして
		   使用できます。*/

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
/*		-- メニューに項目を追加するのを終了し、ウィンドウを画面に
		   表示します(前面に移動する?)。prompt はユーザに提示する
		   プロンプトです。prompt が NULL の場合、プロンプトは
		   表示されません。
		** ここでウィンドウを表示すべきではないかもしれない。
		** 表示するのは select_menu() の役目のはず。 -dean */

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
/*		-- tty-specific hack to allow single line context-sensitive
		   help to behave compatibly with multi-line help menus.
		-- This should only be called when a prompt is active; it
		   sends `mesg' to the message window.  For tty, it forces
		   a --More-- prompt and enables `let' as a viable keystroke
		   for dismissing that prompt, so that the original prompt
		   can be answered from the message line "help menu".
		-- Return value is either `let', '\0' (no selection was made),
		   or '\033' (explicit cancellation was requested).
		-- Interfaces which issue prompts and messages to separate
		   windows typically won't need this functionality, so can
		   substitute genl_message_menu (windows.c) instead.*/

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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Incompatible role!",
							"その職業は選べません！"));
		    flags.initrole = randrole();
		}
 	    } else {
	    	win32y_clear_nhwindow(NHW_WINID_BASE);
		win32y_putstr(NHW_WINID_BASE, 0, E_J("Choosing Character's Role",
						     "キャラクターの職業の選択"));
		/* Prompt for a role */
		win = create_nhwindow(NHW_MENU);
		start_menu(win);
		any.a_void = 0;         /* zero out all bits */
		for (i = 0; roles[i].name.m; i++) {
		    if (ok_role(i, flags.initrace, flags.initgend,
							flags.initalign)) {
			any.a_int = i+1;	/* must be non-zero */
#ifndef JP
			thisch = lowc(roles[i].name.m[0]);
#else
			thisch = lowc(roles[i].filecode[0]);
#endif /*JP*/
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
			    0, ATR_NONE, E_J(an(rolenamebuf),rolenamebuf), MENU_UNSELECTED);
			lastch = thisch;
		    }
		}
		any.a_int = pick_role(flags.initrace, flags.initgend,
				    flags.initalign, PICK_RANDOM)+1;
		if (any.a_int == 0)	/* must be non-zero */
		    any.a_int = randrole()+1;
		add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				E_J("Random","適当に選ぶ"), MENU_UNSELECTED);
		any.a_int = i+1;	/* must be non-zero */
		add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				E_J("Quit","中止"), MENU_UNSELECTED);
		Sprintf(pbuf, E_J("Pick a role for your %s","あなたの%sの職業を選んでください"), plbuf);
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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Incompatible race!",
							"その種族は選べません！"));
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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Choosing Race","種族の選択"));
		    win = create_nhwindow(NHW_MENU);
		    start_menu(win);
		    any.a_void = 0;         /* zero out all bits */
		    for (i = 0; races[i].noun; i++)
			if (ok_race(flags.initrole, i, flags.initgend,
							flags.initalign)) {
			    any.a_int = i+1;	/* must be non-zero */
#ifndef JP
			    add_menu(win, NO_GLYPH, &any, races[i].noun[0],
				0, ATR_NONE, races[i].noun, MENU_UNSELECTED);
#else
			    add_menu(win, NO_GLYPH, &any, lowc(races[i].filecode[0]),
				0, ATR_NONE, races[i].noun, MENU_UNSELECTED);
#endif /*JP*/
			}
		    any.a_int = pick_race(flags.initrole, flags.initgend,
					flags.initalign, PICK_RANDOM)+1;
		    if (any.a_int == 0)	/* must be non-zero */
			any.a_int = randrace(flags.initrole)+1;
		    add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				    E_J("Random","適当に選ぶ"), MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    E_J("Quit","中止"), MENU_UNSELECTED);
		    Sprintf(pbuf, E_J("Pick the race of your %s",
				      "あなたの%sの種族を選んでください"), plbuf);
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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Incompatible gender!",
							"その性別は選べません！"));
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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Choosing Gender","性別の選択"));
		    win = create_nhwindow(NHW_MENU);
		    start_menu(win);
		    any.a_void = 0;         /* zero out all bits */
		    for (i = 0; i < ROLE_GENDERS; i++)
			if (ok_gend(flags.initrole, flags.initrace, i,
							    flags.initalign)) {
			    any.a_int = i+1;
#ifndef JP
			    add_menu(win, NO_GLYPH, &any, genders[i].adj[0],
				0, ATR_NONE, genders[i].adj, MENU_UNSELECTED);
#else
			    add_menu(win, NO_GLYPH, &any, lowc(genders[i].filecode[0]),
				0, ATR_NONE, genders[i].adj, MENU_UNSELECTED);
#endif /*JP*/
			}
		    any.a_int = pick_gend(flags.initrole, flags.initrace,
					    flags.initalign, PICK_RANDOM)+1;
		    if (any.a_int == 0)	/* must be non-zero */
			any.a_int = randgend(flags.initrole, flags.initrace)+1;
		    add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				    E_J("Random","適当に選ぶ"), MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    E_J("Quit","中止"), MENU_UNSELECTED);
		    Sprintf(pbuf, E_J("Pick the gender of your %s",
				      "あなたの%sの性別を選んでください"), plbuf);
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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Choosing Alignment",
							"属性の選択"));
		    win = create_nhwindow(NHW_MENU);
		    start_menu(win);
		    any.a_void = 0;         /* zero out all bits */
		    for (i = 0; i < ROLE_ALIGNS; i++)
			if (ok_align(flags.initrole, flags.initrace,
							flags.initgend, i)) {
			    any.a_int = i+1;
#ifndef JP
			    add_menu(win, NO_GLYPH, &any, aligns[i].adj[0],
				 0, ATR_NONE, aligns[i].adj, MENU_UNSELECTED);
#else
			    add_menu(win, NO_GLYPH, &any, lowc(aligns[i].filecode[0]),
				 0, ATR_NONE, aligns[i].adj, MENU_UNSELECTED);
#endif /*JP*/
			}
		    any.a_int = pick_align(flags.initrole, flags.initrace,
					    flags.initgend, PICK_RANDOM)+1;
		    if (any.a_int == 0)	/* must be non-zero */
			any.a_int = randalign(flags.initrole, flags.initrace)+1;
		    add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				    E_J("Random","適当に選ぶ"), MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    E_J("Quit","中止"), MENU_UNSELECTED);
		    Sprintf(pbuf, E_J("Pick the alignment of your %s",
				      "あなたの%sの属性を選んでください"), plbuf);
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
/*		-- ウィンドウポートのプレイヤー選択処理を行います。
		   pl_character[0] に値を渡します。
		   player_selection() が Quit オプションを提供する場合、
		   プロセスの終了および付随する後始末はこの関数が行う必要が
		   あります。*/

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
/*		-- str で示される名前のファイルを表示します。complain が TRUE
		   なら、ファイルが見つからないときにその旨を表示します。*/

/* Misc. Routines */

void win32y_nhbell(void) {
}
/*		-- Beep at user.  [This will exist at least until sounds are 
		   redone, since sounds aren't attributable to windows anyway.]*/

void win32y_mark_synch(void) {
}
/*		-- Don't go beyond this point in I/O on any channel until
		   all channels are caught up to here.  Can be an empty call
		   for the moment */

void win32y_wait_synch(void) {
}
/*		-- Wait until all pending output is complete (*flush*() for
		   streams goes here).
		-- May also deal with exposure events etc. so that the
		   display is OK when return from wait_synch().*/

/* update_inventory */
void win32y_update_inventory(void) {
}
/*		-- ウィンドウポートに所持品が変更されたことを通知します。
		-- 所持品ウィンドウを開いたままにするウィンドウポート用に
		   呼ばれる関数です。そうでない場合何もする必要はありません。*/

void win32y_delay_output(void) {
	int i;
	for (i=0; i<5; i++) {
	    win32y_get_nh_event();
	    Sleep(10);
	}
}
/*		-- Causes a visible delay of 50ms in the output.
		   Conceptually, this is similar to wait_synch() followed
		   by a nap(50ms), but allows asynchronous operation.*/

void win32y_askname(void) {
	int py;
	py = nhw_cury(NHW_WINID_BASE);
	do {
	    win32y_curs(NHW_WINID_BASE, 0, py);
	    win32y_putstr(NHW_WINID_BASE, 0, E_J("Who are you? ","あなたの名前は？ "));
	    getlin_core(NHW_WINID_BASE, plname, PL_NSIZ);
	    win32y_curs(NHW_WINID_BASE, 0, py+1);
	    if (!plname[0])
		win32y_putstr(NHW_WINID_BASE, 0, E_J("Enter a name for your character...",
						     "あなたのキャラクターの名前を入力してください…。"));
	} while (!plname[0]);
}
/*		-- Ask the user for a player name.*/

void win32y_cliparound(int x, int y) {
}
/*		-- Make sure that the user is more-or-less centered on the
		   screen if the playing area is larger than the screen.
		-- This function is only defined if CLIPPING is defined.*/

void win32y_number_pad(int state) {
}
/*		-- Initialize the number pad to the given state.*/

void win32y_suspend_nhwindows(const char *str) {
}
/*		-- Prepare the window to be suspended.*/

void win32y_resume_nhwindows(void) {
}
/*		-- Restore the windows after being suspended.*/

void win32y_start_screen(void) {
}
/*		-- Only used on Unix tty ports, but must be declared for
		   completeness.  Sets up the tty to work in full-screen
		   graphics mode.  Look at win/tty/termcap.c for an
		   example.  If your window-port does not need this function
		   just declare an empty function.*/

void win32y_end_screen(void) {
}
/*		-- Only used on Unix tty ports, but must be declared for
		   completeness.  The complement of start_screen().*/

void win32y_outrip(winid win, int i) {
	genl_outrip(win, i);
}
/*		-- The tombstone code.  If you want the traditional code use
		   genl_outrip for the value and check the #if in rip.c.*/

void win32y_preference_update(int preference) {
}
/*		-- The player has just changed one of the wincap preference
		   settings, and the NetHack core is notifying your window
		   port of that change.  If your window-port is capable of
		   dynamically adjusting to the change then it should do so.
		   Your window-port will only be notified of a particular
		   change if it indicated that it wants to be by setting the 
		   corresponding bit in the wincap mask.*/

/**/
/* Interface definition, for windows.c */
struct window_procs win32y_procs = {
    "win32y",
    WC_HILITE_PET|WC_PLAYER_SELECTION|WC_MAP_MODE|
    WC_FONT_MAP|WC_FONT_MENU|WC_FONT_MESSAGE|WC_FONT_STATUS|WC_FONT_TEXT|
    WC_FONTSIZ_MAP|WC_FONTSIZ_MENU|WC_FONTSIZ_MESSAGE|WC_FONTSIZ_STATUS|WC_FONTSIZ_TEXT|
    WC_WINDOWCOLORS,
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

