/*	SCCS Id: @(#)fountain.c	3.4	2003/03/23	*/
/*	Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

STATIC_DCL void NDECL(dowatersnakes);
STATIC_DCL void NDECL(dowaterdemon);
STATIC_DCL void NDECL(dowaternymph);
STATIC_PTR void FDECL(gush, (int,int,genericptr_t));
STATIC_DCL void NDECL(dofindgem);

void
floating_above(what)
const char *what;
{
    You(E_J("are floating high above the %s.",
	    "%sのはるか上に浮いている。"), what);
}

STATIC_OVL void
dowatersnakes() /* Fountain of snakes! */
{
    register int num = rn1(5,2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
	if (!Blind)
#ifndef JP
	    pline("An endless stream of %s pours forth!",
		  Hallucination ? makeplural(rndmonnam()) : "snakes");
#else
	    pline("%sの群れがとめどなく流れ出てきた！",
		  Hallucination ? rndmonnam() : "蛇");
#endif /*JP*/
	else
	    You_hear(E_J("%s hissing!","%sがシューッと音を立てるのを"), something);
	while(num-- > 0)
	    if((mtmp = makemon(&mons[PM_WATER_MOCCASIN],
			u.ux, u.uy, NO_MM_FLAGS)) && t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
    } else
	pline_The(E_J("fountain bubbles furiously for a moment, then calms.",
		      "泉は恐ろしげにぶくぶくと泡立ったが、じきにおさまった。"));
}

STATIC_OVL
void
dowaterdemon() /* Water demon */
{
    register struct monst *mtmp;

    if(!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
	if((mtmp = makemon(&mons[PM_WATER_DEMON],u.ux,u.uy, NO_MM_FLAGS))) {
	    if (!Blind)
		You(E_J("unleash %s!","%sを解き放ってしまった！"), a_monnam(mtmp));
	    else
		You_feel(E_J("the presence of evil.","邪悪な存在を感じた。"));

	/* Give those on low levels a (slightly) better chance of survival */
	    if (rnd(100) > (80 + level_difficulty())) {
#ifndef JP
		pline("Grateful for %s release, %s grants you a wish!",
		      mhis(mtmp), mhe(mtmp));
#else
		pline("解放への見返りに、%sはあなたの願いをかなえてくれる！",
		      mhe(mtmp));
#endif /*JP*/
		makewish();
		mongone(mtmp);
	    } else if (t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
	}
    } else
	pline_The(E_J("fountain bubbles furiously for a moment, then calms.",
		      "泉は恐ろしげにぶくぶくと泡立ったが、じきにおさまった。"));
}

STATIC_OVL void
dowaternymph() /* Water Nymph */
{
	register struct monst *mtmp;

	if(!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
	   (mtmp = makemon(&mons[PM_WATER_NYMPH],u.ux,u.uy, NO_MM_FLAGS))) {
		if (!Blind)
		   You(E_J("attract %s!","%sを呼び寄せてしまった！"), a_monnam(mtmp));
		else
		   You_hear(E_J("a seductive voice.","誘惑するような声を"));
		mtmp->msleeping = 0;
		if (t_at(mtmp->mx, mtmp->my))
		    (void) mintrap(mtmp);
	} else
		if (!Blind)
		   pline(E_J("A large bubble rises to the surface and pops.",
			     "大きな泡が水底から浮かんできて、はじけた。"));
		else
		   You_hear(E_J("a loud pop.","大きな泡のはじける音を"));
}

void
dogushforth(drinking) /* Gushing forth along LOS from (u.ux, u.uy) */
int drinking;
{
	int madepool = 0;

	do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t)&madepool);
	if (!madepool) {
	    if (drinking)
		Your(E_J("thirst is quenched.","喉の渇きは癒された。"));
	    else
		pline(E_J("Water sprays all over you.",
			  "水が吹き出してあなたを濡らした。"));
	}
}

STATIC_PTR void
gush(x, y, poolcnt)
int x, y;
genericptr_t poolcnt;
{
	register struct monst *mtmp;
	register struct trap *ttmp;

	if (((x+y)%2) || (x == u.ux && y == u.uy) ||
	    (rn2(1 + distmin(u.ux, u.uy, x, y)))  ||
	    !IS_FLOOR(levl[x][y].typ) ||
	    (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
		return;

	if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
		return;

	if (!((*(int *)poolcnt)++))
	    pline(E_J("Water gushes forth from the overflowing fountain!",
		      "泉はあふれ、水流が激しく噴き出した！"));

	/* Put a pool at x, y */
	levl[x][y].typ = POOL;
	/* No kelp! */
	del_engr_at(x, y);
	water_damage(level.objects[x][y], FALSE, TRUE);

	if ((mtmp = m_at(x, y)) != 0)
		(void) minliquid(mtmp);
	else
		newsym(x,y);
}

STATIC_OVL void
dofindgem() /* Find a gem in the sparkling waters. */
{
	if (!Blind) You(E_J("spot a gem in the sparkling waters!",
			    "きらきらと光る水の中に宝石を見つけた！"));
	else You_feel(E_J("a gem here!","宝石を見つけた！"));
	(void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE-1),
			 u.ux, u.uy, FALSE, FALSE);
	SET_FOUNTAIN_LOOTED(u.ux,u.uy);
	newsym(u.ux, u.uy);
	exercise(A_WIS, TRUE);			/* a discovery! */
}

void
dryup(x, y, isyou)
xchar x, y;
boolean isyou;
{
	if (IS_FOUNTAIN(levl[x][y].typ) &&
	    (!rn2(3) || FOUNTAIN_IS_WARNED(x,y))) {
		if(isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x,y)) {
			struct monst *mtmp;
			SET_FOUNTAIN_WARNED(x,y);
			/* Warn about future fountain use. */
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
			    if (DEADMONSTER(mtmp)) continue;
			    if ((mtmp->data == &mons[PM_WATCHMAN] ||
				mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			       couldsee(mtmp->mx, mtmp->my) &&
			       mtmp->mpeaceful) {
				pline(E_J("%s yells:","%sが叫んだ:"), Amonnam(mtmp));
				verbalize(E_J("Hey, stop using that fountain!",
					      "おい、泉を使うのをやめろ！"));
				break;
			    }
			}
			/* You can see or hear this effect */
			if(!mtmp) pline_The(E_J("flow reduces to a trickle.",
						"水の流れは乏しくなり、したたるだけになった。"));
			return;
		}
#ifdef WIZARD
		if (isyou && wizard) {
			if (yn("Dry up fountain?") == 'n')
				return;
		}
#endif
		/* replace the fountain with ordinary floor */
		levl[x][y].typ = ROOM;
		levl[x][y].looted = 0;
		levl[x][y].blessedftn = 0;
		if (cansee(x,y)) pline_The(E_J("fountain dries up!","泉は枯れた！"));
		/* The location is seen if the hero/monster is invisible */
		/* or felt if the hero is blind.			 */
		newsym(x, y);
		level.flags.nfountains--;
		if(isyou && in_town(x, y))
		    (void) angry_guards(FALSE);
	}
}

void
drinkfountain()
{
	/* What happens when you drink from a fountain? */
	register boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
	register int fate = rnd(30);

	if (Levitation) {
		floating_above(E_J("fountain","泉"));
		return;
	}

	if (mgkftn && u.uluck >= 0 && fate >= 10) {
		int i, ii, littleluck = (u.uluck < 4);

		pline(E_J("Wow!  This makes you feel great!",
			  "わあ！ とても気分が良くなった！"));
		/* blessed restore ability */
		for (ii = 0; ii < A_MAX; ii++)
		    if (ABASE(ii) < AMAX(ii)) {
			ABASE(ii) = AMAX(ii);
			flags.botl = 1;
		    }
		/* gain ability, blessed if "natural" luck is high */
		i = rn2(A_MAX);		/* start at a random attribute */
		for (ii = 0; ii < A_MAX; ii++) {
		    if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
			break;
		    if (++i >= A_MAX) i = 0;
		}
		display_nhwindow(WIN_MESSAGE, FALSE);
		pline(E_J("A wisp of vapor escapes the fountain...",
			  "一条の霞が泉を離れていった…。"));
		exercise(A_WIS, TRUE);
		levl[u.ux][u.uy].blessedftn = 0;
		return;
	}

	if (fate < 10) {
		pline_The(E_J("cool draught refreshes you.",
			      "冷たい水があなたを癒した。"));
		u.uhunger += rn1(50,50); /* don't choke on water */
		newuhs(FALSE);
		if(mgkftn) return;
	} else {
	    switch (fate) {

		case 19: /* Self-knowledge */

			You_feel(E_J("self-knowledgeable...",
				     "自分をよりよく理解した…。"));
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(0);
			exercise(A_WIS, TRUE);
			pline_The(E_J("feeling subsides.","直感は消えていった。"));
			break;

		case 20: /* Foul water */

			pline_The(E_J("water is foul!  You gag and vomit.",
				      "この水は腐っている！ あなたは吐き気を催し、嘔吐した。"));
			morehungry(rn1(20, 11));
			vomit();
			break;

		case 21: /* Poisonous */

			pline_The(E_J("water is contaminated!",
				      "この水は汚染されている！"));
			if (Poison_resistance) {
			   pline(
			      E_J("Perhaps it is runoff from the nearby %s farm.",
				  "おそらくこれは近くの%s農場から流れてきたものだろう。"),
				 fruitname(FALSE));
			   losehp(rnd(4),E_J("unrefrigerated sip of juice",
					     "常温で放置されたジュースを飲んで"),
				KILLED_BY_AN);
			   break;
			}
			losestr(rn1(4,3));
			losehp(rnd(10),E_J("contaminated water","汚染された水を飲んで"), KILLED_BY);
			exercise(A_CON, FALSE);
			break;

		case 22: /* Fountain of snakes! */

			dowatersnakes();
			break;

		case 23: /* Water demon */
			dowaterdemon();
			break;

		case 24: /* Curse an item */ {
			register struct obj *obj;

			pline(E_J("This water's no good!","この水は全くひどい！"));
			morehungry(rn1(20, 11));
			exercise(A_CON, FALSE);
			for(obj = invent; obj ; obj = obj->nobj)
				if (!rn2(7))	curse(obj);
			break;
			}

		case 25: /* See invisible */

			if (Blind) {
			    if (Invisible) {
				You(E_J("feel transparent.",
					"透けているような気になった。"));
			    } else {
			    	You(E_J("feel very self-conscious.",
					"とても自分自身を把握している気になった。"));
			    	pline(E_J("Then it passes.","その感じは過ぎ去った。"));
			    }
			} else {
			   You(E_J("see an image of someone stalking you.",
				   "誰かがあなたを尾けている光景を見た。"));
			   pline(E_J("But it disappears.","だが、それは消え失せた。"));
			}
			HSee_invisible |= FROMOUTSIDE;
			newsym(u.ux,u.uy);
			exercise(A_WIS, TRUE);
			break;

		case 26: /* See Monsters */

			(void) monster_detect((struct obj *)0, 0);
			exercise(A_WIS, TRUE);
			break;

		case 27: /* Find a gem in the sparkling waters. */

			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}

		case 28: /* Water Nymph */

			dowaternymph();
			break;

		case 29: /* Scare */ {
			register struct monst *mtmp;

			pline(E_J("This water gives you bad breath!",
				  "この水はあなたの息をひどく臭くした！"));
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			    if(!DEADMONSTER(mtmp))
				monflee(mtmp, 0, FALSE, FALSE);
			}
			break;

		case 30: /* Gushing forth in this room */

			dogushforth(TRUE);
			break;

		default:

			pline(E_J("This tepid water is tasteless.",
				  "このぬるい水は何の味もしない。"));
			break;
	    }
	}
	dryup(u.ux, u.uy, TRUE);
}

void
dipfountain(obj)
register struct obj *obj;
{
	int res_getwet;

	if (Levitation) {
		floating_above(E_J("fountain","泉"));
		return;
	}

	/* Don't grant Excalibur when there's more than one object.  */
	/* (quantity could be > 1 if merged daggers got polymorphed) */
	if (obj->otyp == LONG_SWORD && obj->quan == 1L
	    && u.ulevel >= 5 && !rn2(Role_if(PM_KNIGHT) ? 10 : 50)
	    && !obj->oartifact
	    && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {

		if (u.ualign.type != A_LAWFUL) {
			/* Ha!  Trying to cheat her. */
			pline(E_J("A freezing mist rises from the water and envelopes the sword.",
				  "凍てつく霧が水から湧きあがり、剣をつつんだ。"));
			pline_The(E_J("fountain disappears!","泉は消え失せた！"));
			curse(obj);
			if (obj->spe > -6 && !rn2(3)) obj->spe--;
			obj->oerodeproof = FALSE;
			exercise(A_WIS, FALSE);
		} else {
			/* The lady of the lake acts! - Eric Backus */
			/* Be *REAL* nice */
	  pline(E_J("From the murky depths, a hand reaches up to bless the sword.",
		    "ほの暗い水底から、剣を祝福せんと手がさしのべられた。"));
			pline(E_J("As the hand retreats, the fountain disappears!",
				  "手が退くと、泉は消え失せた！"));
			obj = oname(obj, artiname(ART_EXCALIBUR));
			discover_artifact(ART_EXCALIBUR);
			bless(obj);
			obj->oeroded = obj->oeroded2 = 0;
			obj->oerodeproof = TRUE;
			exercise(A_WIS, TRUE);
		}
		update_inventory();
		levl[u.ux][u.uy].typ = ROOM;
		levl[u.ux][u.uy].looted = 0;
		newsym(u.ux, u.uy);
		level.flags.nfountains--;
		if(in_town(u.ux, u.uy))
		    (void) angry_guards(FALSE);
		return;
	} else if ((res_getwet = get_wet(obj)) != 0) {
		if (res_getwet & 0x02)
		    useupall(obj);
		if (!rn2(2)) return;
	}

	/* Acid and water don't mix */
//	if (obj->otyp == POT_ACID) {
//	    useup(obj);
//	    return;
//	}

	switch (rnd(30)) {
		case 11: /* Curse the item */
			curse(obj);
			break;
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17: /* Uncurse the item */
			if(obj->cursed) {
			    if (!Blind)
				pline_The(E_J("water glows for a moment.",
					      "水がひととき輝いた。"));
			    uncurse(obj);
			} else {
			    pline(E_J("A feeling of loss comes over you.",
				      "あなたは不意に喪失感におそわれた。"));
			}
			break;
		case 18: /* Water Demon */
		case 19:
			dowaterdemon();
			break;
		case 20: /* Water Nymph */
		case 21:
			dowaternymph();
			break;
		case 22: /* an Endless Stream of Snakes */
		case 23:
			dowatersnakes();
			break;
		case 24: /* Find a gem */
			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}
		case 25: /* Water gushes forth */
			dogushforth(FALSE);
			break;
		case 26: /* Strange feeling */
			pline(E_J("A strange tingling runs up your %s.",
				  "あなたの%sに不思議なうずきが走った。"),
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			You_feel(E_J("a sudden chill.","不意に冷たさを感じた。"));
			break;
		case 28: /* Strange feeling */
			pline(E_J("An urge to take a bath overwhelms you.",
				  "急に、あなたは水浴びをしたいという欲求を抑えられなくなった。"));
#ifndef GOLDOBJ
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
			    You(E_J("lost some of your gold in the fountain!",
				    "泉に金貨をいくらか落としてしまった！"));
			    CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			    exercise(A_WIS, FALSE);
			}
#else
			{
			    long money = money_cnt(invent);
			    struct obj *otmp;
                            if (money > 10) {
				/* Amount to loose.  Might get rounded up as fountains don't pay change... */
			        money = somegold(money) / 10; 
			        for (otmp = invent; otmp && money > 0; otmp = otmp->nobj) if (otmp->oclass == COIN_CLASS) {
				    int denomination = objects[otmp->otyp].oc_cost;
				    long coin_loss = (money + denomination - 1) / denomination;
                                    coin_loss = min(coin_loss, otmp->quan);
				    otmp->quan -= coin_loss;
				    money -= coin_loss * denomination;
				    if (!otmp->quan) delobj(otmp);
				}
			        You(E_J("lost some of your money in the fountain!",
					"泉にいくらかの金貨を落としてしまった！"));
				CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			        exercise(A_WIS, FALSE);
                            }
			}
#endif
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

		    if (FOUNTAIN_IS_LOOTED(u.ux,u.uy)) break;
		    SET_FOUNTAIN_LOOTED(u.ux,u.uy);
		    (void) mkgold((long)
			(rnd((dunlevs_in_dungeon(&u.uz)-dunlev(&u.uz)+1)*2)+5),
			u.ux, u.uy);
		    if (!Blind)
		pline(E_J("Far below you, you see coins glistening in the water.",
			  "水面下深くに金貨がきらめいているのを、あなたは見た。"));
		    exercise(A_WIS, TRUE);
		    newsym(u.ux,u.uy);
		    break;
	}
	update_inventory();
	dryup(u.ux, u.uy, TRUE);
}

#ifdef SINKS
void
breaksink(x,y)
int x, y;
{
    if(cansee(x,y) || (x == u.ux && y == u.uy))
	pline_The(E_J("pipes break!  Water spurts out!",
		      "配管が壊れ、水が噴き出した！"));
    level.flags.nsinks--;
    levl[x][y].doormask = 0;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x,y);
}

void
drinksink()
{
	struct obj *otmp;
	struct monst *mtmp;

	if (Levitation) {
		floating_above(E_J("sink","流し台"));
		return;
	}
	switch(rn2(20)) {
		case 0: You(E_J("take a sip of very cold water.",
				"とても冷たい水を飲んだ。"));
			break;
		case 1: You(E_J("take a sip of very warm water.",
				"とても温かい湯を飲んだ。"));
			break;
		case 2: You(E_J("take a sip of scalding hot water.",
				"火傷するほどの熱湯を飲んだ。"));
			if (Fire_resistance)
				pline(E_J("It seems quite tasty.",
					  "とてもおいしく感じる。"));
			else losehp(rnd(6), E_J("sipping boiling water",
						"熱湯を飲んで"), KILLED_BY);
			break;
		case 3: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline_The(E_J("sink seems quite dirty.",
					      "流し台はひどく汚れているようだ。"));
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT],
						u.ux, u.uy, NO_MM_FLAGS);
				if (mtmp) pline(E_J("Eek!  There's %s in the sink!",
						    "ぎゃっ！ 流し台に%sがいる！"),
					(Blind || !canspotmon(mtmp)) ?
					E_J("something squirmy","何かもぞもぞ動くもの") :
					a_monnam(mtmp));
			}
			break;
		case 4: do {
				otmp = mkobj(POTION_CLASS,FALSE);
				if (otmp->otyp == POT_WATER) {
					obfree(otmp, (struct obj *)0);
					otmp = (struct obj *) 0;
				}
			} while(!otmp);
			otmp->cursed = otmp->blessed = 0;
			pline(E_J("Some %s liquid flows from the faucet.",
				  "蛇口から%s液体が流れ出てきた。"),
			      Blind ? E_J("odd","奇妙な") :
			      hcolor(OBJ_DESCR(objects[otmp->otyp])));
			otmp->dknown = !(Blind || Hallucination);
			otmp->quan++; /* Avoid panic upon useup() */
			otmp->fromsink = 1; /* kludge for docall() */
			(void) dopotion(otmp);
			obfree(otmp, (struct obj *)0);
			break;
		case 5: if (!(levl[u.ux][u.uy].looted & S_LRING)) {
			    You(E_J("find a ring in the sink!",
				    "流し台の中に指輪を見つけた！"));
			    (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
			    levl[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, TRUE);
			    newsym(u.ux,u.uy);
			} else pline(E_J("Some dirty water backs up in the drain.",
					 "汚水がいくらか排水口から逆流してきた。"));
			break;
		case 6: breaksink(u.ux,u.uy);
			break;
		case 7: pline_The(E_J("water moves as though of its own will!",
				      "水が意思を持ったかのように動き出した！"));
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL],
					u.ux, u.uy, NO_MM_FLAGS))
				pline(E_J("But it quiets down.",
					  "だが、水の動きはおさまり、流れ去った。"));
			break;
		case 8: pline(E_J("Yuk, this water tastes awful.",
				  "おえっ、この水はひどい味だ。"));
			more_experienced(1,0);
			newexplevel();
			break;
		case 9: pline(E_J("Gaggg... this tastes like sewage!  You vomit.",
				  "ウエェェ… 下水の味がする！ あなたは嘔吐した。"));
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
			break;
		case 10: pline(E_J("This water contains toxic wastes!",
				   "この水には有毒な廃液が含まれていた！"));
			if (!Unchanging) {
#ifndef JP
				You("undergo a freakish metamorphosis!");
#else
				Your("身体が異常に変異しはじめた！");
#endif /*JP*/
				polyself(FALSE);
			}
			break;
		/* more odd messages --JJB */
		case 11: You_hear(E_J("clanking from the pipes...","_配管の鳴る音を"));
			break;
		case 12: You_hear(E_J("snatches of song from among the sewers...",
				      "_下水からもれてくる歌の断片を"));
			break;
		case 19: if (Hallucination) {
		   pline(E_J("From the murky drain, a hand reaches up... --oops--",
			     "ほの暗い排水口から、手がのびてきた… --おおっと--"));
				break;
			}
#ifndef JP
		default: You("take a sip of %s water.",
			rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot");
#else
		default: You("%sを飲んだ。",
			rn2(3) ? (rn2(2) ? "冷たい水" : "温かい湯") : "熱い湯");
#endif /*JP*/
	}
}
#endif /* SINKS */

/*fountain.c*/
