/*	SCCS Id: @(#)write.c	3.4	2001/11/29	*/
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL int FDECL(cost,(struct obj *));

/*
 * returns basecost of a scroll or a spellbook
 */
STATIC_OVL int
cost(otmp)
register struct obj *otmp;
{

	if (otmp->oclass == SPBOOK_CLASS)
		return(10 * objects[otmp->otyp].oc_level);

	switch (otmp->otyp) {
# ifdef MAIL
	case SCR_MAIL:
		return(2);
/*		break; */
# endif
	case SCR_LIGHT:
	case SCR_GOLD_DETECTION:
	case SCR_FOOD_DETECTION:
	case SCR_MAGIC_MAPPING:
	case SCR_AMNESIA:
	case SCR_FIRE:
		return(4);
/*		break; */
	case SCR_EARTH:
	case SCR_IDENTIFY:
	case SCR_PUNISHMENT:
		return(8);
/*		break; */
	case SCR_CREATE_MONSTER:
	case SCR_CONFUSE_MONSTER:
		return(14);
/*		break; */
	case SCR_REMOVE_CURSE:
	case SCR_DESTROY_ARMOR:
		return(20);
/*		break; */
	case SCR_TAMING:
	case SCR_CHARGING:
	case SCR_SCARE_MONSTER:
	case SCR_TELEPORTATION:
		return(24);
/*		break; */
	case SCR_ENCHANT_ARMOR:
	case SCR_ENCHANT_WEAPON:
	case SCR_STINKING_CLOUD:
		return(28);
/*		break; */
	case SCR_GENOCIDE:
		return(32);
/*		break; */
	case SCR_BLANK_PAPER:
	default:
		impossible("You can't write such a weird scroll!");
	}
	return(1000);
}

static NEARDATA const char write_on[] = { SCROLL_CLASS, SPBOOK_CLASS, 0 };

int
dowrite(pen)
register struct obj *pen;
{
	register struct obj *paper;
	char namebuf[BUFSZ], *nm, *bp;
	register struct obj *new_obj;
	int basecost, actualcost;
	int curseval;
	char qbuf[QBUFSZ];
	int first, last, i;
	boolean by_descr = FALSE;
	const char *typeword;
#ifdef JP
	static const struct getobj_words wow = { 0, "に", "書く", "書き" };
#endif /*JP*/

	if (nohands(youmonst.data)) {
#ifndef JP
	    You("need hands to be able to write!");
#else
	    pline("書くためには、手が必要だ！");
#endif /*JP*/
	    return 0;
	} else if (Glib) {
#ifndef JP
	    pline("%s from your %s.",
		  Tobjnam(pen, "slip"), makeplural(body_part(FINGER)));
#else
	    pline("%sはあなたの%sから滑り落ちた！",
		  xname(pen), body_part(FINGER));
#endif /*JP*/
	    dropx(pen);
	    return 1;
	}

	/* get paper to write on */
	paper = getobj(write_on,E_J("write on",&wow), 0);
	if(!paper)
		return(0);
	typeword = (paper->oclass == SPBOOK_CLASS) ? E_J("spellbook","魔法書") : E_J("scroll","巻物");
	if(Blind && !paper->dknown) {
		You(E_J("don't know if that %s is blank or not!",
			"%sが白紙かどうかわからない！"), typeword);
		return(1);
	}
	paper->dknown = 1;
	if(paper->otyp != SCR_BLANK_PAPER && paper->otyp != SPE_BLANK_PAPER) {
		pline(E_J("That %s is not blank!",
			  "その%sは白紙ではない！"), typeword);
		exercise(A_WIS, FALSE);
		return(1);
	}

	/* what to write */
	Sprintf(qbuf, E_J("What type of %s do you want to write?",
			  "どの種類の%sを書きますか？"), typeword);
	getlin(qbuf, namebuf);
	(void)mungspaces(namebuf);	/* remove any excess whitespace */
	if(namebuf[0] == '\033' || !namebuf[0])
		return(1);
	nm = namebuf;
	if (!strncmpi(nm, "scroll ", 7)) nm += 7;
	else if (!strncmpi(nm, "spellbook ", 10)) nm += 10;
	if (!strncmpi(nm, "of ", 3)) nm += 3;

	if ((bp = strstri(nm, " armour")) != 0) {
		(void)strncpy(bp, " armor ", 7);	/* won't add '\0' */
		(void)mungspaces(bp + 1);	/* remove the extra space */
	}
#ifdef JP
	if (is_kanji(namebuf[0])) {
	    if (!strcmpr(namebuf, "魔法書") || !strcmpr(namebuf, "呪文書")) {
		namebuf[strlen(namebuf) - 6] = 0;
	    } else if (!strcmpr(namebuf, "巻物")) {
		namebuf[strlen(namebuf) - 4] = 0;
	    } else if (strcmpr(namebuf, "の") && strcmpr(namebuf, "す") && strcmpr(namebuf, "る")) {
		Strcat(namebuf, "の");
	    }
	}
#endif /*JP*/

	first = bases[(int)paper->oclass];
	last = bases[(int)paper->oclass + 1] - 1;
	for (i = first; i <= last; i++) {
		/* extra shufflable descr not representing a real object */
		if (!OBJ_NAME(objects[i])) continue;

		if (!strcmpi(OBJ_NAME(objects[i]), nm))
			goto found;
		if (!strcmpi(OBJ_DESCR(objects[i]), nm)) {
			by_descr = TRUE;
			goto found;
		}
#ifdef JP
		if (!strcmpi(JOBJ_NAME(objects[i]), namebuf))
			goto found;
#endif /*JP*/
	}

#ifndef JP
	There("is no such %s!", typeword);
#else
	pline("そのような%sは存在しない！", typeword);
#endif /*JP*/
	return 1;
found:

	if (i == SCR_BLANK_PAPER || i == SPE_BLANK_PAPER) {
		You_cant(E_J("write that!","白紙を書くことはできない！"));
		pline(E_J("It's obscene!","冗談はよしてください！"));
		return 1;
	} else if (i == SPE_BOOK_OF_THE_DEAD) {
		pline(E_J("No mere dungeon adventurer could write that.",
			  "ただの迷宮探索者がそれを書くことなどできない。"));
		return 1;
	} else if (by_descr && paper->oclass == SPBOOK_CLASS &&
		    !objects[i].oc_name_known) {
		/* can't write unknown spellbooks by description */
		pline(
		  E_J("Unfortunately you don't have enough information to go on.",
		      "残念ながら、あなたはそれを書くための知識を持っていない。"  ));
		return 1;
	}

	/* KMH, conduct */
	u.uconduct.literate++;

	new_obj = mksobj(i, FALSE, FALSE);
	new_obj->bknown = (paper->bknown && pen->bknown);

	/* shk imposes a flat rate per use, not based on actual charges used */
	check_unpaid(pen);

	/* see if there's enough ink */
	basecost = cost(new_obj);
	if(pen->spe < basecost/2)  {
		Your(E_J("marker is too dry to write that!",
			 "マーカには、それを書けるだけのインクがない！"));
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}

	/* we're really going to write now, so calculate cost
	 */
	actualcost = rn1(basecost/2,basecost/2);
	curseval = bcsign(pen) + bcsign(paper);
	exercise(A_WIS, TRUE);
	/* dry out marker */
	if (pen->spe < actualcost) {
		pen->spe = 0;
		Your(E_J("marker dries out!","マーカのインクが切れた！"));
		/* scrolls disappear, spellbooks don't */
		if (paper->oclass == SPBOOK_CLASS) {
			pline_The(
		       E_J("spellbook is left unfinished and your writing fades.",
			   "呪文の記述が中断され、あなたの書いた部分は薄れて消えた。"));
			update_inventory();	/* pen charges */
		} else {
			pline_The(E_J("scroll is now useless and disappears!",
				      "巻物は役に立たなくなり、消滅した！"));
			useup(paper);
		}
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}
	pen->spe -= actualcost;

	/* can't write if we don't know it - unless we're lucky */
	if(!(objects[new_obj->otyp].oc_name_known) &&
	   !(objects[new_obj->otyp].oc_uname) &&
	   (rnl(Role_if(PM_WIZARD) ? 3 : 15))) {
#ifndef JP
		You("%s to write that!", by_descr ? "fail" : "don't know how");
#else
		You("それを%s！", by_descr ? "書くのに失敗した" : "どう書くのかわからない");
#endif /*JP*/
		/* scrolls disappear, spellbooks don't */
		if (paper->oclass == SPBOOK_CLASS) {
			You(
       E_J("write in your best handwriting:  \"My Diary\", but it quickly fades.",
	   "最高に上手い字で「私の日記」と記したが、字はすぐ消えてしまった。"));
			update_inventory();	/* pen charges */
		} else {
			if (by_descr) {
			    Strcpy(namebuf, OBJ_DESCR(objects[new_obj->otyp]));
			    wipeout_text(namebuf, (6+MAXULEV - u.ulevel)/6, 0);
			} else
			    Sprintf(namebuf, E_J("%s was here!","%s参上"), plname);
#ifndef JP
			You("write \"%s\" and the scroll disappears.", namebuf);
#else
			pline("あなたが「%s」と書くと、巻物は消滅した。", namebuf);
#endif /*JP*/
			useup(paper);
		}
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}

	/* useup old scroll / spellbook */
	useup(paper);

	/* success */
	if (new_obj->oclass == SPBOOK_CLASS) {
		/* acknowledge the change in the object's description... */
#ifndef JP
		pline_The("spellbook warps strangely, then turns %s.",
		      OBJ_DESCR(objects[new_obj->otyp]));
#else
		pline("魔法書は奇妙にゆがみ、%s本となった。",
		      JOBJ_DESCR(objects[new_obj->otyp]));
#endif /*JP*/
	}
	new_obj->blessed = (curseval > 0);
	new_obj->cursed = (curseval < 0);
#ifdef MAIL
	if (new_obj->otyp == SCR_MAIL) new_obj->spe = 1;
#endif
#ifndef JP
	new_obj = hold_another_object(new_obj, "Oops!  %s out of your grasp!",
					       The(aobjnam(new_obj, "slip")),
					       (const char *)0);
#else
	new_obj = hold_another_object(new_obj, "おっと！ %sがあなたの手から滑り落ちた！",
					       xname(new_obj), (const char *)0);
#endif /*JP*/
	return(1);
}

/*write.c*/
