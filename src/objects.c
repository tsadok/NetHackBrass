/*	SCCS Id: @(#)objects.c	3.4	2002/07/31	*/
/* Copyright (c) Mike Threepoint, 1989.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef OBJECTS_PASS_2_
/* first pass */
struct monst { struct monst *dummy; };	/* lint: struct obj's union */
#include "config.h"
#include "obj.h"
#include "objclass.h"
#include "prop.h"
#include "skills.h"

#else	/* !OBJECTS_PASS_2_ */
/* second pass */
#include "color.h"
#  define COLOR_FIELD(X) X,
#endif	/* !OBJECTS_PASS_2_ */


/* objects have symbols: ) [ = " ( % ! ? + / $ * ` 0 _ . */

/*
 *	Note:  OBJ() and BITS() macros are used to avoid exceeding argument
 *	limits imposed by some compilers.  The ctnr field of BITS currently
 *	does not map into struct objclass, and is ignored in the expansion.
 *	The 0 in the expansion corresponds to oc_pre_discovered, which is
 *	set at run-time during role-specific character initialization.
 */

#ifndef OBJECTS_PASS_2_
/* first pass -- object descriptive text */
# define OBJ(name,desc) name,desc
# define OBJECT(obj,bits,prp,sym,prob,dly,wt,cost,sdam,ldam,oc1,oc2,nut,color) \
	{obj}

NEARDATA struct objdescr obj_descr[] = {
#else
/* second pass -- object definitions */

# define BITS(nmkn,mrg,uskn,ctnr,mgc,chrg,uniq,nwsh,big,tuf,dir,sub,mtrl) \
	nmkn,mrg,uskn,0,mgc,chrg,uniq,nwsh,big,tuf,dir,mtrl,sub /* SCO ODT 1.1 cpp fodder */
# define OBJECT(obj,bits,prp,sym,prob,dly,wt,cost,sdam,ldam,oc1,oc2,nut,color) \
	{0, 0, (char *)0, bits, prp, sym, dly, COLOR_FIELD(color) \
	 prob, wt, cost, sdam, ldam, oc1, oc2, nut}
# ifndef lint
#  define HARDGEM(n) (n >= 8)
# else
#  define HARDGEM(n) (0)
# endif

NEARDATA struct objclass objects[] = {
#endif
/* dummy object[0] -- description [2nd arg] *must* be NULL */
	OBJECT(OBJ("strange object",(char *)0), BITS(1,0,0,0,0,0,0,0,0,0,0,P_NONE,0),
			0, ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),

/* weapons ... */
#define WEAPON(name,app,kn,mg,bi,prob,wt,cost,sdam,ldam,hitbon,dambon,typ,sub,sub2,metal,metal2,color) \
	OBJECT( \
		OBJ(name,app), BITS(kn,mg,1,0,0,1,0,0,bi,metal2,typ,sub,metal), 0, \
		WEAPON_CLASS, prob, sub2, \
		wt, cost, sdam, ldam, hitbon, dambon, wt, color )
#define PROJECTILE(name,app,kn,prob,wt,cost,sdam,ldam,hitbon,dambon,metal,metal2,ammo,color) \
	OBJECT( \
		OBJ(name,app), \
		BITS(kn,1,1,0,0,1,0,0,0,metal2,PIERCE,0,metal), 0, \
		WEAPON_CLASS, prob, (ammo|WP_CONSUMABLE|WP_AMMUNITION), \
		wt, cost, sdam, ldam, hitbon, dambon, wt, color )
#define BOW(name,app,kn,prob,wt,cost,hitbon,metal,sub,ammo,color) \
	OBJECT( \
		OBJ(name,app), BITS(kn,0,1,0,0,1,0,0,0,0,0,sub,metal), 0, \
		WEAPON_CLASS, prob, (ammo|WP_LAUNCHER), \
		wt, cost, 2, 2, hitbon, 0, wt, color )

/* Note: for weapons that don't do an even die of damage (ex. 2-7 or 3-18)
 * the extra damage is added on in weapon.c, not here! */

#define P PIERCE
#define S SLASH
#define B WHACK

/* now AdB+C type of weapons don't need to be described specially in weapon.c.
 * 1<=A<=7, 1<=B<=31, 0<=C<=15 ... youkan */
#define DAM(num,die) ((num<<5)|die)
#define DBON(small,large) ((small<<4)|large)

/* missiles */
PROJECTILE("arrow", (char *)0,
		1, 50, 1, 2, DAM(1,8), DAM(1,8), 0, 0, IRON,  1, WP_ARROW|WP_POISONABLE, HI_METAL),
PROJECTILE("elven arrow", "runed arrow",
		0, 20, 1, 2, DAM(1,9), DAM(1,8), 0, 0, WOOD,  0, WP_ARROW, HI_WOOD),
PROJECTILE("orcish arrow", "crude arrow",
		0, 20, 1, 2, DAM(1,7), DAM(1,8), 0, 0, IRON,  0, WP_ARROW|WP_POISONABLE, CLR_BLACK),
PROJECTILE("ya", "bamboo arrow",
		0, 15, 1, 4, DAM(1,10),DAM(1,10),1, 0, METAL, 0, WP_ARROW|WP_POISONABLE, HI_METAL),
PROJECTILE("crossbow bolt", (char *)0,
		1, 50, 1, 2, DAM(1,7), DAM(1,9), 0, DBON(1,1), IRON, 0, WP_BOLT|WP_POISONABLE, HI_METAL),
PROJECTILE("bullet", (char *)0,
		1, 20, 1,10, DAM(1,20), DAM(1,20), 0, DBON(10,5), IRON, 1, WP_BULLET, HI_METAL),

WEAPON("dart", (char *)0,
	1, 1, 0, 60,  1,  2, DAM(1,3), DAM(1,2), 0, 0, P,   P_THROWING_GROUP,
						WP_THROWING|WP_POISONABLE|WP_CONSUMABLE, IRON, 0, HI_METAL),
WEAPON("shuriken", "throwing star",
	0, 1, 0, 35,  1,  5, DAM(1,8), DAM(1,6), 2, 0, P,   P_THROWING_GROUP,
						WP_THROWING|WP_POISONABLE|WP_CONSUMABLE, IRON, 0, HI_METAL),
WEAPON("boomerang", (char *)0,
	1, 0, 0, 15,  5, 20, DAM(1,9), DAM(1,9), 0, 0, 0,   P_THROWING_GROUP, WP_THROWING, WOOD, 0, HI_WOOD),

/* spears */
WEAPON("spear", (char *)0,
	1, 1, 0, 40, 30,  3, DAM(1,6), DAM(1,8), 1, 0, P,   P_SPEAR_GROUP, WP_THROWING, IRON, 1, HI_METAL),
WEAPON("elven spear", "runed spear",
	0, 1, 0, 10, 30,  3, DAM(1,7), DAM(1,8), 1, 0, P,   P_SPEAR_GROUP, WP_THROWING, IRON, 0, HI_METAL),
WEAPON("orcish spear", "crude spear",
	0, 1, 0, 13, 30,  3, DAM(1,5), DAM(1,8), 1, 0, P,   P_SPEAR_GROUP, WP_THROWING|WP_POISONABLE, IRON, 0, CLR_BLACK),
WEAPON("dwarvish spear", "stout spear",
	0, 1, 0, 12, 35,  3, DAM(1,8), DAM(1,8), 1, 0, P,   P_SPEAR_GROUP, WP_THROWING, IRON, 0, HI_METAL),
WEAPON("javelin", "throwing spear",
	0, 1, 0, 10, 20,  3, DAM(1,6), DAM(1,6), 1, 0, P,   P_SPEAR_GROUP, WP_THROWING, IRON, 0, HI_METAL),
WEAPON("trident", (char *)0,
	1, 0, 0, 13, 25,  5, DAM(1,6), DAM(3,4), 0, DBON(1,0), P, P_SPEAR_GROUP, WP_THROWING, IRON, 0, HI_METAL),
WEAPON("lance", (char *)0,
	1, 0, 0,  4,180, 10, DAM(1, 8), DAM(1,10), 0, 0, P, P_SPEAR_GROUP, 0, IRON, 1, HI_METAL),

/* blades */
WEAPON("dagger", (char *)0,
	1, 1, 0, 33, 10,  4, DAM(1,4), DAM(1,3), 2, 0, P,   P_DAGGER_GROUP,
								WP_THROWING|WP_POISONABLE, IRON, 1, HI_METAL),
WEAPON("elven dagger", "runed dagger",
	0, 1, 0, 10, 10,  4, DAM(1,5), DAM(1,3), 2, 0, P,   P_DAGGER_GROUP, WP_THROWING,   IRON, 0, HI_METAL),
WEAPON("orcish dagger", "crude dagger",
	0, 1, 0, 12, 10,  4, DAM(1,3), DAM(1,3), 2, 0, P,   P_DAGGER_GROUP,
								WP_THROWING|WP_POISONABLE, IRON, 0, CLR_BLACK),
WEAPON("athame", (char *)0,
	1, 1, 0,  0, 20,  4, DAM(1,4), DAM(1,3), 2, 0, S,   P_DAGGER_GROUP, 0, IRON, 0, HI_METAL),
WEAPON("scalpel", (char *)0,
	1, 1, 0,  0,  5,  6, DAM(1,3), DAM(1,3), 2, 0, S,   P_KNIFE_GROUP, 0, METAL, 0, HI_METAL),
WEAPON("knife", (char *)0,
	1, 1, 0, 20,  5,  4, DAM(1,3), DAM(1,2), 3, 0, P|S, P_KNIFE_GROUP,
								WP_THROWING|WP_POISONABLE, IRON, 0, HI_METAL),
WEAPON("stiletto", (char *)0,
	1, 1, 0,  5,  5,  4, DAM(1,4), DAM(1,3), 3, 0, P|S, P_KNIFE_GROUP,
								WP_THROWING|WP_POISONABLE, IRON, 0, HI_METAL),
WEAPON("worm tooth", (char *)0,
	1, 0, 0,  0, 20,  2, DAM(1,2), DAM(1,2), 0, 0, 0,   P_KNIFE_GROUP, 0, 0, 0, CLR_WHITE),
WEAPON("crysknife", (char *)0,
	1, 0, 0,  0, 20,100, DAM(1,10), DAM(1,10), 3, 0, P, P_KNIFE_GROUP, 0, MINERAL, 0, CLR_WHITE),

WEAPON("axe", (char *)0,
	1, 0, 0, 40, 60,  8, DAM(1,6), DAM(2,4), 0, 0, S,   P_AXE_GROUP, WP_THROWING, IRON, 0, HI_METAL),
WEAPON("battle-axe", "double-headed axe",
	0, 0, 1, 10,120, 40, DAM(2,6), DAM(2,8), 0, 0, S,   P_AXE_GROUP, 0, IRON, 0, HI_METAL),
						/* "double-bitted" ? */

/*** swords ***/
/* short blades */
WEAPON("short sword", (char *)0,
	1, 0, 0, 18, 30, 10, DAM(1,6), DAM(1,8), 1, 0, P,   P_SHORT_BLADE_GROUP, 0, IRON, 1, HI_METAL),
WEAPON("elven short sword", "runed short sword",
	0, 0, 0,  2, 30, 10, DAM(1,8), DAM(1,8), 1, 0, P,   P_SHORT_BLADE_GROUP, 0, IRON, 0, HI_METAL),
WEAPON("dwarvish short sword", "broad short sword",
	0, 0, 0,  2, 30, 10, DAM(1,7), DAM(1,8), 1, 0, P,   P_SHORT_BLADE_GROUP, 0, IRON, 0, HI_METAL),
WEAPON("orcish short sword", "crude short sword",
	0, 0, 0,  3, 30, 10, DAM(1,5), DAM(1,8), 1, 0, P,   P_SHORT_BLADE_GROUP, WP_POISONABLE, IRON, 0, CLR_BLACK),

/* sabers */
WEAPON("saber", (char *)0,
	1, 0, 0,  6, 40, 15, DAM(1,8), DAM(1,8), 0, 0, S,   P_SABER_GROUP, 0, IRON, 1, HI_METAL),
WEAPON("scimitar", "curved sword",
	0, 0, 0, 15, 40, 15, DAM(1,8), DAM(1,8), 0, 0, S,   P_SABER_GROUP, WP_POISONABLE, IRON, 0, HI_METAL),

/* broad blades */
WEAPON("broadsword", (char *)0,
	1, 0, 0, 15, 70, 10, DAM(1,10), DAM(2,6), 0, 0, S, P_LONG_BLADE_GROUP, 0, IRON, 0, HI_METAL),
						/* (2d4/1d6+1) --> (1d10/2d6) */
WEAPON("elven broadsword", "runed broadsword",
	0, 0, 0,  4, 70, 10, DAM(1,10), DAM(2,6), 0, DBON(1,1), S, P_LONG_BLADE_GROUP, 0, IRON, 0, HI_METAL),
						/* (1d6+d4/1d6+1) --> (1d10+1/2d6+1) */
WEAPON("long sword", (char *)0,
	1, 0, 0, 40, 50, 15, DAM(1, 8), DAM(1,12), 0, 0, S,   P_LONG_BLADE_GROUP, 0, IRON, 1, HI_METAL),
WEAPON("two-handed sword", (char *)0,
	1, 0, 1, 22,150, 50, DAM(1,12), DAM(3, 6), 0, 0, S,   P_LONG_BLADE_GROUP, 0, IRON, 0, HI_METAL),

WEAPON("katana", "samurai sword",
	0, 0, 0,  4, 40, 80, DAM(1,10), DAM(1,12), 1, 0, S, P_SABER_GROUP, 0, IRON,  0, HI_METAL),
/* special swords set up for artifacts */
WEAPON("tsurugi", "long samurai sword",
	0, 0, 1,  0, 60,500, DAM(1,16), DAM(1, 8), 2, 0, S, P_SABER_GROUP, 0, METAL, 0, HI_METAL),
								/* +2d6 large(specially handled) */
WEAPON("runesword", "runed broadsword",
	0, 0, 0,  0, 40,300, DAM(1,8), DAM(1,10), 0, 0, S,
	P_LONG_BLADE_GROUP, 0, METAL, 0, CLR_BLACK),
								/* (2d4/1d6+1) --> (1d8/1d10) */
								/* +5d2 +d8 from level drain */

/* polearms */
/* spear-type */
WEAPON("partisan", "vulgar polearm",
	0, 0, 1,  5, 80, 10, DAM(1,10), DAM(1, 8), 0, DBON(0,1), P, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("ranseur", "hilted polearm",
	0, 0, 1,  5, 50,  6, DAM(2, 4), DAM(2, 4), 0,         0, P, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("spetum", "forked polearm",
	0, 0, 1,  5, 50,  5, DAM(1, 6), DAM(2, 6), 0, DBON(1,0), P, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("glaive", "single-edged polearm",
	0, 0, 1,  8, 75,  6, DAM(1, 8), DAM(1,12), 0,         0, S, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

/* axe-type */
WEAPON("halberd", "angled poleaxe",
	0, 0, 1,  8,180, 10, DAM(1,12), DAM(2, 8), 0,       0, P|S, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("bardiche", "long poleaxe",
	0, 0, 1,  4,120,  7, DAM(2, 4), DAM(3, 4), 0,         0, S, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("voulge", "pole cleaver",
	0, 0, 1,  4,125,  5, DAM(3, 4), DAM(3, 4), 0,         0, S, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

/* curved/hooked */
WEAPON("fauchard", "pole sickle",
	0, 0, 1,  6, 60,  5, DAM(1, 8), DAM(1, 8), 0, 0, P|S, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("guisarme", "pruning hook",
	0, 0, 1,  6, 80,  5, DAM(2, 4), DAM(1, 8), 0, 0, S,   P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("bill-guisarme", "hooked polearm",
	0, 0, 1,  4,120,  7, DAM(1,12), DAM(1,10), 0, 0, P|S, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

/* other */
WEAPON("lucern hammer", "pronged polearm",
	0, 0, 1,  5,150,  7, DAM(3, 4), DAM(2, 6), 0, 0, B|P, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("bec de corbin", "beaked polearm",
	0, 0, 1,  4,100,  8, DAM(1,10), DAM(1, 8), 0, 0, B|P, P_POLEARM_GROUP, WP_RANGED, IRON, 0, HI_METAL),

/* bludgeons */
WEAPON("mace", (char *)0,
	1, 0, 0, 40, 30,  5, DAM(1,6), DAM(1,6), 1, DBON(1,0), B, P_CRUSHING_GROUP, 0, IRON, 1, HI_METAL),

WEAPON("morning star", (char *)0,
	1, 0, 0, 12,120, 10, DAM(3,4), DAM(2,5), 0, DBON(0,1), B, P_FLAIL_GROUP, WP_RANGED, IRON, 0, HI_METAL),

WEAPON("war hammer", (char *)0,
	1, 0, 0, 15, 50,  5, DAM(1,10), DAM(1,8), 0, 0, B, P_CRUSHING_GROUP, 0, IRON, 0, HI_METAL),

WEAPON("club", (char *)0,
	1, 0, 0, 12, 20,  3, DAM(1,6), DAM(1,3), 1, 0, B, P_CRUSHING_GROUP, 0, WOOD, 1, HI_WOOD),

WEAPON("heavy hammer", (char *)0,
	1, 0, 0,  0,200,500, DAM(2,6), DAM(1,8), 0, 0, B, P_CRUSHING_GROUP, WP_THROWING, IRON, 0, HI_METAL),

WEAPON("quarterstaff", "staff",
	0, 0, 1, 11, 40,  5, DAM(1,6), DAM(1,6), 0, 0, B, P_STAFF_GROUP, 0, WOOD, 0, HI_WOOD),
/* two-piece */
WEAPON("aklys", "thonged club",
	0, 0, 0,  8, 15,  4, DAM(1,6), DAM(1,3), 1, 0, B, P_FLAIL_GROUP, WP_THROWING, IRON, 0, HI_METAL),
WEAPON("flail", (char *)0,
	1, 0, 0, 40, 15,  4, DAM(1,6), DAM(2,4), 1, DBON(1,0), B, P_FLAIL_GROUP, 0, IRON, 0, HI_METAL),

/* misc */
WEAPON("dwarvish mattock", "broad pick",
	0, 0, 1, 13,120, 50, DAM(1,12), DAM(1,8), -1, 0, B,   P_PICKAXE_GROUP, 0, IRON, 0, HI_METAL),
						/* +2d6 large(specially handled) */
WEAPON("bullwhip", (char *)0,
	1, 0, 0,  2, 20,  4, DAM(1,2), DAM(1,1), 1, 0, B,   P_WHIP_GROUP, 0, LEATHER, 0, CLR_BROWN),
#ifdef KOPS
WEAPON("rubber hose", (char *)0,
	1, 0, 0,  0, 20,  3, DAM(1,4), DAM(1,3), 1, 0, B,   P_WHIP_GROUP, 0, PLASTIC, 0, CLR_BROWN),
#endif

/* bows */
BOW("bow", (char *)0,		1, 20, 30,  60, 0, WOOD,    P_BOW_GROUP,      WP_ARROW,    HI_WOOD),
BOW("elven bow", "runed bow",	0, 12, 30,  60, 0, WOOD,    P_BOW_GROUP,      WP_ARROW,    HI_WOOD),
BOW("orcish bow", "crude bow",	0, 12, 30,  60, 0, WOOD,    P_BOW_GROUP,      WP_ARROW,    CLR_BLACK),
BOW("yumi", "long bow",		0,  0, 30,  60, 0, WOOD,    P_BOW_GROUP,      WP_ARROW,    HI_WOOD),
BOW("crossbow", (char *)0,	1, 40, 50,  40, 0, WOOD,    P_BOW_GROUP,      WP_BOLT,     HI_WOOD),
BOW("musket", (char *)0,	1, 10, 80, 500, 0, IRON,    P_FIREARM_GROUP,  WP_BULLET,   HI_METAL),
BOW("revolver", (char *)0,	1,  1, 30,2000,-1, IRON,    P_FIREARM_GROUP,  WP_BULLET,   HI_METAL),
BOW("sling", (char *)0,		1, 40,  3,  20, 0, LEATHER, P_THROWING_GROUP, WP_STONE,    HI_LEATHER),

#undef P
#undef S
#undef B

#undef WEAPON
#undef PROJECTILE
#undef BOW

/* armor ... */
/* IRON denotes ferrous metals, including steel.
 * Only IRON weapons and armor can rust.
 * Only COPPER (including brass) corrodes.
 * Some creatures are vulnerable to SILVER.
 */
/* known, (nonmergable), (?), (non-pre-discovered), magical-obj, (chargable), (non-special), (wishable),
   big(bulky), (not gem/ring), cancel%, subtype, material, */
#define ARMOR(name,desc,kn,mgc,blk,power,prob,delay,wt,cost,ac,can,sub,metal,metal2,c) \
	OBJECT( \
		OBJ(name,desc), BITS(kn,0,1,0,mgc,1,0,0,blk,metal2,can,sub,metal), power, \
		ARMOR_CLASS, prob, delay, wt, cost, \
		0, 0, 10 - ac, 0, wt, c )
#define HELM(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,metal2,c) \
	ARMOR(name,desc,kn,mgc,0,power,prob,delay,wt,cost,ac,can,ARM_HELM,metal,metal2,c)
#define CLOAK(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,metal2,c) \
	ARMOR(name,desc,kn,mgc,0,power,prob,delay,wt,cost,ac,can,ARM_CLOAK,metal,metal2,c)
#define SHIELD(name,desc,kn,mgc,blk,power,prob,delay,wt,cost,ac,can,metal,metal2,c) \
	ARMOR(name,desc,kn,mgc,blk,power,prob,delay,wt,cost,ac,can,ARM_SHIELD,metal,metal2,c)
#define GLOVES(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,metal2,c) \
	ARMOR(name,desc,kn,mgc,0,power,prob,delay,wt,cost,ac,can,ARM_GLOVES,metal,metal2,c)
#define BOOTS(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,metal2,c) \
	ARMOR(name,desc,kn,mgc,0,power,prob,delay,wt,cost,ac,can,ARM_BOOTS,metal,metal2,c)

/* helmets */
HELM("elven leather helm", "leather hat",
		0, 0,  0,	6, 1,  3,   8, 9, 0, LEATHER, 0, HI_LEATHER),
HELM("orcish helm", "iron skull cap",
		0, 0,  0,	4, 1, 30,  10, 9, 0, IRON, 0, CLR_BLACK),
HELM("dwarvish iron helm", "hard hat",
		0, 0,  0,	6, 1, 40,  20, 7, 0, IRON, 0, HI_METAL),
HELM("fedora", (char *)0,
		1, 1,  0,	1, 0,  3,   1,10, 0, CLOTH, 0, CLR_BROWN),
HELM("cornuthaum", "conical hat",
		0, 1,  CLAIRVOYANT,
				3, 1,  4,  80,10, 2, CLOTH, 0, CLR_BLUE),
HELM("dunce cap", "conical hat",
		0, 1,  0,	4, 1,  4,   1,10, 0, CLOTH, 0, CLR_BLUE),
HELM("dented pot", (char *)0,
		1, 0,  0,	3, 0, 10,   8, 9, 0, IRON, 0, CLR_BLACK),
HELM("katyusha", "headpiece",
		0, 1,  0,	3, 0,  1, 100,10, 0, CLOTH, 0, CLR_WHITE),
HELM("nurse cap", (char *)0,
		1, 0,  0,	0, 0,  2,  10,10, 0, CLOTH, 0, CLR_WHITE),
/* With shuffled appearances... */
HELM("helmet", "plumed helmet",
		0, 0,  0,      10, 1, 30,  10, 8, 0, IRON, 1, HI_METAL),
HELM("helm of brilliance", "etched helmet",
		0, 1,  0,	6, 1, 50,  50, 8, 0, IRON, 0, CLR_GREEN),
HELM("helm of opposite alignment", "crested helmet",
		0, 1,  0,	6, 1, 50,  50, 8, 0, IRON, 0, HI_METAL),
HELM("helm of telepathy", "visored helmet",
		0, 1,  TELEPAT, 2, 1, 50,  50, 8, 0, IRON, 0, HI_METAL),
/* ARMOR: known, magicobj, (bulky), power, %, delay, weight, cost, AC, can, subtype, material, color */

/* suits of armor */
/*
 * There is code in polyself.c that assumes (1) and (2).
 * There is code in obj.h, objnam.c, mon.c, read.c that assumes (2).
 *
 *	(1) The dragon scale mails and the dragon scales are together.
 *	(2) That the order of the dragon scale mail and dragon scales is the
 *	    the same defined in monst.c.
 */
#define DRGN_ARMR(name,mgc,power,cost,ac,color) \
	ARMOR(name,(char *)0,1,mgc,1,power,0,5,250,cost,ac,0,ARM_SUIT,DRAGON_HIDE,0,color)
/* 3.4.1: dragon scale mail reclassified as "magic" since magic is
   needed to create them */
DRGN_ARMR("gray dragon scale mail",   1, ANTIMAGIC,  1200, 2, CLR_GRAY),
DRGN_ARMR("silver dragon scale mail", 1, REFLECTING, 1200, 2, DRAGON_SILVER),
#if 0	/* DEFERRED */
DRGN_ARMR("shimmering dragon scale mail", 1, DISPLACED, 1200, 2, CLR_CYAN),
#endif
DRGN_ARMR("red dragon scale mail",    1, FIRE_RES,    900, 2, CLR_RED),
DRGN_ARMR("white dragon scale mail",  1, COLD_RES,    900, 2, CLR_WHITE),
DRGN_ARMR("orange dragon scale mail", 1, FREE_ACTION, 900, 2, CLR_ORANGE),
DRGN_ARMR("black dragon scale mail",  1, DISINT_RES, 1200, 2, CLR_BLACK),
DRGN_ARMR("blue dragon scale mail",   1, SHOCK_RES,   900, 2, CLR_BLUE),
DRGN_ARMR("green dragon scale mail",  1, POISON_RES,  900, 2, CLR_GREEN),
DRGN_ARMR("deep dragon scale mail",   1, DRAIN_RES,   900, 2, CLR_BLACK),
DRGN_ARMR("plain dragon scale mail",  0, 0,           100, 2, CLR_BROWN),
DRGN_ARMR("yellow dragon scale mail", 1, ACID_RES,    900, 2, CLR_YELLOW),

/* For now, only dragons leave these. */
/* 3.4.1: dragon scales left classified as "non-magic"; they confer
   magical properties but are produced "naturally" */
DRGN_ARMR("gray dragon scales",   0, ANTIMAGIC,  700, 7, CLR_GRAY),
DRGN_ARMR("silver dragon scales", 0, REFLECTING, 700, 7, DRAGON_SILVER),
#if 0	/* DEFERRED */
DRGN_ARMR("shimmering dragon scales", 0, DISPLACED,  700, 7, CLR_CYAN),
#endif
DRGN_ARMR("red dragon scales",    0, FIRE_RES,   500, 7, CLR_RED),
DRGN_ARMR("white dragon scales",  0, COLD_RES,   500, 7, CLR_WHITE),
DRGN_ARMR("orange dragon scales", 0, FREE_ACTION,500, 7, CLR_ORANGE),
DRGN_ARMR("black dragon scales",  0, DISINT_RES, 700, 7, CLR_BLACK),
DRGN_ARMR("blue dragon scales",   0, SHOCK_RES,  500, 7, CLR_BLUE),
DRGN_ARMR("green dragon scales",  0, POISON_RES, 500, 7, CLR_GREEN),
DRGN_ARMR("deep dragon scales",   0, DRAIN_RES,  500, 7, CLR_BLACK),
DRGN_ARMR("plain dragon scales",  0, 0,           50, 7, CLR_BROWN),
DRGN_ARMR("yellow dragon scales", 0, ACID_RES,   500, 7, CLR_YELLOW),
#undef DRGN_ARMR

ARMOR("plate mail", (char *)0,
	1, 0, 1, 0,	80, 5, 450, 600,  0/*3*/, 2, ARM_SUIT, IRON, 1, HI_METAL),
//ARMOR("crystal plate mail", (char *)0,
//      1, 0, 1, 0,	10, 5, 450, 820,  1/*3*/, 2, ARM_SUIT, GLASS, 0, CLR_WHITE),
//ARMOR("bronze plate mail", (char *)0,
//	1, 0, 1, 0,	35, 5, 450, 400,  2/*4*/, 0, ARM_SUIT, COPPER, 0, HI_COPPER),
ARMOR("splint mail", (char *)0,
	1, 0, 1, 0,	60, 5, 400,  80,  2/*4*/, 1, ARM_SUIT, IRON, 0, HI_METAL),
ARMOR("banded mail", (char *)0,
	1, 0, 1, 0,	70, 5, 350,  90,  3/*4*/, 0, ARM_SUIT, IRON, 0, HI_METAL),
ARMOR("dwarvish mithril-coat", (char *)0,
	1, 0, 0, 0,	 6, 1, 100, 240,  4, 3, ARM_SUIT, MITHRIL, 0, HI_METAL),
ARMOR("elven mithril-coat", (char *)0,
	1, 0, 0, 0,	 6, 1, 100, 240,  5, 3, ARM_SUIT, MITHRIL, 0, HI_METAL),
ARMOR("chain mail", (char *)0,
	1, 0, 0, 0,	70, 5, 300,  75,  4/*5*/, 1, ARM_SUIT, IRON, 0, HI_METAL),
#ifdef TOURIST
ARMOR("orcish chain mail", "crude chain mail",
	0, 0, 0, 0,	10, 5, 300,  75,  5/*6*/, 1, ARM_SUIT, IRON, 0, CLR_BLACK),
#else
ARMOR("orcish chain mail", "crude chain mail",
	0, 0, 0, 0,	20, 5, 300,  75,  5/*6*/, 1, ARM_SUIT, IRON, 0, CLR_BLACK),
#endif
ARMOR("scale mail", (char *)0,
	1, 0, 0, 0,	70, 5, 250,  45,  5/*6*/, 0, ARM_SUIT, IRON, 0, HI_METAL),
ARMOR("studded leather armor", (char *)0,
	1, 0, 0, 0,	60, 3, 200,  15,  6/*7*/, 1, ARM_SUIT, LEATHER, 0, HI_LEATHER),
ARMOR("ring mail", (char *)0,
	1, 0, 0, 0,	70, 5, 250,  30,  6/*7*/, 0, ARM_SUIT, IRON, 0, HI_METAL),
ARMOR("orcish ring mail", "crude ring mail",
	0, 0, 0, 0,	20, 5, 250,  25,  7/*8*/, 1, ARM_SUIT, IRON, 0, CLR_BLACK),
ARMOR("leather armor", (char *)0,
	1, 0, 0, 0,	60, 3, 150,   5,  7/*8*/, 0, ARM_SUIT, LEATHER, 0, HI_LEATHER),
ARMOR("leather jacket", (char *)0,
	1, 0, 0, 0,	12, 0,	20,  10,  9, 0, ARM_SUIT, LEATHER, 0, CLR_BLACK),
/* dresses */
ARMOR("maid dress", "black dress",
	0, 1, 0, 0,	 7, 0,  20, 100,  9, 0, ARM_SUIT, CLOTH, 0, CLR_BLACK),
ARMOR("nurse uniform", "white dress",
	0, 0, 0, 0,	 0, 0,  15,  10,  9, 0, ARM_SUIT, CLOTH, 0, CLR_WHITE),
/* robes */
ARMOR("robe", "blue robe",
	0, 0, 0, 0,	12, 0,  20, 100,  9, 0, ARM_SUIT, CLOTH, 0, CLR_BLUE),
ARMOR("robe of protection", "red robe",
	0, 1, 0, PROTECTION,	/* +2 AC is gained by protection */
			 8, 0,  20, 100,  9, 3, ARM_SUIT, CLOTH, 0, CLR_RED),
ARMOR("robe of power", "black robe",
	0, 1, 0, 0,	10, 0,  20, 100,  9, 1, ARM_SUIT, CLOTH, 0, CLR_BLACK),
ARMOR("robe of weakness", "green robe",
	0, 1, 0, 0,	10, 0,  20, 100,  9, 0, ARM_SUIT, CLOTH, 0, CLR_GREEN),
/* ARMOR: known, magicobj, (bulky), power, %, delay, weight, cost, AC, can, subtype, material, color */

#ifdef TOURIST
/* shirts */
ARMOR("Hawaiian shirt", (char *)0,
	1, 0, 0, 0,	 8, 0,	 5,   3, 10, 0, ARM_SHIRT, CLOTH, 0, CLR_MAGENTA),
ARMOR("T-shirt", (char *)0,
	1, 0, 0, 0,	 2, 0,	 5,   2, 10, 0, ARM_SHIRT, CLOTH, 0, CLR_WHITE),
#endif

/* cloaks */
/*  'cope' is not a spelling mistake... leave it be */
CLOAK("mummy wrapping", (char *)0,
		1, 0,	0,	    0, 0,  3,  2, 10, 1, CLOTH, 0, CLR_GRAY),
CLOAK("elven cloak", "faded pall",
		0, 0,	STEALTH,    8, 0, 10, 60,  9, 3, CLOTH, 0, CLR_BLACK),
CLOAK("orcish cloak", "coarse mantelet",
		0, 0,	0,	    8, 0, 10, 40, 10, 2, CLOTH, 0, CLR_BLACK),
CLOAK("dwarvish cloak", "hooded cloak",
		0, 0,	0,	    8, 0, 10, 50, 10, 2, CLOTH, 0, HI_CLOTH),
CLOAK("oilskin cloak", "slippery cloak",
		0, 0,	0,	    8, 0, 10, 50,  9, 3, CLOTH, 0, HI_CLOTH),
CLOAK("kitchen apron", "apron",
		0, 0,	0,	    6, 0,  5, 50, 10, 0, CLOTH, 0, CLR_WHITE),
CLOAK("frilled apron", "apron",
		0, 0,	0,	    5, 0,  5, 50, 10, 0, CLOTH, 0, CLR_WHITE),
CLOAK("alchemy smock", "apron",
		0, 1,	POISON_RES, 6, 0, 10, 50, 10, 1, CLOTH, 0, CLR_WHITE),
CLOAK("leather cloak", (char *)0,
		1, 0,	0,	    8, 0, 15, 40,  9, 1, LEATHER, 0, CLR_BROWN),
/* With shuffled appearances... */
CLOAK("cloak of protection", "tattered cape",	/* +2 AC is gained by protection */
		0, 1,	PROTECTION, 9, 0, 10, 50,  9, 3, CLOTH, 0, HI_CLOTH),
CLOAK("cloak of invisibility", "opera cloak",
		0, 1,	INVIS,	   10, 0, 10, 60,  9, 2, CLOTH, 0, CLR_BRIGHT_MAGENTA),
CLOAK("cloak of magic resistance", "ornamental cope",
		0, 1,	ANTIMAGIC,  2, 0, 10, 60,  9, 2, CLOTH, 0, CLR_WHITE),
CLOAK("cloak of displacement", "piece of cloth",
		0, 1,	DISPLACED, 10, 0, 10, 50,  9, 2, CLOTH, 0, HI_CLOTH),

/* shields */
SHIELD("small shield", (char *)0,
		1, 0, 0, 0,	     6, 0, 30,	3,  9, 1, WOOD, 0, HI_WOOD),
SHIELD("elven shield", "blue and green shield",
		0, 0, 0, 0,	     2, 0, 40,	7,  8, 2, WOOD, 0, CLR_GREEN),
SHIELD("Uruk-hai shield", "white-handed shield",
		0, 0, 0, 0,	     2, 0, 50,	7,  9, 2, IRON, 0, HI_METAL),
SHIELD("orcish shield", "red-eyed shield",
		0, 0, 0, 0,	     2, 0, 50,	7,  9, 2, IRON, 0, CLR_RED),
SHIELD("large shield", (char *)0,
		1, 0, 1, 0,	     7, 0,100, 10,  7/*8*/, 3, IRON, 0, HI_METAL),
SHIELD("dwarvish roundshield", "large round shield",
		0, 0, 0, 0,	     4, 0,100, 10,  7/*8*/, 3, IRON, 0, HI_METAL),
SHIELD("shield of firebreak", "red-lined shield",
		0, 1, 0, 0,	     5, 0, 50, 50,  8, 2, IRON, 0, CLR_RED),
SHIELD("shield of isolation", "blue-lined shield",
		0, 1, 0, 0,	     5, 0, 50, 50,  8, 2, IRON, 0, CLR_BLUE),
SHIELD("shield of reflection", "polished silver shield",
		0, 1, 0, REFLECTING, 3, 0, 50, 50,  8, 2, SILVER, 0, HI_SILVER),
/* a shield of reflection turns into a mere silver shield if it lost its power */
SHIELD("shield", (char *)0,
		1, 0, 0, 0,          0, 0, 50,  5,  8, 2, IRON, 1, HI_METAL),

/* gloves */
/* these have their color but not material shuffled, so the IRON must stay
 * CLR_BROWN (== HI_LEATHER)
 */
GLOVES("leather gloves", "old gloves",
		0, 0,  0,	  16, 1, 10,  8, 10, 0, LEATHER, 0, HI_LEATHER),
GLOVES("gauntlets of fumbling", "padded gloves",
		0, 1,  FUMBLING,   8, 1, 10, 50, 10, 0, LEATHER, 0, HI_LEATHER),
GLOVES("gauntlets of power", "riding gloves",
		0, 1,  0,	   8, 1, 30, 50,  9, 0, IRON, 0, CLR_BROWN),
GLOVES("gauntlets of dexterity", "fencing gloves",
		0, 1,  0,	   8, 1, 10, 50, 10, 0, LEATHER, 0, HI_LEATHER),
GLOVES("gauntlets", (char *)0,
		1, 0,  0,	   0, 1, 30, 10,  9, 0, IRON, 1, HI_METAL),

/* boots */
BOOTS("low boots", "walking shoes",
		0, 0,  0,	  25, 2, 10,  8,  9, 0, LEATHER, 0, HI_LEATHER),
BOOTS("iron shoes", "hard shoes",
		0, 0,  0,	   7, 2, 50, 16,  8, 0, IRON, 0, HI_METAL),
BOOTS("high boots", "jackboots",
		0, 0,  0,	  15, 2, 20, 12,  8, 0, LEATHER, 0, HI_LEATHER),
/* With shuffled appearances... */
BOOTS("speed boots", "combat boots",
		0, 1, 0/*FAST*/,  12, 2, 20, 50,  9, 0, LEATHER, 0, HI_LEATHER),
BOOTS("water walking boots", "jungle boots",
		0, 1,  WWALKING,  12, 2, 20, 50,  9, 0, LEATHER, 0, HI_LEATHER),
BOOTS("jumping boots", "hiking boots",
		0, 1,  JUMPING,   12, 2, 20, 50,  9, 0, LEATHER, 0, HI_LEATHER),
BOOTS("elven boots", "mud boots",
		0, 0,  STEALTH,   12, 2, 15,  8,  9, 0, LEATHER, 0, HI_LEATHER),
BOOTS("kicking boots", "buckled boots",
		0, 1,  0,         12, 2, 15,  8,  9, 0, IRON, 0, CLR_BROWN),
BOOTS("fumble boots", "riding boots",
		0, 1,  FUMBLING,  12, 2, 20, 30,  9, 0, LEATHER, 0, HI_LEATHER),
BOOTS("levitation boots", "snow boots",
		0, 1,  LEVITATION,12, 2, 15, 30,  9, 0, LEATHER, 0, HI_LEATHER),
#undef HELM
#undef CLOAK
#undef SHIELD
#undef GLOVES
#undef BOOTS
#undef ARMOR

/* rings ... */
#define RING(name,power,stone,cost,mgc,spec,mohs,metal,color) OBJECT( \
		OBJ(name,stone), \
		BITS(0,0,spec,0,mgc,spec,0,0,0,HARDGEM(mohs),0,P_NONE,metal), \
		power, RING_CLASS, 0, 0, 3, cost, 0, 0, 0, 0, 15, color )
RING("adornment", ADORNED, "wooden",        100, 1, 1, 2, WOOD, HI_WOOD),
RING("gain strength", 0, "granite",         150, 1, 1, 7, MINERAL, HI_MINERAL),
RING("gain constitution", 0, "opal",        150, 1, 1, 7, MINERAL,  HI_MINERAL),
RING("increase accuracy", 0, "clay",        150, 1, 1, 4, MINERAL, CLR_RED),
RING("increase damage", 0, "coral",         150, 1, 1, 4, MINERAL, CLR_ORANGE),
RING("protection", PROTECTION, "black onyx",100, 1, 1, 7, MINERAL, CLR_BLACK),
RING("regeneration", REGENERATION, "moonstone",
					    200, 1, 0, 6, MINERAL, HI_MINERAL),
RING("searching", SEARCHING, "tiger eye",   200, 1, 0, 6, GEMSTONE, CLR_BROWN),
RING("stealth", STEALTH, "jade",            100, 1, 0, 6, GEMSTONE, CLR_GREEN),
RING("sustain ability", FIXED_ABIL, "bronze",
					    100, 1, 0, 4, COPPER, HI_COPPER),
RING("levitation", LEVITATION, "agate",     200, 1, 0, 7, GEMSTONE, CLR_RED),
RING("hunger", HUNGER, "topaz",             100, 1, 0, 8, GEMSTONE, CLR_CYAN),
RING("aggravate monster", AGGRAVATE_MONSTER, "sapphire",
					    150, 1, 0, 9, GEMSTONE, CLR_BLUE),
RING("conflict", CONFLICT, "ruby",          300, 1, 0, 9, GEMSTONE, CLR_RED),
RING("warning", WARNING, "diamond",         100, 1, 0,10, GEMSTONE, CLR_WHITE),
RING("poison resistance", POISON_RES, "pearl",
					    150, 1, 0, 4, IRON, CLR_WHITE),
RING("fire resistance", FIRE_RES, "iron",   200, 1, 0, 5, IRON, HI_METAL),
RING("cold resistance", COLD_RES, "brass",  150, 1, 0, 4, COPPER, HI_COPPER),
RING("shock resistance", SHOCK_RES, "copper",
					    150, 1, 0, 3, COPPER, HI_COPPER),
RING("free action",     FREE_ACTION, "twisted",
					    200, 1, 0, 6, IRON, HI_METAL),
RING("slow digestion",  SLOW_DIGESTION, "steel",
					    200, 1, 0, 8, IRON, HI_METAL),
RING("teleportation", TELEPORT, "silver",   200, 1, 0, 3, SILVER, HI_SILVER),
RING("teleport control", TELEPORT_CONTROL, "gold",
					    300, 1, 0, 3, GOLD, HI_GOLD),
RING("polymorph", POLYMORPH, "ivory",       300, 1, 0, 4, BONE, CLR_WHITE),
RING("polymorph control", POLYMORPH_CONTROL, "emerald",
					    300, 1, 0, 8, GEMSTONE, CLR_BRIGHT_GREEN),
RING("invisibility", INVIS, "wire",         150, 1, 0, 5, IRON, HI_METAL),
RING("see invisible", SEE_INVIS, "engagement",
					    150, 1, 0, 5, IRON, HI_METAL),
RING("protection from shape changers", PROT_FROM_SHAPE_CHANGERS, "shiny",
					    100, 1, 0, 5, IRON, CLR_BRIGHT_CYAN),
#undef RING

/* amulets ... - THE Amulet comes last because it is special */
#define AMULET(name,desc,power,prob) OBJECT( \
		OBJ(name,desc), BITS(0,0,0,0,1,0,0,0,0,0,0,P_NONE,IRON), power, \
		AMULET_CLASS, prob, 0, 20, 150, 0, 0, 0, 0, 20, HI_METAL )

AMULET("amulet of ESP",           "circular",   TELEPAT,    175),
AMULET("amulet of life saving",   "spherical",  LIFESAVED,   75),
AMULET("amulet of strangulation", "oval",       STRANGLED,  135),
AMULET("amulet of restful sleep", "triangular", SLEEPING,   135),
AMULET("amulet versus poison",    "pyramidal",  POISON_RES, 165),
AMULET("amulet of change",        "square",     0,          130),
						/* POLYMORPH */
AMULET("amulet of unchanging",    "concave",    UNCHANGING,	 45),
AMULET("amulet of reflection",    "hexagonal",  REFLECTING,  75),
AMULET("amulet of magical breathing", "octagonal",      MAGICAL_BREATHING, 65),
OBJECT(OBJ("cheap plastic imitation of the Amulet of Yendor",
	"Amulet of Yendor"), BITS(0,0,1,0,0,0,0,0,0,0,0,0,PLASTIC), 0,
	AMULET_CLASS, 0, 0, 20,    0, 0, 0, 0, 0,  1, HI_METAL),
OBJECT(OBJ("Amulet of Yendor",	/* note: description == name */
	"Amulet of Yendor"), BITS(0,0,1,0,1,0,1,1,0,0,0,0,MITHRIL), 0,
	AMULET_CLASS, 0, 0, 20, 30000, 0, 0, 0, 0, 20, HI_METAL),
#undef AMULET

/* tools ... */
#ifdef JP
#define	LOOKING_GLASS	(char *)0
#else /*JP*/
#define	LOOKING_GLASS	"looking glass"
#endif /*JP*/
/* tools with weapon characteristics come last */
#define TOOL(name,desc,kn,mrg,mgc,chg,prob,wt,cost,mat,color) \
	OBJECT( OBJ(name,desc), \
		BITS(kn,mrg,chg,0,mgc,chg,0,0,0,0,0,P_NONE,mat), \
		0, TOOL_CLASS, prob, 0, \
		wt, cost, 0, 0, 0, 0, wt, color )
#define CONTAINER(name,desc,kn,mgc,chg,prob,wt,cost,mat,color) \
	OBJECT( OBJ(name,desc), \
		BITS(kn,0,chg,1,mgc,chg,0,0,0,0,0,P_NONE,mat), \
		0, TOOL_CLASS, prob, 0, \
		wt, cost, 0, 0, 0, 0, wt, color )
#define WEPTOOL(name,desc,kn,mgc,bi,prob,wt,cost,sdam,ldam,hitbon,typ,sub,sub2,mat,mat2,clr) \
	OBJECT( OBJ(name,desc), \
		BITS(kn,0,1,0,mgc,1,0,0,bi,mat2,typ,sub,mat), \
		0, TOOL_CLASS, prob, sub2, \
		wt, cost, sdam, ldam, hitbon, 0, wt, clr )
#ifdef MAGIC_GLASSES
#define GLASSES(name,desc,mgc,power,prob,wt,cost) \
	OBJECT( OBJ(name,desc), \
		BITS(0,0,0,0,mgc,0,0,0,0,0,0,P_NONE,GLASS), \
		power, TOOL_CLASS, prob, 0, \
		wt, cost, 0, 0, 0, 0, wt, HI_GLASS)
#endif
/* containers */
CONTAINER("large box", (char *)0,       1, 0, 0,  40,350,   8, WOOD, HI_WOOD),
CONTAINER("chest", (char *)0,           1, 0, 0,  35,600,  16, WOOD, HI_WOOD),
CONTAINER("ice box", (char *)0,         1, 0, 0,   5,900,  42, PLASTIC, CLR_WHITE),
CONTAINER("sack", "bag",                0, 0, 0,  35, 15,   2, CLOTH, HI_CLOTH),
CONTAINER("oilskin sack", "bag",        0, 0, 0,  15, 15, 100, CLOTH, HI_CLOTH),
CONTAINER("bag of holding", "bag",      0, 1, 0,  20, 15, 100, CLOTH, HI_CLOTH),
CONTAINER("bag of tricks", "bag",       0, 1, 1,  20, 15, 100, CLOTH, HI_CLOTH),
#undef CONTAINER

/* lock opening tools */
TOOL("skeleton key", "key",     0, 0, 0, 0,  80,  3,  10, IRON, HI_METAL),
#ifdef TOURIST
TOOL("lock pick", (char *)0,    1, 0, 0, 0,  60,  4,  20, IRON, HI_METAL),
TOOL("credit card", (char *)0,  1, 0, 0, 0,  15,  1,  10, PLASTIC, CLR_WHITE),
#else
TOOL("lock pick", (char *)0,    1, 0, 0, 0,  75,  4,  20, IRON, HI_METAL),
#endif
/* light sources */
TOOL("tallow candle", "candle", 0, 1, 0, 0,  18,  2,  10, WAX, CLR_WHITE),
TOOL("wax candle", "candle",    0, 1, 0, 0,  15,  2,  20, WAX, CLR_WHITE),
TOOL("magic candle", "candle",  0, 1, 1, 0,   5,  2, 500, WAX, CLR_WHITE),
TOOL("brass lantern", (char *)0,1, 0, 0, 0,  30, 30,  12, COPPER, CLR_YELLOW),
TOOL("oil lamp", "lamp",        0, 0, 0, 0,  45, 20,  10, COPPER, CLR_YELLOW),
TOOL("magic lamp", "lamp",      0, 0, 1, 0,   2, 20,1000, COPPER, CLR_YELLOW),
/* other tools */
#ifdef TOURIST
TOOL("expensive camera", (char *)0,
				1, 0, 0, 1,  15, 12, 200, PLASTIC, CLR_BLACK),
TOOL("mirror", LOOKING_GLASS,   0, 0, 0, 0,  30, 13,  10, GLASS, HI_SILVER),
#else
TOOL("mirror", LOOKING_GLASS,   0, 0, 0, 0,  45, 13,  10, GLASS, HI_SILVER),
#endif
TOOL("crystal ball", "glass orb",
				0, 0, 1, 1,  15, 75,  60, GLASS, HI_GLASS),
/* STEPHEN WHITE'S NEW CODE */
TOOL("orb of maintenance", "glass orb",
				0, 0, 1, 1,   5, 75, 750, GLASS, HI_GLASS),
TOOL("orb of charging", "glass orb",
				0, 0, 1, 1,   5, 75, 750, GLASS, HI_GLASS),
TOOL("orb of destruction", "glass orb",
				0, 0, 1, 0,   5, 75, 750, GLASS, HI_GLASS),
#ifndef MAGIC_GLASSES
TOOL("lenses", (char *)0,	1, 0, 0, 0,  15,  3,  80, GLASS, HI_GLASS),
#else
GLASSES("glasses of magic reading",   "wire-rimmed glasses",   0, 0,	      10, 3, 80),
GLASSES("glasses of gaze protection", "rimless glasses",       1, 0,	       1, 3, 80),
GLASSES("glasses of infravision",     "horn-rimmed glasses",   1, INFRAVISION, 1, 3, 80),
GLASSES("glasses versus flash",	      "silver-rimmed glasses", 1, 0,	       1, 3, 80),
GLASSES("glasses of see invisible",   "thick glasses",	       1, SEE_INVIS,   1, 3, 80),
GLASSES("glasses of phantasmagoria",  "gold-rimmed glasses",   1, HALLUC,      1, 3, 80),
#endif
TOOL("blindfold", (char *)0,    1, 0, 0, 0,  50,  2,  20, CLOTH, CLR_BLACK),
TOOL("towel", (char *)0,        1, 0, 0, 0,  50,  2,  50, CLOTH, CLR_MAGENTA),
#ifdef STEED
TOOL("saddle", (char *)0,       1, 0, 0, 0,   5,200,  15, LEATHER, HI_LEATHER),
TOOL("leash", (char *)0,        1, 0, 0, 0,  45, 12,  20, LEATHER, HI_LEATHER),
#else
TOOL("leash", (char *)0,        1, 0, 0, 0,  50, 12,  20, LEATHER, HI_LEATHER),
#endif
TOOL("stethoscope", (char *)0,  1, 0, 0, 0,  25,  4,  75, IRON, HI_METAL),
TOOL("tinning kit", (char *)0,  1, 0, 0, 1,  15,100,  30, IRON, HI_METAL),
TOOL("tin opener", (char *)0,   1, 0, 0, 0,  35,  4,  30, IRON, HI_METAL),
TOOL("can of grease", (char *)0,1, 0, 0, 1,  15, 15,  20, IRON, HI_METAL),
TOOL("figurine", (char *)0,     1, 0, 1, 0,  25, 50,  80, MINERAL, HI_MINERAL),
TOOL("magic marker", (char *)0, 1, 0, 1, 1,  15,  2,1000, PLASTIC, CLR_RED),
#ifdef FIRSTAID
TOOL("scissors", (char *)0,     1, 0, 0, 0,  20,  5,  30, IRON, HI_METAL),
TOOL("bandage", (char *)0,      1, 1, 0, 0,  40,  2,   2, CLOTH, CLR_WHITE),
#endif
/* traps */
TOOL("land mine",(char *)0,     1, 0, 0, 0,   0,300, 180, IRON, CLR_RED),
TOOL("beartrap", (char *)0,     1, 0, 0, 0,   0,200,  60, IRON, HI_METAL),
/* instruments */
#ifdef FIRSTAID
TOOL("tin whistle", "whistle",  0, 0, 0, 0,  40,  3,  10, METAL, HI_METAL),
#else
TOOL("tin whistle", "whistle",  0, 0, 0, 0,  65,  3,  10, METAL, HI_METAL),
#endif
TOOL("magic whistle", "whistle",0, 0, 1, 1,  30,  3,  10, METAL, HI_METAL),
/* "If tin whistles are made out of tin, what do they make foghorns out of?" */
TOOL("wooden flute", "flute",   0, 0, 0, 0,   4,  5,  12, WOOD, HI_WOOD),
TOOL("magic flute", "flute",    0, 0, 1, 1,   2,  5,  36, WOOD, HI_WOOD),
TOOL("tooled horn", "horn",     0, 0, 0, 0,   5, 18,  15, BONE, CLR_WHITE),
TOOL("frost horn", "horn",      0, 0, 1, 1,   2, 18,  50, BONE, CLR_WHITE),
TOOL("fire horn", "horn",       0, 0, 1, 1,   2, 18,  50, BONE, CLR_WHITE),
TOOL("horn of plenty", "horn",  0, 0, 1, 1,   2, 18,  50, BONE, CLR_WHITE),
TOOL("wooden harp", "harp",     0, 0, 0, 0,   4, 30,  50, WOOD, HI_WOOD),
TOOL("magic harp", "harp",      0, 0, 1, 1,   2, 30,  50, WOOD, HI_WOOD),
TOOL("bell", (char *)0,         1, 0, 0, 0,   2, 30,  50, COPPER, HI_COPPER),
TOOL("bugle", (char *)0,        1, 0, 0, 0,   4, 10,  15, COPPER, HI_COPPER),
TOOL("leather drum", "drum",    0, 0, 0, 0,   4, 25,  25, LEATHER, HI_LEATHER),
TOOL("drum of earthquake", "drum",
				0, 0, 1, 1,   2, 25,  25, LEATHER, HI_LEATHER),
/* tools useful as weapons */
WEPTOOL("pick-axe", (char *)0,         1, 0, 0,
	20, 100,  50, DAM(1, 6), DAM(1, 3), 0, WHACK,  P_PICKAXE_GROUP,       0, IRON, 0, HI_METAL),
WEPTOOL("grappling hook", "iron hook", 0, 0, 0,
	 5,  30,  50, DAM(1, 2), DAM(1, 6), 0, WHACK,  P_FLAIL_GROUP, WP_RANGED, IRON, 0, HI_METAL),
/* 3.4.1: unicorn horn left classified as "magic" */
WEPTOOL("unicorn horn", (char *)0,     1, 1, 1,
	 0,  20, 100, DAM(1,12), DAM(1,12), 2, PIERCE, P_SPEAR_GROUP,         0, BONE, 0, CLR_WHITE),

/* two special unique artifact "tools" */
OBJECT(OBJ("Candelabrum of Invocation", "candelabrum"),
		BITS(0,0,1,0,1,0,1,1,0,0,0,P_NONE,GOLD), 0,
		TOOL_CLASS, 0, 0,10, 5000, 0, 0, 0, 0, 200, HI_GOLD),
OBJECT(OBJ("Bell of Opening", "silver bell"),
		BITS(0,0,1,0,1,1,1,1,0,0,0,P_NONE,SILVER), 0,
		TOOL_CLASS, 0, 0,10, 5000, 0, 0, 0, 0, 50, HI_SILVER),
#undef TOOL
#undef WEPTOOL

/* Comestibles ... */
#define FOOD(name,prob,delay,wt,unk,tin,nutrition,color) OBJECT( \
		OBJ(name,(char *)0), BITS(1,1,unk,0,0,0,0,0,0,0,0,P_NONE,tin), 0, \
		FOOD_CLASS, prob, delay, \
		wt, nutrition/20 + 5, 0, 0, 0, 0, nutrition, color )
/* all types of food (except tins & corpses) must have a delay of at least 1. */
/* delay on corpses is computed and is weight dependant */
/* dog eats foods 0-4 but prefers tripe rations above all others */
/* fortune cookies can be read */
/* carrots improve your vision */
/* +0 tins contain monster meat */
/* +1 tins (of spinach) make you stronger (like Popeye) */
/* food CORPSE is a cadaver of some type */
/* meatballs/sticks/rings are only created from objects via stone to flesh */

/* meat */
FOOD("tripe ration",       140, 2, 10, 0, FLESH, 200, CLR_BROWN),
FOOD("corpse",               0, 1,  0, 0, FLESH,   0, CLR_BROWN),
FOOD("egg",                 85, 1,  1, 1, FLESH,  80, CLR_WHITE),
FOOD("meatball",             0, 1,  1, 0, FLESH,   5, CLR_BROWN),
FOOD("meat stick",           0, 1,  1, 0, FLESH,   5, CLR_BROWN),
FOOD("huge chunk of meat",   0,20,400, 0, FLESH,2000, CLR_BROWN),
/* special case because it's not mergable */
OBJECT(OBJ("meat ring", (char *)0),
    BITS(1,0,0,0,0,0,0,0,0,0,0,0,FLESH),
    0, FOOD_CLASS, 0, 1, 5, 1, 0, 0, 0, 0, 5, CLR_BROWN),

/* fruits & veggies */
FOOD("kelp frond",           0, 1,  1, 0, VEGGY,  30, CLR_GREEN),
FOOD("eucalyptus leaf",      3, 1,  1, 0, VEGGY,  30, CLR_GREEN),
FOOD("apple",               15, 1,  2, 0, VEGGY,  50, CLR_RED),
FOOD("orange",              10, 1,  2, 0, VEGGY,  80, CLR_ORANGE),
FOOD("pear",                10, 1,  2, 0, VEGGY,  50, CLR_BRIGHT_GREEN),
FOOD("melon",               10, 1,  5, 0, VEGGY, 100, CLR_BRIGHT_GREEN),
FOOD("banana",              10, 1,  2, 0, VEGGY,  80, CLR_YELLOW),
FOOD("carrot",              15, 1,  2, 0, VEGGY,  50, CLR_ORANGE),
FOOD("sprig of wolfsbane",   7, 1,  1, 0, VEGGY,  40, CLR_GREEN),
FOOD("clove of garlic",      7, 1,  1, 0, VEGGY,  40, CLR_WHITE),
FOOD("slime mold",          75, 1,  5, 0, VEGGY, 250, HI_ORGANIC),

/* people food */
FOOD("lump of royal jelly",  0, 1,  2, 0, VEGGY, 200, CLR_YELLOW),
FOOD("cream pie",           25, 1, 10, 0, VEGGY, 100, CLR_WHITE),
FOOD("candy bar",           13, 1,  2, 0, VEGGY, 100, CLR_BROWN),
FOOD("fortune cookie",      55, 1,  1, 0, VEGGY,  40, CLR_YELLOW),
FOOD("pancake",             25, 2,  2, 0, VEGGY, 200, CLR_YELLOW),
FOOD("lembas wafer",        20, 2,  5, 0, VEGGY, 800, CLR_WHITE),
FOOD("cram ration",         20, 3, 15, 0, VEGGY, 600, HI_ORGANIC),
FOOD("food ration",        380, 5, 20, 0, VEGGY, 800, HI_ORGANIC),
FOOD("K-ration",             0, 1, 10, 0, VEGGY, 400, HI_ORGANIC),
FOOD("C-ration",             0, 1, 10, 0, VEGGY, 300, HI_ORGANIC),
FOOD("tin",                 75, 0, 10, 1, METAL,   0, HI_METAL),
#undef FOOD

/* potions ... */
#define POTION(name,desc,mgc,power,prob,cost,color) OBJECT( \
		OBJ(name,desc), BITS(0,1,0,0,mgc,0,0,0,0,0,0,P_NONE,GLASS), power, \
		POTION_CLASS, prob, 0, 20, cost, 0, 0, 0, 0, 10, color )
POTION("gain ability", "ruby",          1, 0,          10, 300, CLR_RED),
POTION("restore ability", "pink",       1, 0,          66, 100, CLR_BRIGHT_MAGENTA),
POTION("confusion", "orange",           1, CONFUSION,  40, 100, CLR_ORANGE),
POTION("blindness", "yellow",           1, BLINDED,    50, 150, CLR_YELLOW),
POTION("paralysis", "emerald",          1, 0,          40, 300, CLR_BRIGHT_GREEN),
POTION("speed", "dark green",           1, FAST,       40, 200, CLR_GREEN),
POTION("levitation", "cyan",            1, LEVITATION, 40, 200, CLR_CYAN),
POTION("hallucination", "sky blue",     1, HALLUC,     50, 100, CLR_CYAN),
POTION("invisibility", "brilliant blue",1, INVIS,      40, 150, CLR_BRIGHT_BLUE),
POTION("see invisible", "magenta",      1, SEE_INVIS,  40,  50, CLR_MAGENTA),
POTION("healing", "purple-red",         1, 0,          72, 100, CLR_MAGENTA),
POTION("extra healing", "puce",         1, 0,          47, 100, CLR_RED),
POTION("gain level", "milky",           1, 0,          10, 300, CLR_WHITE),
POTION("enlightenment", "swirly",       1, 0,          20, 200, CLR_BROWN),
POTION("monster detection", "bubbly",   1, 0,          40, 150, CLR_WHITE),
POTION("object detection", "smoky",     1, 0,          40, 150, CLR_GRAY),
POTION("gain energy", "cloudy",         1, 0,          40, 150, CLR_WHITE),
POTION("sleeping",  "effervescent",     1, 0,          40, 100, CLR_GRAY),
POTION("full healing",  "black",        1, 0,          10, 200, CLR_BLACK),
POTION("polymorph", "golden",           1, 0,          10, 200, CLR_YELLOW),
POTION("booze", "brown",                0, 0,          35,  50, CLR_BROWN),
POTION("sickness", "fizzy",             0, 0,          40,  50, CLR_CYAN),
POTION("fruit juice", "dark",           0, 0,          40,  50, CLR_BLACK),
POTION("acid", "white",                 0, 0,          10, 250, CLR_WHITE),
POTION("oil", "murky",                  0, 0,          40, 250, CLR_BROWN),
POTION("water", "clear",                0, 0,          90, 100, CLR_CYAN),
#undef POTION

/* scrolls ... */
#define SCROLL(name,text,mgc,prob,cost) OBJECT( \
		OBJ(name,text), BITS(0,1,0,0,mgc,0,0,0,0,0,0,P_NONE,PAPER), 0, \
		SCROLL_CLASS, prob, 0, 5, cost, 0, 0, 0, 0, 6, HI_PAPER )
	SCROLL("enchant armor",         "ZELGO MER",            1,  43, 300),
	SCROLL("destroy armor",         "JUYED AWK YACC",       1,  45, 100),
	SCROLL("confuse monster",       "NR 9",                 1,  53, 100),
	SCROLL("scare monster",         "XIXAXA XOXAXA XUXAXA", 1,  35, 100),
	SCROLL("remove curse",          "PRATYAVAYAH",          1, 105, 100),
	SCROLL("enchant weapon",        "DAIYEN FOOELS",        1,  50, 300),
	SCROLL("create monster",        "LEP GEX VEN ZEA",      1,  45, 200),
	SCROLL("taming",                "PRIRUTSENIE",          1,  25, 200),
	SCROLL("genocide",              "ELBIB YLOH",           1,  15, 500),
	SCROLL("light",                 "VERR YED HORRE",       1,  50,  50),
	SCROLL("teleportation",         "VENZAR BORGAVVE",      1,  55, 100),
	SCROLL("gold detection",        "THARR",                1,  33, 100),
	SCROLL("food detection",        "YUM YUM",              1,  30,  50),
	SCROLL("identify",              "KERNOD WEL",           1, 180,  50),
	SCROLL("magic mapping",         "ELAM EBOW",            1,  50, 100),
	SCROLL("amnesia",               "DUAM XNAHT",           1,  35, 200),
	SCROLL("fire",                  "ANDOVA BEGARIN",       1,  35, 100),
	SCROLL("earth",                 "KIRJE",                1,  18,  50),
	SCROLL("punishment",            "VE FORBRYDERNE",       1,  35, 300),
	SCROLL("charging",              "HACKEM MUCHE",         1,  15, 500),
	SCROLL("stinking cloud",	"VELOX NEB",            1,  20, 300),
	SCROLL((char *)0,		"FOOBIE BLETCH",        1,   0, 100),
	SCROLL((char *)0,		"TEMOV",                1,   0, 100),
	SCROLL((char *)0,		"GARVEN DEH",           1,   0, 100),
	SCROLL((char *)0,		"READ ME",              1,   0, 100),
	SCROLL((char *)0,		"MAPIRO MAHAMA DIROMAT",1,   0, 100),
	SCROLL((char *)0,		"VAS CORP BET MANI",    1,   0, 100),
	/* these must come last because they have special descriptions */
#ifdef MAIL
	SCROLL("mail",                  "stamped",          0,   0,   0),
#endif
	SCROLL("blank paper",           "unlabeled",        0,  28,  5),
#undef SCROLL

/* spellbooks ... */
#define SPELL(name,desc,sub,prob,delay,level,mgc,dir,color) OBJECT( \
		OBJ(name,desc), BITS(0,0,0,0,mgc,0,0,0,0,0,dir,sub,PAPER), 0, \
		SPBOOK_CLASS, prob, delay, \
		50, level*100, 0, 0, 0, level, 20, color )
SPELL("dig",             "parchment",   P_MATTER_SPELL, 20,  6, 5, 1, RAY,       HI_PAPER),
SPELL("magic missile",   "vellum",      P_ATTACK_SPELL, 45,  2, 2, 1, RAY,       HI_PAPER),
SPELL("fireball",        "ragged",      P_ATTACK_SPELL, 20,  4, 4, 1, RAY,       HI_PAPER),
SPELL("cone of cold",    "dog eared",   P_ATTACK_SPELL, 10,  7, 4, 1, RAY,       HI_PAPER),
SPELL("sleep",           "mottled",     P_ENCHANTMENT_SPELL, 50,  1, 1, 1, RAY,       HI_PAPER),
SPELL("finger of death", "stained",     P_ATTACK_SPELL,  5, 10, 7, 1, RAY,       HI_PAPER),
SPELL("light",           "cloth",       P_DIVINATION_SPELL, 40,  1, 1, 1, NODIR,     HI_CLOTH),
SPELL("torch",           "khaki",       P_DIVINATION_SPELL,  0,  1, 1, 1, NODIR,     HI_CLOTH),
SPELL("detect monsters", "leather",     P_DIVINATION_SPELL, 40,  1, 1, 1, NODIR,     HI_LEATHER),
SPELL("healing",         "white",       P_HEALING_SPELL, 40,  2, 1, 1, IMMEDIATE, CLR_WHITE),
SPELL("knock",           "pink",        P_MATTER_SPELL, 35,  1, 1, 1, IMMEDIATE, CLR_BRIGHT_MAGENTA),
SPELL("force bolt",      "red",         P_ATTACK_SPELL, 35,  2, 1, 1, IMMEDIATE, CLR_RED),
SPELL("confuse monster", "orange",      P_ENCHANTMENT_SPELL, 25,  2, 2, 1, IMMEDIATE, CLR_ORANGE),
SPELL("cure blindness",  "yellow",      P_HEALING_SPELL, 25,  2, 2, 1, IMMEDIATE, CLR_YELLOW),
SPELL("drain life",      "velvet",      P_ATTACK_SPELL, 10,  2, 2, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("slow monster",    "light green", P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_BRIGHT_GREEN),
SPELL("wizard lock",     "dark green",  P_MATTER_SPELL, 30,  3, 2, 1, IMMEDIATE, CLR_GREEN),
SPELL("create monster",  "turquoise",   P_CLERIC_SPELL, 35,  3, 2, 1, NODIR,     CLR_BRIGHT_CYAN),
SPELL("detect food",     "cyan",        P_DIVINATION_SPELL, 30,  3, 2, 1, NODIR,     CLR_CYAN),
SPELL("cause fear",      "light blue",  P_ENCHANTMENT_SPELL, 25,  3, 3, 1, NODIR,     CLR_BRIGHT_BLUE),
SPELL("clairvoyance",    "dark blue",   P_DIVINATION_SPELL, 15,  3, 3, 1, NODIR,     CLR_BLUE),
SPELL("cure sickness",   "indigo",      P_HEALING_SPELL, 32,  3, 3, 1, NODIR,     CLR_BLUE),
SPELL("charm monster",   "magenta",     P_ENCHANTMENT_SPELL, 20,  3, 3, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("haste self",      "purple",      P_ESCAPE_SPELL, 30,  4, 3, 1, NODIR,     CLR_MAGENTA),
SPELL("detect unseen",   "violet",      P_DIVINATION_SPELL, 20,  4, 3, 1, NODIR,     CLR_MAGENTA),
SPELL("levitation",      "tan",         P_ESCAPE_SPELL, 20,  4, 4, 1, NODIR,     CLR_BROWN),
SPELL("extra healing",   "plaid",       P_HEALING_SPELL, 27,  5, 3, 1, IMMEDIATE, CLR_GREEN),
SPELL("restore ability", "light brown", P_HEALING_SPELL, 25,  5, 4, 1, NODIR,     CLR_BROWN),
SPELL("invisibility",    "dark brown",  P_ESCAPE_SPELL, 25,  5, 4, 1, NODIR,     CLR_BROWN),
SPELL("detect treasure", "gray",        P_DIVINATION_SPELL, 20,  5, 4, 1, NODIR,     CLR_GRAY),
SPELL("remove curse",    "wrinkled",    P_CLERIC_SPELL, 25,  5, 3, 1, NODIR,     HI_PAPER),
SPELL("magic mapping",   "dusty",       P_DIVINATION_SPELL, 15,  7, 5, 1, NODIR,     HI_PAPER),
SPELL("identify",        "bronze",      P_DIVINATION_SPELL, 20,  6, 3, 1, NODIR,     HI_COPPER),
SPELL("turn undead",     "copper",      P_CLERIC_SPELL, 15,  8, 6, 1, IMMEDIATE, HI_COPPER),
SPELL("polymorph",       "silver",      P_MATTER_SPELL, 10,  8, 6, 1, IMMEDIATE, HI_SILVER),
SPELL("teleport away",   "gold",        P_ESCAPE_SPELL, 15,  6, 6, 1, IMMEDIATE, HI_GOLD),
SPELL("create familiar", "glittering",  P_CLERIC_SPELL, 10,  7, 6, 1, NODIR,     CLR_WHITE),
SPELL("cancellation",    "shining",     P_MATTER_SPELL, 15,  8, 7, 1, IMMEDIATE, CLR_WHITE),
SPELL("protection",	     "dull",        P_CLERIC_SPELL, 18,  3, 1, 1, NODIR,     HI_PAPER),
SPELL("jumping",	     "thin",        P_ESCAPE_SPELL, 20,  3, 1, 1, IMMEDIATE, HI_PAPER),
SPELL("stone to flesh",	 "thick",       P_HEALING_SPELL, 15,  1, 3, 1, IMMEDIATE, HI_PAPER),
SPELL("know enchantment", "sky blue",   P_DIVINATION_SPELL, 20,  2, 2, 1, NODIR, CLR_BRIGHT_BLUE),
#if 0	/* DEFERRED */
SPELL("flame sphere",    "canvas",      P_MATTER_SPELL, 20,  2, 1, 1, NODIR, CLR_BROWN),
SPELL("freeze sphere",   "hardcover",   P_MATTER_SPELL, 20,  2, 1, 1, NODIR, CLR_BROWN),
#endif
/* blank spellbook must come last because it retains its description */
SPELL("blank paper",     "plain",       P_NONE, 18,  0, 0, 0, 0,         HI_PAPER),
/* a special, one of a kind, spellbook */
OBJECT(OBJ("Book of the Dead", "papyrus"), BITS(0,0,1,0,1,0,1,1,0,0,0,P_NONE,PAPER), 0,
	SPBOOK_CLASS, 0, 0,20, 10000, 0, 0, 0, 7, 20, HI_PAPER),
#undef SPELL

/* wands ... */
#define WAND(name,typ,prob,cost,mgc,dir,metal,color) OBJECT( \
		OBJ(name,typ), BITS(0,0,1,0,mgc,1,0,0,0,0,dir,P_NONE,metal), 0, \
		WAND_CLASS, prob, 0, 7, cost, 0, 0, 0, 0, 30, color )
WAND("light",          "glass",    95, 100, 1, NODIR,     GLASS,    HI_GLASS),
WAND("secret door detection", "balsa",
				   50, 150, 1, NODIR,	  WOOD,     HI_WOOD),
WAND("enlightenment",  "crystal",  15, 150, 1, NODIR,     GLASS,    HI_GLASS),
WAND("create monster", "maple",    45, 200, 1, NODIR,     WOOD,     HI_WOOD),
WAND("wishing",        "pine",      1,1500, 1, NODIR,     WOOD,     HI_WOOD),
WAND("maintenance",    "bone",     14, 200, 1, NODIR,     BONE,     CLR_WHITE),
WAND("nothing",        "oak",      25, 100, 0, IMMEDIATE, WOOD,     HI_WOOD),
WAND("striking",       "ebony",    75, 150, 1, IMMEDIATE, WOOD,     HI_WOOD),
WAND("make invisible", "marble",   45, 150, 1, IMMEDIATE, MINERAL,  HI_MINERAL),
WAND("slow monster",   "tin",      45, 150, 1, IMMEDIATE, METAL,    HI_METAL),
WAND("speed monster",  "brass",    45, 150, 1, IMMEDIATE, COPPER,   HI_COPPER),
WAND("undead turning", "copper",   50, 150, 1, IMMEDIATE, COPPER,   HI_COPPER),
WAND("polymorph",      "silver",   45, 200, 1, IMMEDIATE, SILVER,   HI_SILVER),
WAND("cancellation",   "platinum", 45, 200, 1, IMMEDIATE, PLATINUM, CLR_WHITE),
WAND("teleportation",  "iridium",  45, 200, 1, IMMEDIATE, METAL,    CLR_BRIGHT_CYAN),
WAND("opening",        "zinc",     25, 150, 1, IMMEDIATE, METAL,    HI_METAL),
WAND("locking",        "aluminum", 25, 150, 1, IMMEDIATE, METAL,    HI_METAL),
WAND("probing",        "uranium",  30, 150, 1, IMMEDIATE, METAL,    HI_METAL),
WAND("digging",        "iron",     55, 150, 1, RAY,       IRON,     HI_METAL),
WAND("magic missile",  "steel",    50, 150, 1, RAY,       IRON,     HI_METAL),
WAND("fire",           "hexagonal",40, 175, 1, RAY,       IRON,     HI_METAL),
WAND("cold",           "short",    40, 175, 1, RAY,       IRON,     HI_METAL),
WAND("sleep",          "runed",    50, 175, 1, RAY,       IRON,     HI_METAL),
WAND("death",          "long",      5, 500, 1, RAY,       IRON,     HI_METAL),
WAND("lightning",      "curved",   40, 175, 1, RAY,       IRON,     HI_METAL),
WAND((char *)0,        "forked",    0, 150, 1, 0,         WOOD,     HI_WOOD),
WAND((char *)0,        "spiked",    0, 150, 1, 0,         IRON,     HI_METAL),
WAND((char *)0,        "jeweled",   0, 150, 1, 0,         IRON,     HI_MINERAL),
#undef WAND

/*
 WANDS (22)
 -----------
 *Aluminum, Beryllium, Bone, *Brass, Bronze, *Copper,
 Electrum, Gold, *Iron, Lead, Magnesium, Mercury,
 Nickel, Pewter, *Platinum, Steel, *Silver, Silicon,
 *Tin, Titanium, Tungsten, *Zinc

 STAVES (33)
 ------------
 Avocado Wood, *Balsa, Bamboo, Banyan, Birch, Cedar,
 Cherry, Cinnibar, Cypress, Dogwood, Driftwood,
 *Ebony, Elm, Eucalyptus, Fall, Hemlock, Holly, Ironwood,
 Kukui Wood, Mahogany, Manzanita, *Maple, *Oaken, Pecan,
 Persimmon, *Pine, Poplar, Redwood, Rosewood, Spruce,
 Teak, Walnut, Zebra Wood
 */

/* coins ... - so far, gold is all there is */
#define COIN(name,prob,metal,worth) OBJECT( \
		OBJ(name,(char *)0), BITS(0,1,0,0,0,0,0,0,0,0,0,P_NONE,metal), 0, \
		COIN_CLASS, prob, 0, 1, worth, 0, 0, 0, 0, 0, HI_GOLD )
	COIN("gold piece",      1000, GOLD,1),
#undef COIN

/* gems ... - includes stones and rocks but not boulders */
#define GEM(name,desc,prob,wt,gval,nutr,mohs,glass,color) OBJECT( \
	    OBJ(name,desc), \
	    BITS(0,1,0,0,0,0,0,0,0,HARDGEM(mohs),0,0,glass), 0, GEM_CLASS, prob, \
	    WP_CONSUMABLE|WP_AMMUNITION|WP_STONE, 1, gval, DAM(1,3), DAM(1,3), 0, 0, nutr, color )
#define ROCK(name,desc,kn,prob,wt,gval,sdam,ldam,mgc,nutr,mohs,glass,color) OBJECT( \
	    OBJ(name,desc), \
	    BITS(kn,1,0,0,mgc,0,0,0,0,HARDGEM(mohs),0,0,glass), 0, GEM_CLASS, prob, \
	    WP_CONSUMABLE|WP_AMMUNITION|WP_STONE, wt, gval, sdam, ldam, 0, 0, nutr, color )
GEM("dilithium crystal", "white",      2,  1, 4500, 15,  5, GEMSTONE, CLR_WHITE),
GEM("diamond", "white",                3,  1, 4000, 15, 10, GEMSTONE, CLR_WHITE),
GEM("ruby", "red",                     4,  1, 3500, 15,  9, GEMSTONE, CLR_RED),
GEM("jacinth", "orange",               3,  1, 3250, 15,  9, GEMSTONE, CLR_ORANGE),
GEM("sapphire", "blue",                4,  1, 3000, 15,  9, GEMSTONE, CLR_BLUE),
GEM("black opal", "black",             3,  1, 2500, 15,  8, GEMSTONE, CLR_BLACK),
GEM("emerald", "green",                5,  1, 2500, 15,  8, GEMSTONE, CLR_GREEN),
GEM("turquoise", "green",              6,  1, 2000, 15,  6, GEMSTONE, CLR_GREEN),
GEM("citrine", "yellow",               4,  1, 1500, 15,  6, GEMSTONE, CLR_YELLOW),
GEM("aquamarine", "green",             6,  1, 1500, 15,  8, GEMSTONE, CLR_GREEN),
GEM("amber", "yellowish brown",        8,  1, 1000, 15,  2, GEMSTONE, CLR_BROWN),
GEM("topaz", "yellowish brown",       10,  1,  900, 15,  8, GEMSTONE, CLR_BROWN),
GEM("jet", "black",                    6,  1,  850, 15,  7, GEMSTONE, CLR_BLACK),
GEM("opal", "white",                  12,  1,  800, 15,  6, GEMSTONE, CLR_WHITE),
GEM("chrysoberyl", "yellow",           8,  1,  700, 15,  5, GEMSTONE, CLR_YELLOW),
GEM("garnet", "red",                  12,  1,  700, 15,  7, GEMSTONE, CLR_RED),
GEM("amethyst", "violet",             14,  1,  600, 15,  7, GEMSTONE, CLR_MAGENTA),
GEM("jasper", "red",                  15,  1,  500, 15,  7, GEMSTONE, CLR_RED),
GEM("fluorite", "violet",             15,  1,  400, 15,  4, GEMSTONE, CLR_MAGENTA),
GEM("obsidian", "black",               9,  1,  200, 15,  6, GEMSTONE, CLR_BLACK),
GEM("agate", "orange",                12,  1,  200, 15,  6, GEMSTONE, CLR_ORANGE),
GEM("jade", "green",                  10,  1,  300, 15,  6, GEMSTONE, CLR_GREEN),
GEM("worthless piece of white glass", "white",   77, 1, 0, 6, 5, GLASS, CLR_WHITE),
GEM("worthless piece of blue glass", "blue",     77, 1, 0, 6, 5, GLASS, CLR_BLUE),
GEM("worthless piece of red glass", "red",       77, 1, 0, 6, 5, GLASS, CLR_RED),
GEM("worthless piece of yellowish brown glass", "yellowish brown",
						 77, 1, 0, 6, 5, GLASS, CLR_BROWN),
GEM("worthless piece of orange glass", "orange", 76, 1, 0, 6, 5, GLASS, CLR_ORANGE),
GEM("worthless piece of yellow glass", "yellow", 77, 1, 0, 6, 5, GLASS, CLR_YELLOW),
GEM("worthless piece of black glass",  "black",  76, 1, 0, 6, 5, GLASS, CLR_BLACK),
GEM("worthless piece of green glass", "green",   77, 1, 0, 6, 5, GLASS, CLR_GREEN),
GEM("worthless piece of violet glass", "violet", 77, 1, 0, 6, 5, GLASS, CLR_MAGENTA),

/* Placement note: there is a wishable subrange for
 * "gray stones" in the o_ranges[] array in objnam.c
 * that is currently everything between luckstones and flint (inclusive).
 */
ROCK("luckstone", "gray",	0, 10,  10, 60, DAM(1,3), DAM(1,3), 1, 10, 7, MINERAL, CLR_GRAY),
ROCK("loadstone", "gray",	0, 10, 500,  1, DAM(1,3), DAM(1,3), 1, 10, 6, MINERAL, CLR_GRAY),
ROCK("touchstone", "gray",	0,  8,  10, 45, DAM(1,3), DAM(1,3), 1, 10, 6, MINERAL, CLR_GRAY),
ROCK("flint", "gray",		0, 10,  10,  1, DAM(1,6), DAM(1,6), 0, 10, 7, MINERAL, CLR_GRAY),
ROCK("rock", (char *)0,		1,100,  10,  0, DAM(1,3), DAM(1,3), 0, 10, 7, MINERAL, CLR_GRAY),
#undef GEM
#undef ROCK

/* miscellaneous ... */
/* Note: boulders and rocks are not normally created at random; the
 * probabilities only come into effect when you try to polymorph them.
 * Boulders weigh more than MAX_CARR_CAP; statues use corpsenm to take
 * on a specific type and may act as containers (both affect weight).
 */
OBJECT(OBJ("boulder",(char *)0), BITS(1,0,0,0,0,0,0,0,1,0,0,P_NONE,MINERAL), 0,
		ROCK_CLASS,   100, 0, 6000,  0, DAM(1,20), DAM(1,20), 0, DBON(10,10), 2000, HI_MINERAL),
OBJECT(OBJ("statue", (char *)0), BITS(1,0,0,1,0,0,0,0,0,1,0,P_NONE,MINERAL), 0,
		ROCK_CLASS,   900, 0, 2500,  0, DAM(1,20), DAM(1,20), 0, 0, 2500, HI_MINERAL/*CLR_WHITE*/),

OBJECT(OBJ("heavy iron ball", (char *)0), BITS(1,0,0,0,0,0,0,0,0,0,WHACK,P_THROWING_GROUP,IRON), 0,
		BALL_CLASS,  1000, 0,  480, 10, DAM(1,25), DAM(1,25), 0, 0,  200, HI_METAL),
						/* +d4 when "very heavy" */
OBJECT(OBJ("iron chain", (char *)0), BITS(1,0,0,0,0,0,0,0,0,0,WHACK,P_WHIP_GROUP,IRON), 0,
		CHAIN_CLASS, 1000, 0,  120,  0, DAM(1,4), DAM(1,4), 0, DBON(1,1),  200, HI_METAL),
						/* +1 both l & s */
OBJECT(OBJ("sheaf of straw", (char *)0), BITS(1,1,0,0,0,0,0,0,0,0,0,P_NONE,VEGGY), 0,
		CHAIN_CLASS, 0, 2, 20, 0, 0, 0, 0, 0, 100, CLR_YELLOW),

OBJECT(OBJ("blinding venom", "splash of venom"),
		BITS(0,1,0,0,0,0,0,1,0,0,0,P_NONE,LIQUID), 0,
		VENOM_CLASS,  500, 0,	 1,  0,  0,  0, 0, 0,	 0, HI_ORGANIC),
OBJECT(OBJ("acid venom", "splash of venom"),
		BITS(0,1,0,0,0,0,0,1,0,0,0,P_NONE,LIQUID), 0,
		VENOM_CLASS,  500, 0,	 1,  0, DAM(2,6), DAM(2,6), 0, 0, 0, HI_ORGANIC),
		/* +d6 small or large */

/* fencepost, the deadly Array Terminator -- name [1st arg] *must* be NULL */
	OBJECT(OBJ((char *)0,(char *)0), BITS(0,0,0,0,0,0,0,0,0,0,0,P_NONE,0), 0,
		ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
}; /* objects[] */

#undef DAM
#undef DBON

#ifndef OBJECTS_PASS_2_

/* perform recursive compilation for second structure */
#  undef OBJ
#  undef OBJECT
#  define OBJECTS_PASS_2_
#include "objects.c"

void NDECL(objects_init);

/* dummy routine used to force linkage */
void
objects_init()
{
    return;
}

#endif	/* !OBJECTS_PASS_2_ */

/*objects.c*/
