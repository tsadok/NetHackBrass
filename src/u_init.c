/*	SCCS Id: @(#)u_init.c	3.4	2002/10/22	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

struct trobj {
	short trotyp;
	schar trspe;
	char trclass;
	Bitfield(trquan,6);
	Bitfield(trbless,2);
};

STATIC_DCL void FDECL(ini_inv, (struct trobj *));
STATIC_DCL void FDECL(knows_object,(int));
STATIC_DCL void FDECL(knows_class,(CHAR_P));
STATIC_DCL boolean FDECL(restricted_spell_discipline, (int));
STATIC_DCL int NDECL(shoot_for_the_stars);

#define UNDEF_TYP	0
#define UNDEF_SPE	'\177'
#define UNDEF_BLESS	2

/*
 *	Initial inventory for the various roles.
 */

static struct trobj Archeologist[] = {
	/* if adventure has a name...  idea from tan@uvm-gen */
	/* type,	      spe, class,	     quan, bless */
	{ BULLWHIP,		2, WEAPON_CLASS,	1, 0 },
	{ LEATHER_JACKET,	0, ARMOR_CLASS,		1, 0 },
	{ FEDORA,		0, ARMOR_CLASS,		1, 0 },
	{ FOOD_RATION,		0, FOOD_CLASS,		3, 0 },
	{ PICK_AXE,		0, TOOL_CLASS,		1, 0 },
	{ TINNING_KIT,	       50, TOOL_CLASS,		1, 0 },
	{ TOUCHSTONE,		0, GEM_CLASS,		1, 0 },
	{ SACK,			0, TOOL_CLASS,		1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Barbarian[] = {
#define B_MAJOR	0	/* two-handed sword or battle-axe  */
#define B_MINOR	1	/* matched with axe or short sword */
	{ TWO_HANDED_SWORD,	0, WEAPON_CLASS,	1, 0 },
	{ AXE,			0, WEAPON_CLASS,	1, 0 },
	{ RING_MAIL,		0, ARMOR_CLASS,		1, 0 },
	{ FOOD_RATION,		0, FOOD_CLASS,		1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Cave_man[] = {
//#define C_AMMO	2
	{ CLUB,			1, WEAPON_CLASS,	1, 0 },
	{ SLING,		2, WEAPON_CLASS,	1, 0 },
	{ FLINT,		0, GEM_CLASS,	       15, 0 },	/* quan is *NOT* variable */
	{ ROCK,			0, GEM_CLASS,	       15, 0 },	/* yields 18..33 */
	{ LEATHER_ARMOR,	0, ARMOR_CLASS,		1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Healer[] = {
#define H_ARMOR1 1
#define H_ARMOR2 2
	{ SCALPEL,		0, WEAPON_CLASS,	1, 0 },
	{ LEATHER_GLOVES,	1, ARMOR_CLASS,		1, 0 },
	{ ALCHEMY_SMOCK,	0, ARMOR_CLASS,		1, 0 },
	{ STETHOSCOPE,		0, TOOL_CLASS,		1, 0 },
#ifdef FIRSTAID
	{ SCISSORS,		0, TOOL_CLASS,		1, 0 },
	{ BANDAGE,		0, TOOL_CLASS,		5, 0 },
#endif
	{ POT_HEALING,		0, POTION_CLASS,	4, 0 },
	{ POT_EXTRA_HEALING,	0, POTION_CLASS,	4, 0 },
	{ WAN_SLEEP,		8, WAND_CLASS,		1, 0 },
	/* always blessed, so it's guaranteed readable */
	{ SPE_HEALING,		0, SPBOOK_CLASS,	1, 1 },
	{ SPE_EXTRA_HEALING,	0, SPBOOK_CLASS,	1, 1 },
	{ SPE_STONE_TO_FLESH,	0, SPBOOK_CLASS,	1, 1 },
	{ APPLE,		0, FOOD_CLASS,		8, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Knight[] = {
	{ LONG_SWORD,		0, WEAPON_CLASS,	1, 0 },
	{ LANCE,		0, WEAPON_CLASS,	1, 0 },
	{ RING_MAIL,		0, ARMOR_CLASS,		1, 0 },
	{ HELMET,		0, ARMOR_CLASS,		1, 0 },
	{ SMALL_SHIELD,		0, ARMOR_CLASS,		1, 0 },
	{ LEATHER_GLOVES,	0, ARMOR_CLASS,		1, 0 },
	{ APPLE,		0, FOOD_CLASS,	       10, 0 },
	{ CARROT,		0, FOOD_CLASS,	       10, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Monk[] = {
#define M_BOOK		2
	{ LEATHER_GLOVES,	2, ARMOR_CLASS,		1, 0 },
	{ ROBE,			1, ARMOR_CLASS,		1, 0 },
	{ UNDEF_TYP,    UNDEF_SPE, SPBOOK_CLASS,	1, 1 },
	{ UNDEF_TYP,    UNDEF_SPE, SCROLL_CLASS,	1, 0 },
	{ POT_HEALING,		0, POTION_CLASS,	3, 0 },
	{ FOOD_RATION,		0, FOOD_CLASS,		3, 0 },
	{ APPLE,		0, FOOD_CLASS,		5, 0 },
	{ ORANGE,		0, FOOD_CLASS,		5, 0 },
	/* Yes, we know fortune cookies aren't really from China.  They were
	 * invented by George Jung in Los Angeles, California, USA in 1916.
	 */
	{ FORTUNE_COOKIE,	0, FOOD_CLASS,		3, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Priest[] = {
	{ MACE,			1, WEAPON_CLASS,	1, 1 },
	{ ROBE,			0, ARMOR_CLASS,		1, 0 },
	{ SMALL_SHIELD,		0, ARMOR_CLASS,		1, 0 },
	{ POT_WATER,		0, POTION_CLASS,	4, 1 },	/* holy water */
	{ CLOVE_OF_GARLIC,	0, FOOD_CLASS,		4, 0 },
	{ SPRIG_OF_WOLFSBANE,	0, FOOD_CLASS,		4, 0 },
	{ UNDEF_TYP,    UNDEF_SPE, SPBOOK_CLASS,	2, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Ranger[] = {
//#define RAN_BOW			1
//#define RAN_TWO_ARROWS	2
//#define RAN_ZERO_ARROWS	3
	{ DAGGER,		1, WEAPON_CLASS,	1, 0 },
	{ BOW,			1, WEAPON_CLASS,	1, 0 },
	{ ARROW,		2, WEAPON_CLASS,       50, 0 },
	{ ARROW,		0, WEAPON_CLASS,       30, 0 },
	{ LEATHER_CLOAK,	0, ARMOR_CLASS,		1, 0 },
	{ CRAM_RATION,		0, FOOD_CLASS,		4, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Rogue[] = {
//#define R_DAGGERS	1
	{ SHORT_SWORD,		0, WEAPON_CLASS,	1, 0 },
	{ DAGGER,		0, WEAPON_CLASS,	7, 0 },	/* quan is variable */
	{ LEATHER_ARMOR,	0, ARMOR_CLASS,		1, 0 },
	{ POT_SICKNESS,		0, POTION_CLASS,	1, 0 },
	{ LOCK_PICK,		0, TOOL_CLASS,		1, 0 },
	{ SACK,			0, TOOL_CLASS,		1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Samurai[] = {
//#define S_ARROWS	3
	{ KATANA,		0, WEAPON_CLASS,	1, 0 },
	{ SHORT_SWORD,		0, WEAPON_CLASS,	1, 0 }, /* wakizashi */
	{ YUMI,			0, WEAPON_CLASS,	1, 0 },
	{ YA,			0, WEAPON_CLASS,       25, 0 }, /* variable quan */
	{ SPLINT_MAIL,		0, ARMOR_CLASS,		1, 0 },
	{ 0, 0, 0, 0, 0 }
};
#ifdef TOURIST
static struct trobj Tourist[] = {
//#define T_DARTS		0
	{ DART,			2, WEAPON_CLASS,       30, 0 },	/* quan is *NOT* variable */
	{ UNDEF_TYP,    UNDEF_SPE, FOOD_CLASS,	       10, 0 },
	{ POT_EXTRA_HEALING,	0, POTION_CLASS,	2, 0 },
	{ SCR_MAGIC_MAPPING,	0, SCROLL_CLASS,	4, 0 },
	{ HAWAIIAN_SHIRT,	0, ARMOR_CLASS,		1, 0 },
	{ EXPENSIVE_CAMERA,    99, TOOL_CLASS,		1, 0 },
	{ CREDIT_CARD,		0, TOOL_CLASS,		1, 0 },
	{ 0, 0, 0, 0, 0 }
};
#endif
static struct trobj Valkyrie[] = {
	{ LONG_SWORD,		0, WEAPON_CLASS,	1, 0 },
	{ DAGGER,		0, WEAPON_CLASS,	1, 0 },
	{ SMALL_SHIELD,		3, ARMOR_CLASS,		1, 0 },
	{ FOOD_RATION,		0, FOOD_CLASS,		1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Wizard[] = {
#define W_MULTSTART	2
#define W_MULTEND	6
	{ QUARTERSTAFF,		1, WEAPON_CLASS,	1, 1 },
	{ CLOAK_OF_MAGIC_RESISTANCE, 0, ARMOR_CLASS,	1, 0 },
	{ UNDEF_TYP,    UNDEF_SPE, WAND_CLASS,		1, 0 },
	{ UNDEF_TYP,    UNDEF_SPE, RING_CLASS,		2, 0 },
	{ UNDEF_TYP,    UNDEF_SPE, POTION_CLASS,	3, 0 },
	{ UNDEF_TYP,    UNDEF_SPE, SCROLL_CLASS,	3, 0 },
	{ SPE_FORCE_BOLT,	0, SPBOOK_CLASS,	1, 1 },
	{ UNDEF_TYP,    UNDEF_SPE, SPBOOK_CLASS,	1, 1 },
	{ 0, 0, 0, 0, 0 }
};

/*
 *	Optional extra inventory items.
 */

static struct trobj Tinopener[] = {
	{ TIN_OPENER, 0, TOOL_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Magicmarker[] = {
	{ MAGIC_MARKER, UNDEF_SPE, TOOL_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Lamp[] = {
	{ OIL_LAMP, 1, TOOL_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Blindfold[] = {
	{ BLINDFOLD, 0, TOOL_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Instrument[] = {
	{ WOODEN_FLUTE, 0, TOOL_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Xtra_food[] = {
	{ UNDEF_TYP, UNDEF_SPE, FOOD_CLASS, 2, 0 },
	{ 0, 0, 0, 0, 0 }
};
#ifdef TOURIST
static struct trobj Leash[] = {
	{ LEASH, 0, TOOL_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
static struct trobj Towel[] = {
	{ TOWEL, 0, TOOL_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
#endif	/* TOURIST */
static struct trobj Wishing[] = {
	{ WAN_WISHING, 3, WAND_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
#ifdef GOLDOBJ
static struct trobj Money[] = {
	{ GOLD_PIECE, 0 , COIN_CLASS, 1, 0 },
	{ 0, 0, 0, 0, 0 }
};
#endif

/* race-based substitutions for initial inventory;
   the weaker cloak for elven rangers is intentional--they shoot better */
static struct inv_sub { short race_pm, item_otyp, subs_otyp; } inv_subs[] = {
    { PM_ELF,	DAGGER,			ELVEN_DAGGER	      },
    { PM_ELF,	SPEAR,			ELVEN_SPEAR	      },
    { PM_ELF,	SHORT_SWORD,		ELVEN_SHORT_SWORD     },
    { PM_ELF,	BOW,			ELVEN_BOW	      },
    { PM_ELF,	ARROW,			ELVEN_ARROW	      },
    { PM_ELF,	HELMET,			ELVEN_LEATHER_HELM    },
 /* { PM_ELF,	SMALL_SHIELD,		ELVEN_SHIELD	      }, */
    { PM_ELF,	LEATHER_CLOAK,		ELVEN_CLOAK	      },
    { PM_ELF,	CLOAK_OF_DISPLACEMENT,	ELVEN_CLOAK	      },
    { PM_ELF,	CRAM_RATION,		LEMBAS_WAFER	      },
    { PM_ORC,	DAGGER,			ORCISH_DAGGER	      },
    { PM_ORC,	SPEAR,			ORCISH_SPEAR	      },
    { PM_ORC,	SHORT_SWORD,		ORCISH_SHORT_SWORD    },
    { PM_ORC,	BOW,			ORCISH_BOW	      },
    { PM_ORC,	ARROW,			ORCISH_ARROW	      },
    { PM_ORC,	HELMET,			ORCISH_HELM	      },
    { PM_ORC,	SMALL_SHIELD,		ORCISH_SHIELD	      },
    { PM_ORC,	RING_MAIL,		ORCISH_RING_MAIL      },
    { PM_ORC,	CHAIN_MAIL,		ORCISH_CHAIN_MAIL     },
    { PM_ORC,	LEATHER_CLOAK,		ORCISH_CLOAK	      },
    { PM_DWARF, SPEAR,			DWARVISH_SPEAR	      },
    { PM_DWARF, SHORT_SWORD,		DWARVISH_SHORT_SWORD  },
    { PM_DWARF, HELMET,			DWARVISH_IRON_HELM    },
    { PM_DWARF,	LEATHER_CLOAK,		DWARVISH_CLOAK	      },
 /* { PM_DWARF, SMALL_SHIELD,		DWARVISH_ROUNDSHIELD  }, */
 /* { PM_DWARF, PICK_AXE,		DWARVISH_MATTOCK      }, */
    { PM_GNOME, BOW,			CROSSBOW	      },
    { PM_GNOME, ARROW,			CROSSBOW_BOLT	      },
    { PM_GNOME,	LEATHER_CLOAK,		DWARVISH_CLOAK	      },
    { NON_PM,	STRANGE_OBJECT,		STRANGE_OBJECT	      }
};

static const struct def_skill Skill_A[] = {
    { P_DAGGER_GROUP, P_BASIC },	{ P_KNIFE_GROUP,  P_BASIC },
    { P_PICKAXE_GROUP, P_EXPERT },	{ P_SHORT_BLADE_GROUP, P_BASIC },
 /* { P_SCIMITAR, P_SKILLED },*/	{ P_SABER_GROUP, P_EXPERT },
    { P_CRUSHING_GROUP, P_SKILLED },	{ P_STAFF_GROUP, P_SKILLED },
    { P_THROWING_GROUP, P_SKILLED },	/*{ P_DART, P_BASIC },*/
 /* { P_BOOMERANG, P_EXPERT },*/	{ P_WHIP_GROUP, P_EXPERT },
    { P_SPEAR_GROUP, P_SKILLED },	{ P_FIREARM_GROUP, P_EXPERT },
    { P_ATTACK_SPELL, P_BASIC },	{ P_HEALING_SPELL, P_BASIC },
    { P_DIVINATION_SPELL, P_EXPERT},	{ P_MATTER_SPELL, P_BASIC},
#ifdef STEED
    { P_RIDING, P_BASIC },
#endif
    { P_TWO_WEAPON_COMBAT, P_BASIC },
    { P_BARE_HANDED_COMBAT, P_EXPERT },
    { P_NONE, 0 }
};

static const struct def_skill Skill_B[] = {
    { P_DAGGER_GROUP, P_SKILLED },	{ P_AXE_GROUP, P_EXPERT },
    { P_PICKAXE_GROUP, P_SKILLED },	{ P_SHORT_BLADE_GROUP, P_EXPERT },
 /* { P_BROAD_BLADE_GROUP, P_EXPERT },*/{ P_LONG_BLADE_GROUP, P_EXPERT },
 /* { P_TWO_HANDED_SWORD, P_EXPERT },	{ P_SCIMITAR, P_SKILLED },*/
    { P_SABER_GROUP, P_SKILLED },	{ P_CRUSHING_GROUP, P_EXPERT },
 /* { P_MACE_GROUP, P_SKILLED },*/	{ P_FLAIL_GROUP, P_EXPERT },
    { P_STAFF_GROUP, P_BASIC },		/*{ P_HAMMER, P_EXPERT },*/
    { P_POLEARM_GROUP, P_EXPERT },	{ P_SPEAR_GROUP, P_EXPERT },
 /* { P_TRIDENT, P_SKILLED },	*/	{ P_BOW_GROUP, P_SKILLED },
 /* { P_ATTACK_SPELL, P_SKILLED },*/
#ifdef STEED
    { P_RIDING, P_BASIC },
#endif
    { P_TWO_WEAPON_COMBAT, P_BASIC },
    { P_BARE_HANDED_COMBAT, P_MASTER },
    { P_NONE, 0 }
};

static const struct def_skill Skill_C[] = {
    { P_DAGGER_GROUP, P_BASIC },	{ P_KNIFE_GROUP,  P_SKILLED },
    { P_AXE_GROUP, P_SKILLED },		{ P_PICKAXE_GROUP, P_BASIC },
    { P_CRUSHING_GROUP, P_EXPERT },	/*{ P_MACE_GROUP, P_EXPERT },*/
    { P_FLAIL_GROUP, P_BASIC },		{ P_STAFF_GROUP, P_EXPERT },
 /* { P_HAMMER, P_SKILLED },*/		/*{ P_QUARTERSTAFF, P_EXPERT },*/
    { P_POLEARM_GROUP, P_SKILLED },	{ P_SPEAR_GROUP, P_EXPERT },
 /* { P_JAVELIN, P_SKILLED },*/		/*{ P_TRIDENT, P_SKILLED },*/
    { P_BOW_GROUP, P_SKILLED },		{ P_THROWING_GROUP, P_EXPERT },
    { P_ATTACK_SPELL, P_BASIC },	{ P_MATTER_SPELL, P_SKILLED },
 /* { P_BOOMERANG, P_EXPERT },		{ P_UNICORN_HORN, P_BASIC },*/
    { P_BARE_HANDED_COMBAT, P_MASTER },
    { P_NONE, 0 }
};

static const struct def_skill Skill_H[] = {
    { P_DAGGER_GROUP, P_SKILLED },	{ P_KNIFE_GROUP, P_EXPERT },
    { P_SHORT_BLADE_GROUP, P_SKILLED },	/*{ P_SCIMITAR, P_BASIC },*/
    { P_SABER_GROUP, P_BASIC },		{ P_CRUSHING_GROUP, P_SKILLED },
 /* { P_MACE_GROUP, P_BASIC },*/	{ P_STAFF_GROUP, P_EXPERT },
    { P_POLEARM_GROUP, P_BASIC },	{ P_SPEAR_GROUP, P_BASIC },
 /* { P_JAVELIN, P_BASIC },*/		/*{ P_TRIDENT, P_BASIC },*/
    { P_THROWING_GROUP, P_SKILLED },	{ P_FIREARM_GROUP, P_BASIC },
 /* { P_SHURIKEN, P_SKILLED },*/	/*{ P_UNICORN_HORN, P_EXPERT },*/
    { P_HEALING_SPELL, P_EXPERT },
    { P_BARE_HANDED_COMBAT, P_BASIC },
    { P_NONE, 0 }
};

static const struct def_skill Skill_K[] = {
    { P_DAGGER_GROUP, P_BASIC },	{ P_KNIFE_GROUP, P_BASIC },
    { P_AXE_GROUP, P_SKILLED },		{ P_PICKAXE_GROUP, P_BASIC },
    { P_SHORT_BLADE_GROUP, P_SKILLED },	/*{ P_BROAD_BLADE_GROUP, P_EXPERT },*/
    { P_LONG_BLADE_GROUP, P_EXPERT },	/*{ P_TWO_HANDED_SWORD, P_SKILLED },*/
    /*{ P_SCIMITAR, P_BASIC },*/	{ P_SABER_GROUP, P_SKILLED },
    { P_CRUSHING_GROUP, P_SKILLED },	/*{ P_MACE_GROUP, P_SKILLED },*/
    { P_FLAIL_GROUP, P_SKILLED },	{ P_STAFF_GROUP, P_BASIC },
    /*{ P_HAMMER, P_BASIC },*/		{ P_POLEARM_GROUP, P_EXPERT },
    { P_SPEAR_GROUP, P_EXPERT },	/*{ P_JAVELIN, P_SKILLED },*/
    /*{ P_TRIDENT, P_BASIC },*/		/*{ P_LANCE, P_EXPERT },*/
    { P_BOW_GROUP, P_BASIC },		/*{ P_CROSSBOW, P_SKILLED },*/
    { P_ATTACK_SPELL, P_SKILLED },	{ P_HEALING_SPELL, P_SKILLED },
    { P_CLERIC_SPELL, P_SKILLED },
#ifdef STEED
    { P_RIDING, P_EXPERT },
#endif
    { P_TWO_WEAPON_COMBAT, P_SKILLED },
    { P_BARE_HANDED_COMBAT, P_EXPERT },
    { P_NONE, 0 }
};

static const struct def_skill Skill_Mon[] = {
    { P_STAFF_GROUP, P_BASIC },		{ P_SPEAR_GROUP, P_BASIC },
    /*{ P_JAVELIN, P_BASIC },*/		{ P_BOW_GROUP, P_BASIC },
    { P_THROWING_GROUP, P_BASIC },
    { P_ATTACK_SPELL, P_BASIC },    { P_HEALING_SPELL, P_EXPERT },
    { P_DIVINATION_SPELL, P_BASIC },{ P_ENCHANTMENT_SPELL, P_BASIC },
    { P_CLERIC_SPELL, P_SKILLED },  { P_ESCAPE_SPELL, P_BASIC },
    { P_MATTER_SPELL, P_BASIC },
    { P_MARTIAL_ARTS, P_GRAND_MASTER },
    { P_NONE, 0 }
};

static const struct def_skill Skill_P[] = {
    { P_CRUSHING_GROUP, P_EXPERT },	{ P_STAFF_GROUP, P_EXPERT },
    { P_FLAIL_GROUP, P_EXPERT },	
    /*{ P_HAMMER, P_EXPERT },*/		/*{ P_MACE_GROUP, P_EXPERT },*/
    { P_POLEARM_GROUP, P_SKILLED },	{ P_SPEAR_GROUP, P_SKILLED },
    /*{ P_JAVELIN, P_SKILLED },*/	/*{ P_TRIDENT, P_SKILLED },*/
    /*{ P_LANCE, P_BASIC },*/		{ P_BOW_GROUP, P_BASIC },
    { P_THROWING_GROUP, P_BASIC },	/*{ P_CROSSBOW, P_BASIC },*/
    /*{ P_DART, P_BASIC },*/		/*{ P_SHURIKEN, P_BASIC },*/
    /*{ P_BOOMERANG, P_BASIC },*/	/*{ P_UNICORN_HORN, P_SKILLED },*/
    { P_HEALING_SPELL, P_EXPERT },	{ P_DIVINATION_SPELL, P_EXPERT },
    { P_CLERIC_SPELL, P_EXPERT },
    { P_BARE_HANDED_COMBAT, P_BASIC },
    { P_NONE, 0 }
};

static const struct def_skill Skill_R[] = {
    { P_DAGGER_GROUP, P_EXPERT },	{ P_KNIFE_GROUP,  P_EXPERT },
    { P_SHORT_BLADE_GROUP, P_EXPERT },	/*{ P_BROAD_BLADE_GROUP, P_SKILLED },*/
    { P_LONG_BLADE_GROUP, P_SKILLED },	/*{ P_TWO_HANDED_SWORD, P_BASIC },*/
    /*{ P_SCIMITAR, P_SKILLED },*/	{ P_SABER_GROUP, P_SKILLED },
    { P_CRUSHING_GROUP, P_SKILLED },	/*{ P_MACE_GROUP, P_SKILLED },*/
    { P_FLAIL_GROUP, P_BASIC },		{ P_STAFF_GROUP, P_BASIC },
    /*{ P_HAMMER, P_BASIC },*/		{ P_POLEARM_GROUP, P_SKILLED },
    { P_SPEAR_GROUP, P_BASIC },		{ P_BOW_GROUP, P_EXPERT },
    { P_THROWING_GROUP, P_EXPERT },	{ P_FIREARM_GROUP, P_SKILLED },
    { P_DIVINATION_SPELL, P_SKILLED },	{ P_ESCAPE_SPELL, P_SKILLED },
    { P_MATTER_SPELL, P_SKILLED },
#ifdef STEED
    { P_RIDING, P_BASIC },
#endif
    { P_TWO_WEAPON_COMBAT, P_EXPERT },
    { P_BARE_HANDED_COMBAT, P_EXPERT },
    { P_NONE, 0 }
};

static const struct def_skill Skill_Ran[] = {
    { P_DAGGER_GROUP, P_EXPERT },	{ P_KNIFE_GROUP,  P_SKILLED },
    { P_AXE_GROUP, P_SKILLED },		{ P_PICKAXE_GROUP, P_BASIC },
    { P_SHORT_BLADE_GROUP, P_BASIC },	{ P_CRUSHING_GROUP, P_BASIC },
    { P_FLAIL_GROUP, P_SKILLED },	/*{ P_HAMMER, P_BASIC },*/
    { P_STAFF_GROUP, P_BASIC },		{ P_POLEARM_GROUP, P_SKILLED },
    { P_SPEAR_GROUP, P_SKILLED },	/*{ P_JAVELIN, P_EXPERT },*/
    /*{ P_TRIDENT, P_BASIC },*/		{ P_BOW_GROUP, P_EXPERT },
    { P_THROWING_GROUP, P_EXPERT },	{ P_FIREARM_GROUP, P_SKILLED },
    /*{ P_DART, P_EXPERT },*/		/*{ P_SHURIKEN, P_SKILLED },*/
    /*{ P_BOOMERANG, P_EXPERT },*/	{ P_WHIP_GROUP, P_BASIC },
    { P_HEALING_SPELL, P_BASIC },
    { P_DIVINATION_SPELL, P_EXPERT },
    { P_ESCAPE_SPELL, P_BASIC },
#ifdef STEED
    { P_RIDING, P_BASIC },
#endif
    { P_BARE_HANDED_COMBAT, P_BASIC },
    { P_NONE, 0 }
};

static const struct def_skill Skill_S[] = {
    { P_DAGGER_GROUP, P_BASIC },	{ P_KNIFE_GROUP,  P_SKILLED },
    { P_SHORT_BLADE_GROUP, P_EXPERT },	/*{ P_BROAD_BLADE_GROUP, P_SKILLED },*/
    { P_LONG_BLADE_GROUP, P_EXPERT },	/*{ P_TWO_HANDED_SWORD, P_EXPERT },*/
    /*{ P_SCIMITAR, P_BASIC },*/	{ P_KATANA_GROUP, P_EXPERT },
    { P_STAFF_GROUP, P_BASIC },		{ P_FLAIL_GROUP, P_SKILLED },
    { P_POLEARM_GROUP, P_EXPERT },	{ P_SPEAR_GROUP, P_BASIC },
    /*{ P_JAVELIN, P_BASIC },*/		/*{ P_LANCE, P_SKILLED },*/
    { P_BOW_GROUP, P_EXPERT },		{ P_THROWING_GROUP, P_EXPERT },
    { P_ATTACK_SPELL, P_SKILLED },	{ P_CLERIC_SPELL, P_SKILLED },
#ifdef STEED
    { P_RIDING, P_SKILLED },
#endif
    { P_TWO_WEAPON_COMBAT, P_EXPERT },
    { P_MARTIAL_ARTS, P_MASTER },
    { P_NONE, 0 }
};

#ifdef TOURIST
static const struct def_skill Skill_T[] = {
    { P_DAGGER_GROUP, P_EXPERT },	{ P_KNIFE_GROUP,  P_SKILLED },
    { P_AXE_GROUP, P_BASIC },		{ P_PICKAXE_GROUP, P_BASIC },
    { P_SHORT_BLADE_GROUP, P_EXPERT },	/*{ P_BROAD_BLADE_GROUP, P_BASIC },*/
    { P_LONG_BLADE_GROUP, P_BASIC },	/*{ P_TWO_HANDED_SWORD, P_BASIC },*/
    /*{ P_SCIMITAR, P_SKILLED },*/	{ P_SABER_GROUP, P_SKILLED },
    /*{ P_MACE_GROUP, P_BASIC },*/	{ P_CRUSHING_GROUP, P_BASIC },
    { P_STAFF_GROUP, P_BASIC },		/*{ P_HAMMER, P_BASIC },*/
    { P_FLAIL_GROUP, P_BASIC },		{ P_POLEARM_GROUP, P_BASIC },
    { P_SPEAR_GROUP, P_BASIC },		/*{ P_JAVELIN, P_BASIC },*/
    /*{ P_TRIDENT, P_BASIC },*/		/*{ P_LANCE, P_BASIC },*/
    { P_BOW_GROUP, P_BASIC },		{ P_THROWING_GROUP, P_EXPERT },
    /*{ P_CROSSBOW, P_BASIC },*/	/*{ P_DART, P_EXPERT },*/
    /*{ P_SHURIKEN, P_BASIC },*/	/*{ P_BOOMERANG, P_BASIC },*/
    { P_WHIP_GROUP, P_BASIC },		{ P_FIREARM_GROUP, P_BASIC },
    { P_DIVINATION_SPELL, P_BASIC },	{ P_ENCHANTMENT_SPELL, P_BASIC },
    { P_ESCAPE_SPELL, P_SKILLED },
#ifdef STEED
    { P_RIDING, P_BASIC },
#endif
    { P_TWO_WEAPON_COMBAT, P_SKILLED },
    { P_BARE_HANDED_COMBAT, P_SKILLED },
    { P_NONE, 0 }
};
#endif /* TOURIST */

static const struct def_skill Skill_V[] = {
    { P_DAGGER_GROUP, P_EXPERT },	{ P_AXE_GROUP, P_EXPERT },
    { P_PICKAXE_GROUP, P_SKILLED },	{ P_SHORT_BLADE_GROUP, P_SKILLED },
  /*{ P_BROAD_BLADE_GROUP, P_SKILLED },*/{ P_LONG_BLADE_GROUP, P_EXPERT },
  /*{ P_TWO_HANDED_SWORD, P_EXPERT },*/	/*{ P_SCIMITAR, P_BASIC },*/
    { P_SABER_GROUP, P_BASIC },		{ P_CRUSHING_GROUP, P_EXPERT },
    { P_STAFF_GROUP, P_BASIC },		{ P_POLEARM_GROUP, P_EXPERT },
    { P_SPEAR_GROUP, P_SKILLED },	/*{ P_JAVELIN, P_BASIC },*/
    /*{ P_TRIDENT, P_BASIC },*/		/*{ P_LANCE, P_SKILLED },*/
    { P_THROWING_GROUP, P_BASIC },
    { P_ATTACK_SPELL, P_BASIC },	{ P_ESCAPE_SPELL, P_BASIC },
#ifdef STEED
    { P_RIDING, P_SKILLED },
#endif
    { P_TWO_WEAPON_COMBAT, P_SKILLED },
    { P_BARE_HANDED_COMBAT, P_EXPERT },
    { P_NONE, 0 }
};

static const struct def_skill Skill_W[] = {
    { P_DAGGER_GROUP, P_EXPERT },	{ P_KNIFE_GROUP,  P_SKILLED },
    { P_AXE_GROUP, P_SKILLED },		{ P_SHORT_BLADE_GROUP, P_BASIC },
    { P_CRUSHING_GROUP, P_SKILLED },	/*{ P_MACE_GROUP, P_BASIC },*/
    { P_STAFF_GROUP, P_EXPERT },	{ P_POLEARM_GROUP, P_SKILLED },
    { P_SPEAR_GROUP, P_BASIC },		/*{ P_JAVELIN, P_BASIC },*/
    /*{ P_TRIDENT, P_BASIC },*/		/*{ P_SLING, P_SKILLED },*/
    { P_THROWING_GROUP, P_EXPERT },	/*{ P_SHURIKEN, P_BASIC },*/
    { P_ATTACK_SPELL, P_EXPERT },	{ P_HEALING_SPELL, P_SKILLED },
    { P_DIVINATION_SPELL, P_EXPERT },	{ P_ENCHANTMENT_SPELL, P_SKILLED },
    { P_CLERIC_SPELL, P_SKILLED },	{ P_ESCAPE_SPELL, P_EXPERT },
    { P_MATTER_SPELL, P_EXPERT },
#ifdef STEED
    { P_RIDING, P_BASIC },
#endif
    { P_BARE_HANDED_COMBAT, P_BASIC },
    { P_NONE, 0 }
};


STATIC_OVL void
knows_object(obj)
register int obj;
{
	discover_object(obj,TRUE,FALSE);
	objects[obj].oc_pre_discovered = 1;	/* not a "discovery" */
}

/* Know ordinary (non-magical) objects of a certain class,
 * like all gems except the loadstone and luckstone.
 */
STATIC_OVL void
knows_class(sym)
register char sym;
{
	register int ct;
	for (ct = 1; ct < NUM_OBJECTS; ct++)
		if (objects[ct].oc_class == sym && !objects[ct].oc_magic)
			knows_object(ct);
}

void
u_init()
{
	register int i;

	flags.female = flags.initgend;
	flags.beginner = 1;

	/* zero u, including pointer values --
	 * necessary when aborting from a failed restore */
	(void) memset((genericptr_t)&u, 0, sizeof(u));
	u.ustuck = (struct monst *)0;

#if 0	/* documentation of more zero values as desirable */
	u.usick_cause[0] = 0;
	u.uluck  = u.moreluck = 0;
# ifdef TOURIST
	uarmu = 0;
# endif
	uarm = uarmc = uarmh = uarms = uarmg = uarmf = 0;
	uwep = uball = uchain = uleft = uright = 0;
	uswapwep = uquiver = 0;
	u.twoweap = 0;
	u.uprotection = 0;
	u.ublessed = 0;				/* not worthy yet */
	u.ugangr   = 0;				/* gods not angry */
	u.ugifts   = 0;				/* no divine gifts bestowed */
# ifdef ELBERETH
	u.uevent.uhand_of_elbereth = 0;
# endif
	u.uevent.uheard_tune = 0;
	u.uevent.uopened_dbridge = 0;
	u.uevent.uplayed_tune = 0;
	/*u.uevent.ufound_town = 0;*/ /*ctown*/
	u.uevent.udemigod = 0;		/* not a demi-god yet... */
	u.udg_cnt = 0;
	u.mh = u.mhmax = u.mtimedone = 0;
	u.uz.dnum = u.uz0.dnum = 0;
	u.utotype = 0;
#endif	/* 0 */

	u.uz.dlevel = 1;
	u.uz0.dlevel = 0;
	u.utolev = u.uz;

	u.umoved = FALSE;
	u.umortality = 0;
	u.ugrave_arise = NON_PM;

	u.umonnum = u.umonster = (flags.female &&
			urole.femalenum != NON_PM) ? urole.femalenum :
			urole.malenum;
	set_uasmon();

	u.ulevel = 0;	/* set up some of the initial attributes */
	u.uhp = u.uhpmax = u.uhpbase = newhp();
	u.uenmax = urole.enadv.infix + urace.enadv.infix;
	if (urole.enadv.inrnd > 0)
	    u.uenmax += rnd(urole.enadv.inrnd);
	if (urace.enadv.inrnd > 0)
	    u.uenmax += rnd(urace.enadv.inrnd);
	u.uen = u.uenmax;
	u.uspellprot = 0;
	adjabil(0,1);
	u.ulevel = u.ulevelmax = 1;

	init_uhunger();
	for (i = 0; i <= MAXSPELL; i++) spl_book[i].sp_id = NO_SPELL;
	u.ublesscnt = 300;			/* no prayers just yet */
	u.ualignbase[A_CURRENT] = u.ualignbase[A_ORIGINAL] = u.ualign.type =
			aligns[flags.initalign].value;
	u.ulycn = NON_PM;
	u.uspdbon1 = 0;	/* speed bonus by light-weight */
	u.uspdbon2 = 0; /* speed bonus by potion/wand/spell */

#if defined(BSD) && !defined(POSIX_TYPES)
	(void) time((long *)&u.ubirthday);
#else
	(void) time(&u.ubirthday);
#endif

	/*
	 *  For now, everyone starts out with a night vision range of 1 and
	 *  their xray range disabled.
	 */
	u.nv_range   =  1;
	u.xray_range = -1;

//#ifdef AUTOTHRUST
	u.ulasttgt = (struct monst *)0;
//#endif

	u.ulastuse = u.unextuse = 0;


	/*** Role-specific initializations ***/
	switch (Role_switch) {
	/* rn2(100) > 50 necessary for some choices because some
	 * random number generators are bad enough to seriously
	 * skew the results if we use rn2(2)...  --KAA
	 */
	case PM_ARCHEOLOGIST:
		ini_inv(Archeologist);
/*		if(!rn2(10)) ini_inv(Tinopener);
		else if(!rn2(4)) ini_inv(Lamp);
		else if(!rn2(10)) ini_inv(Magicmarker);*/
		knows_object(SACK);
		knows_object(TOUCHSTONE);
		skill_init(Skill_A);
		break;
	case PM_BARBARIAN:
		if (rn2(100) >= 50) {	/* see above comment */
		    Barbarian[B_MAJOR].trotyp = BATTLE_AXE;
		    Barbarian[B_MINOR].trotyp = SHORT_SWORD;
		}
		ini_inv(Barbarian);
//		if(!rn2(6)) ini_inv(Lamp);
		knows_class(WEAPON_CLASS);
		knows_class(ARMOR_CLASS);
		skill_init(Skill_B);
		break;
	case PM_CAVEMAN:
//		Cave_man[C_AMMO].trquan = rn1(11, 10);	/* 10..20 */
		ini_inv(Cave_man);
		skill_init(Skill_C);
//		u.nv_range = 2;
		break;
	case PM_HEALER:
#ifndef GOLDOBJ
		u.ugold = u.ugold0 = 1000/*rn1(1000, 1001)*/;
#else
		u.umoney0 = 1000/*rn1(1000, 1001)*/;
#endif
		if (flags.female) {
		    Healer[H_ARMOR1].trotyp = NURSE_UNIFORM;
		    Healer[H_ARMOR2].trotyp = NURSE_CAP;
		    Healer[H_ARMOR1].trspe = 0;
		}
		ini_inv(Healer);
//		if(!rn2(25)) ini_inv(Lamp);
		knows_object(POT_FULL_HEALING);
		skill_init(Skill_H);
		break;
	case PM_KNIGHT:
		ini_inv(Knight);
		knows_class(WEAPON_CLASS);
		knows_class(ARMOR_CLASS);
		/* give knights chess-like mobility
		 * -- idea from wooledge@skybridge.scl.cwru.edu */
		HJumping |= FROMOUTSIDE;
		skill_init(Skill_K);
		break;
	case PM_MONK:
		switch (rn2(90) / 30) {
		case 0: Monk[M_BOOK].trotyp = SPE_HEALING; break;
		case 1: Monk[M_BOOK].trotyp = SPE_PROTECTION; break;
		case 2: Monk[M_BOOK].trotyp = SPE_SLEEP; break;
		}
		ini_inv(Monk);
/*		if(!rn2(5)) ini_inv(Magicmarker);
		else if(!rn2(10)) ini_inv(Lamp);*/
		knows_class(ARMOR_CLASS);
		skill_init(Skill_Mon);
		break;
	case PM_PRIEST:
		ini_inv(Priest);
/*		if(!rn2(10)) ini_inv(Magicmarker);
		else if(!rn2(10)) ini_inv(Lamp);*/
		knows_object(POT_WATER);
		skill_init(Skill_P);
		/* KMH, conduct --
		 * Some may claim that this isn't agnostic, since they
		 * are literally "priests" and they have holy water.
		 * But we don't count it as such.  Purists can always
		 * avoid playing priests and/or confirm another player's
		 * role in their YAAP.
		 */
		break;
	case PM_RANGER:
//		Ranger[RAN_TWO_ARROWS].trquan = rn1(10, 50);
//		Ranger[RAN_ZERO_ARROWS].trquan = rn1(10, 30);
		ini_inv(Ranger);
		skill_init(Skill_Ran);
		break;
	case PM_ROGUE:
//		Rogue[R_DAGGERS].trquan = rn1(10, 6);
#ifndef GOLDOBJ
		u.ugold = u.ugold0 = 0;
#else
		u.umoney0 = 0;
#endif
		ini_inv(Rogue);
//		if(!rn2(5)) ini_inv(Blindfold);
		knows_object(SACK);
		skill_init(Skill_R);
		break;
	case PM_SAMURAI:
//		Samurai[S_ARROWS].trquan = rn1(20, 26);
		ini_inv(Samurai);
//		if(!rn2(5)) ini_inv(Blindfold);
		knows_class(WEAPON_CLASS);
		knows_class(ARMOR_CLASS);
		objects[SHORT_SWORD].oc_skill = P_SABER_GROUP;
		skill_init(Skill_S);
		break;
#ifdef TOURIST
	case PM_TOURIST:
//		Tourist[T_DARTS].trquan = rn1(20, 21);
#ifndef GOLDOBJ
		u.ugold = u.ugold0 = 700/*rnd(1000)*/;
#else
		u.umoney0 = 700/*rnd(1000)*/;
#endif
		ini_inv(Tourist);
/*		if(!rn2(25)) ini_inv(Tinopener);
		else if(!rn2(25)) ini_inv(Leash);
		else if(!rn2(25)) ini_inv(Towel);
		else if(!rn2(25)) ini_inv(Magicmarker);*/
		skill_init(Skill_T);
		break;
#endif
	case PM_VALKYRIE:
		ini_inv(Valkyrie);
//		if(!rn2(6)) ini_inv(Lamp);
		knows_class(WEAPON_CLASS);
		knows_class(ARMOR_CLASS);
		skill_init(Skill_V);
		break;
	case PM_WIZARD:
		ini_inv(Wizard);
/*		if(!rn2(5)) ini_inv(Magicmarker);
		if(!rn2(5)) ini_inv(Blindfold);*/
		skill_init(Skill_W);
		break;

	default:	/* impossible */
		break;
	}


	/*** Race-specific initializations ***/
	switch (Race_switch) {
	case PM_HUMAN:
	    /* Nothing special */
	    break;

	case PM_ELF:
	    /*
	     * Elves are people of music and song, or they are warriors.
	     * Non-warriors get an instrument.  We use a kludge to
	     * get only non-magic instruments.
	     */
	    if (Role_if(PM_PRIEST) || Role_if(PM_WIZARD)) {
		static int trotyp[] = {
		    WOODEN_FLUTE, TOOLED_HORN, WOODEN_HARP,
		    BELL, BUGLE, LEATHER_DRUM
		};
		Instrument[0].trotyp = trotyp[rn2(SIZE(trotyp))];
		ini_inv(Instrument);
	    }

	    /* Elves can recognize all elvish objects */
	    knows_object(ELVEN_SHORT_SWORD);
	    knows_object(ELVEN_ARROW);
	    knows_object(ELVEN_BOW);
	    knows_object(ELVEN_SPEAR);
	    knows_object(ELVEN_DAGGER);
	    knows_object(ELVEN_BROADSWORD);
	    knows_object(ELVEN_MITHRIL_COAT);
	    knows_object(ELVEN_LEATHER_HELM);
	    knows_object(ELVEN_SHIELD);
	    knows_object(ELVEN_BOOTS);
	    knows_object(ELVEN_CLOAK);
	    break;

	case PM_DWARF:
	    /* Dwarves can recognize all dwarvish objects */
	    knows_object(DWARVISH_SPEAR);
	    knows_object(DWARVISH_SHORT_SWORD);
	    knows_object(DWARVISH_MATTOCK);
	    knows_object(DWARVISH_IRON_HELM);
	    knows_object(DWARVISH_MITHRIL_COAT);
	    knows_object(DWARVISH_CLOAK);
	    knows_object(DWARVISH_ROUNDSHIELD);
	    break;

	case PM_GNOME:
	    break;

	case PM_ORC:
	    /* compensate for generally inferior equipment */
	    if (!Role_if(PM_WIZARD))
		ini_inv(Xtra_food);
	    /* Orcs can recognize all orcish objects */
	    knows_object(ORCISH_SHORT_SWORD);
	    knows_object(ORCISH_ARROW);
	    knows_object(ORCISH_BOW);
	    knows_object(ORCISH_SPEAR);
	    knows_object(ORCISH_DAGGER);
	    knows_object(ORCISH_CHAIN_MAIL);
	    knows_object(ORCISH_RING_MAIL);
	    knows_object(ORCISH_HELM);
	    knows_object(ORCISH_SHIELD);
	    knows_object(URUK_HAI_SHIELD);
	    knows_object(ORCISH_CLOAK);
	    break;

	default:	/* impossible */
		break;
	}

	if (discover)
		ini_inv(Wishing);

#ifdef WIZARD
	if (wizard)
		read_wizkit();
#endif

#ifndef GOLDOBJ
	u.ugold0 += hidden_gold();	/* in case sack has gold in it */
#else
	if (u.umoney0) ini_inv(Money);
	u.umoney0 += hidden_gold();	/* in case sack has gold in it */
#endif
attrinit:
	init_attr(rn1(10,65)/*75*/);			/* init attribute values */
	max_rank_sz();			/* set max str size for class ranks */
/*
 *	Do we really need this?
 */
	for(i = 0; i < A_MAX; i++)
	    if(!rn2(20)) {
		register int xd = rn2(7) - 2;	/* biased variation */
		(void) adjattrib(i, xd, TRUE);
		if (ABASE(i) < AMAX(i)) AMAX(i) = ABASE(i);
	    }

	/* make sure you can carry all you have - especially for Tourists */
	while (inv_weight() > 0) {
		if (adjattrib(A_STR, 1, TRUE)) continue;
		if (adjattrib(A_CON, 1, TRUE)) continue;
		/* only get here when didn't boost strength or constitution */
		break;
	}
	find_ac();			/* get initial ac value */

	if (iflags.retry_attr) {
		switch (shoot_for_the_stars()) {
		    case 0:
			break;
		    case 2:		/* try again */
			goto attrinit;
		    case 1:		/* accept */
		    default:
			break;
		}
	}

	return;
}

/* skills aren't initialized, so we use the role-specific skill lists */
STATIC_OVL boolean
restricted_spell_discipline(otyp)
int otyp;
{
    const struct def_skill *skills;
    int this_skill = spell_skilltype(otyp);

    switch (Role_switch) {
     case PM_ARCHEOLOGIST:	skills = Skill_A; break;
     case PM_BARBARIAN:		skills = Skill_B; break;
     case PM_CAVEMAN:		skills = Skill_C; break;
     case PM_HEALER:		skills = Skill_H; break;
     case PM_KNIGHT:		skills = Skill_K; break;
     case PM_MONK:		skills = Skill_Mon; break;
     case PM_PRIEST:		skills = Skill_P; break;
     case PM_RANGER:		skills = Skill_Ran; break;
     case PM_ROGUE:		skills = Skill_R; break;
     case PM_SAMURAI:		skills = Skill_S; break;
#ifdef TOURIST
     case PM_TOURIST:		skills = Skill_T; break;
#endif
     case PM_VALKYRIE:		skills = Skill_V; break;
     case PM_WIZARD:		skills = Skill_W; break;
     default:			skills = 0; break;	/* lint suppression */
    }

    while (skills->skill != P_NONE) {
	if (skills->skill == this_skill) return FALSE;
	++skills;
    }
    return TRUE;
}

STATIC_OVL void
ini_inv(trop)
register struct trobj *trop;
{
	struct obj *obj;
	int otyp, i;

	while (trop->trclass) {
		if (trop->trotyp != UNDEF_TYP) {
			otyp = (int)trop->trotyp;
			if (urace.malenum != PM_HUMAN) {
			    /* substitute specific items for generic ones */
			    for (i = 0; inv_subs[i].race_pm != NON_PM; ++i)
				if (inv_subs[i].race_pm == urace.malenum &&
					otyp == inv_subs[i].item_otyp) {
				    otyp = inv_subs[i].subs_otyp;
				    break;
				}
			}
			obj = mksobj(otyp, TRUE, FALSE);
		} else {	/* UNDEF_TYP */
			static NEARDATA short nocreate = STRANGE_OBJECT;
			static NEARDATA short nocreate2 = STRANGE_OBJECT;
			static NEARDATA short nocreate3 = STRANGE_OBJECT;
			static NEARDATA short nocreate4 = STRANGE_OBJECT;
		/*
		 * For random objects, do not create certain overly powerful
		 * items: wand of wishing, ring of levitation, or the
		 * polymorph/polymorph control combination.  Specific objects,
		 * i.e. the discovery wishing, are still OK.
		 * Also, don't get a couple of really useless items.  (Note:
		 * punishment isn't "useless".  Some players who start out with
		 * one will immediately read it and use the iron ball as a
		 * weapon.)
		 */
			obj = mkobj(trop->trclass, FALSE);
			otyp = obj->otyp;
			while (otyp == WAN_WISHING
				|| otyp == nocreate
				|| otyp == nocreate2
				|| otyp == nocreate3
				|| otyp == nocreate4
#ifdef ELBERETH
				|| otyp == RIN_LEVITATION
#endif
				/* 'useless' items */
				|| otyp == POT_HALLUCINATION
				|| otyp == POT_ACID
				|| otyp == SCR_AMNESIA
				|| otyp == SCR_FIRE
				|| otyp == SCR_BLANK_PAPER
				|| otyp == SPE_BLANK_PAPER
				|| otyp == RIN_AGGRAVATE_MONSTER
				|| otyp == RIN_HUNGER
				|| otyp == WAN_NOTHING
				/* Monks don't use weapons */
				|| (otyp == SCR_ENCHANT_WEAPON &&
				    Role_if(PM_MONK))
				/* wizard patch -- they already have one */
				|| (otyp == SPE_FORCE_BOLT &&
				    Role_if(PM_WIZARD))
				/* powerful spells are either useless to
				   low level players or unbalancing; also
				   spells in restricted skill categories */
				|| (obj->oclass == SPBOOK_CLASS &&
				    (objects[otyp].oc_level > 3 ||
				    restricted_spell_discipline(otyp)))
							) {
				dealloc_obj(obj);
				obj = mkobj(trop->trclass, FALSE);
				otyp = obj->otyp;
			}

			/* Don't start with +0 or negative rings */
			if (objects[otyp].oc_charged && obj->spe <= 0)
				obj->spe = rne(3);

			/* Avoid a tin of spinach */
			if (otyp == TIN && obj->spe >= 1) {
				obj->spe = 0;
				obj->corpsenm = PM_ACID_BLOB;
			}

			/* Heavily relies on the fact that 1) we create wands
			 * before rings, 2) that we create rings before
			 * spellbooks, and that 3) not more than 1 object of a
			 * particular symbol is to be prohibited.  (For more
			 * objects, we need more nocreate variables...)
			 */
			switch (otyp) {
			    case WAN_POLYMORPH:
			    case RIN_POLYMORPH:
			    case POT_POLYMORPH:
				nocreate = RIN_POLYMORPH_CONTROL;
				break;
			    case RIN_POLYMORPH_CONTROL:
				nocreate = RIN_POLYMORPH;
				nocreate2 = SPE_POLYMORPH;
				nocreate3 = POT_POLYMORPH;
			}
			/* Don't have 2 of the same ring or spellbook */
			if (obj->oclass == RING_CLASS ||
			    obj->oclass == SPBOOK_CLASS)
				nocreate4 = otyp;
		}
		/* allow only natural materials */
		if (is_material_variable(obj)) change_material(obj, 0);
		if (obj->oclass == WEAPON_CLASS && obj->opoisoned) obj->opoisoned = 0;

#ifdef GOLDOBJ
		if (trop->trclass == COIN_CLASS) {
			/* no "blessed" or "identified" money */
			obj->quan = u.umoney0;
		} else {
#endif
			obj->dknown = obj->bknown = obj->rknown = 1;
			if (objects[otyp].oc_uses_known) obj->known = 1;
			obj->cursed = 0;
			if (obj->opoisoned && u.ualign.type != A_CHAOTIC)
			    obj->opoisoned = 0;
			if (obj->oclass == GEM_CLASS &&
			    is_graystone(obj) && obj->otyp != FLINT) {
				obj->quan = 1L;
			} else if (trop->trotyp != UNDEF_TYP) {
				obj->quan = (long) trop->trquan;
				trop->trquan = 1;
			}
			if (trop->trspe != UNDEF_SPE)
			    obj->spe = trop->trspe;
			if (trop->trbless != UNDEF_BLESS)
			    obj->blessed = trop->trbless;
#ifdef GOLDOBJ
		}
#endif
		/* defined after setting otyp+quan + blessedness */
		obj->owt = weight(obj);
		obj = addinv(obj);

		/* Make the type known if necessary */
		if (OBJ_DESCR(objects[otyp]) && obj->known)
			discover_object(otyp, TRUE, FALSE);
		if (otyp == OIL_LAMP)
			discover_object(POT_OIL, TRUE, FALSE);

		if(obj->oclass == ARMOR_CLASS){
			if (is_shield(obj) && !uarms) {
				setworn(obj, W_ARMS);
				if (uswapwep) setuswapwep((struct obj *) 0);
			} else if (is_helmet(obj) && !uarmh)
				setworn(obj, W_ARMH);
			else if (is_gloves(obj) && !uarmg)
				setworn(obj, W_ARMG);
#ifdef TOURIST
			else if (is_shirt(obj) && !uarmu)
				setworn(obj, W_ARMU);
#endif
			else if (is_cloak(obj) && !uarmc)
				setworn(obj, W_ARMC);
			else if (is_boots(obj) && !uarmf)
				setworn(obj, W_ARMF);
			else if (is_suit(obj) && !uarm)
				setworn(obj, W_ARM);
		}

		if (obj->oclass == WEAPON_CLASS || is_weptool(obj) ||
			otyp == TIN_OPENER || otyp == FLINT || otyp == ROCK) {
		    if (is_ammo(obj) || is_missile(obj)) {
			if (!uquiver) setuqwep(obj);
		    } else if (!uwep) setuwep(obj);
		    else if (!uswapwep) setuswapwep(obj);
		}
		if (obj->oclass == SPBOOK_CLASS &&
				obj->otyp != SPE_BLANK_PAPER)
		    initialspell(obj);

#if !defined(PYRAMID_BUG) && !defined(MAC)
		if(--trop->trquan) continue;	/* make a similar object */
#else
		if(trop->trquan) {		/* check if zero first */
			--trop->trquan;
			if(trop->trquan)
				continue;	/* make a similar object */
		}
#endif
		trop++;
	}
}

int
shoot_for_the_stars()		/* 0:quit 1:accept 2:retry */
{
	winid win;
	anything any;
	menu_item *selected;
	char buf[BUFSZ];
	int n;

	any.a_void = 0;
	win = create_nhwindow(NHW_MENU);

	start_menu(win);

	if (ABASE(A_STR) > 18) {
		if (ABASE(A_STR) > STR18(100))
		    Sprintf(buf, "St:%2d ",ABASE(A_STR)-100);
		else if (ABASE(A_STR) < STR18(100))
		    Sprintf(buf, "St:18/%02d ",ABASE(A_STR)-18);
		else
		    Sprintf(buf, "St:18/** ");
	} else
		Sprintf(buf, "St:%-1d ",ABASE(A_STR));
	Sprintf(eos(buf),
		"Dx:%-1d Co:%-1d In:%-1d Wi:%-1d Ch:%-1d",
		ABASE(A_DEX), ABASE(A_CON), ABASE(A_INT), ABASE(A_WIS), ABASE(A_CHA));
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);

	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	any.a_int = 1+1;
	add_menu(win, NO_GLYPH, &any, 'y', 0, ATR_NONE,
			E_J("accept them","これで良い"), MENU_UNSELECTED);
	any.a_int = 2+1;
	add_menu(win, NO_GLYPH, &any, 'n', 0, ATR_NONE,
			E_J("try again","やり直す"), MENU_UNSELECTED);

	Sprintf(buf, E_J("Will you accept these attributes? [yn]",
			 "この能力値で開始しますか？[yn]"));
	end_menu(win, buf);

	n = select_menu(win, PICK_ONE, &selected);
	destroy_nhwindow(win);
	if (n > 0) {
		n = selected[0].item.a_int - 1;	/* get item selected */
		free((genericptr_t)selected);
	}

	return n;
}

/*u_init.c*/
