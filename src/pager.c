/*	SCCS Id: @(#)pager.c	3.4	2003/08/13	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file contains the command routines dowhatis() and dohelp() and */
/* a few other help related facilities */

#include "hack.h"
#include "dlb.h"

STATIC_DCL boolean FDECL(is_swallow_sym, (int));
STATIC_DCL int FDECL(append_str, (char *, const char *));
STATIC_DCL void FDECL(checkfile,
		      (char *,struct permonst *,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL int FDECL(do_look, (BOOLEAN_P));
STATIC_DCL boolean FDECL(help_menu, (int *));
#ifdef PORT_HELP
extern void NDECL(port_help);
#endif

/* Returns "true" for characters that could represent a monster's stomach. */
STATIC_OVL boolean
is_swallow_sym(c)
int c;
{
    int i;
    for (i = S_sw_tl; i <= S_sw_br; i++)
	if ((int)showsyms[i] == c) return TRUE;
    return FALSE;
}

/*
 * Append new_str to the end of buf if new_str doesn't already exist as
 * a substring of buf.  Return 1 if the string was appended, 0 otherwise.
 * It is expected that buf is of size BUFSZ.
 */
STATIC_OVL int
append_str(buf, new_str)
    char *buf;
    const char *new_str;
{
    int space_left;	/* space remaining in buf */

    if (strstri(buf, new_str)) return 0;

    space_left = BUFSZ - strlen(buf) - 1;
    (void) strncat(buf, E_J(" or ","・"), space_left);
    (void) strncat(buf, new_str, space_left - 4);
    return 1;
}

/*
 * Return the name of the glyph found at (x,y).
 * If not hallucinating and the glyph is a monster, also monster data.
 */
struct permonst *
lookat(x, y, buf, monbuf)
    int x, y;
    char *buf, *monbuf;
{
    register struct monst *mtmp = (struct monst *) 0;
    struct permonst *pm = (struct permonst *) 0;
    int glyph;

    buf[0] = monbuf[0] = 0;
    glyph = glyph_at(x,y);
    if (u.ux == x && u.uy == y && senseself()) {
	char race[QBUFSZ];

	/* if not polymorphed, show both the role and the race */
	race[0] = 0;
	if (!Upolyd) {
	    Sprintf(race, E_J("%s ","%s"), urace.adj);
	}

#ifndef JP
	Sprintf(buf, "%s%s%s called %s",
		Invis ? "invisible " : "",
		race,
		mons[u.umonnum].mname,
		plname);
#else
	Sprintf(buf, "%sという名の%s%s%s",
		plname,
		Invis ? "目に見えない" : "",
		race,
		JMONNAM(u.umonnum));
#endif /*JP*/
	/* file lookup can't distinguish between "gnomish wizard" monster
	   and correspondingly named player character, always picking the
	   former; force it to find the general "wizard" entry instead */
	if (Role_if(PM_WIZARD) && Race_if(PM_GNOME) && !Upolyd)
	    pm = &mons[PM_WIZARD];

#ifdef STEED
	if (u.usteed) {
	    char steedbuf[BUFSZ];

	    Sprintf(steedbuf, E_J(", mounted on %s",
				  ", %sに騎乗中"), y_monnam(u.usteed));
	    /* assert((sizeof buf >= strlen(buf)+strlen(steedbuf)+1); */
	    Strcat(buf, steedbuf);
	}
#endif
	/* When you see yourself normally, no explanation is appended
	   (even if you could also see yourself via other means).
	   Sensing self while blind or swallowed is treated as if it
	   were by normal vision (cf canseeself()). */
	if ((Invisible || u.uundetected) && !Blind && !u.uswallow) {
	    unsigned how = 0;

	    if (Infravision)	 how |= 1;
	    if (Unblind_telepat) how |= 2;
	    if (Detect_monsters) how |= 4;

	    if (how)
		Sprintf(eos(buf), " [seen: %s%s%s%s%s]",
			(how & 1) ? E_J("infravision","赤外視") : "",
			/* add comma if telep and infrav */
			((how & 3) > 2) ? ", " : "",
			(how & 2) ? E_J("telepathy","ESP") : "",
			/* add comma if detect and (infrav or telep or both) */
			((how & 7) > 4) ? ", " : "",
			(how & 4) ? E_J("monster detection","怪物探知") : "");
	}
    } else if (u.uswallow) {
	/* all locations when swallowed other than the hero are the monster */
	Sprintf(buf, E_J("interior of %s","%sの内部"),
			Blind ? E_J("a monster","怪物") : a_monnam(u.ustuck));
	pm = u.ustuck->data;
    } else if (glyph_is_monster(glyph)) {
	bhitpos.x = x;
	bhitpos.y = y;
	mtmp = m_at(x,y);
	if (mtmp != (struct monst *) 0) {
	    char *name, monnambuf[BUFSZ];
	    boolean accurate = !Hallucination;

#ifdef MONSTEED
	    if (accurate)
		mtmp = mrider_or_msteed(mtmp, !canspotmon(mtmp));
#endif /*MONSTEED*/

	    if (mtmp->data == &mons[PM_COYOTE] && accurate)
		name = coyotename(mtmp, monnambuf);
	    else
		name = distant_monnam(mtmp, ARTICLE_NONE, monnambuf);

	    pm = mtmp->data;
#ifndef JP
	    Sprintf(buf, "%s%s%s",
		    (mtmp->mx != x || mtmp->my != y) ?
			((mtmp->isshk && accurate)
				? "tail of " : "tail of a ") : "",
		    (mtmp->mtame && accurate) ? "tame " :
		    (mtmp->mpeaceful && accurate) ? "peaceful " : "",
		    name);
#else
	    Sprintf(buf, "%s%s%s",
		    (mtmp->mtame && accurate) ? "手懐けられた" :
		    (mtmp->mpeaceful && accurate) ? "友好的な" : "",
		    name,
		    (mtmp->mx != x || mtmp->my != y) ? "の尾" : "");
#endif /*JP*/
#ifdef MONSTEED
	    if (is_mriding(mtmp))
		Sprintf(eos(buf), E_J(" riding on %s","(%sに騎乗中)"),
			canspotmon(mtmp->mlink) ?
			    x_monnam(mtmp->mlink, ARTICLE_A, (char *)0,
					SUPPRESS_SADDLE, TRUE) :
			    something);
#endif /*MONSTEED*/
	    if (u.ustuck == mtmp)
		Strcat(buf, (Upolyd && sticks(youmonst.data)) ?
			E_J(", being held",	", 捕らえられている") :
			E_J(", holding you",	", あなたを捕らえている"));
	    if (mtmp->mleashed)
		Strcat(buf, E_J(", leashed to you", ", 綱で引いている"));

	    if (mtmp->mtrapped && cansee(mtmp->mx, mtmp->my)) {
		struct trap *t = t_at(mtmp->mx, mtmp->my);
		int tt = t ? t->ttyp : NO_TRAP;

		/* newsym lets you know of the trap, so mention it here */
		if (tt == BEAR_TRAP || tt == PIT ||
			tt == SPIKED_PIT || tt == WEB)
#ifndef JP
		    Sprintf(eos(buf), ", trapped in %s",
			    an(defsyms[trap_to_defsym(tt)].explanation));
#else
		    Sprintf(eos(buf), ", %sにかかっている",
			    defsyms[trap_to_defsym(tt)].explanation);
#endif /*JP*/
	    }

	    {
		int ways_seen = 0, normal = 0, xraydist;
		boolean useemon = (boolean) canseemon(mtmp);

		xraydist = (u.xray_range<0) ? -1 : u.xray_range * u.xray_range;
		/* normal vision */
		if ((mtmp->wormno ? worm_known(mtmp) : cansee(mtmp->mx, mtmp->my)) &&
			mon_visible(mtmp) && !mtmp->minvis) {
		    ways_seen++;
		    normal++;
		}
		/* see invisible */
		if (useemon && mtmp->minvis)
		    ways_seen++;
		/* infravision */
		if ((!mtmp->minvis || See_invisible) && see_with_infrared(mtmp))
		    ways_seen++;
		/* telepathy */
		if (tp_sensemon(mtmp))
		    ways_seen++;
		/* xray */
		if (useemon && xraydist > 0 &&
			distu(mtmp->mx, mtmp->my) <= xraydist)
		    ways_seen++;
		if (Detect_monsters)
		    ways_seen++;
		if (MATCH_WARN_OF_MON(mtmp))
		    ways_seen++;

		if (ways_seen > 1 || !normal) {
		    if (normal) {
			Strcat(monbuf, E_J("normal vision","通常視覚"));
			/* can't actually be 1 yet here */
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (useemon && mtmp->minvis) {
			Strcat(monbuf, E_J("see invisible","透明視"));
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if ((!mtmp->minvis || See_invisible) &&
			    see_with_infrared(mtmp)) {
			Strcat(monbuf, E_J("infravision","赤外視"));
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (tp_sensemon(mtmp)) {
			Strcat(monbuf, E_J("telepathy","ESP"));
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (useemon && xraydist > 0 &&
			    distu(mtmp->mx, mtmp->my) <= xraydist) {
			/* Eyes of the Overworld */
			Strcat(monbuf, E_J("astral vision","霊視"));
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (Detect_monsters) {
			Strcat(monbuf, E_J("monster detection","怪物探知"));
			if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		    if (MATCH_WARN_OF_MON(mtmp)) {
		    	char wbuf[BUFSZ];
			if (Hallucination)
				Strcat(monbuf, E_J("paranoid delusion",
						   "偏執性妄想"));
			else {
#ifndef JP
				Sprintf(wbuf, "warned of %s",
					makeplural(mtmp->data->mname));
#else
				Sprintf(wbuf, "%sの警告",
					JMONNAM(monsndx(mtmp->data)));
#endif /*JP*/
		    		Strcat(monbuf, wbuf);
		    	}
		    	if (ways_seen-- > 1) Strcat(monbuf, ", ");
		    }
		}
	    }
	}
    }
    else if (glyph_is_object(glyph)) {
	struct obj *otmp = vobj_at(x,y);

#ifndef JP
	if (!otmp || otmp->otyp != glyph_to_obj(glyph)) {
	    if (glyph_to_obj(glyph) != STRANGE_OBJECT) {
		otmp = mksobj(glyph_to_obj(glyph), FALSE, FALSE);
		if (otmp->oclass == COIN_CLASS)
		    otmp->quan = 2L; /* to force pluralization */
		else if (otmp->otyp == SLIME_MOLD)
		    otmp->spe = current_fruit;	/* give the fruit a type */
		else if (otmp->otyp == STATUE) {
		    otmp->madeof = (glyph - GLYPH_STATUE_OFF) & 0x1f;
		    otmp->corpsenm = (glyph - GLYPH_STATUE_OFF) >> 5;
		}
		Strcpy(buf, distant_name(otmp, xname));
		dealloc_obj(otmp);
	    }
	} else
	    Strcpy(buf, distant_name(otmp, xname));
#endif /*JP*/

	if (levl[x][y].typ == STONE || levl[x][y].typ == SCORR)
	    Strcat(buf, E_J(" embedded in stone","石に埋まった"));
	else if (IS_WALL(levl[x][y].typ) || levl[x][y].typ == SDOOR)
	    Strcat(buf, E_J(" embedded in a wall","壁に埋まった"));
	else if (closed_door(x,y))
	    Strcat(buf, E_J(" embedded in a door","扉に埋まった"));
	else if (is_pool(x,y))
	    Strcat(buf, E_J(" in water","水中にある"));
	else if (is_lava(x,y))
	    Strcat(buf, E_J(" in molten lava","溶岩に沈んだ"));	/* [can this ever happen?] */
#ifdef JP
	if (!otmp || otmp->otyp != glyph_to_obj(glyph)) {
	    if (glyph_to_obj(glyph) != STRANGE_OBJECT) {
		otmp = mksobj(glyph_to_obj(glyph), FALSE, FALSE);
		if (otmp->oclass == COIN_CLASS)
		    otmp->quan = 2L; /* to force pluralization */
		else if (otmp->otyp == SLIME_MOLD)
		    otmp->spe = current_fruit;	/* give the fruit a type */
		else if (otmp->otyp == STATUE) {
		    otmp->madeof = (glyph - GLYPH_STATUE_OFF) & 0x1f;
		    otmp->corpsenm = (glyph - GLYPH_STATUE_OFF) >> 5;
		}
		Strcpy(buf, distant_name(otmp, xname));
		dealloc_obj(otmp);
	    }
	} else
	    Strcpy(buf, distant_name(otmp, xname));
#endif /*JP*/
    } else if (glyph_is_trap(glyph)) {
	int tnum = what_trap(glyph_to_trap(glyph));
	Strcpy(buf, defsyms[trap_to_defsym(tnum)].explanation);
    } else if (glyph_is_warning(glyph)) {
	Strcpy(buf, def_warnsyms[glyph - GLYPH_WARNING_OFF].explanation);
    } else if(!glyph_is_cmap(glyph)) {
	Strcpy(buf,E_J("dark part of a room","部屋の暗い部分"));
    } else switch(glyph_to_cmap(glyph)) {
    case S_altar:
	if(!In_endgame(&u.uz))
	    Sprintf(buf, E_J("%s altar","%sの祭壇"),
		align_str(Amask2align(levl[x][y].altarmask & ~AM_SHRINE)));
	else Sprintf(buf, E_J("aligned altar","祭壇"));
	break;
    case S_ndoor:
	if (is_drawbridge_wall(x, y) >= 0)
	    Strcpy(buf,E_J("open drawbridge portcullis",
			   "開いた跳ね橋の落とし格子"));
	else if ((levl[x][y].doormask & ~D_TRAPPED) == D_BROKEN)
	    Strcpy(buf,E_J("broken door","壊された扉"));
	else
	    Strcpy(buf,E_J("doorway","出入り口"));
	break;
    case S_cloud:
	Strcpy(buf, Is_airlevel(&u.uz) ? E_J("cloudy area","雲") : E_J("fog/vapor cloud","霧・蒸気"));
	break;
    default:
	Strcpy(buf,defsyms[glyph_to_cmap(glyph)].explanation);
	break;
    }

    return ((pm && !Hallucination) ? pm : (struct permonst *) 0);
}

/*
 * Look in the "data" file for more info.  Called if the user typed in the
 * whole name (user_typed_name == TRUE), or we've found a possible match
 * with a character/glyph and flags.help is TRUE.
 *
 * NOTE: when (user_typed_name == FALSE), inp is considered read-only and
 *	 must not be changed directly, e.g. via lcase(). We want to force
 *	 lcase() for data.base lookup so that we can have a clean key.
 *	 Therefore, we create a copy of inp _just_ for data.base lookup.
 */
STATIC_OVL void
checkfile(inp, pm, user_typed_name, without_asking)
    char *inp;
    struct permonst *pm;
    boolean user_typed_name, without_asking;
{
    dlb *fp;
    char buf[BUFSZ], newstr[BUFSZ];
    char *ep, *dbase_str;
    long txt_offset;
    int chk_skip;
    boolean found_in_file = FALSE, skipping_entry = FALSE;

    fp = dlb_fopen(DATAFILE, "r");
    if (!fp) {
	pline("Cannot open data file!");
	return;
    }

    /* To prevent the need for entries in data.base like *ngel to account
     * for Angel and angel, make the lookup string the same for both
     * user_typed_name and picked name.
     */
    if (pm != (struct permonst *) 0 && !user_typed_name)
	dbase_str = strcpy(newstr, pm->mname);
    else dbase_str = strcpy(newstr, inp);
    (void) lcase(dbase_str);

    if (!strncmp(dbase_str, "interior of ", 12))
	dbase_str += 12;
    if (!strncmp(dbase_str, "a ", 2))
	dbase_str += 2;
    else if (!strncmp(dbase_str, "an ", 3))
	dbase_str += 3;
    else if (!strncmp(dbase_str, "the ", 4))
	dbase_str += 4;
    if (!strncmp(dbase_str, "tame ", 5))
	dbase_str += 5;
    else if (!strncmp(dbase_str, "peaceful ", 9))
	dbase_str += 9;
    if (!strncmp(dbase_str, "invisible ", 10))
	dbase_str += 10;
    if (!strncmp(dbase_str, "statue of ", 10))
	dbase_str[6] = '\0';
    else if (!strncmp(dbase_str, "figurine of ", 12))
	dbase_str[8] = '\0';
#ifdef JP
    if (!strncmp(dbase_str, "手懐けられた", 12))
	dbase_str += 12;
    else if (!strncmp(dbase_str, "友好的な", 8))
	dbase_str += 8;
    if (!strncmp(dbase_str, "目に見えない", 12))
	dbase_str += 12;
#endif /*JP*/

    /* Make sure the name is non-empty. */
    if (*dbase_str) {
	/* adjust the input to remove "named " and convert to lower case */
	char *alt = 0;	/* alternate description */

	if ((ep = strstri(dbase_str, " named ")) != 0)
	    alt = ep + 7;
	else
	    ep = strstri(dbase_str, " called ");
	if (!ep) ep = strstri(dbase_str, ", ");
	if (ep && ep > dbase_str) *ep = '\0';

	/*
	 * If the object is named, then the name is the alternate description;
	 * otherwise, the result of makesingular() applied to the name is. This
	 * isn't strictly optimal, but named objects of interest to the user
	 * will usually be found under their name, rather than under their
	 * object type, so looking for a singular form is pointless.
	 */

	if (!alt)
	    alt = makesingular(dbase_str);
	else
	    if (user_typed_name)
		(void) lcase(alt);

	/* skip first record; read second */
	txt_offset = 0L;
	if (!dlb_fgets(buf, BUFSZ, fp) || !dlb_fgets(buf, BUFSZ, fp)) {
	    impossible("can't read 'data' file");
	    (void) dlb_fclose(fp);
	    return;
	} else if (sscanf(buf, "%8lx\n", &txt_offset) < 1 || txt_offset <= 0)
	    goto bad_data_file;

	/* look for the appropriate entry */
	while (dlb_fgets(buf,BUFSZ,fp)) {
	    if (*buf == '.') break;  /* we passed last entry without success */

	    if (digit(*buf)) {
		/* a number indicates the end of current entry */
		skipping_entry = FALSE;
	    } else if (!skipping_entry) {
		if (!(ep = index(buf, '\n'))) goto bad_data_file;
		*ep = 0;
		/* if we match a key that begins with "~", skip this entry */
		chk_skip = (*buf == '~') ? 1 : 0;
		if (pmatch(&buf[chk_skip], dbase_str) ||
			(alt && pmatch(&buf[chk_skip], alt))) {
		    if (chk_skip) {
			skipping_entry = TRUE;
			continue;
		    } else {
			found_in_file = TRUE;
			break;
		    }
		}
	    }
	}
    }

    if(found_in_file) {
	long entry_offset;
	int  entry_count;
	int  i;

	/* skip over other possible matches for the info */
	do {
	    if (!dlb_fgets(buf, BUFSZ, fp)) goto bad_data_file;
	} while (!digit(*buf));
	if (sscanf(buf, "%ld,%d\n", &entry_offset, &entry_count) < 2) {
bad_data_file:	impossible("'data' file in wrong format");
		(void) dlb_fclose(fp);
		return;
	}

	if (user_typed_name || without_asking || yn(E_J("More info?","追加情報を見ますか？")) == 'y') {
	    winid datawin;

	    if (dlb_fseek(fp, txt_offset + entry_offset, SEEK_SET) < 0) {
		pline("? Seek error on 'data' file!");
		(void) dlb_fclose(fp);
		return;
	    }
	    datawin = create_nhwindow(NHW_MENU);
	    for (i = 0; i < entry_count; i++) {
		if (!dlb_fgets(buf, BUFSZ, fp)) goto bad_data_file;
		if ((ep = index(buf, '\n')) != 0) *ep = 0;
		if (index(buf+1, '\t') != 0) (void) tabexpand(buf+1);
		putstr(datawin, 0, buf+1);
	    }
	    display_nhwindow(datawin, FALSE);
	    destroy_nhwindow(datawin);
	}
    } else if (user_typed_name)
	pline(E_J("I don't have any information on those things.",
		  "その項目に関する情報はありません。"));

    (void) dlb_fclose(fp);
}

/* getpos() return values */
#define LOOK_TRADITIONAL	0	/* '.' -- ask about "more info?" */
#define LOOK_QUICK		1	/* ',' -- skip "more info?" */
#define LOOK_ONCE		2	/* ';' -- skip and stop looping */
#define LOOK_VERBOSE		3	/* ':' -- show more info w/o asking */

/* also used by getpos hack in do_name.c */
const char what_is_an_unknown_object[] = E_J("an unknown object","知りたいもの");

STATIC_OVL int
do_look(quick)
    boolean quick;	/* use cursor && don't search for "more info" */
{
    char    out_str[BUFSZ], look_buf[BUFSZ];
    const char *x_str, *firstmatch = 0;
    struct permonst *pm = 0;
    int     i, ans = 0;
    int     sym;		/* typed symbol or converted glyph */
    int	    found;		/* count of matching syms found */
    coord   cc;			/* screen pos of unknown glyph */
    boolean save_verbose;	/* saved value of flags.verbose */
    boolean from_screen;	/* question from the screen */
    boolean need_to_look;	/* need to get explan. from glyph */
    boolean hit_trap;		/* true if found trap explanation */
    int skipped_venom;		/* non-zero if we ignored "splash of venom" */
    static const char *mon_interior = "the interior of a monster";

    if (quick) {
	from_screen = TRUE;	/* yes, we want to use the cursor */
    } else {
	i = ynq(E_J("Specify unknown object by cursor?",
		    "知りたいものをカーソルで指定しますか？"));
	if (i == 'q') return 0;
	from_screen = (i == 'y');
    }

    if (from_screen) {
	cc.x = u.ux;
	cc.y = u.uy;
	sym = 0;		/* gcc -Wall lint */
    } else {
	getlin(E_J("Specify what? (type the word)",
		   "何について？(単語を入力)"), out_str);
	if (out_str[0] == '\0' || out_str[0] == '\033')
	    return 0;

	if (out_str[1]) {	/* user typed in a complete string */
	    checkfile(out_str, pm, TRUE, TRUE);
	    return 0;
	}
	sym = out_str[0];
    }

    /* Save the verbose flag, we change it later. */
    save_verbose = flags.verbose;
    flags.verbose = flags.verbose && !quick;
    /*
     * The user typed one letter, or we're identifying from the screen.
     */
    do {
	/* Reset some variables. */
	need_to_look = FALSE;
	pm = (struct permonst *)0;
	skipped_venom = 0;
	found = 0;
	out_str[0] = '\0';

	if (from_screen) {
	    int glyph;	/* glyph at selected position */

	    if (flags.verbose)
		pline(E_J("Please move the cursor to %s.",
			  "カーソルを%sの上に移動してください。"),
		       what_is_an_unknown_object);
	    else
		pline(E_J("Pick an object.","調べるものを選択"));

	    ans = getpos(&cc, quick, what_is_an_unknown_object);
	    if (ans < 0 || cc.x < 0) {
		flags.verbose = save_verbose;
		return 0;	/* done */
	    }
	    flags.verbose = FALSE;	/* only print long question once */

	    /* Convert the glyph at the selected position to a symbol. */
	    glyph = glyph_at(cc.x,cc.y);
	    if (glyph_is_cmap(glyph)) {
		sym = showsyms[glyph_to_cmap(glyph)];
	    } else if (glyph_is_trap(glyph)) {
		sym = showsyms[trap_to_defsym(glyph_to_trap(glyph))];
	    } else if (glyph_is_object(glyph)) {
		int otyp = (int)glyph_to_obj(glyph);
		sym = oc_syms[(int)objects[otyp].oc_class];
		if (sym == '`') {
		    if (iflags.bouldersym && otyp == BOULDER)
			sym = iflags.bouldersym;
		    else if (otyp == STATUE)
			sym = monsyms[(int)mons[glyph_to_mon(glyph)].mlet];
		}
	    } else if (glyph_is_monster(glyph)) {
		/* takes care of pets, detected, ridden, and regular mons */
		sym = monsyms[(int)mons[glyph_to_mon(glyph)].mlet];
	    } else if (glyph_is_swallow(glyph)) {
		sym = showsyms[glyph_to_swallow(glyph)+S_sw_tl];
	    } else if (glyph_is_invisible(glyph)) {
		sym = DEF_INVISIBLE;
	    } else if (glyph_is_warning(glyph)) {
		sym = glyph_to_warning(glyph);
	    	sym = warnsyms[sym];
	    } else if (glyph_is_cloud(glyph)) {
		sym = showsyms[glyph_to_cloud(glyph)+S_thinpcloud];
	    } else {
		impossible("do_look:  bad glyph %d at (%d,%d)",
						glyph, (int)cc.x, (int)cc.y);
		sym = ' ';
	    }
	}

	/*
	 * Check all the possibilities, saving all explanations in a buffer.
	 * When all have been checked then the string is printed.
	 */

	/* Check for monsters */
	for (i = 0; i < MAXMCLASSES; i++) {
	    if (sym == (from_screen ? monsyms[i] : def_monsyms[i]) &&
		monexplain[i]) {
		need_to_look = TRUE;
		if (!found) {
#ifndef JP
		    Sprintf(out_str, "%c       %s", sym, an(monexplain[i]));
#else
		    Sprintf(out_str, "%c       %s", sym, monexplain[i]);
#endif /*JP*/
		    firstmatch = monexplain[i];
		    found++;
		} else {
#ifndef JP
		    found += append_str(out_str, an(monexplain[i]));
#else
		    found += append_str(out_str, monexplain[i]);
#endif /*JP*/
		}
	    }
	}
	/* handle '@' as a special case if it refers to you and you're
	   playing a character which isn't normally displayed by that
	   symbol; firstmatch is assumed to already be set for '@' */
	if ((from_screen ?
		(sym == monsyms[S_HUMAN] && cc.x == u.ux && cc.y == u.uy) :
		(sym == def_monsyms[S_HUMAN] && !iflags.showrace)) &&
	    !(Race_if(PM_HUMAN) || Race_if(PM_ELF)) && !Upolyd)
	    found += append_str(out_str, E_J("you","あなた"));	/* tack on "or you" */

	/*
	 * Special case: if identifying from the screen, and we're swallowed,
	 * and looking at something other than our own symbol, then just say
	 * "the interior of a monster".
	 */
	if (u.uswallow && from_screen && is_swallow_sym(sym)) {
	    if (!found) {
		Sprintf(out_str, "%c       %s", sym, mon_interior);
		firstmatch = mon_interior;
	    } else {
		found += append_str(out_str, mon_interior);
	    }
	    need_to_look = TRUE;
	}

	/* Now check for objects */
	for (i = 1; i < MAXOCLASSES; i++) {
	    if (sym == (from_screen ? oc_syms[i] : def_oc_syms[i])) {
		need_to_look = TRUE;
		if (from_screen && i == VENOM_CLASS) {
		    skipped_venom++;
		    continue;
		}
		if (!found) {
#ifndef JP
		    Sprintf(out_str, "%c       %s", sym, an(objexplain[i]));
#else
		    Sprintf(out_str, "%c       %s", sym, objexplain[i]);
#endif /*JP*/
		    firstmatch = objexplain[i];
		    found++;
		} else {
#ifndef JP
		    found += append_str(out_str, an(objexplain[i]));
#else
		    found += append_str(out_str, objexplain[i]);
#endif /*JP*/
		}
	    }
	}

	if (sym == DEF_INVISIBLE) {
	    if (!found) {
#ifndef JP
		Sprintf(out_str, "%c       %s", sym, an(invisexplain));
#else
		Sprintf(out_str, "%c       %s", sym, invisexplain);
#endif /*JP*/
		firstmatch = invisexplain;
		found++;
	    } else {
#ifndef JP
		found += append_str(out_str, an(invisexplain));
#else
		found += append_str(out_str, invisexplain);
#endif /*JP*/
	    }
	}

#define is_cmap_trap(i) ((i) >= S_arrow_trap && (i) <= S_polymorph_trap)
#define is_cmap_drawbridge(i) ((i) >= S_vodbridge && (i) <= S_hcdbridge)
#define is_cmap_cloud(i) ((i) >= S_thinpcloud && (i) <= S_densepcloud)

	/* Now check for graphics symbols */
	for (hit_trap = FALSE, i = 0; i < MAXPCHARS; i++) {
	    x_str = defsyms[i].explanation;
	    if (sym == (from_screen ? showsyms[i] : defsyms[i].sym) && *x_str) {
#ifndef JP
		/* avoid "an air", "a water", or "a floor of a room" */
		int article = (i == S_room || i == S_grass ||
			       i == S_litgrass) ? 2 :			/* 2=>"the" */
			      !(strcmp(x_str, "air") == 0 ||		/* 1=>"an"  */
				strcmp(x_str, "water") == 0);		/* 0=>(none)*/
#endif /*JP*/

		if (!found) {
		    if (is_cmap_trap(i)) {
#ifndef JP
			Sprintf(out_str, "%c       a trap", sym);
#else
			Sprintf(out_str, "%c       罠", sym);
#endif /*JP*/
			hit_trap = TRUE;
		    } else {
#ifndef JP
			Sprintf(out_str, "%c       %s", defsyms[i].sym/*sym*/,
				article == 2 ? the(x_str) :
				article == 1 ? an(x_str) : x_str);
#else
			Sprintf(out_str, "%c       %s", defsyms[i].sym, x_str);
#endif /*JP*/
		    }
		    firstmatch = x_str;
		    found++;
		} else if (!u.uswallow && !(hit_trap && is_cmap_trap(i)) &&
			   !(found >= 3 && is_cmap_drawbridge(i))) {
#ifndef JP
		    found += append_str(out_str,
					article == 2 ? the(x_str) :
					article == 1 ? an(x_str) : x_str);
#else
		    found += append_str(out_str, x_str);
#endif /*JP*/
		    if (is_cmap_trap(i)) hit_trap = TRUE;
		}

		if (i == S_altar || is_cmap_trap(i))
		    need_to_look = TRUE;
	    }
	}

	/* Now check for warning symbols */
	for (i = 0; i < WARNCOUNT; i++) {
	    x_str = def_warnsyms[i].explanation;
	    if (sym == (from_screen ? warnsyms[i] : def_warnsyms[i].sym)) {
		if (!found) {
			Sprintf(out_str, "%c       %s",
				sym, def_warnsyms[i].explanation);
			firstmatch = def_warnsyms[i].explanation;
			found++;
		} else {
			found += append_str(out_str, def_warnsyms[i].explanation);
		}
		/* Kludge: warning trumps boulders on the display.
		   Reveal the boulder too or player can get confused */
		if (from_screen && sobj_at(BOULDER, cc.x, cc.y))
			Strcat(out_str, E_J(" co-located with a boulder",
					    ", 大岩と同じ位置にいる"));
		break;	/* out of for loop*/
	    }
	}
    
	/* if we ignored venom and list turned out to be short, put it back */
	if (skipped_venom && found < 2) {
	    x_str = objexplain[VENOM_CLASS];
	    if (!found) {
#ifndef JP
		Sprintf(out_str, "%c       %s", sym, an(x_str));
#else
		Sprintf(out_str, "%c       %s", sym, x_str);
#endif /*JP*/
		firstmatch = x_str;
		found++;
	    } else {
#ifndef JP
		found += append_str(out_str, an(x_str));
#else
		found += append_str(out_str, x_str);
#endif /*JP*/
	    }
	}

	/* handle optional boulder symbol as a special case */ 
	if (iflags.bouldersym && sym == iflags.bouldersym) {
	    x_str = E_J("boulder","大岩");
	    if (!found) {
#ifndef JP
		Sprintf(out_str, "%c       %s", sym, an(x_str));
#else
		Sprintf(out_str, "%c       %s", sym, x_str);
#endif /*JP*/
		firstmatch = x_str;
		found++;
	    } else {
		found += append_str(out_str, x_str);
	    }
	}
	/*
	 * If we are looking at the screen, follow multiple possibilities or
	 * an ambiguous explanation by something more detailed.
	 */
	if (from_screen) {
	    if (found > 1 || need_to_look) {
		char monbuf[BUFSZ];
		char temp_buf[BUFSZ];

		pm = lookat(cc.x, cc.y, look_buf, monbuf);
		firstmatch = look_buf;
		if (*firstmatch) {
		    Sprintf(temp_buf, " (%s)", firstmatch);
		    (void)strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
		    found = 1;	/* we have something to look up */
		}
		if (monbuf[0]) {
		    Sprintf(temp_buf, E_J(" [seen: %s]"," [認識:%s]"), monbuf);
		    (void)strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
		}
	    }
	}

	/* Finally, print out our explanation. */
	if (found) {
	    pline("%s", out_str);
	    /* check the data file for information about this thing */
	    if (found == 1 && ans != LOOK_QUICK && ans != LOOK_ONCE &&
			(ans == LOOK_VERBOSE || (flags.help && !quick))) {
		char temp_buf[BUFSZ];
		Strcpy(temp_buf, firstmatch);
		checkfile(temp_buf, pm, FALSE, (boolean)(ans == LOOK_VERBOSE));
	    }
	} else {
	    pline(E_J("I've never heard of such things.",
		      "そのようなものは聞いたことがありません。"));
	}

    } while (from_screen && !quick && ans != LOOK_ONCE);

    flags.verbose = save_verbose;
    return 0;
}


int
dowhatis()
{
	return do_look(FALSE);
}

int
doquickwhatis()
{
	struct monst *mtmp;
	bhitpos.x = bhitpos.y = 0;
	do_look(TRUE);
	if (bhitpos.x != 0 && !Hallucination) {
	    mtmp = m_at(bhitpos.x, bhitpos.y);
	    if (mtmp && canseemon(mtmp) &&
		/* keep consistency with display_minventory() */
		mon_has_worn_wield_items(mtmp)) {
		display_minventory(mtmp, MINV_NOLET, (char *)0);
	    }
	}
	return 0;
}

int
doidtrap()
{
	register struct trap *trap;
	int x, y, tt;

/*	if (!getdir("^")) return 0;*/
	if (!getdir_or_pos(0, GETPOS_TRAP, "^", E_J("identify","識別"))) return 0;
	x = u.ux + u.dx;
	y = u.uy + u.dy;
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if (trap->tx == x && trap->ty == y) {
		if (!trap->tseen) break;
		tt = trap->ttyp;
		if (u.dz) {
		    if (u.dz < 0 ? (tt == TRAPDOOR || tt == HOLE) :
			    tt == ROCKTRAP) break;
		}
		tt = what_trap(tt);
#ifndef JP
		pline("That is %s%s%s.",
		      an(defsyms[trap_to_defsym(tt)].explanation),
		      !trap->madeby_u ? "" : (tt == WEB) ? " woven" :
			  /* trap doors & spiked pits can't be made by
			     player, and should be considered at least
			     as much "set" as "dug" anyway */
			  (tt == HOLE || tt == PIT) ? " dug" : " set",
		      !trap->madeby_u ? "" : " by you");
#else
		pline("これは%s%s%sだ。",
		      !trap->madeby_u ? "" : "あなたが",
		      !trap->madeby_u ? "" : (tt == WEB) ? "張った" :
			  (tt == HOLE || tt == PIT) ? "掘った" : "仕掛けた",
		      defsyms[trap_to_defsym(tt)].explanation);
#endif /*JP*/
		return 0;
	    }
	pline(E_J("I can't see a trap there.",
		  "そこには罠は見当たらない。"));
	return 0;
}

char *
dowhatdoes_core(q, cbuf)
char q;
char *cbuf;
{
	dlb *fp;
	char bufr[BUFSZ];
	register char *buf = &bufr[6], *ep, ctrl, meta;

	fp = dlb_fopen(CMDHELPFILE, "r");
	if (!fp) {
		pline("Cannot open data file!");
		return 0;
	}

  	ctrl = ((q <= '\033') ? (q - 1 + 'A') : 0);
	meta = ((0x80 & q) ? (0x7f & q) : 0);
	while(dlb_fgets(buf,BUFSZ-6,fp)) {
	    if ((ctrl && *buf=='^' && *(buf+1)==ctrl) ||
		(meta && *buf=='M' && *(buf+1)=='-' && *(buf+2)==meta) ||
		*buf==q) {
		ep = index(buf, '\n');
		if(ep) *ep = 0;
		if (ctrl && buf[2] == '\t'){
			buf = bufr + 1;
			(void) strncpy(buf, "^?      ", 8);
			buf[1] = ctrl;
		} else if (meta && buf[3] == '\t'){
			buf = bufr + 2;
			(void) strncpy(buf, "M-?     ", 8);
			buf[2] = meta;
		} else if(buf[1] == '\t'){
			buf = bufr;
			buf[0] = q;
			(void) strncpy(buf+1, "       ", 7);
		}
		(void) dlb_fclose(fp);
		Strcpy(cbuf, buf);
		return cbuf;
	    }
	}
	(void) dlb_fclose(fp);
	return (char *)0;
}

int
dowhatdoes()
{
	char bufr[BUFSZ];
	char q, *reslt;

#if defined(UNIX) || defined(VMS)
	introff();
#endif
	q = yn_function(E_J("What command?","どのコマンドを？"), (char *)0, '\0');
#if defined(UNIX) || defined(VMS)
	intron();
#endif
	reslt = dowhatdoes_core(q, bufr);
	if (reslt)
		pline("%s", reslt);
	else
		pline(E_J("I've never heard of such commands.",
			  "そんなコマンドは聞いたことがありません。"));
	return 0;
}

/* data for help_menu() */
static const char *help_menu_items[] = {
/* 0*/	E_J("Long description of the game and commands.",
	    "ゲームとコマンドの詳細な解説"),
/* 1*/	E_J("List of game commands.",
	    "コマンド一覧"),
/* 2*/	E_J("Concise history of NetHack.",
	    "NetHackの簡単な歴史"),
/* 3*/	E_J("Info on a character in the game display.",
	    "画面に表示される文字の説明"),
/* 4*/	E_J("Info on what a given key does.",
	    "このキーが何を意味するかの説明"),
/* 5*/	E_J("List of game options.",
	    "ゲームのオプション一覧"),
/* 6*/	E_J("Longer explanation of game options.",
	    "ゲームのオプションの詳細な解説"),
/* 7*/	E_J("List of extended commands.",
	    "拡張コマンドの一覧"),
/* 8*/	E_J("The NetHack license.",
	    "NetHackのライセンス"),
#ifdef PORT_HELP
	E_J("%s-specific help and commands.",
	    "%s特有のヘルプおよびコマンド"),
#define PORT_HELP_ID 100
#define WIZHLP_SLOT 10
#else
#define WIZHLP_SLOT 9
#endif
#ifdef WIZARD
	E_J("List of wizard-mode commands.",
	    "wizardモードのコマンド一覧"),
#endif
	"",
	(char *)0
};

STATIC_OVL boolean
help_menu(sel)
	int *sel;
{
	winid tmpwin = create_nhwindow(NHW_MENU);
#ifdef PORT_HELP
	char helpbuf[QBUFSZ];
#endif
	int i, n;
	menu_item *selected;
	anything any;

	any.a_void = 0;		/* zero all bits */
	start_menu(tmpwin);
#ifdef WIZARD
	if (!wizard) help_menu_items[WIZHLP_SLOT] = "",
		     help_menu_items[WIZHLP_SLOT+1] = (char *)0;
#endif
	for (i = 0; help_menu_items[i]; i++)
#ifdef PORT_HELP
	    /* port-specific line has a %s in it for the PORT_ID */
	    if (help_menu_items[i][0] == '%') {
		Sprintf(helpbuf, help_menu_items[i], PORT_ID);
		any.a_int = PORT_HELP_ID + 1;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 helpbuf, MENU_UNSELECTED);
	    } else
#endif
	    {
		any.a_int = (*help_menu_items[i]) ? i+1 : 0;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0,
			ATR_NONE, help_menu_items[i], MENU_UNSELECTED);
	    }
	end_menu(tmpwin, E_J("Select one item:",
			     "項目を選択:"));
	n = select_menu(tmpwin, PICK_ONE, &selected);
	destroy_nhwindow(tmpwin);
	if (n > 0) {
	    *sel = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    return TRUE;
	}
	return FALSE;
}

int
dohelp()
{
	int sel = 0;

	if (help_menu(&sel)) {
		switch (sel) {
			case  0:  display_file(HELP, TRUE);  break;
			case  1:  display_file(SHELP, TRUE);  break;
			case  2:  (void) dohistory();  break;
			case  3:  (void) dowhatis();  break;
			case  4:  (void) dowhatdoes();  break;
			case  5:  option_help();  break;
			case  6:  display_file(OPTIONFILE, TRUE);  break;
			case  7:  (void) doextlist();  break;
			case  8:  display_file(LICENSE, TRUE);  break;
#ifdef WIZARD
			/* handle slot 9 or 10 */
			default: display_file(DEBUGHELP, TRUE);  break;
#endif
#ifdef PORT_HELP
			case PORT_HELP_ID:  port_help();  break;
#endif
		}
	}
	return 0;
}

int
dohistory()
{
	display_file(HISTORY, TRUE);
	return 0;
}

/*
 * Description of an object
 */
void
doobjdesc(otmp)
struct obj *otmp;
{
    dlb *fp;
    char buf[BUFSZ], buf2[BUFSZ];
    char *p, *ps, *pd;
    char *ep;
    long txt_offset;
    int oindex, len, line_cnt, i;
    int skipmode;
    winid datawin;

    if (!otmp->dknown) {
	pline(E_J("You don't know the detail of this object.",
		  "この品物の詳細はわからない。"));
    }

    fp = dlb_fopen(OBJDESCFILE, RDBMODE);
    if (!fp) {
	pline("Cannot open objdesc file!");
	return;
    }
    ep = (char *)0;
    if (otmp->oartifact) {
	oindex = NUM_OBJECTS * 2 + otmp->oartifact;
    } else if (objects[otmp->otyp].oc_name_known) {
	oindex = otmp->otyp * 2;
    } else {
	oindex = (objects[otmp->otyp].oc_descr_idx) * 2 + 1;
    }
    if (dlb_fseek(fp, oindex * sizeof(long), SEEK_SET) < 0) {
	ep = "? Seek error on 'objdesc' file!";
	goto xit;
    }
    if (dlb_fread((char *)&txt_offset, sizeof(long), 1, fp) != 1) {
	ep = "? Cannot read offset from 'objdesc' file!";
	goto xit;
    }

    if (!txt_offset) {
	(void) dlb_fclose(fp);
	return;
    }
    if (dlb_fseek(fp, txt_offset, SEEK_SET) < 0) {
	pline("? Seek error on 'objdesc' file!");
	(void) dlb_fclose(fp);
	return;
    }

    datawin = create_nhwindow(NHW_MENU);
    putstr(datawin, 0, distant_name(otmp, xname));
    putstr(datawin, 0, "");
    if (otmp->otyp == SLIME_MOLD && otmp->spe != current_fruit) {
#ifndef JP
	putstr(datawin, 0, "A delicious food which was beloved by someone,");
	putstr(datawin, 0, "who had a firm policy to eat it last.");
#else
	putstr(datawin, 0, "おいしそうな食べ物だ。これを遺した誰かは、");
	putstr(datawin, 0, "好物を後にとっておく主義だったに違いない。");
#endif
	goto disp;
    }
    line_cnt = 0;
    skipmode = 0;
    while (((len = dlb_fgetc(fp)) & 0xFF) != 0xFF) {
	if (len >= 80) {
	    ep = "? Too long line was found in 'objdesc' file!";
	    break;
	}
	if (len) {
	    if (dlb_fread(buf, sizeof(char), len, fp) != len) {
		ep = "? Read error from 'objdesc' file!";
		break;
	    }
	}
	buf[len] = 0;
	buf2[0] = 0;
	ps = buf; pd = buf2;
	while ((p = index(ps, '%')) != NULL) {
	    len = p - ps;
	    if (len) {
		if (!skipmode) strncpy(pd, ps, len);
		pd += len;
	    }
	    switch (p[1]) {
		case 'n':	/* actual name */
		    i = otmp->otyp;
		    Strcpy(pd,
#ifdef JP
				jobj_descr[i].oc_name ?
					jobj_descr[i].oc_name : obj_descr[i].oc_name
#else
				obj_descr[i].oc_name
#endif /*JP*/
		    );
		    pd = eos(pd);
		    ps = p+2;
		    break;
		case 'd':	/* description name */
		    i = objects[otmp->otyp].oc_descr_idx;
		    Strcpy(pd,
#ifdef JP
				jobj_descr[i].oc_descr ?
					jobj_descr[i].oc_descr : obj_descr[i].oc_descr
#else
				obj_descr[i].oc_descr
#endif /*JP*/
		    );
		    pd = eos(pd);
		    ps = p+2;
		    break;
		case 'f':	/* fruitname */
		    Strcpy(pd, fruitname(FALSE));
		    pd = eos(pd);
		    ps = p+2;
		    break;
		case 'm':	/* material name */
		    Strcpy(pd, material_word_for_simplename(otmp));
		    pd = eos(pd);
#ifdef JP
		    pd -= 2;	/* 'の' を消す */
#endif /*JP*/
		    ps = p+2;
		    break;
		case '%':	/* '%' */
		    *pd++ = '%';
		    ps = p+2;
		    break;
		default:
		    ep = "? Unknown control char!";
		    ps++;
		    break;
	    }
	}
	Strcpy(pd, ps);
	putstr(datawin, 0, buf2);
	if (line_cnt++ > 50) {
	    ep = "? Too many lines in 'objdesc' file!";
	    break;
	}
    }
disp:
    if (!ep) display_nhwindow(datawin, FALSE);
xit2:
    destroy_nhwindow(datawin);
xit:
    if (ep) pline(ep);
    (void) dlb_fclose(fp);
}

/*pager.c*/
