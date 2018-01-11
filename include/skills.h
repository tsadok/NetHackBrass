/*	SCCS Id: @(#)skills.h	3.4	1999/10/27	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SKILLS_H
#define SKILLS_H

/* Much of this code was taken from you.h.  It is now
 * in a separate file so it can be included in objects.c.
 */


/* Code to denote that no skill is applicable */
#define P_NONE				0

/* Weapon Skills -- Stephen White
 * Order matters and are used in macros.
 * Positive values denote hand-to-hand weapons or launchers.
 * Negative values denote ammunition or missiles.
 * Update weapon.c if you ammend any skills.
 * Also used for oc_subtyp.
 */
#define P_DAGGER_GROUP       1  /* blades */
#define P_KNIFE_GROUP        2
#define P_AXE_GROUP          3
#define P_SHORT_BLADE_GROUP  4
//#define P_BROAD_BLADE_GROUP  5
#define P_LONG_BLADE_GROUP   6
#define P_SABER_GROUP        7
#define P_KATANA_GROUP       7	/* when you are a Samurai */
//#define P_???_GROUP       8
//#define P_???_GROUP       9
//#define P_???_GROUP      10
#define P_CRUSHING_GROUP    11
#define P_FLAIL_GROUP       12
//#define P_???_GROUP      13
//#define P_???_GROUP      14
//#define P_???_GROUP      15
#define P_STAFF_GROUP       16	/* Long-shafted bludgeon */
#define P_POLEARM_GROUP     17
#define P_SPEAR_GROUP       18
//#define P_???_GROUP      19
#define P_PICKAXE_GROUP     20
#define P_WHIP_GROUP        21
#define P_BOW_GROUP         22
#define P_FIREARM_GROUP     23
//#define P_BULLET_GROUP      24
#define P_THROWING_GROUP    25
#define P_FIRST_WEAPON      1
#define P_LAST_WEAPON       25

/* Spell Skills added by Larry Stewart-Zerba */
#define P_ATTACK_SPELL      30
#define P_HEALING_SPELL     31
#define P_DIVINATION_SPELL  32
#define P_ENCHANTMENT_SPELL 33
#define P_CLERIC_SPELL      34
#define P_ESCAPE_SPELL      35
#define P_MATTER_SPELL      36
#define P_FIRST_SPELL		P_ATTACK_SPELL
#define P_LAST_SPELL		P_MATTER_SPELL

/* Other types of combat */
#define P_BARE_HANDED_COMBAT	37
#define P_MARTIAL_ARTS		P_BARE_HANDED_COMBAT	/* Role distinguishes */
#define P_TWO_WEAPON_COMBAT	38	/* Finally implemented */
#ifdef STEED
#define P_RIDING		39	/* How well you control your steed */
#define P_LAST_H_TO_H		P_RIDING
#else
#define P_LAST_H_TO_H		P_TWO_WEAPON_COMBAT
#endif
#define P_FIRST_H_TO_H		P_BARE_HANDED_COMBAT

#define P_NUM_SKILLS		(P_LAST_H_TO_H+1)


/* These roles qualify for a martial arts bonus */
#define martial_bonus()	(Role_if(PM_SAMURAI) || Role_if(PM_MONK))


/*
 * These are the standard weapon skill levels.  It is important that
 * the lowest "valid" skill be be 1.  The code calculates the
 * previous amount to practice by calling  practice_needed_to_advance()
 * with the current skill-1.  To work out for the UNSKILLED case,
 * a value of 0 needed.
 */
#define P_ISRESTRICTED	0
#define P_UNSKILLED		0
#define P_MINIMUM		25
#define P_BASIC			50
#define P_SKILLED		75
#define P_EXPERT		100
#define P_MASTER		110	/* Unarmed combat/martial arts only */
#define P_GRAND_MASTER		120	/* Unarmed combat/martial arts only */

#define practice_needed_to_advance(level) ((level)*(level)*20)

/* The hero's skill in various weapons. */
struct skills {
	xchar skill;
	xchar max_skill;
	uchar next_skill;	/* for ordering skills ... under construction */
//	unsigned short advance;
};

#define P_SKILL(type)		(u.weapon_skills[type].skill)
#define P_MAX_SKILL(type)	(u.weapon_skills[type].max_skill)
//#define P_ADVANCE(type)		(u.weapon_skills[type].advance)
#define P_RESTRICTED(type)	(u.weapon_skills[type].max_skill == P_ISRESTRICTED)

/* Initial skill matrix structure; used in u_init.c and weapon.c */
struct def_skill {
	xchar skill;
	xchar skmax;
};

#endif  /* SKILLS_H */
