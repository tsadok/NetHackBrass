/*	SCCS Id: @(#)sit.c	3.4	2002/09/21	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

void
take_gold()
{
#ifndef GOLDOBJ
	if (u.ugold <= 0)  {
		You_feel(E_J("a strange sensation.",
			     "奇妙な感覚におそわれた。"));
	} else {
		You(E_J("notice you have no gold!",
			"金を失ったことに気づいた！"));
		u.ugold = 0;
		flags.botl = 1;
	}
#else
        struct obj *otmp, *nobj;
	int lost_money = 0;
	for (otmp = invent; otmp; otmp = nobj) {
		nobj = otmp->nobj;
		if (otmp->oclass == COIN_CLASS) {
			lost_money = 1;
			delobj(otmp);
		}
	}
	if (!lost_money)  {
		You_feel(E_J("a strange sensation.",
			     "奇妙な感覚におそわれた。"));
	} else {
		You(E_J("notice you have no money!",
			"金を失ったことに気づいた！"));
		flags.botl = 1;
	}
#endif
}

int
dosit()
{
	static const char sit_message[] = E_J("sit on the %s.","%sに座った。");
	register struct trap *trap;
	register int typ = levl[u.ux][u.uy].typ;


#ifdef STEED
	if (u.usteed) {
	    You(E_J("are already sitting on %s.",
		    "すでに%sの上に座っている。"), mon_nam(u.usteed));
	    return (0);
	}
#endif

	if(!can_reach_floor())	{
	    if (Levitation)
		You(E_J("tumble in place.","その場で回転した。"));
	    else
		You(E_J("are sitting on air.","空中に座っている。"));
	    return 0;
	} else if (is_pool(u.ux, u.uy) && !Underwater) {  /* water walking */
	    goto in_water;
	}

	if(OBJ_AT(u.ux, u.uy)) {
	    register struct obj *obj;

	    obj = level.objects[u.ux][u.uy];
#ifndef JP
	    You("sit on %s.", the(xname(obj)));
#else
	    You(sit_message, xname(obj));
#endif /*JP*/
	    if (!(Is_box(obj) || get_material(obj) == CLOTH))
		pline(E_J("It's not very comfortable...",
			  "とても座り心地が悪い…。"));

	} else if ((trap = t_at(u.ux, u.uy)) != 0 ||
		   (u.utrap && (u.utraptype >= TT_LAVA))) {

	    if (u.utrap) {
		exercise(A_WIS, FALSE);	/* you're getting stuck longer */
		if(u.utraptype == TT_BEARTRAP) {
		    You_cant(E_J("sit down with your %s in the bear trap.",
				 "%sがトラバサミに挟まれているので、座れない。"), body_part(FOOT));
		    u.utrap++;
	        } else if(u.utraptype == TT_PIT) {
		    if(trap->ttyp == SPIKED_PIT) {
			You(E_J("sit down on a spike.  Ouch!",
				"棘の上に腰掛けた。いてっ！"));
			losehp(1, E_J("sitting on an iron spike",
				      "鉄の棘の上に腰掛けて"), KILLED_BY);
			exercise(A_STR, FALSE);
		    } else
			You(E_J("sit down in the pit.","落とし穴の中で座った。"));
		    u.utrap += rn2(5);
		} else if(u.utraptype == TT_WEB) {
		    You(E_J("sit in the spider web and get entangled further!",
			    "蜘蛛の巣の上で座ろうとして、さらに絡まってしまった！"));
		    u.utrap += rn1(10, 5);
		} else if(u.utraptype == TT_LAVA) {
		    /* Must have fire resistance or they'd be dead already */
		    You(E_J("sit in the lava!","溶岩の上に座った！"));
		    u.utrap += rnd(4);
		    losehp(d(2,10), E_J("sitting in lava","溶岩の上に座って"), KILLED_BY);
		} else if(u.utraptype == TT_INFLOOR) {
		    You_cant(E_J("maneuver to sit!","座る動作を行えない！"));
		    u.utrap++;
		}
	    } else {
	        You(E_J("sit down.","座った。"));
		dotrap(trap, 0);
	    }
	} else if(Underwater || Is_waterlevel(&u.uz)) {
	    if (Is_waterlevel(&u.uz))
		There(E_J("are no cushions floating nearby.","クッションは浮いていない。"));
	    else
		You(E_J("sit down on the muddy bottom.","泥だらけの水底に座った。"));
	} else if(is_pool(u.ux, u.uy)) {
 in_water:
	    You(E_J("sit in the water.","水の上に座った。"));
	    if (!rn2(10) && uarm)
		(void) rust_dmg(uarm, armor_simple_name(uarm), 1, TRUE, &youmonst);
	    if (!rn2(10) && uarmf && uarmf->otyp != WATER_WALKING_BOOTS)
		(void) rust_dmg(uarm, armor_simple_name(uarm), 1, TRUE, &youmonst);
#ifdef SINKS
	} else if(IS_SINK(typ)) {

	    You(sit_message, defsyms[S_sink].explanation);
#ifndef JP
	    Your("%s gets wet.", humanoid(youmonst.data) ? "rump" : "underside");
#else
	    Your("%sは濡れた。", humanoid(youmonst.data) ? "尻" : "下側");
#endif /*JP*/
#endif
	} else if(IS_ALTAR(typ)) {

	    You(sit_message, defsyms[S_altar].explanation);
	    altar_wrath(u.ux, u.uy);

	} else if(IS_GRAVE(typ)) {

	    You(sit_message, defsyms[S_grave].explanation);

	} else if(typ == STAIRS) {

	    You(sit_message, E_J("stairs","階段"));

	} else if(typ == LADDER) {

	    You(sit_message, E_J("ladder","はしご"));

	} else if (is_lava(u.ux, u.uy)) {

	    /* must be WWalking */
	    You(sit_message, E_J("lava","溶岩の上"));
	    burn_away_slime();
	    if (likes_lava(youmonst.data)) {
		pline_The(E_J("lava feels warm.","溶岩は暖かい。"));
		return 1;
	    }
	    pline_The(E_J("lava burns you!","溶岩があなたを焼いた！"));
	    losehp(d((Fire_resistance ? 2 : 10), 10),
		   E_J("sitting on lava","溶岩の上に座って"), KILLED_BY);

	} else if (is_ice(u.ux, u.uy)) {

	    You(sit_message, defsyms[S_ice].explanation);
	    if (!Cold_resistance) pline_The(E_J("ice feels cold.","氷は冷たい。"));

	} else if (typ == DRAWBRIDGE_DOWN) {

	    You(sit_message, E_J("drawbridge","跳ね橋の上"));

	} else if(IS_THRONE(typ)) {

	    You(sit_message, defsyms[S_throne].explanation);
	    if (rnd(6) > 4)  {
		switch (rnd(13))  {
		    case 1:
			(void) adjattrib(rn2(A_MAX), -rn1(4,3), FALSE);
			losehp(rnd(10), E_J("cursed throne","玉座の呪いで"), KILLED_BY_AN);
			break;
		    case 2:
			(void) adjattrib(rn2(A_MAX), 1, FALSE);
			break;
		    case 3:
#ifndef JP
			pline("A%s electric shock shoots through your body%s",
			      (Shock_resistance) ? "n" : " massive",
			      (is_full_resist(SHOCK_RES) ? "." : "!"));
#else
			pline("%s電撃があなたの身体を貫いた%s",
			      (Shock_resistance) ? "" : "強烈な",
			      (is_full_resist(SHOCK_RES) ? "。" : "！"));
#endif /*JP*/
			if (is_full_resist(SHOCK_RES)) break;
			losehp(Shock_resistance ? rnd(6) : rnd(30),
			       E_J("electric chair","電気椅子で"), E_J(KILLED_BY_AN,KILLED_SUFFIX));
			exercise(A_CON, FALSE);
			break;
		    case 4:
			You_feel(E_J("much, much better!","とても、とても元気になった！"));
			if (Upolyd) {
			    if (u.mh >= (u.mhmax - 5))  u.mhmax += 4;
			    u.mh = u.mhmax;
			}
			if(u.uhp >= (u.uhpmax - 5))  addhpmax(4);
			u.uhp = u.uhpmax;
			make_blinded(0L,TRUE);
			make_sick(0L, (char *) 0, FALSE, SICK_ALL);
			heal_legs();
			flags.botl = 1;
			break;
		    case 5:
			take_gold();
			break;
		    case 6:
			if(u.uluck + rn2(5) < 0) {
			    You_feel(E_J("your luck is changing.","自分の運勢が変わりつつあることに気づいた。"));
			    change_luck(1);
			} else	    makewish();
			break;
		    case 7:
			{
			register int cnt = rnd(10);

			pline(E_J("A voice echoes:","声が響いた:"));
#ifndef JP
			verbalize("Thy audience hath been summoned, %s!",
				  flags.female ? "Dame" : "Sire");
#else
			verbalize("拝謁者が集まってございます、殿下！");
#endif /*JP*/
			while(cnt--)
			    (void) makemon(courtmon(), u.ux, u.uy, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
			break;
			}
		    case 8:
			pline(E_J("A voice echoes:","声が響いた:"));
#ifndef JP
			verbalize("By thy Imperious order, %s...",
				  flags.female ? "Dame" : "Sire");
#else
			verbalize("粛清のご命令を、殿下…。");
#endif /*JP*/
			do_genocide(5);	/* REALLY|ONTHRONE, see do_genocide() */
			break;
		    case 9:
			pline(E_J("A voice echoes:","声が響いた:"));
	verbalize(E_J("A curse upon thee for sitting upon this most holy throne!",
		      "至聖なる玉座に座したる者に呪いあれ！"));
			if (Luck > 0)  {
			    make_blinded(Blinded + rn1(100,250),TRUE);
			} else	    rndcurse();
			break;
		    case 10:
			if (Luck < 0 || (HSee_invisible & INTRINSIC))  {
				if (level.flags.nommap) {
					pline(
					E_J("A terrible drone fills your head!",
					    "ひどい耳鳴りがあなたの頭を埋めつくした！"));
					make_confused(HConfusion + rnd(30),
									FALSE);
				} else {
					pline(E_J("An image forms in your mind.",
						  "あなたの心に景色が浮かび上がった。"));
					do_mapping();
				}
			} else  {
				Your(E_J("vision becomes clear.","視界が明瞭になった。"));
				HSee_invisible |= FROMOUTSIDE;
				newsym(u.ux, u.uy);
			}
			break;
		    case 11:
			if (Luck < 0)  {
			    You_feel(E_J("threatened.","脅されているような気分になった。"));
			    aggravate();
			} else  {

			    You_feel(E_J("a wrenching sensation.","ねじられるような感覚をおぼえた。"));
			    tele();		/* teleport him */
			}
			break;
		    case 12:
			You(E_J("are granted an insight!","見識の力を与えられた！"));
			if (invent) {
			    /* rn2(5) agrees w/seffects() */
			    identify_pack(rn2(5));
			}
			break;
		    case 13:
			Your(E_J("mind turns into a pretzel!",
				 "心はスパゲティのようにこんがらがった！"));
			make_confused(HConfusion + rn1(7,16),FALSE);
			break;
		    default:	impossible("throne effect");
				break;
		}
	    } else {
		if (is_prince(youmonst.data))
		    You_feel(E_J("very comfortable here.",
				 "とても満足した気分になった。"));
		else
		    You_feel(E_J("somehow out of place...",
				 "なんだか場違いな気分になった…。"));
	    }

	    if (!rn2(3) && IS_THRONE(levl[u.ux][u.uy].typ)) {
		/* may have teleported */
		levl[u.ux][u.uy].typ = ROOM;
		pline_The(E_J("throne vanishes in a puff of logic.",
			      "玉座は論理の霞となって消え失せた。"));
		newsym(u.ux,u.uy);
	    }

	} else if (lays_eggs(youmonst.data)) {
		struct obj *uegg;

		if (!flags.female) {
			pline(E_J("Males can't lay eggs!",
				  "雄は卵を生めない！"));
			return 0;
		}

		if (u.uhunger < (int)objects[EGG].oc_nutrition) {
			You(E_J("don't have enough energy to lay an egg.",
				"卵を産むのに必要なだけのエネルギーを持っていない。"));
			return 0;
		}

		uegg = mksobj(EGG, FALSE, FALSE);
		uegg->spe = 1;
		uegg->quan = 1;
		uegg->owt = weight(uegg);
		uegg->corpsenm = egg_type_from_parent(u.umonnum, FALSE);
		uegg->known = uegg->dknown = 1;
		attach_egg_hatch_timeout(uegg);
		You(E_J("lay an egg.","卵を産んだ。"));
		dropy(uegg);
		stackobj(uegg);
		morehungry((int)objects[EGG].oc_nutrition);
	} else if (u.uswallow)
		There(E_J("are no seats in here!","座れない！"));
	else
		pline(E_J("Having fun sitting on the %s?",
			  "%sに座って楽しいかい？"), surface(u.ux,u.uy));
	return(1);
}

void
rndcurse()			/* curse a few inventory items at random! */
{
	int	nobj = 0;
	int	cnt, onum, tmp;
	struct	obj	*otmp;
	static const char mal_aura[] = E_J("feel a malignant aura surround %s.",
					   "%sを暗黒のオーラが包むのを感じた。");

	if (uwep && (uwep->oartifact == ART_MAGICBANE) && rn2(20)) {
	    You(mal_aura, E_J("the magic-absorbing blade","魔法を吸収する刃"));
	    return;
	}

	if(Antimagic) {
	    shieldeff(u.ux, u.uy);
	    You(mal_aura, E_J("you","自分"));
	    damage_resistant_obj(ANTIMAGIC, 1);
	}

	for (otmp = invent; otmp; otmp = otmp->nobj) {
#ifdef GOLDOBJ
	    /* gold isn't subject to being cursed or blessed */
	    if (otmp->oclass == COIN_CLASS) continue;
#endif
	    nobj++;
	}
	if (nobj) {
	    for (cnt = rnd(6/((!!Antimagic) + (!!Half_spell_damage) + 1));
		 cnt > 0; cnt--)  {
		onum = rnd(nobj);
		for (otmp = invent; otmp; otmp = otmp->nobj) {
#ifdef GOLDOBJ
		    /* as above */
		    if (otmp->oclass == COIN_CLASS) continue;
#endif
		    if (--onum == 0) break;	/* found the target */
		}
		/* the !otmp case should never happen; picking an already
		   cursed item happens--avoid "resists" message in that case */
		if (!otmp || otmp->cursed) continue;	/* next target */

		if(otmp->oartifact && spec_ability(otmp, SPFX_INTEL) &&
		   rn2(10) < 8) {
#ifndef JP
		    pline("%s!", Tobjnam(otmp, "resist"));
#else
		    pline("%sは呪いをはね返した！", xname(otmp));
#endif /*JP*/
		    continue;
		}

		if(otmp->blessed)
			unbless(otmp);
		else
			curse(otmp);
	    }
	    update_inventory();
	}

#ifdef STEED
	/* treat steed's saddle as extended part of hero's inventory */
	if (u.usteed && !rn2(4) &&
		(otmp = which_armor(u.usteed, W_SADDLE)) != 0 &&
		!otmp->cursed) {	/* skip if already cursed */
	    if (otmp->blessed)
		unbless(otmp);
	    else
		curse(otmp);
	    if (!Blind) {
#ifndef JP
		pline("%s %s %s.",
		      s_suffix(upstart(y_monnam(u.usteed))),
		      aobjnam(otmp, "glow"),
		      hcolor(otmp->cursed ? NH_BLACK : (const char *)"brown"));
#else
		pline("%sの%sは%s輝いた。",
		      y_monnam(u.usteed), xname(otmp),
		      j_no_ni(hcolor(otmp->cursed ? NH_BLACK : (const char *)"茶色の")));
#endif /*JP*/
		otmp->bknown = TRUE;
	    }
	}
#endif	/*STEED*/
}

void
attrcurse()			/* remove a random INTRINSIC ability */
{
	switch(rnd(11)) {
	case 1 : if (HFire_resistance & INTRINSIC) {
			HFire_resistance &= ~INTRINSIC;
			You_feel(E_J("warmer.","暑くなった。"));
			break;
		}
	case 2 : if (HTeleportation & INTRINSIC) {
			HTeleportation &= ~INTRINSIC;
			You_feel(E_J("less jumpy.","落ち着いた。"));
			break;
		}
	case 3 : if (HPoison_resistance & INTRINSIC) {
			HPoison_resistance &= ~INTRINSIC;
			You_feel(E_J("a little sick!","少し気分が悪くなった！"));
			break;
		}
	case 4 : if (HTelepat & INTRINSIC) {
			HTelepat &= ~INTRINSIC;
			if (Blind && !Blind_telepat)
			    see_monsters();	/* Can't sense mons anymore! */
			Your(E_J("senses fail!","感覚は鈍った！"));
			break;
		}
	case 5 : if (HCold_resistance & INTRINSIC) {
			HCold_resistance &= ~INTRINSIC;
			You_feel(E_J("cooler.","寒くなった。"));
			break;
		}
	case 6 : if (HInvis & INTRINSIC) {
			HInvis &= ~INTRINSIC;
			You_feel(E_J("paranoid.","被害妄想に陥った。"));
			break;
		}
	case 7 : if (HSee_invisible & INTRINSIC) {
			HSee_invisible &= ~INTRINSIC;
#ifndef JP
			You("%s!", Hallucination ? "tawt you taw a puttie tat"
						: "thought you saw something");
#else
			You("%s！", Hallucination ? "見た、見た、ネコたん"
						: "何かを見たような気がした");
#endif /*JP*/
			break;
		}
	case 8 : if (HFast & INTRINSIC) {
			HFast &= ~INTRINSIC;
#ifndef JP
			You_feel("slower.");
#else
			Your("動きは遅くなった。");
#endif /*JP*/
			break;
		}
	case 9 : if (HStealth & INTRINSIC) {
			HStealth &= ~INTRINSIC;
			E_J(You_feel("clumsy."), Your("動きはがさつになった。"));
			break;
		}
	case 10: if (HProtection & INTRINSIC) {
			HProtection &= ~INTRINSIC;
			You_feel(E_J("vulnerable.","傷つきやすくなった。"));
			break;
		}
	case 11: if (HAggravate_monster & INTRINSIC) {
			HAggravate_monster &= ~INTRINSIC;
			You_feel(E_J("less attractive.","注目されなくなった。"));
			break;
		}
	default: break;
	}
}

/*sit.c*/
