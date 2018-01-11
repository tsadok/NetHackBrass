/*	SCCS Id: @(#)do_name.c	3.4	2003/01/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVLB

STATIC_DCL void FDECL(do_oname, (struct obj *));
static void FDECL(getpos_help, (int,const char *));

/*------------------------------------------*/
struct gpos {
	int	centerx, centery;
	int	x, y;
	int	radius;
	int	quadrant;
	int	r2limit, r2min, r2max;
	int	target;
};
STATIC_DCL int FDECL(dxdy_to_quadrant, (int, int));
STATIC_DCL void FDECL(xy_to_radius, (struct gpos *));
STATIC_DCL int FDECL(getpos_target_at, (int,int,int));
STATIC_DCL int FDECL(getprevpos, (struct gpos *));
STATIC_DCL int FDECL(getnextpos, (struct gpos *));
STATIC_DCL void FDECL(show_what_is_here, (struct gpos *gp, BOOLEAN_P));
/*------------------------------------------*/

extern const char what_is_an_unknown_object[];		/* from pager.c */

/* the response for '?' help request in getpos() */
static void
getpos_help(force, goal)
int force;
const char *goal;
{
    char sbuf[BUFSZ];
    boolean doing_what_is;
    winid tmpwin = create_nhwindow(NHW_MENU);

    Sprintf(sbuf, E_J("Use [%s] to move the cursor to %s.",
		      "[%s] でカーソルを動かし、%sを指定してください。"),
	    iflags.num_pad ? "2468" : "hjkl", goal);
    putstr(tmpwin, 0, sbuf);
    putstr(tmpwin, 0, E_J("Use [HJKL] to move the cursor 8 units at a time.",
			  "[HJKL] でカーソルを8文字ごとに移動できます。"));
    if (force != 2)
	putstr(tmpwin, 0, E_J("Or enter a background symbol (ex. <).",
			      "または、背景のシンボルを入力してください。(例: <)"));
    else
	putstr(tmpwin, 0, E_J("Or use + and - to select the target directly.",
			      "または、+ と - で対象を直接選んでください。"));
    /* disgusting hack; the alternate selection characters work for any
       getpos call, but they only matter for dowhatis (and doquickwhatis) */
    doing_what_is = (goal == what_is_an_unknown_object);
    Sprintf(sbuf, E_J("Type a .%s when you are at the right place.",
		      "目的の地点を選んだら、.%s を入力してください。"),
            doing_what_is ? E_J(" or , or ; or :"," または , または ; または :") : "");
    putstr(tmpwin, 0, sbuf);
    if (!force)
	putstr(tmpwin, 0, E_J("Type Space or Escape when you're done.",
			      "スペースまたはESCで中止できます。"));
    putstr(tmpwin, 0, "");
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
}

int
getpos(cc, force, goal)
coord *cc;
boolean force;
const char *goal;
{
    int result = 0;
    int cx, cy, i, c;
    int sidx, tx, ty;
    boolean msg_given = TRUE;	/* clear message window by default */
    static const char pick_chars[] = ".,;:";
    const char *cp;
    const char *sdp;
    if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

    if (flags.verbose) {
	pline(E_J("(For instructions type a ?)",
		  "(? で説明を表示)"));
	msg_given = TRUE;
    }
    cx = cc->x;
    cy = cc->y;
#ifdef CLIPPING
    cliparound(cx, cy);
#endif
    curs(WIN_MAP, cx,cy);
    flush_screen(0);
#ifdef MAC
    lock_mouse_cursor(TRUE);
#endif
    for (;;) {
	c = nh_poskey(&tx, &ty, &sidx);
	if (c == '\033') {
	    cx = cy = -10;
	    msg_given = TRUE;	/* force clear */
	    result = -1;
	    break;
	}
	if(c == 0) {
	    if (!isok(tx, ty)) continue;
	    /* a mouse click event, just assign and return */
	    cx = tx;
	    cy = ty;
	    break;
	}
	if ((cp = index(pick_chars, c)) != 0) {
	    /* '.' => 0, ',' => 1, ';' => 2, ':' => 3 */
	    result = cp - pick_chars;
	    break;
	}
	for (i = 0; i < 8; i++) {
	    int dx, dy;

	    if (sdp[i] == c) {
		/* a normal movement letter or digit */
		dx = xdir[i];
		dy = ydir[i];
	    } else if (sdir[i] == lowc((char)c)) {
		/* a shifted movement letter */
		dx = 8 * xdir[i];
		dy = 8 * ydir[i];
	    } else
		continue;

	    /* truncate at map edge; diagonal moves complicate this... */
	    if (cx + dx < 1) {
		dy -= sgn(dy) * (1 - (cx + dx));
		dx = 1 - cx;		/* so that (cx+dx == 1) */
	    } else if (cx + dx > COLNO-1) {
		dy += sgn(dy) * ((COLNO-1) - (cx + dx));
		dx = (COLNO-1) - cx;
	    }
	    if (cy + dy < 0) {
		dx -= sgn(dx) * (0 - (cy + dy));
		dy = 0 - cy;		/* so that (cy+dy == 0) */
	    } else if (cy + dy > ROWNO-1) {
		dx += sgn(dx) * ((ROWNO-1) - (cy + dy));
		dy = (ROWNO-1) - cy;
	    }
	    cx += dx;
	    cy += dy;
	    goto nxtc;
	}

	if(c == '?'){
	    getpos_help(force, goal);
	} else {
	    if (!index(quitchars, c)) {
		char matching[MAXPCHARS];
		int pass, lo_x, lo_y, hi_x, hi_y, k = 0;
		(void)memset((genericptr_t)matching, 0, sizeof matching);
		for (sidx = 1; sidx < MAXPCHARS; sidx++)
		    if (c == defsyms[sidx].sym || c == (int)showsyms[sidx])
			matching[sidx] = (char) ++k;
		if (k) {
		    for (pass = 0; pass <= 1; pass++) {
			/* pass 0: just past current pos to lower right;
			   pass 1: upper left corner to current pos */
			lo_y = (pass == 0) ? cy : 0;
			hi_y = (pass == 0) ? ROWNO - 1 : cy;
			for (ty = lo_y; ty <= hi_y; ty++) {
			    lo_x = (pass == 0 && ty == lo_y) ? cx + 1 : 1;
			    hi_x = (pass == 1 && ty == hi_y) ? cx : COLNO - 1;
			    for (tx = lo_x; tx <= hi_x; tx++) {
				k = levl[tx][ty].glyph;
				if (glyph_is_cmap(k) &&
					matching[glyph_to_cmap(k)]) {
				    cx = tx,  cy = ty;
				    if (msg_given) {
					clear_nhwindow(WIN_MESSAGE);
					msg_given = FALSE;
				    }
				    goto nxtc;
				}
			    }	/* column */
			}	/* row */
		    }		/* pass */
		    pline(E_J("Can't find dungeon feature '%c'.",
			      "'%c'が見つかりません。"), c);
		    msg_given = TRUE;
		    goto nxtc;
		} else {
		    pline(E_J("Unknown direction: '%s' (%s).",
			      "無効な方向です: '%s' (%s)。"),
			  visctrl((char)c),
			  !force ? E_J("aborted","中止しました") :
			  iflags.num_pad ? E_J("use 2468 or .","2468 または . を押す") :
					   E_J("use hjkl or .","hjkl または . を押す"));
		    msg_given = TRUE;
		} /* k => matching */
	    } /* !quitchars */
	    if (force) goto nxtc;
	    pline(E_J("Done.","完了。"));
	    msg_given = FALSE;	/* suppress clear */
	    cx = -1;
	    cy = 0;
	    result = 0;	/* not -1 */
	    break;
	}
    nxtc:	;
#ifdef CLIPPING
	cliparound(cx, cy);
#endif
	curs(WIN_MAP,cx,cy);
	flush_screen(0);
    }
#ifdef MAC
    lock_mouse_cursor(FALSE);
#endif
    if (msg_given) clear_nhwindow(WIN_MESSAGE);
    cc->x = cx;
    cc->y = cy;
    return result;
}


STATIC_OVL
int dxdy_to_quadrant(dx, dy)
int dx, dy;
{
	if (dx >= 0 && dy <  0) return 0;	/*  3|0  */
	if (dx >  0 && dy >= 0) return 1;	/*  -+-  */
	if (dx <= 0 && dy >  0) return 2;	/*  2|1  */
				return 3;
}

STATIC_OVL
void xy_to_radius(gp)
struct gpos *gp;
{
	int r, dx, dy;
	int absx, absy;
	int a, dd;
	dx = gp->x - gp->centerx;
	dy = gp->y - gp->centery;
	if (dx != 0 || dy != 0) {
	    absx = (dx >= 0) ? dx : -dx;
	    absy = (dy >= 0) ? dy : -dy;
	    a = (absx > absy) ? absx : absy;
	    dd = dx*dx + dy*dy;
	    for (r = a; r < (a*3/2/*1.5*/); r++) {
		if (dd >= r*r && dd < (r+1)*(r+1)) {
		    gp->quadrant = dxdy_to_quadrant(dx, dy);
		    gp->radius = r;
		    gp->r2min = r*r;
		    gp->r2max = (r+1)*(r+1);
		    return;
		}
	    }
	}
	gp->quadrant = 0;
	gp->radius = 0;
	gp->r2min = 0;
	gp->r2max = 1;
	return;
}

STATIC_OVL
int getpos_target_at(x, y, tgt)
int x, y, tgt;
{
	int glyph;
	if ((tgt & GETPOS_MONTGT) &&
	    findtarget(x, y)) return 1;
	glyph = glyph_at(x, y);
	if ((tgt & GETPOS_MON) &&
	    ((glyph_is_monster(glyph) && !glyph_is_pet(glyph)) ||
	     glyph_is_warning(glyph))) return 1;
	if ((tgt & GETPOS_OBJ) && glyph_is_object(glyph)) return 1;
	if ((tgt & GETPOS_TRAP) && glyph_is_trap(glyph)) return 1;
	return 0;
}

static const schar vx[4] = {  1,  0, -1,  0 };
static const schar vy[4] = {  0,  1,  0, -1 };

STATIC_OVL
int getprevpos(gp)
struct gpos *gp;
{
    int x, y, q, r;
    int dx, dy, dxx, dyy;
    int qc;
    int rr0, rr1;
retry:
    if (gp->radius < 0) xy_to_radius(gp);	/* radius is unknown... initialize all members */

    x = gp->x;
    y = gp->y;
    q = gp->quadrant;
    r = gp->radius;
    dx = x - gp->centerx;
    dy = y - gp->centery;
    rr0 = gp->r2min;
    rr1 = gp->r2max;

    do {
	qc = 0;
	do {
	    if (dx == 0 || dy == 0) {
		if (q == 0) {				/* move to previous track */
		    r--;
		    if (r < 0) {
			if (gp->r2limit) {
			    for (r = 1; r*r < gp->r2limit; r++);
			    r--;
			    gp->x = gp->centerx;
			    gp->y = gp->centery - r;
			    gp->radius = r;
			    dy = -r;
			    rr0 = r*r;
			    rr1 = (r+1)*(r+1);
			} else {
			    gp->x = (x >= (COLNO-1)/2) ? 1 : COLNO-1;
			    gp->y = (y >= (ROWNO-2)/2) ? 0 : ROWNO-1;
			    gp->radius = -1;
			    goto retry;
			}
		    } else {
			dy++;
			rr0 = r*r;
			rr1 = (r+1)*(r+1);
			y++;
		    }
		}
		q = (q-1) & 3;
		qc++;
		if (qc > 4 || r <= 0) {			/* no available place is found */
		    gp->radius = -1;
		    return 0;
		}
	    }
	    dxx = dx + vy[q];				/* try walking straight */
	    dyy = dy - vx[q];
	    if (dxx*dxx + dyy*dyy >= rr1) {		/* step off the circle? */
		dxx = dx - vx[q];			/* turn left(inside) */
		dyy = dy - vy[q];
		if (dxx*dxx + dyy*dyy < rr0) {		/* inside? */
		    dxx += vy[q];			/* go diagonal */
		    dyy -= vx[q];
		}
	    }
	    dx = dxx;
	    dy = dyy;
	    x = gp->centerx + dx;
	    y = gp->centery + dy;
	} while (!isok(x, y));
    } while (!getpos_target_at(x, y, gp->target));
xit:
    gp->x = x;
    gp->y = y;
    gp->quadrant = q;
    gp->radius = r;
    gp->r2min = rr0;
    gp->r2max = rr1;

    return 1;
}

STATIC_OVL
int getnextpos(gp)
struct gpos *gp;
{
    int x, y, q, r;
    int dx, dy, dxx, dyy;
    int qc;
    int rr0, rr1;

    if (gp->radius < 0) xy_to_radius(gp);	/* radius is unknown... initialize all members */

    x = gp->x;
    y = gp->y;
    q = gp->quadrant;
    r = gp->radius;
    dx = x - gp->centerx;
    dy = y - gp->centery;
    rr0 = gp->r2min;
    rr1 = gp->r2max;

    if (r == 0) {
	y--;
	dy--;
	r++;
	rr0 = 1*1;
	rr1 = 2*2;
	q = 0;
	if (isok(x, y) && getpos_target_at(x, y, gp->target)) goto xit;
    }

    do {
	qc = 0;
	do {
	    dxx = dx + vx[q];				/* try walking straight */
	    dyy = dy + vy[q];
	    if (dxx*dxx + dyy*dyy >= rr1) {		/* step off the circle? */
		dxx = dx - vy[q];			/* turn right(inside) */
		dyy = dy + vx[q];
		if (dxx*dxx + dyy*dyy < rr0) {		/* inside? */
		    dxx += vx[q];			/* go diagonal */
		    dyy += vy[q];
		}
	    }
	    dx = dxx;
	    dy = dyy;
	    x = gp->centerx + dx;
	    y = gp->centery + dy;
	    if (dx == 0 || dy == 0) {
		if (q == 3) {				/* move to next track */
		    r++;
		    dy--;
		    rr0 = r*r;
		    rr1 = (r+1)*(r+1);
		    y--;
		}
		q = (q+1) & 3;
		qc++;
		if (qc >= 4 || (gp->r2limit && rr0 > gp->r2limit)) {	/* no available place is found */
		    gp->radius = -1;
		    return 0;
		}
	    }
	} while (!isok(x, y));
    } while (!getpos_target_at(x, y, gp->target));
xit:
    gp->x = x;
    gp->y = y;
    gp->quadrant = q;
    gp->radius = r;
    gp->r2min = rr0;
    gp->r2max = rr1;

    return 1;
}

STATIC_OVL
void show_what_is_here(gp, clear_msg)
struct gpos *gp;
boolean clear_msg;
{
    char buf[BUFSZ];
    buf[0] = 0;
    if (gp->target & (GETPOS_MON|GETPOS_OBJ)) {
	char monbuf[BUFSZ];
	char temp_buf[BUFSZ];

	lookat(gp->x, gp->y, temp_buf, monbuf);
#ifndef JP
	Sprintf(buf, "%s.", An(temp_buf));
#else
	Sprintf(buf, "%s。", temp_buf);
#endif /*JP*/
    } else if (gp->target & GETPOS_TRAP) {
	struct trap *ttmp;
	int tt;
	ttmp = t_at(gp->x, gp->y);
	if (ttmp) { /* picked from pager.c */
	    tt = ttmp->ttyp;
#ifndef JP
	    Sprintf(buf, "%s%s%s.",
		    An(defsyms[trap_to_defsym(tt)].explanation),
		    !ttmp->madeby_u ? "" : (tt == WEB) ? " woven" :
		    /* trap doors & spiked pits can't be made by
		       player, and should be considered at least
		       as much "set" as "dug" anyway */
		    (tt == HOLE || tt == PIT) ? " dug" : " set",
		    !ttmp->madeby_u ? "" : " by you");
#else
	    Sprintf(buf, "%s%s%s。",
		    !ttmp->madeby_u ? "" : "あなたが",
		    !ttmp->madeby_u ? "" : (tt == WEB) ? "張った" :
		    (tt == HOLE || tt == PIT) ? "掘った" : "仕掛けた",
		    defsyms[trap_to_defsym(tt)].explanation);
#endif /*JP*/
	}
    }
    if (buf[0]) {
	if (clear_msg) clear_nhwindow(WIN_MESSAGE);
	pline(buf);
    }
}

int
getpos2(cc, rangelimit, tgt, goal)
coord *cc;
int rangelimit;
int tgt;
const char *goal;
{
    int result = 0;
    int cx, cy, i, c;
    int sidx, tx, ty;
    struct gpos gp;	/*test*/
    boolean msg_given = TRUE;	/* clear message window by default */
    static const char pick_chars[] = ".,;:";
    const char *cp;
    const char *sdp;
    if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

    cx = cc->x;
    cy = cc->y;
    /*test*/
    gp.centerx = cx;
    gp.centery = cy;
    gp.x = cx;
    gp.y = cy;
    gp.radius = -1;
    gp.r2limit = rangelimit;
    gp.target = tgt;
    if (getnextpos(&gp)) {
	cx = gp.x;
	cy = gp.y;
	show_what_is_here(&gp, msg_given);
	msg_given = TRUE;
    } else if (flags.verbose) {
	pline(E_J("(For instructions type a ?)",
		  "(? で説明を表示)"));
	msg_given = TRUE;
    }

    /*test*/
#ifdef CLIPPING
    cliparound(cx, cy);
#endif
    curs(WIN_MAP, cx,cy);
    flush_screen(0);
#ifdef MAC
    lock_mouse_cursor(TRUE);
#endif
    for (;;) {
	c = nh_poskey(&tx, &ty, &sidx);
	if (c == '\033') {
	    cx = cy = -10;
	    msg_given = TRUE;	/* force clear */
	    result = -1;
	    break;
	}
	if(c == 0) {
	    if (!isok(tx, ty)) continue;
	    /* a mouse click event, just assign and return */
	    cx = tx;
	    cy = ty;
	    break;
	}
	if (index(pick_chars, c) != 0 ||
	    index(quitchars, c)) {
	    result = 0;
	    break;
	}
	if (c == '+' || c == '-') {
	    gp.x = cx;
	    gp.y = cy;
	    if ((c == '+') ? getnextpos(&gp) : getprevpos(&gp)) {
		cx = gp.x;
		cy = gp.y;
		show_what_is_here(&gp, msg_given);
		msg_given = TRUE;
	    } else {
		cx = gp.centerx;
		cy = gp.centery;
		gp.radius = -1;
	    }
	    goto nxtc;
	}
	gp.radius = -1;
	for (i = 0; i < 8; i++) {
	    int dx, dy;

	    if (sdp[i] == c) {
		/* a normal movement letter or digit */
		dx = xdir[i];
		dy = ydir[i];
	    } else if (sdir[i] == lowc((char)c)) {
		/* a shifted movement letter */
		dx = 8 * xdir[i];
		dy = 8 * ydir[i];
	    } else
		continue;
	    /* truncate at map edge; diagonal moves complicate this... */
	    if (cx + dx < 1) {
		dy -= sgn(dy) * (1 - (cx + dx));
		dx = 1 - cx;		/* so that (cx+dx == 1) */
	    } else if (cx + dx > COLNO-1) {
		dy += sgn(dy) * ((COLNO-1) - (cx + dx));
		dx = (COLNO-1) - cx;
	    }
	    if (cy + dy < 0) {
		dx -= sgn(dx) * (0 - (cy + dy));
		dy = 0 - cy;		/* so that (cy+dy == 0) */
	    } else if (cy + dy > ROWNO-1) {
		dx += sgn(dx) * ((ROWNO-1) - (cy + dy));
		dy = (ROWNO-1) - cy;
	    }
	    cx += dx;
	    cy += dy;
	    goto nxtc;
	}

	if(c == '?'){
	    if (msg_given) clear_nhwindow(WIN_MESSAGE);
	    getpos_help(2, goal);
	} else {
	    pline(E_J("Unknown direction: '%s' (%s).",
		      "無効な方向です: '%s' (%s)。"),
		  visctrl((char)c),
		  iflags.num_pad ? E_J("use 2468 or +-.","2468 または +-. を押す") :
				   E_J("use hjkl or +-.","hjkl または +-. を押す"));
	    msg_given = TRUE;
	}
    nxtc:	;
#ifdef CLIPPING
	cliparound(cx, cy);
#endif
	curs(WIN_MAP,cx,cy);
	flush_screen(0);
    }
#ifdef MAC
    lock_mouse_cursor(FALSE);
#endif
    if (msg_given) clear_nhwindow(WIN_MESSAGE);
    cc->x = cx;
    cc->y = cy;
    return result;
}

int
getnearestpos(cc, rangelimit, tgt)
coord *cc;
int rangelimit;
int tgt;
{
    struct gpos gp;
    gp.centerx = cc->x;
    gp.centery = cc->y;
    gp.x = cc->x;
    gp.y = cc->y;
    gp.radius = -1;
    gp.r2limit = rangelimit;
    gp.target = tgt;
    if (getnextpos(&gp)) {
	cc->x = gp.x;
	cc->y = gp.y;
	return 1;
    }
    return 0;
}

/*---------------------------------------------------------*/

struct monst *
christen_monst(mtmp, name)
struct monst *mtmp;
const char *name;
{
	int lth;
	struct monst *mtmp2;
	char buf[PL_PSIZ];

	/* dogname & catname are PL_PSIZ arrays; object names have same limit */
	lth = *name ? (int)(strlen(name) + 1) : 0;
	if(lth > PL_PSIZ){
		lth = PL_PSIZ;
		name = strncpy(buf, name, PL_PSIZ - 1);
		buf[PL_PSIZ - 1] = '\0';
	}
	if (lth == mtmp->mnamelth) {
		/* don't need to allocate a new monst struct */
		if (lth) Strcpy(NAME(mtmp), name);
		return mtmp;
	}
	mtmp2 = newmonst(mtmp->mxlth + lth);
	*mtmp2 = *mtmp;
	(void) memcpy((genericptr_t)mtmp2->mextra,
		      (genericptr_t)mtmp->mextra, mtmp->mxlth);
	mtmp2->mnamelth = lth;
	if (lth) Strcpy(NAME(mtmp2), name);
	replmon(mtmp,mtmp2);
	return(mtmp2);
}

int
do_mname()
{
	char buf[BUFSZ];
	coord cc;
	register int cx,cy;
	register struct monst *mtmp;
	char qbuf[QBUFSZ];

	if (Hallucination) {
		You(E_J("would never recognize it anyway.",
			"どっちにしろ、そいつの顔を思い出せないだろう。"));
		return 0;
	}
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, FALSE, E_J("the monster you want to name",
				   "名前をつけたい怪物")) < 0 ||
			(cx = cc.x) < 0)
		return 0;
	cy = cc.y;

	if (cx == u.ux && cy == u.uy) {
#ifdef STEED
	    if (u.usteed && canspotmon(u.usteed))
		mtmp = u.usteed;
	    else {
#endif
		pline(E_J("This %s creature is called %s and cannot be renamed.",
			  "この%s生物は%sと呼ばれていて、名前は変えられない。"),
		ACURR(A_CHA) > 14 ?
		(flags.female ? E_J("beautiful","美しい") : E_J("handsome","男前の")) :
		E_J("ugly","醜い"),
		plname);
		return(0);
#ifdef STEED
	    }
#endif
	} else
	    mtmp = m_at(cx, cy);

	if (glyph_is_warning(glyph_at(cx, cy))) {
		pline(E_J("You cannot identify the monster.","その怪物は正体が判明していない。"));
		return(0);
	}

	if (!mtmp || (!sensemon(mtmp) &&
			(!(cansee(cx,cy) || see_with_infrared(mtmp)) || mtmp->mundetected
			|| mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT
			|| (mtmp->minvis && !See_invisible)))) {
		pline(E_J("I see no monster there.","そこに怪物はいないようだ。"));
		return(0);
	}
	/* special case similar to the one in lookat() */
	(void) distant_monnam(mtmp, ARTICLE_THE, buf);
	Sprintf(qbuf, E_J("What do you want to call %s?",
			  "%sを何と呼びますか？"), buf);
	getlin(qbuf,buf);
	if(!*buf || *buf == '\033') return(0);
	/* strip leading and trailing spaces; unnames monster if all spaces */
	(void)mungspaces(buf);

	if (mtmp->data->geno & G_UNIQ)
	    pline(E_J("%s doesn't like being called names!",
		      "%sは名前をつけられることを良しとしない！"), Monnam(mtmp));
	else
	    (void) christen_monst(mtmp, buf);
	return(0);
}

/*
 * This routine changes the address of obj. Be careful not to call it
 * when there might be pointers around in unknown places. For now: only
 * when obj is in the inventory.
 */
STATIC_OVL
void
do_oname(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	const char *aname;
	short objtyp;

#ifndef JP
	Sprintf(qbuf, "What do you want to name %s %s?",
		is_plural(obj) ? "these" : "this", xname(obj));
#else
	Sprintf(qbuf, "%sに何と名前をつけますか？", xname(obj));
#endif /*JP*/
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')	return;
	/* strip leading and trailing spaces; unnames item if all spaces */
	(void)mungspaces(buf);

	/* relax restrictions over proper capitalization for artifacts */
	if ((aname = artifact_name(buf, &objtyp)) != 0 && objtyp == obj->otyp)
		Strcpy(buf, aname);

	if (obj->oartifact) {
		pline_The(E_J("artifact seems to resist the attempt.",
			      "アーティファクトはあなたの試みに抵抗した。"));
		return;
	} else if (restrict_name(obj, buf) || exist_artifact(obj->otyp, buf)) {
		int n = rn2((int)strlen(buf));
		register char c1, c2;
#ifdef JP
		/* 全角文字には jrubout() を使い、消えてしまったら適当に英文字を追加 */
		if (!jrubout(buf, n) || buf[n] == ' ')
#endif /*JP*/
		{
		    c1 = lowc(buf[n]);
		    do c2 = 'a' + rn2('z'-'a'); while (c1 == c2);
		    buf[n] = (buf[n] == c1) ? c2 : highc(c2);  /* keep same case */
		}
		pline(E_J("While engraving your %s slips.",
			  "銘を刻む途中で、あなたの%sが滑った。"), body_part(HAND));
		display_nhwindow(WIN_MESSAGE, FALSE);
		You(E_J("engrave: \"%s\".","銘をつけた:『%s』。"),buf);
	}
	obj = oname(obj, buf);
}

/*
 * Allocate a new and possibly larger storage space for an obj.
 */
struct obj *
realloc_obj(obj, oextra_size, oextra_src, oname_size, name)
struct obj *obj;
int oextra_size;		/* storage to allocate for oextra            */
genericptr_t oextra_src;
int oname_size;			/* size of name string + 1 (null terminator) */
const char *name;
{
	struct obj *otmp;

	otmp = newobj(oextra_size + oname_size);
	*otmp = *obj;	/* the cobj pointer is copied to otmp */
	if (oextra_size) {
	    if (oextra_src)
		(void) memcpy((genericptr_t)otmp->oextra, oextra_src,
							oextra_size);
	} else {
	    otmp->oattached = OATTACHED_NOTHING;
	}
	otmp->oxlth = oextra_size;

	otmp->onamelth = oname_size;
	otmp->timed = 0;	/* not timed, yet */
	otmp->lamplit = 0;	/* ditto */
	/* __GNUC__ note:  if the assignment of otmp->onamelth immediately
	   precedes this `if' statement, a gcc bug will miscompile the
	   test on vax (`insv' instruction used to store bitfield does
	   not set condition codes, but optimizer behaves as if it did).
	   gcc-2.7.2.1 finally fixed this. */
	if (oname_size) {
	    if (name)
		Strcpy(ONAME(otmp), name);
	}

	if (obj->owornmask) {
		boolean save_twoweap = u.twoweap;
		/* unwearing the old instance will clear dual-wield mode
		   if this object is either of the two weapons */
		setworn((struct obj *)0, obj->owornmask);
		setworn(otmp, otmp->owornmask);
		u.twoweap = save_twoweap;
	}

	/* replace obj with otmp */
	replace_object(obj, otmp);

	/* fix ocontainer pointers */
	if (Has_contents(obj)) {
		struct obj *inside;

		for(inside = obj->cobj; inside; inside = inside->nobj)
			inside->ocontainer = otmp;
	}

	/* move timers and light sources from obj to otmp */
	if (obj->timed) obj_move_timers(obj, otmp);
	if (obj->lamplit) obj_move_light_source(obj, otmp);

	/* objects possibly being manipulated by multi-turn occupations
	   which have been interrupted but might be subsequently resumed */
	if (obj->oclass == FOOD_CLASS)
	    food_substitution(obj, otmp);	/* eat food or open tin */
	else if (obj->oclass == SPBOOK_CLASS)
	    book_substitution(obj, otmp);	/* read spellbook */

	/* obfree(obj, otmp);	now unnecessary: no pointers on bill */
	dealloc_obj(obj);	/* let us hope nobody else saved a pointer */
	return otmp;
}

struct obj *
oname(obj, name)
struct obj *obj;
const char *name;
{
	int lth;
	char buf[PL_PSIZ];

	lth = *name ? (int)(strlen(name) + 1) : 0;
	if (lth > PL_PSIZ) {
		lth = PL_PSIZ;
		name = strncpy(buf, name, PL_PSIZ - 1);
		buf[PL_PSIZ - 1] = '\0';
	}
	/* If named artifact exists in the game, do not create another.
	 * Also trying to create an artifact shouldn't de-artifact
	 * it (e.g. Excalibur from prayer). In this case the object
	 * will retain its current name. */
	if (obj->oartifact || (lth && exist_artifact(obj->otyp, name)))
		return obj;

	if (lth == obj->onamelth) {
		/* no need to replace entire object */
		if (lth) Strcpy(ONAME(obj), name);
	} else {
		obj = realloc_obj(obj, obj->oxlth,
			      (genericptr_t)obj->oextra, lth, name);
	}
	if (lth) artifact_exists(obj, name, TRUE);
	if (obj->oartifact) {
	    /* can't dual-wield with artifact as secondary weapon */
	    if (obj == uswapwep) untwoweapon();
	    /* activate warning if you've just named your weapon "Sting" */
	    if (obj == uwep) set_artifact_intrinsic(obj, TRUE, W_WEP);
	}
	if (carried(obj)) update_inventory();
	return obj;
}

static NEARDATA const char callable[] = {
	SCROLL_CLASS, POTION_CLASS, WAND_CLASS, RING_CLASS, AMULET_CLASS,
	GEM_CLASS, SPBOOK_CLASS, ARMOR_CLASS, TOOL_CLASS, 0 };

int
ddocall()
{
	register struct obj *obj;
#ifdef REDO
	char	ch;
#endif
	char allowall[2];
#ifdef JP
	static const struct getobj_words namew = { 0, "に", "名前をつける", "名前をつけ" };
	static const struct getobj_words callw = { 0, "に", "識別名をつける", "識別名をつけ" };
#endif /*JP*/

	switch(
#ifdef REDO
		ch =
#endif
		ynq(E_J("Name an individual object?",
			"個々の品物に名前をつけますか？"))) {
	case 'q':
		break;
	case 'y':
#ifdef REDO
		savech(ch);
#endif
		allowall[0] = ALL_CLASSES; allowall[1] = '\0';
		obj = getobj(allowall, E_J("name",&namew), 0);
		if(obj) do_oname(obj);
		break;
	default :
#ifdef REDO
		savech(ch);
#endif
		obj = getobj(callable, E_J("call",&callw), 0);
		if (obj) {
			/* behave as if examining it in inventory;
			   this might set dknown if it was picked up
			   while blind and the hero can now see */
			(void) xname(obj);

			if (!obj->dknown) {
				You(E_J("would never recognize another one.",
					"品物を区別するためには、それを観察できる必要がある。"));
				return 0;
			}
			docall(obj);
		}
		break;
	}
	return 0;
}

void
docall(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	struct obj otemp;
	register char **str1;

	if (!obj->dknown) return; /* probably blind */
	otemp = *obj;
	otemp.quan = 1L;
	otemp.onamelth = 0;
	otemp.oxlth = 0;
	if (objects[otemp.otyp].oc_class == POTION_CLASS && otemp.fromsink)
	    /* kludge, meaning it's sink water */
#ifndef JP
	    Sprintf(qbuf,"Call a stream of %s fluid:",
		    OBJ_DESCR(objects[otemp.otyp]));
#else
	    Sprintf(qbuf,"%s液体の流れを何と呼びますか:",
		    JOBJ_DESCR(objects[otemp.otyp]));
#endif /*JP*/
	else
#ifndef JP
	    Sprintf(qbuf, "Call %s:", an(xname(&otemp)));
#else
	    Sprintf(qbuf, E_J("Call %s:","%sを何と呼びますか: "), xname(&otemp));
#endif /*JP*/
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')
		return;

	/* clear old name */
	str1 = &(objects[obj->otyp].oc_uname);
	if(*str1) free((genericptr_t)*str1);

	/* strip leading and trailing spaces; uncalls item if all spaces */
	(void)mungspaces(buf);
	if (!*buf) {
	    if (*str1) {	/* had name, so possibly remove from disco[] */
		/* strip name first, for the update_inventory() call
		   from undiscover_object() */
		*str1 = (char *)0;
		undiscover_object(obj->otyp);
	    }
	} else {
	    *str1 = strcpy((char *) alloc((unsigned)strlen(buf)+1), buf);
	    discover_object(obj->otyp, FALSE, TRUE); /* possibly add to disco[] */
	}
}

#endif /*OVLB*/
#ifdef OVL0

static const char * const ghostnames[] = {
	/* these names should have length < PL_NSIZ */
	/* Capitalize the names for aesthetics -dgk */
	"Adri", "Andries", "Andreas", "Bert", "David", "Dirk", "Emile",
	"Frans", "Fred", "Greg", "Hether", "Jay", "John", "Jon", "Karnov",
	"Kay", "Kenny", "Kevin", "Maud", "Michiel", "Mike", "Peter", "Robert",
	"Ron", "Tom", "Wilmar", "Nick Danger", "Phoenix", "Jiro", "Mizue",
	"Stephan", "Lance Braccus", "Shadowhawk"
};

/* ghost names formerly set by x_monnam(), now by makemon() instead */
const char *
rndghostname()
{
    return rn2(7) ? ghostnames[rn2(SIZE(ghostnames))] : (const char *)plname;
}

/* Monster naming functions:
 * x_monnam is the generic monster-naming function.
 *		  seen	      unseen	   detected		  named
 * mon_nam:	the newt	it	the invisible orc	Fido
 * noit_mon_nam:the newt (as if detected) the invisible orc	Fido
 * l_monnam:	newt		it	invisible orc		dog called fido
 * Monnam:	The newt	It	The invisible orc	Fido
 * noit_Monnam: The newt (as if detected) The invisible orc	Fido
 * Adjmonnam:	The poor newt	It	The poor invisible orc	The poor Fido
 * Amonnam:	A newt		It	An invisible orc	Fido
 * a_monnam:	a newt		it	an invisible orc	Fido
 * m_monnam:	newt		xan	orc			Fido
 * y_monnam:	your newt     your xan	your invisible orc	Fido
 */

/* Bug: if the monster is a priest or shopkeeper, not every one of these
 * options works, since those are special cases.
 */
char *
x_monnam(mtmp, article, adjective, suppress, called)
register struct monst *mtmp;
int article;
/* ARTICLE_NONE, ARTICLE_THE, ARTICLE_A: obvious
 * ARTICLE_YOUR: "your" on pets, "the" on everything else
 *
 * If the monster would be referred to as "it" or if the monster has a name
 * _and_ there is no adjective, "invisible", "saddled", etc., override this
 * and always use no article.
 */
const char *adjective;
int suppress;
/* SUPPRESS_IT, SUPPRESS_INVISIBLE, SUPPRESS_HALLUCINATION, SUPPRESS_SADDLE.
 * EXACT_NAME: combination of all the above
 */
boolean called;
{
#ifdef LINT	/* static char buf[BUFSZ]; */
	char buf[BUFSZ];
#else
	static char buf[BUFSZ];
#endif
	struct permonst *mdat = mtmp->data;
	boolean do_hallu, do_invis, do_it, do_saddle;
	boolean name_at_start, has_adjectives;
	char *bp;
	const char *mn = E_J(mdat->mname, JMONNAM(monsndx(mdat)));

	if (program_state.gameover)
	    suppress |= SUPPRESS_HALLUCINATION;
	if (article == ARTICLE_YOUR && !mtmp->mtame)
	    article = ARTICLE_THE;

	do_hallu = Hallucination && !(suppress & SUPPRESS_HALLUCINATION);
	do_invis = mtmp->minvis && !(suppress & SUPPRESS_INVISIBLE);
	do_it = !canspotmon(mtmp) && 
	    article != ARTICLE_YOUR &&
	    !program_state.gameover &&
#ifdef STEED
	    mtmp != u.usteed &&
#endif
	    !(u.uswallow && mtmp == u.ustuck) &&
	    !(suppress & SUPPRESS_IT);
	do_saddle = !(suppress & SUPPRESS_SADDLE);

	buf[0] = 0;

#ifdef JP
	/*  */
	if (mtmp->female && mdat == &mons[PM_HIGH_PRIEST]) mn = "女教皇";
#endif /*JP*/

	/* unseen monsters, etc.  Use "it" */
	if (do_it) {
	    Strcpy(buf, E_J("it","何か"));
	    return buf;
	}

	/* priests and minions: don't even use this function */
	if (mtmp->ispriest || mtmp->isminion) {
	    char priestnambuf[BUFSZ];
	    char *name;
	    long save_prop = EHalluc_resistance;
	    unsigned save_invis = mtmp->minvis;

	    /* when true name is wanted, explicitly block Hallucination */
	    if (!do_hallu) EHalluc_resistance = 1L;
	    if (!do_invis) mtmp->minvis = 0;
	    name = priestname(mtmp, priestnambuf);
	    EHalluc_resistance = save_prop;
	    mtmp->minvis = save_invis;
#ifndef JP
	    if (article == ARTICLE_NONE && !strncmp(name, "the ", 4))
		name += 4;
#endif /*JP*/
	    return strcpy(buf, name);
	}

	/* Shopkeepers: use shopkeeper name.  For normal shopkeepers, just
	 * "Asidonhopo"; for unusual ones, "Asidonhopo the invisible
	 * shopkeeper" or "Asidonhopo the blue dragon".  If hallucinating,
	 * none of this applies.
	 */
	if (mtmp->isshk && !do_hallu) {
	    if (adjective && article == ARTICLE_THE) {
		/* pathological case: "the angry Asidonhopo the blue dragon"
		   sounds silly */
#ifndef JP
		Strcpy(buf, "the ");
		Strcat(strcat(buf, adjective), " ");
		Strcat(buf, shkname(mtmp));
#else /*JP*/
		Sprintf(buf, "%s%s", adjective, shkname(mtmp));
#endif /*JP*/
		return buf;
	    }
	    Strcat(buf, shkname(mtmp));
	    if (mdat == &mons[PM_SHOPKEEPER] && !do_invis)
		return buf;
	    Strcat(buf, E_J(" the ","という名の"));
	    if (do_invis)
		Strcat(buf, E_J("invisible ","姿の見えない"));
	    Strcat(buf, mn);
	    return buf;
	}

	/* Put the adjectives in the buffer */
	if (adjective)
#ifndef JP
	    Strcat(strcat(buf, adjective), " ");
#else /*JP*/
	    Strcat(buf, adjective);
#endif /*JP*/
	if (do_invis)
	    Strcat(buf, E_J("invisible ","姿の見えない"));
#ifdef MONSTEED
	if (do_saddle && is_mridden(mtmp) &&
	    canspotmon(mtmp->mlink) && !Blind) {
	    char buf2[BUFSZ];
	    Strcpy(buf2, buf);
	    Sprintf(eos(buf2), E_J("%s ","%s"),
		    s_suffix(x_monnam(mtmp->mlink, ARTICLE_THE, (char *)0,
				      SUPPRESS_INVISIBLE, FALSE)));
	    Strcpy(buf, buf2);
	    article = ARTICLE_NONE;
	} else
#endif /*MONSTEED*/
#ifdef STEED
	if (do_saddle && (mtmp->misc_worn_check & W_SADDLE) &&
	    !Blind && !Hallucination) {
	    Strcat(buf, E_J("saddled ","鞍をのせた"));
	}
#endif
	if (buf[0] != 0)
	    has_adjectives = TRUE;
	else
	    has_adjectives = FALSE;

	/* Put the actual monster name or type into the buffer now */
	/* Be sure to remember whether the buffer starts with a name */
	if (do_hallu) {
	    Strcat(buf, rndmonnam());
	    name_at_start = FALSE;
	} else if (mtmp->mnamelth) {
	    char *name = NAME(mtmp);

	    if (mdat == &mons[PM_GHOST]) {
		Sprintf(eos(buf), E_J("%s ghost","%s幽霊"), s_suffix(name));
		name_at_start = TRUE;
	    } else if (called) {
#ifndef JP
		Sprintf(eos(buf), "%s called %s", mn, name);
#else /*JP*/
		Sprintf(eos(buf), "%sという名の%s", name, mn);
#endif /*JP*/
		name_at_start = (boolean)type_is_pname(mdat);
#ifndef JP
	    } else if (is_mplayer(mdat) && (bp = strstri(name, " the ")) != 0) {
		/* <name> the <adjective> <invisible> <saddled> <rank> */
		char pbuf[BUFSZ];

		Strcpy(pbuf, name);
		pbuf[bp - name + 5] = '\0'; /* adjectives right after " the " */
		if (has_adjectives)
		    Strcat(pbuf, buf);
		Strcat(pbuf, bp + 5);	/* append the rest of the name */
		Strcpy(buf, pbuf);
		article = ARTICLE_NONE;
		name_at_start = TRUE;
#endif /*JP*/
	    } else {
		Strcat(buf, name);
		name_at_start = TRUE;
	    }
	} else if (is_mplayer(mdat)) {
	    char pbuf[BUFSZ];
	    if (mtmp->cham == CHAM_DOPPELGANGER &&
		mdat == &mons[u.umonster]) {
		Strcat(buf, plname);
		name_at_start = TRUE;
	    } else if (!In_endgame(&u.uz)) {
		Strcpy(pbuf, rank_of((int)mtmp->m_lev,
				 monsndx(mdat),
				 (boolean)mtmp->female));
		Strcat(buf, lcase(pbuf));
		name_at_start = FALSE;
	    }
	} else {
	    Strcat(buf, mn);
	    name_at_start = (boolean)type_is_pname(mdat);
	}

	if (name_at_start && (article == ARTICLE_YOUR || !has_adjectives)) {
	    if (mdat == &mons[PM_WIZARD_OF_YENDOR])
		article = ARTICLE_THE;
	    else
		article = ARTICLE_NONE;
	} else if ((mdat->geno & G_UNIQ) && article == ARTICLE_A) {
	    article = ARTICLE_THE;
	}

	{
	    char buf2[BUFSZ];

	    switch(article) {
		case ARTICLE_YOUR:
		    Strcpy(buf2, E_J("your ","あなたの"));
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
#ifndef JP
		case ARTICLE_THE:
		    Strcpy(buf2, "the ");
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
		case ARTICLE_A:
		    return(an(buf));
#endif /*JP*/
		case ARTICLE_NONE:
		default:
		    return buf;
	    }
	}
}

#endif /* OVL0 */
#ifdef OVLB

char *
l_monnam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_NONE, (char *)0, 
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, TRUE));
}

#endif /* OVLB */
#ifdef OVL0

char *
mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_THE, (char *)0,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE));
}

/* print the name as if mon_nam() was called, but assume that the player
 * can always see the monster--used for probing and for monsters aggravating
 * the player with a cursed potion of invisibility
 */
char *
noit_mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_THE, (char *)0,
		mtmp->mnamelth ? (SUPPRESS_SADDLE|SUPPRESS_IT) :
		    SUPPRESS_IT, FALSE));
}

char *
Monnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = mon_nam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

char *
noit_Monnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = noit_mon_nam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

/* monster's own name */
char *
m_monnam(mtmp)
struct monst *mtmp;
{
	return x_monnam(mtmp, ARTICLE_NONE, (char *)0, EXACT_NAME, FALSE);
}

/* pet name: "your little dog" */
char *
y_monnam(mtmp)
struct monst *mtmp;
{
	int prefix, suppression_flag;

	prefix = mtmp->mtame ? ARTICLE_YOUR : ARTICLE_THE;
	suppression_flag = (mtmp->mnamelth
#ifdef STEED
			    /* "saddled" is redundant when mounted */
			    || mtmp == u.usteed
#endif
			    ) ? SUPPRESS_SADDLE : 0;

	return x_monnam(mtmp, prefix, (char *)0, suppression_flag, FALSE);
}

#endif /* OVL0 */
#ifdef OVLB

char *
Adjmonnam(mtmp, adj)
register struct monst *mtmp;
register const char *adj;
{
	register char *bp = x_monnam(mtmp, ARTICLE_THE, adj,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE);

	*bp = highc(*bp);
	return(bp);
}

char *
a_monnam(mtmp)
register struct monst *mtmp;
{
	return x_monnam(mtmp, ARTICLE_A, (char *)0,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE);
}

char *
Amonnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = a_monnam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

/* used for monster ID by the '/', ';', and 'C' commands to block remote
   identification of the endgame altars via their attending priests */
char *
distant_monnam(mon, article, outbuf)
struct monst *mon;
int article;	/* only ARTICLE_NONE and ARTICLE_THE are handled here */
char *outbuf;
{
    /* high priest(ess)'s identity is concealed on the Astral Plane,
       unless you're adjacent (overridden for hallucination which does
       its own obfuscation) */
    if (mon->data == &mons[PM_HIGH_PRIEST] && !Hallucination &&
	    Is_astralevel(&u.uz) && distu(mon->mx, mon->my) > 2) {
#ifndef JP
	Strcpy(outbuf, article == ARTICLE_THE ? "the " : "");
	Strcat(outbuf, mon->female ? "high priestess" : "high priest");
#else
	Strcpy(outbuf, mon->female ? "女教皇" : "法王");
#endif /*JP*/
    } else {
	Strcpy(outbuf, x_monnam(mon, article, (char *)0, 0, TRUE));
    }
    return outbuf;
}

static const char * const bogusmons[] = {
#ifndef JP
	"jumbo shrimp", "giant pigmy", "gnu", "killer penguin",
	"giant cockroach", "giant slug", "maggot", "pterodactyl",
	"tyrannosaurus rex", "basilisk", "beholder", "nightmare",
	"efreeti", "marid", "rot grub", "bookworm", "master lichen",
	"shadow", "hologram", "jester", "attorney", "sleazoid",
	"killer tomato", "amazon", "robot", "battlemech",
	"rhinovirus", "harpy", "lion-dog", "rat-ant", "Y2K bug",
						/* misc. */
	"grue", "Christmas-tree monster", "luck sucker", "paskald",
	"brogmoid", "dornbeast",		/* Quendor (Zork, &c.) */
	"Ancient Multi-Hued Dragon", "Evil Iggy",
						/* Moria */
	"emu", "kestrel", "xeroc", "venus flytrap",
						/* Rogue */
	"creeping coins",			/* Wizardry */
	"hydra", "siren",			/* Greek legend */
	"killer bunny",				/* Monty Python */
	"rodent of unusual size",		/* The Princess Bride */
	"Smokey the bear",	/* "Only you can prevent forest fires!" */
	"Luggage",				/* Discworld */
	"Ent",					/* Lord of the Rings */
	"tangle tree", "nickelpede", "wiggle",	/* Xanth */
	"white rabbit", "snark",		/* Lewis Carroll */
	"pushmi-pullyu",			/* Dr. Doolittle */
	"smurf",				/* The Smurfs */
	"tribble", "Klingon", "Borg",		/* Star Trek */
	"Ewok",					/* Star Wars */
	"Totoro",				/* Tonari no Totoro */
	"ohmu",					/* Nausicaa */
	"youma",				/* Sailor Moon */
	"nyaasu",				/* Pokemon (Meowth) */
	"Godzilla", "King Kong",		/* monster movies */
	"earthquake beast",			/* old L of SH */
	"Invid",				/* Robotech */
	"Terminator",				/* The Terminator */
	"boomer",				/* Bubblegum Crisis */
	"Dalek",				/* Dr. Who ("Exterminate!") */
	"microscopic space fleet", "Ravenous Bugblatter Beast of Traal",
						/* HGttG */
	"teenage mutant ninja turtle",		/* TMNT */
	"samurai rabbit",			/* Usagi Yojimbo */
	"aardvark",				/* Cerebus */
	"Audrey II",				/* Little Shop of Horrors */
	"witch doctor", "one-eyed one-horned flying purple people eater",
						/* 50's rock 'n' roll */
	"Barney the dinosaur",			/* saccharine kiddy TV */
	"Morgoth",				/* Angband */
	"Vorlon",				/* Babylon 5 */
	"questing beast",		/* King Arthur */
	"Predator",				/* Movie */
	"mother-in-law"				/* common pest */
#else
	"巨大甘えび", "巨大な小人", "ヌー", "殺人ペンギン",
	"巨大ゴキブリ", "大ナメクジ", "ウジ虫", "プテラノドン",
	"ティラノサウルス・レックス", "バシリスク", "ビホルダー", "ナイトメア",
	"イフリート", "マリッド", "腐れ地虫", "本の虫", "アーチノッチ",
	"シャドウ", "ホログラム", "道化師", "弁護士", "俗物",
	"キラートマト", "アマゾネス", "ロボット", "バトルメック",
	"風邪の菌", "ハーピー", "狛犬", "ネズミ蟻", "2000年問題",
						/* misc. */
	"グルー", "クリスマスツリーの怪物", "疫病神", "パスカルド",
	"ブログモイド", "ドルンビースト",	/* Quendor (Zork, &c.) */
	"古代万色ドラゴン", "邪悪なるイギー",
						/* Moria */
	"エミュー", "チョウゲンボウ", "ゼロック", "ハエトリグサ",
						/* Rogue */
	"クリーピングコイン",			/* Wizardry */
	"ヒドラ", "セイレーン",			/* Greek legend */
	"凶悪ウサちゃん",			/* Monty Python */
	"超デカい齧歯類",			/* The Princess Bride */
	"スモーキー・ザ・ベア",			/* "Only you can prevent forest fires!" */
	"ラゲージ",				/* Discworld */
	"エント",				/* Lord of the Rings */
	"触手木", "ニッケルさそり", "ぴくぴく虫",	/* Xanth */
	"白うさぎ", "スナーク",			/* Lewis Carroll */
	"オシツオサレツ",			/* Dr. Doolittle */
	"スマーフ",				/* The Smurfs */
	"トリブル", "クリンゴン", "ボーグ",	/* Star Trek */
	"イウォーク",				/* Star Wars */
	"トトロ",				/* Tonari no Totoro */
	"王蟲",					/* Nausicaa */
	"妖魔",					/* Sailor Moon */
	"ニャース",				/* Pokemon (Meowth) */
	"ゴジラ", "キングコング",		/* monster movies */
	"大ナマズ",				/* old L of SH */
	"インビッド",				/* Robotech */
	"ターミネーター",			/* The Terminator */
	"ブーマー",				/* Bubblegum Crisis */
	"ダレク",				/* Dr. Who ("Exterminate!") */
	"超微細宇宙艦隊", "がつがつむしゃむしゃトラアル獣",
						/* HGttG */
	"ミュータント・ニンジャ・タートルズ",	/* TMNT */
	"侍兎",					/* Usagi Yojimbo */
	"ツチブタ",				/* Cerebus */
	"オードリーII",				/* Little Shop of Horrors */
	"呪術医", "一つ目一角空飛ぶ紫食人族",
						/* 50's rock 'n' roll */
	"恐竜バーニーちゃん",			/* saccharine kiddy TV */
	"モルゴス",				/* Angband */
	"ヴォーロン",				/* Babylon 5 */
	"探求のけもの",				/* King Arthur */
	"プレデター",				/* Movie */
	"義理の母",				/* common pest */

	"ののワさん"				/* THE IDOLM@STER */
#endif /*JP*/
};


/* Return a random monster name, for hallucination.
 * KNOWN BUG: May be a proper name (Godzilla, Barney), may not
 * (the Terminator, a Dalek).  There's no elegant way to deal
 * with this without radically modifying the calling functions.
 */
const char *
rndmonnam()
{
	int name;

	do {
	    name = rn1(SPECIAL_PM + SIZE(bogusmons) - LOW_PM, LOW_PM);
	} while (name < SPECIAL_PM &&
	    (type_is_pname(&mons[name]) || (mons[name].geno & G_NOGEN)));

	if (name >= SPECIAL_PM) return bogusmons[name - SPECIAL_PM];
	return E_J(mons[name].mname,JMONNAM(name));
}

#ifdef REINCARNATION
const char *
roguename() /* Name of a Rogue player */
{
	char *i, *opts;

	if ((opts = nh_getenv("ROGUEOPTS")) != 0) {
		for (i = opts; *i; i++)
			if (!strncmp("name=",i,5)) {
				char *j;
				if ((j = index(i+5,',')) != 0)
					*j = (char)0;
				return i+5;
			}
	}
	return rn2(3) ? (rn2(2) ? "Michael Toy" : "Kenneth Arnold")
		: "Glenn Wichman";
}
#endif /* REINCARNATION */
#endif /* OVLB */

#ifdef OVL2

static NEARDATA const char * const hcolors[] = {
#ifndef JP
	"ultraviolet", "infrared", "bluish-orange",
	"reddish-green", "dark white", "light black", "sky blue-pink",
	"salty", "sweet", "sour", "bitter",
	"striped", "spiral", "swirly", "plaid", "checkered", "argyle",
	"paisley", "blotchy", "guernsey-spotted", "polka-dotted",
	"square", "round", "triangular",
	"cabernet", "sangria", "fuchsia", "wisteria",
	"lemon-lime", "strawberry-banana", "peppermint",
	"romantic", "incandescent"
#else /*JP*/
	"紫外光の", "赤外光の", "青っぽいオレンジ色の",
	"赤みがかった緑色の", "暗い白色の", "明るい黒の", "空ピンク色の",
	"しょっぱい", "甘い", "酸っぱい", "苦い",
	"縞々の", "渦巻き模様の", "渦巻く", "タータンチェックの", "碁盤目の", "アーガイル柄の",
	"ペイズリー柄の", "ぶち模様の", "唐草模様の", "水玉模様の",
	"四角い", "丸い", "三角形の",
	"ワインレッドの", "カクテル色の", "梅色の", "藤色の", /* フクシアは馴染みがないので適当に変更 */
	"レモンライムの", "いちごバナナ色の", "ペパーミント色の",
	"ロマンチックな", "白熱色の"
	/* の→に い→く な→に */
#endif /*JP*/
};

const char *
hcolor(colorpref)
const char *colorpref;
{
	return (Hallucination || !colorpref) ?
		hcolors[rn2(SIZE(hcolors))] : colorpref;
}

/* return a random real color unless hallucinating */
const char *
rndcolor()
{
	int k = rn2(CLR_MAX);
	return Hallucination ? hcolor((char *)0) : (k == NO_COLOR) ?
		E_J("colorless","無色の") : c_obj_colors[k];
}

/* Aliases for road-runner nemesis
 */
static const char * const coynames[] = {
#ifndef JP
	"Carnivorous Vulgaris","Road-Runnerus Digestus",
	"Eatibus Anythingus"  ,"Famishus-Famishus",
	"Eatibus Almost Anythingus","Eatius Birdius",
	"Famishius Fantasticus","Eternalii Famishiis",
	"Famishus Vulgarus","Famishius Vulgaris Ingeniusi",
	"Eatius-Slobbius","Hardheadipus Oedipus",
	"Carnivorous Slobbius","Hard-Headipus Ravenus",
	"Evereadii Eatibus","Apetitius Giganticus",
	"Hungrii Flea-Bagius","Overconfidentii Vulgaris",
	"Caninus Nervous Rex","Grotesques Appetitus",
	"Nemesis Riduclii","Canis latrans"
#else
	"ナントシテモ・タベタイーノ","ロードランナ・ショウカシウス",
	"ナンデモ・タベリウス"  ,"ハラヘリウス・ハラヘリウス",
	"ホトンド・ナンデモ・タベリウス","トリヲ・タベリウス",
	"トンデモナクス・ハラヘリウス","イツマデモ・ハラヘリウス",
	"ハラヘリウス・ヤバンニウス","ハラヘリウス・ヤバンナス・バカス",
	"ヨダレタラシ・タベリウス","イシアタマ・マザコニウス",
	"ニクショク・ヨダレタラシウス","イシアタマ・ウエジニウス",
	"ジュンビヨシス・タベリウス","ショクヨク・ハゲシウス",
	"ハラヘリ・ノミダラケウス","ジシンカジョウス・ヤバンデス",
	"キャニウス・シンケイシツ・レックス","イジョウニ・ショクヨクアルス",
	"フクシュウ・バカバカシス","キャニス・ラトランス"
#endif /*JP*/
};

char *
coyotename(mtmp, buf)
struct monst *mtmp;
char *buf;
{
    if (mtmp && buf) {
	Sprintf(buf, "%s - %s",
	    x_monnam(mtmp, ARTICLE_NONE, (char *)0, 0, TRUE),
	    mtmp->mcan ? coynames[SIZE(coynames)-1] : coynames[rn2(SIZE(coynames)-1)]);
    }
    return buf;
}

char *
mon_its(mtmp)
struct monst *mtmp;
{
	return (is_animal(mtmp->data) || is_neuter(mtmp->data) ||
		!humanoid(mtmp->data) || !canspotmon(mtmp)) ?
			E_J("its","その") : (mtmp->female ? E_J("her","彼女の") : E_J("his","彼の"));
}

char *
mon_it(mtmp)
struct monst *mtmp;
{
	return (is_animal(mtmp->data) || is_neuter(mtmp->data) ||
		!humanoid(mtmp->data) || !canspotmon(mtmp)) ?
			E_J("it","それ") : (mtmp->female ? E_J("she","彼女") : E_J("he","彼"));
}

#endif /* OVL2 */

/*do_name.c*/
