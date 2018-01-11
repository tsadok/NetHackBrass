/*	SCCS Id: @(#)mhitm.c	3.4	2003/01/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"
#include "edog.h"

extern boolean notonhead;

#ifdef OVLB

static NEARDATA boolean vis, far_noise;
static NEARDATA long noisetime;
static NEARDATA struct obj *otmp;

static const char brief_feeling[] =
	E_J("have a %s feeling for a moment, then it passes.",
	    "一瞬%sにおそわれたが、それはすぐに過ぎ去った。");

STATIC_DCL char *FDECL(mon_nam_too, (char *,struct monst *,struct monst *));
STATIC_DCL void FDECL(mrustm, (struct monst *, struct monst *, struct obj *));
STATIC_DCL int FDECL(hitmm, (struct monst *,struct monst *,struct attack *));
STATIC_DCL int FDECL(gazemm, (struct monst *,struct monst *,struct attack *));
STATIC_DCL int FDECL(gulpmm, (struct monst *,struct monst *,struct attack *));
STATIC_DCL int FDECL(explmm, (struct monst *,struct monst *,struct attack *));
STATIC_DCL int FDECL(mdamagem, (struct monst *,struct monst *,struct attack *));
STATIC_DCL void FDECL(mswingsm, (struct monst *, struct monst *, struct obj *));
STATIC_DCL void FDECL(noises,(struct monst *,struct attack *));
STATIC_DCL void FDECL(missmm,(struct monst *,struct monst *,struct attack *));
STATIC_DCL int FDECL(passivemm, (struct monst *, struct monst *, BOOLEAN_P, int));

/* Needed for the special case of monsters wielding vorpal blades (rare).
 * If we use this a lot it should probably be a parameter to mdamagem()
 * instead of a global variable.
 */
static int dieroll;

/* returns mon_nam(mon) relative to other_mon; normal name unless they're
   the same, in which case the reference is to {him|her|it} self */
STATIC_OVL char *
mon_nam_too(outbuf, mon, other_mon)
char *outbuf;
struct monst *mon, *other_mon;
{
	Strcpy(outbuf, mon_nam(mon));
	if (mon == other_mon)
#ifndef JP
	    switch (pronoun_gender(mon)) {
	    case 0:	Strcpy(outbuf, "himself");  break;
	    case 1:	Strcpy(outbuf, "herself");  break;
	    default:	Strcpy(outbuf, "itself"); break;
	    }
#else
	    Strcpy(outbuf, "自分");
#endif /*JP*/
	return outbuf;
}

STATIC_OVL void
noises(magr, mattk)
	register struct monst *magr;
	register struct	attack *mattk;
{
	boolean farq = (distu(magr->mx, magr->my) > 15);

	if(flags.soundok && (farq != far_noise || moves-noisetime > 10)) {
		far_noise = farq;
		noisetime = moves;
#ifndef JP
		You_hear("%s%s.",
			(mattk->aatyp == AT_EXPL) ? "an explosion" : "some noises",
			farq ? " in the distance" : "");
#else
		You_hear("%s%s",
			farq ? "遠くに" : "",
			(mattk->aatyp == AT_EXPL) ? "爆発音を" : "何かが戦う音を");
#endif /*JP*/
	}
}

STATIC_OVL
void
missmm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	struct attack *mattk;
{
	const char *fmt;
	char buf[BUFSZ], mdef_name[BUFSZ];

	if (vis) {
		if (!canspotmons(magr))
		    map_invisible(magr->mx, magr->my);
		if (!canspotmons(mdef))
		    map_invisible(mdef->mx, mdef->my);
		if (mdef->m_ap_type) seemimic(mdef);
		if (magr->m_ap_type) seemimic(magr);
		fmt = (could_seduce(magr,mdef,mattk) && !magr->mcan) ?
#ifndef JP
			"%s pretends to be friendly to" : "%s misses";
#else
			"%sは%sに友好的なふりをした。" : "%sの攻撃は%sにかわされた。";
#endif /*JP*/
#ifndef JP
		Sprintf(buf, fmt, Monnam(magr));
		pline("%s %s.", buf, mon_nam_too(mdef_name, mdef, magr));
#else
		Sprintf(buf, mon_nam(magr));
		pline(fmt, buf, mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
	} else  noises(magr, mattk);
}

/*
 *  fightm()  -- fight some other monster
 *
 *  Returns:
 *	0 - Monster did nothing.
 *	1 - If the monster made an attack.  The monster might have died.
 *
 *  There is an exception to the above.  If mtmp has the hero swallowed,
 *  then we report that the monster did nothing so it will continue to
 *  digest the hero.
 */
int
fightm(mtmp)		/* have monsters fight each other */
	register struct monst *mtmp;
{
	register struct monst *mon;
	int result, has_u_swallowed;
	int i, j , x, y;

	/* perhaps the monster will resist Conflict */
	if(resist(mtmp, RING_CLASS, 0, 0))
	    return(0);

	if(u.ustuck == mtmp) {
	    /* perhaps we're holding it... */
	    if(itsstuck(mtmp))
		return(0);
	}
	has_u_swallowed = (u.uswallow && (mtmp == u.ustuck));

#ifdef MONSTEED
	if (is_mridden(mtmp)) {
	    int tmp = rn2(7);
	    if (!tmp) {
		if (canseemon(mtmp))
		    pline(E_J("%s bucks wildly!",
			      "%sは荒々しく跳ね上げた！"), Monnam(mtmp));
		mdismount_steed(mtmp);
		return 1;
	    } else if (tmp == 1) {
		if (canseemon(mtmp))
		    pline(E_J("%s tries to throw off the rider!",
			      "%sは乗り手を振り落とそうとした！"), Monnam(mtmp));
		return 1;
	    }
	}
#endif /*MONSTEED*/

	for (i=0, j=rn2(8); i<8; i++) {
	    x = mtmp->mx + xdir[(i+j) & 0x07];
	    y = mtmp->my + ydir[(i+j) & 0x07];
	    if (!isok(x, y) || (x == u.ux && y == u.uy)) continue;
	    mon = m_at(x, y);

	    /* Be careful to ignore monsters that are already dead, since we
	     * might be calling this before we've cleaned them up.  This can
	     * happen if the monster attacked a cockatrice bare-handedly, for
	     * instance.
	     */
	    if (!mon || DEADMONSTER(mon) ||
		!monnear(mtmp,mon->mx,mon->my)) continue;

#ifdef MONSTEED
	    mon = target_rider_or_steed(mtmp, mon);
#endif

	    if(!u.uswallow && (mtmp == u.ustuck)) {
		if(!rn2(4)) {
		    pline(E_J("%s releases you!",
			      "%sはあなたを放した！"), Monnam(mtmp));
		    u.ustuck = 0;
		} else
		    break;
	    }

	    /* mtmp can be killed */
	    bhitpos.x = mon->mx;
	    bhitpos.y = mon->my;
	    notonhead = 0;
	    result = mattackm(mtmp,mon);
		    if (result & MM_AGR_DIED) return 1;	/* mtmp died */
	    /*
	     *  If mtmp has the hero swallowed, lie and say there
	     *  was no attack (this allows mtmp to digest the hero).
	     */
	    if (has_u_swallowed) return 0;

	    /* Allow attacked monsters a chance to hit back. Primarily
	     * to allow monsters that resist conflict to respond.
	     */
	    if ((result & MM_HIT) && !(result & MM_DEF_DIED) &&
		rn2(4) && mon->movement >= NORMAL_SPEED) {
		mon->movement -= NORMAL_SPEED;
		notonhead = 0;
		(void) mattackm(mon, mtmp);	/* return attack */
	    }

	    return ((result & MM_HIT) ? 1 : 0);
	}
	return 0;
}

/*
 * mattackm() -- a monster attacks another monster.
 *
 * This function returns a result bitfield:
 *
 *	    --------- aggressor died
 *	   /  ------- defender died
 *	  /  /  ----- defender was hit
 *	 /  /  /
 *	x  x  x
 *
 *	0x4	MM_AGR_DIED
 *	0x2	MM_DEF_DIED
 *	0x1	MM_HIT
 *	0x0	MM_MISS
 *
 * Each successive attack has a lower probability of hitting.  Some rely on the
 * success of previous attacks.  ** this doen't seem to be implemented -dl **
 *
 * In the case of exploding monsters, the monster dies as well.
 */
int
mattackm(magr, mdef)
    register struct monst *magr,*mdef;
{
    int		    i,		/* loop counter */
		    tmp,	/* amour class difference */
		    strike,	/* hit this attack */
		    attk,	/* attack attempted this time */
		    struck = 0,	/* hit at least once */
		    res[NATTK];	/* results of all attacks */
    struct attack   *mattk, alt_attk;
    struct permonst *pa, *pd;

    if (!magr || !mdef) return(MM_MISS);		/* mike@genat */
    if (!magr->mcanmove || magr->msleeping) return(MM_MISS);
    pa = magr->data;  pd = mdef->data;

    /* Grid bugs cannot attack at an angle. */
    if (pa == &mons[PM_GRID_BUG] && magr->mx != mdef->mx
						&& magr->my != mdef->my)
	return(MM_MISS);

    /* Calculate the armour class differential. */
    tmp = find_mac(mdef) + magr->m_lev;
    if (mdef->mconf || !mdef->mcanmove || mdef->msleeping) {
	tmp += 4;
	mdef->msleeping = 0;
    }
    /*if (is_swallowed(magr) && is_swallowing(mdef)) tmp = 100;*/

    /* undetect monsters become un-hidden if they are attacked */
    if (mdef->mundetected) {
	mdef->mundetected = 0;
	newsym(mdef->mx, mdef->my);
	if(canseemon(mdef) && !sensemon(mdef)) {
#ifndef JP
	    if (u.usleep) You("dream of %s.",
				(mdef->data->geno & G_UNIQ) ?
				a_monnam(mdef) : makeplural(m_monnam(mdef)));
	    else pline("Suddenly, you notice %s.", a_monnam(mdef));
#else
	    if (u.usleep) You("%sの夢を見た。", mon_nam(mdef));
	    else pline("突然、あなたは%sに気づいた。", mon_nam(mdef));
#endif /*JP*/
	}
    }

    /* Elves hate orcs. */
    if (is_elf(pa) && is_orc(pd)) tmp++;


    /* Set up the visibility of action */
    vis = (cansee(magr->mx,magr->my) && cansee(mdef->mx,mdef->my) && (canspotmon(magr) || canspotmon(mdef))) &&
	    !is_swallowed(magr);

    /*	Set flag indicating monster has moved this turn.  Necessary since a
     *	monster might get an attack out of sequence (i.e. before its move) in
     *	some cases, in which case this still counts as its move for the round
     *	and it shouldn't move again.
     */
    magr->mlstmv = monstermoves;

    /* Now perform all attacks for the monster. */
    for (i = 0; i < NATTK; i++) {
	res[i] = MM_MISS;
	mattk = getmattk(pa, i, res, &alt_attk);
	otmp = (struct obj *)0;
	attk = 1;
	switch (mattk->aatyp) {
	    case AT_WEAP:		/* "hand to hand" attacks */
		if (magr->weapon_check == NEED_WEAPON || !MON_WEP(magr)) {
		    magr->weapon_check = NEED_HTH_WEAPON;
		    if (mon_wield_item(magr) != 0) return 0;
		}
		possibly_unwield(magr, FALSE);
		otmp = MON_WEP(magr);

		if (otmp) {
		    if (vis) mswingsm(magr, mdef, otmp);
		    tmp += hitval(otmp, mdef);
		}
		/* fall through */
	    case AT_CLAW:
	    case AT_KICK:
	    case AT_BITE:
	    case AT_STNG:
	    case AT_TUCH:
	    case AT_BUTT:
	    case AT_TENT:
		/* Nymph that teleported away on first attack? */
		if (distmin(magr->mx,magr->my,mdef->mx,mdef->my) > 1)
		    return MM_MISS;
		/* Monsters won't attack cockatrices physically if they
		 * have a weapon instead.  This instinct doesn't work for
		 * players, or under conflict or confusion. 
		 */
		if (!magr->mconf && !Conflict && otmp &&
		    mattk->aatyp != AT_WEAP && touch_petrifies(mdef->data)) {
		    strike = 0;
		    break;
		}
		dieroll = rnd(20 + i);
		strike = (tmp > dieroll);
		/* KMH -- don't accumulate to-hit bonuses */
		if (otmp) {
		    tmp -= hitval(otmp, mdef);
		}
		if (strike) {
		    res[i] = hitmm(magr, mdef, mattk);
		    if((mdef->data == &mons[PM_BLACK_PUDDING] ||
			mdef->data == &mons[PM_BROWN_PUDDING] ||
			mdef->data == &mons[PM_GREEN_SLIME])
		       && otmp && get_material(otmp) == IRON
		       && mdef->mhp > 1 && !mdef->mcan)
		    {
			if (clone_mon(mdef, 0, 0)) {
			    if (vis) {
				char buf[BUFSZ];

				Strcpy(buf, Monnam(mdef));
				pline(E_J("%s divides as %s hits it!",
					  "%sは%sの攻撃で分裂した！"), buf, mon_nam(magr));
			    }
			}
		    }
		} else
		    missmm(magr, mdef, mattk);
		break;

	    case AT_HUGS:	/* automatic if prev two attacks succeed */
		strike = (i >= 2 && res[i-1] == MM_HIT && res[i-2] == MM_HIT);
		if (strike)
		    res[i] = hitmm(magr, mdef, mattk);

		break;

	    case AT_GAZE:
		strike = 0;	/* will not wake up a sleeper */
		res[i] = gazemm(magr, mdef, mattk);
		break;

	    case AT_EXPL:
		res[i] = explmm(magr, mdef, mattk);
		if (res[i] == MM_MISS) { /* cancelled--no attack */
		    strike = 0;
		    attk = 0;
		} else
		    strike = 1;	/* automatic hit */
		break;

	    case AT_ENGL:
#ifdef STEED
		if (u.usteed && (mdef == u.usteed)) {
		    strike = 0;
		    break;
		} 
#endif
		if (mattk->adtyp == AD_DRWN &&
		    (is_rider(mdef->data) || amphibious(mdef->data))) {
			strike = 0;
			break;
		}
		/* Engulfing attacks are directed at the hero if
		 * possible. -dlc
		 */
		if (u.uswallow && magr == u.ustuck)
		    strike = 0;
		else {
		    if ((strike = (tmp > rnd(20+i))))
			res[i] = gulpmm(magr, mdef, mattk);
		    else
			missmm(magr, mdef, mattk);
		}
		break;

	    default:		/* no attack */
		strike = 0;
		attk = 0;
		break;
	}

	if (attk && !(res[i] & MM_AGR_DIED))
	    res[i] = passivemm(magr, mdef, strike, res[i] & MM_DEF_DIED);

	if (res[i] & MM_DEF_DIED) return res[i];

	/*
	 *  Wake up the defender.  NOTE:  this must follow the check
	 *  to see if the defender died.  We don't want to modify
	 *  unallocated monsters!
	 */
	if (strike) mdef->msleeping = 0;

	if (res[i] & MM_AGR_DIED)  return res[i];
	/* return if aggressor can no longer attack */
	if (!magr->mcanmove || magr->msleeping) return res[i];
	if (res[i] & MM_HIT) struck = 1;	/* at least one hit */
    }

    return(struck ? MM_HIT : MM_MISS);
}

/* Returns the result of mdamagem(). */
STATIC_OVL int
hitmm(magr, mdef, mattk)
	register struct monst *magr,*mdef;
	struct	attack *mattk;
{
	if(vis){
		int compat;
		char buf[BUFSZ], mdef_name[BUFSZ];

		if (!canspotmons(magr))
		    map_invisible(magr->mx, magr->my);
		if (!canspotmons(mdef))
		    map_invisible(mdef->mx, mdef->my);
		if(mdef->m_ap_type) seemimic(mdef);
		if(magr->m_ap_type) seemimic(magr);
		if((compat = could_seduce(magr,mdef,mattk)) && !magr->mcan) {
#ifndef JP
			Sprintf(buf, "%s %s", Monnam(magr),
				mdef->mcansee ? "smiles at" : "talks to");
			pline("%s %s %s.", buf, mon_nam(mdef),
				compat == 2 ?
					"engagingly" : "seductively");
#else
			Strcpy(buf, mon_nam(magr));
			pline("%sは%sに向かって%s%s。", buf, mon_nam(mdef),
				compat == 2 ? "可愛らしく" : "誘うように",
				mdef->mcansee ? "語りかけた" : "微笑んだ");
#endif /*JP*/
		} else {
		    char magr_name[BUFSZ];

		    Strcpy(magr_name, Monnam(magr));
		    switch (mattk->aatyp) {
			case AT_BITE:
#ifndef JP
				Sprintf(buf, "%s bites", magr_name);
#else
				pline("%sは%sに噛みついた。", magr_name,
				      mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
				break;
			case AT_STNG:
#ifndef JP
				Sprintf(buf, "%s stings", magr_name);
#else
				pline("%sは%sを刺した。", magr_name,
				      mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
				break;
			case AT_BUTT:
#ifndef JP
				Sprintf(buf, "%s butts", magr_name);
#else
				pline("%sは%sを蹴った。", magr_name,
				      mon_nam_too(mdef_name, mdef, magr));
				Strcpy(buf, "は%sを蹴った");
#endif /*JP*/
				break;
			case AT_TUCH:
#ifndef JP
				Sprintf(buf, "%s touches", magr_name);
#else
				pline("%sは%sに触った。", magr_name,
				      mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
				break;
			case AT_TENT:
#ifndef JP
				Sprintf(buf, "%s tentacles suck",
					s_suffix(magr_name));
#else
				pline("%sは触手で%sに吸いついた。", magr_name,
				      mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
				break;
			case AT_HUGS:
				if (magr != u.ustuck) {
#ifndef JP
				    Sprintf(buf,"%s squeezes", magr_name);
#else
				pline("%sは%sを締め上げた。", magr_name,
				      mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
				    break;
				}
			default:
#ifndef JP
				Sprintf(buf,"%s hits", magr_name);
#else
				pline("%sは%sを攻撃した。", magr_name,
				      mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
		    }
#ifndef JP
		    pline("%s %s.", buf, mon_nam_too(mdef_name, mdef, magr));
#endif /*JP*/
		}
	} else  noises(magr, mattk);
	return(mdamagem(magr, mdef, mattk));
}

/* Returns the same values as mdamagem(). */
STATIC_OVL int
gazemm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	struct attack *mattk;
{
	char buf[BUFSZ];

	if (is_swallowed(magr)) return MM_MISS;

	if(vis) {
#ifndef JP
		Sprintf(buf,"%s gazes at", Monnam(magr));
		pline("%s %s...", buf, mon_nam(mdef));
#else
		Strcpy(buf, mon_nam(magr));
		pline("%sは%sをにらんだ…。", buf, mon_nam(mdef));
#endif /*JP*/
	}

	if (magr->mcan || !magr->mcansee ||
	    (magr->minvis && !perceives(mdef->data)) ||
	    !mdef->mcansee || mdef->msleeping) {
	    if(vis) pline(E_J("but nothing happens.","だが、何も起きなかった。"));
	    return(MM_MISS);
	}
	/* call mon_reflects 2x, first test, then, if visible, print message */
	if (magr->data == &mons[PM_MEDUSA] && mon_reflects(mdef, (char *)0)) {
	    if (canseemon(mdef))
		(void) mon_reflects(mdef,
				    E_J("The gaze is reflected away by %s %s.",
					"眼光は%s%sにはね返された。"));
	    if (mdef->mcansee) {
		if (mon_reflects(magr, (char *)0)) {
		    if (canseemon(magr))
			(void) mon_reflects(magr,
					E_J("The gaze is reflected away by %s %s.",
					    "眼光は%s%sにはね返された。"));
		    return (MM_MISS);
		}
		if (mdef->minvis && !perceives(magr->data)) {
		    if (canseemon(magr)) {
			pline(E_J("%s doesn't seem to notice that %s gaze was reflected.",
				  "%sは%sにらみがはね返されたことに気づかないようだ。"),
			      Monnam(magr), mhis(magr));
		    }
		    return (MM_MISS);
		}
		if (canseemon(magr))
		    pline(E_J("%s is turned to stone!","%sは石になった！"), Monnam(magr));
		monstone(magr);
		if (magr->mhp > 0) return (MM_MISS);
		return (MM_AGR_DIED);
	    }
	}

	return(mdamagem(magr, mdef, mattk));
}

/* Returns the same values as mattackm(). */
STATIC_OVL int
gulpmm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	register struct	attack *mattk;
{
	int ax, ay, dx, dy;
	int status;
	int tim_tmp;
	char buf[BUFSZ];
	struct obj *obj;
	coord cc;

	/* defender is too large to swallow */
	if (mdef->data->msize >= MZ_HUGE) return MM_MISS;

	/* if defender is already swallowing another monster, avoid nesting */
	if (is_swallowing(mdef) || is_swallowing(magr)) return MM_MISS;

	/* unleash the victim */
	if (mdef->mleashed) {
		if (canseemon(mdef))
#ifndef JP
		    pline("%s leash goes slack.", s_suffix(Monnam(mdef)));
#else
		    pline("%sを繋いでいた引き綱が緩んだ。", mon_nam(mdef));
#endif /*JP*/
		m_unleash(mdef, FALSE);
	}

	if (vis) {
#ifndef JP
		Sprintf(buf,"%s swallows", Monnam(magr));
		pline("%s %s.", buf, mon_nam(mdef));
#else
		Strcpy(buf, mon_nam(magr));
		pline("%sは%sを飲み込んだ。", buf, mon_nam(mdef));
#endif /*JP*/
	}
	for (obj = mdef->minvent; obj; obj = obj->nobj)
	    (void) snuff_lit(obj);

#ifdef MONSTEED
	/* if msteed is swallowed, throw off its rider first */
	if (is_mridden(mdef)) mdismount_steed(mdef);
#endif

	/*
	 *  Leave the defender in the monster chain at it's current position,
	 *  but don't leave it on the screen.  Move the agressor to the def-
	 *  ender's position.
	 */
	ax = magr->mx; ay = magr->my;
	dx = mdef->mx; dy = mdef->my;
	remove_monster(ax, ay);
	place_monster(magr, dx, dy);
	newsym(ax,ay);			/* erase old position */
	newsym(dx,dy);			/* update new position */

#ifdef MONSTEED
	if (is_mriding(mdef)) {
	    /* place msteed near (dx,dy) */
	    struct monst *mstd;
	    mlanding_spot(mdef, &cc); /* (ax,ay) is the guaranteed landing spot */
	    mstd = unlink_mlink(mdef);
	    unlink_mlink(mstd);
	    place_monster(mstd, cc.x, cc.y);
	    newsym(cc.x, cc.y);
	}
#endif
	magr->mlink = mdef;	magr->mlinktyp = MLT_SWALLOWING;
	mdef->mlink = magr;	mdef->mlinktyp = MLT_SWALLOWER;

	if (mattk->adtyp == AD_DGST) {
		tim_tmp = 25 - (int)magr->m_lev;
		if (tim_tmp > 0) tim_tmp = rnd(tim_tmp) / 2;
		else if (tim_tmp < 0) tim_tmp = -(rnd(-tim_tmp) / 2);
		magr->meating = ((tim_tmp < 2) ? 2 : tim_tmp);
	} else if (mattk->adtyp == AD_DRWN) {
		magr->meating = 3;
	}
	vis = FALSE;

	return MM_HIT;
}

int
mdigest(magr)
struct monst *magr;
{
	struct monst *mdef;
	struct attack *mattk;
	int status;

	if (!is_swallowing(magr)) return FALSE;

	mattk = attacktype_fordmg(magr->data, AT_ENGL, AD_ANY);
	if (!mattk) {
		impossible("Swallower doesn't have AT_ENGL?");
		return FALSE;
	}

	mdef = magr->mlink;

	if (magr->mhp < (magr->mhpmax+3)/4 || magr->mflee) {
		coord cc;
		if (mlanding_spot(magr, &cc) ||
		    enexto(&cc, magr->mx, magr->my, magr->data)) {
			if (mregurgitate(magr, TRUE, FALSE)) {
			    (void) mintrap(mdef);
			    (void) minliquid(mdef);
			}
			return TRUE;
		}
	}

	status = mdamagem(magr, mdef, mattk);

	return TRUE;
}

int
minstomach(mdef)
struct monst *mdef;
{
	char manm[BUFSZ];
	struct monst *magr;
	struct obj *otmp;
	int animal;

	if (!is_swallowed(mdef)) return FALSE;
	if (!mdef->mcanmove || mdef->msleeping) return TRUE;

	magr = mdef->mlink;
	animal = is_animal(magr->data);

	/* search for a way to escape */
	if (!is_animal(mdef->data) && !mindless(mdef->data) &&
	    !nohands(mdef->data) && !mdef->mconf)
	for (otmp = mdef->minvent; otmp; otmp = otmp->nobj) {
	    if (animal && otmp->otyp == WAN_DIGGING && otmp->spe > 0) {
		if (canseemon(magr)) {
		    Strcpy(manm, mon_nam(magr));
#ifndef JP
		    pline("Suddenly %s %s wall is pierced and %s escapes from it!",
			  s_suffix(manm), mbodypart(magr, STOMACH), mdef);
#else
		    pline("突然、%sの腹に穴が開き、%sが飛び出してきた！",
			  manm, mon_nam(mdef));
#endif /*JP*/
		}
		magr->mhp = 1;
		otmp->spe--;
		mregurgitate(magr, FALSE, TRUE);
		return TRUE;
	    } else if (!level.flags.noteleport) {
		int x, y;
		if (can_teleport(mdef->data) && !mdef->mspec_used) {
		    mdef->mspec_used += d(10, 10);
		} else if (otmp->otyp == WAN_TELEPORTATION && otmp->spe > 0) {
		    if (flags.soundok && (distu(magr->mx,magr->my) <= (BOLT_LIM+1)*(BOLT_LIM+1)))
			You_hear(E_J("a zap insize from %s.","、%sの中で杖が振られる音を"),
				 mon_nam(magr));
		    otmp->spe--;
		} else if (otmp->otyp == SCR_TELEPORTATION && !otmp->cursed) {
		    if (flags.soundok)
			You_hear(E_J("%s reading a scroll.","%sが巻物を読む声を"),
				 something);
		    m_useup(mdef, otmp);
		} else continue;
		/* let the victim teleport */
		unlink_mlink(magr);
		unlink_mlink(mdef);
		magr->meating = 0;
		x = magr->mx;
		y = magr->my;
		remove_monster(x, y);
		place_monster(mdef, x, y);
		if (!rloc(mdef, TRUE)) mongone(mdef);
		place_monster(magr, x, y);
		newsym(x, y);
		return TRUE;
	    }
	}

	/* else, attack the swallower */
	mattackm(mdef, magr);
	return TRUE;
}

int
mregurgitate(magr, msg, force)
struct monst *magr;
boolean msg, force;
{
	char manm[BUFSZ];
	struct monst *mdef;
	coord cc;
	if (mlanding_spot(magr, &cc) ||
	    enexto(&cc, magr->mx, magr->my, magr->data)) {
		mdef = unlink_mlink(magr);
		unlink_mlink(mdef);
		mdef->mx = mdef->my = 0; /* for rloc_to() */
		magr->meating = 0;
		rloc_to(mdef, cc.x, cc.y);
		if (msg && (cansee(magr->mx, magr->my) ||
			    cansee(cc.x, cc.y))) {
		    Strcpy(manm, Monnam(magr));
#ifndef JP
		    pline("%s regurgitates %s!", manm, mon_nam(mdef));
#else
		    pline("%sは%sを%s出した！", manm, mon_nam(mdef),
			  is_animal(magr->data) ? "吐き" : "放り");
#endif /*JP*/
		}
		return TRUE;
	} else if (force) {
	    int x, y;
	    /* no spot to place mdef... kill it */
	    mdef = unlink_mlink(magr);
	    unlink_mlink(mdef);
	    x = magr->mx;
	    y = magr->my;
	    remove_monster(x, y); /* temporarily remove magr */
	    place_monster(mdef, x, y);
	    mondead(mdef);
	    place_monster(magr, x, y); /* restore magr */
	} /* if no spot to land, but not forced to be regurgitated,
	     let's keep mdef in the stomach of magr */
	return FALSE;
}

STATIC_OVL int
explmm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	register struct	attack *mattk;
{
	int result;

	if (magr->mcan)
	    return MM_MISS;

	if(cansee(magr->mx, magr->my))
		pline(E_J("%s explodes!","%sは爆発した！"), Monnam(magr));
	else	noises(magr, mattk);

	result = mdamagem(magr, mdef, mattk);

	/* Kill off agressor if it didn't die. */
	if (!(result & MM_AGR_DIED)) {
	    mondead(magr);
	    if (magr->mhp > 0) return result;	/* life saved */
	    result |= MM_AGR_DIED;
	}
	if (magr->mtame)	/* give this one even if it was visible */
	    You(brief_feeling, E_J("melancholy","深い悲しみ"));

	return result;
}

/*
 *  See comment at top of mattackm(), for return values.
 */
STATIC_OVL int
mdamagem(magr, mdef, mattk)
	register struct monst	*magr, *mdef;
	register struct attack	*mattk;
{
	struct obj *obj;
	char buf[BUFSZ];
	struct permonst *pa = magr->data, *pd = mdef->data;
	int num, tmp = d((int)mattk->damn, (int)mattk->damd);
	boolean cancelled;

	if (touch_petrifies(pd) && !resists_ston(magr)) {
	    long protector = attk_protection((int)mattk->aatyp),
		 wornitems = magr->misc_worn_check;

	    /* wielded weapon gives same protection as gloves here */
	    if (otmp != 0) wornitems |= W_ARMG;

	    if (protector == 0L ||
		  (protector != ~0L && (wornitems & protector) != protector)) {
		if (poly_when_stoned(pa)) {
		    mon_to_stone(magr);
		    return MM_HIT; /* no damage during the polymorph */
		}
		if (vis) pline(E_J("%s turns to stone!","%sは石になった！"), Monnam(magr));
		monstone(magr);
		if (magr->mhp > 0) return 0;
		else if (magr->mtame && !vis)
		    You(brief_feeling, E_J("peculiarly sad","不思議な悲しみ"));
		return MM_AGR_DIED;
	    }
	}

	/* cancellation factor is the same as when attacking the hero */
	cancelled = magr->mcan || magic_negation(mdef);

	switch(mattk->adtyp) {
	    case AD_DGST:
		/* eating a Rider or its corpse is fatal */
		if (is_rider(mdef->data)) {
		    if (vis)
#ifndef JP
			pline("%s %s!", Monnam(magr),
			      mdef->data == &mons[PM_FAMINE] ?
				"belches feebly, shrivels up and dies" :
			      mdef->data == &mons[PM_PESTILENCE] ?
				"coughs spasmodically and collapses" :
				"vomits violently and drops dead");
#else
			pline("%sは%s!", mon_nam(magr),
			      mdef->data == &mons[PM_FAMINE] ?
				"弱々しくゲップをすると、しなびて死んだ" :
			      mdef->data == &mons[PM_PESTILENCE] ?
				"咳の発作を起こし、くずおれた" :
				"激しく嘔吐すると、即死した");
#endif /*JP*/
		    mondied(magr);
		    if (magr->mhp > 0) return 0;	/* lifesaved */
		    else if (magr->mtame && !vis)
			You(brief_feeling, E_J("queasy","吐き気"));
		    return MM_AGR_DIED;
		}

		/* normal damage to the victim while digesting */
		if (mdef->mhp > tmp && magr->meating && --(magr->meating))
		    break;

		/* totally digested! */
		if(flags.verbose && flags.soundok) verbalize(E_J("Burrrrp!","ゲエエェェップ！"));
		tmp = mdef->mhp;
		/* Use up amulet of life saving */
		if (!!(obj = mlifesaver(mdef))) m_useup(mdef, obj);

		/* Is a corpse for nutrition possible?  It may kill magr */
		if (!corpse_chance(mdef, magr, TRUE) || magr->mhp < 1)
		    break;

		/* Pets get nutrition from swallowing monster whole.
		 * No nutrition from G_NOCORPSE monster, eg, undead.
		 * DGST monsters don't die from undead corpses
		 */
		num = monsndx(mdef->data);
		if (magr->mtame && !magr->isminion &&
		    !(mvitals[num].mvflags & G_NOCORPSE)) {
		    struct obj *virtualcorpse = mksobj(CORPSE, FALSE, FALSE);
		    int nutrit;

		    virtualcorpse->corpsenm = num;
		    virtualcorpse->owt = weight(virtualcorpse);
		    nutrit = dog_nutrition(magr, virtualcorpse);
		    dealloc_obj(virtualcorpse);

		    /* only 50% nutrition, 25% of normal eating time */
//		    if (magr->meating > 1) magr->meating = (magr->meating+3)/4;
		    magr->meating = 0;
		    if (nutrit > 1) nutrit /= 2;
		    EDOG(magr)->hungrytime += nutrit;
		}
		/* heal the attacker */
		magr->mhp = min(magr->mhp + mdef->mhpmax / 2, magr->mhpmax);
		break;
	    case AD_STUN:
		if (magr->mcan) break;
		if (canseemon(mdef))
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
		if (magr->mcan) {
		    tmp = 0;
		    break;
		}
		goto physical;
	    case AD_WERE:
	    case AD_HEAL:
	    case AD_PHYS:
 physical:
		if (mattk->aatyp == AT_KICK && thick_skinned(pd)) {
		    tmp = 0;
		} else if(mattk->aatyp == AT_WEAP) {
		    if(otmp) {
			if (otmp->otyp == CORPSE &&
				touch_petrifies(&mons[otmp->corpsenm]))
			    goto do_stone;
			tmp += dmgval(otmp, mdef);
			if (otmp->oartifact) {
			    (void)artifact_hit(magr,mdef, otmp, &tmp, dieroll);
			    if (mdef->mhp <= 0)
				return (MM_DEF_DIED |
					(grow_up(magr,mdef) ? 0 : MM_AGR_DIED));
			}
			if (tmp)
				mrustm(magr, mdef, otmp);
		    }
		} else if (magr->data == &mons[PM_PURPLE_WORM] &&
			    mdef->data == &mons[PM_SHRIEKER]) {
		    /* hack to enhance mm_aggression(); we don't want purple
		       worm's bite attack to kill a shrieker because then it
		       won't swallow the corpse; but if the target survives,
		       the subsequent engulf attack should accomplish that */
		    if (tmp >= mdef->mhp) tmp = mdef->mhp - 1;
		}
		break;
	    case AD_FIRE:
		if (cancelled) {
		    tmp = 0;
		    break;
		}
		if (vis)
		    pline(E_J("%s is %s!","%s%s！"), Monnam(mdef),
			  on_fire(mdef->data, mattk));
		if (pd == &mons[PM_STRAW_GOLEM] ||
		    pd == &mons[PM_PAPER_GOLEM]) {
			if (vis) pline(E_J("%s burns completely!",
					   "%sは燃え尽きた！"), Monnam(mdef));
			mondied(mdef);
			if (mdef->mhp > 0) return 0;
			else if (mdef->mtame && !vis)
			    pline(E_J("May %s roast in peace.",
				      "%sよ、安らかに煙れ。"), mon_nam(mdef));
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		if (resists_fire(mdef)) {
		    if (vis)
			pline_The(E_J("fire doesn't seem to burn %s!",
				      "%sは炎に焼かれないようだ！"),
								mon_nam(mdef));
		    shieldeff(mdef->mx, mdef->my);
		    golemeffects(mdef, AD_FIRE, tmp);
		    tmp = 0;
		}
		/* only potions damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
		break;
	    case AD_COLD:
		if (cancelled) {
		    tmp = 0;
		    break;
		}
		if (vis) pline(E_J("%s is covered in frost!",
				   "%sは氷に覆われた！"), Monnam(mdef));
		if (resists_cold(mdef)) {
		    if (vis)
			pline_The(E_J("frost doesn't seem to chill %s!",
				      "%sは冷気に凍えないようだ！"),
								mon_nam(mdef));
		    shieldeff(mdef->mx, mdef->my);
		    golemeffects(mdef, AD_COLD, tmp);
		    tmp = 0;
		}
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
		break;
	    case AD_ELEC:
		if (cancelled) {
		    tmp = 0;
		    break;
		}
		if (elem_hits_shield(mdef, AD_ELEC, E_J("zap", "電撃"))) {
		    tmp = 0;
		    break;
		}
		if (vis) pline(E_J("%s gets zapped!",
				   "%sは電撃をくらった！"), Monnam(mdef));
		tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
		if (resists_elec(mdef)) {
		    if (vis) pline_The(E_J("zap doesn't shock %s!",
					   "%sは感電しない！"), mon_nam(mdef));
		    shieldeff(mdef->mx, mdef->my);
		    golemeffects(mdef, AD_ELEC, tmp);
		    tmp = 0;
		}
		/* only rings damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
		break;
	    case AD_ACID:
		if (magr->mcan) {
		    tmp = 0;
		    break;
		}
		if (resists_acid(mdef)) {
		    if (vis)
			pline(E_J("%s is covered in acid, but it seems harmless.",
				  "%sは酸に覆われたが、傷つかないようだ。"),
			      Monnam(mdef));
		    tmp = 0;
		} else if (vis) {
		    pline(E_J("%s is covered in acid!","%sは酸に覆われた！"), Monnam(mdef));
		    pline(E_J("It burns %s!","%sは灼かれた！"), mon_nam(mdef));
		}
		if (!rn2(30)) erode_armor(mdef, TRUE);
		if (!rn2(6)) erode_obj(MON_WEP(mdef), TRUE, TRUE);
		break;
	    case AD_RUST:
		if (magr->mcan) break;
		if (pd == &mons[PM_IRON_GOLEM]) {
			if (vis) pline(E_J("%s falls to pieces!",
					   "%sはばらばらに崩れ落ちた！"), Monnam(mdef));
			mondied(mdef);
			if (mdef->mhp > 0) return 0;
			else if (mdef->mtame && !vis)
			    pline(E_J("May %s rust in peace.",
				      "%sよ、安らかに濡れ。"), mon_nam(mdef));
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		hurtmarmor(mdef, AD_RUST);
		mdef->mstrategy &= ~STRAT_WAITFORU;
//		tmp = 0;
		break;
	    case AD_CORR:
		if (magr->mcan) break;
		hurtmarmor(mdef, AD_CORR);
		mdef->mstrategy &= ~STRAT_WAITFORU;
		tmp = 0;
		break;
	    case AD_DCAY:
		if (magr->mcan) break;
		if (pd == &mons[PM_WOOD_GOLEM] ||
		    pd == &mons[PM_LEATHER_GOLEM]) {
			if (vis) pline(E_J("%s falls to pieces!",
					   "%sはばらばらに腐り落ちた！"), Monnam(mdef));
			mondied(mdef);
			if (mdef->mhp > 0) return 0;
			else if (mdef->mtame && !vis)
			    pline(E_J("May %s rot in peace.",
				      "%sよ、安らかに腐れ。"), mon_nam(mdef));
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		hurtmarmor(mdef, AD_DCAY);
		mdef->mstrategy &= ~STRAT_WAITFORU;
		tmp = 0;
		break;
	    case AD_STON:
		if (magr->mcan) break;
 do_stone:
		/* may die from the acid if it eats a stone-curing corpse */
		if (munstone(mdef, FALSE)) goto post_stone;
		if (poly_when_stoned(pd)) {
			mon_to_stone(mdef);
			tmp = 0;
			break;
		}
		if (!resists_ston(mdef)) {
			if (vis) pline(E_J("%s turns to stone!",
					   "%sは石になった！"), Monnam(mdef));
			monstone(mdef);
 post_stone:		if (mdef->mhp > 0) return 0;
			else if (mdef->mtame && !vis)
			    You(brief_feeling, E_J("peculiarly sad","不思議な悲しみ"));
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp = (mattk->adtyp == AD_STON ? 0 : 1);
		break;
	    case AD_TLPT:
		if (!cancelled && tmp < mdef->mhp && !tele_restrict(mdef)) {
		    char mdef_Monnam[BUFSZ];
		    /* save the name before monster teleports, otherwise
		       we'll get "it" in the suddenly disappears message */
		    if (vis) Strcpy(mdef_Monnam, Monnam(mdef));
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		    (void) rloc(mdef, FALSE);
		    if (vis && !canspotmon(mdef)
#ifdef STEED
		    	&& mdef != u.usteed
#endif
		    	)
			pline(E_J("%s suddenly disappears!",
				  "%sは突然消え失せた！"), mdef_Monnam);
		}
		break;
	    case AD_SLEE:
		if (!cancelled && !mdef->msleeping &&
			sleep_monst(mdef, rnd(10), -1)) {
		    if (vis) {
			Strcpy(buf, Monnam(mdef));
			pline(E_J("%s is put to sleep by %s.",
				  "%sは%sに眠らされた。"), buf, mon_nam(magr));
		    }
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		    slept_monst(mdef);
		}
		break;
	    case AD_PLYS:
		if(!cancelled && mdef->mcanmove) {
		    if (vis) {
			Strcpy(buf, Monnam(mdef));
			pline(E_J("%s is frozen by %s.",
				  "%sは%sに麻痺させられた。"), buf, mon_nam(magr));
		    }
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		}
		break;
	    case AD_SLOW:
		if (!cancelled && mdef->mspeed != MSLOW) {
		    unsigned int oldspeed = mdef->mspeed;

		    mon_adjust_speed(mdef, -1, (struct obj *)0);
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		    if (mdef->mspeed != oldspeed && vis)
			pline(E_J("%s slows down.",
				  "%sの動きが遅くなった。"), Monnam(mdef));
		}
		break;
	    case AD_CONF:
		/* Since confusing another monster doesn't have a real time
		 * limit, setting spec_used would not really be right (though
		 * we still should check for it).
		 */
		if (!magr->mcan && !mdef->mconf && !magr->mspec_used) {
		    if (vis) pline(E_J("%s looks confused.",
				       "%sは混乱したようだ。"), Monnam(mdef));
		    mdef->mconf = 1;
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		}
		break;
	    case AD_BLND:
		if (can_blnd(magr, mdef, mattk->aatyp, (struct obj*)0)) {
		    register unsigned rnd_tmp;

		    if (vis && mdef->mcansee)
			pline(E_J("%s is blinded.",
				  "%sは目が見えなくなった。"), Monnam(mdef));
		    rnd_tmp = d((int)mattk->damn, (int)mattk->damd);
		    if ((rnd_tmp += mdef->mblinded) > 127) rnd_tmp = 127;
		    mdef->mblinded = rnd_tmp;
		    mdef->mcansee = 0;
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		}
		tmp = 0;
		break;
	    case AD_HALU:
		if (!magr->mcan && haseyes(pd) && mdef->mcansee) {
		    if (vis) pline(E_J("%s looks %sconfused.",
				       "%sは%s混乱したようだ。"),
				    Monnam(mdef), mdef->mconf ? E_J("more ","さらに") : "");
		    mdef->mconf = 1;
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		}
		tmp = 0;
		break;
	    case AD_CURS:
		if (!night() && (pa == &mons[PM_GREMLIN])) break;
		if (!magr->mcan && !rn2(10)) {
		    mdef->mcan = 1;	/* cancelled regardless of lifesave */
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		    if (is_were(pd) && pd->mlet != S_HUMAN)
			were_change(mdef);
		    if (pd == &mons[PM_CLAY_GOLEM]) {
			    if (vis) {
#ifndef JP
				pline("Some writing vanishes from %s head!",
				    s_suffix(mon_nam(mdef)));
				pline("%s is destroyed!", Monnam(mdef));
#else
				pline("%sの額に書かれた何かの文字が消えた！",
					mon_nam(mdef));
				pline("%sは倒された！", mon_nam(mdef));
#endif /*JP*/
			    }
			    mondied(mdef);
			    if (mdef->mhp > 0) return 0;
			    else if (mdef->mtame && !vis)
				You(brief_feeling, E_J("strangely sad","奇妙な悲しみ"));
			    return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		    }
		    if (flags.soundok) {
			    if (!vis) You_hear(E_J("laughter.","あざ笑う声を"));
			    else pline(E_J("%s chuckles.","%sは薄笑いを浮かべた。"), Monnam(magr));
		    }
		}
		break;
	    case AD_SGLD:
		tmp = 0;
		if (is_swallowed(magr)) break;
#ifndef GOLDOBJ
		if (magr->mcan || !mdef->mgold) break;
		/* technically incorrect; no check for stealing gold from
		 * between mdef's feet...
		 */
		magr->mgold += mdef->mgold;
		mdef->mgold = 0;
#else
                if (magr->mcan) break;
		/* technically incorrect; no check for stealing gold from
		 * between mdef's feet...
		 */
                {
		    struct obj *gold = findgold(mdef->minvent);
		    if (!gold) break;
                    obj_extract_self(gold);
		    add_to_minv(magr, gold);
                }
#endif
		mdef->mstrategy &= ~STRAT_WAITFORU;
		if (vis) {
		    Strcpy(buf, Monnam(magr));
		    pline(E_J("%s steals some gold from %s.",
			      "%sは%sから金を盗んだ。"), buf, mon_nam(mdef));
		}
		if (!tele_restrict(magr)) {
		    (void) rloc(magr, FALSE);
		    if (vis && !canspotmon(magr))
			pline(E_J("%s suddenly disappears!",
				  "%sは突然消え失せた！"), buf);
		}
		break;
	    case AD_DRLI:
		if (!cancelled && !rn2(3) && !resists_drli(mdef)) {
			tmp = d(2,6);
			if (vis)
			    pline(E_J("%s suddenly seems weaker!",
				      "%sは突然弱々しくなった！"), Monnam(mdef));
			mdef->mhpmax -= tmp;
			if (mdef->m_lev == 0)
				tmp = mdef->mhp;
			else mdef->m_lev--;
			/* Automatic kill if drained past level 0 */
		}
		break;
#ifdef SEDUCE
	    case AD_SSEX:
#endif
	    case AD_SITM:	/* for now these are the same */
	    case AD_SEDU:
		if (magr->mcan) break;
		if (is_swallowed(magr)) break;
		/* find an object to steal, non-cursed if magr is tame */
		for (obj = mdef->minvent; obj; obj = obj->nobj)
		    if (!magr->mtame || !obj->cursed)
			break;

		if (obj) {
			char onambuf[BUFSZ], mdefnambuf[BUFSZ];

			/* make a special x_monnam() call that never omits
			   the saddle, and save it for later messages */
			Strcpy(mdefnambuf, x_monnam(mdef, ARTICLE_THE, (char *)0, 0, FALSE));

			otmp = obj;
#ifdef STEED
			if (u.usteed == mdef &&
					otmp == which_armor(mdef, W_SADDLE))
				/* "You can no longer ride <steed>." */
				dismount_steed(DISMOUNT_POLY);
#endif
			obj_extract_self(otmp);
			if (otmp->owornmask) {
				mdef->misc_worn_check &= ~otmp->owornmask;
				if (otmp->owornmask & W_WEP)
				    setmnotwielded(mdef,otmp);
				otmp->owornmask = 0L;
				update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
			}
			/* add_to_minv() might free otmp [if it merges] */
			if (vis)
				Strcpy(onambuf, doname(otmp));
			if (vis) {
				Strcpy(buf, Monnam(magr));
#ifndef JP
				pline("%s steals %s from %s!", buf,
				    onambuf, mdefnambuf);
#else
				pline("%sは%sから%sを盗んだ！", buf,
				    mdefnambuf, onambuf);
#endif /*JP*/
				if (vanish_ether_obj(otmp, magr, E_J("steal","盗んだ")))
				    otmp = (struct obj *)0;
			} else if (otmp->etherial) {
				obfree(otmp, (struct obj *)0);
				otmp = (struct obj *)0;
			}
			if (otmp) (void) add_to_minv(magr, otmp);
			possibly_unwield(mdef, FALSE);
			mdef->mstrategy &= ~STRAT_WAITFORU;
			mselftouch(mdef, (const char *)0, FALSE);
			if (mdef->mhp <= 0)
				return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
			if (magr->data->mlet == S_NYMPH &&
			    !tele_restrict(magr)) {
			    (void) rloc(magr, FALSE);
			    if (vis && !canspotmon(magr))
				pline(E_J("%s suddenly disappears!",
					  "%sは突然消え失せた！"), buf);
			}
		}
		tmp = 0;
		break;
	    case AD_DRST:
	    case AD_DRDX:
	    case AD_DRCO:
		if (!cancelled && !rn2(8)) {
		    if (vis)
			pline(E_J("%s %s was poisoned!",
				  "%s%sは毒に冒されている！"), s_suffix(Monnam(magr)),
			      mpoisons_subj(magr, mattk));
		    if (resists_poison(mdef)) {
			if (vis)
			    pline_The(E_J("poison doesn't seem to affect %s.",
					  "毒は%sには効かないようだ。"),
				mon_nam(mdef));
		    } else {
			if (rn2(10)) tmp += rn1(10,6);
			else {
			    if (vis) pline_The(E_J("poison was deadly...",
						   "毒は致死的だった…。"));
			    tmp = mdef->mhp;
			}
		    }
		}
		break;
	    case AD_DRIN:
		if (notonhead || !has_head(pd) || is_swallowed(magr)) {
		    if (vis) pline(E_J("%s doesn't seem harmed.",
				       "%sは傷つかないようだ。"), Monnam(mdef));
		    /* Not clear what to do for green slimes */
		    tmp = 0;
		    break;
		}
		if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
		    if (vis) {
			Strcpy(buf, s_suffix(Monnam(mdef)));
#ifndef JP
			pline("%s helmet blocks %s attack to %s head.",
				buf, s_suffix(mon_nam(magr)),
				mhis(mdef));
#else
			pline("頭への%sの攻撃は%s兜に防がれた。",
				mon_nam(magr), buf);
#endif /*JP*/
		    }
		    break;
		}
#ifndef JP
		if (vis) pline("%s brain is eaten!", s_suffix(Monnam(mdef)));
#else
		if (vis) pline("%sは脳を喰われた！", mon_nam(mdef));
#endif /*JP*/
		if (mindless(pd)) {
		    if (vis) pline(E_J("%s doesn't notice.",
				       "%sは気づかないようだ。"), Monnam(mdef));
		    break;
		}
		tmp += rnd(10); /* fakery, since monsters lack INT scores */
		if (magr->mtame && !magr->isminion) {
		    EDOG(magr)->hungrytime += rnd(60);
		    magr->mconf = 0;
		}
		if (tmp >= mdef->mhp && vis)
		    pline(E_J("%s last thought fades away...",
			      "%s最期の思考が薄れてゆき、消えた…。"),
			          s_suffix(Monnam(mdef)));
		break;
	    case AD_SLIM:
		if (cancelled) break;	/* physical damage only */
		if (!rn2(4) && !flaming(mdef->data) &&
				mdef->data != &mons[PM_GREEN_SLIME]) {
		    (void) newcham(mdef, &mons[PM_GREEN_SLIME], FALSE, vis);
		    mdef->mstrategy &= ~STRAT_WAITFORU;
		    tmp = 0;
		}
		break;
	    case AD_STCK:
		if (cancelled) tmp = 0;
		break;
	    case AD_WRAP: /* monsters cannot grab one another, it's too hard */
		if (magr->mcan) tmp = 0;
		break;
	    case AD_ENCH:
		/* there's no msomearmor() function, so just do damage */
	     /* if (cancelled) break; */
		break;
	    case AD_DRWN:
		/* you'd see the monster inside the water elemental */
		if (magr->meating && --(magr->meating)) {
		    if (flaming(pd)) {
			tmp = d(3,10);
			if (canspotmon(mdef))
			    pline(E_J("%s is being extingushed!","%sは消火されている！"), Monnam(mdef));
		    } else if (canspotmon(mdef)) {
			pline(E_J("%s is drowning in water!","%sは溺れている！"), Monnam(mdef));
		    }
		    if (!rn2(4)) water_damage(mdef->minvent, FALSE, FALSE);
		    else hurtmarmor(mdef, AD_RUST);
		    break;
		}
		if (canspotmon(mdef)) pline(E_J("%s drowns.","%sは溺死した。"), Monnam(mdef));
		mondied(mdef);
		if (mdef->mhp > 0) return 0;	/* lifesaved */
		return (MM_DEF_DIED | (grow_up(magr,mdef) ?
					0 : MM_AGR_DIED));
	    default:	tmp = 0;
			break;
	}
	if(!tmp) return(MM_MISS);

	if((mdef->mhp -= tmp) < 1) {
//	    if (m_at(mdef->mx, mdef->my) == magr) {  /* see gulpmm() */
//		remove_monster(mdef->mx, mdef->my);
//		mdef->mhp = 1;	/* otherwise place_monster will complain */
//		place_monster(mdef, mdef->mx, mdef->my);
//		mdef->mhp = 0;
//	    }
	    monkilled(mdef, "", (int)mattk->adtyp);
	    if (mdef->mhp > 0) return 0; /* mdef lifesaved */

	    if (mattk->adtyp == AD_DGST) {
		/* various checks similar to dog_eat and meatobj.
		 * after monkilled() to provide better message ordering */
		if (mdef->cham != CHAM_ORDINARY) {
		    (void) newcham(magr, (struct permonst *)0, FALSE, TRUE);
		} else if (mdef->data == &mons[PM_GREEN_SLIME]) {
		    (void) newcham(magr, &mons[PM_GREEN_SLIME], FALSE, TRUE);
		} else if (mdef->data == &mons[PM_WRAITH]) {
		    (void) grow_up(magr, (struct monst *)0);
		    /* don't grow up twice */
		    return (MM_DEF_DIED | (magr->mhp > 0 ? 0 : MM_AGR_DIED));
		} else if (mdef->data == &mons[PM_NURSE]) {
		    magr->mhp = magr->mhpmax;
		}
	    }

	    return (MM_DEF_DIED | (grow_up(magr,mdef) ? 0 : MM_AGR_DIED));
	}
	return(MM_HIT);
}

#endif /* OVLB */


#ifdef OVL0

int
noattacks(ptr)			/* returns 1 if monster doesn't attack */
	struct	permonst *ptr;
{
	int i;

	for(i = 0; i < NATTK; i++)
		if(ptr->mattk[i].aatyp) return(0);

	return(1);
}

/* `mon' is hit by a sleep attack; return 1 if it's affected, 0 otherwise */
int
sleep_monst(mon, amt, how)
struct monst *mon;
int amt, how;
{
	if (resists_sleep(mon) ||
		(how >= 0 && resist(mon, (char)how, 0, NOTELL))) {
	    shieldeff(mon->mx, mon->my);
	} else if (mon->mcanmove) {
	    amt += (int) mon->mfrozen;
	    if (amt > 0) {	/* sleep for N turns */
		mon->mcanmove = 0;
		mon->mfrozen = min(amt, 127);
	    } else {		/* sleep until awakened */
		mon->msleeping = 1;
	    }
	    return 1;
	}
	return 0;
}

/* sleeping grabber releases, engulfer doesn't; don't use for paralysis! */
void
slept_monst(mon)
struct monst *mon;
{
	if ((mon->msleeping || !mon->mcanmove) && mon == u.ustuck &&
		!sticks(youmonst.data) && !u.uswallow) {
#ifndef JP
	    pline("%s grip relaxes.", s_suffix(Monnam(mon)));
#else
	    pline("%sの拘束がゆるんだ。", mon_nam(mon));
#endif /*JP*/
	    unstuck(mon);
	}
}

#endif /* OVL0 */
#ifdef OVLB

STATIC_OVL void
mrustm(magr, mdef, obj)
register struct monst *magr, *mdef;
register struct obj *obj;
{
	int hurt = 1;

	if (!magr || !mdef || !obj) return; /* just in case */

	if (dmgtype(mdef->data, AD_CORR))
	    hurt = 3;
	else if (dmgtype(mdef->data, AD_RUST))
	    hurt = 1;
	else
	    return;

	if (!mdef->mcan)
		(void)rust_dmg(obj, xname(obj), hurt, TRUE, magr);
}

STATIC_OVL void
mswingsm(magr, mdef, otemp)
register struct monst *magr, *mdef;
register struct obj *otemp;
{
	char buf[BUFSZ];
	if (!flags.verbose || Blind || !mon_visible(magr)) return;
	Strcpy(buf, mon_nam(mdef));
#ifndef JP
	pline("%s %s %s %s at %s.", Monnam(magr),
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "thrusts" : "swings",
	      mhis(magr), singular(otemp, xname), buf);
#else
	pline("%sは%sを%sに向けて%s。",
	      mon_nam(magr), singular(otemp, xname), buf,
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "突き出した" : "振るった");
#endif /*JP*/
}

/*
 * Passive responses by defenders.  Does not replicate responses already
 * handled above.  Returns same values as mattackm.
 */
STATIC_OVL int
passivemm(magr,mdef,mhit,mdead)
register struct monst *magr, *mdef;
boolean mhit;
int mdead;
{
	register struct permonst *mddat = mdef->data;
	register struct permonst *madat = magr->data;
	char buf[BUFSZ];
	int i, tmp;
	boolean seeagr = canseemon(magr) && !is_swallowed(magr);

	for(i = 0; ; i++) {
	    if(i >= NATTK) return (mdead | mhit); /* no passive attacks */
	    if(mddat->mattk[i].aatyp == AT_NONE) break;
	}
	if (mddat->mattk[i].damn)
	    tmp = d((int)mddat->mattk[i].damn,
				    (int)mddat->mattk[i].damd);
	else if(mddat->mattk[i].damd)
	    tmp = d((int)mddat->mlevel+1, (int)mddat->mattk[i].damd);
	else
	    tmp = 0;

	/* These affect the enemy even if defender killed */
	switch(mddat->mattk[i].adtyp) {
	    case AD_ACID:
		if (mhit && !rn2(2)) {
		    Strcpy(buf, Monnam(magr));
		    if(seeagr)
			pline(E_J("%s is splashed by %s acid!",
				  "%sは%s酸を浴びせられた！"),
			      buf, s_suffix(mon_nam(mdef)));
		    if (resists_acid(magr)) {
			if(seeagr)
			    pline(E_J("%s is not affected.",
				      "%sは影響を受けなかった。"), Monnam(magr));
			tmp = 0;
		    }
		} else tmp = 0;
		goto assess_dmg;
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
		if (mhit && !mdef->mcan && otmp) {
		    (void) drain_item(otmp);
		    /* No message */
		}
		break;
	    default:
		break;
	}
	if (mdead || mdef->mcan) return (mdead|mhit);

	/* These affect the enemy only if defender is still alive */
	if (rn2(3)) switch(mddat->mattk[i].adtyp) {
	    case AD_PLYS: /* Floating eye */
		if (tmp > 127) tmp = 127;
		if (mddat == &mons[PM_FLOATING_EYE]) {
		    if (!rn2(4)) tmp = 127;
		    if (magr->mcansee && haseyes(madat) && mdef->mcansee &&
			(perceives(madat) || !mdef->minvis)) {
			if (seeagr)
			    Sprintf(buf, E_J("%s gaze is reflected by %%s %%s.",
					     "%s眼光は%%s%%sにはね返された。"),
				    s_suffix(mon_nam(mdef)));
			if (mon_reflects(magr,
					 canseemon(magr) ? buf : (char *)0))
				return(mdead|mhit);
			Strcpy(buf, Monnam(magr));
			if(seeagr)
			    pline(E_J("%s is frozen by %s gaze!",
				      "%sは%s眼光で動けなくなった！"),
				  buf, s_suffix(mon_nam(mdef)));
			magr->mcanmove = 0;
			magr->mfrozen = tmp;
			return (mdead|mhit);
		    }
		} else { /* gelatinous cube */
		    Strcpy(buf, Monnam(magr));
		    if(vis/*canseemon(magr)*/)
			pline(E_J("%s is frozen by %s.",
				  "%sは%sに麻痺させられた。"), buf, mon_nam(mdef));
		    magr->mcanmove = 0;
		    magr->mfrozen = tmp;
		    return (mdead|mhit);
		}
		return 1;
	    case AD_COLD:
		if (resists_cold(magr)) {
		    if (seeagr) {
			pline(E_J("%s is mildly chilly.",
				  "%sはほのかな涼しさを感じた。"), Monnam(magr));
			golemeffects(magr, AD_COLD, tmp);
		    }
		    tmp = 0;
		    break;
		}
		if(seeagr)
		    pline(E_J("%s is suddenly very cold!",
			      "%sは突然凍えさせられた！"), Monnam(magr));
		mdef->mhp += tmp / 2;
		if (mdef->mhpmax < mdef->mhp) mdef->mhpmax = mdef->mhp;
		if (mdef->mhpmax > ((int) (mdef->m_lev+1) * 8))
		    (void)split_mon(mdef, magr);
		break;
	    case AD_STUN:
		if (!magr->mstun) {
		    magr->mstun = 1;
		    if (seeagr)
#ifndef JP
			pline("%s %s...", Monnam(magr),
			      makeplural(stagger(magr->data, "stagger")));
#else
			pline("%sは%s…。", Monnam(magr),
			      stagger(magr->data, "よろめいた"));
#endif /*JP*/
		}
		tmp = 0;
		break;
	    case AD_FIRE:
		if (resists_fire(magr)) {
		    if (seeagr) {
			pline(E_J("%s is mildly warmed.",
				  "%sはほのかな暖かさを感じた。"), Monnam(magr));
			golemeffects(magr, AD_FIRE, tmp);
		    }
		    tmp = 0;
		    break;
		}
		if(seeagr)
		    pline(E_J("%s is suddenly very hot!",
			      "%sは突然激しく熱せられた！"), Monnam(magr));
		break;
	    case AD_ELEC:
		if (resists_elec(magr)) {
		    if (seeagr) {
			pline(E_J("%s is mildly tingled.",
				  "%sはかすかな痺れを感じた。"), Monnam(magr));
			golemeffects(magr, AD_ELEC, tmp);
		    }
		    tmp = 0;
		    break;
		}
		if(seeagr)
		    pline(E_J("%s is jolted with electricity!",
			      "%sは電撃に打たれた！"), Monnam(magr));
		break;
	    default: tmp = 0;
		break;
	}
	else tmp = 0;

    assess_dmg:
	if((magr->mhp -= tmp) <= 0) {
		monkilled(magr, "", (int)mddat->mattk[i].adtyp);
		return (mdead | mhit | MM_AGR_DIED);
	}
	return (mdead | mhit);
}

/* "aggressive defense"; what type of armor prevents specified attack
   from touching its target? */
long
attk_protection(aatyp)
int aatyp;
{
    long w_mask = 0L;

    switch (aatyp) {
    case AT_NONE:
    case AT_SPIT:
    case AT_EXPL:
    case AT_BOOM:
    case AT_GAZE:
    case AT_BREA:
    case AT_MAGC:
	w_mask = ~0L;		/* special case; no defense needed */
	break;
    case AT_CLAW:
    case AT_TUCH:
    case AT_WEAP:
	w_mask = W_ARMG;	/* caller needs to check for weapon */
	break;
    case AT_KICK:
	w_mask = W_ARMF;
	break;
    case AT_BUTT:
	w_mask = W_ARMH;
	break;
    case AT_HUGS:
	w_mask = (W_ARMC|W_ARMG); /* attacker needs both to be protected */
	break;
    case AT_BITE:
    case AT_STNG:
    case AT_ENGL:
    case AT_TENT:
    default:
	w_mask = 0L;		/* no defense available */
	break;
    }
    return w_mask;
}

#endif /* OVLB */

/*mhitm.c*/

