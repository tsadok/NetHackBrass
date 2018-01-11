/*	SCCS Id: @(#)drawing.c	3.4	1999/12/02	*/
/* Copyright (c) NetHack Development Team 1992.			  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "tcap.h"

/* Relevent header information in rm.h and objclass.h. */

#ifdef C
#undef C
#endif

#ifdef TEXTCOLOR
#define C(n) n
#else
#define C(n)
#endif

#ifdef JP
#define EJ2(e,j) j,e
#else /*JP*/
#define EJ2(e,j) e
#endif /*JP*/

#define g_FILLER(symbol) 0

uchar oc_syms[MAXOCLASSES] = DUMMY; /* the current object  display symbols */
uchar showsyms[MAXPCHARS]  = DUMMY; /* the current feature display symbols */
uchar monsyms[MAXMCLASSES] = DUMMY; /* the current monster display symbols */
uchar warnsyms[WARNCOUNT]  = DUMMY;  /* the current warning display symbols */

/* Default object class symbols.  See objclass.h. */
const char def_oc_syms[MAXOCLASSES] = {
/* 0*/	'\0',		/* placeholder for the "random class" */
	ILLOBJ_SYM,
	WEAPON_SYM,
	ARMOR_SYM,
	RING_SYM,
/* 5*/	AMULET_SYM,
	TOOL_SYM,
	FOOD_SYM,
	POTION_SYM,
	SCROLL_SYM,
/*10*/	SPBOOK_SYM,
	WAND_SYM,
	GOLD_SYM,
	GEM_SYM,
	ROCK_SYM,
/*15*/	BALL_SYM,
	CHAIN_SYM,
	VENOM_SYM
};

const char invisexplain[] = E_J("remembered, unseen, creature", "�����ȉ����̂����ꏊ");

/* Object descriptions.  Used in do_look(). */
const char * const objexplain[] = {	/* these match def_oc_syms, above */
/* 0*/	0,
	E_J("strange object",		"��ȕ���"),
	E_J("weapon",			"����"),
	E_J("suit or piece of armor",	"���܂��͊Z"),
	E_J("ring",			"�w��"),
/* 5*/	E_J("amulet",			"������"),
	E_J("useful item (pick-axe, key, lamp...)","�֗��ȓ���(��͂��A���A�����v��)"),
	E_J("piece of food",		"�H��"),
	E_J("potion",			"��"),
	E_J("scroll",			"����"),
/*10*/	E_J("spellbook",		"���@��"),
	E_J("wand",			"��"),
	E_J("pile of coins",		"���݂̎R"),
	E_J("gem or rock",		"��ʂ��"),
	E_J("boulder or statue",	"���⒤��"),
/*15*/	E_J("iron ball",		"�S��"),
	E_J("iron chain",		"�S�̍�"),
	E_J("splash of venom",		"�ŉt�̂��Ԃ�")
};

/* Object class names.  Used in object_detect(). */
const char * const oclass_names[] = {
/* 0*/	0,
	E_J("illegal objects",	"��ȕ���"),
	E_J("weapons",		"����"),
	E_J("armor",		"�h��"),
	E_J("rings",		"�w��"),
/* 5*/	E_J("amulets",		"������"),
	E_J("tools",		"����"),
	E_J("food",		"�H��"),
	E_J("potions",		"��"),
	E_J("scrolls",		"����"),
/*10*/	E_J("spellbooks",	"���@��"),
	E_J("wands",		"��"),
	E_J("coins",		"����"),
	E_J("rocks",		"��"),
	E_J("large stones",	"���"),
/*15*/	E_J("iron balls",	"�S��"),
	E_J("chains",		"��"),
	E_J("venoms",		"�ŉt")
};

/* Default monster class symbols.  See monsym.h. */
const char def_monsyms[MAXMCLASSES] = {
	'\0',		/* holder */
	DEF_ANT,
	DEF_BLOB,
	DEF_COCKATRICE,
	DEF_DOG,
	DEF_EYE,
	DEF_FELINE,
	DEF_GREMLIN,
	DEF_HUMANOID,
	DEF_IMP,
	DEF_JELLY,		/* 10 */
	DEF_KOBOLD,
	DEF_LEPRECHAUN,
	DEF_MIMIC,
	DEF_NYMPH,
	DEF_ORC,
	DEF_PIERCER,
	DEF_QUADRUPED,
	DEF_RODENT,
	DEF_SPIDER,
	DEF_TRAPPER,		/* 20 */
	DEF_UNICORN,
	DEF_VORTEX,
	DEF_WORM,
	DEF_XAN,
	DEF_LIGHT,
	DEF_ZRUTY,
	DEF_ANGEL,
	DEF_BAT,
	DEF_CENTAUR,
	DEF_DRAGON,		/* 30 */
	DEF_ELEMENTAL,
	DEF_FUNGUS,
	DEF_GNOME,
	DEF_GIANT,
	'\0',
	DEF_JABBERWOCK,
	DEF_KOP,
	DEF_LICH,
	DEF_MUMMY,
	DEF_NAGA,		/* 40 */
	DEF_OGRE,
	DEF_PUDDING,
	DEF_QUANTMECH,
	DEF_RUSTMONST,
	DEF_SNAKE,
	DEF_TROLL,
	DEF_UMBER,
	DEF_VAMPIRE,
	DEF_WRAITH,
	DEF_XORN,		/* 50 */
	DEF_YETI,
	DEF_ZOMBIE,
	DEF_HUMAN,
	DEF_GHOST,
	DEF_GOLEM,
	DEF_DEMON,
	DEF_EEL,
	DEF_LIZARD,
	DEF_WORM_TAIL,
	DEF_MIMIC_DEF,		/* 60 */
};

/* The explanations below are also used when the user gives a string
 * for blessed genocide, so no text should wholly contain any later
 * text.  They should also always contain obvious names (eg. cat/feline).
 */
const char * const monexplain[MAXMCLASSES] = {
    0,
#ifndef JP
    "ant or other insect",	"blob",			"cockatrice",
    "dog or other canine",	"eye or sphere",	"cat or other feline",
    "gremlin",			"humanoid",		"imp or minor demon",
    "jelly",			"kobold",		"leprechaun",
    "mimic",			"nymph",		"orc",
    "piercer",			"quadruped",		"rodent",
    "arachnid or centipede",	"trapper or lurker above", "unicorn or horse",
    "vortex",		"worm", "xan or other mythical/fantastic insect",
    "light",			"zruty",

    "angelic being",		"bat or bird",		"centaur",
    "dragon",			"elemental",		"fungus or mold",
    "gnome",			"giant humanoid",	0,
    "jabberwock",		"Keystone Kop",		"lich",
    "mummy",			"naga",			"ogre",
    "pudding or ooze",		"quantum mechanic",	"rust monster or disenchanter",
    "snake",			"troll",		"umber hulk",
    "vampire",			"wraith",		"xorn",
    "apelike creature",		"zombie",

    "human or elf",		"ghost",		"golem",
    "major demon",		"sea monster",		"lizard",
    "long worm tail",		"mimic"
#else
    "�a�₻�̑��̍���",		"�u���b�u",		"�R�J�g���X",
    "������уC�k�Ȃ̓���",	"�ڋʁE����",		"�L����уl�R�Ȃ̓���",
    "�O��������",		"�q���[�}�m�C�h",	"�����̈���",
    "�[���[",			"�R�{���h",		"���v���R�[��",
    "�~�~�b�N",			"�j���t",		"�I�[�N",
    "�s�A�T�[",			"�l���b",		"ꖎ���",
    "�w偁E���J�f��",		"�g���b�p�[�E���[�J�[", "���j�R�[������єn",
    "�Q",			"���[��",		"�U������ѐ_��I�ȍ���",
    "��",			"�Y���e�B",

    "�V�g�E�_�̎g��",		"�R�E�����E��",		"�P���^�E���X",
    "�h���S��",			"�G�������^��",		"�L�m�R�E�J�r��",
    "�m�[��",			"���l",			0,
    "�W���o�E�H�b�N",		"�L�[�X�g���E�R�b�v",	"���b�`",
    "�~�C��",			"�i�[�K",		"�I�[�K",
    "�v�����܂��̓E�[�Y",	"�ʎq�͊w",		"�K�E�z���̉���",
    "��",			"�g����",		"�A���o�[�n���N",
    "�z���S",			"���C�X",		"�]�[��",
    "���̂悤�ȉ���",		"�]���r",

    "�l�ԁE�G���t",		"�H��",			"�S�[����",
    "��ʂ̈���",		"�����̉���",		"�g�J�Q",
    "�����O���[���̓���",	"�~�~�b�N"
#endif /*JP*/
};

const struct symdef def_warnsyms[WARNCOUNT] = {
#ifndef JP
	{'0', "unknown creature causing you worry", C(CLR_BRIGHT_GREEN)},  	/* white warning  */
	{'1', "unknown creature causing you concern", C(CLR_YELLOW)},	/* pink warning   */
	{'2', "unknown creature causing you anxiety", C(CLR_ORANGE)},	/* red warning    */
	{'3', "unknown creature causing you disquiet", C(CLR_RED)},	/* ruby warning   */
	{'4', "unknown creature causing you alarm",
						C(CLR_MAGENTA)},        /* purple warning */
	{'5', "unknown creature causing you dread",
						C(CLR_BRIGHT_MAGENTA)}	/* black warning  */
#else
	{'0', "���Ȃ����C�ɂ��Ă��鑶��", 0, C(CLR_BRIGHT_GREEN)},
	{'1', "���Ȃ���S�z�����鑶��",	  0, C(CLR_YELLOW)},
	{'2', "���Ȃ������O�����鑶��",   0, C(CLR_ORANGE)},
	{'3', "���Ȃ��𓮗h�����鑶��",   0, C(CLR_RED)},
	{'4', "���Ȃ����x���������鑶��", 0, C(CLR_MAGENTA)},
	{'5', "���Ȃ������|�����鑶��",	  0, C(CLR_BRIGHT_MAGENTA)}
#endif /*JP*/
};

/*
 *  Default screen symbols with explanations and colors.
 *  Note:  {ibm|dec|mac}_graphics[] arrays also depend on this symbol order.
 */
const struct symdef defsyms[MAXPCHARS] = {
/* 0*/	{' ', EJ2("dark part of a room","�����̈Â�����"), C(NO_COLOR)},	/* stone */
	{'|', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* vwall */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* hwall */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* tlcorn */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* trcorn */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* blcorn */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* brcorn */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* crwall */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* tuwall */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* tdwall */
/*10*/	{'|', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* tlwall */
	{'|', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* trwall */
	{'.', EJ2("doorway",		"�ʘH"),	   C(CLR_GRAY)},	/* ndoor */
	{'-', EJ2("open door",		"�J������"),	   C(CLR_BROWN)},	/* vodoor */
	{'|', EJ2("open door",		"�J������"),	   C(CLR_BROWN)},	/* hodoor */
	{'+', EJ2("closed door",	"������"),	   C(CLR_BROWN)},	/* vcdoor */
	{'+', EJ2("closed door",	"������"),	   C(CLR_BROWN)},	/* hcdoor */
	{'#', EJ2("iron bars",		"�S�i�q"),	   C(HI_METAL)},	/* bars */
	{'#', EJ2("tree",		"��"),		   C(CLR_GREEN)},	/* tree */
	{'#', EJ2("dead tree",		"�͂��"),	   C(CLR_BLACK)},	/* dead tree */
	{' ', EJ2("dark part of a room","�����̈Â�����"), C(NO_COLOR)},	/* dark part of a room */
/*20*/	{'.', EJ2("floor of a room",	"�����̏�"),	   C(CLR_GRAY)},	/* room */
	{'#', EJ2("corridor",		"�L��"),	   C(CLR_GRAY)},	/* dark corr */
	{'#', EJ2("lit corridor",	"���邢�L��"),	   C(CLR_GRAY)},	/* lit corr (see mapglyph.c) */
	{'}', EJ2("water",		"����"),	   C(CLR_BLUE)},	/* pool */
	{'}', EJ2("water",		"����"),	   C(CLR_BRIGHT_BLUE)},	/* pool (lit) */
	{'.', EJ2("ice",		"�X"),		   C(CLR_CYAN)},	/* ice */
	{'.', EJ2("ice",		"�X"),		   C(CLR_BRIGHT_CYAN)},	/* ice (lit) */
	{'{', EJ2("muddy swamp",	"�D��"),	   C(CLR_BLACK)},	/* muddy swamp, bog */
	{'{', EJ2("muddy swamp",	"�D��"),	   C(CLR_GREEN)},	/* muddy swamp, bog (lit) */
	{' ', EJ2("dark part of a room","�����̈Â�����"), C(NO_COLOR)},	/* ground */
	{'.', EJ2("ground",		"�n��"),	   C(CLR_BROWN)},	/* ground (lit) */
	{'.', EJ2("grass",		"���n"),	   C(CLR_BLACK)},	/* grass */
	{'.', EJ2("grass",		"���n"),	   C(CLR_GREEN)},	/* grass (lit) */
	{'.', EJ2("carpet",		"�O�~�̏�"),	   C(CLR_BLACK)},	/* carpet */
	{'.', EJ2("carpet",		"�O�~�̏�"),	   C(CLR_RED)},		/* carpet (lit) */
	{'<', EJ2("staircase up",	"���K�i"),	   C(CLR_GRAY)},	/* upstair */
	{'>', EJ2("staircase down",	"����K�i"),	   C(CLR_GRAY)},	/* dnstair */
	{'<', EJ2("ladder up",		"����q"),	   C(CLR_BROWN)},	/* upladder */
	{'>', EJ2("ladder down",	"�����q"),	   C(CLR_BROWN)},	/* dnladder */
	{'_', EJ2("altar",		"�Ւd"),	   C(CLR_GRAY)},	/* altar */
	{'|', EJ2("grave",		"��"),		   C(CLR_GRAY)},	/* grave */
	{'\\',EJ2("opulent throne",	"���؂ȋʍ�"),	   C(HI_GOLD)},		/* throne */
#ifdef SINKS
/*30*/	{'#', EJ2("sink",		"������"),	   C(CLR_GRAY)},	/* sink */
#else
	{'#', "",					   C(CLR_GRAY)},	/* sink */
#endif
	{'{', EJ2("fountain",		"��"),		   C(CLR_BLUE)},	/* fountain */
	{'}', EJ2("molten lava",	"�ς�������n��"), C(CLR_RED)},		/* lava */
	{'.', EJ2("lowered drawbridge",	"���肽���ˋ�"),   C(CLR_BROWN)},	/* vodbridge */
	{'.', EJ2("lowered drawbridge",	"���肽���ˋ�"),   C(CLR_BROWN)},	/* hodbridge */
	{'#', EJ2("raised drawbridge",	"�オ�������ˋ�"), C(CLR_BROWN)},	/* vcdbridge */
	{'#', EJ2("raised drawbridge",	"�オ�������ˋ�"), C(CLR_BROWN)},	/* hcdbridge */
/*40*/	{' ', EJ2("air",		"��"),	   C(CLR_CYAN)},	/* open air */
	{'#', EJ2("cloud",		"�_"),		   C(CLR_GRAY)},	/* [part of] a cloud */
	{'}', EJ2("water",		"����"),	   C(CLR_BLUE)},	/* under water */
	{'^', EJ2("arrow trap",		"����"),	   C(HI_METAL)},	/* trap */
	{'^', EJ2("dart trap",		"�_�[�c���"),	   C(HI_METAL)},	/* trap */
	{'^', EJ2("falling rock trap",	"���΂��"),	   C(CLR_GRAY)},	/* trap */
	{'^', EJ2("squeaky board",	"�����ޏ���"),	   C(CLR_BROWN)},	/* trap */
	{'^', EJ2("bear trap",		"�g���o�T�~"),	   C(HI_METAL)},	/* trap */
	{'^', EJ2("land mine",		"�n��"),	   C(CLR_RED)},		/* trap */
	{'^', EJ2("rolling boulder trap","�]��������"),C(CLR_GRAY)},	/* trap */
/*50*/	{'^', EJ2("sleeping gas trap",	"�����K�X���"),   C(HI_ZAP)},		/* trap */
	{'^', EJ2("rust trap",		"���H���"),	   C(CLR_BLUE)},	/* trap */
	{'^', EJ2("fire trap",		"�����"),	   C(CLR_ORANGE)},	/* trap */
	{'^', EJ2("pit",		"������"),	   C(CLR_BLACK)},	/* trap */
	{'^', EJ2("spiked pit",	"���̎d�|����ꂽ������"), C(CLR_BLACK)},	/* trap */
	{'^', EJ2("hole",		"��"),		   C(CLR_BROWN)},	/* trap */
	{'^', EJ2("trap door",		"���Ƃ���"),	   C(CLR_BROWN)},	/* trap */
	{'^', EJ2("teleportation trap",	"�e���|�[�g���"), C(CLR_MAGENTA)},	/* trap */
	{'^', EJ2("level teleporter",	"���x���e���|�[�g���"), C(CLR_MAGENTA)},	/* trap */
	{'^', EJ2("magic portal",	"���@�̓���"),	   C(CLR_BRIGHT_MAGENTA)},	/* trap */
/*60*/	{'"', EJ2("web",		"�w偂̑�"),	   C(CLR_GRAY)},	/* web */
	{'^', EJ2("statue trap",	"�������"),	   C(CLR_GRAY)},	/* trap */
	{'^', EJ2("magic trap",		"���@���"),	   C(HI_ZAP)},		/* trap */
	{'^', EJ2("anti-magic field",	"�����@���"),	   C(HI_ZAP)},		/* trap */
	{'^', EJ2("polymorph trap",	"�ω����"),	   C(CLR_BRIGHT_GREEN)},/* trap */
	{'|', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* vbeam */
	{'-', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* hbeam */
	{'\\',EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* lslant */
	{'/', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rslant */
	{'|', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* vbeaml */
	{'|', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* vbeamr */
	{'~', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* hbeamu */
	{'_', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* hbeamd */
	{'*', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_tl */
	{'V', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_tc */
	{'*', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_tr */
	{'>', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_ml */
	{'<', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_mr */
	{'*', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_bl */
	{'^', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_bc */
	{'*', EJ2("wall",		"��"),		   C(CLR_GRAY)},	/* rbeam_br */
	{'*', EJ2("",			""),		   C(CLR_WHITE)},	/* dig beam */
/*70*/	{'!', EJ2("",			""),		   C(CLR_WHITE)},	/* camera flash beam */
	{')', EJ2("",			""),		   C(HI_WOOD)},		/* boomerang open left */
	{'(', EJ2("",			""),		   C(HI_WOOD)},		/* boomerang open right */
	{'0', EJ2("",			""),		   C(HI_ZAP)},		/* 4 magic shield symbols */
	{'#', EJ2("",			""),		   C(HI_ZAP)},
	{'@', EJ2("",			""),		   C(HI_ZAP)},
	{'*', EJ2("",			""),		   C(HI_ZAP)},
	{'/', EJ2("",			""),		   C(CLR_GREEN)},	/* swallow top left	*/
	{'-', EJ2("",			""),		   C(CLR_GREEN)},	/* swallow top center	*/
	{'\\',EJ2("",			""),		   C(CLR_GREEN)},	/* swallow top right	*/
/*80*/	{'|', EJ2("",			""),		   C(CLR_GREEN)},	/* swallow middle left	*/
	{'|', EJ2("",			""),		   C(CLR_GREEN)},	/* swallow middle right	*/
	{'\\',EJ2("",			""),		   C(CLR_GREEN)},	/* swallow bottom left	*/
	{'-', EJ2("",			""),		   C(CLR_GREEN)},	/* swallow bottom center*/
	{'/', EJ2("",			""),		   C(CLR_GREEN)},	/* swallow bottom right	*/
	{'/', EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion top left     */
	{'-', EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion top center   */
	{'\\',EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion top right    */
	{'|', EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion middle left  */
	{' ', EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion middle center*/
/*90*/	{'|', EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion middle right */
	{'\\',EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion bottom left  */
	{'-', EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion bottom center*/
	{'/', EJ2("",			""),		   C(CLR_ORANGE)},	/* explosion bottom right */
	{'*', EJ2("thin poison cloud",	"�󔖂ȓŖ�"),	   C(CLR_WHITE)},	/* stinking cloud */
	{'#', EJ2("dense poison cloud",	"�Z���ȓŖ�"),	   C(CLR_WHITE)},	/* stinking cloud */
/*
 *  Note: Additions to this array should be reflected in the
 *	  {ibm,dec,mac}_graphics[] arrays below.
 */
};

#undef C

#ifdef ASCIIGRAPH

#ifdef PC9800
void NDECL((*ibmgraphics_mode_callback)) = 0;	/* set in tty_start_screen() */
#endif /* PC9800 */

static uchar ibm_graphics[MAXPCHARS] = {
/* 0*/	g_FILLER(S_stone),
	0xb3,	/* S_vwall:	meta-3, vertical rule */
	0xc4,	/* S_hwall:	meta-D, horizontal rule */
	0xda,	/* S_tlcorn:	meta-Z, top left corner */
	0xbf,	/* S_trcorn:	meta-?, top right corner */
	0xc0,	/* S_blcorn:	meta-@, bottom left */
	0xd9,	/* S_brcorn:	meta-Y, bottom right */
	0xc5,	/* S_crwall:	meta-E, cross */
	0xc1,	/* S_tuwall:	meta-A, T up */
	0xc2,	/* S_tdwall:	meta-B, T down */
/*10*/	0xb4,	/* S_tlwall:	meta-4, T left */
	0xc3,	/* S_trwall:	meta-C, T right */
	0xfa,	/* S_ndoor:	meta-z, centered dot */
	0xfe,	/* S_vodoor:	meta-~, small centered square */
	0xfe,	/* S_hodoor:	meta-~, small centered square */
	g_FILLER(S_vcdoor),
	g_FILLER(S_hcdoor),
	240,	/* S_bars:	equivalence symbol */
	241,	/* S_tree:	plus or minus symbol */
	241,	/* S_deadtree:	plus or minus symbol */
	g_FILLER(S_stone),
/*20*/	0xfa,	/* S_room:	meta-z, centered dot */
	0xb0,	/* S_corr:	meta-0, light shading */
	0xb1,	/* S_litcorr:	meta-1, medium shading */
	0xf7,	/* S_pool:	meta-w, approx. equals */
	0xf7,	/* S_pool:	meta-w, approx. equals */
	0xfa,	/* S_ice:	meta-z, centered dot */
	0xfa,	/* S_ice:	meta-z, centered dot */
	0xf7,	/* S_bog:	meta-w, approx. equals */
	0xf7,	/* S_bog:	meta-w, approx. equals */
	g_FILLER(S_stone),
	0xfa,	/* S_ground:	meta-z, centered dot */
	0xfa,	/* S_grass:	meta-z, centered dot */
	0xfa,	/* S_grass:	meta-z, centered dot */
	0xfa,	/* S_carpet:	meta-z, centered dot */
	0xfa,	/* S_carpet:	meta-z, centered dot */
	g_FILLER(S_upstair),
	g_FILLER(S_dnstair),
	g_FILLER(S_upladder),
	g_FILLER(S_dnladder),
	g_FILLER(S_altar),
	0xef,	/* S_grave:	*/
	g_FILLER(S_throne),
/*30*/	g_FILLER(S_sink),
	0xf4,	/* S_fountain:	meta-t, integral top half */
	0xf7,	/* S_lava:	meta-w, approx. equals */
	0xfa,	/* S_vodbridge:	meta-z, centered dot */
	0xfa,	/* S_hodbridge:	meta-z, centered dot */
	g_FILLER(S_vcdbridge),
	g_FILLER(S_hcdbridge),
/*40*/	g_FILLER(S_air),
	g_FILLER(S_cloud),
	0xf7,	/* S_water:	meta-w, approx. equals */
	g_FILLER(S_arrow_trap),
	g_FILLER(S_dart_trap),
	g_FILLER(S_falling_rock_trap),
	g_FILLER(S_squeaky_board),
	g_FILLER(S_bear_trap),
	g_FILLER(S_land_mine),
	g_FILLER(S_rolling_boulder_trap),
/*50*/	g_FILLER(S_sleeping_gas_trap),
	g_FILLER(S_rust_trap),
	g_FILLER(S_fire_trap),
	g_FILLER(S_pit),
	g_FILLER(S_spiked_pit),
	g_FILLER(S_hole),
	g_FILLER(S_trap_door),
	g_FILLER(S_teleportation_trap),
	g_FILLER(S_level_teleporter),
	g_FILLER(S_magic_portal),
/*60*/	g_FILLER(S_web),
	g_FILLER(S_statue_trap),
	g_FILLER(S_magic_trap),
	g_FILLER(S_anti_magic_trap),
	g_FILLER(S_polymorph_trap),
	0xb3,	/* S_vbeam:	meta-3, vertical rule */
	0xc4,	/* S_hbeam:	meta-D, horizontal rule */
	g_FILLER(S_lslant),
	g_FILLER(S_rslant),
	0xdd,	/*g_FILLER(S_vbeaml),*/
	0xde,	/*g_FILLER(S_vbeamr),*/
	0xdf,	/*g_FILLER(S_hbeamu),*/
	0xdc,	/*g_FILLER(S_hbeamd),*/
	0xd9,	/* S_rbeam_tl */
	g_FILLER(S_rbeam_tc),
	0xc0,	/* S_rbeam_tr */
	g_FILLER(S_rbeam_ml),
	g_FILLER(S_rbeam_mr),
	0xbf,	/* S_rbeam_bl */
	g_FILLER(S_rbeam_bc),
	0xba,	/* S_rbeam_br */
	g_FILLER(S_digbeam),
/*70*/	g_FILLER(S_flashbeam),
	g_FILLER(S_boomleft),
	g_FILLER(S_boomright),
	g_FILLER(S_ss1),
	g_FILLER(S_ss2),
	g_FILLER(S_ss3),
	g_FILLER(S_ss4),
	g_FILLER(S_sw_tl),
	g_FILLER(S_sw_tc),
	g_FILLER(S_sw_tr),
/*80*/	0xb3,	/* S_sw_ml:	meta-3, vertical rule */
	0xb3,	/* S_sw_mr:	meta-3, vertical rule */
	g_FILLER(S_sw_bl),
	g_FILLER(S_sw_bc),
	g_FILLER(S_sw_br),
	g_FILLER(S_explode1),
	g_FILLER(S_explode2),
	g_FILLER(S_explode3),
	0xb3,	/* S_explode4:	meta-3, vertical rule */
	g_FILLER(S_explode5),
/*90*/	0xb3,	/* S_explode6:	meta-3, vertical rule */
	g_FILLER(S_explode7),
	g_FILLER(S_explode8),
	g_FILLER(S_explode9),
	g_FILLER(S_thinpcloud),//	0xb0,	/* S_thinpcloud */
	g_FILLER(S_densepcloud),//0xb1,	/* S_densepcloud */
};
#endif  /* ASCIIGRAPH */

#ifdef TERMLIB
void NDECL((*decgraphics_mode_callback)) = 0;  /* set in tty_start_screen() */

static uchar dec_graphics[MAXPCHARS] = {
/* 0*/	g_FILLER(S_stone),
	0xf8,	/* S_vwall:	meta-x, vertical rule */
	0xf1,	/* S_hwall:	meta-q, horizontal rule */
	0xec,	/* S_tlcorn:	meta-l, top left corner */
	0xeb,	/* S_trcorn:	meta-k, top right corner */
	0xed,	/* S_blcorn:	meta-m, bottom left */
	0xea,	/* S_brcorn:	meta-j, bottom right */
	0xee,	/* S_crwall:	meta-n, cross */
	0xf6,	/* S_tuwall:	meta-v, T up */
	0xf7,	/* S_tdwall:	meta-w, T down */
/*10*/	0xf5,	/* S_tlwall:	meta-u, T left */
	0xf4,	/* S_trwall:	meta-t, T right */
	0xfe,	/* S_ndoor:	meta-~, centered dot */
	0xe1,	/* S_vodoor:	meta-a, solid block */
	0xe1,	/* S_hodoor:	meta-a, solid block */
	g_FILLER(S_vcdoor),
	g_FILLER(S_hcdoor),
	0xfb,	/* S_bars:	meta-{, small pi */
	0xe7,	/* S_tree:	meta-g, plus-or-minus */
	0xe7,	/* S_deadtree:	meta-g, plus-or-minus */
	g_FILLER(S_stone),
/*20*/	0xfe,	/* S_room:	meta-~, centered dot */
	g_FILLER(S_corr),
	g_FILLER(S_litcorr),
	0xe0,	/* S_pool:	meta-\, diamond */
	0xe0,	/* S_pool:	meta-\, diamond */
	0xfe,	/* S_ice:	meta-~, centered dot */
	0xfe,	/* S_ice:	meta-~, centered dot */
	0xe0,	/* S_bog:	meta-\, diamond */
	0xe0,	/* S_bog:	meta-\, diamond */
	0xfe,	/* S_ground:	meta-~, centered dot */
	0xfe,	/* S_ground:	meta-~, centered dot */
	0xfe,	/* S_grass:	meta-~, centered dot */
	0xfe,	/* S_grass:	meta-~, centered dot */
	0xfe,	/* S_carpet:	meta-~, centered dot */
	0xfe,	/* S_carpet:	meta-~, centered dot */
	g_FILLER(S_upstair),
	g_FILLER(S_dnstair),
	0xf9,	/* S_upladder:	meta-y, greater-than-or-equals */
	0xfa,	/* S_dnladder:	meta-z, less-than-or-equals */
	g_FILLER(S_altar),	/* 0xc3, \E)3: meta-C, dagger */
	g_FILLER(S_grave),
	g_FILLER(S_throne),
/*30*/	g_FILLER(S_sink),
	g_FILLER(S_fountain),	/* 0xdb, \E)3: meta-[, integral top half */
	0xe0,	/* S_lava:	meta-\, diamond */
	0xfe,	/* S_vodbridge:	meta-~, centered dot */
	0xfe,	/* S_hodbridge:	meta-~, centered dot */
	g_FILLER(S_vcdbridge),
	g_FILLER(S_hcdbridge),
/*40*/	g_FILLER(S_air),
	g_FILLER(S_cloud),
	0xe0,	/* S_water:	meta-\, diamond */
	g_FILLER(S_arrow_trap),
	g_FILLER(S_dart_trap),
	g_FILLER(S_falling_rock_trap),
	g_FILLER(S_squeaky_board),
	g_FILLER(S_bear_trap),
	g_FILLER(S_land_mine),
	g_FILLER(S_rolling_boulder_trap),
/*50*/	g_FILLER(S_sleeping_gas_trap),
	g_FILLER(S_rust_trap),
	g_FILLER(S_fire_trap),
	g_FILLER(S_pit),
	g_FILLER(S_spiked_pit),
	g_FILLER(S_hole),
	g_FILLER(S_trap_door),
	g_FILLER(S_teleportation_trap),
	g_FILLER(S_level_teleporter),
	g_FILLER(S_magic_portal),
/*60*/	g_FILLER(S_web),	/* 0xbd, \E)3: meta-=, int'l currency */
	g_FILLER(S_statue_trap),
	g_FILLER(S_magic_trap),
	g_FILLER(S_anti_magic_trap),
	g_FILLER(S_polymorph_trap),
	0xf8,	/* S_vbeam:	meta-x, vertical rule */
	0xf1,	/* S_hbeam:	meta-q, horizontal rule */
	g_FILLER(S_lslant),
	g_FILLER(S_rslant),
	g_FILLER(S_vbeaml),
	g_FILLER(S_vbeamr),
	g_FILLER(S_hbeamu),
	g_FILLER(S_hbeamd),
	g_FILLER(S_rbeam_tl),
	g_FILLER(S_rbeam_tc),
	g_FILLER(S_rbeam_tr),
	g_FILLER(S_rbeam_ml),
	g_FILLER(S_rbeam_mr),
	g_FILLER(S_rbeam_bl),
	g_FILLER(S_rbeam_bc),
	g_FILLER(S_rbeam_br),
	g_FILLER(S_digbeam),
/*70*/	g_FILLER(S_flashbeam),
	g_FILLER(S_boomleft),
	g_FILLER(S_boomright),
	g_FILLER(S_ss1),
	g_FILLER(S_ss2),
	g_FILLER(S_ss3),
	g_FILLER(S_ss4),
	g_FILLER(S_sw_tl),
	0xef,	/* S_sw_tc:	meta-o, high horizontal line */
	g_FILLER(S_sw_tr),
/*80*/	0xf8,	/* S_sw_ml:	meta-x, vertical rule */
	0xf8,	/* S_sw_mr:	meta-x, vertical rule */
	g_FILLER(S_sw_bl),
	0xf3,	/* S_sw_bc:	meta-s, low horizontal line */
	g_FILLER(S_sw_br),
	g_FILLER(S_explode1),
	0xef,	/* S_explode2:	meta-o, high horizontal line */
	g_FILLER(S_explode3),
	0xf8,	/* S_explode4:	meta-x, vertical rule */
	g_FILLER(S_explode5),
/*90*/	0xf8,	/* S_explode6:	meta-x, vertical rule */
	g_FILLER(S_explode7),
	0xf3,	/* S_explode8:	meta-s, low horizontal line */
	g_FILLER(S_explode9),
	g_FILLER(S_thinpcloud),
	g_FILLER(S_densepcloud),
};
#endif  /* TERMLIB */

#ifdef MAC_GRAPHICS_ENV
static uchar mac_graphics[MAXPCHARS] = {
/* 0*/	g_FILLER(S_stone),
	0xba,	/* S_vwall */
	0xcd,	/* S_hwall */
	0xc9,	/* S_tlcorn */
	0xbb,	/* S_trcorn */
	0xc8,	/* S_blcorn */
	0xbc,	/* S_brcorn */
	0xce,	/* S_crwall */
	0xca,	/* S_tuwall */
	0xcb,	/* S_tdwall */
/*10*/	0xb9,	/* S_tlwall */
	0xcc,	/* S_trwall */
	0xb0,	/* S_ndoor */
	0xee,	/* S_vodoor */
	0xee,	/* S_hodoor */
	0xef,	/* S_vcdoor */
	0xef,	/* S_hcdoor */
	0xf0,	/* S_bars:	equivalency symbol */
	0xf1,	/* S_tree:	plus-or-minus */
	0xf1,	/* S_deadtree:	plus-or-minus */
	g_FILLER(S_stone),
/*20*/	g_FILLER(S_Room),
	0xB0,	/* S_corr */
	g_FILLER(S_litcorr),
	0xe0,	/* S_pool */
	0xe0,	/* S_pool */
	g_FILLER(S_ice),
	g_FILLER(S_ice),
	g_FILLER(S_bog),
	g_FILLER(S_bog),
	g_FILLER(S_ground),
	g_FILLER(S_litground),
	g_FILLER(S_grass),
	g_FILLER(S_litgrass),
	g_FILLER(S_carpet),
	g_FILLER(S_litcarpet),
	g_FILLER(S_upstair),
	g_FILLER(S_dnstair),
	g_FILLER(S_upladder),
	g_FILLER(S_dnladder),
	g_FILLER(S_altar),
	0xef,	/* S_grave:	same as open door */
	g_FILLER(S_throne),
/*30*/	g_FILLER(S_sink),
	g_FILLER(S_fountain),
	g_FILLER(S_lava),
	g_FILLER(S_vodbridge),
	g_FILLER(S_hodbridge),
	g_FILLER(S_vcdbridge),
	g_FILLER(S_hcdbridge),
/*40*/	g_FILLER(S_air),
	g_FILLER(S_cloud),
	g_FILLER(S_water),
	g_FILLER(S_arrow_trap),
	g_FILLER(S_dart_trap),
	g_FILLER(S_falling_rock_trap),
	g_FILLER(S_squeaky_board),
	g_FILLER(S_bear_trap),
	g_FILLER(S_land_mine),
	g_FILLER(S_rolling_boulder_trap),
/*50*/	g_FILLER(S_sleeping_gas_trap),
	g_FILLER(S_rust_trap),
	g_FILLER(S_fire_trap),
	g_FILLER(S_pit),
	g_FILLER(S_spiked_pit),
	g_FILLER(S_hole),
	g_FILLER(S_trap_door),
	g_FILLER(S_teleportation_trap),
	g_FILLER(S_level_teleporter),
	g_FILLER(S_magic_portal),
/*60*/	g_FILLER(S_web),
	g_FILLER(S_statue_trap),
	g_FILLER(S_magic_trap),
	g_FILLER(S_anti_magic_trap),
	g_FILLER(S_polymorph_trap),
	g_FILLER(S_vbeam),
	g_FILLER(S_hbeam),
	g_FILLER(S_lslant),
	g_FILLER(S_rslant),
	g_FILLER(S_vbeaml),
	g_FILLER(S_vbeamr),
	g_FILLER(S_hbeamu),
	g_FILLER(S_hbeamd),
	g_FILLER(S_rbeam_tl),
	g_FILLER(S_rbeam_tc),
	g_FILLER(S_rbeam_tr),
	g_FILLER(S_rbeam_ml),
	g_FILLER(S_rbeam_mr),
	g_FILLER(S_rbeam_bl),
	g_FILLER(S_rbeam_bc),
	g_FILLER(S_rbeam_br),
	g_FILLER(S_digbeam),
/*70*/	g_FILLER(S_flashbeam),
	g_FILLER(S_boomleft),
	g_FILLER(S_boomright),
	g_FILLER(S_ss1),
	g_FILLER(S_ss2),
	g_FILLER(S_ss3),
	g_FILLER(S_ss4),
	g_FILLER(S_sw_tl),
	g_FILLER(S_sw_tc),
	g_FILLER(S_sw_tr),
/*80*/	g_FILLER(S_sw_ml),
	g_FILLER(S_sw_mr),
	g_FILLER(S_sw_bl),
	g_FILLER(S_sw_bc),
	g_FILLER(S_sw_br),
	g_FILLER(S_explode1),
	g_FILLER(S_explode2),
	g_FILLER(S_explode3),
	g_FILLER(S_explode4),
	g_FILLER(S_explode5),
/*90*/	g_FILLER(S_explode6),
	g_FILLER(S_explode7),
	g_FILLER(S_explode8),
	g_FILLER(S_explode9),
	g_FILLER(S_thinpcloud),
	g_FILLER(S_densepcloud),
};
#endif	/* MAC_GRAPHICS_ENV */

#ifdef PC9800
void NDECL((*ascgraphics_mode_callback)) = 0;	/* set in tty_start_screen() */
#endif

/*
 * Convert the given character to an object class.  If the character is not
 * recognized, then MAXOCLASSES is returned.  Used in detect.c invent.c,
 * options.c, pickup.c, sp_lev.c, and lev_main.c.
 */
int
def_char_to_objclass(ch)
    char ch;
{
    int i;
    for (i = 1; i < MAXOCLASSES; i++)
	if (ch == def_oc_syms[i]) break;
    return i;
}

/*
 * Convert a character into a monster class.  This returns the _first_
 * match made.  If there are are no matches, return MAXMCLASSES.
 */
int
def_char_to_monclass(ch)
    char ch;
{
    int i;
    for (i = 1; i < MAXMCLASSES; i++)
	if (def_monsyms[i] == ch) break;
    return i;
}

void
assign_graphics(graph_chars, glth, maxlen, offset)
register uchar *graph_chars;
int glth, maxlen, offset;
{
    register int i;

    for (i = 0; i < maxlen; i++)
	showsyms[i+offset] = (((i < glth) && graph_chars[i]) ?
		       graph_chars[i] : defsyms[i+offset].sym);
}

void
switch_graphics(gr_set_flag)
int gr_set_flag;
{
    switch (gr_set_flag) {
	default:
	case ASCII_GRAPHICS:
	    assign_graphics((uchar *)0, 0, MAXPCHARS, 0);
#ifdef PC9800
	    if (ascgraphics_mode_callback) (*ascgraphics_mode_callback)();
#endif
	    break;
#ifdef ASCIIGRAPH
	case IBM_GRAPHICS:
/*
 * Use the nice IBM Extended ASCII line-drawing characters (codepage 437).
 *
 * OS/2 defaults to a multilingual character set (codepage 850, corresponding
 * to the ISO 8859 character set.  We should probably do a VioSetCp() call to
 * set the codepage to 437.
 */
	    iflags.IBMgraphics = TRUE;
	    iflags.DECgraphics = FALSE;
	    assign_graphics(ibm_graphics, SIZE(ibm_graphics), MAXPCHARS, 0);
#ifdef PC9800
	    if (ibmgraphics_mode_callback) (*ibmgraphics_mode_callback)();
#endif
	    break;
#endif /* ASCIIGRAPH */
#ifdef TERMLIB
	case DEC_GRAPHICS:
/*
 * Use the VT100 line drawing character set.
 */
	    iflags.DECgraphics = TRUE;
	    iflags.IBMgraphics = FALSE;
	    assign_graphics(dec_graphics, SIZE(dec_graphics), MAXPCHARS, 0);
	    if (decgraphics_mode_callback) (*decgraphics_mode_callback)();
	    break;
#endif /* TERMLIB */
#ifdef MAC_GRAPHICS_ENV
	case MAC_GRAPHICS:
	    assign_graphics(mac_graphics, SIZE(mac_graphics), MAXPCHARS, 0);
	    break;
#endif
	}
    return;
}


#ifdef REINCARNATION

/*
 * saved display symbols for objects & features.
 */
static uchar save_oc_syms[MAXOCLASSES] = DUMMY;
static uchar save_showsyms[MAXPCHARS]  = DUMMY;
static uchar save_monsyms[MAXPCHARS]   = DUMMY;

static const uchar r_oc_syms[MAXOCLASSES] = {
/* 0*/	'\0',
	ILLOBJ_SYM,
	WEAPON_SYM,
	']',			/* armor */
	RING_SYM,
/* 5*/	',',			/* amulet */
	TOOL_SYM,
	':',			/* food */
	POTION_SYM,
	SCROLL_SYM,
/*10*/	SPBOOK_SYM,
	WAND_SYM,
	GEM_SYM,		/* gold -- yes it's the same as gems */
	GEM_SYM,
	ROCK_SYM,
/*15*/	BALL_SYM,
	CHAIN_SYM,
	VENOM_SYM
};

# ifdef ASCIIGRAPH
/* Rogue level graphics.  Under IBM graphics mode, use the symbols that were
 * used for Rogue on the IBM PC.  Unfortunately, this can't be completely
 * done because some of these are control characters--armor and rings under
 * DOS, and a whole bunch of them under Linux.  Use the TTY Rogue characters
 * for those cases.
 */
static const uchar IBM_r_oc_syms[MAXOCLASSES] = {	/* a la EPYX Rogue */
/* 0*/	'\0',
	ILLOBJ_SYM,
#  if defined(MSDOS) || defined(OS2) || ( defined(WIN32) && !defined(MSWIN_GRAPHICS) )
	0x18,			/* weapon: up arrow */
/*	0x0a, */ ARMOR_SYM,	/* armor:  Vert rect with o */
/*	0x09, */ RING_SYM,	/* ring:   circle with arrow */
/* 5*/	0x0c,			/* amulet: "female" symbol */
	TOOL_SYM,
	0x05,			/* food:   club (as in cards) */
	0xad,			/* potion: upside down '!' */
	0x0e,			/* scroll: musical note */
/*10*/	SPBOOK_SYM,
	0xe7,			/* wand:   greek tau */
	0x0f,			/* gold:   yes it's the same as gems */
	0x0f,			/* gems:   fancy '*' */
#  else
	')',			/* weapon  */
	ARMOR_SYM,		/* armor */
	RING_SYM,		/* ring */
/* 5*/	',',			/* amulet  */
	TOOL_SYM,
	':',			/* food    */
	0xad,			/* potion: upside down '!' */
	SCROLL_SYM,		/* scroll  */
/*10*/	SPBOOK_SYM,
	0xe7,			/* wand:   greek tau */
	GEM_SYM,		/* gold:   yes it's the same as gems */
	GEM_SYM,		/* gems    */
#  endif
	ROCK_SYM,
/*15*/	BALL_SYM,
	CHAIN_SYM,
	VENOM_SYM
};
# endif /* ASCIIGRAPH */

void
assign_rogue_graphics(is_rlevel)
boolean is_rlevel;
{
    /* Adjust graphics display characters on Rogue levels */

    if (is_rlevel) {
	register int i;

	(void) memcpy((genericptr_t)save_showsyms,
		      (genericptr_t)showsyms, sizeof showsyms);
	(void) memcpy((genericptr_t)save_oc_syms,
		      (genericptr_t)oc_syms, sizeof oc_syms);
	(void) memcpy((genericptr_t)save_monsyms,
		      (genericptr_t)monsyms, sizeof monsyms);

	/* Use a loop: char != uchar on some machines. */
	for (i = 0; i < MAXMCLASSES; i++)
	    monsyms[i] = def_monsyms[i];
# if defined(ASCIIGRAPH) && !defined(MSWIN_GRAPHICS)
	if (iflags.IBMgraphics && iflags.IBMrogue
#  if defined(USE_TILES) && defined(MSDOS)
		&& !iflags.grmode
#  endif
		)
	    monsyms[S_HUMAN] = 0x01; /* smiley face */
# endif
	for (i = 0; i < MAXPCHARS; i++)
	    showsyms[i] = defsyms[i].sym;

/*
 * Some day if these rogue showsyms get much more extensive than this,
 * we may want to create r_showsyms, and IBM_r_showsyms arrays to hold
 * all of this info and to simply initialize it via a for() loop like r_oc_syms.
 */

# ifdef ASCIIGRAPH
	if (!iflags.IBMgraphics
#  if defined(USE_TILES) && defined(MSDOS)
		|| iflags.grmode
#  endif
				) {
# endif
	    showsyms[S_vodoor]  = showsyms[S_hodoor]  = showsyms[S_ndoor] = '+';
	    showsyms[S_upstair] = showsyms[S_dnstair] = '%';
# ifdef ASCIIGRAPH
	} else {
	  if (!iflags.IBMrogue) {
	    for (i = 0; i < sizeof showsyms; i++)
		showsyms[i] = defsyms[i].sym;
	    showsyms[S_vodoor]  = showsyms[S_hodoor]  = showsyms[S_ndoor] = '+';
	    showsyms[S_upstair] = showsyms[S_dnstair] = '%';
	  } else {
	    /* a la EPYX Rogue */
	    showsyms[S_vwall]   = 0xba; /* all walls now use	*/
	    showsyms[S_hwall]   = 0xcd; /* double line graphics	*/
	    showsyms[S_tlcorn]  = 0xc9;
	    showsyms[S_trcorn]  = 0xbb;
	    showsyms[S_blcorn]  = 0xc8;
	    showsyms[S_brcorn]  = 0xbc;
	    showsyms[S_crwall]  = 0xce;
	    showsyms[S_tuwall]  = 0xca;
	    showsyms[S_tdwall]  = 0xcb;
	    showsyms[S_tlwall]  = 0xb9;
	    showsyms[S_trwall]  = 0xcc;
	    showsyms[S_ndoor]   = 0xce;
	    showsyms[S_vodoor]  = 0xce;
	    showsyms[S_hodoor]  = 0xce;
	    showsyms[S_room]    = 0xfa; /* centered dot */
	    showsyms[S_corr]    = 0xb1;
	    showsyms[S_litcorr] = 0xb2;
	    showsyms[S_upstair] = 0xf0; /* Greek Xi */
	    showsyms[S_dnstair] = 0xf0;
#ifndef MSWIN_GRAPHICS
	    showsyms[S_arrow_trap] = 0x04; /* diamond (cards) */
	    showsyms[S_dart_trap] = 0x04;
	    showsyms[S_falling_rock_trap] = 0x04;
	    showsyms[S_squeaky_board] = 0x04;
	    showsyms[S_bear_trap] = 0x04;
	    showsyms[S_land_mine] = 0x04;
	    showsyms[S_rolling_boulder_trap] = 0x04;
	    showsyms[S_sleeping_gas_trap] = 0x04;
	    showsyms[S_rust_trap] = 0x04;
	    showsyms[S_fire_trap] = 0x04;
	    showsyms[S_pit] = 0x04;
	    showsyms[S_spiked_pit] = 0x04;
	    showsyms[S_hole] = 0x04;
	    showsyms[S_trap_door] = 0x04;
	    showsyms[S_teleportation_trap] = 0x04;
	    showsyms[S_level_teleporter] = 0x04;
	    showsyms[S_magic_portal] = 0x04;
	    showsyms[S_web] = 0x04;
	    showsyms[S_statue_trap] = 0x04;
	    showsyms[S_magic_trap] = 0x04;
	    showsyms[S_anti_magic_trap] = 0x04;
	    showsyms[S_polymorph_trap] = 0x04;
	  }
#endif
	}
#endif /* ASCIIGRAPH */

	for (i = 0; i < MAXOCLASSES; i++) {
#ifdef ASCIIGRAPH
	    if (iflags.IBMgraphics && iflags.IBMrogue
#  if defined(USE_TILES) && defined(MSDOS)
		&& !iflags.grmode
#  endif
		)
		oc_syms[i] = IBM_r_oc_syms[i];
	    else
#endif /* ASCIIGRAPH */
		oc_syms[i] = r_oc_syms[i];
	}
#if defined(MSDOS) && defined(USE_TILES)
	if (iflags.grmode) tileview(FALSE);
#endif
    } else {
	(void) memcpy((genericptr_t)showsyms,
		      (genericptr_t)save_showsyms, sizeof showsyms);
	(void) memcpy((genericptr_t)oc_syms,
		      (genericptr_t)save_oc_syms, sizeof oc_syms);
	(void) memcpy((genericptr_t)monsyms,
		      (genericptr_t)save_monsyms, sizeof monsyms);
#if defined(MSDOS) && defined(USE_TILES)
	if (iflags.grmode) tileview(TRUE);
#endif
    }
}
#endif /* REINCARNATION */

/*drawing.c*/
