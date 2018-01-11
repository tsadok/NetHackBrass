/*	SCCS Id: @(#)uhitm.c	3.4	2003/02/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL boolean FDECL(known_hitum, (struct monst *,int *,struct attack *, uchar));
STATIC_DCL void FDECL(steal_it, (struct monst *, struct attack *));
STATIC_DCL boolean FDECL(hitum, (struct monst *,int,struct attack *, uchar));
STATIC_DCL boolean FDECL(hmon_hitmon, (struct monst *,struct obj *,int));
#ifdef STEED
STATIC_DCL int FDECL(joust, (struct monst *,struct obj *));
#endif
STATIC_DCL void NDECL(demonpet);
STATIC_DCL boolean FDECL(m_slips_free, (struct monst *mtmp,struct attack *mattk));
STATIC_DCL int FDECL(explum, (struct monst *,struct attack *));
STATIC_DCL void FDECL(start_engulf, (struct monst *));
STATIC_DCL void NDECL(end_engulf);
STATIC_DCL int FDECL(gulpum, (struct monst *,struct attack *));
STATIC_DCL boolean FDECL(hmonas, (struct monst *,int, uchar));
STATIC_DCL void FDECL(nohandglow, (struct monst *));
STATIC_DCL boolean FDECL(shade_aware, (struct obj *));
STATIC_OVL boolean FDECL(is_caitiff, (struct monst *));

extern boolean notonhead;	/* for long worms */
/* The below might become a parameter instead if we use it a lot */
static int dieroll;
static int misstyp;
/* Used to flag attacks caused by Stormbringer's maliciousness. */
static boolean override_confirmation = FALSE;

#define PROJECTILE(obj)	((obj) && is_ammo(obj))

#define HIT_DISTANT	0x01
#define HIT_MAINWEP	0x02
#define HIT_SWAPWEP	0x04
#define HIT_SHIELD	0x08
#define HIT_KICK	0x10

/* modified from hurtarmor() in mhitu.c */
/* This is not static because it is also used for monsters rusting monsters */
void
hurtmarmor(mdef, attk)
struct monst *mdef;
int attk;
{
	int	hurt;
	struct obj *target;

	switch(attk) {
	    /* 0 is burning, which we should never be called with */
	    case AD_RUST: hurt = 1; break;
	    case AD_CORR: hurt = 3; break;
	    default: hurt = 2; break;
	}
	/* What the following code does: it keeps looping until it
	 * finds a target for the rust monster.
	 * Head, feet, etc... not covered by metal, or covered by
	 * rusty metal, are not targets.  However, your body always
	 * is, no matter what covers it.
	 */
	while (1) {
	    switch(rn2(5)) {
	    case 0:
		target = which_armor(mdef, W_ARMH);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    case 1:
		target = which_armor(mdef, W_ARMC);
		if (target) {
		    (void)rust_dmg(target, xname(target), hurt, TRUE, mdef);
		    break;
		}
		if ((target = which_armor(mdef, W_ARM)) != (struct obj *)0) {
		    (void)rust_dmg(target, xname(target), hurt, TRUE, mdef);
#ifdef TOURIST
		} else if ((target = which_armor(mdef, W_ARMU)) != (struct obj *)0) {
		    (void)rust_dmg(target, xname(target), hurt, TRUE, mdef);
#endif
		}
		break;
	    case 2:
		target = which_armor(mdef, W_ARMS);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    case 3:
		target = which_armor(mdef, W_ARMG);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    case 4:
		target = which_armor(mdef, W_ARMF);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    }
	    break; /* Out of while loop */
	}
}

/* FALSE means it's OK to attack */
boolean
attack_checks(mtmp, wep)
register struct monst *mtmp;
struct obj *wep;	/* uwep for attack(), null for kick_monster() */
{
	char qbuf[QBUFSZ];

	/* if you're close enough to attack, alert any waiting monster */
	mtmp->mstrategy &= ~STRAT_WAITMASK;

	if (u.uswallow && mtmp == u.ustuck) return FALSE;

	if (flags.forcefight) {
		/* Do this in the caller, after we checked that the monster
		 * didn't die from the blow.  Reason: putting the 'I' there
		 * causes the hero to forget the square's contents since
		 * both 'I' and remembered contents are stored in .glyph.
		 * If the monster dies immediately from the blow, the 'I' will
		 * not stay there, so the player will have suddenly forgotten
		 * the square's contents for no apparent reason.
		if (!canspotmon(mtmp) &&
		    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph))
			map_invisible(u.ux+u.dx, u.uy+u.dy);
		 */
		return FALSE;
	}

	/* Put up an invisible monster marker, but with exceptions for
	 * monsters that hide and monsters you've been warned about.
	 * The former already prints a warning message and
	 * prevents you from hitting the monster just via the hidden monster
	 * code below; if we also did that here, similar behavior would be
	 * happening two turns in a row.  The latter shows a glyph on
	 * the screen, so you know something is there.
	 */
	if (!canspotmons(mtmp) &&
		    !glyph_is_warning(glyph_at(u.ux+u.dx,u.uy+u.dy)) &&
		    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
		    !(!Blind && mtmp->mundetected && hides_under(mtmp->data))) {
		pline(E_J("Wait!  There's %s there you can't see!",
			  "待て！ そこには%s見えないものがいる！"),
			something);
		map_invisible(u.ux+u.dx, u.uy+u.dy);
		/* if it was an invisible mimic, treat it as if we stumbled
		 * onto a visible mimic
		 */
		if(mtmp->m_ap_type && !Protection_from_shape_changers) {
		    if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
			u.ustuck = mtmp;
		}
		wakeup(mtmp); /* always necessary; also un-mimics mimics */
		return TRUE;
	}

	if (mtmp->m_ap_type && !Protection_from_shape_changers &&
	   !sensemon(mtmp) &&
	   !glyph_is_warning(glyph_at(u.ux+u.dx,u.uy+u.dy))) {
		/* If a hidden mimic was in a square where a player remembers
		 * some (probably different) unseen monster, the player is in
		 * luck--he attacks it even though it's hidden.
		 */
		if (glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph)) {
		    seemimic(mtmp);
		    return(FALSE);
		}
		stumble_onto_mimic(mtmp);
		return TRUE;
	}

	if (mtmp->mundetected && !canseemon(mtmp) &&
		!glyph_is_warning(glyph_at(u.ux+u.dx,u.uy+u.dy)) &&
		(hides_under(mtmp->data) || mtmp->data->mlet == S_EEL)) {
	    mtmp->mundetected = mtmp->msleeping = 0;
	    newsym(mtmp->mx, mtmp->my);
	    if (glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph)) {
		seemimic(mtmp);
		return(FALSE);
	    }
	    if (!(Blind ? Blind_telepat : Unblind_telepat)) {
		struct obj *obj;

		if (Blind || (is_pool(mtmp->mx,mtmp->my) && !Underwater))
		    pline(E_J("Wait!  There's a hidden monster there!",
			      "待て！ そこには怪物が隠れている！"));
		else if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0)
#ifndef JP
		    pline("Wait!  There's %s hiding under %s!",
			  an(l_monnam(mtmp)), doname(obj));
#else
		    pline("待て！ %sの下に%sが潜んでいる！",
			  doname(obj), l_monnam(mtmp));
#endif /*JP*/
		return TRUE;
	    }
	}

	/*
	 * make sure to wake up a monster from the above cases if the
	 * hero can sense that the monster is there.
	 */
	if ((mtmp->mundetected || mtmp->m_ap_type) &&
	    (sensemon(mtmp) || mon_warning(mtmp))) {
	    mtmp->mundetected = 0;
	    wakeup(mtmp);
	}

	if (flags.confirm && mtmp->mpeaceful
	    && !Confusion && !Hallucination && !Stunned) {
		/* Intelligent chaotic weapons (Stormbringer) want blood */
		if (wep && wep->oartifact == ART_STORMBRINGER) {
			override_confirmation = TRUE;
			return(FALSE);
		}
		if (canspotmon(mtmp)) {
			Sprintf(qbuf, E_J("Really attack %s?",
				"本当に%sを攻撃しますか？"), mon_nam(mtmp));
			if (yn(qbuf) != 'y') {
				flags.move = 0;
				return(TRUE);
			}
		}
	}

	return(FALSE);
}

/*
 * It is unchivalrous for a knight to attack the defenseless or from behind.
 */
STATIC_OVL boolean
is_caitiff(mtmp)
struct monst *mtmp;
{
	return (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL &&
		(!mtmp->mcanmove || mtmp->msleeping ||
		(mtmp->mflee && !mtmp->mavenge)) &&
		 u.ualign.record > -10);
}

void
check_caitiff(mtmp)
struct monst *mtmp;
{
	if (is_caitiff(mtmp)) {
	    E_J(You("caitiff!"), pline("この卑怯者め！"));
	    adjalign(-1);
	}
}

schar
find_roll_to_hit(mtmp, hflg)
register struct monst *mtmp;
uchar hflg;
{
	schar tmp;
	int tmp2;
	struct obj *wep = (struct obj *)0;

	if (hflg & HIT_MAINWEP) wep = uwep;
	else if (hflg & HIT_SWAPWEP) wep = uswapwep;

	/* restricted weapons are hard to handle */
	if (weapon_type(wep) && P_RESTRICTED(weapon_type(wep))) return 2;

	tmp = 1 + abon_luck() + abon() + find_mac(mtmp) + u.uhitinc +
		maybe_polyd(youmonst.data->mlevel, xlev_to_rank(u.ulevel));

	check_caitiff(mtmp);

/*	attacking peaceful creatures is bad for the samurai's giri */
	if (Role_if(PM_SAMURAI) && mtmp->mpeaceful &&
	    u.ualign.record > -10) {
	    You(E_J("dishonorably attack the innocent!",
		    "不埒にも罪無き者を手にかけた！"));
	    adjalign(-1);
	}

/*	Adjust vs. (and possibly modify) monster state.		*/

	if(mtmp->mstun) tmp += 2;
	if(mtmp->mflee) tmp += 2;

	if (mtmp->msleeping) {
		mtmp->msleeping = 0;
		tmp += 2;
	}
	if(!mtmp->mcanmove) {
		tmp += 4;
		if(!rn2(10)) {
			mtmp->mcanmove = 1;
			mtmp->mfrozen = 0;
		}
	}
	if (is_orc(mtmp->data) && maybe_polyd(is_elf(youmonst.data),
			Race_if(PM_ELF)))
	    tmp++;
	if(Role_if(PM_MONK) && !Upolyd) {
	    if (uarm && !is_clothes(uarm)) {
//		Your("armor is rather cumbersome...");
		tmp -= urole.spelarmr;
	    } else if (!wep && !uarms) {
		tmp += (u.ulevel / 3) + 2;
	    }
	}

/*	with a lot of luggage, your agility diminishes */
	if ((tmp2 = near_capacity()) != 0) tmp -= (tmp2*2) - 1;
	if (u.utrap) tmp -= 3;

	/* Modify to-hit depending on distance; but keep it sane.
	 * Polearms get a distance penalty even when wielded; it's
	 * hard to hit at a distance.
	 */
	if (hflg & HIT_DISTANT) {
		int disttmp;
		disttmp = distmin(u.ux, u.uy, mtmp->mx, mtmp->my) - 1;
		if(disttmp > 4) disttmp = 4;
		tmp -= disttmp;
	}

	/* Ki and beseark special ability, see cmd.c. - SW */
	if (Role_special(PM_SAMURAI)  ) tmp += 4;
	if (Role_special(PM_BARBARIAN)) tmp += 2;

/*	Some monsters have a combination of weapon attacks and non-weapon
 *	attacks.  It is therefore wrong to add hitval to tmp; we must add
 *	it only for the specific attack (in hmonas()).
 */
	if (!Upolyd) {
	    if (wep) tmp += hitval(wep, mtmp);		/* melee weapon */
	    else if ((hflg & HIT_KICK) && uarmf) {	/* kick */
		tmp += uarmf->spe;
		if (uarmf->otyp == KICKING_BOOTS) tmp += 5;
	    } else if (uarmg && uarmg->oartifact)	/* bare/gloved hand */
		tmp += spec_abon(uarmg, mtmp);
	    tmp += weapon_hit_bonus(wep);
	}

	/* At least 5% chance guaranteed to hit */
	if (tmp < 2) tmp = 2;
	return tmp;
}

/* try to attack; return FALSE if monster evaded */
/* u.dx and u.dy must be set */
boolean
attack(mtmp)
register struct monst *mtmp;
{
	schar tmp;
	register struct permonst *mdat = mtmp->data;
	uchar hitflags = 0;
	boolean malive = TRUE;
	boolean distant = FALSE;
	boolean twcan = FALSE;
	boolean amain, asub;
	int mdist;

	mdist = u.dx * u.dx + u.dy * u.dy;
	distant = (mdist >= 3);
	if (distant) hitflags |= HIT_DISTANT;

	/* This section of code provides protection against accidentally
	 * hitting peaceful (like '@') and tame (like 'd') monsters.
	 * Protection is provided as long as player is not: blind, confused,
	 * hallucinating or stunned.
	 * changes by wwp 5/16/85
	 * More changes 12/90, -dkh-. if its tame and safepet, (and protected
	 * 07/92) then we assume that you're not trying to attack. Instead,
	 * you'll usually just swap places if this is a movement command
	 */
	/* Intelligent chaotic weapons (Stormbringer) want blood */
	if (!distant && is_safepet(mtmp) && !flags.forcefight) {
	    if (!uwep || uwep->oartifact != ART_STORMBRINGER) {
		/* there are some additional considerations: this won't work
		 * if in a shop or Punished or you miss a random roll or
		 * if you can walk thru walls and your pet cannot (KAA) or
		 * if your pet is a long worm (unless someone does better).
		 * there's also a chance of displacing a "frozen" monster.
		 * sleeping monsters might magically walk in their sleep.
		 */
		boolean foo = (Punished || !rn2(7) || is_longworm(mtmp->data)),
			inshop = FALSE;
		char *p;

		for (p = in_rooms(mtmp->mx, mtmp->my, SHOPBASE); *p; p++)
		    if (tended_shop(&rooms[*p - ROOMOFFSET])) {
			inshop = TRUE;
			break;
		    }

		if (inshop || foo ||
			(IS_ROCK(levl[u.ux][u.uy].typ) &&
					!passes_walls(mtmp->data))) {
		    char buf[BUFSZ];

		    monflee(mtmp, rnd(6), FALSE, FALSE);
		    Strcpy(buf, y_monnam(mtmp));
		    buf[0] = highc(buf[0]);
		    You(E_J("stop.  %s is in the way!",
			    "止まった。%sが行く先にいる！"), buf);
		    return(TRUE);
		} else if ((mtmp->mfrozen || (! mtmp->mcanmove)
				|| (mtmp->data->mmove == 0)) && rn2(6)) {
		    pline(E_J("%s doesn't seem to move!",
			      "%sは動こうとしない！"), Monnam(mtmp));
		    return(TRUE);
		} else return(FALSE);
	    }
	}

	/* possibly set in attack_checks;
	   examined in known_hitum, called via hitum or hmonas below */
	override_confirmation = FALSE;
	if (attack_checks(mtmp, uwep)) return(TRUE);

	if (Upolyd) {
		/* certain "pacifist" monsters don't attack */
		if(noattacks(youmonst.data)) {
			You(E_J("have no way to attack monsters physically.",
				"怪物を物理的に攻撃する手段を持っていない。"));
			mtmp->mstrategy &= ~STRAT_WAITMASK;
			goto atk_done;
		}
	}

	if(check_capacity(E_J("You cannot fight while so heavily loaded.",
			      "こう荷物が重くては、あなたは戦えない。")))
	    goto atk_done;

	if(notonhead) u.ulasttgt = 0;	/* worm tails cannot be traced by mx,my */
	else if(u.ulasttgt != mtmp) u.ulasttgt = mtmp;	/* you targetted it */

	if (u.twoweap && !can_twoweapon())
		untwoweapon();

	if(unweapon) {
	    unweapon = FALSE;
	    if(flags.verbose) {
		if(uwep) {
		    /* supress */
		    if (
#ifdef FIRSTAID
			!(uwep->otyp == SCISSORS &&
			  (mdat == &mons[PM_PAPER_GOLEM] ||
			   mdat == &mons[PM_ROPE_GOLEM]))
#endif /*FIRSTAID*/
		    ) You(E_J("begin bashing monsters with your %s.",
			      "%sで怪物を叩きはじめた。"),
			    E_J(aobjnam(uwep, (char *)0), xname(uwep)));
		} else if (!cantwield(youmonst.data))
#ifndef JP
		    You("begin %sing monsters with your %s %s.",
			Role_if(PM_MONK) ? "strik" : "bash",
			uarmg ? "gloved" : "bare",	/* Del Lamb */
			makeplural(body_part(HAND)));
#else
		    You("%s%sで怪物を%sはじめた。",
			uarmg ? "グローブをはめた" : "素",	/* Del Lamb */
			body_part(HAND),
			Role_if(PM_MONK) ? "殴打し" : "叩き");
#endif /*JP*/
	    }
	}
	exercise(A_STR, TRUE);		/* you're exercising muscles */
	/* andrew@orca: prevent unlimited pick-axe attacks */
	u_wipe_engr(3);

	/* Is the "it died" check actually correct? */
	if(!distant && mdat->mlet == S_LEPRECHAUN && !mtmp->mfrozen &&
	   !mtmp->msleeping && !mtmp->mconf && mtmp->mcansee && !rn2(7) &&
	   (m_move(mtmp, 0) == 2 ||			    /* it died */
	   mtmp->mx != u.ux+u.dx || mtmp->my != u.uy+u.dy)) /* it moved */
		return(FALSE);

	twcan = !!(Stunned);
	amain = !distant || (uwep && is_ranged(uwep) && mdist <= weapon_range(uwep));
	asub  = u.twoweap && (!distant || (uswapwep && is_ranged(uswapwep) &&
					   mdist <= weapon_range(uswapwep)));

	/* primary weapon */
	if (amain) {
	    tmp = find_roll_to_hit(mtmp, hitflags|HIT_MAINWEP);
	    if (Upolyd)
		/* assume monsters don't have ranged attack */
		(void) hmonas(mtmp, tmp, hitflags|HIT_MAINWEP);
	    else
		malive = hitum(mtmp, tmp, youmonst.data->mattk, hitflags|HIT_MAINWEP);
	}

	mtmp->mstrategy &= ~STRAT_WAITMASK;

	/* Two-weapon attack & double-attack */
	if (malive && !Upolyd && !is_caitiff(mtmp) &&
	    !mtmp->wormno && /* for long worm tail */
	    (twcan || !Stunned) && !multi) {
	    if (asub) {						/* two-weapon combat */
		tmp = find_roll_to_hit(mtmp, hitflags|HIT_SWAPWEP);
		malive = hitum(mtmp, tmp, youmonst.data->mattk, hitflags|HIT_SWAPWEP);
	    } else if (amain && doubleattack_roll() >= rnd(20)) {		/* double attack */
		tmp = find_roll_to_hit(mtmp, hitflags|HIT_MAINWEP);
		malive = hitum(mtmp, tmp, youmonst.data->mattk, hitflags|HIT_MAINWEP);
	    }
	}
	/* additional kick */
//	if (malive && !distant && !multi && !is_caitiff(mtmp) &&
//#ifdef STEED
//	    !u.usteed &&
//#endif /*STEED*/
//	    uarmf && uarmf->otyp == KICKING_BOOTS) {
//		tmp = find_roll_to_hit(mtmp, hitflags|HIT_KICK);
//		malive = hitum(mtmp, tmp, youmonst.data->mattk, hitflags|HIT_KICK);
//	}

atk_done:
	/* see comment in attack_checks() */
	/* we only need to check for this if we did an attack_checks()
	 * and it returned 0 (it's okay to attack), and the monster didn't
	 * evade.
	 */
	if (flags.forcefight && mtmp->mhp > 0 && !canspotmons(mtmp) &&
	    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
	    !(u.uswallow && mtmp == u.ustuck))
		map_invisible(u.ux+u.dx, u.uy+u.dy);

	return(TRUE);
}

STATIC_OVL boolean
known_hitum(mon, mhit, uattk, hflg)	/* returns TRUE if monster still lives */
register struct monst *mon;
register int *mhit;
struct attack *uattk;
uchar hflg;
{
	register boolean malive = TRUE;
	struct obj *wep = (struct obj *)0;

	if (override_confirmation) {
	    /* this may need to be generalized if weapons other than
	       Stormbringer acquire similar anti-social behavior... */
	    if (flags.verbose) Your(E_J("bloodthirsty blade attacks!","血に飢えた刃が襲い掛かった！"));
	}

	if (hflg & HIT_MAINWEP) wep = uwep;
	else if (hflg & HIT_SWAPWEP) wep = uswapwep;
	else if (hflg & HIT_SHIELD) wep = uarms;

	if (override_confirmation && wep->oartifact == ART_STORMBRINGER && wep->blessed)
	    wep->blessed = 0;

	if (Fumbling && !rn2(3)) {
		You(E_J("swing wildly and miss %s.",
			"やみくもに振り回したが、%sには当たらなかった。"), mon_nam(mon));
		return (malive);
	}
	if(!*mhit) {
	    missum(mon, uattk, misstyp);
	} else {
	    int oldhp = mon->mhp,
		x = u.ux + u.dx, y = u.uy + u.dy;

	    /* KMH, conduct */
	    if (wep && (wep->oclass == WEAPON_CLASS || is_weptool(wep)))
		u.uconduct.weaphit++;

	    /* we hit the monster; be careful: it might die or
	       be knocked into a different location */
	    notonhead = (mon->mx != x || mon->my != y);
	    malive = hmon(mon, wep, 0);
	    /*	This assumes that Stormbringer was uwep not uswapwep */ 
//	    if (malive && !override_confirmation) {
//		if (u.twoweap) malive = hmon(mon, uswapwep, 0);		/* two-weapon combat */
//		else if (doubleattack_roll() >= rnd(20)) {		/* double attack */
//			malive = hmon(mon, uwep, 0);
//		}
//	    }
	    if (malive) {
		/* monster still alive */
		if(!rn2(25) && mon->mhp < mon->mhpmax/2
			    && !(u.uswallow && mon == u.ustuck)) {
		    /* maybe should regurgitate if swallowed? */
		    if(!rn2(3)) {
			monflee(mon, rnd(100), FALSE, TRUE);
		    } else monflee(mon, 0, FALSE, TRUE);

		    if(u.ustuck == mon && !u.uswallow && !sticks(youmonst.data))
			u.ustuck = 0;
		}
		/* Vorpal Blade hit converted to miss */
		/* could be headless monster or worm tail */
		if (mon->mhp == oldhp) {
		    *mhit = 0;
		    /* a miss does not break conduct */
		    if (wep &&
			(wep->oclass == WEAPON_CLASS || is_weptool(wep)))
			--u.uconduct.weaphit;
		}
		if (mon->wormno && *mhit)
			cutworm(mon, x, y, wep);
	    }
	    /* identify your weapon */
	    if (wep && !wep->known && wepidentify_byhit(wep)) {
		wep->known = 1;
		You(E_J("find the quality of your weapon.",
			"自分の武器の品質を見定めた。"));
		prinv(NULL, wep, 0);
	    }
	}
	return(malive);
}

STATIC_OVL boolean
hitum(mon, tmp, uattk, hflg)		/* returns TRUE if monster still lives */
struct monst *mon;
int tmp;
struct attack *uattk;
uchar hflg;
{
	boolean malive;
	int mhit = (tmp > (dieroll = rnd(20)) || u.uswallow);
if (wizard) pline("[%d:%d]",dieroll,tmp);
	if(tmp > dieroll) exercise(A_DEX, TRUE);
	misstyp = (dieroll - tmp);
	malive = known_hitum(mon, &mhit, uattk, hflg);
	(void) passive(mon, mhit, malive, AT_WEAP, hflg);
	return(malive);
}

boolean			/* general "damage monster" routine */
hmon(mon, obj, thrown)		/* return TRUE if mon still alive */
struct monst *mon;
struct obj *obj;
int thrown;
{
	boolean result, anger_guards;

	anger_guards = (mon->mpeaceful &&
			    (mon->ispriest || mon->isshk ||
			     mon->data == &mons[PM_WATCHMAN] ||
			     mon->data == &mons[PM_WATCH_CAPTAIN]));
	result = hmon_hitmon(mon, obj, thrown);
	if (mon->ispriest && !rn2(2)) ghod_hitsu(mon);
	if (anger_guards) (void)angry_guards(!flags.soundok);
	return result;
}

/* guts of hmon() */
STATIC_OVL boolean
hmon_hitmon(mon, obj, thrown)
struct monst *mon;
struct obj *obj;
int thrown;
{
	int tmp;
	struct permonst *mdat = mon->data;
	int barehand_silver_rings = 0;
	/* The basic reason we need all these booleans is that we don't want
	 * a "hit" message when a monster dies, so we have to know how much
	 * damage it did _before_ outputting a hit message, but any messages
	 * associated with the damage don't come out until _after_ outputting
	 * a hit message.
	 */
	boolean hittxt = FALSE, destroyed = FALSE, already_killed = FALSE;
	boolean get_dmg_bonus = TRUE;
	boolean ispoisoned = FALSE, needpoismsg = FALSE, poiskilled = FALSE;
	boolean silvermsg = FALSE, silverobj = FALSE;
	int stunmon = 0;
	boolean valid_weapon_attack = FALSE;
	boolean unarmed = !uwep && !uarm && !uarms;
#ifdef STEED
	int jousting = 0;
#endif
#ifdef MONSTEED
	int pdismount = 0;	/* dismount a monster */
#endif
	int wtype;
	struct obj *monwep;
	char yourbuf[BUFSZ];
	char unconventional[BUFSZ];	/* substituted for word "attack" in msg */
	char saved_oname[BUFSZ];

	unconventional[0] = '\0';
	saved_oname[0] = '\0';

	wakeup(mon);
	if(!obj) {	/* attack with bare hands */
	    if (mdat == &mons[PM_SHADE])
		tmp = 0;
	    else if (martial_bonus()) {
		int skill = P_SKILL(P_MARTIAL_ARTS);
		int tmp2 = 4;
		if (skill >= P_BASIC) tmp2 = skill / 10;
		tmp = rnd(tmp2);	/* bonus for martial arts */
	    } else
		tmp = rnd(2);
	    /* artifact(gloves) */
	    if (uarmg && uarmg->oartifact &&
		(hittxt = artifact_hit(&youmonst, mon, uarmg, &tmp, dieroll))) {
		if(mon->mhp <= 0) /* artifact killed monster */
		    return FALSE;
		if (tmp == 0) return TRUE;
	    }
	    valid_weapon_attack = (tmp > 1);
	    /* blessed gloves give bonuses when fighting 'bare-handed' */
	    if (uarmg && uarmg->blessed && (is_undead(mdat) || is_demon(mdat)))
		tmp += rnd(4);
	    /* So do silver rings.  Note: rings are worn under gloves, so you
	     * don't get both bonuses.
	     */
	    if (!uarmg) {
		if (uleft && get_material(uleft) == SILVER)
		    barehand_silver_rings++;
		if (uright && get_material(uright) == SILVER)
		    barehand_silver_rings++;
		if (barehand_silver_rings && hates_silver(mdat)) {
		    tmp += rnd(20);
		    silvermsg = TRUE;
		}
	    } else {
		if (get_material(uarmg) == SILVER && hates_silver(mdat)) {
		    tmp += rnd(20);
		    silvermsg = TRUE;
		    silverobj = TRUE;
		    Strcpy(saved_oname, "篭手");
		}
	    }
	} else {
	    Strcpy(saved_oname, cxname(obj));
	    if(obj->oclass == WEAPON_CLASS || is_weptool(obj) ||
	       obj->oclass == GEM_CLASS) {

		/* is it not a melee weapon? */
		if (/* if you strike with a bow... */
		    is_launcher(obj) ||
		    /* or strike with a missile in your hand... */
		    (!thrown && (is_missile(obj) || is_ammo(obj))) ||
		    /* or throw a bullet without a gun... */
		    (is_bullet(obj) && !obj->oshot) ||
		    /* or use a pole at short range and not mounted... */
		    /* (it makes polearms quite useless, so I commented out it... Youkan) */
		    /*(!thrown &&*/
#ifdef STEED
		     /*!u.usteed &&*/
#endif
		     /*is_pole(obj)) ||*/
		    /* or throw a missile without the proper bow... */
		    (is_ammo(obj) && !ammo_and_launcher(obj, uwep))) {
		    /* then do only 1-2 points of damage */
		    if (mdat == &mons[PM_SHADE] && get_material(obj) != SILVER)
			tmp = 0;
		    else
			tmp = rnd(2);
		    if (get_material(obj) == SILVER && hates_silver(mdat)) {
			tmp += rnd(20);
			silvermsg = TRUE;
		    }
		    if (!thrown && obj == uwep && obj->otyp == BOOMERANG &&
			    rnl(4) == 4-1) {
			boolean more_than_1 = (obj->quan > 1L);

#ifndef JP
			pline("As you hit %s, %s%s %s breaks into splinters.",
			      mon_nam(mon), more_than_1 ? "one of " : "",
			      shk_your(yourbuf, obj), xname(obj));
#else
			pline("%sに当たると、%s%s%s砕け散った。",
			      mon_nam(mon), shk_your(yourbuf, obj), xname(obj),
			      more_than_1 ? "のうちの一つが" : "は");
#endif /*JP*/
			if (!more_than_1) uwepgone();	/* set unweapon */
			useup(obj);
			if (!more_than_1) obj = (struct obj *) 0;
			hittxt = TRUE;
			if (mdat != &mons[PM_SHADE])
			    tmp++;
		    }
		} else {
		    tmp = dmgval(obj, mon);
		    /* a minimal hit doesn't exercise proficiency */
		    valid_weapon_attack = (tmp > 1);
		    if (!valid_weapon_attack || mon == u.ustuck || u.twoweap) {
			;	/* no special bonuses */
		    } else if (mon->mflee && Role_if(PM_ROGUE) && !Upolyd) {
			You(E_J("strike %s from behind!",
				"%sの背後をとった！"), mon_nam(mon));
			tmp += rnd(u.ulevel);
			hittxt = TRUE;
		    } else if (obj == uwep &&
			  obj->oclass == WEAPON_CLASS) {
			/*
			 * 2.5% chance of shattering defender's weapon when
			 * using a two-handed weapon; less if uwep is rusted.
			 * [dieroll == 2 is most successful non-beheading or
			 * -bisecting hit, in case of special artifact damage;
			 * the percentage chance is (1/20)*(50/100).]
			 */
			if (dieroll == 2 && (bimanual(obj) ||
			    (Role_if(PM_SAMURAI) && obj->otyp == KATANA && !uarms)) &&
			    ((wtype = weapon_type(uwep)) != P_NONE &&
			    P_SKILL(wtype) >= P_SKILLED) &&
			    ((monwep = MON_WEP(mon)) != 0 &&
			     !is_flimsy(monwep) &&
			     !obj_resists(monwep,
				 50 + 15 * greatest_erosion(obj), 100))) {
			    setmnotwielded(mon,monwep);
			    MON_NOWEP(mon);
			    mon->weapon_check = NEED_WEAPON;
#ifndef JP
			    pline("%s %s %s from the force of your blow!",
				  s_suffix(Monnam(mon)), xname(monwep),
				  otense(monwep, "shatter"));
#else
			    pline("%sの%sはあなたの一撃で粉々に吹き飛んだ！",
				  Monnam(mon), xname(monwep));
#endif /*JP*/
			    m_useup(mon, monwep);
			    /* If someone just shattered MY weapon, I'd flee! */
			    if (rn2(4)) {
				monflee(mon, d(2,3), TRUE, TRUE);
			    }
			    hittxt = TRUE;
			} else if (dieroll <= 3 && is_hammer(uwep) &&
				   has_head(mon->data) && !mindless(mon->data)) {
			    if (dieroll == 2 ||
				(dieroll == 3 && P_SKILL(weapon_type(uwep)) >= P_SKILLED)) {
				stunmon = 1;
			    }
			}
		    }

		    /* artifact(melee weapon) */
		    if (obj->oartifact &&
			(hittxt = artifact_hit(&youmonst, mon, obj, &tmp, dieroll))) {
			if(mon->mhp <= 0) /* artifact killed monster */
			    return FALSE;
			if (tmp == 0) return TRUE;
		    }
		    /* artifact(launcher) */
		    if (thrown && uwep && uwep->oartifact &&
			is_ammo(obj) && ammo_and_launcher(obj, uwep) &&
			(hittxt = artifact_hit(&youmonst, mon, uwep, &tmp, dieroll))) {
			if(mon->mhp <= 0) /* artifact killed monster */
			    return FALSE;
			if (tmp == 0) return TRUE;
		    }
		    if(obj->opoisoned && is_poisonable(obj))
				ispoisoned = TRUE;
		    if (get_material(obj) == SILVER
				&& hates_silver(mdat)) {
			silvermsg = TRUE; silverobj = TRUE;
		    }
#ifdef MONSTEED
		    /* dismount attack */
		    if (is_mriding(mon) && obj == uwep) {
			if (obj->otyp == GRAPPLING_HOOK)
			    pdismount = 60;
			else if (is_pole(obj) &&
				 distu(mon->mx, mon->my) <= 2)
			    pdismount = 30;
			else pdismount = 3;
		    }
#endif /*MONSTEED*/
#ifdef STEED
		    if (u.usteed && !thrown && tmp > 0 &&
			    obj->otyp == LANCE && mon != u.ustuck) {
			jousting = joust(mon, obj);
			/* exercise skill even for minimal damage hits */
			if (jousting) valid_weapon_attack = TRUE;
#ifdef MONSTEED
			if (jousting && is_mriding(mon)) pdismount = 100;
#endif /*MONSTEED*/
		    }
#endif
		    if(thrown && (is_ammo(obj) || is_missile(obj))) {
			if (ammo_and_launcher(obj, uwep)) {
			    /* launcher's enchantment gives extra damage */
			    tmp += uwep->spe;
			    /* Elves and Samurai do extra damage using
			     * their bows&arrows; they're highly trained.
			     */
			    if (Role_if(PM_SAMURAI) &&
				obj->otyp == YA && uwep->otyp == YUMI)
				tmp++;
			    else if (Race_if(PM_ELF) &&
				     obj->otyp == ELVEN_ARROW &&
				     uwep->otyp == ELVEN_BOW)
				tmp++;
			}
		    }
		}
	    } else if(obj->oclass == POTION_CLASS) {
		if (obj->quan > 1L)
		    obj = splitobj(obj, 1L);
		else
		    setuwep((struct obj *)0);
		freeinv(obj);
		potionhit(mon, obj, TRUE);
		if (mon->mhp <= 0) return FALSE;	/* killed */
		hittxt = TRUE;
		/* in case potion effect causes transformation */
		mdat = mon->data;
		tmp = (mdat == &mons[PM_SHADE]) ? 0 : 1;
	    } else {
		if (mdat == &mons[PM_SHADE] && !shade_aware(obj)) {
		    tmp = 0;
		    Strcpy(unconventional, cxname(obj));
		} else {
		    switch(obj->otyp) {
		    case BOULDER: {		/* 1d20 */
			struct obj *mwep;
			if (thrown && (mwep = MON_WEP(mon)) &&
			    mwep->oartifact == ART_GIANTKILLER) {
				if (cansee(mon->mx,mon->my)) {
#ifndef JP
					pline("%s swings %s!",
						Monnam(mon), the(xname(mwep)));
					pline("The boulder is smashed into bits.");
#else
					pline("%sは%sを振るった！",
						Monnam(mon), xname(mwep));
					pline("大岩はこなごなに打ち砕かれた。");
#endif /*JP*/
					newsym(mon->mx,mon->my);
				}
				trans_to_rock(obj);
				tmp = 0;
				hittxt = TRUE;
				break;
			}
		    }	/* fallthru */
		    case HEAVY_IRON_BALL:	/* 1d25 */
		    case IRON_CHAIN:		/* 1d4+1 */
			tmp = dmgval(obj, mon);
			break;
		    case MIRROR:
			if (breaktest(obj)) {
			    You(E_J("break %s mirror.  That's bad luck!",
				    "%s鏡を割ってしまった。なんて縁起の悪い！"),
				shk_your(yourbuf, obj));
			    change_luck(-2);
			    useup(obj);
			    obj = (struct obj *) 0;
			    unarmed = FALSE;	/* avoid obj==0 confusion */
			    get_dmg_bonus = FALSE;
			    hittxt = TRUE;
			}
			tmp = 1;
			break;
#ifdef TOURIST
		    case EXPENSIVE_CAMERA:
			You(E_J("succeed in destroying %s camera.  Congratulations!",
				"%sのカメラを破壊することに成功した。おめでとう！"),
			        shk_your(yourbuf, obj));
			useup(obj);
			return(TRUE);
			/*NOTREACHED*/
			break;
#endif
		    case CORPSE:		/* fixed by polder@cs.vu.nl */
			if (touch_petrifies(&mons[obj->corpsenm])) {
			    static const char withwhat[] = "corpse";
			    tmp = 1;
			    hittxt = TRUE;
#ifndef JP
			    You("hit %s with %s %s.", mon_nam(mon),
				obj->dknown ? the(mons[obj->corpsenm].mname) :
				an(mons[obj->corpsenm].mname),
				(obj->quan > 1) ? makeplural(withwhat) : withwhat);
#else
			    You("%sの死体を%sに叩きつけた。",
				JMONNAM(obj->corpsenm), mon_nam(mon));
#endif /*JP*/
			    if (!munstone(mon, TRUE))
				minstapetrify(mon, TRUE);
			    if (resists_ston(mon)) break;
			    /* note: hp may be <= 0 even if munstoned==TRUE */
			    return (boolean) (mon->mhp > 0);
#if 0
			} else if (touch_petrifies(mdat)) {
			    /* maybe turn the corpse into a statue? */
#endif
			}
			tmp = (obj->corpsenm >= LOW_PM ?
					mons[obj->corpsenm].msize : 0) + 1;
			break;
		    case EGG:
		      {
#define useup_eggs(o)	{ if (thrown) obfree(o,(struct obj *)0); \
			  else useupall(o); \
			  o = (struct obj *)0; }	/* now gone */
			long cnt = obj->quan;

			tmp = 1;		/* nominal physical damage */
			get_dmg_bonus = FALSE;
			hittxt = TRUE;		/* message always given */
			/* egg is always either used up or transformed, so next
			   hand-to-hand attack should yield a "bashing" mesg */
			if (obj == uwep) unweapon = TRUE;
			if (obj->spe && obj->corpsenm >= LOW_PM) {
			    if (obj->quan < 5)
				change_luck((schar) -(obj->quan));
			    else
				change_luck(-5);
			}

			if (touch_petrifies(&mons[obj->corpsenm])) {
			    /*learn_egg_type(obj->corpsenm);*/
#ifndef JP
			    pline("Splat! You hit %s with %s %s egg%s!",
				mon_nam(mon),
				obj->known ? "the" : cnt > 1L ? "some" : "a",
				obj->known ? mons[obj->corpsenm].mname : "petrifying",
				plur(cnt));
#else
			    pline("ベチャ！ %s%s%s卵が%sに命中した！",
				cnt > 1L ? "いくつかの" : "",
				obj->known ? JMONNAM(obj->corpsenm) : "石化する",
				obj->known ? "の" : "",
				mon_nam(mon));
#endif /*JP*/
			    obj->known = 1;	/* (not much point...) */
			    useup_eggs(obj);
			    if (!munstone(mon, TRUE))
				minstapetrify(mon, TRUE);
			    if (resists_ston(mon)) break;
			    return (boolean) (mon->mhp > 0);
			} else {	/* ordinary egg(s) */
			    const char *eggp =
#ifndef JP
				     (obj->corpsenm != NON_PM && obj->known) ?
					      the(mons[obj->corpsenm].mname) :
					      (cnt > 1L) ? "some" : "an";
#else
				     (obj->corpsenm != NON_PM && obj->known) ?
					      JMONNAM(obj->corpsenm) :
					      (cnt > 1L) ? "いくつかの" : "";
#endif /*JP*/
#ifndef JP
			    You("hit %s with %s egg%s.",
				mon_nam(mon), eggp, plur(cnt));
#else
			    You("%sを%s卵で攻撃した。",
				mon_nam(mon), eggp);
#endif /*JP*/
			    if (touch_petrifies(mdat) && !stale_egg(obj)) {
#ifndef JP
				pline_The("egg%s %s alive any more...",
				      plur(cnt),
				      (cnt == 1L) ? "isn't" : "aren't");
#else
				pline("卵はもう生きていないようだ…。");
#endif /*JP*/
				if (obj->timed) obj_stop_timers(obj);
				obj->otyp = ROCK;
				obj->oclass = GEM_CLASS;
				obj->oartifact = 0;
				obj->spe = 0;
				obj->known = obj->dknown = obj->bknown = 0;
				obj->owt = weight(obj);
				if (thrown) place_object(obj, mon->mx, mon->my);
			    } else {
				pline(E_J("Splat!","ベチャ！"));
				useup_eggs(obj);
				exercise(A_WIS, FALSE);
			    }
			}
			break;
#undef useup_eggs
		      }
		    case CLOVE_OF_GARLIC:	/* no effect against demons */
			if (is_undead(mdat)) {
			    monflee(mon, d(2, 4), FALSE, TRUE);
			}
			tmp = 1;
			break;
		    case CREAM_PIE:
		    case BLINDING_VENOM:
			mon->msleeping = 0;
			if (can_blnd(&youmonst, mon, (uchar)
				    (obj->otyp == BLINDING_VENOM
				     ? AT_SPIT : AT_WEAP), obj)) {
			    if (Blind) {
				pline(obj->otyp == CREAM_PIE ?
				      E_J("Splat!","ベチャ！") : E_J("Splash!","ビシャ！"));
			    } else if (obj->otyp == BLINDING_VENOM) {
				pline_The(E_J("venom blinds %s%s!","毒液が%sの%s目を潰した！"), mon_nam(mon),
					  mon->mcansee ? "" : E_J(" further","さらに"));
			    } else {
				char *whom = mon_nam(mon);
				char *what = E_J(The(xname(obj)), xname(obj));
#ifndef JP
				if (!thrown && obj->quan > 1)
				    what = An(singular(obj, xname));
#endif /*JP*/
				/* note: s_suffix returns a modifiable buffer */
				if (haseyes(mdat)
				    && mdat != &mons[PM_FLOATING_EYE])
#ifndef JP
				    whom = strcat(strcat(s_suffix(whom), " "),
						  mbodypart(mon, FACE));
				pline("%s %s over %s!",
				      what, vtense(what, "splash"), whom);
#else
				pline("%sの%sに%sが命中した！",
				      whom, mbodypart(mon, FACE), what);
#endif /*JP*/
			    }
			    setmangry(mon);
			    mon->mcansee = 0;
			    tmp = rn1(25, 21);
			    if(((int) mon->mblinded + tmp) > 127)
				mon->mblinded = 127;
			    else mon->mblinded += tmp;
			} else {
			    pline(obj->otyp==CREAM_PIE ? E_J("Splat!","ベチャ！") : E_J("Splash!","ビシャ！"));
			    setmangry(mon);
			}
			if (thrown) obfree(obj, (struct obj *)0);
			else useup(obj);
			hittxt = TRUE;
			get_dmg_bonus = FALSE;
			tmp = 0;
			break;
		    case ACID_VENOM: /* thrown (or spit) */
			if (resists_acid(mon)) {
				Your(E_J("venom hits %s harmlessly.",
					 "毒液は%sに命中したが、効いていない。"),
					mon_nam(mon));
				tmp = 0;
			} else {
				Your(E_J("venom burns %s!","毒液が%sを灼いた！"), mon_nam(mon));
				tmp = dmgval(obj, mon);
			}
			if (thrown) obfree(obj, (struct obj *)0);
			else useup(obj);
			hittxt = TRUE;
			get_dmg_bonus = FALSE;
			break;
#ifdef FIRSTAID
		    case SCISSORS:
			if (!thrown &&
			    (mdat == &mons[PM_ROPE_GOLEM] ||
			     mdat == &mons[PM_PAPER_GOLEM])) {
				You(E_J("cut %s!","%sを切った！"), mon_nam(mon));
				tmp = rnd(8);
				hittxt = TRUE;
				get_dmg_bonus = FALSE;
			} else tmp = 1;
			break;
#endif /*FIRSTAID*/
		    default:
			/* non-weapons can damage because of their weight */
			/* (but not too much) */
			tmp = obj->owt/100;
			if(tmp < 1) tmp = 1;
			else tmp = rnd(tmp);
			if(tmp > 6) tmp = 6;
			/*
			 * Things like silver wands can arrive here so
			 * so we need another silver check.
			 */
			if (get_material(obj) == SILVER
						&& hates_silver(mdat)) {
				tmp += rnd(20);
				silvermsg = TRUE; silverobj = TRUE;
			}
		    }
		}
	    }
	}

	/****** NOTE: perhaps obj is undefined!! (if !thrown && BOOMERANG)
	 *      *OR* if attacking bare-handed!! */

	if (get_dmg_bonus && tmp > 0) {
		tmp += u.udaminc;
		/* If you throw using a propellor, you don't get a strength
		 * bonus but you do get an increase-damage bonus.
		 */
		if(!thrown || !obj || !uwep || !ammo_and_launcher(obj, uwep))
		    tmp += dbon();
	}

	if (valid_weapon_attack) {
	    struct obj *wep;

	    /* to be valid a projectile must have had the correct projector */
	    wep = PROJECTILE(obj) ? uwep : obj;
//	    tmp += weapon_dam_bonus(wep);
	    /* [this assumes that `!thrown' implies wielded...] */
	    use_skill(weapon_type(wep), 1);
	    if (u.twoweap && !thrown) use_skill(P_TWO_WEAPON_COMBAT, 1);
	}

	/* 
	 * Ki special ability, see cmd.c in function special_ability. 
	 * In this case, we do twice damage! Wow!
	 *
	 * Beseark special ability only does +4 damage. - SW
	 */
	if (Role_special(PM_SAMURAI)  ) tmp *= 2;
	if (Role_special(PM_BARBARIAN)) tmp += 4;


	if (ispoisoned) {
	    int nopoison = 2;
	    if (obj) {
		nopoison = (obj->oartifact == ART_GRIMTOOTH) ? 1000 :
			   (objects[obj->otyp].oc_merge) ? (10 - (obj->owt/10)) : 100;
		if(nopoison < 2) nopoison = 2;
	    }
	    if Role_if(PM_SAMURAI) {
		You(E_J("dishonorably use a poisoned weapon!",
			"卑怯にも毒を塗った武器を用いた！"));
		adjalign(-sgn(u.ualign.type));
	    } else if ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10)) {
		You_feel(E_J("like an evil coward for using a poisoned weapon.",
			     "毒の武器を使うのは卑怯で邪悪だと感じた。"));
		adjalign(-1);
	    }
	    if (obj && !rn2(nopoison)) {
		obj->opoisoned = FALSE;
#ifndef JP
		Your("%s %s no longer poisoned.", xname(obj),
		     otense(obj, "are"));
#else
		Your("%sの毒は消え落ちた。", xname(obj));
#endif /*JP*/
	    }
	    if (resists_poison(mon))
		needpoismsg = TRUE;
	    else if (rn2(10))
		tmp += rnd(6);
	    else poiskilled = TRUE;
	}
	if (tmp < 1) {
	    /* make sure that negative damage adjustment can't result
	       in inadvertently boosting the victim's hit points */
	    tmp = 0;
	    if (mdat == &mons[PM_SHADE]) {
		if (!hittxt) {
		    const char *what = unconventional[0] ? unconventional : E_J("attack","攻撃");
#ifndef JP
		    Your("%s %s harmlessly through %s.",
		    	what, vtense(what, "pass"),
			mon_nam(mon));
#else
		    Your("%sは%sを傷つけることなく通り抜けた。",
			what, mon_nam(mon));
#endif /*JP*/
		    hittxt = TRUE;
		}
	    } else {
		if (get_dmg_bonus) tmp = 1;
	    }
	}

#ifdef STEED
	if (jousting) {
	    tmp += d(2, (obj == uwep) ? 10 : 2);	/* [was in dmgval()] */
	    You(E_J("joust %s%s","%sに突撃した%s"),
			 mon_nam(mon), canseemon(mon) ? exclam(tmp) : E_J(".","。"));
	    if (jousting < 0) {
		Your(E_J("%s shatters on impact!",
			 "%sは衝撃で砕け散った！"), xname(obj));
		/* (must be either primary or secondary weapon to get here) */
		u.twoweap = FALSE;	/* untwoweapon() is too verbose here */
		if (obj == uwep) uwepgone();		/* set unweapon */
		/* minor side-effect: broken lance won't split puddings */
		useup(obj);
		obj = 0;
	    }
	    /* avoid migrating a dead monster */
	    if (mon->mhp > tmp) {
		mhurtle(mon, u.dx, u.dy, 1);
		mdat = mon->data; /* in case of a polymorph trap */
		    mdat = mon->data; /* in case of a polymorph trap */
		if (DEADMONSTER(mon)) already_killed = TRUE;
	    }
	    hittxt = TRUE;
	} else
#endif

//	if (!hittxt) { /* test... */
//	    tmp = reduce_mdmg(mon, tmp, TRUE);
//	    hittxt = !tmp;
//	}

	/* VERY small chance of stunning opponent if unarmed. */
	if (unarmed && tmp > 1 && !thrown && !obj && !Upolyd &&
	    rnd(2500) < P_SKILL(P_BARE_HANDED_COMBAT) &&
	    !bigmonst(mdat) && !thick_skinned(mdat)) {
		stunmon = 2;	/* stun & hurtle */
	}
	if (!thrown && obj && (obj == uarms)) stunmon = 2;	/* stun & hurtle */

	/* stunning opponent (martial arts, maces, shields... ) */
	if (tmp > 1 && stunmon > 0) {
		if (canspotmon(mon))
#ifndef JP
		    pline("%s %s from your %sstrike!", Monnam(mon),
			  makeplural(stagger(mon->data, "stagger")),
			  (tmp > 9) ? "powerful " : "");
#else
		    pline("%sはあなたの%s一撃に%s！", Monnam(mon),
			  (tmp > 9) ? "強力な" : "",
			  stagger(mon->data, "よろめいた"));
#endif /*JP*/
		mon->movement = 0;
		/* avoid migrating a dead monster */
		if (stunmon > 1 && mon->mhp > tmp) {
		    mhurtle(mon, u.dx, u.dy, 1);
		    if (DEADMONSTER(mon)) already_killed = TRUE;
		}
		hittxt = TRUE;
	}

	if (!already_killed) mon->mhp -= tmp;
	/* adjustments might have made tmp become less than what
	   a level draining artifact has already done to max HP */
	if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
	if (mon->mhp < 1)
		destroyed = TRUE;
	if (mon->mtame && (!mon->mflee || mon->mfleetim) && tmp > 0) {
		abuse_dog(mon);
		monflee(mon, 10 * rnd(tmp), FALSE, FALSE);
	}
	if((mdat == &mons[PM_BLACK_PUDDING] || mdat == &mons[PM_BROWN_PUDDING])
		   && obj && obj == uwep
		   && get_material(obj) == IRON
		   && mon->mhp > 1 && !thrown && !mon->mcan
		   /* && !destroyed  -- guaranteed by mhp > 1 */ ) {
		struct monst *mon2;
		mon2 = clone_mon(mon, 0, 0);
		if (mon2) {
			pline(E_J("%s divides as you hit it!",
				  "%sはあなたの攻撃で分裂した！"), Monnam(mon));
			hittxt = TRUE;
			mon->mhpmax  /= 2;
			mon2->mhpmax /= 2;
		}
	}

	if (!hittxt &&			/*( thrown => obj exists )*/
	  (!destroyed || (thrown && m_shot.n > 1 && m_shot.o == obj->otyp))) {
		if (thrown) hit(mshot_xname(obj), mon, exclam(tmp));
		else if (!flags.verbose) E_J(You("hit it."),pline("攻撃した"));
#ifndef JP
		else You("%s %s%s", Role_if(PM_BARBARIAN) ? "smite" : "hit",
			 mon_nam(mon), canseemon(mon) ? exclam(tmp) : ".");
#else
		else You("%sを%sした%s", mon_nam(mon),
			 Role_if(PM_BARBARIAN) ? "強打" : "攻撃",
			 canseemon(mon) ? exclam(tmp) : "。");
#endif /*JP*/
	}

	if (silvermsg) {
		const char *fmt;
		char *whom = mon_nam(mon);
		char silverobjbuf[BUFSZ];

#ifndef JP
		if (canspotmon(mon)) {
		    if (barehand_silver_rings == 1)
			fmt = "Your silver ring sears %s!";
		    else if (barehand_silver_rings == 2)
			fmt = "Your silver rings sear %s!";
		    else if (silverobj && saved_oname[0]) {
		    	Sprintf(silverobjbuf, "Your %s%s %s %%s!",
		    		strstri(saved_oname, "silver") ?
					"" : "silver ",
				saved_oname, vtense(saved_oname, "sear"));
		    	fmt = silverobjbuf;
		    } else
			fmt = "The silver sears %s!";
		} else {
		    *whom = highc(*whom);	/* "it" -> "It" */
		    fmt = "%s is seared!";
		}
#else
		if (canspotmon(mon)) {
		    if (barehand_silver_rings)
			fmt = "あなたの銀の指輪が%sを焼いた！";
		    else if (silverobj && saved_oname[0]) {
		    	Sprintf(silverobjbuf, "あなたの%s%sが%%sを焼いた！",
		    		strstri(saved_oname, "銀") ? "" : "銀の",
				saved_oname);
		    	fmt = silverobjbuf;
		    } else
			fmt = "銀が%sを焼いた！";
		} else {
		    fmt = "%sは焼かれた！";
		}
#endif /*JP*/
		/* note: s_suffix returns a modifiable buffer */
		if (!noncorporeal(mdat))
		    whom = strcat(s_suffix(whom), E_J(" flesh","身体"));
		pline(fmt, whom);
	}
#ifdef SHOWDMG
	if (flags.showdmg && !destroyed) printdmg(tmp);
#endif
	if (needpoismsg)
		pline_The(E_J("poison doesn't seem to affect %s.",
			      "毒は%sには効いていないようだ。"), mon_nam(mon));
	if (poiskilled) {
		pline_The(E_J("poison was deadly...","毒は致死的だった…。"));
		if (!already_killed) xkilled(mon, 0);
		return FALSE;
	} else if (destroyed) {
		/* takes care of most messages */
		if (!already_killed)	/* takes care of most messages */
		    killed_showdmg(mon, (tmp < FATAL_DAMAGE_MODIFIER) ? tmp : 0);
	} else if(u.umconf && !thrown) {
		nohandglow(mon);
		if (!mon->mconf && !resist(mon, SPBOOK_CLASS, 0, NOTELL)) {
			mon->mconf = 1;
			if (!mon->mstun && mon->mcanmove && !mon->msleeping &&
				canseemon(mon))
			    pline(E_J("%s appears confused.",
				      "%sは混乱したようだ。"), Monnam(mon));
		}
	}
#ifdef MONSTEED
	if (!destroyed && pdismount && valid_weapon_attack && obj &&
	    (rn2(100) < (P_SKILL(weapon_type(obj)) * pdismount / 100)) &&
	    mlanding_spot(mon, 0))
		destroyed = !mdismount_steed(mon);
#endif

	return((boolean)(destroyed ? FALSE : TRUE));
}

STATIC_OVL boolean
shade_aware(obj)
struct obj *obj;
{
	if (!obj) return FALSE;
	/*
	 * The things in this list either
	 * 1) affect shades.
	 *  OR
	 * 2) are dealt with properly by other routines
	 *    when it comes to shades.
	 */
	if (obj->otyp == BOULDER || obj->otyp == HEAVY_IRON_BALL
	    || obj->otyp == IRON_CHAIN		/* dmgval handles those first three */
	    || obj->otyp == MIRROR		/* silver in the reflective surface */
	    || obj->otyp == CLOVE_OF_GARLIC	/* causes shades to flee */
	    || get_material(obj) == SILVER)
		return TRUE;
	return FALSE;
}

/* check whether slippery clothing protects from hug or wrap attack */
/* [currently assumes that you are the attacker] */
STATIC_OVL boolean
m_slips_free(mdef, mattk)
struct monst *mdef;
struct attack *mattk;
{
	struct obj *obj;

	if (mattk->adtyp == AD_DRIN) {
	    /* intelligence drain attacks the head */
	    obj = which_armor(mdef, W_ARMH);
	} else {
	    /* grabbing attacks the body */
	    obj = which_armor(mdef, W_ARMC);		/* cloak */
	    if (!obj) obj = which_armor(mdef, W_ARM);	/* suit */
#ifdef TOURIST
	    if (!obj) obj = which_armor(mdef, W_ARMU);	/* shirt */
#endif
	}

	/* if your cloak/armor is greased, monster slips off; this
	   protection might fail (33% chance) when the armor is cursed */
	if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK) &&
		(!obj->cursed || rn2(3))) {
#ifndef JP
	    You("%s %s %s %s!",
		mattk->adtyp == AD_WRAP ?
			"slip off of" : "grab, but cannot hold onto",
		s_suffix(mon_nam(mdef)),
		obj->greased ? "greased" : "slippery",
		/* avoid "slippery slippery cloak"
		   for undiscovered oilskin cloak */
		(obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : cloak_simple_name(obj));
#else
	    You("%s%sの%s%s%s滑ってしまった！",
		mattk->adtyp == AD_WRAP ? "" : "掴みかかったが、",
		mon_nam(mdef),
		obj->greased ? "脂ぎった" : "つるつるの",
		/* avoid "slippery slippery cloak"
		   for undiscovered oilskin cloak */
		(obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : cloak_simple_name(obj),
		mattk->adtyp == AD_WRAP ?
			"の上を" : "で");
#endif /*JP*/

	    if (obj->greased && !rn2(2)) {
		pline_The(E_J("grease wears off.","脂は消え落ちた。"));
		obj->greased = 0;
	    }
	    return TRUE;
	}
	return FALSE;
}

/* used when hitting a monster with a lance while mounted */
STATIC_OVL int	/* 1: joust hit; 0: ordinary hit; -1: joust but break lance */
joust(mon, obj)
struct monst *mon;	/* target */
struct obj *obj;	/* weapon */
{
    int skill_rating, joust_dieroll;

    if (Fumbling || Stunned) return 0;
    /* sanity check; lance must be wielded in order to joust */
    if (obj != uwep && (obj != uswapwep || !u.twoweap)) return 0;

    /* if using two weapons, use worse of lance and two-weapon skills */
    skill_rating = P_SKILL(weapon_type(obj));	/* lance skill */
    if (u.twoweap && P_SKILL(P_TWO_WEAPON_COMBAT) < skill_rating)
	skill_rating = P_SKILL(P_TWO_WEAPON_COMBAT);
    if (skill_rating == P_ISRESTRICTED) skill_rating = P_UNSKILLED; /* 0=>1 */

    /* odds to joust are expert:80%, skilled:60%, basic:40%, unskilled:20% */
    if ((joust_dieroll = rn2(120)) < skill_rating) {
	if (joust_dieroll == 0 && rnl(50) == (50-1) &&
		!unsolid(mon->data) && !obj_resists(obj, 0, 100))
	    return -1;	/* hit that breaks lance */
	return 1;	/* successful joust */
    }
    return 0;	/* no joust bonus; revert to ordinary attack */
}

/*
 * Send in a demon pet for the hero.  Exercise wisdom.
 *
 * This function used to be inline to damageum(), but the Metrowerks compiler
 * (DR4 and DR4.5) screws up with an internal error 5 "Expression Too Complex."
 * Pulling it out makes it work.
 */
STATIC_OVL void
demonpet()
{
	int i;
	struct permonst *pm;
	struct monst *dtmp;

	pline(E_J("Some hell-p has arrived!","地獄から援軍が来た！"));
	i = !rn2(6) ? ndemon(u.ualign.type) : NON_PM;
	pm = i != NON_PM ? &mons[i] : youmonst.data;
	if ((dtmp = makemon(pm, u.ux, u.uy, NO_MM_FLAGS)) != 0)
	    (void)tamedog(dtmp, (struct obj *)0);
	exercise(A_WIS, TRUE);
}

/*
 * Player uses theft attack against monster.
 *
 * If the target is wearing body armor, take all of its possesions;
 * otherwise, take one object.  [Is this really the behavior we want?]
 *
 * This routine implicitly assumes that there is no way to be able to
 * resist petfication (ie, be polymorphed into a xorn or golem) at the
 * same time as being able to steal (poly'd into nymph or succubus).
 * If that ever changes, the check for touching a cockatrice corpse
 * will need to be smarter about whether to break out of the theft loop.
 */
STATIC_OVL void
steal_it(mdef, mattk)
struct monst *mdef;
struct attack *mattk;
{
	struct obj *otmp, *stealoid, **minvent_ptr;
	long unwornmask;

	if (!mdef->minvent) return;		/* nothing to take */

	/* look for worn body armor */
	stealoid = (struct obj *)0;
	if (could_seduce(&youmonst, mdef, mattk)) {
	    /* find armor, and move it to end of inventory in the process */
	    minvent_ptr = &mdef->minvent;
	    while ((otmp = *minvent_ptr) != 0)
		if (otmp->owornmask & W_ARM) {
		    if (stealoid) panic("steal_it: multiple worn suits");
		    *minvent_ptr = otmp->nobj;	/* take armor out of minvent */
		    stealoid = otmp;
		    stealoid->nobj = (struct obj *)0;
		} else {
		    minvent_ptr = &otmp->nobj;
		}
	    *minvent_ptr = stealoid;	/* put armor back into minvent */
	}

	if (stealoid) {		/* we will be taking everything */
	    if (gender(mdef) == (int) u.mfemale &&
			youmonst.data->mlet == S_NYMPH)
		You(E_J("charm %s.  She gladly hands over her possessions.",
			"%sを魅了した。彼女は喜んで自分の所持品を差し出した。"),
		    mon_nam(mdef));
	    else
#ifndef JP
		You("seduce %s and %s starts to take off %s clothes.",
		    mon_nam(mdef), mhe(mdef), mhis(mdef));
#else
		You("%sを誘惑した。%sは自分の服を脱ぎはじめた。",
		    mon_nam(mdef), mhe(mdef));
#endif /*JP*/
	}

	while ((otmp = mdef->minvent) != 0) {
	    if (!Upolyd) break;		/* no longer have ability to steal */
	    /* take the object away from the monster */
	    obj_extract_self(otmp);
	    if ((unwornmask = otmp->owornmask) != 0L) {
		mdef->misc_worn_check &= ~unwornmask;
		if (otmp->owornmask & W_WEP) {
		    setmnotwielded(mdef,otmp);
		    MON_NOWEP(mdef);
		}
		otmp->owornmask = 0L;
		update_mon_intrinsics(mdef, otmp, FALSE, FALSE);

		if (otmp == stealoid)	/* special message for final item */
		    pline(E_J("%s finishes taking off %s suit.",
			      "%sは%s装備を外し終わった。"),
			  Monnam(mdef), mhis(mdef));
	    }
	    /* give the object to the character */
	    if (vanish_ether_obj(otmp, (struct monst *)0, "steal"))
		goto after_theft_chk;
	    otmp = hold_another_object(otmp, E_J("You snatched but dropped %s.",
						 "あなたは%sを取り上げようとして落としてしまった。"),
				       doname(otmp), E_J("You steal: ","あなたは盗んだ: "));
	    if (otmp->where != OBJ_INVENT) goto after_theft_chk;
	    if (otmp->otyp == CORPSE &&
		    touch_petrifies(&mons[otmp->corpsenm]) && !uarmg) {
		char kbuf[BUFSZ];

#ifndef JP
		Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);
#else
		Sprintf(kbuf, "%sの死体を盗んで", JMONNAM(otmp->corpsenm));
#endif /*JP*/
		instapetrify(kbuf);
		break;		/* stop the theft even if hero survives */
	    }
	    /* more take-away handling, after theft message */
after_theft_chk:
	    if (unwornmask & W_WEP) {		/* stole wielded weapon */
		possibly_unwield(mdef, FALSE);
	    } else if (unwornmask & W_ARMG) {	/* stole worn gloves */
		mselftouch(mdef, (const char *)0, TRUE);
		if (mdef->mhp <= 0)	/* it's now a statue */
		    return;		/* can't continue stealing */
	    }

	    if (!stealoid) break;	/* only taking one item */
	}
}

int
damageum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register struct permonst *pd = mdef->data;
	register int	tmp = d((int)mattk->damn, (int)mattk->damd);
	boolean negated;

	/* since hero can't be cancelled, only defender's armor applies */
	negated = magic_negation(mdef);

	if (is_demon(youmonst.data) && !rn2(13) && !uwep
		&& u.umonnum != PM_SUCCUBUS && u.umonnum != PM_INCUBUS
		&& u.umonnum != PM_BALROG) {
	    demonpet();
	    return(0);
	}
	switch(mattk->adtyp) {
	    case AD_STUN:
		if(!Blind)
#ifndef JP
		    pline("%s %s for a moment.", Monnam(mdef),
			  makeplural(stagger(mdef->data, "stagger")));
#else
		    pline("%sは一瞬%s。", Monnam(mdef),
			  stagger(mdef->data, "よろめいた"));
#endif /*JP*/
		mdef->mstun = 1;
		goto physical;
	    case AD_LEGS:
	     /* if (u.ucancelled) { */
	     /*    tmp = 0;	    */
	     /*    break;	    */
	     /* }		    */
		goto physical;
	    case AD_WERE:	    /* no special effect on monsters */
	    case AD_HEAL:	    /* likewise */
	    case AD_PHYS:
 physical:
		if(mattk->aatyp == AT_WEAP) {
		    if(uwep) tmp = 0;
		} else if(mattk->aatyp == AT_KICK) {
		    if(thick_skinned(mdef->data)) tmp = 0;
		    if(mdef->data == &mons[PM_SHADE]) {
			if (!(uarmf && uarmf->blessed)) {
			    impossible("bad shade attack function flow?");
			    tmp = 0;
			} else
			    tmp = rnd(4); /* bless damage */
		    }
		}
		break;
	    case AD_FIRE:
//		if (negated) {
//		    tmp = 0;
//		    break;
//		}
		if (!Blind)
		    pline(E_J("%s is %s!","%s%s！"), Monnam(mdef),
			  on_fire(mdef->data, mattk));
		if (pd == &mons[PM_STRAW_GOLEM] ||
		    pd == &mons[PM_PAPER_GOLEM]) {
		    if (!Blind)
			pline(E_J("%s burns completely!",
				  "%sは燃え尽きた！"), Monnam(mdef));
		    xkilled(mdef,2);
		    tmp = 0;
		    break;
		    /* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		}
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		if (resists_fire(mdef)) {
		    if (!Blind)
			pline_The(E_J("fire doesn't heat %s!",
				      "%sは炎に焼かれなかった！"), mon_nam(mdef));
		    golemeffects(mdef, AD_FIRE, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
		}
		/* only potions damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
		break;
	    case AD_COLD:
//		if (negated) {
//		    tmp = 0;
//		    break;
//		}
		if (!Blind) pline(E_J("%s is covered in frost!",
				      "%sは氷に覆われた！"), Monnam(mdef));
		if (resists_cold(mdef)) {
		    shieldeff(mdef->mx, mdef->my);
		    if (!Blind)
			pline_The(E_J("frost doesn't chill %s!",
				      "%sは凍えなかった！"), mon_nam(mdef));
		    golemeffects(mdef, AD_COLD, tmp);
		    tmp = 0;
		}
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
		break;
	    case AD_ELEC:
//		if (negated) {
//		    tmp = 0;
//		    break;
//		}
		if (elem_hits_shield(mdef, AD_ELEC, E_J("zap", "電撃"))) {
		    tmp = 0;
		    break;
		}
		if (!Blind) pline(E_J("%s is zapped!","%sは電撃をくらった！"), Monnam(mdef));
		tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
		if (resists_elec(mdef)) {
		    if (!Blind)
			pline_The(E_J("zap doesn't shock %s!",
				      "%sは感電しなかった！"), mon_nam(mdef));
		    golemeffects(mdef, AD_ELEC, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
		}
		/* only rings damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
		break;
	    case AD_ACID:
		if (resists_acid(mdef)) tmp = 0;
		break;
	    case AD_STON:
		if (!munstone(mdef, TRUE))
		    minstapetrify(mdef, TRUE);
		tmp = 0;
		break;
#ifdef SEDUCE
	    case AD_SSEX:
#endif
	    case AD_SEDU:
	    case AD_SITM:
		steal_it(mdef, mattk);
		tmp = 0;
		break;
	    case AD_SGLD:
#ifndef GOLDOBJ
		if (mdef->mgold) {
		    u.ugold += mdef->mgold;
		    mdef->mgold = 0;
		    Your(E_J("purse feels heavier.","財布は重くなった。"));
		}
#else
                /* This you as a leprechaun, so steal
                   real gold only, no lesser coins */
	        {
		    struct obj *mongold = findgold(mdef->minvent);
	            if (mongold) {
		        obj_extract_self(mongold);  
		        if (merge_choice(invent, mongold) || inv_cnt() < 52) {
			    addinv(mongold);
			    Your(E_J("purse feels heavier.",
				     "財布は重くなった。"));
			} else {
                            You(E_J("grab %s's gold, but find no room in your knapsack.",
				    "%sの金を奪ったが、背負い袋はもういっぱいだ。"), mon_nam(mdef));
			    dropy(mongold);
		        }
		    }
	        }
#endif
		exercise(A_DEX, TRUE);
		tmp = 0;
		break;
	    case AD_TLPT:
		if (tmp <= 0) tmp = 1;
		if (!negated && tmp < mdef->mhp) {
		    char nambuf[BUFSZ];
		    boolean u_saw_mon = canseemon(mdef) ||
					(u.uswallow && u.ustuck == mdef);
		    /* record the name before losing sight of monster */
		    Strcpy(nambuf, Monnam(mdef));
		    if (u_teleport_mon(mdef, FALSE) &&
			    u_saw_mon && !canseemon(mdef))
			pline(E_J("%s suddenly disappears!","%sは突然消えた！"), nambuf);
		}
		break;
	    case AD_BLND:
		if (can_blnd(&youmonst, mdef, mattk->aatyp, (struct obj*)0)) {
		    if(!Blind && mdef->mcansee)
			pline(E_J("%s is blinded.","%sは目が見えなくなった。"), Monnam(mdef));
		    mdef->mcansee = 0;
		    tmp += mdef->mblinded;
		    if (tmp > 127) tmp = 127;
		    mdef->mblinded = tmp;
		}
		tmp = 0;
		break;
	    case AD_CURS:
		if (night() && !rn2(10) && !mdef->mcan) {
		    if (mdef->data == &mons[PM_CLAY_GOLEM]) {
			if (!Blind)
			    pline(E_J("Some writing vanishes from %s head!",
				      "何かの文字が%s頭から消えた！"),
				s_suffix(mon_nam(mdef)));
			xkilled(mdef, 0);
			/* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		    } else {
			mdef->mcan = 1;
			You(E_J("chuckle.","薄笑いを浮かべた。"));
		    }
		}
		tmp = 0;
		break;
	    case AD_DRLI:
		if (!negated && !rn2(3) && !resists_drli(mdef)) {
			int xtmp = d(2,6);
			pline(E_J("%s suddenly seems weaker!",
				  "%sは突然弱くなった！"), Monnam(mdef));
			mdef->mhpmax -= xtmp;
			if ((mdef->mhp -= xtmp) <= 0 || !mdef->m_lev) {
				pline("%s dies!", Monnam(mdef));
				xkilled(mdef,0);
			} else
				mdef->m_lev--;
			tmp = 0;
		}
		break;
	    case AD_RUST:
		if (pd == &mons[PM_IRON_GOLEM]) {
			pline(E_J("%s falls to pieces!","%sはばらばらに崩れた！"), Monnam(mdef));
			xkilled(mdef,0);
		}
		hurtmarmor(mdef, AD_RUST);
//		tmp = 0;
		break;
	    case AD_CORR:
		hurtmarmor(mdef, AD_CORR);
		tmp = 0;
		break;
	    case AD_DCAY:
		if (pd == &mons[PM_WOOD_GOLEM] ||
		    pd == &mons[PM_LEATHER_GOLEM]) {
			pline(E_J("%s falls to pieces!","%sはばらばらに朽ち果てた！"), Monnam(mdef));
			xkilled(mdef,0);
		}
		hurtmarmor(mdef, AD_DCAY);
		tmp = 0;
		break;
	    case AD_DRST:
	    case AD_DRDX:
	    case AD_DRCO:
		if (!negated && !rn2(8)) {
		    Your(E_J("%s was poisoned!","%sは毒を帯びている！"), mpoisons_subj(&youmonst, mattk));
		    if (resists_poison(mdef))
			pline_The(E_J("poison doesn't seem to affect %s.",
				      "毒は%sには効かないようだ。"),
				mon_nam(mdef));
		    else {
			if (!rn2(10)) {
			    Your(E_J("poison was deadly...","毒は致死的だった…。"));
			    tmp = mdef->mhp;
			} else tmp += rn1(10,6);
		    }
		}
		break;
	    case AD_DRIN:
		if (notonhead || !has_head(mdef->data)) {
		    pline(E_J("%s doesn't seem harmed.",
			      "%sは平気のようだ。"), Monnam(mdef));
		    tmp = 0;
		    if (!Unchanging && mdef->data == &mons[PM_GREEN_SLIME]) {
			if (!Slimed) {
			    You(E_J("suck in some slime and don't feel very well.",
				    "スライムを吸ってしまい、とても気分が悪くなった。"));
			    Slimed = 10L;
			}
		    }
		    break;
		}
		if (m_slips_free(mdef, mattk)) break;

		if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
		    pline(E_J("%s helmet blocks your attack to %s head.",
			      "%sヘルメットが、%s頭への攻撃を防いだ。"),
			  s_suffix(Monnam(mdef)), mhis(mdef));
		    break;
		}

		You(E_J("eat %s brain!","%s脳を喰らった！"), s_suffix(mon_nam(mdef)));
		u.uconduct.food++;
		if (touch_petrifies(mdef->data) && !Stone_resistance && !Stoned) {
#ifdef JP
		    Sprintf(dkiller_buf, "%sの脳に喰らいついて", JMONNAM(monsndx(mdef->data)));
#endif /*JP*/
		    Stoned = 5;
		    killer_format = KILLED_BY_AN;
		    delayed_killer = E_J(mdef->data->mname, dkiller_buf);
		}
		if (!vegan(mdef->data))
		    u.uconduct.unvegan++;
		if (!vegetarian(mdef->data))
		    violated_vegetarian();
		if (mindless(mdef->data)) {
		    pline(E_J("%s doesn't notice.","%sは気にかけていない。"), Monnam(mdef));
		    break;
		}
		tmp += rnd(10);
		morehungry(-rnd(30)); /* cannot choke */
		if (ABASE(A_INT) < AMAX(A_INT)) {
			ABASE(A_INT) += rnd(4);
			if (ABASE(A_INT) > AMAX(A_INT))
				ABASE(A_INT) = AMAX(A_INT);
			flags.botl = 1;
		}
		exercise(A_WIS, TRUE);
		break;
	    case AD_STCK:
		if (!negated && !sticks(mdef->data))
		    u.ustuck = mdef; /* it's now stuck to you */
		break;
	    case AD_WRAP:
		if (!sticks(mdef->data)) {
		    if (!u.ustuck && !rn2(10)) {
			if (m_slips_free(mdef, mattk)) {
			    tmp = 0;
			} else {
			    You(E_J("swing yourself around %s!",
				    "身体をしならせ、%sに巻きついた！"),
				  mon_nam(mdef));
			    u.ustuck = mdef;
			}
		    } else if(u.ustuck == mdef) {
			/* Monsters don't wear amulets of magical breathing */
			if (is_pool(u.ux,u.uy) && !is_swimmer(mdef->data) &&
			    !amphibious(mdef->data)) {
			    You(E_J("drown %s...","%sを溺れさせた…。"), mon_nam(mdef));
			    tmp = mdef->mhp;
			} else if(mattk->aatyp == AT_HUGS)
			    pline(E_J("%s is being crushed.","%sは押し潰されている。"), Monnam(mdef));
		    } else {
			tmp = 0;
			if (flags.verbose)
			    You(E_J("brush against %s %s.","%s%sをかすった。"),
				s_suffix(mon_nam(mdef)),
				mbodypart(mdef, LEG));
		    }
		} else tmp = 0;
		break;
	    case AD_PLYS:
		if (!negated && mdef->mcanmove && !rn2(3) && tmp < mdef->mhp) {
		    if (!Blind) pline(E_J("%s is frozen by you!",
					  "%sは動けなくなった！"), Monnam(mdef));
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		}
		break;
	    case AD_SLEE:
		if (!negated && !mdef->msleeping &&
			    sleep_monst(mdef, rnd(10), -1)) {
		    if (!Blind)
			pline(E_J("%s is put to sleep by you!",
				  "%sは眠りに落とされた！"), Monnam(mdef));
		    slept_monst(mdef);
		}
		break;
	    case AD_SLIM:
		if (negated) break;	/* physical damage only */
		if (!rn2(4) && !flaming(mdef->data) &&
				mdef->data != &mons[PM_GREEN_SLIME]) {
		    You(E_J("turn %s into slime.","%sをスライムに変えた。"), mon_nam(mdef));
		    (void) newcham(mdef, &mons[PM_GREEN_SLIME], FALSE, FALSE);
		    tmp = 0;
		}
		break;
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
		/* there's no msomearmor() function, so just do damage */
	     /* if (negated) break; */
		break;
	    case AD_SLOW:
		if (!negated && mdef->mspeed != MSLOW) {
		    unsigned int oldspeed = mdef->mspeed;

		    mon_adjust_speed(mdef, -1, (struct obj *)0);
		    if (mdef->mspeed != oldspeed && canseemon(mdef))
			pline(E_J("%s slows down.","%sの動きは鈍くなった。"), Monnam(mdef));
		}
		break;
	    case AD_CONF:
		if (!mdef->mconf) {
		    if (canseemon(mdef))
			pline(E_J("%s looks confused.","%sは混乱したようだ。"), Monnam(mdef));
		    mdef->mconf = 1;
		}
		break;
	    default:	tmp = 0;
		break;
	}

	mdef->mstrategy &= ~STRAT_WAITFORU; /* in case player is very fast */
	if((mdef->mhp -= tmp) < 1) {
	    if (mdef->mtame && !cansee(mdef->mx,mdef->my)) {
		You_feel(E_J("embarrassed for a moment.",
			     "一瞬恥ずかしく感じた。"));
		if (tmp) xkilled(mdef, 0); /* !tmp but hp<1: already killed */
	    } else if (!flags.verbose) {
		You(E_J("destroy it!","それを破壊した！"));
		if (tmp) xkilled(mdef, (tmp<<2)|0); /*xkilled(mdef, 0);*/
	    } else
		if (tmp) killed_showdmg(mdef, tmp);
	    return(2);
	}
#ifdef SHOWDMG
	  else printdmg(tmp);
#endif /*SHOWDMG*/
	return(1);
}

STATIC_OVL int
explum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register int tmp = d((int)mattk->damn, (int)mattk->damd);

	You(E_J("explode!","爆発した！"));
	switch(mattk->adtyp) {
	    boolean resistance; /* only for cold/fire/elec */

	    case AD_BLND:
		if (!resists_blnd(mdef)) {
		    pline(E_J("%s is blinded by your flash of light!",
			      "%sはあなたの閃光で目が眩んだ！"), Monnam(mdef));
		    mdef->mblinded = min((int)mdef->mblinded + tmp, 127);
		    mdef->mcansee = 0;
		}
		break;
	    case AD_HALU:
		if (haseyes(mdef->data) && mdef->mcansee) {
		    pline(E_J("%s is affected by your flash of light!",
			      "%sはあなたの閃光でおかしくなった！"),
			  Monnam(mdef));
		    mdef->mconf = 1;
		}
		break;
	    case AD_COLD:
		resistance = resists_cold(mdef);
		goto common;
	    case AD_FIRE:
		resistance = resists_fire(mdef);
		goto common;
	    case AD_ELEC:
		resistance = resists_elec(mdef);
		if (elem_hits_shield(mdef, AD_ELEC, E_J("electric blast", "電撃"))) {
		    break;
		}
common:
		if (!resistance) {
		    pline(E_J("%s gets blasted!","%sは吹き飛ばされた！"), Monnam(mdef));
		    mdef->mhp -= tmp;
		    if (mdef->mhp <= 0) {
			 killed(mdef);
			 return(2);
		    }
		} else {
		    shieldeff(mdef->mx, mdef->my);
		    if (is_golem(mdef->data))
			golemeffects(mdef, (int)mattk->adtyp, tmp);
		    else
			pline_The(E_J("blast doesn't seem to affect %s.",
				      "%sは爆発の影響を受けなかった。"),
				mon_nam(mdef));
		}
		break;
	    default:
		break;
	}
	return(1);
}

STATIC_OVL void
start_engulf(mdef)
struct monst *mdef;
{
	if (!Invisible) {
		map_location(u.ux, u.uy, TRUE);
		tmp_at(DISP_ALWAYS, mon_to_glyph(&youmonst));
		tmp_at(mdef->mx, mdef->my);
	}
	You(E_J("engulf %s!","%sを飲み込んだ！"), mon_nam(mdef));
	delay_output();
	delay_output();
}

STATIC_OVL void
end_engulf()
{
	if (!Invisible) {
		tmp_at(DISP_END, 0);
		newsym(u.ux, u.uy);
	}
}

STATIC_OVL int
gulpum(mdef,mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register int tmp;
	register int dam = d((int)mattk->damn, (int)mattk->damd);
	struct obj *otmp;
	/* Not totally the same as for real monsters.  Specifically, these
	 * don't take multiple moves.  (It's just too hard, for too little
	 * result, to program monsters which attack from inside you, which
	 * would be necessary if done accurately.)  Instead, we arbitrarily
	 * kill the monster immediately for AD_DGST and we regurgitate them
	 * after exactly 1 round of attack otherwise.  -KAA
	 */

	if(mdef->data->msize >= MZ_HUGE) return 0;

	if(u.uhunger < 1500 && !u.uswallow) {
	    for (otmp = mdef->minvent; otmp; otmp = otmp->nobj)
		(void) snuff_lit(otmp);

	    if(!touch_petrifies(mdef->data) || Stone_resistance) {
#ifdef LINT	/* static char msgbuf[BUFSZ]; */
		char msgbuf[BUFSZ];
#else
		static char msgbuf[BUFSZ];
#endif
		start_engulf(mdef);
		switch(mattk->adtyp) {
		    case AD_DGST:
			/* eating a Rider or its corpse is fatal */
			if (is_rider(mdef->data)) {
			 pline(E_J("Unfortunately, digesting any of it is fatal.",
				   "残念ながら、そいつを食べることは死を意味する。" ));
			    end_engulf();
			    Sprintf(msgbuf, E_J("unwisely tried to eat %s","愚かにも%sを食べようとして"),
				    mdef->data->mname);
			    killer = msgbuf;
			    killer_format = NO_KILLER_PREFIX;
			    done(DIED);
			    return 0;		/* lifesaved */
			}

			if (Slow_digestion) {
			    dam = 0;
			    break;
			}

			/* KMH, conduct */
			u.uconduct.food++;
			if (!vegan(mdef->data))
			     u.uconduct.unvegan++;
			if (!vegetarian(mdef->data))
			     violated_vegetarian();

			/* Use up amulet of life saving */
			if (!!(otmp = mlifesaver(mdef))) m_useup(mdef, otmp);

			newuhs(FALSE);
			xkilled(mdef,2);
			if (mdef->mhp > 0) { /* monster lifesaved */
			    You(E_J("hurriedly regurgitate the sizzling in your %s.",
				    "%sに熱いものを感じ、急いで吐き戻した。"),
				body_part(STOMACH));
			} else {
			    tmp = 1 + (mdef->data->cwt >> 8);
			    if (corpse_chance(mdef, &youmonst, TRUE) &&
				!(mvitals[monsndx(mdef->data)].mvflags &
				  G_NOCORPSE)) {
				/* nutrition only if there can be a corpse */
				u.uhunger += (mdef->data->cnutrit+1) / 2;
			    } else tmp = 0;
			    Sprintf(msgbuf, E_J("You totally digest %s.",
						"あなたは%sを完全に消化した。"),
					    mon_nam(mdef));
			    if (tmp != 0) {
				/* setting afternmv = end_engulf is tempting,
				 * but will cause problems if the player is
				 * attacked (which uses his real location) or
				 * if his See_invisible wears off
				 */
				You(E_J("digest %s.","%sを消化している。"), mon_nam(mdef));
				if (Slow_digestion) tmp *= 2;
				nomul(-tmp);
				nomovemsg = msgbuf;
			    } else pline("%s", msgbuf);
			    if (mdef->data == &mons[PM_GREEN_SLIME]) {
#ifndef JP
				Sprintf(msgbuf, "%s isn't sitting well with you.",
					The(mdef->data->mname));
#else
				Sprintf(msgbuf, "%sはいまひとつあなたの腑に落ちない。",
					JMONNAM(PM_GREEN_SLIME));
#endif /*JP*/
				if (!Unchanging) {
					Slimed = 5L;
					flags.botl = 1;
				}
			    } else
			    exercise(A_CON, TRUE);
			}
			end_engulf();
			return(2);
		    case AD_PHYS:
			if (youmonst.data == &mons[PM_FOG_CLOUD]) {
			    pline(E_J("%s is laden with your moisture.",
				      "%sはあなたの湿気に悩まされている。"),
				  Monnam(mdef));
			    if (amphibious(mdef->data) &&
				!flaming(mdef->data)) {
				dam = 0;
				pline(E_J("%s seems unharmed.",
					  "%sは傷つかないようだ。"), Monnam(mdef));
			    }
			} else
			    pline(E_J("%s is pummeled with your debris!",
				      "%sはあなたの砂礫に打ちのめされた！"),
				  Monnam(mdef));
			break;
		    case AD_ACID:
			pline(E_J("%s is covered with your goo!",
				  "%sはあなたの粘液に覆われた！"), Monnam(mdef));
			if (resists_acid(mdef)) {
			    pline(E_J("It seems harmless to %s.",
				      "%sには効いていないようだ。"), mon_nam(mdef));
			    dam = 0;
			}
			break;
		    case AD_BLND:
			if (can_blnd(&youmonst, mdef, mattk->aatyp, (struct obj *)0)) {
			    if (mdef->mcansee)
				pline(E_J("%s can't see in there!",
					  "%sは目を開けていられない！"), Monnam(mdef));
			    mdef->mcansee = 0;
			    dam += mdef->mblinded;
			    if (dam > 127) dam = 127;
			    mdef->mblinded = dam;
			}
			dam = 0;
			break;
		    case AD_ELEC:
			if (rn2(2)) {
			    pline_The(E_J("air around %s crackles with electricity.",
					  "%sのまわりの空気が電光に爆ぜた。"), mon_nam(mdef));
			    if (resists_elec(mdef)) {
				pline(E_J("%s seems unhurt.","%sは傷つかない。"), Monnam(mdef));
				dam = 0;
			    }
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_COLD:
			if (rn2(2)) {
			    if (resists_cold(mdef)) {
				pline(E_J("%s seems mildly chilly.",
					  "%sはほんのりと涼しそうだ。"), Monnam(mdef));
				dam = 0;
			    } else
				pline(E_J("%s is freezing to death!",
					  "%sは凍死しそうだ！"),Monnam(mdef));
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_FIRE:
			if (rn2(2)) {
			    if (resists_fire(mdef)) {
				pline(E_J("%s seems mildly hot.",
					  "%sはほんのりと暖かそうだ。"), Monnam(mdef));
				dam = 0;
			    } else
				pline(E_J("%s is burning to a crisp!",
					  "%sは燃えさかる炎に焼かれている！"),Monnam(mdef));
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		}
		end_engulf();
		if ((mdef->mhp -= dam) <= 0) {
		    killed(mdef);
		    if (mdef->mhp <= 0)	/* not lifesaved */
			return(2);
		}
#ifndef JP
		You("%s %s!", is_animal(youmonst.data) ? "regurgitate"
			: "expel", mon_nam(mdef));
#else
		You("%sを%s出した！", mon_nam(mdef),
		    is_animal(youmonst.data) ? "吐き" : "放り");
#endif /*JP*/
		if (Slow_digestion || is_animal(youmonst.data)) {
		    pline(E_J("Obviously, you didn't like %s taste.",
			      "%s味は明らかにあなたの好みではなかった。"),
			  s_suffix(mon_nam(mdef)));
		}
	    } else {
		char kbuf[BUFSZ];

		You(E_J("bite into %s.","%sに噛みついた。"), mon_nam(mdef));
#ifndef JP
		Sprintf(kbuf, "swallowing %s whole", an(mdef->data->mname));
#else
		Sprintf(kbuf, "%sを丸呑みして", JMONNAM(monsndx(mdef->data)));
#endif /*JP*/
		instapetrify(kbuf);
	    }
	}
	return(0);
}

void
missum(mdef,mattk,missd)
register struct monst *mdef;
register struct attack *mattk;
int missd;
{
	if (could_seduce(&youmonst, mdef, mattk))
		You(E_J("pretend to be friendly to %s.",
			"%sに親しげなふりをした。"), mon_nam(mdef));
	else if(canspotmon(mdef) && flags.verbose) {
		switch (missd) {
		    case 0:
		    case 1:
#ifndef JP
			You("just miss %s.", mon_nam(mdef));
#else
			Your("攻撃は%sをかすめた。", mon_nam(mdef));
#endif /*JP*/
			break;
		    case 2:
		    case 3:
		    case 4:
		    case 5:
#ifndef JP
			You("miss %s.", mon_nam(mdef));
#else
			Your("攻撃は%sにかわされた。", mon_nam(mdef));
#endif /*JP*/
			break;
		    case 6:
		    case 7:
		    case 8:
		    case 9:
#ifndef JP
			You("swing and miss %s.", mon_nam(mdef));
#else
			Your("攻撃は%sに軽くかわされた。", mon_nam(mdef));
#endif /*JP*/
			break;
		    default:
#ifndef JP
			You("swing at %s, but %s easily evades your attack.", mon_nam(mdef), mhe(mdef));
#else
			You("%sめがけて攻撃を放ったが、まるで当たらなかった。", mon_nam(mdef));
#endif /*JP*/
			break;
		}
	} else
#ifndef JP
		You("miss it.");
#else
		Your("攻撃は当たらなかった。");
#endif /*JP*/
	if (!mdef->msleeping && mdef->mcanmove)
		wakeup(mdef);
}

STATIC_OVL boolean
hmonas(mon, tmp, hflg)		/* attack monster as a monster. */
register struct monst *mon;
register int tmp;
uchar hflg;
{
	struct attack *mattk, alt_attk;
	int	i, sum[NATTK], hittmp = 0;
	int	nsum = 0;
	int	dhit = 0;
	int	die;

	for(i = 0; i < NATTK; i++) {

	    sum[i] = 0;
	    mattk = getmattk(youmonst.data, i, sum, &alt_attk);
	    switch(mattk->aatyp) {
		case AT_WEAP:
use_weapon:
	/* Certain monsters don't use weapons when encountered as enemies,
	 * but players who polymorph into them have hands or claws and thus
	 * should be able to use weapons.  This shouldn't prohibit the use
	 * of most special abilities, either.
	 */
	/* Potential problem: if the monster gets multiple weapon attacks,
	 * we currently allow the player to get each of these as a weapon
	 * attack.  Is this really desirable?
	 */
			if (uwep) {
			    hittmp = hitval(uwep, mon);
			    hittmp += weapon_hit_bonus(uwep);
			    tmp += hittmp;
			}
			dhit = (tmp > (dieroll = rnd(20)) || u.uswallow);
			/* KMH -- Don't accumulate to-hit bonuses */
			if (uwep) tmp -= hittmp;
			/* Enemy dead, before any special abilities used */
			if (!known_hitum(mon,&dhit,mattk,hflg)) {
			    sum[i] = 2;
			    break;
			} else sum[i] = dhit;
			/* might be a worm that gets cut in half */
			if (m_at(u.ux+u.dx, u.uy+u.dy) != mon) return((boolean)(nsum != 0));
			/* Do not print "You hit" message, since known_hitum
			 * already did it.
			 */
			if (dhit && mattk->adtyp != AD_SPEL
				&& mattk->adtyp != AD_PHYS)
				sum[i] = damageum(mon,mattk);
			break;
		case AT_CLAW:
			if (i==0 && uwep && !cantwield(youmonst.data)) goto use_weapon;
#ifdef SEDUCE
			/* succubi/incubi are humanoid, but their _second_
			 * attack is AT_CLAW, not their first...
			 */
			if (i==1 && uwep && (u.umonnum == PM_SUCCUBUS ||
				u.umonnum == PM_INCUBUS)) goto use_weapon;
#endif
		case AT_KICK:
		case AT_BITE:
		case AT_STNG:
		case AT_TUCH:
		case AT_BUTT:
		case AT_TENT:
			if (i==0 && uwep && (youmonst.data->mlet==S_LICH)) goto use_weapon;
			die = rnd(20);
			if ((dhit = (tmp > die || u.uswallow)) != 0) {
			    int compat;

			    if (!u.uswallow &&
				(compat=could_seduce(&youmonst, mon, mattk))) {
#ifndef JP
				You("%s %s %s.",
				    mon->mcansee && haseyes(mon->data)
				    ? "smile at" : "talk to",
				    mon_nam(mon),
				    compat == 2 ? "engagingly":"seductively");
#else
				You("%sに%s%sかけた。",
				    mon_nam(mon),
				    compat == 2 ? "可愛らしく":"誘うように",
				    mon->mcansee && haseyes(mon->data)
				    ? "微笑み" : "話し");
#endif /*JP*/
				/* doesn't anger it; no wakeup() */
				sum[i] = damageum(mon, mattk);
				break;
			    }
			    wakeup(mon);
			    /* maybe this check should be in damageum()? */
			    if (mon->data == &mons[PM_SHADE] &&
					!(mattk->aatyp == AT_KICK &&
					    uarmf && uarmf->blessed)) {
				Your(E_J("attack passes harmlessly through %s.",
					 "攻撃は%sを傷つけることなく通り抜けた。"),
				    mon_nam(mon));
				break;
			    }
			    if (mattk->aatyp == AT_KICK)
				    You(E_J("kick %s.","%sを蹴った。"), mon_nam(mon));
			    else if (mattk->aatyp == AT_BITE)
				    You(E_J("bite %s.","%sに噛みついた。"), mon_nam(mon));
			    else if (mattk->aatyp == AT_STNG)
				    You(E_J("sting %s.","%sを刺した。"), mon_nam(mon));
			    else if (mattk->aatyp == AT_BUTT)
				    You(E_J("butt %s.","%sに頭突きした。"), mon_nam(mon));
			    else if (mattk->aatyp == AT_TUCH)
				    You(E_J("touch %s.","%sに触れた。"), mon_nam(mon));
			    else if (mattk->aatyp == AT_TENT)
#ifndef JP
				    Your("tentacles suck %s.", mon_nam(mon));
#else
				    You("触手で%sに吸い付いた。", mon_nam(mon));
#endif /*JP*/
			    else You(E_J("hit %s.","%sを攻撃した。"), mon_nam(mon));
			    sum[i] = damageum(mon, mattk);
			} else {
			    missum(mon, mattk, die - tmp);
			}
			break;

		case AT_HUGS:
			/* automatic if prev two attacks succeed, or if
			 * already grabbed in a previous attack
			 */
			dhit = 1;
			wakeup(mon);
			if (mon->data == &mons[PM_SHADE])
#ifndef JP
			    Your("hug passes harmlessly through %s.",
				mon_nam(mon));
#else
			    You("%sに抱きつこうとしたが、通り抜けてしまった。",
				mon_nam(mon));
#endif /*JP*/
			else if (!sticks(mon->data) && !u.uswallow) {
			    if (mon==u.ustuck) {
#ifndef JP
				pline("%s is being %s.", Monnam(mon),
				    u.umonnum==PM_ROPE_GOLEM ? "choked":
				    "crushed");
#else
				pline("%sは%sれている。", Monnam(mon),
				    u.umonnum==PM_ROPE_GOLEM ? "首を絞めら":
				    "押し潰さ");
#endif /*JP*/
				sum[i] = damageum(mon, mattk);
			    } else if(i >= 2 && sum[i-1] && sum[i-2]) {
				You(E_J("grab %s!","%sをつかんだ！"), mon_nam(mon));
				u.ustuck = mon;
				sum[i] = damageum(mon, mattk);
			    }
			}
			break;

		case AT_EXPL:	/* automatic hit if next to */
			dhit = -1;
			wakeup(mon);
			sum[i] = explum(mon, mattk);
			break;

		case AT_ENGL:
			if((dhit = (tmp > rnd(20+i)))) {
				wakeup(mon);
				if (mon->data == &mons[PM_SHADE])
#ifndef JP
				    Your("attempt to surround %s is harmless.",
					mon_nam(mon));
#else
				    pline("%sを取り囲もうとするあなたの試みは失敗した。",
					mon_nam(mon));
#endif /*JP*/
				else {
				    sum[i]= gulpum(mon,mattk);
				    if (sum[i] == 2 &&
					    (mon->data->mlet == S_ZOMBIE ||
						mon->data->mlet == S_MUMMY) &&
					    rn2(5) &&
					    !Sick_resistance) {
					You_feel(E_J("%ssick.","%s気分が悪くなった。"),
					    (Sick) ? E_J("very ","とても") : "");
					mdamageu(mon, rnd(8));
				    }
				}
			} else
				missum(mon, mattk, 2);
			break;

		case AT_MAGC:
			/* No check for uwep; if wielding nothing we want to
			 * do the normal 1-2 points bare hand damage...
			 */
			if (i==0 && (youmonst.data->mlet==S_KOBOLD
				|| youmonst.data->mlet==S_ORC
				|| youmonst.data->mlet==S_GNOME
				)) goto use_weapon;

		case AT_NONE:
		case AT_BOOM:
			continue;
			/* Not break--avoid passive attacks from enemy */

		case AT_BREA:
		case AT_SPIT:
		case AT_GAZE:	/* all done using #monster command */
			dhit = 0;
			break;

		default: /* Strange... */
			impossible("strange attack of yours (%d)",
				 mattk->aatyp);
	    }
	    if (dhit == -1) {
		u.mh = -1;	/* dead in the current form */
		rehumanize();
	    }
	    if (sum[i] == 2)
		return((boolean)passive(mon, 1, 0, mattk->aatyp, hflg));
							/* defender dead */
	    else {
		(void) passive(mon, sum[i], 1, mattk->aatyp, hflg);
		nsum |= sum[i];
	    }
	    if (!Upolyd)
		break; /* No extra attacks if no longer a monster */
	    if (multi < 0)
		break; /* If paralyzed while attacking, i.e. floating eye */
	}
	return((boolean)(nsum != 0));
}

/*	Special (passive) attacks on you by monsters done here.		*/

int
passive(mon, mhit, malive, aatyp, hflg)
register struct monst *mon;
register boolean mhit;
register int malive;
uchar aatyp;
uchar hflg;
{
	register struct permonst *ptr = mon->data;
	register int i, tmp;
	struct obj *otmp = (struct obj*)0;
	boolean distant;

	if (ptr->msound == MS_SHRIEK && malive && !rn2(3)) m_respond(mon); /* shrieker */
	for(i = 0; ; i++) {
	    if(i >= NATTK) return(malive | mhit);	/* no passive attacks */
	    if(ptr->mattk[i].aatyp == AT_NONE) break;	/* try this one */
	}
	/* Note: tmp not always used */
	if (ptr->mattk[i].damn)
	    tmp = d((int)ptr->mattk[i].damn, (int)ptr->mattk[i].damd);
	else if(ptr->mattk[i].damd)
	    tmp = d((int)mon->m_lev+1, (int)ptr->mattk[i].damd);
	else
	    tmp = 0;

	if (hflg & HIT_MAINWEP) otmp = uwep;
	else if (hflg & HIT_SWAPWEP) otmp = uswapwep;
	else if (hflg & HIT_SHIELD) otmp = uarms;
	distant = (hflg & HIT_DISTANT) == HIT_DISTANT;

/*	These affect you even if they just died */

	switch(ptr->mattk[i].adtyp) {

	  case AD_ACID:
	    if(mhit && !distant && rn2(2)) {
		if (Blind || !flags.verbose) You(E_J("are splashed!","液体を浴びせられた！"));
		else	You(E_J("are splashed by %s acid!","%s酸を浴びせられた！"),
			                s_suffix(mon_nam(mon)));

		if (!Acid_resistance)
			mdamageu(mon, tmp);
		if(!rn2(30)) erode_armor(&youmonst, TRUE);
	    }
	    if (mhit) {
		if (aatyp == AT_KICK) {
		    if (uarmf && !rn2(6))
			(void)rust_dmg(uarmf, xname(uarmf), 3, TRUE, &youmonst);
		} else if (aatyp == AT_WEAP || aatyp == AT_CLAW ||
			   aatyp == AT_MAGC || aatyp == AT_TUCH)
		    passive_obj(mon, otmp, &(ptr->mattk[i]));
	    }
	    exercise(A_STR, FALSE);
	    break;
	  case AD_STON:
	    if (mhit) {		/* successful attack */
		long protector = attk_protection((int)aatyp);

		/* hero using monsters' AT_MAGC attack is hitting hand to
		   hand rather than casting a spell */
		if (aatyp == AT_MAGC) protector = W_ARMG;

		if (protector == 0L ||		/* no protection */
			(protector == W_ARMG && !uarmg && !uwep) ||
			(protector == W_ARMF && !uarmf) ||
			(protector == W_ARMH && !uarmh) ||
			(protector == (W_ARMC|W_ARMG) && (!uarmc || !uarmg))) {
		    if (!Stone_resistance &&
			    !(poly_when_stoned(youmonst.data) &&
				polymon(PM_STONE_GOLEM))) {
			You(E_J("turn to stone...","石になった…。"));
			done_in_by(mon);
			return 2;
		    }
		}
	    }
	    break;
	  case AD_RUST:
	    if(mhit && !mon->mcan) {
		if (aatyp == AT_KICK) {
		    if (uarmf)
			(void)rust_dmg(uarmf, xname(uarmf), 1, TRUE, &youmonst);
		} else if (aatyp == AT_WEAP || aatyp == AT_CLAW ||
			   aatyp == AT_MAGC || aatyp == AT_TUCH)
		    passive_obj(mon, otmp, &(ptr->mattk[i]));
	    }
	    break;
	  case AD_CORR:
	    if(mhit && !mon->mcan) {
		if (aatyp == AT_KICK) {
		    if (uarmf)
			(void)rust_dmg(uarmf, xname(uarmf), 3, TRUE, &youmonst);
		} else if (aatyp == AT_WEAP || aatyp == AT_CLAW ||
			   aatyp == AT_MAGC || aatyp == AT_TUCH)
		    passive_obj(mon, otmp, &(ptr->mattk[i]));
	    }
	    break;
	  case AD_MAGM:
	    /* wrath of gods for attacking Oracle */
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
		pline(E_J("A hail of magic missiles narrowly misses you!",
			  "魔法の矢が降りそそぎ、危うくあなたに当たるところだった！"));
		damage_resistant_obj(ANTIMAGIC, rnd(10));
	    } else {
		You(E_J("are hit by magic missiles appearing from thin air!",
			"虚空から放たれた魔法の矢に撃たれた！"));
		mdamageu(mon, tmp);
	    }
	    break;
	  case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
	    if (mhit) {
		if (aatyp == AT_KICK) {
		    otmp = uarmf;
		    if (!otmp) break;
		} else if (aatyp == AT_BITE || aatyp == AT_BUTT ||
			   (aatyp >= AT_STNG && aatyp < AT_WEAP)) {
		    break;		/* no object involved */
		}
		passive_obj(mon, otmp, &(ptr->mattk[i]));
	    }
	    break;
	  case AD_DISN:	/* disintegration */
	    if (mhit && !is_full_resist(DISINT_RES)) {
		boolean is_armorattk = FALSE;
		if (!otmp) {
		    otmp = uarmg;
		    is_armorattk = TRUE;
		}
		if (aatyp == AT_KICK) {
		    otmp =  uarmf;
		    is_armorattk = TRUE;
		}
		if (!otmp) {
		    if (!Disint_resistance) {
			You(E_J("are disintegrated!","分解された！"));
			tmp = u.uhp;
			if (Half_physical_damage) tmp *= 2; /* sorry */
			mdamageu(mon, tmp);
		    }
		} else {
		    if (is_armorattk) destroy_arm(otmp);
		    else {
			Your(E_J("weapon crumbles away!","武器は粉微塵になった！"));
			remove_worn_item(otmp, FALSE);
			useup(otmp);
			update_inventory();
		    }
		}
	    }
	    break;
	  default:
	    break;
	}

/*	These only affect you if they still live */

	if(malive && !mon->mcan && rn2(3)) {

	    switch(ptr->mattk[i].adtyp) {

	      case AD_PLYS:
		if(ptr == &mons[PM_FLOATING_EYE]) {
		    if (!canseemon(mon)) {
			break;
		    }
		    if(mon->mcansee) {
			if (ureflects(E_J("%s gaze is reflected by your %s.",
					  "%s眼光はあなたの%sにはね返された。"),
				    s_suffix(Monnam(mon))))
			    ;
			else if (Free_action)
			    You(E_J("momentarily stiffen under %s gaze!",
				    "%s眼光に一瞬固まった！"),
				    s_suffix(mon_nam(mon)));
			else {
			    You(E_J("are frozen by %s gaze!","%s眼光で金縛りにあった！"),
				  s_suffix(mon_nam(mon)));
			    nomul((ACURR(A_WIS) > 12 || rn2(4)) ? -tmp : -127);
			}
		    } else {
#ifndef JP
			pline("%s cannot defend itself.",
				Adjmonnam(mon,"blind"));
#else
			pline("%sは自分を守ることができない。",
				Adjmonnam(mon,"盲目の"));
#endif /*JP*/
			if(!rn2(500)) change_luck(-1);
		    }
		} else if (Free_action) {
		    You(E_J("momentarily stiffen.","一瞬固まった。"));
		} else if (!distant) { /* gelatinous cube */
		    You(E_J("are frozen by %s!","%sに麻痺させられた！"), mon_nam(mon));
	    	    nomovemsg = 0;	/* default: "you can move again" */
		    nomul(-tmp);
		    exercise(A_DEX, FALSE);
		}
		break;
	      case AD_COLD:		/* brown mold or blue jelly */
		if(!distant) {
		    if(Cold_resistance) {
			if (is_full_resist(COLD_RES)) {
			    shieldeff(u.ux, u.uy);
			    You_feel(E_J("a mild chill.","ほのかな涼しさを感じた。"));
			    break;
			} else {
			    You_feel(E_J("a little cold.","少し凍えた。"));
			    tmp = (tmp+3) / 4;
			}
			ugolemeffects(AD_COLD, tmp);
		    } else You(E_J("are suddenly very cold!","突然激しく凍えさせられた！"));
		    mdamageu(mon, tmp);
		/* monster gets stronger with your heat! */
		    mon->mhp += tmp / 2;
		    if (mon->mhpmax < mon->mhp) mon->mhpmax = mon->mhp;
		/* at a certain point, the monster will reproduce! */
		    if(mon->mhpmax > ((int) (mon->m_lev+1) * 8))
			(void)split_mon(mon, &youmonst);
		}
		break;
	      case AD_STUN:		/* specifically yellow mold */
		if(!Stunned && !distant)
		    make_stunned((long)tmp, TRUE);
		break;
	      case AD_FIRE:
		if(!distant) {
		    if(Fire_resistance) {
			if (is_full_resist(FIRE_RES)) {
			    shieldeff(u.ux, u.uy);
			    You_feel(E_J("mildly warm.","ほのかな暖かさを感じた。"));
			    break;
			} else {
			    You(E_J("feel a little hot.","少し熱くなった。"));
			    tmp = (tmp+3) / 4;
			}
			ugolemeffects(AD_FIRE, tmp);
		    } else You(E_J("are suddenly very hot!","突然激しく熱せられた！"));
		    mdamageu(mon, tmp);
		}
		break;
	      case AD_ELEC:
		if (!distant || !otmp || is_metallic(otmp)) {
		    if(Shock_resistance) {
			if (is_full_resist(SHOCK_RES)) {
			    shieldeff(u.ux, u.uy);
			    You_feel(E_J("a mild tingle.","肌にかすかな刺激を感じた。"));
			    break;
			} else {
			    tmp = (tmp+3) / 4;
			}
			ugolemeffects(AD_ELEC, tmp);
		    }
		    You(E_J("are jolted with electricity%s","電気の奔流に撃たれた%s"),
			    Shock_resistance ? E_J(".","。") : E_J("!","！"));
		    mdamageu(mon, tmp);
		}
		break;
	      default:
		break;
	    }
	}
	return(malive | mhit);
}

/*
 * Special (passive) attacks on an attacking object by monsters done here.
 * Assumes the attack was successful.
 */
void
passive_obj(mon, obj, mattk)
register struct monst *mon;
register struct obj *obj;	/* null means pick uwep, uswapwep or uarmg */
struct attack *mattk;		/* null means we find one internally */
{
	register struct permonst *ptr = mon->data;
	register int i;

	/* if caller hasn't specified an object, use uwep, uswapwep or uarmg */
	if (!obj) {
	    obj = (u.twoweap && uswapwep && !rn2(2)) ? uswapwep : uwep;
	    if (!obj && mattk->adtyp == AD_ENCH)
		obj = uarmg;		/* no weapon? then must be gloves */
	    if (!obj) return;		/* no object to affect */
	}

	/* if caller hasn't specified an attack, find one */
	if (!mattk) {
	    for(i = 0; ; i++) {
		if(i >= NATTK) return;	/* no passive attacks */
		if(ptr->mattk[i].aatyp == AT_NONE) break; /* try this one */
	    }
	    mattk = &(ptr->mattk[i]);
	}

	switch(mattk->adtyp) {

	case AD_ACID:
	    if(!rn2(6)) {
		erode_obj(obj, TRUE, FALSE);
	    }
	    break;
	case AD_RUST:
	    if(!mon->mcan) {
		erode_obj(obj, FALSE, FALSE);
	    }
	    break;
	case AD_CORR:
	    if(!mon->mcan) {
		erode_obj(obj, TRUE, FALSE);
	    }
	    break;
	case AD_ENCH:
	    if (!mon->mcan) {
		if (drain_item(obj) && carried(obj) &&
		    (obj->known || obj->oclass == ARMOR_CLASS)) {
#ifndef JP
		    Your("%s less effective.", aobjnam(obj, "seem"));
#else
		    Your("%sの威力が落ちたようだ。", cxname(obj));
#endif /*JP*/
	    	}
	    	break;
	    }
	  default:
	    break;
	}

	if (carried(obj)) update_inventory();
}

/* Note: caller must ascertain mtmp is mimicking... */
void
stumble_onto_mimic(mtmp)
struct monst *mtmp;
{
	const char *fmt = E_J("Wait!  That's %s!","待て！ それは%sだ！"),
		   *generic = E_J("a monster","怪物"),
		   *what = 0;

	if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
	    u.ustuck = mtmp;

	if (Blind) {
	    if (!Blind_telepat)
		what = generic;		/* with default fmt */
	    else if (mtmp->m_ap_type == M_AP_MONSTER)
		what = a_monnam(mtmp);	/* differs from what was sensed */
	} else {
	    int glyph = levl[u.ux+u.dx][u.uy+u.dy].glyph;

	    if (glyph_is_cmap(glyph) &&
		    (glyph_to_cmap(glyph) == S_hcdoor ||
		     glyph_to_cmap(glyph) == S_vcdoor))
		fmt = E_J("The door actually was %s!","この扉は%sだった！");
	    else if (glyph_is_object(glyph) &&
		    glyph_to_obj(glyph) == GOLD_PIECE)
		fmt = E_J("That gold was %s!","この金貨は%sだった！");

	    /* cloned Wiz starts out mimicking some other monster and
	       might make himself invisible before being revealed */
	    if (mtmp->minvis && !See_invisible)
		what = generic;
	    else
		what = a_monnam(mtmp);
	}
	if (what) pline(fmt, what);

	wakeup(mtmp);	/* clears mimicking */
}

STATIC_OVL void
nohandglow(mon)
struct monst *mon;
{
	char *hands=E_J(makeplural(body_part(HAND)), (char *)body_part(HAND));

	if (!u.umconf || mon->mconf) return;
	if (u.umconf == 1) {
		if (Blind)
			Your(E_J("%s stop tingling.","%sから刺激が消えた。"), hands);
		else
			Your(E_J("%s stop glowing %s.","%sから%s光が消えた。"), hands, hcolor(NH_RED));
	} else {
		if (Blind)
			pline_The(E_J("tingling in your %s lessens.",
				      "%sに感じていた刺激が弱まった。"), hands);
		else
			Your(E_J("%s no longer glow so brightly %s.",
				 "%sの%s輝きは消えかけている。"),
				 hands, hcolor(NH_RED));
	}
	u.umconf--;
}

int
flash_hits_mon(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;	/* source of flash */
{
	int tmp, amt, res = 0, useeit = canseemon(mtmp);

	/* transparent monsters stops the flashed light */
	if (mtmp->minvis || mtmp->data->mlet != S_LIGHT) res = 1;

	if (mtmp->msleeping) {
	    mtmp->msleeping = 0;
	    if (useeit) {
		pline_The(E_J("flash awakens %s.","眩しい光で%sが目覚めた。"), mon_nam(mtmp));
		res = 1;
	    }
	} else if (mtmp->data->mlet != S_LIGHT) {
	    if (!resists_blnd(mtmp)) {
		tmp = dist2(otmp->ox, otmp->oy, mtmp->mx, mtmp->my);
		if (useeit) {
		    pline(E_J("%s is blinded by the flash!",
			      "%sは閃光に目が眩んだ！"), Monnam(mtmp));
		    res = 1;
		}
		if (mtmp->data == &mons[PM_GREMLIN]) {
		    /* Rule #1: Keep them out of the light. */
		    amt = otmp->otyp == WAN_LIGHT ? d(1 + otmp->spe, 4) :
		          rn2(min(mtmp->mhp,4));
#ifndef JP
		    pline("%s %s!", Monnam(mtmp), amt > mtmp->mhp / 2 ?
			  "wails in agony" : "cries out in pain");
#else
		    pline("%sは%s！", Monnam(mtmp), amt > mtmp->mhp / 2 ?
			  "苦悶に泣き叫んだ" : "苦痛の叫びをあげた");
#endif /*JP*/
		    if ((mtmp->mhp -= amt) <= 0) {
			if (flags.mon_moving)
			    monkilled(mtmp, (char *)0, AD_BLND);
			else
			    killed(mtmp);
		    } else if (cansee(mtmp->mx,mtmp->my) && !canspotmons(mtmp)){
			map_invisible(mtmp->mx, mtmp->my);
		    }
		}
		if (mtmp->mhp > 0) {
		    if (!flags.mon_moving) setmangry(mtmp);
		    if (tmp < 9 && !mtmp->isshk && rn2(4)) {
			if (rn2(4))
			    monflee(mtmp, rnd(100), FALSE, TRUE);
			else
			    monflee(mtmp, 0, FALSE, TRUE);
		    }
		    mtmp->mcansee = 0;
		    mtmp->mblinded = (tmp < 3) ? 0 : rnd(1 + 50/tmp);
		}
	    }
	}
	return res;
}

#if 0 /* test... */
int reduce_mdmg(mtmp, dmg, msg)
struct monst *mtmp;
int dmg;
int msg;
{
	struct obj *obj;
	struct obj *marm[ARM_MAXNUM];
	int i;
	int prot = 0, tmp = 0;
	long mwflags = mtmp->misc_worn_check;
	boolean seeit = canspotmon(mtmp);

	for (i=0; i<ARM_MAXNUM; i++) marm[i] = 0;

	for (obj = mtmp->minvent; obj; obj = obj->nobj) {
	    if (obj->owornmask & mwflags) {
		if (objects[obj->otyp].oc_class == ARMOR_CLASS) {
		    marm[objects[obj->otyp].oc_armcat] = obj;
		}
		if (objects[obj->otyp].oc_oprop == PROTECTION)
		    prot += 2; /* protection effect is handled separately */
	    }
	}

	/* reduce by protection */
	dmg -= prot;
	if (dmg <= 0) {
	    if (msg) You("just miss %s.", mon_nam(mtmp));
	    return 0;
	}
pline("<%d>",dmg);

	/* shield */
	if (marm[ARM_SHIELD]) {
	    tmp += reduce_dmg_amount(marm[ARM_SHIELD]);
	    if (tmp) dmg -= rnd(tmp);
	    if (dmg <= 0) {
		if (msg) {
		    You("barely hit %s.", mon_nam(mtmp));
		    if (seeit)
			pline("%s blocks your attack with %s shield.",
				Monnam(mtmp), mhis(mtmp));
		    else
			pline("Your attack is blocked by something.");
		}
pline("<%d>",dmg);
		return 0;
	    }
	}

	/* armor, boots, helm, gloves */
	switch (rn2(DAMCAN_MAX)) {
	    case DAMCAN_HEAD:	/* head */
		if (marm[ARM_HELM]) {
		    tmp += reduce_dmg_amount(marm[ARM_HELM]);
		    /* maid dress' special power */
		    if(mtmp->female &&
		       marm[ARM_SUIT] && marm[ARM_SUIT]->otyp == MAID_DRESS &&
		       marm[ARM_HELM]->otyp == KATYUSHA) tmp += 2;
		    if (tmp) dmg -= rnd(tmp);
pline("<%d>",dmg);
		    if (dmg <= 0) {
			if (msg) {
			    if (seeit)
				pline("Your attack glances %s %s.",
					s_suffix(mon_nam(mtmp)), helm_simple_name(marm[ARM_HELM]));
			    else
				You("just miss it.");
			}
			return 0;
		    }
		}
		break;
	    case DAMCAN_FEET:	/* feet */
		if (marm[ARM_BOOTS]) {
		    tmp += reduce_dmg_amount(marm[ARM_BOOTS]);
		    if (tmp) dmg -= rnd(tmp);
pline("<%d>",dmg);
		    if (dmg <= 0) {
			if (msg) {
			    if (seeit)
				You("harmlessly hit %s boots.", s_suffix(mon_nam(mtmp)));
			    else
				You("just miss it.");
			}
			return 0;
		    }
		}
		break;
	    case DAMCAN_HAND:	/* hand(s) */
		if (marm[ARM_GLOVES]) {
		    tmp += reduce_dmg_amount(marm[ARM_GLOVES]);
		    if (tmp) dmg -= rnd(tmp);
pline("<%d>",dmg);
		    if (dmg <= 0) {
			if (msg) {
			    if (seeit)
				You("harmlessly hit %s gauntlets.", s_suffix(mon_nam(mtmp)));
			    else
				You("just miss it.");
			}
			return 0;
		    }
		}
		break;
	    default:
		if (marm[ARM_SUIT])
		    tmp += reduce_dmg_amount(marm[ARM_SUIT]);
		if (marm[ARM_CLOAK]) {
		    tmp += reduce_dmg_amount(marm[ARM_CLOAK]);
		    /* maid dress' special power */
		    if(mtmp->female &&
		       marm[ARM_SUIT] && marm[ARM_SUIT]->otyp == MAID_DRESS) {
			if (marm[ARM_CLOAK]->otyp == KITCHEN_APRON) tmp += 3;
			if (marm[ARM_CLOAK]->otyp == FRILLED_APRON) tmp += 4;
		    }
		}
		if (tmp) {
		    dmg -= rnd(tmp);
pline("<%d>",dmg);
		    if (dmg <= 0) {
			if (msg) {
			    if (seeit) {
				You("barely hit %s.", mon_nam(mtmp));
				pline("%s %s blocks your attack.", mhis(mtmp),
					(marm[ARM_SUIT] && !is_clothes(marm[ARM_SUIT])) ?
						"armor" : "clothes");
			    } else
				You("harmlessly hit something.");
			}
			return 0;
		    }
		}
		break;
	}
	return dmg;
}
#endif

/*uhitm.c*/
