/*	SCCS Id: @(#)dothrow.c	3.4	2003/12/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* Contains code for 't' (throw) */

#include "hack.h"
#include "edog.h"

STATIC_DCL int FDECL(throw_obj, (struct obj *,int));
STATIC_DCL void NDECL(autoquiver);
STATIC_DCL int FDECL(gem_accept, (struct monst *, struct obj *));
STATIC_DCL int FDECL(throw_gold, (struct obj *));
STATIC_DCL void FDECL(check_shop_obj, (struct obj *,XCHAR_P,XCHAR_P,BOOLEAN_P));
STATIC_DCL void FDECL(breakobj, (struct obj *,XCHAR_P,XCHAR_P,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL void FDECL(breakmsg, (struct obj *,BOOLEAN_P));
STATIC_DCL boolean FDECL(toss_up,(struct obj *, BOOLEAN_P));
STATIC_DCL boolean FDECL(throwing_weapon, (struct obj *));
STATIC_DCL void FDECL(sho_obj_return_to_u, (struct obj *obj));
STATIC_DCL boolean FDECL(mhurtle_step, (genericptr_t,int,int));
STATIC_DCL int FDECL(fire_gun, (struct obj *, BOOLEAN_P));


static NEARDATA const char toss_objs[] =
	{ ALLOW_COUNT, COIN_CLASS, ALL_CLASSES, WEAPON_CLASS, 0 };
/* different default choices when wielding a sling (gold must be included) */
static NEARDATA const char bullets[] =
	{ ALLOW_COUNT, COIN_CLASS, ALL_CLASSES, GEM_CLASS, 0 };

struct obj *thrownobj = 0;	/* tracks an object until it lands */

extern boolean notonhead;	/* for long worms */

#define	SHOOT_LASTTGT	0x0100

/* Throw the selected object, asking for direction */
STATIC_OVL int
throw_obj(obj, shotlimit)
struct obj *obj;
int shotlimit;
{
	struct obj *otmp;
	int multishot = 1;
	int skill;
	long wep_mask;
	boolean twoweap;
	boolean aml;

	if (shotlimit & SHOOT_LASTTGT) {
	    shotlimit &= ~SHOOT_LASTTGT;
	    goto skip_getdir;
	}

	/* ask "in what direction?" */
#ifndef GOLDOBJ
/*test*/
	if (obj->otyp == BOOMERANG) {
	    if (!getdir((char *)0)) return 0;
	} else {
	    struct monst *mtmp;
	    if (!getdir_or_pos(0, GETPOS_MONTGT, (char *)0, E_J("aim","狙う"))) {
		if (obj->oclass == COIN_CLASS) {
		    u.ugold += obj->quan;
		    flags.botl = 1;
		    dealloc_obj(obj);
		}
		return 0;
	    }
	    if ((mtmp = m_at(u.ux+u.dx, u.uy+u.dy)) != 0 &&
		(mtmp->mx == (u.ux+u.dx) && mtmp->my == (u.uy+u.dy)))
		u.ulasttgt = mtmp;
	}/*test*/
# if 0
	if (!getdir((char *)0)) {
		if (obj->oclass == COIN_CLASS) {
		    u.ugold += obj->quan;
		    flags.botl = 1;
		    dealloc_obj(obj);
		}
		return(0);
	}
# endif
	if(obj->oclass == COIN_CLASS) return(throw_gold(obj));
#else
	if (!getdir((char *)0)) {
	    /* obj might need to be merged back into the singular gold object */
	    freeinv(obj);
	    addinv(obj);
	    return(0);
	}

        /*
	  Throwing money is usually for getting rid of it when
          a leprechaun approaches, or for bribing an oncoming 
          angry monster.  So throw the whole object.

          If the money is in quiver, throw one coin at a time,
          possibly using a sling.
        */
	if(obj->oclass == COIN_CLASS && obj != uquiver) return(throw_gold(obj));
#endif
skip_getdir:
	if(!canletgo(obj,E_J("throw","投げる")))
		return(0);
	if (obj->oartifact == ART_MJOLLNIR && obj != uwep) {
#ifndef JP
	    pline("%s must be wielded before it can be thrown.",
		The(xname(obj)));
#else
	    pline("%sを投げるには、あらかじめ装備しておく必要がある。",
		xname(obj));
#endif /*JP*/
		return(0);
	}
	if ((obj->oartifact == ART_MJOLLNIR && ACURR(A_STR) < STR19(20))
	   || (obj->otyp == BOULDER && !throws_rocks(youmonst.data))) {
		pline(E_J("It's too heavy.","これは重すぎる。"));
		return(1);
	}
	if(!u.dx && !u.dy && !u.dz) {
		You(E_J("cannot throw an object at yourself.",
			"物を自分めがけて投げることはできない。"));
		return(0);
	}
	u_wipe_engr(2);
	if (!uarmg && !Stone_resistance && (obj->otyp == CORPSE &&
		    touch_petrifies(&mons[obj->corpsenm]))) {
#ifndef JP
		You("throw the %s corpse with your bare %s.",
		    mons[obj->corpsenm].mname, body_part(HAND));
		Sprintf(killer_buf, "%s corpse", an(mons[obj->corpsenm].mname));
#else
		You("%sの死体を素%sで投げようとした。",
		    JMONNAM(obj->corpsenm), body_part(HAND));
		Sprintf(killer_buf, "%sの死体に触れて", JMONNAM(obj->corpsenm));
#endif /*JP*/
		instapetrify(killer_buf);
	}
	if (welded(obj)) {
		weldmsg(obj);
		return 1;
	}

	/* Multishot calculations
	 */
	aml = ammo_and_launcher(obj, uwep);
	if ((aml || throwing_weapon(obj) || is_missile(obj)) &&
		!(Confusion || Stunned)) {
	    /* Bonus if the player is proficient in this weapon... */
	    skill = aml ? P_SKILL(weapon_type(uwep)) : P_SKILL(weapon_type(obj));
	    if (skill >= P_SKILLED) multishot++;
	    if (skill >= P_EXPERT ) multishot++;
	    /* ...or is using a special weapon for their role... */
	    switch (Role_switch) {
	    case PM_RANGER:
		multishot++;
		break;
	    case PM_ROGUE:
		if (objects[obj->otyp].oc_skill == P_DAGGER_GROUP ||
		    objects[obj->otyp].oc_skill == P_KNIFE_GROUP) multishot++;
		break;
	    case PM_SAMURAI:
		if (obj->otyp == YA && uwep && uwep->otyp == YUMI) multishot++;
		break;
	    default:
		break;	/* No bonus */
	    }
	    /* ...or using their race's special bow */
	    switch (Race_switch) {
	    case PM_ELF:
		if (obj->otyp == ELVEN_ARROW && uwep &&
				uwep->otyp == ELVEN_BOW) multishot++;
		break;
	    case PM_ORC:
		if (obj->otyp == ORCISH_ARROW && uwep &&
				uwep->otyp == ORCISH_BOW) multishot++;
		break;
	    default:
		break;	/* No bonus */
	    }
	}

	if ((long)multishot > obj->quan) multishot = (int)obj->quan;
	multishot = rnd(multishot);
	if (shotlimit > 0 && multishot > shotlimit) multishot = shotlimit;

	m_shot.s = aml;
	/* give a message if shooting more than one, or if player
	   attempted to specify a count */
	if (multishot > 1 || shotlimit > 0) {
	    /* "You shoot N arrows." or "You throw N daggers." */
#ifndef JP
	    You("%s %d %s.",
		m_shot.s ? "shoot" : "throw",
		multishot,	/* (might be 1 if player gave shotlimit) */
		(multishot == 1) ? singular(obj, xname) :  xname(obj));
#else
	    You("%d%sの%sを%s。",
		multishot,	/* (might be 1 if player gave shotlimit) */
		jjosushi(obj),
		xname(obj),
		m_shot.s ? (is_gun(uwep) ? "撃った" : "射た") : "投げた");
#endif /*JP*/
	}

	wep_mask = obj->owornmask;
	m_shot.o = obj->otyp;
	m_shot.n = multishot;
	for (m_shot.i = 1; m_shot.i <= m_shot.n; m_shot.i++) {
	    twoweap = u.twoweap;
	    /* split this object off from its slot if necessary */
	    if (obj->quan > 1L) {
		otmp = splitobj(obj, 1L);
		/* in case hero identifies the weapon quality,
		   record original group's id */
		if (otmp->oclass == WEAPON_CLASS)
		    otmp->corpsenm = obj->o_id;
	    } else {
		otmp = obj;
		if (otmp->owornmask)
		    remove_worn_item(otmp, FALSE);
		if (otmp->oclass == WEAPON_CLASS)
		    otmp->corpsenm = 0;
	    }
	    freeinv(otmp);
	    throwit(otmp, wep_mask, twoweap);
	}
	m_shot.n = m_shot.i = 0;
	m_shot.o = STRANGE_OBJECT;
	m_shot.s = FALSE;

	return 1;
}


int
dothrow()
{
	register struct obj *obj;
	int shotlimit;
#ifdef JP
	static const struct getobj_words thrww = { 0, 0, "投げる", "投げ" };
#endif /*JP*/

	/*
	 * Since some characters shoot multiple missiles at one time,
	 * allow user to specify a count prefix for 'f' or 't' to limit
	 * number of items thrown (to avoid possibly hitting something
	 * behind target after killing it, or perhaps to conserve ammo).
	 *
	 * Prior to 3.3.0, command ``3t'' meant ``t(shoot) t(shoot) t(shoot)''
	 * and took 3 turns.  Now it means ``t(shoot at most 3 missiles)''.
	 */
	/* kludge to work around parse()'s pre-decrement of `multi' */
	shotlimit = (multi || save_cm) ? multi + 1 : 0;
	multi = 0;		/* reset; it's been used up */

	if (notake(youmonst.data)) {
#ifndef JP
	    You("are physically incapable of throwing anything.");
#else
	    Your("身体は物を投げられるような形態ではない。");
#endif /*JP*/
	    return 0;
	}

	if(check_capacity((char *)0)) return(0);
	obj = getobj(uslinging() ? bullets : toss_objs, E_J("throw",&thrww), 0);
	/* it is also possible to throw food */
	/* (or jewels, or iron balls... ) */

	if (!obj) return(0);
	return throw_obj(obj, shotlimit);
}


/* KMH -- Automatically fill quiver */
/* Suggested by Jeffrey Bay <jbay@convex.hp.com> */
static void
autoquiver()
{
	struct obj *otmp, *oammo = 0, *omissile = 0, *omisc = 0, *altammo = 0;

	if (uquiver)
	    return;

	/* Scan through the inventory */
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (otmp->owornmask || otmp->oartifact || !otmp->dknown) {
		;	/* Skip it */
	    } else if (otmp->otyp == ROCK ||
			/* seen rocks or known flint or known glass */
			(objects[otmp->otyp].oc_name_known &&
			 otmp->otyp == FLINT) ||
			(objects[otmp->otyp].oc_name_known &&
			 otmp->oclass == GEM_CLASS &&
			 get_material(otmp) == GLASS)) {
		if (uslinging())
		    oammo = otmp;
		else if (ammo_and_launcher(otmp, uswapwep))
		    altammo = otmp;
		else if (!omisc)
		    omisc = otmp;
	    } else if (otmp->oclass == GEM_CLASS) {
		;	/* skip non-rock gems--they're ammo but
			   player has to select them explicitly */
	    } else if (is_ammo(otmp)) {
		if (ammo_and_launcher(otmp, uwep))
		    /* Ammo matched with launcher (bow and arrow, crossbow and bolt) */
		    oammo = otmp;
		else if (ammo_and_launcher(otmp, uswapwep))
		    altammo = otmp;
		else
		    /* Mismatched ammo (no better than an ordinary weapon) */
		    omisc = otmp;
	    } else if (is_missile(otmp)) {
		/* Missile (dart, shuriken, etc.) */
		omissile = otmp;
	    } else if (otmp->oclass == WEAPON_CLASS && throwing_weapon(otmp)) {
		/* Ordinary weapon */
		if ((objects[otmp->otyp].oc_skill == P_DAGGER_GROUP ||
		     objects[otmp->otyp].oc_skill == P_KNIFE_GROUP)
			&& !omissile) 
		    omissile = otmp;
		else
		    omisc = otmp;
	    }
	}

	/* Pick the best choice */
	if (oammo)
	    setuqwep(oammo);
	else if (omissile)
	    setuqwep(omissile);
	else if (altammo)
	    setuqwep(altammo);
	else if (omisc)
	    setuqwep(omisc);

	return;
}


/* Throw from the quiver */
int
dofire()
{
	int shotlimit;

	if (notake(youmonst.data)) {
#ifndef JP
	    You("are physically incapable of doing that.");
#else
	    Your("身体はそれをできるような形態ではない。");
#endif /*JP*/
	    return 0;
	}

	if(check_capacity((char *)0)) return(0);
	if (uwep && is_gun(uwep)) {
		if (!fire_gun(uwep, FALSE)) return FALSE;
		if (u.twoweap && is_gun(uswapwep))
			fire_gun(uswapwep, FALSE);
		return TRUE;
	}
	if (!uquiver) {
		if (!flags.autoquiver) {
			/* Don't automatically fill the quiver */
#ifndef JP
			You("have no ammunition readied!");
#else
			Your("矢筒は空だ！");
#endif /*JP*/
			return(dothrow());
		}
		autoquiver();
		if (!uquiver) {
			You(E_J("have nothing appropriate for your quiver!",
				"矢筒に入れられる矢玉を持っていない！"));
			return(dothrow());
		} else {
			You(E_J("fill your quiver:","矢筒に矢玉を込めた:"));
			prinv((char *)0, uquiver, 0L);
		}
	}

	/*
	 * Since some characters shoot multiple missiles at one time,
	 * allow user to specify a count prefix for 'f' or 't' to limit
	 * number of items thrown (to avoid possibly hitting something
	 * behind target after killing it, or perhaps to conserve ammo).
	 *
	 * The number specified can never increase the number of missiles.
	 * Using ``5f'' when the shooting skill (plus RNG) dictates launch
	 * of 3 projectiles will result in 3 being shot, not 5.
	 */
	/* kludge to work around parse()'s pre-decrement of `multi' */
	shotlimit = (multi || save_cm) ? multi + 1 : 0;
	multi = 0;		/* reset; it's been used up */

	return throw_obj(uquiver, shotlimit);
}

/* Throw by 'v' command: throw from quiver, at the last target */
int
dovfire()
{
	struct monst *mtmp;
	int shotlimit;

	if (!autotarget()) return dofire();

	if (uwep && is_gun(uwep)) {
	    if (!fire_gun(uwep, TRUE)) return 0;
	    if (u.twoweap && is_gun(uswapwep))
		fire_gun(uswapwep, TRUE);
	    return 1;
	}

	shotlimit = (multi || save_cm) ? multi + 1 : 0;
	multi = 0;
	return throw_obj(uquiver, shotlimit|SHOOT_LASTTGT);
}

/*
 * Object hits floor at hero's feet.  Called from drop() and throwit().
 */
void
hitfloor(obj)
register struct obj *obj;
{
	if (IS_SOFT(levl[u.ux][u.uy].typ) || u.uinwater) {
		dropy(obj);
		return;
	}
	if (IS_ALTAR(levl[u.ux][u.uy].typ))
		doaltarobj(obj);
	else
#ifndef JP
		pline("%s hit%s the %s.", Doname2(obj),
		      (obj->quan == 1L) ? "s" : "", surface(u.ux,u.uy));
#else
		pline("%sは%sに命中した。", doname(obj), surface(u.ux,u.uy));
#endif /*JP*/

	if (hero_breaks(obj, u.ux, u.uy, TRUE)) return;
	if (ship_object(obj, u.ux, u.uy, FALSE)) return;
	dropy(obj);
	if (!u.uswallow) container_impact_dmg(obj);
}

/*
 * Walk a path from src_cc to dest_cc, calling a proc for each location
 * except the starting one.  If the proc returns FALSE, stop walking
 * and return FALSE.  If stopped early, dest_cc will be the location
 * before the failed callback.
 */
boolean
walk_path(src_cc, dest_cc, check_proc, arg)
    coord *src_cc;
    coord *dest_cc;
    boolean FDECL((*check_proc), (genericptr_t, int, int));
    genericptr_t arg;
{
    int x, y, dx, dy, x_change, y_change, err, i, prev_x, prev_y;
    boolean keep_going = TRUE;

    /* Use Bresenham's Line Algorithm to walk from src to dest */
    dx = dest_cc->x - src_cc->x;
    dy = dest_cc->y - src_cc->y;
    prev_x = x = src_cc->x;
    prev_y = y = src_cc->y;

    if (dx < 0) {
	x_change = -1;
	dx = -dx;
    } else
	x_change = 1;
    if (dy < 0) {
	y_change = -1;
	dy = -dy;
    } else
	y_change = 1;

    i = err = 0;
    if (dx < dy) {
	while (i++ < dy) {
	    prev_x = x;
	    prev_y = y;
	    y += y_change;
	    err += dx;
	    if (err >= dy) {
		x += x_change;
		err -= dy;
	    }
	/* check for early exit condition */
	if (!(keep_going = (*check_proc)(arg, x, y)))
	    break;
	}
    } else {
	while (i++ < dx) {
	    prev_x = x;
	    prev_y = y;
	    x += x_change;
	    err += dy;
	    if (err >= dx) {
		y += y_change;
		err -= dx;
	    }
	/* check for early exit condition */
	if (!(keep_going = (*check_proc)(arg, x, y)))
	    break;
	}
    }

    if (keep_going)
	return TRUE;	/* successful */

    dest_cc->x = prev_x;
    dest_cc->y = prev_y;
    return FALSE;
}

/*
 * Single step for the hero flying through the air from jumping, flying,
 * etc.  Called from hurtle() and jump() via walk_path().  We expect the
 * argument to be a pointer to an integer -- the range -- which is
 * used in the calculation of points off if we hit something.
 *
 * Bumping into monsters won't cause damage but will wake them and make
 * them angry.  Auto-pickup isn't done, since you don't have control over
 * your movements at the time.
 *
 * Possible additions/changes:
 *	o really attack monster if we hit one
 *	o set stunned if we hit a wall or door
 *	o reset nomul when we stop
 *	o creepy feeling if pass through monster (if ever implemented...)
 *	o bounce off walls
 *	o let jumps go over boulders
 */
boolean
hurtle_step(arg, x, y)
    genericptr_t arg;
    int x, y;
{
    int ox, oy, *range = (int *)arg;
    struct obj *obj;
    struct monst *mon;
    boolean may_pass = TRUE;
    struct trap *ttmp;
    
    if (!isok(x,y)) {
	You_feel("the spirits holding you back.");
	return FALSE;
    } else if (!in_out_region(x, y)) {
	return FALSE;
    } else if (*range == 0) {
	return FALSE;			/* previous step wants to stop now */
    }

    if (!Passes_walls || !(may_pass = may_passwall(x, y))) {
	if (IS_ROCK(levl[x][y].typ) || closed_door(x,y)) {
	    const char *s;

	    pline(E_J("Ouch!","痛っ！"));
	    if (IS_TREES(levl[x][y].typ))
		s = E_J("bumping into a tree","木にぶつかって");
	    else if (IS_ROCK(levl[x][y].typ))
		s = E_J("bumping into a wall","壁にぶつかって");
	    else
		s = E_J("bumping into a door","扉にぶつかって");
	    losehp(rnd(2+*range), s, KILLED_BY);
	    return FALSE;
	}
	if (levl[x][y].typ == IRONBARS) {
	    You(E_J("crash into some iron bars.  Ouch!",
		    "鉄格子に激突した。痛い！"));
	    losehp(rnd(2+*range), E_J("crashing into iron bars","鉄格子に激突して"), KILLED_BY);
	    return FALSE;
	}
	if ((obj = sobj_at(BOULDER,x,y)) != 0) {
	    You(E_J("bump into a %s.  Ouch!",
		    "%sにぶつかった。痛い！"), xname(obj));
	    losehp(rnd(2+*range), E_J("bumping into a boulder","大岩にぶつかって"), KILLED_BY);
	    return FALSE;
	}
	if (!may_pass) {
	    /* did we hit a no-dig non-wall position? */
	    You(E_J("smack into something!","何かに突っ込んだ！"));
	    losehp(rnd(2+*range), E_J("touching the edge of the universe",
				      "宇宙の果てに触れて"), KILLED_BY);
	    return FALSE;
	}
	if ((u.ux - x) && (u.uy - y) &&
		bad_rock(youmonst.data,u.ux,y) && bad_rock(youmonst.data,x,u.uy)) {
	    boolean too_much = (invent && (inv_weight() + weight_cap() > 600));
	    /* Move at a diagonal. */
	    if (bigmonst(youmonst.data) || too_much) {
#ifndef JP
		You("%sget forcefully wedged into a crevice.",
			too_much ? "and all your belongings " : "");
		losehp(rnd(2+*range), "wedging into a narrow crevice", KILLED_BY);
#else
		pline("あなた%sは無理矢理裂け目にねじ込まれた。",
			too_much ? "と持ち物" : "");
		losehp(rnd(2+*range), "狭い裂け目にねじ込まれて", KILLED_BY);
#endif /*JP*/
		return FALSE;
	    }
	}
    }

    if ((mon = m_at(x, y)) != 0) {
	You(E_J("bump into %s.","%sにぶつかった。"), a_monnam(mon));
	wakeup(mon);
	return FALSE;
    }
    if ((u.ux - x) && (u.uy - y) &&
	bad_rock(youmonst.data,u.ux,y) && bad_rock(youmonst.data,x,u.uy)) {
	/* Move at a diagonal. */
	if (In_sokoban(&u.uz)) {
	    You(E_J("come to an abrupt halt!","不意に停止した！"));
	    return FALSE;
	}
    }

    ox = u.ux;
    oy = u.uy;
    u.ux = x;
    u.uy = y;
    newsym(ox, oy);		/* update old position */
    vision_recalc(1);		/* update for new position */
    flush_screen(1);
    /* FIXME:
     * Each trap should really trigger on the recoil if
     * it would trigger during normal movement. However,
     * not all the possible side-effects of this are
     * tested [as of 3.4.0] so we trigger those that
     * we have tested, and offer a message for the
     * ones that we have not yet tested.
     */
    if ((ttmp = t_at(x, y)) != 0) {
    	if (ttmp->ttyp == MAGIC_PORTAL) {
    		dotrap(ttmp,0);
    		return FALSE;
	} else if (ttmp->ttyp == FIRE_TRAP) {
    		dotrap(ttmp,0);
	} else if ((ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT ||
		    ttmp->ttyp == HOLE || ttmp->ttyp == TRAPDOOR) &&
		   In_sokoban(&u.uz)) {
		/* Air currents overcome the recoil */
    		dotrap(ttmp,0);
		*range = 0;
		return TRUE;
    	} else {
		if (ttmp->tseen)
#ifndef JP
		    You("pass right over %s %s.",
		    	(ttmp->ttyp == ARROW_TRAP) ? "an" : "a",
		    	defsyms[trap_to_defsym(ttmp->ttyp)].explanation);
#else
		    You("%sの真上を通過した。",
		    	defsyms[trap_to_defsym(ttmp->ttyp)].explanation);
#endif /*JP*/
    	}
    }
    if (--*range < 0)		/* make sure our range never goes negative */
	*range = 0;
    if (*range != 0)
	delay_output();
    return TRUE;
}

STATIC_OVL boolean
mhurtle_step(arg, x, y)
    genericptr_t arg;
    int x, y;
{
	struct monst *mon = (struct monst *)arg;

	/* TODO: Treat walls, doors, iron bars, pools, lava, etc. specially
	 * rather than just stopping before.
	 */
	if (goodpos(x, y, mon, 0) && m_in_out_region(mon, x, y)) {
	    remove_monster(mon->mx, mon->my);
	    newsym(mon->mx, mon->my);
	    place_monster(mon, x, y);
	    newsym(mon->mx, mon->my);
	    set_apparxy(mon);
	    (void) mintrap(mon);
	    return TRUE;
	}
	return FALSE;
}

/*
 * The player moves through the air for a few squares as a result of
 * throwing or kicking something.
 *
 * dx and dy should be the direction of the hurtle, not of the original
 * kick or throw and be only.
 */
void
hurtle(dx, dy, range, verbose)
    int dx, dy, range;
    boolean verbose;
{
    coord uc, cc;

    /* The chain is stretched vertically, so you shouldn't be able to move
     * very far diagonally.  The premise that you should be able to move one
     * spot leads to calculations that allow you to only move one spot away
     * from the ball, if you are levitating over the ball, or one spot
     * towards the ball, if you are at the end of the chain.  Rather than
     * bother with all of that, assume that there is no slack in the chain
     * for diagonal movement, give the player a message and return.
     */
    if(Punished && !carried(uball)) {
	You_feel(E_J("a tug from the iron ball.","鉄球に引っ張られるのを感じた。"));
	nomul(0);
	return;
    } else if (u.utrap) {
#ifndef JP
	You("are anchored by the %s.",
	    u.utraptype == TT_WEB ? "web" : u.utraptype == TT_LAVA ? "lava" :
		u.utraptype == TT_INFLOOR ? surface(u.ux,u.uy) : "trap");
#else
	You("%sに捕らわれている。",
	    u.utraptype == TT_WEB ? "蜘蛛の巣" : u.utraptype == TT_LAVA ? "溶岩" :
		u.utraptype == TT_INFLOOR ? surface(u.ux,u.uy) : "罠");
#endif /*JP*/
	nomul(0);
	return;
    }

    /* make sure dx and dy are [-1,0,1] */
//    dx = sgn(dx);
//    dy = sgn(dy);

    if(!range || (!dx && !dy) || u.ustuck) return; /* paranoia */

    nomul(-range);
    if (verbose)
#ifndef JP
	You("%s in the opposite direction.", range > 1 ? "hurtle" : "float");
#else
	You("反対の方向へ%s。", range > 1 ? "飛ばされた" : "漂った");
#endif /*JP*/
    /* if we're in the midst of shooting multiple projectiles, stop */
    if (m_shot.i < m_shot.n) {
	/* last message before hurtling was "you shoot N arrows" */
#ifndef JP
	You("stop %sing after the first %s.",
	    m_shot.s ? "shoot" : "throw", m_shot.s ? "shot" : "toss");
#else
	You("最初の%sで%sのを止めた。",
	    m_shot.s ? "射撃" : "投擲", m_shot.s ? "撃つ" : "投げる");
#endif /*JP*/
	m_shot.n = m_shot.i;	/* make current shot be the last */
    }
    if (In_sokoban(&u.uz))
	change_luck(-1);	/* Sokoban guilt */
    uc.x = u.ux;
    uc.y = u.uy;
    /* this setting of cc is only correct if dx and dy are [-1,0,1] only */
//    cc.x = u.ux + (dx * range);
//    cc.y = u.uy + (dy * range);
    cc.x = u.ux + dx;
    cc.y = u.uy + dy;
    (void) walk_path(&uc, &cc, hurtle_step, (genericptr_t)&range);
}

/* Move a monster through the air for a few squares.
 */
void
mhurtle(mon, dx, dy, range)
	struct monst *mon;
	int dx, dy, range;
{
    coord mc, cc;

#ifdef MONSTEED
	if (is_mridden(mon)) mon = mon->mlink;
#endif

	/* At the very least, debilitate the monster */
	mon->movement = 0;
	mon->mstun = 1;

	/* Is the monster stuck or too heavy to push?
	 * (very large monsters have too much inertia, even floaters and flyers)
	 */
	if (mon->data->msize >= MZ_HUGE || mon == u.ustuck || mon->mtrapped)
	    return;

    /* Make sure dx and dy are [-1,0,1] */
    dx = sgn(dx);
    dy = sgn(dy);
    if(!range || (!dx && !dy)) return; /* paranoia */

	/* Send the monster along the path */
	mc.x = mon->mx;
	mc.y = mon->my;
	cc.x = mon->mx + (dx * range);
	cc.y = mon->my + (dy * range);
	(void) walk_path(&mc, &cc, mhurtle_step, (genericptr_t)mon);
	return;
}

STATIC_OVL void
check_shop_obj(obj, x, y, broken)
register struct obj *obj;
register xchar x, y;
register boolean broken;
{
	struct monst *shkp = shop_keeper(*u.ushops);

	if(!shkp) return;

	if(broken) {
		if (obj->unpaid) {
		    (void)stolen_value(obj, u.ux, u.uy,
				       (boolean)shkp->mpeaceful, FALSE);
		    subfrombill(obj, shkp);
		}
		obj->no_charge = 1;
		return;
	}

	if (!costly_spot(x, y) || *in_rooms(x, y, SHOPBASE) != *u.ushops) {
		/* thrown out of a shop or into a different shop */
		if (obj->unpaid) {
		    (void)stolen_value(obj, u.ux, u.uy,
				       (boolean)shkp->mpeaceful, FALSE);
		    subfrombill(obj, shkp);
		}
	} else {
		if (costly_spot(u.ux, u.uy) && costly_spot(x, y)) {
		    if(obj->unpaid) subfrombill(obj, shkp);
		    else if(!(x == shkp->mx && y == shkp->my))
			    sellobj(obj, x, y);
		}
	}
}

/*
 * Hero tosses an object upwards with appropriate consequences.
 *
 * Returns FALSE if the object is gone.
 */
STATIC_OVL boolean
toss_up(obj, hitsroof)
struct obj *obj;
boolean hitsroof;
{
    const char *almost;
    /* note: obj->quan == 1 */

    if (hitsroof) {
	if (breaktest(obj)) {
#ifndef JP
		pline("%s hits the %s.", Doname2(obj), ceiling(u.ux, u.uy));
#else
		pline("%sは%sに当たった。", doname(obj), ceiling(u.ux, u.uy));
#endif /*JP*/
		breakmsg(obj, !Blind);
		breakobj(obj, u.ux, u.uy, TRUE, TRUE);
		return FALSE;
	}
	almost = E_J("", "に当た");
    } else {
	almost = E_J(" almost", "近くまで上が");
    }
#ifndef JP
    pline("%s%s hits the %s, then falls back on top of your %s.",
	  Doname2(obj), almost, ceiling(u.ux,u.uy), body_part(HEAD));
#else
    pline("%sは%s%sると、あなたの%sめがけて降ってきた。",
	  doname(obj), ceiling(u.ux,u.uy), almost, body_part(HEAD));
#endif /*JP*/

    /* object now hits you */

    if (obj->oclass == POTION_CLASS) {
	potionhit(&youmonst, obj, TRUE);
    } else if (breaktest(obj)) {
	int otyp = obj->otyp, ocorpsenm = obj->corpsenm;
	int blindinc;

	/* need to check for blindness result prior to destroying obj */
	blindinc = (otyp == CREAM_PIE || otyp == BLINDING_VENOM) &&
		   /* AT_WEAP is ok here even if attack type was AT_SPIT */
		   can_blnd(&youmonst, &youmonst, AT_WEAP, obj) ? rnd(25) : 0;

	breakmsg(obj, !Blind);
	breakobj(obj, u.ux, u.uy, TRUE, TRUE);
	obj = 0;	/* it's now gone */
	switch (otyp) {
	case EGG:
		if (touch_petrifies(&mons[ocorpsenm]) &&
		    !uarmh && !Stone_resistance &&
		    !(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM)))
		goto petrify;
	case CREAM_PIE:
	case BLINDING_VENOM:
		pline(E_J("You've got it all over your %s!",
			  "あなたの%s中が覆われた！"), body_part(FACE));
		if (blindinc) {
		    if (otyp == BLINDING_VENOM && !Blind)
			pline(E_J("It blinds you!","目が見えない！"));
		    u.ucreamed += blindinc;
		    make_blinded(Blinded + (long)blindinc, FALSE);
		    if (!Blind) Your(vision_clears);
		}
		break;
	default:
		break;
	}
	return FALSE;
    } else {		/* neither potion nor other breaking object */
	boolean less_damage = uarmh && is_metallic(uarmh), artimsg = FALSE;
	int dmg = dmgval(obj, &youmonst);

	if (obj->oartifact)
	    /* need a fake die roll here; rn1(18,2) avoids 1 and 20 */
	    artimsg = artifact_hit((struct monst *)0, &youmonst,
				   obj, &dmg, rn1(18,2));

	if (!dmg) {	/* probably wasn't a weapon; base damage on weight */
	    dmg = (int) obj->owt / 100;
	    if (dmg < 1) dmg = 1;
	    else if (dmg > 6) dmg = 6;
	    if (youmonst.data == &mons[PM_SHADE] &&
		    get_material(obj) != SILVER)
		dmg = 0;
	}
	if (dmg > 1 && less_damage) dmg = 1;
	if (dmg > 0) dmg += u.udaminc;
	if (dmg < 0) dmg = 0;	/* beware negative rings of increase damage */
	if (Half_physical_damage) dmg = (dmg + 1) / 2;

	if (uarmh) {
	    if (less_damage && dmg < (Upolyd ? u.mh : u.uhp)) {
		if (!artimsg)
		    pline(E_J("Fortunately, you are wearing a hard helmet.",
			      "幸い、あなたは硬い兜をかぶっていた。"));
	    } else if (flags.verbose &&
		    !(obj->otyp == CORPSE && touch_petrifies(&mons[obj->corpsenm])))
#ifndef JP
		Your("%s does not protect you.", xname(uarmh));
#else
		Your("%sは%sを防げなかった。", xname(uarmh), xname(obj));
#endif /*JP*/
	} else if (obj->otyp == CORPSE && touch_petrifies(&mons[obj->corpsenm])) {
	    if (!Stone_resistance &&
		    !(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
 petrify:
		killer_format = KILLED_BY;
		killer = E_J("elementary physics","初等物理の実験中に");	/* "what goes up..." */
		You(E_J("turn to stone.","石になった。"));
		if (obj) dropy(obj);	/* bypass most of hitfloor() */
		done(STONING);
		return obj ? TRUE : FALSE;
	    }
	}
	hitfloor(obj);
	losehp(dmg, E_J("falling object","落下物に当たって"), KILLED_BY_AN);
    }
    return TRUE;
}

/* return true for weapon meant to be thrown; excludes ammo */
STATIC_OVL boolean
throwing_weapon(obj)
struct obj *obj;
{
	return (((objects[obj->otyp].oc_wprop & WP_THROWING) != 0 &&			/* throwing weapon */
		 (objects[obj->otyp].oc_wprop & WP_WEAPONTYPE) != WP_AMMUNITION));	/* excludes ammo */
			/*is_missile(obj) || is_spear(obj) ||*/
			/* daggers and knife (excludes scalpel) */
			/*(is_blade(obj) && (objects[obj->otyp].oc_dir & PIERCE)) ||*/
			/* special cases [might want to add AXE] */
			/*obj->otyp == HEAVY_HAMMER || obj->otyp == AKLYS);*/
}

/* the currently thrown object is returning to you (not for boomerangs) */
STATIC_OVL void
sho_obj_return_to_u(obj)
struct obj *obj;
{
    /* might already be our location (bounced off a wall) */
//    if (bhitpos.x != u.ux || bhitpos.y != u.uy) {
//	int x = bhitpos.x - u.dx, y = bhitpos.y - u.dy;
//
//	tmp_at(DISP_FLASH, obj_to_glyph(obj));
//	while(x != u.ux || y != u.uy) {
//	    tmp_at(x, y);
//	    delay_output();
//	    x -= u.dx; y -= u.dy;
//	}
//	tmp_at(DISP_END, 0);
//    }
    /* might already be our location (bounced off a wall) */
    tmp_at(DISP_FLASH, obj_to_glyph(obj));
    while (bhitpos.x != u.ux || bhitpos.y != u.uy) {
	bresenham_back(&bhitpos);
	tmp_at(bhitpos.x, bhitpos.y);
	delay_output();
    }
    tmp_at(DISP_END, 0);
}

void
throwit(obj, wep_mask, twoweap)
register struct obj *obj;
long wep_mask;	/* used to re-equip returning boomerang */
boolean twoweap; /* used to restore twoweapon mode if wielded weapon returns */
{
	register struct monst *mon;
	register int range, urange;
	boolean impaired = (Confusion || Stunned || Blind ||
			   Hallucination || Fumbling);

//	if (obj->oclass == WEAPON_CLASS)
//	    obj->leashmon = 0; /* shooter is you */

	if ((obj->cursed || obj->greased) && (u.dx || u.dy) && !rn2(7)) {
	    boolean slipok = TRUE;
	    if (ammo_and_launcher(obj, uwep))
#ifndef JP
		pline("%s!", Tobjnam(obj, "misfire"));
#else
		pline("%sは狙いをそれた！", xname(obj));
#endif /*JP*/
	    else {
		/* only slip if it's greased or meant to be thrown */
		if (obj->greased || throwing_weapon(obj))
		    /* BUG: this message is grammatically incorrect if obj has
		       a plural name; greased gloves or boots for instance. */
#ifndef JP
		    pline("%s as you throw it!", Tobjnam(obj, "slip"));
#else
		    pline("投げようとしたとたん、%sは滑り落ちてしまった！", xname(obj));
#endif /*JP*/
		else slipok = FALSE;
	    }
	    if (slipok) {
		u.dx = rn2(3)-1;
		u.dy = rn2(3)-1;
		if (!u.dx && !u.dy) u.dz = 1;
		impaired = TRUE;
	    }
	}

	if ((u.dx || u.dy || (u.dz < 1)) &&
	    calc_capacity((int)obj->owt) > SLT_ENCUMBER &&
	    (Upolyd ? (u.mh < 5 && u.mh != u.mhmax)
	     : (u.uhp < 10 && u.uhp != u.uhpmax)) &&
	    obj->owt > (unsigned)((Upolyd ? u.mh : u.uhp) * 2) &&
	    !Is_airlevel(&u.uz)) {
#ifndef JP
	    You("have so little stamina, %s drops from your grasp.",
		the(xname(obj)));
#else
	    You("あまりにも消耗していたため、%sをとり落としてしまった。",
		xname(obj));
#endif /*JP*/
	    exercise(A_CON, FALSE);
	    u.dx = u.dy = 0;
	    u.dz = 1;
	}

	thrownobj = obj;
#ifdef PICKUP_THROWN
	obj->othrown = 1;
	if (obj->oclass == WEAPON_CLASS) obj->age = monstermoves;
#endif

	if(u.uswallow) {
		mon = u.ustuck;
		bhitpos.x = mon->mx;
		bhitpos.y = mon->my;
	} else if(u.dz) {
	    if (u.dz < 0 && Role_if(PM_VALKYRIE) &&
		    obj->oartifact == ART_MJOLLNIR && !impaired) {
#ifndef JP
		pline("%s the %s and returns to your hand!",
		      Tobjnam(obj, "hit"), ceiling(u.ux,u.uy));
#else
		pline("%sは%sに命中し、あなたの手に戻った！",
		      xname(obj), ceiling(u.ux,u.uy));
#endif /*JP*/
		obj = addinv(obj);
		(void) encumber_msg();
		setuwep(obj);
		u.twoweap = twoweap;
	    } else if (u.dz < 0 && !Is_airlevel(&u.uz) &&
		    !Underwater && !Is_waterlevel(&u.uz)) {
		(void) toss_up(obj, rn2(5));
	    } else {
		hitfloor(obj);
	    }
	    thrownobj = (struct obj*)0;
	    return;

	} else if(obj->otyp == BOOMERANG && !Underwater) {
		if(Is_airlevel(&u.uz) || Levitation)
		    hurtle(-u.dx, -u.dy, 1, TRUE);
		mon = boomhit(u.dx, u.dy, obj);
		if(mon == &youmonst) {		/* the thing was caught */
			exercise(A_DEX, TRUE);
			obj = addinv(obj);
			(void) encumber_msg();
			if (wep_mask && !(obj->owornmask & wep_mask)) {
			    setworn(obj, wep_mask);
			    u.twoweap = twoweap;
			}
			thrownobj = (struct obj *)0;
			return;
		} else if (mon) {
			/* boomerang is gone */
			thrownobj = (struct obj *)0;
			return;
		}
	} else {
		urange = (int)(ACURRSTR)/2;
		/* balls are easy to throw or at least roll */
		/* also, this insures the maximum range of a ball is greater
		 * than 1, so the effects from throwing attached balls are
		 * actually possible
		 */
		if (obj->otyp == HEAVY_IRON_BALL)
			range = urange - (int)(obj->owt/100);
		else
			range = urange - (int)(obj->owt/40);
		if (obj == uball) {
			if (u.ustuck) range = 1;
			else if (range >= 5) range = 5;
		}
		if (range < 1) range = 1;

		if (is_ammo(obj)) {
		    if (ammo_and_launcher(obj, uwep))
			range++;
		    else if (obj->oclass != GEM_CLASS)
			range /= 2;
		}

		if (Is_airlevel(&u.uz) || Levitation) {
		    /* action, reaction... */
		    urange -= range;
		    if(urange < 1) urange = 1;
		    range -= urange;
		    if(range < 1) range = 1;
		}

		if (obj->otyp == BOULDER)
		    range = 20;		/* you must be giant */
		else if (obj->oartifact == ART_MJOLLNIR)
		    range = (range + 1) / 2;	/* it's heavy */
		else if (obj == uball && u.utrap && u.utraptype == TT_INFLOOR)
		    range = 1;

		if (Underwater) range = 1;

		mon = bhit(u.dx, u.dy, range, THROWN_WEAPON,
			   (int FDECL((*),(MONST_P,OBJ_P)))0,
			   (int FDECL((*),(OBJ_P,OBJ_P)))0,
			   obj);

		/* have to do this after bhit() so u.ux & u.uy are correct */
		if(Is_airlevel(&u.uz) || Levitation)
		    hurtle(-u.dx, -u.dy, urange, TRUE);
	}

	if (mon) {
		boolean obj_gone;

		if (mon->isshk &&
		    obj->where == OBJ_MINVENT && obj->ocarry == mon) {
		    thrownobj = (struct obj*)0;
		    return;		/* alert shk caught it */
		}
		(void) snuff_candle(obj);
		notonhead = (bhitpos.x != mon->mx || bhitpos.y != mon->my);
		obj_gone = thitmonst(mon, obj);
		/* Monster may have been tamed; this frees old mon */
		mon = m_at(bhitpos.x, bhitpos.y);

		/* [perhaps this should be moved into thitmonst or hmon] */
		/* (moved) */
//		if (mon && mon->isshk &&
//			(!inside_shop(u.ux, u.uy) ||
//			 !index(in_rooms(mon->mx, mon->my, SHOPBASE), *u.ushops)))
//		    hot_pursuit(mon);

		if (obj_gone) return;
	}

	if (u.uswallow) {
		/* ball is not picked up by monster */
		if (obj != uball) (void) mpickobj(u.ustuck,obj);
	} else {
		/* the code following might become part of dropy() */
		if (obj->oartifact == ART_MJOLLNIR &&
			Role_if(PM_VALKYRIE) && rn2(100)) {
		    /* we must be wearing Gauntlets of Power to get here */
		    sho_obj_return_to_u(obj);	    /* display its flight */

		    if (!impaired && rn2(100)) {
#ifndef JP
			pline("%s to your hand!", Tobjnam(obj, "return"));
#else
			pline("%sはあなたの手に戻った！", xname(obj));
#endif /*JP*/
			obj = addinv(obj);
			(void) encumber_msg();
			setuwep(obj);
			u.twoweap = twoweap;
			if(cansee(bhitpos.x, bhitpos.y))
			    newsym(bhitpos.x,bhitpos.y);
		    } else {
			int dmg = rn2(2);
			if (!dmg) {
#ifndef JP
			    pline(Blind ? "%s lands %s your %s." :
					"%s back to you, landing %s your %s.",
				  Blind ? Something : Tobjnam(obj, "return"),
				  Levitation ? "beneath" : "at",
				  makeplural(body_part(FOOT)));
#else
			    pline(Blind ? "%sがあなたの%s%sに落ちた。" :
					"%sは戻ってきて、あなたの%s%に落ちた。",
				  Blind ? Something : xname(obj),
				  body_part(FOOT),
				  Levitation ? "の下" : "元");
#endif /*JP*/
			} else {
			    dmg += rnd(3);
#ifndef JP
			    pline(Blind ? "%s your %s!" :
					"%s back toward you, hitting your %s!",
				  Tobjnam(obj, Blind ? "hit" : "fly"),
				  body_part(ARM));
#else
			    pline(Blind ? "%sがあなたの%sを打った！" :
					"%sが飛来して、あなたの%sに命中した！",
				  xname(obj), body_part(ARM));
#endif /*JP*/
			    (void) artifact_hit((struct monst *)0,
						&youmonst, obj, &dmg, 0);
#ifndef JP
			    losehp(dmg, xname(obj),
				obj_is_pname(obj) ? KILLED_BY : KILLED_BY_AN);
#else
			    {
				char jbuf[BUFSZ];
				Sprintf(jbuf, "%sに当たって", xname(obj));
				losehp(dmg, jbuf, KILLED_BY);
			    }
#endif /*JP*/
			}
			if (ship_object(obj, u.ux, u.uy, FALSE)) {
		    	    thrownobj = (struct obj*)0;
			    return;
			}
			dropy(obj);
		    }
		    thrownobj = (struct obj*)0;
		    return;
		}

		if (!IS_SOFT(levl[bhitpos.x][bhitpos.y].typ) &&
			breaktest(obj)) {
		    tmp_at(DISP_FLASH, obj_to_glyph(obj));
		    tmp_at(bhitpos.x, bhitpos.y);
		    delay_output();
		    tmp_at(DISP_END, 0);
		    breakmsg(obj, cansee(bhitpos.x, bhitpos.y));
		    breakobj(obj, bhitpos.x, bhitpos.y, TRUE, TRUE);
		    return;
		}
		if(flooreffects(obj,bhitpos.x,bhitpos.y,E_J("fall","落ちた"))) return;
		obj_no_longer_held(obj);
		if (mon && mon->isshk && is_pick(obj)) {
		    if (cansee(bhitpos.x, bhitpos.y))
#ifndef JP
			pline("%s snatches up %s.",
			      Monnam(mon), the(xname(obj)));
#else
			pline("%sは%sを受け止めた。",
			      mon_nam(mon), xname(obj));
#endif /*JP*/
		    if(*u.ushops)
			check_shop_obj(obj, bhitpos.x, bhitpos.y, FALSE);
		    (void) mpickobj(mon, obj);	/* may merge and free obj */
		    thrownobj = (struct obj*)0;
		    return;
		}
		(void) snuff_candle(obj);
		if (!mon && ship_object(obj, bhitpos.x, bhitpos.y, FALSE)) {
		    thrownobj = (struct obj*)0;
		    return;
		}
		thrownobj = (struct obj*)0;
		place_object(obj, bhitpos.x, bhitpos.y);
		if(*u.ushops && obj != uball)
		    check_shop_obj(obj, bhitpos.x, bhitpos.y, FALSE);

		stackobj(obj);
		if (obj == uball)
		    drop_ball(bhitpos.x, bhitpos.y);
		if (cansee(bhitpos.x, bhitpos.y))
		    newsym(bhitpos.x,bhitpos.y);
		if (obj_sheds_light(obj))
		    vision_full_recalc = 1;
		if (!IS_SOFT(levl[bhitpos.x][bhitpos.y].typ))
		    container_impact_dmg(obj);
	}
}

/* an object may hit a monster; various factors adjust the chance of hitting */
int
omon_adj(mon, obj, mon_notices)
struct monst *mon;
struct obj *obj;
boolean mon_notices;
{
	int tmp = 0;

	/* size of target affects the chance of hitting */
	tmp += (mon->data->msize - MZ_MEDIUM);		/* -2..+5 */
	/* sleeping target is more likely to be hit */
	if (mon->msleeping) {
	    tmp += 2;
	    if (mon_notices) mon->msleeping = 0;
	}
	/* ditto for immobilized target */
	if (!mon->mcanmove || !mon->data->mmove) {
	    tmp += 4;
	    if (mon_notices && mon->data->mmove && !rn2(10)) {
		mon->mcanmove = 1;
		mon->mfrozen = 0;
	    }
	}
	/* some objects are more likely to hit than others */
	switch (obj->otyp) {
	case HEAVY_IRON_BALL:
	    if (obj != uball) tmp += 2;
	    break;
	case BOULDER:
	    tmp += 6;
	    break;
	default:
	    if (obj->oclass == WEAPON_CLASS || is_weptool(obj) ||
		    obj->oclass == GEM_CLASS)
		tmp += hitval(obj, mon);
	    break;
	}
	return tmp;
}

/* thrown object misses target monster */
void
tmiss(obj, mon)
struct obj *obj;
struct monst *mon;
{
    const char *missile = mshot_xname(obj);

    /* If the target can't be seen or doesn't look like a valid target,
       avoid "the arrow misses it," or worse, "the arrows misses the mimic."
       An attentive player will still notice that this is different from
       an arrow just landing short of any target (no message in that case),
       so will realize that there is a valid target here anyway. */
    if (!canseemon(mon) || (mon->m_ap_type && mon->m_ap_type != M_AP_MONSTER))
#ifndef JP
	pline("%s %s.", The(missile), otense(obj, "miss"));
#else
	pline("%sは外れた。", missile);
#endif /*JP*/
    else
	miss(missile, mon);
    if (!rn2(3)) wakeup(mon);
    return;
}

#define quest_arti_hits_leader(obj,mon)	\
  (obj->oartifact && is_quest_artifact(obj) && (mon->data->msound == MS_LEADER))

/*
 * Object thrown by player arrives at monster's location.
 * Return 1 if obj has disappeared or otherwise been taken care of,
 * 0 if caller must take care of it.
 */
int
thitmonst(mon, obj)
register struct monst *mon;
register struct obj   *obj;
{
	register int	tmp; /* Base chance to hit */
	register int	disttmp; /* distance modifier */
	int otyp = obj->otyp;
	boolean guaranteed_hit = (u.uswallow && mon == u.ustuck);

	bhitflag = 0;

	/* Differences from melee weapons:
	 *
	 * Dex still gives a bonus, but strength does not.
	 * Polymorphed players lacking attacks may still throw.
	 * There's a base -1 to hit.
	 * No bonuses for fleeing or stunned targets (they don't dodge
	 *    melee blows as readily, but dodging arrows is hard anyway).
	 * Not affected by traps, etc.
	 * Certain items which don't in themselves do damage ignore tmp.
	 * Distance and monster size affect chance to hit.
	 */
	tmp = abon_luck() + find_mac(mon) + u.uhitinc +
		maybe_polyd(youmonst.data->mlevel, xlev_to_rank(u.ulevel));

	tmp +=  abon_dex();
//	if (ACURR(A_DEX) < 4) tmp -= 3;
//	else if (ACURR(A_DEX) < 6) tmp -= 2;
//	else if (ACURR(A_DEX) < 8) tmp -= 1;
//	else if (ACURR(A_DEX) >= 14) tmp += (ACURR(A_DEX) - 14);

	/* Modify to-hit depending on distance; but keep it sane.
	 * Polearms get a distance penalty even when wielded; it's
	 * hard to hit at a distance.
	 */
	disttmp = 3 - distmin(u.ux, u.uy, mon->mx, mon->my);
	if(disttmp < -4) disttmp = -4;
	tmp += disttmp;

	/* gloves are a hinderance to proper use of bows */
	if (uarmg && uwep &&
	    (objects[uwep->otyp].oc_skill == P_BOW_GROUP || is_gun(uwep))) {
	    switch (uarmg->otyp) {
	    case GAUNTLETS:
		if (!is_metallic(uarmg)) break;
		/* fall through */
	    case GAUNTLETS_OF_POWER:    /* metal */
		tmp -= 2;
		break;
	    case GAUNTLETS_OF_FUMBLING:
		tmp -= 3;
		break;
	    case LEATHER_GLOVES:
	    case GAUNTLETS_OF_DEXTERITY:
		break;
	    default:
		impossible("Unknown type of gloves (%d)", uarmg->otyp);
		break;
	    }
	}

	tmp += omon_adj(mon, obj, TRUE);
	if (is_orc(mon->data) && maybe_polyd(is_elf(youmonst.data),
			Race_if(PM_ELF)))
	    tmp++;
	if (guaranteed_hit) {
	    tmp += 1000; /* Guaranteed hit */
	}

	if (obj->oclass == GEM_CLASS && is_unicorn(mon->data)) {
	    if (mon->mtame) {
#ifndef JP
		pline("%s catches and drops %s.", Monnam(mon), the(xname(obj)));
#else
		pline("%sは%sを受け止めた後、捨てた。", Monnam(mon), xname(obj));
#endif /*JP*/
		return 0;
	    } else {
#ifndef JP
		pline("%s catches %s.", Monnam(mon), the(xname(obj)));
#else
		pline("%sは%sを受け止めた。", Monnam(mon), xname(obj));
#endif /*JP*/
		return gem_accept(mon, obj);
	    }
	}

	/* don't make game unwinnable if naive player throws artifact
	   at leader.... */
	if (quest_arti_hits_leader(obj, mon)) {
	    /* not wakeup(), which angers non-tame monsters */
	    mon->msleeping = 0;
	    mon->mstrategy &= ~STRAT_WAITMASK;

	    if (mon->mcanmove) {
#ifndef JP
		pline("%s catches %s.", Monnam(mon), the(xname(obj)));
#else
		pline("%sは%sを受け止めた。", Monnam(mon), xname(obj));
#endif /*JP*/
		if (mon->mpeaceful) {
		    boolean next2u = monnear(mon, u.ux, u.uy);

		    finish_quest(obj);	/* acknowledge quest completion */
#ifndef JP
		    pline("%s %s %s back to you.", Monnam(mon),
			  (next2u ? "hands" : "tosses"), the(xname(obj)));
#else
		    pline("%sはあなたに%sを%s返した。", Monnam(mon),
			  xname(obj), (next2u ? "" : "投げ"));
#endif /*JP*/
		    if (!next2u) sho_obj_return_to_u(obj);
		    obj = addinv(obj);	/* back into your inventory */
		    (void) encumber_msg();
		} else {
		    /* angry leader caught it and isn't returning it */
		    (void) mpickobj(mon, obj);
		}
		return 1;		/* caller doesn't need to place it */
	    }
	    return(0);
	}

	if (mon && mon->isshk &&
		(!inside_shop(u.ux, u.uy) ||
		 !index(in_rooms(mon->mx, mon->my, SHOPBASE), *u.ushops)))
	    hot_pursuit(mon);

	if (obj->oclass == WEAPON_CLASS || is_weptool(obj) ||
		obj->oclass == GEM_CLASS) {
	    if (is_ammo(obj)) {
		if (!ammo_and_launcher(obj, uwep)) {
		    tmp -= 4;
		} else {
		    /* unfamiliar launchers are hard to handle */
		    if (P_RESTRICTED(weapon_type(uwep)))
			tmp -= 7;
		    tmp += uwep->spe - greatest_erosion(uwep);
		    tmp += weapon_hit_bonus(uwep);
		    if (uwep->oartifact) tmp += spec_abon(uwep, mon);
		    /*
		     * Elves and Samurais are highly trained w/bows,
		     * especially their own special types of bow.
		     * Polymorphing won't make you a bow expert.
		     */
		    if ((Race_if(PM_ELF) || Role_if(PM_SAMURAI)) &&
			(!Upolyd || your_race(youmonst.data)) &&
			objects[uwep->otyp].oc_skill == P_BOW_GROUP) {
			tmp++;
			if (Race_if(PM_ELF) && uwep->otyp == ELVEN_BOW)
			    tmp++;
			else if (Role_if(PM_SAMURAI) && uwep->otyp == YUMI)
			    tmp++;
		    }
		    if (is_gun(uwep)) {
			if (!bimanual(uwep) && !u.twoweap && !uarms) tmp++;
			if (uarmh && uarmh->otyp == FEDORA) tmp++;
			if (uarm && uarm->otyp == LEATHER_JACKET) tmp++;
		    }
		}
	    } else {
		/* we know we're dealing with a weapon or weptool handled
		   by WEAPON_SKILLS once ammo objects have been excluded */
		if (P_RESTRICTED(weapon_type(obj)))
		    tmp -= 5;
		tmp += weapon_hit_bonus(obj);
		if (otyp == BOOMERANG)		/* arbitrary */
		    tmp += 4;
		else if (throwing_weapon(obj))	/* meant to be thrown */
		    tmp += 2;
		else				/* not meant to be thrown */
		    if (!guaranteed_hit) tmp = 1;
	    }
	    /* guarantee 5% to hit */
	    if (tmp < 1) tmp = 1;
#ifdef WIZARD
	    if (wizard) pline("[%d]",tmp);
#endif /*WIZARD*/

	    if (tmp >= rnd(20)) {
		bhitflag = 1;
		if (missile_hits_shield(mon, obj))
		    return 0;
		if (hmon(mon,obj,1)) {	/* mon still alive */
		    cutworm(mon, bhitpos.x, bhitpos.y, obj);
		}
		exercise(A_DEX, TRUE);
		/* projectiles other than magic stones
		   sometimes disappear when thrown */
		if (obj && is_consumable(obj) && !objects[otyp].oc_magic) {
		    /* we were breaking 2/3 of everything unconditionally.
		     * we still don't want anything to survive unconditionally,
		     * but we need ammo to stay around longer on average.
		     */
		    int broken, chance;
		    chance = 3 + greatest_erosion(obj) - obj->spe;
		    if (chance > 1)
			broken = rn2(chance);
		    else
			broken = !rn2(4);
		    if (obj->blessed && !rnl(4))
			broken = 0;

		    if (broken) {
			if (*u.ushops)
			    check_shop_obj(obj, bhitpos.x,bhitpos.y, TRUE);
			obfree(obj, (struct obj *)0);
			return 1;
		    }
		}
		passive_obj(mon, obj, (struct attack *)0);
		/* identify your weapon */
		if (obj) {
		    struct obj *wep;
		    wep = obj;
		    if (is_ammo(obj) && ammo_and_launcher(obj, uwep)) {
			if (!uwep->known && !obj->known && rn2(2)) wep = uwep;
			else if (obj->known) wep = uwep;
		    }
		    if (!wep->known && wepidentify_byhit(wep)) {
			wep->known = 1;
			You(E_J("find the quality of your weapon.",
				"自分の武器の品質を見定めた。"));
			prinv(NULL, wep, 0);
			/* if hero shot a missile and knew its quality,
			   identify the original group of the missile */
			if (wep->oclass == WEAPON_CLASS &&
			    objects[wep->otyp].oc_merge && wep->corpsenm) {
			    struct obj *otmp;
			    for (otmp = invent; otmp; otmp = otmp->nobj) {
				if (otmp->o_id == wep->corpsenm)
				    otmp->known = 1;
			    }
			    /* identify the already-thrown weapon... */
			    for (otmp = fobj; otmp; otmp = otmp->nobj) {
				if (wep->otyp == otmp->otyp && wep->invlet == otmp->invlet &&
#ifdef PICKUP_THROWN
				    otmp->othrown &&
#endif /*PICKUP_THROWN*/
				    wep->spe == otmp->spe && wep->blessed == otmp->blessed)
				    otmp->known = 1;
			    }
			}
		    }
		}
	    } else {
		tmiss(obj, mon);
	    }

	} else if (otyp == HEAVY_IRON_BALL) {
	    exercise(A_STR, TRUE);
	    if (tmp >= rnd(20)) {
		int was_swallowed = guaranteed_hit;

		exercise(A_DEX, TRUE);
		if (missile_hits_shield(mon, obj))
		    return 0;
		if (!hmon(mon,obj,1)) {		/* mon killed */
		    if (was_swallowed && !u.uswallow && obj == uball)
			return 1;	/* already did placebc() */
		}
	    } else {
		tmiss(obj, mon);
	    }

	} else if (otyp == BOULDER) {
	    exercise(A_STR, TRUE);
	    if (tmp >= rnd(20)) {
		exercise(A_DEX, TRUE);
		(void) hmon(mon,obj,1);
	    } else {
		tmiss(obj, mon);
	    }

	} else if ((otyp == EGG || otyp == CREAM_PIE ||
		    otyp == BLINDING_VENOM || otyp == ACID_VENOM) &&
		(guaranteed_hit || ACURR(A_DEX) > rnd(25))) {
	    if (missile_hits_shield(mon, obj)) {
		obfree(obj, (struct obj *)0);
		return 1;
	    }
	    (void) hmon(mon, obj, 1);
	    return 1;	/* hmon used it up */

	} else if (obj->oclass == POTION_CLASS &&
		(guaranteed_hit || ACURR(A_DEX) > rnd(18/*25*/))) {
	    if (missile_hits_shield(mon, obj)) {
		obfree(obj, (struct obj *)0);
		return 1;
	    }
	    potionhit(mon, obj, TRUE);
	    return 1;

	} else if ((befriend_with_obj(mon->data, obj)
#ifdef MONSTEED
		   && !is_mridden(mon) /* msteed cannot be tamed easily */
#endif
		   ) || (mon->mtame && dogfood(mon, obj) <= ACCFOOD)) {
	    if (tamedog(mon, obj))
		return 1;           	/* obj is gone */
	    else {
		/* not tmiss(), which angers non-tame monsters */
		miss(xname(obj), mon);
		mon->msleeping = 0;
		mon->mstrategy &= ~STRAT_WAITMASK;
	    }
	} else if (guaranteed_hit) {
	    /* this assumes that guaranteed_hit is due to swallowing */
	    wakeup(mon);
	    if (obj->otyp == CORPSE && touch_petrifies(&mons[obj->corpsenm])) {
		if (is_animal(u.ustuck->data)) {
			minstapetrify(u.ustuck, TRUE);
			/* Don't leave a cockatrice corpse available in a statue */
			if (!u.uswallow) {
				delobj(obj);
				return 1;
			}
	    	}
	    }
#ifndef JP
	    pline("%s into %s %s.",
		Tobjnam(obj, "vanish"), s_suffix(mon_nam(mon)),
		is_animal(u.ustuck->data) ? "entrails" : "currents");
#else
	    pline("%sは%sの%sの中に消えていった。",
		xname(obj), mon_nam(mon),
		is_animal(u.ustuck->data) ? "消化管" : "流れ");
#endif /*JP*/
	} else {
	    tmiss(obj, mon);
	}

	return 0;
}

STATIC_OVL int
gem_accept(mon, obj)
register struct monst *mon;
register struct obj *obj;
{
	char buf[BUFSZ];
	boolean is_buddy = sgn(mon->data->maligntyp) == sgn(u.ualign.type);
	boolean is_gem = get_material(obj) == GEMSTONE;
	int ret = 0;
#ifndef JP
	static NEARDATA const char nogood[] = " is not interested in your junk.";
	static NEARDATA const char acceptgift[] = " accepts your gift.";
	static NEARDATA const char maybeluck[] = " hesitatingly";
	static NEARDATA const char noluck[] = " graciously";
	static NEARDATA const char addluck[] = " gratefully";
#else
	static NEARDATA const char nogood[] = "あなたのガラクタに興味を示さなかった。";
	static NEARDATA const char acceptgift[] = "あなたの贈り物を受け取った。";
	static NEARDATA const char maybeluck[] = "ためらいながらも";
	static NEARDATA const char noluck[] = "こころよく";
	static NEARDATA const char addluck[] = "ありがたく";
#endif /*JP*/

#ifndef JP
	Strcpy(buf,Monnam(mon));
#else
	Sprintf(buf, "%sは", mon_nam(mon));
#endif /*JP*/
	mon->mpeaceful = 1;
	mon->mavenge = 0;

	/* object properly identified */
	if(obj->dknown && objects[obj->otyp].oc_name_known) {
		if(is_gem) {
			if(is_buddy) {
				Strcat(buf,addluck);
				change_luck(5);
			} else {
				Strcat(buf,maybeluck);
				change_luck(rn2(7)-3);
			}
		} else {
			Strcat(buf,nogood);
			goto nopick;
		}
	/* making guesses */
	} else if(obj->onamelth || objects[obj->otyp].oc_uname) {
		if(is_gem) {
			if(is_buddy) {
				Strcat(buf,addluck);
				change_luck(2);
			} else {
				Strcat(buf,maybeluck);
				change_luck(rn2(3)-1);
			}
		} else {
			Strcat(buf,nogood);
			goto nopick;
		}
	/* value completely unknown to @ */
	} else {
		if(is_gem) {
			if(is_buddy) {
				Strcat(buf,addluck);
				change_luck(1);
			} else {
				Strcat(buf,maybeluck);
				change_luck(rn2(3)-1);
			}
		} else {
			Strcat(buf,noluck);
		}
	}
	Strcat(buf,acceptgift);
	if(*u.ushops) check_shop_obj(obj, mon->mx, mon->my, TRUE);
	(void) mpickobj(mon, obj);	/* may merge and free obj */
	ret = 1;

nopick:
	if(!Blind) pline("%s", buf);
	if (!tele_restrict(mon)) (void) rloc(mon, FALSE);
	return(ret);
}

/*
 * Comments about the restructuring of the old breaks() routine.
 *
 * There are now three distinct phases to object breaking:
 *     breaktest() - which makes the check/decision about whether the
 *                   object is going to break.
 *     breakmsg()  - which outputs a message about the breakage,
 *                   appropriate for that particular object. Should
 *                   only be called after a positve breaktest().
 *                   on the object and, if it going to be called,
 *                   it must be called before calling breakobj().
 *                   Calling breakmsg() is optional.
 *     breakobj()  - which actually does the breakage and the side-effects
 *                   of breaking that particular object. This should
 *                   only be called after a positive breaktest() on the
 *                   object.
 *
 * Each of the above routines is currently static to this source module.
 * There are two routines callable from outside this source module which
 * perform the routines above in the correct sequence.
 *
 *   hero_breaks() - called when an object is to be broken as a result
 *                   of something that the hero has done. (throwing it,
 *                   kicking it, etc.)
 *   breaks()      - called when an object is to be broken for some
 *                   reason other than the hero doing something to it.
 */

/*
 * The hero causes breakage of an object (throwing, dropping it, etc.)
 * Return 0 if the object didn't break, 1 if the object broke.
 */
int
hero_breaks(obj, x, y, from_invent)
struct obj *obj;
xchar x, y;		/* object location (ox, oy may not be right) */
boolean from_invent;	/* thrown or dropped by player; maybe on shop bill */
{
	boolean in_view = !Blind;
	if (!breaktest(obj)) return 0;
	breakmsg(obj, in_view);
	breakobj(obj, x, y, TRUE, from_invent);
	return 1;
}

/*
 * The object is going to break for a reason other than the hero doing
 * something to it.
 * Return 0 if the object doesn't break, 1 if the object broke.
 */
int
breaks(obj, x, y)
struct obj *obj;
xchar x, y;		/* object location (ox, oy may not be right) */
{
	boolean in_view = Blind ? FALSE : cansee(x, y);

	if (!breaktest(obj)) return 0;
	breakmsg(obj, in_view);
	breakobj(obj, x, y, FALSE, FALSE);
	return 1;
}

/*
 * Unconditionally break an object. Assumes all resistance checks
 * and break messages have been delivered prior to getting here.
 */
STATIC_OVL void
breakobj(obj, x, y, hero_caused, from_invent)
struct obj *obj;
xchar x, y;		/* object location (ox, oy may not be right) */
boolean hero_caused;	/* is this the hero's fault? */
boolean from_invent;
{
	switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
		case MIRROR:
			if (hero_caused)
			    change_luck(-2);
			break;
		case POT_WATER:		/* really, all potions */
			if (obj->otyp == POT_OIL && obj->lamplit) {
			    splatter_burning_oil(x,y);
			} else if (distu(x,y) <= 2) {
			    if (!breathless(youmonst.data) || haseyes(youmonst.data)) {
				if (obj->otyp != POT_WATER) {
					if (!breathless(youmonst.data))
			    		     /* [what about "familiar odor" when known?] */
					    You(E_J("smell a peculiar odor...",
						    "独特の匂いを嗅いだ…。"));
					else {
					    int numeyes = eyecount(youmonst.data);
#ifndef JP
					    Your("%s water%s.",
						 (numeyes == 1) ? body_part(EYE) :
							makeplural(body_part(EYE)),
						 (numeyes == 1) ? "s" : "");
#else
					    Your("%sがしばたたいた。",
						 body_part(EYE));
#endif /*JP*/
					}
				}
				potionbreathe(obj);
			    }
			}
			/* monster breathing isn't handled... [yet?] */
			break;
		case EGG:
			/* breaking your own eggs is bad luck */
			if (hero_caused && obj->spe && obj->corpsenm >= LOW_PM)
			    change_luck((schar) -min(obj->quan, 5L));
			break;
	}
	if (hero_caused) {
	    if (from_invent) {
		if (*u.ushops)
			check_shop_obj(obj, x, y, TRUE);
	    } else if (!obj->no_charge && costly_spot(x, y)) {
		/* it is assumed that the obj is a floor-object */
		char *o_shop = in_rooms(x, y, SHOPBASE);
		struct monst *shkp = shop_keeper(*o_shop);

		if (shkp) {		/* (implies *o_shop != '\0') */
		    static NEARDATA long lastmovetime = 0L;
		    static NEARDATA boolean peaceful_shk = FALSE;
		    /*  We want to base shk actions on her peacefulness
			at start of this turn, so that "simultaneous"
			multiple breakage isn't drastically worse than
			single breakage.  (ought to be done via ESHK)  */
		    if (moves != lastmovetime)
			peaceful_shk = shkp->mpeaceful;
		    if (stolen_value(obj, x, y, peaceful_shk, FALSE) > 0L &&
			(*o_shop != u.ushops[0] || !inside_shop(u.ux, u.uy)) &&
			moves != lastmovetime) make_angry_shk(shkp, x, y);
		    lastmovetime = moves;
		}
	    }
	}
	delobj(obj);
}

/*
 * Check to see if obj is going to break, but don't actually break it.
 * Return 0 if the object isn't going to break, 1 if it is.
 */
boolean
breaktest(obj)
struct obj *obj;
{
	if (obj_resists(obj, 1, 99)) return 0;
	if (get_material(obj) == GLASS && !obj->oartifact &&
		obj->oclass != GEM_CLASS)
	    return 1;
	switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
#ifdef TOURIST
		case EXPENSIVE_CAMERA:
#endif
		case POT_WATER:		/* really, all potions */
		case EGG:
		case CREAM_PIE:
		case MELON:
		case ACID_VENOM:
		case BLINDING_VENOM:
			return 1;
		default:
			return 0;
	}
}

STATIC_OVL void
breakmsg(obj, in_view)
struct obj *obj;
boolean in_view;
{
	const char *to_pieces;

	to_pieces = "";
	switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
		default: /* glass or crystal wand */
		    if (obj->oclass != WAND_CLASS)
			impossible("breaking odd object?");
		case PLATE_MAIL:
			if (get_material(obj) != GLASS) break;	/* crystal plate mail? */
#ifndef MAGIC_GLASSES
		case LENSES:
#else
		case GLASSES_OF_MAGIC_READING:
		case GLASSES_OF_GAZE_PROTECTION:
		case GLASSES_OF_INFRAVISION:
		case GLASSES_VERSUS_FLASH:
		case GLASSES_OF_SEE_INVISIBLE:
		case GLASSES_OF_PHANTASMAGORIA:
#endif /*MAGIC_GLASSES*/
		case MIRROR:
		case CRYSTAL_BALL:
		case ORB_OF_DESTRUCTION:
		case ORB_OF_MAINTENANCE:
		case ORB_OF_CHARGING:
#ifdef TOURIST
		case EXPENSIVE_CAMERA:
#endif
			to_pieces = E_J(" into a thousand pieces","こなごなに");
			/*FALLTHRU*/
		case POT_WATER:		/* really, all potions */
			if (!in_view)
			    You_hear(E_J("%s shatter!","!%sが割れる音を"), something);
			else
#ifndef JP
			    pline("%s shatter%s%s!", Doname2(obj),
				(obj->quan==1) ? "s" : "", to_pieces);
#else
			    pline("%sは%s割れた！",
				doname(obj), to_pieces);
#endif /*JP*/
			break;
		case EGG:
		case MELON:
			pline(E_J("Splat!","ビチャ！"));
			break;
		case CREAM_PIE:
			if (in_view) pline(E_J("What a mess!","なんて有様だ！"));
			break;
		case ACID_VENOM:
		case BLINDING_VENOM:
			pline(E_J("Splash!","ビシャッ！"));
			break;
	}
}

STATIC_OVL int
throw_gold(obj)
struct obj *obj;
{
	int range, odx, ody;
#ifndef GOLDOBJ
	long zorks = obj->quan;
#endif
	register struct monst *mon;

	if(!u.dx && !u.dy && !u.dz) {
#ifndef GOLDOBJ
		u.ugold += obj->quan;
		flags.botl = 1;
		dealloc_obj(obj);
#endif
		You(E_J("cannot throw gold at yourself.",
			"自分めがけて金貨を投げることはできない。"));
		return(0);
	}
#ifdef GOLDOBJ
        freeinv(obj);
#endif
	if(u.uswallow) {
		pline(is_animal(u.ustuck->data) ?
			E_J("%s in the %s's entrails.", "%sの消化管の中に消えていった。") :
			E_J("%s into %s.", "%sの中に消えていった。"),
#ifndef GOLDOBJ
			E_J("The gold disappears","金貨は"), mon_nam(u.ustuck));
		u.ustuck->mgold += zorks;
		dealloc_obj(obj);
#else
			E_J("The money disappears","硬貨は"), mon_nam(u.ustuck));
		add_to_minv(u.ustuck, obj);
#endif
		return(1);
	}

	if(u.dz) {
		if (u.dz < 0 && !Is_airlevel(&u.uz) &&
					!Underwater && !Is_waterlevel(&u.uz)) {
	pline_The(E_J("gold hits the %s, then falls back on top of your %s.",
		      "金貨は%sに当たると、あなたの%sめがけて降ってきた。"),
		    ceiling(u.ux,u.uy), body_part(HEAD));
		    /* some self damage? */
		    if(uarmh) pline(E_J("Fortunately, you are wearing a helmet!",
				        "幸い、あなたは兜をかぶっていた！"));
		}
		bhitpos.x = u.ux;
		bhitpos.y = u.uy;
	} else {
		/* consistent with range for normal objects */
		range = (int)((ACURRSTR)/2 - obj->owt/40);

		/* see if the gold has a place to move into */
		odx = u.ux + u.dx;
		ody = u.uy + u.dy;
		if(!ZAP_POS(levl[odx][ody].typ) || closed_door(odx, ody)) {
			bhitpos.x = u.ux;
			bhitpos.y = u.uy;
		} else {
			mon = bhit(u.dx, u.dy, range, THROWN_WEAPON,
				   (int FDECL((*),(MONST_P,OBJ_P)))0,
				   (int FDECL((*),(OBJ_P,OBJ_P)))0,
				   obj);
			if(mon) {
			    if (ghitm(mon, obj))	/* was it caught? */
				return 1;
			} else {
			    if(ship_object(obj, bhitpos.x, bhitpos.y, FALSE))
				return 1;
			}
		}
	}

	if(flooreffects(obj,bhitpos.x,bhitpos.y,E_J("fall","落ちた"))) return(1);
	if(u.dz > 0)
		pline_The(E_J("gold hits the %s.","金貨は%sに当たった。"),
			surface(bhitpos.x,bhitpos.y));
	place_object(obj,bhitpos.x,bhitpos.y);
	if(*u.ushops) sellobj(obj, bhitpos.x, bhitpos.y);
	stackobj(obj);
	newsym(bhitpos.x,bhitpos.y);
	return(1);
}

int
fire_gun(otmp, quick)
struct obj *otmp;
boolean quick;	/* u.dx/u.dy has been already set */
{
	struct obj *bullet;
	struct monst *mtmp;
	boolean obj_gone;
	boolean impaired = (Confusion || Stunned || Fumbling);

	if (Underwater) {
		Your(E_J("gun isn't an underwater gun.",
			 "銃は水中銃ではない。"));
		return(0);
	}

	if (!quick) {
	    if (!getdir_or_pos(0, GETPOS_MONTGT, (char *)0, E_J("shoot","撃つ")))
		return 0;
	    if ((mtmp = m_at(u.ux+u.dx, u.uy+u.dy)) != 0 &&
		(mtmp->mx == (u.ux+u.dx) && mtmp->my == (u.uy+u.dy)))
		u.ulasttgt = mtmp;
	}

	/* reload! */
	if (!otmp->cobj) {
		pline(E_J("Click!","カチッ！"));
		return(1);
	}

	/* ??? */
	if (otmp->cobj->otyp != BULLET) {
	    impossible("Odd thing(%s) in the gun?", doname(otmp->cobj));
	    otmp->cobj = otmp->cobj->nobj;
	    return(0);
	}

	/* get one bullet from the gun */
	bullet = otmp->cobj;
	if (bullet->quan > 1) {
		bullet = splitobj(bullet, 1);
	}
	obj_extract_self(bullet);
	bullet->oshot = 1;

	wake_nearby();	/* gunshot */

	if ((otmp->cursed || bullet->cursed) && (u.dx || u.dy) && !rn2(7))
		impaired = TRUE;
	if (impaired) {
		u.dx = rn2(9)-4;
		u.dy = rn2(9)-4;
		if (!u.dx && !u.dy) u.dz = 1;
	}

	if (u.uswallow) {
#ifndef JP
		You("fire %s!", yname(otmp));
#else
		You("%sを撃った！", xname(otmp));
#endif /*JP*/
		mtmp = u.ustuck;
	} else {
		if (u.dz) {
			You(E_J("fire a warning shot!","威嚇射撃をした！"));
			goto xit;
		}
#ifndef JP
		You("%sfire %s!", (impaired ? "mis" : ""), yname(otmp));
#else
		You("%sを%s！", xname(otmp), (impaired ? "誤射した" : "撃った"));
#endif /*JP*/
		mtmp = bhit(u.dx, u.dy, 30, GUN_BULLET, 0, 0, bullet);
		if(Is_airlevel(&u.uz) || Levitation)
			hurtle(-u.dx, -u.dy, 1, TRUE);
	}
	if (mtmp) {
		obj_gone = thitmonst(mtmp, bullet);
		if (obj_gone) bullet = (struct obj *)0;
	} else {
		uchar typ = levl[bhitpos.x][bhitpos.y].typ;
		if (IS_SINK(typ)) {
			pline(E_J("Suddenly the bullet turns down and hits the sink!",
				  "突然、弾丸が下にそれ、流し台に命中した！"));
		} else {
			/*pline_The("bullet misses.");*/
		}
	}
xit:
	if (bullet) obfree(bullet, (struct obj *)0);
	return(1);
}

/*dothrow.c*/
