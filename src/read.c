/*	SCCS Id: @(#)read.c	3.4	2003/10/22	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* KMH -- Copied from pray.c; this really belongs in a header file */
#define DEVOUT 14
#define STRIDENT 4

#define Your_Own_Role(mndx) \
	((mndx) == urole.malenum || \
	 (urole.femalenum != NON_PM && (mndx) == urole.femalenum))
#define Your_Own_Race(mndx) \
	((mndx) == urace.malenum || \
	 (urace.femalenum != NON_PM && (mndx) == urace.femalenum))

#ifdef OVLB

boolean	known;

static NEARDATA const char readable[] =
		   { ALL_CLASSES, SCROLL_CLASS, SPBOOK_CLASS, 0 };
//static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };

static void FDECL(wand_explode, (struct obj *));
static void NDECL(do_class_genocide);
static void FDECL(stripspe,(struct obj *));
static void FDECL(p_glow1,(struct obj *));
static void FDECL(p_glow2,(struct obj *,const char *));
static void FDECL(randomize,(int *, int));
static void FDECL(forget_single_object, (int));
static void FDECL(forget, (int));
static void FDECL(maybe_tame, (struct monst *,struct obj *));
static int FDECL(getobj_filter_read, (struct obj *));
static int FDECL(getobj_filter_charge, (struct obj *));

STATIC_PTR void FDECL(set_lit, (int,int,genericptr_t));

int
getobj_filter_read(otmp)
struct obj *otmp;
{
	if (otmp->oclass == SCROLL_CLASS ||
	    otmp->oclass == SPBOOK_CLASS)
		return GETOBJ_CHOOSEIT;
	if (otmp->otyp == FORTUNE_COOKIE
#ifdef TOURIST
	    || otmp->otyp == T_SHIRT
#endif
	   ) return GETOBJ_ALLOWALL;
	return 0;
}

int
doread()
{
	register struct obj *scroll;
	register boolean confused;
#ifdef JP
	static const struct getobj_words readw = { 0, 0, "読む", "読み" };
#endif /*JP*/

	known = FALSE;
	if(check_capacity((char *)0)) return (0);

	if (IS_GRAVE(levl[u.ux][u.uy].typ))
	    if (yn(E_J("Do you want to read an epitaph on the grave?",
		       "墓碑の銘を読みますか？")) == 'y') {
		read_engr_at(u.ux, u.uy);
		return (0);
	    }

	scroll = getobj(readable, E_J("read",&readw), getobj_filter_read);
	if(!scroll) return(0);

	/* outrumor has its own blindness check */
	if(scroll->otyp == FORTUNE_COOKIE) {
	    if(flags.verbose)
		You(E_J("break up the cookie and throw away the pieces.",
			"クッキーを割り、かけらを投げ捨てた。"));
	    outrumor(bcsign(scroll), BY_COOKIE);
	    if (!Blind) u.uconduct.literate++;
	    useup(scroll);
	    return(1);
#ifdef TOURIST
	} else if (scroll->otyp == T_SHIRT) {
	    static const char *shirt_msgs[] = { /* Scott Bigham */
#ifndef JP
    "I explored the Dungeons of Doom and all I got was this lousy T-shirt!",
    "Is that Mjollnir in your pocket or are you just happy to see me?",
    "It's not the size of your sword, it's how #enhance'd you are with it.",
    "Madame Elvira's House O' Succubi Lifetime Customer",
    "Madame Elvira's House O' Succubi Employee of the Month",
    "Ludios Vault Guards Do It In Small, Dark Rooms",
    "Yendor Military Soldiers Do It In Large Groups",
    "I survived Yendor Military Boot Camp",
    "Ludios Accounting School Intra-Mural Lacrosse Team",
    "Oracle(TM) Fountains 10th Annual Wet T-Shirt Contest",
    "Hey, black dragon!  Disintegrate THIS!",
    "I'm With Stupid -->",
    "Don't blame me, I voted for Izchak!",
    "Don't Panic",				/* HHGTTG */
    "Furinkan High School Athletic Dept.",	/* Ranma 1/2 */
    "Hel-LOOO, Nurse!",			/* Animaniacs */
#else
    /* 日本ではTシャツに文章を書くという文化がないので、わかりにくいものは適当に改変 */
    "運命の大迷宮をさんざん探検したけどこんな嫌なTシャツしか手に入らなかったよ！",
    "ポケットにミョルニルを入れてるの？ それとも私に会えて嬉しい？",
    "剣の大きさが問題なのではない．それが如何に#enhanceされているかなのだ。",
    "マダム・エルヴァイラのサキュバスの館　生涯顧客",
    "マダム・エルヴァイラのサキュバスの館　今月の功労賞",
    "ローディオス金庫警備保障",			/* 置き換え */
    "イェンダー軍第三師団大迷宮駐屯地",		/* 置き換え */
    "イェンダー軍新兵訓練修了記念",
    "ローディオス会計学校 壁打ちラクロスチーム",
    "オラクル(TM)の泉　第10回濡れTシャツコンテスト",
    "おい、黒ドラゴン！　こいつを分解してみろ！",
    "バカといます →",
    "私のせいじゃない！ Izchakに投票したもの！",
    "あわてるな",				/* HHGTTG */
    "風林館高校陸上部",				/* Ranma 1/2 */
    "一番",	/* 置き換え: 有名な柄 */
    "今やヒッコリーが主流のワークウエアの定番",	/* アジアのいかしたTシャツ */
#endif /*JP*/
	    };
	    char buf[BUFSZ];
	    int erosion;

	    if (Blind) {
#ifndef JP
		You_cant("feel any Braille writing.");
#else
		pline("これには点字は使われていないようだ。");
#endif /*JP*/
		return 0;
	    }
	    u.uconduct.literate++;
	    if(flags.verbose)
		pline(E_J("It reads:","こう書いてある:"));
	    Strcpy(buf, shirt_msgs[scroll->o_id % SIZE(shirt_msgs)]);
	    erosion = greatest_erosion(scroll);
	    if (erosion)
		wipeout_text(buf,
			(int)(strlen(buf) * erosion / (2*MAX_ERODE)),
			     scroll->o_id ^ (unsigned)u.ubirthday);
	    pline(E_J("\"%s\"","『%s』"), buf);
	    return 1;
#endif	/* TOURIST */
	} else if (scroll->oclass != SCROLL_CLASS
		&& scroll->oclass != SPBOOK_CLASS) {
	    pline(silly_thing_to, E_J("read","読む"));
	    return(0);
	} else if (Blind) {
	    const char *what = 0;
	    if (scroll->oclass == SPBOOK_CLASS)
		what = E_J("mystic runes","神秘のルーン");
	    else if (!scroll->dknown)
		what = E_J("formula on the scroll","巻物に記された術式");
	    if (what) {
		pline(E_J("Being blind, you cannot read the %s.",
			  "目が見えなくては、あなたは%sを読むことができない。"), what);
		return(0);
	    }
	}

	/* Actions required to win the game aren't counted towards conduct */
	if (scroll->otyp != SPE_BOOK_OF_THE_DEAD &&
		scroll->otyp != SPE_BLANK_PAPER &&
		scroll->otyp != SCR_BLANK_PAPER)
	    u.uconduct.literate++;

	confused = (Confusion != 0);
#ifdef MAIL
	if (scroll->otyp == SCR_MAIL) confused = FALSE;
#endif
	if(scroll->oclass == SPBOOK_CLASS) {
	    return(study_book(scroll));
	}
	scroll->in_use = TRUE;	/* scroll, not spellbook, now being read */
	if(scroll->otyp != SCR_BLANK_PAPER) {
	  if(Blind)
#ifndef JP
	    pline("As you %s the formula on it, the scroll disappears.",
			is_silent(youmonst.data) ? "cogitate" : "pronounce");
#else
	    pline("記された術式を%sると、巻物は消え去った。",
			is_silent(youmonst.data) ? "念じ" : "唱え");
#endif /*JP*/
	  else
	    pline(E_J("As you read the scroll, it disappears.",
		      "読み上げると、巻物は消え去った。"));
	  if(confused) {
	    if (Hallucination)
		pline(E_J("Being so trippy, you screw up...",
			  "あまりにもラリっていたため、あなたはメッチャクチャにやってしまい…。"));
	    else
		pline(E_J("Being confused, you mis%s the magic words...",
			  "混乱のあまり、あなたは魔法の呪文を%sしまい…。"),
			is_silent(youmonst.data) ? E_J("understand","誤読して") : E_J("pronounce","とちって"));
	  }
	}
	if(!seffects(scroll))  {
		if(!objects[scroll->otyp].oc_name_known) {
		    if(known) {
			makeknown(scroll->otyp);
			more_experienced(0,10);
		    } else if(!objects[scroll->otyp].oc_uname)
			docall(scroll);
		}
		if(scroll->otyp != SCR_BLANK_PAPER)
			useup(scroll);
		else scroll->in_use = FALSE;
	}
	return(1);
}

static void
stripspe(obj)
register struct obj *obj;
{
	if (obj->blessed) pline(nothing_happens);
	else {
		if (obj->spe > 0) {
		    obj->spe = 0;
		    if (obj->otyp == OIL_LAMP || obj->otyp == BRASS_LANTERN)
			obj->age = 0;
#ifndef JP
		    Your("%s %s briefly.",xname(obj), otense(obj, "vibrate"));
#else
		    Your("%sは短く震えた。",xname(obj));
#endif /*JP*/
		} else pline(nothing_happens);
	}
}

static void
p_glow1(otmp)
register struct obj	*otmp;
{
#ifndef JP
	Your("%s %s briefly.", xname(otmp),
	     otense(otmp, Blind ? "vibrate" : "glow"));
#else
	Your("%sは%s。", xname(otmp),
	     Blind ? "短く震えた" : "ほのかに輝いた");
#endif /*JP*/
}

static void
p_glow2(otmp,color)
register struct obj	*otmp;
register const char *color;
{
#ifndef JP
	Your("%s %s%s%s for a moment.",
		xname(otmp),
		otense(otmp, Blind ? "vibrate" : "glow"),
		Blind ? "" : " ",
		Blind ? nul : hcolor(color));
#else
	Your("%sは少しの間%s%s。",
		xname(otmp),
		Blind ? nul : j_no_ni(hcolor(color)),
		Blind ? "震えた" : "輝いた");
#endif /*JP*/
}

/* Is the object chargeable?  For purposes of inventory display; it is */
/* possible to be able to charge things for which this returns FALSE. */
boolean
is_chargeable(obj)
struct obj *obj;
{
	if (obj->oclass == WAND_CLASS) return TRUE;
	/* known && !uname is possible after amnesia/mind flayer */
	if (obj->oclass == RING_CLASS)
	    return (boolean)(objects[obj->otyp].oc_charged &&
			(obj->known || objects[obj->otyp].oc_uname));
	if (is_weptool(obj))	/* specific check before general tools */
	    return FALSE;
	if (obj->oclass == TOOL_CLASS)
	    return (boolean)(objects[obj->otyp].oc_charged || obj->otyp == BRASS_LANTERN);
	return FALSE; /* why are weapons/armor considered charged anyway? */
}

/*
 * recharge an object; curse_bless is -1 if the recharging implement
 * was cursed, +1 if blessed, 0 otherwise.
 */
void
recharge(obj, curse_bless)
struct obj *obj;
int curse_bless;
{
	register int n;
	boolean is_cursed, is_blessed;

	is_cursed = curse_bless < 0;
	is_blessed = curse_bless > 0;

	if (obj->oclass == WAND_CLASS) {
	    /* undo any prior cancellation, even when is_cursed */
	    if (obj->spe == -1) obj->spe = 0;

	    /*
	     * Recharging might cause wands to explode.
	     *	v = number of previous recharges
	     *	      v = percentage chance to explode on this attempt
	     *		      v = cumulative odds for exploding
	     *	0 :   0       0
	     *	1 :   0.29    0.29
	     *	2 :   2.33    2.62
	     *	3 :   7.87   10.28
	     *	4 :  18.66   27.02
	     *	5 :  36.44   53.62
	     *	6 :  62.97   82.83
	     *	7 : 100     100
	     */
	    n = (int)obj->recharged;
	    if (n > 0 && (obj->otyp == WAN_WISHING ||
		    (n * n * n > rn2(7*7*7)))) {	/* recharge_limit */
		wand_explode(obj);
		return;
	    }
	    /* didn't explode, so increment the recharge count */
	    obj->recharged = (unsigned)(n + 1);

	    /* now handle the actual recharging */
	    if (is_cursed) {
		stripspe(obj);
	    } else {
		int lim = (obj->otyp == WAN_WISHING) ? 3 :
			(objects[obj->otyp].oc_dir != NODIR) ? 8 : 15;

		n = (lim == 3) ? 3 : rn1(5, lim + 1 - 5);
		if (!is_blessed) n = rnd(n);

		if (obj->spe < n) obj->spe = n;
		else obj->spe++;
		if (obj->otyp == WAN_WISHING && obj->spe > 3) {
		    wand_explode(obj);
		    return;
		}
		if (obj->spe >= lim) p_glow2(obj, NH_BLUE);
		else p_glow1(obj);
	    }

	} else if (obj->oclass == RING_CLASS &&
					objects[obj->otyp].oc_charged) {
	    /* charging does not affect ring's curse/bless status */
	    int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
	    boolean is_on = (obj == uleft || obj == uright);
	    int explodds[6] = { 5, 10, 33, 66, 85, 99 }; /* +1 to +6 */

	    /* destruction depends on current state, not adjustment */
	    if ((obj->spe <= -5) || (obj->spe >= 7) ||
		((obj->spe > 0) && (explodds[obj->spe - 1] > rn2(100)))) {
#ifndef JP
		Your("%s %s momentarily, then %s!",
		     xname(obj), otense(obj,"pulsate"), otense(obj,"explode"));
#else
		Your("%sが脈動したかと思うと、突然爆発した！", xname(obj));
#endif /*JP*/
		if (is_on) Ring_gone(obj);
		s = rnd(3 * abs(obj->spe));	/* amount of damage */
		useup(obj);
		losehp(s, E_J("exploding ring","指輪の爆発で"), KILLED_BY_AN);
	    } else {
		long mask = is_on ? (obj == uleft ? LEFT_RING :
				     RIGHT_RING) : 0L;
		Your(E_J("%s spins %sclockwise for a moment.","%sは一瞬%s時計回りに回転した。"),
		     xname(obj), s < 0 ? E_J("counter","反") : "");
		/* cause attributes and/or properties to be updated */
		if (is_on) Ring_off(obj);
		obj->spe += s;	/* update the ring while it's off */
		if (obj->spe > 7) obj->spe = 7;
		if (is_on) setworn(obj, mask), Ring_on(obj);
		/* oartifact: if a touch-sensitive artifact ring is
		   ever created the above will need to be revised  */
	    }

	} else if (obj->oclass == TOOL_CLASS) {
	    int rechrg = (int)obj->recharged;

	    if (objects[obj->otyp].oc_charged) {
		/* tools don't have a limit, but the counter used does */
		if (rechrg < 7)	/* recharge_limit */
		    obj->recharged++;
	    }
	    switch(obj->otyp) {
	    case BELL_OF_OPENING:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) obj->spe += rnd(3);
		else obj->spe += 1;
		if (obj->spe > 5) obj->spe = 5;
		break;
	    case MAGIC_MARKER:
	    case TINNING_KIT:
	    case MAGIC_WHISTLE:
#ifdef TOURIST
	    case EXPENSIVE_CAMERA:
#endif
		if (is_cursed) stripspe(obj);
		else if (rechrg && obj->otyp == MAGIC_MARKER) {	/* previously recharged */
		    obj->recharged = 1;	/* override increment done above */
		    if (obj->spe < 3)
			Your(E_J("marker seems permanently dried out.",
				 "マーカは二度と使えないほど乾ききっているようだ。"));
		    else
			pline(nothing_happens);
		} else if (is_blessed) {
		    n = rn1(16,15);		/* 15..30 */
		    if (obj->spe + n <= 50)
			obj->spe = 50;
		    else if (obj->spe + n <= 75)
			obj->spe = 75;
		    else {
			int chrg = (int)obj->spe;
			if ((chrg + n) > 127)
				obj->spe = 127;
			else
				obj->spe += n;
		    }
		    p_glow2(obj, NH_BLUE);
		} else {
		    n = rn1(11,10);		/* 10..20 */
		    if (obj->spe + n <= 50)
			obj->spe = 50;
		    else {
			int chrg = (int)obj->spe;
			if ((chrg + n) > 127)
				obj->spe = 127;
			else
				obj->spe += n;
		    }
		    p_glow2(obj, NH_WHITE);
		}
		break;
	    case OIL_LAMP:
	    case BRASS_LANTERN:
		if (is_cursed) {
		    stripspe(obj);
		    if (obj->lamplit) {
			if (!Blind)
#ifndef JP
			    pline("%s out!", Tobjnam(obj, "go"));
#else
			    pline("%sは消えた！", xname(obj));
#endif /*JP*/
			end_burn(obj, TRUE);
		    }
		} else if (is_blessed) {
		    obj->spe = 1;
		    obj->age = 1500;
		    p_glow2(obj, NH_BLUE);
		} else {
		    obj->spe = 1;
		    obj->age += 750;
		    if (obj->age > 1500) obj->age = 1500;
		    p_glow1(obj);
		}
		break;
	    case CRYSTAL_BALL:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    obj->spe = 6;
		    p_glow2(obj, NH_BLUE);
		} else {
		    if (obj->spe < 5) {
			obj->spe++;
			p_glow1(obj);
		    } else pline(nothing_happens);
		}
		break;
	    case ORB_OF_MAINTENANCE:
		n = (int)obj->recharged - 1;
		if (n * n * n < rn2(7*7*7)) { /* same as wands */
		    if (is_cursed) {
			stripspe(obj);
		    } else {
			if (!is_blessed) n = rnd(7);
			else n = rn1(4, 4);
			if (obj->spe < n) {
			    obj->spe = n;
			    p_glow1(obj);
			} else {
			    obj->spe++;
			    p_glow2(obj, NH_BLUE);
			}
		    }
		    break;
		} /* else fallthru */
	    case ORB_OF_CHARGING:
		Your(E_J("%s cracks, and explodes!","%sはひび割れ、爆発した！"), xname(obj));
		losehp(d(4,8), E_J("exploding orb","オーブの爆発で"), KILLED_BY_AN);
		useup(obj);
		break;
	    case HORN_OF_PLENTY:
	    case BAG_OF_TRICKS:
	    case CAN_OF_GREASE:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    if (obj->spe <= 10)
			obj->spe += rn1(10, 6);
		    else obj->spe += rn1(5, 6);
		    if (obj->spe > 50) obj->spe = 50;
		    p_glow2(obj, NH_BLUE);
		} else {
		    obj->spe += rnd(5);
		    if (obj->spe > 50) obj->spe = 50;
		    p_glow1(obj);
		}
		break;
	    case MAGIC_FLUTE:
	    case MAGIC_HARP:
	    case FROST_HORN:
	    case FIRE_HORN:
	    case DRUM_OF_EARTHQUAKE:
		if (is_cursed) {
		    stripspe(obj);
		} else if (is_blessed) {
		    obj->spe += d(2,4);
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow2(obj, NH_BLUE);
		} else {
		    obj->spe += rnd(4);
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow1(obj);
		}
		break;
	    default:
		goto not_chargable;
		/*NOTREACHED*/
		break;
	    } /* switch */

	} else {
 not_chargable:
	    You(E_J("have a feeling of loss.","損をしたような気がした。"));
	}
}
/* PYEC, orb of charging, scroll of charging use it */
struct obj *
getchargableobj()
{
#ifdef JP
	static const struct getobj_words chargew = { 0, 0, "充填する", "充填し" };
#endif /*JP*/
	return getobj(all_count, E_J("charge",&chargew), getobj_filter_charge);
}


/* Forget known information about this object class. */
static void
forget_single_object(obj_id)
	int obj_id;
{
	objects[obj_id].oc_name_known = 0;
	objects[obj_id].oc_pre_discovered = 0;	/* a discovery when relearned */
	if (objects[obj_id].oc_uname) {
	    free((genericptr_t)objects[obj_id].oc_uname);
	    objects[obj_id].oc_uname = 0;
	}
	undiscover_object(obj_id);	/* after clearing oc_name_known */

	/* clear & free object names from matching inventory items too? */
}


#if 0	/* here if anyone wants it.... */
/* Forget everything known about a particular object class. */
static void
forget_objclass(oclass)
	int oclass;
{
	int i;

	for (i=bases[oclass];
		i < NUM_OBJECTS && objects[i].oc_class==oclass; i++)
	    forget_single_object(i);
}
#endif


/* randomize the given list of numbers  0 <= i < count */
static void
randomize(indices, count)
	int *indices;
	int count;
{
	int i, iswap, temp;

	for (i = count - 1; i > 0; i--) {
	    if ((iswap = rn2(i + 1)) == i) continue;
	    temp = indices[i];
	    indices[i] = indices[iswap];
	    indices[iswap] = temp;
	}
}


/* Forget % of known objects. */
void
forget_objects(percent)
	int percent;
{
	int i, count;
	int indices[NUM_OBJECTS];

	if (percent == 0) return;
	if (percent <= 0 || percent > 100) {
	    impossible("forget_objects: bad percent %d", percent);
	    return;
	}

	for (count = 0, i = 1; i < NUM_OBJECTS; i++)
	    if (OBJ_DESCR(objects[i]) &&
		    (objects[i].oc_name_known || objects[i].oc_uname))
		indices[count++] = i;

	randomize(indices, count);

	/* forget first % of randomized indices */
	count = ((count * percent) + 50) / 100;
	for (i = 0; i < count; i++)
	    forget_single_object(indices[i]);
}


/* Forget some or all of map (depends on parameters). */
void
forget_map(howmuch)
	int howmuch;
{
	register int zx, zy;

	if (In_sokoban(&u.uz))
	    return;

	known = TRUE;
	for(zx = 0; zx < COLNO; zx++) for(zy = 0; zy < ROWNO; zy++)
	    if (howmuch & ALL_MAP || rn2(7)) {
		/* Zonk all memory of this location. */
		levl[zx][zy].seenv = 0;
		levl[zx][zy].waslit = 0;
		levl[zx][zy].glyph = cmap_to_glyph(S_stone);
#ifdef DUNGEON_OVERVIEW
		lastseentyp[zx][zy] = STONE;
#endif
        /* forget overview data for this level */
        forget_mapseen(ledger_no(&u.uz));
	    }
}

/* Forget all traps on the level. */
void
forget_traps()
{
	register struct trap *trap;

	/* forget all traps (except the one the hero is in :-) */
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if ((trap->tx != u.ux || trap->ty != u.uy) && (trap->ttyp != HOLE))
		trap->tseen = 0;
}

/*
 * Forget given % of all levels that the hero has visited and not forgotten,
 * except this one.
 */
void
forget_levels(percent)
	int percent;
{
	int i, count;
	xchar  maxl, this_lev;
	int indices[MAXLINFO];

	if (percent == 0) return;

	if (percent <= 0 || percent > 100) {
	    impossible("forget_levels: bad percent %d", percent);
	    return;
	}

	this_lev = ledger_no(&u.uz);
	maxl = maxledgerno();

	/* count & save indices of non-forgotten visited levels */
	/* Sokoban levels are pre-mapped for the player, and should stay
	 * so, or they become nearly impossible to solve.  But try to
	 * shift the forgetting elsewhere by fiddling with percent
	 * instead of forgetting fewer levels.
	 */
	for (count = 0, i = 0; i <= maxl; i++)
	    if ((level_info[i].flags & VISITED) &&
			!(level_info[i].flags & FORGOTTEN) && i != this_lev) {
		if (ledger_to_dnum(i) == sokoban_dnum)
		    percent += 2;
		else
		    indices[count++] = i;
	    }
	
	if (percent > 100) percent = 100;

	randomize(indices, count);

	/* forget first % of randomized indices */
	count = ((count * percent) + 50) / 100;
	for (i = 0; i < count; i++) {
	    level_info[indices[i]].flags |= FORGOTTEN;
#ifdef DUNGEON_OVERVIEW
	    forget_mapseen(indices[i]);
#endif
	}
}

/*
 * Forget some things (e.g. after reading a scroll of amnesia).  When called,
 * the following are always forgotten:
 *
 *	- felt ball & chain
 *	- traps
 *	- part (6 out of 7) of the map
 *
 * Other things are subject to flags:
 *
 *	howmuch & ALL_MAP	= forget whole map
 *	howmuch & ALL_SPELLS	= forget all spells
 */
static void
forget(howmuch)
int howmuch;
{

	if (Punished) u.bc_felt = 0;	/* forget felt ball&chain */

	forget_map(howmuch);
	forget_traps();

	/* 1 in 3 chance of forgetting some levels */
	if (!rn2(3)) forget_levels(rn2(25));

	/* 1 in 3 chance of forgeting some objects */
	if (!rn2(3)) forget_objects(rn2(25));

	if (howmuch & ALL_SPELLS) losespells();
	/*
	 * Make sure that what was seen is restored correctly.  To do this,
	 * we need to go blind for an instant --- turn off the display,
	 * then restart it.  All this work is needed to correctly handle
	 * walls which are stone on one side and wall on the other.  Turning
	 * off the seen bits above will make the wall revert to stone,  but
	 * there are cases where we don't want this to happen.  The easiest
	 * thing to do is to run it through the vision system again, which
	 * is always correct.
	 */
	docrt();		/* this correctly will reset vision */
}

/* monster is hit by scroll of taming's effect */
static void
maybe_tame(mtmp, sobj)
struct monst *mtmp;
struct obj *sobj;
{
	if (sobj->cursed) {
	    setmangry(mtmp);
	} else {
	    if (mtmp->isshk)
		make_happy_shk(mtmp, FALSE);
	    else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
		(void) tamedog(mtmp, (struct obj *) 0);
	}
}

static int
getobj_filter_charge(otmp)
struct obj *otmp;
{
	return (is_chargeable(otmp) ? GETOBJ_CHOOSEIT : 0);
}

int
seffects(sobj)
register struct obj	*sobj;
{
	register int cval;
	register boolean confused = (Confusion != 0);
	register struct obj *otmp;

	if (objects[sobj->otyp].oc_magic)
		exercise(A_WIS, TRUE);		/* just for trying */
	switch(sobj->otyp) {
#ifdef MAIL
	case SCR_MAIL:
		known = TRUE;
		if (sobj->spe)
		    pline(E_J("This seems to be junk mail addressed to the finder of the Eye of Larn.",
			      "これは『Larnの瞳』の発見者に宛てられたジャンクメールのようだ。"));
		/* note to the puzzled: the game Larn actually sends you junk
		 * mail if you win!
		 */
		else readmail(sobj);
		break;
#endif
	case SCR_ENCHANT_ARMOR:
	    {
		register schar s;
		boolean special_armor;
		boolean same_color;

		otmp = some_armor(&youmonst);
		if(!otmp) {
			strange_feeling(sobj,
					!Blind ? E_J("Your skin glows then fades.","あなたの肌はしばらく輝いた。") :
					E_J("Your skin feels warm for a moment.",
					    "少しの間、あなたは肌に温もりを感じた。"));
			exercise(A_CON, !sobj->cursed);
			exercise(A_STR, !sobj->cursed);
			return(1);
		}
		if(confused) {
			otmp->oerodeproof = !(sobj->cursed);
			if(Blind) {
			    otmp->rknown = FALSE;
#ifndef JP
			    Your("%s %s warm for a moment.",
				xname(otmp), otense(otmp, "feel"));
#else
			    Your("%sが少しの間温かくなった。", xname(otmp));
#endif /*JP*/
			} else {
			    otmp->rknown = TRUE;
#ifndef JP
			    Your("%s %s covered by a %s %s %s!",
				xname(otmp), otense(otmp, "are"),
				sobj->cursed ? "mottled" : "shimmering",
				 hcolor(sobj->cursed ? NH_BLACK : NH_GOLDEN),
				sobj->cursed ? "glow" :
				  (is_shield(otmp) ? "layer" : "shield"));
#else
			    Your("%sは%s%s%sに覆われた！",
				xname(otmp),
				sobj->cursed ? "不規則な" : "きらめく",
				 hcolor(sobj->cursed ? NH_BLACK : NH_GOLDEN),
				sobj->cursed ? "翳り" : "輝き");
#endif /*JP*/
			}
			if (otmp->oerodeproof &&
			    (otmp->oeroded || otmp->oeroded2 || otmp->odamaged)) {
			    otmp->oeroded = otmp->oeroded2 = 0;
			    otmp->odamaged = 0;
#ifndef JP
			    Your("%s %s as good as new!",
				 xname(otmp),
				 otense(otmp, Blind ? "feel" : "look"));
#else
			    Your("%sはまるで新品のようになった！", xname(otmp));
#endif /*JP*/
			}
			break;
		}
		special_armor = is_special_armor(otmp);
		if (sobj->cursed)
		    same_color =
			(otmp->otyp == BLACK_DRAGON_SCALE_MAIL ||
			 otmp->otyp == BLACK_DRAGON_SCALES ||
			 /* black dress */
			 (otmp->otyp == MAID_DRESS && !objects[MAID_DRESS].oc_name_known));
		else
		    same_color =
			(otmp->otyp == SILVER_DRAGON_SCALE_MAIL ||
			 otmp->otyp == SILVER_DRAGON_SCALES ||
			 otmp->otyp == SHIELD_OF_REFLECTION ||
			 get_material(otmp) == SILVER);
		if (Blind) same_color = FALSE;

		/* KMH -- catch underflow */
		s = sobj->cursed ? -otmp->spe : otmp->spe;
		if (s > (special_armor ? 5 : 3) && rn2(s)) {
#ifndef JP
		Your("%s violently %s%s%s for a while, then %s.",
		     xname(otmp),
		     otense(otmp, Blind ? "vibrate" : "glow"),
		     (!Blind && !same_color) ? " " : nul,
		     (Blind || same_color) ? nul :
			hcolor(sobj->cursed ? NH_BLACK : NH_SILVER),
		     otense(otmp, "evaporate"));
#else
		Your("%sは%s激しく%sと、蒸発した。",
		     xname(otmp),
		     (Blind || same_color) ? nul :
			j_no_ni(hcolor(sobj->cursed ? NH_BLACK : NH_SILVER)),
		     Blind ? "振動する" : "輝く");
#endif /*JP*/
			if(is_cloak(otmp)) (void) Cloak_off();
			if(is_boots(otmp)) (void) Boots_off();
			if(is_helmet(otmp)) (void) Helmet_off();
			if(is_gloves(otmp)) (void) Gloves_off();
			if(is_shield(otmp)) (void) Shield_off();
			if(otmp == uarm) (void) Armor_gone();
			useup(otmp);
			break;
		}
		s = sobj->cursed ? -1 :
		    otmp->spe >= 9 ? (rn2(otmp->spe) == 0) :
		    sobj->blessed ? rnd(3-otmp->spe/3) : 1;
		if (s >= 0 && otmp->otyp >= GRAY_DRAGON_SCALES &&
					otmp->otyp <= YELLOW_DRAGON_SCALES) {
			/* dragon scales get turned into dragon scale mail */
			Your(E_J("%s merges and hardens!",
				 "%sは結合し、強固な鎧となった！"), xname(otmp));
			setworn((struct obj *)0, W_ARM);
			/* assumes same order */
			otmp->otyp = GRAY_DRAGON_SCALE_MAIL +
						otmp->otyp - GRAY_DRAGON_SCALES;
			otmp->cursed = 0;
			if (sobj->blessed) {
				otmp->spe++;
				otmp->blessed = 1;
			}
			otmp->known = 1;
			setworn(otmp, W_ARM);
			break;
		}
		if (s >= 0 && otmp->otyp == SHIELD &&
		    get_material(otmp) == SILVER) {
#ifndef JP
			The("surface of your shield becomes very smooth!");
#else
			Your("銀の盾の表面が鏡のように磨き上げられた！");
#endif /*JP*/
			setworn((struct obj *)0, W_ARMS);
			otmp->otyp = SHIELD_OF_REFLECTION;
			change_material(otmp, 0); /* naturally silver */
			otmp->cursed = 0;
			otmp->odamaged = 0;
			if (sobj->blessed) otmp->blessed = 1;
			setworn(otmp, W_ARMS);
			break;
		}
#ifndef JP
		Your("%s %s%s%s%s for a %s.",
			xname(otmp),
		        s == 0 ? "violently " : nul,
			otense(otmp, Blind ? "vibrate" : "glow"),
			(!Blind && !same_color) ? " " : nul,
			(Blind || same_color) ? nul : hcolor(sobj->cursed ? NH_BLACK : NH_SILVER),
			  (s*s>1) ? "while" : "moment");
#else
		Your("%sは%s%s%s%s。",
			xname(otmp),
			(s*s>1) ? "しばらくの間" : "一瞬",
		        s == 0 ? "激しく " : nul,
			(Blind || same_color) ? nul :
			    j_no_ni(hcolor(sobj->cursed ? NH_BLACK : NH_SILVER)),
			Blind ? "振動した" : "輝いた");
#endif /*JP*/
		otmp->cursed = sobj->cursed;
		if (!otmp->blessed || sobj->cursed)
			otmp->blessed = sobj->blessed;
		if (s) {
			otmp->spe += s;
			adj_abon(otmp, s);
			known = otmp->known;
		}

		if ((otmp->spe > (special_armor ? 5 : 3)) &&
		    (special_armor || !rn2(7)))
#ifndef JP
			Your("%s suddenly %s %s.",
				xname(otmp), otense(otmp, "vibrate"),
				Blind ? "again" : "unexpectedly");
#else
			Your("%sは%s震えた。",
				xname(otmp),
				Blind ? "急にもう一度" : "不意に");
#endif /*JP*/
		break;
	    }
	case SCR_DESTROY_ARMOR:
	    {
		otmp = some_armor(&youmonst);
		if(confused) {
			if(!otmp) {
				strange_feeling(sobj,E_J("Your bones itch.","あなたは骨に痒みを感じた。"));
				exercise(A_STR, FALSE);
				exercise(A_CON, FALSE);
				return(1);
			}
			otmp->oerodeproof = sobj->cursed;
			p_glow2(otmp, NH_PURPLE);
			break;
		}
		if(!sobj->cursed || !otmp || !otmp->cursed) {
		    if(!destroy_arm(otmp)) {
			strange_feeling(sobj,E_J("Your skin itches.","あなたの肌が痒くなった。"));
			exercise(A_STR, FALSE);
			exercise(A_CON, FALSE);
			return(1);
		    } else
			known = TRUE;
		} else {	/* armor and scroll both cursed */
#ifndef JP
		    Your("%s %s.", xname(otmp), otense(otmp, "vibrate"));
#else
		    Your("%sが震えた。", xname(otmp));
#endif /*JP*/
		    if (otmp->spe >= -6) otmp->spe--;
		    make_stunned(HStun + rn1(10, 10), TRUE);
		}
	    }
	    break;
	case SCR_CONFUSE_MONSTER:
	case SPE_CONFUSE_MONSTER:
		if(youmonst.data->mlet != S_HUMAN || sobj->cursed) {
			if(!HConfusion) You_feel(E_J("confused.","混乱した。"));
			make_confused(HConfusion + rnd(100),FALSE);
		} else  if(confused) {
		    if(!sobj->blessed) {
#ifndef JP
			Your("%s begin to %s%s.",
			    makeplural(body_part(HAND)),
			    Blind ? "tingle" : "glow ",
			    Blind ? nul : hcolor(NH_PURPLE));
#else
			Your("%sが%s%sはじめた。",
			    body_part(HAND),
			    Blind ? nul : j_no_ni(hcolor(NH_PURPLE)),
			    Blind ? "うずき" : "輝き");
#endif /*JP*/
			make_confused(HConfusion + rnd(100),FALSE);
		    } else {
#ifndef JP
			pline("A %s%s surrounds your %s.",
			    Blind ? nul : hcolor(NH_RED),
			    Blind ? "faint buzz" : " glow",
			    body_part(HEAD));
#else
			pline("%s%sがあなたの%sを取り巻いた。",
			    Blind ? nul : hcolor(NH_RED),
			    Blind ? "かすかな興奮" : "光",
			    body_part(HEAD));
#endif /*JP*/
			make_confused(0L,TRUE);
		    }
		} else {
		    if (!sobj->blessed) {
#ifndef JP
			Your("%s%s %s%s.",
			makeplural(body_part(HAND)),
			Blind ? "" : " begin to glow",
			Blind ? (const char *)"tingle" : hcolor(NH_RED),
			u.umconf ? " even more" : "");
#else
			Your("%sが%s%s%s。",
			body_part(HAND),
			u.umconf ? "より一層" : "",
			Blind ? "" : j_no_ni(hcolor(NH_RED)),
			Blind ? "うずいた" : "輝きはじめた");
#endif /*JP*/
			u.umconf++;
		    } else {
			if (Blind)
#ifndef JP
			    Your("%s tingle %s sharply.",
				makeplural(body_part(HAND)),
				u.umconf ? "even more" : "very");
#else
			    You("%sに%s鋭い刺激を感じた。",
				body_part(HAND),
				u.umconf ? "より一層" : "とても");
#endif /*JP*/
			else
#ifndef JP
			    Your("%s glow a%s brilliant %s.",
				makeplural(body_part(HAND)),
				u.umconf ? "n even more" : "",
				hcolor(NH_RED));
#else
			    Your("%sが%sまぶしく%s輝きを放ちはじめた。",
				body_part(HAND),
				u.umconf ? "より一層" : "",
				hcolor(NH_RED));
#endif /*JP*/
			/* after a while, repeated uses become less effective */
			if (u.umconf >= 40)
			    u.umconf++;
			else
			    u.umconf += rn1(8, 2);
		    }
		}
		break;
	case SCR_SCARE_MONSTER:
	case SPE_CAUSE_FEAR:
	    {	register int ct = 0;
		register struct monst *mtmp;

		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if(cansee(mtmp->mx,mtmp->my)) {
			if(confused || sobj->cursed) {
			    mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
			    mtmp->mcanmove = 1;
			} else
			    if (! resist(mtmp, sobj->oclass, 0, NOTELL))
				monflee(mtmp, 0, FALSE, FALSE);
			if(!mtmp->mtame) ct++;	/* pets don't laugh at you */
		    }
		}
		if(!ct)
		      You_hear(E_J("%s in the distance.","遠くに%sを"),
			       (confused || sobj->cursed) ? E_J("sad wailing","悲しげな嘆き声を") :
							E_J("maniacal laughter","狂気じみた笑い声を"));
		else if(sobj->otyp == SCR_SCARE_MONSTER)
			You_hear(E_J("%s close by.","すぐそばに%sを"),
				  (confused || sobj->cursed) ? E_J("sad wailing","悲しげな嘆き声を") :
						 E_J("maniacal laughter","狂気じみた笑い声を"));
		break;
	    }
	case SCR_BLANK_PAPER:
	    if (Blind)
#ifndef JP
		You("don't remember there being any magic words on this scroll.");
#else
		pline("この巻物には何の魔法の言葉も書かれていなかったはずだ。");
#endif /*JP*/
	    else
		pline(E_J("This scroll seems to be blank.","この巻物は白紙のようだ。"));
	    known = TRUE;
	    break;
	case SCR_REMOVE_CURSE:
	case SPE_REMOVE_CURSE:
	    {	register struct obj *obj;
		if(confused)
		    if (Hallucination)
			You_feel(E_J("the power of the Force against you!",
				     "フォースの力が向けられているのを感じた！"));
		    else
			You_feel(E_J("like you need some help.",
				     "助けが欲しいような気がした。"));
		else
		    if (Hallucination)
			You_feel(E_J("in touch with the Universal Oneness.",
				     "大宇宙の調和に触れたような気がした。"));
		    else
			You_feel(E_J("like someone is helping you.",
				     "誰かに助けられているような気がした。"));

		if (sobj->cursed) {
		    pline_The(E_J("scroll disintegrates.","巻物は崩れ落ちた。"));
		} else {
		    for (obj = invent; obj; obj = obj->nobj) {
			long wornmask;
#ifdef GOLDOBJ
			/* gold isn't subject to cursing and blessing */
			if (obj->oclass == COIN_CLASS) continue;
#endif
			wornmask = (obj->owornmask & ~(W_BALL|W_ART|W_ARTI));
			if (wornmask && !sobj->blessed) {
			    /* handle a couple of special cases; we don't
			       allow auxiliary weapon slots to be used to
			       artificially increase number of worn items */
			    if (obj == uswapwep) {
				if (!u.twoweap) wornmask = 0L;
			    } else if (obj == uquiver) {
				if (obj->oclass == WEAPON_CLASS) {
				    /* mergeable weapon test covers ammo,
				       missiles, spears, daggers & knives */
				    if (!objects[obj->otyp].oc_merge) 
					wornmask = 0L;
				} else if (obj->oclass == GEM_CLASS) {
				    /* possibly ought to check whether
				       alternate weapon is a sling... */
				    if (!uslinging()) wornmask = 0L;
				} else {
				    /* weptools don't merge and aren't
				       reasonable quivered weapons */
				    wornmask = 0L;
				}
			    }
			}
			if (sobj->blessed || wornmask ||
			     obj->otyp == LOADSTONE ||
			     (obj->otyp == LEASH && obj->leashmon)) {
			    if(confused) blessorcurse(obj, 2);
			    else uncurse(obj);
			}
		    }
		}
		if(Punished && !confused) unpunish();
#ifdef STEED
		if (u.usteed) {
		    obj = which_armor(u.usteed, W_SADDLE);
		    if (obj) {
			if(confused) blessorcurse(obj, 2);
			else uncurse(obj);
		    }
		}
#endif
		update_inventory();
		break;
	    }
	case SCR_CREATE_MONSTER:
	case SPE_CREATE_MONSTER:
	    if (create_critters(1 + ((confused || sobj->cursed) ? 12 : 0) +
				((sobj->blessed || rn2(73)) ? 0 : rnd(4)),
			confused ? &mons[PM_ACID_BLOB] : (struct permonst *)0))
		known = TRUE;
	    /* no need to flush monsters; we ask for identification only if the
	     * monsters are not visible
	     */
	    break;
	case SCR_ENCHANT_WEAPON:
		if(uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep))
			&& confused) {
		/* oclass check added 10/25/86 GAN */
			uwep->oerodeproof = !(sobj->cursed);
			if (Blind) {
			    uwep->rknown = FALSE;
			    Your(E_J("weapon feels warm for a moment.",
				     "武器がひととき温かく感じられた。"));
			} else {
			    uwep->rknown = TRUE;
#ifndef JP
			    Your("%s covered by a %s %s %s!",
				aobjnam(uwep, "are"),
				sobj->cursed ? "mottled" : "shimmering",
				hcolor(sobj->cursed ? NH_PURPLE : NH_GOLDEN),
				sobj->cursed ? "glow" : "shield");
#else
			    Your("%sは%s%s%sに覆われた！",
				xname(uwep),
				sobj->cursed ? "不規則な" : "きらめく",
				hcolor(sobj->cursed ? NH_PURPLE : NH_GOLDEN),
				sobj->cursed ? "翳り" : "輝き");
#endif /*JP*/
			}
			if (uwep->oerodeproof && (uwep->oeroded || uwep->oeroded2)) {
			    uwep->oeroded = uwep->oeroded2 = 0;
#ifndef JP
			    Your("%s as good as new!",
				 aobjnam(uwep, Blind ? "feel" : "look"));
#else
			    Your("%sはまるで新品のようになった！", xname(uwep));
#endif /*JP*/
			}
		} else return !chwepon(sobj,
				       sobj->cursed ? -1 :
				       !uwep ? 1 :
				       uwep->spe >= 9 ? (rn2(uwep->spe) == 0) :
				       sobj->blessed ? rnd(3-uwep->spe/3) : 1);
		break;
	case SCR_TAMING:
	case SPE_CHARM_MONSTER:
		if (u.uswallow) {
		    maybe_tame(u.ustuck, sobj);
		} else {
		    int i, j, bd = confused ? 5 : 1;
		    struct monst *mtmp;

		    for (i = -bd; i <= bd; i++) for(j = -bd; j <= bd; j++) {
			if (!isok(u.ux + i, u.uy + j)) continue;
			if ((mtmp = m_at(u.ux + i, u.uy + j)) != 0)
			    maybe_tame(mtmp, sobj);
		    }
		}
		break;
	case SCR_GENOCIDE:
#ifndef JP
		You("have found a scroll of genocide!");
#else
		pline("これは虐殺の巻物だ！");
#endif /*JP*/
		known = TRUE;
		if (sobj->blessed) do_class_genocide();
		else do_genocide(!sobj->cursed | (2 * !!Confusion));
		break;
	case SCR_LIGHT:
		if(!Blind) known = TRUE;
		litroom(!confused && !sobj->cursed, sobj);
		break;
	case SCR_TELEPORTATION:
		if(confused || sobj->cursed) level_tele();
		else {
			if (sobj->blessed && !Teleport_control) {
				HTeleport_control |= 1;
				known = TRUE;
//				if (yn("Do you wish to teleport?")=='n')
//					break;
			}
			tele();
			if(Teleport_control || !couldsee(u.ux0, u.uy0) ||
			   (distu(u.ux0, u.uy0) >= 16))
				known = TRUE;
		}
		break;
	case SCR_GOLD_DETECTION:
		if (confused || sobj->cursed) return(trap_detect(sobj));
		else return(gold_detect(sobj));
	case SCR_FOOD_DETECTION:
	case SPE_DETECT_FOOD:
		if (food_detect(sobj))
			return(1);	/* nothing detected */
		break;
	case SPE_IDENTIFY:
		cval = rn2(5);
		goto id;
	case SCR_IDENTIFY:
		/* known = TRUE; */
		if(confused)
			You(E_J("identify this as an identify scroll.",
				"この巻物が識別の巻物であると識別した。"));
		else
			pline(E_J("This is an identify scroll.","これは識別の巻物だ。"));
		if (sobj->blessed || (!sobj->cursed && !rn2(5))) {
			cval = rn2(5);
			/* Note: if rn2(5)==0, identify all items */
			if (cval == 1 && sobj->blessed && Luck > 0) ++cval;
		} else	cval = 1;
		if(!objects[sobj->otyp].oc_name_known) more_experienced(0,10);
		useup(sobj);
		makeknown(SCR_IDENTIFY);
	id:
		if(invent && !confused) {
		    identify_pack(cval);
		}
		return(1);
	case SCR_CHARGING:
		if (confused) {
		    You_feel(E_J("charged up!","エネルギーで満たされた！"));
		    if (u.uen < u.uenmax)
			u.uen = u.uenmax;
		    else
			u.uen = (u.uenmax += d(5,4));
		    flags.botl = 1;
		    break;
		}
		known = TRUE;
		pline(E_J("This is a charging scroll.","これは充填の巻物だ。"));
		otmp = getchargableobj();
		if (!otmp) break;
		recharge(otmp, sobj->cursed ? -1 : (sobj->blessed ? 1 : 0));
		break;
	case SCR_MAGIC_MAPPING:
		if (level.flags.nommap) {
		    Your(E_J("mind is filled with crazy lines!",
			     "精神にたくさんの無秩序な線の群れがなだれ込んだ！"));
		    if (Hallucination)
			pline(E_J("Wow!  Modern art.","わあ！ モダンアートだ。"));
		    else
			Your(E_J("%s spins in bewilderment.",
				 "%sはぐるぐる回り、わけがわからなくなった。"), body_part(HEAD));
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
		if (sobj->blessed) {
		    register int x, y;

		    for (x = 1; x < COLNO; x++)
		    	for (y = 0; y < ROWNO; y++)
		    	    if (levl[x][y].typ == SDOOR)
		    	    	cvt_sdoor_to_door(&levl[x][y]);
		    /* do_mapping() already reveals secret passages */
		}
		known = TRUE;
	case SPE_MAGIC_MAPPING:
		if (level.flags.nommap) {
#ifndef JP
		    Your("%s spins as %s blocks the spell!", body_part(HEAD), something);
#else
		    pline("何かが呪文を妨害し、あなたの%sはぐるぐると回った！", body_part(HEAD));
#endif /*JP*/
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
		pline(E_J("A map coalesces in your mind!","地図があなたの精神に浮かび上がった！"));
		cval = (sobj->cursed && !confused);
		if(cval) HConfusion = 1;	/* to screw up map */
		do_mapping();
		if(cval) {
		    HConfusion = 0;		/* restore */
		    pline(E_J("Unfortunately, you can't grasp the details.",
			      "残念なことに、あなたは詳細を把握できなかった。"));
		}
		break;
	case SCR_AMNESIA:
		known = TRUE;
		forget(	(!sobj->blessed ? ALL_SPELLS : 0) |
			(!confused || sobj->cursed ? ALL_MAP : 0) );
		if (Hallucination) /* Ommmmmm! */
			Your(E_J("mind releases itself from mundane concerns.",
				 "精神はくだらない悩みから解放された。"));
		else if (!strncmpi(plname, "Maud", 4))
			pline(E_J("As your mind turns inward on itself, you forget everything else.",
				  "あなたの精神は自己の内面にのみ向けられ、他のすべてを忘れてしまった。"));
		else if (rn2(2))
			pline(E_J("Who was that Maud person anyway?",
				  "Maudって娘はいったい誰だい？"));
		else
			pline(E_J("Thinking of Maud you forget everything else.",
				  "Maudのことを考えると、あなたは他のすべてを忘れてしまった。"));
		exercise(A_WIS, FALSE);
		break;
	case SCR_FIRE:
		/*
		 * Note: Modifications have been made as of 3.0 to allow for
		 * some damage under all potential cases.
		 */
		cval = bcsign(sobj);
		if(!objects[sobj->otyp].oc_name_known) more_experienced(0,10);
		useup(sobj);
		makeknown(SCR_FIRE);
		if(confused) {
		    if(Fire_resistance) {
			shieldeff(u.ux, u.uy);
			if(!Blind)
			    pline(E_J("Oh, look, what a pretty fire in your %s.",
				      "ほら見て、あなたの%sの中にとてもかわいらしい炎があるよ。"),
				E_J(makeplural(body_part(HAND)),body_part(HAND)));
			else You_feel(E_J("a pleasant warmth in your %s.",
					  "%sのなかに心地よい暖かさを感じた。"),
				E_J(makeplural(body_part(HAND)),body_part(HAND)));
		    } else {
			pline_The(E_J("scroll catches fire and you burn your %s.",
				      "巻物に火がつき、あなたの%sを焼いた。"),
				E_J(makeplural(body_part(HAND)),body_part(HAND)));
			losehp(1, E_J("scroll of fire","炎の巻物で"), KILLED_BY_AN);
		    }
		    return(1);
		}
		if (Underwater)
			pline_The(E_J("water around you vaporizes violently!",
				      "あなたの周りの水が激しく沸騰した！"));
		else {
		    pline_The(E_J("scroll erupts in a tower of flame!",
				  "巻物から、燃えさかる炎が噴き上がった！"));
		    burn_away_slime();
		}
		explode(u.ux, u.uy, 11, (2*(rn1(3, 3) + 2 * cval) + 1)/3,
							SCROLL_CLASS, EXPL_FIERY);
		return(1);
	case SCR_EARTH:
	    /* TODO: handle steeds */
	    if (
#ifdef REINCARNATION
		!Is_rogue_level(&u.uz) && 
#endif
	    	 (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
	    	register int x, y;

	    	/* Identify the scroll */
#ifndef JP
	    	pline_The("%s rumbles %s you!", ceiling(u.ux,u.uy),
	    			sobj->blessed ? "around" : "above");
#else
	    	Your("%s%sから轟音が響いた！",
	    		sobj->blessed ? "周りの" : "上の", ceiling(u.ux,u.uy));
#endif /*JP*/
	    	known = 1;
	    	if (In_sokoban(&u.uz))
	    	    change_luck(-1);	/* Sokoban guilt */

	    	/* Loop through the surrounding squares */
	    	if (!sobj->cursed) for (x = u.ux-1; x <= u.ux+1; x++) {
	    	    for (y = u.uy-1; y <= u.uy+1; y++) {

	    	    	/* Is this a suitable spot? */
	    	    	if (isok(x, y) && !closed_door(x, y) &&
	    	    			!IS_ROCK(levl[x][y].typ) &&
	    	    			!IS_AIR(levl[x][y].typ) &&
					(x != u.ux || y != u.uy)) {
			    register struct obj *otmp2;
			    register struct monst *mtmp;

	    	    	    /* Make the object(s) */
	    	    	    otmp2 = mksobj(confused ? ROCK : BOULDER,
	    	    	    		FALSE, FALSE);
	    	    	    if (!otmp2) continue;  /* Shouldn't happen */
	    	    	    otmp2->quan = confused ? rn1(5,2) : 1;
	    	    	    otmp2->owt = weight(otmp2);

	    	    	    /* Find the monster here (won't be player) */
	    	    	    mtmp = m_at(x, y);
	    	    	    if (mtmp && !amorphous(mtmp->data) &&
	    	    	    		!passes_walls(mtmp->data) &&
	    	    	    		!noncorporeal(mtmp->data) &&
	    	    	    		!unsolid(mtmp->data)) {
				struct obj *helmet = which_armor(mtmp, W_ARMH);
				int mdmg;

				if (cansee(mtmp->mx, mtmp->my)) {
				    pline(E_J("%s is hit by %s!",
					      "%sに%sが命中した！"), Monnam(mtmp),
	    	    	    			doname(otmp2));
				    if (mtmp->minvis && !canspotmons(mtmp))
					map_invisible(mtmp->mx, mtmp->my);
				}
	    	    	    	mdmg = dmgval(otmp2, mtmp) * otmp2->quan;
				if (helmet) {
				    if(is_metallic(helmet)) {
					if (canspotmon(mtmp))
					    pline(E_J("Fortunately, %s is wearing a hard helmet.",
						      "幸い、%sは硬い兜をかぶっていた。"), mon_nam(mtmp));
					else if (flags.soundok)
					    You_hear(E_J("a clanging sound.","金属の衝突する音を"));
					if (mdmg > 2) mdmg = 2;
				    } else {
					if (canspotmon(mtmp))
#ifndef JP
					    pline("%s's %s does not protect %s.",
						Monnam(mtmp), xname(helmet),
						mhim(mtmp));
#else
					    pline("%sの%sは%sを防げなかった。",
						Monnam(mtmp), xname(helmet), xname(otmp2));
#endif /*JP*/
				    }
				}
	    	    	    	mtmp->mhp -= mdmg;
	    	    	    	if (mtmp->mhp <= 0)
	    	    	    	    xkilled(mtmp, 1);
	    	    	    }
	    	    	    /* Drop the rock/boulder to the floor */
	    	    	    if (!flooreffects(otmp2, x, y, E_J("fall","落ちた"))) {
	    	    	    	place_object(otmp2, x, y);
	    	    	    	stackobj(otmp2);
	    	    	    	newsym(x, y);  /* map the rock */
	    	    	    }
	    	    	}
		    }
		}
		/* Attack the player */
		if (!sobj->blessed) {
		    int dmg;
		    struct obj *otmp2;

		    /* Okay, _you_ write this without repeating the code */
		    otmp2 = mksobj(confused ? ROCK : BOULDER,
				FALSE, FALSE);
		    if (!otmp2) break;
		    otmp2->quan = confused ? rn1(5,2) : 1;
		    otmp2->owt = weight(otmp2);
		    if (!amorphous(youmonst.data) &&
				!Passes_walls &&
				!noncorporeal(youmonst.data) &&
				!unsolid(youmonst.data)) {
#ifndef JP
			You("are hit by %s!", doname(otmp2));
#else
			pline("%sがあなたに命中した！", doname(otmp2));
#endif /*JP*/
			dmg = dmgval(otmp2, &youmonst) * otmp2->quan;
			if (uarmh && !sobj->cursed) {
			    if(is_metallic(uarmh)) {
				pline(E_J("Fortunately, you are wearing a hard helmet.",
					  "幸い、あなたは硬い兜をかぶっていた。"));
				if (dmg > 2) dmg = 2;
			    } else if (flags.verbose) {
#ifndef JP
				Your("%s does not protect you.",
						xname(uarmh));
#else
				Your("%sは%sを防げなかった。",
					xname(uarmh), xname(otmp2));
#endif /*JP*/
			    }
			}
		    } else
			dmg = 0;
		    /* Must be before the losehp(), for bones files */
		    if (!flooreffects(otmp2, u.ux, u.uy, "fall")) {
			place_object(otmp2, u.ux, u.uy);
			stackobj(otmp2);
			newsym(u.ux, u.uy);
		    }
		    if (dmg) losehp(dmg, E_J("scroll of earth","大地の巻物で"), KILLED_BY_AN);
		}
	    }
	    break;
	case SCR_PUNISHMENT:
		known = TRUE;
		if(confused || sobj->blessed) {
			You_feel(E_J("guilty.","罪の意識を覚えた。"));
			break;
		}
		punish(sobj);
		break;
	case SCR_STINKING_CLOUD: {
	        coord cc;

#ifndef JP
		You("have found a scroll of stinking cloud!");
#else
		pline("これは悪臭雲の巻物だ！");
#endif /*JP*/
		known = TRUE;
		pline(E_J("Where do you want to center the cloud?",
			  "雲の中心をどこに定めますか？"));
		cc.x = u.ux;
		cc.y = u.uy;
		if (getpos(&cc, TRUE, E_J("the desired position","望みの位置")) < 0) {
		    pline(Never_mind);
		    return 0;
		}
		if (!cansee(cc.x, cc.y) || distu(cc.x, cc.y) >= 32) {
		    You(E_J("smell rotten eggs.","腐った卵の臭いを嗅いだ。"));
		    return 0;
		}
//		(void) create_gas_cloud(cc.x, cc.y, 3+bcsign(sobj),
//						8+4*bcsign(sobj));
		(void) create_stinking_cloud(cc.x, cc.y, 3+bcsign(sobj),
						8+4*bcsign(sobj), TRUE);
		break;
	}
	default:
		impossible("What weird effect is this? (%u)", sobj->otyp);
	}
	return(0);
}

static void
wand_explode(obj)
register struct obj *obj;
{
    obj->in_use = TRUE;	/* in case losehp() is fatal */
    Your(E_J("%s vibrates violently, and explodes!",
	     "%sは激しく震え、爆発した！"),xname(obj));
    nhbell();
    losehp(rnd(2*(u.uhpmax+1)/3), E_J("exploding wand","杖の爆発で"), KILLED_BY_AN);
    useup(obj);
    exercise(A_STR, FALSE);
}

/*
 * Low-level lit-field update routine.
 */
STATIC_PTR void
set_lit(x,y,val)
int x, y;
genericptr_t val;
{
	if (val)
	    levl[x][y].lit = 1;
	else {
	    levl[x][y].lit = 0;
	    snuff_light_source(x, y);
	}
}

void
litroom(on,obj)
register boolean on;
struct obj *obj;
{
	char is_lit;	/* value is irrelevant; we use its address
			   as a `not null' flag for set_lit() */

	/* first produce the text (provided you're not blind) */
	if(!on) {
		register struct obj *otmp;

		if (!Blind) {
		    if(u.uswallow) {
			pline(E_J("It seems even darker in here than before.",
				  "ここはただでさえ暗かったのに、さらに暗くなったようだ。"));
			return;
		    }
		    if (uwep && artifact_light(uwep) && uwep->lamplit)
#ifndef JP
			pline("Suddenly, the only light left comes from %s!",
				the(xname(uwep)));
#else
			pline("突然、%sの光だけを残して周囲が真っ暗になった！",
				xname(uwep));
#endif /*JP*/
		    else
			You(E_J("are surrounded by darkness!","闇につつまれた！"));
		}

		/* the magic douses lamps, et al, too */
		for(otmp = invent; otmp; otmp = otmp->nobj)
		    if (otmp->lamplit)
			(void) snuff_lit(otmp);
		if (Blind) goto do_it;
	} else {
		if (Blind) goto do_it;
		if(u.uswallow){
			if (is_animal(u.ustuck->data))
				pline(E_J("%s %s is lit.","%s%sが照らされた。"),
				        s_suffix(Monnam(u.ustuck)),
					mbodypart(u.ustuck, STOMACH));
			else
				if (is_whirly(u.ustuck->data))
					pline(E_J("%s shines briefly.","%sは一瞬輝いた。"),
					      Monnam(u.ustuck));
				else
					pline(E_J("%s glistens.","%sはぬらりと光った。"), Monnam(u.ustuck));
			return;
		}
		if (obj->oartifact == ART_SUNSWORD)
			pline(E_J("A brilliant white-hot light bursts from the sword!",
				  "白熱するまばゆい光が剣からほとばしった！"));
		else
			pline(E_J("A lit field surrounds you!","あなたの周囲が明るく照らされた！"));
	}

do_it:
	/* No-op in water - can only see the adjacent squares and that's it! */
	if (Underwater || Is_waterlevel(&u.uz)) return;
	/*
	 *  If we are darkening the room and the hero is punished but not
	 *  blind, then we have to pick up and replace the ball and chain so
	 *  that we don't remember them if they are out of sight.
	 */
	if (Punished && !on && !Blind)
	    move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) {
	    /* Can't use do_clear_area because MAX_RADIUS is too small */
	    /* rogue lighting must light the entire room */
	    int rnum = levl[u.ux][u.uy].roomno - ROOMOFFSET;
	    int rx, ry;
	    if(rnum >= 0) {
		for(rx = rooms[rnum].lx-1; rx <= rooms[rnum].hx+1; rx++)
		    for(ry = rooms[rnum].ly-1; ry <= rooms[rnum].hy+1; ry++)
			set_lit(rx, ry,
				(genericptr_t)(on ? &is_lit : (char *)0));
		rooms[rnum].rlit = on;
	    }
	    /* hallways remain dark on the rogue level */
	} else
#endif
	    do_clear_area(u.ux,u.uy,
		(obj && (obj->oclass==SCROLL_CLASS || obj->oclass == WEAPON_CLASS) &&
		obj->blessed) ? 9 : 5,
		set_lit, (genericptr_t)(on ? &is_lit : (char *)0));

	/*
	 *  If we are not blind, then force a redraw on all positions in sight
	 *  by temporarily blinding the hero.  The vision recalculation will
	 *  correctly update all previously seen positions *and* correctly
	 *  set the waslit bit [could be messed up from above].
	 */
	if (!Blind) {
	    vision_recalc(2);

	    /* replace ball&chain */
	    if (Punished && !on)
		move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
	}

	vision_full_recalc = 1;	/* delayed vision recalculation */
}

static void
do_class_genocide()
{
	int i, j, immunecnt, gonecnt, goodcnt, class, feel_dead = 0;
	char buf[BUFSZ];
	boolean gameover = FALSE;	/* true iff killed self */

	for(j=0; ; j++) {
		if (j >= 5) {
			pline(thats_enough_tries);
			return;
		}
		do {
		    getlin(E_J("What class of monsters do you wish to genocide?",
			       "どの種別の怪物を虐殺しますか？"),
			buf);
		    (void)mungspaces(buf);
		} while (buf[0]=='\033' || !buf[0]);
		/* choosing "none" preserves genocideless conduct */
		if (!strcmpi(buf, "none") ||
		    !strcmpi(buf, "nothing")) return;
#ifdef JP
		if (!strcmpi(buf, "なし") ||
		    !strcmpi(buf, "しない")) return;
#endif /*JP*/

		if (strlen(buf) == 1) {
		    if (buf[0] == ILLOBJ_SYM)
			buf[0] = def_monsyms[S_MIMIC];
		    class = def_char_to_monclass(buf[0]);
		} else {
		    char buf2[BUFSZ];

		    class = 0;
		    Strcpy(buf2, makesingular(buf));
		    Strcpy(buf, buf2);
		}
		immunecnt = gonecnt = goodcnt = 0;
		for (i = LOW_PM; i < NUMMONS; i++) {
		    if (class == 0 &&
			    strstri(monexplain[(int)mons[i].mlet], buf) != 0)
			class = mons[i].mlet;
		    if (mons[i].mlet == class) {
			if (!(mons[i].geno & G_GENO)) immunecnt++;
			else if(mvitals[i].mvflags & G_GENOD) gonecnt++;
			else goodcnt++;
		    }
		}
		/*
		 * TODO[?]: If user's input doesn't match any class
		 *	    description, check individual species names.
		 */
		if (!goodcnt && class != mons[urole.malenum].mlet &&
				class != mons[urace.malenum].mlet) {
			if (gonecnt)
	pline(E_J("All such monsters are already nonexistent.",
		  "その類の怪物はすべてすでに存在しない。"));
			else if (immunecnt ||
				(buf[0] == DEF_INVISIBLE && buf[1] == '\0'))
	You(E_J("aren't permitted to genocide such monsters.",
		"そのような怪物を虐殺することを許されていない。"));
			else
#ifdef WIZARD	/* to aid in topology testing; remove pesky monsters */
			  if (wizard && buf[0] == '*') {
			    register struct monst *mtmp, *mtmp2;

			    gonecnt = 0;
			    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
				mtmp2 = mtmp->nmon;
			    	if (DEADMONSTER(mtmp)) continue;
				mongone(mtmp);
				gonecnt++;
			    }
	pline("Eliminated %d monster%s.", gonecnt, plur(gonecnt));
			    return;
			} else
#endif
	pline(E_J("That symbol does not represent any monster.",
		  "そのシンボルに相当する怪物は存在しません。"));
			continue;
		}

		for (i = LOW_PM; i < NUMMONS; i++) {
		    if(mons[i].mlet == class) {
			char nam[BUFSZ];

			Strcpy(nam, E_J(makeplural(mons[i].mname), JMONNAM(i)));
			/* Although "genus" is Latin for race, the hero benefits
			 * from both race and role; thus genocide affects either.
			 */
			if (Your_Own_Role(i) || Your_Own_Race(i) ||
				((mons[i].geno & G_GENO)
				&& !(mvitals[i].mvflags & G_GENOD))) {
			/* This check must be first since player monsters might
			 * have G_GENOD or !G_GENO.
			 */
			    mvitals[i].mvflags |= (G_GENOD|G_NOCORPSE);
			    reset_rndmonst(i);
			    kill_genocided_monsters();
			    update_inventory();		/* eggs & tins */
			    pline(E_J("Wiped out all %s.","すべての%sを一掃した。"), nam);
			    if (Upolyd && i == u.umonnum) {
				u.mh = -1;
				if (Unchanging) {
				    if (!feel_dead++) You(E_J("die.","死んだ。"));
				    /* finish genociding this class of
				       monsters before ultimately dying */
				    gameover = TRUE;
				} else
				    rehumanize();
			    }
			    /* Self-genocide if it matches either your race
			       or role.  Assumption:  male and female forms
			       share same monster class. */
			    if (i == urole.malenum || i == urace.malenum) {
				u.uhp = -1;
				if (Upolyd) {
				    if (!feel_dead++) You_feel(E_J("dead inside.","己の魂が死んだのを感じた。"));
				} else {
				    if (!feel_dead++) You(E_J("die.","死んだ。"));
				    gameover = TRUE;
				}
			    }
			} else if (mvitals[i].mvflags & G_GENOD) {
			    if (!gameover)
				pline(E_J("All %s are already nonexistent.",
					  "%sはもう存在しない。"), nam);
			} else if (!gameover) {
			  /* suppress feedback about quest beings except
			     for those applicable to our own role */
			  if ((mons[i].msound != MS_LEADER ||
			       quest_info(MS_LEADER) == i)
			   && (mons[i].msound != MS_NEMESIS ||
			       quest_info(MS_NEMESIS) == i)
			   && (mons[i].msound != MS_GUARDIAN ||
			       quest_info(MS_GUARDIAN) == i)
			/* non-leader/nemesis/guardian role-specific monster */
			   && (i != PM_NINJA ||		/* nuisance */
			       Role_if(PM_SAMURAI))) {
				boolean named, uniq;

				named = type_is_pname(&mons[i]) ? TRUE : FALSE;
				uniq = (mons[i].geno & G_UNIQ) ? TRUE : FALSE;
				/* one special case */
				if (i == PM_HIGH_PRIEST) uniq = FALSE;

#ifndef JP
				You("aren't permitted to genocide %s%s.",
				    (uniq && !named) ? "the " : "",
				    (uniq || named) ? mons[i].mname : nam);
#else
				You("%sを虐殺することを許されていない。", nam);
#endif /*JP*/
			    }
			}
		    }
		}
		if (gameover || u.uhp == -1) {
		    killer_format = KILLED_BY_AN;
		    killer = E_J("scroll of genocide","虐殺の巻物で");
		    if (gameover) done(GENOCIDED);
		}
		return;
	}
}

#define REALLY 1
#define PLAYER 2
#define ONTHRONE 4
void
do_genocide(how)
int how;
/* 0 = no genocide; create monsters (cursed scroll) */
/* 1 = normal genocide */
/* 3 = forced genocide of player */
/* 5 (4 | 1) = normal genocide from throne */
{
	char buf[BUFSZ];
	register int	i, killplayer = 0;
	register int mndx;
	register struct permonst *ptr;
	const char *which;

	if (how & PLAYER) {
		mndx = u.umonster;	/* non-polymorphed mon num */
		ptr = &mons[mndx];
		Strcpy(buf, ptr->mname);
		killplayer++;
	} else {
	    for(i = 0; ; i++) {
		if(i >= 5) {
		    pline(thats_enough_tries);
		    return;
		}
		getlin(E_J("What monster do you want to genocide? [type the name]",
			   "どの怪物を虐殺しますか？ [名前を入れてください]"),
			buf);
		(void)mungspaces(buf);
		/* choosing "none" preserves genocideless conduct */
		if (!strcmpi(buf, "none") || !strcmpi(buf, "nothing")
#ifdef JP
		 || !strcmpi(buf, "なし") || !strcmpi(buf, "しない")
#endif /*JP*/
		   ) {
		    /* ... but no free pass if cursed */
		    if (!(how & REALLY)) {
			ptr = rndmonst();
			if (!ptr) return; /* no message, like normal case */
			mndx = monsndx(ptr);
			break;		/* remaining checks don't apply */
		    } else return;
		}

		mndx = name_to_mon(buf);
		if (mndx == NON_PM || (mvitals[mndx].mvflags & G_GENOD)) {
			pline(E_J("Such creatures %s exist in this world.",
				  "そのような存在は%sこの世界にいない。"),
			      (mndx == NON_PM) ? E_J("do not","") : E_J("no longer","もはや"));
			continue;
		}
		ptr = &mons[mndx];
		/* Although "genus" is Latin for race, the hero benefits
		 * from both race and role; thus genocide affects either.
		 */
		if (Your_Own_Role(mndx) || Your_Own_Race(mndx)) {
			killplayer++;
			break;
		}
		if (is_human(ptr)) adjalign(-sgn(u.ualign.type));
		if (is_demon(ptr)) adjalign(sgn(u.ualign.type));

		if(!(ptr->geno & G_GENO)) {
			if(flags.soundok) {
	/* fixme: unconditional "caverns" will be silly in some circumstances */
			    if(flags.verbose)
			pline(E_J("A thunderous voice booms through the caverns:",
				  "雷のような声が洞窟を貫いて響いた:"));
			    verbalize(E_J("No, mortal!  That will not be done.",
					  "\"止せ、定命の者よ！ それは為し得ない。"));
			}
			continue;
		}
		/* KMH -- Unchanging prevents rehumanization */
		if (Unchanging && ptr == youmonst.data)
		    killplayer++;
		break;
	    }
	}

	which = E_J("all ","すべての");
	if (Hallucination) {
	    if (Upolyd)
		Strcpy(buf,youmonst.data->mname);
	    else {
		Strcpy(buf, (flags.female && urole.name.f) ?
				urole.name.f : urole.name.m);
		buf[0] = lowc(buf[0]);
	    }
	} else {
	    Strcpy(buf, E_J(ptr->mname,JMONNAM(mndx))); /* make sure we have standard singular */
	    if ((ptr->geno & G_UNIQ) && ptr != &mons[PM_HIGH_PRIEST])
#ifndef JP
		which = !type_is_pname(ptr) ? "the " : "";
#else
		which = "";
#endif /*JP*/
	}
	if (how & REALLY) {
	    /* setting no-corpse affects wishing and random tin generation */
	    mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
#ifndef JP
	    pline("Wiped out %s%s.", which,
		  (*which != 'a') ? buf : makeplural(buf));
#else
	    pline("%s%sを一掃した。", which, buf);
#endif /*JP*/

	    if (killplayer) {
		/* might need to wipe out dual role */
		if (urole.femalenum != NON_PM && mndx == urole.malenum)
		    mvitals[urole.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
		if (urole.femalenum != NON_PM && mndx == urole.femalenum)
		    mvitals[urole.malenum].mvflags |= (G_GENOD | G_NOCORPSE);
		if (urace.femalenum != NON_PM && mndx == urace.malenum)
		    mvitals[urace.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
		if (urace.femalenum != NON_PM && mndx == urace.femalenum)
		    mvitals[urace.malenum].mvflags |= (G_GENOD | G_NOCORPSE);

		u.uhp = -1;
		if (how & PLAYER) {
		    killer_format = E_J(KILLED_BY_AN,KILLED_SUFFIX);
		    killer = E_J("genocidal confusion","虐殺上の手違いで");
		} else if (how & ONTHRONE) {
		    /* player selected while on a throne */
		    killer_format = E_J(KILLED_BY_AN,KILLED_SUFFIX);
		    killer = E_J("imperious order","横暴な命令で");
		} else { /* selected player deliberately, not confused */
		    killer_format = E_J(KILLED_BY_AN,KILLED_SUFFIX);
		    killer = E_J("scroll of genocide","虐殺の巻物に");
		}

	/* Polymorphed characters will die as soon as they're rehumanized. */
	/* KMH -- Unchanging prevents rehumanization */
		if (Upolyd && ptr != youmonst.data) {
			delayed_killer = killer;
			killer = 0;
			You_feel(E_J("dead inside.","己の魂が死んだのを感じた。"));
		} else
			done(GENOCIDED);
	    } else if (ptr == youmonst.data) {
		rehumanize();
	    }
	    reset_rndmonst(mndx);
	    kill_genocided_monsters();
	    update_inventory();	/* in case identified eggs were affected */
	} else {
	    int cnt = 0;

	    if (!(mons[mndx].geno & G_UNIQ) &&
		    !(mvitals[mndx].mvflags & (G_GENOD | G_EXTINCT)))
		for (i = rn1(3, 4); i > 0; i--) {
		    if (!rn2(100) &&		/* someting slips in... */
			!(mons[PM_CHAMELEON].geno & G_UNIQ) &&
			!(mvitals[PM_CHAMELEON].mvflags & (G_GENOD | G_EXTINCT))) {
			struct monst *mon;
			if (mon = makemon(&mons[PM_CHAMELEON], u.ux, u.uy, NO_MINVENT)) {
			    newcham(mon, ptr, FALSE, FALSE);
			    ++cnt;
			    continue;
			}
		    }
		    if (!makemon(ptr, u.ux, u.uy, NO_MINVENT))
			break;	/* couldn't make one */
		    ++cnt;
		    if (mvitals[mndx].mvflags & G_EXTINCT)
			break;	/* just made last one */
		}
	    if (cnt)
#ifndef JP
		pline("Sent in some %s.", makeplural(buf));
#else
		pline("%sの一団が送り込まれた。", buf);
#endif /*JP*/
	    else
		pline(nothing_happens);
	}
}

void
punish(sobj)
register struct obj	*sobj;
{
	/* KMH -- Punishment is still okay when you are riding */
	You(E_J("are being punished for your misbehavior!",
		"自らの不法のために罰せられた！"));
	if(Punished){
		Your(E_J("iron ball gets heavier.","鉄球はさらに重くなった。"));
		uball->owt += 160 * (1 + sobj->cursed);
		return;
	}
	if (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data)) {
		pline(E_J("A ball and chain appears, then falls away.",
			  "鉄球と鎖が現れたが、抜け落ちた。"));
		dropy(mkobj(BALL_CLASS, TRUE));
		return;
	}
	setworn(mksobj(IRON_CHAIN,	FALSE, FALSE), W_CHAIN);
	setworn(mksobj(HEAVY_IRON_BALL, FALSE, FALSE), W_BALL);
//	setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
//	setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
	uball->spe = 1;		/* special ball (see save) */

	/*
	 *  Place ball & chain if not swallowed.  If swallowed, the ball &
	 *  chain variables will be set at the next call to placebc().
	 */
	if (!u.uswallow) {
	    placebc();
	    if (Blind) set_bc(1);	/* set up ball and chain variables */
	    newsym(u.ux,u.uy);		/* see ball&chain if can't see self */
	}
}

void
unpunish()
{	    /* remove the ball and chain */
	struct obj *savechain = uchain;

	obj_extract_self(uchain);
	newsym(uchain->ox,uchain->oy);
	setworn((struct obj *)0, W_CHAIN);
	dealloc_obj(savechain);
	uball->spe = 0;
	setworn((struct obj *)0, W_BALL);
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to revive
 * one, the disoriented creature becomes a zombie
 */
boolean
cant_create(mtype, revival)
int *mtype;
boolean revival;
{

	/* SHOPKEEPERS can be revived now */
	if (*mtype==PM_GUARD || (*mtype==PM_SHOPKEEPER && !revival)
	     || *mtype==PM_ALIGNED_PRIEST || *mtype==PM_ANGEL) {
		*mtype = PM_HUMAN_ZOMBIE;
		return TRUE;
	} else if (*mtype==PM_LONG_WORM_TAIL) {	/* for create_particular() */
		*mtype = PM_LONG_WORM;
		return TRUE;
	}
	return FALSE;
}

#ifdef WIZARD
/*
 * Make a new monster with the type controlled by the user.
 *
 * Note:  when creating a monster by class letter, specifying the
 * "strange object" (']') symbol produces a random monster rather
 * than a mimic; this behavior quirk is useful so don't "fix" it...
 */
boolean
create_particular()
{
	char buf[BUFSZ], *bufp, monclass = MAXMCLASSES;
	int which, tries, i;
	struct permonst *whichpm;
	struct monst *mtmp;
	boolean madeany = FALSE;
	boolean maketame, makepeaceful, makehostile;

	tries = 0;
	do {
	    which = urole.malenum;	/* an arbitrary index into mons[] */
	    maketame = makepeaceful = makehostile = FALSE;
	    getlin("Create what kind of monster? [type the name or symbol]",
		   buf);
	    bufp = mungspaces(buf);
	    if (*bufp == '\033') return FALSE;
	    /* allow the initial disposition to be specified */
	    if (!strncmpi(bufp, "tame ", 5)) {
		bufp += 5;
		maketame = TRUE;
	    } else if (!strncmpi(bufp, "peaceful ", 9)) {
		bufp += 9;
		makepeaceful = TRUE;
	    } else if (!strncmpi(bufp, "hostile ", 8)) {
		bufp += 8;
		makehostile = TRUE;
	    }
	    /* decide whether a valid monster was chosen */
	    if (strlen(bufp) == 1) {
		monclass = def_char_to_monclass(*bufp);
		if (monclass != MAXMCLASSES) break;	/* got one */
	    } else {
		which = name_to_mon(bufp);
		if (which >= LOW_PM) break;		/* got one */
	    }
	    /* no good; try again... */
	    pline("I've never heard of such monsters.");
	} while (++tries < 5);

	if (tries == 5) {
	    pline(thats_enough_tries);
	} else {
	    (void) cant_create(&which, FALSE);
	    whichpm = &mons[which];
	    for (i = 0; i <= multi; i++) {
		if (monclass != MAXMCLASSES)
		    whichpm = mkclass(monclass, 0);
		if (maketame) {
		    mtmp = makemon(whichpm, u.ux, u.uy, MM_EDOG);
		    if (mtmp) {
			initedog(mtmp);
			set_malign(mtmp);
		    }
		} else {
		    mtmp = makemon(whichpm, u.ux, u.uy, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
		    if ((makepeaceful || makehostile) && mtmp) {
			mtmp->mtame = 0;	/* sanity precaution */
			setmpeaceful(mtmp, makepeaceful);
		    }
		}
		if (mtmp) madeany = TRUE;
	    }
	}
	return madeany;
}
#endif /* WIZARD */

#endif /* OVLB */

/*read.c*/
