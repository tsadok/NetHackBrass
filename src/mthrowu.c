/*	SCCS Id: @(#)mthrowu.c	3.4	2003/05/09	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL int FDECL(drop_throw,(struct obj *,BOOLEAN_P,int,int));
STATIC_DCL boolean FDECL(firemu,(struct monst *,struct obj *));

#define URETREATING(x,y) (distmin(u.ux,u.uy,x,y) > distmin(u.ux0,u.uy0,x,y))

#define POLE_LIM 5	/* How far monsters can use pole-weapons */

#ifndef OVLB

STATIC_DCL const char *breathwep[];

#else /* OVLB */

/*
 * Keep consistent with breath weapons in zap.c, and AD_* in monattk.h.
 */
STATIC_OVL NEARDATA const char *breathwep[] = {
	E_J(	"fragments",			"魔力の散弾"),
	E_J(	"fire",				"炎"),
	E_J(	"frost",			"冷気"),
	E_J(	"sleep gas",			"催眠ガス"),
	E_J(	"a disintegration blast",	"分解のブレス"),
	E_J(	"lightning",			"雷撃"),
	E_J(	"poison gas",			"毒ガス"),
	E_J(	"acid",				"強酸"),
	E_J(	"paralysis gas",		"麻痺性のガス"),
		"strange breath #9"
};

/* hero is hit by something other than a monster */
int
thitu(tlev, dam, obj, name)
int tlev, dam;
struct obj *obj;
const char *name;	/* if null, then format `obj' */
{
	const char *onm, *knm;
	boolean is_acid;
	int kprefix = KILLED_BY_AN;
	char onmbuf[BUFSZ], knmbuf[BUFSZ];

	if (!name) {
	    if (!obj) panic("thitu: name & obj both null?");
	    name = strcpy(onmbuf,
			 (obj->quan > 1L) ? doname(obj) : mshot_xname(obj));
#ifndef JP
	    knm = strcpy(knmbuf, killer_xname(obj));
	    kprefix = KILLED_BY;  /* killer_name supplies "an" if warranted */
#else
//	    if (obj->class == WEAPON_CLASS && obj->leashmon) {
//		struct monst *mtmp;
//		const char *verb;
//		for (mtmp = fmon; mtmp && mtmp->m_id != obj->leashmon; mtmp = mtmp->nmon);
//		if (mtmp) setup_killername(mtmp, knmbuf);
//		else Sprintf(knmbuf, "何者か");
//		if (is_ammo(obj)) {
//		    switch (objects[obj->otyp].oc_wprop & WP_SUBTYPE) {
//			case WP_STONE:	verb = "投げた"; break;
//			case WP_BULLET: verb = "撃った"; break;
//			default:	verb = "射た";	 break;
//		    }
//		} else verb = "投げた";
//		Sprintf(eos(knmbuf), "の%s%sで", verb, killer_xname(obj));
//	    } else
		Sprintf(knmbuf, "%sで", killer_xname(obj));
	    knm = knmbuf;
	    kprefix = KILLED_SUFFIX;
#endif /*JP*/
	} else {
#ifndef JP
	    knm = name;
	    /* [perhaps ought to check for plural here to] */
	    if (!strncmpi(name, "the ", 4) ||
		    !strncmpi(name, "an ", 3) ||
		    !strncmpi(name, "a ", 2)) kprefix = KILLED_BY;
#else
	    Sprintf(knmbuf, "%sで", name);
	    knm = knmbuf;
	    kprefix = KILLED_SUFFIX;
#endif /*JP*/
	}
#ifndef JP
	onm = (obj && obj_is_pname(obj)) ? the(name) :
			    (obj && obj->quan > 1L) ? name : an(name);
#else
	onm = name;
#endif /*JP*/
	is_acid = (obj && obj->otyp == ACID_VENOM);

	if(u.uac + tlev <= rnd(20)) {
		if(Blind || !flags.verbose) pline(E_J("It misses.","何かがあなたをかすめた。"));
		else E_J(You("are almost hit by %s.", onm),
			 pline("%sがあなたをかすめた。", onm));
		return(0);
	} else {
		if(uarms && obj && missile_hits_shield(&youmonst, obj))
		    return(0);
		if(Blind || !flags.verbose) E_J(You("are hit!"),pline("何かがあなたに命中した！"));
		else {
		    if (obj && obj->otyp == BOULDER &&
			uwep && uwep->oartifact == ART_GIANTKILLER) {
#ifndef JP
			You("swing %s!", the(xname(uwep)));
#else
			You("%sを振るった！", xname(uwep));
#endif /*JP*/
			pline(E_J("The boulder is smashed into bits.",
				  "大岩は粉々に砕かれた。"));
				
			return 1;
		    } else
#ifndef JP
			You("are hit by %s%s", onm, exclam(dam));
#else
			pline("%sがあなたに命中した%s", onm, exclam(dam));
#endif /*JP*/
		}

		if (obj && get_material(obj) == SILVER
				&& hates_silver(youmonst.data)) {
			dam += rnd(20);
			pline_The(E_J("silver sears your flesh!",
				      "銀があなたの身体を焼いた！"));
			exercise(A_CON, FALSE);
		}
		if (is_acid && Acid_resistance)
			pline(E_J("It doesn't seem to hurt you.",
				  "あなたは傷つかないようだ。"));
		else {
			if (is_acid) pline(E_J("It burns!","身体が灼ける！"));
			if (dam) dam = reduce_damage(dam, DAMCAN_RANDOM);
			if (Half_physical_damage) dam = (dam+1) / 2;
			losehp(dam, knm, kprefix);
			exercise(A_STR, FALSE);
		}
		return(1);
	}
}

int
missile_hits_shield(mon, missile)
struct monst *mon;
struct obj *missile;
{
	struct obj *shield;
	char nbuf[BUFSZ];
	int siz;
	boolean seemissile;

	if (mon == &youmonst) {
	    shield = uarms;
	    seemissile = !Blind;
	} else {
	    struct obj *otmp;
	    if (!(mon->misc_worn_check & W_ARMS))
		return (0);
	    shield = which_armor(mon, W_ARMS);
	    seemissile = cansee(mon->mx, mon->my);
	}
	if (!shield) return 0;
	/* Use weight as size of the shield */
	siz = (int)(objects[shield->otyp].oc_weight) + ((int)(shield->spe) * 10);
	strcpy(nbuf, mshot_xname(missile));
	if (rn2(120) < siz && weight(missile) < siz) {
	    /* hit sounds */
	    if (missile->oclass == VENOM_CLASS)
		pline(E_J("Splash!","ビシャッ！"));
	    else if (missile->otyp == EGG || missile->otyp == CREAM_PIE)
		pline(E_J("Splat!","ベチャ！"));
	    else if (missile->oclass == POTION_CLASS && !seemissile)
		pline(E_J("Crash!","ガチャン！"));

	    if (shield->where == OBJ_INVENT) {
		/* missle hits your shield */
		pline(E_J("%s hits your shield.","%sがあなたの盾に命中した。"),
		      seemissile ? nbuf: something);
	    } else if (shield->where == OBJ_MINVENT) {
		/* missle hits mon's shield */
		if (!canseemon(mon)) tmiss(missile, mon);
		else
#ifndef JP
		    pline("%s hits %s shield.",
			  Blind ? something : nbuf, s_suffix(mon_nam(mon)));
#else
		    pline("%sが%sの盾に命中した。",
			  Blind ? something : nbuf, mon_nam(mon));
#endif /*JP*/
	    }
	    if (seemissile && missile->oclass == POTION_CLASS)
		pline(E_J("The %s breaks into shards.",
			  "%sは粉々に割れた。"), bottlename());
	    if (missile->otyp == ACID_VENOM || missile->otyp == POT_ACID)
		rust_dmg(shield, E_J("shield","盾"), 1, TRUE, mon);
	    return (1); /* missile hits the shield */
	}
	return (0); /* missile hits the target */
}

int
elem_hits_shield(mon, dtyp, nam)
struct monst *mon;
int dtyp;
const char *nam;
{
	struct obj *shield;
	char nbuf[BUFSZ];
	int styp = AD_PHYS; /* none */

	shield = (struct obj *)0;
	if (mon == &youmonst) {
	    shield = uarms;
	} else {
	    struct obj *otmp;
	    if (!(mon->misc_worn_check & W_ARMS))
		return (0);
	    shield = which_armor(mon, W_ARMS);
	}
	if (shield) {
	    switch (shield->otyp) {
		case SHIELD_OF_FIREBREAK:
		    styp = AD_FIRE;
		    break;
		case SHIELD_OF_ISOLATION:
		    styp = AD_ELEC;
		    break;
		default:
		    break;
	    }
	}
	if (styp == AD_ANY || styp == dtyp) {
	    if (!nam) return (1);
	    if (mon == &youmonst) {
		/* you blocked the attack with your shield */
		You(E_J("block the %s with your shield.","%sを盾で防いだ。"), nam);
	    } else {
		/* monster blocked the attack with its shield */
		if (!Blind) {
#ifndef JP
		    pline("%s blocks the %s with %s shield.",
			  Monnam(mon), nam, mhis(mon));
#else
		    pline("%sは%sを盾で防いだ。", mon_nam(mon), nam);
#endif /*JP*/
		} else {
		    pline(E_J("%s seems unaffected.","%sは影響を受けないようだ。"), Monnam(mon));
		}
	    }
	    return (1); /* element hits the shield */
	}
	return (0); /* element hits the target */
}

/* Be sure this corresponds with what happens to player-thrown objects in
 * dothrow.c (for consistency). --KAA
 * Returns 0 if object still exists (not destroyed).
 */

STATIC_OVL int
drop_throw(obj, ohit, x, y)
register struct obj *obj;
boolean ohit;
int x,y;
{
	int retvalu = 1;
	int create;
	struct monst *mtmp;
	struct trap *t;

	if (obj->otyp == CREAM_PIE || obj->oclass == VENOM_CLASS ||
	    (is_bullet(obj) && obj->oshot) ||
		    (ohit && obj->otyp == EGG))
		create = 0;
	else if (ohit && (is_multigen(obj) || obj->otyp == ROCK))
		create = !rn2(3);
	else create = 1;

	if (obj->otyp == BOULDER && x == u.ux && y == u.uy &&
	    uwep && uwep->oartifact == ART_GIANTKILLER)
		trans_to_rock(obj);

	if (create && !((mtmp = m_at(x, y)) && (mtmp->mtrapped) &&
			(t = t_at(x, y)) && ((t->ttyp == PIT) ||
			(t->ttyp == SPIKED_PIT)))) {
		int objgone = 0;

		if (down_gate(x, y) != -1)
			objgone = ship_object(obj, x, y, FALSE);
		if (!objgone) {
			if (!flooreffects(obj,x,y,E_J("fall","落ちた"))) { /* don't double-dip on damage */
			    place_object(obj, x, y);
			    if (!mtmp && x == u.ux && y == u.uy)
				mtmp = &youmonst;
			    if (mtmp && ohit)
				passive_obj(mtmp, obj, (struct attack *)0);
			    stackobj(obj);
			    retvalu = 0;
			}
		}
	} else obfree(obj, (struct obj*) 0);
	return retvalu;
}

#endif /* OVLB */
#ifdef OVL1

/* an object launched by someone/thing other than player attacks a monster;
   return 1 if the object has stopped moving (hit or its range used up) */
int
ohitmon(mtmp, otmp, range, verbose)
struct monst *mtmp;	/* accidental target */
struct obj *otmp;	/* missile; might be destroyed by drop_throw */
int range;		/* how much farther will object travel if it misses */
			/* Use -1 to signify to keep going even after hit, */
			/* unless its gone (used for rolling_boulder_traps) */
boolean verbose;  /* give message(s) even when you can't see what happened */
{
	int damage, tmp;
	boolean vis, ismimic;
	int objgone = 1;

	ismimic = mtmp->m_ap_type && mtmp->m_ap_type != M_AP_MONSTER;
	vis = cansee(bhitpos.x, bhitpos.y);

	tmp = 5 + find_mac(mtmp) + omon_adj(mtmp, otmp, FALSE);
	if (tmp < rnd(20)) {
	    if (!ismimic) {
		if (vis) miss(distant_name(otmp, mshot_xname), mtmp);
		else if (verbose) pline(E_J("It is missed.","何かは外れた。"));
	    }
	    if (!range) { /* Last position; object drops */
		(void) drop_throw(otmp, 0, mtmp->mx, mtmp->my);
		return 1;
	    }
	} else if (otmp->oclass == POTION_CLASS) {
	    if (ismimic) seemimic(mtmp);
	    mtmp->msleeping = 0;
	    if (vis) otmp->dknown = 1;
	    potionhit(mtmp, otmp, FALSE);
	    return 1;
	} else {
	    damage = dmgval(otmp, mtmp);
	    if (otmp->otyp == ACID_VENOM && resists_acid(mtmp))
		damage = 0;
	    if (ismimic) seemimic(mtmp);
	    mtmp->msleeping = 0;
#ifndef JP
	    if (vis) hit(distant_name(otmp,mshot_xname), mtmp, exclam(damage));
	    else if (verbose) pline("%s is hit%s", Monnam(mtmp), exclam(damage));
#else
	    hit(vis ? distant_name(otmp,mshot_xname) : something, mtmp, exclam(damage));
#endif /*JP*/

	    if (otmp->opoisoned && is_poisonable(otmp)) {
		if (resists_poison(mtmp)) {
		    if (vis) pline_The(E_J("poison doesn't seem to affect %s.",
					   "毒は%sには効かないようだ。"),
				   mon_nam(mtmp));
		} else {
		    if (rn2(30)) {
			damage += rnd(6);
		    } else {
			if (vis) pline_The(E_J("poison was deadly...","毒は致死的だった…。"));
			damage = mtmp->mhp;
		    }
		}
	    }
	    if (get_material(otmp) == SILVER &&
		    hates_silver(mtmp->data)) {
		if (vis) pline_The(E_J("silver sears %s flesh!","銀が%s身体を焼いた！"),
				s_suffix(mon_nam(mtmp)));
		else if (verbose) pline(E_J("Its flesh is seared!","銀が何者かの身体を焼いた！"));
	    }
	    if (otmp->otyp == ACID_VENOM && cansee(mtmp->mx,mtmp->my)) {
		if (resists_acid(mtmp)) {
		    if (vis || verbose)
			pline(E_J("%s is unaffected.","%sは影響を受けなかった。"), Monnam(mtmp));
		    damage = 0;
		} else {
		    if (vis) pline_The(E_J("acid burns %s!","酸が%sの身体を灼いた！"), mon_nam(mtmp));
		    else if (verbose) pline(E_J("It is burned!","酸が何者かの身体を灼いた！"));
		}
	    }
	    mtmp->mhp -= damage;
	    if (mtmp->mhp < 1) {
		if (vis || verbose)
		    pline(E_J("%s is %s!","%sは%sされた！"), Monnam(mtmp),
			(nonliving(mtmp->data) || !canspotmon(mtmp))
			? E_J("destroyed","倒") : E_J("killed","殺"));
		/* don't blame hero for unknown rolling boulder trap */
		if (!flags.mon_moving &&
		    (otmp->otyp != BOULDER || range >= 0 || !otmp->otrapped))
		    xkilled(mtmp,0);
		else mondied(mtmp);
	    }

	    if (can_blnd((struct monst*)0, mtmp,
		    (uchar)(otmp->otyp == BLINDING_VENOM ? AT_SPIT : AT_WEAP),
		    otmp)) {
		if (vis && mtmp->mcansee)
#ifndef JP
		    pline("%s is blinded by %s.", Monnam(mtmp), the(xname(otmp)));
#else
		    pline("%sは%sによって盲目にされた。", mon_nam(mtmp), xname(otmp));
#endif /*JP*/
		mtmp->mcansee = 0;
		tmp = (int)mtmp->mblinded + rnd(25) + 20;
		if (tmp > 127) tmp = 127;
		mtmp->mblinded = tmp;
	    }

	    objgone = drop_throw(otmp, 1, bhitpos.x, bhitpos.y);
	    if (!objgone && range == -1) {  /* special case */
		    obj_extract_self(otmp); /* free it for motion again */
		    return 0;
	    }
	    return 1;
	}
	return 0;
}

void
m_throw(mon, x, y, dx, dy, range, obj)
	register struct monst *mon;
	register int x,y,dx,dy,range;		/* direction and range */
	register struct obj *obj;
{
	register struct monst *mtmp;
	struct obj *singleobj;
	char sym = obj->oclass;
	int hitu, blindinc = 0;
	int lx, ly;
	boolean chk;

	if (is_bullet(obj) && obj->oshot) sym = 0;

	if (obj->quan == 1L) {
	    /*
	     * Remove object from minvent.  This cannot be done later on;
	     * what if the player dies before then, leaving the monster
	     * with 0 daggers?  (This caused the infamous 2^32-1 orcish
	     * dagger bug).
	     *
	     * VENOM is not in minvent - it should already be OBJ_FREE.
	     * The extract below does nothing.
	     */

	    /* not possibly_unwield, which checks the object's */
	    /* location, not its existence */
	    if (MON_WEP(mon) == obj) {
		    setmnotwielded(mon,obj);
		    MON_NOWEP(mon);
	    }
	    obj_extract_self(obj);
	    singleobj = obj;
	    obj = (struct obj *) 0;
	} else {
	    singleobj = splitobj(obj, 1L);
	    obj_extract_self(singleobj);
	}

	singleobj->owornmask = 0; /* threw one of multiple weapons in hand? */
//	if (singleobj->oclass == WEAPON_CLASS)
//	    singleobj->leashmon = mon->m_id; /* mark the shooter */

	if (singleobj->cursed && (dx || dy) && !rn2(7)) {
	    if(canseemon(mon) && flags.verbose) {
		if(is_ammo(singleobj))
		    pline(E_J("%s misfires!","%sは目標を誤った！"), Monnam(mon));
		else
#ifndef JP
		    pline("%s as %s throws it!",
			  Tobjnam(singleobj, "slip"), mon_nam(mon));
#else
		    pline("%sは%sを投げようとしたが、手を滑らせてしまった！",
			  mon_nam(mon), xname(singleobj));
#endif /*JP*/
	    }
	    dx = rn2(3)-1;
	    dy = rn2(3)-1;
	    /* check validity of new direction */
	    if (!dx && !dy) {
		(void) drop_throw(singleobj, 0, x, y);
		return;
	    }
	}

	bresenham_init(&bhitpos, x, y, x+dx, y+dy);

	/* pre-check for doors, walls and boundaries.
	   Also need to pre-check for bars regardless of direction;
	   the random chance for small objects hitting bars is
	   skipped when reaching them at point blank range */
	bresenham_step(&bhitpos);
	if (!isok(bhitpos.x,bhitpos.y)
	    || IS_ROCK(levl[bhitpos.x][bhitpos.y].typ)
	    || closed_door(bhitpos.x, bhitpos.y)
	    || (levl[bhitpos.x][bhitpos.y].typ == IRONBARS &&
		hits_bars(&singleobj, x, y, 0, 0))) {
	    if (singleobj)
		(void) drop_throw(singleobj, 0, x, y);
	    return;
	}

	/* Note: drop_throw may destroy singleobj.  Since obj must be destroyed
	 * early to avoid the dagger bug, anyone who modifies this code should
	 * be careful not to use either one after it's been freed.
	 */
	if (sym) tmp_at(DISP_FLASH, obj_to_glyph(singleobj));
	while(1) { /* Actually the loop is always exited by break */
		chk = FALSE;
		if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
		    if (ohitmon(mtmp, singleobj, range, TRUE))
			break;
		} else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
		    if (multi) nomul(0);

		    if (singleobj->oclass == GEM_CLASS &&
			    singleobj->otyp <= LAST_GEM+9 /* 9 glass colors */
			    && is_unicorn(youmonst.data)) {
			if (singleobj->otyp > LAST_GEM) {
			    You(E_J("catch the %s.","%sを受け止めた。"), xname(singleobj));
			    You(E_J("are not interested in %s junk.",
				    "%sガラクタには興味を示さなかった。"),
				s_suffix(mon_nam(mon)));
			    makeknown(singleobj->otyp);
			    dropy(singleobj);
			} else {
			    You(E_J("accept %s gift in the spirit in which it was intended.",
				    "%s贈り物を意図されたとおりの意味で受け取った。"),
				s_suffix(mon_nam(mon)));
			    (void)hold_another_object(singleobj,
				E_J("You catch, but drop, %s.",
				    "あなたは%sを受け止めたが、落としてしまった。"), xname(singleobj),
				E_J("You catch:","あなたは受け止めた:"));
			}
			break;
		    }
		    if (singleobj->oclass == POTION_CLASS) {
			if (!Blind) singleobj->dknown = 1;
			if (missile_hits_shield(&youmonst, singleobj))
			    break;
			potionhit(&youmonst, singleobj, FALSE);
			break;
		    }
		    switch(singleobj->otyp) {
			int dam, hitv;
			case EGG:
			    if (!touch_petrifies(&mons[singleobj->corpsenm])) {
				impossible("monster throwing egg type %d",
					singleobj->corpsenm);
				hitu = 0;
				break;
			    }
			    /* fall through */
			case CREAM_PIE:
			case BLINDING_VENOM:
			    hitu = thitu(8, 0, singleobj, (char *)0);
			    break;
			default:
			    dam = dmgval(singleobj, &youmonst);
			    hitv = 3 - distmin(u.ux,u.uy, mon->mx,mon->my);
			    hitv += mon->m_lev;
			    if (hitv < -4) hitv = -4;
			    if (is_elf(mon->data) &&
				objects[singleobj->otyp].oc_skill == P_BOW_GROUP) {
				hitv++;
				if (MON_WEP(mon) &&
				    MON_WEP(mon)->otyp == ELVEN_BOW)
				    hitv++;
				if(singleobj->otyp == ELVEN_ARROW) dam++;
			    } else if (is_giant(mon->data) && singleobj->otyp == BOULDER) {
				hitv += 10;
			    }
			    if (bigmonst(youmonst.data)) hitv++;
			    hitv += 8 + singleobj->spe;
			    if (dam < 1) dam = 1;
			    hitu = thitu(hitv, dam, singleobj, (char *)0);
		    }
		    if (hitu && singleobj->opoisoned &&
			is_poisonable(singleobj)) {
			char onmbuf[BUFSZ], knmbuf[BUFSZ];

			Strcpy(onmbuf, xname(singleobj));
#ifndef JP
			Strcpy(knmbuf, killer_xname(singleobj));
#else
			Sprintf(knmbuf, "%sの毒で殺された", killer_xname(singleobj));
#endif /*JP*/
			poisoned(onmbuf, A_STR, knmbuf, -10);
		    }
		    if(hitu &&
		       can_blnd((struct monst*)0, &youmonst,
				(uchar)(singleobj->otyp == BLINDING_VENOM ?
					AT_SPIT : AT_WEAP), singleobj)) {
			blindinc = rnd(25);
			if(singleobj->otyp == CREAM_PIE) {
			    if(!Blind) pline(E_J("Yecch!  You've been creamed.",
						 "うえぇ！ あなたはクリームまみれになった。"));
			    else pline(E_J("There's %s sticky all over your %s.",
					   "%sべとべとするものがあなたの%sを覆った。"),
				       something,
				       body_part(FACE));
			} else if(singleobj->otyp == BLINDING_VENOM) {
			    int num_eyes = eyecount(youmonst.data);
			    /* venom in the eyes */
			    if(!Blind) pline_The(E_J("venom blinds you.",
						     "毒液があなたの視界を奪った。"));
#ifndef JP
			    else Your("%s sting%s.",
				      (num_eyes == 1) ? body_part(EYE) :
						makeplural(body_part(EYE)),
				      (num_eyes == 1) ? "s" : "");
#else
			    else Your("%sに鋭い痛みが走った。", body_part(EYE));
#endif /*JP*/
			}
		    }
		    if (hitu && singleobj->otyp == EGG) {
			if (!Stone_resistance
			    && !(poly_when_stoned(youmonst.data) &&
				 polymon(PM_STONE_GOLEM))) {
			    Stoned = 5;
			    killer = (char *) 0;
			}
		    }
		    stop_occupation();
		    if (hitu /*|| !range*/) {
			(void) drop_throw(singleobj, hitu, u.ux, u.uy);
			break;
		    }
		    chk = TRUE;
		} else {
		    chk = TRUE;
		}
		lx = bhitpos.x;
		ly = bhitpos.y;
		bresenham_step(&bhitpos);
		if (chk) {
		    if ((dist2(x,y,bhitpos.x,bhitpos.y) > range) /* reached end of path */
			/* missile hits edge of screen */
			|| !isok(bhitpos.x,bhitpos.y)
			/* missile hits the wall */
			|| IS_ROCK(levl[bhitpos.x][bhitpos.y].typ)
			/* missile hit closed door */
			|| closed_door(bhitpos.x, bhitpos.y)
			/* missile might hit iron bars */
			|| (levl[bhitpos.x][bhitpos.y].typ == IRONBARS &&
			hits_bars(&singleobj, lx, ly, !rn2(5), 0))
#ifdef SINKS
			/* Thrown objects "sink" */
			|| IS_SINK(levl[bhitpos.x][bhitpos.y].typ)
#endif
								) {
			if (singleobj) /* hits_bars might have destroyed it */
			    (void) drop_throw(singleobj, 0, lx, ly);
			bresenham_back(&bhitpos);
			break;
		    }
		}
		if (sym) {
			tmp_at(lx, ly);
			delay_output();
		}
	}
	if (sym) {
		tmp_at(bhitpos.x, bhitpos.y);
		delay_output();
		tmp_at(DISP_END, 0);
	}

	if (blindinc) {
		u.ucreamed += blindinc;
		make_blinded(Blinded + (long)blindinc, FALSE);
		if (!Blind) Your(vision_clears);
	}
}

#endif /* OVL1 */
#ifdef OVLB

/* Remove an item from the monster's inventory and destroy it. */
void
m_useup(mon, obj)
struct monst *mon;
struct obj *obj;
{
	if (obj->quan > 1L) {
		obj->quan--;
		obj->owt = weight(obj);
	} else {
		obj_extract_self(obj);
		possibly_unwield(mon, FALSE);
		if (obj->owornmask) {
		    mon->misc_worn_check &= ~obj->owornmask;
		    update_mon_intrinsics(mon, obj, FALSE, FALSE);
		}
		obfree(obj, (struct obj*) 0);
	}
}

#endif /* OVLB */
#ifdef OVL1

/* monster attempts ranged weapon attack against player */
/* Return value --> 0:normal  1:polearms(caller should process it) */
int
thrwmu(mtmp)
struct monst *mtmp;
{
	struct obj *otmp, *mwep;
	xchar x, y;
	schar skill;
	int multishot;
	const char *onm;

	/* Rearranged beginning so monsters can use polearms not in a line */
	if (mtmp->weapon_check == NEED_WEAPON || !MON_WEP(mtmp)) {
	    mtmp->weapon_check = NEED_RANGED_WEAPON;
	    /* mon_wield_item resets weapon_check as appropriate */
	    if(mon_wield_item(mtmp) != 0) return 0;
	}

	/* Pick a weapon */
	otmp = select_rwep(mtmp);
	if (!otmp) return 0;

	/* polearms */
	if (is_ranged(otmp)) {
	    if (dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) > POLE_LIM ||
		    !couldsee(mtmp->mx, mtmp->my))
		return 0;	/* Out of range, or intervening wall */
	    return 1;
	}

	x = mtmp->mx;
	y = mtmp->my;
	/* If you are coming toward the monster, the monster
	 * should try to soften you up with missiles.  If you are
	 * going away, you are probably hurt or running.  Give
	 * chase, but if you are getting too far away, throw.
	 */
	if (!lined_up2(mtmp) ||
		(URETREATING(x,y) &&
			rn2(BOLT_LIM - distmin(x,y,mtmp->mux,mtmp->muy))))
	    return 0;

	skill = objects[otmp->otyp].oc_skill;
	mwep = MON_WEP(mtmp);		/* wielded weapon */

	if (is_gun(otmp))
	    if (mwep == otmp) {
		if (firemu(mtmp, mwep)) return 0;
	    } else return 0; /* do not throw a gun itself */

	/* Multishot calculations */
	multishot = 1;
	if ((ammo_and_launcher(otmp, mwep) || skill == P_DAGGER_GROUP ||
		skill == P_THROWING_GROUP) && !mtmp->mconf) {
	    /* Assumes lords are skilled, princes are expert */
	    if (is_prince(mtmp->data)) multishot += 2;
	    else if (is_lord(mtmp->data)) multishot++;

	    switch (monsndx(mtmp->data)) {
	    case PM_RANGER:
		    multishot++;
		    break;
	    case PM_ROGUE:
		    if (skill == P_DAGGER_GROUP) multishot++;
		    break;
	    case PM_NINJA:
	    case PM_SAMURAI:
		    if (otmp->otyp == YA && mwep &&
			mwep->otyp == YUMI) multishot++;
		    break;
	    default:
		break;
	    }
	    /* racial bonus */
	    if ((is_elf(mtmp->data) &&
		    otmp->otyp == ELVEN_ARROW &&
		    mwep && mwep->otyp == ELVEN_BOW) ||
		(is_orc(mtmp->data) &&
		    otmp->otyp == ORCISH_ARROW &&
		    mwep && mwep->otyp == ORCISH_BOW))
		multishot++;

	    if ((long)multishot > otmp->quan) multishot = (int)otmp->quan;
	    if (multishot < 1) multishot = 1;
	    else multishot = rnd(multishot);
	}

	if (canseemon(mtmp)) {
	    char onmbuf[BUFSZ];

	    if (multishot > 1) {
		/* "N arrows"; multishot > 1 implies otmp->quan > 1, so
		   xname()'s result will already be pluralized */
#ifndef JP
		Sprintf(onmbuf, "%d %s", multishot, xname(otmp));
#else
		Sprintf(onmbuf, "%d%sの%s", multishot, jjosushi(otmp), xname(otmp));
#endif /*JP*/
		onm = onmbuf;
	    } else {
		/* "an arrow" */
		onm = singular(otmp, xname);
#ifndef JP
		onm = obj_is_pname(otmp) ? the(onm) : an(onm);
#endif /*JP*/
	    }
	    m_shot.s = ammo_and_launcher(otmp,mwep) ? TRUE : FALSE;
#ifndef JP
	    pline("%s %s %s!", Monnam(mtmp),
		  m_shot.s ? "shoots" : "throws", onm);
#else
	    pline("%sは%sを%sた！", mon_nam(mtmp), onm,
		  m_shot.s ? "撃っ" : "投げ");
#endif /*JP*/
	    m_shot.o = otmp->otyp;
	} else {
	    m_shot.o = STRANGE_OBJECT;	/* don't give multishot feedback */
	}

	m_shot.n = multishot;
	for (m_shot.i = 1; m_shot.i <= m_shot.n; m_shot.i++)
	    m_throw(mtmp, mtmp->mx, mtmp->my, tbx/*sgn(tbx)*/, tby/*sgn(tby)*/,
		    dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy), otmp);
	m_shot.n = m_shot.i = 0;
	m_shot.o = STRANGE_OBJECT;
	m_shot.s = FALSE;

	nomul(0);
	return 0;
}

#endif /* OVL1 */
#ifdef OVLB

int
spitmu(mtmp, mattk)		/* monster spits substance at you */
register struct monst *mtmp;
register struct attack *mattk;
{
	register struct obj *otmp;

	if(mtmp->mcan) {

	    if(flags.soundok)
		pline(E_J("A dry rattle comes from %s throat.",
			  "%s喉から乾いた音ががらがらと鳴った。"),
		                      s_suffix(mon_nam(mtmp)));
	    return 0;
	}
	if(lined_up(mtmp)) {
		switch (mattk->adtyp) {
		    case AD_BLND:
		    case AD_DRST:
			otmp = mksobj(BLINDING_VENOM, TRUE, FALSE);
			break;
		    default:
			impossible("bad attack type in spitmu");
				/* fall through */
		    case AD_ACID:
			otmp = mksobj(ACID_VENOM, TRUE, FALSE);
			break;
		}
		if(!rn2(BOLT_LIM-distmin(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy))) {
		    if (canseemon(mtmp))
			pline(E_J("%s spits venom!","%sは毒液を吐きかけた！"), Monnam(mtmp));
		    m_throw(mtmp, mtmp->mx, mtmp->my, tbx/*sgn(tbx)*/, tby/*sgn(tby)*/,
			dist2(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy), otmp);
		    nomul(0);
		    return 0;
		}
	}
	return 0;
}

#endif /* OVLB */
#ifdef OVL1

int
breamu(mtmp, mattk)			/* monster breathes at you (ranged) */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	/* if new breath types are added, change AD_ACID to max type */
	int typ = (mattk->adtyp == AD_RBRE) ? rnd(AD_PLYS) : mattk->adtyp ;

	if(/*lined_up(mtmp)*/lined_up2(mtmp)) {

	    if(mtmp->mcan) {
		if(flags.soundok) {
		    if(canseemon(mtmp))
			pline(E_J("%s coughs.","%sは咳きこんだ。"), Monnam(mtmp));
		    else
			You_hear(E_J("a cough.","咳を"));
		}
		return(0);
	    }
	    if(!mtmp->mspec_used && rn2(3)) {

		if((typ >= AD_MAGM) && (typ <= AD_PLYS)) {

		    if(canseemon(mtmp))
			pline(E_J("%s breathes %s!","%sは%sを吐いた！"), Monnam(mtmp),
			      breathwep[typ-1]);
		    buzz((int) (-20 - (typ-1)), (int)mattk->damn,
			 mtmp->mx, mtmp->my, /*sgn*/(tbx), /*sgn*/(tby));
		    nomul(0);
		    /* breath runs out sometimes. Also, give monster some
		     * cunning; don't breath if the player fell asleep.
		     */
		    if(!rn2(3))
			mtmp->mspec_used = 10+rn2(20);
		    if ((typ == AD_SLEE && !Sleep_resistance) ||
			(typ == AD_PLYS && !Free_action))
			mtmp->mspec_used += rnd(20);
		} else impossible("Breath weapon %d used", typ-1);
	    }
	}
	return(1);
}

boolean
linedup(ax, ay, bx, by)
register xchar ax, ay, bx, by;
{
	tbx = ax - bx;	/* These two values are set for use */
	tby = ay - by;	/* after successful return.	    */

	/* sometimes displacement makes a monster think that you're at its
	   own location; prevent it from throwing and zapping in that case */
	if (!tbx && !tby) return FALSE;

	if((!tbx || !tby || abs(tbx) == abs(tby)) /* straight line or diagonal */
	   && distmin(tbx, tby, 0, 0) < BOLT_LIM) {
	    if(ax == u.ux && ay == u.uy) return((boolean)(couldsee(bx,by)));
	    else if(clear_path(ax,ay,bx,by)) return TRUE;
	}
	return FALSE;
}

boolean
lined_up(mtmp)		/* is mtmp in position to use ranged attack? */
	register struct monst *mtmp;
{
	return(linedup(mtmp->mux,mtmp->muy,mtmp->mx,mtmp->my));
}

boolean
lined_up2(mtmp)
struct monst *mtmp;
{
	int ax, ay, bx, by;
	ax = mtmp->mux;	ay = mtmp->muy;
	bx = mtmp->mx;	by = mtmp->my;
	tbx = ax - bx;	/* These two values are set for use */
	tby = ay - by;	/* after successful return.	    */
	if (!tbx && !tby) return FALSE;
	/*if(ax == u.ux && ay == u.uy) return((boolean)(couldsee(bx,by)));*/
	return (boolean)(clear_path(bx,by,ax,ay));
}

boolean			/* TRUE: mon did something  FALSE: mon did nothing */
firemu(mtmp, ogun)
struct monst *mtmp;
struct obj *ogun;
{
	const char *onm;
	struct obj *oblt;

	if (!is_gun(ogun) || !can_use_gun(mtmp->data)) return FALSE;

	if (!ogun->cobj) {
		/* no bullets in the gun. Try reloading */
		int mb = maxbullets(ogun);
		oblt = m_carrying(mtmp, BULLET);
		if (!oblt) return FALSE;
		if (oblt->quan > mb)
			oblt = splitobj(oblt, mb);
		obj_extract_self(oblt);
		oblt->nobj = (struct obj *)0;
		oblt->where = OBJ_CONTAINED;
		oblt->ocontainer = ogun;
		ogun->cobj = oblt;
		if (canseemon(mtmp)) {
			onm = xname(ogun);
#ifndef JP
			pline("%s loads %s with %s.", Monnam(mtmp),
				obj_is_pname(ogun) ? the(onm) : an(onm),
				doname(oblt));
#else
			pline("%sは%sに%sを込めた。", mon_nam(mtmp), onm,
				doname(oblt));
#endif /*JP*/
		}
		return TRUE;
	}

	/* get one bullet from the gun */
	oblt = ogun->cobj;
	if (oblt->quan > 1)
		oblt = splitobj(oblt, 1);
	obj_extract_self(oblt);
	oblt->oshot = 1;

	if (canseemon(mtmp)) {
		onm = xname(ogun);
#ifndef JP
		pline("%s fires %s!", Monnam(mtmp),
			obj_is_pname(ogun) ? the(onm) : an(onm));
#else
		pline("%sは%sを撃った！", mon_nam(mtmp), onm);
#endif /*JP*/
	}
	wake_nearby();	/* gunshot */

	m_throw(mtmp, mtmp->mx, mtmp->my, tbx/*sgn(tbx)*/, tby/*sgn(tby)*/,
		dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy), oblt);
	nomul(0);
	return TRUE;
}

#endif /* OVL1 */
#ifdef OVL0

/* Check if a monster is carrying a particular item.
 */
struct obj *
m_carrying(mtmp, type)
struct monst *mtmp;
int type;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == type)
			return(otmp);
	return((struct obj *) 0);
}

/* TRUE iff thrown/kicked/rolled object doesn't pass through iron bars */
boolean
hits_bars(obj_p, x, y, always_hit, whodidit)
struct obj **obj_p;	/* *obj_p will be set to NULL if object breaks */
int x, y;
int always_hit;	/* caller can force a hit for items which would fit through */
int whodidit;	/* 1==hero, 0=other, -1==just check whether it'll pass thru */
{
    struct obj *otmp = *obj_p;
    int obj_type = otmp->otyp;
    boolean hits = always_hit;

    if (!hits)
	switch (otmp->oclass) {
	case WEAPON_CLASS:
	    {
		int oskill = objects[obj_type].oc_skill;
		int wprop = (int)objects[obj_type].oc_wprop;

		hits = ((wprop & WP_WEAPONTYPE) != WP_AMMUNITION &&
			obj_type != DART && obj_type != SHURIKEN &&
			oskill != P_SPEAR_GROUP &&
			oskill != P_KNIFE_GROUP);	/* but not dagger */
		break;
	    }
	case ARMOR_CLASS:
		hits = (objects[obj_type].oc_armcat != ARM_GLOVES);
		break;
	case TOOL_CLASS:
		hits = (obj_type != SKELETON_KEY &&
			obj_type != LOCK_PICK &&
#ifdef TOURIST
			obj_type != CREDIT_CARD &&
#endif
			obj_type != TALLOW_CANDLE &&
			obj_type != WAX_CANDLE &&
			obj_type != MAGIC_CANDLE &&
#ifndef MAGIC_GLASSES
			obj_type != LENSES &&
#else
			!Is_glasses(otmp) &&
#endif /*MAGIC_GLASSES*/
			obj_type != TIN_WHISTLE &&
			obj_type != MAGIC_WHISTLE);
		break;
	case ROCK_CLASS:	/* includes boulder */
		if (obj_type != STATUE ||
			mons[otmp->corpsenm].msize > MZ_TINY) hits = TRUE;
		break;
	case FOOD_CLASS:
		if (obj_type == CORPSE &&
			mons[otmp->corpsenm].msize > MZ_TINY) hits = TRUE;
		else
		    hits = (obj_type == MEAT_STICK ||
			    obj_type == HUGE_CHUNK_OF_MEAT);
		break;
	case SPBOOK_CLASS:
	case WAND_CLASS:
	case BALL_CLASS:
	case CHAIN_CLASS:
		hits = TRUE;
		break;
	default:
		break;
	}

    if (hits && whodidit != -1) {
	if (whodidit ? hero_breaks(otmp, x, y, FALSE) : breaks(otmp, x, y))
	    *obj_p = otmp = 0;		/* object is now gone */
	    /* breakage makes its own noises */
	else if (obj_type == BOULDER || obj_type == HEAVY_IRON_BALL)
	    pline(E_J("Whang!","グワン！"));
	else if (otmp->oclass == COIN_CLASS ||
		objects[obj_type].oc_material == GOLD ||
		objects[obj_type].oc_material == SILVER)
	    pline(E_J("Clink!","キン！"));
	else
	    pline(E_J("Clonk!","カキン！"));
    }

    return hits;
}

#endif /* OVL0 */

/*mthrowu.c*/
