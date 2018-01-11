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
/*		-- �E�B���h�E�ւ̎��̏o�͂� (x,y) ����n�܂�A�J�[�\���� (x,y) ��
		   �ړ����ĕ\�����܂��B���łƂ̌݊����̂��߁A�E�B���h�E�̃T�C�Y��
		   (cols,rows) �Ȃ� 1 <= x < cols, 0 <= y < rows �łȂ����
		   �Ȃ�܂���B
		-- �X�e�[�^�X�E�B���h�E�̂悤�ɃT�C�Y�ς̃E�B���h�E�̏ꍇ�A
		   �E�B���h�E�O�ɃJ�[�\�����ړ����悤�Ƃ����Ƃ��̓���͖���`�ł��B
		   Mac �|�[�g�ł́A�X�e�[�^�X�E�B���h�E��80��2�s���Ɖ��肵�A
		   ������� 0 �ɖ߂�܂��B
		-- curs_on_u(), �X�e�[�^�X�̍X�V�A��ʏ�̈ʒu�̎擾(identify,
		   teleport�Ȃ�)����g�p����܂��B
		-- NHW_MESSAGE, NHW_MENU, NHW_TEXT �E�B���h�E�� tty �|�[�g�ł�
		   curs() ���T�|�[�g���Ă��܂���B*/

/* get_nh_event */
void win32y_get_nh_event(void) {
	DWORD t;
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
}
/*		-- �E�B���h�E�C�x���g(�ĕ`��C�x���g�Ȃ�)�����s���܂��B
		   tty �� X window �|�[�g�ł͉������܂���B*/

/* nhgetch */
int win32y_nhgetch(void) {
	MSG msg;
	while (keybufempty() && GetMessage(&msg, NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	return ((int)keybufget() & 0x00ff);
}
/*		-- ���[�U�����͂���1������Ԃ��܂��B
		-- tty �E�B���h�E�|�[�g�ł́Anhgetch() �� OS �̒񋟂���
		   1�������͂� tgetch() ���g�����Ƃ�z�肵�Ă��܂��B
		   �Ԃ�l�͕K�� 0 �ȊO�̕����łȂ���΂Ȃ�܂���B�܂��A
		   ���^ 0 (0 �Ƀ��^�E�r�b�g��t�������l) ��Ԃ��Ă�
		   �����܂���B*/

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
/*		-- 1�s�̃e�L�X�g str ���w�肳�ꂽ���j���[�E�B���h�E�ɒǉ����܂��B
		   identifier �� 0 �̏ꍇ�A���̍s�͑I���ł��܂���(�܂�
		   �^�C�g���ł�)�B0 �ȊO�Ȃ�A���̒l�����̍s���I�����ꂽ�Ƃ���
		   �Ԃ���܂��Baccelerator �͍s��I������̂Ɏg����L�[�{�[�h���
		   �L�[�ł��B�I���\�ȃA�C�e���� accelerator �� 0 �̏ꍇ�A
		   �E�B���h�E�V�X�e���͎��g�Œ�`���� accelerator �����蓖�Ă�
		   ���܂��܂���Baccelerator �����[�U�Ɍ�����悤�ɕ\������̂�
		   �E�B���h�E�V�X�e���̖�ڂł��B("a - " �� str �̑O�ɕt����Ȃ�)
		   attr �̒l�� putstr() �Ŏg������̂Ɠ����ł��B
		   glyph �͍s�ɕt������I�v�V������ glyph �ł��B�E�B���h�E�|�[�g��
		   glyph ��\���ł��Ȃ��A�܂��͕\���������Ȃ��̂ł���΁A
		   �\�����Ȃ��Ă����܂��܂���B���ɕ\�����ׂ� glyph ���Ȃ��ꍇ��
		   ���̒l�ɂ� NO_GLYPH ���w�肳��܂��B
		-- �S�Ă� accelerator �� [A-Za-z] �͈̔͂ɂȂ���΂Ȃ�܂���B
		   �������Atty �v���C���[�I���R�[�h�� '*' ���g�p���Ă���Ȃǂ�
		   ��O�͂���܂��B
		-- �Ăяo������ accelerator ���w�肷��/���Ȃ��������Ďg��Ȃ�
		   ���Ƃ�z�肵�Ă��܂��B�S�Ă̑I���\�ȃA�C�e���� accelerator ��
		   �w�肷�邩�A�E�B���h�E�V�X�e���ɑS�ĔC���邩�̂ǂ��炩��
		   ���Ă��������B�������g��Ȃ����ƁB
		-- groupacc �̓O���[�v�I��p�� accelerator �ł��B�W���� accelerator
		   �ȊO�̑S�Ă̕����܂��͐������g�p�ł��܂��B0 ���w�肷��ƁA
		   �A�C�e���� group accelerator �̉e���͎󂯂܂���B
		   group accelerator �����j���[�R�}���h(�܂��̓��[�U��`��
		   �G�C���A�X)�Əd�Ȃ����ꍇ�Agroup accelerator �͋@�\���܂���B
		   ���j���[�R�}���h�ƃG�C���A�X���W���̃I�u�W�F�N�g�V���{����
		   �Ԃ���Ȃ��悤�ɒ��ӂ��܂��B
		-- ���̑I���������j���[�\�����ɂ��炩���ߑI�����Ă��������ꍇ�A
		   preselected �� TRUE ���w�肵�Ă��������B*/

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
		if (!strncmp(str, "���Ȃ��͎��ɂ܂���", 18)) nhw_force_more();
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
/*		-- �w�肳�ꂽ������ str ��\�����܂��B�\���\�ȕ����݂̂�
		   �T�|�[�g����܂��B�����͎��̂����P����邱�Ƃ��ł��܂��F
			ATR_NONE (or 0)
			ATR_ULINE
			ATR_BOLD
			ATR_BLINK
			ATR_INVERSE
		   �����̑����̂����ǂꂩ���E�B���h�E�|�[�g���T�|�[�g���Ȃ�
		   �ꍇ�A���T�|�[�g�̑������T�|�[�g���Ă��鑮���ɒu��������
		   ���܂��܂���(��: �S�� ATR_INVERSE �Ɋ��蓖�Ă�Ȃ�)�B
		   putstr() �͕K�v�ɉ����� str �̋󔒂��l�߂���A���s������A
		   ������؂�l�߂��肵�܂��B���s����Ƃ��́A�s���܂ł�
		   �c��̕������N���A���Ȃ���΂Ȃ�܂���B
		-- putstr() �́A�Ă΂ꂽ�����Ɠ��������Ń��[�U���\��������
		   �悤�Ɏ������܂��Btty �|�[�g�ł́Apline() �̒��� more()
		   ���ĂԂ��A�����s�̒��ɕ\�����Ă��܂����Ƃł������������
		   ���܂��B*/

/* nh_poskey */
int win32y_nh_poskey(int *x, int *y, int *mod) {
    return win32y_nhgetch();
}
/*		-- ���[�U������͂��ꂽ 1������Ԃ����A(�����炭�}�E�X�����)
		   �ʒu�w��C�x���g��Ԃ��܂��B�Ԃ�l�� 0 �ȊO�̂Ƃ���
		   1���������͂��ꂽ���Ƃ������܂��B0 �̂Ƃ��́AMAP �E�B���h�E
		   �̒��̈ʒu�� x, y ����� mod �ɕԂ�܂��Bmod �͈ȉ��̂���
		   �P���Ƃ�܂��F
			CLICK_1		mouse click type 1
			CLICK_2		mouse click type 2
		   ���ꂼ��̃N���b�N�^�C�v�ɂ̓n�[�h�E�F�A���T�|�[�g����@�\��
		   ���ł����蓖�Ă��܂��B�}�E�X���T�|�[�g����Ă��Ȃ��ꍇ�A
		   ���̊֐��͏�� 0 �ȊO��1������Ԃ��܂��B*/

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
/*		-- �w�肳�ꂽ�E�B���h�E�� (x,y) �ɃO���t��\�����܂��B�O���t�Ƃ�
		   �E�B���h�E�|�[�g����ʏ�ɕ\�����邠������̂Ɉ�Έ�Ή�
		   ���鐮���ł��B(�V���{���A�t�H���g�A�F�A�����Ȃ�)*/

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
/*		-- �w�肳�ꂽ window ��K�؂ȂƂ��ɃN���A���܂��B*/

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
/*		-- ques, choices, default ����v�����v�g���쐬���ĕ\�����A
		   choices �܂��� default �̒�����1������Ԃ��܂��B
		   ���� choices �� NULL �̏ꍇ�A�S�Ă̓��͉\�ȕ�������
		   1��������͂��ĕԂ��܂��B����͑��̑S�ĂɗD�悵�܂��B
		   choices �͉p�������œ��͂���邱�Ƃ�z�肵�Ă��܂��B
		   ESC �����͂����ƁAchoices �̒��� 'q' �܂��� 'n' �������
		   ��ɂ��̏��Ԃŕϊ����A���݂��Ȃ��ꍇ�� default �ɕϊ����܂��B
		   ���̑S�Ă̒��f�L�����N�^(SPACE, RETURN, NEWLINE) ��
		   default �ɕϊ�����܂��B
		-- choices �̒��� ESC ���܂܂�Ă����ꍇ�A�ȍ~�̕����͑S��
		   ���͉\�ȕ����Ƃ݂Ȃ���܂����AESC ����шȍ~�̕�����
		   �v�����v�g�ɂ͕\������܂���B
		-- choices �̒��� '#' ���܂܂�Ă����ꍇ�A���l�̃J�E���g��
		   �����܂��B�J�E���g�l���O���[�o���ϐ� "yn_number" �ɑ�����A
		   '#' ��Ԃ��Ă��������B
		-- yn_function() �� tty �|�[�g�ł͍ŏ�ʍs���g�p���Ă��܂��B
		   ���̃|�[�g�ł̓|�b�v�A�b�v�E�B���h�E���g���Ă����܂��܂���B
		-- choices �� NULL �̏ꍇ�A�S�Ă̓��͉\�ȕ������󂯕t�����A
		   �啶��/�������͕ϊ����ꂸ�ɂ��̂܂ܕԂ���܂��B�܂�A
		   �Ăяo�������啶��/����������ʂ��Ĕ��肷��K�v������ꍇ�A
		   ���[�U����̓��͂����������ǂ����͌Ăяo�����Ō������Ȃ����
		   �Ȃ�܂���B*/

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
/*		-- ques ���v�����v�g�Ƃ��ĕ\�����A���s�܂ł�1�s�̃e�L�X�g��
		   ���͂��܂��B���͂��ꂽ������͉��s�������ĕԂ���܂��B
		   ESC �̓L�����Z���Ɏg���A���̏ꍇ������� "\033\000" ��
		   �Ԃ���܂��B
		-- getlin() �͏����̑O�� flush_screen(1) ���Ă΂Ȃ���΂Ȃ�܂���B
		-- yn_function() �� tty �|�[�g�ł͍ŏ�ʍs���g�p���Ă��܂��B
		   ���̃|�[�g�ł̓|�b�v�A�b�v�E�B���h�E���g���Ă����܂��܂���B
		-- getlin() �͓��̓o�b�t�@���Œ�ł� BUFSZ �o�C�g�̑傫��������
		   �݂Ȃ��Ă��܂��܂���B�܂��A���͂��ꂽ�����񂪃k��������
		   �܂߂� BUFSZ �o�C�g�Ɏ��܂�悤�ɖ�����؂�K�v������܂��B*/

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
/*		-- �E�B���h�E�|�[�g�Ǝ��̊g���R�}���h���擾���܂��B
		   �L���Ȃ� extcmdlist[] �ւ̃C���f�b�N�X���Ԃ�܂��B
		   �L���łȂ��ꍇ�� -1 ���Ԃ�܂��B*/

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
/*		-- �ȑO�̃��b�Z�[�W��\�����܂��B^P �R�}���h�Ŏg���܂��B
		-- tty �|�[�g�ł� WIN_MESSAGE ��1�s�����̂ڂ��ĕ\�����܂��B*/

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
/*		-- NetHack�Ŏg����E�B���h�E�����������܂��B�܂��A���
		   �������Ă���W���̃E�B���h�E���쐬���܂��B�\���͂��܂���B
		-- �E�B���h�E�|�[�g�Ɋ֌W����R�}���h���C���I�v�V������
		   �S�Ă����ŉ��߂��A��菜���� *argcp ����� *argv ��
		   �X�V���Ȃ���΂Ȃ�܂���B
		-- ���b�Z�[�W�E�B���h�E���쐬���ꂽ�Ƃ��ɁAiflags.window_inited
		   �� TRUE �ɃZ�b�g����K�v������܂��B�������Ȃ��ƑS�Ă�
		   pline() �� raw_print() �Ŏ��s����Ă��܂��܂��B
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
/*		-- �E�B���h�E�V�X�e���𔲂��܂��Braw_print() �Ɏg����
		   �E�B���h�E�������Ă��ׂẴE�B���h�E��j�����܂��B
		   �\�Ȃ� str ��\�����܂��B*/

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
/*		-- "type" �̃E�B���h�E���쐬���܂��B*/

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
/*		-- �I�����ꂽ�A�C�e���̐���Ԃ��܂��B�����I������Ȃ����
		   0 ��Ԃ��܂��B�����I�ɃL�����Z�����ꂽ�ꍇ�� -1 ��Ԃ��܂��B
		   �A�C�e�����I�������ƁAselected �� menu_item �\���̂�
		   �z�񂪊��蓖�Ă��ĕԂ���܂��B�Ăяo�����͂��̔z��
		   �s�v�ɂȂ����� free ����K�v������܂��Bselected �� "count"
		   �t�B�[���h�ɂ̓��[�U�̓��͂����J�E���g�l������܂��B
		   ���[�U���J�E���g�l����͂��Ȃ������ꍇ�� -1 (�S��������) ��
		   ����܂��B�J�E���g�l 0 �͑I������Ȃ������̂Ɠ������ƂƂ��A
		   ���X�g�ɓ����ׂ��ł͂���܂���B�����A�C�e�����I������
		   �Ȃ������Ƃ��́Aselected �� NULL �ɂȂ�܂��B
		   how �̓��j���[�̓��샂�[�h�ł��B�ȉ���3�̃��[�h������܂��B
			PICK_NONE: �I���\�ȃA�C�e���͂Ȃ�
			PICK_ONE:  1�̃A�C�e���������I���\
			PICK_ANY:  �����ł��I���\
		   how �� PICK_NONE ���w�肳�ꂽ�ꍇ�A���̊֐��� 0 �܂��� -1
		   �����Ԃ�l�Ƃ��ĕԂ��Ă͂����܂���B
		-- select_menu() ���E�B���h�E�ɑ΂��ĕ�����Ă�ł����܂��܂���B
		   �E�B���h�E�ɑ΂��� start_menu() �܂��� destroy_nhwindow() ��
		   �Ă΂��܂ŁA�O�̃��j���[���ێ�����Ă��܂��B
		-- NHW_MENU �E�B���h�E�� select_menu() ���Ă΂��K�v���Ȃ�
		   ���ƂɋC�����Ă��������Bcreate_nhwindow() �̎��_��
		   select_menu() ���Ă΂�邩�ǂ�����m����@�͂���܂���B*/

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
/*		-- �E�B���h�E����ʂɕ\�����܂��B�E�B���h�E�ɑ΂��ďo�͑҂���
		   �f�[�^������΁A�E�B���h�E�ɑ����܂��Bblocking �� TRUE ��
		   �ꍇ�Adisplay_nhwindow() �̓f�[�^���S�ĉ�ʂɕ\�������
		   ���[�U���F������܂Ŗ߂�܂���B
		-- tty �E�B���h�E�|�[�g�ł͑S�Ă̌Ăяo���� blocking �ł��B
		-- tty �E�B���h�E�|�[�g�ł́Adisplay_nhwindow(WIN_MESSAGE,???)
		   �͕K�v�ɉ����� --more-- �������s���܂��B*/

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
/*		-- �E�B���h�E���j������Ă��Ȃ��ꍇ�A�j�����܂��B*/

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
/*		-- �E�B���h�E�����j���[�Ƃ��Ďg�p���͂��߂܂��Bstart_menu() ��
		   add_menu() ���O�ɌĂ΂�Ȃ���΂Ȃ�܂���Bstart_menu() ��
		   �Ă񂾌�� putstr() ���E�B���h�E�ɑ΂��ČĂ΂Ȃ��悤�ɂ���
		   ���������BNHW_MENU �^�C�v�̃E�B���h�E�݂̂����j���[�Ƃ���
		   �g�p�ł��܂��B*/

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
/*		-- ���j���[�ɍ��ڂ�ǉ�����̂��I�����A�E�B���h�E����ʂ�
		   �\�����܂�(�O�ʂɈړ�����?)�Bprompt �̓��[�U�ɒ񎦂���
		   �v�����v�g�ł��Bprompt �� NULL �̏ꍇ�A�v�����v�g��
		   �\������܂���B
		** �����ŃE�B���h�E��\�����ׂ��ł͂Ȃ���������Ȃ��B
		** �\������̂� select_menu() �̖�ڂ̂͂��B -dean */

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
							"���̐E�Ƃ͑I�ׂ܂���I"));
		    flags.initrole = randrole();
		}
 	    } else {
	    	win32y_clear_nhwindow(NHW_WINID_BASE);
		win32y_putstr(NHW_WINID_BASE, 0, E_J("Choosing Character's Role",
						     "�L�����N�^�[�̐E�Ƃ̑I��"));
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
				E_J("Random","�K���ɑI��"), MENU_UNSELECTED);
		any.a_int = i+1;	/* must be non-zero */
		add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				E_J("Quit","���~"), MENU_UNSELECTED);
		Sprintf(pbuf, E_J("Pick a role for your %s","���Ȃ���%s�̐E�Ƃ�I��ł�������"), plbuf);
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
							"���̎푰�͑I�ׂ܂���I"));
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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Choosing Race","�푰�̑I��"));
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
				    E_J("Random","�K���ɑI��"), MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    E_J("Quit","���~"), MENU_UNSELECTED);
		    Sprintf(pbuf, E_J("Pick the race of your %s",
				      "���Ȃ���%s�̎푰��I��ł�������"), plbuf);
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
							"���̐��ʂ͑I�ׂ܂���I"));
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
		    win32y_putstr(NHW_WINID_BASE, 0, E_J("Choosing Gender","���ʂ̑I��"));
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
				    E_J("Random","�K���ɑI��"), MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    E_J("Quit","���~"), MENU_UNSELECTED);
		    Sprintf(pbuf, E_J("Pick the gender of your %s",
				      "���Ȃ���%s�̐��ʂ�I��ł�������"), plbuf);
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
							"�����̑I��"));
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
				    E_J("Random","�K���ɑI��"), MENU_UNSELECTED);
		    any.a_int = i+1;	/* must be non-zero */
		    add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				    E_J("Quit","���~"), MENU_UNSELECTED);
		    Sprintf(pbuf, E_J("Pick the alignment of your %s",
				      "���Ȃ���%s�̑�����I��ł�������"), plbuf);
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
/*		-- �E�B���h�E�|�[�g�̃v���C���[�I���������s���܂��B
		   pl_character[0] �ɒl��n���܂��B
		   player_selection() �� Quit �I�v�V������񋟂���ꍇ�A
		   �v���Z�X�̏I������ѕt�������n���͂��̊֐����s���K�v��
		   ����܂��B*/

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
/*		-- str �Ŏ�����閼�O�̃t�@�C����\�����܂��Bcomplain �� TRUE
		   �Ȃ�A�t�@�C����������Ȃ��Ƃ��ɂ��̎|��\�����܂��B*/

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
/*		-- �E�B���h�E�|�[�g�ɏ����i���ύX���ꂽ���Ƃ�ʒm���܂��B
		-- �����i�E�B���h�E���J�����܂܂ɂ���E�B���h�E�|�[�g�p��
		   �Ă΂��֐��ł��B�����łȂ��ꍇ��������K�v�͂���܂���B*/

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
	    win32y_putstr(NHW_WINID_BASE, 0, E_J("Who are you? ","���Ȃ��̖��O�́H "));
	    getlin_core(NHW_WINID_BASE, plname, PL_NSIZ);
	    win32y_curs(NHW_WINID_BASE, 0, py+1);
	    if (!plname[0])
		win32y_putstr(NHW_WINID_BASE, 0, E_J("Enter a name for your character...",
						     "���Ȃ��̃L�����N�^�[�̖��O����͂��Ă��������c�B"));
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

