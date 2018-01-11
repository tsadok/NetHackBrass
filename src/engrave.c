/*	SCCS Id: @(#)engrave.c	3.4	2001/11/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include <ctype.h>

STATIC_VAR NEARDATA struct engr *head_engr;

STATIC_DCL int FDECL(getobj_filter_writewith, (struct obj *));
STATIC_DCL boolean FDECL(is_engr, (struct engr *));
STATIC_DCL struct engr *FDECL(more_engr, (struct engr *));

#ifdef OVLB
/* random engravings */
static const char *random_mesg[] = {
#ifndef JP
	"Elbereth",
	/* trap engravings */
	"Vlad was here", "ad aerarium",
	/* take-offs and other famous engravings */
	"Owlbreath", "Galadriel",
	"Kilroy was here",
	"A.S. ->", "<- A.S.", /* Journey to the Center of the Earth */
	"You won't get it up the steps", /* Adventure */
	"Lasciate ogni speranza o voi ch'entrate.", /* Inferno */
	"Well Come", /* Prisoner */
	"We apologize for the inconvenience.", /* So Long... */
	"See you next Wednesday", /* Thriller */
	"notary sojak", /* Smokey Stover */
	"For a good time call 8?7-5309",
	"Please don't feed the animals.", /* Various zoos around the world */
	"Madam, in Eden, I'm Adam.", /* A palindrome */
	"Two thumbs up!", /* Siskel & Ebert */
	"Hello, World!", /* The First C Program */
#ifdef MAIL
	"You've got mail!", /* AOL */
#endif
	"As if!", /* Clueless */
#else /*JP*/
	"Elbereth",
	"ヴラド参上", "ad aerarium",
	"Owlbreath", "Galadriel",
	"キルロイ参上",
	"A.S. →", "← A.S.",
	"ソレハ　デキマセン",
	"ここから入らんとするものは全て希望を捨てよ。",
	"よく来たな",
	"ご迷惑をおかけして申し訳ございません",
	"近日公開！",
	"のらくろ",
	"8?7-5309に今すぐお電話を",
	"動物にエサを与えないでください",
	"草の名は 知らず珍し 花の咲く",
	"イチ推し！",
	"Hello, World!",
#ifdef MAIL
	"ユー・ガッタ・メール！",
#endif
	"冗談じゃない！",
	"ゆっくりしていってね！！！",
#endif /*JP*/
};

static const char *directions[4] = {
	"west", "north", "east", "sourth"
};

char *
random_engraving(outbuf)
char *outbuf;
{
	const char *rumor;

	/* a random engraving may come from the "rumors" file,
	   or from the list above */
	if (!rn2(4) || !(rumor = getrumor(0, outbuf, TRUE)) || !*rumor)
	    Strcpy(outbuf, random_mesg[rn2(SIZE(random_mesg))]);

	wipeout_text(outbuf, (int)(strlen(outbuf) / 4), 0);
	return outbuf;
}

/* Partial rubouts for engraving characters. -3. */
static const struct {
	char		wipefrom;
	const char *	wipeto;
} rubouts[] = {
	{'A', "^"},     {'B', "Pb["},   {'C', "("},     {'D', "|)["},
	{'E', "|FL[_"}, {'F', "|-"},    {'G', "C("},    {'H', "|-"},
	{'I', "|"},     {'K', "|<"},    {'L', "|_"},    {'M', "|"},
	{'N', "|\\"},   {'O', "C("},    {'P', "F"},     {'Q', "C("},
	{'R', "PF"},    {'T', "|"},     {'U', "J"},     {'V', "/\\"},
	{'W', "V/\\"},  {'Z', "/"},
	{'b', "|"},     {'d', "c|"},    {'e', "c"},     {'g', "c"},
	{'h', "n"},     {'j', "i"},     {'k', "|"},     {'l', "|"},
	{'m', "nr"},    {'n', "r"},     {'o', "c"},     {'q', "c"},
	{'w', "v"},     {'y', "v"},
	{':', "."},     {';', ","},
	{'0', "C("},    {'1', "|"},     {'6', "o"},     {'7', "/"},
	{'8', "3o"}
};

void
wipeout_text(engr, cnt, seed)
char *engr;
int cnt;
unsigned seed;		/* for semi-controlled randomization */
{
	char *s;
	int i, j, nxt, use_rubout, lth = (int)strlen(engr);

	if (lth && cnt > 0) {
	    while (cnt--) {
		/* pick next character */
		if (!seed) {
		    /* random */
		    nxt = rn2(lth);
		    use_rubout = rn2(4);
		} else {
		    /* predictable; caller can reproduce the same sequence by
		       supplying the same arguments later, or a pseudo-random
		       sequence by varying any of them */
		    nxt = seed % lth;
		    seed *= 31,  seed %= (BUFSZ-1);
		    use_rubout = seed & 3;
		}
#ifdef JP
		if (jrubout(engr, nxt)) continue;
#endif /*JP*/
		s = &engr[nxt];
		if (*s == ' ') continue;

		/* rub out unreadable & small punctuation marks */
		if (index("?.,'`-|_", *s)) {
		    *s = ' ';
		    continue;
		}

		if (!use_rubout)
		    i = SIZE(rubouts);
		else
		    for (i = 0; i < SIZE(rubouts); i++)
			if (*s == rubouts[i].wipefrom) {
			    /*
			     * Pick one of the substitutes at random.
			     */
			    if (!seed)
				j = rn2(strlen(rubouts[i].wipeto));
			    else {
				seed *= 31,  seed %= (BUFSZ-1);
				j = seed % (strlen(rubouts[i].wipeto));
			    }
			    *s = rubouts[i].wipeto[j];
			    break;
			}

		/* didn't pick rubout; use '?' for unreadable character */
		if (i == SIZE(rubouts)) *s = '?';
	    }
	}

	/* trim trailing spaces */
	while (lth && engr[lth-1] == ' ') engr[--lth] = 0;
}

boolean
can_reach_floor()
{
	return (boolean)(!u.uswallow &&
#ifdef STEED
			/* Restricted/unskilled riders can't reach the floor */
			!(u.usteed && P_SKILL(P_RIDING) < P_BASIC) &&
#endif
			 (!Levitation ||
			  Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)));
}
#endif /* OVLB */
#ifdef OVL0

const char *
surface(x, y)
register int x, y;
{
	register struct rm *lev = &levl[x][y];

	if ((x == u.ux) && (y == u.uy) && u.uswallow &&
		is_animal(u.ustuck->data))
	    return E_J("maw","胃壁");
	else if (IS_AIR(lev->typ) && Is_airlevel(&u.uz))
	    return E_J("air","空中");
	else if (is_pool(x,y))
	    return (Underwater && !Is_waterlevel(&u.uz)) ? E_J("bottom","水底") : E_J("water","水");
	else if (is_ice(x,y))
	    return E_J("ice","氷");
	else if (is_lava(x,y))
	    return E_J("lava","溶岩");
	else if (lev->typ == DRAWBRIDGE_DOWN)
	    return E_J("bridge","橋");
	else if(IS_ALTAR(levl[x][y].typ))
	    return E_J("altar","祭壇");
	else if(IS_GRAVE(levl[x][y].typ))
	    return E_J("headstone","墓碑");
	else if(IS_FOUNTAIN(levl[x][y].typ))
	    return E_J("fountain","泉");
	else if (is_swamp(x,y))
	    return E_J("swamp","沼");
	else if (lev->typ == GRASS)
	    return E_J("grass","草地");
	else if ((IS_ROOM(lev->typ) && !Is_earthlevel(&u.uz) && lev->typ != GROUND) ||
		 IS_WALL(lev->typ) || IS_DOOR(lev->typ) || lev->typ == SDOOR)
	    return E_J("floor","床");
	else
	    return E_J("ground","地面");
}

const char *
ceiling(x, y)
register int x, y;
{
	register struct rm *lev = &levl[x][y];
	const char *what;

	/* other room types will no longer exist when we're interested --
	 * see check_special_room()
	 */
	if (*in_rooms(x,y,VAULT))
	    what = E_J("vault's ceiling","金庫室の天井");
	else if (*in_rooms(x,y,TEMPLE))
	    what = E_J("temple's ceiling","寺院の天井");
	else if (*in_rooms(x,y,SHOPBASE))
	    what = E_J("shop's ceiling","店の天井");
	else if (IS_AIR(lev->typ))
	    what = E_J("sky","空");
	else if (Underwater)
	    what = E_J("water's surface","水面");
	else if ((IS_ROOM(lev->typ) && !Is_earthlevel(&u.uz)) ||
		 IS_WALL(lev->typ) || IS_DOOR(lev->typ) || lev->typ == SDOOR)
	    what = E_J("ceiling","天井");
	else
	    what = E_J("rock above","頭上の岩壁");

	return what;
}

boolean
is_skipreading_engr_at(x, y)
xchar x, y;
{
	struct engr *ep = engr_at(x, y);
	if (ep) return (ep->engr_type == HEADSTONE);
	else	return FALSE;
}

boolean
is_engr(ep)
struct engr *ep;
{
	return (ep && ep->engr_type <= N_ENGRAVE);
}

struct engr *
engr_at(x, y)
xchar x, y;
{
	register struct engr *ep = head_engr;

	while(ep) {
		if(x == ep->engr_x && y == ep->engr_y &&
		   is_engr(ep))
			return(ep);
		ep = ep->nxt_engr;
	}
	return((struct engr *) 0);
}

#ifdef ELBERETH
/* Decide whether a particular string is engraved at a specified
 * location; a case-insensitive substring match used.
 * Ignore headstones, in case the player names herself "Elbereth".
 */
int
sengr_at(s, x, y)
	const char *s;
	xchar x, y;
{
	register struct engr *ep = engr_at(x,y);

	return (ep && ep->engr_type != HEADSTONE &&
		ep->engr_time <= moves && strstri(ep->engr_txt, s) != 0);
}
#endif /* ELBERETH */

#endif /* OVL0 */
#ifdef OVL2

void
u_wipe_engr(cnt)
register int cnt;
{
	if (can_reach_floor())
		wipe_engr_at(u.ux, u.uy, cnt);
}

#endif /* OVL2 */
#ifdef OVL1

void
wipe_engr_at(x,y,cnt)
register xchar x,y,cnt;
{
	register struct engr *ep = engr_at(x,y);

	/* Headstones are indelible */
	if(ep && ep->engr_type != HEADSTONE && is_engr(ep)){
	    if(ep->engr_type != BURN || is_ice(x,y)) {
		if(ep->engr_type != DUST && ep->engr_type != ENGR_BLOOD) {
			cnt = rn2(1 + 50/(cnt+1)) ? 0 : 1;
		}
		wipeout_text(ep->engr_txt, (int)cnt, 0);
		while(ep->engr_txt[0] == ' ')
			ep->engr_txt++;
		if(!ep->engr_txt[0]) del_engr(ep);
	    }
	}
}

#endif /* OVL1 */
#ifdef OVL2

void
read_engr_at(x,y)
register int x,y;
{
	register struct engr *ep = engr_at(x,y);
	register int	sensed = 0;
	char buf[BUFSZ];
	
	/* Sensing an engraving does not require sight,
	 * nor does it necessarily imply comprehension (literacy).
	 */
	if(ep && ep->engr_txt[0]) {
	    switch(ep->engr_type) {
	    case DUST:
		if(!Blind) {
			sensed = 1;
#ifndef JP
			pline("%s is written here in the %s.", Something,
				is_ice(x,y) ? "frost" : "dust");
#else
			pline("%sの中になにか文字が書かれている。",
				is_ice(x,y) ? "霜" : "埃");
#endif /*JP*/
		}
		break;
	    case ENGRAVE:
	    case HEADSTONE:
		if (!Blind || can_reach_floor()) {
			sensed = 1;
#ifndef JP
			pline("%s is engraved here on the %s.",
				Something,
				surface(x,y));
#else
			pline("%sになにか文字が刻まれている。",
				surface(x,y));
#endif /*JP*/
		}
		break;
	    case BURN:
		if (!Blind || can_reach_floor()) {
			sensed = 1;
#ifndef JP
			pline("Some text has been %s into the %s here.",
				is_ice(x,y) ? "melted" : "burned",
				surface(x,y));
#else
			pline("%sになにか文章が%sれている。", surface(x,y),
				is_ice(x,y) ? "彫り込ま" : "焼き付けら");
#endif /*JP*/
		}
		break;
	    case MARK:
		if(!Blind) {
			sensed = 1;
			pline(E_J("There's some graffiti on the %s here.",
				  "ここの%sには落書きがされている。"),
				surface(x,y));
		}
		break;
	    case ENGR_BLOOD:
		/* "It's a message!  Scrawled in blood!"
		 * "What's it say?"
		 * "It says... `See you next Wednesday.'" -- Thriller
		 */
		if(!Blind) {
			sensed = 1;
			You(E_J("see a message scrawled in blood here.",
				"殴り書きされた血染めの文字を見つけた。"));
		}
		break;
	    case SHOPSIGN:
		if(!Blind) {
			sensed = 1;
			pline("There's a sign on the %s door.",
				directions[ep->engr_typ2]);
		}
		break;
	    default:
		impossible("%s is written in a very strange way.",
				Something);
		sensed = 1;
	    }
	    if (sensed) {
	    	char *et;
	    	unsigned maxelen = BUFSZ - sizeof(E_J("You feel the words: \"\". ","あなたは文字を探った: 『』"));
	    	if (strlen(ep->engr_txt) > maxelen) {
	    		(void) strncpy(buf,  ep->engr_txt, (int)maxelen);
			buf[maxelen] = '\0';
			et = buf;
		} else
			et = ep->engr_txt;
#ifndef JP
		You("%s: \"%s\".",
		      (Blind) ? "feel the words" : "read",  et);
#else
		You("%s: 『%s』",
		      (Blind) ? "文字を探った" : "読んだ",  et);
#endif /*JP*/
		if(flags.run > 1) nomul(0);
		ep->engr_read = 1;
	    }
	}
}

#endif /* OVL2 */
#ifdef OVLB

void
make_engr_at(x,y,s,e_time,e_type)
register int x,y;
register const char *s;
register long e_time;
register xchar e_type;
{
	register struct engr *ep;

	if ((ep = engr_at(x,y)) != 0 && is_engr(ep))
	    del_engr(ep);
	ep = newengr(strlen(s) + 1);
	ep->nxt_engr = head_engr;
	head_engr = ep;
	ep->engr_x = x;
	ep->engr_y = y;
	ep->engr_txt = (char *)(ep + 1);
	Strcpy(ep->engr_txt, s);
	/* engraving Elbereth shows wisdom */
	if (!in_mklev && !strcmp(s, "Elbereth")) exercise(A_WIS, TRUE);
	ep->engr_time = e_time;
	ep->engr_type = e_type > 0 ? e_type : rnd(N_ENGRAVE-1);
	ep->engr_read = 0;
	ep->engr_lth = strlen(s) + 1;
}

/* delete any engraving at location <x,y> */
void
del_engr_at(x, y)
int x, y;
{
	register struct engr *ep = engr_at(x, y);

	if (ep) {
	    if (!is_engr(ep)) return;
	    del_engr(ep);
	}
}

/*
 *	freehand - returns true if player has a free hand
 */
int
freehand()
{
	return(!uwep || !welded(uwep) ||
	   (!bimanual(uwep) && (!uarms || !uarms->cursed)));
/*	if ((uwep && bimanual(uwep)) ||
	    (uwep && uarms))
		return(0);
	else
		return(1);*/
}

static NEARDATA const char styluses[] =
	{ ALL_CLASSES, ALLOW_NONE, TOOL_CLASS, WEAPON_CLASS, WAND_CLASS,
	  GEM_CLASS, RING_CLASS, 0 };

int
getobj_filter_writewith(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;

	switch (otmp->oclass) {
	    case TOOL_CLASS:
		if (otyp != MAGIC_MARKER &&
		    otyp != TOWEL)
			break;
	    case WEAPON_CLASS:
	    case WAND_CLASS:
	    case GEM_CLASS:
	    case RING_CLASS:
		return GETOBJ_CHOOSEIT;
	    default:
		break;
	}
	return 0;
}

/* Mohs' Hardness Scale:
 *  1 - Talc		 6 - Orthoclase
 *  2 - Gypsum		 7 - Quartz
 *  3 - Calcite		 8 - Topaz
 *  4 - Fluorite	 9 - Corundum
 *  5 - Apatite		10 - Diamond
 *
 * Since granite is a igneous rock hardness ~ 7, anything >= 8 should
 * probably be able to scratch the rock.
 * Devaluation of less hard gems is not easily possible because obj struct
 * does not contain individual oc_cost currently. 7/91
 *
 * steel     -	5-8.5	(usu. weapon)
 * diamond    - 10			* jade	     -	5-6	 (nephrite)
 * ruby       -  9	(corundum)	* turquoise  -	5-6
 * sapphire   -  9	(corundum)	* opal	     -	5-6
 * topaz      -  8			* glass      - ~5.5
 * emerald    -  7.5-8	(beryl)		* dilithium  -	4-5??
 * aquamarine -  7.5-8	(beryl)		* iron	     -	4-5
 * garnet     -  7.25	(var. 6.5-8)	* fluorite   -	4
 * agate      -  7	(quartz)	* brass      -	3-4
 * amethyst   -  7	(quartz)	* gold	     -	2.5-3
 * jasper     -  7	(quartz)	* silver     -	2.5-3
 * onyx       -  7	(quartz)	* copper     -	2.5-3
 * moonstone  -  6	(orthoclase)	* amber      -	2-2.5
 */

/* return 1 if action took 1 (or more) moves, 0 if error or aborted */
int
doengrave()
{
	boolean dengr = FALSE;	/* TRUE if we wipe out the current engraving */
	boolean doblind = FALSE;/* TRUE if engraving blinds the player */
	boolean doknown = FALSE;/* TRUE if we identify the stylus */
	boolean eow = FALSE;	/* TRUE if we are overwriting oep */
	boolean jello = FALSE;	/* TRUE if we are engraving in slime */
	boolean ptext = TRUE;	/* TRUE if we must prompt for engrave text */
	boolean teleengr =FALSE;/* TRUE if we move the old engraving */
	boolean zapwand = FALSE;/* TRUE if we remove a wand charge */
	xchar type = DUST;	/* Type of engraving made */
	char buf[BUFSZ];	/* Buffer for final/poly engraving text */
	char ebuf[BUFSZ];	/* Buffer for initial engraving text */
	char qbuf[QBUFSZ];	/* Buffer for query text */
	char post_engr_text[BUFSZ]; /* Text displayed after engraving prompt */
	const char *everb;	/* Present tense of engraving type */
	const char *eloc;	/* Where the engraving is (ie dust/floor/...) */
	char *sp;		/* Place holder for space count of engr text */
	int len;		/* # of nonspace chars of new engraving text */
	int maxelen;		/* Max allowable length of engraving text */
	struct engr *oep = engr_at(u.ux,u.uy);
				/* The current engraving */
	struct obj *otmp;	/* Object selected with which to engrave */
	char *writer;

#ifdef JP
	static const struct getobj_words engrw = { 0, "を使って", "書く", "書き" };
#endif /*JP*/

	multi = 0;		/* moves consumed */
	nomovemsg = (char *)0;	/* occupation end message */

	buf[0] = (char)0;
	ebuf[0] = (char)0;
	post_engr_text[0] = (char)0;
	maxelen = BUFSZ - 1;
	if (is_demon(youmonst.data) || youmonst.data->mlet == S_VAMPIRE)
	    type = ENGR_BLOOD;

	/* Can the adventurer engrave at all? */

	if(u.uswallow) {
		if (is_animal(u.ustuck->data)) {
			pline(E_J("What would you write?  \"Jonah was here\"?",
				  "なんて書くつもりだい？ “ヨナ参上”？"));
			return(0);
		} else if (is_whirly(u.ustuck->data)) {
			You_cant(E_J("reach the %s.","%sに届かない。"), surface(u.ux,u.uy));
			return(0);
		} else
			jello = TRUE;
	} else if (is_lava(u.ux, u.uy)) {
		You_cant(E_J("write on the lava!","溶岩の上に字を書けない！"));
		return(0);
	} else if (is_pool(u.ux,u.uy) || IS_FOUNTAIN(levl[u.ux][u.uy].typ) ||
		   is_swamp(u.ux,u.uy)) {
		You_cant(E_J("write on the water!","水の上に字を書けない！"));
		return(0);
	}
	if(Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)/* in bubble */) {
		You_cant(E_J("write in thin air!","空中に文字を書けない！"));
		return(0);
	}
	if (cantwield(youmonst.data)) {
		You_cant(E_J("even hold anything!","何かをつかむことすらできない！"));
		return(0);
	}
	if (check_capacity((char *)0)) return (0);

	/* One may write with finger, or weapon, or wand, or..., or...
	 * Edited by GAN 10/20/86 so as not to change weapon wielded.
	 */

	otmp = getobj(styluses, E_J("write with",&engrw), getobj_filter_writewith);
	if(!otmp) return(0);		/* otmp == zeroobj if fingers */

	if (otmp == &zeroobj) writer = E_J(makeplural(body_part(FINGER)),(char *)body_part(FINGER));
	else writer = xname(otmp);

	/* There's no reason you should be able to write with a wand
	 * while both your hands are tied up.
	 */
	if (!freehand() && otmp != uwep && !otmp->owornmask) {
#ifndef JP
		You("have no free %s to write with!", body_part(HAND));
#else
		Your("%sはふさがっていて、字を書くことができない！", body_part(HAND));
#endif /*JP*/
		return(0);
	}

	if (jello) {
#ifndef JP
		You("tickle %s with your %s.", mon_nam(u.ustuck), writer);
		Your("message dissolves...");
#else
		You("%sで%sをくすぐった。", writer, mon_nam(u.ustuck));
		Your("書いた文字は溶けていった…。");
#endif /*JP*/
		return(0);
	}
	if (otmp->oclass != WAND_CLASS && !can_reach_floor()) {
		You_cant(E_J("reach the %s!","%sに届かない！"), surface(u.ux,u.uy));
		return(0);
	}
	if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
		You(E_J("make a motion towards the altar with your %s.",
			"%sを持って祭壇に近づこうとした。"), writer);
		altar_wrath(u.ux, u.uy);
		return(0);
	}
	if (IS_GRAVE(levl[u.ux][u.uy].typ)) {
	    if (otmp == &zeroobj) { /* using only finger */
#ifndef JP
		You("would only make a small smudge on the %s.",
			surface(u.ux, u.uy));
#else
		Your("試みは%sの上に小さな汚れを擦りつけるだけだろう。",
			surface(u.ux, u.uy));
#endif /*JP*/
		return(0);
	    } else if (!levl[u.ux][u.uy].disturbed) {
		You(E_J("disturb the undead!","死者の眠りを妨げた！"));
		levl[u.ux][u.uy].disturbed = 1;
		(void) makemon(&mons[PM_GHOUL], u.ux, u.uy, NO_MM_FLAGS);
		exercise(A_WIS, FALSE);
		return(1);
	    }
	}

	/* SPFX for items */

	switch (otmp->oclass) {
	    default:
	    case AMULET_CLASS:
	    case CHAIN_CLASS:
	    case POTION_CLASS:
	    case COIN_CLASS:
		break;

	    case RING_CLASS:
		/* "diamond" rings and others should work */
	    case GEM_CLASS:
		/* diamonds & other hard gems should work */
		if (objects[otmp->otyp].oc_tough) {
			type = ENGRAVE;
			break;
		}
		break;

	    case ARMOR_CLASS:
		if (is_boots(otmp)) {
			type = DUST;
			break;
		}
		/* fall through */
	    /* Objects too large to engrave with */
	    case BALL_CLASS:
	    case ROCK_CLASS:
		You_cant(E_J("engrave with such a large object!",
			     "そんなに大きな物体で文字を刻むことはできない！"));
		ptext = FALSE;
		break;

	    /* Objects too silly to engrave with */
	    case FOOD_CLASS:
	    case SCROLL_CLASS:
	    case SPBOOK_CLASS:
		Your(E_J("%s would get %s.","%sが%sなってしまう！"), xname(otmp),
			is_ice(u.ux,u.uy) ? E_J("all frosty","氷まみれに") : E_J("too dirty","汚れて駄目に"));
		ptext = FALSE;
		break;

	    case RANDOM_CLASS:	/* This should mean fingers */
		break;

	    /* The charge is removed from the wand before prompting for
	     * the engraving text, because all kinds of setup decisions
	     * and pre-engraving messages are based upon knowing what type
	     * of engraving the wand is going to do.  Also, the player
	     * will have potentially seen "You wrest .." message, and
	     * therefore will know they are using a charge.
	     */
	    case WAND_CLASS:
#if 0 /* no engrave-ID */
		if (zappable(otmp)) {
		    check_unpaid(otmp);
		    zapwand = TRUE;
		    if (Levitation) ptext = FALSE;

		    switch (otmp->otyp) {
		    /* DUST wands */
		    default:
			break;

			/* NODIR wands */
		    case WAN_LIGHT:
		    case WAN_SECRET_DOOR_DETECTION:
		    case WAN_CREATE_MONSTER:
		    case WAN_ENLIGHTENMENT:
		    case WAN_WISHING:
		    case WAN_MAINTENANCE:
			zapnodir(otmp);
			break;

			/* IMMEDIATE wands */
			/* If wand is "IMMEDIATE", remember to affect the
			 * previous engraving even if turning to dust.
			 */
		    case WAN_STRIKING:
			Strcpy(post_engr_text,
			"The wand unsuccessfully fights your attempt to write!"
			);
			break;
		    case WAN_SLOW_MONSTER:
			if (!Blind) {
			   Sprintf(post_engr_text,
				   "The bugs on the %s slow down!",
				   surface(u.ux, u.uy));
			}
			break;
		    case WAN_SPEED_MONSTER:
			if (!Blind) {
			   Sprintf(post_engr_text,
				   "The bugs on the %s speed up!",
				   surface(u.ux, u.uy));
			}
			break;
		    case WAN_POLYMORPH:
			if(oep)  {
			    if (!Blind) {
				type = (xchar)0;	/* random */
				(void) random_engraving(buf);
			    }
			    dengr = TRUE;
			}
			break;
		    case WAN_NOTHING:
		    case WAN_UNDEAD_TURNING:
		    case WAN_OPENING:
		    case WAN_LOCKING:
		    case WAN_PROBING:
			break;

			/* RAY wands */
		    case WAN_MAGIC_MISSILE:
			ptext = TRUE;
			if (!Blind) {
			   Sprintf(post_engr_text,
				   "The %s is riddled by bullet holes!",
				   surface(u.ux, u.uy));
			}
			break;

		    /* can't tell sleep from death - Eric Backus */
		    case WAN_SLEEP:
		    case WAN_DEATH:
			if (!Blind) {
			   Sprintf(post_engr_text,
				   "The bugs on the %s stop moving!",
				   surface(u.ux, u.uy));
			}
			break;

		    case WAN_COLD:
			if (!Blind)
			    Strcpy(post_engr_text,
				"A few ice cubes drop from the wand.");
			if(!oep || (oep->engr_type != BURN))
			    break;
		    case WAN_CANCELLATION:
		    case WAN_MAKE_INVISIBLE:
			if (oep && oep->engr_type != HEADSTONE) {
			    if (!Blind)
				pline_The("engraving on the %s vanishes!",
					surface(u.ux,u.uy));
			    dengr = TRUE;
			}
			break;
		    case WAN_TELEPORTATION:
			if (oep && oep->engr_type != HEADSTONE) {
			    if (!Blind)
				pline_The("engraving on the %s vanishes!",
					surface(u.ux,u.uy));
			    teleengr = TRUE;
			}
			break;

		    /* type = ENGRAVE wands */
		    case WAN_DIGGING:
			ptext = TRUE;
			type  = ENGRAVE;
			if(!objects[otmp->otyp].oc_name_known) {
			    if (flags.verbose)
				pline("This %s is a wand of digging!",
				xname(otmp));
			    doknown = TRUE;
			}
			if (!Blind)
			    Strcpy(post_engr_text,
				IS_GRAVE(levl[u.ux][u.uy].typ) ?
				"Chips fly out from the headstone." :
				is_ice(u.ux,u.uy) ?
				"Ice chips fly up from the ice surface!" :
				"Gravel flies up from the floor.");
			else
			    Strcpy(post_engr_text, "You hear drilling!");
			break;

		    /* type = BURN wands */
		    case WAN_FIRE:
			ptext = TRUE;
			type  = BURN;
			if(!objects[otmp->otyp].oc_name_known) {
			if (flags.verbose)
			    pline("This %s is a wand of fire!", xname(otmp));
			    doknown = TRUE;
			}
			Strcpy(post_engr_text,
				Blind ? "You feel the wand heat up." :
					"Flames fly from the wand.");
			break;
		    case WAN_LIGHTNING:
			ptext = TRUE;
			type  = BURN;
			if(!objects[otmp->otyp].oc_name_known) {
			    if (flags.verbose)
				pline("This %s is a wand of lightning!",
					xname(otmp));
			    doknown = TRUE;
			}
			if (!Blind) {
			    Strcpy(post_engr_text,
				    "Lightning arcs from the wand.");
			    doblind = TRUE;
			} else
			    Strcpy(post_engr_text, "You hear crackling!");
			break;

		    /* type = MARK wands */
		    /* type = ENGR_BLOOD wands */
		    }
		} else /* end if zappable */
		    if (!can_reach_floor()) {
			You_cant("reach the %s!", surface(u.ux,u.uy));
			return(0);
		    }
#endif /* no engrave-ID */
		break;

	    case WEAPON_CLASS:
		if (is_blade(otmp)) {
		    if ((int)otmp->spe > -3)
			type = ENGRAVE;
		    else
#ifndef JP
			Your("%s too dull for engraving.", aobjnam(otmp,"are"));
#else
			Your("%sはなまくらすぎて文字を刻めない。", cxname(otmp));
#endif /*JP*/
		}
		break;

	    case TOOL_CLASS:
		if(otmp == ublindf) {
		    pline(
		E_J("That is a bit difficult to engrave with, don't you think?",
		    "それを文字を刻むために使うのは少々難しいようですが、そう思いませんか？"));
		    return(0);
		}
		switch (otmp->otyp)  {
		    case MAGIC_MARKER:
			if (otmp->spe <= 0)
			    Your(E_J("marker has dried out.","マーカは乾き切っている。"));
			else
			    type = MARK;
			break;
		    case TOWEL:
			/* Can't really engrave with a towel */
			ptext = FALSE;
			if (oep)
			    if ((oep->engr_type == DUST ) ||
				(oep->engr_type == ENGR_BLOOD) ||
				(oep->engr_type == MARK )) {
				if (!Blind)
				    You(E_J("wipe out the message here.",
					    "ここにあったメッセージを拭き取った。"));
				else
#ifndef JP
				    Your("%s %s %s.", xname(otmp),
					 otense(otmp, "get"),
					 is_ice(u.ux,u.uy) ?
					 "frosty" : "dusty");
#else
				    Your("%s%sた。", xname(otmp),
					 is_ice(u.ux,u.uy) ?
					 "氷まみれになっ" : "埃で汚れ");
#endif /*JP*/
				dengr = TRUE;
			    } else
				Your(E_J("%s can't wipe out this engraving.",
					 "%sではここの文字を拭き取ることはできない。"),
				     xname(otmp));
			else
#ifndef JP
			    Your("%s %s %s.", xname(otmp), otense(otmp, "get"),
				  is_ice(u.ux,u.uy) ? "frosty" : "dusty");
#else
			    Your("%s%sた。", xname(otmp),
				  is_ice(u.ux,u.uy) ? "氷まみれになっ" : "埃で汚れ");
#endif /*JP*/
			break;
		    default:
			break;
		}
		break;

	    case VENOM_CLASS:
#ifdef WIZARD
		if (wizard) {
		    pline("Writing a poison pen letter??");
		    break;
		}
#endif
	    case ILLOBJ_CLASS:
		impossible("You're engraving with an illegal object!");
		break;
	}

	if (IS_GRAVE(levl[u.ux][u.uy].typ)) {
	    if (type == ENGRAVE || type == 0)
		type = HEADSTONE;
	    else {
		/* ensures the "cannot wipe out" case */
		type = DUST;
		dengr = FALSE;
		teleengr = FALSE;
		buf[0] = (char)0;
	    }
	}

	/* End of implement setup */

	/* Identify stylus */
	if (doknown) {
	    makeknown(otmp->otyp);
	    more_experienced(0,10);
	}

	if (teleengr) {
	    rloc_engr(oep);
	    oep = (struct engr *)0;
	}

	if (dengr) {
	    del_engr(oep);
	    oep = (struct engr *)0;
	}

	/* Something has changed the engraving here */
	if (*buf) {
	    make_engr_at(u.ux, u.uy, buf, moves, type);
	    pline_The(E_J("engraving now reads: \"%s\".","文字はこう変化した: 『%s』"), buf);
	    ptext = FALSE;
	}

	if (zapwand && (otmp->spe < 0)) {
#ifndef JP
	    pline("%s %sturns to dust.",
		  The(xname(otmp)), Blind ? "" : "glows violently, then ");
#else
	    pline("%sは%s塵となって消えた。",
		  xname(otmp), Blind ? "" : "激しく輝くと、");
#endif /*JP*/
	    if (!IS_GRAVE(levl[u.ux][u.uy].typ))
		You(E_J("are not going to get anywhere trying to write in the %s with your dust.",
			"残された塵で%sの中になんとか文字を書こうとしたが、無理だった。"),
		    is_ice(u.ux,u.uy) ? E_J("frost","霜") : E_J("dust","埃"));
	    useup(otmp);
	    ptext = FALSE;
	}

	if (!ptext) {		/* Early exit for some implements. */
	    if (otmp->oclass == WAND_CLASS && !can_reach_floor())
		You_cant(E_J("reach the %s!","%sに届かない！"), surface(u.ux,u.uy));
	    return(1);
	}

	/* Special effects should have deleted the current engraving (if
	 * possible) by now.
	 */

	if (oep) {
	    register char c = 'n';

	    /* Give player the choice to add to engraving. */

	    if (type == HEADSTONE) {
		/* no choice, only append */
		c = 'y';
	    } else if ( (type == (xchar)oep->engr_type) && (!Blind ||
		 (oep->engr_type == BURN) || (oep->engr_type == ENGRAVE)) ) {
		c = yn_function(E_J("Do you want to add to the current engraving?",
				    "今の文章に文字を書き加えますか？"),
				ynqchars, 'y');
		if (c == 'q') {
		    pline(Never_mind);
		    return(0);
		}
	    }

	    if (c == 'n' || Blind) {

		if( (oep->engr_type == DUST) || (oep->engr_type == ENGR_BLOOD) ||
		    (oep->engr_type == MARK) ) {
		    if (!Blind) {
			You(E_J("wipe out the message that was %s here.",
				"%s文字を拭き消した。"),
			    ((oep->engr_type == DUST)  ? E_J("written in the dust","埃の中に書かれていた") :
			    ((oep->engr_type == ENGR_BLOOD) ? E_J("scrawled in blood","血で殴り書きされた") :
							 E_J("written","ここに書かれていた"))));
			del_engr(oep);
			oep = (struct engr *)0;
		    } else
		   /* Don't delete engr until after we *know* we're engraving */
			eow = TRUE;
		} else
		    if ( (type == DUST) || (type == MARK) || (type == ENGR_BLOOD) ) {
#ifndef JP
			You(
			 "cannot wipe out the message that is %s the %s here.",
			 oep->engr_type == BURN ?
			   (is_ice(u.ux,u.uy) ? "melted into" : "burned into") :
			   "engraved in", surface(u.ux,u.uy));
#else
			You("ここの%sに%s文字を消し去ることができない。",
			    surface(u.ux,u.uy), oep->engr_type == BURN ?
			    (is_ice(u.ux,u.uy) ? "彫り込まれた" : "焼き付けられた") :
			    "刻まれた");
#endif /*JP*/
			return(1);
		    } else
			if ( (type != (xchar)oep->engr_type) || (c == 'n') ) {
			    if (!Blind || can_reach_floor())
				You(E_J("will overwrite the current message.",
					"今の文字を上書きすることにした。"));
			    eow = TRUE;
			}
	    }
	}

	eloc = surface(u.ux,u.uy);
	switch(type){
	    default:
		everb = (oep && !eow ? "add to the weird writing on" :
				       "write strangely on");
		break;
	    case DUST:
		everb = (oep && !eow ? E_J("add to the writing in", "文字を書き加えた") :
				       E_J("write in","文字を書いた"));
		eloc = is_ice(u.ux,u.uy) ? E_J("frost","霜の中") : E_J("dust","埃の中");
		break;
	    case HEADSTONE:
		everb = (oep && !eow ? E_J("add to the epitaph on","銘を書き加えた") :
				       E_J("engrave on","銘を刻んだ"));
		break;
	    case ENGRAVE:
		everb = (oep && !eow ? E_J("add to the engraving in","文字を書き加えた") :
				       E_J("engrave in","文字を刻んだ"));
		break;
	    case BURN:
		everb = (oep && !eow ?
			( is_ice(u.ux,u.uy) ? E_J("add to the text melted into","文字を書き加えた") :
					      E_J("add to the text burned into","文字を焼き加えた")) :
			( is_ice(u.ux,u.uy) ? E_J("melt into","文字を書いた") : E_J("burn into","文字を焼き付けた")));
		break;
	    case MARK:
		everb = (oep && !eow ? "add to the graffiti on" :
				       "scribble on");
		break;
	    case ENGR_BLOOD:
		everb = (oep && !eow ? "add to the scrawl on" :
				       "scrawl on");
		break;
	}

	/* Tell adventurer what is going on */
	if (otmp != &zeroobj)
#ifndef JP
	    You("%s the %s with %s.", everb, eloc, doname(otmp));
#else
	    You("%sを使って%sに%s。", doname(otmp), eloc, everb);
#endif /*JP*/
	else
#ifndef JP
	    You("%s the %s with your %s.", everb, eloc,
		makeplural(body_part(FINGER)));
#else
	    You("%sで%sに%s。", body_part(FINGER), eloc, everb);
#endif /*JP*/

	/* Prompt for engraving! */
#ifndef JP
	Sprintf(qbuf,"What do you want to %s the %s here?", everb, eloc);
#else
	Sprintf(qbuf,"%sに何と書きますか？", eloc);
#endif /*JP*/
	getlin(qbuf, ebuf);

	/* Count the actual # of chars engraved not including spaces */
	len = strlen(ebuf);
	for (sp = ebuf; *sp; sp++) if (isspace(*sp)) len -= 1;

	if (len == 0 || index(ebuf, '\033')) {
	    if (zapwand) {
		if (!Blind)
#ifndef JP
		    pline("%s, then %s.",
			  Tobjnam(otmp, "glow"), otense(otmp, "fade"));
#else
		    pline("%sが一度輝き、光はゆっくりと消えた。", xname(otmp));
#endif /*JP*/
		return(1);
	    } else {
		pline(Never_mind);
		return(0);
	    }
	}

	/* A single `x' is the traditional signature of an illiterate person */
	if (len != 1 || (!index(ebuf, 'x') && !index(ebuf, 'X')))
	    u.uconduct.literate++;

	/* Mix up engraving if surface or state of mind is unsound.
	   Note: this won't add or remove any spaces. */
	for (sp = ebuf; *sp; sp++) {
	    if (isspace(*sp)) continue;
	    if (((type == DUST || type == ENGR_BLOOD) && !rn2(25)) ||
		    (Blind && !rn2(11)) || (Confusion && !rn2(7)) ||
		    (Stunned && !rn2(4)) || (Hallucination && !rn2(2))) {
#ifdef JP
		if (is_kanji(*sp)) {
		    char *tmp = "？";
		    *sp++ = *tmp++;
		    *sp   = *tmp;
		} else
#endif /*JP*/
		*sp = ' ' + rnd(96 - 2);	/* ASCII '!' thru '~'
						   (excludes ' ' and DEL) */
	    }
	}

	/* Previous engraving is overwritten */
	if (eow) {
	    del_engr(oep);
	    oep = (struct engr *)0;
	}

	/* Figure out how long it took to engrave, and if player has
	 * engraved too much.
	 */
	switch(type){
	    default:
		multi = -(len/10);
		if (multi) nomovemsg = "You finish your weird engraving.";
		break;
	    case DUST:
		multi = -(len/10);
		if (multi) nomovemsg = E_J("You finish writing in the dust.","あなたは埃に文字を書き終えた。");
		break;
	    case HEADSTONE:
	    case ENGRAVE:
		multi = -(len/10);
		if ((otmp->oclass == WEAPON_CLASS) &&
		    ((otmp->otyp != ATHAME) || otmp->cursed)) {
		    multi = -len;
		    maxelen = ((otmp->spe + 3) * 2) + 1;
			/* -2 = 3, -1 = 5, 0 = 7, +1 = 9, +2 = 11
			 * Note: this does not allow a +0 anything (except
			 *	 an athame) to engrave "Elbereth" all at once.
			 *	 However, you could now engrave "Elb", then
			 *	 "ere", then "th".
			 */
		    Your(E_J("%s dull.","%sは刃こぼれした。"), E_J(aobjnam(otmp, "get"),xname(otmp)));
		    if (otmp->unpaid) {
			struct monst *shkp = shop_keeper(*u.ushops);
			if (shkp) {
			    You(E_J("damage it, you pay for it!","商品をだめにした以上、対価を支払わざるをえない！"));
			    bill_dummy_object(otmp);
			}
		    }
		    if (len > maxelen) {
			multi = -maxelen;
			otmp->spe = -3;
		    } else if (len > 1)
			otmp->spe -= len >> 1;
		    else otmp->spe -= 1; /* Prevent infinite engraving */
		} else
		    if ( (otmp->oclass == RING_CLASS) ||
			 (otmp->oclass == GEM_CLASS) )
			multi = -len;
		if (multi) nomovemsg = E_J("You finish engraving.","あなたは文字を刻み終えた。");
		break;
	    case BURN:
		multi = -(len/10);
		if (otmp->oclass == WAND_CLASS) {
		    maxelen = otmp->spe + 1;
		    if (len > maxelen) {
			Your(E_J("wand has run out of power!","杖の魔力は使い果たされてしまった！"));
			otmp->spe = 0;
			multi = -(maxelen/10);
		    } else {
			otmp->spe -= len - 1;
		    }
		}
		if (multi)
		    nomovemsg = is_ice(u.ux,u.uy) ?
			E_J("You finish melting your message into the ice.",
			    "あなたは氷を溶かして文字を彫り込む作業を完了した。"):
			E_J("You finish burning your message into the floor.",
			    "あなたは床に文字を焼き付け終えた。");
		break;
	    case MARK:
		multi = -(len/10);
		if ((otmp->oclass == TOOL_CLASS) &&
		    (otmp->otyp == MAGIC_MARKER)) {
		    maxelen = (otmp->spe) * 2; /* one charge / 2 letters */
		    if (len > maxelen) {
			Your(E_J("marker dries out.","マーカのインクが切れた。"));
			otmp->spe = 0;
			multi = -(maxelen/10);
		    } else
			if (len > 1) otmp->spe -= len >> 1;
			else otmp->spe -= 1; /* Prevent infinite grafitti */
		}
		if (multi) nomovemsg = E_J("You finish defacing the dungeon.","あなたは地下迷宮の美観を損ね終えた。");
		break;
	    case ENGR_BLOOD:
		multi = -(len/10);
		if (multi) nomovemsg = E_J("You finish scrawling.","血文字をのたくらせ終えた。");
		break;
	}

	/* Chop engraving down to size if necessary */
	if (len > maxelen) {
	    for (sp = ebuf; (maxelen && *sp); sp++)
		if (!isspace(*sp)) maxelen--;
	    if (!maxelen && *sp) {
		*sp = (char)0;
		if (multi) nomovemsg = E_J("You cannot write any more.","あなたはこれ以上書くことができない。");
		You(E_J("only are able to write \"%s\"","『%s』までしか書けなかった。"), ebuf);
	    }
	}

	/* Add to existing engraving */
	if (oep) Strcpy(buf, oep->engr_txt);

	(void) strncat(buf, ebuf, (BUFSZ - (int)strlen(buf) - 1));

	make_engr_at(u.ux, u.uy, buf, (moves - multi), type);

	if (post_engr_text[0]) pline(post_engr_text);

	if (doblind && !resists_blnd(&youmonst)) {
	    You(E_J("are blinded by the flash!","閃光に目が眩んだ！"));
	    make_blinded((long)rnd(50),FALSE);
	    if (!Blind) Your(vision_clears);
	}

	return(1);
}

void
save_engravings(fd, mode)
int fd, mode;
{
	register struct engr *ep = head_engr;
	register struct engr *ep2;
	unsigned no_more_engr = 0;

	while (ep) {
	    ep2 = ep->nxt_engr;
	    if (ep->engr_lth && ep->engr_txt[0] && perform_bwrite(mode)) {
		bwrite(fd, (genericptr_t)&(ep->engr_lth), sizeof(ep->engr_lth));
		bwrite(fd, (genericptr_t)ep, sizeof(struct engr) + ep->engr_lth);
	    }
	    if (release_data(mode))
		dealloc_engr(ep);
	    ep = ep2;
	}
	if (perform_bwrite(mode))
	    bwrite(fd, (genericptr_t)&no_more_engr, sizeof no_more_engr);
	if (release_data(mode))
	    head_engr = 0;
}

void
rest_engravings(fd)
int fd;
{
	register struct engr *ep;
	unsigned lth;

	head_engr = 0;
	while(1) {
		mread(fd, (genericptr_t) &lth, sizeof(unsigned));
		if(lth == 0) return;
		ep = newengr(lth);
		mread(fd, (genericptr_t) ep, sizeof(struct engr) + lth);
		ep->nxt_engr = head_engr;
		head_engr = ep;
		ep->engr_txt = (char *) (ep + 1);	/* Andreas Bormann */
		/* mark as finished for bones levels -- no problem for
		 * normal levels as the player must have finished engraving
		 * to be able to move again */
		ep->engr_time = moves;
	}
}

void
del_engr(ep)
register struct engr *ep;
{
	if (ep == head_engr) {
		head_engr = ep->nxt_engr;
	} else {
		register struct engr *ept;

		for (ept = head_engr; ept; ept = ept->nxt_engr)
		    if (ept->nxt_engr == ep) {
			ept->nxt_engr = ep->nxt_engr;
			break;
		    }
		if (!ept) {
		    impossible("Error in del_engr?");
		    return;
		}
	}
	dealloc_engr(ep);
}

/* randomly relocate an engraving */
void
rloc_engr(ep)
struct engr *ep;
{
	int tx, ty, tryct = 200;

	do  {
	    if (--tryct < 0) return;
	    tx = rn1(COLNO-3,2);
	    ty = rn2(ROWNO);
	} while (engr_at(tx, ty) ||
		!goodpos(tx, ty, (struct monst *)0, 0));

	ep->engr_x = tx;
	ep->engr_y = ty;
}


/* Epitaphs for random headstones */
static const char *epitaphs[] = {
#ifndef JP
	"Rest in peace",
	"R.I.P.",
	"Rest In Pieces",
	"Note -- there are NO valuable items in this grave",
	"1994-1995. The Longest-Lived Hacker Ever",
	"The Grave of the Unknown Hacker",
	"We weren't sure who this was, but we buried him here anyway",
	"Sparky -- he was a very good dog",
	"Beware of Electric Third Rail",
	"Made in Taiwan",
	"Og friend. Og good dude. Og died. Og now food",
	"Beetlejuice Beetlejuice Beetlejuice",
	"Look out below!",
	"Please don't dig me up. I'm perfectly happy down here. -- Resident",
	"Postman, please note forwarding address: Gehennom, Asmodeus's Fortress, fifth lemure on the left",
	"Mary had a little lamb/Its fleece was white as snow/When Mary was in trouble/The lamb was first to go",
	"Be careful, or this could happen to you!",
	"Soon you'll join this fellow in hell! -- the Wizard of Yendor",
	"Caution! This grave contains toxic waste",
	"Sum quod eris",
	"Here lies an Atheist, all dressed up and no place to go",
	"Here lies Ezekiel, age 102.  The good die young.",
	"Here lies my wife: Here let her lie! Now she's at rest and so am I.",
	"Here lies Johnny Yeast. Pardon me for not rising.",
	"He always lied while on the earth and now he's lying in it",
	"I made an ash of myself",
	"Soon ripe. Soon rotten. Soon gone. But not forgotten.",
	"Here lies the body of Jonathan Blake. Stepped on the gas instead of the brake.",
	"Go away!"
#else
	"安らかに眠れ",
	"R.I.P.",
	"安らかに細切れ",
	"注意 ─ この墓の中に価値ある品物はまったく入ってません",
	"1994-1995. 史上最も長生きしたハッカー",
	"名も無きハッカーの墓",
	"誰かは知らねど、とにかくここに埋葬した誰かの墓",
	"スパーキー ─ 素晴らしい犬だった",
	"送電用の第三軌条に注意",
	"Made in Taiwan",
	"シンユウ, オレノダチ. シンダアト, オレノメシ",
	"ビートルジュース ビートルジュース ビートルジュース",
	"下を見ろ！",
	"掘り起こさないで下さい。今とっても幸せなんです ─ 住人",
	"郵便屋さんへ、こちらに転居しました: ゲヘナ, アスモデウス要塞, 左から5番目の死霊宛",
	"メリーさんの羊 まっしろ羊 メリーさんの危機に まず逃げた",
	"気をつけよ、さもなくばお前もこうなるぞ！",
	"お前もすぐにこやつ同様地獄行きよ！ ─ イェンダーの魔法使い",
	"危険！　墓内に有害廃棄物収蔵",
	"我はかつて汝であり、汝はやがて我となる",
	"無神論者ここに眠る。旅支度済めども、行くべき所なし",
	"エゼキエルここに眠る。享年102歳。いい奴ほど早く死ぬ",
	"わが妻ここに眠る: ここに眠らせとこう！ 妻は安息を得、私もです",
	"ジョニー・イーストここに眠る。寝たままで失礼します", /* 実在のおもしろ碑文 */
	"彼はいつも土の上で寝ていたが、今や土の中で寝ている",
	"自分で自分を灰にした",
	"早く咲き、早く散り、早く去ぬ。しかし決して忘られぬ",
	"ジョナサン・ブレーキここに眠る。ブレーキとアクセルを間違えた",
	"立ち去れ！"
#endif /*JP*/
};

/* Create a headstone at the given location.
 * The caller is responsible for newsym(x, y).
 */
void
make_grave(x, y, str)
int x, y;
const char *str;
{
	/* Can we put a grave here? */
	if ((levl[x][y].typ != ROOM && levl[x][y].typ != GRAVE) || t_at(x,y)) return;

	/* Make the grave */
	levl[x][y].typ = GRAVE;

	/* Engrave the headstone */
	if (!str) str = epitaphs[rn2(SIZE(epitaphs))];
	del_engr_at(x, y);
	make_engr_at(x, y, str, 0L, HEADSTONE);
	return;
}


#endif /* OVLB */

/*engrave.c*/
