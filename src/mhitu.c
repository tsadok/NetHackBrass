/*	SCCS Id: @(#)mhitu.c	3.4	2003/11/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

STATIC_VAR NEARDATA struct obj *otmp;

STATIC_DCL void FDECL(urustm, (struct monst *, struct obj *));
# ifdef OVL1
STATIC_DCL boolean FDECL(u_slip_free, (struct monst *,struct attack *));
STATIC_DCL int FDECL(passiveum, (struct permonst *,struct monst *,struct attack *));
# endif /* OVL1 */

#ifdef OVLB
# ifdef SEDUCE
STATIC_DCL void FDECL(mayberem, (struct obj *, const char *));
# endif
#endif /* OVLB */

STATIC_DCL boolean FDECL(diseasemu, (struct permonst *));
STATIC_DCL int FDECL(hitmu, (struct monst *,struct attack *));
STATIC_DCL int FDECL(gulpmu, (struct monst *,struct attack *));
STATIC_DCL int FDECL(explmu, (struct monst *,struct attack *,BOOLEAN_P));
STATIC_DCL void FDECL(missmu,(struct monst *,BOOLEAN_P,struct attack *));
STATIC_DCL void FDECL(mswings,(struct monst *,struct obj *));
STATIC_DCL void FDECL(wildmiss, (struct monst *,struct attack *));

STATIC_DCL void FDECL(hitmsg,(struct monst *,struct attack *));
STATIC_DCL boolean NDECL(armor_cancel);

/* See comment in mhitm.c.  If we use this a lot it probably should be */
/* changed to a parameter to mhitu. */
static int dieroll;

static boolean instadeath;
static boolean polearms;

#ifdef OVL1


STATIC_OVL void
hitmsg(mtmp, mattk)
register struct monst *mtmp;
register struct attack *mattk;
{
	int compat;

	/* Note: if opposite gender, "seductively" */
	/* If same gender, "engagingly" for nymph, normal msg for others */
	if((compat = could_seduce(mtmp, &youmonst, mattk))
			&& !mtmp->mcan && !mtmp->mspec_used) {
#ifndef JP
		pline("%s %s you %s.", Monnam(mtmp),
			Blind ? "talks to" : "smiles at",
			compat == 2 ? "engagingly" : "seductively");
#else
		pline("%sはあなたに向かって%s%s。", Monnam(mtmp),
			compat == 2 ? "可愛らしく" : "誘うように",
			Blind ? "語りかけた" : "微笑んだ");
#endif /*JP*/
	} else
	    switch (mattk->aatyp) {
		case AT_BITE:
			pline(E_J("%s bites!","%sは噛みついた！"), Monnam(mtmp));
			break;
		case AT_KICK:
#ifndef JP
			pline("%s kicks%c", Monnam(mtmp),
				    thick_skinned(youmonst.data) ? '.' : '!');
#else
			pline("%sは蹴った%s", Monnam(mtmp),
				    thick_skinned(youmonst.data) ? "。" : "！");
#endif /*JP*/
			break;
		case AT_STNG:
			pline(E_J("%s stings!","%sは刺した！"), Monnam(mtmp));
			break;
		case AT_BUTT:
			pline(E_J("%s butts!","%sは突進した！"), Monnam(mtmp));
			break;
		case AT_TUCH:
			pline(E_J("%s touches you!","%sに触られた！"), Monnam(mtmp));
			break;
		case AT_TENT:
			pline(E_J("%s tentacles suck you!","%s触手が吸い付いた！"),
				        s_suffix(Monnam(mtmp)));
			break;
		case AT_EXPL:
		case AT_BOOM:
			pline(E_J("%s explodes!","%sは爆発した！"), Monnam(mtmp));
			break;
		default:
			pline(E_J("%s hits!","%sはあなたを攻撃した！"), Monnam(mtmp));
	    }
}

STATIC_OVL void
missmu(mtmp, nearmiss, mattk)		/* monster missed you */
register struct monst *mtmp;
register boolean nearmiss;
register struct attack *mattk;
{
	if (!canspotmons(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);

	if(could_seduce(mtmp, &youmonst, mattk) && !mtmp->mcan)
	    pline(E_J("%s pretends to be friendly.",
		      "%sは友好的なふりをした。"), Monnam(mtmp));
	else {
	    if (!flags.verbose || !nearmiss)
		pline(E_J("%s misses.","%sの攻撃は外れた。"), Monnam(mtmp));
	    else
		pline(E_J("%s just misses!","%sの攻撃があなたをかすめた！"), Monnam(mtmp));
	}
	stop_occupation();
}

STATIC_OVL void
mswings(mtmp, otemp)		/* monster swings obj */
register struct monst *mtmp;
register struct obj *otemp;
{
	if (!flags.verbose || Blind || !mon_visible(mtmp))
		return;
#ifndef JP
	pline("%s %s %s %s.", Monnam(mtmp),
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "thrusts" : "swings",
	      mhis(mtmp), singular(otemp, xname));
#else
	pline("%sは%sを%sた。", Monnam(mtmp), singular(otemp, xname),
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "突き出し" : "振り回し");
#endif /*JP*/
}

/* return how a poison attack was delivered */
const char *
mpoisons_subj(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
	if (mattk->aatyp == AT_WEAP) {
	    struct obj *mwep = (mtmp == &youmonst) ? uwep : MON_WEP(mtmp);
	    /* "Foo's attack was poisoned." is pretty lame, but at least
	       it's better than "sting" when not a stinging attack... */
	    return (!mwep || !mwep->opoisoned) ? E_J("attack","攻撃") : E_J("weapon","武器");
	} else {
	    return (mattk->aatyp == AT_TUCH) ? E_J("contact","接触") :
		   (mattk->aatyp == AT_GAZE) ? E_J("gaze","眼光") :
		   (mattk->aatyp == AT_BITE) ? E_J("bite","牙") : E_J("sting","針");
	}
}

/* called when your intrinsic speed is taken away */
void
u_slow_down()
{
	HFast = 0L;
	if (!Fast)
	    You(E_J("slow down.","身のこなしが遅くなった。"));
	else	/* speed boots */
	    Your(E_J("quickness feels less natural.",
		     "素早さは不自然になった。"));
	exercise(A_DEX, FALSE);
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL void
wildmiss(mtmp, mattk)		/* monster attacked your displaced image */
	register struct monst *mtmp;
	register struct attack *mattk;
{
	int compat;

	/* no map_invisible() -- no way to tell where _this_ is coming from */

	if (!flags.verbose) return;
	if (!cansee(mtmp->mx, mtmp->my)) return;
		/* maybe it's attacking an image around the corner? */

	compat = (mattk->adtyp == AD_SEDU || mattk->adtyp == AD_SSEX) &&
		 could_seduce(mtmp, &youmonst, (struct attack *)0);

	if (!mtmp->mcansee || (Invis && !perceives(mtmp->data))) {
	    const char *swings =
		mattk->aatyp == AT_BITE ? E_J("snaps","噛みつい") :
		mattk->aatyp == AT_KICK ? E_J("kicks","蹴っ") :
		(mattk->aatyp == AT_STNG ||
		 mattk->aatyp == AT_BUTT ||
		 nolimbs(mtmp->data)) ? E_J("lunges","突進し") : E_J("swings","振り回し");

	    if (compat)
		pline(E_J("%s tries to touch you and misses!",
			  "%sはあなたに触れようとして、空を切った！"), Monnam(mtmp));
	    else
		switch(rn2(3)) {
		case 0: pline(E_J("%s %s wildly and misses!",
				  "%sは荒々しく%sたが、外れた！"), Monnam(mtmp),
			      swings);
		    break;
		case 1: pline(E_J("%s attacks a spot beside you.",
				  "%sはあなたの横の空間を攻撃した。"), Monnam(mtmp));
		    break;
		case 2: pline(E_J("%s strikes at %s!",
				  "%sは何もない%sを打ち据えた！"), Monnam(mtmp),
				levl[mtmp->mux][mtmp->muy].typ == WATER
				    ? E_J("empty water","水中") : E_J("thin air","空間"));
		    break;
		default:pline(E_J("%s %s wildly!",
				  "%sは荒々しく%sた！"), Monnam(mtmp), swings);
		    break;
		}

	} else if (Displaced) {
	    if (compat)
#ifndef JP
		pline("%s smiles %s at your %sdisplaced image...",
			Monnam(mtmp),
			compat == 2 ? "engagingly" : "seductively",
			Invis ? "invisible " : "");
#else
		pline("%sはあなたの%s幻影に向かって%s微笑んだ…。",
			Monnam(mtmp), Invis ? "透明な" : "",
			compat == 2 ? "可愛らしく" : "誘うように");
#endif /*JP*/
	    else
		pline(E_J("%s strikes at your %sdisplaced image and misses you!",
			  "%sはあなたの%s幻影を攻撃した！"),
			/* Note: if you're both invisible and displaced,
			 * only monsters which see invisible will attack your
			 * displaced image, since the displaced image is also
			 * invisible.
			 */
			Monnam(mtmp),
			Invis ? E_J("invisible ","透明な") : "");

	} else if (Underwater) {
	    /* monsters may miss especially on water level where
	       bubbles shake the player here and there */
	    if (compat)
		pline(E_J("%s reaches towards your distorted image.",
			  "%sはあなたのゆらめく虚像に近づいていった。"),Monnam(mtmp));
	    else
		pline(E_J("%s is fooled by water reflections and misses!",
			  "%sは水に映ったあなたに惑わされ、目標を誤った！"),Monnam(mtmp));

	} else impossible("%s attacks you without knowing your location?",
		Monnam(mtmp));
}

void
expels(mtmp, mdat, message)
register struct monst *mtmp;
register struct permonst *mdat; /* if mtmp is polymorphed, mdat != mtmp->data */
boolean message;
{
	if (message) {
		if (is_animal(mdat))
			You(E_J("get regurgitated!","吐き戻された！"));
		else {
			char blast[40];
			register int i;

#ifndef JP
			blast[0] = '\0';
#else
			Strcpy(blast, "から");
#endif /*JP*/
			for(i = 0; i < NATTK; i++)
				if(mdat->mattk[i].aatyp == AT_ENGL)
					break;
			if (mdat->mattk[i].aatyp != AT_ENGL)
			      impossible("Swallower has no engulfing attack?");
			else {
				if (is_whirly(mdat)) {
					switch (mdat->mattk[i].adtyp) {
						case AD_ELEC:
							Strcpy(blast,
						      E_J(" in a shower of sparks",
							  "の火花の雨から"));
							break;
						case AD_COLD:
							Strcpy(blast,
							E_J(" in a blast of frost",
							    "の荒れ狂う霜の嵐から"));
							break;
					}
				} else
					Strcpy(blast, E_J(" with a squelch","から飛沫にまみれて"));
#ifndef JP
				You("get expelled from %s%s!",
				    mon_nam(mtmp), blast);
#else
				You("%s%s放り出された！",
				    mon_nam(mtmp), blast);
#endif /*JP*/
			}
		}
	}
	unstuck(mtmp);	/* ball&chain returned in unstuck() */
	mnexto(mtmp);
	newsym(u.ux,u.uy);
	spoteffects(TRUE);
	/* to cover for a case where mtmp is not in a next square */
	if(um_dist(mtmp->mx,mtmp->my,1))
		pline(E_J("Brrooaa...  You land hard at some distance.",
			  "あなたは吹っ飛ばされ、床へしたたかに打ちつけられた。"));
}

#endif /* OVLB */
#ifdef OVL0

/* select a monster's next attack, possibly substituting for its usual one */
struct attack *
getmattk(mptr, indx, prev_result, alt_attk_buf)
struct permonst *mptr;
int indx, prev_result[];
struct attack *alt_attk_buf;
{
    struct attack *attk = &mptr->mattk[indx];

    /* prevent a monster with two consecutive disease or hunger attacks
       from hitting with both of them on the same turn; if the first has
       already hit, switch to a stun attack for the second */
    if (indx > 0 && prev_result[indx - 1] > 0 &&
	    (attk->adtyp == AD_DISE ||
		attk->adtyp == AD_PEST ||
		attk->adtyp == AD_FAMN) &&
	    attk->adtyp == mptr->mattk[indx - 1].adtyp) {
	*alt_attk_buf = *attk;
	attk = alt_attk_buf;
	attk->adtyp = AD_STUN;
    }
    return attk;
}

/*
 * mattacku: monster attacks you
 *	returns 1 if monster dies (e.g. "yellow light"), 0 otherwise
 *	Note: if you're displaced or invisible the monster might attack the
 *		wrong position...
 *	Assumption: it's attacking you or an empty square; if there's another
 *		monster which it attacks by mistake, the caller had better
 *		take care of it...
 */
int
mattacku(mtmp)
	register struct monst *mtmp;
{
	struct	attack	*mattk, alt_attk;
	int	i, j, tmp, sum[NATTK];
	struct	permonst *mdat = mtmp->data;
	boolean ranged = (distu(mtmp->mx, mtmp->my) > 3);
		/* Is it near you?  Affects your actions */
	boolean range2 = !monnear(mtmp, mtmp->mux, mtmp->muy);
		/* Does it think it's near you?  Affects its actions */
	boolean foundyou = (mtmp->mux==u.ux && mtmp->muy==u.uy);
		/* Is it attacking you or your image? */
	boolean youseeit = canseemon(mtmp);
		/* Might be attacking your image around the corner, or
		 * invisible, or you might be blind....
		 */
	instadeath = FALSE;

	if(!ranged) nomul(0);
	if(mtmp->mhp <= 0 || (Underwater && !is_swimmer(mtmp->data)))
	    return(0);

	/* If swallowed, can only be affected by u.ustuck */
	if(u.uswallow) {
	    if(mtmp != u.ustuck)
		return(0);
	    u.ustuck->mux = u.ux;
	    u.ustuck->muy = u.uy;
	    range2 = 0;
	    foundyou = 1;
	    if(u.uinvulnerable) return (0); /* stomachs can't hurt you! */
	}

#ifdef STEED
	else if (u.usteed) {
		if (mtmp == u.usteed)
			/* Your steed won't attack you */
			return (0);
		/* Orcs like to steal and eat horses and the like */
		if (!rn2(is_orc(mtmp->data) ? 2 : 4) &&
				distu(mtmp->mx, mtmp->my) <= 2) {
			/* Attack your steed instead */
			i = mattackm(mtmp, u.usteed);
			if ((i & MM_AGR_DIED))
				return (1);
			if (i & MM_DEF_DIED || u.umoved)
				return (0);
			/* Let your steed retaliate */
			return (!!(mattackm(u.usteed, mtmp) & MM_DEF_DIED));
		}
	}
#endif

	if (u.uundetected && !range2 && foundyou && !u.uswallow) {
		u.uundetected = 0;
		if (is_hider(youmonst.data)) {
		    coord cc; /* maybe we need a unexto() function? */
		    struct obj *obj;

		    You(E_J("fall from the %s!","%sから落下した！"), ceiling(u.ux,u.uy));
		    if (enexto(&cc, u.ux, u.uy, youmonst.data)) {
			remove_monster(mtmp->mx, mtmp->my);
			newsym(mtmp->mx,mtmp->my);
			place_monster(mtmp, u.ux, u.uy);
			if(mtmp->wormno) worm_move(mtmp);
			teleds(cc.x, cc.y, TRUE);
			set_apparxy(mtmp);
			newsym(u.ux,u.uy);
		    } else {
			pline(E_J("%s is killed by a falling %s (you)!",
				  "%sは降ってきた%s(あなた)に殺された！"),
					Monnam(mtmp), E_J(youmonst.data->mname,JMONNAM(u.umonnum)));
			killed(mtmp);
			newsym(u.ux,u.uy);
			if (mtmp->mhp > 0) return 0;
			else return 1;
		    }
		    if (youmonst.data->mlet != S_PIERCER)
			return(0);	/* trappers don't attack */

		    obj = which_armor(mtmp, WORN_HELMET);
		    if (obj && is_metallic(obj)) {
			Your(E_J("blow glances off %s helmet.",
				 "突撃は%s兜に逸らされた。"),
			               s_suffix(mon_nam(mtmp)));
		    } else {
			if (3 + find_mac(mtmp) <= rnd(20)) {
			    pline(E_J("%s is hit by a falling piercer (you)!",
				      "%sは降ってきたピアサー(あなた)に打たれた！"),
								Monnam(mtmp));
			    if ((mtmp->mhp -= d(3,6)) < 1)
				killed(mtmp);
			} else
			  pline(E_J("%s is almost hit by a falling piercer (you)!",
				    "%sは危うく降ってきたピアサー(あなた)に当たるところだった！"),
								Monnam(mtmp));
		    }
		} else {
		    if (!youseeit)
			pline("It tries to move where you are hiding.");
		    else {
			/* Ugly kludge for eggs.  The message is phrased so as
			 * to be directed at the monster, not the player,
			 * which makes "laid by you" wrong.  For the
			 * parallelism to work, we can't rephrase it, so we
			 * zap the "laid by you" momentarily instead.
			 */
			struct obj *obj = level.objects[u.ux][u.uy];

			if (obj ||
			      (youmonst.data->mlet == S_EEL && is_pool(u.ux, u.uy))) {
			    int save_spe = 0; /* suppress warning */
			    if (obj) {
				save_spe = obj->spe;
				if (obj->otyp == EGG) obj->spe = 0;
			    }
			    if (youmonst.data->mlet == S_EEL)
#ifndef JP
		pline("Wait, %s!  There's a hidden %s named %s there!",
				m_monnam(mtmp), youmonst.data->mname, plname);
			    else
		pline("Wait, %s!  There's a %s named %s hiding under %s!",
				m_monnam(mtmp), youmonst.data->mname, plname,
				doname(level.objects[u.ux][u.uy]));
#else
		pline("待て、%s！ そこには%sという名の%sが潜んでいる！",
				m_monnam(mtmp), plname, JMONNAM(u.umonnum));
			    else
		pline("待て、%s！ そこの%sの下に%sという名の%sが潜んでいる！",
				m_monnam(mtmp),	doname(level.objects[u.ux][u.uy]),
				plname, JMONNAM(u.umonnum));
#endif /*JP*/
			    if (obj) obj->spe = save_spe;
			} else
			    impossible("hiding under nothing?");
		    }
		    newsym(u.ux,u.uy);
		}
		return(0);
	}
	if (youmonst.data->mlet == S_MIMIC && youmonst.m_ap_type &&
		    !range2 && foundyou && !u.uswallow) {
#ifndef JP
		if (!youseeit) pline("It gets stuck on you.");
		else pline("Wait, %s!  That's a %s named %s!",
			   m_monnam(mtmp), youmonst.data->mname, plname);
#else
		if (!youseeit) pline("何かがあなたに引っかかった。");
		else pline("待て、%s！ それは%sという名の%sだ！",
			   m_monnam(mtmp), plname, JMONNAM(u.umonnum));
#endif /*JP*/
		u.ustuck = mtmp;
		youmonst.m_ap_type = M_AP_NOTHING;
		youmonst.mappearance = 0;
		newsym(u.ux,u.uy);
		return(0);
	}

	/* player might be mimicking an object */
	if (youmonst.m_ap_type == M_AP_OBJECT && !range2 && foundyou && !u.uswallow) {
	    if (!youseeit)
		 pline(E_J("%s %s!","%sがあなた%sした！"), Something,
			(likes_gold(mtmp->data) && youmonst.mappearance == GOLD_PIECE) ?
			E_J("tries to pick you up","を拾い上げようと") :
			E_J("disturbs you","の邪魔を"));
#ifndef JP
	    else pline("Wait, %s!  That %s is really %s named %s!",
			m_monnam(mtmp),
			mimic_obj_name(&youmonst),
			an(mons[u.umonnum].mname),
			plname);
#else
	    else pline("待て、%s！ その%sは実は%sという名の%sだ！",
			m_monnam(mtmp), mimic_obj_name(&youmonst),
			plname, JMONNAM(u.umonnum));
#endif /*JP*/
	    if (multi < 0) {	/* this should always be the case */
		char buf[BUFSZ];
#ifndef JP
		Sprintf(buf, "You appear to be %s again.",
			Upolyd ? (const char *) an(youmonst.data->mname) :
			    (const char *) "yourself");
#else
		Sprintf(buf, "あなたは再び%sに化けた。",
			Upolyd ? JMONNAM(u.umonnum) : "自分");
#endif /*JP*/
		unmul(buf);	/* immediately stop mimicking */
	    }
	    return 0;
	}

/*	Work out the armor class differential	*/
	tmp = AC_VALUE(u.uac) + 10;		/* tmp ~= 0 - 20 */
	tmp += mtmp->m_lev;
	if(multi < 0) tmp += 4;
	if((Invis && !perceives(mdat)) || !mtmp->mcansee)
		tmp -= 2;
	if(mtmp->mtrapped) tmp -= 2;
	if(mtmp->mstun) tmp -= 4;
	if(tmp <= 0) tmp = 1;

	/* make eels visible the moment they hit/miss us */
	if(mdat->mlet == S_EEL && mtmp->minvis && cansee(mtmp->mx,mtmp->my)) {
		mtmp->minvis = 0;
		newsym(mtmp->mx,mtmp->my);
	}

/*	Special demon handling code */
	if(!mtmp->cham && is_demon(mdat) && !range2
	   && mtmp->data != &mons[PM_BALROG]
	   && mtmp->data != &mons[PM_SUCCUBUS]
	   && mtmp->data != &mons[PM_INCUBUS])
	    if(!mtmp->mcan && !rn2(13))	msummon(mtmp);

/*	Special lycanthrope handling code */
	if(!mtmp->cham && is_were(mdat) && !range2) {

	    if(is_human(mdat)) {
		if(!rn2(5 - (night() * 2)) && !mtmp->mcan) new_were(mtmp);
	    } else if(!rn2(30) && !mtmp->mcan) new_were(mtmp);
	    mdat = mtmp->data;

	    if(!rn2(10) && !mtmp->mcan) {
	    	int numseen, numhelp;
		char buf[BUFSZ], genericwere[BUFSZ];

		Strcpy(genericwere, E_J("creature","怪物"));
		numhelp = were_summon(mdat, FALSE, &numseen, genericwere);
		if (youseeit) {
			pline(E_J("%s summons help!",
				  "%sは助けを呼んだ！"), Monnam(mtmp));
			if (numhelp > 0) {
			    if (numseen == 0)
				You_feel(E_J("hemmed in.","囲まれた気がした。"));
			} else pline(E_J("But none comes.","しかし何も現れなった。"));
		} else {
			const char *from_nowhere;

			if (flags.soundok) {
#ifndef JP
				pline("%s %s!", Something,
					makeplural(growl_sound(mtmp)));
#else
				pline("%sが%s！", Something, growl_sound(mtmp));
#endif /*JP*/
				from_nowhere = "";
			} else from_nowhere = E_J(" from nowhere","何もないところから");
			if (numhelp > 0) {
			    if (numseen < 1) You_feel(E_J("hemmed in.","囲まれた気がした。"));
			    else {
#ifndef JP
				if (numseen == 1)
			    		Sprintf(buf, "%s appears",
							an(genericwere));
			    	else
			    		Sprintf(buf, "%s appear",
							makeplural(genericwere));
				pline("%s%s!", upstart(buf), from_nowhere);
#else
				pline("%sが%s現れた！", from_nowhere, genericwere);
#endif /*JP*/
			    }
			} /* else no help came; but you didn't know it tried */
		}
	    }
	}

	if(u.uinvulnerable) {
	    /* monsters won't attack you */
	    if(mtmp == u.ustuck)
		pline(E_J("%s loosens its grip slightly.",
			  "%sの絞めつけが少し緩んだ。"), Monnam(mtmp));
	    else if(!range2) {
		if (youseeit || sensemon(mtmp))
		    pline(E_J("%s starts to attack you, but pulls back.",
			      "%sはあなたに攻撃しようとしたが、引っ込めた。"),
			  Monnam(mtmp));
		else
		    You_feel(E_J("%s move nearby.",
			    "%sが近くで動いているようだ。"), something);
	    }
	    return (0);
	}

	/* Unlike defensive stuff, don't let them use item _and_ attack. */
	if(find_offensive(mtmp)) {
		int foo = use_offensive(mtmp);

		if (foo != 0) return(foo==1);
	}

	for(i = 0; i < NATTK; i++) {

	    polearms = FALSE;

	    sum[i] = 0;
	    mattk = getmattk(mdat, i, sum, &alt_attk);
	    if (u.uswallow && (mattk->aatyp != AT_ENGL))
		continue;
	    switch(mattk->aatyp) {
		case AT_CLAW:	/* "hand to hand" attacks */
		case AT_KICK:
		case AT_BITE:
		case AT_STNG:
		case AT_TUCH:
		case AT_BUTT:
		case AT_TENT:
			if(!range2 && (!MON_WEP(mtmp) || mtmp->mconf || Conflict ||
					!touch_petrifies(youmonst.data))) {
			    if (foundyou) {
				if(tmp > (j = rnd(20+i))) {
				    if (mattk->aatyp != AT_KICK ||
					    !thick_skinned(youmonst.data))
					sum[i] = hitmu(mtmp, mattk);
				} else
				    missmu(mtmp, (tmp == j), mattk);
			    } else
				wildmiss(mtmp, mattk);
			}
			break;

		case AT_HUGS:	/* automatic if prev two attacks succeed */
			/* Note: if displaced, prev attacks never succeeded */
			if((!range2 && i>=2 && sum[i-1] && sum[i-2])
							|| mtmp == u.ustuck)
				sum[i]= hitmu(mtmp, mattk);
			break;

		case AT_GAZE:	/* can affect you either ranged or not */
			/* Medusa gaze already operated through m_respond in
			 * dochug(); don't gaze more than once per round.
			 */
			if (mdat != &mons[PM_MEDUSA])
				sum[i] = gazemu(mtmp, mattk);
			break;

		case AT_EXPL:	/* automatic hit if next to, and aimed at you */
			if(!range2) sum[i] = explmu(mtmp, mattk, foundyou);
			break;

		case AT_ENGL:
			if (!range2) {
			    if(foundyou) {
				if(u.uswallow || tmp > (j = rnd(20+i))) {
				    /* Force swallowing monster to be
				     * displayed even when player is
				     * moving away */
				    flush_screen(1);
				    sum[i] = gulpmu(mtmp, mattk);
				} else {
				    missmu(mtmp, (tmp == j), mattk);
				}
			    } else if (is_animal(mtmp->data)) {
				pline(E_J("%s gulps some air!",
					  "%sは空気を飲み込んだ！"), Monnam(mtmp));
			    } else {
				if (youseeit)
				    pline(E_J("%s lunges forward and recoils!",
					      "%sはなだれを打って突進し、渦巻いた！"),
					  Monnam(mtmp));
				else
				    You_hear(E_J("a %s nearby.","すぐそばで%sを"),
					     is_whirly(mtmp->data) ?
						E_J("rushing noise","気流の轟音を") :
						E_J("splat","泥が跳ねる音を"));
			   }
			}
			break;
		case AT_BREA:
			if(range2) sum[i] = breamu(mtmp, mattk);
			/* Note: breamu takes care of displacement */
			break;
		case AT_SPIT:
			if(range2) sum[i] = spitmu(mtmp, mattk);
			/* Note: spitmu takes care of displacement */
			break;
		case AT_WEAP: {
			if(range2) {
#ifdef REINCARNATION
			    if (!Is_rogue_level(&u.uz))
#endif
				if (thrwmu(mtmp)) {
				    /* polearms */
				    int hitv;
				    hitv = 1 - distmin(u.ux,u.uy, mtmp->mx,mtmp->my);
				    if (hitv < -4) hitv = -4;
				    tmp += hitv;
				    polearms = TRUE; /* used by passiveum() */
				    goto weaponhit;
				}
			} else {
			    int hittmp;

			    /* Rare but not impossible.  Normally the monster
			     * wields when 2 spaces away, but it can be
			     * teleported or whatever....
			     */
			    if (mtmp->weapon_check == NEED_WEAPON ||
							!MON_WEP(mtmp)) {
				mtmp->weapon_check = NEED_HTH_WEAPON;
				/* mon_wield_item resets weapon_check as
				 * appropriate */
				if (mon_wield_item(mtmp) != 0) break;
			    }
			weaponhit:
			    hittmp = 0;
			    if (foundyou) {
				otmp = MON_WEP(mtmp);
				if(otmp) {
				    hittmp = hitval(otmp, &youmonst);
				    tmp += hittmp;
				    mswings(mtmp, otmp);
				}
				if(tmp > (j = dieroll = rnd(20+i)))
				    sum[i] = hitmu(mtmp, mattk);
				else
				    missmu(mtmp, (tmp == j), mattk);
				/* KMH -- Don't accumulate to-hit bonuses */
				if (otmp)
					tmp -= hittmp;
			    } else
				wildmiss(mtmp, mattk);
			}
			break;
		    }
		case AT_MAGC:
			if (range2)
			    sum[i] = buzzmu(mtmp, mattk);
			else {
			    if (foundyou)
				sum[i] = castmu(mtmp, mattk, TRUE, TRUE);
			    else
				sum[i] = castmu(mtmp, mattk, TRUE, FALSE);
			}
			break;

		default:		/* no attack */
			break;
	    }
	    if(flags.botl) bot();
	/* give player a chance of waking up before dying -kaa */
	    if(sum[i] == 1) {	    /* successful attack */
		if (u.usleep && u.usleep < monstermoves && !rn2(10)) {
		    multi = -1;
		    nomovemsg = E_J("The combat suddenly awakens you.",
				    "攻撃があなたを目覚めさせた。");
		}
	    }
	    if(sum[i] == 2) return 1;		/* attacker dead */
	    if(sum[i] == 3) break;  /* attacker teleported, no more attacks */
	    /* sum[i] == 0: unsuccessful attack */
	}
	return(0);
}

#endif /* OVL0 */
#ifdef OVLB

/*
 * helper function for some compilers that have trouble with hitmu
 */

void
hurtarmor(attk)
int attk;
{
	int	hurt;

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
		if (!uarmh || !rust_dmg(uarmh, xname(uarmh), hurt, FALSE, &youmonst))
			continue;
		break;
	    case 1:
		if (uarmc) {
		    (void)rust_dmg(uarmc, xname(uarmc), hurt, TRUE, &youmonst);
		    break;
		}
		/* Note the difference between break and continue;
		 * break means it was hit and didn't rust; continue
		 * means it wasn't a target and though it didn't rust
		 * something else did.
		 */
		if (uarm)
		    (void)rust_dmg(uarm, xname(uarm), hurt, TRUE, &youmonst);
#ifdef TOURIST
		else if (uarmu)
		    (void)rust_dmg(uarmu, xname(uarmu), hurt, TRUE, &youmonst);
#endif
		break;
	    case 2:
		if (!uarms || !rust_dmg(uarms, xname(uarms), hurt, FALSE, &youmonst))
		    continue;
		break;
	    case 3:
		if (!uarmg || !rust_dmg(uarmg, xname(uarmg), hurt, FALSE, &youmonst))
		    continue;
		break;
	    case 4:
		if (!uarmf || !rust_dmg(uarmf, xname(uarmf), hurt, FALSE, &youmonst))
		    continue;
		break;
	    }
	    break; /* Out of while loop */
	}
}

#endif /* OVLB */
#ifdef OVL1

STATIC_OVL boolean
diseasemu(mdat)
struct permonst *mdat;
{
	if (Sick_resistance) {
		You_feel(E_J("a slight illness.","ほんの少し体調が悪くなった気がした。"));
		return FALSE;
	} else {
//#ifdef JP
//		Sprintf(dkiller_buf, "%sに\tうつされた病気で", JMONNAM(monsndx(mdat)));
//#endif /*JP*/
		make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON), 20),
			E_J(mdat->mname, JMONNAM(monsndx(mdat))), TRUE, SICK_NONVOMITABLE);
		return TRUE;
	}
}

/* check whether slippery clothing protects from hug or wrap attack */
STATIC_OVL boolean
u_slip_free(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
	struct obj *obj = (uarmc ? uarmc : uarm);

#ifdef TOURIST
	if (!obj) obj = uarmu;
#endif
	if (mattk->adtyp == AD_DRIN) obj = uarmh;

	/* if your cloak/armor is greased, monster slips off; this
	   protection might fail (33% chance) when the armor is cursed */
	if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK) &&
		(!obj->cursed || rn2(3))) {
#ifndef JP
	    pline("%s %s your %s %s!",
		  Monnam(mtmp),
		  (mattk->adtyp == AD_WRAP) ?
			"slips off of" : "grabs you, but cannot hold onto",
		  obj->greased ? "greased" : "slippery",
		  /* avoid "slippery slippery cloak"
		     for undiscovered oilskin cloak */
		  (obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : cloak_simple_name(obj));
#else
	    pline("%sは%sとしたが、 あなたの%s%sで滑ってしまった！",
		  Monnam(mtmp),
		  (mattk->adtyp == AD_WRAP) ?
			"巻きつこう" : "あなたを掴もう",
		  obj->greased ? "脂の塗られた" : "つるつるした",
		  (obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : cloak_simple_name(obj));
#endif /*JP*/

	    if (obj->greased && !rn2(2)) {
		pline_The(E_J("grease wears off.","脂は消え落ちた。"));
		obj->greased = 0;
		update_inventory();
	    }
	    return TRUE;
	}
	return FALSE;
}

/* armor that sufficiently covers the body might be able to block magic */
int
magic_negation(mon)
struct monst *mon;
{
	const int acan[4] = { 0, 25, 50, 75 };
	struct obj *au, *aa, *ac, *ah, *af, *as, *ag;
	int armpro = 0;
	int tmp;

	if (mon == &youmonst) {
		aa = uarm;	ac = uarmc;	ag = uarmg;
		ah = uarmh;	af = uarmf;	as = uarms;
#ifdef TOURIST
		au = uarmu;
#endif
	} else {
		aa = which_armor(mon, W_ARM);
		ac = which_armor(mon, W_ARMC);
		ah = which_armor(mon, W_ARMH);
		ag = which_armor(mon, W_ARMG);
		af = which_armor(mon, W_ARMF);
		as = which_armor(mon, W_ARMS);
#ifdef TOURIST
		au = which_armor(mon, W_ARMU);
#endif
	}

	/* shields are good for defense */
	if (as && acan[objects[as->otyp].a_can] > rn2(100)) return TRUE;

	if (mon == &youmonst)
	/* special handling for ladies */
	if (flags.female && aa && aa->otyp == MAID_DRESS) {
		armpro = 50;
		if (ac && (ac->otyp == KITCHEN_APRON || ac->otyp == FRILLED_APRON))
			armpro += 25;
		if (ah && ah->otyp == KATYUSHA)
			armpro += 5;
	} else if (flags.female && aa && aa->otyp == NURSE_UNIFORM) {
		armpro = 75;
		if (ah && ah->otyp == NURSE_CAP)
			armpro += 20;
	}

	/* other armor */
	if (aa && armpro < (tmp = acan[objects[aa->otyp].a_can])) armpro = tmp;
	if (ac && armpro < (tmp = acan[objects[ac->otyp].a_can])) armpro = tmp;
	if (ah && armpro < (tmp = acan[objects[ah->otyp].a_can])) armpro = tmp;
	if (ag && armpro < (tmp = acan[objects[ag->otyp].a_can])) armpro = tmp;
	if (af && armpro < (tmp = acan[objects[af->otyp].a_can])) armpro = tmp;

	return (rn2(100) < armpro);
}

/*
 * hitmu: monster hits you
 *	  returns 2 if monster dies (e.g. "yellow light"), 1 otherwise
 *	  3 if the monster lives but teleported/paralyzed, so it can't keep
 *	       attacking you
 */
STATIC_OVL int
hitmu(mtmp, mattk)
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	register struct permonst *mdat = mtmp->data;
	register int uncancelled, ptmp;
	int dmg, permdmg;
	char	 buf[BUFSZ];
	struct permonst *olduasmon = youmonst.data;
	int res;
	int artihit = 0;
	int hitpart = DAMCAN_RANDOM;
	boolean shield = TRUE;

	if (!canspotmons(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);

/*	If the monster is undetected & hits you, you should know where
 *	the attack came from.
 */
	if(mtmp->mundetected && (hides_under(mdat) || mdat->mlet == S_EEL)) {
	    mtmp->mundetected = 0;
	    if (!(Blind ? Blind_telepat : Unblind_telepat)) {
		struct obj *obj;
		const char *what;

		if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0) {
		    if (Blind && !obj->dknown)
			what = something;
		    else if (is_pool(mtmp->mx, mtmp->my) && !Underwater)
			what = E_J("the water","水");
		    else
			what = doname(obj);

		    pline(E_J("%s was hidden under %s!","%sが%sの下に潜んでいる！"),
			    Amonnam(mtmp), what);
		}
		newsym(mtmp->mx, mtmp->my);
	    }
	}

/*	First determine the base damage done */
	dmg = d((int)mattk->damn, (int)mattk->damd);
	if(is_undead(mdat) && midnight())
		dmg += d((int)mattk->damn, (int)mattk->damd); /* extra damage */

/*	Next a cancellation factor	*/
/*	Use uncancelled when the cancellation factor takes into account certain
 *	armor's special magic protection.  Otherwise just use !mtmp->mcan.
 */
	uncancelled = !mtmp->mcan && !armor_cancel();

	permdmg = 0;
/*	Now, adjust damages via resistances or specific attacks */
	switch(mattk->adtyp) {
	    case AD_PHYS:
		if (mattk->aatyp == AT_HUGS && !sticks(youmonst.data)) {
		    if(!u.ustuck && rn2(2)) {
			if (u_slip_free(mtmp, mattk)) {
			    dmg = 0;
			} else {
			    u.ustuck = mtmp;
			    pline(E_J("%s grabs you!","%sはあなたをつかんだ！"), Monnam(mtmp));
			}
		    } else if(u.ustuck == mtmp) {
			exercise(A_STR, FALSE);
			You(E_J("are being %s.","%sれている。"),
			      (mtmp->data == &mons[PM_ROPE_GOLEM])
			      ? E_J("choked","締め上げら") : E_J("crushed","押し潰さ"));
		    }
		} else {			  /* hand to hand weapon */
		    if(mattk->aatyp == AT_WEAP && otmp) {
			if (otmp->otyp == CORPSE
				&& touch_petrifies(&mons[otmp->corpsenm])) {
			    dmg = 1;
#ifndef JP
			    pline("%s hits you with the %s corpse.",
				Monnam(mtmp), mons[otmp->corpsenm].mname);
#else
			    pline("%sはあなたを%sの死体で殴りつけた。",
				Monnam(mtmp), JMONNAM(otmp->corpsenm));
#endif /*JP*/
			    if (!Stoned)
				goto do_stone;
			}
			dmg += dmgval(otmp, &youmonst);
			if (dmg <= 0) dmg = 1;
			if (!(otmp->oartifact &&
				(artihit = artifact_hit(mtmp, &youmonst, otmp, &dmg,dieroll)))) {
			    if (dieroll == 2 && is_hammer(otmp) &&
				has_head(youmonst.data) && !mindless(youmonst.data)) {
#ifndef JP
				pline("You %s from %s %sstrike!",
				      makeplural(stagger(youmonst.data, "stagger")),
				      s_suffix(mon_nam(mtmp)),
				      (dmg > 9) ? "powerful " : "");
#else
				You("%sの%s殴打で%s！",
				    mon_nam(mtmp), (dmg > 9) ? "強力な" : "",
				    stagger(youmonst.data, "よろめいた"));
#endif /*JP*/
				make_stunned(HStun + rnd(4), FALSE);
			    } else
				hitmsg(mtmp, mattk);
			}
			if (artihit && dmg > FATAL_DAMAGE_MODIFIER) instadeath = TRUE;
			if (!dmg) break;
			if (u.mh > 1 && u.mh > ((u.uac>0) ? dmg : dmg+u.uac) &&
				   get_material(otmp) == IRON &&
					(u.umonnum==PM_BLACK_PUDDING
					|| u.umonnum==PM_BROWN_PUDDING)) {
			    /* This redundancy necessary because you have to
			     * take the damage _before_ being cloned.
			     */
			    if (u.uac < 0) dmg += u.uac;
			    if (dmg < 1) dmg = 1;
			    if (dmg > 1) exercise(A_STR, FALSE);
			    u.mh -= dmg;
			    flags.botl = 1;
			    dmg = 0;
			    if(cloneu())
			    You(E_J("divide as %s hits you!",
				    "%sの攻撃で分裂した！"), mon_nam(mtmp));
			}
			urustm(mtmp, otmp);
		    } else if (mattk->aatyp != AT_TUCH || dmg != 0 ||
				mtmp != u.ustuck)
			hitmsg(mtmp, mattk);
		}
		break;
	    case AD_DISE:
		if (uncancelled || mdat == &mons[PM_PESTILENCE]) {
		    hitmsg(mtmp, mattk);
		    if (!diseasemu(mdat)) dmg = 0;
		} else dmg = 0;
		break;
	    case AD_FIRE:
		hitmsg(mtmp, mattk);
//		if (uncancelled) {
		    pline(E_J("You're %s!","あなた%s！"), on_fire(youmonst.data, mattk));
		    if (youmonst.data == &mons[PM_STRAW_GOLEM] ||
		        youmonst.data == &mons[PM_PAPER_GOLEM]) {
			    You("roast!");
			    /* KMH -- this is okay with unchanging */
			    rehumanize();
			    break;
		    } else if (Fire_resistance) {
			if (is_full_resist(FIRE_RES)) {
			    pline_The(E_J("fire doesn't feel hot!","炎は熱くない！"));
			    dmg = 0;
			} else {
			    pline_The(E_J("fire feels slightly hot.",
					  "あなたは熱に耐えている。"));
			    dmg = (dmg + 3) / 4;
			}
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(SCROLL_CLASS, AD_FIRE);
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_FIRE);
		    if((int) mtmp->m_lev > rn2(25))
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(TOOL_CLASS, AD_FIRE);
		    burn_away_slime();
//		} else dmg = 0;
		break;
	    case AD_COLD:
		hitmsg(mtmp, mattk);
//		if (uncancelled) {
		    pline(E_J("You're covered in frost!",
			      "あなたは霜に覆われた！"));
		    if (Cold_resistance) {
			if (is_full_resist(COLD_RES)) {
			    pline_The(E_J("frost doesn't seem cold!",
					  "霜は冷たくない！"));
			    dmg = 0;
			} else {
			    pline_The(E_J("frost freezes you a bit.",
					  "あなたは冷気に耐えている。"));
			    dmg = (dmg + 3) / 4;
			}
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_COLD);
//		} else dmg = 0;
		break;
	    case AD_ELEC:
		hitmsg(mtmp, mattk);
//		if (uncancelled) {
		    if (elem_hits_shield(&youmonst, AD_ELEC, E_J("zap", "電撃"))) {
			dmg = 0;
			break;
		    }
		    You(E_J("get zapped!","感電した！"));
		    if (Shock_resistance) {
			if (is_full_resist(SHOCK_RES)) {
			    pline_The(E_J("zap doesn't shock you!",
					  "あなたは電撃の影響を受けない！"));
			    dmg = 0;
			} else {
			    pline_The(E_J("zap shocks you a bit.",
					  "あなたは電撃に耐えた。"));
			    dmg = (dmg + 3) / 4;
			}
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(WAND_CLASS, AD_ELEC);
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(RING_CLASS, AD_ELEC);
//		} else dmg = 0;
		break;
	    case AD_SLEE:
		hitmsg(mtmp, mattk);
		if (uncancelled && multi >= 0 && !rn2(5)) {
		    if (Sleep_resistance) break;
		    fall_asleep(-rnd(10), TRUE);
		    if (Blind) You(E_J("are put to sleep!","眠らされた！"));
		    else You(E_J("are put to sleep by %s!",
				 "%sに眠らされた！"), mon_nam(mtmp));
		}
		break;
	    case AD_BLND:
		if (can_blnd(mtmp, &youmonst, mattk->aatyp, (struct obj*)0)) {
		    if (!Blind) pline(E_J("%s blinds you!",
					  "%sはあなたの目を突いた！"), Monnam(mtmp));
		    make_blinded(Blinded+(long)dmg,FALSE);
		    if (!Blind) Your(vision_clears);
		}
		dmg = 0;
		break;
	    case AD_DRST:
		ptmp = A_STR;
		goto dopois;
	    case AD_DRDX:
		ptmp = A_DEX;
		goto dopois;
	    case AD_DRCO:
		ptmp = A_CON;
dopois:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(8)) {
#ifndef JP
		    Sprintf(buf, "%s %s",
			    s_suffix(Monnam(mtmp)), mpoisons_subj(mtmp, mattk));
		    poisoned(buf, ptmp, mdat->mname, 30);
#else
		    char buf2[BUFSZ];
		    Sprintf(buf, "%sの%s",
			    mon_nam(mtmp), mpoisons_subj(mtmp, mattk));
		    Sprintf(buf2, "%sに殺された", JMONNAM(monsndx(mdat)));
		    poisoned(buf, ptmp, buf2, 30);
#endif /*JP*/
		}
		break;
	    case AD_DRIN:
		hitmsg(mtmp, mattk);
		if (defends(AD_DRIN, uwep) || !has_head(youmonst.data)) {
		    You(E_J("don't seem harmed.",
			    "傷つかなかったようだ。"));
		    /* Not clear what to do for green slimes */
		    break;
		}
		if (u_slip_free(mtmp,mattk)) break;

		hitpart = DAMCAN_HEAD;	/* known where to be hit */
		if (uarmh && rn2(8)) {
		    /* not body_part(HEAD) */
		    Your(E_J("%s blocks the attack to your head.",
			     "%sが頭を攻撃から守った。"), helm_simple_name(uarmh));
		    break;
		}
		if (Half_physical_damage) dmg = (dmg+1) / 2;
		mdamageu(mtmp, dmg);

		if (!uarmh || uarmh->otyp != DUNCE_CAP) {
		    Your(E_J("brain is eaten!","脳が喰われた！"));
		    /* No such thing as mindless players... */
		    if (ABASE(A_INT) <= ATTRMIN(A_INT)) {
			int lifesaved = 0;
			struct obj *wore_amulet = uamul;

			while(1) {
			    /* avoid looping on "die(y/n)?" */
			    if (lifesaved && (discover || wizard)) {
				if (wore_amulet && !uamul) {
				    /* used up AMULET_OF_LIFE_SAVING; still
				       subject to dying from brainlessness */
				    wore_amulet = 0;
				} else {
				    /* explicitly chose not to die;
				       arbitrarily boost intelligence */
				    ABASE(A_INT) = ATTRMIN(A_INT) + 2;
				    You_feel(E_J("like a scarecrow.",
						 "案山子みたいな気分だ。"));
				    break;
				}
			    }

			    if (lifesaved)
				pline(E_J("Unfortunately your brain is still gone.",
					  "残念ながら、あなたは脳を失くしたままだ。"));
			    else
				Your(E_J("last thought fades away.",
					 "最後の思考が薄れてゆき、消えた。"));
			    killer = E_J("brainlessness","脳を失って");
			    killer_format = KILLED_BY;
			    done(DIED);
			    lifesaved++;
			}
		    }
		}
		/* adjattrib gives dunce cap message when appropriate */
		(void) adjattrib(A_INT, -rnd(2), FALSE);
		forget_levels(25);	/* lose memory of 25% of levels */
		forget_objects(25);	/* lose memory of 25% of objects */
		exercise(A_WIS, FALSE);
		break;
	    case AD_PLYS:
		hitmsg(mtmp, mattk);
		if (uncancelled && multi >= 0 && !rn2(3)) {
		    if (Free_action) {
			You(E_J("momentarily stiffen.",
				"一瞬動けなくなった。"));
		    } else {
			if (Blind) You(E_J("are frozen!","動けなくなった！"));
			else You(E_J("are frozen by %s!",
				     "%sに麻痺させられた！"), mon_nam(mtmp));
			nomovemsg = 0;	/* default: "you can move again" */
			nomul(-rnd(10));
			exercise(A_DEX, FALSE);
		    }
		}
		break;
	    case AD_DRLI:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(3) && !Drain_resistance) {
		    losexp(E_J("life drainage","生命力を奪われて"));
		}
		break;
	    case AD_LEGS:
		{ register long side = rn2(2) ? RIGHT_SIDE : LEFT_SIDE;
		  const char *sidestr = (side == RIGHT_SIDE) ? E_J("right","右") : E_J("left","左");

		/* This case is too obvious to ignore, but Nethack is not in
		 * general very good at considering height--most short monsters
		 * still _can_ attack you when you're flying or mounted.
		 * [FIXME: why can't a flying attacker overcome this?]
		 */
		  hitpart = DAMCAN_FEET;	/* known where to be hit */
		  if (
#ifdef STEED
			u.usteed ||
#endif
				    Levitation || Flying) {
		    pline(E_J("%s tries to reach your %s %s!",
			      "%sはあなたの%s%sに近寄ろうとした！"), Monnam(mtmp),
			  sidestr, body_part(LEG));
		    dmg = 0;
		  } else if (mtmp->mcan) {
		    pline(E_J("%s nuzzles against your %s %s!",
			      "%sはあなたの%s%sに擦り寄った！"), Monnam(mtmp),
			  sidestr, body_part(LEG));
		    dmg = 0;
		  } else {
		    if (uarmf) {
			if (rn2(2) && (uarmf->otyp == LOW_BOOTS ||
					     uarmf->otyp == IRON_SHOES))
			    pline(E_J("%s pricks the exposed part of your %s %s!",
				      "%sはあなたの%s%sの露出部を刺した！"),
				Monnam(mtmp), sidestr, body_part(LEG));
			else if (!rn2(5))
			    pline(E_J("%s pricks through your %s boot!",
				      "%sはあなたの%sのブーツを突き破り、刺した！"),
				Monnam(mtmp), sidestr);
			else {
			    pline(E_J("%s scratches your %s boot!",
				      "%sはあなたの%sのブーツに掻き傷をつけた！"), Monnam(mtmp),
				sidestr);
			    dmg = 0;
			    break;
			}
		    } else pline(E_J("%s pricks your %s %s!",
				     "%sはあなたの%s%sを刺した！"), Monnam(mtmp),
			  sidestr, body_part(LEG));
		    set_wounded_legs(side, rnd(60-ACURR(A_DEX)));
		    exercise(A_STR, FALSE);
		    exercise(A_DEX, FALSE);
		  }
		  break;
		}
	    case AD_STON:	/* cockatrice */
		hitmsg(mtmp, mattk);
		if(!rn2(3)) {
		    if (mtmp->mcan) {
			if (flags.soundok)
			    You_hear(E_J("a cough from %s!","!%sが咳き込むのを"), mon_nam(mtmp));
		    } else {
			if (flags.soundok)
#ifndef JP
			    You_hear("%s hissing!", s_suffix(mon_nam(mtmp)));
#else
			    You_hear("!%sがシューッと唸るのを", mon_nam(mtmp));
#endif /*JP*/
			if(uncancelled/*!rn2(10)*/ ||
			    (flags.moonphase == NEW_MOON && !have_lizard())) {
 do_stone:
			    if (!Stoned && !Stone_resistance
				    && !(poly_when_stoned(youmonst.data) &&
					polymon(PM_STONE_GOLEM))) {
				Stoned = 5;
#ifndef JP
				delayed_killer = mtmp->data->mname;

				if (mtmp->data->geno & G_UNIQ) {
				    if (!type_is_pname(mtmp->data)) {
					/* "the" buffer may be reallocated */
					Strcpy(dkiller_buf, the(delayed_killer));
					delayed_killer = dkiller_buf;
				    }
				    killer_format = KILLED_BY;
				} else
				    killer_format = KILLED_BY_AN;
#else
				Sprintf(dkiller_buf, "%sの攻撃で", JMONNAM(monsndx(mtmp->data)));
				delayed_killer = dkiller_buf;
				killer_format = KILLED_BY;
#endif /*JP*/
				return(1);
				/* You("turn to stone..."); */
				/* done_in_by(mtmp); */
			    }
			}
		    }
		}
		break;
	    case AD_STCK:
		hitmsg(mtmp, mattk);
		if (uncancelled && !u.ustuck && !sticks(youmonst.data))
			u.ustuck = mtmp;
		break;
	    case AD_WRAP:
		if ((!mtmp->mcan || u.ustuck == mtmp) && !sticks(youmonst.data)) {
		    if (!u.ustuck && !rn2(10)) {
			if (u_slip_free(mtmp, mattk)) {
			    dmg = 0;
			} else {
			    pline(E_J("%s swings itself around you!",
				      "%sはあなたに巻きついた！"),
				  Monnam(mtmp));
			    u.ustuck = mtmp;
			}
		    } else if(u.ustuck == mtmp) {
			if (is_pool(mtmp->mx,mtmp->my) && !Swimming
			    && !Amphibious) {
			    boolean moat =
				(levl[mtmp->mx][mtmp->my].typ != POOL) &&
				(levl[mtmp->mx][mtmp->my].typ != WATER) &&
				!Is_medusa_level(&u.uz) &&
				!Is_waterlevel(&u.uz);

			    pline(E_J("%s drowns you...",
				      "%sはあなたを溺れさせた…。"), Monnam(mtmp));
			    killer_format = KILLED_BY_AN;
#ifndef JP
			    Sprintf(buf, "%s by %s",
				    moat ? "moat" : "pool of water",
				    an(mtmp->data->mname));
#else
			    Sprintf(buf, "%sに%sに引きずり込まれて",
				    JMONNAM(monsndx(mtmp->data)),
				    moat ? "堀" : "水中");
#endif /*JP*/
			    killer = buf;
			    done(DROWNING);
			} else if(mattk->aatyp == AT_HUGS)
			    You(E_J("are being crushed.","押し潰されている。"));
		    } else {
			dmg = 0;
			if(flags.verbose)
			    pline(E_J("%s brushes against your %s.",
				      "%sはあなたの%sをかすった。"), Monnam(mtmp),
				   body_part(LEG));
		    }
		} else dmg = 0;
		break;
	    case AD_WERE:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(6/*4*/) && u.ulycn == NON_PM &&
			!Protection_from_shape_changers &&
			!defends(AD_WERE,uwep)) {
		    You_feel(E_J("feverish.","熱っぽくなった。"));
		    exercise(A_CON, FALSE);
		    u.ulycn = monsndx(mdat);
		}
		break;
	    case AD_SGLD:
		hitmsg(mtmp, mattk);
		if (youmonst.data->mlet == mdat->mlet) break;
		if(!mtmp->mcan) stealgold(mtmp);
		break;

	    case AD_SITM:	/* for now these are the same */
	    case AD_SEDU:
		if (is_animal(mtmp->data)) {
			hitmsg(mtmp, mattk);
			if (mtmp->mcan) break;
			/* Continue below */
		} else if (dmgtype(youmonst.data, AD_SEDU)
#ifdef SEDUCE
			|| dmgtype(youmonst.data, AD_SSEX)
#endif
						) {
			pline(E_J("%s %s.","%sは%s。"), Monnam(mtmp), mtmp->minvent ?
		E_J("brags about the goods some dungeon explorer provided",
		    "迷宮探検者が提供してくれた数々の品を自慢した。") :
		E_J("makes some remarks about how difficult theft is lately",
		    "このごろ窃盗がいかに難しいかについて私見を述べた。"));
			if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
			return 3;
		} else if (mtmp->mcan) {
		    if (!Blind) {
#ifndef JP
			pline("%s tries to %s you, but you seem %s.",
			    Adjmonnam(mtmp, "plain"),
			    flags.female ? "charm" : "seduce",
			    flags.female ? "unaffected" : "uninterested");
#else
			pline("%sはあなたを%sしようとしたが、あなたは%sなかった。",
			    Adjmonnam(mtmp, "不器量な"),
			    flags.female ? "魅了" : "誘惑",
			    flags.female ? "心動かされ" : "興味が持て");
#endif /*JP*/
		    }
		    if(rn2(3)) {
			if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
			return 3;
		    }
		    break;
		}
		buf[0] = '\0';
		switch (steal(mtmp, buf)) {
		  case -1:
			return 2;
		  case 0:
			break;
		  default:
			if (!is_animal(mtmp->data) && !tele_restrict(mtmp))
			    (void) rloc(mtmp, FALSE);
			if (is_animal(mtmp->data) && *buf) {
			    if (canseemon(mtmp))
#ifndef JP
				pline("%s tries to %s away with %s.",
				      Monnam(mtmp),
				      locomotion(mtmp->data, "run"),
				      buf);
#else
				pline("%sは%sを持って逃げ出した。",
				      Monnam(mtmp), buf);
#endif /*JP*/
			}
			monflee(mtmp, 0, FALSE, FALSE);
			return 3;
		}
		break;
#ifdef SEDUCE
	    case AD_SSEX:
		if(could_seduce(mtmp, &youmonst, mattk) == 1
			&& !mtmp->mcan)
		    if (doseduce(mtmp))
			return 3;
		break;
#endif
	    case AD_SAMU:
		hitmsg(mtmp, mattk);
		/* when the Wiz hits, 1/20 steals the amulet */
		if (u.uhave.amulet ||
		     u.uhave.bell || u.uhave.book || u.uhave.menorah
		     || u.uhave.questart) /* carrying the Quest Artifact */
		    if (!rn2(20)) stealamulet(mtmp);
		break;

	    case AD_TLPT:
		hitmsg(mtmp, mattk);
		if (uncancelled) {
		    if(flags.verbose)
			Your(E_J("position suddenly seems very uncertain!",
				 "位置は突然とても不確定になったようだ！"));
		    tele();
		}
		break;
	    case AD_RUST:
		hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		if (u.umonnum == PM_IRON_GOLEM) {
			You(E_J("rust!","錆びた！"));
			/* KMH -- this is okay with unchanging */
			rehumanize();
			break;
		}
		hurtarmor(AD_RUST);
		break;
	    case AD_CORR:
		hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		hurtarmor(AD_CORR);
		break;
	    case AD_DCAY:
		hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		if (u.umonnum == PM_WOOD_GOLEM ||
		    u.umonnum == PM_LEATHER_GOLEM) {
			You(E_J("rot!","腐った！"));
			/* KMH -- this is okay with unchanging */
			rehumanize();
			break;
		}
		hurtarmor(AD_DCAY);
		break;
	    case AD_HEAL:
		/* a cancelled nurse is just an ordinary monster */
		if (mtmp->mcan) {
		    hitmsg(mtmp, mattk);
		    break;
		}
		if(!uwep
#ifdef TOURIST
		   && !uarmu
#endif
		   && !uarm && !uarmh && !uarms && !uarmg && !uarmc && !uarmf) {
		    boolean goaway = FALSE;
		    pline(E_J("%s hits!  (I hope you don't mind.)",
			      "%sは叩いた！(気にしないで下さい)"), Monnam(mtmp));
		    if (Upolyd) {
			u.mh += rnd(7);
			if (!rn2(7)) {
			    /* no upper limit necessary; effect is temporary */
			    u.mhmax++;
			    if (!rn2(13)) goaway = TRUE;
			}
			if (u.mh > u.mhmax) u.mh = u.mhmax;
		    } else {
			u.uhp += rnd(7);
			if (!rn2(7)) {
			    /* hard upper limit via nurse care: 25 * ulevel */
			    if (u.uhpmax < 5 * u.ulevel + d(2 * u.ulevel, 10))
				addhpmax(1);
			    if (!rn2(13)) goaway = TRUE;
			}
			if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
		    }
		    if (!rn2(3)) exercise(A_STR, TRUE);
		    if (!rn2(3)) exercise(A_CON, TRUE);
		    if (Sick) make_sick(0L, (char *) 0, FALSE, SICK_ALL);
		    flags.botl = 1;
		    if (goaway) {
			mongone(mtmp);
			return 2;
		    } else if (!rn2(33)) {
			if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
			monflee(mtmp, d(3, 6), TRUE, FALSE);
			return 3;
		    }
		    dmg = 0;
		} else {
		    if (Role_if(PM_HEALER)) {
			if (flags.soundok && !(moves % 5))
		      verbalize(E_J("Doc, I can't help you unless you cooperate.",
				    "先生、ご協力いただかないとお助けできません。"));
			dmg = 0;
		    } else hitmsg(mtmp, mattk);
		}
		break;
	    case AD_CURS:
		hitmsg(mtmp, mattk);
		if(!night() && mdat == &mons[PM_GREMLIN]) break;
		if(!mtmp->mcan && !rn2(10)) {
		    if (flags.soundok) {
			if (Blind) You_hear(E_J("laughter.","笑い声を"));
			else       pline(E_J("%s chuckles.","%sは薄笑いを浮かべた。"), Monnam(mtmp));
		    }
		    if (u.umonnum == PM_CLAY_GOLEM) {
			pline(E_J("Some writing vanishes from your head!",
				  "あなたの額に書かれた何かの文字が消えた！"));
			/* KMH -- this is okay with unchanging */
			rehumanize();
			break;
		    }
		    attrcurse();
		}
		break;
	    case AD_STUN:
		hitmsg(mtmp, mattk);
		if(!mtmp->mcan && !rn2(4)) {
		    make_stunned(HStun + dmg, TRUE);
		    dmg /= 2;
		}
		break;
	    case AD_ACID:
		hitmsg(mtmp, mattk);
		if (!mtmp->mcan && !rn2(3))
		    if (Acid_resistance) {
			pline(E_J("You're covered in acid, but it seems harmless.",
				  "あなたは酸に覆われたが、傷つかないようだ。"));
			dmg = 0;
		    } else {
			pline(E_J("You're covered in acid!  It burns!",
				  "あなたは酸に覆われた！ 身体が灼ける！"));
			exercise(A_STR, FALSE);
		    }
		else		dmg = 0;
		break;
	    case AD_SLOW:
		hitmsg(mtmp, mattk);
		if (uncancelled && HFast &&
					!defends(AD_SLOW, uwep) && !rn2(4))
		    u_slow_down();
		break;
	    case AD_DREN:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(4))
		    drain_en(dmg);
		dmg = 0;
		break;
	    case AD_CONF:
		hitmsg(mtmp, mattk);
		if(!mtmp->mcan && !rn2(4) && !mtmp->mspec_used) {
		    mtmp->mspec_used = mtmp->mspec_used + (dmg + rn2(6));
		    if(Confusion)
			 You(E_J("are getting even more confused.",
				 "ますます混乱した。"));
		    else You(E_J("are getting confused.","混乱した。"));
		    make_confused(HConfusion + dmg, FALSE);
		}
		dmg = 0;
		break;
	    case AD_DETH:
		pline(E_J("%s reaches out with its deadly touch.",
			  "%sは死の力を秘めた腕をあなたに伸ばした。"), Monnam(mtmp));
		if (is_undead(youmonst.data)) {
		    /* Still does normal damage */
		    pline(E_J("Was that the touch of death?",
			      "これが死の接触？"));
		    break;
		} else if (vs_death_factor(youmonst.data)) {
		    You(E_J("seem unaffected.","影響を受けないようだ。"));
		    break;
		} else if (Drain_resistance) {
		    You(E_J("stand against it!","恐るべき力に耐えている！"));
		    break;
		}
		switch (rn2(20)) {
		case 19: case 18: case 17:
		    if (!Antimagic) {
			killer_format = KILLED_BY_AN;
			killer = E_J("touch of death","死の接触で");
			done(DIED);
			dmg = 0;
			break;
		    } /* else FALLTHRU */
		default: /* case 16: ... case 5: */
		    You_feel(E_J("your life force draining away...",
				 "あなたの生命力が流れ去っていく…。"));
		    permdmg = 1;	/* actual damage done below */
		    break;
		case 4: case 3: case 2: case 1: case 0:
		    if (Antimagic) shieldeff(u.ux, u.uy);
		    pline(E_J("Lucky for you, it didn't work!",
			      "運のよいことに、それは効果を発揮しなかった！"));
		    dmg = 0;
		    damage_resistant_obj(ANTIMAGIC, rnd(3));
		    break;
		}
		break;
	    case AD_PEST:
		pline(E_J("%s reaches out, and you feel fever and chills.",
			  "%sは腕をあなたに伸ばし、あなたは熱と寒気を感じた。"),
			Monnam(mtmp));
		(void) diseasemu(mdat); /* plus the normal damage */
		break;
	    case AD_FAMN:
		pline(E_J("%s reaches out, and your body shrivels.",
			  "%sは腕をあなたに伸ばし、あなたの身体は干からびた。"),
			Monnam(mtmp));
		exercise(A_CON, FALSE);
		if (!is_fainted()) morehungry(rn1(40,40));
		/* plus the normal damage */
		break;
	    case AD_SLIM:    
		hitmsg(mtmp, mattk);
		if (!uncancelled) break;
		if (flaming(youmonst.data)) {
		    pline_The(E_J("slime burns away!","スライムは焼け落ちた！"));
		    dmg = 0;
		} else if (Unchanging ||
				youmonst.data == &mons[PM_GREEN_SLIME]) {
		    You(E_J("are unaffected.","影響を受けなかった。"));
		    dmg = 0;
		} else if (!Slimed) {
#ifdef JP
		    Sprintf(dkiller_buf, "%sに触られて", JMONNAM(monsndx(mtmp->data)));
#endif /*JP*/
		    You(E_J("don't feel very well.","非常に気分が悪くなった。"));
		    Slimed = 10L;
		    flags.botl = 1;
		    killer_format = KILLED_BY_AN;
		    delayed_killer = E_J(mtmp->data->mname, dkiller_buf);
		} else
		    pline(E_J("Yuck!","うえっ！"));
		break;
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
		hitmsg(mtmp, mattk);
		/* uncancelled is sufficient enough; please
		   don't make this attack less frequent */
		if (uncancelled) {
		    struct obj *obj = some_armor(&youmonst);

		    if (drain_item(obj)) {
#ifndef JP
			Your("%s less effective.", aobjnam(obj, "seem"));
#else
			Your("%sは弱くなったようだ。", xname(obj));
#endif /*JP*/
		    }
		}
		break;
	    default:	dmg = 0;
			break;
	}
	if(u.uhp < 1) done_in_by(mtmp);

/*	Negative armor class reduces damage done instead of fully protecting
 *	against hits.
 */
#if 0
	if (dmg && u.uac < 0) {
		dmg -= rnd(-u.uac);
		if (dmg < 1) dmg = 1;
	}
#endif

	if(dmg) {
	    /* reduce damage by armor */
	    dmg = reduce_damage(dmg, hitpart);

	    if (Half_physical_damage
		/* Mitre of Holiness */
		|| (Role_if(PM_PRIEST) && uarmh && is_quest_artifact(uarmh) &&
		    (is_undead(mtmp->data) || is_demon(mtmp->data))))
		dmg = (dmg+1) / 2;

	    if (permdmg) {	/* Death's life force drain */
		int lowerlimit, *hpmax_p;
		/*
		 * Apply some of the damage to permanent hit points:
		 *	polymorphed	    100% against poly'd hpmax
		 *	hpmax > 25*lvl	    100% against normal hpmax
		 *	hpmax > 10*lvl	50..100%
		 *	hpmax >  5*lvl	25..75%
		 *	otherwise	 0..50%
		 * Never reduces hpmax below 1 hit point per level.
		 */
		permdmg = rn2(dmg / 2 + 1);
		if (Upolyd || u.uhpmax > 25 * u.ulevel) permdmg = dmg;
		else if (u.uhpmax > 10 * u.ulevel) permdmg += dmg / 2;
		else if (u.uhpmax > 5 * u.ulevel) permdmg += dmg / 4;

		if (Upolyd) {
		    hpmax_p = &u.mhmax;
		    /* [can't use youmonst.m_lev] */
		    lowerlimit = min((int)youmonst.data->mlevel, u.ulevel);
		} else {
		    hpmax_p = &u.uhpmax;
		    lowerlimit = u.ulevel;
		}
		if (*hpmax_p - permdmg > lowerlimit) {
		    if (Upolyd) u.mhmax -= permdmg;
		    else addhpmax(-permdmg);
//		    *hpmax_p -= permdmg;
		} else if (*hpmax_p > lowerlimit) {
		    if (Upolyd) u.mhmax = lowerlimit;
		    else addhpmax(-(u.uhpmax - lowerlimit));
//		    *hpmax_p = lowerlimit;
		} else	/* unlikely... */
		    ;	/* already at or below minimum threshold; do nothing */
		flags.botl = 1;
	    }
	    mdamageu(mtmp, dmg);
	}

	if (dmg)
	    res = passiveum(olduasmon, mtmp, mattk);
	else
	    res = 1;
	stop_occupation();
	return res;
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL int
gulpmu(mtmp, mattk)	/* monster swallows you, or damage if u.uswallow */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	struct trap *t = t_at(u.ux, u.uy);
	int	tmp = d((int)mattk->damn, (int)mattk->damd);
	int	tim_tmp;
	register struct obj *otmp2;
	int	i;

	if (!u.uswallow) {	/* swallows you */
		if (youmonst.data->msize >= MZ_HUGE) return(0);
		if ((t && ((t->ttyp == PIT) || (t->ttyp == SPIKED_PIT))) &&
		    sobj_at(BOULDER, u.ux, u.uy))
			return(0);

		if (Punished) unplacebc();	/* ball&chain go away */
		remove_monster(mtmp->mx, mtmp->my);
		mtmp->mtrapped = 0;		/* no longer on old trap */
		place_monster(mtmp, u.ux, u.uy);
		u.ustuck = mtmp;
		newsym(mtmp->mx,mtmp->my);
#ifdef STEED
		if (is_animal(mtmp->data) && u.usteed) {
			char buf[BUFSZ];
			/* Too many quirks presently if hero and steed
			 * are swallowed. Pretend purple worms don't
			 * like horses for now :-)
			 */
			Strcpy(buf, mon_nam(u.usteed));
			pline (E_J("%s lunges forward and plucks you off %s!",
				   "%sは突進し、あなたを%sから突き落とした！"),
				Monnam(mtmp), buf);
			dismount_steed(DISMOUNT_ENGULFED);
		} else
#endif
		pline(E_J("%s engulfs you!","%sはあなたを飲み込んだ！"), Monnam(mtmp));
		stop_occupation();
		reset_occupations();	/* behave as if you had moved */

		if (u.utrap) {
			You(E_J("are released from the %s!","%sから引きはがされた！"),
				u.utraptype==TT_WEB ? E_J("web","蜘蛛の巣") : E_J("trap","罠"));
			u.utrap = 0;
		}

		i = number_leashed();
		if (i > 0) {
#ifndef JP
		    const char *s = (i > 1) ? "leashes" : "leash";
		    pline_The("%s %s loose.", s, vtense(s, "snap"));
#else
		    pline("引き綱が弾け飛び、緩んだ。");
#endif /*JP*/
		    unleash_all();
		}

		if (touch_petrifies(youmonst.data) && !resists_ston(mtmp)) {
			minstapetrify(mtmp, TRUE);
			if (mtmp->mhp > 0) return 0;
			else return 2;
		}

		display_nhwindow(WIN_MESSAGE, FALSE);
		vision_recalc(2);	/* hero can't see anything */
		u.uswallow = 1;
		/* u.uswldtim always set > 1 */
		tim_tmp = 25 - (int)mtmp->m_lev;
		if (tim_tmp > 0) tim_tmp = rnd(tim_tmp) / 2;
		else if (tim_tmp < 0) tim_tmp = -(rnd(-tim_tmp) / 2);
		tim_tmp += -u.uac + 10;
		u.uswldtim = (unsigned)((tim_tmp < 2) ? 2 : tim_tmp);
		swallowed(1);
		for (otmp2 = invent; otmp2; otmp2 = otmp2->nobj)
		    (void) snuff_lit(otmp2);
		if (drownbymon()) {
		    You(E_J("are entrapped in %s!", "%sの中に閉じ込められた！"),
			(Hallucination) ? E_J("a fishbowl","金魚鉢") : E_J("water","水"));
		    if (!Amphibious && !Breathless) start_suffocation(SUFFO_WATER);
		}
	}

	if (mtmp != u.ustuck) return(0);
	if (u.uswldtim > 0) u.uswldtim -= 1;

	switch(mattk->adtyp) {

		case AD_DGST:
		    if (Slow_digestion) {
			/* Messages are handled below */
			u.uswldtim = 0;
			tmp = 0;
		    } else if (u.uswldtim == 0) {
			pline(E_J("%s totally digests you!",
				  "%sはあなたを完全に消化した！"), Monnam(mtmp));
			tmp = u.uhp;
			if (Half_physical_damage) tmp *= 2; /* sorry */
			instadeath = TRUE;
		    } else {
			pline(E_J("%s%s digests you!",
				  "%sはあなたを%s消化した！"), Monnam(mtmp),
			      (u.uswldtim == 2) ? E_J(" thoroughly","ほとんど") :
			      (u.uswldtim == 1) ? E_J(" utterly", "ほぼ完全に") : "");
			exercise(A_STR, FALSE);
		    }
		    break;
		case AD_PHYS:
		    if (mtmp->data == &mons[PM_FOG_CLOUD]) {
#ifndef JP
			You("are laden with moisture and %s",
			    flaming(youmonst.data) ? "are smoldering out!" :
			    Breathless ? "find it mildly uncomfortable." :
			    amphibious(youmonst.data) ? "feel comforted." :
			    "can barely breathe!");
#else
			You("湿気に取り囲まれ、%s",
			    flaming(youmonst.data) ? "くすぶっている！" :
			    Breathless ? "やや不愉快さを感じている。" :
			    amphibious(youmonst.data) ? "快適だ。" :
			    "ほとんど息ができない！");
#endif /*JP*/
			/* NB: Amphibious includes Breathless */
			if (Amphibious && !flaming(youmonst.data)) tmp = 0;
		    } else {
			You(E_J("are pummeled with debris!",
				"砂礫に打ちのめされた！"));
			exercise(A_STR, FALSE);
		    }
		    break;
		case AD_ACID:
		    if (Acid_resistance) {
			You(E_J("are covered with a seemingly harmless goo.",
				"無害に見えるベトベトに覆われている。"));
			tmp = 0;
		    } else {
		      if (Hallucination) pline(E_J("Ouch!  You've been slimed!",
						"痛っ！ あなたはスライムをかぶった！"));
		      else You(E_J("are covered in slime!  It burns!",
				   "どろどろの粘液に包まれている！ 身体が灼ける！"));
		      exercise(A_STR, FALSE);
		    }
		    hurtarmor(AD_CORR);
		    break;
		case AD_BLND:
		    if (can_blnd(mtmp, &youmonst, mattk->aatyp, (struct obj*)0)) {
			if(!Blind) {
#ifndef JP
			    You_cant("see in here!");
#else
			    pline("何も見えない！");
#endif /*JP*/
			    make_blinded((long)tmp,FALSE);
			    if (!Blind) Your(vision_clears);
			} else
			    /* keep him blind until disgorged */
			    make_blinded(Blinded+1,FALSE);
		    }
		    tmp = 0;
		    break;
		case AD_ELEC:
		    if(!mtmp->mcan && rn2(2)) {
			pline_The(E_J("air around you crackles with electricity.",
				      "周りの空気が電光に爆ぜた。"));
			if (Shock_resistance) {
			    if (is_full_resist(SHOCK_RES)) {
				shieldeff(u.ux, u.uy);
				You(E_J("seem unhurt.","傷つかないようだ。"));
				ugolemeffects(AD_ELEC,tmp);
				tmp = 0;
			    } else {
				You(E_J("resist the shock.","電撃に耐えている。"));
				tmp = (tmp+3) / 4;
			    }
			}
			if((int) mtmp->m_lev > rn2(20))
				destroy_item(WAND_CLASS, AD_ELEC);
			if((int) mtmp->m_lev > rn2(20))
				destroy_item(RING_CLASS, AD_ELEC);
		    } else tmp = 0;
		    break;
		case AD_COLD:
		    if(!mtmp->mcan && rn2(2)) {
			if (Cold_resistance) {
			    if (is_full_resist(COLD_RES)) {
				shieldeff(u.ux, u.uy);
				You_feel(E_J("mildly chilly.","ほのかな涼しさを感じている。"));
				ugolemeffects(AD_COLD,tmp);
				tmp = 0;
			    } else {
				You_feel(E_J("slightly cold.","寒さに耐えている。"));
				tmp = (tmp+3) / 4;
			    }
			} else You(E_J("are freezing to death!","極寒の中で死にそうだ！"));
			if((int) mtmp->m_lev > rn2(20))
				destroy_item(POTION_CLASS, AD_COLD);
		    } else tmp = 0;
		    break;
		case AD_FIRE:
		    if(!mtmp->mcan && rn2(2)) {
			if (Fire_resistance) {
			    if (is_full_resist(FIRE_RES)) {
				shieldeff(u.ux, u.uy);
				You_feel(E_J("mildly hot.","ほのかな暖かさを感じている。"));
				ugolemeffects(AD_FIRE,tmp);
				tmp = 0;
			    } else {
				You(E_J("endure the heat.","熱に耐えている。"));
				tmp = (tmp+3) / 4;
			    }
			} else You(E_J("are burning to a crisp!","激しく焼かれている！"));
			burn_away_slime();
			if((int) mtmp->m_lev > rn2(20))
				destroy_item(SCROLL_CLASS, AD_FIRE);
			if((int) mtmp->m_lev > rn2(20))
				destroy_item(POTION_CLASS, AD_FIRE);
			if((int) mtmp->m_lev > rn2(20))
				destroy_item(SPBOOK_CLASS, AD_FIRE);
			if((int) mtmp->m_lev > rn2(20))
				destroy_item(TOOL_CLASS, AD_FIRE);
		    } else tmp = 0;
		    break;
		case AD_DISE:
		    if (!diseasemu(mtmp->data)) tmp = 0;
		    break;
		case AD_DISN:
		    if(!mtmp->mcan && rn2(2)) {
			tmp = 0;
			if (is_full_resist(DISINT_RES)) {
			    shieldeff(u.ux, u.uy);
                            You(E_J("are not affected.","影響を受けなかった。"));
			    tmp = 0;
			    break;
                        } else if (uarms) {
                        /* destroy shield; other possessions are safe */
                            (void) destroy_arm(uarms);
                            break;
                        } else if (uarm) {
                         /* destroy suit; if present, cloak goes too */
                            if (uarmc) (void) destroy_arm(uarmc);
                            (void) destroy_arm(uarm);
                            break;
                        }
                        /* no shield or suit, you're dead; wipe out cloak
                           and/or shirt in case of life-saving or bones */
                        if (uarmc) (void) destroy_arm(uarmc);
#ifdef TOURIST
                        if (uarmu) (void) destroy_arm(uarmu);
#endif
			if (Disint_resistance) {
			    You(E_J("are not disintegrated.",
				    "分解されなかった。"));
			    break;
			}
			You(E_J("are disintegrated!","分解された！"));
			tmp = u.uhp;
			if (Half_physical_damage) tmp *= 2; /* sorry */
		    } else tmp = 0;
		    break;
		case AD_DRWN:
		    tmp = 0;
		    if (flaming(&mons[u.umonnum])) {
			You(E_J("are being extingushed!","消火されている！"));
			tmp = d(3,10);
		    }
		    if (!rn2(4)) water_damage(invent, FALSE, FALSE);
		    else hurtarmor(AD_RUST);
		    if (u.umonnum == PM_IRON_GOLEM) {
			You(E_J("rust!","錆びた！"));
			rehumanize();
			break;
		    }
		    break;
		default:
		    tmp = 0;
		    break;
	}

	if (Half_physical_damage) tmp = (tmp+1) / 2;

	mdamageu(mtmp, tmp);
	if (tmp) stop_occupation();

	if (touch_petrifies(youmonst.data) && !resists_ston(mtmp)) {
#ifndef JP
	    pline("%s very hurriedly %s you!", Monnam(mtmp),
		  is_animal(mtmp->data)? "regurgitates" : "expels");
#else
	    pline("%sは急に焦ってあなたを%sした！", Monnam(mtmp),
		  is_animal(mtmp->data)? "吐き戻" : "放り出");
#endif /*JP*/
	    expels(mtmp, mtmp->data, FALSE);
	} else if (!u.uswldtim || youmonst.data->msize >= MZ_HUGE) {
#ifndef JP
	    You("get %s!", is_animal(mtmp->data)? "regurgitated" : "expelled");
#else
	    You("%sされた！", is_animal(mtmp->data)? "吐き戻" : "放り出");
#endif /*JP*/
	    if (flags.verbose && (is_animal(mtmp->data) ||
		    (dmgtype(mtmp->data, AD_DGST) && Slow_digestion)))
		pline(E_J("Obviously %s doesn't like your taste.",
			  "明らかに、%sはあなたの味が気に入らなかったようだ。"), mon_nam(mtmp));
	    expels(mtmp, mtmp->data, FALSE);
	}
	return(1);
}

STATIC_OVL int
explmu(mtmp, mattk, ufound)	/* monster explodes in your face */
register struct monst *mtmp;
register struct attack  *mattk;
boolean ufound;
{
    int itemhit = 0;

    if (mtmp->mcan) return(0);

    if (!ufound)
#ifndef JP
	pline("%s explodes at a spot in %s!",
	    canseemon(mtmp) ? Monnam(mtmp) : "It",
	    levl[mtmp->mux][mtmp->muy].typ == WATER
		? "empty water" : "thin air");
#else
	pline("%sは%sに向かって爆発した！",
	    canseemon(mtmp) ? Monnam(mtmp) : "何か",
	    levl[mtmp->mux][mtmp->muy].typ == WATER
		? "何もない水中" : "何もない空間");
#endif /*JP*/
    else {
	register int tmp = d((int)mattk->damn, (int)mattk->damd);
	register boolean not_affected = defends((int)mattk->adtyp, uwep);
	boolean resist = FALSE;

	hitmsg(mtmp, mattk);

	switch (mattk->adtyp) {
	    case AD_COLD:
		if (elem_hits_shield(&youmonst, AD_COLD, E_J("freezing blast", "凍気の爆発")))
		    break;
		resist |= Cold_resistance;
		not_affected |= (resist && is_full_resist(COLD_RES));
		goto common;
	    case AD_FIRE:
		if (elem_hits_shield(&youmonst, AD_FIRE, E_J("fiery blast", "爆発")))
		    break;
		resist |= Fire_resistance;
		not_affected |= (resist && is_full_resist(FIRE_RES));
		goto common;
	    case AD_ELEC:
		if (elem_hits_shield(&youmonst, AD_ELEC, E_J("electric blast", "電撃")))
		    break;
		resist |= Shock_resistance;
		not_affected |= (resist && is_full_resist(SHOCK_RES));
common:

		itemhit = 3;
		if (ACURR(A_DEX) > rnd(20)) itemhit = 6; /* ducked */
		if (!not_affected) {
		    if (itemhit != 3) {
			You(E_J("duck some of the blast.",
				"身を屈めて直撃を避けた。"));
			tmp = (tmp+1) / 2;
		    } else {
		        if (flags.verbose) You(E_J("get blasted!","爆発に巻き込まれた！"));
		    }
		    if (resist) tmp = (tmp+3) / 4;
		    if (mattk->adtyp == AD_FIRE) burn_away_slime();
		    if (Half_physical_damage) tmp = (tmp+1) / 2;
		    mdamageu(mtmp, tmp);
		}
		break;

	    case AD_BLND:
		not_affected = resists_blnd(&youmonst);
		if (!not_affected) {
		    /* sometimes you're affected even if it's invisible */
		    if (mon_visible(mtmp) || (rnd(tmp /= 2) > u.ulevel)) {
			You(E_J("are blinded by a blast of light!",
				"激しい閃光で目が見えなくなった！"));
			make_blinded((long)tmp, FALSE);
			if (!Blind) Your(vision_clears);
		    } else if (flags.verbose)
			You(E_J("get the impression it was not terribly bright.",
				"そうひどく眩しくはなかったという感想を持った。"));
		}
		break;

	    case AD_HALU:
		not_affected |= Blind ||
			(u.umonnum == PM_BLACK_LIGHT ||
			 u.umonnum == PM_VIOLET_FUNGUS ||
			 dmgtype(youmonst.data, AD_STUN));
		if (!not_affected) {
		    boolean chg;
		    if (!Hallucination)
			You(E_J("are caught in a blast of kaleidoscopic light!",
				"色とりどりの派手な光の爆発に包まれた！"));
		    chg = make_hallucinated(HHallucination + (long)tmp,FALSE,0L);
#ifndef JP
		    You("%s.", chg ? "are freaked out" : "seem unaffected");
#else
		    You("%s。", chg ? "幻覚状態に陥った" : "影響を受けないようだ");
#endif /*JP*/
		}
		break;

	    default:
		break;
	}
	if (not_affected) {
	    You(E_J("seem unaffected by it.","影響を受けないようだ。"));
	    ugolemeffects((int)mattk->adtyp, tmp);
	}
	if (itemhit && !rn2(itemhit)) destroy_items(mattk->adtyp);
    }
    mondead(mtmp);
    wake_nearto(mtmp->mx, mtmp->my, 7*7);
    if (mtmp->mhp > 0) return(0);
    return(2);	/* it dies */
}

int
gazemu(mtmp, mattk)	/* monster gazes at you */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	boolean resist = FALSE;
#ifdef MAGIC_GLASSES
	resist = (ublindf && ublindf->otyp == GLASSES_OF_GAZE_PROTECTION);
#endif
	switch(mattk->adtyp) {
	    case AD_STON:
		if (mtmp->mcan || !mtmp->mcansee || resist) {
		    if (!canseemon(mtmp)) break;	/* silently */
#ifndef JP
		    pline("%s %s.", Monnam(mtmp),
			  (mtmp->data == &mons[PM_MEDUSA] && mtmp->mcan) ?
				"doesn't look all that ugly" :
				"gazes ineffectually");
#else
		    pline("%sは%s。", Monnam(mtmp),
			  (mtmp->data == &mons[PM_MEDUSA] && mtmp->mcan) ?
				"はそれほど醜くは見えない" :
				"あなたを力なくにらみつけた。");
#endif /*JP*/
		    break;
		}
		if (Reflecting && couldsee(mtmp->mx, mtmp->my) &&
			mtmp->data == &mons[PM_MEDUSA]) {
		    /* hero has line of sight to Medusa and she's not blind */
		    boolean useeit = canseemon(mtmp);

		    if (useeit)
			(void) ureflects(E_J("%s gaze is reflected by your %s.",
					     "%s眼光はあなたの%sで反射した。"),
					 s_suffix(Monnam(mtmp)));
		    if (mon_reflects(mtmp, !useeit ? (char *)0 :
				     E_J("The gaze is reflected away by %s %s!",
					 "眼光は%s%sにはね返された！")))
			break;
		    if (!m_canseeu(mtmp)) { /* probably you're invisible */
			if (useeit)
#ifndef JP
			    pline("%s doesn't seem to notice that %s gaze was reflected.",
				  Monnam(mtmp), mhis(mtmp));
#else
			    pline("%sは自分の眼光が反射されたことに気づかないようだ。",
				  Monnam(mtmp));
#endif /*JP*/
			break;
		    }
		    if (useeit)
			pline(E_J("%s is turned to stone!","%sは石になった！"), Monnam(mtmp));
		    stoned = TRUE;
		    killed(mtmp);

		    if (mtmp->mhp > 0) break;
		    return 2;
		}
		if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my) &&
		    !Stone_resistance) {
		    You(E_J("meet %s gaze.","%s目をまともに見てしまった。"), s_suffix(mon_nam(mtmp)));
		    stop_occupation();
		    if(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
			break;
		    You(E_J("turn to stone...","石になった…。"));
		    killer_format = KILLED_BY;
#ifndef JP
		    killer = mtmp->data->mname;
#else
		    Sprintf(killer_buf, "%sに睨まれて", JMONNAM(monsndx(mtmp->data)));
		    killer = killer_buf;
#endif /*JP*/
		    done(STONING);
		}
		break;
	    case AD_CONF:
		if(!mtmp->mcan && canseemon(mtmp) &&
		   couldsee(mtmp->mx, mtmp->my) && !resist &&
		   mtmp->mcansee && !mtmp->mspec_used && rn2(5)) {
		    int conf = d(3,4);

		    mtmp->mspec_used = mtmp->mspec_used + (conf + rn2(6));
		    if(!Confusion)
			pline(E_J("%s gaze confuses you!","%s眼光はあなたを混乱させた！"),
			                  s_suffix(Monnam(mtmp)));
		    else
			You(E_J("are getting more and more confused.",
				"ますます混乱した。"));
		    make_confused(HConfusion + conf, FALSE);
		    stop_occupation();
		}
		break;
	    case AD_STUN:
		if(!mtmp->mcan && canseemon(mtmp) &&
		   couldsee(mtmp->mx, mtmp->my) && !resist &&
		   mtmp->mcansee && !mtmp->mspec_used && rn2(5)) {
		    int stun = d(2,6);

		    mtmp->mspec_used = mtmp->mspec_used + (stun + rn2(6));
		    pline(E_J("%s stares piercingly at you!",
			      "%sの眼光があなたを貫いた！"), Monnam(mtmp));
		    make_stunned(HStun + stun, TRUE);
		    stop_occupation();
		}
		break;
	    case AD_BLND:
		if (!mtmp->mcan && canseemon(mtmp) &&
		    !resists_blnd(&youmonst) &&
		    distu(mtmp->mx,mtmp->my) <= BOLT_LIM*BOLT_LIM) {
		    int blnd = d((int)mattk->damn, (int)mattk->damd);

		    You(E_J("are blinded by %s radiance!",
			    "%s輝きで目が見えなくなった！"),
			              s_suffix(mon_nam(mtmp)));
		    make_blinded((long)blnd,FALSE);
		    stop_occupation();
		    /* not blind at this point implies you're wearing
		       the Eyes of the Overworld; make them block this
		       particular stun attack too */
		    if (!Blind) Your(vision_clears);
		    else make_stunned((long)d(1,3),TRUE);
		}
		break;
	    case AD_FIRE:
		if (!mtmp->mcan && canseemon(mtmp) && !resist &&
			couldsee(mtmp->mx, mtmp->my) &&
			mtmp->mcansee && !mtmp->mspec_used && rn2(5)) {
		    int dmg = d(2,6);

		    pline(E_J("%s attacks you with a fiery gaze!",
			      "%sはあなたを灼熱の眼光で攻撃した！"), Monnam(mtmp));
		    stop_occupation();
		    if (Fire_resistance) {
			if (is_full_resist(FIRE_RES)) {
			    pline_The(E_J("fire doesn't feel hot!","炎は熱くない！"));
			    dmg = 0;
			} else {
			    You(E_J("endure the heat!","熱に耐えている！"));
			    dmg = (dmg+3) / 4;
			}
		    }
		    burn_away_slime();
		    if ((int) mtmp->m_lev > rn2(20))
			destroy_item(SCROLL_CLASS, AD_FIRE);
		    if ((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_FIRE);
		    if ((int) mtmp->m_lev > rn2(25))
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		    if ((int) mtmp->m_lev > rn2(20))
			destroy_item(TOOL_CLASS, AD_FIRE);
		    if (dmg) mdamageu(mtmp, dmg);
		}
		break;
#ifdef PM_BEHOLDER /* work in progress */
	    case AD_SLEE:
		if(!mtmp->mcan && canseemon(mtmp) && !resist &&
		   couldsee(mtmp->mx, mtmp->my) && mtmp->mcansee &&
		   multi >= 0 && !rn2(5) && !Sleep_resistance) {

		    fall_asleep(-rnd(10), TRUE);
		    pline(E_J("%s gaze makes you very sleepy...",
			      "%s眼光をくらい、あなたは眠気を抑えられなくなった…。"),
			  s_suffix(Monnam(mtmp)));
		}
		break;
	    case AD_SLOW:
		if(!mtmp->mcan && canseemon(mtmp) && mtmp->mcansee &&
		   (HFast & (INTRINSIC|TIMEOUT)) && !resist &&
		   !defends(AD_SLOW, uwep) && !rn2(4))

		    u_slow_down();
		    stop_occupation();
		break;
#endif
	    default: impossible("Gaze attack %d?", mattk->adtyp);
		break;
	}
	return(0);
}

#endif /* OVLB */
#ifdef OVL1

void
mdamageu(mtmp, n)	/* mtmp hits you for n points damage */
register struct monst *mtmp;
register int n;
{
#ifdef SHOWDMG
	if (flags.showdmg && n > 0 && !instadeath)
#ifndef JP
		pline("(%dpt%s)", n, (n == 1 ? "" : "s"));
#else
		pline("(%dpt)", n);
#endif /*JP*/
	instadeath = FALSE;
#endif
	flags.botl = 1;
	if (Upolyd) {
		u.mh -= n;
		if (u.mh < 1) rehumanize();
	} else {
		u.uhp -= n;
		if(u.uhp < 1) done_in_by(mtmp);
	}
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL void
urustm(mon, obj)
register struct monst *mon;
register struct obj *obj;
{
	int hurt;

	if (!mon || !obj) return; /* just in case */
	if (dmgtype(youmonst.data, AD_CORR))
	    hurt = 3; /*corrode*/
	else if (dmgtype(youmonst.data, AD_RUST))
	    hurt = 1/*rust*/;
	else
	    return;

	rust_dmg(obj, xname(obj), hurt, TRUE, mon);
}

#endif /* OVLB */
#ifdef OVL1

int
could_seduce(magr,mdef,mattk)
struct monst *magr, *mdef;
struct attack *mattk;
/* returns 0 if seduction impossible,
 *	   1 if fine,
 *	   2 if wrong gender for nymph */
{
	register struct permonst *pagr;
	boolean agrinvis, defperc;
	xchar genagr, gendef;

	if (is_animal(magr->data)) return (0);
	if(magr == &youmonst) {
		pagr = youmonst.data;
		agrinvis = (Invis != 0);
		genagr = poly_gender();
	} else {
		pagr = magr->data;
		agrinvis = magr->minvis;
		genagr = gender(magr);
	}
	if(mdef == &youmonst) {
		defperc = (See_invisible != 0);
		gendef = poly_gender();
	} else {
		defperc = perceives(mdef->data);
		gendef = gender(mdef);
	}

	if(agrinvis && !defperc
#ifdef SEDUCE
		&& mattk && mattk->adtyp != AD_SSEX
#endif
		)
		return 0;

	if(pagr->mlet != S_NYMPH
		&& ((pagr != &mons[PM_INCUBUS] && pagr != &mons[PM_SUCCUBUS])
#ifdef SEDUCE
		    || (mattk && mattk->adtyp != AD_SSEX)
#endif
		   ))
		return 0;
	
	if(genagr == 1 - gendef)
		return 1;
	else
		return (pagr->mlet == S_NYMPH) ? 2 : 0;
}

#endif /* OVL1 */
#ifdef OVLB

#ifdef SEDUCE
/* Returns 1 if monster teleported */
int
doseduce(mon)
register struct monst *mon;
{
	register struct obj *ring, *nring;
	boolean fem = (mon->data == &mons[PM_SUCCUBUS]); /* otherwise incubus */
	char qbuf[QBUFSZ];

	if (mon->mcan || mon->mspec_used) {
#ifndef JP
		pline("%s acts as though %s has got a %sheadache.",
		      Monnam(mon), mhe(mon),
		      mon->mcan ? "severe " : "");
#else
		pline("%sは%s頭痛に悩まされている風を装った。",
		      Monnam(mon), mon->mcan ? "ひどい" : "");
#endif /*JP*/
		return 0;
	}

	if (unconscious()) {
		pline(E_J("%s seems dismayed at your lack of response.",
			  "%sは反応がないことに落胆したようだ。"),
		      Monnam(mon));
		return 0;
	}

	if (Blind) pline(E_J("It caresses you...","何かがあなたをそっと撫でている…。"));
	else You_feel(E_J("very attracted to %s.","%sにとても惹かれた。"), mon_nam(mon));

	for(ring = invent; ring; ring = nring) {
	    nring = ring->nobj;
	    if (ring->otyp != RIN_ADORNMENT) continue;
	    if (fem) {
		if (rn2(20) < ACURR(A_CHA)) {
#ifndef JP
		    Sprintf(qbuf, "\"That %s looks pretty.  May I have it?\"",
			safe_qbuf("",sizeof("\"That  looks pretty.  May I have it?\""),
			xname(ring), simple_typename(ring->otyp), "ring"));
#else
		    Sprintf(qbuf, "「この%sはとても素敵ね。私にくださらない？」",
			safe_qbuf("",sizeof("「このはとても素敵ね。私にくださらない？」"),
			xname(ring), simple_typename(ring->otyp), "指輪"));
#endif /*JP*/
		    makeknown(RIN_ADORNMENT);
		    if (yn(qbuf) == 'n') continue;
		} else pline(E_J("%s decides she'd like your %s, and takes it.",
				 "%sはあなたの%sを気に入り、貰うことに決めた。"),
			Blind ? E_J("She","彼女") : Monnam(mon), xname(ring));
		makeknown(RIN_ADORNMENT);
		if (ring==uleft || ring==uright) Ring_gone(ring);
		if (ring==uwep) setuwep((struct obj *)0);
		if (ring==uswapwep) setuswapwep((struct obj *)0);
		if (ring==uquiver) setuqwep((struct obj *)0);
		freeinv(ring);
		(void) mpickobj(mon,ring);
	    } else {
		char buf[BUFSZ];

		if (uleft && uright && uleft->otyp == RIN_ADORNMENT
				&& uright->otyp==RIN_ADORNMENT)
			break;
		if (ring==uleft || ring==uright) continue;
		if (rn2(20) < ACURR(A_CHA)) {
#ifndef JP
		    Sprintf(qbuf,"\"That %s looks pretty.  Would you wear it for me?\"",
			safe_qbuf("",
			    sizeof("\"That  looks pretty.  Would you wear it for me?\""),
			    xname(ring), simple_typename(ring->otyp), "ring"));
#else
		    Sprintf(qbuf,"「この%sはとても素敵だね。指にはめてみせてくれないかい？」",
			safe_qbuf("",
			    sizeof("「このはとても素敵だね。指にはめてみせてくれないかい？」"),
			    xname(ring), simple_typename(ring->otyp), "指輪"));
#endif /*JP*/
		    makeknown(RIN_ADORNMENT);
		    if (yn(qbuf) == 'n') continue;
		} else {
		    pline(E_J("%s decides you'd look prettier wearing your %s,",
			      "%sは、あなたは%sを着けたほうが可愛く見えると判断した。"),
			Blind ? E_J("He","彼") : Monnam(mon), xname(ring));
		    pline(E_J("and puts it on your finger.","彼は指輪をあなたの指にはめることにした。"));
		}
		makeknown(RIN_ADORNMENT);
		if (!uright) {
#ifndef JP
		    pline("%s puts %s on your right %s.",
			Blind ? "He" : Monnam(mon), the(xname(ring)), body_part(HAND));
#else
		    pline("%sは%sをあなたの右%sにはめた。",
			Blind ? "彼" : Monnam(mon), xname(ring), body_part(HAND));
#endif /*JP*/
		    setworn(ring, RIGHT_RING);
		} else if (!uleft) {
#ifndef JP
		    pline("%s puts %s on your left %s.",
			Blind ? "He" : Monnam(mon), the(xname(ring)), body_part(HAND));
#else
		    pline("%sは%sをあなたの左%sにはめた。",
			Blind ? "彼" : Monnam(mon), xname(ring), body_part(HAND));
#endif /*JP*/
		    setworn(ring, LEFT_RING);
		} else if (uright && uright->otyp != RIN_ADORNMENT) {
		    Strcpy(buf, xname(uright));
		    pline(E_J("%s replaces your %s with your %s.",
			      "%sは%sを外し、%sをはめた。"),
			Blind ? E_J("He","彼") : Monnam(mon), buf, xname(ring));
		    Ring_gone(uright);
		    setworn(ring, RIGHT_RING);
		} else if (uleft && uleft->otyp != RIN_ADORNMENT) {
		    Strcpy(buf, xname(uleft));
		    pline(E_J("%s replaces your %s with your %s.",
			      "%sは%sを外し、%sをはめた。"),
			Blind ? E_J("He","彼") : Monnam(mon), buf, xname(ring));
		    Ring_gone(uleft);
		    setworn(ring, LEFT_RING);
		} else impossible("ring replacement");
		Ring_on(ring);
		prinv((char *)0, ring, 0L);
	    }
	}

	if (!uarmc && !uarmf && !uarmg && !uarms && !uarmh
#ifdef TOURIST
								&& !uarmu
#endif
									)
#ifndef JP
		pline("%s murmurs sweet nothings into your ear.",
			Blind ? (fem ? "She" : "He") : Monnam(mon));
#else
		pline("%sはあなたの耳のそばで甘い言葉をささやいた。",
			Blind ? (fem ? "彼女" : "彼") : Monnam(mon));
#endif /*JP*/
	else
#ifndef JP
		pline("%s murmurs in your ear, while helping you undress.",
			Blind ? (fem ? "She" : "He") : Monnam(mon));
#else
		pline("%sはあなたにささやきかけながら、服を脱がせはじめた。",
			Blind ? (fem ? "彼女" : "彼") : Monnam(mon));
#endif /*JP*/
	mayberem(uarmc, cloak_simple_name(uarmc));
	if(!uarmc)
		mayberem(uarm, E_J("suit", (uarm && is_clothes(uarm) ? "服" : "鎧")));
	mayberem(uarmf, E_J("boots","ブーツ"));
	if(!uwep || !welded(uwep))
		mayberem(uarmg, E_J("gloves","手袋"));
	mayberem(uarms, E_J("shield","盾"));
	mayberem(uarmh, helm_simple_name(uarmh));
#ifdef TOURIST
	if(!uarmc && !uarm)
		mayberem(uarmu, E_J("shirt","シャツ"));
#endif

	if (uarm || uarmc) {
		verbalize(E_J("You're such a %s; I wish...",
			      "こんなに%sと…たらいいのに…。"),
				flags.female ? E_J("sweet lady","可愛い女性") : E_J("nice guy","いい男"));
		if (!tele_restrict(mon)) (void) rloc(mon, FALSE);
		return 1;
	}
	if (u.ualign.type == A_CHAOTIC)
		adjalign(1);

	/* by this point you have discovered mon's identity, blind or not... */
	pline(E_J("Time stands still while you and %s lie in each other's arms...",
		  "あなたと%sは腕を回して抱き合い、時は静かに止まった…。"),
		noit_mon_nam(mon));
	if (rn2(50) > ACURR(A_CHA) + ACURR(A_INT)) {
		/* Don't bother with mspec_used here... it didn't get tired! */
		pline(E_J("%s seems to have enjoyed it more than you...",
			  "%sはあなたよりも楽しんだようだ…。"),
			noit_Monnam(mon));
		switch (rn2(5)) {
			case 0: You_feel(E_J("drained of energy.","活力が流れ去るのを感じた。"));
				u.uen = 0;
				u.uenmax -= rnd(Half_physical_damage ? 5 : 10);
			        exercise(A_CON, FALSE);
				if (u.uenmax < 0) u.uenmax = 0;
				break;
			case 1: You(E_J("are down in the dumps.","落ち込んだ。"));
				(void) adjattrib(A_CON, -1, TRUE);
			        exercise(A_CON, FALSE);
				flags.botl = 1;
				break;
			case 2: Your(E_J("senses are dulled.","五感は鈍った。"));
				(void) adjattrib(A_WIS, -1, TRUE);
			        exercise(A_WIS, FALSE);
				flags.botl = 1;
				break;
			case 3:
				if (!resists_drli(&youmonst)) {
				    You_feel(E_J("out of shape.","体調が悪くなった。"));
				    losexp(E_J("overexertion","やり過ぎて"));
				} else {
				    You(E_J("have a curious feeling...",
					    "興味深い感覚を覚えた…。"));
				}
				break;
			case 4: {
				int tmp;
				You_feel(E_J("exhausted.","疲労困憊した。"));
			        exercise(A_STR, FALSE);
				tmp = rn1(10, 6);
				if(Half_physical_damage) tmp = (tmp+1) / 2;
				losehp(tmp, E_J("exhaustion","精力を使い果たして"), KILLED_BY);
				break;
			}
		}
	} else {
		mon->mspec_used = rnd(100); /* monster is worn out */
		You(E_J("seem to have enjoyed it more than %s...",
			"%sよりも楽しんだようだ…。"),
		    noit_mon_nam(mon));
		switch (rn2(5)) {
		case 0: You_feel(E_J("raised to your full potential.",
				     "自分の活力が最大限に引き出されるのを感じた。"));
			exercise(A_CON, TRUE);
			u.uen = (u.uenmax += rnd(5));
			break;
		case 1: You_feel(E_J("good enough to do it again.",
				     "もう一度できるほど充分に元気だ。"));
			(void) adjattrib(A_CON, 1, TRUE);
			exercise(A_CON, TRUE);
			flags.botl = 1;
			break;
		case 2: You(E_J("will always remember %s...",
				"%sをいつまでも忘れないだろう…。"), noit_mon_nam(mon));
			(void) adjattrib(A_WIS, 1, TRUE);
			exercise(A_WIS, TRUE);
			flags.botl = 1;
			break;
		case 3: pline(E_J("That was a very educational experience.",
				  "とても教育的な経験だった。"));
			pluslvl(FALSE);
			exercise(A_WIS, TRUE);
			break;
		case 4: You_feel(E_J("restored to health!","すっかり健康になった！"));
			u.uhp = u.uhpmax;
			if (Upolyd) u.mh = u.mhmax;
			exercise(A_STR, TRUE);
			flags.botl = 1;
			break;
		}
	}

	if (mon->mtame) /* don't charge */ ;
	else if (rn2(20) < ACURR(A_CHA)) {
#ifndef JP
		pline("%s demands that you pay %s, but you refuse...",
			noit_Monnam(mon),
			Blind ? (fem ? "her" : "him") : mhim(mon));
#else
		pline("%sは支払いを要求したが、あなたは拒否した…。",
			noit_Monnam(mon));
#endif /*JP*/
	} else if (u.umonnum == PM_LEPRECHAUN)
		pline(E_J("%s tries to take your money, but fails...",
			  "%sはあなたの金を取ろうとしたが、失敗した…。"),
				noit_Monnam(mon));
	else {
#ifndef GOLDOBJ
		long cost;

		if (u.ugold > (long)LARGEST_INT - 10L)
			cost = (long) rnd(LARGEST_INT) + 500L;
		else
			cost = (long) rnd((int)u.ugold + 10) + 500L;
		if (mon->mpeaceful) {
			cost /= 5L;
			if (!cost) cost = 1L;
		}
		if (cost > u.ugold) cost = u.ugold;
#ifndef JP
		if (!cost) verbalize("It's on the house!");
#else
		if (!cost) verbalize("今回は私のおごり%s！", fem ? "よ" : "だ");
#endif /*JP*/
		else {
		    pline(E_J("%s takes %ld %s for services rendered!",
			      "%sはサービス料として%ld %sを持っていった！"),
			    noit_Monnam(mon), cost, currency(cost));
		    u.ugold -= cost;
		    mon->mgold += cost;
		    flags.botl = 1;
		}
#else
		long cost;
                long umoney = money_cnt(invent);

		if (umoney > (long)LARGEST_INT - 10L)
			cost = (long) rnd(LARGEST_INT) + 500L;
		else
			cost = (long) rnd((int)umoney + 10) + 500L;
		if (mon->mpeaceful) {
			cost /= 5L;
			if (!cost) cost = 1L;
		}
		if (cost > umoney) cost = umoney;
#ifndef JP
		if (!cost) verbalize("It's on the house!");
#else
		if (!cost) verbalize("今回は私のおごり%s！", fem ? "よ" : "だ");
#endif /*JP*/
		else { 
		    pline(E_J("%s takes %ld %s for services rendered!",
			      "%sはサービス料として%ld %sを持っていった！"),
			    noit_Monnam(mon), cost, currency(cost));
                    money2mon(mon, cost);
		    flags.botl = 1;
		}
#endif
	}
	if (!rn2(25)) mon->mcan = 1; /* monster is worn out */
	if (!tele_restrict(mon)) (void) rloc(mon, FALSE);
	return 1;
}

STATIC_OVL void
mayberem(obj, str)
register struct obj *obj;
const char *str;
{
	char qbuf[QBUFSZ];
#ifdef JP
	boolean nugi = obj && ((objects[obj->otyp].oc_armcat == ARM_SUIT)
#ifdef TOURIST
			    || (objects[obj->otyp].oc_armcat == ARM_SHIRT)
#endif
			);
#endif /*JP*/

	if (!obj || !obj->owornmask) return;

	if (rn2(20) < ACURR(A_CHA)) {
#ifndef JP
		Sprintf(qbuf,"\"Shall I remove your %s, %s?\"",
			str,
			(!rn2(2) ? "lover" : !rn2(2) ? "dear" : "sweetheart"));
#else
		Sprintf(qbuf,"「%sを%sてもいい、%s？」",
			str,
			nugi ? "脱がせ" : "外し",
			(!rn2(2) ? "愛しい人" : !rn2(2) ? "素敵な人" : "可愛い人"));
#endif /*JP*/
		if (yn(qbuf) == 'n') return;
	} else {
		char hairbuf[BUFSZ];

		Sprintf(hairbuf, E_J("let me run my fingers through your %s",
				     "指で%sを触らせて"),
			body_part(HAIR));
#ifndef JP
		verbalize("Take off your %s; %s.", str,
			(obj == uarm)  ? "let's get a little closer" :
			(obj == uarmc || obj == uarms) ? "it's in the way" :
			(obj == uarmf) ? "let me rub your feet" :
			(obj == uarmg) ? "they're too clumsy" :
#ifdef TOURIST
			(obj == uarmu) ? "let me massage you" :
#endif
			/* obj == uarmh */
			hairbuf);
#else
		verbalize("%sを%s ─ %s。", str, nugi ? "脱いで" : "取って",
			(obj == uarm)  ? "もっと近くに寄らせて" :
			(obj == uarmc || obj == uarms) ? "邪魔だもの" :
			(obj == uarmf) ? "足をさすってあげる" :
			(obj == uarmg) ? "綺麗な手を見たいな" :
#ifdef TOURIST
			(obj == uarmu) ? "肌に触らせて" :
#endif
			/* obj == uarmh */
			hairbuf);
#endif /*JP*/
	}
	remove_worn_item(obj, TRUE);
}
#endif  /* SEDUCE */

#endif /* OVLB */

#ifdef OVL1

STATIC_OVL int
passiveum(olduasmon,mtmp,mattk)
struct permonst *olduasmon;
register struct monst *mtmp;
register struct attack *mattk;
{
	int i, tmp;

	for (i = 0; ; i++) {
	    if (i >= NATTK) return 1;
	    if (olduasmon->mattk[i].aatyp == AT_NONE ||
	    		olduasmon->mattk[i].aatyp == AT_BOOM) break;
	}
	if (olduasmon->mattk[i].damn)
	    tmp = d((int)olduasmon->mattk[i].damn,
				    (int)olduasmon->mattk[i].damd);
	else if(olduasmon->mattk[i].damd)
	    tmp = d((int)olduasmon->mlevel+1, (int)olduasmon->mattk[i].damd);
	else
	    tmp = 0;

	/* These affect the enemy even if you were "killed" (rehumanized) */
	switch(olduasmon->mattk[i].adtyp) {
	    case AD_ACID:
		if (!polearms && !rn2(2)) {
		    pline(E_J("%s is splashed by your acid!",
			      "%sはあなたの酸を浴びせられた！"), Monnam(mtmp));
		    if (resists_acid(mtmp)) {
			pline(E_J("%s is not affected.",
				  "%sは影響を受けなかった。"), Monnam(mtmp));
			tmp = 0;
		    }
		} else tmp = 0;
		if (!polearms && !rn2(30)) erode_armor(mtmp, TRUE);
		if (!rn2(6)) erode_obj(MON_WEP(mtmp), TRUE, TRUE);
		goto assess_dmg;
	    case AD_STON: /* cockatrice */
	    {
		long protector = attk_protection((int)mattk->aatyp),
		     wornitems = mtmp->misc_worn_check;

		/* wielded weapon gives same protection as gloves here */
		if (MON_WEP(mtmp) != 0) wornitems |= W_ARMG;

		if (!resists_ston(mtmp) && (protector == 0L ||
			(protector != ~0L &&
			    (wornitems & protector) != protector))) {
		    if (poly_when_stoned(mtmp->data)) {
			mon_to_stone(mtmp);
			return (1);
		    }
		    pline(E_J("%s turns to stone!","%sは石になった！"), Monnam(mtmp));
		    stoned = 1;
		    xkilled(mtmp, 0);
		    if (mtmp->mhp > 0) return 1;
		    return 2;
		}
		return 1;
	    }
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
	    	if (otmp) {
	    	    (void) drain_item(otmp);
	    	    /* No message */
	    	}
	    	return (1);
	    default:
		break;
	}
	if (!Upolyd) return 1;

	/* These affect the enemy only if you are still a monster */
	if (rn2(3)) switch(youmonst.data->mattk[i].adtyp) {
	    case AD_PHYS:
	    	if (youmonst.data->mattk[i].aatyp == AT_BOOM) {
	    	    You(E_J("explode!","爆発した！"));
	    	    /* KMH, balance patch -- this is okay with unchanging */
	    	    rehumanize();
		    if (polearms) return 1;
	    	    goto assess_dmg;
	    	}
	    	break;
	    case AD_PLYS: /* Floating eye */
		if (tmp > 127) tmp = 127;
		if (u.umonnum == PM_FLOATING_EYE) {
		    if (!rn2(4)) tmp = 127;
		    if (mtmp->mcansee && haseyes(mtmp->data) && rn2(3) &&
				(perceives(mtmp->data) || !Invis)) {
			if (Blind)
			    pline(E_J("As a blind %s, you cannot defend yourself.",
				      "盲目の%sであるあなたは、自分を守ることができない。"),
					E_J(youmonst.data->mname, JMONNAM(u.umonnum)));
		        else {
			    if (mon_reflects(mtmp,
					    E_J("Your gaze is reflected by %s %s.",
						"あなたの眼光は%s%sに反射した！")))
				return 1;
			    pline(E_J("%s is frozen by your gaze!",
				      "%sはあなたの眼光で動けなくなった！"), Monnam(mtmp));
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = tmp;
			    return 3;
			}
		    }
		} else { /* gelatinous cube */
		    if (polearms) return 1;
		    pline(E_J("%s is frozen by you.",
			      "%sはあなたに麻痺させられた。"), Monnam(mtmp));
		    mtmp->mcanmove = 0;
		    mtmp->mfrozen = tmp;
		    return 3;
		}
		return 1;
	    case AD_COLD: /* Brown mold or blue jelly */
		if (polearms) return 1;
		if (resists_cold(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
		    pline(E_J("%s is mildly chilly.",
			      "%sはかすかに冷やされた。"), Monnam(mtmp));
		    golemeffects(mtmp, AD_COLD, tmp);
		    tmp = 0;
		    break;
		}
		pline(E_J("%s is suddenly very cold!",
			  "%sは突然激しい寒さに襲われた！"), Monnam(mtmp));
		u.mh += tmp / 2;
		if (u.mhmax < u.mh) u.mhmax = u.mh;
		if (u.mhmax > ((youmonst.data->mlevel+1) * 8))
		    (void)split_mon(&youmonst, mtmp);
		break;
	    case AD_STUN: /* Yellow mold */
		if (polearms) return 1;
		if (!mtmp->mstun) {
		    mtmp->mstun = 1;
#ifndef JP
		    pline("%s %s.", Monnam(mtmp),
			  makeplural(stagger(mtmp->data, "stagger")));
#else
		    pline("%sは%s。", Monnam(mtmp),
			  stagger(mtmp->data, "よろめいた"));
#endif /*JP*/
		}
		tmp = 0;
		break;
	    case AD_FIRE: /* Red mold */
		if (polearms) return 1;
		if (resists_fire(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
		    pline(E_J("%s is mildly warm.",
			      "%sはかすかに暖まった。"), Monnam(mtmp));
		    golemeffects(mtmp, AD_FIRE, tmp);
		    tmp = 0;
		    break;
		}
		pline(E_J("%s is suddenly very hot!",
			  "%sは突然激しく熱せられた！"), Monnam(mtmp));
		break;
	    case AD_ELEC:
		if (polearms && !is_metallic(MON_WEP(mtmp))) return 1;
		if (resists_elec(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
		    pline(E_J("%s is slightly tingled.",
			      "%sは少しだけ痺れた。"), Monnam(mtmp));
		    golemeffects(mtmp, AD_ELEC, tmp);
		    tmp = 0;
		    break;
		}
		pline(E_J("%s is jolted with your electricity!",
			  "%sはあなたの電撃に打たれた！"), Monnam(mtmp));
		break;
	    default: tmp = 0;
		break;
	}
	else tmp = 0;

    assess_dmg:
	if((mtmp->mhp -= tmp) <= 0) {
		pline(E_J("%s dies!","%sは死んだ！"), Monnam(mtmp));
		xkilled(mtmp,0);
		if (mtmp->mhp > 0) return 1;
		return 2;
	}
	return 1;
}

#endif /* OVL1 */
#ifdef OVLB

#include "edog.h"
struct monst *
cloneu()
{
	register struct monst *mon;
	int mndx = monsndx(youmonst.data);

	if (u.mh <= 1) return(struct monst *)0;
	if (mvitals[mndx].mvflags & G_EXTINCT) return(struct monst *)0;
	mon = makemon(youmonst.data, u.ux, u.uy, NO_MINVENT|MM_EDOG);
	mon = christen_monst(mon, plname);
	initedog(mon);
	mon->m_lev = youmonst.data->mlevel;
	mon->mhpmax = u.mhmax;
	mon->mhp = u.mh / 2;
	u.mh -= mon->mhp;
	flags.botl = 1;
	return(mon);
}

#endif /* OVLB */

boolean
armor_cancel()
{
	const int acan[4] = { 0, 25, 50, 75 };
	int armpro = 0;
	int tmp;

	/* shields are good for defense */
	if (uarms &&
	    acan[objects[uarms->otyp].a_can] > rn2(100)) return TRUE;

	/* special handling for ladies */
	if (flags.female && uarm && uarm->otyp == MAID_DRESS) {
		armpro = 50;
		if (uarmc && (uarmc->otyp == KITCHEN_APRON || uarmc->otyp == FRILLED_APRON))
			armpro += 25;
		if (uarmh && uarmh->otyp == KATYUSHA)
			armpro += 5;
	} else if (flags.female && uarm && uarm->otyp == NURSE_UNIFORM) {
		armpro = 75;
		if (uarmh && uarmh->otyp == NURSE_CAP)
			armpro += 20;
	}

	/* other armor */
	if (uarm  && armpro < (tmp = acan[objects[uarm->otyp ].a_can])) armpro = tmp;
	if (uarmc && armpro < (tmp = acan[objects[uarmc->otyp].a_can])) armpro = tmp;
	if (uarmh && armpro < (tmp = acan[objects[uarmh->otyp].a_can])) armpro = tmp;

	return (rn2(100) < armpro);
}

boolean
is_full_resist(prop)
uchar prop;
{
	long m = u.uprops[prop].extrinsic;
	if (m & (W_RING | W_ART | W_ARTI)) return TRUE;
	if ((m & W_ARM) && uarm && Is_dragon_armor(uarm)) return TRUE;
	if ((m & W_WEP) && uwep && uwep->oartifact) return TRUE;
	return FALSE;
}

int
reduce_dmg_amount(otmp)
struct obj *otmp;
{
	int tmp;
	tmp = ARM_BASE(otmp);
	if (otmp->spe >= 0)
	     tmp += min(otmp->spe, tmp);
	else tmp = max(tmp + otmp->spe, 0);
	return tmp;
}

int
reduce_damage(dmg, hitpart)
int dmg;
int hitpart;
{
#if 1 /* test... */
	int tmp;	/* damage is reduced by rnd(tmp) */

	tmp = 0;

	/* protection from rings/spells */
	dmg -= u.uprotection;
	dmg -= u.uspellprot;
	dmg -= u.ublessed;
	if (dmg < 1) return 1;

	/* shield */
	if (uarms)
		tmp += reduce_dmg_amount(uarms);

	switch ( (hitpart > 0) ? hitpart : rn2(DAMCAN_MAX) ) {
	    case DAMCAN_HEAD:	/* head */
		if (uarmh) {
			tmp += reduce_dmg_amount(uarmh);
			/* maid dress' special power */
			if(flags.female && uarm && uarm->otyp == MAID_DRESS &&
			   uarmh->otyp == KATYUSHA) tmp += 2;
		}
		break;
	    case DAMCAN_FEET:	/* feet */
		if (uarmf)
			tmp += reduce_dmg_amount(uarmf);
		break;
	    case DAMCAN_HAND:	/* hand(s) */
		if (uarmg)
			tmp += reduce_dmg_amount(uarmg);
		break;
	    default:
		if (uarm)
			tmp += reduce_dmg_amount(uarm);
		if (uarmc) {
			tmp += reduce_dmg_amount(uarmc);
			/* maid dress' special power */
			if(flags.female && uarm && uarm->otyp == MAID_DRESS) {
				if (uarmc->otyp == KITCHEN_APRON) tmp += 3;
				if (uarmc->otyp == FRILLED_APRON) tmp += 4;
			}
		}
/* Because Hawaiian shirt do not improve AC, it do not reduce damage */
//#ifdef TOURIST
//		if (uarmu)
//			tmp += reduce_dmg_amount(uarmu);
//#endif
		break;
	}
	/* reduce damage by armor rating at the part being hit */
	if (tmp > 0) dmg -= rnd(tmp);
	/* at least 1 damage remains */
	if (dmg < 1) dmg = 1;
	return dmg;

#else /* test... */

	int tmp;	/* damage is reduced by rnd(tmp) */

	tmp = 0;

	/* protection from rings/spells */
	dmg -= u.uprotection;
	dmg -= u.uspellprot;
	dmg -= u.ublessed;
	if (dmg < 0) return 0;

	/* shield */
	if (uarms)
		tmp += reduce_dmg_amount(uarms);

	switch ( (hitpart > 0) ? hitpart : rn2(DAMCAN_MAX) ) {
	    case DAMCAN_HEAD:	/* head */
		if (uarmh) {
			tmp += reduce_dmg_amount(uarmh);
			/* maid dress' special power */
			if(flags.female && uarm && uarm->otyp == MAID_DRESS &&
			   uarmh->otyp == KATYUSHA) tmp += 2;
		}
		break;
	    case DAMCAN_FEET:	/* feet */
		if (uarmf)
			tmp += reduce_dmg_amount(uarmf);
		break;
	    case DAMCAN_HAND:	/* hand(s) */
		if (uarmg)
			tmp += reduce_dmg_amount(uarmg);
		break;
	    default:
		if (uarm)
			tmp += reduce_dmg_amount(uarm);
		if (uarmc) {
			tmp += reduce_dmg_amount(uarmc);
			/* maid dress' special power */
			if(flags.female && uarm && uarm->otyp == MAID_DRESS) {
				if (uarmc->otyp == KITCHEN_APRON) tmp += 3;
				if (uarmc->otyp == FRILLED_APRON) tmp += 4;
			}
		}
/* Because Hawaiian shirt do not improve AC, it do not reduce damage */
//#ifdef TOURIST
//		if (uarmu)
//			tmp += reduce_dmg_amount(uarmu);
//#endif
		break;
	}
	/* reduce damage by armor rating at the part being hit */
	if (tmp > 0) dmg -= rnd(tmp);
	/* at least 1 damage remains */
	if (dmg < 0) dmg = 0;
	return dmg;
#endif
}

/*mhitu.c*/
