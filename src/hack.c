/*	SCCS Id: @(#)hack.c	3.4	2003/04/30	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVL1
STATIC_DCL void NDECL(maybe_wail);
#endif /*OVL1*/
STATIC_DCL int NDECL(moverock);
STATIC_DCL int FDECL(still_chewing,(XCHAR_P,XCHAR_P));
#ifdef SINKS
STATIC_DCL void NDECL(dosinkfall);
#endif
STATIC_DCL boolean FDECL(findtravelpath, (BOOLEAN_P));
STATIC_DCL boolean FDECL(monstinroom, (struct permonst *,int));

STATIC_DCL void FDECL(move_update, (BOOLEAN_P));
STATIC_DCL void FDECL(struggle_sub, (const char *));

#define IS_SHOP(x)	(rooms[x].rtype >= SHOPBASE)

#ifdef OVL2

boolean
revive_nasty(x, y, msg)
int x,y;
const char *msg;
{
    register struct obj *otmp, *otmp2;
    struct monst *mtmp;
    coord cc;
    boolean revived = FALSE;

    for(otmp = level.objects[x][y]; otmp; otmp = otmp2) {
	otmp2 = otmp->nexthere;
	if (otmp->otyp == CORPSE &&
	    (is_rider(&mons[otmp->corpsenm]) ||
	     otmp->corpsenm == PM_WIZARD_OF_YENDOR)) {
	    /* move any living monster already at that location */
	    if((mtmp = m_at(x,y)) && enexto(&cc, x, y, mtmp->data))
		rloc_to(mtmp, cc.x, cc.y);
	    if(msg) Norep("%s", msg);
	    revived = revive_corpse(otmp);
	}
    }

    /* this location might not be safe, if not, move revived monster */
    if (revived) {
	mtmp = m_at(x,y);
	if (mtmp && !goodpos(x, y, mtmp, 0) &&
	    enexto(&cc, x, y, mtmp->data)) {
	    rloc_to(mtmp, cc.x, cc.y);
	}
	/* else impossible? */
    }

    return (revived);
}

STATIC_OVL int
moverock()
{
    register xchar rx, ry, sx, sy;
    register struct obj *otmp;
    register struct trap *ttmp;
    register struct monst *mtmp;

    sx = u.ux + u.dx,  sy = u.uy + u.dy;	/* boulder starting position */
    while ((otmp = sobj_at(BOULDER, sx, sy)) != 0) {
	/* make sure that this boulder is visible as the top object */
	if (otmp != level.objects[sx][sy]) movobj(otmp, sx, sy);

	rx = u.ux + 2 * u.dx;	/* boulder destination position */
	ry = u.uy + 2 * u.dy;
	nomul(0);
	if (Levitation || Is_airlevel(&u.uz)) {
	    if (Blind) feel_location(sx, sy);
#ifndef JP
	    You("don't have enough leverage to push %s.", the(xname(otmp)));
#else
	    You("%sを押すのに十分な力を得られない。", xname(otmp));
#endif /*JP*/
	    /* Give them a chance to climb over it? */
	    return -1;
	}
	if (verysmall(youmonst.data)
#ifdef STEED
		 && !u.usteed
#endif
				    ) {
	    if (Blind) feel_location(sx, sy);
	    pline(E_J("You're too small to push that %s.",
		      "あなたは%sを押すには小さすぎる。"), xname(otmp));
	    goto cannot_push;
	}
	if (isok(rx,ry) && !IS_ROCK(levl[rx][ry].typ) &&
	    levl[rx][ry].typ != IRONBARS &&
	    (!IS_DOOR(levl[rx][ry].typ) || !(u.dx && u.dy) || (
#ifdef REINCARNATION
		!Is_rogue_level(&u.uz) &&
#endif
		(levl[rx][ry].doormask & ~D_BROKEN) == D_NODOOR)) &&
	    !sobj_at(BOULDER, rx, ry)) {
	    ttmp = t_at(rx, ry);
	    mtmp = m_at(rx, ry);

		/* KMH -- Sokoban doesn't let you push boulders diagonally */
	    if (In_sokoban(&u.uz) && u.dx && u.dy) {
	    	if (Blind) feel_location(sx,sy);
#ifndef JP
	    	pline("%s won't roll diagonally on this %s.",
	        		The(xname(otmp)), surface(sx, sy));
#else
	    	pline("ここでは%sを斜めに転がすことはできない。", xname(otmp));
#endif /*JP*/
	    	goto cannot_push;
	    }

	    if (revive_nasty(rx, ry, E_J("You sense movement on the other side.",
					 "あなたは向こう側で何かが動くのに気づいた。")))
		return (-1);

	    if (mtmp && !noncorporeal(mtmp->data) &&
		    (!mtmp->mtrapped ||
			 !(ttmp && ((ttmp->ttyp == PIT) ||
				    (ttmp->ttyp == SPIKED_PIT))))) {
		if (Blind) feel_location(sx, sy);
		if (canspotmons(mtmp))
		    pline(E_J("There's %s on the other side.",
			      "向こう側に%sがいる。"),
#ifndef MONSTEED
			  a_monnam(mtmp)
#else
			  a_monnam(mrider_or_msteed(mtmp, !canspotmon(mtmp)))
#endif /*MONSTEED*/
			 );
		else {
#ifndef JP
		    You_hear("a monster behind %s.", the(xname(otmp)));
#else
		    pline("%sの向こうに怪物がいるようだ。", xname(otmp));
#endif /*JP*/
		    map_invisible(rx, ry);
		}
		if (flags.verbose)
		    pline(E_J("Perhaps that's why %s cannot move it.",
			      "%sが岩を動かせないのはおそらくそのせいだ。"),
#ifdef STEED
				u.usteed ? y_monnam(u.usteed) :
#endif
				E_J("you","あなた"));
		goto cannot_push;
	    }

	    if (ttmp)
		switch(ttmp->ttyp) {
		case LANDMINE:
		    if (rn2(10)) {
			obj_extract_self(otmp);
			place_object(otmp, rx, ry);
			unblock_point(sx, sy);
			newsym(sx, sy);
#ifndef JP
			pline("KAABLAMM!!!  %s %s land mine.",
			      Tobjnam(otmp, "trigger"),
			      ttmp->madeby_u ? "your" : "a");
#else
			pline("ドグオォォン!!! %sが%s地雷を起爆した。",
			      xname(otmp),
			      ttmp->madeby_u ? "あなたの仕掛けた" : "");
#endif /*JP*/
			blow_up_landmine(ttmp);
			/* if the boulder remains, it should fill the pit */
			fill_pit(u.ux, u.uy);
			if (cansee(rx,ry)) newsym(rx,ry);
			continue;
		    }
		    break;
		case SPIKED_PIT:
		case PIT:
		    obj_extract_self(otmp);
		    /* vision kludge to get messages right;
		       the pit will temporarily be seen even
		       if this is one among multiple boulders */
		    if (!Blind) viz_array[ry][rx] |= IN_SIGHT;
		    if (!flooreffects(otmp, rx, ry, E_J("fall","落ちた"))) {
			place_object(otmp, rx, ry);
		    }
		    if (mtmp && !Blind) newsym(rx, ry);
		    continue;
		case HOLE:
		case TRAPDOOR:
		    if (Blind)
#ifndef JP
			pline("Kerplunk!  You no longer feel %s.",
				the(xname(otmp)));
#else
			pline("ゴトン！ あなたの押していた%sが消えた。",
				xname(otmp));
#endif /*JP*/
		    else
#ifndef JP
			pline("%s%s and %s a %s in the %s!",
			  Tobjnam(otmp,
			   (ttmp->ttyp == TRAPDOOR) ? "trigger" : "fall"),
			  (ttmp->ttyp == TRAPDOOR) ? nul : " into",
			  otense(otmp, "plug"),
			  (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole",
			  surface(rx, ry));
#else
			pline("%sが%sの%sにはまり、ふさいだ！",
			  xname(otmp), surface(rx, ry),
			  (ttmp->ttyp == TRAPDOOR) ? "落とし扉" : "穴");
#endif /*JP*/
		    deltrap(ttmp);
		    delobj(otmp);
		    bury_objs(rx, ry);
		    if (cansee(rx,ry)) newsym(rx,ry);
		    continue;
		case LEVEL_TELEP:
		case TELEP_TRAP:
#ifdef STEED
		    if (u.usteed)
#ifndef JP
			pline("%s pushes %s and suddenly it disappears!",
			      upstart(y_monnam(u.usteed)), the(xname(otmp)));
#else
			pline("%sが押した%sが突然消えた！",
			      y_monnam(u.usteed), xname(otmp));
#endif /*JP*/
		    else
#endif
#ifndef JP
		    You("push %s and suddenly it disappears!",
			the(xname(otmp)));
#else
		    pline("あなたが押した%sが突然消えた！", xname(otmp));
#endif /*JP*/
		    if (ttmp->ttyp == TELEP_TRAP)
			rloco(otmp);
		    else {
			int newlev = random_teleport_level();
			d_level dest;

			if (newlev == depth(&u.uz) || In_endgame(&u.uz))
			    continue;
			obj_extract_self(otmp);
			add_to_migration(otmp);
			get_level(&dest, newlev);
			otmp->ox = dest.dnum;
			otmp->oy = dest.dlevel;
			otmp->owornmask = (long)MIGR_RANDOM;
		    }
		    seetrap(ttmp);
		    continue;
		}
	    if (closed_door(rx, ry))
		goto nopushmsg;
	    if (boulder_hits_pool(otmp, rx, ry, TRUE))
		continue;
	    /*
	     * Re-link at top of fobj chain so that pile order is preserved
	     * when level is restored.
	     */
	    if (otmp != fobj) {
		remove_object(otmp);
		place_object(otmp, otmp->ox, otmp->oy);
	    }

	    {
#ifdef LINT /* static long lastmovetime; */
		long lastmovetime;
		lastmovetime = 0;
#else
		/* note: reset to zero after save/restore cycle */
		static NEARDATA long lastmovetime;
#endif
#ifdef STEED
		if (!u.usteed) {
#endif
		  if (moves > lastmovetime+2 || moves < lastmovetime)
#ifndef JP
		    pline("With %s effort you move %s.",
			  throws_rocks(youmonst.data) ? "little" : "great",
			  the(xname(otmp)));
#else
		    You("%s%sを動かした。",
			  throws_rocks(youmonst.data) ? "軽々と" : "渾身の力をこめて",
			  xname(otmp));
#endif /*JP*/
		  exercise(A_STR, TRUE);
#ifdef STEED
		} else 
#ifndef JP
		    pline("%s moves %s.",
			  upstart(y_monnam(u.usteed)), the(xname(otmp)));
#else
		    pline("%sは%sを動かした。",
			  y_monnam(u.usteed), xname(otmp));
#endif /*JP*/
#endif
		lastmovetime = moves;
	    }

	    /* Move the boulder *after* the message. */
	    if (glyph_is_invisible(levl[rx][ry].glyph))
		unmap_object(rx, ry);
	    movobj(otmp, rx, ry);	/* does newsym(rx,ry) */
	    if (Blind) {
		feel_location(rx,ry);
		feel_location(sx, sy);
	    } else {
		newsym(sx, sy);
	    }
	} else {
	nopushmsg:
#ifdef STEED
	  if (u.usteed)
#ifndef JP
	    pline("%s tries to move %s, but cannot.",
		  upstart(y_monnam(u.usteed)), the(xname(otmp)));
#else
	    pline("%sは%sを動かそうとしたが、駄目だった。",
		  y_monnam(u.usteed), xname(otmp));
#endif /*JP*/
	  else
#endif
#ifndef JP
	    You("try to move %s, but in vain.", the(xname(otmp)));
#else
	    You("%sを動かそうとしたが、無理だった。", xname(otmp));
#endif /*JP*/
	    if (Blind) feel_location(sx, sy);
	cannot_push:
	    if (throws_rocks(youmonst.data)) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
#ifndef JP
		    You("aren't skilled enough to %s %s from %s.",
			(flags.pickup && !In_sokoban(&u.uz))
			    ? "pick up" : "push aside",
			the(xname(otmp)), y_monnam(u.usteed));
#else
		    You("%sに乗ったまま%sを%sるほど乗馬に習熟していない。",
			y_monnam(u.usteed), xname(otmp),
			(flags.pickup && !In_sokoban(&u.uz))
			    ? "拾え" : "どかせ");
#endif /*JP*/
		} else
#endif
		{
		    pline(E_J("However, you can easily %s.",
			      "しかしながら、あなたは簡単に岩を%sる。"),
			(flags.pickup && !In_sokoban(&u.uz))
			    ? E_J("pick it up","拾い上げられ") :
			      E_J("push it aside","脇にどけられ"));
		    if (In_sokoban(&u.uz))
			change_luck(-1);	/* Sokoban guilt */
		    break;
		}
		break;
	    }

	    if (
#ifdef STEED
		!u.usteed &&
#endif	    
		(((!invent || inv_weight() <= -850) &&
		 (!u.dx || !u.dy || (IS_ROCK(levl[u.ux][sy].typ)
				     && IS_ROCK(levl[sx][u.uy].typ))))
		|| verysmall(youmonst.data))) {
		pline(E_J("However, you can squeeze yourself into a small opening.",
			  "しかしながら、あなたは狭い隙間に身体をねじ込むことはできる。"));
		if (In_sokoban(&u.uz))
		    change_luck(-1);	/* Sokoban guilt */
		break;
	    } else
		return (-1);
	}
    }
    return (0);
}

/*
 *  still_chewing()
 *
 *  Chew on a wall, door, or boulder.  Returns TRUE if still eating, FALSE
 *  when done.
 */
STATIC_OVL int
still_chewing(x,y)
    xchar x, y;
{
    struct rm *lev = &levl[x][y];
    struct obj *boulder = sobj_at(BOULDER,x,y);
    const char *digtxt = (char *)0, *dmgtxt = (char *)0;

    if (digging.down)		/* not continuing previous dig (w/ pick-axe) */
	(void) memset((genericptr_t)&digging, 0, sizeof digging);

    if (!boulder && IS_ROCK(lev->typ) && !may_dig(x,y)) {
	You(E_J("hurt your teeth on the %s.",
		"%sに噛みついて歯を欠いてしまった。"),
	    IS_TREES(lev->typ) ? E_J("tree","木") : E_J("hard stone","硬い石"));
	nomul(0);
	return 1;
    } else if (digging.pos.x != x || digging.pos.y != y ||
		!on_level(&digging.level, &u.uz)) {
	digging.down = FALSE;
	digging.chew = TRUE;
	digging.warned = FALSE;
	digging.pos.x = x;
	digging.pos.y = y;
	assign_level(&digging.level, &u.uz);
	/* solid rock takes more work & time to dig through */
	digging.effort =
	    (IS_ROCK(lev->typ) && !IS_TREES(lev->typ) ? 30 : 60) + u.udaminc;
#ifndef JP
	You("start chewing %s %s.",
	    (boulder || IS_TREES(lev->typ)) ? "on a" : "a hole in the",
	    boulder ? "boulder" :
	    IS_TREES(lev->typ) ? "tree" : IS_ROCK(lev->typ) ? "rock" : "door");
#else
	You("%s%sを齧りはじめた。",
	    boulder ? "大岩" :
	    IS_TREES(lev->typ) ? "木" : IS_ROCK(lev->typ) ? "岩" : "扉",
	    (boulder || IS_TREES(lev->typ)) ? "" : "の穴");
#endif /*JP*/
	watch_dig((struct monst *)0, x, y, FALSE);
	return 1;
    } else if ((digging.effort += (30 + u.udaminc)) <= 100)  {
	if (flags.verbose)
#ifndef JP
	    You("%s chewing on the %s.",
		digging.chew ? "continue" : "begin",
		boulder ? "boulder" :
		IS_TREES(lev->typ) ? "tree" :
		IS_ROCK(lev->typ) ? "rock" : "door");
#else
	    You("%s%sを齧りはじめた。",
		digging.chew ? "再び" : "",
		boulder ? "大岩" :
		IS_TREES(lev->typ) ? "木" :
		IS_ROCK(lev->typ) ? "岩" : "扉");
#endif /*JP*/
	digging.chew = TRUE;
	watch_dig((struct monst *)0, x, y, FALSE);
	return 1;
    }

    /* Okay, you've chewed through something */
    u.uconduct.food++;
    u.uhunger += rnd(20);

    if (boulder) {
	delobj(boulder);		/* boulder goes bye-bye */
	You(E_J("eat the boulder.","大岩を食べた。"));	/* yum */

	/*
	 *  The location could still block because of
	 *	1. More than one boulder
	 *	2. Boulder stuck in a wall/stone/door.
	 *
	 *  [perhaps use does_block() below (from vision.c)]
	 */
	if (IS_ROCK(lev->typ) || closed_door(x,y) || sobj_at(BOULDER,x,y)) {
	    block_point(x,y);	/* delobj will unblock the point */
	    /* reset dig state */
	    (void) memset((genericptr_t)&digging, 0, sizeof digging);
	    return 1;
	}

    } else if (IS_WALL(lev->typ)) {
	if (*in_rooms(x, y, SHOPBASE)) {
	    add_damage(x, y, 10L * ACURRSTR);
	    dmgtxt = E_J("damage","壁を傷物にした");
	}
	digtxt = E_J("chew a hole in the wall.",
		     "壁を齧って穴を開けた。");
	if (level.flags.is_maze_lev) {
	    lev->typ = ROOM;
	} else if (level.flags.is_cavernous_lev && !in_town(x, y)) {
	    lev->typ = CORR;
	} else {
	    lev->typ = DOOR;
	    lev->doormask = D_NODOOR;
	}
    } else if (IS_TREE(lev->typ)) {
	digtxt = E_J("chew through the tree.",
		     "木を齧って倒した。");
	lev->typ = ROOM;
    } else if (lev->typ == SDOOR) {
	if (lev->doormask & D_TRAPPED) {
	    lev->doormask = D_NODOOR;
	    b_trapped(E_J("secret door","隠し扉"), 0);
	} else {
	    digtxt = E_J("chew through the secret door.",
			 "隠し扉を齧って穴を開けた。");
	    lev->doormask = D_BROKEN;
	}
	lev->typ = DOOR;

    } else if (IS_DOOR(lev->typ)) {
	if (*in_rooms(x, y, SHOPBASE)) {
	    add_damage(x, y, 400L);
	    dmgtxt = E_J("break","扉を壊した");
	}
	if (lev->doormask & D_TRAPPED) {
	    lev->doormask = D_NODOOR;
	    b_trapped(E_J("door","扉"), 0);
	} else {
	    digtxt = E_J("chew through the door.",
			 "扉を齧って穴を開けた。");
	    lev->doormask = D_BROKEN;
	}

    } else { /* STONE or SCORR */
	digtxt = E_J("chew a passage through the rock.",
		     "岩を齧ってトンネルを作った。");
	lev->typ = CORR;
    }

    unblock_point(x, y);	/* vision */
    newsym(x, y);
    if (digtxt) You(digtxt);	/* after newsym */
    if (dmgtxt) pay_for_damage(dmgtxt, FALSE);
    (void) memset((genericptr_t)&digging, 0, sizeof digging);
    return 0;
}

#endif /* OVL2 */
#ifdef OVLB

void
movobj(obj, ox, oy)
register struct obj *obj;
register xchar ox, oy;
{
	/* optimize by leaving on the fobj chain? */
	remove_object(obj);
	newsym(obj->ox, obj->oy);
	place_object(obj, ox, oy);
	newsym(ox, oy);
}

#ifdef SINKS
static NEARDATA const char fell_on_sink[] = E_J("fell onto a sink",
						"流し台に墜落した");

STATIC_OVL void
dosinkfall()
{
	register struct obj *obj;

	if (is_floater(youmonst.data) || (HLevitation & FROMOUTSIDE)) {
	    You(E_J("wobble unsteadily for a moment.",
		    "一瞬不安定によろめいた。"));
	} else {
	    long save_ELev = ELevitation, save_HLev = HLevitation;

	    /* fake removal of levitation in advance so that final
	       disclosure will be right in case this turns out to
	       be fatal; fortunately the fact that rings and boots
	       are really still worn has no effect on bones data */
	    ELevitation = HLevitation = 0L;
	    You(E_J("crash to the floor!","床めがけて墜落した！"));
	    losehp(rn1(8, 25 - (int)ACURR(A_CON)),
		   fell_on_sink, NO_KILLER_PREFIX);
	    exercise(A_DEX, FALSE);
	    selftouch(E_J("Falling, you","落下中、あなたは"));
	    for (obj = level.objects[u.ux][u.uy]; obj; obj = obj->nexthere)
		if (obj->oclass == WEAPON_CLASS || is_weptool(obj)) {
		    You(E_J("fell on %s.","%sの上に落ちた。"), doname(obj));
		    losehp(rnd(3), fell_on_sink, NO_KILLER_PREFIX);
		    exercise(A_CON, FALSE);
		}
	    ELevitation = save_ELev;
	    HLevitation = save_HLev;
	}

	ELevitation &= ~W_ARTI;
	HLevitation &= ~(I_SPECIAL|TIMEOUT);
	HLevitation++;
	if(uleft && uleft->otyp == RIN_LEVITATION) {
	    obj = uleft;
	    Ring_off(obj);
	    off_msg(obj);
	}
	if(uright && uright->otyp == RIN_LEVITATION) {
	    obj = uright;
	    Ring_off(obj);
	    off_msg(obj);
	}
	if(uarmf && uarmf->otyp == LEVITATION_BOOTS) {
	    obj = uarmf;
	    (void)Boots_off();
	    off_msg(obj);
	}
	HLevitation--;
}
#endif

boolean
may_dig(x,y)
register xchar x,y;
/* intended to be called only on ROCKs */
{
    return (boolean)(!((IS_STWALL(levl[x][y].typ) || IS_TREES(levl[x][y].typ)) &&
			(levl[x][y].wall_info & W_NONDIGGABLE)));
}

boolean
may_passwall(x,y)
register xchar x,y;
{
   return (boolean)(!((IS_STWALL(levl[x][y].typ) || IS_TREES(levl[x][y].typ)) &&
			(levl[x][y].wall_info & W_NONPASSWALL)));
}

#endif /* OVLB */
#ifdef OVL1

boolean
bad_rock(mdat,x,y)
struct permonst *mdat;
register xchar x,y;
{
	return((boolean) ((In_sokoban(&u.uz) && sobj_at(BOULDER,x,y)) ||
	       (IS_ROCK(levl[x][y].typ)
		    && (!tunnels(mdat) || needspick(mdat) || !may_dig(x,y))
		    && !(passes_walls(mdat) && may_passwall(x,y)))));
}

boolean
invocation_pos(x, y)
xchar x, y;
{
	return((boolean)(Invocation_lev(&u.uz) && x == inv_pos.x && y == inv_pos.y));
}

#endif /* OVL1 */
#ifdef OVL3

/* return TRUE if (dx,dy) is an OK place to move
 * mode is one of DO_MOVE, TEST_MOVE or TEST_TRAV
 */
boolean 
test_move(ux, uy, dx, dy, mode)
int ux, uy, dx, dy;
int mode;
{
    int x = ux+dx;
    int y = uy+dy;
    register struct rm *tmpr = &levl[x][y];
    register struct rm *ust;

    /*
     *  Check for physical obstacles.  First, the place we are going.
     */
    if (IS_ROCK(tmpr->typ) || tmpr->typ == IRONBARS) {
	if (Blind && mode == DO_MOVE) feel_location(x,y);
	if (Passes_walls && may_passwall(x,y)) {
	    ;	/* do nothing */
	} else if (tmpr->typ == IRONBARS) {
	    if (!(Passes_walls || passes_bars(youmonst.data)))
		return FALSE;
	} else if (tunnels(youmonst.data) && !needspick(youmonst.data)) {
	    /* Eat the rock. */
	    if (mode == DO_MOVE && still_chewing(x,y)) return FALSE;
	} else if (flags.autodig && !flags.run && !flags.nopick &&
		   uwep && is_pick(uwep)) {
	/* MRKR: Automatic digging when wielding the appropriate tool */
	    if (mode == DO_MOVE)
		(void) use_pick_axe2(uwep);
	    return FALSE;
	} else {
	    if (mode == DO_MOVE) {
collision:
		if (Rocketskate
#ifdef STEED
		    && !u.usteed
#endif
		    ) {
#ifndef JP
		    static const char *bang[4] = { "Bang", "Wham", "Clash", "Ouch" };
#else
		    static const char *bang[4] = { "ドシン", "バン", "ドン", "いてっ"};
#endif /*JP*/
		    char buf[BUFSZ];
		    pline(E_J("%s!  You crash into %s!","%s！ あなたは%sに激突した！"),
			bang[rn2(4)], placenam(tmpr->typ));
		    Sprintf(buf, E_J("crashed into %s","%sに激突して"), placenam(tmpr->typ));
		    losehp(rnd(4), buf, E_J(NO_KILLER_PREFIX,KILLED_BY));
		    nomul(0); /* clear waterwalking prop by rocketskate */
		    if ((is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy) || is_swamp(u.ux,u.uy)) &&
			!Wwalking && !Levitation && !Flying && !is_clinger(youmonst.data)) {
			    spoteffects(TRUE);
		    }
		    return FALSE;
		}
		if (Is_stronghold(&u.uz) && is_db_wall(x,y))
		    pline_The(E_J("drawbridge is up!","跳ね橋は上がっている！"));
		if (Passes_walls && !may_passwall(x,y) && In_sokoban(&u.uz))
	    	    pline_The(E_J("Sokoban walls resist your ability.",
				  "倉庫番の壁はあなたの能力を受け付けない。"));
	    }
	    return FALSE;
	}
    } else if (IS_DOOR(tmpr->typ)) {
	if (closed_door(x,y)) {
	    if (Blind && mode == DO_MOVE) feel_location(x,y);
	    if (Passes_walls)
		;	/* do nothing */
	    else if (can_ooze(&youmonst)) {
		if (mode == DO_MOVE) You(E_J("ooze under the door.","扉の下からにじみ出た。"));
	    } else if (tunnels(youmonst.data) && !needspick(youmonst.data)) {
		/* Eat the door. */
		if (mode == DO_MOVE && still_chewing(x,y)) return FALSE;
	    } else {
		if (mode == DO_MOVE) {
		    if (amorphous(youmonst.data))
			You(E_J("try to ooze under the door, but can't squeeze your possessions through.",
				"扉の下からにじみ出ようとしたが、所持品を通り抜けさせることができなかった。"));
		    else if (Rocketskate
#ifdef STEED
			     && !u.usteed
#endif
			    ) goto collision;
		    else if (x == ux || y == uy) {
			if (Blind || Stunned || ACURR(A_DEX) < 10 || Fumbling) {
#ifdef STEED
			    if (u.usteed) {
				You_cant(E_J("lead %s through that closed door.",
					     "%sに閉じた扉をくぐらせることはできない。"),
				         y_monnam(u.usteed));
			    } else
#endif
			    {
			        pline(E_J("Ouch!  You bump into a door.",
					  "いてっ！ あなたは扉にぶつかった！"));
			        exercise(A_DEX, FALSE);
			    }
			} else pline(E_J("That door is closed.","扉は閉じている。"));
		    }
		} else if (mode == TEST_TRAV) goto testdiag;
		return FALSE;
	    }
	} else {
	testdiag:
	    if (dx && dy && !Passes_walls
		&& ((tmpr->doormask & ~D_BROKEN)
#ifdef REINCARNATION
		    || Is_rogue_level(&u.uz)
#endif
		    || block_door(x,y))) {
		/* Diagonal moves into a door are not allowed. */
		if (Blind && mode == DO_MOVE)
		    feel_location(x,y);
		return FALSE;
	    }
	}
    }
    if (dx && dy
	    && bad_rock(youmonst.data,ux,y) && bad_rock(youmonst.data,x,uy)) {
	/* Move at a diagonal. */
	if (In_sokoban(&u.uz)) {
	    if (mode == DO_MOVE)
		You(E_J("cannot pass that way.","その方向には進めない。"));
	    return FALSE;
	}
	if (bigmonst(youmonst.data)) {
	    if (mode == DO_MOVE)
		Your(E_J("body is too large to fit through.",
			 "身体は隙間を通り抜けるには大きすぎる。"));
	    return FALSE;
	}
	if (invent && (inv_weight() + weight_cap() > 600)) {
	    if (mode == DO_MOVE)
		You(E_J("are carrying too much to get through.",
			"物を持ちすぎていて、通り抜けられない。"));
	    return FALSE;
	}
    }
    /* Pick travel path that does not require crossing a trap.
     * Avoid water and lava using the usual running rules.
     * (but not u.ux/u.uy because findtravelpath walks toward u.ux/u.uy) */
    if (flags.run == 8 && mode != DO_MOVE && (x != u.ux || y != u.uy)) {
	struct trap* t = t_at(x, y);

	if ((t && t->tseen) ||
	    (!Levitation && !Flying &&
	     !is_clinger(youmonst.data) &&
	     (is_pool(x, y) || is_lava(x, y) || is_swamp(x, y)) && levl[x][y].seenv))
	    return FALSE;
    }

    ust = &levl[ux][uy];

    /* Now see if other things block our way . . */
    if (dx && dy && !Passes_walls
		     && (IS_DOOR(ust->typ) && ((ust->doormask & ~D_BROKEN)
#ifdef REINCARNATION
			     || Is_rogue_level(&u.uz)
#endif
			     || block_entry(x, y))
			 )) {
	/* Can't move at a diagonal out of a doorway with door. */
	return FALSE;
    }

    if (sobj_at(BOULDER,x,y) && (In_sokoban(&u.uz) || !Passes_walls)) {
	if (!(Blind || Hallucination) && (flags.run >= 2) && mode != TEST_TRAV)
	    return FALSE;
	if (mode == DO_MOVE) {
	    /* tunneling monsters will chew before pushing */
	    if (tunnels(youmonst.data) && !needspick(youmonst.data) &&
		!In_sokoban(&u.uz)) {
		if (still_chewing(x,y)) return FALSE;
	    } else
		if (moverock() < 0) return FALSE;
	} else if (mode == TEST_TRAV) {
	    struct obj* obj;

	    /* don't pick two boulders in a row, unless there's a way thru */
	    if (sobj_at(BOULDER,ux,uy) && !In_sokoban(&u.uz)) {
		if (!Passes_walls &&
		    !(tunnels(youmonst.data) && !needspick(youmonst.data)) &&
		    !carrying(PICK_AXE) && !carrying(DWARVISH_MATTOCK) &&
		    !((obj = carrying(WAN_DIGGING)) &&
		      !objects[obj->otyp].oc_name_known))
		    return FALSE;
	    }
	}
	/* assume you'll be able to push it when you get there... */
    }

    /* OK, it is a legal place to move. */
    return TRUE;
}

/*
 * Find a path from the destination (u.tx,u.ty) back to (u.ux,u.uy).
 * A shortest path is returned.  If guess is TRUE, consider various
 * inaccessible locations as valid intermediate path points.
 * Returns TRUE if a path was found.
 */
static boolean
findtravelpath(guess)
boolean guess;
{
    /* if travel to adjacent, reachable location, use normal movement rules */
    if (!guess && iflags.travel1 && distmin(u.ux, u.uy, u.tx, u.ty) == 1) {
	flags.run = 0;
	if (test_move(u.ux, u.uy, u.tx-u.ux, u.ty-u.uy, TEST_MOVE)) {
	    u.dx = u.tx-u.ux;
	    u.dy = u.ty-u.uy;
	    nomul(0);
	    iflags.travelcc.x = iflags.travelcc.y = -1;
	    return TRUE;
	}
	flags.run = 8;
    }
    if (u.tx != u.ux || u.ty != u.uy) {
	xchar travel[COLNO][ROWNO];
	xchar travelstepx[2][COLNO*ROWNO];
	xchar travelstepy[2][COLNO*ROWNO];
	xchar tx, ty, ux, uy;
	int n = 1;			/* max offset in travelsteps */
	int set = 0;			/* two sets current and previous */
	int radius = 1;			/* search radius */
	int i;

	/* If guessing, first find an "obvious" goal location.  The obvious
	 * goal is the position the player knows of, or might figure out
	 * (couldsee) that is closest to the target on a straight path.
	 */
	if (guess) {
	    tx = u.ux; ty = u.uy; ux = u.tx; uy = u.ty;
	} else {
	    tx = u.tx; ty = u.ty; ux = u.ux; uy = u.uy;
	}

    noguess:
	(void) memset((genericptr_t)travel, 0, sizeof(travel));
	travelstepx[0][0] = tx;
	travelstepy[0][0] = ty;

	while (n != 0) {
	    int nn = 0;

	    for (i = 0; i < n; i++) {
		int dir;
		int x = travelstepx[set][i];
		int y = travelstepy[set][i];
		static int ordered[] = { 0, 2, 4, 6, 1, 3, 5, 7 };
		/* no diagonal movement for grid bugs */
		int dirmax = u.umonnum == PM_GRID_BUG ? 4 : 8;

		for (dir = 0; dir < dirmax; ++dir) {
		    int nx = x+xdir[ordered[dir]];
		    int ny = y+ydir[ordered[dir]];

		    if (!isok(nx, ny)) continue;
		    if ((!Passes_walls && !can_ooze(&youmonst) &&
			closed_door(x, y)) || sobj_at(BOULDER, x, y)) {
			/* closed doors and boulders usually
			 * cause a delay, so prefer another path */
			if (travel[x][y] > radius-3) {
			    travelstepx[1-set][nn] = x;
			    travelstepy[1-set][nn] = y;
			    /* don't change travel matrix! */
			    nn++;
			    continue;
			}
		    }
		    if (test_move(x, y, nx-x, ny-y, TEST_TRAV) &&
			(levl[nx][ny].seenv || (!Blind && couldsee(nx, ny)))) {
			if (nx == ux && ny == uy) {
			    if (!guess) {
				u.dx = x-ux;
				u.dy = y-uy;
				if (x == u.tx && y == u.ty) {
				    nomul(0);
				    /* reset run so domove run checks work */
				    flags.run = 8;
				    iflags.travelcc.x = iflags.travelcc.y = -1;
				}
				return TRUE;
			    }
			} else if (!travel[nx][ny]) {
			    travelstepx[1-set][nn] = nx;
			    travelstepy[1-set][nn] = ny;
			    travel[nx][ny] = radius;
			    nn++;
			}
		    }
		}
	    }
	    
	    n = nn;
	    set = 1-set;
	    radius++;
	}

	/* if guessing, find best location in travel matrix and go there */
	if (guess) {
	    int px = tx, py = ty;	/* pick location */
	    int dist, nxtdist, d2, nd2;

	    dist = distmin(ux, uy, tx, ty);
	    d2 = dist2(ux, uy, tx, ty);
	    for (tx = 1; tx < COLNO; ++tx)
		for (ty = 0; ty < ROWNO; ++ty)
		    if (travel[tx][ty]) {
			nxtdist = distmin(ux, uy, tx, ty);
			if (nxtdist == dist && couldsee(tx, ty)) {
			    nd2 = dist2(ux, uy, tx, ty);
			    if (nd2 < d2) {
				/* prefer non-zigzag path */
				px = tx; py = ty;
				d2 = nd2;
			    }
			} else if (nxtdist < dist && couldsee(tx, ty)) {
			    px = tx; py = ty;
			    dist = nxtdist;
			    d2 = dist2(ux, uy, tx, ty);
			}
		    }

	    if (px == u.ux && py == u.uy) {
		/* no guesses, just go in the general direction */
		u.dx = sgn(u.tx - u.ux);
		u.dy = sgn(u.ty - u.uy);
		if (test_move(u.ux, u.uy, u.dx, u.dy, TEST_MOVE))
		    return TRUE;
		goto found;
	    }
	    tx = px;
	    ty = py;
	    ux = u.ux;
	    uy = u.uy;
	    set = 0;
	    n = radius = 1;
	    guess = FALSE;
	    goto noguess;
	}
	return FALSE;
    }

found:
    u.dx = 0;
    u.dy = 0;
    nomul(0);
    return FALSE;
}

void
domove()
{
	register struct monst *mtmp;
	register struct rm *tmpr;
	register xchar x,y;
	struct trap *trap;
	int wtcap;
	boolean on_ice;
	xchar chainx, chainy, ballx, bally;	/* ball&chain new positions */
	int bc_control;				/* control for ball&chain */
	boolean cause_delay = FALSE;	/* dragging ball will skip a move */
	const char *predicament;

	u_wipe_engr(rnd(5));

	if (flags.travel) {
	    if (!findtravelpath(FALSE))
		(void) findtravelpath(TRUE);
	    iflags.travel1 = 0;
	}

	if(((wtcap = near_capacity()) >= OVERLOADED
	    || (wtcap > SLT_ENCUMBER &&
		(Upolyd ? (u.mh < 5 && u.mh != u.mhmax)
			: (u.uhp < 10 && u.uhp != u.uhpmax))))
	   && !Is_airlevel(&u.uz)) {
	    if(wtcap < OVERLOADED) {
		You(E_J("don't have enough stamina to move.",
			"動けるだけの体力がない。"));
		exercise(A_CON, FALSE);
	    } else
		You(E_J("collapse under your load.",
			"重荷の下で倒れた。"));
	    nomul(0);
	    return;
	}
	if(u.uswallow) {
		u.dx = u.dy = 0;
		u.ux = x = u.ustuck->mx;
		u.uy = y = u.ustuck->my;
		mtmp = u.ustuck;
	} else {
		if (Is_airlevel(&u.uz) && rn2(4) &&
			!Levitation && !Flying) {
		    switch(rn2(3)) {
		    case 0:
			You(E_J("tumble in place.","その場で回転した。"));
			exercise(A_DEX, FALSE);
			break;
		    case 1:
			You_cant(E_J("control your movements very well.",
				     "自分の動きをうまく制御できない。")); break;
		    case 2:
			pline(E_J("It's hard to walk in thin air.",
				  "何もない空気の上を歩くのは困難だ。"));
			exercise(A_DEX, TRUE);
			break;
		    }
		    return;
		}

		/* check slippery ice */
		on_ice = !Levitation && is_ice(u.ux, u.uy);
		if (on_ice) {
		    static int skates = 0;
		    if (!skates) skates = find_skates();
		    if ((uarmf && uarmf->otyp == skates)
			    || resists_cold(&youmonst) || Flying
			    || is_floater(youmonst.data) || is_clinger(youmonst.data)
			    || is_whirly(youmonst.data))
			on_ice = FALSE;
		    else if (!rn2(Cold_resistance ? 3 : 2)) {
			HFumbling |= FROMOUTSIDE;
			HFumbling &= ~TIMEOUT;
			HFumbling += 1;  /* slip on next move */
		    }
		}
		if (!on_ice && (HFumbling & FROMOUTSIDE) && !Rocketskate)
		    HFumbling &= ~FROMOUTSIDE;

		x = u.ux + u.dx;
		y = u.uy + u.dy;
		if(Stunned || (Confusion && !rn2(5))) {
			register int tries = 0;

			do {
				if(tries++ > 50) {
					nomul(0);
					return;
				}
				confdir();
				x = u.ux + u.dx;
				y = u.uy + u.dy;
			} while(!isok(x, y) || bad_rock(youmonst.data, x, y));
		}
		/* turbulence might alter your actual destination */
		if (u.uinwater) {
			water_friction();
			if (!u.dx && !u.dy) {
				nomul(0);
				return;
			}
			x = u.ux + u.dx;
			y = u.uy + u.dy;
		}
		if(!isok(x, y)) {
			nomul(0);
			return;
		}
		if (((trap = t_at(x, y)) && trap->tseen) ||
		    (Blind && !Levitation && !Flying &&
		     !is_clinger(youmonst.data) &&
		     (is_pool(x, y) || is_lava(x, y)) && levl[x][y].seenv)) {
			if(flags.run >= 2) {
				nomul(0);
				flags.move = 0;
				return;
			} else
				nomul(0);
		}

		if (u.ustuck && (x != u.ustuck->mx || y != u.ustuck->my)) {
		    if (distu(u.ustuck->mx, u.ustuck->my) > 2) {
			/* perhaps it fled (or was teleported or ... ) */
			u.ustuck = 0;
		    } else if (sticks(youmonst.data)) {
			/* When polymorphed into a sticking monster,
			 * u.ustuck means it's stuck to you, not you to it.
			 */
			You(E_J("release %s.",
				"%sを放した。"), mon_nam(u.ustuck));
			u.ustuck = 0;
		    } else {
			/* If holder is asleep or paralyzed:
			 *	37.5% chance of getting away,
			 *	12.5% chance of waking/releasing it;
			 * otherwise:
			 *	 7.5% chance of getting away.
			 * [strength ought to be a factor]
			 * If holder is tame and there is no conflict,
			 * guaranteed escape.
			 */
			switch (rn2(!u.ustuck->mcanmove ? 8 : 40)) {
			case 0: case 1: case 2:
			pull_free:
			    You(E_J("pull free from %s.",
				    "%sから自分を引き離した。"), mon_nam(u.ustuck));
			    u.ustuck = 0;
			    break;
			case 3:
			    if (!u.ustuck->mcanmove) {
				/* it's free to move on next turn */
				u.ustuck->mfrozen = 1;
				u.ustuck->msleeping = 0;
			    }
			    /*FALLTHRU*/
			default:
			    if (u.ustuck->mtame &&
				!Conflict && !u.ustuck->mconf)
				goto pull_free;
			    You(E_J("cannot escape from %s!",
				    "%sから逃れられない！"), mon_nam(u.ustuck));
			    nomul(0);
			    return;
			}
		    }
		}

		mtmp = m_at(x,y);
		if (mtmp) {
			/* Don't attack if you're running, and can see it */
			/* We should never get here if forcefight */
			if (flags.run &&
			    ((!Blind && mon_visible(mtmp) &&
			      ((mtmp->m_ap_type != M_AP_FURNITURE &&
				mtmp->m_ap_type != M_AP_OBJECT) ||
			       Protection_from_shape_changers)) ||
			     sensemon(mtmp))) {
				nomul(0);
				flags.move = 0;
				return;
			}
#ifdef MONSTEED		/* same for msteed */
			if (flags.run && is_mriding(mtmp) &&
			    ((!Blind && mon_visible(mtmp->mlink) &&
			      ((mtmp->mlink->m_ap_type != M_AP_FURNITURE &&
				mtmp->mlink->m_ap_type != M_AP_OBJECT) ||
			       Protection_from_shape_changers)) ||
			     sensemon(mtmp->mlink))) {
				nomul(0);
				flags.move = 0;
				return;
			}
#endif /*MONSTEED*/
		}
	}

	u.ux0 = u.ux;
	u.uy0 = u.uy;
	bhitpos.x = x;
	bhitpos.y = y;
	tmpr = &levl[x][y];

	/* attack monster */
	if(mtmp) {
	    struct monst *mseen;
	    mseen = canspotmon(mtmp) ? mtmp : (struct monst *) 0;
#ifdef MONSTEED
	    if (!mseen && is_mriding(mtmp) && canspotmon(mtmp->mlink))
		mseen = mtmp->mlink;
#endif /*MONSTEED*/
	    nomul(0);
	    /* only attack if we know it's there */
	    /* or if we used the 'F' command to fight blindly */
	    /* or if it hides_under, in which case we call attack() to print
	     * the Wait! message.
	     * This is different from ceiling hiders, who aren't handled in
	     * attack().
	     */

	    /* If they used a 'm' command, trying to move onto a monster
	     * prints the below message and wastes a turn.  The exception is
	     * if the monster is unseen and the player doesn't remember an
	     * invisible monster--then, we fall through to attack() and
	     * attack_check(), which still wastes a turn, but prints a
	     * different message and makes the player remember the monster.		     */
	    if(flags.nopick &&
		  (mseen || glyph_is_invisible(levl[x][y].glyph))){
		if(mtmp->m_ap_type && !Protection_from_shape_changers
						    && !sensemon(mtmp))
		    stumble_onto_mimic(mtmp);
		else if (mseen->mpeaceful && !Hallucination)
		    pline(E_J("Pardon me, %s.",
			      "ちょっと失礼、%sさん。"), m_monnam(mseen));
		else
		    You(E_J("move right into %s.",
			    "%sめがけて進もうとした。"), mon_nam(mseen));
		return;
	    }
	    if(flags.forcefight || !mtmp->mundetected || sensemon(mtmp) ||
		mon_warning(mtmp) ||
		    ((hides_under(mtmp->data) || mtmp->data->mlet == S_EEL) &&
			!is_safepet(mtmp))){
		gethungry();
		if(wtcap >= HVY_ENCUMBER && moves%3) {
		    if (Upolyd && u.mh > 1) {
			u.mh--;
		    } else if (!Upolyd && u.uhp > 1) {
			u.uhp--;
		    } else {
			You(E_J("pass out from exertion!",
				"過労で気絶した！"));
			exercise(A_CON, FALSE);
			fall_asleep(-10, FALSE);
		    }
		}
		if(multi < 0) return;	/* we just fainted */

		/* try to attack; note that it might evade */
		/* also, we don't attack tame when _safepet_ */
#ifdef MONSTEED
		mtmp = target_rider_or_steed(&youmonst, mtmp);
#endif /*MONSTEED*/
		if(attack(mtmp)) return;
	    }
	}

	/* specifying 'F' with no monster wastes a turn */
	if (flags.forcefight ||
	    /* remembered an 'I' && didn't use a move command */
	    (glyph_is_invisible(levl[x][y].glyph) && !flags.nopick)) {
		boolean expl = (Upolyd && attacktype(youmonst.data, AT_EXPL));
	    	char buf[BUFSZ];
		Sprintf(buf,E_J("a vacant spot on the %s",
				"%sの上の何もない場所"), surface(x,y));
#ifndef JP
		You("%s %s.",
		    expl ? "explode at" : "attack",
		    !Underwater ? "thin air" :
		    is_pool(x,y) ? "empty water" : buf);
#else
		You("%s%sした。",
		    !Underwater ? "何もない空間" :
		    is_pool(x,y) ? "何もない水場" : buf,
		    expl ? "めがけて爆発" : "を攻撃");
#endif /*JP*/
		unmap_object(x, y); /* known empty -- remove 'I' if present */
		newsym(x, y);
		nomul(0);
		if (expl) {
		    u.mh = -1;		/* dead in the current form */
		    rehumanize();
		}
		return;
	}
	if (glyph_is_invisible(levl[x][y].glyph)) {
	    unmap_object(x, y);
	    newsym(x, y);
	}
	/* not attacking an animal, so we try to move */
#ifdef STEED
	if (u.usteed && !u.usteed->mcanmove && (u.dx || u.dy)) {
#ifndef JP
		pline("%s won't move!", upstart(y_monnam(u.usteed)));
#else
		pline("%sは動こうとしない！", y_monnam(u.usteed));
#endif /*JP*/
		nomul(0);
		return;
	} else
#endif
	if(!youmonst.data->mmove) {
		You(E_J("are rooted %s.","%sに根付いている。"),
		    Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) ?
		    E_J("in place","この場") : E_J("to the ground","地面"));
		nomul(0);
		return;
	}
	if(u.utrap) {
		if(u.utraptype == TT_PIT) {
		    if (!rn2(2) && sobj_at(BOULDER, u.ux, u.uy)) {
			Your(E_J("%s gets stuck in a crevice.",
				 "%sが裂け目に挟み込まれている。"), body_part(LEG));
			display_nhwindow(WIN_MESSAGE, FALSE);
			clear_nhwindow(WIN_MESSAGE);
			You(E_J("free your %s.",
				"%sを引き抜いた。"), body_part(LEG));
		    } else if (!(--u.utrap)) {
			You(E_J("%s to the edge of the pit.",
				"落とし穴の縁まで%s。"),
				(In_sokoban(&u.uz) && Levitation) ?
				E_J("struggle against the air currents and float",
				    "気流に逆らってなんとか浮かび上がった") :
#ifdef STEED
				u.usteed ? E_J("ride","乗騎を進めた") :
#endif
				E_J("crawl","はい上がった"));
			fill_pit(u.ux, u.uy);
			vision_full_recalc = 1;	/* vision limits change */
		    } else if (flags.verbose) {
#ifdef STEED
			if (u.usteed)
#ifndef JP
			    Norep("%s is still in a pit.",
				  upstart(y_monnam(u.usteed)));
#else
			    Norep("%sがまだ落とし穴にはまっている。",
				  y_monnam(u.usteed));
#endif /*JP*/
			else
#endif
			Norep( (Hallucination && !rn2(5)) ?
				E_J("You've fallen, and you can't get up.",
				    "あなたは倒れ、立ち上がれない。") :
				E_J("You are still in a pit.",
				    "あなたはまだ落とし穴にはまっている。") );
		    }
		} else if (u.utraptype == TT_LAVA) {
		    struggle_sub(E_J("stuck in the lava","は溶岩にはまっている。"));
		    if(!is_lava(x,y)) {
			u.utrap--;
			if((u.utrap & 0xff) == 0) {
#ifdef STEED
			    if (u.usteed)
				You(E_J("lead %s to the edge of the lava.",
					"%sを溶岩の縁まで進めた。"),
				    y_monnam(u.usteed));
			    else
#endif
			     You(E_J("pull yourself to the edge of the lava.",
				     "自分の身体を溶岩から引き抜いた。" ));
			    u.utrap = 0;
			}
		    }
		    u.umoved = TRUE;
		} else if (u.utraptype == TT_WEB) {
		    if(uwep && uwep->oartifact == ART_STING) {
			u.utrap = 0;
			pline(E_J("Sting cuts through the web!",
				  "スティングが蜘蛛の巣を切り払った！"));
			return;
		    }
		    if(--u.utrap) {
			struggle_sub(E_J("stuck to the web","は蜘蛛の巣にかかっている。"));
		    } else {
#ifdef STEED
			if (u.usteed)
#ifndef JP
			    pline("%s breaks out of the web.",
				  upstart(y_monnam(u.usteed)));
#else
			    pline("%sは蜘蛛の巣を引きちぎった。",
				  y_monnam(u.usteed));
#endif /*JP*/
			else
#endif
			You(E_J("disentangle yourself.","蜘蛛の巣をふりほどいた。"));
		    }
		} else if (u.utraptype == TT_SWAMP) {
		    if(--u.utrap) {
			struggle_sub(E_J("stuck in the mud","は泥沼にはまっている。"));
		    }
		} else if (u.utraptype == TT_INFLOOR) {
		    if(--u.utrap) {
			if(flags.verbose) {
			    predicament = E_J("stuck in the","にはまっている。");
#ifdef STEED
			    if (u.usteed)
#ifndef JP
				Norep("%s is %s %s.",
				      upstart(y_monnam(u.usteed)),
				      predicament, surface(u.ux, u.uy));
#else
				Norep("%sは%s%s",
				      y_monnam(u.usteed), surface(u.ux, u.uy),
				      predicament);
#endif /*JP*/
			    else
#endif
#ifndef JP
			    Norep("You are %s %s.", predicament,
				  surface(u.ux, u.uy));
#else
			    Norep("あなたは%s%s", surface(u.ux, u.uy),
				  predicament);
#endif /*JP*/
			}
		    } else {
#ifdef STEED
			if (u.usteed)
#ifndef JP
			    pline("%s finally wiggles free.",
				  upstart(y_monnam(u.usteed)));
#else
			    pline("%sはようやく抜け出すことができた。",
				  y_monnam(u.usteed));
#endif /*JP*/
			else
#endif
			You(E_J("finally wiggle free.",
				"ようやく抜け出すことができた。"));
		    }
		} else {
		    if(flags.verbose) {
			predicament = E_J("caught in a bear trap",
					  "はトラバサミに捕らえられている。");
#ifdef STEED
			if (u.usteed)
#ifndef JP
			    Norep("%s is %s.", upstart(y_monnam(u.usteed)),
				  predicament);
#else
			    Norep("%s%s", y_monnam(u.usteed),
				  predicament);
#endif /*JP*/
			else
#endif
			Norep(E_J("You are %s.","あなた%s"), predicament);
		    }
		    if((u.dx && u.dy) || !rn2(5)) u.utrap--;
		}
		return;
	}

	if (!test_move(u.ux, u.uy, x-u.ux, y-u.uy, DO_MOVE)) {
	    flags.move = 0;
	    nomul(0);
	    return;
	}

	/* Move ball and chain.  */
	if (Punished)
	    if (!drag_ball(x,y, &bc_control, &ballx, &bally, &chainx, &chainy,
			&cause_delay, TRUE))
		return;

	/* Check regions entering/leaving */
	if (!in_out_region(x,y))
	    return;

 	/* now move the hero */
	mtmp = m_at(x, y);
	u.ux += u.dx;
	u.uy += u.dy;
#ifdef STEED
	/* Move your steed, too */
	if (u.usteed) {
		u.usteed->mx = u.ux;
		u.usteed->my = u.uy;
		exercise_steed();
	}
#endif

	/*
	 * If safepet at destination then move the pet to the hero's
	 * previous location using the same conditions as in attack().
	 * there are special extenuating circumstances:
	 * (1) if the pet dies then your god angers,
	 * (2) if the pet gets trapped then your god may disapprove,
	 * (3) if the pet was already trapped and you attempt to free it
	 * not only do you encounter the trap but you may frighten your
	 * pet causing it to go wild!  moral: don't abuse this privilege.
	 *
	 * Ceiling-hiding pets are skipped by this section of code, to
	 * be caught by the normal falling-monster code.
	 */
	if (is_safepet(mtmp) && !(is_hider(mtmp->data) && mtmp->mundetected)) {
	    /* if trapped, there's a chance the pet goes wild */
	    if (mtmp->mtrapped) {
		if (!rn2(mtmp->mtame)) {
		    mtmp->mtame = mtmp->mpeaceful = mtmp->msleeping = 0;
		    if (mtmp->mleashed) m_unleash(mtmp, TRUE);
		    growl(mtmp);
		} else {
		    yelp(mtmp);
		}
	    }
	    mtmp->mundetected = 0;
	    if (mtmp->m_ap_type) seemimic(mtmp);
	    else if (!mtmp->mtame) newsym(mtmp->mx, mtmp->my);

	    if (mtmp->mtrapped &&
		    (trap = t_at(mtmp->mx, mtmp->my)) != 0 &&
		    (trap->ttyp == PIT || trap->ttyp == SPIKED_PIT) &&
		    sobj_at(BOULDER, trap->tx, trap->ty)) {
		/* can't swap places with pet pinned in a pit by a boulder */
		u.ux = u.ux0,  u.uy = u.uy0;	/* didn't move after all */
	    } else if (u.ux0 != x && u.uy0 != y &&
		       bad_rock(mtmp->data, x, u.uy0) &&
		       bad_rock(mtmp->data, u.ux0, y) &&
		       (bigmonst(mtmp->data) || (curr_mon_load(mtmp) > 600))) {
		/* can't swap places when pet won't fit thru the opening */
		u.ux = u.ux0,  u.uy = u.uy0;	/* didn't move after all */
#ifndef JP
		You("stop.  %s won't fit through.", upstart(y_monnam(mtmp)));
#else
		You("止まった。%sが隙間を通り抜けられない。", y_monnam(mtmp));
#endif /*JP*/
	    } else {
		char pnambuf[BUFSZ];

		/* save its current description in case of polymorph */
		Strcpy(pnambuf, y_monnam(mtmp));
		mtmp->mtrapped = 0;
		remove_monster(x, y);
		place_monster(mtmp, u.ux0, u.uy0);

		/* check for displacing it into pools and traps */
		switch (minliquid(mtmp) ? 2 : mintrap(mtmp)) {
		case 0:
#ifndef JP
		    You("%s %s.", mtmp->mtame ? "displaced" : "frightened",
			pnambuf);
#else
		    You("%s%sた。",pnambuf,
			mtmp->mtame ? "と入れ替わっ" : "を怯えさせ");
#endif /*JP*/
		    break;
		case 1:		/* trapped */
		case 3:		/* changed levels */
		    /* there's already been a trap message, reinforce it */
		    abuse_dog(mtmp);
		    adjalign(-3);
		    break;
		case 2:
		    /* it may have drowned or died.  that's no way to
		     * treat a pet!  your god gets angry.
		     */
		    if (rn2(4)) {
			You_feel(E_J("guilty about losing your pet like this.",
				     "ペットをこのような形で失ったことに罪の意識を覚えた。"));
			u.ugangr++;
			adjalign(-15);
		    }

		    /* you killed your pet by direct action.
		     * minliquid and mintrap don't know to do this
		     */
		    u.uconduct.killer++;
		    break;
		default:
		    pline("that's strange, unknown mintrap result!");
		    break;
		}
	    }
	}

	reset_occupations();
	if (flags.run) {
	    if ( flags.run < 8 )
		if (IS_DOOR(tmpr->typ) || IS_ROCK(tmpr->typ) ||
			IS_FURNITURE(tmpr->typ))
		    nomul(0);
	}

	if (hides_under(youmonst.data))
	    u.uundetected = OBJ_AT(u.ux, u.uy);
	else if (youmonst.data->mlet == S_EEL)
	    u.uundetected = is_pool(u.ux, u.uy) && !Is_waterlevel(&u.uz);
	else if (u.dx || u.dy)
	    u.uundetected = 0;

	/*
	 * Mimics (or whatever) become noticeable if they move and are
	 * imitating something that doesn't move.  We could extend this
	 * to non-moving monsters...
	 */
	if ((u.dx || u.dy) && (youmonst.m_ap_type == M_AP_OBJECT
				|| youmonst.m_ap_type == M_AP_FURNITURE))
	    youmonst.m_ap_type = M_AP_NOTHING;

	check_leash(u.ux0,u.uy0);

	if(u.ux0 != u.ux || u.uy0 != u.uy) {
	    u.umoved = TRUE;
	    /* Clean old position -- vision_recalc() will print our new one. */
	    newsym(u.ux0,u.uy0);
	    /* Since the hero has moved, adjust what can be seen/unseen. */
	    vision_recalc(1);	/* Do the work now in the recover time. */
	    invocation_message();
	}

	if (Punished)				/* put back ball and chain */
	    move_bc(0,bc_control,ballx,bally,chainx,chainy);

	spoteffects(TRUE);

	/* delay next move because of ball dragging */
	/* must come after we finished picking up, in spoteffects() */
	if (cause_delay) {
	    nomul(-2);
	    nomovemsg = "";
	}

	if (flags.run && iflags.runmode != RUN_TPORT) {
	    /* display every step or every 7th step depending upon mode */
	    if (iflags.runmode != RUN_LEAP || !(moves % 7L)) {
		if (flags.time) flags.botl = 1;
		curs_on_u();
		delay_output();
		if (iflags.runmode == RUN_CRAWL) {
		    delay_output();
		    delay_output();
		    delay_output();
		    delay_output();
		}
	    }
	}
}

void
struggle_sub(predicament)
const char *predicament;
{
	if(flags.verbose) {
#ifdef STEED
	    if (u.usteed)
#ifndef JP
		Norep("%s is %s.", upstart(y_monnam(u.usteed)),
		      predicament);
#else
		Norep("%s%s", y_monnam(u.usteed),
		      predicament);
#endif /*JP*/
	    else
#endif
	    Norep(E_J("You are %s.","あなた%s"), predicament);
	}
}

void
invocation_message()
{
	/* a special clue-msg when on the Invocation position */
	if(invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy)) {
	    char buf[BUFSZ];
	    struct obj *otmp = carrying(CANDELABRUM_OF_INVOCATION);

	    nomul(0);		/* stop running or travelling */
#ifdef STEED
	    if (u.usteed) Sprintf(buf, E_J("beneath %s","%sの下"), y_monnam(u.usteed));
	    else
#endif
	    if (Levitation || Flying) Strcpy(buf, E_J("beneath you","下方"));
	    else Sprintf(buf, E_J("under your %s","%s元"),
			      E_J(makeplural(body_part(FOOT)), body_part(FOOT)));

	    You_feel(E_J("a strange vibration %s.","%sに奇妙な振動を感じた。"), buf);
	    u.uevent.uvibrated = 1;
	    if (otmp && otmp->spe == 7 && otmp->lamplit)
#ifndef JP
		pline("%s %s!", The(xname(otmp)),
		    Blind ? "throbs palpably" : "glows with a strange light");
#else
		pline("%s%sた！", xname(otmp),
		    Blind ? "がびりびりと震え出し" : "の炎が奇妙な光を放っ");
#endif /*JP*/
	}
}

#endif /* OVL3 */
#ifdef OVL2

void
spoteffects(pick)
boolean pick;
{
	register struct monst *mtmp;

	if(u.uinwater) {
		int was_underwater;

		if (!is_pool(u.ux,u.uy)) {
			if (Is_waterlevel(&u.uz))
				You(E_J("pop into an air bubble.",
					"気泡の中に飛び出した。"));
			else if (is_lava(u.ux, u.uy))	/* oops! */
				You(E_J("leave the water...",
					"水から抜け出た…。"));
			else if (is_swamp(u.ux, u.uy))
				You(E_J("are on shallows.",
					"浅瀬にたどりついた。"));
			else
				You(E_J("are on solid %s again.",
					"しっかりした%sの上に戻ってきた。"),
				    is_ice(u.ux, u.uy) ? E_J("ice","氷") : E_J("land","陸"));
		}
		else if (Is_waterlevel(&u.uz))
			goto stillinwater;
		else if (Levitation)
			You(E_J("pop out of the water like a cork!",
				"コルク栓のように水から飛び出した！"));
		else if (Flying)
			You(E_J("fly out of the water.",
				"水から飛び出した。"));
		else if (Wwalking)
			You(E_J("slowly rise above the surface.",
				"ゆっくりと水面まで上がってきた。"));
		else
			goto stillinwater;
		was_underwater = Underwater && !Is_waterlevel(&u.uz);
		u.uinwater = 0;		/* leave the water */
		if (was_underwater) {	/* restore vision */
			docrt();
			vision_full_recalc = 1;
		}
	}
stillinwater:;
	if (u.utraptype == TT_SWAMP) {
		if (!is_swamp(u.ux, u.uy)) {
			if (is_lava(u.ux, u.uy))	/* oops! */
				You(E_J("get out of the mud...",
					"泥から抜け出した…。"));
			else
				You(E_J("are on solid %s again.",
					"しっかりした%sの上に戻ってきた。"),
				    is_ice(u.ux, u.uy) ? E_J("ice","氷") : E_J("land","陸"));
		}
		else if (Levitation)
			You(E_J("rise out of the swamp.",
				"沼の上に浮き上がった。"));
		else if (Flying)
			You(E_J("fly out of the swamp.",
				"沼から飛び出した。"));
		else if (Wwalking)
			You(E_J("slowly rise above the muddy water.",
				"ゆっくりと沼の表面まで上がってきた。"));
		else goto stillinswamp;
		u.utrap = 0;
		u.utraptype = 0;
	}
stillinswamp:
	if (!Levitation && !u.ustuck && !Flying) {
	    /* limit recursive calls through teleds() */
	    if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy) ||
		is_swamp(u.ux, u.uy)) {
#ifdef STEED
		if (u.usteed && !is_flyer(u.usteed->data) &&
			!is_floater(u.usteed->data) &&
			!is_clinger(u.usteed->data)) {
		    dismount_steed(Underwater ?
			    DISMOUNT_FELL : DISMOUNT_GENERIC);
		    /* dismount_steed() -> float_down() -> pickup() */
		    if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz))
			pick = FALSE;
		} else
#endif
		if (is_lava(u.ux, u.uy)) {
		    if (lava_effects()) return;
		} else if (is_swamp(u.ux, u.uy)) {
		    if (swamp_effects()) return;
		} else if (!Wwalking && drown())
		    return;
	    }
	}
	if (!u.utrap) u.utraptype = 0;
	check_special_room(FALSE);
#ifdef SINKS
	if(IS_SINK(levl[u.ux][u.uy].typ) && Levitation)
		dosinkfall();
#endif
	if (!in_steed_dismounting) { /* if dismounting, we'll check again later */
		struct trap *trap = t_at(u.ux, u.uy);
		boolean pit;
		pit = (trap && (trap->ttyp == PIT || trap->ttyp == SPIKED_PIT));
		if (trap && pit)
			dotrap(trap, 0);	/* fall into pit */
		if (pick) (void) pickup(1);
		if (trap && !pit)
			dotrap(trap, 0);	/* fall into arrow trap, etc. */
	}
	if((mtmp = m_at(u.ux, u.uy)) && !u.uswallow) {
		mtmp->mundetected = mtmp->msleeping = 0;
		switch(mtmp->data->mlet) {
		    case S_PIERCER:
			pline(E_J("%s suddenly drops from the %s!",
				  "突然、%sが%sから落ちてきた！"),
			      Amonnam(mtmp), ceiling(u.ux,u.uy));
			if(mtmp->mtame) /* jumps to greet you, not attack */
			    ;
			else if(uarmh && is_metallic(uarmh))
			    pline(E_J("Its blow glances off your helmet.",
				      "ピアサーの一撃はあなたの兜に逸らされた。"));
			else if (u.uac + 3 <= rnd(20))
#ifndef JP
			    You("are almost hit by %s!",
				x_monnam(mtmp, ARTICLE_A, "falling", 0, TRUE));
#else
			    You("もう少しで落ちてきた%sに当たるところだった！",
				mon_nam(mtmp));
#endif /*JP*/
			else {
			    int dmg;
#ifndef JP
			    You("are hit by %s!",
				x_monnam(mtmp, ARTICLE_A, "falling", 0, TRUE));
#else
			    pline("落ちてきた%sがあなたを直撃した！",
				mon_nam(mtmp));
#endif /*JP*/
			    dmg = d(4,6);
			    if(Half_physical_damage) dmg = (dmg+1) / 2;
			    mdamageu(mtmp, dmg);
			}
			break;
		    default:	/* monster surprises you. */
			if(mtmp->mtame)
			    pline(E_J("%s jumps near you from the %s.",
				      "%sが%sからあなたのそばに飛び降りてきた。"),
					Amonnam(mtmp), ceiling(u.ux,u.uy));
			else if(mtmp->mpeaceful) {
				You(E_J("surprise %s!","%sを驚かせた！"),
				    Blind && !sensemon(mtmp) ?
				    something : a_monnam(mtmp));
				mtmp->mpeaceful = 0;
			} else
			    pline(E_J("%s attacks you by surprise!",
				      "%sはあなたに不意打ちをくらわせた！"),
					Amonnam(mtmp));
			break;
		}
		mnexto(mtmp); /* have to move the monster */
	}
}

STATIC_OVL boolean
monstinroom(mdat,roomno)
struct permonst *mdat;
int roomno;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(!DEADMONSTER(mtmp) && mtmp->data == mdat &&
		   index(in_rooms(mtmp->mx, mtmp->my, 0), roomno + ROOMOFFSET))
			return(TRUE);
	return(FALSE);
}

char *
in_rooms(x, y, typewanted)
register xchar x, y;
register int typewanted;
{
	static char buf[5];
	char rno, *ptr = &buf[4];
	int typefound, min_x, min_y, max_x, max_y_offset, step;
	register struct rm *lev;

#define goodtype(rno) (!typewanted || \
	     ((typefound = rooms[rno - ROOMOFFSET].rtype) == typewanted) || \
	     ((typewanted == SHOPBASE) && (typefound > SHOPBASE))) \

	switch (rno = levl[x][y].roomno) {
		case NO_ROOM:
			return(ptr);
		case SHARED:
			step = 2;
			break;
		case SHARED_PLUS:
			step = 1;
			break;
		default:			/* i.e. a regular room # */
			if (goodtype(rno))
				*(--ptr) = rno;
			return(ptr);
	}

	min_x = x - 1;
	max_x = x + 1;
	if (x < 1)
		min_x += step;
	else
	if (x >= COLNO)
		max_x -= step;

	min_y = y - 1;
	max_y_offset = 2;
	if (min_y < 0) {
		min_y += step;
		max_y_offset -= step;
	} else
	if ((min_y + max_y_offset) >= ROWNO)
		max_y_offset -= step;

	for (x = min_x; x <= max_x; x += step) {
		lev = &levl[x][min_y];
		y = 0;
		if (((rno = lev[y].roomno) >= ROOMOFFSET) &&
		    !index(ptr, rno) && goodtype(rno))
			*(--ptr) = rno;
		y += step;
		if (y > max_y_offset)
			continue;
		if (((rno = lev[y].roomno) >= ROOMOFFSET) &&
		    !index(ptr, rno) && goodtype(rno))
			*(--ptr) = rno;
		y += step;
		if (y > max_y_offset)
			continue;
		if (((rno = lev[y].roomno) >= ROOMOFFSET) &&
		    !index(ptr, rno) && goodtype(rno))
			*(--ptr) = rno;
	}
	return(ptr);
}

/* is (x,y) in a town? */
boolean
in_town(x, y)
register int x, y;
{
	s_level *slev = Is_special(&u.uz);
	register struct mkroom *sroom;
	boolean has_subrooms = FALSE;

	if (!slev || !slev->flags.town) return FALSE;

	/*
	 * See if (x,y) is in a room with subrooms, if so, assume it's the
	 * town.  If there are no subrooms, the whole level is in town.
	 */
	for (sroom = &rooms[0]; sroom->hx > 0; sroom++) {
	    if (sroom->nsubrooms > 0) {
		has_subrooms = TRUE;
		if (inside_room(sroom, x, y)) return TRUE;
	    }
	}

	return !has_subrooms;
}

STATIC_OVL void
move_update(newlev)
register boolean newlev;
{
	char *ptr1, *ptr2, *ptr3, *ptr4;

	Strcpy(u.urooms0, u.urooms);
	Strcpy(u.ushops0, u.ushops);
	if (newlev) {
		u.urooms[0] = '\0';
		u.uentered[0] = '\0';
		u.ushops[0] = '\0';
		u.ushops_entered[0] = '\0';
		Strcpy(u.ushops_left, u.ushops0);
		return;
	}
	Strcpy(u.urooms, in_rooms(u.ux, u.uy, 0));

	for (ptr1 = &u.urooms[0],
	     ptr2 = &u.uentered[0],
	     ptr3 = &u.ushops[0],
	     ptr4 = &u.ushops_entered[0];
	     *ptr1; ptr1++) {
		if (!index(u.urooms0, *ptr1))
			*(ptr2++) = *ptr1;
		if (IS_SHOP(*ptr1 - ROOMOFFSET)) {
			*(ptr3++) = *ptr1;
			if (!index(u.ushops0, *ptr1))
				*(ptr4++) = *ptr1;
		}
	}
	*ptr2 = '\0';
	*ptr3 = '\0';
	*ptr4 = '\0';

	/* filter u.ushops0 -> u.ushops_left */
	for (ptr1 = &u.ushops0[0], ptr2 = &u.ushops_left[0]; *ptr1; ptr1++)
		if (!index(u.ushops, *ptr1))
			*(ptr2++) = *ptr1;
	*ptr2 = '\0';
}

void
check_special_room(newlev)
register boolean newlev;
{
	register struct monst *mtmp;
	char *ptr;

	move_update(newlev);

	if (*u.ushops0)
	    u_left_shop(u.ushops_left, newlev);

	if (!*u.uentered && !*u.ushops_entered)		/* implied by newlev */
	    return;		/* no entrance messages necessary */

	/* Did we just enter a shop? */
	if (*u.ushops_entered)
	    u_entered_shop(u.ushops_entered);

	for (ptr = &u.uentered[0]; *ptr; ptr++) {
	    int roomno = *ptr - ROOMOFFSET,
		rt = rooms[roomno].rtype;
	    boolean msg_given = TRUE;

	    /* Did we just enter some other special room? */
	    /* vault.c insists that a vault remain a VAULT,
	     * and temples should remain TEMPLEs,
	     * but everything else gives a message only the first time */
	    switch (rt) {
		case ZOO:
		    pline(E_J("Welcome to David's treasure zoo!",
			      "デビッドの宝物動物園へようこそ！"));
		    break;
		case SWAMP:
#ifndef JP
		    pline("It %s rather %s down here.",
			  Blind ? "feels" : "looks",
			  Blind ? "humid" : "muddy");
#else /*JP*/
		    pline("ここの%sはかなり%s。",
			  Blind ? "空気" : "地面",
			  Blind ? "湿っている" : "どろどろしている");
#endif /*JP*/
		    break;
		case COURT:
		    You(E_J("enter an opulent throne room!",
			    "華やかな玉座の間に入った！"));
		    break;
		case LEPREHALL:
		    You(E_J("enter a leprechaun hall!",
			    "レプラコーンの広間に入った！"));
		    break;
		case MORGUE:
		    if(midnight()) {
#ifndef JP
			const char *run = locomotion(youmonst.data, "Run");
			pline("%s away!  %s away!", run, run);
#else /*JP*/
			pline("逃げろ！　逃げろ！");
#endif /*JP*/
		    } else
			You(E_J("have an uncanny feeling...",
				"薄気味の悪さを感じた…。"));
		    break;
		case BEEHIVE:
		    You(E_J("enter a giant beehive!",
			    "巨大な蜂の巣に入った！"));
		    break;
		case COCKNEST:
		    You(E_J("enter a disgusting nest!",
			    "吐き気のする巣穴に入った！"));
		    break;
		case ANTHOLE:
		    You(E_J("enter an anthole!","蟻の巣に入った！"));
		    break;
		case BARRACKS:
		    if(monstinroom(&mons[PM_SOLDIER], roomno) ||
			monstinroom(&mons[PM_SERGEANT], roomno) ||
			monstinroom(&mons[PM_LIEUTENANT], roomno) ||
			monstinroom(&mons[PM_CAPTAIN], roomno))
			You(E_J("enter a military barracks!",
				"軍の兵舎に踏み入った！"));
		    else
			You(E_J("enter an abandoned barracks.",
				"放棄された兵舎に踏み入った。"));
		    break;
		case DELPHI:
		    if(monstinroom(&mons[PM_ORACLE], roomno))
                /* TODO: 3.6 has a bugfix wherein this message is only
                   given if the Oracle is peaceful, otherwise you get
                   a different message. */
			verbalize(E_J("%s, %s, welcome to Delphi!",
				      "%s%s、デルファイへようこそ！"),
					Hello((struct monst *) 0), plname);
            else
			    msg_given = FALSE;
		        break;
		case TEMPLE:
		    intemple(roomno + ROOMOFFSET);
		    /*FALLTHRU*/
		default:
		    msg_given = (rt == TEMPLE);
		    rt = 0;
		    break;
	    }
#ifdef DUNGEON_OVERVIEW
	    if (msg_given) room_discovered(roomno);
#endif

	    if (rt != 0) {
		rooms[roomno].rtype = OROOM;
		if (!search_special(rt)) {
			/* No more room of that type */
			switch(rt) {
			    case COURT:
				level.flags.has_court = 0;
				break;
			    case SWAMP:
				level.flags.has_swamp = 0;
				break;
			    case MORGUE:
				level.flags.has_morgue = 0;
				break;
			    case ZOO:
				level.flags.has_zoo = 0;
				break;
			    case BARRACKS:
				level.flags.has_barracks = 0;
				break;
			    case TEMPLE:
				level.flags.has_temple = 0;
				break;
			    case BEEHIVE:
				level.flags.has_beehive = 0;
				break;
			}
		}
		if (rt == COURT || rt == SWAMP || rt == MORGUE || rt == ZOO)
		    for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			if (!DEADMONSTER(mtmp) && !Stealth && !rn2(3)) mtmp->msleeping = 0;
	    }
	}

	return;
}

#endif /* OVL2 */
#ifdef OVLB

int
dopickup()
{
	int count;
	struct trap *traphere = t_at(u.ux, u.uy);
 	/* awful kludge to work around parse()'s pre-decrement */
	count = (multi || (save_cm && *save_cm == ',')) ? multi + 1 : 0;
	multi = 0;	/* always reset */
	/* uswallow case added by GAN 01/29/87 */
	if(u.uswallow) {
	    if (!u.ustuck->minvent) {
		if (is_animal(u.ustuck->data)) {
		    You(E_J("pick up %s tongue.","%sの舌を拾い上げた。"), 
			E_J(s_suffix(mon_nam(u.ustuck)), mon_nam(u.ustuck)));
		    pline(E_J("But it's kind of slimy, so you drop it.",
			      "しかし、それはぬるぬるして不快だったので捨ててしまった。"));
		} else
#ifndef JP
		    You("don't %s anything in here to pick up.",
			  Blind ? "feel" : "see");
#else /*JP*/
		    pline("ここには何も拾えるものがない%s。", Blind ? "ようだ" : "");
#endif /*JP*/
		return(1);
	    } else {
	    	int tmpcount = -count;
		return loot_mon(u.ustuck, &tmpcount, (boolean *)0);
	    }
	}
	if(is_pool(u.ux, u.uy)) {
	    if (Wwalking || is_floater(youmonst.data) || is_clinger(youmonst.data)
			|| (Flying && !Breathless)) {
		You(E_J("cannot dive into the water to pick things up.",
			"物を拾うために水に潜ることができない。"));
		return(0);
	    } else if (!Underwater) {
#ifndef JP
		You_cant("even see the bottom, let alone pick up %s.",
				something);
#else /*JP*/
		pline("何かを拾うのはおろか、水底さえ見えない。");
#endif /*JP*/
		return(0);
	    }
	}
	if (is_lava(u.ux, u.uy)) {
	    if (Wwalking || is_floater(youmonst.data) || is_clinger(youmonst.data)
			|| (Flying && !Breathless)) {
#ifndef JP
		You_cant("reach the bottom to pick things up.");
#else /*JP*/
		You("物を拾うために底までたどり着くことができない。");
#endif /*JP*/
		return(0);
	    } else if (!likes_lava(youmonst.data)) {
#ifndef JP
		You("would burn to a crisp trying to pick things up.");
#else /*JP*/
		pline("何か拾おうとしたりしたら、あなたは丸焦げになってしまうだろう。");
#endif /*JP*/
		return(0);
	    }
	}
	if(!OBJ_AT(u.ux, u.uy)) {
		There(E_J("is nothing here to pick up.",
			  "拾えるものは何もない。"));
		return(0);
	}
	if (!can_reach_floor()) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
		    You(E_J("aren't skilled enough to reach from %s.",
			    "、%sから床まで届くほど乗馬に習熟していない。"),
			y_monnam(u.usteed));
		else
#endif
		You(E_J("cannot reach the %s.","%sまで届かない。"), surface(u.ux,u.uy));
		return(0);
	}

 	if (traphere && traphere->tseen) {
		/* Allow pickup from holes and trap doors that you escaped from
		 * because that stuff is teetering on the edge just like you, but
		 * not pits, because there is an elevation discrepancy with stuff
		 * in pits.
		 */
		if ((traphere->ttyp == PIT || traphere->ttyp == SPIKED_PIT) &&
		     (!u.utrap || (u.utrap && u.utraptype != TT_PIT))) {
			if (yn(E_J("You cannot pick up the object in the pit. Do you want to go down?",
				   "ここからでは届かない。落し穴の底に降りますか？")) == 'y') {
			    You(E_J("carefully slide down into the %s",
				    "注意深く%sの中に滑り込んだ。"),
				    defsyms[trap_to_defsym(traphere->ttyp)].explanation);
			    u.utraptype = TT_PIT;
			    u.utrap = rn1(6,2);
			    vision_full_recalc = 1;	/* vision limits change */	/*[Sakusha]*/
			} else {
			    You(E_J("cannot reach the bottom of the pit.",
				    "落し穴の底まで届かない。"));
			    return(0);
			}
		}
	}

	return (pickup(-count));
}

#endif /* OVLB */
#ifdef OVL2

/* stop running if we see something interesting */
/* turn around a corner if that is the only way we can proceed */
/* do not turn left or right twice */
void
lookaround()
{
    register int x, y, i, x0 = 0, y0 = 0, m0 = 1, i0 = 9;
    register int corrct = 0, noturn = 0;
    register struct monst *mtmp;
    register struct trap *trap;

    /* Grid bugs stop if trying to move diagonal, even if blind.  Maybe */
    /* they polymorphed while in the middle of a long move. */
    if (u.umonnum == PM_GRID_BUG && u.dx && u.dy) {
	nomul(0);
	return;
    }

    if(Blind || flags.run == 0) return;
    for(x = u.ux-1; x <= u.ux+1; x++) for(y = u.uy-1; y <= u.uy+1; y++) {
	if(!isok(x,y)) continue;

	if(u.umonnum == PM_GRID_BUG && x != u.ux && y != u.uy) continue;

	if(x == u.ux && y == u.uy) continue;

	if((mtmp = m_at(x,y)) &&
		    mtmp->m_ap_type != M_AP_FURNITURE &&
		    mtmp->m_ap_type != M_AP_OBJECT &&
		    (!mtmp->minvis || See_invisible) && !mtmp->mundetected) {
	    if((flags.run != 1 && !mtmp->mtame)
					|| (x == u.ux+u.dx && y == u.uy+u.dy))
		goto stop;
	}

	if (levl[x][y].typ == STONE) continue;
	if (x == u.ux-u.dx && y == u.uy-u.dy) continue;

	if (IS_ROCK(levl[x][y].typ) || IS_FLOOR(levl[x][y].typ) ||
	    IS_AIR(levl[x][y].typ))
	    continue;
	else if (closed_door(x,y) ||
		 (mtmp && mtmp->m_ap_type == M_AP_FURNITURE &&
		  (mtmp->mappearance == S_hcdoor ||
		   mtmp->mappearance == S_vcdoor))) {
	    if(x != u.ux && y != u.uy) continue;
	    if(flags.run != 1) goto stop;
	    goto bcorr;
	} else if (levl[x][y].typ == CORR) {
bcorr:
	    if(!IS_FLOOR(levl[u.ux][u.uy].typ)) {
		if(flags.run == 1 || flags.run == 3 || flags.run == 8) {
		    i = dist2(x,y,u.ux+u.dx,u.uy+u.dy);
		    if(i > 2) continue;
		    if(corrct == 1 && dist2(x,y,x0,y0) != 1)
			noturn = 1;
		    if(i < i0) {
			i0 = i;
			x0 = x;
			y0 = y;
			m0 = mtmp ? 1 : 0;
		    }
		}
		corrct++;
	    }
	    continue;
	} else if ((trap = t_at(x,y)) && trap->tseen) {
	    if(flags.run == 1) goto bcorr;	/* if you must */
	    if(x == u.ux+u.dx && y == u.uy+u.dy) goto stop;
	    continue;
	} else if (is_pool(x,y) || is_lava(x,y)) {
	    /* water and lava only stop you if directly in front, and stop
	     * you even if you are running
	     */
	    if(!Levitation && !Flying && !is_clinger(youmonst.data) &&
				x == u.ux+u.dx && y == u.uy+u.dy)
			/* No Wwalking check; otherwise they'd be able
			 * to test boots by trying to SHIFT-direction
			 * into a pool and seeing if the game allowed it
			 */
			goto stop;
	    continue;
	} else {		/* e.g. objects or trap or stairs */
	    if(flags.run == 1) goto bcorr;
	    if(flags.run == 8) continue;
	    if(mtmp) continue;		/* d */
	    if(((x == u.ux - u.dx) && (y != u.uy + u.dy)) ||
	       ((y == u.uy - u.dy) && (x != u.ux + u.dx)))
	       continue;
	}
stop:
	nomul(0);
	return;
    } /* end for loops */

    if(corrct > 1 && flags.run == 2) goto stop;
    if((flags.run == 1 || flags.run == 3 || flags.run == 8) &&
	!noturn && !m0 && i0 && (corrct == 1 || (corrct == 2 && i0 == 1)))
    {
	/* make sure that we do not turn too far */
	if(i0 == 2) {
	    if(u.dx == y0-u.uy && u.dy == u.ux-x0)
		i = 2;		/* straight turn right */
	    else
		i = -2;		/* straight turn left */
	} else if(u.dx && u.dy) {
	    if((u.dx == u.dy && y0 == u.uy) || (u.dx != u.dy && y0 != u.uy))
		i = -1;		/* half turn left */
	    else
		i = 1;		/* half turn right */
	} else {
	    if((x0-u.ux == y0-u.uy && !u.dy) || (x0-u.ux != y0-u.uy && u.dy))
		i = 1;		/* half turn right */
	    else
		i = -1;		/* half turn left */
	}

	i += u.last_str_turn;
	if(i <= 2 && i >= -2) {
	    u.last_str_turn = i;
	    u.dx = x0-u.ux;
	    u.dy = y0-u.uy;
	}
    }
}

/* something like lookaround, but we are not running */
/* react only to monsters that might hit us */
int
monster_nearby()
{
	register int x,y;
	register struct monst *mtmp;

	/* Also see the similar check in dochugw() in monmove.c */
	for(x = u.ux-1; x <= u.ux+1; x++)
	    for(y = u.uy-1; y <= u.uy+1; y++) {
		if(!isok(x,y)) continue;
		if(x == u.ux && y == u.uy) continue;
		if((mtmp = m_at(x,y)) &&
		   mtmp->m_ap_type != M_AP_FURNITURE &&
		   mtmp->m_ap_type != M_AP_OBJECT &&
		   (!mtmp->mpeaceful || Hallucination) &&
		   (!is_hider(mtmp->data) || !mtmp->mundetected) &&
		   !noattacks(mtmp->data) &&
		   mtmp->mcanmove && !mtmp->msleeping &&  /* aplvax!jcn */
		   !onscary(u.ux, u.uy, mtmp) &&
		   canspotmon(mtmp))
			return(1);
	}
	return(0);
}

void
nomul(nval)
	register int nval;
{
	if(multi < nval) return;	/* This is a bug fix by ab@unido */
	u.uinvulnerable = FALSE;	/* Kludge to avoid ctrl-C bug -dlc */
	u.usleep = 0;
	multi = nval;
	flags.travel = iflags.travel1 = flags.mv = flags.run = 0;
}

/* called when a non-movement, multi-turn action has completed */
void
unmul(msg_override)
const char *msg_override;
{
	multi = 0;	/* caller will usually have done this already */
	if (msg_override) nomovemsg = msg_override;
	else if (!nomovemsg) nomovemsg = You_can_move_again;
	if (*nomovemsg) pline(nomovemsg);
	nomovemsg = 0;
	u.usleep = 0;
	if (afternmv) (*afternmv)();
	afternmv = 0;
}

#endif /* OVL2 */
#ifdef OVL1

STATIC_OVL void
maybe_wail()
{
    static short powers[] = { TELEPORT, SEE_INVIS, POISON_RES, COLD_RES,
			      SHOCK_RES, FIRE_RES, SLEEP_RES, DISINT_RES,
			      TELEPORT_CONTROL, STEALTH, FAST, INVIS };

    if (moves <= wailmsg + 50) return;

    wailmsg = moves;
    if (Role_if(PM_WIZARD) || Race_if(PM_ELF) || Role_if(PM_VALKYRIE)) {
	const char *who;
	int i, powercnt;

#ifndef JP
	who = (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE)) ?
		urole.name.m : "Elf";
#else
	who = Role_if(PM_WIZARD) ? "Wizard" :
	      Role_if(PM_VALKYRIE) ? "Valkyrie" : "Elf";
#endif /*JP*/
	if (u.uhp == 1) {
	    pline("%s is about to die.", who);
	} else {
	    for (i = 0, powercnt = 0; i < SIZE(powers); ++i)
		if (u.uprops[powers[i]].intrinsic & INTRINSIC) ++powercnt;

	    pline(powercnt >= 4 ? "%s, all your powers will be lost..."
				: "%s, your life force is running out.", who);
	}
    } else {
	You_hear(u.uhp == 1 ? E_J("the wailing of the Banshee...",
				  "_バンシーのすすり泣きを")
			    : E_J("the howling of the CwnAnnwn...",
				  "_クーン・アンヌーンの遠吠を"));
    }
}

void
losehp(n, knam, k_format)
register int n;
register const char *knam;
boolean k_format;
{
#ifdef SHOWDMG
	if (n && flags.showdmg)  pline("(%dpt%s)", n, (n == 1 ? "" : "s"));
#endif
	if (Upolyd) {
		u.mh -= n;
		if (u.mhmax < u.mh) u.mhmax = u.mh;
		flags.botl = 1;
		if (u.mh < 1)
		    rehumanize();
		else if (n > 0 && u.mh*10 < u.mhmax && Unchanging)
		    maybe_wail();
		return;
	}

	u.uhp -= n;
	if(u.uhp > u.uhpmax)
		addhpmax(u.uhp - u.uhpmax);	/* perhaps n was negative */
	flags.botl = 1;
	if(u.uhp < 1) {
		killer_format = k_format;
		killer = knam;		/* the thing that killed you */
		You(E_J("die...","死にました…。"));
		done(DIED);
	} else if (n > 0 && u.uhp*10 < u.uhpmax) {
		maybe_wail();
	}
}

int
weight_cap()
{
	register long carrcap;
	int s = ACURR(A_STR);

/* -original-	carrcap = (((ACURRSTR + ACURR(A_CON))/2)+1)*50; */
	carrcap = ACURR(A_CON) * 20;
	if      (s <=  18) carrcap += s * 25;			/*  3-18 */
	else if (s <= 118) carrcap += 450 + (s-18);		/* 18/xx */
	else		   carrcap += 550 + (s-118) * 50;	/* 19-25 */

	if (Upolyd) {
		/* consistent with can_carry() in mon.c */
		if (youmonst.data->mlet == S_NYMPH)
			carrcap = MAX_CARR_CAP;
		else if (!youmonst.data->cwt)
			carrcap = (carrcap * (long)youmonst.data->msize) / MZ_HUMAN;
		else if (!strongmonst(youmonst.data)
			|| (strongmonst(youmonst.data) && (youmonst.data->cwt > WT_HUMAN)))
			carrcap = (carrcap * (long)youmonst.data->cwt / WT_HUMAN);
	}

	if (Levitation || Is_airlevel(&u.uz)    /* pugh@cornell */
#ifdef STEED
			|| (u.usteed && strongmonst(u.usteed->data))
#endif
	)
		carrcap = MAX_CARR_CAP;
	else {
		if(carrcap > MAX_CARR_CAP) carrcap = MAX_CARR_CAP;
		if (!Flying) {
			if(EWounded_legs & LEFT_SIDE) carrcap -= 100;
			if(EWounded_legs & RIGHT_SIDE) carrcap -= 100;
		}
		if (carrcap < 0) carrcap = 0;
	}
	return((int) carrcap);
}

static int wc;	/* current weight_cap(); valid after call to inv_weight() */

/* returns how far beyond the normal capacity the player is currently. */
/* inv_weight() is negative if the player is below normal capacity. */
int
inv_weight()
{
	register struct obj *otmp = invent;
	register int wt = 0;

#ifndef GOLDOBJ
	/* when putting stuff into containers, gold is inserted at the head
	   of invent for easier manipulation by askchain & co, but it's also
	   retained in u.ugold in order to keep the status line accurate; we
	   mustn't add its weight in twice under that circumstance */
	wt = (otmp && otmp->oclass == COIN_CLASS) ? 0 :
		(int)((u.ugold + 50L) / 100L);
#endif
	while (otmp) {
#ifndef GOLDOBJ
		if (otmp->otyp != BOULDER || !throws_rocks(youmonst.data))
#else
		if (otmp->oclass == COIN_CLASS)
			wt += (int)(((long)otmp->quan + 50L) / 100L);
		else if (otmp->otyp != BOULDER || !throws_rocks(youmonst.data))
#endif
			wt += otmp->owt;
		otmp = otmp->nobj;
	}
	wc = weight_cap();

	/* speed bonus for light-weight */
	u.uspdbon1 = (wt < wc) ? (wc-wt)/100 : 0;

	return (wt - wc);
}

/*
 * Returns 0 if below normal capacity, or the number of "capacity units"
 * over the normal capacity the player is loaded.  Max is 5.
 */
int
calc_capacity(xtra_wt)
int xtra_wt;
{
    int cap, wt = inv_weight() + xtra_wt;

    if (wt <= 0) return UNENCUMBERED;
    if (wc <= 1) return OVERLOADED;
    cap = (wt*2 / wc) + 1;
    return min(cap, OVERLOADED);
}

int
near_capacity()
{
    return calc_capacity(0);
}

int
max_capacity()
{
    int wt = inv_weight();

    return (wt - (2 * wc));
}

boolean
check_capacity(str)
const char *str;
{
    if(near_capacity() >= EXT_ENCUMBER) {
	if(str)
	    pline(str);
	else
	    You_cant(E_J("do that while carrying so much stuff.",
			 "こんなに荷物を抱えていては、それはできない。"));
	return 1;
    }
    return 0;
}

#endif /* OVL1 */
#ifdef OVLB

int
inv_cnt()
{
	register struct obj *otmp = invent;
	register int ct = 0;

	while(otmp){
		ct++;
		otmp = otmp->nobj;
	}
	return(ct);
}

#ifdef GOLDOBJ
/* Counts the money in an object chain. */
/* Intended use is for your or some monsters inventory, */
/* now that u.gold/m.gold is gone.*/
/* Counting money in a container might be possible too. */
long
money_cnt(otmp)
struct obj *otmp;
{
        while(otmp) {
	        /* Must change when silver & copper is implemented: */
 	        if (otmp->oclass == COIN_CLASS) return otmp->quan;
  	        otmp = otmp->nobj;
	}
	return 0;
}
#endif
#endif /* OVLB */

#ifdef SHOWDMG
void
printdmg(tmp)
int tmp;
{
#ifndef JP
	if (flags.showdmg && tmp) pline("(%dpt%s)", tmp, (tmp == 1 ? "" : "s"));
#else
	if (flags.showdmg && tmp) pline("(%dpts)", tmp);
#endif /*JP*/
}
#endif /*SHOWDMG*/

/*hack.c*/
