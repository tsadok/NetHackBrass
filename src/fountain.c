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
	    "%s�̂͂邩��ɕ����Ă���B"), what);
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
	    pline("%s�̌Q�ꂪ�Ƃ߂ǂȂ�����o�Ă����I",
		  Hallucination ? rndmonnam() : "��");
#endif /*JP*/
	else
	    You_hear(E_J("%s hissing!","%s���V���[�b�Ɖ��𗧂Ă�̂�"), something);
	while(num-- > 0)
	    if((mtmp = makemon(&mons[PM_WATER_MOCCASIN],
			u.ux, u.uy, NO_MM_FLAGS)) && t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
    } else
	pline_The(E_J("fountain bubbles furiously for a moment, then calms.",
		      "��͋��낵���ɂԂ��Ԃ��ƖA���������A�����ɂ����܂����B"));
}

STATIC_OVL
void
dowaterdemon() /* Water demon */
{
    register struct monst *mtmp;

    if(!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
	if((mtmp = makemon(&mons[PM_WATER_DEMON],u.ux,u.uy, NO_MM_FLAGS))) {
	    if (!Blind)
		You(E_J("unleash %s!","%s�����������Ă��܂����I"), a_monnam(mtmp));
	    else
		You_feel(E_J("the presence of evil.","�׈��ȑ��݂��������B"));

	/* Give those on low levels a (slightly) better chance of survival */
	    if (rnd(100) > (80 + level_difficulty())) {
#ifndef JP
		pline("Grateful for %s release, %s grants you a wish!",
		      mhis(mtmp), mhe(mtmp));
#else
		pline("����ւ̌��Ԃ�ɁA%s�͂��Ȃ��̊肢�����Ȃ��Ă����I",
		      mhe(mtmp));
#endif /*JP*/
		makewish();
		mongone(mtmp);
	    } else if (t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
	}
    } else
	pline_The(E_J("fountain bubbles furiously for a moment, then calms.",
		      "��͋��낵���ɂԂ��Ԃ��ƖA���������A�����ɂ����܂����B"));
}

STATIC_OVL void
dowaternymph() /* Water Nymph */
{
	register struct monst *mtmp;

	if(!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
	   (mtmp = makemon(&mons[PM_WATER_NYMPH],u.ux,u.uy, NO_MM_FLAGS))) {
		if (!Blind)
		   You(E_J("attract %s!","%s���Ăъ񂹂Ă��܂����I"), a_monnam(mtmp));
		else
		   You_hear(E_J("a seductive voice.","�U�f����悤�Ȑ���"));
		mtmp->msleeping = 0;
		if (t_at(mtmp->mx, mtmp->my))
		    (void) mintrap(mtmp);
	} else
		if (!Blind)
		   pline(E_J("A large bubble rises to the surface and pops.",
			     "�傫�ȖA�����ꂩ�畂����ł��āA�͂������B"));
		else
		   You_hear(E_J("a loud pop.","�傫�ȖA�̂͂����鉹��"));
}

void
dogushforth(drinking) /* Gushing forth along LOS from (u.ux, u.uy) */
int drinking;
{
	int madepool = 0;

	do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t)&madepool);
	if (!madepool) {
	    if (drinking)
		Your(E_J("thirst is quenched.","�A�̊����͖����ꂽ�B"));
	    else
		pline(E_J("Water sprays all over you.",
			  "���������o���Ă��Ȃ���G�炵���B"));
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
		      "��͂��ӂ�A�����������������o�����I"));

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
			    "���炫��ƌ��鐅�̒��ɕ�΂��������I"));
	else You_feel(E_J("a gem here!","��΂��������I"));
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
				pline(E_J("%s yells:","%s������:"), Amonnam(mtmp));
				verbalize(E_J("Hey, stop using that fountain!",
					      "�����A����g���̂���߂�I"));
				break;
			    }
			}
			/* You can see or hear this effect */
			if(!mtmp) pline_The(E_J("flow reduces to a trickle.",
						"���̗���͖R�����Ȃ�A�������邾���ɂȂ����B"));
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
		if (cansee(x,y)) pline_The(E_J("fountain dries up!","��͌͂ꂽ�I"));
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
		floating_above(E_J("fountain","��"));
		return;
	}

	if (mgkftn && u.uluck >= 0 && fate >= 10) {
		int i, ii, littleluck = (u.uluck < 4);

		pline(E_J("Wow!  This makes you feel great!",
			  "�킠�I �ƂĂ��C�����ǂ��Ȃ����I"));
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
			  "����̉�����𗣂�Ă������c�B"));
		exercise(A_WIS, TRUE);
		levl[u.ux][u.uy].blessedftn = 0;
		return;
	}

	if (fate < 10) {
		pline_The(E_J("cool draught refreshes you.",
			      "�₽���������Ȃ���������B"));
		u.uhunger += rn1(50,50); /* don't choke on water */
		newuhs(FALSE);
		if(mgkftn) return;
	} else {
	    switch (fate) {

		case 19: /* Self-knowledge */

			You_feel(E_J("self-knowledgeable...",
				     "���������悭���������c�B"));
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(0);
			exercise(A_WIS, TRUE);
			pline_The(E_J("feeling subsides.","�����͏����Ă������B"));
			break;

		case 20: /* Foul water */

			pline_The(E_J("water is foul!  You gag and vomit.",
				      "���̐��͕����Ă���I ���Ȃ��͓f���C���Â��A�q�f�����B"));
			morehungry(rn1(20, 11));
			vomit();
			break;

		case 21: /* Poisonous */

			pline_The(E_J("water is contaminated!",
				      "���̐��͉�������Ă���I"));
			if (Poison_resistance) {
			   pline(
			      E_J("Perhaps it is runoff from the nearby %s farm.",
				  "�����炭����͋߂���%s�_�ꂩ�痬��Ă������̂��낤�B"),
				 fruitname(FALSE));
			   losehp(rnd(4),E_J("unrefrigerated sip of juice",
					     "�퉷�ŕ��u���ꂽ�W���[�X�������"),
				KILLED_BY_AN);
			   break;
			}
			losestr(rn1(4,3));
			losehp(rnd(10),E_J("contaminated water","�������ꂽ���������"), KILLED_BY);
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

			pline(E_J("This water's no good!","���̐��͑S���Ђǂ��I"));
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
					"�����Ă���悤�ȋC�ɂȂ����B"));
			    } else {
			    	You(E_J("feel very self-conscious.",
					"�ƂĂ��������g��c�����Ă���C�ɂȂ����B"));
			    	pline(E_J("Then it passes.","���̊����͉߂��������B"));
			    }
			} else {
			   You(E_J("see an image of someone stalking you.",
				   "�N�������Ȃ�������Ă�����i�������B"));
			   pline(E_J("But it disappears.","�����A����͏����������B"));
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
				  "���̐��͂��Ȃ��̑����Ђǂ��L�������I"));
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
				  "���̂ʂ邢���͉��̖������Ȃ��B"));
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
		floating_above(E_J("fountain","��"));
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
				  "���Ă�����������N��������A�����񂾁B"));
			pline_The(E_J("fountain disappears!","��͏����������I"));
			curse(obj);
			if (obj->spe > -6 && !rn2(3)) obj->spe--;
			obj->oerodeproof = FALSE;
			exercise(A_WIS, FALSE);
		} else {
			/* The lady of the lake acts! - Eric Backus */
			/* Be *REAL* nice */
	  pline(E_J("From the murky depths, a hand reaches up to bless the sword.",
		    "�ق̈Â����ꂩ��A�����j������Ǝ肪�����ׂ̂�ꂽ�B"));
			pline(E_J("As the hand retreats, the fountain disappears!",
				  "�肪�ނ��ƁA��͏����������I"));
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
					      "�����ЂƂƂ��P�����B"));
			    uncurse(obj);
			} else {
			    pline(E_J("A feeling of loss comes over you.",
				      "���Ȃ��͕s�ӂɑr�����ɂ�����ꂽ�B"));
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
				  "���Ȃ���%s�ɕs�v�c�Ȃ��������������B"),
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			You_feel(E_J("a sudden chill.","�s�ӂɗ₽�����������B"));
			break;
		case 28: /* Strange feeling */
			pline(E_J("An urge to take a bath overwhelms you.",
				  "�}�ɁA���Ȃ��͐����т��������Ƃ����~����}�����Ȃ��Ȃ����B"));
#ifndef GOLDOBJ
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
			    You(E_J("lost some of your gold in the fountain!",
				    "��ɋ��݂������炩���Ƃ��Ă��܂����I"));
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
					"��ɂ����炩�̋��݂𗎂Ƃ��Ă��܂����I"));
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
			  "���ʉ��[���ɋ��݂�����߂��Ă���̂��A���Ȃ��͌����B"));
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
		      "�z�ǂ����A���������o�����I"));
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
		floating_above(E_J("sink","������"));
		return;
	}
	switch(rn2(20)) {
		case 0: You(E_J("take a sip of very cold water.",
				"�ƂĂ��₽���������񂾁B"));
			break;
		case 1: You(E_J("take a sip of very warm water.",
				"�ƂĂ��������������񂾁B"));
			break;
		case 2: You(E_J("take a sip of scalding hot water.",
				"�Ώ�����قǂ̔M�������񂾁B"));
			if (Fire_resistance)
				pline(E_J("It seems quite tasty.",
					  "�ƂĂ���������������B"));
			else losehp(rnd(6), E_J("sipping boiling water",
						"�M���������"), KILLED_BY);
			break;
		case 3: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline_The(E_J("sink seems quite dirty.",
					      "������͂Ђǂ�����Ă���悤���B"));
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT],
						u.ux, u.uy, NO_MM_FLAGS);
				if (mtmp) pline(E_J("Eek!  There's %s in the sink!",
						    "������I �������%s������I"),
					(Blind || !canspotmon(mtmp)) ?
					E_J("something squirmy","��������������������") :
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
				  "�֌�����%s�t�̂�����o�Ă����B"),
			      Blind ? E_J("odd","���") :
			      hcolor(OBJ_DESCR(objects[otmp->otyp])));
			otmp->dknown = !(Blind || Hallucination);
			otmp->quan++; /* Avoid panic upon useup() */
			otmp->fromsink = 1; /* kludge for docall() */
			(void) dopotion(otmp);
			obfree(otmp, (struct obj *)0);
			break;
		case 5: if (!(levl[u.ux][u.uy].looted & S_LRING)) {
			    You(E_J("find a ring in the sink!",
				    "������̒��Ɏw�ւ��������I"));
			    (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
			    levl[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, TRUE);
			    newsym(u.ux,u.uy);
			} else pline(E_J("Some dirty water backs up in the drain.",
					 "�����������炩�r��������t�����Ă����B"));
			break;
		case 6: breaksink(u.ux,u.uy);
			break;
		case 7: pline_The(E_J("water moves as though of its own will!",
				      "�����ӎv�����������̂悤�ɓ����o�����I"));
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL],
					u.ux, u.uy, NO_MM_FLAGS))
				pline(E_J("But it quiets down.",
					  "�����A���̓����͂����܂�A���ꋎ�����B"));
			break;
		case 8: pline(E_J("Yuk, this water tastes awful.",
				  "�������A���̐��͂Ђǂ������B"));
			more_experienced(1,0);
			newexplevel();
			break;
		case 9: pline(E_J("Gaggg... this tastes like sewage!  You vomit.",
				  "�E�G�F�F�c �����̖�������I ���Ȃ��͚q�f�����B"));
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
			break;
		case 10: pline(E_J("This water contains toxic wastes!",
				   "���̐��ɂ͗L�łȔp�t���܂܂�Ă����I"));
			if (!Unchanging) {
#ifndef JP
				You("undergo a freakish metamorphosis!");
#else
				Your("�g�̂��ُ�ɕψق��͂��߂��I");
#endif /*JP*/
				polyself(FALSE);
			}
			break;
		/* more odd messages --JJB */
		case 11: You_hear(E_J("clanking from the pipes...","_�z�ǂ̖鉹��"));
			break;
		case 12: You_hear(E_J("snatches of song from among the sewers...",
				      "_�����������Ă���̂̒f�Ђ�"));
			break;
		case 19: if (Hallucination) {
		   pline(E_J("From the murky drain, a hand reaches up... --oops--",
			     "�ق̈Â��r��������A�肪�̂тĂ����c --��������--"));
				break;
			}
#ifndef JP
		default: You("take a sip of %s water.",
			rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot");
#else
		default: You("%s�����񂾁B",
			rn2(3) ? (rn2(2) ? "�₽����" : "��������") : "�M����");
#endif /*JP*/
	}
}
#endif /* SINKS */

/*fountain.c*/
