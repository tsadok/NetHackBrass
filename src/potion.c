/*	SCCS Id: @(#)potion.c	3.4	2002/10/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVLB
boolean notonhead = FALSE;

static NEARDATA int nothing, unkn;
static NEARDATA const char beverages[] = { POTION_CLASS, 0 };

STATIC_DCL long FDECL(itimeout, (long));
STATIC_DCL long FDECL(itimeout_incr, (long,int));
STATIC_DCL void NDECL(ghost_from_bottle);
STATIC_DCL short FDECL(mixtype, (struct obj *,struct obj *));

/* force `val' to be within valid range for intrinsic timeout value */
STATIC_OVL long
itimeout(val)
long val;
{
    if (val >= TIMEOUT) val = TIMEOUT;
    else if (val < 1) val = 0;

    return val;
}

/* increment `old' by `incr' and force result to be valid intrinsic timeout */
STATIC_OVL long
itimeout_incr(old, incr)
long old;
int incr;
{
    return itimeout((old & TIMEOUT) + (long)incr);
}

/* set the timeout field of intrinsic `which' */
void
set_itimeout(which, val)
long *which, val;
{
    *which &= ~TIMEOUT;
    *which |= itimeout(val);
}

/* increment the timeout field of intrinsic `which' */
void
incr_itimeout(which, incr)
long *which;
int incr;
{
    set_itimeout(which, itimeout_incr(*which, incr));
}

void
make_confused(xtime,talk)
long xtime;
boolean talk;
{
	long old = HConfusion;

	if (!xtime && old) {
		if (talk)
#ifndef JP
		    You_feel("less %s now.",
			Hallucination ? "trippy" : "confused");
#else
		    Your("%s�͂����܂����B",
			Hallucination ? "�g���b�v" : "����");
#endif /*JP*/
	}
	if ((xtime && !old) || (!xtime && old)) flags.botl = TRUE;

	set_itimeout(&HConfusion, xtime);
}

void
make_stunned(xtime,talk)
long xtime;
boolean talk;
{
	long old = HStun;

	if (!xtime && old) {
		if (talk)
#ifndef JP
		    You_feel("%s now.",
			Hallucination ? "less wobbly" : "a bit steadier");
#else
		    Your("%s���B",
			Hallucination ? "�t���t���͂����܂�" : "�p���͈�������߂�");
#endif /*JP*/
	}
	if (xtime && !old) {
		if (talk) {
#ifdef STEED
			if (u.usteed)
				You(E_J("wobble in the saddle.","�Ƃ̏�ł�낯���B"));
			else
#endif
			You(E_J("%s...","%s�c�B"), stagger(youmonst.data, E_J("stagger","���߂���")));
		}
	}
	if ((!xtime && old) || (xtime && !old)) flags.botl = TRUE;

	set_itimeout(&HStun, xtime);
}

void
make_sick(xtime, cause, talk, type)
long xtime;
const char *cause;	/* sickness cause */
boolean talk;
int type;
{
	long old = Sick;

	if (xtime > 0L) {
	    if (Sick_resistance) return;
	    if (!old) {
		/* newly sick */
		You_feel(E_J("deathly sick.","�a�C�Ŏ��ɂ������B"));
	    } else {
		/* already sick */
#ifndef JP
		if (talk) You_feel("%s worse.",
			      xtime <= Sick/2L ? "much" : "even");
#else
		if (talk) Your("�a���%s���������B",
				xtime <= Sick/2L ? "�����낵��" : "�����");
#endif /*JP*/
	    }
	    set_itimeout(&Sick, xtime);
	    u.usick_type |= type;
	    flags.botl = TRUE;
	} else if (old && (type & u.usick_type)) {
	    /* was sick, now not */
	    u.usick_type &= ~type;
	    if (u.usick_type) { /* only partly cured */
		if (talk) You_feel(E_J("somewhat better.","�����͋C�����ǂ��Ȃ����B"));
		set_itimeout(&Sick, Sick * 2); /* approximation */
	    } else {
		if (talk) pline(E_J("What a relief!","�����A���������I"));
		Sick = 0L;		/* set_itimeout(&Sick, 0L) */
	    }
	    flags.botl = TRUE;
	}

	if (Sick) {
	    exercise(A_CON, FALSE);
	    if (cause) {
		(void) strncpy(u.usick_cause, cause, sizeof(u.usick_cause));
		u.usick_cause[sizeof(u.usick_cause)-1] = 0;
		}
	    else
		u.usick_cause[0] = 0;
	} else
	    u.usick_cause[0] = 0;
}

void
make_vomiting(xtime, talk)
long xtime;
boolean talk;
{
	long old = Vomiting;

	if(!xtime && old)
	    if(talk) You_feel(E_J("much less nauseated now.","�f���C�͂����܂����B"));

	set_itimeout(&Vomiting, xtime);
}

#ifndef JP
static const char vismsg[] = "vision seems to %s for a moment but is %s now.";
static const char eyemsg[] = "%s momentarily %s.";
#else
static const char vismsg[] = "���E�͈�u%s���A%s�B";
static const char eyemsg[] = "%s����u%s�B";
#endif /*JP*/

void
make_blinded(xtime, talk)
long xtime;
boolean talk;
{
	long old = Blinded;
	boolean u_could_see, can_see_now;
	int eyecnt;
	char buf[BUFSZ];

	/* we need to probe ahead in case the Eyes of the Overworld
	   are or will be overriding blindness */
	u_could_see = !Blind;
	Blinded = xtime ? 1L : 0L;
	can_see_now = !Blind;
	Blinded = old;		/* restore */

	if (u.usleep) talk = FALSE;

	if (can_see_now && !u_could_see) {	/* regaining sight */
	    if (talk) {
		if (Hallucination)
		    pline(E_J("Far out!  Everything is all cosmic again!",
			      "�������I �S�Ă��܂��F���I����Ō�����I"));
		else
		    You(E_J("can see again.","�Ăіڂ�������悤�ɂȂ����B"));
	    }
	} else if (old && !xtime) {
	    /* clearing temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blindfolded) {
		    Strcpy(buf, body_part(EYE));
#ifndef JP
		    eyecnt = eyecount(youmonst.data);
		    Your(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "itches" : "itch");
#else
		    Your(eyemsg, buf, "�y���Ȃ���");
#endif /*JP*/
		} else {	/* Eyes of the Overworld */
		    Your(vismsg, E_J("brighten","�܂Ԃ����P����"),
			 Hallucination ? E_J("sadder","�ĂщA�T�Ȑ��E�ɋt�߂肵��") : E_J("normal","�����Ɍ��ɖ߂���"));
		}
	    }
	}

	if (u_could_see && !can_see_now) {	/* losing sight */
	    if (talk) {
		if (Hallucination)
		    pline(E_J("Oh, bummer!  Everything is dark!  Help!",
			      "���킠�A�T�C�e�[�I ���������Ȃ���I �����āI"));
		else
		    pline(E_J("A cloud of darkness falls upon you.",
			      "���Ȃ��̎��E�͎����̂Ƃ΂�ɕ�܂ꂽ�B"));
	    }
	    /* Before the hero goes blind, set the ball&chain variables. */
	    if (Punished) set_bc(0);
	} else if (!old && xtime) {
	    /* setting temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blindfolded) {
		    Strcpy(buf, body_part(EYE));
#ifndef JP
		    eyecnt = eyecount(youmonst.data);
		    Your(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "twitches" : "twitch");
#else
		    Your(eyemsg, buf, "�z������");
#endif /*JP*/
		} else {	/* Eyes of the Overworld */
		    Your(vismsg, E_J("dim","�Â��Ȃ���"),
			 Hallucination ? E_J("happier","���E�͍ĂыP�������߂���") : E_J("normal","�����Ɍ��ɖ߂���"));
		}
	    }
	}

	set_itimeout(&Blinded, xtime);

	if (u_could_see ^ can_see_now) {  /* one or the other but not both */
	    flags.botl = 1;
	    vision_full_recalc = 1;	/* blindness just got toggled */
	    if (Blind_telepat || Infravision) see_monsters();
	}
}

boolean
make_hallucinated(xtime, talk, mask)
long xtime;	/* nonzero if this is an attempt to turn on hallucination */
boolean talk;
long mask;	/* nonzero if resistance status should change by mask */
{
	long old = HHallucination;
	boolean changed = 0;
	const char *message, *verb;

#ifndef JP
	message = (!xtime) ? "Everything %s SO boring now." :
			     "Oh wow!  Everything %s so cosmic!";
	verb = (!Blind) ? "looks" : "feels";
#else
	message = (!xtime) ? "���₷�ׂĂ��ƂĂ��܂�Ȃ�%s�B" :
			     "���[�I�I �������������F��%s�I";
	verb = (!Blind) ? "������" : "������";
#endif /*JP*/

	if (mask) {
	    if (HHallucination) changed = TRUE;

	    if (!xtime) EHalluc_resistance |= mask;
	    else EHalluc_resistance &= ~mask;
	} else {
	    if (!EHalluc_resistance && (!!HHallucination != !!xtime))
		changed = TRUE;
	    set_itimeout(&HHallucination, xtime);

	    /* clearing temporary hallucination without toggling vision */
	    if (!changed && !HHallucination && old && talk) {
		if (!haseyes(youmonst.data)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blind) {
#ifndef JP
		    char buf[BUFSZ];
		    int eyecnt = eyecount(youmonst.data);

		    Strcpy(buf, body_part(EYE));
		    Your(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "itches" : "itch");
#else
		    Your(eyemsg, body_part(EYE), "�y���Ȃ���");
#endif /*JP*/
		} else {	/* Grayswandir */
		    Your(vismsg, E_J("flatten","���ɂȂ���"), E_J("normal","�����Ɍ��ɖ߂���"));
		}
	    }
	}

	if (changed) {
	    if (u.uswallow) {
		swallowed(0);	/* redraw swallow display */
	    } else {
		/* The see_* routines should be called *before* the pline. */
		see_monsters();
		see_objects();
		see_traps();
	    }

	    /* for perm_inv and anything similar
	    (eg. Qt windowport's equipped items display) */
	    update_inventory();

	    flags.botl = 1;
	    if (talk) pline(message, verb);
	}
	return changed;
}

STATIC_OVL void
ghost_from_bottle()
{
	struct monst *mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, NO_MM_FLAGS);

	if (!mtmp) {
		pline(E_J("This bottle turns out to be empty.",
			  "���̃r���͋󂾂����悤���B"));
		return;
	}
	if (Blind) {
		pline(E_J("As you open the bottle, %s emerges.",
			  "�r�����J����ƁA%s����яo�Ă����悤���B"), something);
		return;
	}
#ifndef JP
	pline("As you open the bottle, an enormous %s emerges!",
		Hallucination ? rndmonnam() : (const char *)"ghost");
#else
	pline("���Ȃ����r�����J�����Ƃ���A�����狐���%s����яo���Ă����I",
		Hallucination ? rndmonnam() : (const char *)"�H��");
#endif /*JP*/
	if(flags.verbose)
	    You(E_J("are frightened to death, and unable to move.",
		    "���|�̂��܂蓮���Ȃ��Ȃ����B"));
	nomul(-3);
	nomovemsg = E_J("You regain your composure.","���Ȃ��͕��Â����߂����B");
}

/* "Quaffing is like drinking, except you spill more."  -- Terry Pratchett
 */
int
dodrink()
{
	register struct obj *otmp;
	const char *potion_descr;
#ifdef JP
	static const struct getobj_words drinkw = { 0, 0, "����", "����" };
#endif /*JP*/

	if (Strangled) {
		pline("If you can't breathe air, how can you drink liquid?");
		return 0;
	}
	/* Is there a fountain to drink from here? */
	if (IS_FOUNTAIN(levl[u.ux][u.uy].typ) && !Levitation) {
		if(yn(E_J("Drink from the fountain?",
			  "�򂩂琅�����݂܂����H")) == 'y') {
			drinkfountain();
			return 1;
		}
	}
#ifdef SINKS
	/* Or a kitchen sink? */
	if (IS_SINK(levl[u.ux][u.uy].typ)) {
		if (yn(E_J("Drink from the sink?",
			   "�����̎֌����琅�����݂܂����H")) == 'y') {
			drinksink();
			return 1;
		}
	}
#endif

	/* Or are you surrounded by water? */
	if (Underwater) {
		if (yn(E_J("Drink the water around you?",
			   "�܂��̐������݂܂����H")) == 'y') {
		    pline(E_J("Do you know what lives in this water!",
			      "���̐��̒��ɉ����������Ă��邩�m���Ă���̂��I"));
			return 1;
		}
	}

	otmp = getobj(beverages, E_J("drink",&drinkw), 0);
	if(!otmp) return(0);
	otmp->in_use = TRUE;		/* you've opened the stopper */

#define POTION_OCCUPANT_CHANCE(n) (13 + 2*(n))	/* also in muse.c */

	potion_descr = OBJ_DESCR(objects[otmp->otyp]);
	if (potion_descr) {
	    if (!strcmp(potion_descr, "milky") &&
		    flags.ghost_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.ghost_count))) {
		ghost_from_bottle();
		useup(otmp);
		return(1);
	    } else if (!strcmp(potion_descr, "smoky") &&
		    flags.djinni_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.djinni_count))) {
		djinni_from_bottle(otmp);
		useup(otmp);
		return(1);
	    }
	}
	return dopotion(otmp);
}

int
dopotion(otmp)
register struct obj *otmp;
{
	int retval;

	otmp->in_use = TRUE;
	nothing = unkn = 0;
	if((retval = peffects(otmp)) >= 0) return(retval);

	if(nothing) {
	    unkn++;
#ifndef JP
	    You("have a %s feeling for a moment, then it passes.",
		  Hallucination ? "normal" : "peculiar");
#else
	    You("%s���o�ɂ�����ꂽ���A�����ɏ����������B",
		  Hallucination ? "���̕ϓN���Ȃ�" : "���");
#endif /*JP*/
	}
	if(otmp->dknown && !objects[otmp->otyp].oc_name_known) {
		if(!unkn) {
			makeknown(otmp->otyp);
			more_experienced(0,10);
		} else if(!objects[otmp->otyp].oc_uname)
			docall(otmp);
	}
	useup(otmp);
	return(1);
}

int
peffects(otmp)
	register struct obj	*otmp;
{
	register int i, ii, lim;

	switch(otmp->otyp){
	case POT_RESTORE_ABILITY:
	case SPE_RESTORE_ABILITY:
		unkn++;
		if(otmp->cursed) {
		    pline(E_J("Ulch!  This makes you feel mediocre!",
			      "�����I �������ƂĂ����}�ȑ��݂ɂȂ����݂������I"));
		    break;
		} else {
		    pline(E_J("Wow!  This makes you feel %s!","�킠�I %s�C�����ǂ��Ȃ����I"),
			  (otmp->blessed) ?
				(unfixable_trouble_count(FALSE) ? E_J("better","���Ȃ�") : E_J("great","�ō���"))
			  : E_J("good",""));
		    i = rn2(A_MAX);		/* start at a random point */
		    for (ii = 0; ii < A_MAX; ii++) {
			lim = AMAX(i);
			if (i == A_STR && u.uhs >= 3) --lim;	/* WEAK */
			if (ABASE(i) < lim) {
			    ABASE(i) = lim;
			    flags.botl = 1;
			    unkn = 0;
			    /* only first found if not blessed */
			    if (!otmp->blessed) break;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_HALLUCINATION:
		if (Hallucination || Halluc_resistance) nothing++;
		(void) make_hallucinated(itimeout_incr(HHallucination,
					   rn1(200, 600 - 300 * bcsign(otmp))),
				  TRUE, 0L);
		break;
	case POT_WATER:
		if(!otmp->blessed && !otmp->cursed) {
		    pline(E_J("This tastes like water.",
			      "���̂悤�Ȗ�������B"));
		    u.uhunger += rnd(10);
		    newuhs(FALSE);
		    break;
		}
		unkn++;
		if(is_undead(youmonst.data) || is_demon(youmonst.data) ||
				u.ualign.type == A_CHAOTIC) {
		    if(otmp->blessed) {
			pline(E_J("This burns like acid!",
				  "����͎_�̂悤�Ɏ܂����I"));
			exercise(A_CON, FALSE);
			if (u.ulycn >= LOW_PM) {
#ifndef JP
			    Your("affinity to %s disappears!",
				 makeplural(mons[u.ulycn].mname));
#else
			    Your("%s�ւ̋����͏����������I",
				 JMONNAM(u.ulycn));
#endif /*JP*/
			    if (youmonst.data == &mons[u.ulycn])
				you_unwere(FALSE);
			    u.ulycn = NON_PM;	/* cure lycanthropy */
			}
			losehp(d(2,6), E_J("potion of holy water","�����ɏĂ����"), KILLED_BY_AN);
		    } else if(otmp->cursed) {
			You_feel(E_J("quite proud of yourself.",
				     "�������ƂĂ��ւ炵���v�����B"));
			healup(d(2,6),0,0,0);
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, TRUE);
		    }
		} else {
		    if(otmp->blessed) {
			You_feel(E_J("full of awe.","�،h�̔O�ɖ������ꂽ�B"));
			make_sick(0L, (char *) 0, TRUE, SICK_ALL);
			exercise(A_WIS, TRUE);
			exercise(A_CON, TRUE);
			if (u.ulycn >= LOW_PM)
			    you_unwere(TRUE);	/* "Purified" */
			/* make_confused(0L,TRUE); */
		    } else {
			if(u.ualign.type == A_LAWFUL) {
			    pline(E_J("This burns like acid!",
				      "����͎_�̂悤�Ɏ܂����I"));
			    losehp(d(2,6), E_J("potion of unholy water","�s��̐��ɏĂ����"),
				KILLED_BY_AN);
			} else
			    You_feel(E_J("full of dread.","���|�ɖ������ꂽ�B"));
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, FALSE);
		    }
		}
		break;
	case POT_BOOZE:
		unkn++;
#ifndef JP
		pline("Ooph!  This tastes like %s%s!",
		      otmp->odiluted ? "watered down " : "",
		      Hallucination ? "dandelion wine" : "liquid fire");
#else
		pline("�����ՁI ����͂܂��%s%s���I",
		      otmp->odiluted ? "�������" : "",
		      Hallucination ? "�^���|�|��" : "�R���鐅");
#endif /*JP*/
		if (!otmp->blessed)
		    make_confused(itimeout_incr(HConfusion, d(3,8)), FALSE);
		/* the whiskey makes us feel better */
		if (!otmp->odiluted) healup(1, 0, FALSE, FALSE);
		u.uhunger += 10 * (2 + bcsign(otmp));
		newuhs(FALSE);
		exercise(A_WIS, FALSE);
		if(otmp->cursed) {
			You(E_J("pass out.","�C�₵���B"));
			multi = -rnd(15);
			nomovemsg = E_J("You awake with a headache.","���Ȃ��͖ڊo�߂��B���ɂ�����c�B");
		}
		break;
	case POT_ENLIGHTENMENT:
		if(otmp->cursed) {
			unkn++;
			You(E_J("have an uneasy feeling...",
				"���������Ȃ��C���ɂȂ����c�B"));
			exercise(A_WIS, FALSE);
		} else {
			if (otmp->blessed) {
				(void) adjattrib(A_INT, 1, FALSE);
				(void) adjattrib(A_WIS, 1, FALSE);
			}
			You_feel(E_J("self-knowledgeable...",
				     "���������悭���������c�B"));
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(0);
			pline_The(E_J("feeling subsides.",
				      "���@�͂͌��ɖ߂����B"));
			exercise(A_WIS, TRUE);
		}
		break;
	case SPE_INVISIBILITY:
		/* spell cannot penetrate mummy wrapping */
		if (BInvis && uarmc->otyp == MUMMY_WRAPPING) {
#ifndef JP
			You_feel("rather itchy under your %s.", xname(uarmc));
#else
			pline("�g�ɂ���%s�̉����ނ��y���Ȃ����B", xname(uarmc));
#endif /*JP*/
			break;
		}
		/* FALLTHRU */
	case POT_INVISIBILITY:
		if (Invis || Blind || BInvis) {
		    nothing++;
		} else {
		    self_invis_message();
		}
		if (otmp->blessed) HInvis |= FROMOUTSIDE;
		else incr_itimeout(&HInvis, rn1(15,31));
		newsym(u.ux,u.uy);	/* update position */
		if(otmp->cursed) {
		    pline(E_J("For some reason, you feel your presence is known.",
			      "�ǂ������킯���A���Ȃ��͎����̑��݂��m���Ă��邱�ƂɋC�Â����B"));
		    aggravate();
		}
		break;
	case POT_SEE_INVISIBLE:
		/* tastes like fruit juice in Rogue */
	case POT_FRUIT_JUICE:
	    {
		int msg = Invisible && !Blind;

		unkn++;
		if (otmp->cursed)
		    pline(E_J("Yecch!  This tastes %s.","�����A%s��������I"),
			  Hallucination ? E_J("overripe","�n�ꂷ����") : E_J("rotten","������"));
		else
#ifndef JP
		    pline(Hallucination ?
		      "This tastes like 10%% real %s%s all-natural beverage." :
				"This tastes like %s%s.",
			  otmp->odiluted ? "reconstituted " : "",
			  fruitname(TRUE));
#else
		    pline(Hallucination ?
		      "�����%s�V�R%s�ʏ`10%%���薳�Y�����R�����̖�������B" :
				"�����%s%s�̖�������B",
			  otmp->odiluted ? "�Z�k�Ҍ���" : "",
			  fruitname(!Hallucination));
#endif /*JP*/
		if (otmp->otyp == POT_FRUIT_JUICE) {
		    u.uhunger += (otmp->odiluted ? 5 : 10) * (2 + bcsign(otmp));
		    newuhs(FALSE);
		    break;
		}
		if (!otmp->cursed) {
			/* Tell them they can see again immediately, which
			 * will help them identify the potion...
			 */
			make_blinded(0L,TRUE);
		}
		if (otmp->blessed)
			incr_itimeout(&HSee_invisible, rn1(500,1000));
//			HSee_invisible |= FROMOUTSIDE;
		else
			incr_itimeout(&HSee_invisible, rn1(100,750));
		set_mimic_blocking(); /* do special mimic handling */
		see_monsters();	/* see invisible monsters */
		newsym(u.ux,u.uy); /* see yourself! */
		if (msg && !Blind) { /* Blind possible if polymorphed */
		    You(E_J("can see through yourself, but you are visible!",
			    "�����̓����ʂ����g�̂�F���ł���悤�ɂȂ����I"));
		    unkn--;
		}
		break;
	    }
	case POT_PARALYSIS:
		if (Free_action)
		    You(E_J("stiffen momentarily.","��u�����Ȃ��Ȃ����B"));
		else {
		    if (Levitation || Is_airlevel(&u.uz)||Is_waterlevel(&u.uz))
			You(E_J("are motionlessly suspended.",
				"���݂�̂܂ܐg�������Ƃ�Ȃ��Ȃ����B"));
#ifdef STEED
		    else if (u.usteed)
			You(E_J("are frozen in place!","���̏�œ����Ȃ��Ȃ����I"));
#endif
		    else
#ifndef JP
			Your("%s are frozen to the %s!",
			     makeplural(body_part(FOOT)), surface(u.ux, u.uy));
#else
			Your("%s��%s�ɓB�t���ɂȂ����I",
			     body_part(FOOT), surface(u.ux, u.uy));
#endif /*JP*/
		    nomul(-(rn1(10, 25 - 12*bcsign(otmp))));
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		}
		break;
	case POT_SLEEPING:
		if(Sleep_resistance || Free_action)
		    You(E_J("yawn.","�����т������B"));
		else {
		    You(E_J("suddenly fall asleep!","�ˑR����ɗ������I"));
		    fall_asleep(-rn1(10, 25 - 12*bcsign(otmp)), TRUE);
		}
		break;
	case POT_MONSTER_DETECTION:
	case SPE_DETECT_MONSTERS:
		if (otmp->blessed) {
		    int x, y;

		    if (Detect_monsters) nothing++;
		    unkn++;
		    /* after a while, repeated uses become less effective */
		    if (HDetect_monsters >= 300L)
			i = 1;
		    else
			i = rn1(40,21);
		    incr_itimeout(&HDetect_monsters, i);
		    for (x = 1; x < COLNO; x++) {
			for (y = 0; y < ROWNO; y++) {
			    if (levl[x][y].glyph == GLYPH_INVISIBLE) {
				unmap_object(x, y);
				newsym(x,y);
			    }
			    if (MON_AT(x,y)) unkn = 0;
			}
		    }
		    see_monsters();
		    if (unkn) You_feel(E_J("lonely.","�ǓƂɂȂ����B"));
		    break;
		}
		if (monster_detect(otmp, 0))
			return(1);		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_OBJECT_DETECTION:
	case SPE_DETECT_TREASURE:
		if (object_detect(otmp, 0))
			return(1);		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_SICKNESS:
		pline(E_J("Yecch!  This stuff tastes like poison.",
			  "�����I ���̉t�͓̂ł̂悤�Ȗ�������B"));
		if (otmp->blessed) {
		    pline(E_J("(But in fact it was mildly stale %s.)",
			      "(�������ۂ̂Ƃ���A����͏��������Ȃ���%s�W���[�X�������B)"),
			  fruitname(TRUE));
		    if (!Role_if(PM_HEALER)) {
			/* NB: blessed otmp->fromsink is not possible */
			losehp(1, E_J("mildly contaminated potion",
				      "�y���������ꂽ��������"), KILLED_BY_AN);
		    }
		} else {
		    if(Poison_resistance)
			pline(
			  E_J("(But in fact it was biologically contaminated %s.)",
			      "(�������ۂ̂Ƃ���A����͐����w�I�ɉ������ꂽ%s�W���[�X�������B)"),
			      fruitname(TRUE));
		    if (Role_if(PM_HEALER))
			pline(E_J("Fortunately, you have been immunized.",
				  "�K���A���Ȃ��͖Ɖu�������Ă����B"));
		    else {
			int typ = rn2(A_MAX);

			if (!Fixed_abil) {
			    poisontell(typ);
			    (void) adjattrib(typ,
			    		Poison_resistance ? -1 : -rn1(4,3),
			    		TRUE);
			}
			if(!Poison_resistance) {
			    if (otmp->fromsink)
				losehp(rnd(10)+5*!!(otmp->cursed),
				       E_J("contaminated tap water",
					   "�������ꂽ�������������"), KILLED_BY);
			    else
				losehp(rnd(10)+5*!!(otmp->cursed),
				       E_J("contaminated potion",
					   "�������ꂽ��������"), KILLED_BY_AN);
			}
			exercise(A_CON, FALSE);
		    }
		}
		if(Hallucination) {
			You(E_J("are shocked back to your senses!",
				"���C�Ɉ����߂��ꂽ�I"));
			(void) make_hallucinated(0L,FALSE,0L);
		}
		break;
	case POT_CONFUSION:
		if(!Confusion)
		    if (Hallucination) {
			pline(E_J("What a trippy feeling!",
				  "�Ȃ�Ă������ȋC�����I"));
			unkn++;
		    } else
			pline(E_J("Huh, What?  Where am I?",
				  "�������A���H �����͂ǂ��H"));
		else	nothing++;
		make_confused(itimeout_incr(HConfusion,
					    rn1(7, 16 - 8 * bcsign(otmp))),
			      FALSE);
		break;
	case POT_GAIN_ABILITY:
		if(otmp->cursed) {
		    pline(E_J("Ulch!  That potion tasted foul!",
			      "�������I ���̖�͂����萅�̂悤�Ȗ����I"));
		    unkn++;
		} else if (Fixed_abil) {
		    nothing++;
		} else {      /* If blessed, increase all; if not, try up to */
		    if (otmp->blessed) {
			for (i = 0; i < A_MAX; i++)
			    if (!otmp->odiluted || (rn2(100) < 50))
				adjattrib(i, 1, 0);
		    } else { /* 6 times to find one which can be increased. */
			for (i = ((otmp->odiluted) ? 1 : A_MAX); i > 0; i--) {
			    /* only give "your X is already as high as it can get"
			       message on last attempt (except blessed potions) */
			    if (adjattrib(rn2(A_MAX), 1, (i == 1) ? 0 : -1))
				break;
			}
		    }
		}
		break;
	case POT_SPEED:
		if(otmp->cursed) {
		    if(!Wounded_legs && !Rocketskate
#ifdef STEED
			&& !u.usteed
#endif
			) {
			boolean legs = !(cantweararm(youmonst.data) || slithy(youmonst.data));
#ifndef JP
			Your("%s suddenly start%s rushing beyond your control!",
				legs ? makeplural(body_part(LEG)) : "body",
				legs ? "" : "s");
#else
			Your("%s������ł��Ȃ��قǑ��������͂��߂��I",
				legs ? body_part(LEG) : "�g��");
#endif /*JP*/
			u.uspdbon2 = -1;	/* incredible speed & fumbling */
			incr_itimeout(&HFast, rn1(30, 30));
			HFumbling |= FROMOUTSIDE;
			if (!(HFumbling & TIMEOUT))
				incr_itimeout(&HFumbling, rnd(20));
			break;
		    } else {
#ifndef JP
			Your("%s tremble.", makeplural(body_part(LEG)));
#else
			Your("%s�͐k�����B", body_part(LEG));
#endif /*JP*/
		    }
		    break;
		}
		if(Wounded_legs && !otmp->cursed
#ifdef STEED
		   && !u.usteed	/* heal_legs() would heal steeds legs */
#endif
						) {
			heal_legs();
			unkn++;
			break;
		} /* and fall through */
	case SPE_HASTE_SELF:
		/* already temporary fast */
		if(Very_fast) {
#ifndef JP
			Your("%s get new energy.",
				makeplural(body_part(LEG)));
#else
			Your("%s�͐V���Ȋ��͂𓾂��B", body_part(LEG));
#endif /*JP*/
			unkn++;
			break;
		}
		You(E_J("are suddenly moving %sfaster.","�ˑR%s�f����������悤�ɂȂ����B"),
			(Fast || BFast) ? "" : E_J("much ","�ƂĂ�"));
		exercise(A_DEX, TRUE);
		u.uspdbon2 = 2+bcsign(otmp)*2;
		incr_itimeout(&HFast, rn1(10, 50 + 150 * bcsign(otmp)));
		break;
	case POT_BLINDNESS:
		if(Blind) nothing++;
		make_blinded(itimeout_incr(Blinded,
					   rn1(200, 250 - 125 * bcsign(otmp))),
			     (boolean)!Blind);
		break;
	case POT_GAIN_LEVEL:
		if (otmp->cursed) {
			unkn++;
			/* they went up a level */
			if((ledger_no(&u.uz) == 1 && u.uhave.amulet) ||
				Can_rise_up(u.ux, u.uy, &u.uz)) {
			    const char *riseup =E_J("rise up, through the %s!",
						    "�����яオ��A%s��ʂ蔲�����I");
			    if(ledger_no(&u.uz) == 1) {
			        You(riseup, ceiling(u.ux,u.uy));
				goto_level(&earth_level, FALSE, FALSE, FALSE);
			    } else {
			        register int newlev = depth(&u.uz)-1;
				d_level newlevel;

				get_level(&newlevel, newlev);
				if(on_level(&newlevel, &u.uz)) {
				    pline(E_J("It tasted bad.","����͕s�����B"));
				    break;
				} else You(riseup, ceiling(u.ux,u.uy));
				goto_level(&newlevel, FALSE, FALSE, FALSE);
			    }
			}
			else You(E_J("have an uneasy feeling.",
				     "���������Ȃ��C���ɂ�����ꂽ�B"));
			break;
		}
		pluslvl(FALSE);
		if (otmp->blessed && !otmp->odiluted)
			/* blessed potions place you at a random spot in the
			 * middle of the new level instead of the low point
			 */
			u.uexp = rndexp(TRUE);
		break;
	case POT_HEALING:
		You_feel(E_J("better.","���C�ɂȂ����B"));
		i = bcsign(otmp);
		if ((i >= 0) && otmp->odiluted) i--;
		healup(d(6 + 2 * i, 6/*4*/),
		       (i >= 0) ? 1 : 0, (i > 0), (i >= 0));
		exercise(A_CON, TRUE);
		break;
	case POT_EXTRA_HEALING:
		You_feel(E_J("much better.","�ƂĂ����C�ɂȂ����B"));
		i = bcsign(otmp);
		if ((i >= 0) && otmp->odiluted) i--;
		healup(d(6 + 2 * i, 12/*8*/),
		       (i > 0) ? 5 : (i == 0) ? 2 : 0,
		       (i >= 0), TRUE);
		(void) make_hallucinated(0L,TRUE,0L);
		exercise(A_CON, TRUE);
		exercise(A_STR, TRUE);
		break;
	case POT_FULL_HEALING:
		You_feel(E_J("completely healed.","���S�ɉ񕜂����B"));
		i = bcsign(otmp);
		if ((i >= 0) && otmp->odiluted) i--;
		healup(400, 4+4*i, (i >= 0), TRUE);
		/* Restore one lost level if blessed */
		if ((i > 0) && u.ulevel < u.ulevelmax) {
		    /* when multiple levels have been lost, drinking
		       multiple potions will only get half of them back */
		    pluslvl(FALSE);
		    u.ulevelmax -= 1;
		}
		(void) make_hallucinated(0L,TRUE,0L);
		exercise(A_STR, TRUE);
		exercise(A_CON, TRUE);
		break;
	case POT_LEVITATION:
	case SPE_LEVITATION:
		if (otmp->cursed) HLevitation &= ~I_SPECIAL;
		if(!Levitation) {
			/* kludge to ensure proper operation of float_up() */
			HLevitation = 1;
			float_up();
			/* reverse kludge */
			HLevitation = 0;
			if (otmp->cursed && !Is_waterlevel(&u.uz)) {
	if((u.ux != xupstair || u.uy != yupstair)
	   && (u.ux != sstairs.sx || u.uy != sstairs.sy || !sstairs.up)
	   && (!xupladder || u.ux != xupladder || u.uy != yupladder)
	) {
					You(E_J("hit your %s on the %s.",
						"%s��%s�ɂԂ����B"),
						body_part(HEAD),
						ceiling(u.ux,u.uy));
					losehp(uarmh ? 1 : rnd(10),
						E_J("colliding with the ceiling",
						    "�V��ɏՓ˂���"),
						KILLED_BY);
				} else (void) doup();
			}
		} else
			nothing++;
		if (otmp->blessed) {
		    incr_itimeout(&HLevitation, rn1(50,250));
		    HLevitation |= I_SPECIAL;
		} else incr_itimeout(&HLevitation, rn1(140,10));
		spoteffects(FALSE);	/* for sinks */
		break;
	case POT_GAIN_ENERGY:			/* M. Stephenson */
		{	register int num;
			if(otmp->cursed)
			    You_feel(E_J("lackluster.","���C���������B"));
			else
			    pline(E_J("Magical energies course through your body.",
				      "���@�̗͂����Ȃ��̐g�̂������߂������B"));
			num = rnd(5) + 5 * otmp->blessed + 1;
			if (otmp->odiluted) num = rnd(num);
			u.uenmax += (otmp->cursed) ? -num : num;
			u.uen += (otmp->cursed) ? -num : num;
			if(u.uenmax <= 0) u.uenmax = 0;
			if(u.uen <= 0) u.uen = 0;
			flags.botl = 1;
			exercise(A_WIS, TRUE);
		}
		break;
	case POT_OIL:				/* P. Winner */
		{
			boolean good_for_you = FALSE;

			if (otmp->lamplit) {
			    if (likes_fire(youmonst.data)) {
				pline(E_J("Ahh, a refreshing drink.",
					  "�����A���̈�t�͐����Ԃ�B"));
				good_for_you = TRUE;
			    } else {
				You(E_J("burn your %s.","%s�ɉΏ��𕉂����B"), body_part(FACE));
				losehp(d(Fire_resistance ? 1 : 3, 4),
				       E_J("burning potion of oil","�Ή��r�������"), KILLED_BY_AN);
			    }
			} else if(otmp->cursed)
			    pline(E_J("This tastes like castor oil.",
				      "����̓q�}�V���̂悤�Ȗ�������B"));
			else
			    pline(E_J("That was smooth!","�������肪�悢�I"));
			exercise(A_WIS, good_for_you);
		}
		break;
	case POT_ACID:
		if (Acid_resistance)
			/* Not necessarily a creature who _likes_ acid */
#ifndef JP
			pline("This tastes %s.", Hallucination ? "tangy" : "sour");
#else
			pline("�����%s��������B", Hallucination ? "������1000����" : "�_���ς�");
#endif /*JP*/
		else {
#ifndef JP
			pline("This burns%s!", otmp->blessed ? " a little" :
					otmp->cursed ? " a lot" : " like acid");
			losehp(d(otmp->cursed ? 2 : 1, otmp->blessed ? 4 : 8),
					"potion of acid", KILLED_BY_AN);
#else
			pline("�A��%s�I", otmp->blessed ? "�܂���悤��" :
					otmp->cursed ? "�܂���" : "�_�Ɏ܂����悤��");
			losehp(d(otmp->cursed ? 2 : 1, otmp->blessed ? 4 : 8),
					E_J("potion of acid","�_�̖�������"), KILLED_BY_AN);
#endif /*JP*/
			exercise(A_CON, FALSE);
		}
		if (Stoned) fix_petrification();
		unkn++; /* holy/unholy water can burn like acid too */
		break;
	case POT_POLYMORPH:
#ifndef JP
		You_feel("a little %s.", Hallucination ? "normal" : "strange");
#else
		You_feel("����%s���o�ɂ�����ꂽ�B", Hallucination ? "���ʂ�" : "�s�v�c��");
#endif /*JP*/
		if (!Unchanging) polyself(FALSE);
		break;
	default:
		impossible("What a funny potion! (%u)", otmp->otyp);
		return(0);
	}
	return(-1);
}

void
healup(nhp, nxtra, curesick, cureblind)
	int nhp, nxtra;
	register boolean curesick, cureblind;
{
	if (nhp) {
		if (Upolyd) {
			u.mh += nhp;
			if (u.mh > u.mhmax) u.mh = (u.mhmax += nxtra);
		} else {
			u.uhp += nhp;
			if(u.uhp > u.uhpmax) addhpmax(nxtra);
		}
	}
	if(cureblind)	make_blinded(0L,TRUE);
	if(curesick)	make_sick(0L, (char *) 0, TRUE, SICK_ALL);
	flags.botl = 1;
	return;
}

void
strange_feeling(obj,txt)
register struct obj *obj;
register const char *txt;
{
	if (flags.beginner || !txt)
#ifndef JP
		You("have a %s feeling for a moment, then it passes.",
		Hallucination ? "normal" : "strange");
#else
		You("%s���o�ɂ�����ꂽ���A����͂����ɉ߂��������B",
		Hallucination ? "�Ȃ�Ă��Ƃ̂Ȃ�" : "�s�v�c��");
#endif /*JP*/
	else
		pline(txt);

	if(!obj)	/* e.g., crystal ball finds no traps */
		return;

	if(obj->dknown && !objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	useup(obj);
}

const char *bottlenames[] = {
#ifndef JP
	"bottle", "phial", "flagon", "carafe", "flask", "jar", "vial"
#else
	"�r", "���", "������", "������", "�t���X�R", "�K���X�e��", "���r��"
#endif /*JP*/
};


const char *
bottlename()
{
	return bottlenames[rn2(SIZE(bottlenames))];
}

void
potionhit(mon, obj, your_fault)
register struct monst *mon;
register struct obj *obj;
boolean your_fault;
{
	register const char *botlnam = bottlename();
	boolean isyou = (mon == &youmonst);
	int distance;

	if(isyou) {
		distance = 0;
#ifndef JP
		pline_The("%s crashes on your %s and breaks into shards.",
			botlnam, body_part(HEAD));
		losehp(rnd(2), "thrown potion", KILLED_BY_AN);
#else
		pline_The("%s�����Ȃ���%s�ɖ������A���X�ɍӂ����B",
			botlnam, body_part(HEAD));
		losehp(rnd(2), "������ꂽ��ɓ�������", KILLED_BY_AN);
#endif /*JP*/
	} else {
		distance = distu(mon->mx,mon->my);
		if (!cansee(mon->mx,mon->my)) pline(E_J("Crash!","�K�`�����I"));
		else {
		    char *mnam = mon_nam(mon);
		    char buf[BUFSZ];

		    if(has_head(mon->data)) {
#ifndef JP
			Sprintf(buf, "%s %s",
				s_suffix(mnam),
				(notonhead ? "body" : "head"));
#else
			Sprintf(buf, "%s��%s", mnam,
				(notonhead ? "�g��" : "��"));
#endif /*JP*/
		    } else {
			Strcpy(buf, mnam);
		    }
		    pline_The(E_J("%s crashes on %s and breaks into shards.",
				  "%s��%s�ɖ������A���X�ɍӂ����B"),
			   botlnam, buf);
		}
		if(rn2(5) && mon->mhp > 1)
			mon->mhp--;
	}

	/* oil doesn't instantly evaporate */
	if (obj->otyp != POT_OIL && cansee(mon->mx,mon->my))
#ifndef JP
		pline("%s.", Tobjnam(obj, "evaporate"));
#else
		pline("%s�͊��������B", xname(obj));
#endif /*JP*/

    if (isyou) {
	switch (obj->otyp) {
	case POT_OIL:
		if (obj->lamplit)
		    splatter_burning_oil(u.ux, u.uy);
		break;
	case POT_POLYMORPH:
#ifndef JP
		You_feel("a little %s.", Hallucination ? "normal" : "strange");
#else
		You("����%s�ȋC���ɂȂ����B", Hallucination ? "����" : "�s�v�c");
#endif /*JP*/
		if (!Unchanging && !Antimagic) polyself(FALSE);
		else if (Antimagic) damage_resistant_obj(ANTIMAGIC, 1);
		break;
	case POT_ACID:
		if (!Acid_resistance) {
#ifndef JP
		    pline("This burns%s!", obj->blessed ? " a little" :
				    obj->cursed ? " a lot" : "");
#else
		    pline("�g�̂�%s�܂����I", obj->blessed ? "����" :
				    obj->cursed ? "�Ђǂ�" : "");
#endif /*JP*/
		    losehp(d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8),
				E_J("potion of acid","�_�̖�𗁂т�"), KILLED_BY_AN);
		}
		break;
	}
    } else {
	boolean angermon = TRUE;

	if (!your_fault) angermon = FALSE;
	switch (obj->otyp) {
	case POT_HEALING:
	case POT_EXTRA_HEALING:
	case POT_FULL_HEALING:
		if (mon->data == &mons[PM_PESTILENCE]) goto do_illness;
		/*FALLTHRU*/
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
 do_healing:
		angermon = FALSE;
		if(mon->mhp < mon->mhpmax) {
		    mon->mhp = mon->mhpmax;
		    if (canseemon(mon))
			pline(E_J("%s looks sound and hale again.",
				  "%s�͍Ăь��C�Ɗ��͂����߂����悤���B"), Monnam(mon));
		}
		break;
	case POT_SICKNESS:
		if (mon->data == &mons[PM_PESTILENCE]) goto do_healing;
		if (dmgtype(mon->data, AD_DISE) ||
			   dmgtype(mon->data, AD_PEST) || /* won't happen, see prior goto */
			   resists_poison(mon)) {
		    if (canseemon(mon))
			pline(E_J("%s looks unharmed.",
				  "%s�͏����Ȃ��悤���B"), Monnam(mon));
		    break;
		}
 do_illness:
		if((mon->mhpmax > 3) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhpmax /= 2;
		if((mon->mhp > 2) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhp /= 2;
		if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
		if (canseemon(mon))
		    pline(E_J("%s looks rather ill.",
			      "%s�͂��Ȃ�C���������Ȃ����悤���B"), Monnam(mon));
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!resist(mon, POTION_CLASS, 0, NOTELL))  mon->mconf = TRUE;
		break;
	case POT_INVISIBILITY:
		angermon = FALSE;
		mon_set_minvis(mon);
		break;
	case POT_SLEEPING:
		/* wakeup() doesn't rouse victims of temporary sleep */
		if (sleep_monst(mon, rnd(12), POTION_CLASS)) {
		    pline(E_J("%s falls asleep.","%s�͖����Ă��܂����B"), Monnam(mon));
		    slept_monst(mon);
		}
		break;
	case POT_PARALYSIS:
		if (mon->mcanmove) {
			mon->mcanmove = 0;
			/* really should be rnd(5) for consistency with players
			 * breathing potions, but...
			 */
			mon->mfrozen = rnd(25);
		}
		break;
	case POT_SPEED:
		angermon = FALSE;
		mon_adjust_speed(mon, 1, obj);
		break;
	case POT_BLINDNESS:
		if(haseyes(mon->data)) {
		    register int btmp = 64 + rn2(32) +
			rn2(32) * !resist(mon, POTION_CLASS, 0, NOTELL);
		    btmp += mon->mblinded;
		    mon->mblinded = min(btmp,127);
		    mon->mcansee = 0;
		}
		break;
	case POT_WATER:
		if (is_undead(mon->data) || is_demon(mon->data) ||
			is_were(mon->data)) {
		    if (obj->blessed) {
#ifndef JP
			pline("%s %s in pain!", Monnam(mon),
			      is_silent(mon->data) ? "writhes" : "shrieks");
#else
			pline("%s�͋��%s���I", Monnam(mon),
			      is_silent(mon->data) ? "�ɐg���悶�点" : "�̋��т�����");
#endif /*JP*/
			mon->mhp -= d(2,6);
			/* should only be by you */
			if (mon->mhp < 1) killed(mon);
			else if (is_were(mon->data) && !is_human(mon->data))
			    new_were(mon);	/* revert to human */
		    } else if (obj->cursed) {
			angermon = FALSE;
			if (canseemon(mon))
			    pline(E_J("%s looks healthier.",
				      "%s�͂�茒�N�ɂȂ����悤���B"), Monnam(mon));
			mon->mhp += d(2,6);
			if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
			if (is_were(mon->data) && is_human(mon->data) &&
				!Protection_from_shape_changers)
			    new_were(mon);	/* transform into beast */
		    }
		} else if(mon->data == &mons[PM_GREMLIN]) {
		    angermon = FALSE;
		    (void)split_mon(mon, (struct monst *)0);
		} else if(mon->data == &mons[PM_IRON_GOLEM]) {
		    if (canseemon(mon))
			pline(E_J("%s rusts.","%s�͎K�т��B"), Monnam(mon));
		    mon->mhp -= d(1,6);
		    /* should only be by you */
		    if (mon->mhp < 1) killed(mon);
		}
		break;
	case POT_OIL:
		if (obj->lamplit)
			splatter_burning_oil(mon->mx, mon->my);
		break;
	case POT_ACID:
		if (!resists_acid(mon) && !resist(mon, POTION_CLASS, 0, NOTELL)) {
#ifndef JP
		    pline("%s %s in pain!", Monnam(mon),
			  is_silent(mon->data) ? "writhes" : "shrieks");
#else
		    pline("%s�͋��%s���I", Monnam(mon),
			  is_silent(mon->data) ? "�ɐg���悶�点" : "�̋��т�����");
#endif /*JP*/
		    mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
		    }
		}
		break;
	case POT_POLYMORPH:
		(void) bhitm(mon, obj);
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
		break;
*/
	}
	if (angermon)
	    wakeup(mon);
	else
	    mon->msleeping = 0;
    }

	/* Note: potionbreathe() does its own docall() */
	if ((distance==0 || ((distance < 3) && rn2(5))) &&
	    (!breathless(youmonst.data) || haseyes(youmonst.data)))
		potionbreathe(obj);
	else if (obj->dknown && !objects[obj->otyp].oc_name_known &&
		   !objects[obj->otyp].oc_uname && cansee(mon->mx,mon->my))
		docall(obj);
	if(*u.ushops && obj->unpaid) {
	        register struct monst *shkp =
			shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

		if(!shkp)
		    obj->unpaid = 0;
		else {
		    (void)stolen_value(obj, u.ux, u.uy,
				 (boolean)shkp->mpeaceful, FALSE);
		    subfrombill(obj, shkp);
		}
	}
	obfree(obj, (struct obj *)0);
}

/* vapors are inhaled or get in your eyes */
void
potionbreathe(obj)
register struct obj *obj;
{
	register int i, ii, isdone, kn = 0;

	switch(obj->otyp) {
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
		if(obj->cursed) {
		    if (!breathless(youmonst.data))
			pline(E_J("Ulch!  That potion smells terrible!",
				  "�������I ���̖�͂Ђǂ��L��������I"));
		    else if (haseyes(youmonst.data)) {
			int numeyes = eyecount(youmonst.data);
#ifndef JP
			Your("%s sting%s!",
			     (numeyes == 1) ? body_part(EYE) : makeplural(body_part(EYE)),
			     (numeyes == 1) ? "s" : "");
#else
			pline("%s���h���悤�ɒɂ��I", body_part(EYE));
#endif /*JP*/
		    }
		    break;
		} else {
		    i = rn2(A_MAX);		/* start at a random point */
		    for(isdone = ii = 0; !isdone && ii < A_MAX; ii++) {
			if(ABASE(i) < AMAX(i)) {
			    ABASE(i)++;
			    /* only first found if not blessed */
			    isdone = !(obj->blessed);
			    flags.botl = 1;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_FULL_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		/*FALL THROUGH*/
	case POT_EXTRA_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		/*FALL THROUGH*/
	case POT_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		exercise(A_CON, TRUE);
		break;
	case POT_SICKNESS:
		if (!Role_if(PM_HEALER)) {
			if (Upolyd) {
			    if (u.mh <= 5) u.mh = 1; else u.mh -= 5;
			} else {
			    if (u.uhp <= 5) u.uhp = 1; else u.uhp -= 5;
			}
			flags.botl = 1;
			exercise(A_CON, FALSE);
		}
		break;
	case POT_HALLUCINATION:
		You(E_J("have a momentary vision.","��u���������B"));
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!Confusion)
			You_feel(E_J("somewhat dizzy.","�����߂܂��������B"));
		make_confused(itimeout_incr(HConfusion, rnd(5)), FALSE);
		break;
	case POT_INVISIBILITY:
		if (!Blind && !Invis) {
		    kn++;
#ifndef JP
		    pline("For an instant you %s!",
			See_invisible ? "could see right through yourself"
			: "couldn't see yourself");
#else
		    pline("�ق�̂ЂƎ��A������%s�I",
			See_invisible ? "�g�̂������Č�����"
			: "�p�������Ȃ��Ȃ���");
#endif /*JP*/
		}
		break;
	case POT_PARALYSIS:
		kn++;
		if (!Free_action) {
		    pline(E_J("%s seems to be holding you.",
			      "%s�����Ȃ���߂炦�Ă���悤���B"), Something);
		    nomul(-rnd(5));
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		} else You(E_J("stiffen momentarily.","��u���������Ȃ��Ȃ����B"));
		break;
	case POT_SLEEPING:
		kn++;
		if (!Free_action && !Sleep_resistance) {
		    You_feel(E_J("rather tired.","��J���ނ����B"));
		    nomul(-rnd(5));
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		} else You(E_J("yawn.","�����т������B"));
		break;
	case POT_SPEED:
		if (!Fast) Your(E_J("knees seem more flexible now.",
				    "�G�͂��_��ɂȂ����悤���B"));
		incr_itimeout(&HFast, rnd(5));
		exercise(A_DEX, TRUE);
		break;
	case POT_BLINDNESS:
		if (!Blind && !u.usleep) {
		    kn++;
		    pline(E_J("It suddenly gets dark.","�����肪�ˑR�^���ÂɂȂ����B"));
		}
		make_blinded(itimeout_incr(Blinded, rnd(5)), FALSE);
		if (!Blind && !u.usleep) Your(vision_clears);
		break;
	case POT_WATER:
		if(u.umonnum == PM_GREMLIN) {
		    (void)split_mon(&youmonst, (struct monst *)0);
		} else if (u.ulycn >= LOW_PM) {
		    /* vapor from [un]holy water will trigger
		       transformation but won't cure lycanthropy */
		    if (obj->blessed && youmonst.data == &mons[u.ulycn])
			you_unwere(FALSE);
		    else if (obj->cursed && !Upolyd)
			you_were();
		}
		break;
	case POT_ACID:
	case POT_POLYMORPH:
		exercise(A_CON, FALSE);
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
	case POT_OIL:
		break;
*/
	}
	/* note: no obfree() */
	if (obj->dknown) {
	    if (kn)
		makeknown(obj->otyp);
	    else if (!objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	}
}

STATIC_OVL short
mixtype(o1, o2)
register struct obj *o1, *o2;
/* returns the potion type when o1 is dipped in o2 */
{
	/* cut down on the number of cases below */
	if (o1->oclass == POTION_CLASS &&
	    (o2->otyp == POT_GAIN_LEVEL ||
	     o2->otyp == POT_GAIN_ENERGY ||
	     o2->otyp == POT_HEALING ||
	     o2->otyp == POT_EXTRA_HEALING ||
	     o2->otyp == POT_FULL_HEALING ||
	     o2->otyp == POT_ENLIGHTENMENT ||
	     o2->otyp == POT_FRUIT_JUICE)) {
		struct obj *swp;

		swp = o1; o1 = o2; o2 = swp;
	}

	switch (o1->otyp) {
		case POT_HEALING:
			switch (o2->otyp) {
			    case POT_SPEED:
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_EXTRA_HEALING;
			}
		case POT_EXTRA_HEALING:
			switch (o2->otyp) {
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_FULL_HEALING;
			}
		case POT_FULL_HEALING:
			switch (o2->otyp) {
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_GAIN_ABILITY;
			}
		case UNICORN_HORN:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_FRUIT_JUICE;
			    case POT_HALLUCINATION:
			    case POT_BLINDNESS:
			    case POT_CONFUSION:
				return POT_WATER;
			}
			break;
		case AMETHYST:		/* "a-methyst" == "not intoxicated" */
			if (o2->otyp == POT_BOOZE)
			    return POT_FRUIT_JUICE;
			break;
		case POT_GAIN_LEVEL:
		case POT_GAIN_ENERGY:
			switch (o2->otyp) {
			    case POT_CONFUSION:
				return (rn2(3) ? POT_BOOZE : POT_ENLIGHTENMENT);
			    case POT_HEALING:
				return POT_EXTRA_HEALING;
			    case POT_EXTRA_HEALING:
				return POT_FULL_HEALING;
			    case POT_FULL_HEALING:
				return POT_GAIN_ABILITY;
			    case POT_FRUIT_JUICE:
				return POT_SEE_INVISIBLE;
			    case POT_BOOZE:
				return POT_HALLUCINATION;
			}
			break;
		case POT_FRUIT_JUICE:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_SICKNESS;
			    case POT_SPEED:
				return POT_BOOZE;
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_SEE_INVISIBLE;
			}
			break;
		case POT_ENLIGHTENMENT:
			switch (o2->otyp) {
			    case POT_LEVITATION:
				if (rn2(3)) return POT_GAIN_LEVEL;
				break;
			    case POT_FRUIT_JUICE:
				return POT_BOOZE;
			    case POT_BOOZE:
				return POT_CONFUSION;
			}
			break;
	}

	return 0;
}


int
get_wet(obj)
register struct obj *obj;
/* returns >0 if something happened (potion should be used up) */
/* Bit0 means something happened, Bit1 means obj should be used up */
{
	char Your_buf[BUFSZ];

	if (snuff_lit(obj)) return(1);

	if (obj->greased) {
		grease_protect(obj,(char *)0,&youmonst);
		return(0);
	}
	(void) Shk_Your(Your_buf, obj);
	/* (Rusting shop goods ought to be charged for.) */
	switch (obj->oclass) {
	    case POTION_CLASS:
		if (obj->otyp == POT_WATER) return 0;
		/* KMH -- Water into acid causes an explosion */
		if (obj->otyp == POT_ACID) {
			pline(E_J("It boils vigorously!","�˕����N�����I"));
			You(E_J("are caught in the explosion!","�����Ɋ������܂ꂽ�I"));
			losehp(d(obj->quan, 10), E_J("elementary chemistry","�������w������"), KILLED_BY);
			makeknown(obj->otyp);
			update_inventory();
			return (3);
		}
#ifndef JP
		pline("%s %s%s.", Your_buf, aobjnam(obj,"dilute"),
		      obj->odiluted ? " further" : "");
#else
		pline("%s%s��%s���܂����B", Your_buf, xname(obj),
		      obj->odiluted ? "�����" : "");
#endif /*JP*/
		if(obj->unpaid && costly_spot(u.ux, u.uy)) {
		    You(E_J("dilute it, you pay for it.",
			    "��𔖂߂��̂ŁA�ُ����Ȃ���΂Ȃ�Ȃ��B"));
		    bill_dummy_object(obj);
		}
		if (obj->odiluted) {
			obj->odiluted = 0;
#ifdef UNIXPC
			obj->blessed = FALSE;
			obj->cursed = FALSE;
#else
			obj->blessed = obj->cursed = FALSE;
#endif
			obj->otyp = POT_WATER;
		} else obj->odiluted++;
		update_inventory();
		return 1;
	    case SCROLL_CLASS:
		if (obj->otyp != SCR_BLANK_PAPER
#ifdef MAIL
		    && obj->otyp != SCR_MAIL
#endif
		    ) {
			if (!Blind) {
#ifndef JP
				boolean oq1 = obj->quan == 1L;
				pline_The("scroll%s %s.",
					  oq1 ? "" : "s", otense(obj, "fade"));
#else
				pline("�����̕����͂ɂ���ŏ������B");
#endif /*JP*/
			}
			if(obj->unpaid && costly_spot(u.ux, u.uy)) {
			    You(E_J("erase it, you pay for it.",
				    "�����𔒎��ɂ����̂ŁA�ُ����Ȃ���΂Ȃ�Ȃ��B"));
			    bill_dummy_object(obj);
			}
			obj->otyp = SCR_BLANK_PAPER;
			obj->spe = 0;
			update_inventory();
			return 1;
		} else break;
	    case SPBOOK_CLASS:
		if (obj->otyp != SPE_BLANK_PAPER) {

			if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
#ifndef JP
	pline("%s suddenly heats up; steam rises and it remains dry.",
				The(xname(obj)));
#else
	pline("%s�͓ˑR���M���A���C�𐁂��グ�A�S���G��Ȃ������B",
				xname(obj));
#endif /*JP*/
			} else {
			    if (!Blind) {
#ifndef JP
				    boolean oq1 = obj->quan == 1L;
				    pline_The("spellbook%s %s.",
					oq1 ? "" : "s", otense(obj, "fade"));
#else
				    pline("���@���̕����͂ɂ���ŏ������B");
#endif /*JP*/
			    }
			    if(obj->unpaid && costly_spot(u.ux, u.uy)) {
			        You(E_J("erase it, you pay for it.",
					"�{�𔒎��ɂ����̂ŁA�ُ����Ȃ���΂Ȃ�Ȃ��B"));
			        bill_dummy_object(obj);
			    }
			    obj->otyp = SPE_BLANK_PAPER;
			    update_inventory();
			}
			return 1;
		}
		break;
//	    case WEAPON_CLASS:
	    /* Just "fall through" to generic rustprone check for now. */
	    /* fall through */
	    default:
		if (!obj->oerodeproof && is_rustprone(obj) && !rn2(2)) {
			if (obj->oeroded < MAX_ERODE) {
#ifndef JP
			    pline("%s %s some%s.",
				  Your_buf, aobjnam(obj, "rust"),
				  obj->oeroded ? " more" : "what");
#else
			    pline("%s%s��%s�K�т��B",
				  Your_buf, xname(obj),
				  obj->oeroded ? "�����" : "�����炩");
#endif /*JP*/
			    obj->oeroded++;
			    update_inventory();
			    return 1;
			} else if (!rn2(3)) {
#ifndef JP
			    pline("%s %s apart!",
				  Your_buf, aobjnam(obj, "break"));
#else
			    pline("%s%s�͍ӂ��U�����I",
				  Your_buf, xname(obj));
#endif /*JP*/
			    if (obj == uball) unpunish();
			    return 3;
			} else break;
		} else break;
	}
#ifndef JP
	pline("%s %s wet.", Your_buf, aobjnam(obj,"get"));
#else
	pline("%s%s�͔G�ꂽ�B", Your_buf, xname(obj));
#endif /*JP*/
	return 0;
}

int
dodip()
{
	register struct obj *potion, *obj;
	struct obj *singlepotion;
	const char *tmp;
	uchar here;
	char allowall[2];
	short mixture;
	char qbuf[QBUFSZ], Your_buf[BUFSZ];
#ifdef JP
	static const struct getobj_words dipw1 = { 0, 0, "�Z��", "�Z��" };
	static const struct getobj_words dipw2 = { 0, "��", "�Z��", "�Z��" };
#endif /*JP*/

	allowall[0] = ALL_CLASSES; allowall[1] = '\0';
	if(!(obj = getobj(allowall, E_J("dip",&dipw1), 0)))
		return(0);

	here = levl[u.ux][u.uy].typ;
	/* Is there a fountain to dip into here? */
	if (IS_FOUNTAIN(here)) {
		if(yn(E_J("Dip it into the fountain?",
			  "��ɐZ���܂����H")) == 'y') {
			dipfountain(obj);
			return(1);
		}
	} else if (is_pool(u.ux,u.uy) || is_swamp(u.ux,u.uy)) {
		tmp = waterbody_name(u.ux,u.uy);
		Sprintf(qbuf, E_J("Dip it into the %s?","%s�ɐZ���܂����H"), tmp);
		if (yn(qbuf) == 'y') {
		    if (Levitation) {
			floating_above(tmp);
#ifdef STEED
		    } else if (u.usteed && !is_swimmer(u.usteed->data) &&
			    P_SKILL(P_RIDING) < P_BASIC) {
			rider_cant_reach(); /* not skilled enough to reach */
#endif
		    } else {
			if (get_wet(obj) & 0x02) useupall(obj);
		    }
		    return 1;
		}
	}

	if(!(potion = getobj(beverages, E_J("dip into",&dipw2), 0)))
		return(0);
	if (potion == obj && potion->quan == 1L) {
		pline(E_J("That is a potion bottle, not a Klein bottle!",
			  "����͖�̃r���ł����āA�N���C���̚�ł͂Ȃ��I"));
		return 0;
	}
	potion->in_use = TRUE;		/* assume it will be used up */
	if(potion->otyp == POT_WATER) {
		boolean useeit = !Blind;
		if (useeit) (void) Shk_Your(Your_buf, obj);
		if (potion->blessed) {
			if (obj->cursed) {
				if (useeit)
#ifndef JP
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  hcolor(NH_AMBER));
#else
				    pline("%s%s��%s%s�P�����B",
					  Your_buf, xname(obj),
					  Hallucination ? "" : "�_�炩��",
					  j_no_ni((char *)hcolor(NH_AMBER)));
#endif /*JP*/
				uncurse(obj);
				obj->bknown=1;
	poof:
				if(!(objects[potion->otyp].oc_name_known) &&
				   !(objects[potion->otyp].oc_uname))
					docall(potion);
				useup(potion);
				return(1);
			} else if(!obj->blessed) {
				if (useeit) {
#ifndef JP
				    tmp = hcolor(NH_LIGHT_BLUE);
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  index(vowels, *tmp) ? "n" : "", tmp);
#else
				    pline("%s%s��%s%s�_�炩�Ȍ�䊂ɂ܂ꂽ�B",
					  Your_buf, xname(obj),
					  hcolor(NH_LIGHT_BLUE));
#endif /*JP*/
				}
				bless(obj);
				obj->bknown=1;
				goto poof;
			}
		} else if (potion->cursed) {
			if (obj->blessed) {
				if (useeit)
#ifndef JP
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  hcolor((const char *)"brown"));
#else
				    pline("%s%s��%s�P�����B",
					  Your_buf, xname(obj),
					  j_no_ni(hcolor((const char *)"�ݐF��")));
#endif /*JP*/
				unbless(obj);
				obj->bknown=1;
				goto poof;
			} else if(!obj->cursed) {
				if (useeit) {
#ifndef JP
				    tmp = hcolor(NH_BLACK);
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  index(vowels, *tmp) ? "n" : "", tmp);
#else
				    pline("%s%s��%s�Ȃ�ɂ܂ꂽ�B",
					  Your_buf, xname(obj), hcolor(NH_BLACK));
#endif /*JP*/
				}
				curse(obj);
				obj->bknown=1;
				goto poof;
			}
		} else {
			int w;
			w = get_wet(obj);
			if (w & 0x02) useupall(obj);
			if (w & 0x01) goto poof;
		}
	} else if (obj->otyp == POT_POLYMORPH ||
		potion->otyp == POT_POLYMORPH) {
	    /* some objects can't be polymorphed */
	    if (obj->otyp == potion->otyp ||	/* both POT_POLY */
		    obj->otyp == WAN_POLYMORPH ||
		    obj->otyp == SPE_POLYMORPH ||
		    obj == uball || obj == uskin ||
		    obj_resists(obj->otyp == POT_POLYMORPH ?
				potion : obj, 5, 95)) {
		pline(nothing_happens);
	    } else {
	    	boolean was_wep = FALSE, was_swapwep = FALSE, was_quiver = FALSE;
		short save_otyp = obj->otyp;
		/* KMH, conduct */
		u.uconduct.polypiles++;

		if (obj == uwep) was_wep = TRUE;
		else if (obj == uswapwep) was_swapwep = TRUE;
		else if (obj == uquiver) was_quiver = TRUE;

		obj = poly_obj(obj, STRANGE_OBJECT);

		if (was_wep) setuwep(obj);
		else if (was_swapwep) setuswapwep(obj);
		else if (was_quiver) setuqwep(obj);

		if (obj->otyp != save_otyp) {
			makeknown(POT_POLYMORPH);
			useup(potion);
			prinv((char *)0, obj, 0L);
			return 1;
		} else {
			pline(E_J("Nothing seems to happen.",
				  "�����N����Ȃ��悤���B"));
			goto poof;
		}
	    }
	    potion->in_use = FALSE;	/* didn't go poof */
	    return(1);
	} else if(obj->oclass == POTION_CLASS && obj->otyp != potion->otyp) {
		int q1, q2;
		/* Mixing potions is dangerous... */
		pline_The(E_J("potions mix...","��t�������荇�����c�B"));
		/* KMH, balance patch -- acid is particularly unstable */
		if (obj->cursed || obj->otyp == POT_ACID || !rn2(10)) {
			pline(E_J("BOOM!  They explode!","�o���I ��͔��������I"));
			exercise(A_STR, FALSE);
			if (!breathless(youmonst.data) || haseyes(youmonst.data))
				potionbreathe(obj);
			useupall(obj);
			useupall(potion);
			losehp(rnd(10), E_J("alchemic blast","���w�������̔�����"), KILLED_BY_AN);
			return(1);
		}

		obj->blessed = obj->cursed = obj->bknown = 0;
		if (Blind || Hallucination) obj->dknown = 0;

		/* Quantities should be same, otherwize mixing will fail */
		q1 = obj->quan;
		q2 = potion->quan;
		if (q1 > q2) {
			q1 = q2;
			q2 = obj->quan;
		}
		if (q1 >= rnd(q2) &&
		    (mixture = mixtype(obj, potion)) != 0) {
			obj->otyp = mixture;
		} else {
		    switch ((obj->odiluted || potion->odiluted) ? 1 : rnd(8)) {
			case 1:
				obj->otyp = POT_WATER;
				break;
			case 2:
			case 3:
				obj->otyp = POT_SICKNESS;
				break;
			case 4:
				{
				  struct obj *otmp;
				  otmp = mkobj(POTION_CLASS,FALSE);
				  obj->otyp = otmp->otyp;
				  obfree(otmp, (struct obj *)0);
				}
				break;
			default:
				if (!Blind)
			  pline_The(E_J("mixture glows brightly and evaporates.",
					"�����t�͖��邭�P���ƁA�������Ă��܂����B"));
				useupall(obj);
				useupall(potion);
				return(1);
		    }
		}

		obj->quan = q2;
		obj->owt = weight(obj);
		obj->odiluted = (obj->otyp != POT_WATER);

		if (obj->otyp == POT_WATER && !Hallucination) {
			pline_The(E_J("mixture bubbles%s.","�����t�͌������A��%s�����B"),
				Blind ? "" : E_J(", then clears","���A�����ɂ�"));
		} else if (!Blind) {
#ifndef JP
			pline_The("mixture looks %s.",
				hcolor(OBJ_DESCR(objects[obj->otyp])));
#else
			pline("�����t��%s��ɂȂ����B",
				hcolor(JOBJ_DESCR(objects[obj->otyp])));
#endif /*JP*/
		}

		useupall(potion);
		update_inventory();
		return(1);
	}

#ifdef INVISIBLE_OBJECTS
	if (potion->otyp == POT_INVISIBILITY && !obj->oinvis) {
		obj->oinvis = TRUE;
		if (!Blind) {
		    if (!See_invisible) pline("Where did %s go?",
		    		the(xname(obj)));
		    else You("notice a little haziness around %s.",
		    		the(xname(obj)));
		}
		goto poof;
	} else if (potion->otyp == POT_SEE_INVISIBLE && obj->oinvis) {
		obj->oinvis = FALSE;
		if (!Blind) {
		    if (!See_invisible) pline("So that's where %s went!",
		    		the(xname(obj)));
		    else pline_The("haziness around %s disappears.",
		    		the(xname(obj)));
		}
		goto poof;
	}
#endif

	if(is_poisonable(obj)) {
	    if(potion->otyp == POT_SICKNESS && !obj->opoisoned) {
#ifndef JP
		char buf[BUFSZ];
		if (potion->quan > 1L)
		    Sprintf(buf, "One of %s", the(xname(potion)));
		else
		    Strcpy(buf, The(xname(potion)));
		pline("%s forms a coating on %s.",
		      buf, the(xname(obj)));
#else
		pline("%s��%s�̕\�ʂɔ疌��������B",
		      xname(potion), xname(obj));
#endif /*JP*/
		obj->opoisoned = TRUE;
		goto poof;
	    } else if(obj->opoisoned &&
		      (potion->otyp == POT_HEALING ||
		       potion->otyp == POT_EXTRA_HEALING ||
		       potion->otyp == POT_FULL_HEALING)) {
#ifndef JP
		pline("A coating wears off %s.", the(xname(obj)));
#else
		pline("%s�ɓh��ꂽ�ł͒��a���ꂽ�B", xname(obj));
#endif /*JP*/
		obj->opoisoned = 0;
		goto poof;
	    }
	}

	if (potion->otyp == POT_OIL) {
	    boolean wisx = FALSE;
	    if (potion->lamplit) {	/* burning */
		int omat = get_material(obj);
		/* the code here should be merged with fire_damage */
		if (catch_lit(obj)) {
		    /* catch_lit does all the work if true */
		} else if (obj->oerodeproof || obj_resists(obj, 5, 95) ||
			   !is_flammable(obj) || obj->oclass == FOOD_CLASS) {
#ifndef JP
		    pline("%s %s to burn for a moment.",
			  Yname2(obj), otense(obj, "seem"));
#else
		    pline("%s�͈�u�R���オ�肻���ɂȂ����B", Yname2(obj));
#endif /*JP*/
		} else {
		    if ((omat == PLASTIC || omat == PAPER) && !obj->oartifact)
			obj->oeroded = MAX_ERODE;
#ifndef JP
		    pline_The("burning oil %s %s.",
			    obj->oeroded == MAX_ERODE ? "destroys" : "damages",
			    yname(obj));
#else
		    pline("%s�͔R����������ɂ����%s���B", yname(obj),
			    obj->oeroded == MAX_ERODE ? "�j�󂳂�" : "����");
#endif /*JP*/
		    if (obj->oeroded == MAX_ERODE) {
			obj_extract_self(obj);
			obfree(obj, (struct obj *)0);
			obj = (struct obj *) 0;
		    } else {
			/* we know it's carried */
			if (obj->unpaid) {
			    /* create a dummy duplicate to put on bill */
			    verbalize(E_J("You burnt it, you bought it!",
					  "�R�₵���񂾂���A�����Ă��炤��I"));
			    bill_dummy_object(obj);
			}
			obj->oeroded++;
		    }
		}
	    } else if (potion->cursed) {
#ifndef JP
		pline_The("potion spills and covers your %s with oil.",
			  makeplural(body_part(FINGER)));
#else
		pline("��r������t�̂����ڂ�A���Ȃ���%s����܂݂�ɂ����B",
			body_part(FINGER));
#endif /*JP*/
		incr_itimeout(&Glib, d(2,10));
	    } else if (obj->oclass != WEAPON_CLASS && !is_weptool(obj)) {
		/* the following cases apply only to weapons */
		goto more_dips;
	    /* Oil removes rust and corrosion, but doesn't unburn.
	     * Arrows, etc are classed as metallic due to arrowhead
	     * material, but dipping in oil shouldn't repair them.
	     */
	    } else if ((!is_rustprone(obj) && !is_corrodeable(obj)) ||
			is_ammo(obj) || (!obj->oeroded && !obj->oeroded2)) {
		/* uses up potion, doesn't set obj->greased */
#ifndef JP
		pline("%s %s with an oily sheen.",
		      Yname2(obj), otense(obj, "gleam"));
#else
		pline("%s�͖����������ɋP�����B",
		      Yname2(obj));
#endif /*JP*/
	    } else {
#ifndef JP
		pline("%s %s less %s.",
		      Yname2(obj), otense(obj, "are"),
		      (obj->oeroded && obj->oeroded2) ? "corroded and rusty" :
			obj->oeroded ? "rusty" : "corroded");
#else
		pline("%s��%s��%s�������B",
		      Yname2(obj),
		      (obj->oeroded && obj->oeroded2) ? "���H�ƎK" :
			obj->oeroded ? "�K" : "���H",
		      max(obj->oeroded, obj->oeroded2) > 1 ? "����" : "");
#endif /*JP*/
		if (obj->oeroded > 0) obj->oeroded--;
		if (obj->oeroded2 > 0) obj->oeroded2--;
		wisx = TRUE;
	    }
	    exercise(A_WIS, wisx);
	    makeknown(potion->otyp);
	    useup(potion);
	    return 1;
	}
    more_dips:

	/* Allow filling of MAGIC_LAMPs to prevent identification by player */
	if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP) &&
	   (potion->otyp == POT_OIL)) {
	    /* Turn off engine before fueling, turn off fuel too :-)  */
	    if (obj->lamplit || potion->lamplit) {
		useup(potion);
		explode(u.ux, u.uy, 11, d(6,6), 0, EXPL_FIERY);
		exercise(A_WIS, FALSE);
		return 1;
	    }
	    /* Adding oil to an empty magic lamp renders it into an oil lamp */
	    if ((obj->otyp == MAGIC_LAMP) && obj->spe == 0) {
		obj->otyp = OIL_LAMP;
		obj->age = 0;
	    }
	    if (obj->age > 1000L) {
#ifndef JP
		pline("%s %s full.", Yname2(obj), otense(obj, "are"));
#else
		pline("%s�̖��͈�t���B", Yname2(obj));
#endif /*JP*/
		potion->in_use = FALSE;	/* didn't go poof */
	    } else {
		You(E_J("fill %s with oil.","%s�ɖ����[�����B"), yname(obj));
		check_unpaid(potion);	/* Yendorian Fuel Tax */
		obj->age += 2*potion->age;	/* burns more efficiently */
		if (obj->age > 1500L) obj->age = 1500L;
		useup(potion);
		exercise(A_WIS, TRUE);
	    }
	    makeknown(POT_OIL);
	    obj->spe = 1;
	    update_inventory();
	    return 1;
	}

	potion->in_use = FALSE;		/* didn't go poof */
	if ((obj->otyp == UNICORN_HORN || obj->otyp == AMETHYST) &&
	    (mixture = mixtype(obj, potion)) != 0) {
		char oldbuf[BUFSZ], newbuf[BUFSZ];
		short old_otyp = potion->otyp;
		boolean old_dknown = FALSE;
		boolean more_than_one = potion->quan > 1;

		oldbuf[0] = '\0';
		if (potion->dknown) {
		    old_dknown = TRUE;
#ifndef JP
		    Sprintf(oldbuf, "%s ",
			    hcolor(OBJ_DESCR(objects[potion->otyp])));
#else
		    Strcpy(oldbuf, hcolor(JOBJ_DESCR(objects[potion->otyp])));
#endif /*JP*/
		}
		/* with multiple merged potions, split off one and
		   just clear it */
		if (potion->quan > 1L) {
		    singlepotion = splitobj(potion, 1L);
		} else singlepotion = potion;
		
		if(singlepotion->unpaid && costly_spot(u.ux, u.uy)) {
		    You(E_J("use it, you pay for it.",
			    "����g���Ă��܂����̂ŁA�ُ����˂΂�ȂȂ��B"));
		    bill_dummy_object(singlepotion);
		}
		singlepotion->otyp = mixture;
		singlepotion->blessed = 0;
		if (mixture == POT_WATER)
		    singlepotion->cursed = singlepotion->odiluted = 0;
		else
		    singlepotion->cursed = obj->cursed;  /* odiluted left as-is */
		singlepotion->bknown = FALSE;
		if (Blind) {
		    singlepotion->dknown = FALSE;
		} else {
		    singlepotion->dknown = !Hallucination;
		    if (mixture == POT_WATER && singlepotion->dknown)
			Sprintf(newbuf, E_J("clears","�����ɂȂ���"));
		    else
#ifndef JP
			Sprintf(newbuf, "turns %s",
				hcolor(OBJ_DESCR(objects[mixture])));
		    pline_The("%spotion%s %s.", oldbuf,
			      more_than_one ? " that you dipped into" : "",
			      newbuf);
#else
			Sprintf(newbuf, "�A%s��ɕω�����",
				hcolor(JOBJ_DESCR(objects[mixture])));
		    pline("%s%s���%s�B",
			  more_than_one ?
			    (obj->otyp == AMETHYST ? "�΂�Z����" : "�p��Z����") : "",
			  oldbuf, newbuf);
#endif /*JP*/
		    if(!objects[old_otyp].oc_uname &&
			!objects[old_otyp].oc_name_known && old_dknown) {
			struct obj fakeobj;
			fakeobj = zeroobj;
			fakeobj.dknown = 1;
			fakeobj.otyp = old_otyp;
			fakeobj.oclass = POTION_CLASS;
			docall(&fakeobj);
		    }
		}
		obj_extract_self(singlepotion);
		singlepotion = hold_another_object(singlepotion,
				E_J("You juggle and drop %s!","%s�͂��Ȃ��̎�𓦂�ė����Ă��܂����I"),
					doname(singlepotion), (const char *)0);
		update_inventory();
		return(1);
	}

	pline(E_J("Interesting...","�ʔ����c�B"));
	return(1);
}


void
djinni_from_bottle(obj)
register struct obj *obj;
{
	struct monst *mtmp;
	int chance;

	if(!(mtmp = makemon(&mons[PM_DJINNI], u.ux, u.uy, NO_MM_FLAGS))){
		pline(E_J("It turns out to be empty.","�r���͋󂾂����悤���B"));
		return;
	}

	if (!Blind) {
#ifndef JP
		pline("In a cloud of smoke, %s emerges!", a_monnam(mtmp));
		pline("%s speaks.", Monnam(mtmp));
#else
		pline("�N���オ�鉌�̒�����A%s�����ꂽ�I", mon_nam(mtmp));
		pline("%s�͂���������:", Monnam(mtmp));
#endif /*JP*/
	} else {
#ifndef JP
		You("smell acrid fumes.");
#else
		pline("�@�����L���������B");
#endif /*JP*/
		pline(E_J("%s speaks.","%s��������:"), Something);
	}

	chance = rn2(5);
	if (obj->blessed) chance = (chance == 4) ? rnd(4) : 0;
	else if (obj->cursed) chance = (chance == 0) ? rn2(4) : 4;
	/* 0,1,2,3,4:  b=80%,5,5,5,5; nc=20%,20,20,20,20; c=5%,5,5,5,80 */

	switch (chance) {
	case 0 : verbalize(E_J("I am in your debt.  I will grant one wish!",
			       "���O�ɂ͎؂肪�ł����B��肢�������Ă�낤�I"));
		makewish();
		mongone(mtmp);
		break;
	case 1 : verbalize(E_J("Thank you for freeing me!","���R�ɂ��Ă���Ă��肪�Ƃ��I"));
		(void) tamedog(mtmp, (struct obj *)0);
		break;
	case 2 : verbalize(E_J("You freed me!","���R���I"));
		setmpeaceful(mtmp, TRUE);
		break;
	case 3 : verbalize(E_J("It is about time!","���낻��s���Ȃ���I"));
		pline(E_J("%s vanishes.","%s�͏������B"), Monnam(mtmp));
		mongone(mtmp);
		break;
	default: verbalize(E_J("You disturbed me, fool!","�����҂߁A�䂪�����W�����ȁI"));
		break;
	}
}

/* clone a gremlin or mold (2nd arg non-null implies heat as the trigger);
   hit points are cut in half (odd HP stays with original) */
struct monst *
split_mon(mon, mtmp)
struct monst *mon,	/* monster being split */
	     *mtmp;	/* optional attacker whose heat triggered it */
{
	struct monst *mtmp2;
	char reason[BUFSZ];

	reason[0] = '\0';
	if (mtmp) Sprintf(reason, E_J(" from %s heat","%s�̔M��"),
			  (mtmp == &youmonst) ? (const char *)E_J("your","���Ȃ���") :
			      (const char *)s_suffix(mon_nam(mtmp)));

	if (mon == &youmonst) {
	    mtmp2 = cloneu();
	    if (mtmp2) {
		mtmp2->mhpmax = u.mhmax / 2;
		u.mhmax -= mtmp2->mhpmax;
		flags.botl = 1;
		You(E_J("multiply%s!","%s���􂵂��I"), reason);
	    }
	} else {
	    mtmp2 = clone_mon(mon, 0, 0);
	    if (mtmp2) {
		mtmp2->mhpmax = mon->mhpmax / 2;
		mon->mhpmax -= mtmp2->mhpmax;
		if (canspotmon(mon))
		    pline(E_J("%s multiplies%s!","%s��%s���􂵂��I"), Monnam(mon), reason);
	    }
	}
	return mtmp2;
}

#endif /* OVLB */

/*potion.c*/
