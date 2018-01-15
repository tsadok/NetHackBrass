/*	SCCS Id: @(#)do.c	3.4	2003/12/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* Contains code for 'd', 'D' (drop), '>', '<' (up, down) */

#include "hack.h"
#include "lev.h"

#ifdef SINKS
# ifdef OVLB
STATIC_DCL void FDECL(trycall, (struct obj *));
# endif /* OVLB */
STATIC_DCL void FDECL(dosinkring, (struct obj *));
#endif /* SINKS */

STATIC_PTR int FDECL(drop, (struct obj *));
STATIC_PTR int NDECL(wipeoff);

#ifdef OVL0
STATIC_DCL int FDECL(menu_drop, (int));
#endif
#ifdef OVL2
STATIC_DCL int NDECL(currentlevel_rewrite);
STATIC_DCL void NDECL(final_level);
/* static boolean FDECL(badspot, (XCHAR_P,XCHAR_P)); */
#endif

#ifdef OVLB

static NEARDATA const char drop_types[] =
	{ ALLOW_COUNT, COIN_CLASS, ALL_CLASSES, 0 };
#ifdef JP
static const struct getobj_words drpw = { 0, 0, "���Ƃ�", "���Ƃ�" };
#endif /*JP*/

/* 'd' command: drop one inventory item */
int
dodrop()
{
#ifndef GOLDOBJ
	int result, i = (invent || u.ugold) ? 0 : (SIZE(drop_types) - 1);
#else
	int result, i = (invent) ? 0 : (SIZE(drop_types) - 1);
#endif
	if (*u.ushops) sellobj_state(SELL_DELIBERATE);
	result = drop(getobj(&drop_types[i], E_J("drop",&drpw), 0));
	if (*u.ushops) sellobj_state(SELL_NORMAL);
	reset_occupations();

	return result;
}

#endif /* OVLB */
#ifdef OVL0

/* Called when a boulder is dropped, thrown, or pushed.  If it ends up
 * in a pool, it either fills the pool up or sinks away.  In either case,
 * it's gone for good...  If the destination is not a pool, returns FALSE.
 */
boolean
boulder_hits_pool(otmp, rx, ry, pushing)
struct obj *otmp;
register int rx, ry;
boolean pushing;
{
	if (!otmp || otmp->otyp != BOULDER)
	    impossible("Not a boulder?");
	else if (!Is_waterlevel(&u.uz) &&
		 (is_pool(rx,ry) || is_lava(rx,ry) || is_swamp(rx,ry))) {
	    boolean lava = is_lava(rx,ry), fills_up;
	    boolean swamp = is_swamp(rx,ry);
 	    const char *what = waterbody_name(rx,ry);
	    schar ltyp = levl[rx][ry].typ;
	    int chance = rn2(10);		/* water: 90%; lava: 10% */
	    fills_up = swamp ? 1 : lava ? chance == 0 : chance != 0;

	    if (fills_up) {
		struct trap *ttmp = t_at(rx, ry);

		if (ltyp == DRAWBRIDGE_UP) {
		    levl[rx][ry].drawbridgemask &= ~DB_UNDER; /* clear lava */
		    levl[rx][ry].drawbridgemask |= DB_FLOOR;
		} else
		    levl[rx][ry].typ = ROOM;

		if (ttmp) (void) delfloortrap(ttmp);
		bury_objs(rx, ry);
		
		newsym(rx,ry);
		if (pushing) {
#ifndef JP
		    You("push %s into the %s.", the(xname(otmp)), what);
#else
		    You("%s��%s�ɉ������񂾁B", xname(otmp), what);
#endif /*JP*/
		    if (flags.verbose && !Blind)
			pline(E_J("Now you can cross it!","����œn���悤�ɂȂ����I"));
		    /* no splashing in this case */
		}
	    }
	    if (!fills_up || !pushing) {	/* splashing occurs */
		if (!u.uinwater) {
		    if (pushing ? !Blind : cansee(rx,ry)) {
#ifndef JP
			There("is a large splash as %s %s the %s.",
			      the(xname(otmp)), fills_up? "fills":"falls into",
			      what);
#else
			pline("%s��%s�𐷑�ɂ͂ˏグ��%s%s�B",
			      xname(otmp), lava ? "�}�O�}�̔�" : "�����Ԃ�",
			      what, fills_up? "�𖄂߂�":"�̒��ɗ�����");
#endif /*JP*/
		    } else if (flags.soundok)
#ifndef JP
			You_hear("a%s splash.", lava ? " sizzling" : "");
#else
			You_hear(lava ? "�n��̂͂˂鉹��" : "�����Ԃ��̉���");
#endif /*JP*/
		    wake_nearto(rx, ry, 40);
		}

		if (fills_up && u.uinwater && distu(rx,ry) == 0) {
		    u.uinwater = 0;
		    docrt();
		    vision_full_recalc = 1;
		    You(E_J("find yourself on dry land again!",
			    "�Ăъ������n�ʂ̏�ɖ߂����I"));
		} else if (lava && distu(rx,ry) <= 2) {
		    You(E_J("are hit by molten lava%c","�ς�������n��ɑł��ꂽ%s"),
			Fire_resistance ? E_J('.',"�B") : E_J('!',"�I"));
			burn_away_slime();
		    if (!is_full_resist(FIRE_RES))
			losehp(d((Fire_resistance ? 1 : 3), 6),
			       E_J("molten lava","�ς�������n��ɓ�������"), KILLED_BY);
		} else if (!fills_up && flags.verbose &&
			   (pushing ? !Blind : cansee(rx,ry)))
#ifndef JP
		    pline("It sinks without a trace!");
#else
		    pline("%s�͐Ռ`���Ȃ�����ł������I", xname(otmp));
#endif /*JP*/
	    }

	    /* boulder is now gone */
	    if (pushing) delobj(otmp);
	    else obfree(otmp, (struct obj *)0);
	    return TRUE;
	}
	return FALSE;
}

/* Used for objects which sometimes do special things when dropped; must be
 * called with the object not in any chain.  Returns TRUE if the object goes
 * away.
 */
boolean
flooreffects(obj,x,y,verb)
struct obj *obj;
int x,y;
const char *verb;
{
	struct trap *t;
	struct monst *mtmp;

	if (obj->where != OBJ_FREE)
	    panic("flooreffects: obj not free");

	/* make sure things like water_damage() have no pointers to follow */
	obj->nobj = obj->nexthere = (struct obj *)0;

	if (obj->etherial) {
//		pline("%s %s to dust.", Doname2(obj),
//			otense(obj, "crumble"));
		obfree(obj, (struct obj *)0);
		return TRUE;
	}

	if (obj->otyp == BOULDER && boulder_hits_pool(obj, x, y, FALSE))
		return TRUE;
	else if (obj->otyp == BOULDER && (t = t_at(x,y)) != 0 &&
		 (t->ttyp==PIT || t->ttyp==SPIKED_PIT
			|| t->ttyp==TRAPDOOR || t->ttyp==HOLE)) {
		if (((mtmp = m_at(x, y)) && mtmp->mtrapped) ||
			(u.utrap && u.ux == x && u.uy == y)) {
		    if (*verb)
#ifndef JP
			pline_The("boulder %s into the pit%s.",
				vtense((const char *)0, verb),
				(mtmp) ? "" : " with you");
#else
			pline("����%s��������%s�B",
				(mtmp) ? "" : "���Ȃ��ƂƂ���", verb);
#endif /*JP*/
		    if (mtmp) {
			if (!passes_walls(mtmp->data) &&
				!throws_rocks(mtmp->data)) {
			    if (hmon(mtmp, obj, TRUE) && !is_whirly(mtmp->data))
				return FALSE;	/* still alive */
			}
			mtmp->mtrapped = 0;
		    } else {
			if (!Passes_walls && !throws_rocks(youmonst.data)) {
			    losehp(rnd(15), E_J("squished under a boulder",
						"���ɉ����ׂ��ꂽ"),
				   NO_KILLER_PREFIX);
			    return FALSE;	/* player remains trapped */
			} else u.utrap = 0;
		    }
		}
		if (*verb) {
			if (Blind) {
				if ((x == u.ux) && (y == u.uy))
					You_hear(E_J("a CRASH! beneath you.",
						     "�����猃�����Փˉ���"));
				else
					You_hear(E_J("the boulder %s.","��₪%s����"), verb);
			} else if (cansee(x, y)) {
#ifndef JP
				pline_The("boulder %s%s.",
				    t->tseen ? "" : "triggers and ",
				    t->ttyp == TRAPDOOR ? "plugs a trap door" :
				    t->ttyp == HOLE ? "plugs a hole" :
				    "fills a pit");
#else
				pline("��₪%s%s�B",
				    t->tseen ? "" : "�B���ꂽ㩂ɓ˂����݁A",
				    t->ttyp == TRAPDOOR ? "���Ƃ������ӂ�����" :
				    t->ttyp == HOLE ? "�����ӂ�����" :
				    "�������𖄂߂�");
#endif /*JP*/
			}
		}
		deltrap(t);
		obfree(obj, (struct obj *)0);
		bury_objs(x, y);
		newsym(x,y);
		return TRUE;
	} else if (is_lava(x, y)) {
		return fire_damage(obj, FALSE, FALSE, x, y);
	} else if (is_pool(x, y) || is_swamp(x, y)) {
		/* Reasonably bulky objects (arbitrary) splash when dropped.
		 * If you're floating above the water even small things make noise.
		 * Stuff dropped near fountains always misses */
		if ((Blind || (Levitation || Flying)) && flags.soundok &&
		    ((x == u.ux) && (y == u.uy))) {
		    if (!Underwater) {
			if (weight(obj) > 9) {
				pline(E_J("Splash!","�U�u���I"));
		        } else if (Levitation || Flying) {
				pline(E_J("Plop!","�h�{���I"));
		        }
		    }
		    map_background(x, y, 0);
		    newsym(x, y);
		}
		water_damage(obj, FALSE, FALSE);
	} else if (u.ux == x && u.uy == y &&
		(!u.utrap || u.utraptype != TT_PIT) &&
		(t = t_at(x,y)) != 0 && t->tseen &&
			(t->ttyp==PIT || t->ttyp==SPIKED_PIT)) {
		/* you escaped a pit and are standing on the precipice */
		if (Blind && flags.soundok)
#ifndef JP
			You_hear("%s %s downwards.",
				The(xname(obj)), otense(obj, "tumble"));
#else
			You_hear("%s���]���藎���鉹��", xname(obj));
#endif /*JP*/
		else
#ifndef JP
			pline("%s %s into %s pit.",
				The(xname(obj)), otense(obj, "tumble"),
				the_your[t->madeby_u]);
#else
			pline("%s��%s�������ɓ]���藎�����B",
				xname(obj), the_your[t->madeby_u]);
#endif /*JP*/
	}
	return FALSE;
}

#endif /* OVL0 */
#ifdef OVLB

void
doaltarobj(obj)  /* obj is an object dropped on an altar */
	register struct obj *obj;
{
	if (Blind)
		return;

	/* KMH, conduct */
	u.uconduct.gnostic++;

	if ((obj->blessed || obj->cursed) && obj->oclass != COIN_CLASS) {
#ifndef JP
		There("is %s flash as %s %s the altar.",
			an(hcolor(obj->blessed ? NH_AMBER : NH_BLACK)),
			doname(obj), otense(obj, "hit"));
#else
		pline("%s�͍Ւd�ɐG���ƁA%s�������B",
			doname(obj),
			j_no_ni(hcolor(obj->blessed ? NH_AMBER : NH_BLACK)));
#endif /*JP*/
		if (!Hallucination) obj->bknown = 1;
	} else {
#ifndef JP
		pline("%s %s on the altar.", Doname2(obj),
			otense(obj, "land"));
#else
		pline("%s���Ւd�ɒu���ꂽ�B", doname(obj));
#endif /*JP*/
		obj->bknown = 1;
	}
}

#ifdef SINKS
STATIC_OVL
void
trycall(obj)
register struct obj *obj;
{
	if(!objects[obj->otyp].oc_name_known &&
	   !objects[obj->otyp].oc_uname)
	   docall(obj);
}

STATIC_OVL
void
dosinkring(obj)  /* obj is a ring being dropped over a kitchen sink */
register struct obj *obj;
{
	register struct obj *otmp,*otmp2;
	register boolean ideed = TRUE;

	You(E_J("drop %s down the drain.","%s��r�����ɗ��Ƃ����B"), doname(obj));
	obj->in_use = TRUE;	/* block free identification via interrupt */
	switch(obj->otyp) {	/* effects that can be noticed without eyes */
	    case RIN_SEARCHING:
		You(E_J("thought your %s got lost in the sink, but there it is!",
			"%s��������Ɏ���ꂽ���Ǝv�������A�w�ւ͂����ɂ������I"),
			xname(obj));
		goto giveback;
	    case RIN_SLOW_DIGESTION:
		pline_The(E_J("ring is regurgitated!","�w�ւ͓f���߂��ꂽ�I"));
giveback:
		obj->in_use = FALSE;
		dropx(obj);
		trycall(obj);
		return;
	    case RIN_LEVITATION:
		pline_The(E_J("sink quivers upward for a moment.",
			      "������͈�u��Ɍ������Đk�����B"));
		break;
	    case RIN_POISON_RESISTANCE:
#ifndef JP
		You("smell rotten %s.", makeplural(fruitname(FALSE)));
#else
		You("������%s�̏L����k�����B", fruitname(FALSE));
#endif /*JP*/
		break;
	    case RIN_AGGRAVATE_MONSTER:
		pline(E_J("Several flies buzz angrily around the sink.",
			  "��������̔���������̎����{�����悤�ɔ�щ�����B"));
		break;
	    case RIN_SHOCK_RESISTANCE:
		pline(E_J("Static electricity surrounds the sink.",
			  "�Ód�C������������������B"));
		break;
	    case RIN_CONFLICT:
		You_hear(E_J("loud noises coming from the drain.",
			     "�傫�ȑ������r�����̒����狿���̂�"));
		break;
	    case RIN_SUSTAIN_ABILITY:	/* KMH */
		pline_The(E_J("water flow seems fixed.",
			      "���̗��ꂪ���ɂȂ����悤���B"));
		break;
	    case RIN_GAIN_STRENGTH:
		pline_The(E_J("water flow seems %ser now.",
			      "���̏o�鐨����%s���Ȃ����悤���B"),
			(obj->spe<0) ? E_J("weak","��") : E_J("strong","��"));
		break;
	    case RIN_GAIN_CONSTITUTION:
		pline_The(E_J("water flow seems %ser now.",
			      "���̗����ʂ�%s���Ȃ����悤���B"),
			(obj->spe<0) ? E_J("less","����") : E_J("great","��"));
		break;
	    case RIN_INCREASE_ACCURACY:	/* KMH */
		pline_The(E_J("water flow %s the drain.",
			      "����%s�ė��ꂽ�B"),
			(obj->spe<0) ? E_J("misses","�r�������O��") :
				       E_J("hits","�܂������r�����߂���"));
		break;
	    case RIN_INCREASE_DAMAGE:
		pline_The(E_J("water's force seems %ser now.",
			      "������%s�Ȃ����悤���B"),
			(obj->spe<0) ? E_J("small","�ׂ�") : E_J("great","������"));
		break;
	    case RIN_HUNGER:
		ideed = FALSE;
		for(otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp2) {
		    otmp2 = otmp->nexthere;
		    if (otmp != uball && otmp != uchain &&
			    !obj_resists(otmp, 1, 99)) {
			if (!Blind) {
#ifndef JP
			    pline("Suddenly, %s %s from the sink!",
				  doname(otmp), otense(otmp, "vanish"));
#else
			    pline("�ˑR�A%s�������䂩��������I", doname(otmp));
#endif /*JP*/
			    ideed = TRUE;
			}
			delobj(otmp);
		    }
		}
		break;
	    case MEAT_RING:
		/* Not the same as aggravate monster; besides, it's obvious. */
		pline(E_J("Several flies buzz around the sink.",
			  "��������̔���������̎�����щ�����B"));
		break;
	    default:
		ideed = FALSE;
		break;
	}
	if(!Blind && !ideed && obj->otyp != RIN_HUNGER) {
	    ideed = TRUE;
	    switch(obj->otyp) {		/* effects that need eyes */
		case RIN_ADORNMENT:
		    pline_The(E_J("faucets flash brightly for a moment.",
				  "�֌�����u�܂Ԃ����P�����B"));
		    break;
		case RIN_REGENERATION:
		    pline_The(E_J("sink looks as good as new.",
				  "�����䂪�V�i���l�Ɍ������B"));
		    break;
		case RIN_INVISIBILITY:
		    You(E_J("don't see anything happen to the sink.",
			    "������ɉ��̕ω����������Ȃ������B"));
		    break;
		case RIN_FREE_ACTION:
		    You(E_J("see the ring slide right down the drain!",
			    "�w�ւ��Ԃ��邱�ƂȂ��r����������~��Ă䂭�̂������I"));
		    break;
		case RIN_SEE_INVISIBLE:
		    You(E_J("see some air in the sink.",
			    "������ɉ����̋C�z�������B"));
		    break;
		case RIN_STEALTH:
		pline_The(E_J("sink seems to blend into the floor for a moment.",
			      "������͈�u���ɗn�����񂾂悤�Ɍ������B"));
		    break;
		case RIN_FIRE_RESISTANCE:
		pline_The(E_J("hot water faucet flashes brightly for a moment.",
			      "�����̎֌�����u�܂Ԃ����������B"));
		    break;
		case RIN_COLD_RESISTANCE:
		pline_The(E_J("cold water faucet flashes brightly for a moment.",
			      "�␅�̎֌�����u�܂Ԃ����������B"));
		    break;
		case RIN_PROTECTION_FROM_SHAPE_CHAN:
		    pline_The(E_J("sink looks nothing like a fountain.",
				  "������Ɛ�̈Ⴂ�����炩�Ɍ������B"));
		    break;
		case RIN_PROTECTION:
#ifndef JP
		    pline_The("sink glows %s for a moment.",
			    hcolor((obj->spe<0) ? NH_BLACK : NH_SILVER));
#else
		    pline("������͈�u%s�P�����B",
			  j_no_ni(hcolor((obj->spe<0) ? NH_BLACK : NH_SILVER)));
#endif /*JP*/
		    break;
		case RIN_WARNING:
#ifndef JP
		    pline_The("sink glows %s for a moment.", hcolor(NH_WHITE));
#else
		    pline("������͈�u%s�P�����B", j_no_ni(hcolor(NH_WHITE)));
#endif /*JP*/
		    break;
		case RIN_TELEPORTATION:
		    pline_The(E_J("sink momentarily vanishes.",
				  "��u�����䂪�����Ȃ��Ȃ����B"));
		    break;
		case RIN_TELEPORT_CONTROL:
	    pline_The(E_J("sink looks like it is being beamed aboard somewhere.",
			  "������͂܂�łǂ����֓d������Ă��邩�̂悤�Ɍ������B"));
		    break;
		case RIN_POLYMORPH:
		    pline_The(E_J("sink momentarily looks like a fountain.",
				  "������͈�u��̂悤�Ɍ������B"));
		    break;
		case RIN_POLYMORPH_CONTROL:
	pline_The(E_J("sink momentarily looks like a regularly erupting geyser.",
		      "������͈�u�Ԍ���̂悤�Ɍ������B"));
		    break;
	    }
	}
	if(ideed)
	    trycall(obj);
	else
	    You_hear(E_J("the ring bouncing down the drainpipe.",
			 "�w�ւ��r���ǂ̒��łԂ��藎���Ă�������"));
	if (!rn2(20)) {
		pline_The(E_J("sink backs up, leaving %s.",
			      "������̐��͋t�����A%s��f���o�����B"), doname(obj));
		obj->in_use = FALSE;
		dropx(obj);
	} else
		useup(obj);
}
#endif

#endif /* OVLB */
#ifdef OVL0

/* some common tests when trying to drop or throw items */
boolean
canletgo(obj,word)
register struct obj *obj;
register const char *word;
{
	if(obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL)){
		if (*word)
#ifndef JP
			Norep("You cannot %s %s you are wearing.",word,
				something);
#else
			Norep("���p���̂��̂�%s���Ƃ��ł��Ȃ��B",word);
#endif /*JP*/
		return(FALSE);
	}
	if (obj->otyp == LOADSTONE && obj->cursed) {
		/* getobj() kludge sets corpsenm to user's specified count
		   when refusing to split a stack of cursed loadstones */
		if (*word) {
			/* getobj() ignores a count for throwing since that is
			   implicitly forced to be 1; replicate its kludge... */
#ifndef JP
			if (!strcmp(word, "throw") && obj->quan > 1L)
			    obj->corpsenm = 1;
			pline("For some reason, you cannot %s%s the stone%s!",
			      word, obj->corpsenm ? " any of" : "",
			      plur(obj->quan));
#else
			pline("�ǂ������킯���A���Ȃ��͂��̐΂�%s���Ƃ��ł��Ȃ��I", word);
#endif /*JP*/
		}
		obj->corpsenm = 0;		/* reset */
		obj->bknown = 1;
		return(FALSE);
	}
	if (obj->otyp == LEASH && obj->leashmon != 0) {
		if (*word)
			pline_The(E_J("leash is tied around your %s.",
				      "�����j�͂��Ȃ���%s�Ɍ��΂�Ă���B"),
					body_part(HAND));
		return(FALSE);
	}
#ifdef STEED
	if (obj->owornmask & W_SADDLE) {
		if (*word)
#ifndef JP
			You("cannot %s %s you are sitting on.", word,
				something);
#else
			Norep("��ɍ����Ă�����̂�%s���Ƃ͂ł��Ȃ��B",word);
#endif /*JP*/
		return (FALSE);
	}
#endif
	return(TRUE);
}

STATIC_PTR
int
drop(obj)
register struct obj *obj;
{
	if(!obj) return(0);
	if(!canletgo(obj,E_J("drop","�u��")))
		return(0);
	if(obj == uwep) {
		if(welded(uwep)) {
			weldmsg(obj);
			return(0);
		}
		setuwep((struct obj *)0);
	}
	if(obj == uquiver) {
		setuqwep((struct obj *)0);
	}
	if (obj == uswapwep) {
		setuswapwep((struct obj *)0);
	}

	if (u.uswallow) {
		/* barrier between you and the floor */
		if(flags.verbose)
		{
			char buf[BUFSZ];

			/* doname can call s_suffix, reusing its buffer */
			Strcpy(buf, s_suffix(mon_nam(u.ustuck)));
			You(E_J("drop %s into %s %s.","%s��%s%s�̒��ɒu�����B"),
				doname(obj), buf,
				mbodypart(u.ustuck, STOMACH));
		}
	} else {
#ifdef SINKS
	    if((obj->oclass == RING_CLASS || obj->otyp == MEAT_RING) &&
			IS_SINK(levl[u.ux][u.uy].typ)) {
		dosinkring(obj);
		return(1);
	    }
#endif
	    if (!can_reach_floor()) {
		if(flags.verbose) You(E_J("drop %s.","%s�𗎂Ƃ����B"), doname(obj));
#ifndef GOLDOBJ
		if (obj->oclass != COIN_CLASS || obj == invent) freeinv(obj);
#else
		/* Ensure update when we drop gold objects */
		if (obj->oclass == COIN_CLASS) flags.botl = 1;
		freeinv(obj);
#endif
		hitfloor(obj);
		return(1);
	    }
	    if (!IS_ALTAR(levl[u.ux][u.uy].typ) && flags.verbose)
		You(E_J("drop %s.","%s��u�����B"), doname(obj));
	}
	dropx(obj);
	return(1);
}

/* Called in several places - may produce output */
/* eg ship_object() and dropy() -> sellobj() both produce output */
void
dropx(obj)
register struct obj *obj;
{
#ifndef GOLDOBJ
	if (obj->oclass != COIN_CLASS || obj == invent) freeinv(obj);
#else
        /* Ensure update when we drop gold objects */
        if (obj->oclass == COIN_CLASS) flags.botl = 1;
        freeinv(obj);
#endif
	if (!u.uswallow) {
	    if (ship_object(obj, u.ux, u.uy, FALSE)) return;
	    if (IS_ALTAR(levl[u.ux][u.uy].typ))
		doaltarobj(obj); /* set bknown */
	}
	dropy(obj);
}

void
dropy(obj)
register struct obj *obj;
{
	if (obj == uwep) setuwep((struct obj *)0);
	if (obj == uquiver) setuqwep((struct obj *)0);
	if (obj == uswapwep) setuswapwep((struct obj *)0);

	if (vanish_ether_obj(obj, (struct monst *)0, E_J("drop","���Ƃ���"))) return;
	if (!u.uswallow && flooreffects(obj,u.ux,u.uy,E_J("drop","������"))) return;
	/* uswallow check done by GAN 01/29/87 */
	if(u.uswallow) {
	    boolean could_petrify = FALSE;
	    boolean could_poly = FALSE;
	    boolean could_slime = FALSE;
	    boolean could_grow = FALSE;
	    boolean could_heal = FALSE;

	    if (obj != uball) {		/* mon doesn't pick up ball */
		if (obj->otyp == CORPSE) {
		    could_petrify = touch_petrifies(&mons[obj->corpsenm]);
		    could_poly = polyfodder(obj);
		    could_slime = (obj->corpsenm == PM_GREEN_SLIME);
		    could_grow = (obj->corpsenm == PM_WRAITH);
		    could_heal = (obj->corpsenm == PM_NURSE);
		}
		(void) mpickobj(u.ustuck,obj);
		if (is_animal(u.ustuck->data)) {
		    if (could_poly || could_slime) {
			(void) newcham(u.ustuck,
				       could_poly ? (struct permonst *)0 :
				       &mons[PM_GREEN_SLIME],
				       FALSE, could_slime);
			delobj(obj);	/* corpse is digested */
		    } else if (could_petrify) {
			minstapetrify(u.ustuck, TRUE);
			/* Don't leave a cockatrice corpse in a statue */
			if (!u.uswallow) delobj(obj);
		    } else if (could_grow) {
			(void) grow_up(u.ustuck, (struct monst *)0);
			delobj(obj);	/* corpse is digested */
		    } else if (could_heal) {
			u.ustuck->mhp = u.ustuck->mhpmax;
			delobj(obj);	/* corpse is digested */
		    }
		}
	    }
	} else  {
	    place_object(obj, u.ux, u.uy);
	    if (obj == uball)
		drop_ball(u.ux,u.uy);
	    else
		sellobj(obj, u.ux, u.uy);
	    stackobj(obj);
	    if(Blind && Levitation)
		map_object(obj, 0);
	    newsym(u.ux,u.uy);	/* remap location under self */
	}
}

/* things that must change when not held; recurse into containers.
   Called for both player and monsters */
void
obj_no_longer_held(obj)
struct obj *obj;
{
	if (!obj) {
	    return;
	} else if ((Is_container(obj) || obj->otyp == STATUE) && obj->cobj) {
	    struct obj *contents;
	    for(contents=obj->cobj; contents; contents=contents->nobj)
		obj_no_longer_held(contents);
	}
	switch(obj->otyp) {
	case CRYSKNIFE:
	    /* KMH -- Fixed crysknives have only 10% chance of reverting */
	    /* only changes when not held by player or monster */
	    if (!obj->oerodeproof || !rn2(10)) {
		obj->otyp = WORM_TOOTH;
		obj->oerodeproof = 0;
	    }
	    break;
	}
}

/* 'D' command: drop several things */
int
doddrop()
{
	int result = 0;

	add_valid_menu_class(0); /* clear any classes already there */
	if (*u.ushops) sellobj_state(SELL_DELIBERATE);
	if (flags.menu_style != MENU_TRADITIONAL ||
		(result = ggetobj(E_J("drop",&drpw), drop, 0, FALSE, (unsigned *)0)) < -1)
	    result = menu_drop(result);
	if (*u.ushops) sellobj_state(SELL_NORMAL);
	reset_occupations();

	return result;
}

/* Drop things from the hero's inventory, using a menu. */
STATIC_OVL int
menu_drop(retry)
int retry;
{
    int n, i, n_dropped = 0;
    long cnt;
    struct obj *otmp, *otmp2;
#ifndef GOLDOBJ
    struct obj *u_gold = 0;
#endif
    menu_item *pick_list;
    boolean all_categories = TRUE;
    boolean drop_everything = FALSE;

#ifndef GOLDOBJ
    if (u.ugold) {
	/* Hack: gold is not in the inventory, so make a gold object
	   and put it at the head of the inventory list. */
	u_gold = mkgoldobj(u.ugold);	/* removes from u.ugold */
	u_gold->in_use = TRUE;
	u.ugold = u_gold->quan;		/* put the gold back */
	assigninvlet(u_gold);		/* might end up as NOINVSYM */
	u_gold->nobj = invent;
	invent = u_gold;
    }
#endif
    if (retry) {
	all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
	all_categories = FALSE;
	n = query_category(E_J("Drop what type of items?",
			       "�ǂ̎�ނ̕i���𗎂Ƃ��܂����H"),
			invent,
			UNPAID_TYPES | ALL_TYPES | CHOOSE_ALL |
			BUC_BLESSED | BUC_CURSED | BUC_UNCURSED | BUC_UNKNOWN,
			&pick_list, PICK_ANY);
	if (!n) goto drop_done;
	for (i = 0; i < n; i++) {
	    if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
		all_categories = TRUE;
	    else if (pick_list[i].item.a_int == 'A')
		drop_everything = TRUE;
	    else
		add_valid_menu_class(pick_list[i].item.a_int);
	}
	free((genericptr_t) pick_list);
    } else if (flags.menu_style == MENU_COMBINATION) {
	unsigned ggoresults = 0;
	all_categories = FALSE;
	/* Gather valid classes via traditional NetHack method */
	i = ggetobj(E_J("drop",&drpw), drop, 0, TRUE, &ggoresults);
	if (i == -2) all_categories = TRUE;
	if (ggoresults & ALL_FINISHED) {
		n_dropped = i;
		goto drop_done;
	}
    }

    if (drop_everything) {
	for(otmp = invent; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;
	    n_dropped += drop(otmp);
	}
    } else {
	/* should coordinate with perm invent, maybe not show worn items */
	n = query_objlist(E_J("What would you like to drop?",
			      "���𗎂Ƃ��܂����H"), invent,
			USE_INVLET|INVORDER_SORT, &pick_list,
			PICK_ANY, all_categories ? allow_all : allow_category);
	if (n > 0) {
	    for (i = 0; i < n; i++) {
		otmp = pick_list[i].item.a_obj;
		cnt = pick_list[i].count;
		if (cnt < otmp->quan) {
		    if (welded(otmp)) {
			;	/* don't split */
		    } else if (otmp->otyp == LOADSTONE && otmp->cursed) {
			/* same kludge as getobj(), for canletgo()'s use */
			otmp->corpsenm = (int) cnt;	/* don't split */
		    } else {
#ifndef GOLDOBJ
			if (otmp->oclass == COIN_CLASS)
			    (void) splitobj(otmp, otmp->quan - cnt);
			else
#endif
			    otmp = splitobj(otmp, cnt);
		    }
		}
		n_dropped += drop(otmp);
	    }
	    free((genericptr_t) pick_list);
	}
    }

 drop_done:
#ifndef GOLDOBJ
    if (u_gold && invent && invent->oclass == COIN_CLASS) {
	/* didn't drop [all of] it */
	u_gold = invent;
	invent = u_gold->nobj;
	u_gold->in_use = FALSE;
	dealloc_obj(u_gold);
	update_inventory();
    }
#endif
    return n_dropped;
}

#endif /* OVL0 */
#ifdef OVL2

/* on a ladder, used in goto_level */
static NEARDATA boolean at_ladder = FALSE;

int
dodown()
{
	struct trap *trap = 0;
	boolean stairs_down = ((u.ux == xdnstair && u.uy == ydnstair) ||
		    (u.ux == sstairs.sx && u.uy == sstairs.sy && !sstairs.up)),
		ladder_down = (u.ux == xdnladder && u.uy == ydnladder);

#ifdef STEED
	if (u.usteed && !u.usteed->mcanmove) {
		pline(E_J("%s won't move!",
			  "%s�͓������Ƃ��Ȃ��I"), Monnam(u.usteed));
		return(0);
	} else if (u.usteed && u.usteed->meating) {
		pline(E_J("%s is still eating.",
			  "%s�͂܂��H�ׂĂ���B"), Monnam(u.usteed));
		return(0);
	} else
#endif
	if (Levitation) {
	    if ((HLevitation & I_SPECIAL) || (ELevitation & W_ARTI)) {
		/* end controlled levitation */
		if (ELevitation & W_ARTI) {
		    struct obj *obj;

		    for(obj = invent; obj; obj = obj->nobj) {
			if (obj->oartifact &&
					artifact_has_invprop(obj,LEVITATION)) {
			    if (obj->age < monstermoves)
				obj->age = monstermoves + rnz(100);
			    else
				obj->age += rnz(100);
			}
		    }
		}
		if (float_down(I_SPECIAL|TIMEOUT, W_ARTI))
		    return (1);   /* came down, so moved */
	    }
	    floating_above(stairs_down ? E_J("stairs","�K�i") : ladder_down ?
			   E_J("ladder","�͂���") : surface(u.ux, u.uy));
	    return (0);   /* didn't move */
	}
	if (!stairs_down && !ladder_down) {
		if (!(trap = t_at(u.ux,u.uy)) ||
			(trap->ttyp != TRAPDOOR && trap->ttyp != HOLE &&
			 trap->ttyp != PIT && trap->ttyp != SPIKED_PIT)
			|| !Can_fall_thru(&u.uz) || !trap->tseen) {

			if (flags.autodig && !flags.nopick &&
				uwep && is_pick(uwep)) {
				return use_pick_axe2(uwep);
			} else {
#ifndef JP
				You_cant("go down here.");
#else
				pline("�����ɂ͉��֐i�ޓ����Ȃ��B");
#endif /*JP*/
				return(0);
			}
		}
	}
	if(u.ustuck) {
		You(E_J("are %s, and cannot go down.","%s��Ă��邽�߁A���֐i�ނ��Ƃ͂ł��Ȃ��B"),
			!u.uswallow ? E_J("being held","�߂炦��") : is_animal(u.ustuck->data) ?
			E_J("swallowed","���ݍ���") : E_J("engulfed","�͂�"));
		return(1);
	}
	if (on_level(&valley_level, &u.uz) && !u.uevent.gehennom_entered) {
		You(E_J("are standing at the gate to Gehennom.",
			"�Q�w�i�̖�̑O�ɗ����Ă���B"));
		pline(E_J("Unspeakable cruelty and harm lurk down there.",
			  "���t�ɂł��Ȃ��قǂ̎c�s�ƎS�������̉��ɉQ�����Ă���B"));
		if (yn(E_J("Are you sure you want to enter?",
			   "����ł��i�ނƂ����̂ł����H")) != 'y')
			return(0);
		else pline(E_J("So be it.","�Ȃ�΁A�������邪�悢�B"));
		u.uevent.gehennom_entered = 1;	/* don't ask again */
	}

	if(!next_to_u()) {
		You(E_J("are held back by your pet!",
			"�����̃y�b�g�Ɉ����߂��ꂽ�I"));
		return(0);
	}

	if (trap) {
	    if (trap->ttyp == PIT || trap->ttyp == SPIKED_PIT) {
		You(E_J("carefully slide down into the %s",
			"���Ӑ[��%s�̒��Ɋ��荞�񂾁B"),
			defsyms[trap_to_defsym(trap->ttyp)].explanation );
		u.utraptype = TT_PIT;
		u.utrap = rn1(6,2);
		vision_full_recalc = 1;	/* vision limits change */	/*[Sakusha]*/
		return(0);
	    } /*else*/
#ifndef JP
	    You("%s %s.", locomotion(youmonst.data, "jump"),
		trap->ttyp == HOLE ? "down the hole" : "through the trap door");
#else
	    You("%s��%s���񂾁B", trap->ttyp == HOLE ? "���̒�" : "���Ƃ���",
		locomotion(youmonst.data, "����"));
#endif /*JP*/
	}

	if (trap && Is_stronghold(&u.uz)) {
		goto_hell(FALSE, TRUE);
	} else {
		at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
		next_level(!trap);
		at_ladder = FALSE;
	}
	return(1);
}

int
doup()
{
	if( (u.ux != xupstair || u.uy != yupstair)
	     && (!xupladder || u.ux != xupladder || u.uy != yupladder)
	     && (!sstairs.sx || u.ux != sstairs.sx || u.uy != sstairs.sy
			|| !sstairs.up)
	  ) {
#ifndef JP
		You_cant("go up here.");
#else
		pline("�����ɂ͏�ɐi�ޓ��͂Ȃ��B");
#endif /*JP*/
		return(0);
	}
#ifdef STEED
	if (u.usteed && !u.usteed->mcanmove) {
		pline(E_J("%s won't move!","%s�͓������Ƃ��Ȃ��I"), Monnam(u.usteed));
		return(0);
	} else if (u.usteed && u.usteed->meating) {
		pline(E_J("%s is still eating.","%s�͂܂��H�ׂĂ���B"), Monnam(u.usteed));
		return(0);
	} else
#endif
	if(u.ustuck) {
		You(E_J("are %s, and cannot go up.","%s��Ă��邽�߁A��֐i�ނ��Ƃ͂ł��Ȃ��B"),
			!u.uswallow ? E_J("being held","�߂炦��") : is_animal(u.ustuck->data) ?
			E_J("swallowed","���ݍ���") : E_J("engulfed","�͂�"));
		return(1);
	}
	if(near_capacity() > SLT_ENCUMBER) {
		/* No levitation check; inv_weight() already allows for it */
		Your(E_J("load is too heavy to climb the %s.",
			 "�ו���%s�����ɂ͏d������B"),
			levl[u.ux][u.uy].typ == STAIRS ? E_J("stairs","�K�i") : E_J("ladder","�͂���"));
		return(1);
	}
	if(ledger_no(&u.uz) == 1) {
		if (yn(E_J("Beware, there will be no return! Still climb?",
			   "���ӂ���A��x�Ɩ߂�Ȃ����I ����ł���邩�H")) != 'y')
			return(0);
	}
	if(!next_to_u()) {
		You(E_J("are held back by your pet!",
			"�����̃y�b�g�Ɉ����߂��ꂽ�I"));
		return(0);
	}
	at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
	prev_level(TRUE);
	at_ladder = FALSE;
	return(1);
}

d_level save_dlevel = {0, 0};

/* check that we can write out the current level */
STATIC_OVL int
currentlevel_rewrite()
{
	register int fd;
	char whynot[BUFSZ];

	/* since level change might be a bit slow, flush any buffered screen
	 *  output (like "you fall through a trap door") */
	mark_synch();

	fd = create_levelfile(ledger_no(&u.uz), whynot);
	if (fd < 0) {
		/*
		 * This is not quite impossible: e.g., we may have
		 * exceeded our quota. If that is the case then we
		 * cannot leave this level, and cannot save either.
		 * Another possibility is that the directory was not
		 * writable.
		 */
		pline("%s", whynot);
		return -1;
	}

#ifdef MFLOPPY
	if (!savelev(fd, ledger_no(&u.uz), COUNT_SAVE)) {
		(void) close(fd);
		delete_levelfile(ledger_no(&u.uz));
		pline("NetHack is out of disk space for making levels!");
		You("can save, quit, or continue playing.");
		return -1;
	}
#endif
	return fd;
}

#ifdef INSURANCE
void
save_currentstate()
{
	int fd;

	if (flags.ins_chkpt) {
		/* write out just-attained level, with pets and everything */
		fd = currentlevel_rewrite();
		if(fd < 0) return;
		bufon(fd);
		savelev(fd,ledger_no(&u.uz), WRITE_SAVE);
		bclose(fd);
	}

	/* write out non-level state */
	savestateinlock();
}
#endif

/*
static boolean
badspot(x, y)
register xchar x, y;
{
	return((levl[x][y].typ != ROOM && levl[x][y].typ != AIR &&
			 levl[x][y].typ != CORR) || MON_AT(x, y));
}
*/

void
goto_level(newlevel, at_stairs, falling, portal)
d_level *newlevel;
boolean at_stairs, falling, portal;
{
	int fd, l_idx;
	xchar new_ledger;
	boolean cant_go_back,
		up = (depth(newlevel) < depth(&u.uz)),
		newdungeon = (u.uz.dnum != newlevel->dnum),
		was_in_W_tower = In_W_tower(u.ux, u.uy, &u.uz),
		familiar = FALSE;
	boolean new = FALSE;	/* made a new level? */
	struct monst *mtmp;
	char whynot[BUFSZ];

	if (dunlev(newlevel) > dunlevs_in_dungeon(newlevel))
		newlevel->dlevel = dunlevs_in_dungeon(newlevel);
	if (newdungeon && In_endgame(newlevel)) { /* 1st Endgame Level !!! */
		if (u.uhave.amulet)
		    assign_level(newlevel, &earth_level);
		else return;
	}
	new_ledger = ledger_no(newlevel);
	if (new_ledger <= 0)
		done(ESCAPED);	/* in fact < 0 is impossible */

	/* If you have the amulet and are trying to get out of Gehennom, going
	 * up a set of stairs sometimes does some very strange things!
	 * Biased against law and towards chaos, but not nearly as strongly
	 * as it used to be (prior to 3.2.0).
	 * Odds:	    old				    new
	 *	"up"    L      N      C		"up"    L      N      C
	 *	 +1   75.0   75.0   75.0	 +1   75.0   75.0   75.0
	 *	  0    0.0   12.5   25.0	  0    6.25   8.33  12.5
	 *	 -1    8.33   4.17   0.0	 -1    6.25   8.33  12.5
	 *	 -2    8.33   4.17   0.0	 -2    6.25   8.33   0.0
	 *	 -3    8.33   4.17   0.0	 -3    6.25   0.0    0.0
	 */
	if (Inhell && up && u.uhave.amulet && !newdungeon && !portal &&
				(dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz)-3)) {
		if (!rn2(4)) {
		    int odds = 3 + (int)u.ualign.type,		/* 2..4 */
			diff = odds <= 1 ? 0 : rn2(odds);	/* paranoia */

		    if (diff != 0) {
			assign_rnd_level(newlevel, &u.uz, diff);
			/* if inside the tower, stay inside */
			if (was_in_W_tower &&
			    !On_W_tower_level(newlevel)) diff = 0;
		    }
		    if (diff == 0)
			assign_level(newlevel, &u.uz);

		    new_ledger = ledger_no(newlevel);

		    pline(E_J("A mysterious force momentarily surrounds you...",
			      "�s�v�c�ȗ͏ꂪ��u���Ȃ����񂾁c�B"));
		    if (on_level(newlevel, &u.uz)) {
			(void) safe_teleds(FALSE);
			(void) next_to_u();
			return;
		    } else
			at_stairs = at_ladder = FALSE;
		}
	}

	/* Prevent the player from going past the first quest level unless
	 * (s)he has been given the go-ahead by the leader.
	 */
	if (on_level(&u.uz, &qstart_level) && !newdungeon && !ok_to_quest()) {
		pline(E_J("A mysterious force prevents you from descending.",
			  "�s�v�c�ȗ͂����Ȃ��̍~��铹��j�񂾁B"));
		return;
	}

	if (on_level(newlevel, &u.uz)) return;		/* this can happen */

	fd = currentlevel_rewrite();
	if (fd < 0) return;

	if (falling) /* assuming this is only trap door or hole */
	    impact_drop((struct obj *)0, u.ux, u.uy, newlevel->dlevel);

	check_special_room(TRUE);		/* probably was a trap door */
	if (Punished) unplacebc();
	u.utrap = 0;				/* needed in level_tele */
	fill_pit(u.ux, u.uy);
	if (drownbymon()) reset_drownbymon();
	u.ustuck = 0;				/* idem */
	u.uinwater = 0;
	u.uundetected = 0;	/* not hidden, even if means are available */
	keepdogs(FALSE);
	if (u.uswallow)				/* idem */
		u.uswldtim = u.uswallow = 0;
	u.ulasttgt = (struct monst *)0;		/* autothrust */
#ifdef DUNGEON_OVERVIEW
	recalc_mapseen(); /* recalculate map overview before we leave the level */
#endif /* DUNGEON_OVERVIEW */
	/*
	 *  We no longer see anything on the level.  Make sure that this
	 *  follows u.uswallow set to null since uswallow overrides all
	 *  normal vision.
	 */
	vision_recalc(2);

	/*
	 * Save the level we're leaving.  If we're entering the endgame,
	 * we can get rid of all existing levels because they cannot be
	 * reached any more.  We still need to use savelev()'s cleanup
	 * for the level being left, to recover dynamic memory in use and
	 * to avoid dangling timers and light sources.
	 */
	cant_go_back = (newdungeon && In_endgame(newlevel));
	if (!cant_go_back) {
	    update_mlstmv();	/* current monsters are becoming inactive */
	    bufon(fd);		/* use buffered output */
	}
	savelev(fd, ledger_no(&u.uz),
		cant_go_back ? FREE_SAVE : (WRITE_SAVE | FREE_SAVE));
	bclose(fd);
	if (cant_go_back) {
	    /* discard unreachable levels; keep #0 */
	    for (l_idx = maxledgerno(); l_idx > 0; --l_idx)
		delete_levelfile(l_idx);
	}

#ifdef REINCARNATION
	if (Is_rogue_level(newlevel) || Is_rogue_level(&u.uz))
		assign_rogue_graphics(Is_rogue_level(newlevel));
#endif
#ifdef USE_TILES
	substitute_tiles(newlevel);
#endif
#ifdef DUNGEON_OVERVIEW
	/* record this level transition as a potential seen branch unless using
	 * some non-standard means of transportation (level teleport).
	 */
	if ((at_stairs || falling || portal) && (u.uz.dnum != newlevel->dnum))
		recbranch_mapseen(&u.uz, newlevel);
#endif /* DUNGEON_OVERVIEW */
	assign_level(&u.uz0, &u.uz);
	assign_level(&u.uz, newlevel);
	assign_level(&u.utolev, newlevel);
	u.utotype = 0;
	if (dunlev_reached(&u.uz) < dunlev(&u.uz))
		dunlev_reached(&u.uz) = dunlev(&u.uz);
	reset_rndmonst(NON_PM);   /* u.uz change affects monster generation */

	/* set default level change destination areas */
	/* the special level code may override these */
	(void) memset((genericptr_t) &updest, 0, sizeof updest);
	(void) memset((genericptr_t) &dndest, 0, sizeof dndest);

	if (!(level_info[new_ledger].flags & LFILE_EXISTS)) {
		/* entering this level for first time; make it now */
		if (level_info[new_ledger].flags & (FORGOTTEN|VISITED)) {
		    impossible("goto_level: returning to discarded level?");
		    level_info[new_ledger].flags &= ~(FORGOTTEN|VISITED);
		}
		mklev();
		new = TRUE;	/* made the level */
	} else {
		/* returning to previously visited level; reload it */
		fd = open_levelfile(new_ledger, whynot);
		if (fd < 0) {
			pline("%s", whynot);
			pline(E_J("Probably someone removed it.",
				  "�����N�����폜���Ă��܂����̂��낤�B"));
			killer = whynot;
			done(TRICKED);
			/* we'll reach here if running in wizard mode */
			error("Cannot continue this game.");
		}
		minit();	/* ZEROCOMP */
		getlev(fd, hackpid, new_ledger, FALSE);
		(void) close(fd);
	}
	/* do this prior to level-change pline messages */
	vision_reset();		/* clear old level's line-of-sight */
	vision_full_recalc = 0;	/* don't let that reenable vision yet */
	flush_screen(-1);	/* ensure all map flushes are postponed */

	if (portal && !In_endgame(&u.uz)) {
	    /* find the portal on the new level */
	    register struct trap *ttrap;

	    for (ttrap = ftrap; ttrap; ttrap = ttrap->ntrap)
		if (ttrap->ttyp == MAGIC_PORTAL) break;

	    if (!ttrap) panic("goto_level: no corresponding portal!");
	    seetrap(ttrap);
	    u_on_newpos(ttrap->tx, ttrap->ty);
	} else if (at_stairs && !In_endgame(&u.uz)) {
	    if (up) {
		if (at_ladder) {
		    u_on_newpos(xdnladder, ydnladder);
		} else {
		    if (newdungeon) {
			if (Is_stronghold(&u.uz)) {
			    register xchar x, y;

			    do {
				x = (COLNO - 2 - rnd(5));
				y = rn1(ROWNO - 4, 3);
			    } while(occupied(x, y) ||
				    IS_WALL(levl[x][y].typ));
			    u_on_newpos(x, y);
			} else u_on_sstairs();
		    } else u_on_dnstairs();
		}
		/* Remove bug which crashes with levitation/punishment  KAA */
		if (Punished && !Levitation) {
#ifndef JP
			pline("With great effort you climb the %s.",
				at_ladder ? "ladder" : "stairs");
#else
			You("���ɋ�J����%s��������B",
				at_ladder ? "�͂���" : "�K�i");
#endif /*JP*/
		} else if (at_ladder)
		    You(E_J("climb up the ladder.","�͂�����������B"));
	    } else {	/* down */
		if (at_ladder) {
		    u_on_newpos(xupladder, yupladder);
		} else {
		    if (newdungeon) u_on_sstairs();
		    else u_on_upstairs();
		}
		if (u.dz && Flying)
		    You(E_J("fly down along the %s.","%s�ɉ����č~�������B"),
			at_ladder ? E_J("ladder","�͂���") : E_J("stairs","�K�i"));
		else if (u.dz &&
		    (near_capacity() > UNENCUMBERED || Punished || Fumbling)) {
#ifndef JP
		    You("fall down the %s.", at_ladder ? "ladder" : "stairs");
#else
		    You("%s��]���������B", at_ladder ? "�͂���" : "�K�i");
#endif /*JP*/
		    if (Punished) {
			drag_down();
			if (carried(uball)) {
			    if (uwep == uball)
				setuwep((struct obj *)0);
			    if (uswapwep == uball)
				setuswapwep((struct obj *)0);
			    if (uquiver == uball)
				setuqwep((struct obj *)0);
			    freeinv(uball);
			}
		    }
#ifdef STEED
		    /* falling off steed has its own losehp() call */
		    if (u.usteed)
			dismount_steed(DISMOUNT_FELL);
		    else
#endif
			losehp(rnd(3), E_J("falling downstairs","�K�i���痎����"), KILLED_BY);
		    selftouch(E_J("Falling, you","�������A���Ȃ���"));
		} else if (u.dz && at_ladder)
		    You(E_J("climb down the ladder.","�͂��������肽�B"));
	    }
	} else {	/* trap door or level_tele or In_endgame */
	    if (was_in_W_tower && On_W_tower_level(&u.uz))
		/* Stay inside the Wizard's tower when feasible.	*/
		/* Note: up vs down doesn't really matter in this case. */
		place_lregion(dndest.nlx, dndest.nly,
				dndest.nhx, dndest.nhy,
				0,0, 0,0, LR_DOWNTELE, (d_level *) 0);
	    else if (up)
		place_lregion(updest.lx, updest.ly,
				updest.hx, updest.hy,
				updest.nlx, updest.nly,
				updest.nhx, updest.nhy,
				LR_UPTELE, (d_level *) 0);
	    else
		place_lregion(dndest.lx, dndest.ly,
				dndest.hx, dndest.hy,
				dndest.nlx, dndest.nly,
				dndest.nhx, dndest.nhy,
				LR_DOWNTELE, (d_level *) 0);
	    if (falling) {
		if (Punished) ballfall();
		selftouch(E_J("Falling, you","�������A���Ȃ���"));
	    }
	}

	if (Punished) placebc();
	obj_delivery();		/* before killing geno'd monsters' eggs */
	losedogs();
	kill_genocided_monsters();  /* for those wiped out while in limbo */
	/*
	 * Expire all timers that have gone off while away.  Must be
	 * after migrating monsters and objects are delivered
	 * (losedogs and obj_delivery).
	 */
	run_timers();

	initrack();

	if ((mtmp = m_at(u.ux, u.uy)) != 0
#ifdef STEED
		&& mtmp != u.usteed
#endif
		) {
	    /* There's a monster at your target destination; it might be one
	       which accompanied you--see mon_arrive(dogmove.c)--or perhaps
	       it was already here.  Randomly move you to an adjacent spot
	       or else the monster to any nearby location.  Prior to 3.3.0
	       the latter was done unconditionally. */
	    coord cc;

	    if (!rn2(2) &&
		    enexto(&cc, u.ux, u.uy, youmonst.data) &&
		    distu(cc.x, cc.y) <= 2)
		u_on_newpos(cc.x, cc.y);	/*[maybe give message here?]*/
	    else
		mnexto(mtmp);

	    if ((mtmp = m_at(u.ux, u.uy)) != 0) {
		impossible("mnexto failed (do.c)?");
		(void) rloc(mtmp, FALSE);
	    }
	}

	/* initial movement of bubbles just before vision_recalc */
	if (Is_waterlevel(&u.uz))
		movebubbles();

	if (level_info[new_ledger].flags & FORGOTTEN) {
	    forget_map(ALL_MAP);	/* forget the map */
	    forget_traps();		/* forget all traps too */
	    familiar = TRUE;
	    level_info[new_ledger].flags &= ~FORGOTTEN;
	}

	/* Reset the screen. */
	vision_reset();		/* reset the blockages */
	docrt();		/* does a full vision recalc */
	flush_screen(-1);

	/*
	 *  Move all plines beyond the screen reset.
	 */

	/* give room entrance message, if any */
	check_special_room(FALSE);

	/* Check whether we just entered Gehennom. */
	if (!In_hell(&u.uz0) && Inhell) {
	    if (Is_valley(&u.uz)) {
		You(E_J("arrive at the Valley of the Dead...",
			"���̒J�ɓ��������c�B"));
		pline_The(E_J("odor of burnt flesh and decay pervades the air.",
			      "������ɂ͓��̏Ă���L���ƕ��L���[�����Ă���B"));
#ifdef MICRO
		display_nhwindow(WIN_MESSAGE, FALSE);
#endif
#ifndef JP
		You_hear("groans and moans everywhere.");
#else
		pline("�����璆����ߒɂȂ��߂��Ƌꂵ���Ȃ��������������Ă���B");
#endif /*JP*/
#ifdef RECORD_ACHIEVE
        achieve.enter_gehennom = 1;
#endif
	    } else pline(E_J("It is hot here.  You smell smoke...",
			     "�����͔M���B���̏L��������c�B"));
	}

	if (familiar) {
	    static const char * const fam_msgs[4] = {
		E_J("You have a sense of deja vu.","���Ȃ��͊������ɂ�����ꂽ�B"),
		E_J("You feel like you've been here before.","�����ɂ͑O�ɂ������悤�ȋC������B"),
		E_J("This place %s familiar...","���̏ꏊ�͒m���Ă���悤�ȁc�B"),
		0	/* no message */
	    };
	    static const char * const halu_fam_msgs[4] = {
		E_J("Whoa!  Everything %s different.","���Ђ�I �����������Ⴄ�݂������B"),
		E_J("You are surrounded by twisty little passages, all alike.",
		    "���Ȃ��͓���g�񂾁A���ׂē����Ɍ����鏬���Ɉ͂܂�Ă���B"),
		E_J("Gee, this %s like uncle Conan's place...",
		    "���A�����̓R�i����������̓y�n�݂��������c�B"),
		0	/* no message */
	    };
	    const char *mesg;
	    char buf[BUFSZ];
	    int which = rn2(4);

	    if (Hallucination)
		mesg = halu_fam_msgs[which];
	    else
		mesg = fam_msgs[which];
#ifndef JP
	    if (mesg && index(mesg, '%')) {
		Sprintf(buf, mesg, !Blind ? "looks" : "seems");
		mesg = buf;
	    }
#endif /*JP*/
	    if (mesg) pline(mesg);
	}

#ifdef REINCARNATION
	if (new && Is_rogue_level(&u.uz))
	    You(E_J("enter what seems to be an older, more primitive world.",
		    "�͂邩�ɌÂ��A�t���̐��E�ɓ��ݓ������悤���B"));
#endif
	/* Final confrontation */
	if (In_endgame(&u.uz) && newdungeon && u.uhave.amulet)
		resurrect();
	if (newdungeon && In_V_tower(&u.uz) && In_hell(&u.uz0))
		pline_The(E_J("heat and smoke are gone.",
			      "�M�Ɖ����������B"));

	/* the message from your quest leader */
	if (!In_quest(&u.uz0) && at_dgn_entrance("The Quest") &&
		!(u.uevent.qexpelled || u.uevent.qcompleted || quest_status.leader_is_dead)) {

		if (u.uevent.qcalled) {
			com_pager(Role_if(PM_ROGUE) ? 4 : 3);
		} else {
			com_pager(2);
			u.uevent.qcalled = TRUE;
		}
	}

	/* once Croesus is dead, his alarm doesn't work any more */
	if (Is_knox(&u.uz) && (new || !mvitals[PM_CROESUS].died)) {
		You(E_J("penetrated a high security area!",
			"���d�x�����ɐN�������I"));
		pline(E_J("An alarm sounds!","�x�񂪖苿�����I"));
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		    if (!DEADMONSTER(mtmp) && mtmp->msleeping) mtmp->msleeping = 0;
	}

	if (on_level(&u.uz, &astral_level))
	    final_level();
	else
	    onquest();
	assign_level(&u.uz0, &u.uz); /* reset u.uz0 */

#ifdef INSURANCE
	save_currentstate();
#endif

	/* assume this will always return TRUE when changing level */
	(void) in_out_region(u.ux, u.uy);
	(void) pickup(1);
}

STATIC_OVL void
final_level()
{
	struct monst *mtmp;
	struct obj *otmp;
	coord mm;
	int i;

	/* reset monster hostility relative to player */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp)) reset_hostility(mtmp);

	/* create some player-monsters */
	create_mplayers(rn1(4, 3), TRUE);

	/* create a guardian angel next to player, if worthy */
	if (Conflict) {
	    pline(
	     E_J("A voice booms: \"Thy desire for conflict shall be fulfilled!\"",
		 "����������:�g���̓����ւ̊��]�͖��������I�h"));
	    for (i = rnd(4); i > 0; --i) {
		mm.x = u.ux;
		mm.y = u.uy;
		if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL]))
		    (void) mk_roamer(&mons[PM_ANGEL], u.ualign.type,
				     mm.x, mm.y, FALSE);
	    }

	} else if (u.ualign.record > 8) {	/* fervent */
	    pline(E_J("A voice whispers: \"Thou hast been worthy of me!\"",
		      "����������:�g���͉�ɂ��̉��l���������I�h"));
	    mm.x = u.ux;
	    mm.y = u.uy;
	    if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL])) {
		if ((mtmp = mk_roamer(&mons[PM_ANGEL], u.ualign.type,
				      mm.x, mm.y, TRUE)) != 0) {
		    if (!Blind)
			pline(E_J("An angel appears near you.",
				  "���Ȃ��̖T�ɓV�g�����ꂽ�B"));
		    else
			You_feel(E_J("the presence of a friendly angel near you.",
				     "�����̓V�g���߂��Ɍ��ꂽ�̂��������B"));
		    /* guardian angel -- the one case mtame doesn't
		     * imply an edog structure, so we don't want to
		     * call tamedog().
		     */
		    mtmp->mtame = 10;
		    /* make him strong enough vs. endgame foes */
		    mtmp->m_lev = rn1(8,15);
		    mtmp->mhp = mtmp->mhpmax =
					d((int)mtmp->m_lev,10) + 30 + rnd(30);
		    if ((otmp = select_hwep(mtmp)) == 0) {
			otmp = mksobj(SABER, FALSE, FALSE);
			change_material(otmp, SILVER);
			if (mpickobj(mtmp, otmp))
			    panic("merged weapon?");
		    }
		    bless(otmp);
		    if (otmp->spe < 4) otmp->spe += rnd(4);
		    if ((otmp = which_armor(mtmp, W_ARMS)) == 0 ||
			    otmp->otyp != SHIELD_OF_REFLECTION) {
			(void) mongets(mtmp, AMULET_OF_REFLECTION);
			m_dowear(mtmp, TRUE);
		    }
		}
	    }
	}
}

static char *dfr_pre_msg = 0,	/* pline() before level change */
	    *dfr_post_msg = 0;	/* pline() after level change */

/* change levels at the end of this turn, after monsters finish moving */
void
schedule_goto(tolev, at_stairs, falling, portal_flag, pre_msg, post_msg)
d_level *tolev;
boolean at_stairs, falling;
int portal_flag;
const char *pre_msg, *post_msg;
{
	int typmask = 0100;		/* non-zero triggers `deferred_goto' */

	/* destination flags (`goto_level' args) */
	if (at_stairs)	 typmask |= 1;
	if (falling)	 typmask |= 2;
	if (portal_flag) typmask |= 4;
	if (portal_flag < 0) typmask |= 0200;	/* flag for portal removal */
	u.utotype = typmask;
	/* destination level */
	assign_level(&u.utolev, tolev);

	if (pre_msg)
	    dfr_pre_msg = strcpy((char *)alloc(strlen(pre_msg) + 1), pre_msg);
	if (post_msg)
	    dfr_post_msg = strcpy((char *)alloc(strlen(post_msg)+1), post_msg);
}

/* handle something like portal ejection */
void
deferred_goto()
{
	if (!on_level(&u.uz, &u.utolev)) {
	    d_level dest;
	    int typmask = u.utotype; /* save it; goto_level zeroes u.utotype */

	    assign_level(&dest, &u.utolev);
	    if (dfr_pre_msg) pline(dfr_pre_msg);
	    goto_level(&dest, !!(typmask&1), !!(typmask&2), !!(typmask&4));
	    if (typmask & 0200) {	/* remove portal */
		struct trap *t = t_at(u.ux, u.uy);

		if (t) {
		    deltrap(t);
		    newsym(u.ux, u.uy);
		}
	    }
	    if (dfr_post_msg) pline(dfr_post_msg);
	}
	u.utotype = 0;		/* our caller keys off of this */
	if (dfr_pre_msg)
	    free((genericptr_t)dfr_pre_msg),  dfr_pre_msg = 0;
	if (dfr_post_msg)
	    free((genericptr_t)dfr_post_msg),  dfr_post_msg = 0;
}

#endif /* OVL2 */
#ifdef OVL3

/*
 * Return TRUE if we created a monster for the corpse.  If successful, the
 * corpse is gone.
 */
boolean
revive_corpse(corpse)
struct obj *corpse;
{
    struct monst *mtmp, *mcarry;
    boolean is_uwep, chewed;
    xchar where;
    char *cname, cname_buf[BUFSZ];
    struct obj *container = (struct obj *)0;
    int container_where = 0;
    
    where = corpse->where;
    is_uwep = corpse == uwep;
    cname = eos(strcpy(cname_buf, E_J("bite-covered ","�H�ׂ�����")));
    Strcpy(cname, corpse_xname(corpse, TRUE));
    mcarry = (where == OBJ_MINVENT) ? corpse->ocarry : 0;

    if (where == OBJ_CONTAINED) {
    	struct monst *mtmp2 = (struct monst *)0;
	container = corpse->ocontainer;
    	mtmp2 = get_container_location(container, &container_where, (int *)0);
	/* container_where is the outermost container's location even if nested */
	if (container_where == OBJ_MINVENT && mtmp2) mcarry = mtmp2;
    }
    mtmp = revive(corpse);	/* corpse is gone if successful */

    if (mtmp) {
	chewed = (mtmp->mhp < mtmp->mhpmax);
	if (chewed) cname = cname_buf;	/* include "bite-covered" prefix */
	switch (where) {
	    case OBJ_INVENT:
		if (is_uwep)
		    pline_The(E_J("%s writhes out of your grasp!",
				  "%s�����Ȃ��̎肩��������o���I"), cname);
		else
		    You_feel(E_J("squirming in your backpack!",
				 "�w�����܂ŉ��������������Ɠ����̂��������I"));
		break;

	    case OBJ_FLOOR:
		if (cansee(mtmp->mx, mtmp->my))
		    pline(E_J("%s rises from the dead!","%s����݂��������I"), chewed ?
			  Adjmonnam(mtmp, E_J("bite-covered","�H�ׂ�����")) : Monnam(mtmp));
		break;

	    case OBJ_MINVENT:		/* probably a nymph's */
		if (cansee(mtmp->mx, mtmp->my)) {
		    if (canseemon(mcarry))
#ifndef JP
			pline("Startled, %s drops %s as it revives!",
			      mon_nam(mcarry), an(cname));
#else
			pline("%s����݂�����A%s�͋����ė��Ƃ��Ă��܂����I",
			      cname, mon_nam(mcarry));
#endif /*JP*/
		    else
			pline(E_J("%s suddenly appears!","%s���ˑR���ꂽ�I"), chewed ?
			      Adjmonnam(mtmp, E_J("bite-covered","�H�ׂ�����")) : Monnam(mtmp));
		}
		break;
	   case OBJ_CONTAINED:
	   	if (container_where == OBJ_MINVENT && cansee(mtmp->mx, mtmp->my) &&
		    mcarry && canseemon(mcarry) && container) {
		        char sackname[BUFSZ];
		        Sprintf(sackname, E_J("%s %s","%s%s"), s_suffix(mon_nam(mcarry)),
				xname(container)); 
	   		pline(E_J("%s writhes out of %s!",
				  "%s��%s���甇���o�Ă����I"), Amonnam(mtmp), sackname);
	   	} else if (container_where == OBJ_INVENT && container) {
#ifndef JP
		        char sackname[BUFSZ];
		        Strcpy(sackname, an(xname(container)));
	   		pline("%s %s out of %s in your pack!",
	   			Blind ? Something : Amonnam(mtmp),
				locomotion(mtmp->data,"writhes"),
	   			sackname);
#else
	   		pline("%s�����Ȃ��̎����Ă���%s�̒�����%s�o�Ă����I",
	   			Blind ? Something : mon_nam(mtmp),
				xname(container),
				locomotion(mtmp->data,"����"));
#endif /*JP*/
	   	} else if (container_where == OBJ_FLOOR && container &&
		            cansee(mtmp->mx, mtmp->my)) {
#ifndef JP
		        char sackname[BUFSZ];
		        Strcpy(sackname, an(xname(container)));
			pline("%s escapes from %s!", Amonnam(mtmp), sackname);
#else
			pline("%s��%s�̒����甲���o�����I", mon_nam(mtmp), xname(container));
#endif /*JP*/
		}
		break;
	    default:
		/* we should be able to handle the other cases... */
		impossible("revive_corpse: lost corpse @ %d", where);
		break;
	}
	return TRUE;
    }
    return FALSE;
}

/* Revive the corpse via a timeout. */
/*ARGSUSED*/
void
revive_mon(arg, timeout)
genericptr_t arg;
long timeout;
{
    struct obj *body = (struct obj *) arg;

    /* if we succeed, the corpse is gone, otherwise, rot it away */
    if (!revive_corpse(body)) {
	if (is_rider(&mons[body->corpsenm]))
	    You_feel(E_J("less hassled.","�ς킵�����班��������ꂽ�C�������B"));
	(void) start_timer(250L - (monstermoves-body->age),
					TIMER_OBJECT, ROT_CORPSE, arg);
    }
}

int
donull()
{
	return(1);	/* Do nothing, but let other things happen */
}

int
dorest()
{
	static long lastrest = 0L;
	if ((moves - lastrest) && u.urest < 100) u.urest += 2;
	return(1);	/* Do nothing, but let other things happen */
}

#endif /* OVL3 */
#ifdef OVLB

STATIC_PTR int
wipeoff()
{
	if(u.ucreamed < 4)	u.ucreamed = 0;
	else			u.ucreamed -= 4;
	if (Blinded < 4)	Blinded = 0;
	else			Blinded -= 4;
	if (!Blinded) {
		pline(E_J("You've got the glop off.",
			  "�x�g�x�g��@�����B"));
		u.ucreamed = 0;
		Blinded = 1;
		make_blinded(0L,TRUE);
		return(0);
	} else if (!u.ucreamed) {
		Your(E_J("%s feels clean now.",
			 "%s�͂��ꂢ�ɂȂ����B"), body_part(FACE));
		return(0);
	}
	return(1);		/* still busy */
}

int
dowipe()
{
	if(u.ucreamed)  {
		static NEARDATA char buf[39];

		Sprintf(buf, E_J("wiping off your %s","%s��@����"), body_part(FACE));
		set_occupation(wipeoff, buf, 0);
		/* Not totally correct; what if they change back after now
		 * but before they're finished wiping?
		 */
		return(1);
	}
	Your(E_J("%s is already clean.","%s�͉���Ă��Ȃ��B"), body_part(FACE));
	return(1);
}

void
set_wounded_legs(side, timex)
register long side;
register int timex;
{
	/* KMH -- STEED
	 * If you are riding, your steed gets the wounded legs instead.
	 * You still call this function, but don't lose hp.
	 * Caller is also responsible for adjusting messages.
	 */

	if(!Wounded_legs) {
		ATEMP(A_DEX)--;
		flags.botl = 1;
	}

	if(!Wounded_legs || (HWounded_legs & TIMEOUT))
		HWounded_legs = timex;
	EWounded_legs = side;
	(void)encumber_msg();
}

void
heal_legs()
{
	if(Wounded_legs) {
		if (ATEMP(A_DEX) < 0) {
			ATEMP(A_DEX)++;
			flags.botl = 1;
		}

#ifdef STEED
		if (!u.usteed)
#endif
		{
		    /* KMH, intrinsics patch */
#ifndef JP
		    if((EWounded_legs & BOTH_SIDES) == BOTH_SIDES) {
			Your("%s feel somewhat better.",
				makeplural(body_part(LEG)));
		    } else {
			Your("%s feels somewhat better.",
				body_part(LEG));
		    }
#else
		    Your("%s%s�̉���͂����炩�ǂ��Ȃ����B",
			 ((EWounded_legs & BOTH_SIDES) == BOTH_SIDES) ? "��" : "",
			 body_part(LEG));
#endif /*JP*/
		}
		HWounded_legs = EWounded_legs = 0;
	}
	(void)encumber_msg();
}

#endif /* OVLB */

/*do.c*/
