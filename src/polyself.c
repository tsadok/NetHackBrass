/*	SCCS Id: @(#)polyself.c	3.4	2003/01/08	*/
/*	Copyright (C) 1987, 1988, 1989 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Polymorph self routine.
 *
 * Note:  the light source handling code assumes that both youmonst.m_id
 * and youmonst.mx will always remain 0 when it handles the case of the
 * player polymorphed into a light-emitting monster.
 */

#include "hack.h"

#ifdef OVLB
STATIC_DCL void FDECL(polyman, (const char *,const char *));
STATIC_DCL void NDECL(break_armor);
STATIC_DCL void FDECL(drop_weapon,(int));
STATIC_DCL void NDECL(uunstick);
STATIC_DCL int FDECL(armor_to_dragon,(int));
STATIC_DCL void NDECL(newman);

/* update the youmonst.data structure pointer */
void
set_uasmon()
{
	set_mon_data(&youmonst, &mons[u.umonnum], 0);
}

/* make a (new) human out of the player */
STATIC_OVL void
polyman(fmt, arg)
const char *fmt, *arg;
{
	boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
		was_mimicking = (youmonst.m_ap_type == M_AP_OBJECT);
	boolean could_pass_walls = Passes_walls;
	boolean was_blind = !!Blind;

	if (Upolyd) {
		u.acurr = u.macurr;	/* restore old attribs */
		u.amax = u.mamax;
		u.umonnum = u.umonster;
		flags.female = u.mfemale;
	}
	set_uasmon();

	u.mh = u.mhmax = 0;
	u.mtimedone = 0;
	skinback(FALSE);
	u.uundetected = 0;

	if (sticky) uunstick();
	find_ac();
	if (was_mimicking) {
	    if (multi < 0) unmul("");
	    youmonst.m_ap_type = M_AP_NOTHING;
	}

	newsym(u.ux,u.uy);

	You(fmt, arg);
	/* check whether player foolishly genocided self while poly'd */
	if ((mvitals[urole.malenum].mvflags & G_GENOD) ||
			(urole.femalenum != NON_PM &&
			(mvitals[urole.femalenum].mvflags & G_GENOD)) ||
			(mvitals[urace.malenum].mvflags & G_GENOD) ||
			(urace.femalenum != NON_PM &&
			(mvitals[urace.femalenum].mvflags & G_GENOD))) {
	    /* intervening activity might have clobbered genocide info */
	    killer = delayed_killer;
	    if (!killer || !strstri(killer, E_J("genocid","虐殺"))) {
		killer_format = KILLED_BY;
		killer = E_J("self-genocide","自分を虐殺して");
	    }
	    done(GENOCIDED);
	}

	if (u.twoweap && !could_twoweap(youmonst.data))
	    untwoweapon();

	if (u.utraptype == TT_PIT) {
	    if (could_pass_walls) {	/* player forms cannot pass walls */
		u.utrap = rn1(6,2);
	    }
	}
	if (was_blind && !Blind) {	/* reverting from eyeless */
	    Blinded = 1L;
	    make_blinded(0L, TRUE);	/* remove blindness */
	}

	if (drownbymon() && !Amphibious) {
	    You(E_J("are no longer able to breathe in water!",
		    "もはや水中で呼吸できない！"));
	    start_suffocation(SUFFO_WATER);
	}

	if(!Levitation && !u.ustuck &&
	   (is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy)))
		spoteffects(TRUE);

	see_monsters();
}

void
change_sex()
{
	/* setting u.umonster for caveman/cavewoman or priest/priestess
	   swap unintentionally makes `Upolyd' appear to be true */
	boolean already_polyd = (boolean) Upolyd;

	/* Some monsters are always of one sex and their sex can't be changed */
	/* succubi/incubi can change, but are handled below */
	/* !already_polyd check necessary because is_male() and is_female()
           are true if the player is a priest/priestess */
	if (!already_polyd || (!is_male(youmonst.data) && !is_female(youmonst.data) && !is_neuter(youmonst.data)))
	    flags.female = !flags.female;
	if (already_polyd)	/* poly'd: also change saved sex */
	    u.mfemale = !u.mfemale;
	max_rank_sz();		/* [this appears to be superfluous] */
	if ((already_polyd ? u.mfemale : flags.female) && urole.name.f)
	    Strcpy(pl_character, urole.name.f);
	else
	    Strcpy(pl_character, urole.name.m);
	u.umonster = ((already_polyd ? u.mfemale : flags.female) && urole.femalenum != NON_PM) ?
			urole.femalenum : urole.malenum;
	if (!already_polyd) {
	    u.umonnum = u.umonster;
	} else if (u.umonnum == PM_SUCCUBUS || u.umonnum == PM_INCUBUS) {
	    flags.female = !flags.female;
	    /* change monster type to match new sex */
	    u.umonnum = (u.umonnum == PM_SUCCUBUS) ? PM_INCUBUS : PM_SUCCUBUS;
	    set_uasmon();
	}
	/* some equipments may convey different properties */
	refresh_ladies_wear(0);
}

STATIC_OVL void
newman()
{
	int tmp, oldlvl;
	int oldhpmax;

	oldlvl = u.ulevel;
	u.ulevel = u.ulevel + rn1(5, -2);
	if (u.ulevel > 127 || u.ulevel < 1) { /* level went below 0? */
	    u.ulevel = oldlvl; /* restore old level in case they lifesave */
	    goto dead;
	}
	if (u.ulevel > MAXULEV) u.ulevel = MAXULEV;
	/* If your level goes down, your peak level goes down by
	   the same amount so that you can't simply use blessed
	   full healing to undo the decrease.  But if your level
	   goes up, your peak level does *not* undergo the same
	   adjustment; you might end up losing out on the chance
	   to regain some levels previously lost to other causes. */
	if (u.ulevel < oldlvl) u.ulevelmax -= (oldlvl - u.ulevel);
	if (u.ulevelmax < u.ulevel) u.ulevelmax = u.ulevel;

	if (!rn2(10)) change_sex();

	adjabil(oldlvl, (int)u.ulevel);
	reset_rndmonst(NON_PM);	/* new monster generation criteria */

	/* random experience points for the new experience level */
	u.uexp = rndexp(FALSE);

	/* u.uhpmax * u.ulevel / oldlvl: proportionate hit points to new level
	 * -10 and +10: don't apply proportionate HP to 10 of a starting
	 *   character's hit points (since a starting character's hit points
	 *   are not on the same scale with hit points obtained through level
	 *   gain)
	 * 9 - rn2(19): random change of -9 to +9 hit points
	 */
	tmp = u.uhpbase;
	oldhpmax = u.uhpmax;
#ifndef LINT
	u.uhpbase = ((u.uhpbase - 10) * (long)u.ulevel / oldlvl + 10) +
		(9 - rn2(19));
#endif

#ifdef LINT
	u.uhp = u.uhp + tmp;
#else
	u.uhp = u.uhp * (long)u.uhpbase/oldhpmax;
#endif

	tmp = u.uenmax;
#ifndef LINT
	u.uenmax = u.uenmax * (long)u.ulevel / oldlvl + 9 - rn2(19);
#endif
	if (u.uenmax < 0) u.uenmax = 0;
#ifndef LINT
	u.uen = (tmp ? u.uen * (long)u.uenmax / tmp : u.uenmax);
#endif

	redist_attr();
	recalchpmax();
	u.uhunger = rn1(500,500);
	if (Sick) make_sick(0L, (char *) 0, FALSE, SICK_ALL);
	Stoned = 0;
	delayed_killer = 0;
	if (u.uhp <= 0 || u.uhpmax <= 0) {
		if (Polymorph_control) {
		    if (u.uhp <= 0) u.uhp = 1;
		    if (u.uhpmax <= 0) u.uhpmax = 1;
		} else {
dead: /* we come directly here if their experience level went to 0 or less */
		    Your(E_J("new form doesn't seem healthy enough to survive.",
			     "新しい姿には生き抜くだけの生命力がないようだ。"));
		    killer_format = KILLED_BY_AN;
		    killer=E_J("unsuccessful polymorph","変身に失敗して");
		    done(DIED);
		    newuhs(FALSE);
		    return; /* lifesaved */
		}
	}
	newuhs(FALSE);
	polyman(E_J("feel like a new %s!","新たな%sに生まれ変わったような気がした！"),
		(flags.female && urace.individual.f) ? urace.individual.f :
		(urace.individual.m) ? urace.individual.m : urace.noun);
	if (Slimed) {
		Your(E_J("body transforms, but there is still slime on you.",
			 "姿は変わったが、まだスライムがついたままだ。"));
		Slimed = 10L;
	}
	flags.botl = 1;
	see_monsters();
	(void) encumber_msg();
}

void
polyself(forcecontrol)
boolean forcecontrol;     
{
	char buf[BUFSZ];
	int old_light, new_light;
	int mntmp = NON_PM;
	int tries=0;
	boolean draconian = (uarm &&
				uarm->otyp >= GRAY_DRAGON_SCALE_MAIL &&
				uarm->otyp <= YELLOW_DRAGON_SCALES);
	boolean iswere = (u.ulycn >= LOW_PM || is_were(youmonst.data));
	boolean isvamp = (youmonst.data->mlet == S_VAMPIRE || u.umonnum == PM_VAMPIRE_BAT);

        if(!Polymorph_control && !forcecontrol && !draconian && !iswere && !isvamp) {
	    if (rn2(20) > ACURR(A_CON)) {
		You(shudder_for_moment);
		losehp(rnd(30), E_J("system shock","システム\tショックで"), KILLED_BY_AN);
		exercise(A_CON, FALSE);
		return;
	    }
	}
	old_light = Upolyd ? emits_light(youmonst.data) : 0;

	if (Polymorph_control || forcecontrol) {
		do {
			getlin(E_J("Become what kind of monster? [type the name]",
				   "どの種類の怪物になりますか？[名前を入力]"),
				buf);
			mntmp = name_to_mon(buf);
			if (mntmp < LOW_PM)
				pline(E_J("I've never heard of such monsters.",
					  "そのような怪物は聞いたことがありません。"));
			/* Note:  humans are illegal as monsters, but an
			 * illegal monster forces newman(), which is what we
			 * want if they specified a human.... */
			else if (!polyok(&mons[mntmp]) && !your_race(&mons[mntmp]))
				You(E_J("cannot polymorph into that.",
					"その怪物には変身できない。"));
			else break;
		} while(++tries < 5);
		if (tries==5) pline(thats_enough_tries);
		/* allow skin merging, even when polymorph is controlled */
		if (draconian &&
		    (mntmp == armor_to_dragon(uarm->otyp) || tries == 5))
		    goto do_merge;
	} else if (draconian || iswere || isvamp) {
		/* special changes that don't require polyok() */
		if (draconian) {
		    do_merge:
			mntmp = armor_to_dragon(uarm->otyp);
			if (!(mvitals[mntmp].mvflags & G_GENOD)) {
				/* allow G_EXTINCT */
				You(E_J("merge with your scaly armor.",
					"身につけている鱗と融合した。"));
				uskin = uarm;
				uarm = (struct obj *)0;
				/* save/restore hack */
				uskin->owornmask |= I_SPECIAL;
			}
		} else if (iswere) {
			if (is_were(youmonst.data))
				mntmp = PM_HUMAN; /* Illegal; force newman() */
			else
				mntmp = u.ulycn;
		} else {
			if (youmonst.data->mlet == S_VAMPIRE)
				mntmp = PM_VAMPIRE_BAT;
			else
				mntmp = PM_VAMPIRE;
		}
		/* if polymon fails, "you feel" message has been given
		   so don't follow up with another polymon or newman */
		if (mntmp == PM_HUMAN) newman();	/* werecritter */
		else (void) polymon(mntmp);
		goto made_change;    /* maybe not, but this is right anyway */
	}

	if (mntmp < LOW_PM) {
		tries = 0;
		do {
			/* randomly pick an "ordinary" monster */
			mntmp = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
		} while((!polyok(&mons[mntmp]) || is_placeholder(&mons[mntmp]))
				&& tries++ < 200);
	}

	/* The below polyok() fails either if everything is genocided, or if
	 * we deliberately chose something illegal to force newman().
	 */
	if (!polyok(&mons[mntmp]) || !rn2(5) || your_race(&mons[mntmp]))
		newman();
	else if(!polymon(mntmp)) return;

	if (!uarmg) selftouch(E_J("No longer petrify-resistant, you",
				  "石化への耐性を失い、あなたは"));

 made_change:
	new_light = Upolyd ? emits_light(youmonst.data) : 0;
	if (u.uspellit && new_light) extinguish_torch();
	if (old_light != new_light) {
	    if (old_light)
		del_light_source(LS_MONSTER, (genericptr_t)&youmonst);
	    if (new_light == 1) ++new_light;  /* otherwise it's undetectable */
	    if (new_light)
		new_light_source(u.ux, u.uy, new_light,
				 LS_MONSTER, (genericptr_t)&youmonst);
	}
	if (u.twoweap && !can_twoweapon()) untwoweapon();
}

/* (try to) make a mntmp monster out of the player */
int
polymon(mntmp)	/* returns 1 if polymorph successful */
int	mntmp;
{
	boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
		was_blind = !!Blind, dochange = FALSE;
	boolean could_pass_walls = Passes_walls;
	int mlvl;

	if (mvitals[mntmp].mvflags & G_GENOD) {	/* allow G_EXTINCT */
#ifndef JP
		You_feel("rather %s-ish.",mons[mntmp].mname);
#else
		You("かなり%sっぽくなった。",JMONNAM(mntmp));
#endif /*JP*/
		exercise(A_WIS, TRUE);
		return(0);
	}

	/* KMH, conduct */
	u.uconduct.polyselfs++;

	if (!Upolyd) {
		/* Human to monster; save human stats */
		u.macurr = u.acurr;
		u.mamax = u.amax;
		u.mfemale = flags.female;
	} else {
		/* Monster to monster; restore human stats, to be
		 * immediately changed to provide stats for the new monster
		 */
		u.acurr = u.macurr;
		u.amax = u.mamax;
		flags.female = u.mfemale;
	}

	if (youmonst.m_ap_type) {
	    /* stop mimicking immediately */
	    if (multi < 0) unmul("");
	} else if (mons[mntmp].mlet != S_MIMIC) {
	    /* as in polyman() */
	    youmonst.m_ap_type = M_AP_NOTHING;
	}
	if (is_male(&mons[mntmp])) {
		if(flags.female) dochange = TRUE;
	} else if (is_female(&mons[mntmp])) {
		if(!flags.female) dochange = TRUE;
	} else if (!is_neuter(&mons[mntmp]) && mntmp != u.ulycn) {
		if(!rn2(10)) dochange = TRUE;
	}
	if (dochange) {
		flags.female = !flags.female;
#ifndef JP
		You("%s %s%s!",
		    (u.umonnum != mntmp) ? "turn into a" : "feel like a new",
		    (is_male(&mons[mntmp]) || is_female(&mons[mntmp])) ? "" :
			flags.female ? "female " : "male ",
		    mons[mntmp].mname);
#else
		You("%s%s%sに%sた！",
		    (u.umonnum != mntmp) ? "" : "新たな",
		    (is_male(&mons[mntmp]) || is_female(&mons[mntmp])) ? "" :
			flags.female ? "女性の" : "男性の",
		    JMONNAM(mntmp),
		    (u.umonnum != mntmp) ? "変身し" : "なっ");
#endif /*JP*/
	} else {
		if (u.umonnum != mntmp)
#ifndef JP
			You("turn into %s!", an(mons[mntmp].mname));
#else
			You("%sに変身した！", JMONNAM(mntmp));
#endif /*JP*/
		else
#ifndef JP
			You_feel("like a new %s!", mons[mntmp].mname);
#else
			You("新たな%sになった！", JMONNAM(mntmp));
#endif /*JP*/
	}
	if (Stoned && poly_when_stoned(&mons[mntmp])) {
		/* poly_when_stoned already checked stone golem genocide */
		You(E_J("turn to stone!","石になった！"));
		mntmp = PM_STONE_GOLEM;
		Stoned = 0;
		delayed_killer = 0;
	}

	u.mtimedone = rn1(500, 500);
	u.umonnum = mntmp;
	set_uasmon();

	/* New stats for monster, to last only as long as polymorphed.
	 * Currently only strength gets changed.
	 */
	if(strongmonst(&mons[mntmp])) ABASE(A_STR) = AMAX(A_STR) = STR18(100);

	if (Stone_resistance && Stoned) { /* parnes@eniac.seas.upenn.edu */
		Stoned = 0;
		delayed_killer = 0;
		You(E_J("no longer seem to be petrifying.",
			"もう石化しないようだ。"));
	}
	if (Sick_resistance && Sick) {
		make_sick(0L, (char *) 0, FALSE, SICK_ALL);
		You(E_J("no longer feel sick.",
			"もはや病苦を感じない。"));
	}
	if (Slimed) {
	    if (flaming(youmonst.data)) {
		pline_The(E_J("slime burns away!",
			      "スライムは燃え尽きた！"));
		Slimed = 0L;
		flags.botl = 1;
	    } else if (mntmp == PM_GREEN_SLIME) {
		/* do it silently */
		Slimed = 0L;
		flags.botl = 1;
	    }
	}
	if (Strangled) {
	    long cure = 0;
	    if (Breathless) cure |= SUFFO_WATER|SUFFO_NOAIR;
	    if (Amphibious) cure |= SUFFO_WATER;
	    if (amorphous(youmonst.data) || noncorporeal(youmonst.data) ||
		unsolid(youmonst.data)) {
		cure |= SUFFO_NECK;
		if (StrangledBy & SUFFO_NECK) {
		    Your(E_J("%s becomes free from constriction.",
			     "%sは絞めつけから解放された。"), body_part(NECK));
		    if (uamul && uamul->otyp == AMULET_OF_STRANGULATION)
			setnotworn(uamul);
		}
	    }
	    if (cure) end_suffocation(cure);
	}
	if (nohands(youmonst.data)) Glib = 0;

	/*
	mlvl = adj_lev(&mons[mntmp]);
	 * We can't do the above, since there's no such thing as an
	 * "experience level of you as a monster" for a polymorphed character.
	 */
	mlvl = (int)mons[mntmp].mlevel;
	if (youmonst.data->mlet == S_DRAGON && mntmp >= PM_GRAY_DRAGON) {
		u.mhmax = In_endgame(&u.uz) ? (8*mlvl) : (4*mlvl + d(mlvl,4));
	} else if (is_golem(youmonst.data)) {
		u.mhmax = golemhp(mntmp);
	} else {
		if (!mlvl) u.mhmax = rnd(4);
		else u.mhmax = d(mlvl, 8);
		if (is_home_elemental(&mons[mntmp])) u.mhmax *= 3;
	}
	u.mh = u.mhmax;

	if (u.ulevel < mlvl) {
	/* Low level characters can't become high level monsters for long */
#ifdef DUMB
		/* DRS/NS 2.2.6 messes up -- Peter Kendell */
		int mtd = u.mtimedone, ulv = u.ulevel;

		u.mtimedone = mtd * ulv / mlvl;
#else
		u.mtimedone = u.mtimedone * u.ulevel / mlvl;
#endif
	}

	if (uskin && mntmp != armor_to_dragon(uskin->otyp))
		skinback(FALSE);
	refresh_ladies_wear(0);
	break_armor();
	drop_weapon(1);
	if (hides_under(youmonst.data))
		u.uundetected = OBJ_AT(u.ux, u.uy);
	else if (youmonst.data->mlet == S_EEL)
		u.uundetected = is_pool(u.ux, u.uy);
	else
		u.uundetected = 0;

	if (u.utraptype == TT_PIT) {
	    if (could_pass_walls && !Passes_walls) {
		u.utrap = rn1(6,2);
	    } else if (!could_pass_walls && Passes_walls) {
		u.utrap = 0;
	    }
	}
	if (was_blind && !Blind) {	/* previous form was eyeless */
	    Blinded = 1L;
	    make_blinded(0L, TRUE);	/* remove blindness */
	}
	newsym(u.ux,u.uy);		/* Change symbol */

	if (!sticky && !u.uswallow && u.ustuck && sticks(youmonst.data)) u.ustuck = 0;
	else if (sticky && !sticks(youmonst.data)) uunstick();
#ifdef STEED
	if (u.usteed) {
	    if (touch_petrifies(u.usteed->data) &&
	    		!Stone_resistance && rnl(3)) {
	    	char buf[BUFSZ];

	    	pline(E_J("No longer petrifying-resistant, you touch %s.",
			  "石化への耐性を失い、あなたは%sに触ってしまった。"),
	    			mon_nam(u.usteed));
#ifndef JP
	    	Sprintf(buf, "riding %s", an(u.usteed->data->mname));
#else
	    	Sprintf(buf, "%sに乗って", JMONNAM(monsndx(u.usteed->data->mname)));
#endif /*JP*/
	    	instapetrify(buf);
 	    }
	    if (!can_ride(u.usteed)) dismount_steed(DISMOUNT_POLY);
	}
#endif

	if (flags.verbose) {
	    static const char use_thec[] = E_J("Use the command #%s to %s.",
					       "%sには #%sコマンドを使ってください。");
	    static const char monsterc[] = "monster";
#ifndef JP
	    if (can_breathe(youmonst.data))
		pline(use_thec,monsterc,"use your breath weapon");
	    if (attacktype(youmonst.data, AT_SPIT))
		pline(use_thec,monsterc,"spit venom");
	    if (youmonst.data->mlet == S_NYMPH)
		pline(use_thec,monsterc,"remove an iron ball");
	    if (attacktype(youmonst.data, AT_GAZE))
		pline(use_thec,monsterc,"gaze at monsters");
	    if (is_hider(youmonst.data))
		pline(use_thec,monsterc,"hide");
	    if (is_were(youmonst.data))
		pline(use_thec,monsterc,"summon help");
	    if (webmaker(youmonst.data))
		pline(use_thec,monsterc,"spin a web");
	    if (u.umonnum == PM_GREMLIN)
		pline(use_thec,monsterc,"multiply in a fountain");
	    if (is_unicorn(youmonst.data))
		pline(use_thec,monsterc,"use your horn");
	    if (is_mind_flayer(youmonst.data))
		pline(use_thec,monsterc,"emit a mental blast");
	    if (youmonst.data->msound == MS_SHRIEK) /* worthless, actually */
		pline(use_thec,monsterc,"shriek");
	    if (lays_eggs(youmonst.data) && flags.female)
		pline(use_thec,"sit","lay an egg");
#else
	    if (can_breathe(youmonst.data))
		pline(use_thec,"ブレスを吐く",monsterc);
	    if (attacktype(youmonst.data, AT_SPIT))
		pline(use_thec,"毒液を吐く",monsterc);
	    if (youmonst.data->mlet == S_NYMPH)
		pline(use_thec,"鉄球を外す",monsterc);
	    if (attacktype(youmonst.data, AT_GAZE))
		pline(use_thec,"敵を睨む",monsterc);
	    if (is_hider(youmonst.data))
		pline(use_thec,"隠れる",monsterc);
	    if (is_were(youmonst.data))
		pline(use_thec,"助けを呼ぶ",monsterc);
	    if (webmaker(youmonst.data))
		pline(use_thec,"網を張る",monsterc);
	    if (u.umonnum == PM_GREMLIN)
		pline(use_thec,"泉で分裂する",monsterc);
	    if (is_unicorn(youmonst.data))
		pline(use_thec,"角を使う",monsterc);
	    if (is_mind_flayer(youmonst.data))
		pline(use_thec,"精神波を浴びせる",monsterc);
	    if (youmonst.data->msound == MS_SHRIEK) /* worthless, actually */
		pline(use_thec,"金切り声を上げる",monsterc);
	    if (lays_eggs(youmonst.data) && flags.female)
		pline(use_thec,"卵を産む","sit");
#endif /*JP*/
	}
	/* you now know what an egg of your type looks like */
	if (lays_eggs(youmonst.data)) {
	    learn_egg_type(u.umonnum);
	    /* make queen bees recognize killer bee eggs */
	    learn_egg_type(egg_type_from_parent(u.umonnum, TRUE));
	}
	find_ac();
	if((!Levitation && !u.ustuck && !Flying &&
	    (is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy))) ||
	   (Underwater && !Swimming))
	    spoteffects(TRUE);
	if (Passes_walls && u.utrap && u.utraptype == TT_INFLOOR) {
	    u.utrap = 0;
	    pline_The(E_J("rock seems to no longer trap you.",
			  "岩盤はもはやあなたを捕らえられないようだ。"));
	} else if (likes_lava(youmonst.data) && u.utrap && u.utraptype == TT_LAVA) {
	    u.utrap = 0;
	    pline_The(E_J("lava now feels soothing.",
			  "溶岩はとても癒される。"));
	}
	if (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data)) {
	    if (Punished) {
#ifndef JP
		You("slip out of the iron chain.");
#else
		pline("鉄の鎖はあなたをすり抜けた。");
#endif /*JP*/
		unpunish();
	    }
	}
	if (drownbymon() && !Amphibious) {
	    You(E_J("are no longer able to breathe in water!",
		    "もはや水中では呼吸できない！"));
	    start_suffocation(SUFFO_WATER);
	}
	if (u.utrap && (u.utraptype == TT_WEB || u.utraptype == TT_BEARTRAP) &&
		(amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data) ||
		  (youmonst.data->msize <= MZ_SMALL && u.utraptype == TT_BEARTRAP))) {
#ifndef JP
	    You("are no longer stuck in the %s.",
		    u.utraptype == TT_WEB ? "web" : "bear trap");
#else
	    You("もはや%sに捕らえられていない。",
		    u.utraptype == TT_WEB ? "蜘蛛の巣" : "トラバサミ");
#endif /*JP*/
	    /* probably should burn webs too if PM_FIRE_ELEMENTAL */
	    u.utrap = 0;
	}
	if (webmaker(youmonst.data) && u.utrap && u.utraptype == TT_WEB) {
	    You(E_J("orient yourself on the web.",
		    "蜘蛛の巣の上に陣取った。"));
	    u.utrap = 0;
	}
	flags.botl = 1;
	vision_full_recalc = 1;
	see_monsters();
	exercise(A_CON, FALSE);
	exercise(A_WIS, TRUE);
	(void) encumber_msg();
	return(1);
}

STATIC_OVL void
break_armor()
{
    register struct obj *otmp;
    boolean was_floating = (Levitation || Flying);

    if (breakarm(youmonst.data)) {
	if ((otmp = uarm) != 0) {
		if (donning(otmp)) cancel_don();
#ifndef JP
		You("break out of your armor!");
#else
		You("%sを破り抜いた！", armor_simple_name(otmp));
#endif /*JP*/
		exercise(A_STR, FALSE);
		(void) Armor_gone();
		useup(otmp);
	}
	if ((otmp = uarmc) != 0) {
	    if(otmp->oartifact) {
		Your(E_J("%s falls off!",
			 "%sが脱げ落ちた！"), cloak_simple_name(otmp));
		(void) Cloak_off();
		dropx(otmp);
	    } else {
		Your(E_J("%s tears apart!",
			 "%sはずたずたに裂けた！"), cloak_simple_name(otmp));
		(void) Cloak_off();
		useup(otmp);
	    }
	}
#ifdef TOURIST
	if (uarmu) {
		Your(E_J("shirt rips to shreds!","シャツは引きちぎれた！"));
		useup(uarmu);
	}
#endif
    } else if (sliparm(youmonst.data)) {
	if (((otmp = uarm) != 0) && (racial_exception(&youmonst, otmp) < 1)) {
		if (donning(otmp)) cancel_don();
#ifndef JP
		Your("armor falls around you!");
#else
		Your("%sはあなたのまわりに落ちた！", armor_simple_name(otmp));
#endif /*JP*/
		(void) Armor_gone();
		dropx(otmp);
	}
	if ((otmp = uarmc) != 0) {
		if (is_whirly(youmonst.data))
			Your(E_J("%s falls, unsupported!",
				 "%sは支えを失って落ちた！"), cloak_simple_name(otmp));
		else You(E_J("shrink out of your %s!",
			     "%sの下で縮んだ！"), cloak_simple_name(otmp));
		(void) Cloak_off();
		dropx(otmp);
	}
#ifdef TOURIST
	if ((otmp = uarmu) != 0) {
		if (is_whirly(youmonst.data))
			You(E_J("seep right through your shirt!",
				"シャツの外に通り抜けた！"));
		else You(E_J("become much too small for your shirt!",
			     "シャツに対してとても小さくなった！"));
		setworn((struct obj *)0, otmp->owornmask & W_ARMU);
		dropx(otmp);
	}
#endif
    }
    if (has_horns(youmonst.data)) {
	if ((otmp = uarmh) != 0) {
	    if (is_flimsy(otmp) && !donning(otmp)) {
		char hornbuf[BUFSZ], yourbuf[BUFSZ];

		/* Future possiblities: This could damage/destroy helmet */
#ifndef JP
		Sprintf(hornbuf, "horn%s", plur(num_horns(youmonst.data)));
		Your("%s %s through %s %s.", hornbuf, vtense(hornbuf, "pierce"),
		     shk_your(yourbuf, otmp), xname(otmp));
#else
		Your("角が%s%sを突き抜けた！",
		     shk_your(yourbuf, otmp), xname(otmp));
#endif /*JP*/
	    } else {
		if (donning(otmp)) cancel_don();
		Your(E_J("helmet falls to the %s!",
			 "兜が%sに落ちた！"), surface(u.ux, u.uy));
		(void) Helmet_off();
		dropx(otmp);
	    }
	}
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data)) {
	if ((otmp = uarmg) != 0) {
	    if (donning(otmp)) cancel_don();
	    /* Drop weapon along with gloves */
#ifndef JP
	    You("drop your gloves%s!", uwep ? " and weapon" : "");
#else
	    You("篭手%sを落とした！", uwep ? "と武器" : "");
#endif /*JP*/
	    drop_weapon(0);
	    (void) Gloves_off();
	    dropx(otmp);
	}
	if ((otmp = uarms) != 0) {
	    You(E_J("can no longer hold your shield!",
		    "もう盾を構えていられない！"));
	    (void) Shield_off();
	    dropx(otmp);
	}
	if ((otmp = uarmh) != 0) {
	    if (donning(otmp)) cancel_don();
	    Your(E_J("helmet falls to the %s!",
		     "兜が%sに落ちた！"), surface(u.ux, u.uy));
	    (void) Helmet_off();
	    dropx(otmp);
	}
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data) ||
		slithy(youmonst.data) || youmonst.data->mlet == S_CENTAUR) {
	if ((otmp = uarmf) != 0) {
	    if (donning(otmp)) cancel_don();
	    if (is_whirly(youmonst.data))
		Your(E_J("boots fall away!","ブーツが落下した！"));
	    else Your(E_J("boots %s off your feet!",
			  "ブーツが足から%s落ちた！"),
			verysmall(youmonst.data) ? E_J("slide","滑り") : E_J("are pushed","脱げ"));
	    (void) Boots_off();
	    dropx(otmp);
	}
    }
	if (is_pool(u.ux,u.uy) && was_floating && !(Levitation || Flying) &&
		!breathless(youmonst.data) && !amphibious(youmonst.data) &&
		!Swimming) drown();
}

STATIC_OVL void
drop_weapon(alone)
int alone;
{
    struct obj *otmp;
    struct obj *otmp2;

    if ((otmp = uwep) != 0) {
	/* !alone check below is currently superfluous but in the
	 * future it might not be so if there are monsters which cannot
	 * wear gloves but can wield weapons
	 */
	if (!alone || cantwield(youmonst.data)) {
	    struct obj *wep = uwep;

	    if (alone) You(E_J("find you must drop your weapon%s!",
			       "%s武器を構えていられない！"),
			   	u.twoweap ? E_J("s","両方の") : "");
	    otmp2 = u.twoweap ? uswapwep : 0;
	    uwepgone();
	    if (!wep->cursed || wep->otyp != LOADSTONE)
		dropx(otmp);
	    if (otmp2 != 0) {
		uswapwepgone();
		if (!otmp2->cursed || otmp2->otyp != LOADSTONE)
		    dropx(otmp2);
	    }
	    untwoweapon();
	} else if (!could_twoweap(youmonst.data)) {
	    untwoweapon();
	}
    }
}

void
rehumanize()
{
	/* You can't revert back while unchanging */
	if (Unchanging && (u.mh < 1)) {
		killer_format = NO_KILLER_PREFIX;
		killer = E_J("killed while stuck in creature form",
			     "変身が解けないまま殺された");
		done(DIED);
	}

	if (emits_light(youmonst.data))
	    del_light_source(LS_MONSTER, (genericptr_t)&youmonst);
	polyman(E_J("return to %s form!","%s姿に戻った！"), urace.adj);

	if (u.uhp < 1) {
	    char kbuf[256];

	    Sprintf(kbuf, E_J("reverting to unhealthy %s form",
			      "生命力のない%sの姿に戻って"), urace.adj);
	    killer_format = KILLED_BY;
	    killer = kbuf;
	    done(DIED);
	}
	if (!uarmg) selftouch(E_J("No longer petrify-resistant, you",
				  "石化への耐性を失って、あなたは"));
	nomul(0);

	refresh_ladies_wear(0);
	flags.botl = 1;
	vision_full_recalc = 1;
	(void) encumber_msg();
}

int
dobreathe()
{
	struct attack *mattk;

	if (Strangled) {
	    You_cant(E_J("breathe.  Sorry.","息ができない。すみません。"));
	    return(0);
	}
	if (u.uen < 15) {
	    You(E_J("don't have enough energy to breathe!",
		    "ブレスを吐くのに十分なエネルギーを持っていない。"));
	    return(0);
	}
	u.uen -= 15;
	flags.botl = 1;

	if (!getdir_or_pos(0, GETPOS_MONTGT, (char *)0, E_J("breathe at","ブレスの目標"))) return(0);

	mattk = attacktype_fordmg(youmonst.data, AT_BREA, AD_ANY);
	if (!mattk)
	    impossible("bad breath attack?");	/* mouthwash needed... */
	else
	    buzz((int) (20 + mattk->adtyp-1), (int)mattk->damn,
		u.ux, u.uy, u.dx, u.dy);
	return(1);
}

int
dospit()
{
	struct obj *otmp;

	if (!getdir((char *)0)) return(0);
	otmp = mksobj(u.umonnum==PM_COBRA ? BLINDING_VENOM : ACID_VENOM,
			TRUE, FALSE);
	otmp->spe = 1; /* to indicate it's yours */
	throwit(otmp, 0L, FALSE);
	return(1);
}

int
doremove()
{
	if (!Punished) {
		You(E_J("are not chained to anything!",
			"鎖に繋がれていない！"));
		return(0);
	}
	unpunish();
	return(1);
}

int
dospinweb()
{
	register struct trap *ttmp = t_at(u.ux,u.uy);

	if (Levitation || Is_airlevel(&u.uz)
	    || Underwater || Is_waterlevel(&u.uz)) {
		You(E_J("must be on the ground to spin a web.",
			"網を張るには地面の上にいなければならない。"));
		return(0);
	}
	if (u.uswallow) {
		You(E_J("release web fluid inside %s.",
			"%sの内部に糸を吐き出した。"), mon_nam(u.ustuck));
		if (is_animal(u.ustuck->data)) {
			expels(u.ustuck, u.ustuck->data, TRUE);
			return(0);
		}
		if (is_whirly(u.ustuck->data)) {
			int i;

			for (i = 0; i < NATTK; i++)
				if (u.ustuck->data->mattk[i].aatyp == AT_ENGL)
					break;
			if (i == NATTK)
			       impossible("Swallower has no engulfing attack?");
			else {
				char sweep[30];

				sweep[0] = '\0';
				switch(u.ustuck->data->mattk[i].adtyp) {
					case AD_FIRE:
						Strcpy(sweep, E_J("ignites and ",
								  "燃え尽き"));
						break;
					case AD_ELEC:
						Strcpy(sweep, E_J("fries and ",
								  "火花を散らし"));
						break;
					case AD_COLD:
						Strcpy(sweep,
						      E_J("freezes, shatters and ",
							  "凍って砕け"));
						break;
				}
				pline_The(E_J("web %sis swept away!",
					      "蜘蛛の糸は%s、消え去った！"), sweep);
			}
			return(0);
		}		     /* default: a nasty jelly-like creature */
		pline_The(E_J("web dissolves into %s.",
			      "蜘蛛の糸は%sの中に溶けていった。"), mon_nam(u.ustuck));
		return(0);
	}
	if (u.utrap) {
		You(E_J("cannot spin webs while stuck in a trap.",
			"罠にかかっている間は網を張れない。"));
		return(0);
	}
	exercise(A_DEX, TRUE);
	if (ttmp) switch (ttmp->ttyp) {
		case PIT:
		case SPIKED_PIT: You(E_J("spin a web, covering up the pit.",
					 "網を張り、落とし穴を覆った。"));
			deltrap(ttmp);
			bury_objs(u.ux, u.uy);
			newsym(u.ux, u.uy);
			return(1);
		case SQKY_BOARD: pline_The(E_J("squeaky board is muffled.",
					       "きしむ床板の音は消された。"));
			deltrap(ttmp);
			newsym(u.ux, u.uy);
			return(1);
		case TELEP_TRAP:
		case LEVEL_TELEP:
		case MAGIC_PORTAL:
			Your(E_J("webbing vanishes!","網は消え去った！"));
			return(0);
		case WEB: You(E_J("make the web thicker.",
				  "蜘蛛の巣を補強した。"));
			return(1);
		case HOLE:
		case TRAPDOOR:
#ifndef JP
			You("web over the %s.",
			    (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole");
#else
			You("%sの上に網を張った。",
			    (ttmp->ttyp == TRAPDOOR) ? "落とし扉" : "穴");
#endif /*JP*/
			deltrap(ttmp);
			newsym(u.ux, u.uy);
			return 1;
		case ROLLING_BOULDER_TRAP:
			You(E_J("spin a web, jamming the trigger.",
				"網を張り、起動装置を動かなくした。"));
			deltrap(ttmp);
			newsym(u.ux, u.uy);
			return(1);
		case ARROW_TRAP:
		case DART_TRAP:
		case BEAR_TRAP:
		case ROCKTRAP:
		case FIRE_TRAP:
		case LANDMINE:
		case SLP_GAS_TRAP:
		case RUST_TRAP:
		case MAGIC_TRAP:
		case ANTI_MAGIC:
		case POLY_TRAP:
			You(E_J("have triggered a trap!",
				"罠を起動してしまった！"));
			dotrap(ttmp, 0);
			return(1);
		default:
			impossible("Webbing over trap type %d?", ttmp->ttyp);
			return(0);
		}
	else if (On_stairs(u.ux, u.uy)) {
	    /* cop out: don't let them hide the stairs */
#ifndef JP
	    Your("web fails to impede access to the %s.",
		 (levl[u.ux][u.uy].typ == STAIRS) ? "stairs" : "ladder");
#else
	    Your("蜘蛛の巣では%sを通行止めにはできなかった。",
		 (levl[u.ux][u.uy].typ == STAIRS) ? "階段" : "はしご");
#endif /*JP*/
	    return(1);
		 
	}
	ttmp = maketrap(u.ux, u.uy, WEB);
	if (ttmp) {
		ttmp->tseen = 1;
		ttmp->madeby_u = 1;
	}
	newsym(u.ux, u.uy);
	return(1);
}

int
dosummon()
{
	int placeholder;
	if (u.uen < 10) {
	    You(E_J("lack the energy to send forth a call for help!",
		    "助けを召喚するのに十分なエネルギーを持っていない！"));
	    return(0);
	}
	u.uen -= 10;
	flags.botl = 1;

	You(E_J("call upon your brethren for help!",
		"自らの同胞を召喚した！"));
	exercise(A_WIS, TRUE);
	if (!were_summon(youmonst.data, TRUE, &placeholder, (char *)0))
		pline(E_J("But none arrive.","しかし、何も現れなかった。"));
	return(1);
}

int
dogaze()
{
	register struct monst *mtmp;
	int looked = 0;
	char qbuf[QBUFSZ];
	int i;
	uchar adtyp = 0;

	for (i = 0; i < NATTK; i++) {
	    if(youmonst.data->mattk[i].aatyp == AT_GAZE) {
		adtyp = youmonst.data->mattk[i].adtyp;
		break;
	    }
	}
	if (adtyp != AD_CONF && adtyp != AD_FIRE) {
	    impossible("gaze attack %d?", adtyp);
	    return 0;
	}


	if (Blind) {
	    You_cant(E_J("see anything to gaze at.",
			 "睨むものが何も見えない。"));
	    return 0;
	}
	if (u.uen < 15) {
	    You(E_J("lack the energy to use your special gaze!",
		    "眼光の力を使うのに十分なエネルギーを持っていない！"));
	    return(0);
	}
	u.uen -= 15;
	flags.botl = 1;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
		looked++;
		if (Invis && !perceives(mtmp->data))
		    pline(E_J("%s seems not to notice your gaze.",
			      "%sはあなたの睨みに気づかないようだ。"), Monnam(mtmp));
		else if (mtmp->minvis && !See_invisible)
		    You_cant(E_J("see where to gaze at %s.",
				 "%sのどこを睨めばいいのかわからない。"), Monnam(mtmp));
		else if (mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT) {
		    looked--;
		    continue;
		} else if (flags.safe_dog && !Confusion && !Hallucination
		  && mtmp->mtame) {
		    You(E_J("avoid gazing at %s.",
			    "%sを睨むのを避けた。"), y_monnam(mtmp));
		} else {
		    if (flags.confirm && mtmp->mpeaceful && !Confusion
							&& !Hallucination) {
#ifndef JP
			Sprintf(qbuf, "Really %s %s?",
			    (adtyp == AD_CONF) ? "confuse" : "attack",
			    mon_nam(mtmp));
#else
			Sprintf(qbuf, "本当に%sを%sますか？",
			    mon_nam(mtmp),
			    (adtyp == AD_CONF) ? "混乱させ" : "攻撃し");
#endif /*JP*/
			if (yn(qbuf) != 'y') continue;
			setmangry(mtmp);
		    }
		    if (!mtmp->mcanmove || mtmp->mstun || mtmp->msleeping ||
				    !mtmp->mcansee || !haseyes(mtmp->data)) {
			looked--;
			continue;
		    }
		    /* No reflection check for consistency with when a monster
		     * gazes at *you*--only medusa gaze gets reflected then.
		     */
		    if (adtyp == AD_CONF) {
			if (!mtmp->mconf)
			    Your(E_J("gaze confuses %s!",
				     "眼光が%sを混乱させた！"), mon_nam(mtmp));
			else
			    pline(E_J("%s is getting more and more confused.",
				      "%sはますます混乱したようだ。"),
							Monnam(mtmp));
			mtmp->mconf = 1;
		    } else if (adtyp == AD_FIRE) {
			int dmg = d(2,6);
			You(E_J("attack %s with a fiery gaze!",
				"%sを炎の睨みで攻撃した！"), mon_nam(mtmp));
			if (resists_fire(mtmp)) {
			    pline_The(E_J("fire doesn't burn %s!",
					  "%sは炎に焼かれなかった！"), mon_nam(mtmp));
			    dmg = 0;
			}
			if((int) u.ulevel > rn2(20))
			    (void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			if((int) u.ulevel > rn2(20))
			    (void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			if((int) u.ulevel > rn2(25))
			    (void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
			if (dmg && !DEADMONSTER(mtmp)) mtmp->mhp -= dmg;
			if (mtmp->mhp <= 0) killed(mtmp);
		    }
		    /* For consistency with passive() in uhitm.c, this only
		     * affects you if the monster is still alive.
		     */
		    if (!DEADMONSTER(mtmp) &&
			  (mtmp->data==&mons[PM_FLOATING_EYE]) && !mtmp->mcan) {
			if (!Free_action) {
			    You(E_J("are frozen by %s gaze!",
				    "%s眼光で動けなくなった！"),
					     s_suffix(mon_nam(mtmp)));
			    nomul((u.ulevel > 6 || rn2(4)) ?
				    -d((int)mtmp->m_lev+1,
					    (int)mtmp->data->mattk[0].damd)
				    : -200);
			    return 1;
			} else
			    You(E_J("stiffen momentarily under %s gaze.",
				    "%s眼光で一瞬固まった。"),
				    s_suffix(mon_nam(mtmp)));
		    }
		    /* Technically this one shouldn't affect you at all because
		     * the Medusa gaze is an active monster attack that only
		     * works on the monster's turn, but for it to *not* have an
		     * effect would be too weird.
		     */
		    if (!DEADMONSTER(mtmp) &&
			    (mtmp->data == &mons[PM_MEDUSA]) && !mtmp->mcan) {
			pline(
			 E_J("Gazing at the awake %s is not a very good idea.",
			     "目覚めた%sを睨みつけるのはうまい考えとは言えない。" ),
			    l_monnam(mtmp));
			/* as if gazing at a sleeping anything is fruitful... */
			You(E_J("turn to stone...","石になった…。"));
			killer_format = KILLED_BY;
			killer = E_J("deliberately meeting Medusa's gaze",
				     "自分でメデューサと目を合わせて");
			done(STONING);
		    }
		}
	    }
	}
	if (!looked) You(E_J("gaze at no place in particular.",
			     "どこへともなく睨みをきかせた。"));
	return 1;
}

int
dohide()
{
	boolean ismimic = youmonst.data->mlet == S_MIMIC;

	if (u.uundetected || (ismimic && youmonst.m_ap_type != M_AP_NOTHING)) {
		You(E_J("are already hiding.","もう隠れている。"));
		return(0);
	}
	if (ismimic) {
		/* should bring up a dialog "what would you like to imitate?" */
		youmonst.m_ap_type = M_AP_OBJECT;
		youmonst.mappearance = STRANGE_OBJECT;
	} else
		u.uundetected = 1;
	newsym(u.ux,u.uy);
	return(1);
}

int
domindblast()
{
	struct monst *mtmp, *nmon;

	if (u.uen < 10) {
	    You(E_J("concentrate but lack the energy to maintain doing so.",
		    "精神を集中し続けるためのエネルギーを持っていなかった。"));
	    return(0);
	}
	u.uen -= 10;
	flags.botl = 1;

	You(E_J("concentrate.","精神集中した。"));
	pline(E_J("A wave of psychic energy pours out.",
		  "精神エネルギーの波が噴き出した。"));
	for(mtmp=fmon; mtmp; mtmp = nmon) {
		int u_sen;

		nmon = mtmp->nmon;
		if (DEADMONSTER(mtmp))
			continue;
		if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM)
			continue;
		if(mtmp->mpeaceful)
			continue;
		u_sen = telepathic(mtmp->data) && !mtmp->mcansee;
		if (u_sen || (telepathic(mtmp->data) && rn2(2)) || !rn2(10)) {
#ifndef JP
			You("lock in on %s %s.", s_suffix(mon_nam(mtmp)),
				u_sen ? "telepathy" :
				telepathic(mtmp->data) ? "latent telepathy" :
				"mind");
#else
			You("%sの%sを捉えた。", mon_nam(mtmp),
				u_sen ? "テレパシー" :
				telepathic(mtmp->data) ? "潜在テレパシー" :
				"精神");
#endif /*JP*/
			mtmp->mhp -= rnd(15);
			if (mtmp->mhp <= 0)
				killed(mtmp);
		}
	}
	return 1;
}

STATIC_OVL void
uunstick()
{
	pline(E_J("%s is no longer in your clutches.",
		  "%sはあなたの捕縛を逃れた。"), Monnam(u.ustuck));
	u.ustuck = 0;
}

void
skinback(silently)
boolean silently;
{
	if (uskin) {
		if (!silently) Your(E_J("skin returns to its original form.",
					"肌は元の姿に戻った。"));
		uarm = uskin;
		uskin = (struct obj *)0;
		/* undo save/restore hack */
		uarm->owornmask &= ~I_SPECIAL;
	}
}

#endif /* OVLB */
#ifdef OVL1

const char *
mbodypart(mon, part)
struct monst *mon;
int part;
{
	static NEARDATA const char
#ifndef JP
	*humanoid_parts[] = { "arm", "eye", "face", "finger",
		"fingertip", "foot", "hand", "handed", "head", "leg",
		"light headed", "neck", "spine", "toe", "hair",
		"blood", "lung", "nose", "stomach"},
	*jelly_parts[] = { "pseudopod", "dark spot", "front",
		"pseudopod extension", "pseudopod extremity",
		"pseudopod root", "grasp", "grasped", "cerebral area",
		"lower pseudopod", "viscous", "middle", "surface",
		"pseudopod extremity", "ripples", "juices",
		"surface", "sensor", "stomach" },
	*animal_parts[] = { "forelimb", "eye", "face", "foreclaw", "claw tip",
		"rear claw", "foreclaw", "clawed", "head", "rear limb",
		"light headed", "neck", "spine", "rear claw tip",
		"fur", "blood", "lung", "nose", "stomach" },
	*bird_parts[] = { "wing", "eye", "face", "wing", "wing tip",
		"foot", "wing", "winged", "head", "leg",
		"light headed", "neck", "spine", "toe",
		"feathers", "blood", "lung", "bill", "stomach" },
	*horse_parts[] = { "foreleg", "eye", "face", "forehoof", "hoof tip",
		"rear hoof", "foreclaw", "hooved", "head", "rear leg",
		"light headed", "neck", "backbone", "rear hoof tip",
		"mane", "blood", "lung", "nose", "stomach"},
	*sphere_parts[] = { "appendage", "optic nerve", "body", "tentacle",
		"tentacle tip", "lower appendage", "tentacle", "tentacled",
		"body", "lower tentacle", "rotational", "equator", "body",
		"lower tentacle tip", "cilia", "life force", "retina",
		"olfactory nerve", "interior" },
	*fungus_parts[] = { "mycelium", "visual area", "front", "hypha",
		"hypha", "root", "strand", "stranded", "cap area",
		"rhizome", "sporulated", "stalk", "root", "rhizome tip",
		"spores", "juices", "gill", "gill", "interior" },
	*vortex_parts[] = { "region", "eye", "front", "minor current",
		"minor current", "lower current", "swirl", "swirled",
		"central core", "lower current", "addled", "center",
		"currents", "edge", "currents", "life force",
		"center", "leading edge", "interior" },
	*snake_parts[] = { "vestigial limb", "eye", "face", "large scale",
		"large scale tip", "rear region", "scale gap", "scale gapped",
		"head", "rear region", "light headed", "neck", "length",
		"rear scale", "scales", "blood", "lung", "forked tongue", "stomach" },
	*fish_parts[] = { "fin", "eye", "premaxillary", "pelvic axillary",
		"pelvic fin", "anal fin", "pectoral fin", "finned", "head", "peduncle",
		"played out", "gills", "dorsal fin", "caudal fin",
		"scales", "blood", "gill", "nostril", "stomach" };
#else /*JP*/
	*humanoid_parts[] = { "腕", "目", "顔", "指",
		"指先", "足", "手", "手", "頭", "脚",
		"めまいがした", "首", "背骨", "つま先", "髪",
		"血", "肺", "鼻", "胃"},
	*jelly_parts[] = { "偽足", "眼点", "前面", "伸ばした偽足",
		"偽足の先端", "偽足", "偽足", "偽足", "中枢神経", "偽足",
		"苦痛に震えた", "中央", "表面", "偽足の先端", "波紋",
		"体液", "表面", "感覚器", "体内" },
	*animal_parts[] = { "前足", "目", "顔", "前爪", "爪の先",
		"後爪", "前爪", "引っ掛ける", "頭", "後足",
		"めまいがした", "首", "背骨", "後足の爪の先",
		"毛皮", "血", "肺", "鼻", "胃" },
	*bird_parts[] = { "翼", "目", "顔", "翼", "翼の先",
		"脚", "翼", "翼", "頭", "脚",
		"めまいがした", "首", "背骨", "つま先",
		"羽毛", "血", "肺", "くちばし", "胃" },
	*horse_parts[] = { "前足", "目", "顔", "前蹄", "蹄",
		"後蹄", "前爪", "蹄にはさむ", "頭", "後足",
		"めまいがした", "首", "背骨", "後足の蹄",
		"たてがみ", "血", "肺", "鼻", "胃"},
	*sphere_parts[] = { "突起", "視覚神経", "体", "触手",
		"触手の先端", "下部の突起", "触手", "触手に持つ",
		"体", "下部の触手", "回転した", "中心線", "体",
		"下部の触手の先端", "繊毛", "生命力", "網膜",
		"嗅覚神経", "内部" },
	*fungus_parts[] = { "菌糸体", "視覚領域", "前面", "菌糸",
		"菌糸", "根", "蔓", "蔓にからみつける", "傘",
		"根茎", "胞子を噴き出した", "軸", "根", "根茎の先端",
		"胞子", "樹液", "気孔", "気孔", "内部" },
	*vortex_parts[] = { "領域", "目", "前面", "弱い流れ",
		"弱い流れ", "下部の流れ", "渦巻", "渦巻く",
		"中心核", "下部の流れ", "乱れた", "中心部",
		"流れ", "外周", "流れ", "生命力",
		"中心", "前線", "内部" },
	*snake_parts[] = { "足の痕跡", "目", "顔", "大きな鱗",
		"大きな鱗の先", "後部", "鱗の隙間", "鱗の隙間にはさまった",
		"頭", "後部", "めまいがした", "首", "全身",
		"後部の鱗", "鱗", "血", "肺", "舌", "胃" },
	*fish_parts[] = { "ひれ", "目", "顔", "ひれの先",
		"ひれの先", "臀鰭", "胸鰭", "ひれに付けている", "頭", "尾部",
		"脱力した", "鰓", "背鰭", "尾鰭",
		"鱗", "血", "鰓", "鼻孔", "胃" };
#endif /*JP*/
	/* claw attacks are overloaded in mons[]; most humanoids with
	   such attacks should still reference hands rather than claws */
	static const char not_claws[] = {
		S_HUMAN, S_MUMMY, S_ZOMBIE, S_ANGEL,
		S_NYMPH, S_LEPRECHAUN, S_QUANTMECH, S_VAMPIRE,
		S_ORC, S_GIANT,		/* quest nemeses */
		'\0'		/* string terminator; assert( S_xxx != 0 ); */
	};
	struct permonst *mptr = mon->data;

#ifndef JP
	if (part == HAND || part == HANDED) {	/* some special cases */
	    if (mptr->mlet == S_DOG || mptr->mlet == S_FELINE ||
		    mptr->mlet == S_YETI)
		return part == HAND ? "paw" : "pawed";
	    if (humanoid(mptr) && attacktype(mptr, AT_CLAW) &&
		    !index(not_claws, mptr->mlet) &&
		    mptr != &mons[PM_STONE_GOLEM] &&
		    mptr != &mons[PM_INCUBUS] && mptr != &mons[PM_SUCCUBUS])
		return part == HAND ? "claw" : "clawed";
	}
	if ((mptr == &mons[PM_MUMAK] || mptr == &mons[PM_MASTODON]) &&
		part == NOSE)
	    return "trunk";
#endif /*JP*/
	if (mptr == &mons[PM_SHARK] && part == HAIR)
	    return E_J("skin","鮫肌");	/* sharks don't have scales */
	if (mptr == &mons[PM_JELLYFISH] && (part == ARM || part == FINGER ||
	    part == HAND || part == FOOT || part == TOE))
	    return E_J("tentacle","口腕");
	if (mptr == &mons[PM_FLOATING_EYE] && part == EYE)
	    return E_J("cornea","角膜");
	if (humanoid(mptr) &&
		(part == ARM || part == FINGER || part == FINGERTIP ||
		    part == HAND || part == HANDED))
	    return humanoid_parts[part];
	if (mptr == &mons[PM_RAVEN])
	    return bird_parts[part];
	if (mptr->mlet == S_CENTAUR || mptr->mlet == S_UNICORN ||
		(mptr == &mons[PM_ROTHE] && part != HAIR))
	    return horse_parts[part];
	if (mptr->mlet == S_LIGHT) {
		if (part == HANDED) return E_J("rayed","光線");
		else if (part == ARM || part == FINGER ||
				part == FINGERTIP || part == HAND) return E_J("ray","光線");
		else return E_J("beam","光線");
	}
	if (mptr->mlet == S_EEL && mptr != &mons[PM_JELLYFISH])
	    return fish_parts[part];
	if (slithy(mptr) || (mptr->mlet == S_DRAGON && part == HAIR))
	    return snake_parts[part];
	if (mptr->mlet == S_EYE)
	    return sphere_parts[part];
	if (mptr->mlet == S_JELLY || mptr->mlet == S_PUDDING ||
		mptr->mlet == S_BLOB || mptr == &mons[PM_JELLYFISH])
	    return jelly_parts[part];
	if (mptr->mlet == S_VORTEX || mptr->mlet == S_ELEMENTAL)
	    return vortex_parts[part];
	if (mptr->mlet == S_FUNGUS)
	    return fungus_parts[part];
	if (humanoid(mptr))
	    return humanoid_parts[part];
	return animal_parts[part];
}

const char *
body_part(part)
int part;
{
	return mbodypart(&youmonst, part);
}

#endif /* OVL1 */
#ifdef OVL0

int
poly_gender()
{
/* Returns gender of polymorphed player; 0/1=same meaning as flags.female,
 * 2=none.
 */
	if (is_neuter(youmonst.data) || !humanoid(youmonst.data)) return 2;
	return flags.female;
}

#endif /* OVL0 */
#ifdef OVLB

void
ugolemeffects(damtype, dam)
int damtype, dam;
{
	int heal = 0;
	/* We won't bother with "slow"/"haste" since players do not
	 * have a monster-specific slow/haste so there is no way to
	 * restore the old velocity once they are back to human.
	 */
	if (u.umonnum != PM_FLESH_GOLEM && u.umonnum != PM_IRON_GOLEM)
		return;
	switch (damtype) {
		case AD_ELEC: if (u.umonnum == PM_FLESH_GOLEM)
				heal = dam / 6; /* Approx 1 per die */
			break;
		case AD_FIRE: if (u.umonnum == PM_IRON_GOLEM)
				heal = dam;
			break;
	}
	if (heal && (u.mh < u.mhmax)) {
		u.mh += heal;
		if (u.mh > u.mhmax) u.mh = u.mhmax;
		flags.botl = 1;
		pline(E_J("Strangely, you feel better than before.",
			  "不思議なことに、あなたは前より元気になった。"));
		exercise(A_STR, TRUE);
	}
}

STATIC_OVL int
armor_to_dragon(atyp)
int atyp;
{
	if (atyp >= GRAY_DRAGON_SCALE_MAIL &&
	    atyp <= YELLOW_DRAGON_SCALE_MAIL) {
		return (PM_GRAY_DRAGON + atyp - GRAY_DRAGON_SCALE_MAIL);
	} else if (atyp >= GRAY_DRAGON_SCALES &&
		   atyp <= YELLOW_DRAGON_SCALES) {
		return (PM_GRAY_DRAGON + atyp - GRAY_DRAGON_SCALES);
	}
	return -1;
//	switch(atyp) {
//	    case GRAY_DRAGON_SCALE_MAIL:
//	    case GRAY_DRAGON_SCALES:
//		return PM_GRAY_DRAGON;
//	    case SILVER_DRAGON_SCALE_MAIL:
//	    case SILVER_DRAGON_SCALES:
//		return PM_SILVER_DRAGON;
//#if 0	/* DEFERRED */
//	    case SHIMMERING_DRAGON_SCALE_MAIL:
//	    case SHIMMERING_DRAGON_SCALES:
//		return PM_SHIMMERING_DRAGON;
//#endif
//	    case RED_DRAGON_SCALE_MAIL:
//	    case RED_DRAGON_SCALES:
//		return PM_RED_DRAGON;
//	    case ORANGE_DRAGON_SCALE_MAIL:
//	    case ORANGE_DRAGON_SCALES:
//		return PM_ORANGE_DRAGON;
//	    case WHITE_DRAGON_SCALE_MAIL:
//	    case WHITE_DRAGON_SCALES:
//		return PM_WHITE_DRAGON;
//	    case BLACK_DRAGON_SCALE_MAIL:
//	    case BLACK_DRAGON_SCALES:
//		return PM_BLACK_DRAGON;
//	    case BLUE_DRAGON_SCALE_MAIL:
//	    case BLUE_DRAGON_SCALES:
//		return PM_BLUE_DRAGON;
//	    case GREEN_DRAGON_SCALE_MAIL:
//	    case GREEN_DRAGON_SCALES:
//		return PM_GREEN_DRAGON;
//	    case PLAIN_DRAGON_SCALE_MAIL:
//	    case PLAIN_DRAGON_SCALES:
//		return PM_PLAIN_DRAGON;
//	    case YELLOW_DRAGON_SCALE_MAIL:
//	    case YELLOW_DRAGON_SCALES:
//		return PM_YELLOW_DRAGON;
//	    default:
//		return -1;
//	}
}

#endif /* OVLB */

/*polyself.c*/
