/*	SCCS Id: @(#)steed.c	3.4	2003/01/10	*/
/* Copyright (c) Kevin Hugo, 1998-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"


#ifdef STEED

/* Monsters that might be ridden */
static NEARDATA const char steeds[] = {
	S_QUADRUPED, S_UNICORN, S_ANGEL, S_CENTAUR, S_DRAGON, S_JABBERWOCK, S_DOG, '\0'
};

STATIC_DCL boolean FDECL(landing_spot, (coord *, int, int));

/* caller has decided that hero can't reach something while mounted */
void
rider_cant_reach()
{
     You(E_J("aren't skilled enough to reach from %s.",
	     "、%sから床まで届くほど乗馬に習熟していない。"), y_monnam(u.usteed));
}

/*** Putting the saddle on ***/

/* Can this monster wear a saddle? */
boolean
can_saddle(mtmp)
	struct monst *mtmp;
{
	struct permonst *ptr = mtmp->data;

	return (index(steeds, ptr->mlet) && (ptr->msize >= MZ_MEDIUM) &&
			(!humanoid(ptr) || ptr->mlet == S_CENTAUR) &&
			!amorphous(ptr) && !noncorporeal(ptr) &&
			(ptr->mlet != S_DOG || Race_if(PM_ORC)) &&	/* orcs can ride on wolves */
			!is_whirly(ptr) && !unsolid(ptr));
}


int
use_saddle(otmp)
	struct obj *otmp;
{
	struct monst *mtmp;
	struct permonst *ptr;
	int chance, skill;
	const char *s;


	/* Can you use it? */
	if (nohands(youmonst.data)) {
#ifndef JP
		You("have no hands!");	/* not `body_part(HAND)' */
#else
		pline("あなたには手がない！");
#endif /*JP*/
		return 0;
	} else if (!freehand()) {
		You(E_J("have no free %s.","%sが空いていない。"), body_part(HAND));
		return 0;
	}

	/* Select an animal */
	if (u.uswallow || Underwater || !getdir((char *)0)) {
	    pline(Never_mind);
	    return 0;
	}
	if (!u.dx && !u.dy) {
	    pline(E_J("Saddle yourself?  Very funny...",
		      "自分に鞍をつける？ それは面白い…。"));
	    return 0;
	}
	if (!isok(u.ux+u.dx, u.uy+u.dy) ||
			!(mtmp = m_at(u.ux+u.dx, u.uy+u.dy)) ||
			!canspotmon(mtmp)) {
	    pline(E_J("I see nobody there.","そこには何もいない。"));
	    return 1;
	}

	/* Is this a valid monster? */
	if (mtmp->misc_worn_check & W_SADDLE ||
			which_armor(mtmp, W_SADDLE)) {
	    pline(E_J("%s doesn't need another one.",
		      "%sはすでに鞍をつけている。"), Monnam(mtmp));
	    return 1;
	}
	ptr = mtmp->data;
	if (touch_petrifies(ptr) && !uarmg && !Stone_resistance) {
	    char kbuf[BUFSZ];

	    You(E_J("touch %s.","%sに触った。"), mon_nam(mtmp));
 	    if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
#ifndef JP
		Sprintf(kbuf, "attempting to saddle %s", an(mtmp->data->mname));
#else
		Sprintf(kbuf, "%sに鞍を載せようとして", JMONNAM(monsndx(mtmp->data)));
#endif /*JP*/
		instapetrify(kbuf);
 	    }
	}
	if (ptr == &mons[PM_INCUBUS] || ptr == &mons[PM_SUCCUBUS]) {
	    pline(E_J("Shame on you!","恥を知れ！"));
	    exercise(A_WIS, FALSE);
	    return 1;
	}
	if (mtmp->isminion || mtmp->isshk || mtmp->ispriest ||
			mtmp->isgd || mtmp->iswiz) {
	    pline(E_J("I think %s would mind.","%sは怒ると思いますよ。"), mon_nam(mtmp));
	    return 1;
	}
	if (!can_saddle(mtmp)) {
		You_cant(E_J("saddle such a creature.","この種の怪物には鞍をつけられない。"));
		return 1;
	}

	/* Calculate your chance */
	skill = P_SKILL(P_RIDING);
	chance = ACURR(A_DEX) + ACURR(A_CHA)/2 + 2*mtmp->mtame;
	chance += u.ulevel * (mtmp->mtame ? 20 : 5);
	if (!mtmp->mtame) chance -= 10*mtmp->m_lev;
	if (Role_if(PM_KNIGHT))
	    chance += 20;
	chance += (skill < P_BASIC  ) ? -(P_BASIC - skill)/2        :		/* -25 -   0 */
		  (skill < P_SKILLED) ?  (skill - P_BASIC)/2        :		/*   0 - +12 */
		  (skill < P_EXPERT ) ?  (skill - P_SKILLED + 30)/2 : +30;	/* +15 - +27, +30 */
	if (Confusion || Fumbling || Glib)
	    chance -= 20;
	else if (uarmg &&
		(s = OBJ_DESCR(objects[uarmg->otyp])) != (char *)0 &&
		!strncmp(s, "riding ", 7))
	    /* Bonus for wearing "riding" (but not fumbling) gloves */
	    chance += 10;
	else if (uarmf &&
		(s = OBJ_DESCR(objects[uarmf->otyp])) != (char *)0 &&
		!strncmp(s, "riding ", 7))
	    /* ... or for "riding boots" */
	    chance += 10;
	if (otmp->cursed)
	    chance -= 50;

	/* Make the attempt */
	if (rn2(100) < chance) {
	    You(E_J("put the saddle on %s.",
		    "%sに鞍を取り付けた。"), mon_nam(mtmp));
	    if (otmp->owornmask) remove_worn_item(otmp, FALSE);
	    freeinv(otmp);
	    /* mpickobj may free otmp it if merges, but we have already
	       checked for a saddle above, so no merger should happen */
	    (void) mpickobj(mtmp, otmp);
	    mtmp->misc_worn_check |= W_SADDLE;
	    otmp->owornmask = W_SADDLE;
	    otmp->leashmon = mtmp->m_id;
	    update_mon_intrinsics(mtmp, otmp, TRUE, FALSE);
	} else
	    pline(E_J("%s resists!","%sは抵抗した！"), Monnam(mtmp));
	return 1;
}


/*** Riding the monster ***/

/* Can we ride this monster?  Caller should also check can_saddle() */
boolean
can_ride(mtmp)
	struct monst *mtmp;
{
	return (mtmp->mtame && humanoid(youmonst.data) &&
			!verysmall(youmonst.data) && !bigmonst(youmonst.data) &&
#ifdef MONSTEED
			!is_mriding(mtmp) && !is_mridden(mtmp) &&
#endif
			(!Underwater || is_swimmer(mtmp->data)));
}


int
doride()
{
	boolean forcemount = FALSE;

	if (u.usteed)
	    dismount_steed(DISMOUNT_BYCHOICE);
	else if (getdir((char *)0) && isok(u.ux+u.dx, u.uy+u.dy)) {
#ifdef WIZARD
	if (wizard && yn("Force the mount to succeed?") == 'y')
		forcemount = TRUE;
#endif
	    return (mount_steed(m_at(u.ux+u.dx, u.uy+u.dy), forcemount));
	} else
	    return 0;
	return 1;
}


/* Start riding, with the given monster */
boolean
mount_steed(mtmp, force)
	struct monst *mtmp;	/* The animal */
	boolean force;		/* Quietly force this animal */
{
	struct obj *otmp;
	char buf[BUFSZ];
	struct permonst *ptr;

	/* Sanity checks */
	if (u.usteed) {
	    You(E_J("are already riding %s.",
		    "すでに%sに乗っている。"), mon_nam(u.usteed));
	    return (FALSE);
	}

	/* Is the player in the right form? */
	if (Hallucination && !force) {
	    pline(E_J("Maybe you should find a designated driver.",
		      "おそらくあなたには運転代行業者が必要だ。"));
		/* designated driver は飲み会などで飲まずに運転する係の人のこと */
	    return (FALSE);
	}
	/* While riding Wounded_legs refers to the steed's,
	 * not the hero's legs.
	 * That opens up a potential abuse where the player
	 * can mount a steed, then dismount immediately to
	 * heal leg damage, because leg damage is always
	 * healed upon dismount (Wounded_legs context switch).
	 * By preventing a hero with Wounded_legs from
	 * mounting a steed, the potential for abuse is
	 * minimized, if not eliminated altogether.
	 */
	if (Wounded_legs) {
#ifndef JP
	    Your("%s are in no shape for riding.", makeplural(body_part(LEG)));
#else
	    Your("%sは乗馬できるような状態にない。", body_part(LEG));
#endif /*JP*/
#ifdef WIZARD
	    if (force && wizard && yn("Heal your legs?") == 'y')
		HWounded_legs = EWounded_legs = 0;
	    else
#endif
	    return (FALSE);
	}

	if (Upolyd && (!humanoid(youmonst.data) || verysmall(youmonst.data) ||
			bigmonst(youmonst.data) || slithy(youmonst.data))) {
#ifndef JP
	    You("won't fit on a saddle.");
#else
	    Your("身体では鞍にうまく跨れない。");
#endif /*JP*/
	    return (FALSE);
	}
	if(!force && (near_capacity() > SLT_ENCUMBER)) {
#ifndef JP
	    You_cant("do that while carrying so much stuff.");
#else
	    pline("そんなに多くの荷物を抱えていては、騎乗することはできない。");
#endif /*JP*/
	    return (FALSE);
	}

	/* Can the player reach and see the monster? */
	if (!mtmp || (!force && ((Blind && !Blind_telepat) ||
		mtmp->mundetected ||
		mtmp->m_ap_type == M_AP_FURNITURE ||
		mtmp->m_ap_type == M_AP_OBJECT))) {
	    pline("I see nobody there.");
	    return (FALSE);
	}
	if (u.uswallow || u.ustuck || u.utrap || Punished ||
	    !test_move(u.ux, u.uy, mtmp->mx-u.ux, mtmp->my-u.uy, TEST_MOVE)) {
	    if (Punished || !(u.uswallow || u.ustuck || u.utrap))
		You(E_J("are unable to swing your %s over.",
			"跨るために%sを上げることができない。"), body_part(LEG)); 
	    else
		You(E_J("are stuck here for now.","今のところこの場所から動けない。"));
	    return (FALSE);
	}

	/* Is this a valid monster? */
	otmp = which_armor(mtmp, W_SADDLE);
	if (!otmp) {
	    pline(E_J("%s is not saddled.","%sには鞍が乗っていない。"), Monnam(mtmp));
	    return (FALSE);
	}
	ptr = mtmp->data;
	if (touch_petrifies(ptr) && !Stone_resistance) {
	    char kbuf[BUFSZ];

	    You(E_J("touch %s.","%sに触った。"), mon_nam(mtmp));
#ifndef JP
	    Sprintf(kbuf, "attempting to ride %s", an(mtmp->data->mname));
#else
	    Sprintf(kbuf, "%sに乗ろうとして", JMONNAM(monsndx(mtmp->data)));
#endif /*JP*/
	    instapetrify(kbuf);
	}
	if (!mtmp->mtame || mtmp->isminion) {
	    pline(E_J("I think %s would mind.","%sは怒ると思いますよ。"), mon_nam(mtmp));
	    return (FALSE);
	}
	if (mtmp->mtrapped) {
	    struct trap *t = t_at(mtmp->mx, mtmp->my);

#ifndef JP
	    You_cant("mount %s while %s's trapped in %s.",
		     mon_nam(mtmp), mhe(mtmp),
		     an(defsyms[trap_to_defsym(t->ttyp)].explanation));
#else
	    pline("%sは%sに捕らわれているため、乗ることはできない。",
		  mon_nam(mtmp),
		  defsyms[trap_to_defsym(t->ttyp)].explanation);
#endif /*JP*/
	    return (FALSE);
	}

	if (!force && !Role_if(PM_KNIGHT) && !(--mtmp->mtame)) {
	    /* no longer tame */
	    newsym(mtmp->mx, mtmp->my);
	    pline(E_J("%s resists%s!","%sは抵抗し%sた！"), Monnam(mtmp),
		  mtmp->mleashed ? E_J(" and its leash comes off","、引き綱をふりほどい") : "");
	    if (mtmp->mleashed) m_unleash(mtmp, FALSE);
	    return (FALSE);
	}
	if (!force && Underwater && !is_swimmer(ptr)) {
	    You_cant(E_J("ride that creature while under water.",
			 "水中からその怪物に乗ることはできない。"));
	    return (FALSE);
	}
	if (!can_saddle(mtmp) || !can_ride(mtmp)) {
	    You_cant(E_J("ride such a creature.","そのような怪物には乗ることができない。"));
	    return (0);
	}

	/* Is the player impaired? */
	if (!force && !is_floater(ptr) && !is_flyer(ptr) &&
			Levitation && !Lev_at_will) {
	    You(E_J("cannot reach %s.","%sに届かない。"), mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && uarm && is_metallic(uarm) &&
			greatest_erosion(uarm)) {
	    Your(E_J("%s armor is too stiff to be able to mount %s.",
		     "%s鎧が動きを阻害していて、騎乗することができない。"),
			uarm->oeroded ? E_J("rusty","錆び") : E_J("corroded","腐食し"),
			mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && (Confusion || Fumbling || Glib || Wounded_legs ||
		otmp->cursed || (u.ulevel+mtmp->mtame < rnd(MAXULEV/2+5)))) {
	    if (Levitation) {
		pline(E_J("%s slips away from you.",
			  "%sはあなたの手を逃れた。"), Monnam(mtmp));
		return FALSE;
	    }
	    You(E_J("slip while trying to get on %s.",
		    "%sに乗ろうとして、滑り落ちてしまった。"), mon_nam(mtmp));

	    Sprintf(buf, E_J("slipped while mounting %s",
			     "%sに乗ろうとして転落死した"),
		    /* "a saddled mumak" or "a saddled pony called Dobbin" */
		    x_monnam(mtmp, ARTICLE_A, (char *)0,
			SUPPRESS_IT|SUPPRESS_INVISIBLE|SUPPRESS_HALLUCINATION,
			     TRUE));
	    losehp(rn1(5,10), buf, NO_KILLER_PREFIX);
	    return (FALSE);
	}

	/* Success */
	if (!force) {
	    if (Levitation && !is_floater(ptr) && !is_flyer(ptr))
	    	/* Must have Lev_at_will at this point */
	    	pline(E_J("%s magically floats up!",
			  "%sは魔力で浮かび上がった！"), Monnam(mtmp));
	    You(E_J("mount %s.","%sに騎乗した。"), mon_nam(mtmp));
	}
	/* setuwep handles polearms differently when you're mounted */
	if (uwep && is_pole(uwep)) unweapon = FALSE;
	u.usteed = mtmp;
	remove_monster(mtmp->mx, mtmp->my);
	teleds(mtmp->mx, mtmp->my, TRUE);
	return (TRUE);
}


/* You and your steed have moved */
void
exercise_steed()
{
	if (!u.usteed)
		return;

	/* It takes many turns of riding to exercise skill */
	if (u.urideturns++ >= 40/*100*/) {
	    u.urideturns = 0;
	    use_skill(P_RIDING, 1);
	}
	return;
}


/* The player kicks or whips the steed */
void
kick_steed()
{
#ifndef JP
	char He[4];
#endif /*JP*/
	if (!u.usteed)
	    return;

	/* [ALI] Various effects of kicking sleeping/paralyzed steeds */
	if (u.usteed->msleeping || !u.usteed->mcanmove) {
	    /* We assume a message has just been output of the form
	     * "You kick <steed>."
	     */
#ifndef JP
	    Strcpy(He, mhe(u.usteed));
	    *He = highc(*He);
#endif /*JP*/
	    if ((u.usteed->mcanmove || u.usteed->mfrozen) && !rn2(2)) {
		if (u.usteed->mcanmove)
		    u.usteed->msleeping = 0;
		else if (u.usteed->mfrozen > 2)
		    u.usteed->mfrozen -= 2;
		else {
		    u.usteed->mfrozen = 0;
		    u.usteed->mcanmove = 1;
		}
		if (u.usteed->msleeping || !u.usteed->mcanmove)
#ifndef JP
		    pline("%s stirs.", He);
#else
		    pline("%sは身じろぎした。", mon_nam(u.usteed));
#endif /*JP*/
		else
#ifndef JP
		    pline("%s rouses %sself!", He, mhim(u.usteed));
#else
		    pline("%sは自らを奮い起こした！", mon_nam(u.usteed));
#endif /*JP*/
	    } else
#ifndef JP
		pline("%s does not respond.", He);
#else
		pline("%sは何の反応もしない。", mon_nam(u.usteed));
#endif /*JP*/
	    return;
	}

	/* Make the steed less tame and check if it resists */
	if (u.usteed->mtame) u.usteed->mtame--;
	if (!u.usteed->mtame && u.usteed->mleashed) m_unleash(u.usteed, TRUE);
	if (!u.usteed->mtame || (u.ulevel+u.usteed->mtame < rnd(MAXULEV/2+5))) {
	    newsym(u.usteed->mx, u.usteed->my);
	    dismount_steed(DISMOUNT_THROWN);
	    return;
	}

	pline(E_J("%s gallops!","%sは駆け出した！"), Monnam(u.usteed));
	u.ugallop += rn1(20, 30);
	return;
}

/*
 * Try to find a dismount point adjacent to the steed's location.
 * If all else fails, try enexto().  Use enexto() as a last resort because
 * enexto() chooses its point randomly, possibly even outside the
 * room's walls, which is not what we want.
 * Adapted from mail daemon code.
 */
STATIC_OVL boolean
landing_spot(spot, reason, forceit)
coord *spot;	/* landing position (we fill it in) */
int reason;
int forceit;
{
    int i = 0, x, y, distance, min_distance = -1;
    boolean found = FALSE;
    struct trap *t;

    /* avoid known traps (i == 0) and boulders, but allow them as a backup */
    if (reason != DISMOUNT_BYCHOICE || Stunned || Confusion || Fumbling) i = 1;
    for (; !found && i < 2; ++i) {
	for (x = u.ux-1; x <= u.ux+1; x++)
	    for (y = u.uy-1; y <= u.uy+1; y++) {
		if (!isok(x, y) || (x == u.ux && y == u.uy)) continue;

		if (ACCESSIBLE(levl[x][y].typ) &&
			    !MON_AT(x,y) && !closed_door(x,y)) {
		    distance = distu(x,y);
		    if (min_distance < 0 || distance < min_distance ||
			    (distance == min_distance && rn2(2))) {
			if (i > 0 || (((t = t_at(x, y)) == 0 || !t->tseen) &&
				      (!sobj_at(BOULDER, x, y) ||
				       throws_rocks(youmonst.data)))) {
			    spot->x = x;
			    spot->y = y;
			    min_distance = distance;
			    found = TRUE;
			}
		    }
		}
	    }
    }

    /* If we didn't find a good spot and forceit is on, try enexto(). */
    if (forceit && min_distance < 0 &&
		!enexto(spot, u.ux, u.uy, youmonst.data))
	return FALSE;

    return found;
}

/* Stop riding the current steed */
void
dismount_steed(reason)
	int reason;		/* Player was thrown off etc. */
{
	struct monst *mtmp;
	struct obj *otmp;
	coord cc;
	const char *verb = E_J("fall","落下した");
	boolean repair_leg_damage = TRUE;
	unsigned save_utrap = u.utrap;
	boolean have_spot = landing_spot(&cc,reason,0);
	
	mtmp = u.usteed;		/* make a copy of steed pointer */
	/* Sanity check */
	if (!mtmp)		/* Just return silently */
	    return;

	/* Check the reason for dismounting */
	otmp = which_armor(mtmp, W_SADDLE);
	switch (reason) {
	    case DISMOUNT_THROWN:
		verb = E_J("are thrown","投げ出された");
	    case DISMOUNT_FELL:
#ifndef JP
		You("%s off of %s!", verb, mon_nam(mtmp));
#else
		You("%sから%s！", mon_nam(mtmp), verb);
#endif /*JP*/
		if (!have_spot) have_spot = landing_spot(&cc,reason,1);
		losehp(rn1(10,10), E_J("riding accident","乗馬中の事故で"), KILLED_BY_AN);
		set_wounded_legs(BOTH_SIDES, (int)HWounded_legs + rn1(5,5));
		repair_leg_damage = FALSE;
		break;
	    case DISMOUNT_POLY:
		You(E_J("can no longer ride %s.",
			"もはや%sに乗っていられない。"), mon_nam(u.usteed));
		if (!have_spot) have_spot = landing_spot(&cc,reason,1);
		break;
	    case DISMOUNT_ENGULFED:
		/* caller displays message */
		break;
	    case DISMOUNT_BONES:
		/* hero has just died... */
		break;
	    case DISMOUNT_GENERIC:
		/* no messages, just make it so */
		break;
	    case DISMOUNT_BYCHOICE:
	    default:
		if (otmp && otmp->cursed) {
#ifndef JP
		    You("can't.  The saddle %s cursed.",
			otmp->bknown ? "is" : "seems to be");
#else
		    You("降りられない。鞍が呪われている%s。",
			otmp->bknown ? "" : "ようだ");
#endif /*JP*/
		    otmp->bknown = TRUE;
		    return;
		}
		if (!have_spot) {
		    You(E_J("can't. There isn't anywhere for you to stand.",
			    "降りられない。あなたが立てる場所がない。"));
		    return;
		}
		if (!mtmp->mnamelth) {
#ifndef JP
			pline("You've been through the dungeon on %s with no name.",
				an(mtmp->data->mname));
#else
			You("名無しの%sとともに迷宮を旅してきた。",
				JMONNAM(monsndx(mtmp->data)));
#endif /*JP*/
			if (Hallucination)
				pline(E_J("It felt good to get out of the rain.",
					  "雨よけにはちょうど良いと思われていたようだ。"));
		} else
			You(E_J("dismount %s.","%sを降りた。"), mon_nam(mtmp));
	}
	/* While riding these refer to the steed's legs
	 * so after dismounting they refer to the player's
	 * legs once again.
	 */
	if (repair_leg_damage) HWounded_legs = EWounded_legs = 0;

	/* Release the steed and saddle */
	u.usteed = 0;
	u.ugallop = 0L;

	/* Set player and steed's position.  Try moving the player first
	   unless we're in the midst of creating a bones file. */
	if (reason == DISMOUNT_BONES) {
	    /* move the steed to an adjacent square */
	    if (enexto(&cc, u.ux, u.uy, mtmp->data))
		rloc_to(mtmp, cc.x, cc.y);
	    else	/* evidently no room nearby; move steed elsewhere */
		(void) rloc(mtmp, FALSE);
	    return;
	}
	if (!DEADMONSTER(mtmp)) {
	    place_monster(mtmp, u.ux, u.uy);
	    if (!u.uswallow && !u.ustuck && have_spot) {
		struct permonst *mdat = mtmp->data;

		/* The steed may drop into water/lava */
		if (!is_flyer(mdat) && !is_floater(mdat) && !is_clinger(mdat)) {
		    if (is_pool(u.ux, u.uy)) {
			if (!Underwater)
			    pline(E_J("%s falls into the %s!",
				      "%sは%sに落ちた！"), Monnam(mtmp),
							surface(u.ux, u.uy));
			if (!is_swimmer(mdat) && !amphibious(mdat)) {
			    killed(mtmp);
			    adjalign(-1);
			}
		    } else if (is_lava(u.ux, u.uy)) {
			pline(E_J("%s is pulled into the lava!",
				  "%sは溶岩に引きずり込まれた！"), Monnam(mtmp));
			if (!likes_lava(mdat)) {
			    killed(mtmp);
			    adjalign(-1);
			}
		    }
		}
	    /* Steed dismounting consists of two steps: being moved to another
	     * square, and descending to the floor.  We have functions to do
	     * each of these activities, but they're normally called
	     * individually and include an attempt to look at or pick up the
	     * objects on the floor:
	     * teleds() --> spoteffects() --> pickup()
	     * float_down() --> pickup()
	     * We use this kludge to make sure there is only one such attempt.
	     *
	     * Clearly this is not the best way to do it.  A full fix would
	     * involve having these functions not call pickup() at all, instead
	     * calling them first and calling pickup() afterwards.  But it
	     * would take a lot of work to keep this change from having any
	     * unforseen side effects (for instance, you would no longer be
	     * able to walk onto a square with a hole, and autopickup before
	     * falling into the hole).
	     */
		/* [ALI] No need to move the player if the steed died. */
		if (!DEADMONSTER(mtmp)) {
		    /* Keep steed here, move the player to cc;
		     * teleds() clears u.utrap
		     */
		    in_steed_dismounting = TRUE;
		    teleds(cc.x, cc.y, TRUE);
		    in_steed_dismounting = FALSE;

		    /* Put your steed in your trap */
		    if (save_utrap)
			(void) mintrap(mtmp);
		}
	    /* Couldn't... try placing the steed */
	    } else if (enexto(&cc, u.ux, u.uy, mtmp->data)) {
		/* Keep player here, move the steed to cc */
		rloc_to(mtmp, cc.x, cc.y);
		/* Player stays put */
	    /* Otherwise, kill the steed */
	    } else {
		killed(mtmp);
		adjalign(-1);
	    }
	}

	/* Return the player to the floor */
	if (reason != DISMOUNT_ENGULFED) {
	    in_steed_dismounting = TRUE;
	    (void) float_down(0L, W_SADDLE);
	    in_steed_dismounting = FALSE;
	    flags.botl = 1;
	    (void)encumber_msg();
	    vision_full_recalc = 1;
	} else
	    flags.botl = 1;
	/* polearms behave differently when not mounted */
	if (uwep && is_pole(uwep)) unweapon = TRUE;
	return;
}

void
place_monster(mon, x, y)
struct monst *mon;
int x, y;
{
    if (mon == u.usteed ||
	    /* special case is for convoluted vault guard handling */
	    (DEADMONSTER(mon) && !(mon->isgd && x == 0 && y == 0))) {
	impossible("placing %s onto map?",
		   (mon == u.usteed) ? "steed" : "defunct monster");
	return;
    }
    mon->mx = x, mon->my = y;
    level.monsters[x][y] = mon;
#ifdef MONSTEED
    if ((is_mriding(mon) || is_swallowing(mon)) && mon->mlink) {
	mon->mlink->mx = x;
	mon->mlink->my = y;
    }
#endif /*MONSTEED*/
}

#ifdef MONSTEED

boolean mcanride(mrid, mstd)
struct monst *mrid, *mstd;
{
	struct permonst *rp, *sp;

	if (!mrid || !mstd) {
		impossible("mcanride: NULL mrider/msteed?");
		return FALSE;
	}
	rp = mrid->data;
	sp = mstd->data;
	if (!is_elf(rp) && !which_armor(mstd, W_SADDLE))
		return FALSE;
	return (humanoid(rp) && !verysmall(rp) &&
		!bigmonst(rp) &&
		index(steeds, sp->mlet) && (sp->msize >= MZ_MEDIUM) &&
		(!humanoid(sp) || sp->mlet == S_CENTAUR) &&
		!amorphous(sp) && !noncorporeal(sp) &&
		(sp->mlet != S_DOG || is_orc(rp)) &&	/* orcs can ride on wolves */
		!is_whirly(sp) && !unsolid(sp) &&
		(mrid->mpeaceful == mstd->mpeaceful) &&
		!mrid->mtame && !mstd->mtame);
}
/*
    decide which adjacent square to land on
    since monsters only dismount its steed by accident,
    there is no check for traps, pools and etc.
 */
boolean
mlanding_spot(mtmp, cc)
struct monst *mtmp;
coord *cc;
{
	int x,y;
	int i,j;
	j = rn2(8);
	for (i=0; i<8; i++) {
	    x = mtmp->mx + xdir[(i+j) & 0x07];
	    y = mtmp->my + ydir[(i+j) & 0x07];
	    if (!isok(x, y) || (x == u.ux && y == u.uy) ||
		!ACCESSIBLE(levl[x][y].typ) || MON_AT(x,y) ||
		closed_door(x,y)) continue;
		if (cc) {
		    cc->x = x;
		    cc->y = y;
		}
		return TRUE;
	}
	return FALSE;
}

struct monst *
msearchsteed(mtmp)
struct monst *mtmp;
{
	struct monst *mstd = 0;
	int x,y;
	int i,j;
	j = rn2(8);
	for (i=0; i<8; i++) {
	    x = mtmp->mx + xdir[(i+j) & 0x07];
	    y = mtmp->my + ydir[(i+j) & 0x07];
	    if (!isok(x, y) || (x == u.ux && y == u.uy)) continue;
	    if (MON_AT(x,y) && ACCESSIBLE(levl[x][y].typ) &&
		!closed_door(x,y)) {
		mstd = m_at(x,y);
		if (is_mriding(mstd) || !mstd->mcanmove ||
		    mstd->mconf || mstd->mstun ||
		    mstd->mhp < mstd->mhpmax/4) continue;
		if (isfavoritesteed(mtmp->data, mstd->data) &&
		    mcanride(mtmp, mstd))
			return mstd;
	    }
	}
	return 0;
}

struct monst *
mrider_or_msteed(mtmp, selsteed)
struct monst *mtmp;
boolean selsteed;
{
	if (!mtmp) {impossible("mrider_or_msteed: Null monst?"); return 0;}
	return (is_mriding(mtmp) && selsteed) ? mtmp->mlink : mtmp;
}

struct monst *
target_rider_or_steed(magr, mdef)
struct monst *magr, *mdef;
{
	int lev = 0;
	if (!magr || !mdef) {impossible("target_rider_or_steed: Null monst?"); return 0;}
	/* if defender is alone, choose it */
	if (mdef == &youmonst) {
	    if (!u.usteed) return mdef;
	} else if (!is_mriding(mdef)) return mdef;
	/* calc attacker's height */
	if (magr == &youmonst) {
	    if (Flying || Levitation ||
		(uwep && is_ranged(uwep)))
		lev = 7;
	    else if (u.usteed)
		lev = (u.usteed)->data->msize;
	    lev += (youmonst.data)->msize;
	} else {
	    struct obj *otmp;
	    if (is_flying(magr) || is_floating(magr) ||
		((otmp = MON_WEP(magr)) && is_ranged(otmp)))
		lev = 7;
	    else if (is_mriding(magr))
		lev = magr->mlink->data->msize;
	    lev += magr->data->msize;
	}
	/* calc difference of height level */
	if (mdef == &youmonst) {
	    if (Flying || Levitation)
		 lev -= 7;
	    else lev -= (u.usteed)->data->msize;
	    lev -= (youmonst.data)->msize;
	    return (rn2(15) < lev+10) ? mdef : u.usteed;
	} else {
	    if (is_flying(mdef) || is_floating(mdef))
		 lev -= 7;
	    else lev -= mdef->mlink->data->msize;
	    lev -= mdef->data->msize;
	    return (rn2(15) < lev+10) ? mdef : mdef->mlink;
	}
}

/* dismount monster riding on another monster */
/* 0:rider died  1:rider survived(landed somewhere) */
int
mdismount_steed(mtmp)
struct monst *mtmp;
{
	struct monst *mrid, *mstd;
	coord cc;
	char nstd[BUFSZ];
	char *fallmsg = E_J("%s %s off of %s%s","%sは%sから%s%s");
	char *verb = E_J("falls","落ちた");
	boolean seeit;

	if (is_mriding(mtmp)) {
	    mrid = mtmp;
	    mstd = mtmp->mlink;
	} else if (is_mridden(mtmp)){
	    mrid = mtmp->mlink;
	    mstd = mtmp;
	} else {
	    impossible("non-rider dismounts from non-steed?");
	    return 0;
	}

	/* unlink rider from steed */
	unlink_mlink(mrid);
	unlink_mlink(mstd);

	seeit = (canspotmon(mrid) || canspotmon(mstd)) && cansee(mrid->mx, mrid->my);

	if (canspotmon(mstd))
#ifndef JP
	    Sprintf(nstd, "%s %s", mhis(mrid),
		    x_monnam(mstd, ARTICLE_NONE, 0, SUPPRESS_SADDLE, FALSE));
#else
	    Strcpy(nstd, x_monnam(mstd, ARTICLE_NONE, 0, SUPPRESS_SADDLE, FALSE));
#endif /*JP*/
	else Strcpy(nstd, something);

	/* search somewhere to land */
	/* adjacent square first */
	if (mlanding_spot(mrid, &cc)) {
	    ;
	} else if (enexto(&cc, mrid->mx, mrid->my, mrid->data)) {
	    verb = E_J("is thrown","投げ出された");
	} else {
	    /* no place to land... rider dies */
	    if (seeit)
#ifndef JP
		pline(fallmsg, Monnam(mrid), verb, nstd, "!");
#else
		pline(fallmsg, Monnam(mrid), nstd, verb, "！");
#endif /*JP*/
	    monkilled(mrid, E_J("riding accident","d乗馬中の事故で"), AD_PHYS);
	    if (!DEADMONSTER(mrid)) { /* lifesaved? */
		if (seeit)
		    pline(E_J("Unfortunately %s is crushed to death under %s...",
			      "不幸なことに、%sは%sの下で押し潰された…。"),
			  mon_nam(mrid), nstd);
		monkilled(mrid, mon_nam(mstd), AD_PHYS);
	    }
	    place_monster(mstd, mstd->mx, mstd->my);
	    newsym(mstd->mx, mstd->my);
	    return 0;
	}

	/* place the rider to new place and the steed to the current place */
	rloc_to(mrid, cc.x, cc.y);
	place_monster(mstd, mstd->mx, mstd->my);
	newsym(mstd->mx, mstd->my);

	if (seeit) {
	    if (is_flyer(mrid->data) || is_floater(mrid->data))
#ifndef JP
		pline(fallmsg, Monnam(mrid),
			is_flyer(mrid->data) ? "flies" : "floats", nstd, ".");
#else
		pline(fallmsg, Monnam(mrid), nstd,
			is_flyer(mrid->data) ? "飛び降りた" : "浮き去った", "。");
#endif /*JP*/
	    else
#ifndef JP
		pline(fallmsg, Monnam(mrid), verb, nstd, "!");
#else
		pline(fallmsg, Monnam(mrid), nstd, verb, "！");
#endif /*JP*/
	}
	mrid->movement = 0;
	(void) mintrap(mrid);
	(void) minliquid(mrid);
	return !DEADMONSTER(mrid);
}

#endif /*MONSTEED*/

#endif /* STEED */

/*steed.c*/
