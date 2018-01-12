/*	SCCS Id: @(#)weapon.c	3.4	2002/11/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *	This module contains code for calculation of "to hit" and damage
 *	bonuses for any given weapon used, as well as weapons selection
 *	code for monsters.
 */
#include "hack.h"

/* Categories whose names don't come from OBJ_NAME(objects[type])
 */
#define PN_BARE_HANDED			(-1)	/* includes martial arts */
#define PN_TWO_WEAPONS			(-2)
#define PN_RIDING			(-3)
#define PN_POLEARMS			(-4)
#define PN_SABER			(-5)
#define PN_HAMMER			(-6)
#define PN_WHIP				(-7)
#define PN_ATTACK_SPELL			(-8)
#define PN_HEALING_SPELL		(-9)
#define PN_DIVINATION_SPELL		(-10)
#define PN_ENCHANTMENT_SPELL		(-11)
#define PN_CLERIC_SPELL			(-12)
#define PN_ESCAPE_SPELL			(-13)
#define PN_MATTER_SPELL			(-14)

STATIC_DCL void FDECL(give_may_advance_msg, (int));

#ifndef OVLB

STATIC_DCL NEARDATA const short skill_names_indices[];
STATIC_DCL NEARDATA const char *odd_skill_names[];
STATIC_DCL NEARDATA const char *barehands_or_martial[];

#else	/* OVLB */

struct skill_name {
	int        num;
	const char *name;
} skill_names[] = {
	{ P_DAGGER_GROUP,	E_J("dagger group",		"短剣") },
	{ P_KNIFE_GROUP,	E_J("knife group",		"ナイフ") },
	{ P_AXE_GROUP,		E_J("axe group",		"斧") },
	{ P_SHORT_BLADE_GROUP,	E_J("short blade group",	"ショートソード") },
	{ P_SABER_GROUP,	E_J("saber group",		"サーベル") },
	{ P_SABER_GROUP,	E_J("katana group",		"カタナ") },
/*	{ P_BROAD_BLADE_GROUP,	"broad blade group" },*/
	{ P_LONG_BLADE_GROUP,	E_J("long blade group",		"ロングソード") },
	{ P_CRUSHING_GROUP,	E_J("crushing group",		"鈍器") },
	{ P_FLAIL_GROUP,	E_J("flail group",		"フレイル") },
	{ P_STAFF_GROUP,	E_J("staff group",		"棒術") },
	{ P_POLEARM_GROUP,	E_J("polearm group",		"長柄武器") },
	{ P_SPEAR_GROUP,	E_J("spear group",		"槍") },
	{ P_BOW_GROUP,		E_J("bow group",		"弓") },
	{ P_FIREARM_GROUP,	E_J("firearm group",		"銃火器") },
	{ P_THROWING_GROUP,	E_J("throwing group",		"投擲") },
	{ P_WHIP_GROUP,		E_J("whip group",		"鞭") },
	{ P_PICKAXE_GROUP,	E_J("pick-axe group",		"つるはし") },
	{ P_ATTACK_SPELL,	E_J("attack spell",		"攻撃") },
	{ P_HEALING_SPELL,	E_J("healing spell",		"治癒") },
	{ P_DIVINATION_SPELL,	E_J("divination spell",		"予知") },
	{ P_ENCHANTMENT_SPELL,	E_J("enchantment spell",	"補助") },
	{ P_CLERIC_SPELL,	E_J("cleric spell",		"僧侶") },
	{ P_ESCAPE_SPELL,	E_J("escape spell",		"脱出") },
	{ P_MATTER_SPELL,	E_J("matter spell",		"物質") },
	{ P_BARE_HANDED_COMBAT,	E_J("bare handed combat",	"素手での戦闘") },
	{ P_BARE_HANDED_COMBAT,	E_J("martial arts",		"マーシャルアーツ") },
	{ P_TWO_WEAPON_COMBAT,	E_J("two weapon combat",	"二刀流") },
#ifdef STEED
	{ P_RIDING,		E_J("riding",			"乗馬") },
#endif
	{ P_NONE,		"no skill" }
};

STATIC_VAR NEARDATA const short skill_names_indices[P_NUM_SKILLS] = {
	0,                DAGGER,         KNIFE,        AXE,
	PICK_AXE,         SHORT_SWORD,    BROADSWORD,   LONG_SWORD,
	TWO_HANDED_SWORD, SCIMITAR,       PN_SABER,     CLUB,
	MACE,             MORNING_STAR,   FLAIL,
	PN_HAMMER,        QUARTERSTAFF,   PN_POLEARMS,  SPEAR,
	JAVELIN,          TRIDENT,        LANCE,        BOW,
	SLING,            CROSSBOW,       DART,
	SHURIKEN,         BOOMERANG,      PN_WHIP,      UNICORN_HORN,
	PN_ATTACK_SPELL,     PN_HEALING_SPELL,
	PN_DIVINATION_SPELL, PN_ENCHANTMENT_SPELL,
	PN_CLERIC_SPELL,     PN_ESCAPE_SPELL,
	PN_MATTER_SPELL,
	PN_BARE_HANDED,   PN_TWO_WEAPONS,
#ifdef STEED
	PN_RIDING
#endif
};

/* note: entry [0] isn't used */
STATIC_VAR NEARDATA const char * const odd_skill_names[] = {
    "no skill",
    "bare hands",		/* use barehands_or_martial[] instead */
    "two weapon combat",
    "riding",
    "polearms",
    "saber",
    "hammer",
    "whip",
    "attack spells",
    "healing spells",
    "divination spells",
    "enchantment spells",
    "clerical spells",
    "escape spells",
    "matter spells",
};
/* indexed vis `is_martial() */
STATIC_VAR NEARDATA const char * const barehands_or_martial[] = {
    "bare handed combat", "martial arts"
};

STATIC_OVL void
give_may_advance_msg(skill)
int skill;
{
	You_feel("more confident in your %sskills.",
		skill == P_NONE ?
			"" :
		skill <= P_LAST_WEAPON ?
			"weapon " :
		skill <= P_LAST_SPELL ?
			"spell casting " :
		"fighting ");
}

#endif	/* OVLB */

STATIC_DCL boolean FDECL(can_advance, (int, BOOLEAN_P));
STATIC_DCL boolean FDECL(could_advance, (int));
STATIC_DCL boolean FDECL(peaked_skill, (int));
STATIC_DCL int FDECL(slots_required, (int));
STATIC_DCL const char* FDECL(get_skill_name, (int));

#ifdef OVL1

STATIC_DCL char *FDECL(skill_level_name, (int,char *));
STATIC_DCL void FDECL(skill_advance, (int));

#endif	/* OVL1 */

const char *
get_skill_name(type)
int type;
{
	struct skill_name *sn;

	for ( sn = skill_names; sn->num; sn++ ) {
	    if (sn->num == type) {
		if ((type == P_BARE_HANDED_COMBAT && martial_bonus()) ||
		    (type == P_SABER_GROUP && Role_if(PM_SAMURAI))) sn++;
		return sn->name;
	    }
	}
	return (char *)0;
}

/*#define P_NAME(type) ((skill_names_indices[type] > 0) ? \
		      OBJ_NAME(objects[skill_names_indices[type]]) : \
		      (type == P_BARE_HANDED_COMBAT) ? \
			barehands_or_martial[martial_bonus()] : \
			odd_skill_names[-skill_names_indices[type]])*/
#define P_NAME(type) get_skill_name(type)

#ifdef OVLB
static NEARDATA const char kebabable[] = {
	S_XORN, S_DRAGON, S_JABBERWOCK, S_NAGA, S_GIANT, '\0'
};

/*
 *	hitval returns an integer representing the "to hit" bonuses
 *	of "otmp" against the monster.
 */
int
hitval(otmp, mon)
struct obj *otmp;
struct monst *mon;
{
	int	tmp = 0;
	struct permonst *ptr = mon->data;
	boolean Is_weapon = (otmp->oclass == WEAPON_CLASS || is_weptool(otmp));

	if (Is_weapon)
		tmp += otmp->spe;

/*	Put weapon specific "to hit" bonuses in below:		*/
	tmp += (schar)(objects[otmp->otyp].oc_hitbon);

/*	Put weapon vs. monster type "to hit" bonuses in below:	*/

	/* Blessed weapons used against undead or demons */
	if (Is_weapon && otmp->blessed &&
	   (is_demon(ptr) || is_undead(ptr))) tmp += 2;

	if (is_spear(otmp) &&
	   index(kebabable, ptr->mlet)) tmp += 2;

	/* trident is highly effective against swimmers */
	if (otmp->otyp == TRIDENT && is_swimmer(ptr)) {
	   if (is_pool(mon->mx, mon->my)) tmp += 4;
	   else if (ptr->mlet == S_EEL || ptr->mlet == S_SNAKE) tmp += 2;
	}

	/* Picks used against xorns and earth elementals */
	if (is_pick(otmp) &&
	   (passes_walls(ptr) && thick_skinned(ptr))) tmp += 2;

#ifdef INVISIBLE_OBJECTS
	/* Invisible weapons against monsters who can't see invisible */
	if (otmp->oinvis && !perceives(ptr)) tmp += 3;
#endif

	/* Check specially named weapon "to hit" bonuses */
	if (otmp->oartifact) tmp += spec_abon(otmp, mon);

	return tmp;
}

/* Historical note: The original versions of Hack used a range of damage
 * which was similar to, but not identical to the damage used in Advanced
 * Dungeons and Dragons.  I figured that since it was so close, I may as well
 * make it exactly the same as AD&D, adding some more weapons in the process.
 * This has the advantage that it is at least possible that the player would
 * already know the damage of at least some of the weapons.  This was circa
 * 1987 and I used the table from Unearthed Arcana until I got tired of typing
 * them in (leading to something of an imbalance towards weapons early in
 * alphabetical order).  The data structure still doesn't include fields that
 * fully allow the appropriate damage to be described (there's no way to say
 * 3d6 or 1d6+1) so we add on the extra damage in dmgval() if the weapon
 * doesn't do an exact die of damage.
 *
 * Of course new weapons were added later in the development of Nethack.  No
 * AD&D consistency was kept, but most of these don't exist in AD&D anyway.
 *
 * Second edition AD&D came out a few years later; luckily it used the same
 * table.  As of this writing (1999), third edition is in progress but not
 * released.  Let's see if the weapon table stays the same.  --KAA
 * October 2000: It didn't.  Oh, well.
 */

/*
 *	dmgval returns an integer representing the damage bonuses
 *	of "otmp" against the monster.
 */
int
dmgval(otmp, mon)
struct obj *otmp;
struct monst *mon;
{
	int num,die,bon;
	int tmp = 0, otyp = otmp->otyp;
	int mat = get_material(otmp);
	struct permonst *ptr = mon->data;
	boolean Is_weapon = (otmp->oclass == WEAPON_CLASS || is_weptool(otmp));

	if (otyp == CREAM_PIE) return 0;

	if (bigmonst(ptr)) {
	    if (tmp = objects[otyp].oc_wldam) {
		num = (tmp>>5) & 0x07;				/* Bit[7-5] */
		die = tmp & 0x1f;				/* Bit[4-0] */
		bon = (objects[otyp].oc_dambon) & 0x0f;		/* Bit[3-0] */
		tmp = d(num,die)+bon;
	    }
	    switch (otyp) {
		case TSURUGI:
		case DWARVISH_MATTOCK:	tmp += d(2,6); break;
	    }
	} else {
	    if (tmp = objects[otyp].oc_wsdam) {
		num = (tmp>>5) & 0x07;				/* Bit[7-5] */
		die = tmp & 0x1f;				/* Bit[4-0] */
		bon = (objects[otyp].oc_dambon >> 4) & 0x0f;	/* Bit[7-4] */
		tmp = d(num,die)+bon;
	    }
	}

	if (otyp == CLUB && mat != WOOD) tmp++;

	if (Is_weapon) {
		tmp += otmp->spe;
		/* negative enchantment mustn't produce negative damage */
		if (tmp < 0) tmp = 0;
	}

	if (is_flimsy(otmp) && thick_skinned(ptr))
		/* thick skinned/scaled creatures don't feel it */
		tmp = 0;
	if (ptr == &mons[PM_SHADE] && mat != SILVER)
		tmp = 0;

	/* "very heavy iron ball"; weight increase is in increments of 160 */
	if (otyp == HEAVY_IRON_BALL && tmp > 0) {
	    int wt = (int)objects[HEAVY_IRON_BALL].oc_weight;

	    if ((int)otmp->owt > wt) {
		wt = ((int)otmp->owt - wt) / 160;
		tmp += rnd(4 * wt);
		if (tmp > 25) tmp = 25;	/* objects[].oc_wldam */
	    }
	}

/*	Put weapon vs. monster type damage bonuses in below:	*/
	if (Is_weapon || otmp->oclass == GEM_CLASS ||
		otmp->oclass == BALL_CLASS || otmp->oclass == CHAIN_CLASS) {
	    int bonus = 0;

	    if (otmp->blessed && (is_undead(ptr) || is_demon(ptr)))
		bonus += rnd(4);
	    if (is_axe(otmp) && is_wooden(ptr))
		bonus += rnd(4);
	    if (mat == SILVER && hates_silver(ptr))
		bonus += rnd(20);

	    /* if the weapon is going to get a double damage bonus, adjust
	       this bonus so that effectively it's added after the doubling */
	    if (bonus > 1 && otmp->oartifact && spec_dbon(otmp, mon, 25) >= 25)
		bonus = (bonus + 1) / 2;

	    tmp += bonus;
	}

	if (tmp > 0) {
		/* It's debateable whether a rusted blunt instrument
		   should do less damage than a pristine one, since
		   it will hit with essentially the same impact, but
		   there ought to some penalty for using damaged gear
		   so always subtract erosion even for blunt weapons. */
		tmp -= greatest_erosion(otmp);
		if (tmp < 1) tmp = 1;
	}

	return(tmp);
}

int
doubleattack_roll()
{
	int roll = -5;

	if (u.twoweap || near_capacity() != UNENCUMBERED) return 0;
	if (!uwep) {
		if (uarms) return 0;
		if (!martial_bonus()) roll -= 5;
	} else {
		int w;
		w = weight(uwep) / 5;		/* knife: -1, long sword: -10, ... */
		if (ACURR(A_STR) >= 118)	/* high Str reduces weapon-weight penalty */
		    w = w*(125-ACURR(A_STR))/8;	
		roll -= w;
	}
	roll += abon();				/* maybe up to +10 if highly-trained */
	if (uarms) roll -= weight(uarms)/10;	/* small shield: -3, shield of reflection: -5, ... */
	roll += weapon_hit_bonus(uwep);		/* maybe up to +5(weapon skill) or +7(martial arts skill) */
	roll += u.uspdbon1;			/* speed bonus by light-weight */
	roll += u.uspdbon2;			/* speed bonus by potion/wand/spell */
	if (uwep && uwep->oartifact == ART_QUICK_BLADE)
		roll = (roll+5 < 10) ? 10 : (roll+5);	/* Quick Blade is really quick! */
#ifdef WIZARD
	if (wizard) pline("[x2:%d]", roll);
#endif /* WIZARD */
	return (roll < 0) ? 0 : roll;
}


#endif /* OVLB */
#ifdef OVL0

STATIC_DCL struct obj *FDECL(oselect, (struct monst *,int));
#define Oselect(x)	if ((otmp = oselect(mtmp, x)) != 0) return(otmp);

STATIC_OVL struct obj *
oselect(mtmp, x)
struct monst *mtmp;
int x;
{
	struct obj *otmp;

	for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
	    if (otmp->otyp == x &&
		    /* never select non-cockatrice corpses */
		    !((x == CORPSE || x == EGG) &&
			!touch_petrifies(&mons[otmp->corpsenm])) &&
		    (!otmp->oartifact || touch_artifact(otmp,mtmp)))
		return otmp;
	}
	return (struct obj *)0;
}

static NEARDATA const int rwep[] =
{	REVOLVER, MUSKET,
	DWARVISH_SPEAR, ELVEN_SPEAR, SPEAR, ORCISH_SPEAR, JAVELIN,
	SHURIKEN, YA, ELVEN_ARROW, ARROW, ORCISH_ARROW, CROSSBOW_BOLT,
	ELVEN_DAGGER, DAGGER, ORCISH_DAGGER, KNIFE,
	FLINT, ROCK, LOADSTONE, LUCKSTONE, DART,
	/* BOOMERANG, */ CREAM_PIE
	/* note: CREAM_PIE should NOT be #ifdef KOPS */
};

/*static NEARDATA const int pwep[] =
{	HALBERD, VOULGE, BARDICHE, LUCERN_HAMMER, SPETUM, BILL_GUISARME, MORNING_STAR,
	GLAIVE, BEC_DE_CORBIN, PARTISAN, RANSEUR, GUISARME, FAUCHARD
};*/
static NEARDATA const int pwep[] = {
	HALBERD, VOULGE, BARDICHE, LUCERN_HAMMER, MORNING_STAR, 
	SPETUM, GLAIVE, BILL_GUISARME, PARTISAN, RANSEUR, BEC_DE_CORBIN, 
	GUISARME, FAUCHARD, GRAPPLING_HOOK
};

static struct obj *propellor;

struct obj *
select_rwep(mtmp)	/* select a ranged weapon for the monster */
register struct monst *mtmp;
{
	register struct obj *otmp;
	int i;

#ifdef KOPS
	char mlet = mtmp->data->mlet;
#endif

	propellor = &zeroobj;
	Oselect(EGG); /* cockatrice egg */
#ifdef KOPS
	if(mlet == S_KOP)	/* pies are first choice for Kops */
	    Oselect(CREAM_PIE);
#endif
	if(throws_rocks(mtmp->data))	/* ...boulders for giants */
	    Oselect(BOULDER);

	/* Select polearms first; they do more damage and aren't expendable */
	/* The limit of 13 here is based on the monster polearm range limit
	 * (defined as 5 in mthrowu.c).  5 corresponds to a distance of 2 in
	 * one direction and 1 in another; one space beyond that would be 3 in
	 * one direction and 2 in another; 3^2+2^2=13.
	 */
	if (dist2(mtmp->mx, mtmp->my, mtmp->mux, mtmp->muy) <= 13 && couldsee(mtmp->mx, mtmp->my)) {
	    for (i = 0; i < SIZE(pwep); i++) {
		/* Only strong monsters can wield big (esp. long) weapons.
		 * Big weapon is basically the same as bimanual.
		 * All monsters can wield the remaining weapons.
		 */
		if ((strongmonst(mtmp->data) && (mtmp->misc_worn_check & W_ARMS) == 0)
			|| !objects[pwep[i]].oc_bimanual) {
		    if ((otmp = oselect(mtmp, pwep[i])) != 0) {
			if (get_material(otmp) != SILVER || !hates_silver(mtmp->data)) {
			    propellor = otmp; /* force the monster to wield it */
			    return otmp;
			}
		    }
		}
	    }
	}

	/*
	 * other than these two specific cases, always select the
	 * most potent ranged weapon to hand.
	 */
	for (i = 0; i < SIZE(rwep); i++) {

	    /* firearms */
	    if (rwep[i] == MUSKET || rwep[i] == REVOLVER) {
		if (can_use_gun(mtmp->data) &&
		    ((strongmonst(mtmp->data) && (mtmp->misc_worn_check & W_ARMS) == 0)
			|| !objects[pwep[i]].oc_bimanual) &&
		    (otmp = oselect(mtmp, rwep[i])) &&
		    (otmp->cobj || m_carrying(mtmp, BULLET))) {
			propellor = otmp;
			return otmp;
		}
	    }

	    /* shooting gems from slings; this goes just before the darts */
	    /* (shooting rocks is already handled via the rwep[] ordering) */
	    if (rwep[i] == DART && !likes_gems(mtmp->data) &&
		    m_carrying(mtmp, SLING)) {		/* propellor */
		for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		    if (otmp->oclass == GEM_CLASS &&
			    (otmp->otyp != LOADSTONE || !otmp->cursed)) {
			propellor = m_carrying(mtmp, SLING);
			return otmp;
		    }
	    }

		/* KMH -- This belongs here so darts will work */
	    propellor = &zeroobj;

	    if ((objects[rwep[i]].oc_wprop & WP_WEAPONTYPE) == WP_AMMUNITION) {
		switch (objects[rwep[i]].oc_wprop & WP_SUBTYPE) {
		case WP_BULLET:
		  if (!can_use_gun(mtmp->data)) break;
		  propellor = (oselect(mtmp, REVOLVER));
		  if (!propellor) propellor = (oselect(mtmp, MUSKET));
		  break;
		case WP_ARROW:
		  propellor = (oselect(mtmp, YUMI));
		  if (!propellor) propellor = (oselect(mtmp, ELVEN_BOW));
		  if (!propellor) propellor = (oselect(mtmp, BOW));
		  if (!propellor) propellor = (oselect(mtmp, ORCISH_BOW));
		  break;
		case WP_STONE:	/* sling */
		  propellor = (oselect(mtmp, SLING));
		  break;
		case WP_BOLT:
		  propellor = (oselect(mtmp, CROSSBOW));
		}
		if ((otmp = MON_WEP(mtmp)) && otmp->cursed && otmp != propellor
				&& mtmp->weapon_check == NO_WEAPON_WANTED)
			propellor = 0;
	    }
	    /* propellor = obj, propellor to use
	     * propellor = &zeroobj, doesn't need a propellor
	     * propellor = 0, needed one and didn't have one
	     */
	    if (propellor != 0) {
		/* Note: cannot use m_carrying for loadstones, since it will
		 * always select the first object of a type, and maybe the
		 * monster is carrying two but only the first is unthrowable.
		 */
		if (rwep[i] != LOADSTONE) {
			/* Don't throw a cursed weapon-in-hand or an artifact */
			if ((otmp = oselect(mtmp, rwep[i])) && !otmp->oartifact
			    && (!otmp->cursed || otmp != MON_WEP(mtmp)))
				return(otmp);
		} else for(otmp=mtmp->minvent; otmp; otmp=otmp->nobj) {
		    if (otmp->otyp == LOADSTONE && !otmp->cursed)
			return otmp;
		}
	    }
	  }

	/* failure */
	return (struct obj *)0;
}

/* Weapons in order of preference */
//static NEARDATA short hwep[] = {
//	  CORPSE,  /* cockatrice corpse */
//	  TSURUGI, RUNESWORD, DWARVISH_MATTOCK, TWO_HANDED_SWORD, BATTLE_AXE,
//	  KATANA, UNICORN_HORN, CRYSKNIFE, TRIDENT, ELVEN_BROADSWORD, BROADSWORD,
//	  MORNING_STAR, LONG_SWORD, WAR_HAMMER, SCIMITAR, SABER,
//	  ELVEN_SHORT_SWORD, DWARVISH_SHORT_SWORD, SHORT_SWORD,
//	  ORCISH_SHORT_SWORD, MACE, AXE, DWARVISH_SPEAR, /*SILVER_SPEAR,*/
//	  ELVEN_SPEAR, SPEAR, ORCISH_SPEAR, FLAIL, BULLWHIP, QUARTERSTAFF,
//	  JAVELIN, AKLYS, CLUB, PICK_AXE,
//#ifdef KOPS
//	  RUBBER_HOSE,
//#endif /* KOPS */
//	  /*SILVER_DAGGER,*/ ELVEN_DAGGER, DAGGER, ORCISH_DAGGER,
//	  ATHAME, SCALPEL, KNIFE, WORM_TOOTH
//	};
static const NEARDATA short hwep[] = {
	CORPSE,	/* cockatrice corpse */
	TSURUGI, DWARVISH_MATTOCK, TWO_HANDED_SWORD, HALBERD,
	BATTLE_AXE, ELVEN_BROADSWORD, RUNESWORD, VOULGE, BARDICHE,
	TRIDENT, LUCERN_HAMMER, MORNING_STAR, SPETUM, BROADSWORD,
	GLAIVE, UNICORN_HORN, KATANA, LONG_SWORD, BILL_GUISARME,
	PARTISAN, CRYSKNIFE, LANCE, RANSEUR, FLAIL, AXE, BEC_DE_CORBIN,
	GUISARME, FAUCHARD, HEAVY_HAMMER, WAR_HAMMER, ELVEN_SHORT_SWORD,
	SCIMITAR, SABER, DWARVISH_SPEAR, ELVEN_SPEAR, DWARVISH_SHORT_SWORD,
	SPEAR, SHORT_SWORD, ORCISH_SHORT_SWORD, ORCISH_SPEAR, BULLWHIP,
	GRAPPLING_HOOK, MACE, JAVELIN, QUARTERSTAFF, PICK_AXE,
	CLUB, AKLYS, ELVEN_DAGGER, DAGGER, ATHAME,
#ifdef KOPS
	RUBBER_HOSE,
#endif /* KOPS */
	ORCISH_DAGGER, SCALPEL, STILETTO, KNIFE, WORM_TOOTH
};

struct obj *
select_hwep(mtmp)	/* select a hand to hand weapon for the monster */
register struct monst *mtmp;
{
	register struct obj *otmp;
	register int i;
	boolean strong = strongmonst(mtmp->data);
	boolean wearing_shield = (mtmp->misc_worn_check & W_ARMS) != 0;

	/* prefer artifacts to everything else */
	for(otmp=mtmp->minvent; otmp; otmp = otmp->nobj) {
		if (otmp->oclass == WEAPON_CLASS
			&& otmp->oartifact && touch_artifact(otmp,mtmp)
			&& ((strong && !wearing_shield)
			    || !objects[otmp->otyp].oc_bimanual))
		    return otmp;
	}

	if(is_giant(mtmp->data))	/* giants just love to use clubs */
	    Oselect(CLUB);

	/* only strong monsters can wield big (esp. long) weapons */
	/* big weapon is basically the same as bimanual */
	/* all monsters can wield the remaining weapons */
	for (i = 0; i < SIZE(hwep); i++) {
	    if (hwep[i] == CORPSE && !(mtmp->misc_worn_check & W_ARMG))
		continue;
	    if ((strong && !wearing_shield)
			|| !objects[hwep[i]].oc_bimanual) {
		otmp = oselect(mtmp, hwep[i]);
		if ( otmp &&
			(get_material(otmp) != SILVER ||
			 !hates_silver(mtmp->data) ) ) return otmp;
	    }
	}

	/* failure */
	return (struct obj *)0;
}

/* Called after polymorphing a monster, robbing it, etc....  Monsters
 * otherwise never unwield stuff on their own.  Might print message.
 */
void
possibly_unwield(mon, polyspot)
struct monst *mon;
boolean polyspot;
{
	struct obj *obj, *mw_tmp;

	if (!(mw_tmp = MON_WEP(mon)))
		return;
	for (obj = mon->minvent; obj; obj = obj->nobj)
		if (obj == mw_tmp) break;
	if (!obj) { /* The weapon was stolen or destroyed */
		MON_NOWEP(mon);
		mon->weapon_check = NEED_WEAPON;
		return;
	}
	if (!attacktype(mon->data, AT_WEAP)) {
		setmnotwielded(mon, mw_tmp);
		MON_NOWEP(mon);
		mon->weapon_check = NO_WEAPON_WANTED;
		obj_extract_self(obj);
		if (cansee(mon->mx, mon->my)) {
		    pline(E_J("%s drops %s.","%sは%sを落とした。"),
			  Monnam(mon), distant_name(obj, doname));
		    newsym(mon->mx, mon->my);
		}
		/* might be dropping object into water or lava */
		if (!flooreffects(obj, mon->mx, mon->my, E_J("drop","落ちた"))) {
		    if (polyspot) bypass_obj(obj);
		    place_object(obj, mon->mx, mon->my);
		    stackobj(obj);
		}
		return;
	}
	/* The remaining case where there is a change is where a monster
	 * is polymorphed into a stronger/weaker monster with a different
	 * choice of weapons.  This has no parallel for players.  It can
	 * be handled by waiting until mon_wield_item is actually called.
	 * Though the monster still wields the wrong weapon until then,
	 * this is OK since the player can't see it.  (FIXME: Not okay since
	 * probing can reveal it.)
	 * Note that if there is no change, setting the check to NEED_WEAPON
	 * is harmless.
	 * Possible problem: big monster with big cursed weapon gets
	 * polymorphed into little monster.  But it's not quite clear how to
	 * handle this anyway....
	 */
	if (!(mw_tmp->cursed && mon->weapon_check == NO_WEAPON_WANTED))
	    mon->weapon_check = NEED_WEAPON;
	return;
}

/* Let a monster try to wield a weapon, based on mon->weapon_check.
 * Returns 1 if the monster took time to do it, 0 if it did not.
 */
int
mon_wield_item(mon)
register struct monst *mon;
{
	struct obj *obj;
	boolean vis = canseemon(mon) && !is_swallowed(mon);

	/* This case actually should never happen */
	if (mon->weapon_check == NO_WEAPON_WANTED) return 0;
	switch(mon->weapon_check) {
		case NEED_HTH_WEAPON:
			obj = select_hwep(mon);
			break;
		case NEED_RANGED_WEAPON:
			(void)select_rwep(mon);
			obj = propellor;
			break;
		case NEED_PICK_AXE:
			obj = m_carrying(mon, PICK_AXE);
			/* KMH -- allow other picks */
			if (!obj && !which_armor(mon, W_ARMS))
			    obj = m_carrying(mon, DWARVISH_MATTOCK);
			break;
		case NEED_AXE:
			/* currently, only 2 types of axe */
			obj = m_carrying(mon, BATTLE_AXE);
			if (!obj || which_armor(mon, W_ARMS))
			    obj = m_carrying(mon, AXE);
			break;
		case NEED_PICK_OR_AXE:
			/* prefer pick for fewer switches on most levels */
			obj = m_carrying(mon, DWARVISH_MATTOCK);
			if (!obj) obj = m_carrying(mon, BATTLE_AXE);
			if (!obj || which_armor(mon, W_ARMS)) {
			    obj = m_carrying(mon, PICK_AXE);
			    if (!obj) obj = m_carrying(mon, AXE);
			}
			break;
		default: impossible("weapon_check %d for %s?",
				mon->weapon_check, mon_nam(mon));
			return 0;
	}
	if (obj && obj != &zeroobj) {
		struct obj *mw_tmp = MON_WEP(mon);
		if (mw_tmp && mw_tmp->otyp == obj->otyp) {
		/* already wielding it */
			mon->weapon_check = NEED_WEAPON;
			return 0;
		}
		/* Actually, this isn't necessary--as soon as the monster
		 * wields the weapon, the weapon welds itself, so the monster
		 * can know it's cursed and needn't even bother trying.
		 * Still....
		 */
		if (mw_tmp && mw_tmp->cursed && mw_tmp->otyp != CORPSE) {
		    if (vis) {
			char welded_buf[BUFSZ];
			const char *mon_hand = mbodypart(mon, HAND);

#ifndef JP
			if (bimanual(mw_tmp)) mon_hand = makeplural(mon_hand);
			Sprintf(welded_buf, "%s welded to %s %s",
				otense(mw_tmp, "are"),
				mhis(mon), mon_hand);
#endif /*JP*/

			if (obj->otyp == PICK_AXE) {
#ifndef JP
			    pline("Since %s weapon%s %s,",
				  s_suffix(mon_nam(mon)),
				  plur(mw_tmp->quan), welded_buf);
			    pline("%s cannot wield that %s.",
				mon_nam(mon), xname(obj));
#else
			    pline("武器が%sから離れないため、%sは%sを装備できない。",
				  mon_hand, mon_nam(mon), xname(obj));
#endif /*JP*/
			} else {
			    pline(E_J("%s tries to wield %s.","%sは%sを構えようとした。"),
				  Monnam(mon), doname(obj));
#ifndef JP
			    pline("%s %s %s!",
				  s_suffix(Monnam(mon)),
				  xname(mw_tmp), welded_buf);
#else
			    pline("%sの%sは%sから離れない！",
				  mon_nam(mon), xname(mw_tmp), mon_hand);
#endif /*JP*/
			}
			mw_tmp->bknown = 1;
		    }
		    mon->weapon_check = NO_WEAPON_WANTED;
		    return 1;
		}
		mon->mw = obj;		/* wield obj */
		setmnotwielded(mon, mw_tmp);
		mon->weapon_check = NEED_WEAPON;
		if (vis) {
		    pline(E_J("%s wields %s!","%sは%sを装備した！"), Monnam(mon), doname(obj));
		    if (obj->cursed && obj->otyp != CORPSE) {
#ifndef JP
			pline("%s %s to %s %s!",
			    Tobjnam(obj, "weld"),
			    is_plural(obj) ? "themselves" : "itself",
			    s_suffix(mon_nam(mon)), mbodypart(mon,HAND));
#else
			pline("%sは%sの%sに貼りついた！",
				xname(obj), mon_nam(mon), mbodypart(mon,HAND));
#endif /*JP*/
			obj->bknown = 1;
		    }
		}
		if (artifact_light(obj) && !obj->lamplit) {
		    begin_burn(obj, FALSE);
		    if (vis)
#ifndef JP
			pline("%s brilliantly in %s %s!",
			    Tobjnam(obj, "glow"), 
			    s_suffix(mon_nam(mon)), mbodypart(mon,HAND));
#else
			pline("%sは%sの%sでまぶしく輝きはじめた！",
			    xname(obj),  mon_nam(mon), mbodypart(mon,HAND));
#endif /*JP*/
		}
		obj->owornmask = W_WEP;
		return 1;
	}
	mon->weapon_check = NEED_WEAPON;
	return 0;
}

int
abon_luck()
{
	int l = Luck;
	return  (l >= LUCKMAX) ? +3 :
		(l >=  5     ) ? +2 :
		(l >   0     ) ? +1 :
		(l ==  0     ) ?  0 :
		(l >  -5     ) ? -1 :
		(l >  LUCKMIN) ? -2 : -3;
}

int
abon_dex()
{
	const schar dexbon[23] = {	/* dexterity bonus */
		-3, -2, -2, -1,		/*  3- 6 */
		-1,  0,  0,  0,		/*  7-10 */
		 0,  0,  0, +1,		/* 11-14 */
		+1, +1, +1, +2,		/* 15-18 */
		+2, +3, +3, +4,		/* 19-22 */
		+4, +5, +7		/* 23-25 */
	};
	return ((int)dexbon[ACURR(A_DEX)-3]);
}

int
abon()		/* attack bonus for strength & dexterity */
{
	int sbon;
	int str = ACURR(A_STR);

	if (Upolyd) return(adj_lev(&mons[u.umonnum]) - 3);
	if (str < 6) sbon = -2;
	else if (str < 8) sbon = -1;
	else if (str < 17) sbon = 0;
	else if (str <= STR18(50)) sbon = 1;	/* up to 18/50 */
	else if (str < STR18(100)) sbon = 2;
	else sbon = 3;

/* Game tuning kludge: make it a bit easier for a low level character to hit */
//	sbon += (u.ulevel < 3) ? 1 : 0;

	sbon += abon_dex();

	return sbon;
}

#endif /* OVL0 */
#ifdef OVL1

int
dbon()		/* damage bonus for strength */
{
	int str = ACURR(A_STR);
	int tmp = 0;

	if (Upolyd) return(0);

	/* two weapons are a bit heavier... */
	if (u.twoweap) tmp = -2;

	if	(str <   6) return (tmp-2);	/*     3 - 5	 */
	else if (str <  12) return (tmp-1);	/*     6 - 11	 */
	else if (str <  18) return (tmp+0);	/*    12 - 17	 */
	else if (str <  68) return (tmp+1);	/*    18 - 18/49 */
	else if (str < 118) return (tmp+2);	/* 18/50 - 18/99 */
	else if (str < 120) return (tmp+3);	/* 18/** - 19	 */
	else if (str < 125) return (tmp+4);	/*    20 - 24	 */
	else		    return (tmp+5);	/*    25	 */

}

/* copy the skill level name into the given buffer */
STATIC_OVL char *
skill_level_name(skill, buf)
int skill;
char *buf;
{
    const char *ptr;
    int s = P_SKILL(skill);

    ptr = (s < P_BASIC   ) ? "Unskilled" :
	  (s < P_SKILLED ) ? "Basic" :
	  (s < P_EXPERT  ) ? "Skilled" :
	  (s < P_MASTER  ) ? "Expert" :
	  (s < P_GRAND_MASTER  ) ? "Master" :
	  "Grand Master";
    Strcpy(buf, ptr);
    return buf;
}

/* return the # of slots required to advance the skill */
STATIC_OVL int
slots_required(skill)
int skill;
{
    int tmp = P_SKILL(skill);

    /* The more difficult the training, the more slots it takes.
     *	unskilled -> basic	1
     *	basic -> skilled	2
     *	skilled -> expert	3
     */
    if (skill <= P_LAST_WEAPON || skill == P_TWO_WEAPON_COMBAT)
	return tmp;

    /* Fewer slots used up for unarmed or martial.
     *	unskilled -> basic	1
     *	basic -> skilled	1
     *	skilled -> expert	2
     *	expert -> master	2
     *	master -> grand master	3
     */
    return (tmp + 1) / 2;
}

const static struct skill_range {
	short first, last;
	const char *name;
} skill_ranges[] = {
    { P_FIRST_H_TO_H, P_LAST_H_TO_H, E_J("Fighting Skills",	"戦闘技能\0" ) },
    { P_FIRST_WEAPON, P_LAST_WEAPON, E_J("Weapon Skills",	"武器技能\0" ) },
    { P_FIRST_SPELL,  P_LAST_SPELL,  E_J("Spellcasting Skills",	"呪文詠唱技能\0" ) },
};

/*
 * The `#enhance' extended command.  What we _really_ would like is
 * to keep being able to pick things to advance until we couldn't any
 * more.  This is currently not possible -- the menu code has no way
 * to call us back for instant action.  Even if it did, we would also need
 * to be able to update the menu since selecting one item could make
 * others unselectable.
 */
int
show_weapon_skills()
{
	enhance_weapon_skill(FALSE);
	return (FALSE);
}

void
enhance_weapon_skill(to_advance)
boolean to_advance;
{
    int pass, i, n, len, longest,
	eventually_advance, maxxed_cnt;
    char buf[BUFSZ];
    const char *prefix;
    menu_item *selected;
    anything any;
    winid win;

#ifdef WIZARD
	if (wizard && !to_advance && yn("Advance skills without practice?") == 'y')
	    to_advance = TRUE;
#endif
    do {
	eventually_advance = maxxed_cnt = 0;
	for (longest = 0, i = 0; i < P_NUM_SKILLS; i++) {
		if (P_RESTRICTED(i)) continue;
		if (!to_advance && P_SKILL(i) == 0) continue;
		if ((len = strlen(P_NAME(i))) > longest)
		    longest = len;
		else if (to_advance && P_SKILL(i) < P_MAX_SKILL(i)) eventually_advance++;
	}
	if (to_advance && eventually_advance == 0) {
		pline("All of your skills cannot be enhanced any further.");
		return;
	}

	win = create_nhwindow(NHW_MENU);
	start_menu(win);

	/* List the skills, making ones that could be advanced
	   selectable.  List the miscellaneous skills first.
	   Possible future enhancement:  list spell skills before
	   weapon skills for spellcaster roles. */
	for (pass = 0; pass < SIZE(skill_ranges); pass++) {
	    for (i = skill_ranges[pass].first, maxxed_cnt = 0;
		 i <= skill_ranges[pass].last; i++) {
		if (!to_advance && P_SKILL(i) == 0 ) continue;
		maxxed_cnt++;
	    }
	    if (!maxxed_cnt) continue;
	    for (i = skill_ranges[pass].first;
		 i <= skill_ranges[pass].last; i++) {
		/* Print headings for skill types */
		any.a_void = 0;
		if (i == skill_ranges[pass].first)
		    add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
			     skill_ranges[pass].name, MENU_UNSELECTED);
		if (P_RESTRICTED(i)) continue;
		if (!to_advance && P_SKILL(i) == 0) continue;
		/*
		 * Sigh, this assumes a monospaced font unless
		 * iflags.menu_tab_sep is set in which case it puts
		 * tabs between columns.
		 * The 12 is the longest skill level name.
		 * The "    " is room for a selection letter and dash, "a - ".
		 */
		if (P_SKILL(i) < P_MAX_SKILL(i) && to_advance)
		    prefix = "";	/* will be preceded by menu choice */
		else prefix = "    ";
//		(void) skill_level_name(i, sklnambuf);

		if (!iflags.menu_tab_sep)
			if (to_advance)
			     Sprintf(buf, " %s %-*s %3d%%(+%2d%%)",
					prefix, longest, P_NAME(i), P_SKILL(i), calc_skillgain(i));
			else Sprintf(buf, " %s %-*s %3d%%", prefix, longest, P_NAME(i), P_SKILL(i) );
		else
			if (to_advance)
			     Sprintf(buf, " %s%s\t%3d%%(+%2d%%)",
					prefix, P_NAME(i), P_SKILL(i), calc_skillgain(i));
			else Sprintf(buf, " %s%s\t%3d%%", prefix, P_NAME(i), P_SKILL(i) );
		any.a_int = (P_SKILL(i) < P_MAX_SKILL(i) && to_advance) ? i+1 : 0;
		add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 buf, MENU_UNSELECTED);
	    }
	}
	if (!to_advance) {
		add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
		any.a_int = 1+1;
		add_menu(win, NO_GLYPH, &any, 'w', 0, ATR_NONE,
				E_J("Show weapon capability",
				    "技能に対応する武器を表示"), MENU_UNSELECTED);
		any.a_int = 2+1;
		add_menu(win, NO_GLYPH, &any, 's', 0, ATR_NONE,
				E_J("Show spellcasting capability",
				    "技能に対応する魔法を表示"), MENU_UNSELECTED);
	}
	Strcpy(buf, (to_advance) ? E_J("Pick a skill to advance:","上達させたい技能を選んでください:") :
				   E_J("Current skills:","現在の技能:"));
	end_menu(win, buf);
	n = select_menu(win, PICK_ONE, &selected);
	destroy_nhwindow(win);
	if (n > 0) {
		i = selected[0].item.a_int - 1;	/* get item selected */
		free((genericptr_t)selected);
		if (to_advance) {
			add_weapon_skill(i, calc_skillgain(i));
		} else {
			switch (i) {
			    case 1:
				show_weaponskill();
				break;
			    case 2:
				show_spell_skill();
				break;
			    default:
				break;
			}
		}
	}
    } while (!n && to_advance);
}

/*
 * Change from restricted to unrestricted, allowing P_BASIC as max.  This
 * function may be called with with P_NONE.  Used in pray.c.
 */
void
unrestrict_weapon_skill(skill)
int skill;
{
    if (skill < P_NUM_SKILLS && P_RESTRICTED(skill)) {
	P_SKILL(skill) = P_UNSKILLED;
	P_MAX_SKILL(skill) = P_MINIMUM/*P_BASIC*/;
//	P_ADVANCE(skill) = 0;
    }
}

#endif /* OVL1 */
#ifdef OVLB

void
use_skill(skill,degree)
int skill;
int degree;
{
	if (skill != P_NONE && !P_RESTRICTED(skill)) {
	    if (skill_gain_check(skill, degree))
		add_weapon_skill(skill, 1);
	}
}

boolean
skill_gain_check(type,degree)
int type;
int degree;
{
	int skill = P_SKILL(type);
	int prob;
	if (P_RESTRICTED(type)) return FALSE;
	if (type == P_BARE_HANDED_COMBAT && skill >= P_GRAND_MASTER) return FALSE;
	if (skill >= P_EXPERT) return FALSE;
	if (skill < P_BASIC)		 prob = (skill >= P_MAX_SKILL(type)) ? 50             : skill/20;
	else if (skill < P_SKILLED     ) prob = (skill >= P_MAX_SKILL(type)) ? skill*2        : skill/10;
	else if (skill < P_EXPERT      ) prob = (skill >= P_MAX_SKILL(type)) ? skill*skill/25 : skill/5;
	else if (skill < P_MASTER      ) prob = (skill >= P_MAX_SKILL(type)) ? skill*skill/10 : skill/2;
	else if (skill < P_GRAND_MASTER) prob = (skill >= P_MAX_SKILL(type)) ? skill*skill/5  : skill;
	prob /= degree;
	if (prob == 0) return TRUE;
	return !rn2(prob);
}

/* skill enhancement by experienced */
int
calc_skillgain(type)
int type;
{
	int skill = P_SKILL(type);
	int gain;
	if (P_RESTRICTED(type)) return 0;
	if (skill >= P_MAX_SKILL(type)) return 0;
	gain =  (skill < P_BASIC  ) ? 15 :
		(skill < P_SKILLED) ? 10 :
		(skill < P_EXPERT ) ?  5 :
		(skill < P_MASTER ) ?  2 : 1;
	if (skill+gain > P_MAX_SKILL(type)) gain = P_MAX_SKILL(type) - skill;
	return gain;
}


void
add_weapon_skill(type, n)
int type;	/* skill type */
int n;		/* number of points to gain; normally one */
{
//    int i, before, after;
//
//    for (i = 0, before = 0; i < P_NUM_SKILLS; i++)
//	if (can_advance(i, FALSE)) before++;
//    u.weapon_slots += n;
//    for (i = 0, after = 0; i < P_NUM_SKILLS; i++)
//	if (can_advance(i, FALSE)) after++;
//    if (before < after)
//	give_may_advance_msg(P_NONE);

	int oldskill, newskill;
	oldskill = P_SKILL(type);
	newskill = P_SKILL(type) + n;
	P_SKILL(type) = newskill;
	if (newskill > P_MAX_SKILL(type)) {
		You(E_J("have become better at %s!",
			"%sの技能により熟達した！"), P_NAME(type));
		P_MAX_SKILL(type) = newskill; /* increase upper limit */
	} else if ((newskill/10) > (oldskill/10))
#ifndef JP
		You("are %s skilled in %s.",
		    (newskill == P_MAX_SKILL(type)) ? "most" : "more", P_NAME(type));
#else
		Your("%sの技能は%s向上した。", P_NAME(type),
		    (newskill == P_MAX_SKILL(type)) ? "最大限まで" : "さらに");
#endif /*JP*/
}

//void
//lose_weapon_skill(n)
//int n;	/* number of slots to lose; normally one */
//{
//    int skill;
//
//    while (--n >= 0) {
//	/* deduct first from unused slots, then from last placed slot, if any */
//	if (u.weapon_slots) {
//	    u.weapon_slots--;
//	} else if (u.skills_advanced) {
//	    skill = u.skill_record[--u.skills_advanced];
//	    if (P_SKILL(skill) <= P_UNSKILLED)
//		panic("lose_weapon_skill (%d)", skill);
//	    P_SKILL(skill)--;	/* drop skill one level */
//	    /* Lost skill might have taken more than one slot; refund rest. */
//	    u.weapon_slots = slots_required(skill) - 1;
//	    /* It might now be possible to advance some other pending
//	       skill by using the refunded slots, but giving a message
//	       to that effect would seem pretty confusing.... */
//	}
//    }
//}

int
weapon_type(obj)
struct obj *obj;
{
	/* KMH -- now uses the object table */
	int type;

	if (!obj)
		/* Not using a weapon */
		return (P_BARE_HANDED_COMBAT);
	if (obj->oclass != WEAPON_CLASS && obj->oclass != TOOL_CLASS &&
	    obj->oclass != GEM_CLASS)
		/* Not a weapon, weapon-tool, or ammo */
		return (P_NONE);
	type = objects[obj->otyp].oc_skill;
	return (type);
}

int
uwep_skill_type()
{
	if (u.twoweap)
		return P_TWO_WEAPON_COMBAT;
	return weapon_type(uwep);
}

int
wepidentify_byhit(obj)
struct obj *obj;
{
	int type;
	if (is_ammo(obj) && ammo_and_launcher(obj, uwep))
	    type = weapon_type(uwep);
	else
	    type = weapon_type(obj);
	if (type == P_NONE) return 0;
	return (rn2(3000) < P_SKILL(type));
}

/*
 * Return hit bonus/penalty based on skill of weapon.
 * Treat restricted weapons as unskilled.
 */
int
weapon_hit_bonus(weapon)
struct obj *weapon;
{
    int type, wep_type, skill, tmp, bonus = 0;
    static const char bad_skill[] = "weapon_hit_bonus: bad skill %d";

    wep_type = weapon_type(weapon);
    /* use two weapon skill only if attacking with one of the wielded weapons */
    type = (u.twoweap && (weapon == uwep || weapon == uswapwep)) ?
	    P_TWO_WEAPON_COMBAT : wep_type;
    if (type == P_NONE) {
	bonus = 0;
    } else if (type <= P_LAST_WEAPON) {
	schar hbon1[11] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% */
		-5, -4, -3, -2, -1,  0, +1, +2, +3, +4, +5
	};
	skill = P_SKILL(type);
	bonus = hbon1[skill/10];
    } else if (type == P_TWO_WEAPON_COMBAT) {
	schar hbon2[11] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% */
		-10,-9, -8, -7, -6, -5, -4, -3, -2, -1,  0
	};
	skill = P_SKILL(P_TWO_WEAPON_COMBAT);
	if ((tmp = P_SKILL(wep_type)) < skill) skill = tmp;
	bonus = hbon2[skill/10];
    } else if (type == P_BARE_HANDED_COMBAT) {
	schar hbon3[13] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 110% 120% */
		 0,  0,  0, +1, +2, +3, +3, +4, +4, +4,  +5,  +6, +7
	};
	bonus = hbon3[P_SKILL(type)/10];
	if (!martial_bonus()) bonus /= 2;
    }

#ifdef STEED
	/* KMH -- It's harder to hit while you are riding */
	if (u.usteed) {
		skill = P_SKILL(P_RIDING);
		bonus -= (skill < 50) ? 2 :
			 (skill < 75) ? 1 : 0;
		if (u.twoweap) bonus -= 2;
	}
#endif

    return bonus;
}

/*
 * Return damage bonus/penalty based on skill of weapon.
 * Treat restricted weapons as unskilled.
 */
int
weapon_dam_bonus(weapon)
struct obj *weapon;
{
    int type, wep_type, skill, tmp, bonus = 0;

    wep_type = weapon_type(weapon);
    /* use two weapon skill only if attacking with one of the wielded weapons */
    type = (u.twoweap && (weapon == uwep || weapon == uswapwep)) ?
	    P_TWO_WEAPON_COMBAT : wep_type;
    if (type == P_NONE) {
	bonus = 0;
    } else if (type <= P_LAST_WEAPON) {
	schar dbon1[11] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% */
		-2, -2, -2, -1, -1,  0,  0, +1, +1, +2, +3
	};
	skill = P_SKILL(type);
	bonus = dbon1[skill/10];
    } else if (type == P_TWO_WEAPON_COMBAT) {
	schar dbon2[11] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% */
		-3, -3, -3, -2, -2, -1, -1,  0,  0, +1, +2
	};
	skill = P_SKILL(P_TWO_WEAPON_COMBAT);
	if ((tmp = P_SKILL(wep_type)) < skill) skill = tmp;
	bonus = dbon2[skill/10];
    } else if (type == P_BARE_HANDED_COMBAT) {
	schar dbon3[13] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 110% 120% */
		 0,  0,  0, +1, +2, +3, +3, +4, +4, +5,  +6,  +7, +9
	};
	schar dbon4[13] = {
	   /*  0% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 110% 120% */
		 0,  0,  0,  0,  0, +1, +1, +1, +2, +2,  +2,  +2, +3
	};
	skill = P_SKILL(type);
	bonus = martial_bonus() ? dbon3[skill/10] : dbon4[skill/10];
    }

#ifdef STEED
	/* KMH -- Riding gives some thrusting damage */
	if (u.usteed && type != P_TWO_WEAPON_COMBAT) {
		skill = P_SKILL(P_RIDING);
		bonus += (skill < 75) ? 0 :
			 (skill < 90) ? 1 :
			 (skill < 100) ? 2 : 3;
	}
#endif

    return bonus;
}

/*
 * Initialize weapon skill array for the game.  Start by setting all
 * skills to restricted, then set the skill for every weapon the
 * hero is holding, finally reading the given array that sets
 * maximums.
 */
void
skill_init(class_skill)
const struct def_skill *class_skill;
{
	struct obj *obj;
	int skmax, skill;

	/* initialize skill array; by default, everything is restricted */
	for (skill = 0; skill < P_NUM_SKILLS; skill++) {
	    P_SKILL(skill) = P_ISRESTRICTED;
	    P_MAX_SKILL(skill) = P_ISRESTRICTED;
//	    P_ADVANCE(skill) = 0;
	}

	/* Set skill for all weapons in inventory to be basic */
	for (obj = invent; obj; obj = obj->nobj) {
	    if (obj->oclass != WEAPON_CLASS /*&& !is_weptool(obj)*/) continue;
	    if (objects[obj->otyp].oc_skill < P_NONE) continue; /* projectile */
	    skill = weapon_type(obj);
	    if (skill != P_NONE)
		P_SKILL(skill) = P_BASIC;
	}

	/* set skills for magic */
	if (Role_if(PM_HEALER) || Role_if(PM_MONK)) {
		P_SKILL(P_HEALING_SPELL) = P_BASIC;
	} else if (Role_if(PM_PRIEST)) {
		P_SKILL(P_CLERIC_SPELL) = P_BASIC;
	} else if (Role_if(PM_WIZARD)) {
		P_SKILL(P_ATTACK_SPELL) = P_BASIC;
		P_SKILL(P_ENCHANTMENT_SPELL) = P_BASIC;
	}

	/* walk through array to set skill maximums */
	for (; class_skill->skill != P_NONE; class_skill++) {
	    skmax = class_skill->skmax;
	    skill = class_skill->skill;

	    P_MAX_SKILL(skill) = skmax;
	    if (P_SKILL(skill) == P_ISRESTRICTED)	/* skill pre-set */
		P_SKILL(skill) = P_UNSKILLED;
	}

	/* High potential fighters already know how to use their hands. */
	if (P_MAX_SKILL(P_BARE_HANDED_COMBAT) > P_EXPERT)
	    P_SKILL(P_BARE_HANDED_COMBAT) = P_BASIC;

	/* Roles that start with a horse know how to ride it */
#ifdef STEED
	if (urole.petnum == PM_PONY)
	    P_SKILL(P_RIDING) = P_BASIC;
#endif

	/*
	 * Make sure we haven't missed setting the max on a skill
	 * & set advance
	 */
	for (skill = 0; skill < P_NUM_SKILLS; skill++) {
	    if (!P_RESTRICTED(skill)) {
		if (P_MAX_SKILL(skill) < P_SKILL(skill)) {
		    impossible("skill_init: curr > max: %s", P_NAME(skill));
		    P_MAX_SKILL(skill) = P_SKILL(skill);
		}
//		P_ADVANCE(skill) = practice_needed_to_advance(P_SKILL(skill)-1);
	    }
	}
}

void
setmnotwielded(mon,obj)
register struct monst *mon;
register struct obj *obj;
{
    if (!obj) return;
    if (artifact_light(obj) && obj->lamplit) {
	end_burn(obj, FALSE);
	if (canseemon(mon) && !is_swallowed(mon))
#ifndef JP
	    pline("%s in %s %s %s glowing.", The(xname(obj)),
		  s_suffix(mon_nam(mon)), mbodypart(mon,HAND),
		  otense(obj, "stop"));
#else
	    pline("%sが構えていた%sは輝くのを止めた。",
		  mon_nam(mon), xname(obj));
#endif /*JP*/
    }
    obj->owornmask &= ~W_WEP;
}

/*int
show_current_weaponskill()
{
	winid win;

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	add_menu(tmpwin, 0, 0, "Weapons Skills Menu:");
	add_menu(tmpwin, 0, 0, "");
	add_menu(tmpwin, 'a', 0, "a - Weapons in avaliable groups");
	add_menu(tmpwin, 'b', 0, "b - Skill in avaliable groups");
	add_menu(tmpwin, 0, 0, "");
	end_menu(tmpwin, '\033', "\033", "Your choice?");

  tmpwin = create_nhwindow(NHW_MENU);
	putstr(tmpwin, 0, "Skill Groups:");
	putstr(tmpwin, 0, "");
	for (i = 1; i <= OTHER_GROUP; i++) {
	    if (weapon_skills[i].skill == 0 || weapon_skills[i].restricted)
		continue;
	    Sprintf(buf, "%s ", weapon_skills[i].name);
	    Sprintf(buf2, "(%d%%)", weapon_skills[i].skill);
	    length = 26 - strlen(buf);
	    if (length < 1) length = 1;
	    for(k = 0; k <= length; k++) Strcat(buf, " ");
	    Strcat(buf, buf2);
	    putstr(tmpwin, 0, buf);
	}
    
	display_nhwindow(tmpwin, TRUE);
	destroy_nhwindow(tmpwin);
	break;
	
}*/

/*	 Calculate weapon range from skill	*/
/*	4	5	9  :	10 :::		*/
/*	  :	 :::	 :::::	  :::::		*/
/*	 :::	:::::	 :::::	 :::::::	*/
/*	::@::	::@::	:::@:::	 :::@:::	*/
/*	 :::	:::::	 :::::	 :::::::	*/
/*	  :	 :::	 ::::: 	  :::::		*/
/*			   :	   :::		*/
int
weapon_range(wep)
struct obj *wep;
{
	int max_range;
	int typ = weapon_type(wep);
	if (typ == P_NONE || P_SKILL(typ) < P_BASIC) max_range = 4;
	else if (P_SKILL(typ) < P_SKILLED) max_range = 5;
	else if (P_SKILL(typ) < P_EXPERT) max_range = 9;
	else max_range = 10;
	return max_range;
}

int
show_weaponskill()
{
    const schar skills[] = {
	P_DAGGER_GROUP, P_KNIFE_GROUP, P_AXE_GROUP, P_SHORT_BLADE_GROUP,
	P_LONG_BLADE_GROUP, P_SABER_GROUP, P_SPEAR_GROUP,
	P_CRUSHING_GROUP, P_FLAIL_GROUP, P_STAFF_GROUP, P_POLEARM_GROUP, P_PICKAXE_GROUP,
	P_WHIP_GROUP, P_BOW_GROUP, P_FIREARM_GROUP, P_THROWING_GROUP,
	P_NONE /* terminator */
    };
	show_available_skills(skills);
	return 0;
}

int
show_spell_skill()
{
    const schar skills[] = {
	P_ATTACK_SPELL, P_HEALING_SPELL, P_DIVINATION_SPELL,
	P_ENCHANTMENT_SPELL, P_CLERIC_SPELL, P_ESCAPE_SPELL, P_MATTER_SPELL,
	P_NONE /* terminator */
    };
	show_available_skills(skills);
	return 0;
}

void
show_available_skills(skills)
const schar *skills;
{
	winid tmpwin;
	anything any;
	menu_item *selected;
	int i,l,c,m;
	int skill;
	int i_begin = 0, i_end = 0;
	int cnt = 0;
	boolean isweapon = TRUE;
	const char *actualn;
	char buf[BUFSZ];

	any.a_void = 0;
	tmpwin = create_nhwindow(NHW_MENU);

	start_menu(tmpwin);

	while (skill = *skills++) {
	    if (skill >= P_FIRST_WEAPON && skill <= P_LAST_WEAPON) {
		i_begin = DART;
		i_end   = UNICORN_HORN;
	    } else if (skill >= P_FIRST_SPELL && skill <= P_LAST_SPELL) {
		i_begin = SPE_DIG;
		i_end   = SPE_KNOW_ENCHANTMENT;
		isweapon = FALSE;
	    } else continue;
	    if(P_RESTRICTED(skill)) continue;
	    m = 0;		/* count items in the skill category */
	    for ( i = i_begin; i <= i_end; i++ ) {
		if (objects[i].oc_skill == skill) m++;
		if (i == SLING) i = PICK_AXE - 1;	/* skip a gap... */
	    }
	    if (m == 0) continue;		/* empty category??? */
	    Strcpy(buf, P_NAME(skill));		/* prepare heading */
	    for ( i=0; buf[i]; ) {
#ifndef JP
		buf[i] = highc(buf[i]);
#endif /*JP*/
		while (letter(buf[i])) i++;
		while (buf[i] && !letter(buf[i])) i++;
	    }
	    Strcat(buf, ": ");
	    if (P_MAX_SKILL(skill) >= P_SKILLED) {
		Sprintf(eos(buf), "(+%s)", P_MAX_SKILL(skill) >= P_EXPERT ? "+" : "");
	    } else if (P_MAX_SKILL(skill) < P_BASIC) {
		Sprintf(eos(buf), "(-)");
	    }
	    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	    c = 0;
	    l = 0;
	    for ( i = i_begin; i <= i_end; i++ ) {
		if (objects[i].oc_skill == skill) {
		    c++;
		    if (l == 0) {
			Strcpy(buf, "     ");
			l = 5;
		    } else {
			Strcat(buf, ", ");
			l += 2;
		    }
		    if (Role_if(PM_SAMURAI) && Japanese_item_name(i))
			 actualn = Japanese_item_name(i);
		    else actualn = E_J(OBJ_NAME(objects[i]), JOBJ_NAME(objects[i]));
		    Strcat(buf, actualn);
		    l += strlen(actualn);
		    if (l > 50) {
			if (c < m) Strcat(buf, ", ");
			add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
			l = 0;
		    }
		}
		if (i == SLING) i = PICK_AXE - 1;	/* dirty code... */
	    }
	    if (l) add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
	    cnt++;
	}
#ifndef JP
	if (cnt)
	    Sprintf(buf, "%ss in avaliable Skill Groups:", isweapon ? "Weapon" : "Spell");
	else
	    Sprintf(buf, "No available skill for %ss.", isweapon ? "weapon" : "spell");
#else
	if (cnt)
	    Sprintf(buf, "鍛錬できる%s技能:", isweapon ? "武器" : "呪文");
	else
	    Sprintf(buf, "鍛錬できる%s技能はありません", isweapon ? "武器" : "呪文");
#endif /*JP*/
	end_menu(tmpwin, buf);

	select_menu(tmpwin, PICK_NONE, &selected);
	destroy_nhwindow(tmpwin);
}

#endif /* OVLB */

/*weapon.c*/
