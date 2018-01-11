/*	SCCS Id: @(#)role.c	3.4	2003/01/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"


/*** Table of all roles ***/
/* According to AD&D, HD for some classes (ex. Wizard) should be smaller
 * (4-sided for wizards).  But this is not AD&D, and using the AD&D
 * rule here produces an unplayable character.  Thus I have used a minimum
 * of an 10-sided hit die for everything.  Another AD&D change: wizards get
 * a minimum strength of 4 since without one you can't teleport or cast
 * spells. --KAA
 *
 * As the wizard has been updated (wizard patch 5 jun '96) their HD can be
 * brought closer into line with AD&D. This forces wizards to use magic more
 * and distance themselves from their attackers. --LSZ
 *
 * With the introduction of races, some hit points and energy
 * has been reallocated for each race.  The values assigned
 * to the roles has been reduced by the amount allocated to
 * humans.  --KMH
 *
 * God names use a leading underscore to flag goddesses.
 */
const struct Role roles[] = {
{	{E_J("Archeologist",	"çlå√äwé“"	), 0}, {
	{E_J("Digger",		"åäå@ÇË"	), 0},
	{E_J("Field Worker",	"é¿íní≤ç∏àı"	), 0},
	{E_J("Investigator",	"å§ãÜé“"	), 0},
	{E_J("Exhumer",		"ï≠ïÊí≤ç∏àı"	), 0},
	{E_J("Excavator",	"à‚ê’î≠å@â∆"	), 0},
	{E_J("Spelunker",	"íTåüâ∆"	), 0},
	{E_J("Speleologist",	"ì¥åAäwé“"	), 0},
	{E_J("Collector",	"é˚èWâ∆"	), 0},
	{E_J("Curator",		"îéï®äŸí∑"	), 0} },
#ifndef JP
	"Quetzalcoatl", "Camaxtli", "Huhetotl", /* Central American */
	"Arc", "the College of Archeology", "the Tomb of the Toltec Kings",
#else
	"ÉPÉcÉ@ÉãÉRÉAÉgÉã", "ÉJÉ}ÉLÉVÉgÉä", "ÉtÉtÉFÉeÉHÉgÉã",
	"Arc", "çlå√äwëÂäw", "ÉgÉãÉeÉJâ§â∆ÇÃï≠ïÊ",
#endif /*JP*/
	PM_ARCHEOLOGIST, NON_PM, NON_PM,
	PM_LORD_CARNARVON, PM_STUDENT, PM_MINION_OF_HUHETOTL,
	NON_PM, PM_HUMAN_MUMMY, S_SNAKE, S_MUMMY,
	ART_ORB_OF_DETECTION,
	MH_HUMAN|MH_DWARF|MH_GNOME | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   6, 14, 10, 10,  6, 14 },	/* {  7, 10, 10,  7,  7,  7 } */
	{  10, 25, 10, 20, 10, 25 },	/* { 20, 20, 20, 10, 20, 10 } */
	{  16, 19, 18, 19, 17, 19 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 11, 0,  0, 4,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	10, 5, 0, 2, 10, A_INT, SPE_MAGIC_MAPPING,   -4
},
{	{E_J("Barbarian",	"ñÏîÿêl"	), 0}, {
	{E_J("Plunderer",	"ìêêl"		), E_J("Plunderess", 0) },
	{E_J("Pillager",	"ó™íDÇ∑ÇÈé“"	), 0},
	{E_J("Bandit",		"ñÏìê"		), 0},
	{E_J("Brigand",		"ïêëïã≠ìê"	), 0},
	{E_J("Raider",		"èPåÇÇ∑ÇÈé“"	), 0},
	{E_J("Reaver",		"ã≠íDÇ∑ÇÈé“"	), 0},
	{E_J("Slayer",		"éEùCÇ∑ÇÈé“"	), 0},
	{E_J("Chieftain",	"éÒóÃ"		), E_J("Chieftainess","èóéÒóÃ")},
	{E_J("Conqueror",	"ê™ïûâ§"	), E_J("Conqueress",0)} },
#ifndef JP
	"Mitra", "Crom", "Set", /* Hyborian */
	"Bar", "the Camp of the Duali Tribe", "the Duali Oasis",
#else
	"É~ÉgÉâ", "ÉNÉçÉÄ", "ÉZÉg", /* Hyborian */
	"Bar", "ÉfÉÖÉAÉäë∞ÇÃÉLÉÉÉìÉv", "ÉfÉÖÉAÉäÅEÉIÉAÉVÉX",
#endif /*JP*/
	PM_BARBARIAN, NON_PM, NON_PM,
	PM_PELIAS, PM_CHIEFTAIN, PM_THOTH_AMON,
	PM_OGRE, PM_TROLL, S_OGRE, S_TROLL,
	ART_HEART_OF_AHRIMAN,
	MH_HUMAN|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  16,  4,  4, 16, 16,  4 },	/* {  16,  7,  7, 15, 16,  6 } */
	{  30,  6,  7, 20, 30,  7 },
	{ 118, 12, 16, 18, 20, 10 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0,10,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	10, 14, 0, 0,  8, A_INT, SPE_HASTE_SELF,      -4
},
{	{E_J("Caveman",		"ì¥åAêl"	), E_J("Cavewoman",0)}, {
	{E_J("Troglodyte",	"åäãèêl"	), 0},
	{E_J("Aborigine",	"å¥èZñØ"	), 0},
	{E_J("Wanderer",	"ï˙òQé“"	), 0},
	{E_J("Vagrant",		"ïÇòQé“"	), 0},
	{E_J("Wayfarer",	"ó∑êl"		), 0},
	{E_J("Roamer",		"ó¨òQêl"	), 0},
	{E_J("Nomad",		"óVñqñØ"	), 0},
	{E_J("Rover",		"óñKé“"	), 0},
	{E_J("Pioneer",		"êÊãÏé“"	), 0} },
#ifndef JP
	"Anu", "_Ishtar", "Anshar", /* Babylonian */
	"Cav", "the Caves of the Ancestors", "the Dragon's Lair",
#else
	"ÉAÉk", "_ÉCÉVÉÖÉ^Éã", "ÉAÉìÉVÉÉÉã", /* Babylonian */
	"Cav", "çÇëcÇÃì¥åA", "ó≥ÇÃê±Ç›èà",
#endif /*JP*/
	PM_CAVEMAN, PM_CAVEWOMAN, PM_LITTLE_DOG,
	PM_SHAMAN_KARNOV, PM_NEANDERTHAL, PM_CHROMATIC_DRAGON,
	PM_BUGBEAR, PM_HILL_GIANT, S_HUMANOID, S_GIANT,
	ART_SCEPTRE_OF_MIGHT,
	MH_HUMAN|MH_DWARF|MH_GNOME | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  20,  6,  6,  8, 16,  4 },	/* {  10,  7,  7,  7,  8,  6 } */
	{  30,  6,  7, 20, 30,  7 },
	{ 118, 14, 16, 18, 20, 15 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 12, 0, 1,  8, A_INT, SPE_DIG,             -4
},
{	{E_J("Healer",		"ñ¸ÇµéË"	), 0}, {
	{E_J("Rhizotomist",	"ñÚëêçÃÇË"	), 0}, /* rhizotomist ñÚå¯ÇÃÇΩÇﬂÇ…êAï®ÇÃç™ÇèWÇﬂÇÈêl */
	{E_J("Empiric",		"à„äwé¿èKê∂"	), 0}, /* empiric óùò_ÇÊÇËÇ‡åoå±ÅEé¿å±ÇèdÇÒÇ∂ÇÈêl */
	{E_J("Embalmer",	"à‚ëÃêÆïúét"	), 0},
	{E_J("Dresser",		"äOâ»èïéË"	), 0}, /* dresser (Br)äOâ»èïéËÅAïÔë—åW */
	{E_J("Medicus ossium",	"ê⁄çúà„"	), E_J("Medica ossium",0)}, /* medicus (Latin)à„é“, ossium (Latin)çú */
	{E_J("Herbalist",	"ñÚëêäwé“"	), 0},
	{E_J("Magister",	"à„äwîéém"	), E_J("Magistra",0) }, /* magister (Latin)êÊê∂ÅAäwçZí∑ */
	{E_J("Physician",	"ñÚét"		), 0}, /* physician ì‡â»à„ */
	{E_J("Chirurgeon",	"ìTà„"		), 0} }, /* chirurgeon äOâ»à„ surgeon ÇÃå√åÍ */
#ifndef JP
	"_Athena", "Hermes", "Poseidon", /* Greek */
	"Hea", "the Temple of Epidaurus", "the Temple of Coeus",
#else
	"_ÉAÉeÉi", "ÉwÉãÉÅÉX", "É|ÉZÉCÉhÉì", /* Greek */
	"Hea", "ÉGÉsÉ_ÉEÉçÉXéõâ@", "ÉRÉCÉIÉXéõâ@",
#endif /*JP*/
	PM_HEALER, NON_PM, NON_PM,
	PM_HIPPOCRATES, PM_ATTENDANT, PM_CYCLOPS,
	PM_GIANT_RAT, PM_SNAKE, S_RODENT, S_YETI,
	ART_STAFF_OF_AESCULAPIUS,
	MH_HUMAN|MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   4, 10, 14,  9,  9, 14 },	/* {  7,  7, 13,  7, 11, 16 } */
	{  10, 20, 20, 15, 20, 20 },	/* { 15, 20, 20, 15, 25,  5 } */
	{  12, 19, 19, 19, 19, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 11, 0,  0, 4,  1, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },20,	/* Energy */
	10, 3,-3, 2, 10, A_WIS, SPE_CURE_SICKNESS,   -4
},
{	{E_J("Knight",		"ãRém"		), 0}, {
	{E_J("Gallant",		"è¨ê©"		), 0}, /* óÁãVê≥ÇµÇ¢ÅEóEä∏Ç»é·Ç¢íjÅcñÛÇπÇ»Ç¢ÇÃÇ≈pageàµÇ¢Ç≈ */
	{E_J("Esquire",		"è]ém"		), 0}, /* ãRémÇÃâ∫Ç≈ê¢òbÇÇ∑ÇÈé“ */
	{E_J("Bachelor",	"ãRï∫"		), 0}, /* â∫à ÇÃãRém ìùé°é“Ç©ÇÁãRémÇÃà ÇìæÇΩÇ™ÅAãRémícÇ…ÇÕñ¢èäëÆ */
	{E_J("Sergeant",	"ãRï∫í∑"	), 0}, /* ÇÌÇ©ÇÁÇ»Ç¢ÇÃÇ≈ìKìñÇ…Åc */
	{E_J("Knight",		"ãRém"		), 0}, /* (BachelorÇÃÇ∆Ç´Ç…èñîCÇ≥ÇÍÇΩÇÕÇ∏Ç»ÇÃÇæÇ™Åc) */
	{E_J("Banneret",	"ãRémící∑"	), 0}, /* éläpÇ¢ä¯ÇéùÇ¡Çƒé©ï™ÇÃãRémícÇó¶Ç¢ÇÈínà ÇÃãRém */
	{E_J("Chevalier",	"èdãRém"	), E_J("Chevaliere",0)}, /* ÉtÉâÉìÉXåÍÇÃãRém ÉVÉÖÉoÉäÉG */
	{E_J("Seignieur",	"éÍåMãRém"	), E_J("Dame",0)}, /* ïïåöóÃéÂ Sir,DameÇÕãRémÇÃèÃçÜ */
	{E_J("Paladin",		"êπãRém"	), 0} },
#ifndef JP
	"Lugh", "_Brigit", "Manannan Mac Lir", /* Celtic */
	"Kni", "Camelot Castle", "the Isle of Glass",
#else
	"ÉãÅ[Ét", "_ÉuÉäÉWÉbÉg", "É}ÉiÉiÉìÅEÉ}ÉNÉäÅ[Éã", /* Celtic */
	"Kni", "ÉLÉÉÉÅÉçÉbÉgèÈ", "ÉKÉâÉXÇÃìá",
#endif /*JP*/
	PM_KNIGHT, NON_PM, PM_PONY,
	PM_KING_ARTHUR, PM_PAGE, PM_IXOTH,
	PM_QUASIT, PM_OCHRE_JELLY, S_IMP, S_JELLY,
	ART_MAGIC_MIRROR_OF_MERLIN,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  16,  6,  8,  6, 10, 14 },	/* { 13,  7, 14,  8, 10, 17 } */
	{  25, 10, 15, 10, 20, 20 },	/* { 30, 15, 15, 10, 20, 10 } */
	{  68, 17, 19, 13, 18, 19 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },10,	/* Energy */
	10, 8,-2, 0,  9, A_WIS, SPE_TURN_UNDEAD,     -4
},
{	{E_J("Monk",		"èCçsëm"	), 0}, {
	{E_J("Candidate",	"ñÂÇí@Ç≠é“"	), 0},
	{E_J("Novice",		"èâêSé“"	), 0},
	{E_J("Initiate",	"ì¸ñÂé“"	), 0},
	{E_J("Student of Stones","ìyÇÃèKÇ¢éË"	), 0},
	{E_J("Student of Waters","êÖÇÃèKÇ¢éË"	), 0},
	{E_J("Student of Metals","ã‡ÇÃèKÇ¢éË"	), 0},
	{E_J("Student of Winds","ñÿÇÃèKÇ¢éË"	), 0},
	{E_J("Student of Fire",	"âŒÇÃèKÇ¢éË"	), 0},
	{E_J("Master",		"ñ∆ãñäFì`"	), 0} },
#ifndef JP
	"Shan Lai Ching", "Chih Sung-tzu", "Huan Ti", /* Chinese */
	"Mon", "the Monastery of Chan-Sune",
	  "the Monastery of the Earth-Lord",
#else
	"‚ZíÈ", "ê‘èºéq", "â©íÈ", /* Chinese */
	"Mon", "É`ÉÉÉìÅEÉXÅ[èCìπâ@", "ínâ§ÇÃèCìπâ@",
#endif /*JP*/
	PM_MONK, NON_PM, NON_PM,
	PM_GRAND_MASTER, PM_ABBOT, PM_MASTER_KAEN,
	PM_EARTH_ELEMENTAL, PM_XORN, S_ELEMENTAL, S_XORN,
	ART_EYES_OF_THE_OVERWORLD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   6, 10, 16, 16, 14,  8 },	/* { 10,  7,  8,  8,  7,  7 } */
	{  10, 15, 25, 20, 20, 10 },	/* { 25, 10, 20, 20, 15, 10 } */
	{  10, 18, 20, 20, 20, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 12, 0,  0, 6,  1, 0 },	/* Hit points */
	{  2, 2,  0, 2,  0, 2 },10,	/* Energy */
	10, 8,-2, 2, 20, A_WIS, SPE_RESTORE_ABILITY, -4
},
/*	ÉçÅ[É}ÅEÉJÉgÉäÉbÉNã≥âÔÇÃäKëwëgêD
	ã≥çc > êïã@ã® > ëÂéiã≥ > éiã≥ > éiç’ > èïç’
 */
{	{E_J("Priest",		"ëmóµ"		), E_J("Priestess","ìÚëm")}, {
	{E_J("Aspirant",	"èCìπém"	), E_J(0,"èCìπèó")},
	{E_J("Acolyte",		"éòé“"		), 0}, /* éiç’ÇÃéËì`Ç¢ */
	{E_J("Adept",		"éòç’"		), 0},
	{E_J("Priest",		"ëmóµ"		), E_J("Priestess","ìÚëm")},
	{E_J("Curate",		"èïç’"		), 0},
	{E_J("Canon",		"êπé“"		), E_J("Canoness","êπèó")},
	{E_J("Lama",		"éiã≥"		), 0},
	{E_J("Patriarch",	"ëÂéiã≥"	), E_J("Matriarch",0)},
	{E_J("High Priest",	"ëÂëmê≥"	), E_J("High Priestess","èóã≥çc")} },
	0, 0, 0,	/* chosen randomly from among the other roles */
#ifndef JP
	"Pri", "the Great Temple", "the Temple of Nalzok",
#else
	"Pri", "àÃëÂÇ»ÇÈéõâ@", "ÉiÉãÉ]ÉNÇÃéõâ@",
#endif /*JP*/
	PM_PRIEST, PM_PRIESTESS, NON_PM,
	PM_ARCH_PRIEST, PM_ACOLYTE, PM_NALZOK,
	PM_HUMAN_ZOMBIE, PM_WRAITH, S_ZOMBIE, S_WRAITH,
	ART_MITRE_OF_HOLINESS,
	MH_HUMAN|MH_ELF | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  10, 10, 16,  7, 10,  7 },	/* {  7,  7, 10,  7,  7,  7 } */
	{  15, 15, 25, 10, 25, 10 },	/* { 15, 10, 30, 15, 20, 10 } */
	{  18, 17, 20, 15, 18, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 12, 0,  0, 6,  1, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 2 },10,	/* Energy */
	0, 3,-2, 2, 10, A_WIS, SPE_REMOVE_CURSE,    -4
},
  /* Note:  Rogue precedes Ranger so that use of `-R' on the command line
     retains its traditional meaning. */
{	{E_J("Rogue",		"ìêëØ"), 0}, {
	{E_J("Footpad",		"í«Ç¢îçÇ¨"),     0},
	{E_J("Cutpurse",	"ÉXÉä"),    0},
	{E_J("Rogue",		"ìêëØ"),       0},
	{E_J("Pilferer",	"Ç±ÇªìD"),    0},
	{E_J("Robber",		"ã≠ìê"),      0},
	{E_J("Burglar",		"ãÛëÉ"),     0},
	{E_J("Filcher",		"ñúà¯Ç´î∆"),     0},
	{E_J("Magsman",		"çºã\ét"),     E_J("Magswoman", 0) },
	{E_J("Thief",		"ëÂìDñ_"),       0} },
#ifndef JP
	"Issek", "Mog", "Kos", /* Nehwon */
	"Rog", "the Thieves' Guild Hall", "the Assassins' Guild Hall",
#else
	"ÉCÉZÉN", "ÉÇÉO", "ÉRÉX", /* Nehwon */
	"Rog", "ìêëØÉMÉãÉhÇÃÉAÉWÉg", "à√éEé“ÉMÉãÉhÇÃÉAÉWÉg",
#endif /*JP*/
	PM_ROGUE, NON_PM, NON_PM,
	PM_MASTER_OF_THIEVES, PM_THUG, PM_MASTER_ASSASSIN,
	PM_LEPRECHAUN, PM_GUARDIAN_NAGA, S_NYMPH, S_NAGA,
	ART_MASTER_KEY_OF_THIEVERY,
	MH_HUMAN|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  12, 12,  7, 16,  7,  6 },	/* {  7,  7,  7, 10,  7,  6 } */
	{  25, 20, 10, 25, 10, 10 },	/* { 20, 10, 10, 30, 20, 10 } */
	{  18, 17, 17, 20, 17, 16 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 10, 0,  0, 6,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 8, 0, 1,  9, A_INT, SPE_DETECT_TREASURE, -4
},
{	{E_J("Ranger","ÉåÉìÉWÉÉÅ["), 0}, {
#if 0	/* OBSOLETE */
	{"Edhel",       "Elleth"},
	{"Edhel",       "Elleth"},      /* elf-maid */
	{"Ohtar",       "Ohtie"},       /* warrior */
	{"Kano",			/* commander (Q.) ['a] */
			"Kanie"},	/* educated guess, until further research- SAC */
	{"Arandur",			/* king's servant, minister (Q.) - guess */
			"Aranduriel"},	/* educated guess */
	{"Hir",         "Hiril"},       /* lord, lady (S.) ['ir] */
	{"Aredhel",     "Arwen"},       /* noble elf, maiden (S.) */
	{"Ernil",       "Elentariel"},  /* prince (S.), elf-maiden (Q.) */
	{"Elentar",     "Elentari"},	/* Star-king, -queen (Q.) */
	"Solonor Thelandira", "Aerdrie Faenya", "Lolth", /* Elven */
#endif
	{E_J("Tenderfoot",	"êVïƒ"),    0},
	{E_J("Lookout",		"å©í£ÇË"),       0},
	{E_J("Trailblazer",	"êÊì±é“"),   0},
	{E_J("Reconnoiterer",	"í„é@"), E_J("Reconnoiteress",0)},
	{E_J("Scout",		"êÀåÛ"),         0},
	{E_J("Arbalester",	"éÀéË"),    0},	/* One skilled at crossbows */
	{E_J("Archer",		"ã|ï∫"),        0},
	{E_J("Sharpshooter",	"ã≠éÀéË"),  0},
	{E_J("Marksman",	"ë_åÇéË"),      E_J("Markswoman",0)} },
#ifndef JP
	"Mercury", "_Venus", "Mars", /* Roman/planets */
	"Ran", "Orion's camp", "the cave of the wumpus",
#else
	"É}Å[ÉLÉÖÉäÅ[", "_ÉîÉBÅ[ÉiÉX", "É}Å[ÉY",
	"Ran", "ÉIÉäÉIÉìÇÃÉLÉÉÉìÉv", "ÉèÉìÉpÉXÇÃì¥åA",
#endif /*JP*/
	PM_RANGER, NON_PM, PM_LITTLE_DOG /* Orion & canis major */,
	PM_ORION, PM_HUNTER, PM_SCORPIUS,
	PM_FOREST_CENTAUR, PM_SCORPION, S_CENTAUR, S_SPIDER,
	ART_LONGBOW_OF_DIANA,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7, 12,  7, 16,  6, 12 },	/* { 13, 13, 13,  9, 13,  7 } */
	{  10, 20, 10, 30, 10, 20 },	/* { 30, 10, 10, 20, 20, 10 } */
	{  17, 19, 16, 21, 13, 19 },
	/* Init   Lower  Higher */
	{ 13, 0,  0, 6,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },12,	/* Energy */
	10, 9, 2, 1, 10, A_INT, SPE_INVISIBILITY,   -4
},
{	{E_J("Samurai",		"éò"	), 0}, {
/*	{"Hatamoto",      0},*/  /* Banner Knight */
/*	{"Kuge",          0},*/  /* Noble of the Court */
/*	{"Ninja","Kunoichi"},*/  /* secret society */
	{E_J("Ronin",		"òQêl"	), 0},  /* no allegiance */
	{E_J("Ashigaru",	"ë´åy"	), 0},  /* footman */
	{E_J("Kumigashira",	"éòëgì™"), 0},  /* leader of Ashigaru */
	{E_J("Bushou",		"ïêè´"	), 0},  /* general */
	{E_J("Ryoshu",		"óÃéÂ"	), 0},  /* has a territory */
	{E_J("Joshu",		"èÈéÂ"	), 0},  /* heads a castle */
	{E_J("Kokushu",		"çëéÂ"	), 0},  /* heads a province */
	{E_J("Daimyo",		"ëÂñº"	), 0},  /* a samurai lord */
	{E_J("Shogun",		"è´åR"	), 0} },/* supreme commander, warlord */
#ifndef JP
	"_Amaterasu Omikami", "Raijin", "Susanowo", /* Japanese */
	"Sam", "the Castle of the Taro Clan", "the Shogun's Castle",
#else
	"_ìVè∆ëÂê_", "óãê_", "ê{ç≤îVíj", /* Japanese */
	"Sam", "ëæòYàÍë∞ÇÃèÈ", "è´åRÇÃèÈ",
#endif /*JP*/
	PM_SAMURAI, NON_PM, PM_LITTLE_DOG,
	PM_LORD_SATO, PM_ROSHI, PM_ASHIKAGA_TAKAUJI,
	PM_WOLF, PM_STALKER, S_DOG, S_ELEMENTAL,
	ART_TSURUGI_OF_MURAMASA,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  12,  5,  5, 16, 16,  6 },	/* { 10,  8,  7, 10, 17,  6 } */
	{  25, 10, 10, 20, 25, 10 },	/* { 30, 10,  8, 30, 14,  8 } */
	{  68, 15, 15, 20, 19, 15 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 13, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 10, 0, 0,  8, A_INT, SPE_CLAIRVOYANCE,    -4
},
#ifdef TOURIST
{	{E_J("Tourist",		"äœåıãq"	), 0}, {
	{E_J("Rambler",		"éUï‡é“"	), 0},
	{E_J("Sightseer",	"äœåıãq"	), 0},
	{E_J("Excursionist",	"é¸óVãq"	), 0},
	{E_J("Peregrinator",	"óñKé“"),E_J("Peregrinatrix",0)},
	{E_J("Traveler",	"ó∑çsé“"	), 0},
	{E_J("Journeyer",	"ó∑êl"		), 0},
	{E_J("Voyager",		"çqäCé“"	), 0},
	{E_J("Explorer",	"íTåüâ∆"	), 0},
	{E_J("Adventurer",	"ñ`åØé“"	), 0} },
#ifndef JP
	"Blind Io", "_The Lady", "Offler", /* Discworld */
	"Tou", "Ankh-Morpork", "the Thieves' Guild Hall",
#else
	"ñ”ñ⁄ÇÃÉCÉI", "_Åsèóê_Åt", "ÉIÉtÉâÅ[", /* Discworld */
	"Tou", "ÉAÉìÉN=ÉÇÉãÉ|Å[ÉN", "ìêëØÉMÉãÉhÇÃÉAÉWÉg",
#endif /*JP*/
	PM_TOURIST, NON_PM, NON_PM,
	PM_TWOFLOWER, PM_GUIDE, PM_MASTER_OF_THIEVES,
	PM_GIANT_SPIDER, PM_FOREST_CENTAUR, S_SPIDER, S_CENTAUR,
	ART_YENDORIAN_EXPRESS_CARD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   8, 12,  6,  6, 10, 15 },	/* {  7, 10,  6,  7,  7, 10 } */
	{  20, 20, 10, 10, 20, 20 },	/* { 15, 10, 10, 15, 30, 20 } */
	{  17, 19, 19, 15, 18, 19 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{  8, 0,  0, 4,  0, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	0, 5, 1, 2, 10, A_INT, SPE_CHARM_MONSTER,   -4
},
#endif
{	{E_J("Valkyrie",	"ÉèÉãÉLÉÖÅ[Éå"	), 0}, {
	{E_J("Stripling",	"êVï∫"), 0},
	{E_J("Skirmisher",	"êÀåÛï∫"), 0},
	{E_J("Fighter",		"êÌém"), 0},
	{E_J("Man-at-arms",	"èdëïï∫"),		E_J("Woman-at-arms",	"åïÇÃâ≥èó") },
	{E_J("Warrior",		"êÌì¨ï∫"),		E_J(0,			"êÌâ≥èó") },
	{E_J("Swashbuckler",	"åïém"),		E_J(0,			"èÇéùÇ¬â≥èó") },
	{E_J("Hero",		"âpóY"),		E_J("Heroine",		0)},
	{E_J("Champion",	"ã≠é“"),		E_J(0,			"èóåÜ") },
	{E_J("Lord",		"ãMêl"),		E_J("Lady",		"ãMïwêl")} },
#ifndef JP
	"Tyr", "Odin", "Loki", /* Norse */
	"Val", "the Shrine of Destiny", "the cave of Surtur",
#else
	"ÉeÉÖÅ[Éã", "ÉIÅ[ÉfÉBÉì", "ÉçÉL", /* Norse */
	"Val", "â^ñΩÇÃêπì∞", "ÉXÉãÉgÇÃì¥åA",
#endif /*JP*/
	PM_VALKYRIE, NON_PM, NON_PM /*PM_WINTER_WOLF_CUB*/,
	PM_NORN, PM_WARRIOR, PM_LORD_SURTUR,
	PM_FIRE_ANT, PM_FIRE_GIANT, S_ANT, S_GIANT,
	ART_ORB_OF_FATE,
	MH_HUMAN|MH_DWARF | ROLE_FEMALE | ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  16,  6, 10,  6, 16,  6 },	/* { 10,  7,  7,  7, 10,  7 } */
	{  30, 10, 10, 10, 30, 10 },	/* { 30,  6,  7, 20, 30,  7 } */
	{ 118, 12, 17, 16, 20, 16 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 10,-2, 0,  9, A_WIS, SPE_CONE_OF_COLD,    -4
},
{	{E_J("Wizard",		"ñÇñ@égÇ¢"	), 0}, {
	{E_J("Evoker",		"éËïiét"), 0},
	{E_J("Conjurer",	"äÔèpét"), 0},
	{E_J("Thaumaturge",	"éÙèpét"), 0},
	{E_J("Magician",	"ñÇèpét"), 0},
	{E_J("Enchanter",	"êSèpét"), E_J("Enchantress",0)},
	{E_J("Sorcerer",	"ñÇìπét"), E_J("Sorceress",0)},
	{E_J("Necromancer",	"ódèpét"), 0},
	{E_J("Wizard",		"ñÇñ@égÇ¢"), 0},
	{E_J("Mage",		"ëÂñÇèpét"), 0} },
#ifndef JP
	"Ptah", "Thoth", "Anhur", /* Egyptian */
	"Wiz", "the Lonely Tower", "the Tower of Darkness",
#else
	"ÉvÉ^Én", "ÉgÅ[Ég", "ÉAÉìÉtÉã", /* Egyptian */
	"Wiz", "âBÇ≥ÇÍÇµìÉ", "à√çïÇÃìÉ",
#endif /*JP*/
	PM_WIZARD, NON_PM, PM_KITTEN,
	PM_NEFERET_THE_GREEN, PM_APPRENTICE, PM_DARK_ONE,
	PM_VAMPIRE_BAT, PM_XORN, S_BAT, S_WRAITH,
	ART_EYE_OF_THE_AETHIOPICA,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   6, 16, 14, 12,  6,  6 },	/* {  7, 10,  7,  7,  7,  7 } */
	{  10, 25, 20, 25, 10, 10 },	/* { 10, 30, 10, 20, 20, 10 } */
	{  10, 20, 20, 19, 16, 18 },	/* Maximum ability */
	/* Init   Lower  Higher */
	{ 10, 0,  0, 2,  0, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 3 },12,	/* Energy */
	0, 1, 0, 3, 10, A_INT, SPE_MAGIC_MISSILE,   -4
},
/* Array terminator */
{{0, 0}}
};


/* The player's role, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Role urole =
{	{"Undefined", 0}, { {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	"L", "N", "C", "Xxx", "home", "locate",
	NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM,
	NON_PM, NON_PM, 0, 0, 0, 0,
	/* Str Int Wis Dex Con Cha */
	{   7,  7,  7,  7,  7,  7 },
	{  20, 15, 15, 20, 20, 10 },
	{ 118, 18, 18, 18, 18, 18 },	/* max */
	/* Init   Lower  Higher */
	{ 10, 0,  0, 8,  1, 0 },	/* Hit points */
	{  2, 0,  0, 2,  0, 3 },14,	/* Energy */
	0, 10, 0, 0,  4, A_INT, 0, -3
};



/* Table of all races */
const struct Race races[] = {
{
#ifndef JP
	"human", "human", "humanity", "Hum",
	{"man", "woman"},
#else
	"êlä‘", "êlä‘ÇÃ", "êlóﬁ", "Hum",
	{"íj", "èó"},
#endif /*JP*/
	PM_HUMAN, NON_PM, PM_HUMAN_MUMMY, PM_HUMAN_ZOMBIE,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	MH_HUMAN, 0, MH_GNOME|MH_ORC,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   2,  0,  0,  0,  0,  0 },	/* STR18(100), 18, 18, 18, 18, 18 */
	/* Init   Lower  Higher */
	{  2, 0,  0, 2,  1, 0 },	/* Hit points */
	{  1, 0,  2, 0,  2, 0 }		/* Energy */
},{
#ifndef JP
	"elf", "elven", "elvenkind", "Elf",
	{0, 0},
#else
	"ÉGÉãÉt", "ÉGÉãÉtÇÃ", "ÉGÉãÉtë∞", "Elf",
	{0, 0},
#endif /*JP*/
	PM_ELF, NON_PM, PM_ELF_MUMMY, PM_ELF_ZOMBIE,
	MH_ELF | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ELF, MH_ELF, MH_ORC,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   0,  2,  2,  0, -2,  1 },	/* 18, 20, 20, 18, 16, 19 */
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  1, 0 },	/* Hit points */
	{  2, 0,  3, 0,  3, 0 }		/* Energy */
},{
#ifndef JP
	"dwarf", "dwarven", "dwarvenkind", "Dwa",
	{0, 0},
#else
	"ÉhÉèÅ[Ét", "ÉhÉèÅ[ÉtÇÃ", "ÉhÉèÅ[Étë∞", "Dwa",
	{0, 0},
#endif /*JP*/
	PM_DWARF, NON_PM, PM_DWARF_MUMMY, PM_DWARF_ZOMBIE,
	MH_DWARF | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	MH_DWARF, MH_DWARF|MH_GNOME, MH_ORC,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   2, -2, -2,  2,  2, -2 },	/* STR18(100), 16, 16, 20, 20, 16 */
	/* Init   Lower  Higher */
	{  4, 0,  0, 3,  2, 0 },	/* Hit points */
	{  0, 0,  0, 0,  0, 0 }		/* Energy */
},{
#ifndef JP
	"gnome", "gnomish", "gnomehood", "Gno",
	{0, 0},
#else
	"ÉmÅ[ÉÄ", "ÉmÅ[ÉÄÇÃ", "ÉmÅ[ÉÄë∞", "Gno",
	{0, 0},
#endif /*JP*/
	PM_GNOME, NON_PM, PM_GNOME_MUMMY, PM_GNOME_ZOMBIE,
	MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	MH_GNOME, MH_DWARF|MH_GNOME, MH_HUMAN,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   1,  1,  0,  0,  0,  0 },	/* STR18(50),19, 18, 18, 18, 18 */
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  2, 0,  2, 0,  2, 0 }		/* Energy */
},{
#ifndef JP
	"orc", "orcish", "orcdom", "Orc",
	{0, 0},
#else
	"ÉIÅ[ÉN", "ÉIÅ[ÉNÇÃ", "ÉIÅ[ÉNë∞", "Orc",
	{0, 0},
#endif /*JP*/
	PM_ORC, NON_PM, PM_ORC_MUMMY, PM_ORC_ZOMBIE,
	MH_ORC | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ORC, 0, MH_HUMAN|MH_ELF|MH_DWARF,
	/* Str Int Wis Dex Con Cha */
	{   3,  3,  3,  3,  3,  3 },
	{   2, -2, -2,  0,  0, -3 },	/* STR18(100),16, 16, 18, 18, 15 */
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  1, 0,  1, 0,  1, 0 }		/* Energy */
},
/* Array terminator */
{ 0, 0, 0, 0 }};


/* The player's race, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Race urace =
{	"something", "undefined", "something", "Xxx",
	{0, 0},
	NON_PM, NON_PM, NON_PM, NON_PM,
	0, 0, 0, 0,
	/*    Str     Int Wis Dex Con Cha */
	{      3,      3,  3,  3,  3,  3 },
	{ STR18(100), 18, 18, 18, 18, 18 },
	/* Init   Lower  Higher */
	{  2, 0,  0, 2,  1, 0 },	/* Hit points */
	{  1, 0,  2, 0,  2, 0 }		/* Energy */
};


/* Table of all genders */
const struct Gender genders[] = {
#ifndef JP
	{"male",	"he",	"him",	"his",	"Mal",	ROLE_MALE},
	{"female",	"she",	"her",	"her",	"Fem",	ROLE_FEMALE},
	{"neuter",	"it",	"it",	"its",	"Ntr",	ROLE_NEUTER}
#else
	{"íjê´",	"îﬁ",	"îﬁ",	"îﬁÇÃ",	"Mal",	ROLE_MALE},
	{"èóê´",	"îﬁèó",	"îﬁèó","îﬁèóÇÃ","Fem",	ROLE_FEMALE},
	{"íÜê´",	"ÇªÇÍ",	"ÇªÇÍ",	"ÇªÇÃ",	"Ntr",	ROLE_NEUTER}
#endif /*JP*/
};


/* Table of all alignments */
const struct Align aligns[] = {
#ifndef JP
	{"law",		"lawful",	"Law",	ROLE_LAWFUL,	A_LAWFUL},
	{"balance",	"neutral",	"Neu",	ROLE_NEUTRAL,	A_NEUTRAL},
	{"chaos",	"chaotic",	"Cha",	ROLE_CHAOTIC,	A_CHAOTIC},
	{"evil",	"unaligned",	"Una",	0,		A_NONE}
#else
	{"ñ@",		"íÅèò",		"Law",	ROLE_LAWFUL,	A_LAWFUL},
	{"ãœçt",	"íÜóß",		"Neu",	ROLE_NEUTRAL,	A_NEUTRAL},
	{"ç¨ì◊",	"ç¨ì◊",		"Cha",	ROLE_CHAOTIC,	A_CHAOTIC},
	{"é◊à´",	"ñ≥êS",		"Una",	0,		A_NONE}
#endif /*JP*/
};

STATIC_DCL char * FDECL(promptsep, (char *, int));
STATIC_DCL int FDECL(role_gendercount, (int));
STATIC_DCL int FDECL(race_alignmentcount, (int));

/* used by str2XXX() */
static char NEARDATA randomstr[] = "random";


boolean
validrole(rolenum)
	int rolenum;
{
	return (rolenum >= 0 && rolenum < SIZE(roles)-1);
}


int
randrole()
{
	return (rn2(SIZE(roles)-1));
}


int
str2role(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; roles[i].name.m; i++) {
	    /* Does it match the male name? */
	    if (!strncmpi(str, roles[i].name.m, len))
		return i;
	    /* Or the female name? */
	    if (roles[i].name.f && !strncmpi(str, roles[i].name.f, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, roles[i].filecode))
		return i;
	}

	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean
validrace(rolenum, racenum)
	int rolenum, racenum;
{
	/* Assumes validrole */
	return (racenum >= 0 && racenum < SIZE(races)-1 &&
		(roles[rolenum].allow & races[racenum].allow & ROLE_RACEMASK));
}


int
randrace(rolenum)
	int rolenum;
{
	int i, n = 0;

	/* Count the number of valid races */
	for (i = 0; races[i].noun; i++)
	    if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK)
	    	n++;

	/* Pick a random race */
	/* Use a factor of 100 in case of bad random number generators */
	if (n) n = rn2(n*100)/100;
	for (i = 0; races[i].noun; i++)
	    if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role has no permitted races? */
	return (rn2(SIZE(races)-1));
}


int
str2race(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; races[i].noun; i++) {
	    /* Does it match the noun? */
	    if (!strncmpi(str, races[i].noun, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, races[i].filecode))
		return i;
	}

	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean
validgend(rolenum, racenum, gendnum)
	int rolenum, racenum, gendnum;
{
	/* Assumes validrole and validrace */
	return (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		(roles[rolenum].allow & races[racenum].allow &
		 genders[gendnum].allow & ROLE_GENDMASK));
}


int
randgend(rolenum, racenum)
	int rolenum, racenum;
{
	int i, n = 0;

	/* Count the number of valid genders */
	for (i = 0; i < ROLE_GENDERS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		genders[i].allow & ROLE_GENDMASK)
	    	n++;

	/* Pick a random gender */
	if (n) n = rn2(n);
	for (i = 0; i < ROLE_GENDERS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		genders[i].allow & ROLE_GENDMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role/race has no permitted genders? */
	return (rn2(ROLE_GENDERS));
}


int
str2gend(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_GENDERS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, genders[i].adj, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, genders[i].filecode))
		return i;
	}
	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean
validalign(rolenum, racenum, alignnum)
	int rolenum, racenum, alignnum;
{
	/* Assumes validrole and validrace */
	return (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		(roles[rolenum].allow & races[racenum].allow &
		 aligns[alignnum].allow & ROLE_ALIGNMASK));
}


int
randalign(rolenum, racenum)
	int rolenum, racenum;
{
	int i, n = 0;

	/* Count the number of valid alignments */
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK)
	    	n++;

	/* Pick a random alignment */
	if (n) n = rn2(n);
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role/race has no permitted alignments? */
	return (rn2(ROLE_ALIGNS));
}


int
str2align(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_ALIGNS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, aligns[i].adj, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, aligns[i].filecode))
		return i;
	}
	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}

/* is rolenum compatible with any racenum/gendnum/alignnum constraints? */
boolean
ok_role(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (rolenum >= 0 && rolenum < SIZE(roles)-1) {
	allow = roles[rolenum].allow;
	if (racenum >= 0 && racenum < SIZE(races)-1 &&
		!(allow & races[racenum].allow & ROLE_RACEMASK))
	    return FALSE;
	if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		!(allow & genders[gendnum].allow & ROLE_GENDMASK))
	    return FALSE;
	if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		!(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < SIZE(roles)-1; i++) {
	    allow = roles[i].allow;
	    if (racenum >= 0 && racenum < SIZE(races)-1 &&
		    !(allow & races[racenum].allow & ROLE_RACEMASK))
		continue;
	    if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		    !(allow & genders[gendnum].allow & ROLE_GENDMASK))
		continue;
	    if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		    !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random role subject to any racenum/gendnum/alignnum constraints */
/* If pickhow == PICK_RIGID a role is returned only if there is  */
/* a single possibility */
int
pick_role(racenum, gendnum, alignnum, pickhow)
int racenum, gendnum, alignnum, pickhow;
{
    int i;
    int roles_ok = 0;

    for (i = 0; i < SIZE(roles)-1; i++) {
	if (ok_role(i, racenum, gendnum, alignnum))
	    roles_ok++;
    }
    if (roles_ok == 0 || (roles_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    roles_ok = rn2(roles_ok);
    for (i = 0; i < SIZE(roles)-1; i++) {
	if (ok_role(i, racenum, gendnum, alignnum)) {
	    if (roles_ok == 0)
		return i;
	    else
		roles_ok--;
	}
    }
    return ROLE_NONE;
}

/* is racenum compatible with any rolenum/gendnum/alignnum constraints? */
boolean
ok_race(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (racenum >= 0 && racenum < SIZE(races)-1) {
	allow = races[racenum].allow;
	if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		!(allow & roles[rolenum].allow & ROLE_RACEMASK))
	    return FALSE;
	if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		!(allow & genders[gendnum].allow & ROLE_GENDMASK))
	    return FALSE;
	if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		!(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < SIZE(races)-1; i++) {
	    allow = races[i].allow;
	    if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		    !(allow & roles[rolenum].allow & ROLE_RACEMASK))
		continue;
	    if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		    !(allow & genders[gendnum].allow & ROLE_GENDMASK))
		continue;
	    if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		    !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random race subject to any rolenum/gendnum/alignnum constraints */
/* If pickhow == PICK_RIGID a race is returned only if there is  */
/* a single possibility */
int
pick_race(rolenum, gendnum, alignnum, pickhow)
int rolenum, gendnum, alignnum, pickhow;
{
    int i;
    int races_ok = 0;

    for (i = 0; i < SIZE(races)-1; i++) {
	if (ok_race(rolenum, i, gendnum, alignnum))
	    races_ok++;
    }
    if (races_ok == 0 || (races_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    races_ok = rn2(races_ok);
    for (i = 0; i < SIZE(races)-1; i++) {
	if (ok_race(rolenum, i, gendnum, alignnum)) {
	    if (races_ok == 0)
		return i;
	    else
		races_ok--;
	}
    }
    return ROLE_NONE;
}

/* is gendnum compatible with any rolenum/racenum/alignnum constraints? */
/* gender and alignment are not comparable (and also not constrainable) */
boolean
ok_gend(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (gendnum >= 0 && gendnum < ROLE_GENDERS) {
	allow = genders[gendnum].allow;
	if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		!(allow & roles[rolenum].allow & ROLE_GENDMASK))
	    return FALSE;
	if (racenum >= 0 && racenum < SIZE(races)-1 &&
		!(allow & races[racenum].allow & ROLE_GENDMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < ROLE_GENDERS; i++) {
	    allow = genders[i].allow;
	    if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		    !(allow & roles[rolenum].allow & ROLE_GENDMASK))
		continue;
	    if (racenum >= 0 && racenum < SIZE(races)-1 &&
		    !(allow & races[racenum].allow & ROLE_GENDMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random gender subject to any rolenum/racenum/alignnum constraints */
/* gender and alignment are not comparable (and also not constrainable) */
/* If pickhow == PICK_RIGID a gender is returned only if there is  */
/* a single possibility */
int
pick_gend(rolenum, racenum, alignnum, pickhow)
int rolenum, racenum, alignnum, pickhow;
{
    int i;
    int gends_ok = 0;

    for (i = 0; i < ROLE_GENDERS; i++) {
	if (ok_gend(rolenum, racenum, i, alignnum))
	    gends_ok++;
    }
    if (gends_ok == 0 || (gends_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    gends_ok = rn2(gends_ok);
    for (i = 0; i < ROLE_GENDERS; i++) {
	if (ok_gend(rolenum, racenum, i, alignnum)) {
	    if (gends_ok == 0)
		return i;
	    else
		gends_ok--;
	}
    }
    return ROLE_NONE;
}

/* is alignnum compatible with any rolenum/racenum/gendnum constraints? */
/* alignment and gender are not comparable (and also not constrainable) */
boolean
ok_align(rolenum, racenum, gendnum, alignnum)
int rolenum, racenum, gendnum, alignnum;
{
    int i;
    short allow;

    if (alignnum >= 0 && alignnum < ROLE_ALIGNS) {
	allow = aligns[alignnum].allow;
	if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		!(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	if (racenum >= 0 && racenum < SIZE(races)-1 &&
		!(allow & races[racenum].allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < ROLE_ALIGNS; i++) {
	    allow = races[i].allow;
	    if (rolenum >= 0 && rolenum < SIZE(roles)-1 &&
		    !(allow & roles[rolenum].allow & ROLE_ALIGNMASK))
		continue;
	    if (racenum >= 0 && racenum < SIZE(races)-1 &&
		    !(allow & races[racenum].allow & ROLE_ALIGNMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}

/* pick a random alignment subject to any rolenum/racenum/gendnum constraints */
/* alignment and gender are not comparable (and also not constrainable) */
/* If pickhow == PICK_RIGID an alignment is returned only if there is  */
/* a single possibility */
int
pick_align(rolenum, racenum, gendnum, pickhow)
int rolenum, racenum, gendnum, pickhow;
{
    int i;
    int aligns_ok = 0;

    for (i = 0; i < ROLE_ALIGNS; i++) {
	if (ok_align(rolenum, racenum, gendnum, i))
	    aligns_ok++;
    }
    if (aligns_ok == 0 || (aligns_ok > 1 && pickhow == PICK_RIGID))
	return ROLE_NONE;
    aligns_ok = rn2(aligns_ok);
    for (i = 0; i < ROLE_ALIGNS; i++) {
	if (ok_align(rolenum, racenum, gendnum, i)) {
	    if (aligns_ok == 0)
		return i;
	    else
		aligns_ok--;
	}
    }
    return ROLE_NONE;
}

void
rigid_role_checks()
{
    /* Some roles are limited to a single race, alignment, or gender and
     * calling this routine prior to XXX_player_selection() will help
     * prevent an extraneous prompt that actually doesn't allow
     * you to choose anything further. Note the use of PICK_RIGID which
     * causes the pick_XX() routine to return a value only if there is one
     * single possible selection, otherwise it returns ROLE_NONE.
     *
     */
    if (flags.initrole == ROLE_RANDOM) {
	/* If the role was explicitly specified as ROLE_RANDOM
	 * via -uXXXX-@ then choose the role in here to narrow down
	 * later choices. Pick a random role in this case.
	 */
	flags.initrole = pick_role(flags.initrace, flags.initgend,
					flags.initalign, PICK_RANDOM);
	if (flags.initrole < 0)
	    flags.initrole = randrole();
    }
    if (flags.initrole != ROLE_NONE) {
	if (flags.initrace == ROLE_NONE)
	     flags.initrace = pick_race(flags.initrole, flags.initgend,
						flags.initalign, PICK_RIGID);
	if (flags.initalign == ROLE_NONE)
	     flags.initalign = pick_align(flags.initrole, flags.initrace,
						flags.initgend, PICK_RIGID);
	if (flags.initgend == ROLE_NONE)
	     flags.initgend = pick_gend(flags.initrole, flags.initrace,
						flags.initalign, PICK_RIGID);
    }
}

#define BP_ALIGN	0
#define BP_GEND		1
#define BP_RACE		2
#define BP_ROLE		3
#define NUM_BP		4

STATIC_VAR char pa[NUM_BP], post_attribs;

STATIC_OVL char *
promptsep(buf, num_post_attribs)
char *buf;
int num_post_attribs;
{
#ifndef JP
	const char *conj = "and ";
	if (num_post_attribs > 1
	    && post_attribs < num_post_attribs && post_attribs > 1)
	 	Strcat(buf, ","); 
	Strcat(buf, " ");
	--post_attribs;
	if (!post_attribs && num_post_attribs > 1) Strcat(buf, conj);
#else
	if (post_attribs-- < num_post_attribs) Strcat(buf, "ÅE");
#endif /*JP*/
	return buf;
}

STATIC_OVL int
role_gendercount(rolenum)
int rolenum;
{
	int gendcount = 0;
	if (validrole(rolenum)) {
		if (roles[rolenum].allow & ROLE_MALE) ++gendcount;
		if (roles[rolenum].allow & ROLE_FEMALE) ++gendcount;
		if (roles[rolenum].allow & ROLE_NEUTER) ++gendcount;
	}
	return gendcount;
}

STATIC_OVL int
race_alignmentcount(racenum)
int racenum;
{
	int aligncount = 0;
	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
		if (races[racenum].allow & ROLE_CHAOTIC) ++aligncount;
		if (races[racenum].allow & ROLE_LAWFUL) ++aligncount;
		if (races[racenum].allow & ROLE_NEUTRAL) ++aligncount;
	}
	return aligncount;
}

char *
root_plselection_prompt(suppliedbuf, buflen, rolenum, racenum, gendnum, alignnum)
char *suppliedbuf;
int buflen, rolenum, racenum, gendnum, alignnum;
{
	int k, gendercount = 0, aligncount = 0;
	char buf[BUFSZ];
	static char err_ret[] = E_J(" character's","ÉLÉÉÉâÉNÉ^Å[ÇÃ");
	boolean donefirst = FALSE;

	if (!suppliedbuf || buflen < 1) return err_ret;

	/* initialize these static variables each time this is called */
	post_attribs = 0;
	for (k=0; k < NUM_BP; ++k)
		pa[k] = 0;
	buf[0] = '\0';
	*suppliedbuf = '\0';
	
	/* How many alignments are allowed for the desired race? */
	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM)
		aligncount = race_alignmentcount(racenum);

	if (alignnum != ROLE_NONE && alignnum != ROLE_RANDOM) {
		/* if race specified, and multiple choice of alignments for it */
		if ((racenum >= 0) && (aligncount > 1)) {
			if (donefirst) Strcat(buf, " ");
			Strcat(buf, aligns[alignnum].adj);
			donefirst = TRUE;
		} else {
			if (donefirst) Strcat(buf, " ");
			Strcat(buf, aligns[alignnum].adj);
			donefirst = TRUE;
		}
	} else {
		/* if alignment not specified, but race is specified
			and only one choice of alignment for that race then
			don't include it in the later list */
		if ((((racenum != ROLE_NONE && racenum != ROLE_RANDOM) &&
			ok_race(rolenum, racenum, gendnum, alignnum))
		      && (aligncount > 1))
		     || (racenum == ROLE_NONE || racenum == ROLE_RANDOM)) {
			pa[BP_ALIGN] = 1;
			post_attribs++;
		}
	}
	/* <your lawful> */

	/* How many genders are allowed for the desired role? */
	if (validrole(rolenum))
		gendercount = role_gendercount(rolenum);

	if (gendnum != ROLE_NONE  && gendnum != ROLE_RANDOM) {
		if (validrole(rolenum)) {
		     /* if role specified, and multiple choice of genders for it,
			and name of role itself does not distinguish gender */
			if ((rolenum != ROLE_NONE) && (gendercount > 1)
						&& !roles[rolenum].name.f) {
				if (donefirst) Strcat(buf, " ");
				Strcat(buf, genders[gendnum].adj);
				donefirst = TRUE;
			}
	        } else {
			if (donefirst) Strcat(buf, " ");
	        	Strcat(buf, genders[gendnum].adj);
			donefirst = TRUE;
	        }
	} else {
		/* if gender not specified, but role is specified
			and only one choice of gender then
			don't include it in the later list */
		if ((validrole(rolenum) && (gendercount > 1)) || !validrole(rolenum)) {
			pa[BP_GEND] = 1;
			post_attribs++;
		}
	}
	/* <your lawful female> */

	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
		if (validrole(rolenum) && ok_race(rolenum, racenum, gendnum, alignnum)) {
			if (donefirst) Strcat(buf, " "); 
			Strcat(buf, (rolenum == ROLE_NONE) ?
				races[racenum].noun :
				races[racenum].adj);
			donefirst = TRUE;
		} else if (!validrole(rolenum)) {
			if (donefirst) Strcat(buf, " ");
			Strcat(buf, races[racenum].noun);
			donefirst = TRUE;
		} else {
			pa[BP_RACE] = 1;
			post_attribs++;
		}
	} else {
		pa[BP_RACE] = 1;
		post_attribs++;
	}
	/* <your lawful female gnomish> || <your lawful female gnome> */

	if (validrole(rolenum)) {
		if (donefirst) Strcat(buf, " ");
		if (gendnum != ROLE_NONE) {
		    if (gendnum == 1  && roles[rolenum].name.f)
			Strcat(buf, roles[rolenum].name.f);
		    else
  			Strcat(buf, roles[rolenum].name.m);
		} else {
			if (roles[rolenum].name.f) {
				Strcat(buf, roles[rolenum].name.m);
				Strcat(buf, "/");
				Strcat(buf, roles[rolenum].name.f);
			} else 
				Strcat(buf, roles[rolenum].name.m);
		}
		donefirst = TRUE;
	} else if (rolenum == ROLE_NONE) {
		pa[BP_ROLE] = 1;
		post_attribs++;
	}
	
	if ((racenum == ROLE_NONE || racenum == ROLE_RANDOM) && !validrole(rolenum)) {
		if (donefirst) Strcat(buf, " ");
		Strcat(buf, E_J("character","ÉLÉÉÉâÉNÉ^Å["));
		donefirst = TRUE;
	}
	/* <your lawful female gnomish cavewoman> || <your lawful female gnome>
	 *    || <your lawful female character>
	 */
	if (buflen > (int) (strlen(buf) + 1)) {
		Strcpy(suppliedbuf, buf);
		return suppliedbuf;
	} else
		return err_ret;
}

char *
build_plselection_prompt(buf, buflen, rolenum, racenum, gendnum, alignnum)
char *buf;
int buflen, rolenum, racenum, gendnum, alignnum;
{
	const char *defprompt = "Shall I pick a character for you? [ynq] ";
	int num_post_attribs = 0;
	char tmpbuf[BUFSZ];
	
	if (buflen < QBUFSZ)
		return (char *)defprompt;

#ifndef JP
	Strcpy(tmpbuf, "Shall I pick ");
#else
	tmpbuf[0] = 0;
#endif /*JP*/
	if (racenum != ROLE_NONE || validrole(rolenum))
		Strcat(tmpbuf, E_J("your ","Ç†Ç»ÇΩÇÃ"));
	else {
#ifndef JP
		Strcat(tmpbuf, "a ");
#endif /*JP*/
	}
	/* <your> */

	(void)  root_plselection_prompt(eos(tmpbuf), buflen - strlen(tmpbuf),
					rolenum, racenum, gendnum, alignnum);
	Sprintf(buf, "%s", s_suffix(tmpbuf));

	/* buf should now be:
	 * < your lawful female gnomish cavewoman's> || <your lawful female gnome's>
	 *    || <your lawful female character's>
	 *
         * Now append the post attributes to it
	 */

	num_post_attribs = post_attribs;
	if (post_attribs) {
		if (pa[BP_RACE]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("race","éÌë∞"));
		}
		if (pa[BP_ROLE]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("role","êEã∆"));
		}
		if (pa[BP_GEND]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("gender","ê´ï "));
		}
		if (pa[BP_ALIGN]) {
			(void) promptsep(eos(buf), num_post_attribs);
			Strcat(buf, E_J("alignment","ëÆê´"));
		}
	}
	Strcat(buf, E_J(" for you? [ynq] ","ÇìKìñÇ…åàÇﬂÇ‹Ç∑Ç©ÅH [ynq] "));
	return buf;
}

#undef BP_ALIGN
#undef BP_GEND
#undef BP_RACE
#undef BP_ROLE
#undef NUM_BP

void
plnamesuffix()
{
	char *sptr, *eptr;
	int i;

	/* Look for tokens delimited by '-' */
	if ((eptr = index(plname, '-')) != (char *) 0)
	    *eptr++ = '\0';
	while (eptr) {
	    /* Isolate the next token */
	    sptr = eptr;
	    if ((eptr = index(sptr, '-')) != (char *)0)
		*eptr++ = '\0';

	    /* Try to match it to something */
	    if ((i = str2role(sptr)) != ROLE_NONE)
		flags.initrole = i;
	    else if ((i = str2race(sptr)) != ROLE_NONE)
		flags.initrace = i;
	    else if ((i = str2gend(sptr)) != ROLE_NONE)
		flags.initgend = i;
	    else if ((i = str2align(sptr)) != ROLE_NONE)
		flags.initalign = i;
	}
	if(!plname[0]) {
	    askname();
	    plnamesuffix();
	}

	/* commas in the plname confuse the record file, convert to spaces */
	for (sptr = plname; *sptr; sptr++) {
		if (*sptr == ',') *sptr = ' ';
	}
}


/*
 *	Special setup modifications here:
 *
 *	Unfortunately, this is going to have to be done
 *	on each newgame or restore, because you lose the permonst mods
 *	across a save/restore.  :-)
 *
 *	1 - The Rogue Leader is the Tourist Nemesis.
 *	2 - Priests start with a random alignment - convert the leader and
 *	    guardians here.
 *	3 - Elves can have one of two different leaders, but can't work it
 *	    out here because it requires hacking the level file data (see
 *	    sp_lev.c).
 *
 * This code also replaces quest_init().
 */
void
role_init()
{
	int alignmnt;
	int i,j,tmp;

	/* Strip the role letter out of the player name.
	 * This is included for backwards compatibility.
	 */
	plnamesuffix();

	/* Check for a valid role.  Try flags.initrole first. */
	if (!validrole(flags.initrole)) {
	    /* Try the player letter second */
	    if ((flags.initrole = str2role(pl_character)) < 0)
	    	/* None specified; pick a random role */
	    	flags.initrole = randrole();
	}

	/* We now have a valid role index.  Copy the role name back. */
	/* This should become OBSOLETE */
	Strcpy(pl_character, roles[flags.initrole].name.m);
	pl_character[PL_CSIZ-1] = '\0';

	/* Check for a valid race */
	if (!validrace(flags.initrole, flags.initrace))
	    flags.initrace = randrace(flags.initrole);

	/* Check for a valid gender.  If new game, check both initgend
	 * and female.  On restore, assume flags.female is correct. */
	if (flags.pantheon == -1) {	/* new game */
	    if (!validgend(flags.initrole, flags.initrace, flags.female))
		flags.female = !flags.female;
	}
	if (!validgend(flags.initrole, flags.initrace, flags.initgend))
	    /* Note that there is no way to check for an unspecified gender. */
	    flags.initgend = flags.female;

	/* Check for a valid alignment */
	if (!validalign(flags.initrole, flags.initrace, flags.initalign))
	    /* Pick a random alignment */
	    flags.initalign = randalign(flags.initrole, flags.initrace);
	alignmnt = aligns[flags.initalign].value;

	/* Initialize urole and urace */
	urole = roles[flags.initrole];
	urace = races[flags.initrace];
	for ( i=0; i<A_MAX; i++ ) {
		if ( i==A_STR ) {
		    tmp = urole.attrmax[A_STR];
		    if ( urace.attradj[i] > 0 ) {
			for ( j=0; j<urace.attradj[i]; j++ ) {
				if ( tmp < 18 ) tmp++;
				else if ( tmp < STR18(50) ) tmp = STR18(50);
				else if ( tmp < STR18(100) ) tmp = STR18(100);
				else tmp++;
			}
		    } else if ( urace.attradj[i] < 0 ) {
			for ( j=0; j<-urace.attradj[i]; j++ ) {
				if ( tmp <= 18 ) tmp--;
				else if ( tmp <= STR18(50) ) tmp = 18;
				else if ( tmp <= STR18(100) ) tmp = STR18(50);
				else tmp--;
			}
		    }
		    urole.attrmax[A_STR] = tmp;
		} else {
			urole.attrmax[i] += urace.attradj[i];
		}
	}

	/* Fix up the quest leader */
	if (urole.ldrnum != NON_PM) {
	    mons[urole.ldrnum].msound = MS_LEADER;
	    mons[urole.ldrnum].mflags2 |= (M2_PEACEFUL);
	    mons[urole.ldrnum].mflags3 |= M3_CLOSE;
	    mons[urole.ldrnum].maligntyp = alignmnt * 3;
	}

	/* Fix up the quest guardians */
	if (urole.guardnum != NON_PM) {
	    mons[urole.guardnum].mflags2 |= (M2_PEACEFUL);
	    mons[urole.guardnum].maligntyp = alignmnt * 3;
	}

	/* Fix up the quest nemesis */
	if (urole.neminum != NON_PM) {
	    mons[urole.neminum].msound = MS_NEMESIS;
	    mons[urole.neminum].mflags2 &= ~(M2_PEACEFUL);
	    mons[urole.neminum].mflags2 |= (M2_NASTY|M2_STALK|M2_HOSTILE);
	    mons[urole.neminum].mflags3 |= M3_WANTSARTI | M3_WAITFORU;
	}

	/* Fix up the god names */
	if (flags.pantheon == -1) {		/* new game */
	    flags.pantheon = flags.initrole;	/* use own gods */
	    while (!roles[flags.pantheon].lgod)	/* unless they're missing */
		flags.pantheon = randrole();
	}
	if (!urole.lgod) {
	    urole.lgod = roles[flags.pantheon].lgod;
	    urole.ngod = roles[flags.pantheon].ngod;
	    urole.cgod = roles[flags.pantheon].cgod;
	}

	/* Fix up infravision */
	if (mons[urace.malenum].mflags3 & M3_INFRAVISION) {
	    /* although an infravision intrinsic is possible, infravision
	     * is purely a property of the physical race.  This means that we
	     * must put the infravision flag in the player's current race
	     * (either that or have separate permonst entries for
	     * elven/non-elven members of each class).  The side effect is that
	     * all NPCs of that class will have (probably bogus) infravision,
	     * but since infravision has no effect for NPCs anyway we can
	     * ignore this.
	     */
	    mons[urole.malenum].mflags3 |= M3_INFRAVISION;
	    if (urole.femalenum != NON_PM)
	    	mons[urole.femalenum].mflags3 |= M3_INFRAVISION;
	}

	/* Artifacts are fixed in hack_artifacts() */

	/* Success! */
	return;
}

const char *
Hello(mtmp)
struct monst *mtmp;
{
	switch (Role_switch) {
#ifndef JP
	case PM_KNIGHT:
	    return ("Salutations"); /* Olde English */
	case PM_SAMURAI:
	    return (mtmp && mtmp->data == &mons[PM_SHOPKEEPER] ?
	    		"Irasshaimase" : "Konnichi wa"); /* Japanese */
#endif /*JP*/
#ifdef TOURIST
	case PM_TOURIST:
	    return (E_J("Aloha","ÉAÉçÅ[Én"));       /* Hawaiian */
#endif
#ifndef JP
	case PM_VALKYRIE:
	    return (
#ifdef MAIL
	    		mtmp && mtmp->data == &mons[PM_MAIL_DAEMON] ? "Hallo" :
#endif
	    		"Velkommen");   /* Norse */
#endif /*JP*/
	default:
	    return (E_J("Hello","Ç±ÇÒÇ…ÇøÇÕ"));
	}
}

const char *
Goodbye()
{
	switch (Role_switch) {
	case PM_KNIGHT:
#ifndef JP
	    return ("Fare thee well");  /* Olde English */
#endif /*JP*/
	case PM_SAMURAI:
	    return (E_J("Sayonara","Ç≥ÇÁÇŒ"));        /* Japanese */
#ifdef TOURIST
	case PM_TOURIST:
	    return (E_J("Aloha","ÉAÉçÅ[Én"));           /* Hawaiian */
#endif
	case PM_VALKYRIE:
	    return (E_J("Farvel","Ç≥ÇÁÇŒ"));          /* Norse */
	default:
	    return (E_J("Goodbye","Ç≥ÇÊÇ§Ç»ÇÁ"));
	}
}

/* role.c */
