/*	SCCS Id: @(#)cmd.c	3.4	2003/02/06	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "func_tab.h"
#include "eshk.h"
/* #define DEBUG */	/* uncomment for debugging */

/*
 * Some systems may have getchar() return EOF for various reasons, and
 * we should not quit before seeing at least NR_OF_EOFS consecutive EOFs.
 */
#if defined(SYSV) || defined(DGUX) || defined(HPUX)
#define NR_OF_EOFS	20
#endif

#define CMD_TRAVEL (char)0x90

#ifdef DEBUG
/*
 * only one "wiz_debug_cmd" routine should be available (in whatever
 * module you are trying to debug) or things are going to get rather
 * hard to link :-)
 */
extern int NDECL(wiz_debug_cmd);
#endif

#ifdef DUMB	/* stuff commented out in extern.h, but needed here */
extern int NDECL(doapply); /**/
extern int NDECL(dorub); /**/
extern int NDECL(dojump); /**/
extern int NDECL(doextlist); /**/
extern int NDECL(dodrop); /**/
extern int NDECL(doddrop); /**/
extern int NDECL(dodown); /**/
extern int NDECL(doup); /**/
extern int NDECL(donull); /**/
extern int NDECL(dowipe); /**/
extern int NDECL(do_mname); /**/
extern int NDECL(ddocall); /**/
extern int NDECL(dotakeoff); /**/
extern int NDECL(doremring); /**/
extern int NDECL(dowear); /**/
extern int NDECL(doputon); /**/
extern int NDECL(doddoremarm); /**/
extern int NDECL(dokick); /**/
extern int NDECL(dofire); /**/
extern int NDECL(dothrow); /**/
extern int NDECL(doeat); /**/
extern int NDECL(done2); /**/
extern int NDECL(doengrave); /**/
extern int NDECL(dopickup); /**/
extern int NDECL(ddoinv); /**/
extern int NDECL(dotypeinv); /**/
extern int NDECL(dolook); /**/
extern int NDECL(doprgold); /**/
extern int NDECL(doprwep); /**/
extern int NDECL(doprarm); /**/
extern int NDECL(doprring); /**/
extern int NDECL(dopramulet); /**/
extern int NDECL(doprtool); /**/
extern int NDECL(dosuspend); /**/
extern int NDECL(doforce); /**/
extern int NDECL(doopen); /**/
extern int NDECL(doclose); /**/
extern int NDECL(dosh); /**/
extern int NDECL(dodiscovered); /**/
extern int NDECL(doset); /**/
extern int NDECL(dotogglepickup); /**/
extern int NDECL(dowhatis); /**/
extern int NDECL(doquickwhatis); /**/
extern int NDECL(dowhatdoes); /**/
extern int NDECL(dohelp); /**/
extern int NDECL(dohistory); /**/
extern int NDECL(doloot); /**/
extern int NDECL(dodrink); /**/
extern int NDECL(dodip); /**/
extern int NDECL(dosacrifice); /**/
extern int NDECL(dopray); /**/
extern int NDECL(doturn); /**/
extern int NDECL(doredraw); /**/
extern int NDECL(doread); /**/
extern int NDECL(dosave); /**/
extern int NDECL(dosearch); /**/
extern int NDECL(doidtrap); /**/
extern int NDECL(dopay); /**/
extern int NDECL(dosit); /**/
extern int NDECL(dotalk); /**/
extern int NDECL(docast); /**/
extern int NDECL(dovspell); /**/
extern int NDECL(dotele); /**/
extern int NDECL(dountrap); /**/
extern int NDECL(doversion); /**/
extern int NDECL(doextversion); /**/
extern int NDECL(doswapweapon); /**/
extern int NDECL(dowield); /**/
extern int NDECL(dowieldquiver); /**/
extern int NDECL(dozap); /**/
extern int NDECL(doorganize); /**/
#endif /* DUMB */

#ifdef OVL1
static int NDECL((*timed_occ_fn));
#endif /* OVL1 */

extern boolean notonhead;	/* for long worms */

STATIC_PTR int NDECL(doprev_message);
STATIC_PTR int NDECL(timed_occupation);
STATIC_PTR int NDECL(doextcmd);
STATIC_PTR int NDECL(domonability);
STATIC_PTR int NDECL(dotravel);
# ifdef WIZARD
STATIC_PTR int NDECL(wiz_prefix);
STATIC_PTR void FDECL(wiz_unavailable_cmd, (char));
STATIC_PTR int NDECL(wiz_wish);
STATIC_PTR int NDECL(wiz_identify);
STATIC_PTR int NDECL(wiz_map);
STATIC_PTR int NDECL(wiz_genesis);
STATIC_PTR int NDECL(wiz_where);
STATIC_PTR int NDECL(wiz_detect);
STATIC_PTR int NDECL(wiz_panic);
STATIC_PTR int NDECL(wiz_polyself);
STATIC_PTR int NDECL(wiz_level_tele);
STATIC_PTR int NDECL(wiz_level_change);
STATIC_PTR int NDECL(wiz_show_seenv);
STATIC_PTR int NDECL(wiz_show_vision);
STATIC_PTR int NDECL(wiz_mon_polycontrol);
STATIC_PTR int NDECL(wiz_show_wmodes);
STATIC_PTR int NDECL(wiz_gain_fullstat);
STATIC_DCL void FDECL(wiz_roominfo_sub, (int));
STATIC_PTR int NDECL(wiz_roominfo);
STATIC_PTR int NDECL(wiz_roomlist);
STATIC_PTR int NDECL(wiz_objdesc);
#if defined(__BORLANDC__) && !defined(_WIN32)
extern void FDECL(show_borlandc_stats, (winid));
#endif
#ifdef DEBUG_MIGRATING_MONS
STATIC_PTR int NDECL(wiz_migrate_mons);
#endif
STATIC_DCL void FDECL(count_obj, (struct obj *, long *, long *, BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(obj_chain, (winid, const char *, struct obj *, long *, long *));
STATIC_DCL void FDECL(mon_invent_chain, (winid, const char *, struct monst *, long *, long *));
STATIC_DCL void FDECL(mon_chain, (winid, const char *, struct monst *, long *, long *));
STATIC_DCL void FDECL(contained, (winid, const char *, long *, long *));
STATIC_PTR int NDECL(wiz_show_stats);
#  ifdef PORT_DEBUG
STATIC_DCL int NDECL(wiz_port_debug);
#  endif
# endif
STATIC_PTR int NDECL(enter_explore_mode);
STATIC_PTR int NDECL(doattributes);
STATIC_PTR int NDECL(doconduct); /**/
STATIC_PTR boolean NDECL(minimal_enlightenment);
//#ifdef AUTOTHRUST
STATIC_DCL int NDECL(dovattack);			/* autothrust */
STATIC_DCL int NDECL(autothrust);			/* autothrust */
STATIC_DCL struct monst* FDECL(autotarget_near, (int));	/* autothrust */
//#endif
STATIC_PTR int NDECL(playersteal);
STATIC_DCL int NDECL(specialpower);
STATIC_DCL int FDECL(getobj_filter_healer_special_power, (struct obj *));
#ifdef OVLB
STATIC_DCL void FDECL(enlght_line, (const char *,const char *,const char *));
STATIC_DCL char *FDECL(enlght_combatinc, (const char *,int,int,char *));
#ifdef UNIX
static void NDECL(end_of_input);
#endif
#endif /* OVLB */

static const char* readchar_queue="";

STATIC_DCL char *NDECL(parse);
STATIC_DCL boolean FDECL(help_dir, (CHAR_P,const char *));

#ifdef OVL1

STATIC_PTR int
doprev_message()
{
    return nh_doprev_message();
}

/* Count down by decrementing multi */
STATIC_PTR int
timed_occupation()
{
	(*timed_occ_fn)();
	if (multi > 0)
		multi--;
	return multi > 0;
}

/* If you have moved since initially setting some occupations, they
 * now shouldn't be able to restart.
 *
 * The basic rule is that if you are carrying it, you can continue
 * since it is with you.  If you are acting on something at a distance,
 * your orientation to it must have changed when you moved.
 *
 * The exception to this is taking off items, since they can be taken
 * off in a number of ways in the intervening time, screwing up ordering.
 *
 *	Currently:	Take off all armor.
 *			Picking Locks / Forcing Chests.
 *			Setting traps.
 */
void
reset_occupations()
{
	reset_remarm();
	reset_pick();
	reset_trapset();
}

/* If a time is given, use it to timeout this function, otherwise the
 * function times out by its own means.
 */
void
set_occupation(fn, txt, xtime)
int NDECL((*fn));
const char *txt;
int xtime;
{
	if (xtime) {
		occupation = timed_occupation;
		timed_occ_fn = fn;
	} else
		occupation = fn;
	occtxt = txt;
	occtime = 0;
	return;
}

#ifdef REDO

static char NDECL(popch);

/* Provide a means to redo the last command.  The flag `in_doagain' is set
 * to true while redoing the command.  This flag is tested in commands that
 * require additional input (like `throw' which requires a thing and a
 * direction), and the input prompt is not shown.  Also, while in_doagain is
 * TRUE, no keystrokes can be saved into the saveq.
 */
#define BSIZE 20
static char pushq[BSIZE], saveq[BSIZE];
static NEARDATA int phead, ptail, shead, stail;

static char
popch() {
	/* If occupied, return '\0', letting tgetch know a character should
	 * be read from the keyboard.  If the character read is not the
	 * ABORT character (as checked in pcmain.c), that character will be
	 * pushed back on the pushq.
	 */
	if (occupation) return '\0';
	if (in_doagain) return(char)((shead != stail) ? saveq[stail++] : '\0');
	else		return(char)((phead != ptail) ? pushq[ptail++] : '\0');
}

char
pgetchar() {		/* curtesy of aeb@cwi.nl */
	register int ch;

	if(!(ch = popch()))
		ch = nhgetch();
	return((char)ch);
}

/* A ch == 0 resets the pushq */
void
pushch(ch)
char ch;
{
	if (!ch)
		phead = ptail = 0;
	if (phead < BSIZE)
		pushq[phead++] = ch;
	return;
}

/* A ch == 0 resets the saveq.	Only save keystrokes when not
 * replaying a previous command.
 */
void
savech(ch)
char ch;
{
	if (!in_doagain) {
		if (!ch)
			phead = ptail = shead = stail = 0;
		else if (shead < BSIZE)
			saveq[shead++] = ch;
	}
	return;
}
#endif /* REDO */

#endif /* OVL1 */
#ifdef OVLB

STATIC_PTR int
doextcmd()	/* here after # - now read a full-word command */
{
	int idx, retval;

	/* keep repeating until we don't run help or quit */
	do {
	    idx = get_ext_cmd();
	    if (idx < 0) return 0;	/* quit */

	    retval = (*extcmdlist[idx].ef_funct)();
	} while (extcmdlist[idx].ef_funct == doextlist);

	return retval;
}

int
doextlist()	/* here after #? - now list all full-word commands */
{
	register const struct ext_func_tab *efp;
	char	 buf[BUFSZ];
	winid datawin;

	datawin = create_nhwindow(NHW_TEXT);
	putstr(datawin, 0, "");
	putstr(datawin, 0, E_J("            Extended Commands List",
			       "            拡張コマンドの一覧"));
	putstr(datawin, 0, "");
	putstr(datawin, 0, E_J("    Press '#', then type:",
			       "    '#'を押した後、以下を入力:"));
	putstr(datawin, 0, "");

	for(efp = extcmdlist; efp->ef_txt; efp++) {
		Sprintf(buf, E_J("    %-15s - %s.",
				 "    %-15s - %s"), efp->ef_txt, efp->ef_desc);
		putstr(datawin, 0, buf);
	}
	display_nhwindow(datawin, FALSE);
	destroy_nhwindow(datawin);
	return 0;
}

#ifdef TTY_GRAPHICS
#define MAX_EXT_CMD 40		/* Change if we ever have > 40 ext cmds */
/*
 * This is currently used only by the tty port and is
 * controlled via runtime option 'extmenu'
 */
int
extcmd_via_menu()	/* here after # - now show pick-list of possible commands */
{
    const struct ext_func_tab *efp;
    menu_item *pick_list = (menu_item *)0;
    winid win;
    anything any;
    const struct ext_func_tab *choices[MAX_EXT_CMD];
    char buf[BUFSZ];
    char cbuf[QBUFSZ], prompt[QBUFSZ], fmtstr[20];
    int i, n, nchoices, acount;
    int ret,  biggest;
    int accelerator, prevaccelerator;
    int  matchlevel = 0;

    ret = 0;
    cbuf[0] = '\0';
    biggest = 0;
    while (!ret) {
	    i = n = 0;
	    accelerator = 0;
	    any.a_void = 0;
	    /* populate choices */
	    for(efp = extcmdlist; efp->ef_txt; efp++) {
		if (!matchlevel || !strncmp(efp->ef_txt, cbuf, matchlevel)) {
			choices[i++] = efp;
			if ((int)strlen(efp->ef_desc) > biggest) {
				biggest = strlen(efp->ef_desc);
				Sprintf(fmtstr,"%%-%ds", biggest + 15);
			}
#ifdef DEBUG
			if (i >= MAX_EXT_CMD - 2) {
			    impossible("Exceeded %d extended commands in doextcmd() menu",
					MAX_EXT_CMD - 2);
			    return 0;
			}
#endif
		}
	    }
	    choices[i] = (struct ext_func_tab *)0;
	    nchoices = i;
	    /* if we're down to one, we have our selection so get out of here */
	    if (nchoices == 1) {
		for (i = 0; extcmdlist[i].ef_txt != (char *)0; i++)
			if (!strncmpi(extcmdlist[i].ef_txt, cbuf, matchlevel)) {
				ret = i;
				break;
			}
		break;
	    }

	    /* otherwise... */
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    prevaccelerator = 0;
	    acount = 0;
	    for(i = 0; choices[i]; ++i) {
		accelerator = choices[i]->ef_txt[matchlevel];
		if (accelerator != prevaccelerator || nchoices < (ROWNO - 3)) {
		    if (acount) {
 			/* flush the extended commands for that letter already in buf */
			Sprintf(buf, fmtstr, prompt);
			any.a_char = prevaccelerator;
			add_menu(win, NO_GLYPH, &any, any.a_char, 0,
					ATR_NONE, buf, FALSE);
			acount = 0;
		    }
		}
		prevaccelerator = accelerator;
		if (!acount || nchoices < (ROWNO - 3)) {
		    Sprintf(prompt, "%s [%s]", choices[i]->ef_txt,
				choices[i]->ef_desc);
		} else if (acount == 1) {
		    Sprintf(prompt, E_J("%s or %s","%s または %s"), choices[i-1]->ef_txt,
				choices[i]->ef_txt);
		} else {
		    Strcat(prompt,E_J(" or "," または "));
		    Strcat(prompt, choices[i]->ef_txt);
		}
		++acount;
	    }
	    if (acount) {
		/* flush buf */
		Sprintf(buf, fmtstr, prompt);
		any.a_char = prevaccelerator;
		add_menu(win, NO_GLYPH, &any, any.a_char, 0, ATR_NONE, buf, FALSE);
	    }
	    Sprintf(prompt, E_J("Extended Command: %s",
				"拡張コマンド: %s"), cbuf);
	    end_menu(win, prompt);
	    n = select_menu(win, PICK_ONE, &pick_list);
	    destroy_nhwindow(win);
	    if (n==1) {
		if (matchlevel > (QBUFSZ - 2)) {
			free((genericptr_t)pick_list);
#ifdef DEBUG
			impossible("Too many characters (%d) entered in extcmd_via_menu()",
				matchlevel);
#endif
			ret = -1;
		} else {
			cbuf[matchlevel++] = pick_list[0].item.a_char;
			cbuf[matchlevel] = '\0';
			free((genericptr_t)pick_list);
		}
	    } else {
		if (matchlevel) {
			ret = 0;
			matchlevel = 0;
		} else
			ret = -1;
	    }
    }
    return ret;
}
#endif

/* #monster command - use special monster ability while polymorphed */
STATIC_PTR int
domonability()
{
	if (can_breathe(youmonst.data)) return dobreathe();
	else if (attacktype(youmonst.data, AT_SPIT)) return dospit();
	else if (youmonst.data->mlet == S_NYMPH) return doremove();
	else if (attacktype(youmonst.data, AT_GAZE)) return dogaze();
	else if (is_were(youmonst.data)) return dosummon();
	else if (webmaker(youmonst.data)) return dospinweb();
	else if (is_hider(youmonst.data)) return dohide();
	else if (is_mind_flayer(youmonst.data)) return domindblast();
	else if (u.umonnum == PM_GREMLIN) {
	    if(IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
		if (split_mon(&youmonst, (struct monst *)0))
		    dryup(u.ux, u.uy, TRUE);
	    } else There(E_J("is no fountain here.","泉がない。"));
	} else if (is_unicorn(youmonst.data)) {
	    use_unicorn_horn((struct obj *)0);
	    return 1;
	} else if (youmonst.data->msound == MS_SHRIEK) {
	    You(E_J("shriek.","金切り声をあげた。"));
	    if(u.uburied)
		pline(E_J("Unfortunately sound does not carry well through rock.",
			  "残念ながら、音は岩の向こうまでは届かなかった。"));
	    else aggravate();
	} else if (Upolyd)
		pline(E_J("Any special ability you may have is purely reflexive.",
			  "あなたの持つ(かもしれない)特殊能力は、能動的には発動できない。"));
	else return specialpower();
//	else You("don't have a special ability in your normal form!");
	return 0;
}

STATIC_PTR int
enter_explore_mode()
{
#ifndef JP
	if(!discover && !wizard) {
		pline("Beware!  From explore mode there will be no return to normal game.");
		if (yn("Do you want to enter explore mode?") == 'y') {
			clear_nhwindow(WIN_MESSAGE);
			You("are now in non-scoring explore mode.");
			discover = TRUE;
		}
		else {
			clear_nhwindow(WIN_MESSAGE);
			pline("Resuming normal game.");
		}
	}
#else
	pline("('X'コマンドを空けるため、探検モードは廃止されました)");
#endif /*JP*/
	return 0;
}

#ifdef WIZARD

/* 2-stroke wizard commands */
STATIC_PTR int
wiz_prefix()
{
#ifdef REDO
	static int wizfnum = -1;
#else
	int wizfnum;
#endif
	struct wizcmdmenuitem {
	    int NDECL((*fnc));
	    const char *msg;
	};
	struct wizcmdmenuitem wizcmdmenu[] = {
	    { wiz_detect,	"edetect secret doors and traps"	},
	    { wiz_map,		"fdo magic mapping"			},
	    { wiz_genesis,	"gcreate monster"			},
	    { wiz_identify,	"iidentify items in pack"		},
	    { wiz_level_change,	"lchange experience level"		},
	    { wiz_show_rndmonst_prob,	"mshow prob. of monster generation"	},
	    { wiz_where,	"otell locations of special levels"	},
	    { wiz_polyself,	"ppolymorph self"			},
	    { wiz_level_tele,	"vdo trans-level teleport"		},
	    { wiz_wish,		"wmake wish"				},
	};

	if (!wizard) {
	    wiz_unavailable_cmd('W');
	    return 0;
	}

#ifdef REDO
	if (!in_doagain || wizfnum < 0)
#endif
	{
	    int i, n;
	    const char *p;
	    menu_item *selected;
	    winid win;
	    anything any;

	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    for (i=0; i < SIZE(wizcmdmenu); i++) {
		any.a_int = i+1;
		p = wizcmdmenu[i].msg;
		add_menu(win, NO_GLYPH, &any, *p, 0, ATR_NONE, p+1, MENU_UNSELECTED);
	    }
	    end_menu(win, "Wizard command:");
	    n = select_menu(win, PICK_ONE, &selected);
	    destroy_nhwindow(win);
	    if (n > 0) {
		wizfnum = selected[0].item.a_int-1;	/* get item selected */
		free((genericptr_t)selected);
	    } else wizfnum = -1;
	}

	if (wizfnum >= 0) return (wizcmdmenu[wizfnum].fnc)();
	pline("Never mind.");
	return 0;
}

STATIC_PTR void
wiz_unavailable_cmd(sym)
char sym;
{
	pline(E_J("Unavailable command '^%c'.",
		  "'^%c' は無効なコマンドです。"), sym);
}

/* ^W command - wish for something */
STATIC_PTR int
wiz_wish()	/* Unlimited wishes for debug mode by Paul Polderman */
{
	if (wizard) {
	    boolean save_verbose = flags.verbose;

	    flags.verbose = FALSE;
	    makewish();
	    flags.verbose = save_verbose;
	    (void) encumber_msg();
	} else
	    wiz_unavailable_cmd('W');
	return 0;
}

/* ^I command - identify hero's inventory */
STATIC_PTR int
wiz_identify()
{
	if (wizard)	identify_pack(0);
	else		wiz_unavailable_cmd('I');
	return 0;
}

/* ^F command - reveal the level map and any traps on it */
STATIC_PTR int
wiz_map()
{
	if (wizard) {
	    struct trap *t;
	    long save_Hconf = HConfusion,
		 save_Hhallu = HHallucination;

	    HConfusion = HHallucination = 0L;
	    for (t = ftrap; t != 0; t = t->ntrap) {
		t->tseen = 1;
		map_trap(t, TRUE);
	    }
	    do_mapping();
	    HConfusion = save_Hconf;
	    HHallucination = save_Hhallu;
	} else
	    wiz_unavailable_cmd('F');
	return 0;
}

/* ^G command - generate monster(s); a count prefix will be honored */
STATIC_PTR int
wiz_genesis()
{
	if (wizard)	(void) create_particular();
	else		wiz_unavailable_cmd('G');
	return 0;
}

/* ^O command - display dungeon layout */
STATIC_PTR int
wiz_where()
{
	if (wizard) (void) print_dungeon(FALSE, (schar *)0, (xchar *)0);
	else	    wiz_unavailable_cmd('O');
	return 0;
}

/* ^E command - detect unseen (secret doors, traps, hidden monsters) */
STATIC_PTR int
wiz_detect()
{
	if(wizard)  (void) findit();
	else	    wiz_unavailable_cmd('E');
	return 0;
}

/* ^V command - level teleport */
STATIC_PTR int
wiz_level_tele()
{
	if (wizard)	level_tele();
	else		wiz_unavailable_cmd('V');
	return 0;
}

/* #monpolycontrol command - choose new form for shapechangers, polymorphees */
STATIC_PTR int
wiz_mon_polycontrol()
{
    iflags.mon_polycontrol = !iflags.mon_polycontrol;
    pline("Monster polymorph control is %s.",
	  iflags.mon_polycontrol ? "on" : "off");
    return 0;
}

/* #levelchange command - adjust hero's experience level */
STATIC_PTR int
wiz_level_change()
{
    char buf[BUFSZ];
    int newlevel;
    int ret;

    getlin("To what experience level do you want to be set?", buf);
    (void)mungspaces(buf);
    if (buf[0] == '\033' || buf[0] == '\0') ret = 0;
    else ret = sscanf(buf, "%d", &newlevel);

    if (ret != 1) {
	pline(Never_mind);
	return 0;
    }
    if (newlevel == u.ulevel) {
	You("are already that experienced.");
    } else if (newlevel < u.ulevel) {
	if (u.ulevel == 1) {
	    You("are already as inexperienced as you can get.");
	    return 0;
	}
	if (newlevel < 1) newlevel = 1;
	while (u.ulevel > newlevel)
	    losexp("#levelchange");
    } else {
	if (u.ulevel >= MAXULEV) {
	    You("are already as experienced as you can get.");
	    return 0;
	}
	if (newlevel > MAXULEV) newlevel = MAXULEV;
	while (u.ulevel < newlevel)
	    pluslvl(FALSE);
    }
    u.ulevelmax = u.ulevel;
    return 0;
}

/* #panic command - test program's panic handling */
STATIC_PTR int
wiz_panic()
{
	if (yn("Do you want to call panic() and end your game?") == 'y')
		panic("crash test.");
        return 0;
}

/* #polyself command - change hero's form */
STATIC_PTR int
wiz_polyself()
{
        polyself(TRUE);
        return 0;
}

/* #seenv command */
STATIC_PTR int
wiz_show_seenv()
{
	winid win;
	int x, y, v, startx, stopx, curx;
	char row[COLNO+1];

	win = create_nhwindow(NHW_TEXT);
	/*
	 * Each seenv description takes up 2 characters, so center
	 * the seenv display around the hero.
	 */
	startx = max(1, u.ux-(COLNO/4));
	stopx = min(startx+(COLNO/2), COLNO);
	/* can't have a line exactly 80 chars long */
	if (stopx - startx == COLNO/2) startx++;

	for (y = 0; y < ROWNO; y++) {
	    for (x = startx, curx = 0; x < stopx; x++, curx += 2) {
		if (x == u.ux && y == u.uy) {
		    row[curx] = row[curx+1] = '@';
		} else {
		    v = levl[x][y].seenv & 0xff;
		    if (v == 0)
			row[curx] = row[curx+1] = ' ';
		    else
			Sprintf(&row[curx], "%02x", v);
		}
	    }
	    /* remove trailing spaces */
	    for (x = curx-1; x >= 0; x--)
		if (row[x] != ' ') break;
	    row[x+1] = '\0';

	    putstr(win, 0, row);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
	return 0;
}

/* #vision command */
STATIC_PTR int
wiz_show_vision()
{
	winid win;
	int x, y, v;
	char row[COLNO+1];

	win = create_nhwindow(NHW_TEXT);
	Sprintf(row, "Flags: 0x%x could see, 0x%x in sight, 0x%x temp lit",
		COULD_SEE, IN_SIGHT, TEMP_LIT);
	putstr(win, 0, row);
	putstr(win, 0, "");
	for (y = 0; y < ROWNO; y++) {
	    for (x = 1; x < COLNO; x++) {
		if (x == u.ux && y == u.uy)
		    row[x] = '@';
		else {
		    v = viz_array[y][x]; /* data access should be hidden */
		    if (v == 0)
			row[x] = ' ';
		    else
			row[x] = '0' + viz_array[y][x];
		}
	    }
	    /* remove trailing spaces */
	    for (x = COLNO-1; x >= 1; x--)
		if (row[x] != ' ') break;
	    row[x+1] = '\0';

	    putstr(win, 0, &row[1]);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
	return 0;
}

/* #wmode command */
STATIC_PTR int
wiz_show_wmodes()
{
	winid win;
	int x,y;
	char row[COLNO+1];
	struct rm *lev;

	win = create_nhwindow(NHW_TEXT);
	for (y = 0; y < ROWNO; y++) {
	    for (x = 0; x < COLNO; x++) {
		lev = &levl[x][y];
		if (x == u.ux && y == u.uy)
		    row[x] = '@';
		else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
		    row[x] = '0' + (lev->wall_info & WM_MASK);
		else if (lev->typ == CORR)
		    row[x] = '#';
		else if (IS_ROOM(lev->typ) || IS_DOOR(lev->typ))
		    row[x] = '.';
		else
		    row[x] = 'x';
	    }
	    row[COLNO] = '\0';
	    putstr(win, 0, row);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
	return 0;
}

#endif /* WIZARD */


/* -enlightenment and conduct- */
static winid en_win;
static const char
	You_[] = E_J("You ","あなたは"),
	are[]  = E_J("are ","だ"),	    were[]  = E_J("were ","だった"),
	have[] = E_J("have ","いる"), had[]   = E_J("had ","いた"),
	can[]  = E_J("can ","できる"),	    could[] = E_J("could ","できた");
static const char
	have_been[]  = "have been ",
	have_never[] = "have never ", never[] = "never ";

#ifndef JP
#define enl_msg(prefix,present,past,suffix) \
			enlght_line(prefix, final ? past : present, suffix)
#else
#define enl_msg(prefix,present,past,suffix) \
			enlght_line(prefix, suffix, final ? past : present)
#endif /*JP*/
#define you_are(attr)	enl_msg(You_,are,were,attr)
#define you_have(attr)	enl_msg(You_,have,had,attr)
#define you_can(attr)	enl_msg(You_,can,could,attr)
#define you_have_been(goodthing) enl_msg(You_,have_been,were,goodthing)
#define you_have_never(badthing) enl_msg(You_,have_never,never,badthing)
#define you_have_X(something)	enl_msg(You_,have,(const char *)"",something)

static void
enlght_line(start, middle, end)
const char *start, *middle, *end;
{
	char buf[BUFSZ];

	Sprintf(buf, E_J("%s%s%s.","%s%s%s。"), start, middle, end);
	putstr(en_win, 0, buf);
}

/* format increased damage or chance to hit */
static char *
enlght_combatinc(inctyp, incamt, final, outbuf)
const char *inctyp;
int incamt, final;
char *outbuf;
{
	char numbuf[24];
	const char *modif, *bonus;

	if (final
#ifdef WIZARD
		|| wizard
#endif
	  ) {
	    Sprintf(numbuf, E_J("%s%d","%s%dの"),
		    (incamt > 0) ? "+" : "", incamt);
	    modif = (const char *) numbuf;
	} else {
	    int absamt = abs(incamt);

	    if (absamt <= 3) modif = E_J("small","若干の");
	    else if (absamt <= 6) modif = E_J("moderate","それなりの");
	    else if (absamt <= 12) modif = E_J("large","大きな");
	    else modif = E_J("huge","莫大な");
	}
	bonus = (incamt > 0) ? E_J("bonus","ボーナスを得て") : E_J("penalty","不利を被って");
#ifndef JP
	/* "bonus to hit" vs "damage bonus" */
	if (!strcmp(inctyp, "damage")) {
	    const char *ctmp = inctyp;
	    inctyp = bonus;
	    bonus = ctmp;
	}
	Sprintf(outbuf, "%s %s %s", an(modif), bonus, inctyp);
#else
	Sprintf(outbuf, "%sに%s%s", inctyp, modif, bonus);
#endif /*JP*/
	return outbuf;
}

void
enlightenment(final)
int final;	/* 0 => still in progress; 1 => over, survived; 2 => dead */
{
	int ltmp;
	int spdtmp;
	char buf[BUFSZ];

	en_win = create_nhwindow(NHW_MENU);
	putstr(en_win, 0, final ? E_J("Final Attributes:","最終属性:") :
				  E_J("Current Attributes:","現在の属性:"));
	putstr(en_win, 0, "");

#ifdef ELBERETH
	if (u.uevent.uhand_of_elbereth) {
	    static const char * const hofe_titles[3] = {
			E_J("the Hand of Elbereth","エルベレスの御手"),
			E_J("the Envoy of Balance","調和の使者"),
			E_J("the Glory of Arioch", "アリオッチの栄誉")
	    };
	    you_are(hofe_titles[u.uevent.uhand_of_elbereth - 1]);
	}
#endif

	/* note: piousness 20 matches MIN_QUEST_ALIGN (quest.h) */
	if (u.ualign.record >= 20)	you_are(E_J("piously aligned",	 "敬虔な信徒"));
	else if (u.ualign.record > 13)	you_are(E_J("devoutly aligned",	 "誠実な信徒"));
	else if (u.ualign.record > 8)	you_are(E_J("fervently aligned", "忠実な信徒"));
	else if (u.ualign.record > 3)	you_are(E_J("stridently aligned","大げさな信徒"));
	else if (u.ualign.record == 3)	you_are(E_J("aligned",		 "平均的な信徒"));
	else if (u.ualign.record > 0)	you_are(E_J("haltingly aligned", "迷い多き信徒"));
	else if (u.ualign.record == 0)	you_are(E_J("nominally aligned", "名ばかりの信徒"));
	else if (u.ualign.record >= -3)	you_have(E_J("strayed","信仰の道を外れて"));
	else if (u.ualign.record >= -8)	you_have(E_J("sinned","神に背いて"));
	else E_J(you_have("transgressed"),you_are("咎人"));
#ifdef WIZARD
	if (wizard) {
		Sprintf(buf, " %d", u.ualign.record);
#ifndef JP
		enl_msg("Your alignment ", "is", "was", buf);
#else
		enl_msg("あなたの信仰値は", " だ", " だった", buf);
#endif /*JP*/
	}
#endif

	/*** Resistances to troubles ***/
#ifndef JP
	if (Fire_resistance)
	    if (is_full_resist(FIRE_RES)) you_are("protected from fire");
	    else you_are("fire resistant");
	if (Cold_resistance)
	    if (is_full_resist(COLD_RES)) you_are("protected from cold");
	    else you_are("cold resistant");
	if (Sleep_resistance) you_are("sleep resistant");
	if (Disint_resistance)
	    if (is_full_resist(DISINT_RES)) you_are("protected from disintegration");
	    else you_are("disintegration-resistant");
	if (Shock_resistance)
	    if (is_full_resist(SHOCK_RES)) you_are("protected from electric shock");
	    else you_are("shock resistant");
	if (Poison_resistance) you_are("poison resistant");
	if (Drain_resistance) you_are("level-drain resistant");
	if (Sick_resistance) you_are("immune to sickness");
	if (Antimagic) you_are("magic-protected");
	if (Acid_resistance)
	    if (is_full_resist(ACID_RES)) you_are("protected from acid");
	    else you_are("acid resistant");
	if (Stone_resistance)
		you_are("petrification resistant");
	if (Invulnerable) you_are("invulnerable");
	if (u.uedibility || (uarmc && uarmc->otyp == KITCHEN_APRON && !uarmc->cursed))
		you_can("recognize detrimental food");
#else /*JP*/
	if (Fire_resistance)
	    if (is_full_resist(FIRE_RES)) you_have("火から完全に守られて");
	    else you_have("火への耐性を持って");
	if (Cold_resistance)
	    if (is_full_resist(COLD_RES)) you_have("冷気から完全に守られて");
	    else you_have("冷気への耐性を持って");
	if (Sleep_resistance) you_have("眠りへの耐性を持って");
	if (Disint_resistance)
	    if (is_full_resist(DISINT_RES)) you_have("分解から完全に守られて");
	    else you_have("分解への耐性を持って");
	if (Shock_resistance)
	    if (is_full_resist(SHOCK_RES)) you_have("電撃から完全に守られて");
	    else you_have("電撃への耐性を持って");
	if (Poison_resistance) you_have("毒への耐性を持って");
	if (Drain_resistance) you_have("レベルドレインへの耐性を持って");
	if (Sick_resistance) enl_msg("あなたには", "る", "った", "病気に対する免疫があ");
	if (Antimagic) you_have("魔法防御の能力を持って");
	if (Acid_resistance)
	    if (is_full_resist(ACID_RES)) you_are("酸から完全に守られて");
	    else you_have("酸への耐性を持って");
	if (Stone_resistance)
		you_have("石化への耐性を持って");
	if (Invulnerable) you_are("無敵");
	if (u.uedibility || (uarmc && uarmc->otyp == KITCHEN_APRON && !uarmc->cursed))
		you_can("有害な食物を見分けることが");
#endif /*JP*/

	/*** Troubles ***/
#ifndef JP
	if (Halluc_resistance)
		enl_msg("You resist", "", "ed", " hallucinations");
	if (final) {
		if (Hallucination) you_are("hallucinating");
		if (Stunned) you_are("stunned");
		if (Confusion) you_are("confused");
		if (Blinded) you_are("blinded");
		if (Sick) {
			if (u.usick_type & SICK_VOMITABLE)
				you_are("sick from food poisoning");
			if (u.usick_type & SICK_NONVOMITABLE)
				you_are("sick from illness");
		}
	}
	if (Stoned) you_are("turning to stone");
	if (Slimed) you_are("turning into slime");
	if (Strangled) you_are((StrangledBy & SUFFO_NECK) ? "being strangled" :
			       (StrangledBy & SUFFO_WATER) ? "drowning" :
			       (u.uburied) ? "buried" : "suffocating");
	if (Glib) {
		Sprintf(buf, "slippery %s", makeplural(body_part(FINGER)));
		you_have(buf);
	}
	if (Fumbling) enl_msg("You fumble", "", "d", "");
	if (Wounded_legs
#ifdef STEED
	    && !u.usteed
#endif
			  ) {
		Sprintf(buf, "wounded %s", makeplural(body_part(LEG)));
		you_have(buf);
	}
#if defined(WIZARD) && defined(STEED)
	if (Wounded_legs && u.usteed && wizard) {
	    Strcpy(buf, x_monnam(u.usteed, ARTICLE_YOUR, (char *)0, 
		    SUPPRESS_SADDLE | SUPPRESS_HALLUCINATION, FALSE));
	    *buf = highc(*buf);
	    enl_msg(buf, " has", " had", " wounded legs");
	}
#endif
	if (Sleeping) enl_msg("You ", "fall", "fell", " asleep");
	if (Hunger) enl_msg("You hunger", "", "ed", " rapidly");
#else /*JP*/
	if (Halluc_resistance)
		enl_msg(You_, "い", "かった", "幻覚に惑わされな");
	if (final) {
		if (Hallucination) you_have("幻覚を見て");
		if (Stunned) you_have("よろめいて");
		if (Confusion) you_have("混乱して");
		if (Blinded) you_are("盲目");
		if (Sick) {
			if (u.usick_type & SICK_VOMITABLE)
				you_have("食中毒にかかって");
			if (u.usick_type & SICK_NONVOMITABLE)
				you_have("病気で死にかけて");
		}
	}
	if (Stoned) enl_msg(You_, "る", "った", "石に変わりつつあ");
	if (Slimed) enl_msg(You_, "る", "った", "スライムと化しつつあ");
	if (Strangled) you_have((StrangledBy & SUFFO_NECK) ? "首を絞められて" :
				(StrangledBy & SUFFO_WATER) ? "溺れて" :
				(u.uburied) ? "生き埋めにされて" : "窒息して");
	if (Glib) {
		Sprintf(buf, "あなたの%sは滑りやす", body_part(FINGER));
		enl_msg(buf, "い", "かった", "");
	}
	if (Fumbling) you_are("ひどく不器用");
	if (Wounded_legs
#ifdef STEED
	    && !u.usteed
#endif
			  ) {
		Sprintf(buf, "%sを怪我して", body_part(LEG));
		you_have(buf);
	}
#if defined(WIZARD) && defined(STEED)
	if (Wounded_legs && u.usteed && wizard) {
	    Strcpy(buf, x_monnam(u.usteed, ARTICLE_YOUR, (char *)0, 
		    SUPPRESS_SADDLE | SUPPRESS_HALLUCINATION, FALSE));
	    enl_msg(buf, have, had, "脚を怪我して");
	}
#endif
	if (Sleeping) enl_msg(You_, "", "ことがあった", "不意に眠りに落ちる");
	if (Hunger) enl_msg("あなたは", "なる", "なっていた", "通常よりも速く空腹に");
#endif /*JP*/

	/*** Vision and senses ***/
#ifndef JP
	if (See_invisible) enl_msg(You_, "see", "saw", " invisible");
	if (Blind_telepat) you_are("telepathic");
	if (Warning) you_are("warned");
	if (Warn_of_mon && flags.warntype) {
		Sprintf(buf, "aware of the presence of %s",
			(flags.warntype & M2_ORC) ? "orcs" :
			(flags.warntype & M2_DEMON) ? "demons" :
			something); 
		you_are(buf);
	}
	if (Undead_warning) you_are("warned of undead");
	if (Searching) you_have("automatic searching");
	if (Clairvoyant) you_are("clairvoyant");
	if (Infravision) you_have("infravision");
	if (Detect_monsters) you_are("sensing the presence of monsters");
	if (u.umconf) you_are("going to confuse monsters");
#else /*JP*/
	if (See_invisible) you_can("透明なものを見ることが");
	if (Blind_telepat) enl_msg("あなたには", "る", "った", "テレパシーの能力があ");
	if (Warning) you_have("警戒能力を持って");
	if (Warn_of_mon && flags.warntype) {
		Sprintf(buf, "%sの気配を感じ取ることが",
			(flags.warntype & M2_ORC) ? "オーク" :
			(flags.warntype & M2_DEMON) ? "悪魔" :
			something); 
		you_can(buf);
	}
	if (Undead_warning) you_can("不死の存在を感じ取ることが");
	if (Searching) you_have("自動探査能力を持って");
	if (Clairvoyant) you_can("ときどき周囲の地形を透視");
	if (Infravision) enl_msg("あなたには", "る", "った", "赤外線視力があ");
	if (Detect_monsters) you_have("怪物の存在を感じ取って");
	if (u.umconf) you_can("怪物を混乱させることが");
#endif /*JP*/

	/*** Appearance and behavior ***/
	if (Adornment) {
	    int adorn = 0;

	    if(uleft && uleft->otyp == RIN_ADORNMENT) adorn += uleft->spe;
	    if(uright && uright->otyp == RIN_ADORNMENT) adorn += uright->spe;
	    if(uarmh && uarmh->otyp == KATYUSHA && flags.female) adorn += uarmh->spe;
#ifndef JP
	    if (adorn < 0)
		you_are("poorly adorned");
	    else
		you_are("adorned");
#else
	    if (adorn)
		enl_msg("あなたの装いは魅力を", "いる", "いた",
			adorn > 0 ? "増して" : "削いで");
#endif /*JP*/
	}
	if (Invisible) you_are(E_J("invisible","透明"));
#ifndef JP
	else if (Invis) you_are("invisible to others");
#else
	else if (Invis) enl_msg("あなたの姿は", "い", "かった", "他者から見られな");
#endif /*JP*/
	/* ordinarily "visible" is redundant; this is a special case for
	   the situation when invisibility would be an expected attribute */
	else if ((HInvis || EInvis || pm_invisible(youmonst.data)) && BInvis)
	    E_J(you_are("visible"),you_have("姿を現して"));
	if (Displaced) E_J(you_are("displaced"),you_have("自分の幻影を見せて"));
	if (Stealth) E_J(you_are("stealthy"),you_can("隠密のうちに行動"));
	if (Aggravate_monster) E_J(enl_msg("You aggravate", "", "d", " monsters"),
				   you_have("反感をかって"));
	if (Conflict) E_J(enl_msg("You cause", "", "d", " conflict"),
			  you_have("闘争を引き起こして"));

	/*** Transportation ***/
	if (Jumping) you_can(E_J("jump","跳躍することが"));
	if (Teleportation) you_can(E_J("teleport","テレポートすることが"));
	if (Teleport_control) E_J(you_have("teleport control"),you_can("テレポートを制御することが"));
	if (Lev_at_will) E_J(you_are("levitating, at will"),you_have("自分の意思で浮遊して"));
	else if (Levitation) E_J(you_are("levitating"),you_have("浮遊して"));	/* without control */
	else if (Flying) you_can(E_J("fly","飛行"));
	if (Wwalking) you_can(E_J("walk on water","水の上を歩くことが"));
	if (Swimming) E_J(you_can("swim"),enl_msg("あなたは", "る", "た", "泳げ"));
	if (Breathless) E_J(you_can("survive without air"),enl_msg("あなたは", "る", "た", "空気なしで生きられ"));
	else if (Amphibious) you_can(E_J("breathe water","水中で呼吸"));
	if (Passes_walls) you_can(E_J("walk through walls","壁を透過して移動"));
#ifdef STEED
	/* If you die while dismounting, u.usteed is still set.  Since several
	 * places in the done() sequence depend on u.usteed, just detect this
	 * special case. */
#ifndef JP
	if (u.usteed && (final < 2 || strcmp(killer, "riding accident"))) {
	    Sprintf(buf, "riding %s", y_monnam(u.usteed));
	    you_are(buf);
	}
#else
	if (u.usteed && (final < 2 || strcmp(killer, "乗馬中の事故で"))) {
	    Sprintf(buf, "%sに乗って", m_monnam(u.usteed));
	    you_have(buf);
	}
#endif /*JP*/
#endif
	if (u.uswallow) {
	    Sprintf(buf, E_J("swallowed by %s","%sに飲まれて"), a_monnam(u.ustuck));
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%u)", u.uswldtim);
#endif
	    E_J(you_are(buf),you_have(buf));
	} else if (u.ustuck) {
#ifndef JP
	    Sprintf(buf, "%s %s",
		    (Upolyd && sticks(youmonst.data)) ? "holding" : "held by",
		    a_monnam(u.ustuck));
	    you_are(buf);
#else
	    Sprintf(buf, "%s%s",
		    mon_nam(u.ustuck),
		    (Upolyd && sticks(youmonst.data)) ? "を捕らえて" : "に捕らえられて");
	    you_have(buf);
#endif /*JP*/
	}

	/*** Physical attributes ***/
	if (u.uhitinc)
	    you_have(enlght_combatinc(E_J("to hit","命中率"), u.uhitinc, final, buf));
	if (u.udaminc)
	    you_have(enlght_combatinc(E_J("damage","攻撃力"), u.udaminc, final, buf));
	if (Slow_digestion) E_J(you_have("slower digestion"),enl_msg(You_,"する","していた","食物をゆっくりと消化"));
	if (Regeneration) E_J(enl_msg("You regenerate", "", "d", ""),you_have("再生能力を持って"));
	if (u.uspellprot || Protection) {
	    int prot = 0;

//	    if(uleft && uleft->otyp == RIN_PROTECTION) prot += uleft->spe;
//	    if(uright && uright->otyp == RIN_PROTECTION) prot += uright->spe;
	    prot += u.uprotection;
	    if (HProtection & INTRINSIC) prot += u.ublessed;
	    prot += u.uspellprot;

	    if (prot <= 0)
		E_J(you_are("ineffectively protected"),you_have("力なく守られて"));
	    else
#ifdef WIZARD
		if (wizard) {
		    Sprintf(buf, "protected(%d:%d:%d)",
			u.uprotection, u.ublessed, u.uspellprot);
		    you_are(buf);
		} else
#endif
		E_J(you_are("protected"),you_have("守られて"));
	}
	if (Protection_from_shape_changers)
		E_J(you_are("protected from shape changers"),
		    you_have("変身する怪物から守られて"));
	if (Polymorph) E_J(you_are("polymorphing"),enl_msg(You_, "", "った", "変身体質だ"));
	if (Polymorph_control) E_J(you_have("polymorph control"),you_can("変身を制御"));
	if (u.ulycn >= LOW_PM) {
		Strcpy(buf, E_J(an(mons[u.ulycn].mname),JMONNAM(u.ulycn)));
		you_are(buf);
	}
	if (Upolyd) {
	    if (u.umonnum == u.ulycn) Strcpy(buf, E_J("in beast form","獣の姿をとって"));
	    else E_J(Sprintf(buf, "polymorphed into %s", an(youmonst.data->mname)),
		     Sprintf(buf, "%sに変化して", JMONNAM(monsndx(youmonst.data))));
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", u.mtimedone);
#endif
	    E_J(you_are(buf),you_have(buf));
	}
	if (Unchanging) E_J(you_can("not change from your current form"),
	    enl_msg(You_, "い", "かった", "現在の姿から変化できな"));
	spdtmp = (Very_fast) ? 20 : (Fast) ? 16 : 12;
	if (BFast && !Very_fast) {
		if (!Fast) spdtmp = 16;
		else spdtmp += 2;
	}
//Sprintf(buf, "Base speed: %d",spdtmp);
//enl_msg("","","",buf);
//Sprintf(buf, "Bonus speed by light-weight: %d",u.uspdbon1);
	spdtmp += speed_bonus();
	Sprintf(buf, E_J("%sfast","素速く行動"),
		(spdtmp >= 28) ? E_J("incredibly ","信じられないほど") :
		(spdtmp >= 24) ? E_J("extremely ", "極めて") :
		(spdtmp >= 20) ? E_J("very ",	   "とても") : ""
	);
#ifdef WIZARD
	if (wizard) {
	    if (spdtmp < 16) {
		Sprintf(buf, "a normal speed (%d)", spdtmp);
		you_have(buf);
	    } else {
		Sprintf(eos(buf), " (%d)", spdtmp);
	    }
	}
#endif
	if (spdtmp >= 16) E_J(you_are(buf),you_can(buf));
	if (Reflecting) you_have(E_J("reflection","反射能力を持って"));
	if (Free_action) E_J(you_have("free action"),enl_msg(You_,"い","かった","麻痺しな"));
	if (Fixed_abil) you_have(E_J("fixed abilities","能力値は固定されて"));
	if (Lifesaved)
	    E_J(enl_msg("Your life ", "will be", "would have been", " saved"),
		enl_msg("あなたの命は救われる","","はずだった",""));
	if (u.twoweap) you_are(E_J("wielding two weapons at once","二刀流"));

	/*** Miscellany ***/
	if (Luck) {
	    ltmp = abs((int)Luck);
#ifndef JP
	    Sprintf(buf, "%s%slucky",
		    ltmp >= 10 ? "extremely " : ltmp >= 5 ? "very " : "",
		    Luck < 0 ? "un" : "");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", Luck);
#endif
	    you_are(buf);
#else
	    Sprintf(buf, "%s運が%s",
		    ltmp >= 10 ? "きわめて" : ltmp >= 5 ? "とても" : "",
		    Luck < 0 ? "悪" : "良");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), "(%d)", Luck);
#endif
	    enl_msg(You_, "い", "かった", buf);
#endif /*JP*/
	}
#ifdef WIZARD
	 else if (wizard) E_J(enl_msg("Your luck ", "is", "was", " zero"),
			      enl_msg("あなたの運は0だ","","った",""));
#endif
	if (u.moreluck > 0) E_J(you_have("extra luck"),enl_msg("あなたの運勢は高められて","いる","いた",""));
	else if (u.moreluck < 0) you_have(E_J("reduced luck","運勢は引き下げられて"));
	if (carrying(LUCKSTONE) || stone_luck(TRUE)) {
	    ltmp = stone_luck(FALSE);
#ifndef JP
	    if (ltmp <= 0)
		enl_msg("Bad luck ", "does", "did", " not time out for you");
	    if (ltmp >= 0)
		enl_msg("Good luck ", "does", "did", " not time out for you");
#else
	    if (ltmp <= 0)
		enl_msg("あなたの運勢は時を経ても良くならな", "い", "かった", "");
	    if (ltmp >= 0)
		enl_msg("あなたの運勢は時を経ても悪くならな", "い", "かった", "");
#endif /*JP*/
	}

	if (u.ugangr) {
#ifndef JP
	    Sprintf(buf, " %sangry with you",
		    u.ugangr > 6 ? "extremely " : u.ugangr > 3 ? "very " : "");
#else
	    Sprintf(buf, "%sの%s怒りをかって", u_gname(),
		    u.ugangr > 6 ? "猛烈な" : u.ugangr > 3 ? "激しい" : "");
#endif /*JP*/
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", u.ugangr);
#endif
	    E_J(enl_msg(u_gname(), " is", " was", buf),you_have(buf));
	} else
	    /*
	     * We need to suppress this when the game is over, because death
	     * can change the value calculated by can_pray(), potentially
	     * resulting in a false claim that you could have prayed safely.
	     */
	  if (!final) {
#ifndef JP
#if 0
	    /* "can [not] safely pray" vs "could [not] have safely prayed" */
	    Sprintf(buf, "%s%ssafely pray%s", can_pray(FALSE) ? "" : "not ",
		    final ? "have " : "", final ? "ed" : "");
#else
	    Sprintf(buf, "%ssafely pray", can_pray(FALSE) ? "" : "not ");
#endif
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", u.ublesscnt);
#endif
	    you_can(buf);
#else
	    Sprintf(buf, "安全に祈ることが");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), "(%d)", u.ublesscnt);
#endif
	    if (can_pray(FALSE)) {
		you_can(buf);
	    } else {
		enl_msg(You_, "できない", "できなかった", buf);
	    }
#endif /*JP*/
	}

    {
	const char *p;

	buf[0] = '\0';
	if (final < 2) {    /* still in progress, or quit/escaped/ascended */
	    p = E_J("survived after being killed ","殺されたのち生還した");
	    switch (u.umortality) {
	    case 0:  p = !final ? (char *)0 : E_J("survived","生還した");  break;
	    case 1:  Strcpy(buf, E_J("once","一度"));  break;
	    case 2:  Strcpy(buf, E_J("twice","二度"));  break;
	    case 3:  Strcpy(buf, E_J("thrice","三度"));  break;
	    default: Sprintf(buf, E_J("%d times","%d回"), u.umortality);
		     break;
	    }
	} else {		/* game ended in character's death */
	    p = E_J("are dead","死んだ");
	    switch (u.umortality) {
	    case 0:  impossible("dead without dying?");
	    case 1:  break;			/* just "are dead" */
#ifndef JP
	    default: Sprintf(buf, " (%d%s time!)", u.umortality,
			     ordin(u.umortality));
#else
	    default: Sprintf(buf, "(%d回も！)", u.umortality);
#endif /*JP*/
		     break;
	    }
	}
	if (p) enl_msg(You_, E_J("have been killed ","殺された"), p, buf);
    }

	display_nhwindow(en_win, TRUE);
	destroy_nhwindow(en_win);
	return;
}

/*
 * Courtesy function for non-debug, non-explorer mode players
 * to help refresh them about who/what they are.
 * Returns FALSE if menu cancelled (dismissed with ESC), TRUE otherwise.
 */
STATIC_OVL boolean
minimal_enlightenment()
{
	winid tmpwin;
	menu_item *selected;
	anything any;
	int genidx, n;
	char buf[BUFSZ], buf2[BUFSZ];
	static const char untabbed_fmtstr[] = "%-15s: %-12s";
	static const char untabbed_deity_fmtstr[] = "%-17s%s";
	static const char tabbed_fmtstr[] = "%s:\t%-12s";
	static const char tabbed_deity_fmtstr[] = "%s\t%s";
	static const char *fmtstr;
	static const char *deity_fmtstr;

	fmtstr = iflags.menu_tab_sep ? tabbed_fmtstr : untabbed_fmtstr;
	deity_fmtstr = iflags.menu_tab_sep ?
			tabbed_deity_fmtstr : untabbed_deity_fmtstr; 
	any.a_void = 0;
	buf[0] = buf2[0] = '\0';
	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings, E_J("Starting","開始時"), FALSE);

	/* Starting name, race, role, gender */
	Sprintf(buf, fmtstr, E_J("name","名前"), plname);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	Sprintf(buf, fmtstr, E_J("race","種族"), urace.noun);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	Sprintf(buf, fmtstr, E_J("role","職業"),
		(flags.initgend && urole.name.f) ? urole.name.f : urole.name.m);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	Sprintf(buf, fmtstr, E_J("gender","性別"), genders[flags.initgend].adj);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

	/* Starting alignment */
	Sprintf(buf, fmtstr, E_J("alignment","属性"), align_str(u.ualignbase[A_ORIGINAL]));
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

	/* Current name, race, role, gender */
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", FALSE);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings, E_J("Current","現在"), FALSE);
	Sprintf(buf, fmtstr, E_J("race","種族"), Upolyd ? E_J(youmonst.data->mname,
							JMONNAM(u.umonnum)) : urace.noun);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	if (Upolyd) {
	    Sprintf(buf, fmtstr, E_J("role (base)","職業 (本来)"),
		(u.mfemale && urole.name.f) ? urole.name.f : urole.name.m);
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	} else {
	    Sprintf(buf, fmtstr, E_J("role","職業"),
		(flags.female && urole.name.f) ? urole.name.f : urole.name.m);
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	}
	/* don't want poly_gender() here; it forces `2' for non-humanoids */
	genidx = is_neuter(youmonst.data) ? 2 : flags.female;
	Sprintf(buf, fmtstr, E_J("gender","性別"), genders[genidx].adj);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	if (Upolyd && (int)u.mfemale != genidx) {
	    Sprintf(buf, fmtstr, E_J("gender (base)","性別 (本来)"), genders[u.mfemale].adj);
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);
	}

	/* Current alignment */
	Sprintf(buf, fmtstr, E_J("alignment","属性"), align_str(u.ualign.type));
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

	/* Deity list */
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", FALSE);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings, E_J("Deities","信仰"), FALSE);
	Sprintf(buf2, deity_fmtstr, align_gname(A_CHAOTIC),
	    (u.ualignbase[A_ORIGINAL] == u.ualign.type
		&& u.ualign.type == A_CHAOTIC) ? " (s,c)" :
	    (u.ualignbase[A_ORIGINAL] == A_CHAOTIC)       ? " (s)" :
	    (u.ualign.type   == A_CHAOTIC)       ? " (c)" : "");
	Sprintf(buf, fmtstr, E_J("Chaotic","混沌"), buf2);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

	Sprintf(buf2, deity_fmtstr, align_gname(A_NEUTRAL),
	    (u.ualignbase[A_ORIGINAL] == u.ualign.type
		&& u.ualign.type == A_NEUTRAL) ? " (s,c)" :
	    (u.ualignbase[A_ORIGINAL] == A_NEUTRAL)       ? " (s)" :
	    (u.ualign.type   == A_NEUTRAL)       ? " (c)" : "");
	Sprintf(buf, fmtstr, E_J("Neutral","中立"), buf2);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

	Sprintf(buf2, deity_fmtstr, align_gname(A_LAWFUL),
	    (u.ualignbase[A_ORIGINAL] == u.ualign.type &&
		u.ualign.type == A_LAWFUL)  ? " (s,c)" :
	    (u.ualignbase[A_ORIGINAL] == A_LAWFUL)        ? " (s)" :
	    (u.ualign.type   == A_LAWFUL)        ? " (c)" : "");
	Sprintf(buf, fmtstr, E_J("Lawful","秩序"), buf2);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, FALSE);

	end_menu(tmpwin, E_J("Base Attributes","基本属性"));
	n = select_menu(tmpwin, PICK_NONE, &selected);
	destroy_nhwindow(tmpwin);
	return (n != -1);
}

STATIC_PTR int
doattributes()
{
	if (!minimal_enlightenment())
		return 0;
	if (wizard || discover)
		enlightenment(0);
	return 0;
}

/* KMH, #conduct
 * (shares enlightenment's tense handling)
 */
STATIC_PTR int
doconduct()
{
	show_conduct(0);
	return 0;
}

void
show_conduct(final)
int final;
{
	char buf[BUFSZ];
	int ngenocided;

	/* Create the conduct window */
	en_win = create_nhwindow(NHW_MENU);
	putstr(en_win, 0, E_J("Voluntary challenges:","自発的な挑戦:"));
	putstr(en_win, 0, "");

	if (!u.uconduct.food)
#ifndef JP
	    enl_msg(You_, "have gone", "went", " without food");
#else
	    enl_msg(You_, "っていない", "らなかった", "食事を取");
#endif /*JP*/
	    /* But beverages are okay */
	else if (!u.uconduct.unvegan)
#ifndef JP
	    you_have_X("followed a strict vegan diet");
#else
	    enl_msg(You_, "ている", "た", "厳格な菜食主義を守っ");
#endif /*JP*/
	else if (!u.uconduct.unvegetarian)
#ifndef JP
	    you_have_been("vegetarian");
#else
	    you_are("菜食主義者");
#endif /*JP*/

	if (!u.uconduct.gnostic)
#ifndef JP
	    you_have_been("an atheist");
#else
	    you_are("無神論者");
#endif /*JP*/

	if (!u.uconduct.weaphit)
#ifndef JP
	    you_have_never("hit with a wielded weapon");
#else
	    enl_msg(You_, "たことがない", "なかった", "手にした武器で一度も攻撃を加え");
#endif /*JP*/
#ifdef WIZARD
	else if (wizard) {
#ifndef JP
	    Sprintf(buf, "used a wielded weapon %ld time%s",
		    u.uconduct.weaphit, plur(u.uconduct.weaphit));
	    you_have_X(buf);
#else
	    Sprintf(buf, "手にした武器で%ld回攻撃し", u.uconduct.weaphit);
	    enl_msg(You_, "ている", "た", buf);
#endif /*JP*/
	}
#endif
	if (!u.uconduct.killer)
#ifndef JP
	    you_have_been("a pacifist");
#else
	    you_are("平和主義者");
#endif /*JP*/

	if (!u.uconduct.literate)
#ifndef JP
	    you_have_been("illiterate");
#else
	    you_are("文盲");
#endif /*JP*/
#ifdef WIZARD
	else if (wizard) {
#ifndef JP
	    Sprintf(buf, "read items or engraved %ld time%s",
		    u.uconduct.literate, plur(u.uconduct.literate));
	    you_have_X(buf);
#else
	    Sprintf(buf, "品物や碑文などを%ld回読ん", u.uconduct.literate);
	    enl_msg(You_, "でいる", "だ", buf);
#endif /*JP*/
	}
#endif

	ngenocided = num_genocides();
	if (ngenocided == 0) {
#ifndef JP
	    you_have_never("genocided any monsters");
#else
	    enl_msg(You_, "していない", "しなかった", "怪物を虐殺");
#endif /*JP*/
	} else {
#ifndef JP
	    Sprintf(buf, "genocided %d type%s of monster%s",
		    ngenocided, plur(ngenocided), plur(ngenocided));
	    you_have_X(buf);
#else
	    Sprintf(buf, "%d種の怪物を虐殺した", ngenocided);
	    enl_msg(You_, "", "", buf);
#endif /*JP*/
	}

	if (!u.uconduct.polypiles)
#ifndef JP
	    you_have_never("polymorphed an object");
#else
	    enl_msg(You_, "ていない", "なかった", "物体を変化させ");
#endif /*JP*/
#ifdef WIZARD
	else if (wizard) {
	    Sprintf(buf, "polymorphed %ld item%s",
		    u.uconduct.polypiles, plur(u.uconduct.polypiles));
	    you_have_X(buf);
	}
#endif

	if (!u.uconduct.polyselfs)
#ifndef JP
	    you_have_never("changed form");
#else
	    enl_msg(You_, "ていない", "なかった", "変身し");
#endif /*JP*/
#ifdef WIZARD
	else if (wizard) {
	    Sprintf(buf, "changed form %ld time%s",
		    u.uconduct.polyselfs, plur(u.uconduct.polyselfs));
	    you_have_X(buf);
	}
#endif

	if (!u.uconduct.wishes)
#ifndef JP
	    you_have_X("used no wishes");
#else
	    enl_msg(You_, "ていない", "なかった", "願いごとをし");
#endif /*JP*/
	else {
#ifndef JP
	    Sprintf(buf, "used %ld wish%s",
		    u.uconduct.wishes, (u.uconduct.wishes > 1L) ? "es" : "");
	    you_have_X(buf);

	    if (!u.uconduct.wisharti)
		enl_msg(You_, "have not wished", "did not wish",
			" for any artifacts");
#else
	    Sprintf(buf, "%ld%sの願いごとをした",
		    u.uconduct.wishes, (u.uconduct.wishes < 10L) ? "つ" : "個");
	    enl_msg(You_, "", "", buf);

	    if (!u.uconduct.wisharti)
		enl_msg(You_, "っていない", "わなかった", "アーティファクトを願");
#endif /*JP*/
	}

	/* Pop up the window and wait for a key */
	display_nhwindow(en_win, TRUE);
	destroy_nhwindow(en_win);
}

#endif /* OVLB */
#ifdef OVL1

#ifndef M
# ifndef NHSTDC
#  define M(c)		(0x80 | (c))
# else
#  define M(c)		((c) - 128)
# endif /* NHSTDC */
#endif
#ifndef C
#define C(c)		(0x1f & (c))
#endif

static const struct func_tab cmdlist[] = {
	{C('d'), FALSE, dokick}, /* "D" is for door!...?  Msg is in dokick.c */
#ifdef WIZARD
	{C('e'), TRUE, wiz_detect},
	{C('f'), TRUE, wiz_map},
	{C('g'), TRUE, wiz_genesis},
	{C('i'), TRUE, wiz_identify},
#endif
	{C('l'), TRUE, doredraw}, /* if number_pad is set */
#ifdef WIZARD
	{C('o'), TRUE, wiz_where},
#endif
	{C('p'), TRUE, doprev_message},
	{C('r'), TRUE, doredraw},
	{C('t'), TRUE, dotele},
	{C('v'), TRUE, doversion},	/* youkan */
#ifdef WIZARD
	{C('w'), TRUE, wiz_prefix},	/* youkan */
//	{C('w'), TRUE, wiz_wish},
#endif
	{C('x'), TRUE, doattributes},
#ifdef SUSPEND
	{C('z'), TRUE, dosuspend},
#endif
	{'a', FALSE, doapply},
	{'A', FALSE, doddoremarm},
	{M('a'), TRUE, doorganize},
/*	'b', 'B' : go sw */
	{M('b'), FALSE, playersteal},
	{'c', FALSE, doclose},
	{'C', TRUE, do_mname},
	{M('c'), TRUE, dotalk},
	{'d', FALSE, dodrop},
	{'D', FALSE, doddrop},
	{M('d'), FALSE, dodip},
	{'e', FALSE, doeat},
	{'E', FALSE, doengrave},
	{M('e'), TRUE, show_weapon_skills},
	{'f', FALSE, dofire},
/*	'F' : fight (one time) */
	{M('f'), FALSE, doforce},
/*	'g', 'G' : multiple go */
/*	'h', 'H' : go west */
	{'h', TRUE, dohelp}, /* if number_pad is set */
	{'i', TRUE, ddoinv},
	{'I', TRUE, dotypeinv},		/* Robert Viduya */
	{M('i'), TRUE, doinvoke},
/*	'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N' : move commands */
	{'j', FALSE, dojump}, /* if number_pad is on */
	{M('j'), FALSE, dojump},
	{'k', FALSE, dokick}, /* if number_pad is on */
	{'l', FALSE, doloot}, /* if number_pad is on */
	{M('l'), FALSE, doloot},
/*	'n' prefixes a count if number_pad is on */
	{M('m'), TRUE, domonability},
	{'N', TRUE, ddocall}, /* if number_pad is on */
	{M('n'), TRUE, ddocall},
	{M('N'), TRUE, ddocall},
	{'o', FALSE, doopen},
	{'O', TRUE, doset},
	{M('o'), FALSE, dosacrifice},
	{'p', FALSE, dopay},
	{'P', FALSE, doputon},
	{M('p'), TRUE, dopray},
	{'q', FALSE, dodrink},
	{'Q', FALSE, dowieldquiver},
	{M('q'), TRUE, done2},
	{'r', FALSE, doread},
	{'R', FALSE, doremring},
#ifdef STEED
	{M('r'), FALSE, doride/*dorub*/},
#else
	{M('r'), FALSE, dorub},
#endif
	{'s', TRUE, dosearch, E_J("searching","探すの")},
	{'S', TRUE, dosave},
	{M('s'), FALSE, dosit},
	{'t', FALSE, dothrow},
	{'T', FALSE, dotakeoff},
	{M('t'), TRUE, doturn},
/*	'u', 'U' : go ne */
	{'u', FALSE, dountrap}, /* if number_pad is on */
	{M('u'), FALSE, dountrap},
	{'v', TRUE, dovattack},
	{'V', TRUE, dohistory},
	{M('v'), TRUE, doextversion},
	{'w', FALSE, dowield},
	{'W', FALSE, dowear},
	{M('w'), FALSE, dowipe},
	{'x', FALSE, doswapweapon},
	{'X', TRUE, enter_explore_mode},
/*	'y', 'Y' : go nw */
	{'z', FALSE, dozap},
	{'Z', TRUE, docast},
	{'<', FALSE, doup},
	{'>', FALSE, dodown},
	{'/', TRUE, dowhatis},
	{'&', TRUE, dowhatdoes},
	{'?', TRUE, dohelp},
	{M('?'), TRUE, doextlist},
#ifdef SHELL
	{'!', TRUE, dosh},
#endif
	{'.', TRUE, dorest, E_J("waiting","休息")},
	{' ', TRUE, dorest, E_J("waiting","休息")},
	{',', FALSE, dopickup},
	{':', TRUE, dolook},
	{';', TRUE, doquickwhatis},
	{'^', TRUE, doidtrap},
	{'\\', TRUE, dodiscovered},		/* Robert Viduya */
	{'@', TRUE, dotogglepickup},
	{M('2'), FALSE, dotwoweapon},
	{WEAPON_SYM,  TRUE, doprwep},
	{ARMOR_SYM,  TRUE, doprarm},
	{RING_SYM,  TRUE, doprring},
	{AMULET_SYM, TRUE, dopramulet},
	{TOOL_SYM, TRUE, doprtool},
	{'*', TRUE, doprinuse},	/* inventory of all equipment in use */
	{GOLD_SYM, TRUE, doprgold},
	{SPBOOK_SYM, TRUE, dovspell},			/* Mike Stephenson */
	{'#', TRUE, doextcmd},
	{'_', TRUE, dotravel},
	{0,0,0,0}
};

struct ext_func_tab extcmdlist[] = {
#ifndef JP
	{"adjust", "adjust inventory letters", doorganize, TRUE},
	{"bereave", "steal items from monsters", playersteal, FALSE},  /* jla */
	{"chat", "talk to someone", dotalk, TRUE},	/* converse? */
	{"conduct", "list which challenges you have adhered to", doconduct, TRUE},
	{"dip", "dip an object into something", dodip, FALSE},
	{"force", "force a lock", doforce, FALSE},
	{"invoke", "invoke an object's powers", doinvoke, TRUE},
	{"jump", "jump to a location", dojump, FALSE},
	{"loot", "loot a box on the floor", doloot, FALSE},
	{"monster", "use a monster's special ability", domonability, TRUE},
	{"name", "name an item or type of object", ddocall, TRUE},
	{"offer", "offer a sacrifice to the gods", dosacrifice, FALSE},
	{"pray", "pray to the gods for help", dopray, TRUE},
	{"quit", "exit without saving current game", done2, TRUE},
#ifdef STEED
	{"ride", "ride (or stop riding) a monster", doride, FALSE},
#endif
	{"rub", "rub a lamp or a stone", dorub, FALSE},
	{"sit", "sit down", dosit, FALSE},
	{"skills", "check weapons skills", show_weapon_skills, TRUE},
	{"turn", "turn undead", doturn, TRUE},
	{"twoweapon", "toggle two-weapon combat", dotwoweapon, FALSE},
	{"untrap", "untrap something", dountrap, FALSE},
	{"version", "list compile time options for this version of NetHack",
		doextversion, TRUE},
	{"wipe", "wipe off your face", dowipe, FALSE},
	{"?", "get this list of extended commands", doextlist, TRUE},
#else
	{"adjust",   "所持品の文字を調整する", doorganize, TRUE},
	{"bereave",  "怪物から品物を盗む", playersteal, FALSE},  /* jla */
	{"chat",     "誰かに話しかける", dotalk, TRUE},	/* converse? */
	{"conduct",  "あなたがこだわる挑戦の一覧を表示する", doconduct, TRUE},
	{"dip",	     "所持品を何かに浸す", dodip, FALSE},
	{"force",    "鍵をこじ開ける", doforce, FALSE},
	{"invoke",   "品物の魔力を解放する", doinvoke, TRUE},
	{"jump",     "指定の地点まで跳躍する", dojump, FALSE},
	{"loot",     "床にある箱・袋をあさる", doloot, FALSE},
	{"monster",  "怪物の特殊能力を使う", domonability, TRUE},
	{"name",     "所持品に個別の名前または識別名をつける", ddocall, TRUE},
	{"offer",    "生贄を神に捧げる", dosacrifice, FALSE},
	{"pray",     "神に助けを請う", dopray, TRUE},
	{"quit",     "今のゲームを放棄する", done2, TRUE},
#ifdef STEED
	{"ride",     "怪物に乗る/降りる", doride, FALSE},
#endif
	{"rub",      "ランプや石をこする", dorub, FALSE},
	{"sit",      "座る", dosit, FALSE},
	{"skills",   "スキルを確認する", show_weapon_skills, TRUE},
	{"turn",     "不死の怪物を祓う", doturn, TRUE},
	{"twoweapon","二刀流を始める/やめる", dotwoweapon, FALSE},
	{"untrap",   "罠を外す", dountrap, FALSE},
	{"version",  "このバージョンのNetHackのコンパイルオプションを確認する",
			doextversion, TRUE},
	{"wipe",     "顔を拭う", dowipe, FALSE},
	{"?",	     "この拡張コマンド一覧を表示する", doextlist, TRUE},
#endif /*JP*/
#if defined(WIZARD)
	/*
	 * There must be a blank entry here for every entry in the table
	 * below.
	 */
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
#ifdef PORT_DEBUG
	{(char *)0, (char *)0, donull, TRUE},
#endif
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
#ifdef DEBUG_MIGRATING_MONS
	{(char *)0, (char *)0, donull, TRUE},
#endif
	{(char *)0, (char *)0, donull, TRUE},
        {(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
#ifdef DEBUG
	{(char *)0, (char *)0, donull, TRUE},
#endif
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
#endif
	{(char *)0, (char *)0, donull, TRUE}	/* sentinel */
};

#if defined(WIZARD)
static const struct ext_func_tab debug_extcmdlist[] = {
	{"levelchange", "change experience level", wiz_level_change, TRUE},
	{"lightsources", "show mobile light sources", wiz_light_sources, TRUE},
#ifdef DEBUG_MIGRATING_MONS
	{"migratemons", "migrate n random monsters", wiz_migrate_mons, TRUE},
#endif
	{"monpolycontrol", "control monster polymorphs", wiz_mon_polycontrol, TRUE},
	{"panic", "test panic routine (fatal to game)", wiz_panic, TRUE},
	{"polyself", "polymorph self", wiz_polyself, TRUE},
#ifdef PORT_DEBUG
	{"portdebug", "wizard port debug command", wiz_port_debug, TRUE},
#endif
	{"seenv", "show seen vectors", wiz_show_seenv, TRUE},
	{"stats", "show memory statistics", wiz_show_stats, TRUE},
	{"timeout", "look at timeout queue", wiz_timeout_queue, TRUE},
	{"vision", "show vision array", wiz_show_vision, TRUE},
#ifdef DEBUG
	{"wizdebug", "wizard debug command", wiz_debug_cmd, TRUE},
#endif
	{"wmode", "show wall modes", wiz_show_wmodes, TRUE},
	{"rrllududab", "gain full stats", wiz_gain_fullstat, TRUE},
	{"roomlist", "show list of rooms", wiz_roomlist, TRUE},
	{"cloudlist", "show list of clouds", wiz_cloud_list, TRUE},
	{"objdesc", "objdesc test", wiz_objdesc, TRUE},
	{(char *)0, (char *)0, donull, TRUE}
};

/*
 * Insert debug commands into the extended command list.  This function
 * assumes that the last entry will be the help entry.
 *
 * You must add entries in ext_func_tab every time you add one to the
 * debug_extcmdlist().
 */
void
add_debug_extended_commands()
{
	int i, j, k, n;

	/* count the # of help entries */
	for (n = 0; extcmdlist[n].ef_txt[0] != '?'; n++)
	    ;

	for (i = 0; debug_extcmdlist[i].ef_txt; i++) {
	    for (j = 0; j < n; j++)
		if (strcmp(debug_extcmdlist[i].ef_txt, extcmdlist[j].ef_txt) < 0) break;

	    /* insert i'th debug entry into extcmdlist[j], pushing down  */
	    for (k = n; k >= j; --k)
		extcmdlist[k+1] = extcmdlist[k];
	    extcmdlist[j] = debug_extcmdlist[i];
	    n++;	/* now an extra entry */
	}
}


static const char template[] = "%-18s %4ld  %6ld";
static const char count_str[] = "                   count  bytes";
static const char separator[] = "------------------ -----  ------";

STATIC_OVL void
count_obj(chain, total_count, total_size, top, recurse)
	struct obj *chain;
	long *total_count;
	long *total_size;
	boolean top;
	boolean recurse;
{
	long count, size;
	struct obj *obj;

	for (count = size = 0, obj = chain; obj; obj = obj->nobj) {
	    if (top) {
		count++;
		size += sizeof(struct obj) + obj->oxlth + obj->onamelth;
	    }
	    if (recurse && obj->cobj)
		count_obj(obj->cobj, total_count, total_size, TRUE, TRUE);
	}
	*total_count += count;
	*total_size += size;
}

STATIC_OVL void
obj_chain(win, src, chain, total_count, total_size)
	winid win;
	const char *src;
	struct obj *chain;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count = 0, size = 0;

	count_obj(chain, &count, &size, TRUE, FALSE);
	*total_count += count;
	*total_size += size;
	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

STATIC_OVL void
mon_invent_chain(win, src, chain, total_count, total_size)
	winid win;
	const char *src;
	struct monst *chain;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count = 0, size = 0;
	struct monst *mon;

	for (mon = chain; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, TRUE, FALSE);
	*total_count += count;
	*total_size += size;
	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

STATIC_OVL void
contained(win, src, total_count, total_size)
	winid win;
	const char *src;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count = 0, size = 0;
	struct monst *mon;

	count_obj(invent, &count, &size, FALSE, TRUE);
	count_obj(fobj, &count, &size, FALSE, TRUE);
	count_obj(level.buriedobjlist, &count, &size, FALSE, TRUE);
	count_obj(migrating_objs, &count, &size, FALSE, TRUE);
	/* DEADMONSTER check not required in this loop since they have no inventory */
	for (mon = fmon; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, FALSE, TRUE);
	for (mon = migrating_mons; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, FALSE, TRUE);

	*total_count += count; *total_size += size;

	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

STATIC_OVL void
mon_chain(win, src, chain, total_count, total_size)
	winid win;
	const char *src;
	struct monst *chain;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count, size;
	struct monst *mon;

	for (count = size = 0, mon = chain; mon; mon = mon->nmon) {
	    count++;
	    size += sizeof(struct monst) + mon->mxlth + mon->mnamelth;
	}
	*total_count += count;
	*total_size += size;
	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

/*
 * Display memory usage of all monsters and objects on the level.
 */
static int
wiz_show_stats()
{
	char buf[BUFSZ];
	winid win;
	long total_obj_size = 0, total_obj_count = 0;
	long total_mon_size = 0, total_mon_count = 0;

	win = create_nhwindow(NHW_TEXT);
	putstr(win, 0, "Current memory statistics:");
	putstr(win, 0, "");
	Sprintf(buf, "Objects, size %d", (int) sizeof(struct obj));
	putstr(win, 0, buf);
	putstr(win, 0, "");
	putstr(win, 0, count_str);

	obj_chain(win, "invent", invent, &total_obj_count, &total_obj_size);
	obj_chain(win, "fobj", fobj, &total_obj_count, &total_obj_size);
	obj_chain(win, "buried", level.buriedobjlist,
				&total_obj_count, &total_obj_size);
	obj_chain(win, "migrating obj", migrating_objs,
				&total_obj_count, &total_obj_size);
	mon_invent_chain(win, "minvent", fmon,
				&total_obj_count,&total_obj_size);
	mon_invent_chain(win, "migrating minvent", migrating_mons,
				&total_obj_count, &total_obj_size);

	contained(win, "contained",
				&total_obj_count, &total_obj_size);

	putstr(win, 0, separator);
	Sprintf(buf, template, "Total", total_obj_count, total_obj_size);
	putstr(win, 0, buf);

	putstr(win, 0, "");
	putstr(win, 0, "");
	Sprintf(buf, "Monsters, size %d", (int) sizeof(struct monst));
	putstr(win, 0, buf);
	putstr(win, 0, "");

	mon_chain(win, "fmon", fmon,
				&total_mon_count, &total_mon_size);
	mon_chain(win, "migrating", migrating_mons,
				&total_mon_count, &total_mon_size);

	putstr(win, 0, separator);
	Sprintf(buf, template, "Total", total_mon_count, total_mon_size);
	putstr(win, 0, buf);

#if defined(__BORLANDC__) && !defined(_WIN32)
	show_borlandc_stats(win);
#endif

	display_nhwindow(win, FALSE);
	destroy_nhwindow(win);
	return 0;
}

void
sanity_check()
{
	obj_sanity_check();
	timer_sanity_check();
}

#ifdef DEBUG_MIGRATING_MONS
static int
wiz_migrate_mons()
{
	int mcount = 0;
	char inbuf[BUFSZ];
	struct permonst *ptr;
	struct monst *mtmp;
	d_level tolevel;
	getlin("How many random monsters to migrate? [0]", inbuf);
	if (*inbuf == '\033') return 0;
	mcount = atoi(inbuf);
	if (mcount < 0 || mcount > (COLNO * ROWNO) || Is_botlevel(&u.uz))
		return 0;
	while (mcount > 0) {
		if (Is_stronghold(&u.uz))
		    assign_level(&tolevel, &valley_level);
		else
		    get_level(&tolevel, depth(&u.uz) + 1);
		ptr = rndmonst();
		mtmp = makemon(ptr, 0, 0, NO_MM_FLAGS);
		if (mtmp) migrate_to_level(mtmp, ledger_no(&tolevel),
				MIGR_RANDOM, (coord *)0);
		mcount--;
	}
	return 0;
}
#endif

#endif /* WIZARD */

#define unctrl(c)	((c) <= C('z') ? (0x60 | (c)) : (c))
#define unmeta(c)	(0x7f & (c))


void
rhack(cmd)
register char *cmd;
{
	boolean do_walk, do_rush, prefix_seen, bad_command,
		firsttime = (cmd == 0);

	iflags.menu_requested = FALSE;
	if (firsttime) {
		flags.nopick = 0;
		cmd = parse();
	}
	if (*cmd == '\033') {
		flags.move = FALSE;
		return;
	}
#ifdef REDO
	if (*cmd == DOAGAIN && !in_doagain && saveq[0]) {
		in_doagain = TRUE;
		stail = 0;
		rhack((char *)0);	/* read and execute command */
		in_doagain = FALSE;
		return;
	}
	/* Special case of *cmd == ' ' handled better below */
	if(!*cmd || *cmd == (char)0377)
#else
	if(!*cmd || *cmd == (char)0377 || (!flags.rest_on_space && *cmd == ' '))
#endif
	{
		nhbell();
		flags.move = FALSE;
		return;		/* probably we just had an interrupt */
	}
	if (iflags.num_pad && iflags.num_pad_mode == 1) {
		/* This handles very old inconsistent DOS/Windows behaviour
		 * in a new way: earlier, the keyboard handler mapped these,
		 * which caused counts to be strange when entered from the
		 * number pad. Now do not map them until here. 
		 */
		switch (*cmd) {
		    case '5':       *cmd = 'g'; break;
		    case M('5'):    *cmd = 'G'; break;
		    case M('0'):    *cmd = 'I'; break;
        	}
        }
	/* handle most movement commands */
	do_walk = do_rush = prefix_seen = FALSE;
	flags.travel = iflags.travel1 = 0;
	switch (*cmd) {
	 case 'g':  if (movecmd(cmd[1])) {
			flags.run = 2;
			do_rush = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case '5':  if (!iflags.num_pad) break;	/* else FALLTHRU */
	 case 'G':  if (movecmd(lowc(cmd[1]))) {
			flags.run = 3;
			do_rush = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case '-':  if (!iflags.num_pad) break;	/* else FALLTHRU */
	/* Effects of movement commands and invisible monsters:
	 * m: always move onto space (even if 'I' remembered)
	 * F: always attack space (even if 'I' not remembered)
	 * normal movement: attack if 'I', move otherwise
	 */
	 case 'F':  if (movecmd(cmd[1])) {
			flags.forcefight = 1;
			do_walk = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case 'm':  if (movecmd(cmd[1]) || u.dz) {
			flags.run = 0;
			flags.nopick = 1;
			if (!u.dz) do_walk = TRUE;
			else cmd[0] = cmd[1];	/* "m<" or "m>" */
		    } else
			prefix_seen = TRUE;
		    break;
	 case 'M':  if (movecmd(lowc(cmd[1]))) {
			flags.run = 1;
			flags.nopick = 1;
			do_rush = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case '0':  if (!iflags.num_pad) break;
		    (void)ddoinv(); /* a convenience borrowed from the PC */
		    flags.move = FALSE;
		    multi = 0;
		    return;
	 case CMD_TRAVEL:
		    if (iflags.travelcmd) {
			    flags.travel = 1;
			    iflags.travel1 = 1;
			    flags.run = 8;
			    flags.nopick = 1;
			    do_rush = TRUE;
			    break;
		    }
		    /*FALLTHRU*/
	 default:   if (movecmd(*cmd)) {	/* ordinary movement */
			flags.run = 0;	/* only matters here if it was 8 */
			do_walk = TRUE;
		    } else if (movecmd(iflags.num_pad ?
				       unmeta(*cmd) : lowc(*cmd))) {
			flags.run = 1;
			do_rush = TRUE;
		    } else if (movecmd(unctrl(*cmd))) {
			flags.run = 3;
			do_rush = TRUE;
		    }
		    break;
	}
	if (Rocketskate && (do_walk || do_rush)
#ifdef STEED
	    && !u.usteed
#endif
		) { /* cursed poton of speed */
		do_rush = prefix_seen = FALSE;
		flags.run = 0;
		flags.travel = 0;
		do_walk = TRUE;
		multi = rnd(6);
	}

	/* some special prefix handling */
	/* overload 'm' prefix for ',' to mean "request a menu" */
	if (prefix_seen && cmd[1] == ',') {
		iflags.menu_requested = TRUE;
		++cmd;
	}

	if (do_walk) {
	    if (multi) flags.mv = TRUE;
	    domove();
	    flags.forcefight = 0;
	    return;
	} else if (do_rush) {
	    if (firsttime) {
		if (!multi) multi = max(COLNO,ROWNO);
		u.last_str_turn = 0;
	    }
	    flags.mv = TRUE;
	    domove();
	    return;
	} else if (prefix_seen && cmd[1] == '\033') {	/* <prefix><escape> */
	    /* don't report "unknown command" for change of heart... */
	    bad_command = FALSE;
	} else if (*cmd == ' ' && !flags.rest_on_space) {
	    bad_command = TRUE;		/* skip cmdlist[] loop */

	/* handle all other commands */
	} else {
	    register const struct func_tab *tlist;
	    int res, NDECL((*func));

	    getobj_override = (struct obj *)0;
	    for (tlist = cmdlist; tlist->f_char; tlist++) {
		if ((*cmd & 0xff) != (tlist->f_char & 0xff)) continue;

		if (u.uburied && !tlist->can_if_buried) {
		    You_cant(E_J("do that while you are buried!",
				 "埋まっているため、それはできない！"));
		    res = 0;
		} else {
		    /* we discard 'const' because some compilers seem to have
		       trouble with the pointer passed to set_occupation() */
		    func = ((struct func_tab *)tlist)->f_funct;
		    if (tlist->f_text && !occupation && multi)
			set_occupation(func, tlist->f_text, multi);
		    res = (*func)();		/* perform the command */
		}
		if (!res) {
		    flags.move = FALSE;
		    multi = 0;
		}
		return;
	    }
	    /* if we reach here, cmd wasn't found in cmdlist[] */
	    bad_command = TRUE;
	}

	if (bad_command) {
	    char expcmd[10];
	    register char *cp = expcmd;

	    while (*cmd && (int)(cp - expcmd) < (int)(sizeof expcmd - 3)) {
		if (*cmd >= 040 && *cmd < 0177) {
		    *cp++ = *cmd++;
		} else if (*cmd & 0200) {
		    *cp++ = 'M';
		    *cp++ = '-';
		    *cp++ = *cmd++ &= ~0200;
		} else {
		    *cp++ = '^';
		    *cp++ = *cmd++ ^ 0100;
		}
	    }
	    *cp = '\0';
	    if (!prefix_seen || !iflags.cmdassist ||
		!help_dir(0, E_J("Invalid direction key!","無効な方向キーです！")))
		Norep(E_J("Unknown command '%s'.","無効なコマンド: '%s'"), expcmd);
	}
	/* didn't move */
	flags.move = FALSE;
	multi = 0;
	return;
}

int
xytod(x, y)	/* convert an x,y pair into a direction code */
schar x, y;
{
	register int dd;

	for(dd = 0; dd < 8; dd++)
	    if(x == xdir[dd] && y == ydir[dd]) return dd;

	return -1;
}

void
dtoxy(cc,dd)	/* convert a direction code into an x,y pair */
coord *cc;
register int dd;
{
	cc->x = xdir[dd];
	cc->y = ydir[dd];
	return;
}

int
movecmd(sym)	/* also sets u.dz, but returns false for <> */
char sym;
{
	register const char *dp;
	register const char *sdp;
	if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

	u.dz = 0;
	if(!(dp = index(sdp, sym))) return 0;
	u.dx = xdir[dp-sdp];
	u.dy = ydir[dp-sdp];
	u.dz = zdir[dp-sdp];
	if (u.dx && u.dy && u.umonnum == PM_GRID_BUG) {
		u.dx = u.dy = 0;
		return 0;
	}
	return !u.dz;
}

/*
 * uses getdir() but unlike getdir() it specifically
 * produces coordinates using the direction from getdir()
 * and verifies that those coordinates are ok.
 *
 * If the call to getdir() returns 0, Never_mind is displayed.
 * If the resulting coordinates are not okay, emsg is displayed.
 *
 * Returns non-zero if coordinates in cc are valid.
 */
int get_adjacent_loc(prompt,emsg,x,y,cc)
const char *prompt, *emsg;
xchar x,y;
coord *cc;
{
	xchar new_x, new_y;
	if (!getdir(prompt)) {
		pline(Never_mind);
		return 0;
	}
	new_x = x + u.dx;
	new_y = y + u.dy;
	if (cc && isok(new_x,new_y)) {
		cc->x = new_x;
		cc->y = new_y;
	} else {
		if (emsg) pline(emsg);
		return 0;
	}
	return 1;
}

int
getdir(s)
const char *s;
{
	char dirsym;

#ifdef REDO
	if(in_doagain || *readchar_queue)
	    dirsym = readchar();
	else
#endif
	    dirsym = yn_function ((s && *s != '^') ? s : E_J("In what direction?","どの方向に？"),
					(char *)0, '\0');
#ifdef REDO
	savech(dirsym);
#endif
	if(dirsym == '.' || dirsym == 's')
		u.dx = u.dy = u.dz = 0;
	else if(!movecmd(dirsym) && !u.dz) {
		boolean did_help = FALSE;
		if(!index(quitchars, dirsym)) {
		    if (iflags.cmdassist) {
			did_help = help_dir((s && *s == '^') ? dirsym : 0,
					E_J("Invalid direction key!","無効な方向キーです！"));
		    }
		    if (!did_help) pline(E_J("What a strange direction!","なんとおかしな方向だ！"));
		}
		return 0;
	}
	if(!u.dz && (Stunned || (Confusion && !rn2(5)))) confdir();
	return 1;
}

/* test */
int
getdir_or_pos(range, tgt, s, goal)
int range, tgt;
const char *s, *goal;
{
	char dirsym;

#ifdef REDO
	if(in_doagain || *readchar_queue)
	    dirsym = readchar();
	else
#endif
	    dirsym = yn_function ((s && *s != '^') ? s :
					E_J("In what direction (or position)?",
					    "どの方向(または位置)に？"),
					(char *)0, '\0');
#ifdef REDO
	savech(dirsym);
#endif
	if(dirsym == '.' || dirsym == 's')
	    u.dx = u.dy = u.dz = 0;
	else if (dirsym == '+' || dirsym == '-') {
	    coord cc;
	    cc.x = u.ux;
	    cc.y = u.uy;
	    if (getpos2(&cc, (range) ? range*range+1 : 0, tgt, goal) < 0)
		return 0;
	    u.dx = cc.x - u.ux;
	    u.dy = cc.y - u.uy;
	} else if(!movecmd(dirsym) && !u.dz) {
	    boolean did_help = FALSE;
	    if(!index(quitchars, dirsym)) {
		if (iflags.cmdassist) {
		    did_help = help_dir((s && *s == '^') ? dirsym : 0,
				E_J("+Invalid direction key!","+無効な方向キーです！"));
		}
		if (!did_help) pline(E_J("What a strange direction!","なんとおかしな方向だ！"));
	    }
	    return 0;
	}
	if(!u.dz && (Stunned || (Confusion && !rn2(5)))) {
	    u.dx = rn2(9) - 4;
	    u.dy = rn2(9) - 4;
	}
	return 1;
}
int getpos2test(void) {
	int r;
//	r = getdir_or_pos(/*120*/0, GETPOS_MON|GETPOS_OBJ|GETPOS_TRAP, 0, "aim");
//	if (r == 0) pline("Cancelled.");
//	else pline("(dx,dy) = (%d,%d)", u.dx, u.dy);
if (levl[u.ux][u.uy].roomno >= ROOMOFFSET)
pline("Roomno:%d, Type:%d",levl[u.ux][u.uy].roomno - ROOMOFFSET, rooms[levl[u.ux][u.uy].roomno - ROOMOFFSET].rtype);
else
pline("Not in a room.");
	return 0;
}
/* test */

STATIC_OVL boolean
help_dir(sym, msg)
char sym;
const char *msg;
{
	char ctrl;
	winid win;
	static const char wiz_only_list[] = "EFGIOVW";
	char buf[BUFSZ], buf2[BUFSZ], *expl;
	boolean dir_or_pos = FALSE;

	win = create_nhwindow(NHW_TEXT);
	if (!win) return FALSE;
	if (msg) {
		if (*msg == '+') {
		    dir_or_pos = TRUE;
		    msg++;
		}
		Sprintf(buf, E_J("cmdassist: %s","操作のヘルプ: %s"), msg);
		putstr(win, 0, buf);
		putstr(win, 0, "");
	}
	if (letter(sym)) { 
	    sym = highc(sym);
	    ctrl = (sym - 'A') + 1;
	    if ((expl = dowhatdoes_core(ctrl, buf2))
		&& (!index(wiz_only_list, sym)
#ifdef WIZARD
		    || wizard
#endif
	                     )) {
#ifndef JP
		Sprintf(buf, "Are you trying to use ^%c%s?", sym,
			index(wiz_only_list, sym) ? "" :
			" as specified in the Guidebook");
#else
		Sprintf(buf, "%s ^%c コマンドを入力しようとしていますか？",
			index(wiz_only_list, sym) ? "" : "ガイドブックに載っている",
			sym);
#endif /*JP*/
		putstr(win, 0, buf);
		putstr(win, 0, "");
		putstr(win, 0, expl);
		putstr(win, 0, "");
#ifndef JP
		putstr(win, 0, "To use that command, you press");
		Sprintf(buf,
			"the <Ctrl> key, and the <%c> key at the same time.", sym);
#else
		Sprintf(buf,
			"このコマンドを使うには、<Ctrl>キーを押しながら<%c>キーを押してください。", sym);
#endif /*JP*/
		putstr(win, 0, buf);
		putstr(win, 0, "");
	    }
	}
#ifndef JP
	if (iflags.num_pad && u.umonnum == PM_GRID_BUG) {
	    putstr(win, 0, "Valid direction keys in your current form (with number_pad on) are:");
	    putstr(win, 0, "             8   ");
	    putstr(win, 0, "             |   ");
	    putstr(win, 0, "          4- . -6");
	    putstr(win, 0, "             |   ");
	    putstr(win, 0, "             2   ");
	} else if (u.umonnum == PM_GRID_BUG) {
	    putstr(win, 0, "Valid direction keys in your current form are:");
	    putstr(win, 0, "             k   ");
	    putstr(win, 0, "             |   ");
	    putstr(win, 0, "          h- . -l");
	    putstr(win, 0, "             |   ");
	    putstr(win, 0, "             j   ");
	} else if (iflags.num_pad) {
	    putstr(win, 0, "Valid direction keys (with number_pad on) are:");
	    putstr(win, 0, "          7  8  9");
	    putstr(win, 0, "           \\ | / ");
	    putstr(win, 0, "          4- . -6");
	    putstr(win, 0, "           / | \\ ");
	    putstr(win, 0, "          1  2  3");
	} else {
	    putstr(win, 0, "Valid direction keys are:");
	    putstr(win, 0, "          y  k  u");
	    putstr(win, 0, "           \\ | / ");
	    putstr(win, 0, "          h- . -l");
	    putstr(win, 0, "           / | \\ ");
	    putstr(win, 0, "          b  j  n");
	};
	putstr(win, 0, "");
	putstr(win, 0, "          <  up");
	putstr(win, 0, "          >  down");
	putstr(win, 0, "          .  direct at yourself");
	if (dir_or_pos)
	    putstr(win, 0, "          +  directly aim the target by the cursor");
	putstr(win, 0, "");
	putstr(win, 0, "(Suppress this message with !cmdassist in config file.)");
#else /*JP*/
	if (iflags.num_pad && u.umonnum == PM_GRID_BUG) {
	    putstr(win, 0, "現在の姿において有効な方向キー(number_padオプションがonのとき):");
	    putstr(win, 0, "            8    ");
	    putstr(win, 0, "            |    ");
	    putstr(win, 0, "        4─ . ─6");
	    putstr(win, 0, "            |    ");
	    putstr(win, 0, "            2    ");
	} else if (u.umonnum == PM_GRID_BUG) {
	    putstr(win, 0, "現在の姿において有効な方向キー:");
	    putstr(win, 0, "            k    ");
	    putstr(win, 0, "            |    ");
	    putstr(win, 0, "        h─ . ─l");
	    putstr(win, 0, "            |    ");
	    putstr(win, 0, "            j    ");
	} else if (iflags.num_pad) {
	    putstr(win, 0, "有効な方向キー(number_padオプションがonのとき):");
	    putstr(win, 0, "        7   8   9");
	    putstr(win, 0, "         ＼ | ／ ");
	    putstr(win, 0, "        4─ . ─6");
	    putstr(win, 0, "         ／ | ＼ ");
	    putstr(win, 0, "        1   2   3");
	} else {
	    putstr(win, 0, "有効な方向キー:");
	    putstr(win, 0, "        y   k   u");
	    putstr(win, 0, "         ＼ | ／ ");
	    putstr(win, 0, "        h─ . ─l");
	    putstr(win, 0, "         ／ | ＼ ");
	    putstr(win, 0, "        b   j   n");
	};
	putstr(win, 0, "");
	putstr(win, 0, "          <  上方向");
	putstr(win, 0, "          >  下方向");
	putstr(win, 0, "          .  あなた自身を狙う");
	if (dir_or_pos)
	    putstr(win, 0, "          +  対象をカーソルで直接指定する");
	putstr(win, 0, "");
	putstr(win, 0, "(ヘルプが不要の場合は !cmdassist と設定ファイルに記述してください)");
#endif /*JP*/
	display_nhwindow(win, FALSE);
	destroy_nhwindow(win);
	return TRUE;
}

#endif /* OVL1 */
#ifdef OVLB

void
confdir()
{
	register int x = (u.umonnum == PM_GRID_BUG) ? 2*rn2(4) : rn2(8);
	u.dx = xdir[x];
	u.dy = ydir[x];
	return;
}

#endif /* OVLB */
#ifdef OVL0

int
isok(x,y)
register int x, y;
{
	/* x corresponds to curx, so x==1 is the first column. Ach. %% */
	return x >= 1 && x <= COLNO-1 && y >= 0 && y <= ROWNO-1;
}

static NEARDATA int last_multi;

/*
 * convert a MAP window position into a movecmd
 */
const char *
click_to_cmd(x, y, mod)
    int x, y, mod;
{
    int dir;
    static char cmd[4];
    cmd[1]=0;

    x -= u.ux;
    y -= u.uy;

    if (iflags.travelcmd) {
        if (abs(x) <= 1 && abs(y) <= 1 ) {
            x = sgn(x), y = sgn(y);
        } else {
            u.tx = u.ux+x;
            u.ty = u.uy+y;
            cmd[0] = CMD_TRAVEL;
            return cmd;
        }

        if(x == 0 && y == 0) {
            /* here */
            if(IS_FOUNTAIN(levl[u.ux][u.uy].typ) || IS_SINK(levl[u.ux][u.uy].typ)) {
                cmd[0]=mod == CLICK_1 ? 'q' : M('d');
                return cmd;
            } else if(IS_THRONE(levl[u.ux][u.uy].typ)) {
                cmd[0]=M('s');
                return cmd;
            } else if((u.ux == xupstair && u.uy == yupstair)
                      || (u.ux == sstairs.sx && u.uy == sstairs.sy && sstairs.up)
                      || (u.ux == xupladder && u.uy == yupladder)) {
                return "<";
            } else if((u.ux == xdnstair && u.uy == ydnstair)
                      || (u.ux == sstairs.sx && u.uy == sstairs.sy && !sstairs.up)
                      || (u.ux == xdnladder && u.uy == ydnladder)) {
                return ">";
            } else if(OBJ_AT(u.ux, u.uy)) {
                cmd[0] = Is_container(level.objects[u.ux][u.uy]) ? M('l') : ',';
                return cmd;
            } else {
                return "."; /* just rest */
            }
        }

        /* directional commands */

        dir = xytod(x, y);

	if (!m_at(u.ux+x, u.uy+y) && !test_move(u.ux, u.uy, x, y, TEST_MOVE)) {
            cmd[1] = (iflags.num_pad ? ndir[dir] : sdir[dir]);
            cmd[2] = 0;
            if (IS_DOOR(levl[u.ux+x][u.uy+y].typ)) {
                /* slight assistance to the player: choose kick/open for them */
                if (levl[u.ux+x][u.uy+y].doormask & D_LOCKED) {
                    cmd[0] = C('d');
                    return cmd;
                }
                if (levl[u.ux+x][u.uy+y].doormask & D_CLOSED) {
                    cmd[0] = 'o';
                    return cmd;
                }
            }
            if (levl[u.ux+x][u.uy+y].typ <= SCORR) {
                cmd[0] = 's';
                cmd[1] = 0;
                return cmd;
            }
        }
    } else {
        /* convert without using floating point, allowing sloppy clicking */
        if(x > 2*abs(y))
            x = 1, y = 0;
        else if(y > 2*abs(x))
            x = 0, y = 1;
        else if(x < -2*abs(y))
            x = -1, y = 0;
        else if(y < -2*abs(x))
            x = 0, y = -1;
        else
            x = sgn(x), y = sgn(y);

        if(x == 0 && y == 0)	/* map click on player to "rest" command */
            return ".";

        dir = xytod(x, y);
    }

    /* move, attack, etc. */
    cmd[1] = 0;
    if(mod == CLICK_1) {
	cmd[0] = (iflags.num_pad ? ndir[dir] : sdir[dir]);
    } else {
	cmd[0] = (iflags.num_pad ? M(ndir[dir]) :
		(sdir[dir] - 'a' + 'A')); /* run command */
    }

    return cmd;
}

STATIC_OVL char *
parse()
{
#ifdef LINT	/* static char in_line[COLNO]; */
	char in_line[COLNO];
#else
	static char in_line[COLNO];
#endif
	register int foo;
	boolean prezero = FALSE;

	multi = 0;
	flags.move = 1;
	flush_screen(1); /* Flush screen buffer. Put the cursor on the hero. */

	if (!iflags.num_pad || (foo = readchar()) == 'n')
	    for (;;) {
		foo = readchar();
		if (foo >= '0' && foo <= '9') {
		    multi = 10 * multi + foo - '0';
		    if (multi < 0 || multi >= LARGEST_INT) multi = LARGEST_INT;
		    if (multi > 9) {
			clear_nhwindow(WIN_MESSAGE);
			Sprintf(in_line, "Count: %d", multi);
			pline(in_line);
			mark_synch();
		    }
		    last_multi = multi;
		    if (!multi && foo == '0') prezero = TRUE;
		} else break;	/* not a digit */
	    }

	if (foo == '\033') {   /* esc cancels count (TH) */
	    clear_nhwindow(WIN_MESSAGE);
	    multi = last_multi = 0;
# ifdef REDO
	} else if (foo == DOAGAIN || in_doagain) {
	    multi = last_multi;
	} else {
	    last_multi = multi;
	    savech(0);	/* reset input queue */
	    savech((char)foo);
# endif
	}

	if (multi) {
	    multi--;
	    save_cm = in_line;
	} else {
	    save_cm = (char *)0;
	}
	in_line[0] = foo;
	in_line[1] = '\0';
	if (foo == 'g' || foo == 'G' || foo == 'm' || foo == 'M' ||
	    foo == 'F' || (iflags.num_pad && (foo == '5' || foo == '-'))) {
	    foo = readchar();
#ifdef REDO
	    savech((char)foo);
#endif
	    in_line[1] = foo;
	    in_line[2] = 0;
	}
	clear_nhwindow(WIN_MESSAGE);
	if (prezero) in_line[0] = '\033';
	return(in_line);
}

#endif /* OVL0 */
#ifdef OVLB

#ifdef UNIX
static
void
end_of_input()
{
#ifndef NOSAVEONHANGUP
	if (!program_state.done_hup++ && program_state.something_worth_saving)
	    (void) dosave0();
#endif
	exit_nhwindows((char *)0);
	clearlocks();
	terminate(EXIT_SUCCESS);
}
#endif

#endif /* OVLB */
#ifdef OVL0

char
readchar()
{
	register int sym;
	int x = u.ux, y = u.uy, mod = 0;

	if ( *readchar_queue )
	    sym = *readchar_queue++;
	else
#ifdef REDO
	    sym = in_doagain ? Getchar() : nh_poskey(&x, &y, &mod);
#else
	    sym = Getchar();
#endif

#ifdef UNIX
# ifdef NR_OF_EOFS
	if (sym == EOF) {
	    register int cnt = NR_OF_EOFS;
	  /*
	   * Some SYSV systems seem to return EOFs for various reasons
	   * (?like when one hits break or for interrupted systemcalls?),
	   * and we must see several before we quit.
	   */
	    do {
		clearerr(stdin);	/* omit if clearerr is undefined */
		sym = Getchar();
	    } while (--cnt && sym == EOF);
	}
# endif /* NR_OF_EOFS */
	if (sym == EOF)
	    end_of_input();
#endif /* UNIX */

	if(sym == 0) {
	    /* click event */
	    readchar_queue = click_to_cmd(x, y, mod);
	    sym = *readchar_queue++;
	}
	return((char) sym);
}

STATIC_PTR int
dotravel()
{
	/* Keyboard travel command */
	static char cmd[2];
	coord cc;

	if (!iflags.travelcmd) return 0;
	cmd[1]=0;
	cc.x = iflags.travelcc.x;
	cc.y = iflags.travelcc.y;
	if (cc.x == -1 && cc.y == -1) {
	    /* No cached destination, start attempt from current position */
	    cc.x = u.ux;
	    cc.y = u.uy;
	}
	pline(E_J("Where do you want to travel to?",
		  "どの地点まで移動しますか？"));
	if (getpos(&cc, TRUE, E_J("the desired destination","目的地")) < 0) {
		/* user pressed ESC */
		return 0;
	}
	iflags.travelcc.x = u.tx = cc.x;
	iflags.travelcc.y = u.ty = cc.y;
	cmd[0] = CMD_TRAVEL;
	readchar_queue = cmd;
	return 0;
}

#ifdef PORT_DEBUG
# ifdef WIN32CON
extern void NDECL(win32con_debug_keystrokes);
extern void NDECL(win32con_handler_info);
# endif

int
wiz_port_debug()
{
	int n, k;
	winid win;
	anything any;
	int item = 'a';
	int num_menu_selections;
	struct menu_selection_struct {
		char *menutext;
		void NDECL((*fn));
	} menu_selections[] = {
#ifdef WIN32CON
		{"test win32 keystrokes", win32con_debug_keystrokes},
		{"show keystroke handler information", win32con_handler_info},
#endif
		{(char *)0, (void NDECL((*)))0}		/* array terminator */
	};

	num_menu_selections = SIZE(menu_selections) - 1;
	if (num_menu_selections > 0) {
		menu_item *pick_list;
		win = create_nhwindow(NHW_MENU);
		start_menu(win);
		for (k=0; k < num_menu_selections; ++k) {
			any.a_int = k+1;
			add_menu(win, NO_GLYPH, &any, item++, 0, ATR_NONE,
				menu_selections[k].menutext, MENU_UNSELECTED);
		}
		end_menu(win, "Which port debugging feature?");
		n = select_menu(win, PICK_ONE, &pick_list);
		destroy_nhwindow(win);
		if (n > 0) {
			n = pick_list[0].item.a_int - 1;
			free((genericptr_t) pick_list);
			/* execute the function */
			(*menu_selections[n].fn)();
		}
	} else
		pline("No port-specific debug capability defined.");
	return 0;
}
# endif /*PORT_DEBUG*/

#endif /* OVL0 */
#ifdef OVLB
/*
 *   Parameter validator for generic yes/no function to prevent
 *   the core from sending too long a prompt string to the
 *   window port causing a buffer overflow there.
 */
char
yn_function(query,resp, def)
const char *query,*resp;
char def;
{
	char qbuf[QBUFSZ];
	unsigned truncspot, reduction = sizeof(" [N]  ?") + 1;

	if (resp) reduction += strlen(resp) + sizeof(" () ");
	if (strlen(query) < (QBUFSZ - reduction))
		return (*windowprocs.win_yn_function)(query, resp, def);
	paniclog("Query truncated: ", query);
	reduction += sizeof("...");
	truncspot = QBUFSZ - reduction;
	(void) strncpy(qbuf, query, (int)truncspot);
	qbuf[truncspot] = '\0';
	Strcat(qbuf,"...");
	return (*windowprocs.win_yn_function)(qbuf, resp, def);
}
#endif


/* versatile supplemental weapon attack command */
STATIC_PTR int
dovattack()
{
	/* autothrust */
	if (!uwep) {
	    if (uquiver) return dovfire();
	    You(E_J("are not wielding a weapon.",
		    "武器を構えていない。"));
	    return 0;
	}
	if (is_ranged(uwep) ||
	    (u.twoweap && uswapwep && is_ranged(uswapwep))) return autothrust();
	if (is_launcher(uwep)) {
	    if (is_gun(uwep)) return dovfire();
	    if (!uquiver || !ammo_and_launcher(uquiver, uwep))
		 return dofire();
	    return dovfire();
	}
	if (uquiver) return dovfire();

	pline(E_J("Your wielding weapon has no other use than a melee combat.",
		  "装備中の武器には、接近戦以外の用途はない。"));
	return 0;
}

//#ifdef AUTOTHRUST
struct monst *
findtarget(x, y)
int x, y;
{
	struct monst *mtmp;
	if (!isok(x, y)) return (struct monst *)0;
	mtmp = m_at(x, y);
	if (couldsee(x, y) &&			/* your weapon should not be blocked */
	    mtmp && (canspotmons(mtmp) || mon_warning(mtmp)) &&	/* you can see the target */
	    !(mtmp->m_ap_type && !Protection_from_shape_changers &&
	     !Warning &&
	     !sensemon(mtmp)) &&		/* do not count mimics */
	    (((Confusion || Stunned) && !rn2(3)) ||
	     (Hallucination && mtmp) ||
	     (!mtmp->mpeaceful && !mtmp->mtame))) {
		/* set u.dx,u.dy: attack() needs them */
		u.dx = x - u.ux;
		u.dy = y - u.uy;
		u.dz = 0;
		return mtmp;
	}
	return (struct monst *)0;
}

struct monst *
autotarget_near(rangelimit)
int rangelimit;
{
	int x = u.ux, y = u.uy;
	int dx, dy;
	struct monst *mtmp;

	for (dx = 0; dx < 4; dx++ ) {
	    for (dy = 0; dy <= dx; dy++ ) {
		if ((dx*dx + dy*dy) > rangelimit) break;
		if (            (mtmp = findtarget(x+dx, y+dy))) return mtmp;
		if (dx &&       (mtmp = findtarget(x-dx, y+dy))) return mtmp;
		if (      dy && (mtmp = findtarget(x+dx, y-dy))) return mtmp;
		if (dx && dy && (mtmp = findtarget(x-dx, y-dy))) return mtmp;
		if (dx == dy) continue;
		if (            (mtmp = findtarget(x+dy, y+dx))) return mtmp;
		if (dy &&       (mtmp = findtarget(x-dy, y+dx))) return mtmp;
		if (      dx && (mtmp = findtarget(x+dy, y-dx))) return mtmp;
		if (dy && dx && (mtmp = findtarget(x-dy, y-dx))) return mtmp;
	    }
	}
	return mtmp;
}

/* aim the last (or nearest) target and
   set the direction u.dx, u.dy
   return FALSE if nothing is found */
boolean
autotarget()
{
	if (u.ulasttgt)		/* if last target is alive and in sight, aim it */
	    u.ulasttgt = findtarget(u.ulasttgt->mx, u.ulasttgt->my);
	if (!u.ulasttgt) {	/* else find the nearest target */
	    coord cc;
	    cc.x = u.ux;
	    cc.y = u.uy;
	    if (!getnearestpos(&cc, 0, GETPOS_MONTGT))
		return FALSE;
	    u.ulasttgt = m_at(cc.x, cc.y);
	}
	if (u.ulasttgt) {
	    u.dx = u.ulasttgt->mx - u.ux;
	    u.dy = u.ulasttgt->my - u.uy;
	    u.dz = 0;
	} else { /* may be confused or stunned */
	    u.dx = rn2(9) - 4;
	    u.dy = rn2(9) - 4;
	    if (!u.dx && !u.dy) u.dz = -1;
	}
	return TRUE;
}

int
autothrust()
{
	int x, y;
	int r1, r2;
	struct monst *mtmp;

	r1 = r2 = 0;
	if (uwep && is_ranged(uwep))
	    r1 = weapon_range(uwep);
	if (u.twoweap && uswapwep && is_ranged(uswapwep))
	    r2 = weapon_range(uswapwep);

	if (!r1 && !r2) {
		You("don't wield any long range weapon.");
		return FALSE;
	}
	if (u.uswallow) {
		u.dx = u.dy = u.dz = 0;
		if (u.ustuck) attack(u.ustuck);
		return TRUE;
	}
	/* two-weapon combat with different ranged weapons is
	   too complicated, thus I limit the range to
	   the closer one */
	if (!r1) r1 = r2;
	if (!r2) r2 = r1;
	if (r1 > r2) r1 = r2;

	if (u.ulasttgt) {		/* if last target is alive and in range, attack it */
		x = u.ulasttgt->mx;
		y = u.ulasttgt->my;
		mtmp = findtarget(x, y);
		if (mtmp && distu(x, y) <= r1) {
#ifdef MONSTEED
			mtmp = target_rider_or_steed(&youmonst, mtmp);
#endif /*MONSTEED*/
			u.dx = x - u.ux;
			u.dy = y - u.uy;
			u.dz = 0;
			(void) attack(mtmp);
			return TRUE;
		}
	}						/* else find another target */
	if (mtmp = autotarget_near(r1)) {
#ifdef MONSTEED
		mtmp = target_rider_or_steed(&youmonst, mtmp);
#endif /*MONSTEED*/
		(void) attack(mtmp);
	} else {
		if (Confusion || Stunned) {
#ifndef JP
			You("%s your weapon wildly into thin air.",
				objects[uwep->otyp].oc_dir ? "thrust" : "swing");
#else
			You("自分の武器を虚空に向かって%s。",
				objects[uwep->otyp].oc_dir ? "突き出した" : "振り回した");
#endif /*JP*/
			return TRUE;
		}
		E_J(You("don't see any target in range."),
		    pline("射程距離内に目標はいない。"));
		return FALSE;
	}
	return TRUE;
}
//#endif

int
getobj_filter_healer_special_power(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	if (otyp == BANDAGE)
		return GETOBJ_CHOOSEIT;
	return 0;
}

STATIC_PTR int
playersteal()
{
	int x, y;
	int chanch, base, dexadj, intadj, statbonus;
	long gstolen = 0;
	struct monst *mtmp, *mwatch;
	boolean no_steal = FALSE;
	boolean found = FALSE;
	boolean failed = FALSE;
	char *verb = E_J("steal","盗む");

	if (nohands(youmonst.data)) {
	    pline(E_J("Could be hard without hands ...",
		      "手がなければ困難だろう…。"));
	    no_steal = TRUE;
	} else if (near_capacity() > SLT_ENCUMBER) {
	    Your(E_J("load is too heavy to attempt to steal.",
		     "荷物は盗みを試みるには重すぎる。"));
	    no_steal = TRUE;
	} else if (Blinded) {
	    E_J(You("must be able to see to steal."),
		pline("盗むには目が見えていなければならない。"));
	    no_steal = TRUE;
	}
	if (no_steal) {
	    /* discard direction typeahead, if any */
	    display_nhwindow(WIN_MESSAGE, TRUE);    /* --More-- */
	    return 0;
	}

	if(!getdir(NULL)) return(0);
	if(!u.dx && !u.dy) return(0);

	x = u.ux + u.dx;
	y = u.uy + u.dy;

	if(u.uswallow) {
	    You(E_J("search around but don't find anything.",
		    "周囲を調べたが、何も見つけられなかった。"));
	    return(1);
	}

	u_wipe_engr(2);

	if(MON_AT(x, y)) {
	    mtmp = m_at(x, y);

	    if (Role_if(PM_KNIGHT)) {
			You_feel(E_J("like a common thief.",
				     "自分がいわゆる泥棒であることに気づいた。"));
			adjalign(-sgn(u.ualign.type));
	    }

	    if (!mtmp->mpeaceful &&
		(!mtmp->mcansee || !u.uundetected ||
		 (mtmp->mux == u.ux && mtmp->muy == u.uy)))
			verb = E_J("rob","奪う");

	    /* calculate chanch of sucess */
	    if (Role_if(PM_ROGUE)) {
			base = 5 + (u.ulevel * 2);
			dexadj = 3;
			intadj = 15;
	    } else {
			base = 5;
			dexadj = 1;
			intadj = 3;
	    }        
		 if (ACURR(A_DEX) < 10) statbonus = (ACURR(A_DEX) - 10) * dexadj;
	    else if (ACURR(A_DEX) > 14) statbonus = (ACURR(A_DEX) - 14) * dexadj;

		 if (ACURR(A_INT) < 10) statbonus += (ACURR(A_INT) - 10) * intadj / 10;
	    else if (ACURR(A_INT) > 14) statbonus += (ACURR(A_INT) - 14) * intadj / 10;

	    chanch = base + statbonus;

	    if (uarmg && !uarmg->otyp == GAUNTLETS_OF_DEXTERITY) 
			chanch -= 5;
	    if (!uarmg) chanch += 5;
	    if (uarms)	chanch -= 10;
	    if (!uarm || weight(uarm) < 75) chanch += 10;
	    else chanch += ((175 - weight(uarm)) / 10);
	    if (chanch < 5) chanch = 5;
	    if (chanch > 95) chanch = 95;
#ifdef WIZARD
	    if (wizard) pline("[%d%]",chanch);
#endif /*WIZARD*/
	    if (rnd(100) < chanch || mtmp->mtame) {
		struct obj *otmp, *otmp2;
#ifndef GOLDOBJ
		/* temporalily change mgold to obj */
		if (mtmp->mgold) {
		    otmp = mksobj(GOLD_PIECE, FALSE, FALSE);
		    otmp->quan = mtmp->mgold;
		    add_to_minv(mtmp, otmp);
		    mtmp->mgold = 0;
		}
#endif
		if (!mtmp->minvent) {
		    You(E_J("don't find anything to %s.",
			    "%sものを何も見つけられなかった。"), verb);
		    goto cleanup;
		}
		otmp = display_minventory(mtmp, MINV_ALL,
			E_J("Which item do you want to steal?","どの品物を盗みますか？"));
		if (!otmp) {
		    You(E_J("%s nothing.","%sのをやめた。"), verb);
		    goto cleanup;
		} else {
		    int ch;
		    int owt = weight(otmp);
		    if (otmp->owornmask) owt *= 2;
		    if (otmp->oclass == COIN_CLASS) owt *= 2; /* balance... */
		    if (otmp->oclass == ARMOR_CLASS && objects[otmp->otyp].oc_delay)
			owt *= objects[otmp->otyp].oc_delay;
		    if (((otmp->owornmask & W_ARM) && (mtmp->misc_worn_check & W_ARMC))
#ifdef TOURIST
			|| ((otmp->owornmask & W_ARMU) && (mtmp->misc_worn_check & (W_ARMC|W_ARM)))
#endif
		       ) failed = TRUE;
		    ch = rn2(200);
#ifdef WIZARD
		    if (wizard) pline("[%d/%d]",owt,ch);
#endif /*WIZARD*/
		    if (owt > 190) owt = 190;
		    if (ch < owt*2 ||
			(!mtmp->mpeaceful && !mtmp->msleeping &&
			 !mtmp->mfrozen && mtmp->mcanmove)) found = TRUE;
		    if (ch < owt) failed = TRUE;
		    if (failed) {
			You(E_J("fail to %s it.","%sのに失敗した。"), verb);
		    } else {
			obj_extract_self(otmp);
			if (otmp->owornmask) {
			    mtmp->misc_worn_check &= ~otmp->owornmask;
			    if (otmp->owornmask & W_WEP)
				setmnotwielded(mtmp,otmp);
			    otmp->owornmask = 0L;
			    update_mon_intrinsics(mtmp, otmp, FALSE, FALSE);
			    possibly_unwield(mtmp, FALSE);
			}
#ifndef GOLDOBJ
			if (otmp->oclass == COIN_CLASS) {
			    u.ugold += otmp->quan;
			    gstolen = otmp->quan;
			    obfree(otmp, (struct obj *)0);
#ifndef JP
			    You("%s %d gold piece%s from %s.", verb, gstolen,
				(gstolen == 1) ? "" : "s", mon_nam(mtmp));
#else
			    You("%sから金貨%d枚を%sことに成功した。",
				mon_nam(mtmp), gstolen, verb);
#endif /*JP*/
			} else /* continues to the next block */
#endif
			{
			    static const char *stealmsg = E_J("%s %s from %s%s",
							      "%sから%sを%sことに成功した%s");
			    gstolen = getprice(otmp, FALSE);
			    otmp = hold_another_object(otmp, (const char *)0,
					(const char *)0, (const char *)0);
#ifndef JP
			    You(stealmsg, verb, doname(otmp), mon_nam(mtmp),
				(otmp->where == OBJ_INVENT) ? "." : ", but cannot hold it!");
#else
			    You(stealmsg, mon_nam(mtmp), doname(otmp), verb,
				(otmp->where == OBJ_INVENT) ? "。" : "が、とり落としてしまった！");
#endif /*JP*/
			    if (otmp->where != OBJ_INVENT)
				found = TRUE;
			}
		    }
		}
cleanup:
#ifndef GOLDOBJ
		/* restore temporal goldobj to mgold */
		for (otmp = mtmp->minvent; otmp; otmp = otmp2) {
		    otmp2 = otmp->nobj;
		    if (otmp->oclass == COIN_CLASS) {
			obj_extract_self(otmp);
			mtmp->mgold += otmp->quan;
			obfree(otmp, (struct obj *)0);
		    }
		}
#endif
		if (!found && !failed) exercise(A_DEX, TRUE);
	    } else {
		You(E_J("fail to %s anything.","何も%sことができなかった。"), verb);
		if (mtmp->isshk || rn2(3)) found = TRUE;
	    }
	    if (found) {
		boolean waspeaceful = mtmp->mpeaceful;
		wakeup(mtmp);
		if (mtmp->isshk) {
		    if (gstolen > 1000)
			ESHK(mtmp)->robbed += gstolen;
		    if (waspeaceful && in_town(u.ux+u.dx, u.uy+u.dy)) {
			pline(E_J("%s yells:","%sは叫んだ:"), shkname(mtmp));
			verbalize(E_J("Guards! Guards!","衛兵！ 衛兵！"));
			(void) angry_guards(!(flags.soundok));
		    }
		} else if (mtmp->data == &mons[PM_WATCHMAN] ||
			   mtmp->data == &mons[PM_WATCH_CAPTAIN])
			(void) angry_guards(!(flags.soundok));
	    }
	    if (!found && in_town(u.ux+u.dx, u.uy+u.dy) &&
		mtmp->mpeaceful && !mtmp->mtame) {
		for(mwatch = fmon; mwatch; mwatch = mwatch->nmon) {
		    if (!DEADMONSTER(mwatch) &&
			!mwatch->msleeping && !mwatch->mfrozen &&
			mwatch->mcanmove &&
			(mwatch->data == &mons[PM_WATCHMAN] ||
			 mwatch->data == &mons[PM_WATCH_CAPTAIN]) &&
			cansee(mwatch->mx, mwatch->my) &&
			(rnd(100) >= chanch)) {
			    pline(E_J("%s yells:","%sは叫んだ:"), Amonnam(mwatch));
			    verbalize(E_J("Halt, thief!  You're under arrest!",
					  "動くな、泥棒め！ お前を逮捕する！"));
			    (void) angry_guards(!(flags.soundok));
			    setmangry(mtmp);
			    break;
		    }
		}
	    }
	} else { 
	    pline(E_J("I don't see anybody to rob there!",
		      "そこには盗む相手が誰もいない！"));
	    return(0);
	}
	return(1);
}

int
specialpower()      /* Special class abilites */
{
	/* 
	 * STEPHEN WHITE'S NEW CODE
	 *
	 * For clarification, lastuse (as declared in decl.{c|h}) is the
	 * actual length of time the power is active, nextuse is when you can
	 * next use the ability.
	 */
#ifdef WIZARD
	if (wizard && u.unextuse) {
	    if (yn("Force the ability to succeed?") == 'y')
		u.unextuse = 0;
	}
#endif
	
	if (u.unextuse) {
#ifndef JP
	    You("have to wait %s before using you ability again.",
		(u.unextuse > 500) ? "for a while" : "a little longer");
#else
	    pline("再び能力が使えるまで、あなたは%s待たねばならない。",
		(u.unextuse > 500) ? "しばらく" : "少し");
#endif /*JP*/
	    return(0);
	} else switch (Role_switch) {
	    case PM_CAVEMAN:
	    case PM_MONK:
	    case PM_PRIEST:
	    case PM_ROGUE:
#ifdef TOURIST
	    case PM_TOURIST:
#endif
		You(E_J("don't have a special ability!",
			"特殊能力を持っていない！"));
		break;

	    case PM_ARCHEOLOGIST: {
#ifndef JP
		const char *a = "archeological experience ";
		if (findit()) Your("%ssenses secret things.", a);
		else Your("%stells nothing special is around here.", a);
#else
		const char *a = "考古学調査の経験";
		if (findit()) pline("%sから、あなたは隠されたものに気づいた。", a);
		else pline("%sから、あなたは周囲に特別なものがないことを確認した。", a);
#endif /*JP*/
		}
		u.unextuse = rn1(250,250);
		break;

	    case PM_BARBARIAN: 
		You(E_J("fly into a beseark rage!",
			"狂戦士の怒りを呼びさました！"));
		u.ulastuse = d(2,8) + u.ulevel;
		incr_itimeout(&HFast, u.ulastuse);
		u.unextuse = rn1(1000,500);
		return(0);
		break;

	    case PM_WIZARD: 
		if(Hallucination || Stunned || Confusion) {
		    You(E_J("don't quite feal up to studing right now.",
			    "今は調査などをする気になれない。"));
		    break;
		} else if((ACURR(A_INT) + ACURR(A_WIS)) < rnd(60)) {
		    You(E_J("try to study but get distracted, try again later.",
			    "調査を行おうとしたが、集中できなかった。また後ほど試せ。"));
		    u.unextuse = rn1(500,500);
		    break;
		} else if(invent) {
		    You(E_J("start to study.","調査を開始した。"));
		    identify_pack(1);
		} else {
		    pline(E_J("Identify what?","何を識別するって？"));
		    break;
		}
		u.unextuse = rn1(500,1500);
		break;

	    case PM_RANGER: 
	    case PM_VALKYRIE: 
		if(!uwep) {
		    You(E_J("are not wielding a weapon!",
			    "武器を構えていない！"));
		    break;
		} else if(uwep->known == TRUE) {
		    You(E_J("already know what that weapon's quality!",
			    "すでにその武器の品質を見極めている！"));
		    break;
		} else if(is_missile(uwep) || is_ammo(uwep)) {
		    You(E_J("can't practice with that!",
			    "その武器は見極められない！"));
		    break;
		} else {
		    You(E_J("study and practice with your weapon.",
			    "自分の武器を試し振りし、見極めはじめた。"));
		    if (rnd(15) <= ACURR(A_INT)) {
			You(E_J("were able to descern it's general quality.",
				"武器の一般的な品質を見極めることができた。"));
			makeknown(uwep->otyp);
			uwep->known = TRUE;
			prinv((char *)0, uwep, 0L);
		    } else 
			pline(E_J("Unfortunatly, you still don't know it's quality.",
				  "残念ながら、あなたはその品質がわからなかった。"));
		}
		u.unextuse = rn1(250,250);
		break;

	    case PM_HEALER: 
		if (Hallucination || Stunned || Confusion) {
		    You(E_J("are in no condition to perform surgery!",
			    "手術を行えるような状態ではない！"));
		    break;
		}
		if (Sick || Slimed) {
		    if(carrying(SCALPEL)) {
			pline(E_J("Using your scalpel, you cure your infection!",
				  "メスを使い、あなたは感染した部分を切除した！"));
			make_sick(0L, (char *) 0, TRUE, SICK_NONVOMITABLE);
			Slimed = 0L;
			if(u.uhp > 6) u.uhp -= 5;
			else          u.uhp = 1;
			u.unextuse = rn1(500,500);
			flags.botl = 1;
			break;
		    } else pline(E_J("If only you had a scalpel ...",
				     "メスさえあれば手術を行えるのに…。"));
		} 
		if (u.uhp < u.uhpmax) {
		    int tmp = 0;
#ifdef FIRSTAID
		    struct obj *bandage = 0;
#ifdef JP
		    static const struct getobj_words aidw = { 0, "を使って", "応急手当する", "応急手当し" };
#endif /*JP*/
		    if(carrying(BANDAGE)) {
			bandage = getobj((const char *)0, E_J("bandage",&aidw),
					 getobj_filter_healer_special_power);
		    }
		    if (bandage) {
			if (bandage->otyp != BANDAGE) {
			    pline(E_J("That is not good for medical operation.",
				      "それは医療行為に適していない。"));
			} else if (bandage->cursed) {
#ifndef JP
			    Your("medical experience tells it's not clean.");
			    You("throw it away.");
#else
			    pline("長年の医療経験から、あなたはその包帯が清潔でないことに気づいた。");
			    You("包帯を投げ捨てた。");
#endif /*JP*/
			} else {
			    You(E_J("quickly bandage your wounds.",
				    "素早く自分の傷に包帯を巻いた。"));
			    tmp = (u.ulevel * (bcsign(bandage)+2)) + rn1(15,5);
			}
			useup(bandage);
		    }
#endif /*FIRSTAID*/
		    if (!tmp) {
			You(E_J("bandage your wounds as best you can.",
				"できるかぎりの応急処置を行った。"));
			tmp = (u.ulevel) + rn1(5,5);
		    }
		    healup(tmp, 0, FALSE, FALSE);
		    You_feel(E_J("better.","元気になった。"));
		    u.unextuse = rn1(1000,500);
		    flags.botl = 1;
		} else pline(E_J("Don't need healing!","あなたには治療の必要がない！"));
		break;
	    case PM_KNIGHT: 
		if (u.uhp < u.uhpmax || Sick) {
			You(E_J("lay your hands on your wounds and pray.",
				"自分の傷に手をかざし、祈った。"));
			pline(E_J("A warm glow spreads through your body!",
				  "暖かな輝きがあなたの身体に広がった！"));
			if(Sick) make_sick(0L, (char *) 0, TRUE, SICK_ALL);
			else     u.uhp += (u.ulevel * 4);
			if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
			u.unextuse = 3000;
			flags.botl = 1;
		} else pline(E_J("Don't need healing!","あなたには治療の必要がない！"));
		break;
	    case PM_SAMURAI: 
		You(E_J("scream \"KIIIII!\"","気合を込めて叫びをあげた！"));
		aggravate();
		u.ulastuse = rnd(2) + 1;
		u.unextuse = rn1(1000,500);
		return(0);
		break;
	    default:
		break;
	}
	return(1);
}

#ifdef WIZARD
STATIC_DCL int
wiz_gain_fullstat()
{
	int i;
	for(i = 0; i < A_MAX; i++) adjattrib(i, ATTRMAX(i)-ABASE(i), FALSE);
	addhpmax(400);
	flags.botl = 1;
	return 0;
}

STATIC_OVL void
wiz_roominfo_sub(n)
int n;
{
	struct mkroom *croom;
	winid w;
	int i;
	char buf[BUFSZ];

	croom = &rooms[n];
	w = create_nhwindow(NHW_MENU);
	Sprintf(buf, "Room #%d, Type:%d", n, croom->rtype);
	putstr(w, 0, buf);
	putstr(w, 0, "");
	Sprintf(buf, "Pos:(%d,%d)-(%d,%d)", croom->lx, croom->ly, croom->hx, croom->hy);
	putstr(w, 0, buf);
	Sprintf(buf, "fdoor:%d, doorct:%d", croom->fdoor, croom->doorct);
	putstr(w, 0, buf);
	for (i=0; i<croom->doorct; i++) {
		Sprintf(buf, "Door #%d:(%d,%d)", i, doors[croom->fdoor + i].x, doors[croom->fdoor + i].y);
		putstr(w, 0, buf);
	}
	display_nhwindow(w, FALSE);
	destroy_nhwindow(w);
}

STATIC_OVL int
wiz_roominfo()
{
	int n;

	n = levl[u.ux][u.uy].roomno;
	switch (n) {
	    case 0: pline("Not in a room."); return 0;
	    case 1: pline("On shared boundary."); return 0;
	    case 2: pline("On shared boundary[2]."); return 0;
	}
	n -= ROOMOFFSET;
	wiz_roominfo_sub(n);
	return 0;
}

STATIC_OVL int
wiz_roomlist()
{
	winid w;
	int i, n;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	if (!nroom) {
		pline("No room on this level.");
		return 0;
	}
	w = create_nhwindow(NHW_MENU);
	start_menu(w);
	for (i=0; i<nroom; i++) {
		any.a_int = i+1;
		Sprintf(buf, "Room #%d, Type:%d", i, rooms[i].rtype);
		add_menu(w, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 buf, MENU_UNSELECTED);
	}
	for (i=0; i<nsubroom; i++) {
		any.a_int = i+1+(subrooms - rooms);
		Sprintf(buf, "Subroom #%d, Type:%d", i+(subrooms - rooms), subrooms[i].rtype);
		add_menu(w, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 buf, MENU_UNSELECTED);
	}
	end_menu(w, "Pick a room to check:");
	n = select_menu(w, PICK_ONE, &selected);
	destroy_nhwindow(w);
	if (n > 0) {
		int x, y;
		n = selected[0].item.a_int - 1;	/* get item selected */
		free((genericptr_t)selected);
		wiz_roominfo_sub(n);
	}
	return 0;
}

STATIC_OVL int
wiz_objdesc()
{
	pline(*in_rooms(u.ux, u.uy, SHOPBASE) ? "[TRUE]" : "[FALSE]");
	return 0;
}
#endif

/*cmd.c*/
