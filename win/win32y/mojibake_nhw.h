#ifndef __NHW_H__
#define __NHW_H__

#include "hack.h"
#include "win32api.h"

#define	NHW_WINID_MAP		0
#define	NHW_WINID_MESSAGE	1
#define	NHW_WINID_STATUS	2
#define	NHW_WINID_TEXT		3
#define	NHW_WINID_MENU		4
#define	NHW_WINID_BASE		5
#define	NHW_WINID_USER		6
#define	NHW_WINID_MAX		6

#define NHW_ATR_BOLD	1
#define NHW_ATR_INVERT	2
#define NHW_ATR_ULINE	3

/* ================================================
	back buffer structure
   ================================================ */
typedef struct nhwBackBufTag {
	HBITMAP	hBMP;	/* DIBSection */
	HDC	hdcMem;	/* memory dc */
	LPBYTE	lpBMP;	/* pixel buffer address */
	HWND	hWnd;	/* handle of main window */
	int	w;	/* width */
	int	h;	/* height */
} nhwBackBuf;

typedef struct nhwWinInfoTag {
	HFONT	font;	/* font used by this window type */
	int	fw;	/* font width */
	int	fh;	/* font height */
	int	isprop;	/* font is proportional */
	int	winw;	/* width of this window */
	int	winh;	/* height of this window */
	int	winx;	/* where to be displayed (x) */
	int	winy;	/* where to be displayed (y) */
	int	rows;	/* winh / fh */
	int	cx;	/* cursor x */
	int	cy;	/* cursor y */
} nhwWinInfo;

/* ================================================
	NetHack window structure
   ================================================ */
typedef struct {
	int	type;	/* MENU or TEXT */
	int	num;	/* num of items */
	char	hasacc; /* has accelerator */
	char	format; /* 0:none 1:do formatting -1:not defined yet */
	int	tabpos[16];	/* tab column position (by pixel) */
	int	width;	/* max width of the menu (by pixel) */
	BYTE	*content; /* Menu */
} NHWin;

/* ================================================
	menu structure
   ================================================ */
#define	DEFMENUSIZE	4096

#define MF_ALIGNRIGHT	0x00000001
#define MF_ALIGNCENTER	0x00000002
#define MF_NOSPACING	0x00000004

typedef struct {
	int	ostart;	/* starting offset in string */
	int	oend;	/* ending offset in string (0:until EOS) */
	int	flags;	/* formatting flags */
	int	column;	/* column to place string */
} NHWMenuFormat;

typedef struct NHWMenuItemTag {
	long	nextoffset;
	long	prevoffset;

	NHWMenuFormat *fmt;

	anything id;
	int	attr;
	int	count;
	char	accelerator;
	char	groupacc;
	char	selected;
	char	dummy;

	char	str[4];
} NHWMenuItem;

typedef struct NHWMenuHeaderTag {
	long	bufsize;	/* buffer size including this header */
	long	storeoffset;	/* next offset to store data */
	long	firstoffset;	/* offset of first item in link list(0:none) */
	long	lastoffset;	/* offset of last  item in link list(0:none) */
	long	curroffset;	/* current item */
} NHWMenuHeader;

typedef int (* NHWMenuCheckFunc)(BYTE *);
typedef NHWMenuFormat *(*NHWItemCheckFunc)(NHWMenuItem *);

typedef struct {
	NHWMenuCheckFunc menucheckfunc;
	NHWItemCheckFunc itemcheckfunc;
} NHWMenuFormatInfo;

BYTE *NHWCreateMenu(void);
BYTE *NHWAddMenuItem(BYTE *, NHWMenuItem *, char *, int);
void NHWDisposeMenu(BYTE *);
NHWMenuItem *NHWFirstMenuItem(BYTE *);
NHWMenuItem *NHWNextMenuItem(BYTE *);
NHWMenuItem *NHWPrevMenuItem(BYTE *);

/* ================================================
	prototype definition
   ================================================ */
void  win32y_raw_print(char *);
void  win32y_raw_print_bold(char *);
void  win32y_curs(winid, int, int);
void  win32y_get_nh_event(void);
int   win32y_nhgetch(void);
int   win32y_nh_poskey(int *, int *, int *);
char  win32y_yn_function(const char *, const char *, char);
void  win32y_getlin(const char *, char *);
int   win32y_get_ext_cmd(void);
int   win32y_doprev_message(void);
void  win32y_init_nhwindows(int *, char **);
void  win32y_exit_nhwindows(const char *);
winid win32y_create_nhwindow(int);
void  win32y_clear_nhwindow(winid);
void  win32y_display_nhwindow(winid, boolean);
void  win32y_destroy_nhwindow(winid);
void  win32y_putstr(winid, int, const char *);
void  win32y_print_glyph(winid, int, int, int);
void  win32y_start_menu(winid);
void  win32y_add_menu(winid, int, const anything *,
		      char, char, int, char *, boolean);
int   win32y_select_menu(winid, int, menu_item **);
void  win32y_end_menu(winid, char *);
char  win32y_message_menu(char, int, const char *);
void  win32y_player_selection(void);
void  win32y_display_file(char *, boolean);
void  win32y_nhbell(void);
void  win32y_mark_synch(void);
void  win32y_wait_synch(void);
void  win32y_update_inventory(void);
void  win32y_update_positionbar(char *);
void  win32y_delay_output(void);
void  win32y_askname(void);
void  win32y_cliparound(int, int);
void  win32y_number_pad(int);
void  win32y_suspend_nhwindows(const char *);
void  win32y_resume_nhwindows(void);
void  win32y_start_screen(void);
void  win32y_end_screen(void);
void  win32y_outrip(winid, int);
void  win32y_preference_update(int);

void  win32y_consume_prevmsgkey(void);

/* ================================================
	function prototype definition
   ================================================ */
void  nhw_raw_print(char *);
void  nhw_raw_print_bold(char *);
void  nhw_print(int, char *, int);
void  nhw_print_msg(char *);
void  nhw_dontpack_msg(void);
char  nhw_do_more(char);
void  nhw_curs(int, int, int);
int   nhw_cury(int);
void  nhw_setpos(int, int, int);
int   nhw_setcolor(int);
void  nhw_clear(int);
int   nhw_getstrwidth(int, char *);
int   nhw_getMwidth(int);
HWND  nhw_getHWND(void);
void  nhw_display(HDC);
void  nhw_update(int);
void  nhw_printglyph(int, int, int, int, int);
void  nhw_show_menu(int);
void  nhw_hide_menu(void);
int   nhw_getmaxrows(int);
int   nhw_getmpwidth(int);
void  nhw_print_menuprompt(int, char, char);
void  nhw_setfont(int, char *, int);
void  nhw_setmapfont(int, int);
void  nhw_drawcursor(int, int, int);
int   nhw_is_proportional(int);
void  nhw_show_topten(void);
void  nhw_toptenfont(void);
int   getlin_core(int, char *, int);
char  getyn_core(int, const char *, char, int *);
int   getext_core(int, char *, int, char * (*)(char *));

void  init_nhw(HWND);
void  tini_nhw(void);

#endif /*__NHW_H__*/
