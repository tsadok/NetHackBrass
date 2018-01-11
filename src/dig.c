/*	SCCS Id: @(#)dig.c	3.4	2003/03/23	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"
/* #define DEBUG */	/* turn on for diagnostics */

#ifdef OVLB

static NEARDATA boolean did_dig_msg;

STATIC_DCL boolean NDECL(rm_waslit);
STATIC_DCL void FDECL(mkcavepos, (XCHAR_P,XCHAR_P,int,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL void FDECL(mkcavearea, (BOOLEAN_P));
STATIC_DCL int FDECL(dig_typ, (struct obj *,XCHAR_P,XCHAR_P));
STATIC_DCL int NDECL(dig);
STATIC_DCL void NDECL(dig_up_grave);

/* Indices returned by dig_typ() */
#define DIGTYP_UNDIGGABLE 0
#define DIGTYP_ROCK       1
#define DIGTYP_STATUE     2
#define DIGTYP_BOULDER    3
#define DIGTYP_DOOR       4
#define DIGTYP_TREE       5


STATIC_OVL boolean
rm_waslit()
{
    register xchar x, y;

    if(levl[u.ux][u.uy].typ == ROOM && levl[u.ux][u.uy].waslit)
	return(TRUE);
    for(x = u.ux-2; x < u.ux+3; x++)
	for(y = u.uy-1; y < u.uy+2; y++)
	    if(isok(x,y) && levl[x][y].waslit) return(TRUE);
    return(FALSE);
}

/* Change level topology.  Messes with vision tables and ignores things like
 * boulders in the name of a nice effect.  Vision will get fixed up again
 * immediately after the effect is complete.
 */
STATIC_OVL void
mkcavepos(x, y, dist, waslit, rockit)
    xchar x,y;
    int dist;
    boolean waslit, rockit;
{
    register struct rm *lev;

    if(!isok(x,y)) return;
    lev = &levl[x][y];

    if(rockit) {
	register struct monst *mtmp;

	if(IS_ROCK(lev->typ)) return;
	if(t_at(x, y)) return; /* don't cover the portal */
	if ((mtmp = m_at(x, y)) != 0)	/* make sure crucial monsters survive */
	    if(!passes_walls(mtmp->data)) (void) rloc(mtmp, FALSE);
    } else if(lev->typ == ROOM) return;

    unblock_point(x,y);	/* make sure vision knows this location is open */

    /* fake out saved state */
    lev->seenv = 0;
    lev->doormask = 0;
    if(dist < 3) lev->lit = (rockit ? FALSE : TRUE);
    if(waslit) lev->waslit = (rockit ? FALSE : TRUE);
    lev->horizontal = FALSE;
    viz_array[y][x] = (dist < 3 ) ?
	(IN_SIGHT|COULD_SEE) : /* short-circuit vision recalc */
	COULD_SEE;
    lev->typ = (rockit ? STONE : ROOM);
    if(dist >= 3)
	impossible("mkcavepos called with dist %d", dist);
    if(Blind)
	feel_location(x, y);
    else newsym(x,y);
}

STATIC_OVL void
mkcavearea(rockit)
register boolean rockit;
{
    int dist;
    xchar xmin = u.ux, xmax = u.ux;
    xchar ymin = u.uy, ymax = u.uy;
    register xchar i;
    register boolean waslit = rm_waslit();

    if(rockit) pline(E_J("Crash!  The ceiling collapses around you!",
			 "轟音とともに、天井があなたのまわりに崩落した！"));
    else pline(E_J("A mysterious force %s cave around you!",
		   "不思議な力があなたのまわり%sた！"),
	     (levl[u.ux][u.uy].typ == CORR) ? E_J("creates a","に洞窟を作っ") :
					      E_J("extends the","の洞窟を広げ"));
    display_nhwindow(WIN_MESSAGE, TRUE);

    for(dist = 1; dist <= 2; dist++) {
	xmin--; xmax++;

	/* top and bottom */
	if(dist < 2) { /* the area is wider that it is high */
	    ymin--; ymax++;
	    for(i = xmin+1; i < xmax; i++) {
		mkcavepos(i, ymin, dist, waslit, rockit);
		mkcavepos(i, ymax, dist, waslit, rockit);
	    }
	}

	/* left and right */
	for(i = ymin; i <= ymax; i++) {
	    mkcavepos(xmin, i, dist, waslit, rockit);
	    mkcavepos(xmax, i, dist, waslit, rockit);
	}

	flush_screen(1);	/* make sure the new glyphs shows up */
	delay_output();
    }

    if(!rockit && levl[u.ux][u.uy].typ == CORR) {
	levl[u.ux][u.uy].typ = ROOM;
	if(waslit) levl[u.ux][u.uy].waslit = TRUE;
	newsym(u.ux, u.uy); /* in case player is invisible */
    }

    vision_full_recalc = 1;	/* everything changed */
}

/* When digging into location <x,y>, what are you actually digging into? */
STATIC_OVL int
dig_typ(otmp, x, y)
struct obj *otmp;
xchar x, y;
{
	boolean ispick = is_pick(otmp);

	return (ispick && sobj_at(STATUE, x, y) ? DIGTYP_STATUE :
		ispick && sobj_at(BOULDER, x, y) ? DIGTYP_BOULDER :
		closed_door(x, y) ? DIGTYP_DOOR :
		IS_TREES(levl[x][y].typ) ?
			(ispick ? DIGTYP_UNDIGGABLE : DIGTYP_TREE) :
		ispick && IS_ROCK(levl[x][y].typ) &&
			(!level.flags.arboreal || IS_WALL(levl[x][y].typ)) ?
			DIGTYP_ROCK : DIGTYP_UNDIGGABLE);
}

boolean
is_digging()
{
	if (occupation == dig) {
	    return TRUE;
	}
	return FALSE;
}

#define BY_YOU		(&youmonst)
#define BY_OBJECT	((struct monst *)0)

boolean
dig_check(madeby, verbose, x, y)
	struct monst	*madeby;
	boolean		verbose;
	int		x, y;
{
	struct trap *ttmp = t_at(x, y);
	const char *verb = (madeby == BY_YOU && uwep && is_axe(uwep)) ?
			E_J("chop","切り倒す") : E_J("dig in","掘る");

	if (On_stairs(x, y)) {
	    if (x == xdnladder || x == xupladder) {
		if(verbose) pline_The(E_J("ladder resists your effort.",
					  "はしごはあなたの行為を受け付けなかった。"));
	    } else if(verbose) pline_The(E_J("stairs are too hard to %s.",
					"階段は%sには固すぎる。"), verb);
	    return(FALSE);
	} else if (IS_THRONE(levl[x][y].typ) && madeby != BY_OBJECT) {
	    if(verbose) pline_The(E_J("throne is too hard to break apart.",
				      "この玉座はあまりに頑丈で、壊すことができない。"));
	    return(FALSE);
	} else if (IS_ALTAR(levl[x][y].typ) && (madeby != BY_OBJECT ||
				Is_astralevel(&u.uz) || Is_sanctum(&u.uz))) {
	    if(verbose) pline_The(E_J("altar is too hard to break apart.",
				      "この祭壇はあまりに頑丈で、壊すことができない。"));
	    return(FALSE);
	} else if (Is_airlevel(&u.uz)) {
	    if(verbose) You(E_J("cannot %s thin air.","何もない空間を%sことはできない。"), verb);
	    return(FALSE);
	} else if (Is_waterlevel(&u.uz)) {
	    if(verbose) pline_The(E_J("water splashes and subsides.",
				      "水がはねた。"));
	    return(FALSE);
	} else if ((IS_ROCK(levl[x][y].typ) && levl[x][y].typ != SDOOR &&
		      (levl[x][y].wall_info & W_NONDIGGABLE) != 0)
		|| (ttmp &&
		      (ttmp->ttyp == MAGIC_PORTAL || !Can_dig_down(&u.uz)))) {
	    if(verbose) pline_The(E_J("%s here is too hard to %s.",
				      "ここの%sは固すぎて、%sことができない。"),
				  surface(x,y), verb);
	    return(FALSE);
	} else if (sobj_at(BOULDER, x, y)) {
	    if(verbose) There(E_J("isn't enough room to %s here.",
				  "%s作業をするための十分な空間がない。"), verb);
	    return(FALSE);
	} else if (madeby == BY_OBJECT &&
		    /* the block against existing traps is mainly to
		       prevent broken wands from turning holes into pits */
		    (ttmp || is_pool(x,y) || is_lava(x,y))) {
	    /* digging by player handles pools separately */
	    return FALSE;
	}
	return(TRUE);
}

STATIC_OVL int
dig()
{
	register struct rm *lev;
	register xchar dpx = digging.pos.x, dpy = digging.pos.y;
	register boolean ispick = uwep && is_pick(uwep);
	const char *verb =
	    (!uwep || is_pick(uwep)) ? E_J("dig into","掘る") : E_J("chop through","切り倒す");

	lev = &levl[dpx][dpy];
	/* perhaps a nymph stole your pick-axe while you were busy digging */
	/* or perhaps you teleported away */
	if (u.uswallow || !uwep || (!ispick && !is_axe(uwep)) ||
	    !on_level(&digging.level, &u.uz) ||
	    ((digging.down ? (dpx != u.ux || dpy != u.uy)
			   : (distu(dpx,dpy) > 2))))
		return(0);

	if (digging.down) {
	    if(!dig_check(BY_YOU, TRUE, u.ux, u.uy)) return(0);
	} else { /* !digging.down */
	    if (IS_TREES(lev->typ) && !may_dig(dpx,dpy) &&
			dig_typ(uwep, dpx, dpy) == DIGTYP_TREE) {
		pline(E_J("This tree seems to be petrified.",
			  "この木は石化しているようだ。"));
  		return(0);
  	    }
	    if (IS_ROCK(lev->typ) && !may_dig(dpx,dpy) &&
			dig_typ(uwep, dpx, dpy) == DIGTYP_ROCK) {
		pline(E_J("This wall is too hard to %s.",
			  "この壁は固すぎて、%sことができない。"), verb);
  		return(0);
	    }
	}
	if(Fumbling && !rn2(3)) {
	    switch(rn2(3)) {
	    case 0:
		if(!welded(uwep)) {
		    You(E_J("fumble and drop your %s.",
			    "手を滑らせ、%sを落としてしまった。"), xname(uwep));
		    dropx(uwep);
		} else {
#ifdef STEED
		    if (u.usteed)
#ifndef JP
			Your("%s %s and %s %s!",
			     xname(uwep),
			     otense(uwep, "bounce"), otense(uwep, "hit"),
			     mon_nam(u.usteed));
#else
			Your("%sはすっぽ抜けてはね返り、%sに命中した！",
			     xname(uwep), mon_nam(u.usteed));
#endif /*JP*/
		    else
#endif
#ifndef JP
			pline("Ouch!  Your %s %s and %s you!",
			      xname(uwep),
			      otense(uwep, "bounce"), otense(uwep, "hit"));
#else
			pline("痛っ！ あなたの%sははね返り、あなたに命中した！",
			      xname(uwep));
#endif /*JP*/
		    set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
		}
		break;
	    case 1:
		pline(E_J("Bang!  You hit with the broad side of %s!",
			  "ガン！ あなたは%sの側面を打ちつけてしまった！"),
		      E_J(the(xname(uwep)),xname(uwep)));
		break;
	    default: Your(E_J("swing misses its mark.","一振りは狙いを外れた。"));
		break;
	    }
	    return(0);
	}

	digging.effort += 10 + rn2(5) + abon() +
			   uwep->spe - greatest_erosion(uwep) + u.udaminc;
	if (Race_if(PM_DWARF))
	    digging.effort *= 2;
	if (lev->typ == DEADTREE)
	    digging.effort *= 2;
	if (digging.down) {
		register struct trap *ttmp;

		if (digging.effort > 250) {
		    (void) dighole(FALSE);
		    (void) memset((genericptr_t)&digging, 0, sizeof digging);
		    return(0);	/* done with digging */
		}

		if (digging.effort <= 50 ||
		    ((ttmp = t_at(dpx,dpy)) != 0 &&
			(ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT ||
			 ttmp->ttyp == TRAPDOOR || ttmp->ttyp == HOLE)))
		    return(1);

		if (IS_ALTAR(lev->typ)) {
		    altar_wrath(dpx, dpy);
		    angry_priest();
		}

		if (dighole(TRUE)) {	/* make pit at <u.ux,u.uy> */
		    digging.level.dnum = 0;
		    digging.level.dlevel = -1;
		}
		return(0);
	}

	if (digging.effort > 100) {
		register const char *digtxt, *dmgtxt = (const char*) 0;
		register struct obj *obj;
		register boolean shopedge = *in_rooms(dpx, dpy, SHOPBASE);

		if ((obj = sobj_at(STATUE, dpx, dpy)) != 0) {
			if (break_statue(obj))
				digtxt = E_J("The statue shatters.","像は砕けた。");
			else
				/* it was a statue trap; break_statue()
				 * printed a message and updated the screen
				 */
				digtxt = (char *)0;
		} else if ((obj = sobj_at(BOULDER, dpx, dpy)) != 0) {
			struct obj *bobj;

			fracture_rock(obj);
			if ((bobj = sobj_at(BOULDER, dpx, dpy)) != 0) {
			    /* another boulder here, restack it to the top */
			    obj_extract_self(bobj);
			    place_object(bobj, dpx, dpy);
			}
			digtxt = E_J("The boulder falls apart.",
				     "大岩はこなごなに砕けた。");
		} else if (lev->typ == STONE || lev->typ == SCORR ||
				IS_TREES(lev->typ)) {
			if(Is_earthlevel(&u.uz)) {
			    if(uwep->blessed && !rn2(3)) {
				mkcavearea(FALSE);
				goto cleanup;
			    } else if((uwep->cursed && !rn2(4)) ||
					  (!uwep->blessed && !rn2(6))) {
				mkcavearea(TRUE);
				goto cleanup;
			    }
			}
			if (IS_TREES(lev->typ)) {
			    digtxt = E_J("You cut down the tree.","あなたは木を切り倒した。");
			    lev->typ = GROUND/*ROOM*/;
			    if (!rn2(5)) (void) rnd_treefruit_at(dpx, dpy);
			} else {
			    digtxt = E_J("You succeed in cutting away some rock.",
					 "あなたは岩を掘り抜いた。");
			    lev->typ = CORR;
			}
		} else if(IS_WALL(lev->typ)) {
			if(shopedge) {
			    add_damage(dpx, dpy, 10L * ACURRSTR);
			    dmgtxt = E_J("damage","壁を傷物にした");
			}
			if (level.flags.is_maze_lev) {
			    lev->typ = ROOM;
			} else if (level.flags.is_cavernous_lev &&
				   !in_town(dpx, dpy)) {
			    lev->typ = CORR;
			} else {
			    lev->typ = DOOR;
			    lev->doormask = D_NODOOR;
			}
			digtxt = E_J("You make an opening in the wall.",
				     "あなたは壁に穴を開けた。");
		} else if(lev->typ == SDOOR) {
			cvt_sdoor_to_door(lev);	/* ->typ = DOOR */
			digtxt = E_J("You break through a secret door!",
				     "あなたは隠し扉を叩き壊した！");
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else if(closed_door(dpx, dpy)) {
			digtxt = E_J("You break through the door.",
				     "あなたは扉を叩き壊した。");
			if(shopedge) {
			    add_damage(dpx, dpy, 400L);
			    dmgtxt = E_J("break","扉を壊した");
			}
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else return(0); /* statue or boulder got taken */

		if(!does_block(dpx,dpy,&levl[dpx][dpy]))
		    unblock_point(dpx,dpy);	/* vision:  can see through */
		if(Blind)
		    feel_location(dpx, dpy);
		else
		    newsym(dpx, dpy);
		if(digtxt && !digging.quiet) pline(digtxt); /* after newsym */
		if(dmgtxt)
		    pay_for_damage(dmgtxt, FALSE);

		if(Is_earthlevel(&u.uz) && !rn2(3)) {
		    register struct monst *mtmp;

		    switch(rn2(2)) {
		      case 0:
			mtmp = makemon(&mons[PM_EARTH_ELEMENTAL],
					dpx, dpy, NO_MM_FLAGS);
			break;
		      default:
			mtmp = makemon(&mons[PM_XORN],
					dpx, dpy, NO_MM_FLAGS);
			break;
		    }
		    if(mtmp) pline_The(E_J("debris from your digging comes to life!",
					   "あなたの砕いた岩の破片が生命を帯びた！"));
		}
		if(IS_DOOR(lev->typ) && (lev->doormask & D_TRAPPED)) {
			lev->doormask = D_NODOOR;
			b_trapped(E_J("door","扉"), 0);
			newsym(dpx, dpy);
		}
cleanup:
		digging.lastdigtime = moves;
		digging.quiet = FALSE;
		digging.level.dnum = 0;
		digging.level.dlevel = -1;
		return(0);
	} else {		/* not enough effort has been spent yet */
		static const char *const d_target[6] = {
#ifndef JP
			"", "rock", "statue", "boulder", "door", "tree"
#else
			"", "岩を掘削し", "像を壊し", "大岩を壊し", "扉を壊し", "木を伐り"
#endif /*JP*/
		};
		int dig_target = dig_typ(uwep, dpx, dpy);

		if (IS_WALL(lev->typ) || dig_target == DIGTYP_DOOR) {
		    if(*in_rooms(dpx, dpy, SHOPBASE)) {
#ifndef JP
			pline("This %s seems too hard to %s.",
			      IS_DOOR(lev->typ) ? "door" : "wall", verb);
#else
			pline("この%sは固すぎて%sことができない。",
			      IS_DOOR(lev->typ) ? "扉" : "壁", verb);
#endif /*JP*/
			return(0);
		    }
		} else if (!IS_ROCK(lev->typ) && dig_target == DIGTYP_ROCK)
		    return(0); /* statue or boulder got taken */
		if(!did_dig_msg) {
		    You(E_J("hit the %s with all your might.",
			    "全身の力をこめて%sはじめた。"),
			d_target[dig_target]);
		    did_dig_msg = TRUE;
		}
	}
	return(1);
}

/* When will hole be finished? Very rough indication used by shopkeeper. */
int
holetime()
{
	if(occupation != dig || !*u.ushops) return(-1);
	return ((250 - digging.effort) / 20);
}

/* Return typ of liquid to fill a hole with, or ROOM, if no liquid nearby */
schar
fillholetyp(x,y)
int x, y;
{
    register int x1, y1;
    int lo_x = max(1,x-1), hi_x = min(x+1,COLNO-1),
	lo_y = max(0,y-1), hi_y = min(y+1,ROWNO-1);
    int pool_cnt = 0, moat_cnt = 0, lava_cnt = 0, swamp_cnt = 0;

    for (x1 = lo_x; x1 <= hi_x; x1++)
	for (y1 = lo_y; y1 <= hi_y; y1++)
	    if (levl[x1][y1].typ == POOL)
		pool_cnt++;
	    else if (levl[x1][y1].typ == MOAT ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_MOAT))
		moat_cnt++;
	    else if (levl[x1][y1].typ == LAVAPOOL ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_LAVA))
		lava_cnt++;
	    else if (levl[x1][y1].typ == BOG ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_BOG))
		swamp_cnt++;
    pool_cnt /= 3;		/* not as much liquid as the others */

    if (lava_cnt > moat_cnt + pool_cnt && rn2(lava_cnt + 1))
	return LAVAPOOL;
    else if (moat_cnt > 0 && rn2(moat_cnt + 1))
	return MOAT;
    else if (pool_cnt > 0 && rn2(pool_cnt + 1))
	return POOL;
    else if (swamp_cnt > 0 && rn2(swamp_cnt + 1))
	return BOG;
    else
	return ROOM;
}

void
digactualhole(x, y, madeby, ttyp)
register int	x, y;
struct monst	*madeby;
int ttyp;
{
	struct obj *oldobjs, *newobjs;
	register struct trap *ttmp;
	char surface_type[BUFSZ];
	struct rm *lev = &levl[x][y];
	boolean shopdoor;
	struct monst *mtmp = m_at(x, y);	/* may be madeby */
	boolean madeby_u = (madeby == BY_YOU);
	boolean madeby_obj = (madeby == BY_OBJECT);
	boolean at_u = (x == u.ux) && (y == u.uy);
	boolean wont_fall = Levitation || Flying;

	if (u.utrap && u.utraptype == TT_INFLOOR) u.utrap = 0;

	/* these furniture checks were in dighole(), but wand
	   breaking bypasses that routine and calls us directly */
	if (IS_FOUNTAIN(lev->typ)) {
	    dogushforth(FALSE);
	    SET_FOUNTAIN_WARNED(x,y);		/* force dryup */
	    dryup(x, y, madeby_u);
	    return;
#ifdef SINKS
	} else if (IS_SINK(lev->typ)) {
	    breaksink(x, y);
	    return;
#endif
	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(x, y) >= 0)) {
	    int bx = x, by = y;
	    /* if under the portcullis, the bridge is adjacent */
	    (void) find_drawbridge(&bx, &by);
	    destroy_drawbridge(bx, by);
	    return;
	}

	if (ttyp != PIT && !Can_dig_down(&u.uz)) {
	    impossible("digactualhole: can't dig %s on this level.",
		       defsyms[trap_to_defsym(ttyp)].explanation);
	    ttyp = PIT;
	}

	/* maketrap() might change it, also, in this situation,
	   surface() returns an inappropriate string for a grave */
	if (IS_GRAVE(lev->typ))
	    Strcpy(surface_type, E_J("grave","墓"));
	else
	    Strcpy(surface_type, surface(x,y));
	shopdoor = IS_DOOR(lev->typ) && *in_rooms(x, y, SHOPBASE);
	oldobjs = level.objects[x][y];
	ttmp = maketrap(x, y, ttyp);
	if (!ttmp) return;
	newobjs = level.objects[x][y];
	ttmp->tseen = (madeby_u || cansee(x,y));
	ttmp->madeby_u = madeby_u;
	newsym(ttmp->tx,ttmp->ty);

	if (ttyp == PIT) {

	    if(madeby_u) {
		You(E_J("dig a pit in the %s.","%sに落とし穴を掘った。"), surface_type);
		if (shopdoor) pay_for_damage(E_J("ruin","扉を台無しにした"), FALSE);
	    } else if (!madeby_obj && canseemon(madeby))
		pline(E_J("%s digs a pit in the %s.","%sは%sに落とし穴を掘った。"),
		      Monnam(madeby), surface_type);
	    else if (cansee(x, y) && flags.verbose)
		pline(E_J("A pit appears in the %s.","%sに落とし穴が現れた。"), surface_type);

	    if(at_u) {
		if (!wont_fall) {
		    if (!Passes_walls)
			u.utrap = rn1(4,2);
		    u.utraptype = TT_PIT;
		    vision_full_recalc = 1;	/* vision limits change */
		} else
		    u.utrap = 0;
		if (oldobjs != newobjs)	/* something unearthed */
			(void) pickup(1);	/* detects pit */
	    } else if(mtmp) {
		if(is_flying(mtmp) || is_floating(mtmp)) {
		    if(canseemon(mtmp))
			pline(E_J("%s %s over the pit.","%sは落とし穴の上%sいる。"),
			      Monnam(mtmp), (is_flying(mtmp)) ?
			      E_J("flies","を飛んで") : E_J("floats","に浮いて"));
		} else if(mtmp != madeby)
		    (void) mintrap(mtmp);
	    }
	} else {	/* was TRAPDOOR now a HOLE*/

	    if(madeby_u)
		You(E_J("dig a hole through the %s.","%sを掘り抜いた。"), surface_type);
	    else if(!madeby_obj && canseemon(madeby))
		pline(E_J("%s digs a hole through the %s.","%sは%sを掘り抜いた。"),
		      Monnam(madeby), surface_type);
	    else if(cansee(x, y) && flags.verbose)
		pline(E_J("A hole appears in the %s.","%sに穴が現れた。"), surface_type);

	    if (at_u) {
		if (!u.ustuck && !wont_fall && !next_to_u()) {
		    You(E_J("are jerked back by your pet!","自分のペットに引き戻された！"));
		    wont_fall = TRUE;
		}

		/* Floor objects get a chance of falling down.  The case where
		 * the hero does NOT fall down is treated here.  The case
		 * where the hero does fall down is treated in goto_level().
		 */
		if (u.ustuck || wont_fall) {
		    if (newobjs)
			impact_drop((struct obj *)0, x, y, 0);
		    if (oldobjs != newobjs)
			(void) pickup(1);
		    if (shopdoor && madeby_u) pay_for_damage(E_J("ruin","扉を台無しにした"), FALSE);

		} else {
		    d_level newlevel;

		    if (*u.ushops && madeby_u)
			shopdig(1); /* shk might snatch pack */
		    /* handle earlier damage, eg breaking wand of digging */
		    else if (!madeby_u) pay_for_damage(E_J("dig into","壁を傷物にした"), TRUE);

		    You(E_J("fall through...","落ちていく…。"));
		    /* Earlier checks must ensure that the destination
		     * level exists and is in the present dungeon.
		     */
		    newlevel.dnum = u.uz.dnum;
		    newlevel.dlevel = u.uz.dlevel + 1;
		    goto_level(&newlevel, FALSE, TRUE, FALSE);
		    /* messages for arriving in special rooms */
		    spoteffects(FALSE);
		}
	    } else {
		if (shopdoor && madeby_u) pay_for_damage(E_J("ruin","扉を台無しにした"), FALSE);
		if (newobjs)
		    impact_drop((struct obj *)0, x, y, 0);
		if (mtmp) {
		     /*[don't we need special sokoban handling here?]*/
		    if (is_flying(mtmp) || is_floating(mtmp) ||
		        mtmp->data == &mons[PM_WUMPUS] ||
			(mtmp->wormno && count_wsegs(mtmp) > 5) ||
			mtmp->data->msize >= MZ_HUGE) return;
		    if (mtmp == u.ustuck)	/* probably a vortex */
			    return;		/* temporary? kludge */

		    if (teleport_pet(mtmp, FALSE)) {
			d_level tolevel;

			if (Is_stronghold(&u.uz)) {
			    assign_level(&tolevel, &valley_level);
			} else if (Is_botlevel(&u.uz)) {
			    if (canseemon(mtmp))
				pline(E_J("%s avoids the trap.","%sは罠を避けた。"), Monnam(mtmp));
			    return;
			} else {
			    get_level(&tolevel, depth(&u.uz) + 1);
			}
			if (mtmp->isshk) make_angry_shk(mtmp, 0, 0);
			migrate_to_level(mtmp, ledger_no(&tolevel),
					 MIGR_RANDOM, (coord *)0);
		    }
		}
	    }
	}
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
dighole(pit_only)
boolean pit_only;
{
	register struct trap *ttmp = t_at(u.ux, u.uy);
	struct rm *lev = &levl[u.ux][u.uy];
	struct obj *boulder_here;
	schar typ;
	boolean nohole = !Can_dig_down(&u.uz);

	if ((ttmp && (ttmp->ttyp == MAGIC_PORTAL || nohole)) ||
	   (IS_ROCK(lev->typ) && lev->typ != SDOOR &&
	    (lev->wall_info & W_NONDIGGABLE) != 0)) {
		pline_The(E_J("%s here is too hard to dig in.",
			      "ここの%sは固すぎて掘ることができない。"), surface(u.ux,u.uy));

	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
#ifndef JP
		pline_The("%s sloshes furiously for a moment, then subsides.",
			is_lava(u.ux, u.uy) ? "lava" : "water");
#else
		pline("%sが眼下で荒れ狂ったが、次第におさまった。",
			is_lava(u.ux, u.uy) ? "溶岩" : "水");
#endif /*JP*/
		wake_nearby();	/* splashing */

	} else if (lev->typ == DRAWBRIDGE_DOWN ||
		   (is_drawbridge_wall(u.ux, u.uy) >= 0)) {
		/* drawbridge_down is the platform crossing the moat when the
		   bridge is extended; drawbridge_wall is the open "doorway" or
		   closed "door" where the portcullis/mechanism is located */
		if (pit_only) {
		    pline_The(E_J("drawbridge seems too hard to dig through.",
				  "跳ね橋は固すぎて、掘ることはできない。"));
		    return FALSE;
		} else {
		    int x = u.ux, y = u.uy;
		    /* if under the portcullis, the bridge is adjacent */
		    (void) find_drawbridge(&x, &y);
		    destroy_drawbridge(x, y);
		    return TRUE;
		}

	} else if ((boulder_here = sobj_at(BOULDER, u.ux, u.uy)) != 0) {
		if (ttmp && (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) &&
		    rn2(2)) {
			pline_The(E_J("boulder settles into the pit.",
				      "大岩が落とし穴の中におさまった。"));
			ttmp->ttyp = PIT;	 /* crush spikes */
		} else {
			/*
			 * digging makes a hole, but the boulder immediately
			 * fills it.  Final outcome:  no hole, no boulder.
			 */
			pline(E_J("KADOOM! The boulder falls in!",
				  "ドーン！ 大岩が穴に落下した！"));
			(void) delfloortrap(ttmp);
		}
		delobj(boulder_here);
		return TRUE;

	} else if (IS_GRAVE(lev->typ)) {        
	    digactualhole(u.ux, u.uy, BY_YOU, PIT);
	    dig_up_grave();
	    return TRUE;
	} else if (lev->typ == DRAWBRIDGE_UP) {
		/* must be floor or ice, other cases handled above */
		/* dig "pit" and let fluid flow in (if possible) */
		typ = fillholetyp(u.ux,u.uy);

		if (typ == ROOM) {
			/*
			 * We can't dig a hole here since that will destroy
			 * the drawbridge.  The following is a cop-out. --dlc
			 */
			pline_The(E_J("%s here is too hard to dig in.",
				      "%sは固すぎて掘ることはできない。"),
			      surface(u.ux, u.uy));
			return FALSE;
		}

		lev->drawbridgemask &= ~DB_UNDER;
		lev->drawbridgemask |= (typ == LAVAPOOL) ? DB_LAVA :
				       (typ == BOG) ? DB_BOG : DB_MOAT;

 liquid_flow:
		if (ttmp) (void) delfloortrap(ttmp);
		/* if any objects were frozen here, they're released now */
		unearth_objs(u.ux, u.uy);

#ifndef JP
		pline("As you dig, the hole fills with %s!",
		      typ == LAVAPOOL ? "lava" : "water");
#else
		pline("掘ったとたん、穴に%sが流れ込んだ！",
		      typ == LAVAPOOL ? "溶岩" : "水");
#endif /*JP*/
		if (!Levitation && !Flying) {
		    if (typ == LAVAPOOL)
			(void) lava_effects();
		    else if (!Wwalking && typ != BOG)
			(void) drown();
		}
		return TRUE;

	/* the following two are here for the wand of digging */
	} else if (IS_THRONE(lev->typ)) {
		pline_The(E_J("throne is too hard to break apart.",
			      "玉座はあまりに頑丈で、壊すことはできない。"));

	} else if (IS_ALTAR(lev->typ)) {
		pline_The(E_J("altar is too hard to break apart.",
			      "祭壇はあまりに頑丈で、壊すことはできない。"));

	} else {
		typ = fillholetyp(u.ux,u.uy);

		if (typ != ROOM) {
			lev->typ = typ;
			goto liquid_flow;
		}

		/* finally we get to make a hole */
		if (nohole || pit_only)
			digactualhole(u.ux, u.uy, BY_YOU, PIT);
		else
			digactualhole(u.ux, u.uy, BY_YOU, HOLE);

		return TRUE;
	}

	return FALSE;
}

STATIC_OVL void
dig_up_grave()
{
	struct obj *otmp;

	/* Grave-robbing is frowned upon... */
	exercise(A_WIS, FALSE);
	if (Role_if(PM_ARCHEOLOGIST)) {
	    adjalign(-sgn(u.ualign.type)*3);
#ifndef JP
	    You_feel("like a despicable grave-robber!");
#else
	    pline("これは唾棄すべき盗掘行為だ！");
#endif /*JP*/
	} else if (Role_if(PM_SAMURAI)) {
	    adjalign(-sgn(u.ualign.type));
	    You(E_J("disturb the honorable dead!","尊ぶべき死者を冒涜した！"));
	} else if ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10)) {
	    adjalign(-sgn(u.ualign.type));
	    You(E_J("have violated the sanctity of this grave!",
		    "墓の安息を破った！"));
	}

	switch (rn2(5)) {
	case 0:
	case 1:
	    You(E_J("unearth a corpse.","死体を掘り出した。"));
	    if (!!(otmp = mk_tt_object(CORPSE, u.ux, u.uy)))
	    	otmp->age -= 100;		/* this is an *OLD* corpse */;
	    break;
	case 2:
	    if (!Blind) pline(Hallucination ? E_J("Dude!  The living dead!","見ろよ！ ゾンビだ！") :
 			E_J("The grave's owner is very upset!","墓の主はとても腹を立てた！"));
 	    (void) makemon(mkclass(S_ZOMBIE,0), u.ux, u.uy, NO_MM_FLAGS);
	    break;
	case 3:
	    if (!Blind) pline(Hallucination ? E_J("I want my mummy!","ママはどこ？") :
 			E_J("You've disturbed a tomb!","あなたは墓所を騒がせた！"));
 	    (void) makemon(mkclass(S_MUMMY,0), u.ux, u.uy, NO_MM_FLAGS);
	    break;
	default:
	    /* No corpse */
	    pline_The(E_J("grave seems unused.  Strange....",
			  "この墓は使われていないようだ。おかしい…。"));
	    break;
	}
	levl[u.ux][u.uy].typ = ROOM;
	del_engr_at(u.ux, u.uy);
	newsym(u.ux,u.uy);
	return;
}

int
use_pick_axe(obj)
struct obj *obj;
{
	boolean ispick;
	char dirsyms[12];
	char qbuf[QBUFSZ];
	register char *dsp = dirsyms;
	register int rx, ry;
	int res = 0;
	register const char *sdp, *verb;

	if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

	/* Check tool */
	if (obj != uwep) {
	    if (!wield_tool(obj, E_J("swing","振るう"))) return 0;
	    else res = 1;
	}
	ispick = is_pick(obj);
	verb = ispick ? E_J("dig","掘る") : E_J("chop","切り倒す");

	if (u.utrap && u.utraptype == TT_WEB) {
	    pline(E_J("%s you can't %s while entangled in a web.",
		      "%s、蜘蛛の巣にかかっていては%sことはできない。"),
		  /* res==0 => no prior message;
		     res==1 => just got "You now wield a pick-axe." message */
		  !res ? E_J("Unfortunately,","残念ながら") : E_J("But","だが"), verb);
	    return res;
	}

	while(*sdp) {
		(void) movecmd(*sdp);	/* sets u.dx and u.dy and u.dz */
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		/* Include down even with axe, so we have at least one direction */
		if (u.dz > 0 ||
		    (u.dz == 0 && isok(rx, ry) &&
		     dig_typ(obj, rx, ry) != DIGTYP_UNDIGGABLE))
			*dsp++ = *sdp;
		sdp++;
	}
	*dsp = 0;
#ifndef JP
	Sprintf(qbuf, "In what direction do you want to %s? [%s]", verb, dirsyms);
#else
	Sprintf(qbuf, "どの方向に%sいますか? [%s]", ispick ? "つるはしを使" : "斧を振る", dirsyms);
#endif /*JP*/
	if(!getdir(qbuf))
		return(res);

	return(use_pick_axe2(obj));
}

/* MRKR: use_pick_axe() is split in two to allow autodig to bypass */
/*       the "In what direction do you want to dig?" query.        */
/*       use_pick_axe2() uses the existing u.dx, u.dy and u.dz    */

int
use_pick_axe2(obj) 
struct obj *obj;
{
	register int rx, ry;
	register struct rm *lev;
	int dig_target;
	boolean ispick = is_pick(obj);
	const char *verbing = ispick ? E_J("digging","掘削") : E_J("chopping","伐採");
	int mat;
	struct monst *mtmp;

	if (u.uswallow && attack(u.ustuck)) {
		;  /* return(1) */
	} else if (Underwater) {
		pline(E_J("Turbulence torpedoes your %s attempts.",
			  "水の乱流のせいで、あなたの%s試みは失敗した。"), verbing);
	} else if(u.dz < 0) {
		if(Levitation)
			You(E_J("don't have enough leverage.",
				"十分な足場を得られない。"));
		else
			You_cant(E_J("reach the %s.","%sに届かない。"),ceiling(u.ux,u.uy));
	} else if(!u.dx && !u.dy && !u.dz) {
		char buf[BUFSZ];
		int dam;

		dam = rnd(2) + dbon() + obj->spe;
		if (dam <= 0) dam = 1;
#ifndef JP
		You("hit yourself with %s.", yname(uwep));
		Sprintf(buf, "%s own %s", uhis(),
				OBJ_NAME(objects[obj->otyp]));
#else
		You("自分を%sで殴った。", xname(uwep));
		Sprintf(buf, "自分を%sで殴って", JOBJ_NAME(objects[obj->otyp]));
#endif /*JP*/
		losehp(dam, buf, KILLED_BY);
		flags.botl=1;
		return(1);
	} else if(u.dz == 0) {
		if(Stunned || (Confusion && !rn2(5))) confdir();
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		if(!isok(rx, ry)) {
			pline(E_J("Clash!","ガキン！"));
			return(1);
		}
		lev = &levl[rx][ry];
		mtmp = m_at(rx, ry);
		if (mtmp) {
#ifdef MONSTEED
		    mtmp = target_rider_or_steed(&youmonst, mtmp);
#endif /*MONSTEED*/
		    if(attack(mtmp))
			return(1);
		}
		dig_target = dig_typ(obj, rx, ry);
		if (dig_target == DIGTYP_UNDIGGABLE) {
			/* ACCESSIBLE or POOL */
			struct trap *trap = t_at(rx, ry);

			if (trap && trap->ttyp == WEB) {
			    if (!trap->tseen) {
				seetrap(trap);
#ifndef JP
				There("is a spider web there!");
#else
				pline("そこには蜘蛛の巣がある！");
#endif /*JP*/
			    }
#ifndef JP
			    Your("%s entangled in the web.",
				aobjnam(obj, "become"));
#else
			    Your("%sは蜘蛛の巣に絡め取られた。",
				xname(obj));
#endif /*JP*/
			    /* you ought to be able to let go; tough luck */
			    /* (maybe `move_into_trap()' would be better) */
			    nomul(-d(2,2));
			    nomovemsg = E_J("You pull free.","あなたは自分を蜘蛛の巣から引きはがした。");
			} else if (lev->typ == IRONBARS) {
			    pline(E_J("Clang!","ガキン！"));
			    wake_nearby();
			} else if (IS_TREES(lev->typ))
#ifndef JP
			    You("need an axe to cut down a tree.");
#else
			    pline("木を切り倒すためには斧が必要だ。");
#endif /*JP*/
			else if (IS_ROCK(lev->typ))
#ifndef JP
			    You("need a pick to dig rock.");
#else
			    pline("岩を掘り抜くためにはつるはしが必要だ。");
#endif /*JP*/
			else if (!ispick && (sobj_at(STATUE, rx, ry) ||
					     sobj_at(BOULDER, rx, ry))) {
			    boolean vibrate = !rn2(3);
#ifndef JP
			    pline("Sparks fly as you whack the %s.%s",
				sobj_at(STATUE, rx, ry) ? "statue" : "boulder",
				vibrate ? " The axe-handle vibrates violently!" : "");
#else
			    pline("あなたが%sを叩きつけると、火花が飛んだ。%s",
				sobj_at(STATUE, rx, ry) ? "像" : "大岩",
				vibrate ? "斧の柄が激しく震えた！" : "");
#endif /*JP*/
			    if (vibrate) losehp(2, E_J("axing a hard object",
							"固い物体を斧で叩いて"), KILLED_BY);
			}
			else
#ifndef JP
			    You("swing your %s through thin air.",
				aobjnam(obj, (char *)0));
#else
			    You("何もない空間で%sを振り回した。",
				xname(obj));
#endif /*JP*/
		} else {
			static const char * const d_action[6] = {
				E_J("swinging",			"振り回し"),
				E_J("digging",			"岩を掘り"),
				E_J("chipping the statue",	"像を壊し"),
				E_J("hitting the boulder",	"大岩を砕き"),
				E_J("chopping at the door",	"扉を叩き壊し"),
				E_J("cutting the tree",		"木を切り")
			};
			if (dig_target == DIGTYP_STATUE) {
			    mat = get_material(sobj_at(STATUE, rx, ry));
			    if ( mat != MINERAL && mat != GLASS ) {
				pline(E_J("The statue is too hard to break apart.",
					  "この像は固すぎて壊せない。"));
				return (1);
			    }
			}
			did_dig_msg = FALSE;
			digging.quiet = FALSE;
			if (digging.pos.x != rx || digging.pos.y != ry ||
			    !on_level(&digging.level, &u.uz) || digging.down) {
			    if (flags.autodig &&
				dig_target == DIGTYP_ROCK && !digging.down &&
				digging.pos.x == u.ux &&
				digging.pos.y == u.uy &&
				(moves <= digging.lastdigtime+2 &&
				 moves >= digging.lastdigtime)) {
				/* avoid messages if repeated autodigging */
				did_dig_msg = TRUE;
				digging.quiet = TRUE;
			    }
			    digging.down = digging.chew = FALSE;
			    digging.warned = FALSE;
			    digging.pos.x = rx;
			    digging.pos.y = ry;
			    assign_level(&digging.level, &u.uz);
			    digging.effort = 0;
			    if (!digging.quiet)
				You(E_J("start %s.","%sはじめた。"), d_action[dig_target]);
			} else {
#ifndef JP
			    You("%s %s.", digging.chew ? "begin" : "continue",
					d_action[dig_target]);
#else
			    You("%s%sはじめた。", digging.chew ? "" : "再び",
					d_action[dig_target]);
#endif /*JP*/
			    digging.chew = FALSE;
			}
			set_occupation(dig, verbing, 0);
		}
	} else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		/* it must be air -- water checked above */
#ifndef JP
		You("swing your %s through thin air.", aobjnam(obj, (char *)0));
#else
		You("何もない空間に向かって%sを振り回した。", xname(obj));
#endif /*JP*/
	} else if (!can_reach_floor()) {
		You_cant(E_J("reach the %s.","%sに届かない。"), surface(u.ux,u.uy));
	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
		/* Monsters which swim also happen not to be able to dig */
		You(E_J("cannot stay under%s long enough.",
			"%s中にそんなに長時間潜っていられない。"),
				is_pool(u.ux, u.uy) ? E_J("water","水") : E_J(" the lava","溶岩の"));
	} else if (!ispick) {
#ifndef JP
		Your("%s merely scratches the %s.",
				aobjnam(obj, (char *)0), surface(u.ux,u.uy));
#else
		Your("%sは%sに掻き傷をつけられただけだった。",
				xname(obj), surface(u.ux,u.uy));
#endif /*JP*/
		u_wipe_engr(3);
	} else {
		if (digging.pos.x != u.ux || digging.pos.y != u.uy ||
			!on_level(&digging.level, &u.uz) || !digging.down) {
		    digging.chew = FALSE;
		    digging.down = TRUE;
		    digging.warned = FALSE;
		    digging.pos.x = u.ux;
		    digging.pos.y = u.uy;
		    assign_level(&digging.level, &u.uz);
		    digging.effort = 0;
		    You(E_J("start %s downward.","下方向へ%sを開始した。"), verbing);
		    if (*u.ushops) shopdig(0);
		} else
		    You(E_J("continue %s downward.","下方向への%sを再開した。"), verbing);
		did_dig_msg = FALSE;
		set_occupation(dig, verbing, 0);
	}
	return(1);
}

/*
 * Town Watchmen frown on damage to the town walls, trees or fountains.
 * It's OK to dig holes in the ground, however.
 * If mtmp is assumed to be a watchman, a watchman is found if mtmp == 0
 * zap == TRUE if wand/spell of digging, FALSE otherwise (chewing)
 */
void
watch_dig(mtmp, x, y, zap)
    struct monst *mtmp;
    xchar x, y;
    boolean zap;
{
	struct rm *lev = &levl[x][y];

	if (in_town(x, y) &&
	    (closed_door(x, y) || lev->typ == SDOOR ||
	     IS_WALL(lev->typ) || IS_FOUNTAIN(lev->typ) || IS_TREE(lev->typ))) {
	    if (!mtmp) {
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if ((mtmp->data == &mons[PM_WATCHMAN] ||
			 mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			mtmp->mcansee && m_canseeu(mtmp) &&
			couldsee(mtmp->mx, mtmp->my) && mtmp->mpeaceful)
			break;
		}
	    }

	    if (mtmp) {
		if(zap || digging.warned) {
		    verbalize(E_J("Halt, vandal!  You're under arrest!",
				  "動くな！ お前を器物損壊の現行犯で逮捕する！"));
		    (void) angry_guards(!(flags.soundok));
		} else {
		    const char *str;

		    if (IS_DOOR(lev->typ))
			str = E_J("door","扉");
		    else if (IS_TREE(lev->typ))
			str = E_J("tree","木");
		    else if (IS_ROCK(lev->typ))
			str = E_J("wall","壁");
		    else
			str = E_J("fountain","泉");
		    verbalize(E_J("Hey, stop damaging that %s!",
				  "おい、%sを傷つけるのをやめろ！"), str);
		    digging.warned = TRUE;
		}
		if (is_digging())
		    stop_occupation();
	    }
	}
}

#endif /* OVLB */
#ifdef OVL0

/* Return TRUE if monster died, FALSE otherwise.  Called from m_move(). */
boolean
mdig_tunnel(mtmp)
register struct monst *mtmp;
{
	register struct rm *here;
	int pile = rnd(12);

	here = &levl[mtmp->mx][mtmp->my];
	if (here->typ == SDOOR)
	    cvt_sdoor_to_door(here);	/* ->typ = DOOR */

	/* Eats away door if present & closed or locked */
	if (closed_door(mtmp->mx, mtmp->my)) {
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
		add_damage(mtmp->mx, mtmp->my, 0L);
	    unblock_point(mtmp->mx, mtmp->my);	/* vision */
	    if (here->doormask & D_TRAPPED) {
		here->doormask = D_NODOOR;
		if (mb_trapped(mtmp)) {	/* mtmp is killed */
		    newsym(mtmp->mx, mtmp->my);
		    return TRUE;
		}
	    } else {
		if (!rn2(3) && flags.verbose)	/* not too often.. */
#ifndef JP
		    You_feel("an unexpected draft.");
#else
		    pline("不意にすきま風が吹き込んできた。");
#endif /*JP*/
		here->doormask = D_BROKEN;
	    }
	    newsym(mtmp->mx, mtmp->my);
	    return FALSE;
	} else if (!IS_ROCK(here->typ) && !IS_TREE(here->typ)) /* no dig */
	    return FALSE;

	/* Only rock, trees, and walls fall through to this point. */
	if ((here->wall_info & W_NONDIGGABLE) != 0) {
	    impossible("mdig_tunnel:  %s at (%d,%d) is undiggable",
		       (IS_WALL(here->typ) ? "wall" : "stone"),
		       (int) mtmp->mx, (int) mtmp->my);
	    return FALSE;	/* still alive */
	}

	if (IS_WALL(here->typ)) {
	    /* KMH -- Okay on arboreal levels (room walls are still stone) */
	    if (flags.soundok && flags.verbose && !rn2(5))
		You_hear(E_J("crashing rock.","岩を砕く音を"));
	    if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE))
		add_damage(mtmp->mx, mtmp->my, 0L);
	    if (level.flags.is_maze_lev) {
		here->typ = ROOM;
	    } else if (level.flags.is_cavernous_lev &&
		       !in_town(mtmp->mx, mtmp->my)) {
		here->typ = CORR;
	    } else {
		here->typ = DOOR;
		here->doormask = D_NODOOR;
	    }
	} else if (IS_TREE(here->typ)) {
	    here->typ = ROOM;
	    if (pile && pile < 5)
		(void) rnd_treefruit_at(mtmp->mx, mtmp->my);
	} else {
	    here->typ = CORR;
	    if (pile && pile < 5)
		(void) mksobj_at((pile == 1) ? BOULDER : ROCK,
			     mtmp->mx, mtmp->my, TRUE, FALSE);
	}
	newsym(mtmp->mx, mtmp->my);
	if (!sobj_at(BOULDER, mtmp->mx, mtmp->my))
	    unblock_point(mtmp->mx, mtmp->my);	/* vision */

	return FALSE;
}

#endif /* OVL0 */
#ifdef OVL3

/* digging via wand zap or spell cast */
void
zap_dig()
{
	struct rm *room;
	struct monst *mtmp;
	struct obj *otmp;
	int zx, zy, digdepth;
	boolean shopdoor, shopwall, maze_dig;
	/*
	 * Original effect (approximately):
	 * from CORR: dig until we pierce a wall
	 * from ROOM: pierce wall and dig until we reach
	 * an ACCESSIBLE place.
	 * Currently: dig for digdepth positions;
	 * also down on request of Lennart Augustsson.
	 */

	if (u.uswallow) {
	    mtmp = u.ustuck;

	    if (!is_whirly(mtmp->data)) {
		if (is_animal(mtmp->data))
		    You(E_J("pierce %s %s wall!","%s%s壁に穴を穿った！"),
			s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH));
		mtmp->mhp = 1;		/* almost dead */
		expels(mtmp, mtmp->data, !is_animal(mtmp->data));
	    }
	    return;
	} /* swallowed */

	if (u.dz) {
	    if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !Underwater) {
		if (u.dz < 0 || On_stairs(u.ux, u.uy)) {
		    if (On_stairs(u.ux, u.uy))
			pline_The(E_J("beam bounces off the %s and hits the %s.",
				      "光線は%sではね返り、%sに命中した。"),
			      (u.ux == xdnladder || u.ux == xupladder) ?
			      E_J("ladder","はしご") : E_J("stairs","階段"), ceiling(u.ux, u.uy));
		    You(E_J("loosen a rock from the %s.",
			    "%sから岩のかけらを揺り落とした。"), ceiling(u.ux, u.uy));
		    pline(E_J("It falls on your %s!",
			      "切片はあなたの%sめがけて降ってきた！"), body_part(HEAD));
		    losehp(rnd((uarmh && is_metallic(uarmh)) ? 2 : 6),
			   E_J("falling rock","落石で"), KILLED_BY_AN);
		    otmp = mksobj_at(ROCK, u.ux, u.uy, FALSE, FALSE);
		    if (otmp) {
			(void)xname(otmp);	/* set dknown, maybe bknown */
			stackobj(otmp);
		    }
		    newsym(u.ux, u.uy);
		} else {
		    watch_dig((struct monst *)0, u.ux, u.uy, TRUE);
		    (void) dighole(FALSE);
		}
	    }
	    return;
	} /* up or down */

	/* normal case: digging across the level */
	shopdoor = shopwall = FALSE;
	maze_dig = level.flags.is_maze_lev && !Is_earthlevel(&u.uz);
	zx = u.ux + u.dx;
	zy = u.uy + u.dy;
	digdepth = rn1(18, 8);
	tmp_at(DISP_BEAM, cmap_to_glyph(S_digbeam));
	while (--digdepth >= 0) {
	    if (!isok(zx,zy)) break;
	    room = &levl[zx][zy];
	    tmp_at(zx,zy);
	    delay_output();	/* wait a little bit */
	    if (closed_door(zx, zy) || room->typ == SDOOR) {
		if (*in_rooms(zx,zy,SHOPBASE)) {
		    add_damage(zx, zy, 400L);
		    shopdoor = TRUE;
		}
		if (room->typ == SDOOR)
		    room->typ = DOOR;
		else if (cansee(zx, zy))
		    pline_The(E_J("door is razed!","扉は引き裂かれた！"));
		watch_dig((struct monst *)0, zx, zy, TRUE);
		room->doormask = D_NODOOR;
		unblock_point(zx,zy); /* vision */
		digdepth -= 2;
		if (maze_dig) break;
	    } else if (maze_dig) {
		if (IS_WALL(room->typ)) {
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			if (*in_rooms(zx,zy,SHOPBASE)) {
			    add_damage(zx, zy, 200L);
			    shopwall = TRUE;
			}
			room->typ = ROOM;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The(E_J("wall glows then fades.",
				      "壁はしばらく輝いた。"));
		    break;
		} else if (IS_TREE(room->typ)) { /* check trees before stone */
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			room->typ = GROUND/*ROOM*/;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The(E_J("tree shudders but is unharmed.",
				      "木は震えたが、傷つかなかった。"));
		    break;
		} else if (room->typ == STONE || room->typ == SCORR) {
		    if (!(room->wall_info & W_NONDIGGABLE)) {
			room->typ = CORR;
			unblock_point(zx,zy); /* vision */
		    } else if (!Blind)
			pline_The(E_J("rock glows then fades.",
				      "岩壁はしばらく輝いた。"));
		    break;
		}
	    } else if (IS_ROCK(room->typ)) {
		if (!may_dig(zx,zy)) break;
		if (IS_WALL(room->typ) || room->typ == SDOOR) {
		    if (*in_rooms(zx,zy,SHOPBASE)) {
			add_damage(zx, zy, 200L);
			shopwall = TRUE;
		    }
		    watch_dig((struct monst *)0, zx, zy, TRUE);
		    if (level.flags.is_cavernous_lev && !in_town(zx, zy)) {
			room->typ = CORR;
		    } else {
			room->typ = DOOR;
			room->doormask = D_NODOOR;
		    }
		    digdepth -= 2;
		} else if (IS_TREE(room->typ)) {
		    room->typ = GROUND/*ROOM*/;
		    digdepth -= 2;
		} else {	/* IS_ROCK but not IS_WALL or SDOOR */
		    room->typ = CORR;
		    digdepth--;
		}
		unblock_point(zx,zy); /* vision */
	    }
	    zx += u.dx;
	    zy += u.dy;
	} /* while */
	tmp_at(DISP_END,0);	/* closing call */
	if (shopdoor || shopwall)
	    pay_for_damage(shopdoor ? E_J("destroy","扉を壊した") : E_J("dig into","壁を傷物にした"), FALSE);
	return;
}

/* move objects from fobj/nexthere lists to buriedobjlist, keeping position */
/* information */
struct obj *
bury_an_obj(otmp)
	struct obj *otmp;
{
	struct obj *otmp2;
	boolean under_ice;

#ifdef DEBUG
	pline("bury_an_obj: %s", xname(otmp));
#endif
	if (otmp == uball)
		unpunish();
	/* after unpunish(), or might get deallocated chain */
	otmp2 = otmp->nexthere;
	/*
	 * obj_resists(,0,0) prevents Rider corpses from being buried.
	 * It also prevents The Amulet and invocation tools from being
	 * buried.  Since they can't be confined to bags and statues,
	 * it makes sense that they can't be buried either, even though
	 * the real reason there (direct accessibility when carried) is
	 * completely different.
	 */
	if (otmp == uchain || obj_resists(otmp, 0, 0))
		return(otmp2);

	if (otmp->otyp == LEASH && otmp->leashmon != 0)
		o_unleash(otmp);

	if (otmp->lamplit && otmp->otyp != POT_OIL)
		end_burn(otmp, TRUE);

	obj_extract_self(otmp);

	under_ice = is_ice(otmp->ox, otmp->oy);
	if (otmp->otyp == ROCK && !under_ice) {
		/* merges into burying material */
		obfree(otmp, (struct obj *)0);
		return(otmp2);
	}
	/*
	 * Start a rot on organic material.  Not corpses -- they
	 * are already handled.
	 */
	if (otmp->otyp == CORPSE ||
	    (otmp->otyp == STATUE && get_material(otmp) == LIQUID)) {
	    ;		/* should cancel timer if under_ice */
	} else if ((under_ice ? otmp->oclass == POTION_CLASS : is_organic(otmp))
		&& !obj_resists(otmp, 5, 95)) {
	    (void) start_timer((under_ice ? 0L : 250L) + (long)rnd(250),
			       TIMER_OBJECT, ROT_ORGANIC, (genericptr_t)otmp);
	}
	add_to_buried(otmp);
	return(otmp2);
}

void
bury_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	if(level.objects[x][y] != (struct obj *)0)
		pline("bury_objs: at %d, %d", x, y);
#endif
	for (otmp = level.objects[x][y]; otmp; otmp = otmp2)
		otmp2 = bury_an_obj(otmp);

	/* don't expect any engravings here, but just in case */
	del_engr_at(x, y);
	newsym(x, y);
}

/* move objects from buriedobjlist to fobj/nexthere lists */
void
unearth_objs(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

#ifdef DEBUG
	pline("unearth_objs: at %d, %d", x, y);
#endif
	for (otmp = level.buriedobjlist; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
		if (otmp->ox == x && otmp->oy == y) {
		    obj_extract_self(otmp);
		    if (otmp->timed)
			(void) stop_timer(ROT_ORGANIC, (genericptr_t)otmp);
		    place_object(otmp, x, y);
		    stackobj(otmp);
		}
	}
	del_engr_at(x, y);
	newsym(x, y);
}

/*
 * The organic material has rotted away while buried.  As an expansion,
 * we could add add partial damage.  A damage count is kept in the object
 * and every time we are called we increment the count and reschedule another
 * timeout.  Eventually the object rots away.
 *
 * This is used by buried objects other than corpses.  When a container rots
 * away, any contents become newly buried objects.
 */
/* ARGSUSED */
void
rot_organic(arg, timeout)
genericptr_t arg;
long timeout;	/* unused */
{
	struct obj *obj = (struct obj *) arg;

	while (Has_contents(obj)) {
	    /* We don't need to place contained object on the floor
	       first, but we do need to update its map coordinates. */
	    obj->cobj->ox = obj->ox,  obj->cobj->oy = obj->oy;
	    /* Everything which can be held in a container can also be
	       buried, so bury_an_obj's use of obj_extract_self insures
	       that Has_contents(obj) will eventually become false. */
	    (void)bury_an_obj(obj->cobj);
	}
	obj_extract_self(obj);
	obfree(obj, (struct obj *) 0);
}

/*
 * Called when a corpse has rotted completely away.
 */
void
rot_corpse(arg, timeout)
genericptr_t arg;
long timeout;	/* unused */
{
	xchar x = 0, y = 0;
	struct obj *obj = (struct obj *) arg;
	boolean on_floor = obj->where == OBJ_FLOOR,
		in_invent = obj->where == OBJ_INVENT;

	if (on_floor) {
	    x = obj->ox;
	    y = obj->oy;
	} else if (in_invent) {
	    if (flags.verbose) {
		char *cname = corpse_xname(obj, FALSE);
		if (In_Cocyutus(&u.uz)) {
#ifndef JP
		    Your("%s%s %s and %s away%c",
			 obj == uwep ? "wielded " : nul, cname,
			 vtense(cname, "freeze"),
			 vtense(cname, "crumble"), obj == uwep ? '!' : '.');
#else
		    Your("%s%sは凍りつき、砕け散った%s",
			 obj == uwep ? "装備していた" : nul, cname,
			 obj == uwep ? "！" : "。");
#endif /*JP*/
		} else {
#ifndef JP
		    Your("%s%s %s away%c",
			 obj == uwep ? "wielded " : nul, cname,
		     otense(obj, "rot"), obj == uwep ? '!' : '.');
#else
		    Your("%s%sは腐り落ちた%s",
			 obj == uwep ? "装備していた" : nul, cname,
			 obj == uwep ? "！" : "。");
#endif /*JP*/
		}
	    }
	    if (obj == uwep) {
		uwepgone();	/* now bare handed */
		stop_occupation();
	    } else if (obj == uswapwep) {
		uswapwepgone();
		stop_occupation();
	    } else if (obj == uquiver) {
		uqwepgone();
		stop_occupation();
	    }
	} else if (obj->where == OBJ_MINVENT && obj->owornmask) {
	    if (obj == MON_WEP(obj->ocarry)) {
		setmnotwielded(obj->ocarry,obj);
		MON_NOWEP(obj->ocarry);
	    }
	}
	rot_organic(arg, timeout);
	if (on_floor) newsym(x, y);
	else if (in_invent) update_inventory();
}

#if 0
void
bury_monst(mtmp)
struct monst *mtmp;
{
#ifdef DEBUG
	pline("bury_monst: %s", mon_nam(mtmp));
#endif
	if(canseemon(mtmp)) {
	    if(is_flying(mtmp) || is_floating(mtmp)) {
		pline_The("%s opens up, but %s is not swallowed!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
		return;
	    } else
	        pline_The("%s opens up and swallows %s!",
			surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
	}

	mtmp->mburied = TRUE;
	wakeup(mtmp);			/* at least give it a chance :-) */
	newsym(mtmp->mx, mtmp->my);
}

void
bury_you()
{
#ifdef DEBUG
	pline("bury_you");
#endif
    if (!Levitation && !Flying) {
	if(u.uswallow)
	    You_feel("a sensation like falling into a trap!");
	else
	    pline_The("%s opens beneath you and you fall in!",
		  surface(u.ux, u.uy));

	u.uburied = TRUE;
	if(!Strangled && !Breathless) Strangled = 6;
	under_ground(1);
    }
}

void
unearth_you()
{
#ifdef DEBUG
	pline("unearth_you");
#endif
	u.uburied = FALSE;
	under_ground(0);
	if(!uamul || uamul->otyp != AMULET_OF_STRANGULATION)
		Strangled = 0;
	vision_recalc(0);
}

void
escape_tomb()
{
#ifdef DEBUG
	pline("escape_tomb");
#endif
	if ((Teleportation || can_teleport(youmonst.data)) &&
	    (Teleport_control || rn2(3) < Luck+2)) {
		You("attempt a teleport spell.");
		(void) dotele();	/* calls unearth_you() */
	} else if(u.uburied) { /* still buried after 'port attempt */
		boolean good;

		if(amorphous(youmonst.data) || Passes_walls ||
		   noncorporeal(youmonst.data) || unsolid(youmonst.data) ||
		   (tunnels(youmonst.data) && !needspick(youmonst.data))) {

		    You("%s up through the %s.",
			(tunnels(youmonst.data) && !needspick(youmonst.data)) ?
			 "try to tunnel" : (amorphous(youmonst.data)) ?
			 "ooze" : "phase", surface(u.ux, u.uy));

		    if(tunnels(youmonst.data) && !needspick(youmonst.data))
			good = dighole(TRUE);
		    else good = TRUE;
		    if(good) unearth_you();
		}
	}
}

void
bury_obj(otmp)
struct obj *otmp;
{

#ifdef DEBUG
	pline("bury_obj");
#endif
	if(cansee(otmp->ox, otmp->oy))
	   pline_The("objects on the %s tumble into a hole!",
		surface(otmp->ox, otmp->oy));

	bury_objs(otmp->ox, otmp->oy);
}
#endif

#ifdef DEBUG
int
wiz_debug_cmd() /* in this case, bury everything at your loc and around */
{
	int x, y;

	for (x = u.ux - 1; x <= u.ux + 1; x++)
	    for (y = u.uy - 1; y <= u.uy + 1; y++)
		if (isok(x,y)) bury_objs(x,y);
	return 0;
}

#endif /* DEBUG */
#endif /* OVL3 */

/*dig.c*/
