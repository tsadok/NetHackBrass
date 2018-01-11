/*	SCCS Id: @(#)apply.c	3.4	2003/11/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"

#ifdef OVLB

static const char tools[] = { ALL_CLASSES, TOOL_CLASS, WEAPON_CLASS, WAND_CLASS, 0 };
static const char tools_too[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS,
				  WEAPON_CLASS, ARMOR_CLASS,
				  WAND_CLASS, GEM_CLASS, 0 };

#ifdef TOURIST
STATIC_DCL int FDECL(use_camera, (struct obj *));
#endif
STATIC_DCL int FDECL(use_towel, (struct obj *));
STATIC_DCL boolean FDECL(its_dead, (int,int,int *));
STATIC_DCL int FDECL(use_stethoscope, (struct obj *));
STATIC_DCL void FDECL(use_whistle, (struct obj *));
STATIC_DCL void FDECL(use_magic_whistle, (struct obj *));
STATIC_DCL void FDECL(use_leash, (struct obj *));
STATIC_DCL int FDECL(use_mirror, (struct obj *));
STATIC_DCL void FDECL(use_bell, (struct obj **));
STATIC_DCL void FDECL(use_candelabrum, (struct obj *));
STATIC_DCL void FDECL(use_candle, (struct obj **));
STATIC_DCL void FDECL(use_lamp, (struct obj *));
STATIC_DCL void FDECL(light_cocktail, (struct obj *));
STATIC_DCL void FDECL(use_tinning_kit, (struct obj *));
STATIC_DCL void FDECL(use_figurine, (struct obj **));
STATIC_DCL void FDECL(use_grease, (struct obj *));
STATIC_DCL void FDECL(use_trap, (struct obj *));
STATIC_DCL void FDECL(use_stone, (struct obj *));
STATIC_PTR int NDECL(set_trap);		/* occupation callback */
STATIC_DCL int FDECL(use_whip, (struct obj *));
STATIC_DCL int FDECL(use_pole, (struct obj *));
STATIC_DCL int FDECL(use_cream_pie, (struct obj *));
STATIC_DCL int FDECL(use_grapple, (struct obj *));
STATIC_DCL int FDECL(use_shield, (struct obj *));
STATIC_OVL int FDECL(use_tin_opener, (struct obj *));
STATIC_DCL int FDECL(do_break_wand, (struct obj *));
STATIC_DCL boolean FDECL(figurine_location_checks,
				(struct obj *, coord *, BOOLEAN_P));
STATIC_DCL boolean NDECL(uhave_graystone);
STATIC_DCL int FDECL(getobj_filter_useorapply, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_rub, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_grease, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_tin_opener, (struct obj *));
//STATIC_DCL int FDECL(getobj_filter_repair, (struct obj *));

#ifdef FIRSTAID
STATIC_DCL int FDECL(use_scissors, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_scissors, (struct obj *));
STATIC_PTR int NDECL(putbandage);
#endif

STATIC_DCL int FDECL(use_gun, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_bullet, (struct obj *));
STATIC_DCL int FDECL(use_bullet, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_gun, (struct obj *));


#ifdef	AMIGA
void FDECL( amii_speaker, ( struct obj *, char *, int ) );
#endif

static const char no_elbow_room[] = E_J("don't have enough elbow-room to maneuver.",
					"腕を動かせるだけの余裕がない。");

#ifdef FIRSTAID
static NEARDATA struct {
	struct	obj *bandage;
	int	usedtime, reqtime;
} bandage;
#endif

#ifdef TOURIST
STATIC_OVL int
use_camera(obj)
	struct obj *obj;
{
	if(Underwater) {
		pline(E_J("Using your camera underwater would void the warranty.",
			  "水中でのカメラの使用は保証の対象外となります。"));
		return(0);
	}
//	if(!getdir((char *)0)) return(0);
	if(!getdir_or_pos(0, GETPOS_MONTGT, (char *)0, E_J("take a picture","写真を撮る"))) return 0;

	if (obj->spe <= 0) {
		pline(nothing_happens);
		return (1);
	}
	consume_obj_charge(obj, TRUE);

	if (obj->cursed && !rn2(2)) {
		(void) zapyourself(obj, TRUE);
	} else if (u.uswallow) {
		You(E_J("take a picture of %s %s.",
			"%s%sの写真を撮った。"), s_suffix(mon_nam(u.ustuck)),
		    mbodypart(u.ustuck, STOMACH));
	} else if (u.dz) {
		You(E_J("take a picture of the %s.",
			"%sの写真を撮った。"),
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
	} else if (!u.dx && !u.dy) {
		(void) zapyourself(obj, TRUE);
	} else {
		obj->ox = u.ux,  obj->oy = u.uy;
		(void) bhit(u.dx, u.dy, COLNO, FLASHED_LIGHT,
			(int FDECL((*),(MONST_P,OBJ_P)))flash_hits_mon,
			(int FDECL((*),(OBJ_P,OBJ_P)))0,
			obj);
	}
	return 1;
}
#endif

STATIC_OVL int
use_towel(obj)
	struct obj *obj;
{
	if(!freehand()) {
		You(E_J("have no free %s!","%sが空いていない！"), body_part(HAND));
		return 0;
	} else if (obj->owornmask) {
#ifndef JP
		You("cannot use it while you're wearing it!");
#else
		pline("身に着けているものを使うことはできない！");
#endif /*JP*/
		return 0;
	} else if (obj->cursed) {
		long old;
		switch (rn2(3)) {
		case 2:
		    old = Glib;
		    Glib += rn1(10, 3);
#ifndef JP
		    Your("%s %s!", makeplural(body_part(HAND)),
			(old ? "are filthier than ever" : "get slimy"));
#else
		    Your("両%sは%sれた！", body_part(HAND),
			(old ? "ますます汚" : "ぬるぬるした汚れにまみ"));
#endif /*JP*/
		    return 1;
		case 1:
		    if (!ublindf) {
			old = u.ucreamed;
			u.ucreamed += rn1(10, 3);
#ifndef JP
			pline("Yecch! Your %s %s gunk on it!", body_part(FACE),
			      (old ? "has more" : "now has"));
#else
			pline("うえぇ！ あなたの%sは%sれた！", body_part(FACE),
			      (old ? "ますます汚" : "ベトベトに覆わ"));
#endif /*JP*/
			make_blinded(Blinded + (long)u.ucreamed - old, TRUE);
		    } else {
#ifndef MAGIC_GLASSES
			const char *what = (ublindf->otyp == LENSES) ?
					    E_J("lenses","レンズ") : E_J("blindfold","目隠し");
#else
			const char *what = (Is_glasses(ublindf)) ?
					    E_J("glasses","眼鏡") : E_J("blindfold","目隠し");
#endif /*MAGIC_GLASSES*/
			if (ublindf->cursed) {
			    You(E_J("push your %s %s.","%sを押し%sた。"), what,
				rn2(2) ? E_J("cock-eyed","歪め") : E_J("crooked","曲げ"));
			} else {
			    struct obj *saved_ublindf = ublindf;
			    You(E_J("push your %s off.","%sを押しやった。"), what);
			    Blindf_off(ublindf);
			    dropx(saved_ublindf);
			}
		    }
		    return 1;
		case 0:
		    break;
		}
	}

	if (Glib) {
		Glib = 0;
#ifndef JP
		You("wipe off your %s.", makeplural(body_part(HAND)));
#else
		You("%sをぬぐった。", body_part(HAND));
#endif /*JP*/
		return 1;
	} else if(u.ucreamed) {
		Blinded -= u.ucreamed;
		u.ucreamed = 0;

		if (!Blinded) {
			pline(E_J("You've got the glop off.",
				  "あなたは顔についたベトベトをぬぐい取った。"));
			Blinded = 1;
			make_blinded(0L,TRUE);
		} else {
			Your(E_J("%s feels clean now.","%sは清潔になった。"), body_part(FACE));
		}
		return 1;
	}

	Your(E_J("%s and %s are already clean.",
		 "%sと%sは汚れていない。"),
		body_part(FACE), E_J(makeplural(body_part(HAND)), body_part(HAND)));

	return 0;
}

/* maybe give a stethoscope message based on floor objects */
STATIC_OVL boolean
its_dead(rx, ry, resp)
int rx, ry, *resp;
{
	struct obj *otmp;
	struct trap *ttmp;

	if (!can_reach_floor()) return FALSE;

	/* additional stethoscope messages from jyoung@apanix.apana.org.au */
	if (Hallucination && sobj_at(CORPSE, rx, ry)) {
	    /* (a corpse doesn't retain the monster's sex,
	       so we're forced to use generic pronoun here) */
	    /* スタートレック レナード・マッコイ医師の台詞 */
#ifndef JP
	    You_hear("a voice say, \"It's dead, Jim.\"");
#else
	    pline("へんじがない。ただのしかばねのようだ。");
#endif /*JP*/
	    *resp = 1;
	    return TRUE;
	} else if (Role_if(PM_HEALER) && ((otmp = sobj_at(CORPSE, rx, ry)) != 0 ||
				    (otmp = sobj_at(STATUE, rx, ry)) != 0)) {
	    /* possibly should check uppermost {corpse,statue} in the pile
	       if both types are present, but it's not worth the effort */
	    if (vobj_at(rx, ry)->otyp == STATUE) otmp = vobj_at(rx, ry);
	    if (otmp->otyp == CORPSE) {
#ifndef JP
		You("determine that %s unfortunate being is dead.",
		    (rx == u.ux && ry == u.uy) ? "this" : "that");
#else
		You("%sの不幸な生物は死んでいるという結論を下した。",
		    (rx == u.ux && ry == u.uy) ? "こ" : "そ");
#endif /*JP*/
	    } else {
		ttmp = t_at(rx, ry);
#ifndef JP
		pline("%s appears to be in %s health for a statue.",
		      The(mons[otmp->corpsenm].mname),
		      (ttmp && ttmp->ttyp == STATUE_TRAP) ?
			"extraordinary" : "excellent");
#else
		pline("%sは彫像にしては%sている。",
		      JMONNAM(otmp->corpsenm),
		      (ttmp && ttmp->ttyp == STATUE_TRAP) ?
			"信じられないほど生命力に溢れ" : "躍動感に満ち");
#endif /*JP*/
	    }
	    return TRUE;
	}
	return FALSE;
}

static const char hollow_str[] = E_J("a hollow sound.  This must be a secret %s!",
				     "虚ろな音が聞こえる。隠し%sがあるに違いない！");

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless.  As a compromise, one use per turn is free, another
   uses up the turn; this makes curse status have a tangible effect. */
STATIC_OVL int
use_stethoscope(obj)
	register struct obj *obj;
{
	static long last_used_move = -1;
	static short last_used_movement = 0;
	struct monst *mtmp;
	struct rm *lev;
	int rx, ry, res;
	boolean interference = (u.uswallow && is_whirly(u.ustuck->data) &&
				!rn2(Role_if(PM_HEALER) ? 10 : 3));

	if (nohands(youmonst.data)) {	/* should also check for no ears and/or deaf */
#ifndef JP
		You("have no hands!");	/* not `body_part(HAND)' */
#else
		pline("あなたには手がない！");
#endif /*JP*/
		return 0;
	} else if (!freehand()) {
		You(E_J("have no free %s.","%sが空いていない。"), body_part(HAND));
		return 0;
	}
	if (!getdir((char *)0)) return 0;

	res = (moves == last_used_move) &&
	      (youmonst.movement == last_used_movement);
	last_used_move = moves;
	last_used_movement = youmonst.movement;

#ifdef STEED
	if (u.usteed && u.dz > 0) {
		if (interference) {
			pline(E_J("%s interferes.","%sが邪魔だ。"), Monnam(u.ustuck));
			mstatusline(u.ustuck);
		} else
			mstatusline(u.usteed);
		return res;
	} else
#endif
	if (u.uswallow && (u.dx || u.dy || u.dz)) {
		mstatusline(u.ustuck);
		return res;
	} else if (u.uswallow && interference) {
		pline(E_J("%s interferes.","%sが邪魔だ。"), Monnam(u.ustuck));
		mstatusline(u.ustuck);
		return res;
	} else if (u.dz) {
		if (Underwater)
		    You_hear(E_J("faint splashing.","かすかな水音を"));
		else if (u.dz < 0 || !can_reach_floor())
		    You_cant(E_J("reach the %s.","%sに届かない。"),
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
		else if (its_dead(u.ux, u.uy, &res))
		    ;	/* message already given */
		else if (Is_stronghold(&u.uz))
		    You_hear(E_J("the crackling of hellfire.",
				 "地獄の業火が燃えさかる音を"));
		else
		    pline_The(E_J("%s seems healthy enough.",
				  "%sは充分健康なようだ。"), surface(u.ux,u.uy));
		return res;
	} else if (obj->cursed && !rn2(2)) {
		You_hear(E_J("your heart beat.","自分の心臓の鼓動を"));
		return res;
	}
	if (Stunned || (Confusion && !rn2(5))) confdir();
	if (!u.dx && !u.dy) {
		ustatusline();
		return res;
	}
	rx = u.ux + u.dx; ry = u.uy + u.dy;
	if (!isok(rx,ry)) {
		You_hear(E_J("a faint typing noise.","かすかな打鍵音を"));
		return 0;
	}
	if ((mtmp = m_at(rx,ry)) != 0) {
		mstatusline(mtmp);
		if (mtmp->mundetected) {
			mtmp->mundetected = 0;
			if (cansee(rx,ry)) newsym(mtmp->mx,mtmp->my);
		}
		if (!canspotmons(mtmp))
			map_invisible(rx,ry);
		return res;
	}
	if (glyph_is_invisible(levl[rx][ry].glyph)) {
		unmap_object(rx, ry);
		newsym(rx, ry);
		pline_The(E_J("invisible monster must have moved.",
			      "目に見えない怪物は移動したにちがいない。"));
	}
	lev = &levl[rx][ry];
	switch(lev->typ) {
	case SDOOR:
#ifndef JP
		You_hear(hollow_str, "door");
#else
		pline(hollow_str, "扉"); /* 眠っていない・水面下でないので、plineで代用可 */
#endif /*JP*/
		cvt_sdoor_to_door(lev);		/* ->typ = DOOR */
		if (Blind) feel_location(rx,ry);
		else newsym(rx,ry);
		return res;
	case SCORR:
#ifndef JP
		You_hear(hollow_str, "passage");
#else
		pline(hollow_str, "通路");
#endif /*JP*/
		lev->typ = CORR;
		unblock_point(rx,ry);
		if (Blind) feel_location(rx,ry);
		else newsym(rx,ry);
		return res;
	}

	if (!its_dead(rx, ry, &res))
#ifndef JP
	    You("hear nothing special.");	/* not You_hear()  */
#else
	    pline("とくに何も聞こえない。");
#endif /*JP*/
	return res;
}

static const char whistle_str[] = E_J("produce a %s whistling sound.",
				      "笛を吹いて%s音を鳴らした。");

STATIC_OVL void
use_whistle(obj)
struct obj *obj;
{
	You(whistle_str, obj->cursed ? E_J("shrill","絹を裂くような") : E_J("high","かん高い"));
	wake_nearby();
}

STATIC_OVL void
use_magic_whistle(obj)
struct obj *obj;
{
	register struct monst *mtmp, *nextmon;

	if(obj->spe > 0) {
	    consume_obj_charge(obj, TRUE);
	    if(obj->cursed && !rn2(2)) {
		You(E_J("produce a high-pitched humming noise.",
			"かん高く唸る騒音を吹き鳴らした。"));
		wake_nearby();
	    } else {
		int pet_cnt = 0;
		You(whistle_str, Hallucination ? E_J("normal","ありきたりの") : E_J("strange","不思議な"));
		for(mtmp = fmon; mtmp; mtmp = nextmon) {
		    nextmon = mtmp->nmon; /* trap might kill mon */
		    if (DEADMONSTER(mtmp)) continue;
		    if (mtmp->mtame) {
			if (mtmp->mtrapped) {
			    /* no longer in previous trap (affects mintrap) */
			    mtmp->mtrapped = 0;
			    fill_pit(mtmp->mx, mtmp->my);
			}
			mnexto(mtmp);
			if (canspotmon(mtmp)) ++pet_cnt;
			if (mintrap(mtmp) == 2) change_luck(-1);
		    }
		}
		if (pet_cnt > 0) makeknown(obj->otyp);
	    }
	} else use_whistle(obj);
}

boolean
um_dist(x,y,n)
register xchar x, y, n;
{
	return((boolean)(abs(u.ux - x) > n  || abs(u.uy - y) > n));
}

int
number_leashed()
{
	register int i = 0;
	register struct obj *obj;

	for(obj = invent; obj; obj = obj->nobj)
		if(obj->otyp == LEASH && obj->leashmon != 0) i++;
	return(i);
}

void
o_unleash(otmp)		/* otmp is about to be destroyed or stolen */
register struct obj *otmp;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(mtmp->m_id == (unsigned)otmp->leashmon)
			mtmp->mleashed = 0;
	otmp->leashmon = 0;
}

void
m_unleash(mtmp, feedback)	/* mtmp is about to die, or become untame */
register struct monst *mtmp;
boolean feedback;
{
	register struct obj *otmp;

	if (feedback) {
	    if (canseemon(mtmp))
#ifndef JP
		pline("%s pulls free of %s leash!", Monnam(mtmp), mhis(mtmp));
#else
		pline("%sは自分を繋ぐ引き綱をふりほどいた！", Monnam(mtmp));
#endif /*JP*/
	    else
		Your(E_J("leash falls slack.","引き綱の手応えがなくなった。"));
	}
	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == LEASH &&
				otmp->leashmon == (int)mtmp->m_id)
			otmp->leashmon = 0;
	mtmp->mleashed = 0;
}

void
unleash_all()		/* player is about to die (for bones) */
{
	register struct obj *otmp;
	register struct monst *mtmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == LEASH) otmp->leashmon = 0;
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		mtmp->mleashed = 0;
}

#define MAXLEASHED	2

/* ARGSUSED */
STATIC_OVL void
use_leash(obj)
struct obj *obj;
{
	coord cc;
	register struct monst *mtmp;
	int spotmon;

	if(!obj->leashmon && number_leashed() >= MAXLEASHED) {
		You(E_J("cannot leash any more pets.",
			"これ以上ペットを繋いで歩けない。"));
		return;
	}

	if(!get_adjacent_loc((char *)0, (char *)0, u.ux, u.uy, &cc)) return;

	if((cc.x == u.ux) && (cc.y == u.uy)) {
#ifdef STEED
		if (u.usteed && u.dz > 0) {
		    mtmp = u.usteed;
		    spotmon = 1;
		    goto got_target;
		}
#endif
		pline(E_J("Leash yourself?  Very funny...",
			  "自分を縛るって？ ずいぶんおかしな人だ…。"));
		return;
	}

	if(!(mtmp = m_at(cc.x, cc.y))) {
#ifndef JP
		There("is no creature there.");
#else
		pline("そこには何もいない。");
#endif /*JP*/
		return;
	}

	spotmon = canspotmon(mtmp);
#ifdef STEED
 got_target:
#endif

	if(!mtmp->mtame) {
	    if(!spotmon)
#ifndef JP
		There("is no creature there.");
#else
		pline("そこには何もいない。");
#endif /*JP*/
	    else
#ifndef JP
		pline("%s %s leashed!", Monnam(mtmp), (!obj->leashmon) ?
				"cannot be" : "is not");
#else
		pline("%s%sない！", mon_nam(mtmp), (!obj->leashmon) ?
				"を繋ぐことはでき" : "は繋がれてい");
#endif /*JP*/
	    return;
	}
	if(!obj->leashmon) {
		if(mtmp->mleashed) {
#ifndef JP
			pline("This %s is already leashed.",
			      spotmon ? l_monnam(mtmp) : "monster");
#else
			pline("その%sはすでに繋がれている。",
			      spotmon ? l_monnam(mtmp) : "怪物");
#endif /*JP*/
			return;
		}
#ifndef JP
		You("slip the leash around %s%s.",
		    spotmon ? "your " : "", l_monnam(mtmp));
#else
		You("引き綱を%s%sに繋いだ。",
		    spotmon ? "あなたの" : "", l_monnam(mtmp));
#endif /*JP*/
		mtmp->mleashed = 1;
		obj->leashmon = (int)mtmp->m_id;
		mtmp->msleeping = 0;
		return;
	}
	if(obj->leashmon != (int)mtmp->m_id) {
		pline(E_J("This leash is not attached to that creature.",
			  "この引き綱はその怪物には繋がれていない。"));
		return;
	} else {
		if(obj->cursed) {
			pline_The(E_J("leash would not come off!",
				      "この引き綱は外れない！"));
			obj->bknown = TRUE;
			return;
		}
		mtmp->mleashed = 0;
		obj->leashmon = 0;
#ifndef JP
		You("remove the leash from %s%s.",
		    spotmon ? "your " : "", l_monnam(mtmp));
#else
		You("%sから引き綱を外した。", l_monnam(mtmp));
#endif /*JP*/
	}
	return;
}

struct obj *
get_mleash(mtmp)	/* assuming mtmp->mleashed has been checked */
register struct monst *mtmp;
{
	register struct obj *otmp;

	otmp = invent;
	while(otmp) {
		if(otmp->otyp == LEASH && otmp->leashmon == (int)mtmp->m_id)
			return(otmp);
		otmp = otmp->nobj;
	}
	return((struct obj *)0);
}

#endif /* OVLB */
#ifdef OVL1

boolean
next_to_u()
{
	register struct monst *mtmp;
	register struct obj *otmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if(mtmp->mleashed) {
			if (distu(mtmp->mx,mtmp->my) > 2) mnexto(mtmp);
			if (distu(mtmp->mx,mtmp->my) > 2) {
			    for(otmp = invent; otmp; otmp = otmp->nobj)
				if(otmp->otyp == LEASH &&
					otmp->leashmon == (int)mtmp->m_id) {
				    if(otmp->cursed) return(FALSE);
#ifndef JP
				    You_feel("%s leash go slack.",
					(number_leashed() > 1) ? "a" : "the");
#else
				    Your("引き綱が緩んだ。");
#endif /*JP*/
				    mtmp->mleashed = 0;
				    otmp->leashmon = 0;
				}
			}
		}
	}
#ifdef STEED
	/* no pack mules for the Amulet */
	if (u.usteed && mon_has_amulet(u.usteed)) return FALSE;
#endif
	return(TRUE);
}

#endif /* OVL1 */
#ifdef OVL0

void
check_leash(x, y)
register xchar x, y;
{
	register struct obj *otmp;
	register struct monst *mtmp;

	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (otmp->otyp != LEASH || otmp->leashmon == 0) continue;
	    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if ((int)mtmp->m_id == otmp->leashmon) break; 
	    }
	    if (!mtmp) {
		impossible("leash in use isn't attached to anything?");
		otmp->leashmon = 0;
		continue;
	    }
	    if (dist2(u.ux,u.uy,mtmp->mx,mtmp->my) >
		    dist2(x,y,mtmp->mx,mtmp->my)) {
		if (!um_dist(mtmp->mx, mtmp->my, 3)) {
		    ;	/* still close enough */
		} else if (otmp->cursed && !breathless(mtmp->data)) {
		    if (um_dist(mtmp->mx, mtmp->my, 5) ||
			    (mtmp->mhp -= rnd(2)) <= 0) {
			long save_pacifism = u.uconduct.killer;

#ifndef JP
			Your("leash chokes %s to death!", mon_nam(mtmp));
#else
			Your("綱が%sを絞め殺した！", mon_nam(mtmp));
#endif /*JP*/
			/* hero might not have intended to kill pet, but
			   that's the result of his actions; gain experience,
			   lose pacifism, take alignment and luck hit, make
			   corpse less likely to remain tame after revival */
			xkilled(mtmp, 0);	/* no "you kill it" message */
			/* life-saving doesn't ordinarily reset this */
			if (mtmp->mhp > 0) u.uconduct.killer = save_pacifism;
		    } else {
			pline("%s chokes on the leash!", Monnam(mtmp));
			/* tameness eventually drops to 1 here (never 0) */
			if (mtmp->mtame && rn2(mtmp->mtame)) mtmp->mtame--;
		    }
		} else {
		    if (um_dist(mtmp->mx, mtmp->my, 5)) {
			pline(E_J("%s leash snaps loose!",
				  "%s綱が弾け飛び、緩んだ！"), s_suffix(Monnam(mtmp)));
			m_unleash(mtmp, FALSE);
		    } else {
			You(E_J("pull on the leash.","綱を引っぱった。"));
			if (mtmp->data->msound != MS_SILENT)
			    switch (rn2(3)) {
			    case 0:  growl(mtmp);   break;
			    case 1:  yelp(mtmp);    break;
			    default: whimper(mtmp); break;
			    }
		    }
		}
	    }
	}
}

#endif /* OVL0 */
#ifdef OVLB

#define WEAK	3	/* from eat.c */

static const char look_str[] = E_J("look %s.","%s見える。");

STATIC_OVL int
use_mirror(obj)
struct obj *obj;
{
	register struct monst *mtmp;
	register char mlet;
	boolean vis;

//	if(!getdir((char *)0)) return 0;
	if(!getdir_or_pos(0, GETPOS_MONTGT, (char *)0, E_J("reflect","映す"))) return 0;
	if(obj->cursed && !rn2(2)) {
		if (!Blind)
			pline_The(E_J("mirror fogs up and doesn't reflect!",
				      "鏡は曇って何も映さない！"));
		return 1;
	}
	if(!u.dx && !u.dy && !u.dz) {
		if(!Blind && !Invisible) {
		    if (u.umonnum == PM_FLOATING_EYE) {
			if (!Free_action) {
			pline(Hallucination ?
			      E_J("Yow!  The mirror stares back!","ギャッ！ 鏡が睨み返してきた！") :
			      E_J("Yikes!  You've frozen yourself!","しまった！ あなたは自分を麻痺させた！"));
			nomul(-rnd((MAXULEV+6) - u.ulevel));
			} else You(E_J("stiffen momentarily under your gaze.",
				       "自分の眼光で一瞬固まった。"));
		    } else if (youmonst.data->mlet == S_VAMPIRE)
			You(E_J("don't have a reflection.","鏡に映らない。"));
		    else if (u.umonnum == PM_UMBER_HULK) {
			pline(E_J("Huh?  That doesn't look like you!",
				  "えっ？ 鏡の姿はあなたと似ても似つかない！"));
			make_confused(HConfusion + d(3,4),FALSE);
		    } else if (Hallucination)
#ifndef JP
			You(look_str, hcolor((char *)0));
#else
			You(look_str, j_no_ni(hcolor((char *)0)));
#endif /*JP*/
		    else if (Sick)
			You(look_str, E_J("peaked","調子が悪そうに"));
		    else if (u.uhs >= WEAK)
			You(look_str, E_J("undernourished","栄養失調気味に"));
		    else You(E_J("look as %s as ever.","いつ見ても%s。"),
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? E_J("beautiful","美人だ") : E_J("handsome","いい男だ")) :
				E_J("ugly","醜い"));
		} else {
			You_cant(E_J("see your %s %s.","自分の%s%sを見ることができない。"),
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? E_J("beautiful","美しい") : E_J("handsome","ハンサムな")) :
				E_J("ugly","醜い"),
				body_part(FACE));
		}
		return 1;
	}
	if(u.uswallow) {
		if (!Blind) You(E_J("reflect %s %s.","%s%sを映した。"),
				s_suffix(mon_nam(u.ustuck)),
		    mbodypart(u.ustuck, STOMACH));
		return 1;
	}
	if(Underwater) {
		You(Hallucination ?
		    E_J("give the fish a chance to fix their makeup.","魚に化粧直しの機会を与えてやった。") :
		    E_J("reflect the murky water.","濁った水を映した。"));
		return 1;
	}
	if(u.dz) {
		if (!Blind)
		    You(E_J("reflect the %s.","%sを映した。"),
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
		return 1;
	}
	mtmp = bhit(u.dx, u.dy, COLNO, INVIS_BEAM,
		    (int FDECL((*),(MONST_P,OBJ_P)))0,
		    (int FDECL((*),(OBJ_P,OBJ_P)))0,
		    obj);
	if (!mtmp || !haseyes(mtmp->data))
		return 1;

	vis = canseemon(mtmp);
	mlet = mtmp->data->mlet;
	if (mtmp->msleeping) {
		if (vis)
		    pline (E_J("%s is too tired to look at your mirror.",
			       "%sはあまりに疲れていて、あなたの鏡を見られないようだ。"),
			    Monnam(mtmp));
	} else if (!mtmp->mcansee) {
	    if (vis)
		pline(E_J("%s can't see anything right now.",
			  "%sは今何も見ることができない。"), Monnam(mtmp));
	/* some monsters do special things */
	} else if (mlet == S_VAMPIRE || mlet == S_GHOST) {
	    if (vis)
		pline (E_J("%s doesn't have a reflection.",
			   "%sは鏡に映らない。"), Monnam(mtmp));
	} else if(!mtmp->mcan && !mtmp->minvis &&
					mtmp->data == &mons[PM_MEDUSA]) {
		if (mon_reflects(mtmp, E_J("The gaze is reflected away by %s %s!",
					   "眼光は%s%sにはね返された！")))
			return 1;
		if (vis)
			pline(E_J("%s is turned to stone!",
				  "%sは石になった！"), Monnam(mtmp));
		stoned = TRUE;
		killed(mtmp);
	} else if(!mtmp->mcan && !mtmp->minvis &&
					mtmp->data == &mons[PM_FLOATING_EYE]) {
		int tmp = d((int)mtmp->m_lev, (int)mtmp->data->mattk[0].damd);
		if (!rn2(4)) tmp = 120;
		if (vis)
			pline(E_J("%s is frozen by its reflection.",
				  "%sは自分の眼光の反射で動けなくなった。"), Monnam(mtmp));
		else You_hear(E_J("%s stop moving.","%sが動きを止めたのを"),something);
		mtmp->mcanmove = 0;
		if ( (int) mtmp->mfrozen + tmp > 127)
			mtmp->mfrozen = 127;
		else mtmp->mfrozen += tmp;
	} else if(!mtmp->mcan && !mtmp->minvis &&
					mtmp->data == &mons[PM_UMBER_HULK]) {
		if (vis)
			pline (E_J("%s confuses itself!",
				   "%sは自分の眼光で混乱した！"), Monnam(mtmp));
		mtmp->mconf = 1;
	} else if(!mtmp->mcan && !mtmp->minvis && (mlet == S_NYMPH
				     || mtmp->data==&mons[PM_SUCCUBUS])) {
		if (vis) {
		    pline (E_J("%s admires herself in your mirror.",
			       "%sは鏡に映った自分の姿にうっとりと見とれた。"), Monnam(mtmp));
		    pline (E_J("She takes it!","彼女は鏡を取っていった！"));
		} else pline (E_J("It steals your mirror!","何かがあなたの鏡を盗んだ！"));
		setnotworn(obj); /* in case mirror was wielded */
		freeinv(obj);
		(void) mpickobj(mtmp,obj);
		if (!tele_restrict(mtmp)) (void) rloc(mtmp, FALSE);
	} else if (!is_unicorn(mtmp->data) && !humanoid(mtmp->data) &&
			(!mtmp->minvis || perceives(mtmp->data)) && rn2(5)) {
		if (vis)
		    pline(E_J("%s is frightened by its reflection.",
			      "%sは鏡に映った自分の姿におびえた。"), Monnam(mtmp));
		monflee(mtmp, d(2,4), FALSE, FALSE);
	} else if (!Blind) {
		if (mtmp->minvis && !See_invisible)
		    ;
		else if ((mtmp->minvis && !perceives(mtmp->data))
			 || !haseyes(mtmp->data))
		    pline(E_J("%s doesn't seem to notice its reflection.",
			      "%sは鏡に映った自分の姿に気づかないようだ。"),
			Monnam(mtmp));
		else
#ifndef JP
		    pline("%s ignores %s reflection.",
			  Monnam(mtmp), mhis(mtmp));
#else
		    pline("%sは鏡を無視した。", mon_nam(mtmp));
#endif /*JP*/
	}
	return 1;
}

STATIC_OVL void
use_bell(optr)
struct obj **optr;
{
	register struct obj *obj = *optr;
	struct monst *mtmp;
	boolean wakem = FALSE, learno = FALSE,
		ordinary = (obj->otyp != BELL_OF_OPENING || !obj->spe),
		invoking = (obj->otyp == BELL_OF_OPENING &&
			 invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy));

#ifndef JP
	You("ring %s.", the(xname(obj)));
#else
	You("%sを鳴らした。", xname(obj));
#endif /*JP*/

	if (Underwater || (u.uswallow && ordinary)) {
#ifdef	AMIGA
	    amii_speaker( obj, "AhDhGqEqDhEhAqDqFhGw", AMII_MUFFLED_VOLUME );
#endif
	    pline(E_J("But the sound is muffled.",
		      "しかし、音はほとんど響かなかった。"));

	} else if (invoking && ordinary) {
	    /* needs to be recharged... */
	    pline(E_J("But it makes no sound.","しかし、音がしなかった。"));
	    learno = TRUE;	/* help player figure out why */

	} else if (ordinary) {
#ifdef	AMIGA
	    amii_speaker( obj, "ahdhgqeqdhehaqdqfhgw", AMII_MUFFLED_VOLUME );
#endif
	    if (obj->cursed && !rn2(4) &&
		    /* note: once any of them are gone, we stop all of them */
		    !(mvitals[PM_WOOD_NYMPH].mvflags & G_GONE) &&
		    !(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
		    !(mvitals[PM_MOUNTAIN_NYMPH].mvflags & G_GONE) &&
		    (mtmp = makemon(mkclass(S_NYMPH, 0),
					u.ux, u.uy, NO_MINVENT)) != 0) {
		You(E_J("summon %s!","%sを召喚した！"), a_monnam(mtmp));
		if (!obj_resists(obj, 93, 100)) {
#ifndef JP
		    pline("%s shattered!", Tobjnam(obj, "have"));
#else
		    pline("%sは粉々に割れた！", xname(obj));
#endif /*JP*/
		    useup(obj);
		    *optr = 0;
		} else switch (rn2(3)) {
			default:
				break;
			case 1:
				mon_adjust_speed(mtmp, 2, (struct obj *)0);
				break;
			case 2: /* no explanation; it just happens... */
				nomovemsg = "";
				nomul(-rnd(2));
				break;
		}
	    }
	    wakem = TRUE;

	} else {
	    /* charged Bell of Opening */
	    consume_obj_charge(obj, TRUE);

	    if (u.uswallow) {
		if (!obj->cursed)
		    (void) openit();
		else
		    pline(nothing_happens);

	    } else if (obj->cursed) {
		coord mm;

		mm.x = u.ux;
		mm.y = u.uy;
		mkundead(&mm, FALSE, NO_MINVENT);
		wakem = TRUE;

	    } else  if (invoking) {
#ifndef JP
		pline("%s an unsettling shrill sound...",
		      Tobjnam(obj, "issue"));
#else
		pline("%sから、背筋の凍るような、かん高い音が響いた…。",
		      xname(obj));
#endif /*JP*/
#ifdef	AMIGA
		amii_speaker( obj, "aefeaefeaefeaefeaefe", AMII_LOUDER_VOLUME );
#endif
		obj->age = moves;
		learno = TRUE;
		wakem = TRUE;

	    } else if (obj->blessed) {
		int res = 0;

#ifdef	AMIGA
		amii_speaker( obj, "ahahahDhEhCw", AMII_SOFT_VOLUME );
#endif
		if (uchain) {
		    unpunish();
		    res = 1;
		}
		res += openit();
		switch (res) {
		  case 0:  pline(nothing_happens); break;
		  case 1:  pline(E_J("%s opens...","%sが開いた…。"), Something);
			   learno = TRUE; break;
		  default: pline(E_J("Things open around you...","あなたの周囲の物が開いた…。"));
			   learno = TRUE; break;
		}

	    } else {  /* uncursed */
#ifdef	AMIGA
		amii_speaker( obj, "AeFeaeFeAefegw", AMII_OKAY_VOLUME );
#endif
		if (findit() != 0) learno = TRUE;
		else pline(nothing_happens);
	    }

	}	/* charged BofO */

	if (learno) {
	    makeknown(BELL_OF_OPENING);
	    obj->known = 1;
	}
	if (wakem) wake_nearby();
}

STATIC_OVL void
use_candelabrum(obj)
register struct obj *obj;
{
#ifndef JP
	const char *s = (obj->spe != 1) ? "candles" : "candle";
#else
	const char *s = "ろうそく";
#endif /*JP*/

	if(Underwater) {
		You(E_J("cannot make fire under water.","水中では火を起こせない。"));
		return;
	}
	if(obj->lamplit) {
		You(E_J("snuff the %s.","%sの火を消した。"), s);
		end_burn(obj, TRUE);
		return;
	}
	if(obj->spe <= 0) {
		pline(E_J("This %s has no %s.",
			  "この%sには%sがない。"), xname(obj), s);
		return;
	}
	if(u.uswallow || obj->cursed) {
		if (!Blind)
#ifndef JP
		    pline_The("%s %s for a moment, then %s.",
			      s, vtense(s, "flicker"), vtense(s, "die"));
#else
		    pline("%sの炎は一瞬またたき、消えた。", s);
#endif /*JP*/
		return;
	}
	if(obj->spe < 7) {
#ifndef JP
		There("%s only %d %s in %s.",
		      vtense(s, "are"), obj->spe, s, the(xname(obj)));
		if (!Blind)
		    pline("%s lit.  %s dimly.",
			  obj->spe == 1 ? "It is" : "They are",
			  Tobjnam(obj, "shine"));
#else
		pline("%sには%d本の%sが立てられている。",
		      xname(obj), obj->spe, s);
		if (!Blind)
		    pline("%sが灯り、弱く輝いた。", s);
#endif /*JP*/
	} else {
#ifndef JP
		pline("%s's %s burn%s", The(xname(obj)), s,
			(Blind ? "." : " brightly!"));
#else
		pline("%sの%sが%s", xname(obj), s,
			(Blind ? "灯った。" : "まばゆく燃え上がった！"));
#endif /*JP*/
	}
	if (!invocation_pos(u.ux, u.uy)) {
#ifndef JP
		pline_The("%s %s being rapidly consumed!", s, vtense(s, "are"));
#else
		pline("%sは急速に燃え尽きようとしている！", s);
#endif /*JP*/
		obj->age /= 2;
	} else {
		if(obj->spe == 7) {
		    if (Blind)
#ifndef JP
		      pline("%s a strange warmth!", Tobjnam(obj, "radiate"));
#else
		      pline("%sは不思議な暖かさを放っている！", xname(obj));
#endif /*JP*/
		    else
#ifndef JP
		      pline("%s with a strange light!", Tobjnam(obj, "glow"));
#else
		      pline("%sは不思議な光を放っている！", xname(obj));
#endif /*JP*/
		}
		obj->known = 1;
	}
	begin_burn(obj, FALSE);
}

STATIC_OVL void
use_candle(optr)
struct obj **optr;
{
	register struct obj *obj = *optr;
	register struct obj *otmp;
#ifndef JP
	const char *s = (obj->quan != 1) ? "candles" : "candle";
#else
	const char *s = "ろうそく";
#endif /*JP*/
	char qbuf[QBUFSZ];

	if(u.uswallow) {
		You(no_elbow_room);
		return;
	}
	if(Underwater) {
		pline(E_J("Sorry, fire and water don't mix.",
			  "ごめんなさい、水と火は相容れないんです。"));
		return;
	}

	otmp = carrying(CANDELABRUM_OF_INVOCATION);
	if(!otmp || otmp->spe == 7) {
		use_lamp(obj);
		return;
	}

#ifndef JP
	Sprintf(qbuf, "Attach %s", the(xname(obj)));
	Sprintf(eos(qbuf), " to %s?",
		safe_qbuf(qbuf, sizeof(" to ?"), the(xname(otmp)),
			the(simple_typename(otmp->otyp)), "it"));
#else
	Sprintf(qbuf, "%sを", xname(obj));
	Sprintf(eos(qbuf), "%sに立てますか？",
		safe_qbuf(qbuf, sizeof("に立てますか？"), xname(otmp),
			simple_typename(otmp->otyp), "これ"));
#endif /*JP*/
	if(yn(qbuf) == 'n') {
		if (!obj->lamplit)
#ifndef JP
		    You("try to light %s...", the(xname(obj)));
#else
		    You("%sを灯す…。", xname(obj));
#endif /*JP*/
		use_lamp(obj);
		return;
	} else {
		if ((long)otmp->spe + obj->quan > 7L)
		    obj = splitobj(obj, 7L - (long)otmp->spe);
		else *optr = 0;
#ifndef JP
		You("attach %ld%s %s to %s.",
		    obj->quan, !otmp->spe ? "" : " more",
		    s, the(xname(otmp)));
#else
		You("%s%ld本の%sを%sに立てた。",
		    !otmp->spe ? "" : "さらに", obj->quan,
		    s, xname(otmp));
#endif /*JP*/
		if (!otmp->spe || otmp->age > obj->age)
		    otmp->age = obj->age;
		otmp->spe += (int)obj->quan;
		if (otmp->lamplit && !obj->lamplit)
#ifndef JP
		    pline_The("new %s magically %s!", s, vtense(s, "ignite"));
#else
		    pline("新たな%sはひとりでに灯った！", s);
#endif /*JP*/
		else if (!otmp->lamplit && obj->lamplit)
#ifndef JP
		    pline("%s out.", (obj->quan > 1L) ? "They go" : "It goes");
#else
		    pline("火は消えた。");
#endif /*JP*/
		if (obj->unpaid)
#ifndef JP
		    verbalize("You %s %s, you bought %s!",
			      otmp->lamplit ? "burn" : "use",
			      (obj->quan > 1L) ? "them" : "it",
			      (obj->quan > 1L) ? "them" : "it");
#else
		    verbalize("%sたんだから、買ってもらうよ！",
			      otmp->lamplit ? "燃やし" : "使っ");
#endif /*JP*/
		if (obj->quan < 7L && otmp->spe == 7)
#ifndef JP
		    pline("%s now has seven%s candles attached.",
			  The(xname(otmp)), otmp->lamplit ? " lit" : "");
#else
		    pline("%sには今や七本の%sろうそくが立てられている。",
			  xname(otmp), otmp->lamplit ? "灯った" : "");
#endif /*JP*/
		/* candelabrum's light range might increase */
		if (otmp->lamplit) obj_merge_light_sources(otmp, otmp);
		/* candles are no longer a separate light source */
		if (obj->lamplit) end_burn(obj, TRUE);
		/* candles are now gone */
		useupall(obj);
	}
}

boolean
snuff_candle(otmp)  /* call in drop, throw, and put in box, etc. */
register struct obj *otmp;
{
	register boolean candle = Is_candle(otmp);

	if ((candle || otmp->otyp == CANDELABRUM_OF_INVOCATION) &&
		otmp->lamplit) {
	    char buf[BUFSZ];
	    xchar x, y;
	    register boolean many = candle ? otmp->quan > 1L : otmp->spe > 1;

	    (void) get_obj_location(otmp, &x, &y, 0);
	    if (otmp->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
#ifndef JP
		pline("%s %scandle%s flame%s extinguished.",
		      Shk_Your(buf, otmp),
		      (candle ? "" : "candelabrum's "),
		      (many ? "s'" : "'s"), (many ? "s are" : " is"));
#else
		pline("%s%sろうそくの炎は消えた。",
		      Shk_Your(buf, otmp), (candle ? "" : "燭台の"));
#endif /*JP*/
	   end_burn(otmp, TRUE);
	   return(TRUE);
	}
	return(FALSE);
}

/* called when lit lamp is hit by water or put into a container or
   you've been swallowed by a monster; obj might be in transit while
   being thrown or dropped so don't assume that its location is valid */
boolean
snuff_lit(obj)
struct obj *obj;
{
	xchar x, y;

	if (obj->lamplit) {
	    if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
		    obj->otyp == BRASS_LANTERN || obj->otyp == POT_OIL) {
		(void) get_obj_location(obj, &x, &y, 0);
		if (obj->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
#ifndef JP
		    pline("%s %s out!", Yname2(obj), otense(obj, "go"));
#else
		    pline("%sの灯りは消えた！", yname(obj));
#endif /*JP*/
		end_burn(obj, TRUE);
		return TRUE;
	    }
	    if (snuff_candle(obj)) return TRUE;
	}
	return FALSE;
}

/* Called when potentially lightable object is affected by fire_damage().
   Return TRUE if object was lit and FALSE otherwise --ALI */
boolean
catch_lit(obj)
struct obj *obj;
{
	xchar x, y;

	if (!obj->lamplit &&
	    (obj->otyp == MAGIC_LAMP || ignitable(obj))) {
	    if ((obj->otyp == MAGIC_LAMP ||
		 obj->otyp == CANDELABRUM_OF_INVOCATION) &&
		obj->spe == 0)
		return FALSE;
	    else if (obj->otyp != MAGIC_LAMP && obj->age == 0)
		return FALSE;
	    if (!get_obj_location(obj, &x, &y, 0))
		return FALSE;
	    if (obj->otyp == CANDELABRUM_OF_INVOCATION && obj->cursed)
		return FALSE;
	    if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
		 obj->otyp == BRASS_LANTERN) && obj->cursed && !rn2(2))
		return FALSE;
	    if (obj->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
#ifndef JP
		pline("%s %s light!", Yname2(obj), otense(obj, "catch"));
#else
		pline("%sに灯りがついた！", yname(obj));
#endif /*JP*/
	    if (obj->otyp == POT_OIL) makeknown(obj->otyp);
	    if (obj->unpaid && costly_spot(u.ux, u.uy) && (obj->where == OBJ_INVENT)) {
	        /* if it catches while you have it, then it's your tough luck */
		check_unpaid(obj);
#ifndef JP
	        verbalize("That's in addition to the cost of %s %s, of course.",
				Yname2(obj), obj->quan == 1 ? "itself" : "themselves");
#else
	        verbalize("もちろん、その分は%sの価格に上乗せされますよ。", yname(obj));
#endif /*JP*/
		bill_dummy_object(obj);
	    }
	    begin_burn(obj, FALSE);
	    return TRUE;
	}
	return FALSE;
}

STATIC_OVL void
use_lamp(obj)
struct obj *obj;
{
	char buf[BUFSZ];

	if(Underwater) {
		pline(E_J("This is not a diving lamp.",
			  "これは水中ランプではない。"));
		return;
	}
	if(obj->lamplit) {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN)
#ifndef JP
		    pline("%s lamp is now off.", Shk_Your(buf, obj));
#else
		    You("%sの灯りを消した。", cxname(obj));
#endif /*JP*/
		else
#ifndef JP
		    You("snuff out %s.", yname(obj));
#else
		    You("%sの火を消した。", cxname(obj));
#endif /*JP*/
		end_burn(obj, TRUE);
		return;
	}
	/* magic lamps with an spe == 0 (wished for) cannot be lit */
	if ((!Is_candle(obj) && obj->age == 0)
			|| (obj->otyp == MAGIC_LAMP && obj->spe == 0)) {
		if (obj->otyp == BRASS_LANTERN)
			Your(E_J("lamp has run out of power.",
				 "ランプは電池が切れている。"));
		else pline(E_J("This %s has no oil.",
			       "この%sは油を切らしている。"), xname(obj));
		return;
	}
	if (obj->cursed && !rn2(2)) {
#ifndef JP
		pline("%s for a moment, then %s.",
		      Tobjnam(obj, "flicker"), otense(obj, "die"));
#else
		pline("%sの炎は一瞬またたき、消えた。", xname(obj));
#endif /*JP*/
	} else {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN) {
		    check_unpaid(obj);
#ifndef JP
		    pline("%s lamp is now on.", Shk_Your(buf, obj));
#else
		    pline("ランプが灯った。");
#endif /*JP*/
		} else {	/* candle(s) */
#ifndef JP
		    pline("%s flame%s %s%s",
			s_suffix(Yname2(obj)),
			plur(obj->quan), otense(obj, "burn"),
			Blind ? "." : " brightly!");
#else
		    pline("%s%s",
			xname(obj),
			Blind ? "に火をつけた。" : "の炎が明るく燃え上がった！");
#endif /*JP*/
		    if (obj->unpaid && costly_spot(u.ux, u.uy) &&
			  obj->age == 20L * (long)objects[obj->otyp].oc_cost) {
#ifndef JP
			const char *ithem = obj->quan > 1L ? "them" : "it";
			verbalize("You burn %s, you bought %s!", ithem, ithem);
#else
			verbalize("火をつけたんだから、買ってもらうよ！");
#endif /*JP*/
			bill_dummy_object(obj);
		    }
		}
		begin_burn(obj, FALSE);
	}
}

STATIC_OVL void
light_cocktail(obj)
	struct obj *obj;	/* obj is a potion of oil */
{
	char buf[BUFSZ];

	if (u.uswallow) {
	    You(no_elbow_room);
	    return;
	}

	if (obj->lamplit) {
	    You(E_J("snuff the lit potion.",
		    "ビンの火を消した。"));
	    end_burn(obj, TRUE);
	    /*
	     * Free & add to re-merge potion.  This will average the
	     * age of the potions.  Not exactly the best solution,
	     * but its easy.
	     */
	    freeinv(obj);
	    (void) addinv(obj);
	    return;
	} else if (Underwater) {
#ifndef JP
	    There("is not enough oxygen to sustain a fire.");
#else
	    pline("ここには燃焼を続けるのに充分な酸素がない。");
#endif /*JP*/
	    return;
	}

#ifndef JP
	You("light %s potion.%s", shk_your(buf, obj),
	    Blind ? "" : "  It gives off a dim light.");
#else
	You("ビンに火をともした。%s",
	    Blind ? "" : "炎は弱い光を放っている。");
#endif /*JP*/
	if (obj->unpaid && costly_spot(u.ux, u.uy)) {
	    /* Normally, we shouldn't both partially and fully charge
	     * for an item, but (Yendorian Fuel) Taxes are inevitable...
	     */
	    check_unpaid(obj);
	    verbalize(E_J("That's in addition to the cost of the potion, of course.",
			  "もちろん、燃やした分は価格に上乗せされますからね。"));
	    bill_dummy_object(obj);
	}
	makeknown(obj->otyp);

	if (obj->quan > 1L) {
	    obj = splitobj(obj, 1L);
	    begin_burn(obj, FALSE);	/* burn before free to get position */
	    obj_extract_self(obj);	/* free from inv */

	    /* shouldn't merge */
	    obj = hold_another_object(obj, E_J("You drop %s!","あなたは%sを取り落とした！"),
				      doname(obj), (const char *)0);
	} else
	    begin_burn(obj, FALSE);
}

static NEARDATA const char cuddly[] = { TOOL_CLASS, GEM_CLASS, 0 };

int
getobj_filter_rub(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	if ((otmp->oclass == TOOL_CLASS &&
	     (otyp == OIL_LAMP || otyp == MAGIC_LAMP || otyp == BRASS_LANTERN)) ||
	    (otmp->oclass == GEM_CLASS && is_graystone(otmp)))
		return GETOBJ_CHOOSEIT;
	return 0;
}

int
dorub()
{
#ifdef JP
	static const struct getobj_words rubw = {
	    0, 0, "こする", "こすり"
	};
#endif /*JP*/
	struct obj *obj = getobj(cuddly, E_J("rub",&rubw), getobj_filter_rub);

	if (obj && obj->oclass == GEM_CLASS) {
	    if (is_graystone(obj)) {
		use_stone(obj);
		return 1;
	    } else {
		pline(E_J("Sorry, I don't know how to use that.",
			  "すみません、それをどう使えばいいのかわかりません。"));
		return 0;
	    }
	}

	if (!obj || !wield_tool(obj, E_J("rub","こする"))) return 0;

	/* now uwep is obj */
	if (uwep->otyp == MAGIC_LAMP) {
	    if (uwep->spe > 0 && !rn2(3)) {
		check_unpaid_usage(uwep, TRUE);		/* unusual item use */
		djinni_from_bottle(uwep);
		makeknown(MAGIC_LAMP);
		uwep->otyp = OIL_LAMP;
		uwep->spe = 0; /* for safety */
		uwep->age = rn1(500,1000);
		if (uwep->lamplit) begin_burn(uwep, TRUE);
		update_inventory();
	    } else if (rn2(2) && !Blind)
#ifndef JP
		You("see a puff of smoke.");
#else
		pline("煙が出てきた。");
#endif /*JP*/
	    else pline(nothing_happens);
	} else if (obj->otyp == BRASS_LANTERN) {
	    /* message from Adventure */
	    pline(E_J("Rubbing the electric lamp is not particularly rewarding.",
		      "電気式のランプをこすることはあまり意味のある行為とはいえない。"));
	    pline(E_J("Anyway, nothing exciting happens.",
		      "とにかく、何も面白いことは起きなかった。"));
	} else pline(nothing_happens);
	return 1;
}

int
dojump()
{
	/* Physical jump */
	return jump(0);
}

int
jump(magic)
int magic; /* 0=Physical, otherwise skill level */
{
	coord cc;

	if (!magic && (nolimbs(youmonst.data) || slithy(youmonst.data))) {
		/* normally (nolimbs || slithy) implies !Jumping,
		   but that isn't necessarily the case for knights */
		You_cant(E_J("jump; you have no legs!","跳べない。あなたには脚がない！"));
		return 0;
	} else if (!magic && !Jumping) {
		You_cant(E_J("jump very far.","遠くには跳べない。"));
		return 0;
	} else if (u.uswallow) {
		if (magic) {
			You(E_J("bounce around a little.","少し跳ね回った。"));
			return 1;
		}
		pline(E_J("You've got to be kidding!","ご冗談でしょう！"));
		return 0;
	} else if (u.uinwater) {
		if (magic) {
			You(E_J("swish around a little.","その場で音を立てて回転した。"));
			return 1;
		}
		pline(E_J("This calls for swimming, not jumping!",
			  "その動作のためには、跳ぶのではなく泳がねばならない！"));
		return 0;
	} else if (u.ustuck) {
		if (u.ustuck->mtame && !Conflict && !u.ustuck->mconf) {
		    You(E_J("pull free from %s.",
			    "%sから自分を引きはがした。"), mon_nam(u.ustuck));
		    u.ustuck = 0;
		    return 1;
		}
		if (magic) {
			You(E_J("writhe a little in the grasp of %s!",
				"%sに締め上げられたまま、身をよじらせた！"), mon_nam(u.ustuck));
			return 1;
		}
		You(E_J("cannot escape from %s!",
			"%sから逃れられない！"), mon_nam(u.ustuck));
		return 0;
	} else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		if (magic) {
			You(E_J("flail around a little.","その場で揺れ動いた。"));
			return 1;
		}
#ifndef JP
		You("don't have enough traction to jump.");
#else
		There("跳ぶためのしっかりした足場がない。");
#endif /*JP*/
		return 0;
	} else if (!magic && near_capacity() > UNENCUMBERED) {
		You(E_J("are carrying too much to jump!",
			"荷物が重すぎて跳び上がれない！"));
		return 0;
	} else if (!magic && (u.uhunger <= 100 || ACURR(A_STR) < 6)) {
#ifndef JP
		You("lack the strength to jump!");
#else
		pline("あなたには跳び上がるだけの力がない！");
#endif /*JP*/
		return 0;
	} else if (Wounded_legs) {
		long wl = (Wounded_legs & BOTH_SIDES);
		const char *bp = body_part(LEG);

#ifndef JP
		if (wl == BOTH_SIDES) bp = makeplural(bp);
#endif /*JP*/
#ifdef STEED
		if (u.usteed)
		    pline(E_J("%s is in no shape for jumping.",
			      "%sは跳べる状態ではない。"), Monnam(u.usteed));
		else
#endif
#ifndef JP
		Your("%s%s %s in no shape for jumping.",
		     (wl == LEFT_SIDE) ? "left " :
			(wl == RIGHT_SIDE) ? "right " : "",
		     bp, (wl == BOTH_SIDES) ? "are" : "is");
#else
		Your("%s%sは跳べる状態ではない。",
		     (wl == LEFT_SIDE) ? "左" :
			(wl == RIGHT_SIDE) ? "右" : "両",  bp);
#endif /*JP*/
		return 0;
	}
#ifdef STEED
	else if (u.usteed && u.utrap) {
		pline(E_J("%s is stuck in a trap.",
			  "%sは罠にはまっている。"), Monnam(u.usteed));
		return (0);
	}
#endif

	pline(E_J("Where do you want to jump?",
		  "どの地点まで跳びますか？"));
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, TRUE, E_J("the desired position","目標地点")) < 0)
		return 0;	/* user pressed ESC */
	if (!magic && !(HJumping & ~INTRINSIC) && !EJumping &&
			distu(cc.x, cc.y) != 5) {
		/* The Knight jumping restriction still applies when riding a
		 * horse.  After all, what shape is the knight piece in chess?
		 */
		pline(E_J("Illegal move!","桂馬飛びではない！"));
		return 0;
	} else if (distu(cc.x, cc.y) > (magic ? magic : 26/*9*/)) {
		pline(E_J("Too far!","遠すぎる！"));
		return 0;
	} else if (!cansee(cc.x, cc.y)) {
		You(E_J("cannot see where to land!","着地地点を確認できない！"));
		return 0;
	} else if (!isok(cc.x, cc.y)) {
		You(E_J("cannot jump there!","そこには跳べない！"));
		return 0;
	} else {
	    coord uc;
	    int range, temp;

	    if(u.utrap)
		switch(u.utraptype) {
		case TT_BEARTRAP: {
		    register long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;
		    You(E_J("rip yourself free of the bear trap!  Ouch!",
			    "自分をトラバサミから引き剥がした！ 痛い！"));
		    losehp(rnd(10), E_J("jumping out of a bear trap",
					"トラバサミから跳び出して"), KILLED_BY);
		    set_wounded_legs(side, rn1(1000,500));
		    break;
		  }
		case TT_PIT:
		    You(E_J("leap from the pit!","落し穴から跳びあがった！"));
		    break;
		case TT_WEB:
		    You(E_J("tear the web apart as you pull yourself free!",
			    "蜘蛛の巣を引きちぎって跳び出した！"));
		    deltrap(t_at(u.ux,u.uy));
		    break;
		case TT_LAVA:
		    You(E_J("pull yourself above the lava!",
			    "身体を溶岩から引き抜いた！"));
		    u.utrap = 0;
		    return 1;
		case TT_INFLOOR:
#ifndef JP
		    You("strain your %s, but you're still stuck in the floor.",
			makeplural(body_part(LEG)));
#else
		    You("%sに力を込めたが、床に挟まったままだ。",
			body_part(LEG));
#endif /*JP*/
		    set_wounded_legs(LEFT_SIDE, rn1(10, 11));
		    set_wounded_legs(RIGHT_SIDE, rn1(10, 11));
		    return 1;
		}

	    /*
	     * Check the path from uc to cc, calling hurtle_step at each
	     * location.  The final position actually reached will be
	     * in cc.
	     */
	    uc.x = u.ux;
	    uc.y = u.uy;
	    /* calculate max(abs(dx), abs(dy)) as the range */
	    range = cc.x - uc.x;
	    if (range < 0) range = -range;
	    temp = cc.y - uc.y;
	    if (temp < 0) temp = -temp;
	    if (range < temp)
		range = temp;
	    (void) walk_path(&uc, &cc, hurtle_step, (genericptr_t)&range);

	    /* A little Sokoban guilt... */
	    if (In_sokoban(&u.uz))
		change_luck(-1);

	    teleds(cc.x, cc.y, TRUE);
	    nomul(-1);
	    nomovemsg = "";
	    morehungry(rnd(25));
	    return 1;
	}
}

boolean
tinnable(corpse)
struct obj *corpse;
{
	if (corpse->oeaten) return 0;
	if (!mons[corpse->corpsenm].cnutrit) return 0;
	return 1;
}

STATIC_OVL void
use_tinning_kit(obj)
register struct obj *obj;
{
	register struct obj *corpse, *can;
#ifdef JP
	static const struct getobj_words tinw = { 0, 0, "缶詰にする", "缶詰にし" };
#endif /*JP*/

	/* This takes only 1 move.  If this is to be changed to take many
	 * moves, we've got to deal with decaying corpses...
	 */
	if (obj->spe <= 0) {
#ifndef JP
		You("seem to be out of tins.");
#else
		pline("もう缶がないようだ。");
#endif /*JP*/
		return;
	}
	if (!(corpse = floorfood(E_J("tin",&tinw), 2))) return;
	if (corpse->oeaten) {
#ifndef JP
		You("cannot tin %s which is partly eaten.",something);
#else
		pline("食べかけのものを缶詰にすることはできない。");
#endif /*JP*/
		return;
	}
	if (touch_petrifies(&mons[corpse->corpsenm])
		&& !Stone_resistance && !uarmg) {
	    char kbuf[BUFSZ];

	    if (poly_when_stoned(youmonst.data))
#ifndef JP
		You("tin %s without wearing gloves.",
			an(mons[corpse->corpsenm].mname));
#else
		You("手袋をせずに%sを缶詰にした。",
			JMONNAM(corpse->corpsenm));
#endif /*JP*/
	    else {
#ifndef JP
		pline("Tinning %s without wearing gloves is a fatal mistake...",
			an(mons[corpse->corpsenm].mname));
		Sprintf(kbuf, "trying to tin %s without gloves",
			an(mons[corpse->corpsenm].mname));
#else
		pline("手袋をせずに%sを缶詰にしようとするのは、致命的な過ちだ…。",
			JMONNAM(corpse->corpsenm));
		Sprintf(kbuf, "手袋をせずに%sを缶詰にしようとして",
			JMONNAM(corpse->corpsenm));
#endif /*JP*/
	    }
	    instapetrify(kbuf);
	}
	if (is_rider(&mons[corpse->corpsenm])) {
		(void) revive_corpse(corpse);
		verbalize(E_J("Yes...  But War does not preserve its enemies...",
			      "ああ…。だが《戦争》はけして敵を手元に残さない…。"));
		return;
	}
	if (mons[corpse->corpsenm].cnutrit == 0) {
		pline(E_J("That's too insubstantial to tin.",
			  "これは缶詰にするには実体がなさすぎる。"));
		return;
	}
	consume_obj_charge(obj, TRUE);

	if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
	    static const char you_buy_it[] = E_J("You tin it, you bought it!",
						 "缶詰にしたんだから、買ってもらうよ！");

	    can->corpsenm = corpse->corpsenm;
	    can->cursed = obj->cursed;
	    can->blessed = obj->blessed;
	    can->owt = weight(can);
	    can->known = 1;
	    can->spe = -1;  /* Mark tinned tins. No spinach allowed... */
	    if (carried(corpse)) {
		if (corpse->unpaid)
		    verbalize(you_buy_it);
		useup(corpse);
	    } else {
		if (costly_spot(corpse->ox, corpse->oy) && !corpse->no_charge)
		    verbalize(you_buy_it);
		useupf(corpse, 1L);
	    }
	    can = hold_another_object(can, E_J("You make, but cannot pick up, %s.",
					       "あなたは%sを作ったが、拾えなかった。"),
				      doname(can), (const char *)0);
	} else impossible("Tinning failed.");
}

#ifdef FIRSTAID
int
getobj_filter_scissors(otmp)
struct obj *otmp;
{
	return ((get_material(otmp) == CLOTH) &&
		!otmp->owornmask &&
		!Is_container(otmp) &&
		!is_helmet(otmp) &&
		(otmp->otyp != BANDAGE) ?
			GETOBJ_CHOOSEIT : 0);
}

STATIC_OVL int
use_scissors(obj)
register struct obj *obj;
{
	static const char clothonly[] = { ARMOR_CLASS, TOOL_CLASS, 0 };
	struct obj *otmp, *bobj;
	int cnt;
#ifdef JP
	static const struct getobj_words cutw = {
	    0, 0, "切る", "切り"
	};
#endif /*JP*/

	otmp = getobj(clothonly, E_J("cut",&cutw), getobj_filter_scissors);
	if (!otmp) return 0;
	if (get_material(otmp) != CLOTH) {
	    You_cant(E_J("cut that!","それを切ることはできない！"));
	    return 0;
	}
	if (otmp->owornmask) {
#ifndef JP
	    You_cant("cut %s you're wearing.", something);
#else
	    You("着用中のものを切ることはできない。");
#endif /*JP*/
	    return 0;
	}

	switch (otmp->otyp) {
	    case BANDAGE:
		cnt = 0;
		break;
	    case MUMMY_WRAPPING:
		cnt = 1;
		break;
	    case KITCHEN_APRON:
	    case FRILLED_APRON:
		cnt = 3;
		break;
	    case TOWEL:
#ifdef TOURIST
	    case T_SHIRT:
	    case HAWAIIAN_SHIRT:
#endif /*TOURIST*/
		cnt = 5;
		break;
	    case NURSE_UNIFORM:
		cnt = 7;
		break;
	    default:
		cnt = weight(otmp) / 4;
	}

	if (!Role_if(PM_HEALER) &&
	    !(flags.female && ((uarm && uarm->otyp == NURSE_UNIFORM) ||
			       (uarmh && uarmh->otyp == NURSE_CAP)))
	   ) {
//	    cnt = rn2(cnt+1);
	    if (ACURR(A_DEX) < rnd(18)) cnt = rn2(cnt+1);
	    if (cnt && obj->cursed) cnt = rn2(cnt);
	}
	if (cnt <= 0) {
	    You(E_J("cut your %s in sunder.","%sをずたずたに切ってしまった。"), xname(otmp));
	    useup(otmp);
	    update_inventory();
	    return 1;
	}

	bobj = mksobj(BANDAGE, FALSE, FALSE);
	bobj->quan = (long)cnt;
	bobj->owt = weight(bobj);
	bobj->cursed  = (obj->cursed  || otmp->cursed );
	bobj->blessed = (obj->blessed || otmp->blessed);
	if (bobj->cursed && bobj->blessed)
		bobj->blessed = bobj->cursed = 0;
	useup(otmp);
#ifndef JP
	You("cut your %s into %s.", xname(otmp), (cnt > 1) ? "some bandages" : "a bandage");
#else
	You("%sを切って包帯にした。", xname(otmp));
#endif /*JP*/
#ifndef JP
	bobj = hold_another_object(bobj,
			(u.uswallow || Is_airlevel(&u.uz) ||
			 u.uinwater || Is_waterlevel(&u.uz)) ?
			       "Oops!  %s away from you!" :
			       "Oops!  %s to the floor!",
			 The(aobjnam(otmp, "slip")),
			 (const char *)0);
#else
	bobj = hold_another_object(bobj,
			(u.uswallow || Is_airlevel(&u.uz) ||
			 u.uinwater || Is_waterlevel(&u.uz)) ?
			       "うわっ！ %sは漂い去った！" :
			       "うわっ！ %sが床に滑り落ちた！",
			 xname(otmp),
			 (const char *)0);
#endif /*JP*/

	update_inventory();
	return 1;
}

STATIC_PTR
int
putbandage(void)		/* called during each move whilst putting a bandage */
{
	struct obj *otmp = bandage.bandage;

	if(!carried(otmp))	/* perhaps it was stolen? */
		return(0);
	if(bandage.usedtime++ >= 50) {
		You(E_J("give up your attempt to put the bandage.",
			"包帯を巻くのをあきらめた。"));
		return(0);
	}
	if(bandage.usedtime < bandage.reqtime)
		return(1);		/* still busy */
	You(E_J("finish first aid.","応急処置を終えた。"));
	if (!otmp->cursed || !rn2(3)) {
		You_feel(E_J("better.","元気になった。"));
		healup(d(4 + 2 * bcsign(otmp), 4), 0, FALSE, FALSE);
	} else {
	    const char *mp = E_J("medical malpractice","医療ミスで");
	    if (!Sick_resistance && !rn2(13)) {
		pline(E_J("The bandage is badly contaminated!",
			  "この包帯はひどく汚染されていた！"));
		make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON),20),
			  mp, TRUE, SICK_NONVOMITABLE);
	    } else {
		pline(E_J("The bandage must be unclean!",
			  "この包帯は不潔だったに違いない！"));
		You_feel(E_J("sick.","具合が悪くなった。"));
		losehp(rnd(15), mp, KILLED_BY_AN);
	    }
	}
	useup(otmp);
	otmp = (struct obj *) 0;
	return(0);
}
#endif /*FIRSTAID*/

void
use_unicorn_horn(obj)
struct obj *obj;
{
#define PROP_COUNT 6		/* number of properties we're dealing with */
#define ATTR_COUNT (A_MAX*3)	/* number of attribute points we might fix */
	int idx, val, val_limit,
	    trouble_count, unfixable_trbl, did_prop, did_attr;
	int trouble_list[PROP_COUNT + ATTR_COUNT];

	if (obj && obj->cursed) {
	    long lcount = (long) rnd(100);

	    switch (rn2(6)) {
	    case 0: make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON),20),
			xname(obj), TRUE, SICK_NONVOMITABLE);
		    break;
	    case 1: make_blinded(Blinded + lcount, TRUE);
		    break;
	    case 2: if (!Confusion)
			You(E_J("suddenly feel %s.","急に%sした。"),
			    Hallucination ? E_J("trippy","トリップ") : E_J("confused","混乱"));
		    make_confused(HConfusion + lcount, TRUE);
		    break;
	    case 3: make_stunned(HStun + lcount, TRUE);
		    break;
	    case 4: (void) adjattrib(rn2(A_MAX), -1, FALSE);
		    break;
	    case 5: (void) make_hallucinated(HHallucination + lcount, TRUE, 0L);
		    break;
	    }
	    if (!rn2(5) && uwep != obj) {
		Your(E_J("unicorn horn disintegrates.",
			 "ユニコーンの角は崩れ去った。"));
		useup(obj);
	    }
	    return;
	}

/*
 * Entries in the trouble list use a very simple encoding scheme.
 */
#define prop2trbl(X)	((X) + A_MAX)
#define attr2trbl(Y)	(Y)
#define prop_trouble(X) trouble_list[trouble_count++] = prop2trbl(X)
#define attr_trouble(Y) trouble_list[trouble_count++] = attr2trbl(Y)

	trouble_count = unfixable_trbl = did_prop = did_attr = 0;

	/* collect property troubles */
	if (Sick) prop_trouble(SICK);
	if (Blinded > (long)u.ucreamed) prop_trouble(BLINDED);
	if (HHallucination) prop_trouble(HALLUC);
	if (Vomiting) prop_trouble(VOMITING);
	if (HConfusion) prop_trouble(CONFUSION);
	if (HStun) prop_trouble(STUNNED);

	unfixable_trbl = unfixable_trouble_count(TRUE);

	/* collect attribute troubles */
	for (idx = 0; idx < A_MAX; idx++) {
	    val_limit = AMAX(idx);
	    /* don't recover strength lost from hunger */
	    if (idx == A_STR && u.uhs >= WEAK) val_limit--;
	    /* don't recover more than 3 points worth of any attribute */
	    if (val_limit > ABASE(idx) + 3) val_limit = ABASE(idx) + 3;

	    for (val = ABASE(idx); val < val_limit; val++)
		attr_trouble(idx);
	    /* keep track of unfixed trouble, for message adjustment below */
	    unfixable_trbl += (AMAX(idx) - val_limit);
	}

	if (trouble_count == 0) {
	    pline(nothing_happens);
	    return;
	} else if (trouble_count > 1) {		/* shuffle */
	    int i, j, k;

	    for (i = trouble_count - 1; i > 0; i--)
		if ((j = rn2(i + 1)) != i) {
		    k = trouble_list[j];
		    trouble_list[j] = trouble_list[i];
		    trouble_list[i] = k;
		}
	}

	/*
	 *		Chances for number of troubles to be fixed
	 *		 0	1      2      3      4	    5	   6	  7
	 *   blessed:  22.7%  22.7%  19.5%  15.4%  10.7%   5.7%   2.6%	 0.8%
	 *  uncursed:  35.4%  35.4%  22.9%   6.3%    0	    0	   0	  0
	 */
	val_limit = rn2( d(2, (obj && obj->blessed) ? 4 : 2) );
	if (val_limit > trouble_count) val_limit = trouble_count;

	/* fix [some of] the troubles */
	for (val = 0; val < val_limit; val++) {
	    idx = trouble_list[val];

	    switch (idx) {
	    case prop2trbl(SICK):
		make_sick(0L, (char *) 0, TRUE, SICK_ALL);
		did_prop++;
		break;
	    case prop2trbl(BLINDED):
		make_blinded((long)u.ucreamed, TRUE);
		did_prop++;
		break;
	    case prop2trbl(HALLUC):
		(void) make_hallucinated(0L, TRUE, 0L);
		did_prop++;
		break;
	    case prop2trbl(VOMITING):
		make_vomiting(0L, TRUE);
		did_prop++;
		break;
	    case prop2trbl(CONFUSION):
		make_confused(0L, TRUE);
		did_prop++;
		break;
	    case prop2trbl(STUNNED):
		make_stunned(0L, TRUE);
		did_prop++;
		break;
	    default:
		if (idx >= 0 && idx < A_MAX) {
		    ABASE(idx) += 1;
		    did_attr++;
		} else
		    panic("use_unicorn_horn: bad trouble? (%d)", idx);
		break;
	    }
	}

	if (did_attr)
	    pline(E_J("This makes you feel %s!","あなたは%s気分が良くなった！"),
		  (did_prop + did_attr) == (trouble_count + unfixable_trbl) ?
		  E_J("great","とても") : E_J("better","やや"));
	else if (!did_prop)
	    pline(E_J("Nothing seems to happen.","何も起きないようだ。"));

	if ((did_attr || did_prop) && !rn2(20)) {
		Your(E_J("unicorn horn disintegrates.",
			 "ユニコーンの角は崩れ去った。"));
		remove_worn_item(obj, FALSE);
		useup(obj);
	}

	flags.botl = (did_attr || did_prop);

#undef PROP_COUNT
#undef ATTR_COUNT
#undef prop2trbl
#undef attr2trbl
#undef prop_trouble
#undef attr_trouble
}

/*
 * Timer callback routine: turn figurine into monster
 */
void
fig_transform(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *figurine = (struct obj *)arg;
	struct monst *mtmp;
	coord cc;
	boolean cansee_spot, silent, okay_spot;
	boolean redraw = FALSE;
	char monnambuf[BUFSZ], carriedby[BUFSZ];

	if (!figurine) {
#ifdef DEBUG
	    pline("null figurine in fig_transform()");
#endif
	    return;
	}
	silent = (timeout != monstermoves); /* happened while away */
	okay_spot = get_obj_location(figurine, &cc.x, &cc.y, 0);
	if (figurine->where == OBJ_INVENT ||
	    figurine->where == OBJ_MINVENT)
		okay_spot = enexto(&cc, cc.x, cc.y,
				   &mons[figurine->corpsenm]);
	if (!okay_spot ||
	    !figurine_location_checks(figurine,&cc, TRUE)) {
		/* reset the timer to try again later */
		(void) start_timer((long)rnd(5000), TIMER_OBJECT,
				FIG_TRANSFORM, (genericptr_t)figurine);
		return;
	}

	cansee_spot = cansee(cc.x, cc.y);
	mtmp = make_familiar(figurine, cc.x, cc.y, TRUE);
	if (mtmp) {
#ifndef JP
	    Sprintf(monnambuf, "%s",an(m_monnam(mtmp)));
#else
	    Strcpy(monnambuf, m_monnam(mtmp));
#endif /*JP*/
	    switch (figurine->where) {
		case OBJ_INVENT:
		    if (Blind)
#ifndef JP
			You_feel("%s %s from your pack!", something,
			    locomotion(mtmp->data,"drop"));
#else
			You("%sがあなたの背負い袋から%s出るのを感じた！", something,
			    locomotion(mtmp->data,"転げ"));
#endif /*JP*/
		    else
#ifndef JP
			You("see %s %s out of your pack!",
			    monnambuf,
			    locomotion(mtmp->data,"drop"));
#else
			Your("背負い袋から%sが%s出た！",
			    monnambuf,
			    locomotion(mtmp->data,"転げ"));
#endif /*JP*/
		    break;

		case OBJ_FLOOR:
		    if (cansee_spot && !silent) {
#ifndef JP
			You("suddenly see a figurine transform into %s!", monnambuf);
#else
			Your("目の前で人形が%sに変化した！", monnambuf);
#endif /*JP*/
			redraw = TRUE;	/* update figurine's map location */
		    }
		    break;

		case OBJ_MINVENT:
		    if (cansee_spot && !silent) {
			struct monst *mon;
			mon = figurine->ocarry;
			/* figurine carring monster might be invisible */
			if (canseemon(figurine->ocarry)) {
			    Sprintf(carriedby, E_J("%s pack","%s荷物の中"),
				     s_suffix(a_monnam(mon)));
			}
			else if (is_pool(mon->mx, mon->my))
			    Strcpy(carriedby, E_J("empty water","水の中"));
			else
			    Strcpy(carriedby, E_J("thin air","何もない空中"));
#ifndef JP
			You("see %s %s out of %s!", monnambuf,
			    locomotion(mtmp->data, "drop"), carriedby);
#else
			You("%sから%sが現れるのを目撃した！", carriedby, monnambuf);
#endif /*JP*/
		    }
		    break;
#if 0
		case OBJ_MIGRATING:
		    break;
#endif

		default:
		    impossible("figurine came to life where? (%d)",
				(int)figurine->where);
		break;
	    }
	}
	/* free figurine now */
	obj_extract_self(figurine);
	obfree(figurine, (struct obj *)0);
	if (redraw) newsym(cc.x, cc.y);
}

STATIC_OVL boolean
figurine_location_checks(obj, cc, quietly)
struct obj *obj;
coord *cc;
boolean quietly;
{
	xchar x,y;

	if (carried(obj) && u.uswallow) {
		if (!quietly)
#ifndef JP
 			You("don't have enough room in here.");
#else
			pline("ここには十分な場所がない。");
#endif /*JP*/
		return FALSE;
	}
	x = cc->x; y = cc->y;
	if (!isok(x,y)) {
		if (!quietly)
#ifndef JP
 			You("cannot put the figurine there.");
#else
			pline("ここには人形を置けない。");
#endif /*JP*/
		return FALSE;
	}
	if (IS_ROCK(levl[x][y].typ) &&
	    !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x,y))) {
		if (!quietly)
		    You(E_J("cannot place a figurine in %s!",
			    "人形を%sの中には置けない！"),
			IS_TREES(levl[x][y].typ) ? E_J("a tree","木") : E_J("solid rock","固い石"));
		return FALSE;
	}
	if (sobj_at(BOULDER,x,y) && !passes_walls(&mons[obj->corpsenm])
			&& !throws_rocks(&mons[obj->corpsenm])) {
		if (!quietly)
			You(E_J("cannot fit the figurine on the boulder.",
				"大岩の上にうまく人形を乗せられなかった。"));
		return FALSE;
	}
	return TRUE;
}

STATIC_OVL void
use_figurine(optr)
struct obj **optr;
{
	register struct obj *obj = *optr;
	xchar x, y;
	coord cc;

	if (u.uswallow) {
		/* can't activate a figurine while swallowed */
		if (!figurine_location_checks(obj, (coord *)0, FALSE))
			return;
	}
	if(!getdir((char *)0)) {
		flags.move = multi = 0;
		return;
	}
	x = u.ux + u.dx; y = u.uy + u.dy;
	cc.x = x; cc.y = y;
	/* Passing FALSE arg here will result in messages displayed */
	if (!figurine_location_checks(obj, &cc, FALSE)) return;
#ifndef JP
	You("%s and it transforms.",
	    (u.dx||u.dy) ? "set the figurine beside you" :
	    (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) ||
	     is_pool(cc.x, cc.y)) ?
		"release the figurine" :
	    (u.dz < 0 ?
		"toss the figurine into the air" :
		"set the figurine on the ground"));
#else
	pline("あなたが人形を%sと、人形は姿を変えた。",
	    (u.dx||u.dy) ? "わきに置く" :
	    (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz) ||
	     is_pool(cc.x, cc.y)) ? "手放す" :
	    (u.dz < 0 ? "空中に投げ上げる" : "地面に置く"));
#endif /*JP*/
	(void) make_familiar(obj, cc.x, cc.y, FALSE);
	(void) stop_timer(FIG_TRANSFORM, (genericptr_t)obj);
	useup(obj);
	*optr = 0;
}

static NEARDATA const char lubricables[] = { ALL_CLASSES, ALLOW_NONE, 0 };
static NEARDATA const char need_to_remove_outer_armor[] =
			E_J("need to remove your %s to grease your %s.",
			    "%sに脂を塗るには、%sを脱ぐ必要がある。");

STATIC_OVL void
use_grease(obj)
struct obj *obj;
{
	struct obj *otmp;
	char buf[BUFSZ];
#ifdef JP
	static const struct getobj_words grw = { 0, "に", "脂を塗る", "脂を塗り" };
#endif /*JP*/

	if (Glib) {
#ifndef JP
	    pline("%s from your %s.", Tobjnam(obj, "slip"),
		  makeplural(body_part(FINGER)));
#else
	    pline("%sはあなたの%sから滑り落ちた。", xname(obj), body_part(FINGER));
#endif /*JP*/
	    dropx(obj);
	    return;
	}

	if (obj->spe > 0) {
		if ((obj->cursed || Fumbling) && !rn2(2)) {
			consume_obj_charge(obj, TRUE);

#ifndef JP
			pline("%s from your %s.", Tobjnam(obj, "slip"),
			      makeplural(body_part(FINGER)));
#else
			pline("%sはあなたの%sから滑り落ちた。", xname(obj), body_part(FINGER));
#endif /*JP*/
			dropx(obj);
			return;
		}
		otmp = getobj(lubricables, E_J("grease",&grw), 0);
		if (!otmp) return;
		if ((otmp->owornmask & WORN_ARMOR) && uarmc) {
			Strcpy(buf, xname(uarmc));
#ifndef JP
			You(need_to_remove_outer_armor, buf, xname(otmp));
#else
			pline(need_to_remove_outer_armor, xname(otmp), buf);
#endif /*JP*/
			return;
		}
#ifdef TOURIST
		if ((otmp->owornmask & WORN_SHIRT) && (uarmc || uarm)) {
			Strcpy(buf, uarmc ? xname(uarmc) : "");
			if (uarmc && uarm) Strcat(buf, E_J(" and ","と"));
			Strcat(buf, uarm ? xname(uarm) : "");
#ifndef JP
			You(need_to_remove_outer_armor, buf, xname(otmp));
#else
			pline(need_to_remove_outer_armor, xname(otmp), buf);
#endif /*JP*/
			return;
		}
#endif
		consume_obj_charge(obj, TRUE);

		if (otmp != &zeroobj) {
#ifndef JP
			You("cover %s with a thick layer of grease.",
			    yname(otmp));
#else
			You("%sを厚い脂の層で覆った。", xname(otmp));
#endif /*JP*/
			otmp->greased = 1;
			if (obj->cursed && !nohands(youmonst.data)) {
			    incr_itimeout(&Glib, rnd(15));
#ifndef JP
			    pline("Some of the grease gets all over your %s.",
				makeplural(body_part(HAND)));
#else
			    Your("%sまで脂まみれになってしまった。", body_part(HAND));
#endif /*JP*/
			}
		} else {
			Glib += rnd(15);
#ifndef JP
			You("coat your %s with grease.",
			    makeplural(body_part(FINGER)));
#else
			You("自分の%sを脂で覆った", body_part(FINGER));
#endif /*JP*/
		}
	} else {
#ifndef JP
	    if (obj->known)
		pline("%s empty.", Tobjnam(obj, "are"));
	    else
		pline("%s to be empty.", Tobjnam(obj, "seem"));
#else
	    pline("%sは空%sだ。", xname(obj), (obj->known) ? "" : "のよう");
#endif /*JP*/
	}
	update_inventory();
}

static struct trapinfo {
	struct obj *tobj;
	xchar tx, ty;
	int time_needed;
	boolean force_bungle;
} trapinfo;

void
reset_trapset()
{
	trapinfo.tobj = 0;
	trapinfo.force_bungle = 0;
}

/* touchstones - by Ken Arnold */
STATIC_OVL void
use_stone(tstone)
struct obj *tstone;
{
    struct obj *obj;
    boolean do_scratch;
    const char *streak_color, *choices;
    char stonebuf[QBUFSZ];
    static const char scritch[] = E_J("\"scritch, scritch\"",
				      "カリカリ…。");
    static const char allowall[3] = { COIN_CLASS, ALL_CLASSES, 0 };
    static const char justgems[3] = { ALLOW_NONE, GEM_CLASS, 0 };
#ifndef GOLDOBJ
    struct obj goldobj;
#endif
#ifdef JP
    static const struct getobj_words tsw = { 0, 0, "石でこする", "石でこすり" };
#endif /*JP*/

    /* in case it was acquired while blinded */
    if (!Blind) tstone->dknown = 1;
    /* when the touchstone is fully known, don't bother listing extra
       junk as likely candidates for rubbing */
    choices = (tstone->otyp == TOUCHSTONE && tstone->dknown &&
		objects[TOUCHSTONE].oc_name_known) ? justgems : allowall;
#ifndef JP
    Sprintf(stonebuf, "rub on the stone%s", plur(tstone->quan));
#endif /*JP*/
    if ((obj = getobj(choices, E_J(stonebuf,&tsw), 0)) == 0)
	return;
#ifndef GOLDOBJ
    if (obj->oclass == COIN_CLASS) {
	u.ugold += obj->quan;	/* keep botl up to date */
	goldobj = *obj;
	dealloc_obj(obj);
	obj = &goldobj;
    }
#endif

    if (obj == tstone && obj->quan == 1) {
#ifndef JP
	You_cant("rub %s on itself.", the(xname(obj)));
#else
	You("%sでそれ自体をこすることはできない。", xname(obj));
#endif /*JP*/
	return;
    }

    if (tstone->otyp == TOUCHSTONE && tstone->cursed &&
	    obj->oclass == GEM_CLASS && !is_graystone(obj) &&
	    !obj_resists(obj, 80, 100)) {
	if (Blind)
	    pline(E_J("You feel something shatter.","何かが砕けたようだ。"));
	else if (Hallucination)
	    pline(E_J("Oh, wow, look at the pretty shards.",
		      "うわぁい、この可愛い破片を見てよ！"));
	else
#ifndef JP
	    pline("A sharp crack shatters %s%s.",
		  (obj->quan > 1) ? "one of " : "", the(xname(obj)));
#else
	    pline("強くこすりすぎて、%sを%s砕いてしまった。",
		  xname(obj), (obj->quan > 1) ? "一つ" : "");
#endif /*JP*/
#ifndef GOLDOBJ
     /* assert(obj != &goldobj); */
#endif
	useup(obj);
	return;
    }

    if (Blind) {
	pline(scritch);
	return;
    } else if (Hallucination) {
	pline(E_J("Oh wow, man: Fractals!","おお、すごい…、フラクタルだ！"));
	return;
    }

    do_scratch = FALSE;
    streak_color = 0;

    switch (obj->oclass) {
    case GEM_CLASS:	/* these have class-specific handling below */
    case RING_CLASS:
	if (tstone->otyp != TOUCHSTONE) {
	    do_scratch = TRUE;
	} else if (obj->oclass == GEM_CLASS && (tstone->blessed ||
		(!tstone->cursed &&
		    (Role_if(PM_ARCHEOLOGIST) || Race_if(PM_GNOME))))) {
	    makeknown(TOUCHSTONE);
	    makeknown(obj->otyp);
	    prinv((char *)0, obj, 0L);
	    return;
	} else {
	    /* either a ring or the touchstone was not effective */
	    if (get_material(obj) == GLASS) {
		do_scratch = TRUE;
		break;
	    }
	}
	streak_color = c_obj_colors[objects[obj->otyp].oc_color];
	break;		/* gem or ring */

    default:
	switch (get_material(obj)) {
	case CLOTH:
#ifndef JP
	    pline("%s a little more polished now.", Tobjnam(tstone, "look"));
#else
	    pline("%sは少し磨かれた。", xname(tstone));
#endif /*JP*/
	    return;
	case LIQUID:
	    if (!obj->known)		/* note: not "whetstone" */
		You(E_J("must think this is a wetstone, do you?",
			"これぞウェットストーンとでも思ってるんじゃないか？"));
	    else
#ifndef JP
		pline("%s a little wetter now.", Tobjnam(tstone, "are"));
#else
		pline("%sは少し湿った。", xname(tstone));
#endif /*JP*/
	    return;
	case WAX:
	    streak_color = E_J("waxy","ろうの");
	    break;		/* okay even if not touchstone */
	case WOOD:
	    streak_color = E_J("wooden","木質の");
	    break;		/* okay even if not touchstone */
	case GOLD:
	    do_scratch = TRUE;	/* scratching and streaks */
	    streak_color = E_J("golden","金色の");
	    break;
	case SILVER:
	    do_scratch = TRUE;	/* scratching and streaks */
	    streak_color = E_J("silvery","銀色の");
	    break;
	default:
	    /* Objects passing the is_flimsy() test will not
	       scratch a stone.  They will leave streaks on
	       non-touchstones and touchstones alike. */
	    if (is_flimsy(obj))
		streak_color = c_obj_colors[objects[obj->otyp].oc_color];
	    else
		do_scratch = (tstone->otyp != TOUCHSTONE);
	    break;
	}
	break;		/* default oclass */
    }

#ifndef JP
    Sprintf(stonebuf, "stone%s", plur(tstone->quan));
#endif /*JP*/
    if (do_scratch)
#ifndef JP
	pline("You make %s%sscratch marks on the %s.",
	      streak_color ? streak_color : (const char *)"",
	      streak_color ? " " : "", stonebuf);
#else
	pline("石に%s条痕がついた。",
	      streak_color ? streak_color : (const char *)"");
#endif /*JP*/
    else if (streak_color)
#ifndef JP
	pline("You see %s streaks on the %s.", streak_color, stonebuf);
#else
	pline("石に%s筋がついた。", streak_color);
#endif /*JP*/
    else
	pline(scritch);
    return;
}

/* Place a landmine/bear trap.  Helge Hafting */
STATIC_OVL void
use_trap(otmp)
struct obj *otmp;
{
	int ttyp, tmp;
	const char *what = (char *)0;
	char buf[BUFSZ];
	const char *occutext = E_J("setting the trap","罠を仕掛けるのを中止した");

	if (nohands(youmonst.data))
	    what = E_J("without hands","手がなくては");
	else if (Stunned)
	    what = E_J("while stunned","不安定な姿勢では");
	else if (u.uswallow)
	    what = is_animal(u.ustuck->data) ? E_J("while swallowed","飲み込まれているので") :
			E_J("while engulfed","包み込まれているので");
	else if (Underwater)
	    what = E_J("underwater","水中には");
	else if (Levitation)
	    what = E_J("while levitating","浮遊していては");
	else if (is_pool(u.ux, u.uy))
	    what = E_J("in water","水中には");
	else if (is_lava(u.ux, u.uy))
	    what = E_J("in lava","溶岩の中には");
	else if (On_stairs(u.ux, u.uy))
	    what = (u.ux == xdnladder || u.ux == xupladder) ?
			E_J("on the ladder","はしごの上には") :
			E_J("on the stairs","階段の上には");
	else if (IS_FURNITURE(levl[u.ux][u.uy].typ) ||
		IS_ROCK(levl[u.ux][u.uy].typ) ||
		closed_door(u.ux, u.uy) || t_at(u.ux, u.uy))
	    what = E_J("here","ここ");
	if (what) {
	    You_cant(E_J("set a trap %s!","%s罠を仕掛けられない！"),what);
	    reset_trapset();
	    return;
	}
	ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
	if (otmp == trapinfo.tobj &&
		u.ux == trapinfo.tx && u.uy == trapinfo.ty) {
#ifndef JP
	    You("resume setting %s %s.",
		shk_your(buf, otmp),
		defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
#else
	    You("%sを仕掛けるのを再開した。",
		defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
#endif /*JP*/
	    set_occupation(set_trap, occutext, 0);
	    return;
	}
	trapinfo.tobj = otmp;
	trapinfo.tx = u.ux,  trapinfo.ty = u.uy;
	tmp = ACURR(A_DEX);
	trapinfo.time_needed = (tmp > 17) ? 2 : (tmp > 12) ? 3 :
				(tmp > 7) ? 4 : 5;
	if (Blind) trapinfo.time_needed *= 2;
	tmp = ACURR(A_STR);
	if (ttyp == BEAR_TRAP && tmp < 18)
	    trapinfo.time_needed += (tmp > 12) ? 1 : (tmp > 7) ? 2 : 4;
	/*[fumbling and/or confusion and/or cursed object check(s)
	   should be incorporated here instead of in set_trap]*/
#ifdef STEED
	if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
	    boolean chance;

	    if (Fumbling || otmp->cursed) chance = (rnl(10) > 3);
	    else  chance = (rnl(10) > 5);
	    You(E_J("aren't very skilled at reaching from %s.",
		    "%sに乗ったままの作業に習熟していない。"),
		mon_nam(u.usteed));
#ifndef JP
	    Sprintf(buf, "Continue your attempt to set %s?",
		the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
#else
	    Sprintf(buf, "%sを仕掛ける試みを続けますか？",
		defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
#endif /*JP*/
	    if(yn(buf) == 'y') {
		if (chance) {
			switch(ttyp) {
			    case LANDMINE:	/* set it off */
			    	trapinfo.time_needed = 0;
			    	trapinfo.force_bungle = TRUE;
				break;
			    case BEAR_TRAP:	/* drop it without arming it */
				reset_trapset();
#ifndef JP
				You("drop %s!",
			  the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
#else
				You("%sを落とした！",
			defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
#endif /*JP*/
				dropx(otmp);
				return;
			}
		}
	    } else {
	    	reset_trapset();
		return;
	    }
	}
#endif
#ifndef JP
	You("begin setting %s %s.",
	    shk_your(buf, otmp),
	    defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
#else
	You("%sを仕掛けはじめた。",
	    defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
#endif /*JP*/
	set_occupation(set_trap, occutext, 0);
	return;
}

STATIC_PTR
int
set_trap()
{
	struct obj *otmp = trapinfo.tobj;
	struct trap *ttmp;
	int ttyp;

	if (!otmp || !carried(otmp) ||
		u.ux != trapinfo.tx || u.uy != trapinfo.ty) {
	    /* ?? */
	    reset_trapset();
	    return 0;
	}

	if (--trapinfo.time_needed > 0) return 1;	/* still busy */

	ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
	ttmp = maketrap(u.ux, u.uy, ttyp);
	if (ttmp) {
	    ttmp->tseen = 1;
	    ttmp->madeby_u = 1;
	    newsym(u.ux, u.uy); /* if our hero happens to be invisible */
	    if (*in_rooms(u.ux,u.uy,SHOPBASE)) {
		add_damage(u.ux, u.uy, 0L);		/* schedule removal */
	    }
	    if (!trapinfo.force_bungle)
#ifndef JP
		You("finish arming %s.",
			the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
#else
		You("%sを仕掛け終えた。",
			defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
#endif /*JP*/
	    if (((otmp->cursed || Fumbling) && (rnl(10) > 5)) || trapinfo.force_bungle)
		dotrap(ttmp,
			(unsigned)(trapinfo.force_bungle ? FORCEBUNGLE : 0));
	} else {
	    /* this shouldn't happen */
	    Your(E_J("trap setting attempt fails.","罠設置の試みは失敗に終わった。"));
	}
	useup(otmp);
	reset_trapset();
	return 0;
}

STATIC_OVL int
use_whip(obj)
struct obj *obj;
{
    char buf[BUFSZ];
    struct monst *mtmp;
    struct obj *otmp;
    int rx, ry, proficient, res = 0;
    const char *msg_slipsfree = E_J("The bullwhip slips free.","鞭は滑った。");
    const char *msg_snap = E_J("Snap!","ピシッ！");

    if (obj != uwep) {
	if (!wield_tool(obj, E_J("lash","しばく"))) return 0;
	else res = 1;
    }
    if (!getdir((char *)0)) return res;

    if (Stunned || (Confusion && !rn2(5))) confdir();
    rx = u.ux + u.dx;
    ry = u.uy + u.dy;
    mtmp = m_at(rx, ry);

    /* fake some proficiency checks */
    proficient = 0;
    if (Role_if(PM_ARCHEOLOGIST)) ++proficient;
    if (ACURR(A_DEX) < 6) proficient--;
//    else if (ACURR(A_DEX) >= 14) proficient += (ACURR(A_DEX) - 14);
    else if (ACURR(A_DEX) > 14) ++proficient;
    if (P_SKILL(P_WHIP_GROUP) >= P_SKILLED) ++proficient;
    if (P_SKILL(P_WHIP_GROUP) >= P_EXPERT ) ++proficient;
    if (Fumbling) --proficient;
    if (proficient > 3) proficient = 3;
    if (proficient < 0) proficient = 0;

    if (u.uswallow && attack(u.ustuck)) {
	There(E_J("is not enough room to flick your bullwhip.",
		  "鞭を振るうだけの空間がない。"));

    } else if (Underwater) {
#ifndef JP
	There("is too much resistance to flick your bullwhip.");
#else
	pline("ここでは水の抵抗が大きすぎて、鞭を振るえない。");
#endif /*JP*/

    } else if (u.dz < 0) {
	You(E_J("flick a bug off of the %s.",
		"%sの虫をはたき落とした。"),ceiling(u.ux,u.uy));

    } else if ((!u.dx && !u.dy) || (u.dz > 0)) {
	int dam;

#ifdef STEED
	/* Sometimes you hit your steed by mistake */
	if (u.usteed && !rn2(proficient + 2)) {
	    You(E_J("whip %s!","%sを鞭打った！"), mon_nam(u.usteed));
	    kick_steed();
	    return 1;
	}
#endif
	if (Levitation
#ifdef STEED
			|| u.usteed
#endif
		) {
	    /* Have a shot at snaring something on the floor */
	    otmp = level.objects[u.ux][u.uy];
	    if (otmp && otmp->otyp == CORPSE && otmp->corpsenm == PM_HORSE) {
		pline(E_J("Why beat a dead horse?","なぜ死んだ馬を鞭打つのか？"));
		return 1;
	    }
	    if (otmp && proficient) {
#ifndef JP
		You("wrap your bullwhip around %s on the %s.",
		    an(singular(otmp, xname)), surface(u.ux, u.uy));
#else
		You("鞭で%sの%sを巻きつけた。",
		    surface(u.ux, u.uy), singular(otmp, xname));
#endif /*JP*/
		if (rnl(6) || pickup_object(otmp, 1L, TRUE) < 1)
		    pline(msg_slipsfree);
		return 1;
	    }
	}
	dam = rnd(2) + dbon() + obj->spe;
	if (dam <= 0) dam = 1;
	You(E_J("hit your %s with your bullwhip.",
		"自分の%sを鞭で打ってしまった。"), body_part(FOOT));
#ifndef JP
	Sprintf(buf, "killed %sself with %s bullwhip", uhim(), uhis());
	losehp(dam, buf, NO_KILLER_PREFIX);
#else
	losehp(dam, "自分の鞭で自分を打って", KILLED_BY);
#endif /*JP*/
	flags.botl = 1;
	return 1;

    } else if ((Fumbling || Glib) && !rn2(5)) {
	pline_The(E_J("bullwhip slips out of your %s.",
		      "鞭はあなたの%sからすっぽ抜けた。"), body_part(HAND));
	dropx(obj);

    } else if (u.utrap && u.utraptype == TT_PIT) {
	/*
	 *     Assumptions:
	 *
	 *	if you're in a pit
	 *		- you are attempting to get out of the pit
	 *		- or, if you are applying it towards a small
	 *		  monster then it is assumed that you are
	 *		  trying to hit it.
	 *	else if the monster is wielding a weapon
	 *		- you are attempting to disarm a monster
	 *	else
	 *		- you are attempting to hit the monster
	 *
	 *	if you're confused (and thus off the mark)
	 *		- you only end up hitting.
	 *
	 */
	const char *wrapped_what = (char *)0;

	if (mtmp) {
	    if (bigmonst(mtmp->data)) {
		wrapped_what = strcpy(buf, mon_nam(mtmp));
	    } else if (proficient) {
		if (attack(mtmp)) return 1;
		else pline(msg_snap);
	    }
	}
	if (!wrapped_what) {
	    if (IS_FURNITURE(levl[rx][ry].typ))
		wrapped_what = something;
	    else if (sobj_at(BOULDER, rx, ry))
		wrapped_what = E_J("a boulder","大岩");
	}
	if (wrapped_what) {
	    coord cc;

	    cc.x = rx; cc.y = ry;
	    You(E_J("wrap your bullwhip around %s.",
		    "鞭を%sに巻きつけた。"), wrapped_what);
	    if (proficient && rn2(proficient + 2)) {
		if (!mtmp || enexto(&cc, rx, ry, youmonst.data)) {
		    You(E_J("yank yourself out of the pit!",
			    "自分を落し穴から引き上げた！"));
		    teleds(cc.x, cc.y, TRUE);
		    u.utrap = 0;
		    vision_full_recalc = 1;
		}
	    } else {
		pline(msg_slipsfree);
	    }
	    if (mtmp) wakeup(mtmp);
	} else pline(msg_snap);

    } else if (mtmp) {
	if (!canspotmons(mtmp) &&
		!glyph_is_invisible(levl[rx][ry].glyph)) {
	   pline(E_J("A monster is there that you couldn't see.",
		     "そこには目に見えない怪物がいる。"));
	   map_invisible(rx, ry);
	}
	otmp = MON_WEP(mtmp);	/* can be null */
	if (otmp) {
	    char onambuf[BUFSZ];
	    const char *mon_hand;
	    boolean gotit = proficient && (!Fumbling || !rn2(10));

	    Strcpy(onambuf, cxname(otmp));
	    if (gotit) {
		mon_hand = mbodypart(mtmp, HAND);
#ifndef JP
		if (bimanual(otmp)) mon_hand = makeplural(mon_hand);
#endif /*JP*/
	    } else
		mon_hand = 0;	/* lint suppression */

	    You(E_J("wrap your bullwhip around %s %s.",
		    "鞭を%s%sに巻きつけた。"),
		s_suffix(mon_nam(mtmp)), onambuf);
	    if (gotit && otmp->cursed) {
#ifndef JP
		pline("%s welded to %s %s%c",
		      (otmp->quan == 1L) ? "It is" : "They are",
		      mhis(mtmp), mon_hand,
		      !otmp->bknown ? '!' : '.');
#else
		pline("%sは%sの%s%sに貼り付いている%s",
		      onambuf, mon_nam(mtmp),
		      bimanual(otmp) ? "両" : "", mon_hand,
		      !otmp->bknown ? "！" : "。");
#endif /*JP*/
		otmp->bknown = 1;
		gotit = FALSE;	/* can't pull it free */
	    }
	    if (gotit) {
		obj_extract_self(otmp);
		possibly_unwield(mtmp, FALSE);
		setmnotwielded(mtmp,otmp);

		if (!vanish_ether_obj(otmp, (struct monst *)0, E_J("snatch","つかんだ")))
		switch (rn2(proficient + 1)) {
		case 2:
		    /* to floor near you */
		    You(E_J("yank %s %s to the %s!","%s%sを%sに叩き落とした！"),
			s_suffix(mon_nam(mtmp)), onambuf, surface(u.ux, u.uy));
		    place_object(otmp, u.ux, u.uy);
		    stackobj(otmp);
		    break;
		case 3:
		    /* right to you */
#if 0
		    if (!rn2(25)) {
			/* proficient with whip, but maybe not
			   so proficient at catching weapons */
			int hitu, hitvalu;

			hitvalu = 8 + otmp->spe;
			hitu = thitu(hitvalu,
				     dmgval(otmp, &youmonst),
				     otmp, (char *)0);
			if (hitu) {
			    pline_The("%s hits you as you try to snatch it!",
				the(onambuf));
			}
			place_object(otmp, u.ux, u.uy);
			stackobj(otmp);
			break;
		    }
#endif /* 0 */
		    /* right into your inventory */
#ifndef JP
		    You("snatch %s %s!", s_suffix(mon_nam(mtmp)), onambuf);
#else
		    You("%sから%sを奪い取った！", mon_nam(mtmp), onambuf);
#endif /*JP*/
		    if (otmp->otyp == CORPSE &&
			    touch_petrifies(&mons[otmp->corpsenm]) &&
			    !uarmg && !Stone_resistance &&
			    !(poly_when_stoned(youmonst.data) &&
				polymon(PM_STONE_GOLEM))) {
			char kbuf[BUFSZ];

#ifndef JP
			Sprintf(kbuf, "%s corpse",
				an(mons[otmp->corpsenm].mname));
#else
			Sprintf(kbuf, "%sの死体", JMONNAM(otmp->corpsenm));
#endif /*JP*/
			pline(E_J("Snatching %s is a fatal mistake.",
				  "%sをつかみ取るのは致命的な過ちだ。"), kbuf);
#ifdef JP
			Strcat(kbuf, "をキャッチして");
#endif /*JP*/
			instapetrify(kbuf);
		    }
		    otmp = hold_another_object(otmp, E_J("You drop %s!","あなたは%sを落としてしまった！"),
					       doname(otmp), (const char *)0);
		    break;
		default:
		    /* to floor beneath mon */
#ifndef JP
		    You("yank %s from %s %s!", the(onambuf),
			s_suffix(mon_nam(mtmp)), mon_hand);
#else
		    You("%sを%sの%sから叩き落とした！", onambuf,
			mon_nam(mtmp), mon_hand);
#endif /*JP*/
		    obj_no_longer_held(otmp);
		    place_object(otmp, mtmp->mx, mtmp->my);
		    stackobj(otmp);
		    break;
		}
	    } else {
		pline(msg_slipsfree);
	    }
	    wakeup(mtmp);
	} else {
	    if (mtmp->m_ap_type &&
		!Protection_from_shape_changers && !sensemon(mtmp))
		stumble_onto_mimic(mtmp);
	    else You(E_J("flick your bullwhip towards %s.",
			 "%sめがけて鞭を振るった！"), mon_nam(mtmp));
	    if (proficient) {
		if (attack(mtmp)) return 1;
		else pline(msg_snap);
	    }
	}

    } else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
	    /* it must be air -- water checked above */
	    You(E_J("snap your whip through thin air.",
		    "鞭を虚空に向けて振るった。"));

    } else {
	pline(msg_snap);

    }
    return 1;
}


static const char
#ifndef JP
	not_enough_room[] = "There's not enough room here to use that.",
	where_to_hit[] = "Where do you want to hit?",
	cant_see_spot[] = "won't hit anything if you can't see that spot.",
	cant_reach[] = "can't reach that spot from here.";
#else
	not_enough_room[] = "ここにはその武器を振るうのに充分な広さがない。",
	where_to_hit[] = "どこを狙いますか？",
	cant_see_spot[] = "視認できない場所を狙うことはできない。",
	cant_reach[] = "ここからその地点を狙えない。";
#endif /*JP*/

/* Distance attacks by pole-weapons */
STATIC_OVL int
use_pole (obj)
	struct obj *obj;
{
	int res = 0, max_range = 4;
	coord cc;
	struct monst *mtmp;


	/* Are you allowed to use the pole? */
	if (u.uswallow) {
	    pline(not_enough_room);
	    return (0);
	}
	if (obj != uwep) {
	    if (!wield_tool(obj, E_J("swing","振るう"))) return(0);
	    else res = 1;
	}
     /* assert(obj == uwep); */

	/* Prompt for a location */
	pline(where_to_hit);
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, TRUE, E_J("the spot to hit","攻撃地点")) < 0)
	    return 0;	/* user pressed ESC */

	/* Calculate range */
	max_range = weapon_range(uwep);
	if (distu(cc.x, cc.y) > max_range) {
	    pline(E_J("Too far!","遠すぎる！"));
	    return (res);
/*	} else if (!cansee(cc.x, cc.y) &&
		   ((mtmp = m_at(cc.x, cc.y)) == (struct monst *)0 ||
		    !canseemon(mtmp))) {
	    You(cant_see_spot);
	    return (res);*/
	} else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
	    You(cant_reach);
	    return res;
	}

	/* Attack the monster there */
	if ((mtmp = m_at(cc.x, cc.y)) != (struct monst *)0) {
#ifdef MONSTEED
	    mtmp = target_rider_or_steed(&youmonst, mtmp);
#endif /*MONSTEED*/
	    u.dx = cc.x - u.ux;
	    u.dy = cc.y - u.uy;
	    u.dz = 0;
	    (void) attack(mtmp);
	} else
	    /* Now you know that nothing is there... */
	    pline(nothing_happens);
	return (1);
}

STATIC_OVL int
use_cream_pie(obj)
struct obj *obj;
{
	boolean wasblind = Blind;
	boolean wascreamed = u.ucreamed;
	boolean several = FALSE;

	if (obj->quan > 1L) {
		several = TRUE;
		obj = splitobj(obj, 1L);
	}
	if (Hallucination)
		You(E_J("give yourself a facial.",
			"顔のお手入れを始めた。"));
	else
#ifndef JP
		pline("You immerse your %s in %s%s.", body_part(FACE),
			several ? "one of " : "",
			several ? makeplural(the(xname(obj))) : the(xname(obj)));
#else
		You("%sを%sに突っ込んだ。", body_part(FACE), xname(obj));
#endif /*JP*/
	if(can_blnd((struct monst*)0, &youmonst, AT_WEAP, obj)) {
		int blindinc = rnd(25);
		u.ucreamed += blindinc;
		make_blinded(Blinded + (long)blindinc, FALSE);
		if (!Blind || (Blind && wasblind))
#ifndef JP
			pline("There's %ssticky goop all over your %s.",
				wascreamed ? "more " : "",
				body_part(FACE));
#else
			Your("%sじゅうがべとつくクリームで%s覆われた！",
				body_part(FACE), wascreamed ? "さらに" : "");
#endif /*JP*/
		else /* Blind  && !wasblind */
			You_cant(E_J("see through all the sticky goop on your %s.",
				     "%sじゅうべとつくクリームに覆われ、なにも見えない。"),
				body_part(FACE));
	}
	if (obj->unpaid) {
		verbalize(E_J("You used it, you bought it!","使ったんなら、買ってもらうよ！"));
		bill_dummy_object(obj);
	}
	obj_extract_self(obj);
	delobj(obj);
	return(0);
}

STATIC_OVL int
use_grapple (obj)
	struct obj *obj;
{
	int res = 0, typ, max_range = 4, tohit;
	int skill = 0;
	coord cc;
	struct monst *mtmp;
	struct obj *otmp;

	/* Are you allowed to use the hook? */
	if (u.uswallow) {
	    pline(not_enough_room);
	    return (0);
	}
	if (obj != uwep) {
	    if (!wield_tool(obj, E_J("cast","投げる"))) return(0);
	    else res = 1;
	}
     /* assert(obj == uwep); */

	/* Prompt for a location */
	pline(where_to_hit);
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, TRUE, E_J("the spot to hit","目標地点")) < 0)
	    return 0;	/* user pressed ESC */
	u.dx = cc.x - u.ux;
	u.dy = cc.y - u.uy;
	u.dz = 0;

	/* Calculate range */
	max_range = weapon_range(uwep);
	if (distu(cc.x, cc.y) > max_range) {
	    pline(E_J("Too far!","遠すぎる！"));
	    return (res);
	} else if (!cansee(cc.x, cc.y)) {
	    You(cant_see_spot);
	    return (res);
	} else if (!couldsee(cc.x, cc.y)) { /* Eyes of the Overworld */
	    You(cant_reach);
	    return res;
	}

	/* What do you want to hit? */
	tohit = rn2(5);
	typ = weapon_type(uwep);
	if (typ != P_NONE) skill = P_SKILL(typ);
	skill += uwep->spe * 10;
	if (skill >= rn2(50)) {
	    winid tmpwin = create_nhwindow(NHW_MENU);
	    anything any;
	    char buf[BUFSZ];
	    menu_item *selected;

	    any.a_void = 0;	/* set all bits to zero */
	    any.a_int = 1;	/* use index+1 (cant use 0) as identifier */
	    start_menu(tmpwin);
	    any.a_int++;
	    Sprintf(buf, E_J("an object on the %s",
			     "%sの上の品物"), surface(cc.x, cc.y));
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 buf, MENU_UNSELECTED);
	    any.a_int++;
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			E_J("a monster","怪物"), MENU_UNSELECTED);
	    any.a_int++;
#ifndef JP
	    Sprintf(buf, "the %s", surface(cc.x, cc.y));
#endif /*JP*/
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			E_J(buf,surface(cc.x, cc.y)), MENU_UNSELECTED);
	    end_menu(tmpwin, E_J("Aim for what?","何を狙いますか？"));
	    tohit = rn2(4);
	    if (select_menu(tmpwin, PICK_ONE, &selected) > 0)
		tohit = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    destroy_nhwindow(tmpwin);
	}

	/* What did you hit? */
	switch (tohit) {
	case 0:	/* Trap */
	    /* FIXME -- untrap needs to deal with non-adjacent traps */
	    break;
	case 1:	/* Object */
	    if ((otmp = level.objects[cc.x][cc.y]) != 0) {
		You(E_J("snag an object from the %s!",
			"品物を%sから吊り上げた！"), surface(cc.x, cc.y));
		(void) pickup_object(otmp, 1L, FALSE);
		/* If pickup fails, leave it alone */
		newsym(cc.x, cc.y);
		return (1);
	    }
	    break;
	case 2:	/* Monster */
	    if ((mtmp = m_at(cc.x, cc.y)) == (struct monst *)0) break;
	    if (verysmall(mtmp->data) && !rn2(4) &&
			enexto(&cc, u.ux, u.uy, (struct permonst *)0)) {
		You(E_J("pull in %s!","%sを引き寄せた！"), mon_nam(mtmp));
		mtmp->mundetected = 0;
		rloc_to(mtmp, cc.x, cc.y);
		return (1);
//	    } else if ((!bigmonst(mtmp->data) && !strongmonst(mtmp->data)) ||
//		       rn2(4)) {
//		(void) thitmonst(mtmp, uwep);
#ifdef MONSTEED
	    } else if (is_mriding(mtmp) &&
			enexto(&cc, mtmp->mx, mtmp->my, (struct permonst *)0)) {
		if (rn1(16,3) < acurrstr() && rn2(max(skill,20)) > 9) {
		    You(E_J("drag down %s!","%sを引きずり下ろした！"), mon_nam(mtmp));
		    mdismount_steed(mtmp);
		    use_skill(typ, 1);
		} else {
		    You(E_J("tried to pluck down %s, but fails.",
			    "%sを落馬させようとしたが、失敗した。"), mon_nam(mtmp));
		}
		return (1);
#endif /*MONSTEED*/
	    } else {
#ifdef MONSTEED
		mtmp = target_rider_or_steed(&youmonst, mtmp);
#endif /*MONSTEED*/
		(void) attack(mtmp);
		return (1);
	    }
	    /* FALL THROUGH */
	case 3:	/* Surface */
	    if (IS_AIR(levl[cc.x][cc.y].typ) || is_pool(cc.x, cc.y))
		pline_The(E_J("hook slices through the %s.",
			      "鉤は%sを貫いた。"), surface(cc.x, cc.y));
	    else {
		You(E_J("are yanked toward the %s!",
			"%sに向かって引っぱられた！"), surface(cc.x, cc.y));
		hurtle(sgn(cc.x-u.ux), sgn(cc.y-u.uy), 1, FALSE);
		spoteffects(TRUE);
	    }
	    return (1);
	default:	/* Yourself (oops!) */
	    if (skill <= P_BASIC) {
		You(E_J("hook yourself!","自分を吊り上げた！"));
		losehp(rn1(10,10), E_J("a grappling hook","鉤縄にかかって"), KILLED_BY);
		return (1);
	    }
	    break;
	}
	pline(nothing_happens);
	return (1);
}


STATIC_OVL int
use_shield (obj)
	struct obj *obj;
{
	int res = 0;
	int rx, ry;
//	coord cc;
	struct monst *mtmp;
//	struct obj *otmp;

	/* Are you allowed to use the hook? */
	if (u.uswallow) {
	    pline(not_enough_room);
	    return (0);
	}
	if (obj != uarms) {
	    You("are not wearing that.");
	    return(0);
	}

	/* Prompt for a location */
	if (!getdir((char *)0))
	    return 0;	/* user pressed ESC */

	if (Stunned || (Confusion && !rn2(5))) confdir();
	rx = u.ux + u.dx;
	ry = u.uy + u.dy;
	mtmp = m_at(rx, ry);

	pline(nothing_happens);
	return (1);
}

int
getobj_filter_bullet(otmp)
struct obj *otmp;
{
	if (otmp->otyp == BULLET) return GETOBJ_CHOOSEIT;
	return 0;
}

STATIC_OVL int
use_gun (obj)
struct obj *obj;
{
	int res = 0;
	int cnt = 0, gcnt = 0, maxb = 0;
	static const char bullets[] = { ALLOW_COUNT, WEAPON_CLASS, 0 };
	struct obj *otmp;
	struct obj *lblt = 0;
#ifdef JP
	static const struct getobj_words gunw = { 0, 0, "銃に装填する", "銃に装填し" };
#endif /*JP*/

	/* show bullets loaded in the gun */
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
		cnt += otmp->quan;
		gcnt++;
		lblt = otmp;
	}
	if (cnt) {
	    if (gcnt == 1) {
#ifndef JP
		pline("%s %s loaded in your %s.", doname(obj->cobj),
			((obj->cobj)->quan == 1 ? "is" : "are"), xname(obj));
#else
		pline("%sには%sが装填されている。",
			xname(obj), doname(obj->cobj));
#endif /*JP*/
	    } else {
		winid win;
		char buf[BUFSZ];
		/* Create window */
		win = create_nhwindow(NHW_MENU);
		Sprintf(buf, E_J("Bullets loaded in the %s:",
				 "%sに装填されている弾丸:"), xname(obj));
		putstr(win, 0, buf);
		putstr(win, 0, "");

		/* Get the weapon's characteristics */
		for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
		    Sprintf(buf, "  %s", doname(otmp));
		    putstr(win, 0, buf);
		}
		/* Pop up the window and wait for a key */
		display_nhwindow(win, TRUE);
		destroy_nhwindow(win);
	    }
	} else {
		pline(E_J("No bullet is loaded in your %s.",
			  "あなたの%sには弾丸が装填されていない。"), xname(obj));
	}

	maxb = maxbullets(obj);
	if (cnt >= maxb) {
		if (getobj_override)
#ifndef JP
		    pline("No more bullet can be loaded in the gun.");
#else
		    pline("銃にはすでに%s弾が込められている。", maxb > 1 ? "全" : "");
#endif /*JP*/
		return (0);
	}

	/* Get bullets to load */
	otmp = getobj(bullets, E_J("load with",&gunw), getobj_filter_bullet);
	if(!otmp) return 0;
	if (!is_bullet(otmp)) {
		You_cant("load %s with %s.", yname(obj), the(xname(otmp)));
		return (0);
	}

	if (otmp->quan > (long)(maxb - cnt))
		otmp = splitobj(otmp, (long)(maxb - cnt));
	remove_worn_item(otmp, FALSE);
	You(E_J("load your %s with %s.",
		"%sに%sを込めた。"), xname(obj), doname(otmp));

	obj_extract_self(otmp);
	/* merge if possible */
	if (!(lblt && merged(&lblt, &otmp))) {
		otmp->nobj = (struct obj *)0;
		otmp->where = OBJ_CONTAINED;
		otmp->ocontainer = obj;
		if (lblt) lblt->nobj = otmp;
		else obj->cobj = otmp;
	}
	obj->owt = weight(obj);

	return (1);
}
int
getobj_filter_gun(otmp)
struct obj *otmp;
{
	return is_gun(otmp);
}

STATIC_OVL int
use_bullet(obj)
struct obj *obj;
{
	static const char guns[] = { ALLOW_COUNT, WEAPON_CLASS, 0 };
	struct obj *otmp;
#ifdef JP
	static const struct getobj_words gunw = { "どの銃", "に", "装填する", "装填し" };
#endif /*JP*/

	for (otmp = invent; otmp; otmp = otmp->nobj)
		if (is_gun(otmp)) break;

	if (!otmp) {
		You("don't have any guns.", "銃を持っていない。");
		return 0;
	}

	/* Get a gun to load */
	otmp = getobj(guns, E_J("load to",&gunw), getobj_filter_gun);
	if (otmp) {
		getobj_override = obj;
		return use_gun(otmp);
	}
	return 0;
}


#define BY_OBJECT	((struct monst *)0)

/* return 1 if the wand is broken, hence some time elapsed */
STATIC_OVL int
do_break_wand(obj)
    struct obj *obj;
{
    static const char nothing_else_happens[] = E_J("But nothing else happens...",
						   "しかし、何も起こらなかった…。");
    register int i, x, y;
    register struct monst *mon;
    int dmg, damage;
    boolean affects_objects;
    boolean shop_damage = FALSE;
    int expltype = EXPL_MAGICAL;
    char confirm[QBUFSZ], the_wand[BUFSZ], buf[BUFSZ];

    Strcpy(the_wand, yname(obj));
#ifndef JP
    Sprintf(confirm, "Are you really sure you want to break %s?",
	safe_qbuf("", sizeof("Are you really sure you want to break ?"),
				the_wand, ysimple_name(obj), "the wand"));
#else
    Sprintf(confirm, "本当に%sを壊しますか？",
	safe_qbuf("", sizeof("本当にを壊しますか？"),
				the_wand, ysimple_name(obj), "杖"));
#endif /*JP*/
    if (yn(confirm) == 'n' ) return 0;

    if (nohands(youmonst.data)) {
	You_cant(E_J("break %s without hands!",
		     "手がないので、%sを折ることができない！"), the_wand);
	return 0;
    } else if (ACURR(A_STR) < 10) {
	You(E_J("don't have the strength to break %s!",
		"%sをへし折るだけの力がない！"), the_wand);
	return 0;
    }
    pline(E_J("Raising %s high above your %s, you break it in two!",
	      "あなたは%sを%s上高くかかげ、真っ二つにへし折った！"),
	  the_wand, body_part(HEAD));

    /* [ALI] Do this first so that wand is removed from bill. Otherwise,
     * the freeinv() below also hides it from setpaid() which causes problems.
     */
    if (obj->unpaid) {
	check_unpaid(obj);		/* Extra charge for use */
	bill_dummy_object(obj);
    }

    current_wand = obj;		/* destroy_item might reset this */
    freeinv(obj);		/* hide it from destroy_item instead... */
    setnotworn(obj);		/* so we need to do this ourselves */

    if (obj->spe <= 0) {
	pline(nothing_else_happens);
	goto discard_broken_wand;
    }
    obj->ox = u.ux;
    obj->oy = u.uy;
    dmg = obj->spe * 4;
    affects_objects = FALSE;

    switch (obj->otyp) {
    case WAN_WISHING:
    case WAN_NOTHING:
    case WAN_LOCKING:
    case WAN_PROBING:
    case WAN_ENLIGHTENMENT:
    case WAN_OPENING:
    case WAN_SECRET_DOOR_DETECTION:
	pline(nothing_else_happens);
	goto discard_broken_wand;
    case WAN_DEATH:
    case WAN_LIGHTNING:
	dmg *= 4;
	goto wanexpl;
    case WAN_FIRE:
	expltype = EXPL_FIERY;
    case WAN_COLD:
	if (expltype == EXPL_MAGICAL) expltype = EXPL_FROSTY;
	dmg *= 2;
    case WAN_MAGIC_MISSILE:
    wanexpl:
	explode(u.ux, u.uy,
		(obj->otyp - WAN_MAGIC_MISSILE), dmg, WAND_CLASS, expltype);
	makeknown(obj->otyp);	/* explode described the effect */
	goto discard_broken_wand;
    case WAN_STRIKING:
	/* we want this before the explosion instead of at the very end */
	pline(E_J("A wall of force smashes down around you!",
		  "力場の壁があなたの周囲をなぎ倒した！"));
	dmg = d(1 + obj->spe,6);	/* normally 2d12 */
    case WAN_CANCELLATION:
    case WAN_POLYMORPH:
    case WAN_TELEPORTATION:
    case WAN_UNDEAD_TURNING:
	affects_objects = TRUE;
	break;
    default:
	break;
    }

    /* magical explosion and its visual effect occur before specific effects */
    explode(obj->ox, obj->oy, 0, rnd(dmg), WAND_CLASS, EXPL_MAGICAL);

    /* this makes it hit us last, so that we can see the action first */
    for (i = 0; i <= 8; i++) {
	bhitpos.x = x = obj->ox + xdir[i];
	bhitpos.y = y = obj->oy + ydir[i];
	if (!isok(x,y)) continue;

	if (obj->otyp == WAN_DIGGING) {
	    if(dig_check(BY_OBJECT, FALSE, x, y)) {
		if (IS_WALL(levl[x][y].typ) || IS_DOOR(levl[x][y].typ)) {
		    /* normally, pits and holes don't anger guards, but they
		     * do if it's a wall or door that's being dug */
		    watch_dig((struct monst *)0, x, y, TRUE);
		    if (*in_rooms(x,y,SHOPBASE)) shop_damage = TRUE;
		}		    
		digactualhole(x, y, BY_OBJECT,
			      (rn2(obj->spe) < 3 || !Can_dig_down(&u.uz)) ?
			       PIT : HOLE);
	    }
	    continue;
	} else if(obj->otyp == WAN_CREATE_MONSTER) {
	    /* u.ux,u.uy creates it near you--x,y might create it in rock */
	    (void) makemon((struct permonst *)0, u.ux, u.uy, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
	    continue;
	} else {
	    if (x == u.ux && y == u.uy) {
		/* teleport objects first to avoid race with tele control and
		   autopickup.  Other wand/object effects handled after
		   possible wand damage is assessed */
		if (obj->otyp == WAN_TELEPORTATION &&
		    affects_objects && level.objects[x][y]) {
		    (void) bhitpile(obj, bhito, x, y);
		    if (flags.botl) bot();		/* potion effects */
		}
		damage = zapyourself(obj, FALSE);
		if (damage) {
#ifndef JP
		    Sprintf(buf, "killed %sself by breaking a wand", uhim());
		    losehp(damage, buf, NO_KILLER_PREFIX);
#else
		    losehp(damage, "杖を破壊して自爆した", NO_KILLER_PREFIX);
#endif /*JP*/
		}
		if (flags.botl) bot();		/* blindness */
	    } else if ((mon = m_at(x, y)) != 0) {
		(void) bhitm(mon, obj);
	     /* if (flags.botl) bot(); */
	    }
	    if (affects_objects && level.objects[x][y]) {
		(void) bhitpile(obj, bhito, x, y);
		if (flags.botl) bot();		/* potion effects */
	    }
	}
    }

    /* Note: if player fell thru, this call is a no-op.
       Damage is handled in digactualhole in that case */
    if (shop_damage) pay_for_damage(E_J("dig into","店に穴を開けた"), FALSE);

    if (obj->otyp == WAN_LIGHT)
	litroom(TRUE, obj);	/* only needs to be done once */

 discard_broken_wand:
    obj = current_wand;		/* [see dozap() and destroy_item()] */
    current_wand = 0;
    if (obj)
	delobj(obj);
    nomul(0);
    return 1;
}

STATIC_OVL boolean
uhave_graystone()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(is_graystone(otmp))
			return TRUE;
	return FALSE;
}

int
getobj_filter_repair(otmp)
struct obj *otmp;
{
	if ((otmp->oeroded || otmp->oeroded2) &&
	    (otmp->oclass != POTION_CLASS)) /* don't revert diluted potions */
	    return GETOBJ_CHOOSEIT;
	else
	    return 0;
}

int
getobj_filter_useorapply(otmp)
struct obj *otmp;
{
	int type = otmp->otyp;

	switch (otmp->oclass) {
	    case TOOL_CLASS:
	    case WAND_CLASS:
		return GETOBJ_CHOOSEIT;

	    case WEAPON_CLASS:
		if (is_pick(otmp) ||	/* pick-axe */
		    is_ranged(otmp) ||	/* polearms */
		    is_axe(otmp) ||	/* axe */
		    is_gun(otmp) ||	/* firearms */
//		    type == BULLET ||	/* firearms */
		    type == BULLWHIP) return GETOBJ_CHOOSEIT;
		break;
	    case POTION_CLASS:
		/* only applicable potion is oil, and it will only
		   be offered as a choice when already discovered */
		if (type == POT_OIL && otmp->dknown &&
		    objects[POT_OIL].oc_name_known)
			return GETOBJ_CHOOSEIT;
		break;
	    case FOOD_CLASS:
		if (type == CREAM_PIE ||
		    type == EUCALYPTUS_LEAF) return GETOBJ_CHOOSEIT;
		break;
	    case GEM_CLASS:
		if (is_graystone(otmp))
			return GETOBJ_CHOOSEIT;
		break;
	    case ARMOR_CLASS:
		if (type == KITCHEN_APRON)
			return GETOBJ_CHOOSEIT;
		break;

	    default:
		break;
	}
	return 0;
}

int
getobj_filter_tin_opener(otmp)
struct obj *otmp;
{
	if (otmp->otyp == TIN) return GETOBJ_CHOOSEIT;
	return 0;
}
STATIC_OVL int
use_tin_opener (obj)
struct obj *obj;
{
	static const char tins[] = { ALLOW_COUNT, FOOD_CLASS, 0 };
	struct obj *otmp;
	int tincnt;
	char qbuf[BUFSZ], c;
#ifdef JP
	static const struct getobj_words eattinw = { 0, 0, "食べる", "食べ" };
#endif /*JP*/

	/* count tins on the ground */
	for (tincnt = 0, otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
	    tincnt += (otmp->otyp == TIN);

	if(!carrying(TIN) && !tincnt) {
	    You(E_J("have no tin to open.","缶詰を持っていない。"));
	    return 0;
	}

	if (!obj || !wield_tool(obj, E_J("hold","持つ"))) return 0;

	/* Check tins on the ground */
	if (tincnt) {
	    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
		if(otmp->otyp == TIN) {
#ifndef JP
		    Sprintf(qbuf, "There %s %s here; eat %s?",
			    otense(otmp, "are"),
			    doname(otmp),
			    (otmp->quan == 1L) ? "it" : "one");
#else
		    Sprintf(qbuf, "ここには%sがある。%s食べますか？",
			    doname(otmp),
			    (otmp->quan == 1L) ? "" : "一つ");
#endif /*JP*/
		    if((c = yn_function(qbuf,ynqchars,'n')) == 'y') {
			getobj_override = otmp;
			return doeat();
		    } else if(c == 'q')
			return(0);
		}
	    }
	}

	/* Get a tin to eat */
	otmp = getobj(tins, E_J("eat",&eattinw), getobj_filter_tin_opener);
	if (otmp) {
	    getobj_override = otmp;
	    return doeat();
	}
	return 0;
}

int
doapply()
{
	struct obj *obj;
	register int res = 1;
#ifdef JP
	static const struct getobj_words appw = { 0, 0, "使う", "使い" };
#endif /*JP*/

	if(check_capacity((char *)0)) return (0);

#ifndef JP
	obj = getobj(carrying(POT_OIL) || uhave_graystone()
		? tools_too : tools, "use or apply", getobj_filter_useorapply);
#else
	obj = getobj(carrying(POT_OIL) || uhave_graystone()
		? tools_too : tools, &appw, getobj_filter_useorapply);
#endif /*JP*/
	if(!obj) return 0;

	if (obj->oartifact && !touch_artifact(obj, &youmonst))
	    return 1;	/* evading your grasp costs a turn; just be
			   grateful that you don't drop it as well */

	if (obj->oclass == WAND_CLASS)
	    return do_break_wand(obj);

	switch(obj->otyp){
	case BLINDFOLD:
#ifndef MAGIC_GLASSES
	case LENSES:
#else
	case GLASSES_OF_MAGIC_READING:
	case GLASSES_OF_GAZE_PROTECTION:
	case GLASSES_OF_INFRAVISION:
	case GLASSES_VERSUS_FLASH:
	case GLASSES_OF_SEE_INVISIBLE:
	case GLASSES_OF_PHANTASMAGORIA:
#endif /*MAGIC_GLASSES*/
		if (obj == ublindf) {
		    if (!cursed(obj)) Blindf_off(obj);
		} else if (!ublindf)
		    Blindf_on(obj);
		else You(E_J("are already %s.","すでに%sている。"),
			ublindf->otyp == TOWEL ?     E_J("covered by a towel","頭にタオルを巻い") :
			ublindf->otyp == BLINDFOLD ? E_J("wearing a blindfold","目隠しをし") :
#ifndef MAGIC_GLASSES
						     E_J("wearing lenses","レンズをつけ"));
#else
						     E_J("wearing glasses","眼鏡をかけ"));
#endif
		break;
	case CREAM_PIE:
		res = use_cream_pie(obj);
		break;
	case BULLWHIP:
		res = use_whip(obj);
		break;
	case GRAPPLING_HOOK:
		res = use_grapple(obj);
		break;
	case LARGE_BOX:
	case CHEST:
	case ICE_BOX:
	case SACK:
	case BAG_OF_HOLDING:
	case OILSKIN_SACK:
		res = use_container(obj, 1);
		break;
	case BAG_OF_TRICKS:
		bagotricks(obj);
		break;
	case CAN_OF_GREASE:
		use_grease(obj);
		break;
	case LOCK_PICK:
#ifdef TOURIST
	case CREDIT_CARD:
#endif
	case SKELETON_KEY:
		(void) pick_lock(obj);
		break;
	case PICK_AXE:
	case DWARVISH_MATTOCK:
		res = use_pick_axe(obj);
		break;
	case TINNING_KIT:
		use_tinning_kit(obj);
		break;
	case LEASH:
		use_leash(obj);
		break;
#ifdef STEED
	case SADDLE:
		res = use_saddle(obj);
		break;
#endif
	case MAGIC_WHISTLE:
		use_magic_whistle(obj);
		break;
	case TIN_WHISTLE:
		use_whistle(obj);
		break;
	case EUCALYPTUS_LEAF:
		/* MRKR: Every Australian knows that a gum leaf makes an */
		/*	 excellent whistle, especially if your pet is a  */
		/*	 tame kangaroo named Skippy.			 */
		if (obj->blessed) {
		    use_magic_whistle(obj);
		    /* sometimes the blessing will be worn off */
		    if (!rn2(5/*49*/)) {
			if (!Blind) {
			    char buf[BUFSZ];

#ifndef JP
			    pline("%s %s %s.", Shk_Your(buf, obj),
				  aobjnam(obj, "glow"), hcolor("brown"));
#else
			    pline("%sは%s輝いた。",
				  xname(obj), j_no_ni(hcolor("褐色の")));
#endif /*JP*/
			    obj->bknown = 1;
			}
			unbless(obj);
		    }
		} else {
		    use_whistle(obj);
		}
		break;
	case STETHOSCOPE:
		res = use_stethoscope(obj);
		break;
	case MIRROR:
		res = use_mirror(obj);
		break;
	case BELL:
	case BELL_OF_OPENING:
		use_bell(&obj);
		break;
	case CANDELABRUM_OF_INVOCATION:
		use_candelabrum(obj);
		break;
	case WAX_CANDLE:
	case TALLOW_CANDLE:
	case MAGIC_CANDLE:
		use_candle(&obj);
		break;
	case OIL_LAMP:
	case MAGIC_LAMP:
	case BRASS_LANTERN:
		use_lamp(obj);
		break;
	case POT_OIL:
		light_cocktail(obj);
		break;
#ifdef TOURIST
	case EXPENSIVE_CAMERA:
		res = use_camera(obj);
		break;
#endif
	case KITCHEN_APRON:
		if(!freehand()) {
			You(E_J("have no free %s!",
				"%sが空いていない！"), body_part(HAND));
		} else if (Glib) {
			Glib = 0;
#ifndef JP
			You("wipe off your %s.", makeplural(body_part(HAND)));
#else
			You("%sを拭いた。", body_part(HAND));
#endif /*JP*/
			return 1;
		} else {
#ifndef JP
			Your("%s are already clean.", makeplural(body_part(HAND)));
#else
			Your("%sは汚れていない。", body_part(HAND));
#endif /*JP*/
		}
		break;
	case TOWEL:
		res = use_towel(obj);
		break;
	case CRYSTAL_BALL:
		use_crystal_ball(obj);
		break;
	case ORB_OF_MAINTENANCE:
	    if(obj->spe > 0) {
#ifdef JP
		static const struct getobj_words omw = { 0, 0, "修理する", "修理し" };
#endif /*JP*/
		struct obj *otmp;
		boolean fsav;

		consume_obj_charge(obj, TRUE);

		if (obj->blessed) {
		    int nohap =1;
		    for(otmp = invent; otmp; otmp = otmp->nobj) {
			if (otmp->oeroded || otmp->oeroded2) {
			    otmp->oeroded = otmp->oeroded2 = 0;
#ifndef JP
			    Your("%s %s as good as new!",
				     xname(otmp),
				     otense(otmp, Blind ? "feel" : "look"));
#else
			    Your("%sは新品同様になった%s！",
				     xname(otmp), Blind ? "ようだ" : "");
#endif /*JP*/
			    nohap = 0;
			}
		    }
		    if (nohap) pline(nothing_happens);
		    break;
		}

		fsav = flags.verbose;
		flags.verbose = 0;
		otmp = getobj(all_count, E_J("repair",&omw), getobj_filter_repair);
		flags.verbose = fsav;
		if (!otmp) {
		    if (!Blind)
#ifndef JP
			pline("%s glows and fades.", The(xname(obj)));
#else
			pline("%sは一瞬輝いたが、光は消えた。", xname(obj));
#endif /*JP*/
		    break;
		}
		if (!is_damageable(otmp) ||
		    otmp->oclass == FOOD_CLASS || otmp->oclass == POTION_CLASS ||
		    otmp->oclass == SCROLL_CLASS || otmp->oclass == SPBOOK_CLASS ||
		    otmp->oclass == GEM_CLASS || otmp->oclass == ROCK_CLASS) {
		    pline(nothing_happens);
		    break;
		}
		if (obj->cursed) {
		    int nohap = 0;
		    if (!rn2(2)) {
			if (otmp->oeroded < MAX_ERODE &&
			    (is_rustprone(otmp) || is_flammable(otmp))) {
			    otmp->oeroded++;
#ifndef JP
			    Your("%s %s!", xname(otmp),
				 otense(otmp, is_rustprone(otmp) ? "rust" : "smoulder"));
#else
			    Your("%sは%sた！", xname(otmp), is_rustprone(otmp) ? "錆び" : "焦げ");
#endif /*JP*/
			} else nohap = 1;
		    } else {
			if (otmp->oeroded2 < MAX_ERODE &&
			    (is_corrodeable(otmp) || is_rottable(otmp))) {
			    otmp->oeroded2++;
#ifndef JP
			    Your("%s %s!", xname(otmp),
				 otense(otmp, is_corrodeable(otmp) ? "corrode", : "rot"));
#else
			    Your("%sは腐%sた！", xname(otmp), is_corrodeable(otmp) ? "食し" : "っ");
#endif /*JP*/
			} else nohap = 1;
		    }
		    if (nohap) pline(nothing_happens);
		} else if (otmp->oeroded || otmp->oeroded2) {
		    otmp->oeroded = otmp->oeroded2 = 0;
#ifndef JP
		    Your("%s %s as good as new!",
			     xname(otmp),
			     otense(otmp, Blind ? "feel" : "look"));
#else
		    Your("%sは新品同様になった%s！",
			     xname(otmp), Blind ? "ようだ" : "");
#endif /*JP*/
		} else pline(nothing_happens);
	    } else pline(E_J("This orb is burnt out.","オーブの魔力は尽きている。"));
	    break;
/* STEPHEN WHITE'S NEW CODE */
	case ORB_OF_CHARGING:
		if(obj->spe > 0) {
			struct obj *otmp;
			consume_obj_charge(obj, TRUE);
			makeknown(ORB_OF_CHARGING);
			otmp = getchargableobj();
			if (!otmp) break;
			recharge(otmp, obj->cursed ? -1 : (obj->blessed ? 1 : 0));
		} else pline(E_J("This orb is burnt out.","オーブの魔力は尽きている。"));
		break;
	case ORB_OF_DESTRUCTION:
		useup(obj);
		pline(E_J("As you activate the orb, it explodes!",
			  "力を引き出そうとしたとたん、オーブは爆発した！"));
		explode(u.ux, u.uy, 11, d(12,6), WAND_CLASS, EXPL_FIERY);
		check_unpaid(obj);
		break;
	case MAGIC_MARKER:
		res = dowrite(obj);
		break;
	case TIN_OPENER:
		res = use_tin_opener(obj);
		break;
	case FIGURINE:
		use_figurine(&obj);
		break;
	case UNICORN_HORN:
		use_unicorn_horn(obj);
		break;
#ifdef FIRSTAID
	case SCISSORS:
		res = use_scissors(obj);
		break;
	case BANDAGE:
		if ((Upolyd && u.mh < u.mhmax) || (!Upolyd && u.uhp < u.uhpmax)) {
			int tmp;
			tmp = rn1(1 + 500/((int)(ACURR(A_DEX) + ACURR(A_INT))), 10);
			if (Role_if(PM_HEALER) ||
			    (flags.female && uarm && uarm->otyp == NURSE_UNIFORM))
				tmp /= 3;
			if (flags.female && uarmh && uarmh->otyp == NURSE_CAP) tmp /= 2;
			bandage.reqtime = tmp;
			bandage.usedtime = 0;
			bandage.bandage = obj;
			set_occupation(putbandage, E_J("putting the bandage","包帯を巻くの"), 0);
			You(E_J("start putting a bandage on your wounds.",
				"傷に包帯を巻きはじめた。"));
		} else {
			You(E_J("are not wounded.","怪我をしていない。"));
			res = 0;
		}
		break;
#endif /*FIRSTAID*/
	case WOODEN_FLUTE:
	case MAGIC_FLUTE:
	case TOOLED_HORN:
	case FROST_HORN:
	case FIRE_HORN:
	case WOODEN_HARP:
	case MAGIC_HARP:
	case BUGLE:
	case LEATHER_DRUM:
	case DRUM_OF_EARTHQUAKE:
		res = do_play_instrument(obj);
		break;
	case HORN_OF_PLENTY:	/* not a musical instrument */
		if (obj->spe > 0) {
		    struct obj *otmp;
		    const char *what;

		    consume_obj_charge(obj, TRUE);
		    if (!rn2(13)) {
			otmp = mkobj(POTION_CLASS, FALSE);
			if (objects[otmp->otyp].oc_magic) do {
			    otmp->otyp = rnd_class(POT_BOOZE, POT_WATER);
			} while (otmp->otyp == POT_SICKNESS);
			what = E_J("A potion","薬");
		    } else {
			otmp = mkobj(FOOD_CLASS, FALSE);
			if (otmp->otyp == FOOD_RATION && !rn2(7))
			    otmp->otyp = LUMP_OF_ROYAL_JELLY;
			what = E_J("Some food","食べ物");
		    }
		    pline(E_J("%s spills out.",
			      "%sが出てきた。"), what);
		    otmp->blessed = obj->blessed;
		    otmp->cursed = obj->cursed;
		    otmp->owt = weight(otmp);
#ifndef JP
		    otmp = hold_another_object(otmp, u.uswallow ?
					"Oops!  %s out of your reach!" :
					(Is_airlevel(&u.uz) ||
					 Is_waterlevel(&u.uz) ||
					 levl[u.ux][u.uy].typ < IRONBARS ||
					 levl[u.ux][u.uy].typ >= ICE) ?
					       "Oops!  %s away from you!" :
					       "Oops!  %s to the floor!",
					       The(aobjnam(otmp, "slip")),
					       (const char *)0);
#else
		    otmp = hold_another_object(otmp, u.uswallow ?
				"しまった！ %sを取りそこねた！" :
					(Is_airlevel(&u.uz) ||
					 Is_waterlevel(&u.uz) ||
					 levl[u.ux][u.uy].typ < IRONBARS ||
					 levl[u.ux][u.uy].typ >= ICE) ?
				"しまった！ %sはあなたを離れて漂っていった！" :
				"しまった！ %sを床に落とした！",
					       xname(otmp), (const char *)0);
#endif /*JP*/
		    makeknown(HORN_OF_PLENTY);
		} else
		    pline(nothing_happens);
		break;
	case LAND_MINE:
	case BEARTRAP:
		use_trap(obj);
		break;
	case FLINT:
	case LUCKSTONE:
	case LOADSTONE:
	case TOUCHSTONE:
		use_stone(obj);
		break;
	case BULLET:
		res = use_bullet(obj);
		break;
	default:
		/* Pole-weapons can strike at a distance */
		if (is_ranged(obj)) {
			res = use_pole(obj);
			break;
		} else if (is_pick(obj) || is_axe(obj)) {
			res = use_pick_axe(obj);
			break;
		} else if (is_shield(obj)) {
			res = use_shield(obj);
			break;
		} else if (is_gun(obj)) {
			res = use_gun(obj);
			break;
		} else if (obj->oclass == POTION_CLASS) {
			pline(silly_thing_to, E_J("use or apply", "使う"));
			goto xit;
		}
		pline(E_J("Sorry, I don't know how to use that.",
			  "すみません、それをどう使えばいいのかわかりません。"));
	xit:
		nomul(0);
		return 0;
	}
	if (res && obj && obj->oartifact) arti_speak(obj);
	nomul(0);
	return res;
}

/* Keep track of unfixable troubles for purposes of messages saying you feel
 * great.
 */
int
unfixable_trouble_count(is_horn)
	boolean is_horn;
{
	int unfixable_trbl = 0;

	if (Stoned) unfixable_trbl++;
	if (Strangled) unfixable_trbl++;
	if (Wounded_legs
#ifdef STEED
		    && !u.usteed
#endif
				) unfixable_trbl++;
	if (Slimed) unfixable_trbl++;
	/* lycanthropy is not desirable, but it doesn't actually make you feel
	   bad */

	/* we'll assume that intrinsic stunning from being a bat/stalker
	   doesn't make you feel bad */
	if (!is_horn) {
	    if (Confusion) unfixable_trbl++;
	    if (Sick) unfixable_trbl++;
	    if (HHallucination) unfixable_trbl++;
	    if (Vomiting) unfixable_trbl++;
	    if (HStun) unfixable_trbl++;
	}
	return unfixable_trbl;
}

#endif /* OVLB */

/*apply.c*/
