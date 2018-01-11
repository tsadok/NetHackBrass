/*	SCCS Id: @(#)wield.c	3.4	2003/01/29	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* KMH -- Differences between the three weapon slots.
 *
 * The main weapon (uwep):
 * 1.  Is filled by the (w)ield command.
 * 2.  Can be filled with any type of item.
 * 3.  May be carried in one or both hands.
 * 4.  Is used as the melee weapon and as the launcher for
 *     ammunition.
 * 5.  Only conveys intrinsics when it is a weapon, weapon-tool,
 *     or artifact.
 * 6.  Certain cursed items will weld to the hand and cannot be
 *     unwielded or dropped.  See erodeable_wep() and will_weld()
 *     below for the list of which items apply.
 *
 * The secondary weapon (uswapwep):
 * 1.  Is filled by the e(x)change command, which swaps this slot
 *     with the main weapon.  If the "pushweapon" option is set,
 *     the (w)ield command will also store the old weapon in the
 *     secondary slot.
 * 2.  Can be field with anything that will fit in the main weapon
 *     slot; that is, any type of item.
 * 3.  Is usually NOT considered to be carried in the hands.
 *     That would force too many checks among the main weapon,
 *     second weapon, shield, gloves, and rings; and it would
 *     further be complicated by bimanual weapons.  A special
 *     exception is made for two-weapon combat.
 * 4.  Is used as the second weapon for two-weapon combat, and as
 *     a convenience to swap with the main weapon.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld (see #3 for reasons), but they also
 *     prevent two-weapon combat.
 *
 * The quiver (uquiver):
 * 1.  Is filled by the (Q)uiver command.
 * 2.  Can be filled with any type of item.
 * 3.  Is considered to be carried in a special part of the pack.
 * 4.  Is used as the item to throw with the (f)ire command.
 *     This is a convenience over the normal (t)hrow command.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld; their effect is handled by the normal
 *     throwing code.
 *
 * No item may be in more than one of these slots.
 */


STATIC_DCL int FDECL(ready_weapon, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_wield, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_ready1, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_ready2, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_wieldsub, (struct obj *));
STATIC_DCL int NDECL(dowieldsubweapon);

/* used by will_weld() */
/* probably should be renamed */
#define erodeable_wep(optr)	((optr)->oclass == WEAPON_CLASS \
				|| is_weptool(optr) \
				|| (optr)->otyp == HEAVY_IRON_BALL \
				|| (optr)->otyp == IRON_CHAIN)

/* used by welded(), and also while wielding */
#define will_weld(optr)		((optr)->cursed \
				&& (erodeable_wep(optr) \
				   || (optr)->otyp == TIN_OPENER))


/*** Functions that place a given item in a slot ***/
/* Proper usage includes:
 * 1.  Initializing the slot during character generation or a
 *     restore.
 * 2.  Setting the slot due to a player's actions.
 * 3.  If one of the objects in the slot are split off, these
 *     functions can be used to put the remainder back in the slot.
 * 4.  Putting an item that was thrown and returned back into the slot.
 * 5.  Emptying the slot, by passing a null object.  NEVER pass
 *     zeroobj!
 *
 * If the item is being moved from another slot, it is the caller's
 * responsibility to handle that.  It's also the caller's responsibility
 * to print the appropriate messages.
 */
void
setuwep(obj)
register struct obj *obj;
{
	struct obj *olduwep = uwep;

	if (obj == uwep) return; /* necessary to not set unweapon */
	/* This message isn't printed in the caller because it happens
	 * *whenever* Sunsword is unwielded, from whatever cause.
	 */
	setworn(obj, W_WEP);
	if (uwep == obj && artifact_light(olduwep) && olduwep->lamplit) {
	    end_burn(olduwep, FALSE);
#ifndef JP
	    if (!Blind) pline("%s glowing.", Tobjnam(olduwep, "stop"));
#else
	    if (!Blind) pline("%sから光が消えた。", xname(olduwep));
#endif /*JP*/
	}
	/* Note: Explicitly wielding a pick-axe will not give a "bashing"
	 * message.  Wielding one via 'a'pplying it will.
	 * 3.2.2:  Wielding arbitrary objects will give bashing message too.
	 */
	if (obj) {
		unweapon = (obj->oclass == WEAPON_CLASS) ?
				is_launcher(obj) || is_ammo(obj) ||
				is_missile(obj) /*|| (is_pole(obj)*/ /* allow short-range combat with polearms */
#ifdef STEED
				/*&& !u.usteed*/
#endif
				/*)*/ : !is_weptool(obj);
	} else
		unweapon = TRUE;	/* for "bare hands" message */
	update_inventory();
}

STATIC_OVL int
ready_weapon(wep)
struct obj *wep;
{
	/* Separated function so swapping works easily */
	int res = 0;

	if (!wep) {
	    /* No weapon */
	    if (uwep) {
#ifndef JP
		You("are empty %s.", body_part(HANDED));
#else
		You("%sを空けた。", body_part(HAND));
#endif /*JP*/
		setuwep((struct obj *) 0);
		res++;
	    } else
#ifndef JP
		You("are already empty %s.", body_part(HANDED));
#else
		Your("%sはすでに空いている。", body_part(HAND));
#endif /*JP*/
	} else if (!uarmg && !Stone_resistance && wep->otyp == CORPSE
				&& touch_petrifies(&mons[wep->corpsenm])) {
	    /* Prevent wielding cockatrice when not wearing gloves --KAA */
	    char kbuf[BUFSZ];

#ifndef JP
	    You("wield the %s corpse in your bare %s.",
		mons[wep->corpsenm].mname, makeplural(body_part(HAND)));
	    Sprintf(kbuf, "%s corpse", an(mons[wep->corpsenm].mname));
#else
	    You("%sの死体を素%sで持ってしまった。",
		JMONNAM(wep->corpsenm), body_part(HAND));
	    Sprintf(kbuf, "%sの死体で", JMONNAM(wep->corpsenm));
#endif /*JP*/
	    instapetrify(kbuf);
	} else if (uarms && bimanual(wep))
#ifndef JP
	    You("cannot wield a two-handed %s while wearing a shield.",
		is_sword(wep) ? "sword" :
		    wep->otyp == BATTLE_AXE ? "axe" : "weapon");
#else
	    pline("盾を構えているため、両手持ちの%sは装備できない。",
		is_sword(wep) ? "剣" :
		    wep->otyp == BATTLE_AXE ? "斧" : "武器");
#endif /*JP*/
	else if (wep->oartifact && !touch_artifact(wep, &youmonst)) {
	    res++;	/* takes a turn even though it doesn't get wielded */
	} else {
	    /* Weapon WILL be wielded after this point */
	    res++;
	    if (will_weld(wep)) {
#ifndef JP
		const char *tmp = xname(wep), *thestr = "The ";
		if (strncmp(tmp, thestr, 4) && !strncmp(The(tmp),thestr,4))
		    tmp = thestr;
		else tmp = "";
		pline("%s%s %s to your %s!", tmp, aobjnam(wep, "weld"),
			(wep->quan == 1L) ? "itself" : "themselves", /* a3 */
			bimanual(wep) ?
				(const char *)makeplural(body_part(HAND))
				: body_part(HAND));
#else
		pline("%sがあなたの%s%sに張り付いた！", xname(wep),
			bimanual(wep) ? "両" : "", body_part(HAND));
#endif /*JP*/
		wep->bknown = TRUE;
	    } else {
		/* The message must be printed before setuwep (since
		 * you might die and be revived from changing weapons),
		 * and the message must be before the death message and
		 * Lifesaved rewielding.  Yet we want the message to
		 * say "weapon in hand", thus this kludge.
		 */
		long dummy = wep->owornmask;
		wep->owornmask |= W_WEP;
		prinv((char *)0, wep, 0L);
		wep->owornmask = dummy;
	    }
	    setuwep(wep);

	    /* KMH -- Talking artifacts are finally implemented */
	    arti_speak(wep);

	    if (artifact_light(wep) && !wep->lamplit) {
		begin_burn(wep, FALSE);
		if (!Blind)
#ifndef JP
		    pline("%s to glow brilliantly!", Tobjnam(wep, "begin"));
#else
		    pline("%sはまばゆい光を放ちはじめた！", xname(wep));
#endif /*JP*/
	    }

#if 0
	    /* we'll get back to this someday, but it's not balanced yet */
	    if (Race_if(PM_ELF) && !wep->oartifact &&
			    get_material(wep) == IRON) {
		/* Elves are averse to wielding cold iron */
		You("have an uneasy feeling about wielding cold iron.");
		change_luck(-1);
	    }
#endif
	    if (wep->oclass == WEAPON_CLASS && P_RESTRICTED(weapon_type(wep)))
		You_feel(E_J("this kind of weapon does not fit you at all.",
			     "この種の武器はまるで自分に合わないと感じた。"));

	    if (wep->unpaid) {
		struct monst *this_shkp;

		if ((this_shkp = shop_keeper(inside_shop(u.ux, u.uy))) !=
		    (struct monst *)0) {
		    pline(E_J("%s says \"You be careful with my %s!\"",
			      "%sは「%sは気をつけて扱って下さいよ！」と言った。"),
			  shkname(this_shkp),
			  xname(wep));
		}
	    }
	}
	return(res);
}

void
setuqwep(obj)
register struct obj *obj;
{
	setworn(obj, W_QUIVER);
	update_inventory();
}

void
setuswapwep(obj)
register struct obj *obj;
{
	setworn(obj, W_SWAPWEP);
	update_inventory();
}

/*** Commands to change particular slot(s) ***/

static NEARDATA const char wield_objs[] =
	{ ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, TOOL_CLASS, 0 };
static NEARDATA const char ready_objs[] =
	{ ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, 0 };
static NEARDATA const char bullets[] =	/* (note: different from dothrow.c) */
	{ ALL_CLASSES, ALLOW_NONE, GEM_CLASS, WEAPON_CLASS, 0 };

int
getobj_filter_wield(otmp)
struct obj *otmp;
{
	if (otmp->owornmask & W_WEP)
		return GETOBJ_ADDELSE;
	if (otmp->oclass == WEAPON_CLASS || is_weptool(otmp))
		return GETOBJ_CHOOSEIT;
	return 0;
}
int
getobj_filter_ready1(otmp)
struct obj *otmp;
{
	if (otmp->owornmask & (W_WEP | W_SWAPWEP | W_QUIVER))
		return GETOBJ_ADDELSE;
	if (otmp->oclass == WEAPON_CLASS)
		return GETOBJ_CHOOSEIT;
	return 0;
}
int
getobj_filter_ready2(otmp)
struct obj *otmp;
{
	if (otmp->owornmask & (W_WEP | W_SWAPWEP | W_QUIVER))
		return GETOBJ_ADDELSE;
	if (otmp->oclass == WEAPON_CLASS ||
	    otmp->oclass == GEM_CLASS)
		return GETOBJ_CHOOSEIT;
	return 0;
}

int
dowield()
{
	register struct obj *wep, *oldwep;
	int result;
#ifdef JP
	static const struct getobj_words wieldw = { 0, 0, "装備する", "装備し" };
#endif /*JP*/

	/* May we attempt this? */
	multi = 0;
	if (cantwield(youmonst.data)) {
		pline(E_J("Don't be ridiculous!","ご冗談を！"));
		return(0);
	}

	/* Prompt for a new weapon */
	if (!(wep = getobj(wield_objs, E_J("wield",&wieldw), getobj_filter_wield)))
		/* Cancelled */
		return (0);
	else if (wep == uwep) {
	    You(E_J("are already wielding that!",
		    "すでにそれを構えている！"));
	    if (is_weptool(wep)) unweapon = FALSE;	/* [see setuwep()] */
		return (0);
	} else if (welded(uwep)) {
		weldmsg(uwep);
		/* previously interrupted armor removal mustn't be resumed */
		reset_remarm();
		return (0);
	}

	/* Handle no object, or object in other slot */
	if (wep == &zeroobj)
		wep = (struct obj *) 0;
	else if (wep == uswapwep)
		return (doswapweapon());
	else if (wep == uquiver)
		setuqwep((struct obj *) 0);
	else if (wep->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) {
		You(E_J("cannot wield that!","それを構えることはできない！"));
		return (0);
	}

	/* Set your new primary weapon */
	oldwep = uwep;
	result = ready_weapon(wep);
	if (flags.pushweapon && oldwep && uwep != oldwep)
		setuswapwep(oldwep);
	untwoweapon();

	return (result);
}

int
doswapweapon()
{
	register struct obj *oldwep, *oldswap;
	int result = 0;


	/* May we attempt this? */
	multi = 0;
	if (cantwield(youmonst.data)) {
		pline(E_J("Don't be ridiculous!",
			  "冗談はよしてください！"));
		return(0);
	}
	if (welded(uwep)) {
		weldmsg(uwep);
		return (0);
	}

	/* Unwield your current secondary weapon */
	oldwep = uwep;
	oldswap = uswapwep;
	setuswapwep((struct obj *) 0);

	/* Set your new primary weapon */
	result = ready_weapon(oldswap);

	/* Set your new secondary weapon */
	if (uwep == oldwep)
		/* Wield failed for some reason */
		setuswapwep(oldswap);
	else {
		setuswapwep(oldwep);
		if (uswapwep)
			prinv((char *)0, uswapwep, 0L);
		else
			You(E_J("have no secondary weapon readied.",
				"予備の武器を用意していない。"));
	}

	if (u.twoweap && !can_twoweapon())
		untwoweapon();

	return (result);
}

int
getobj_filter_wieldsub(otmp)
struct obj *otmp;
{
	if (otmp->owornmask & W_SWAPWEP)
		return GETOBJ_ADDELSE;
	if ((otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) &&
	    !(otmp->owornmask & W_WEP) && !is_ammo(otmp) && !is_missile(otmp))
		return GETOBJ_CHOOSEIT;
	return 0;
}
int
dowieldsubweapon()	/* when user do #twoweapon with uwep && !uswapwep */
{
	register struct obj *wep;
#ifdef JP
	static const struct getobj_words wieldsubw = {
	    0, "を副武器として", "装備する", "装備し"
	};
#endif /*JP*/

	/* May we attempt this? */
	multi = 0;
	if (cantwield(youmonst.data)) {
		pline(E_J("Don't be ridiculous!",
			  "冗談はよしてください！"));
		return(0);
	}
	/* Prompt for a new weapon */
	if (!(wep = getobj(wield_objs, E_J("wield as your secondary weapon",&wieldsubw), getobj_filter_wieldsub)))
		/* Cancelled */
		return (0);
	else if (wep == uwep) {
	    You(E_J("are already wielding that as your main weapon!",
		    "それをすでに主武器として構えている！"));
	    return (0);
	} else if (wep == uswapwep) {
	    You(E_J("have already selected that!",
		    "それをすでに準備済みだ！"));
	    return (0);
	}

	/* Handle no object, or object in other slot */
	if (wep == &zeroobj)
		wep = (struct obj *) 0;
	else if (wep == uquiver)
		setuqwep((struct obj *) 0);
	else if (wep->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) {
		You("cannot wield that!");
		return (0);
	}

//	if (Glib || uswapwep->cursed) {
//		if (!Glib)
//			uswapwep->bknown = TRUE;
//		drop_uswapwep();
//		return (1);
//	}

	/* Set your new secondary weapon */
	setuswapwep(wep);

	return (1);
}

int
dowieldquiver()
{
	register struct obj *newquiver;
	boolean slinging = (uslinging() || (uswapwep && uswapwep->otyp == SLING));
#ifdef JP
	static const struct getobj_words quivw = { 0, 0, "矢筒に準備する", "矢筒に準備し" };
#endif /*JP*/

	/* Since the quiver isn't in your hands, don't check cantwield(), */
	/* will_weld(), touch_petrifies(), etc. */
	multi = 0;

	/* Because 'Q' used to be quit... */
	if (flags.suppress_alert < FEATURE_NOTICE_VER(3,3,0))
		pline(E_J("Note: Please use #quit if you wish to exit the game.",
			  "メモ: ゲームを放棄するには #quit を使用してください。"));

	/* Prompt for a new quiver */
	if (!(newquiver = getobj(slinging ? bullets : ready_objs, E_J("ready",&quivw),
				 slinging ? getobj_filter_ready2 : getobj_filter_ready1)))
		/* Cancelled */
		return (0);

	/* Handle no object, or object in other slot */
	/* Any type is okay, since we give no intrinsics anyways */
	if (newquiver == &zeroobj) {
		/* Explicitly nothing */
		if (uquiver) {
			You(E_J("now have no ammunition readied.",
				"矢筒を空にした。"));
			setuqwep(newquiver = (struct obj *) 0);
		} else {
#ifndef JP
			You("already have no ammunition readied!");
#else
			Your("矢筒はすでに空だ！");
#endif /*JP*/
			return(0);
		}
	} else if (newquiver == uquiver) {
		pline(E_J("That ammunition is already readied!",
			  "それはすでに矢筒に入っている！"));
		return(0);
	} else if (newquiver == uwep) {
		/* Prevent accidentally readying the main weapon */
#ifndef JP
		pline("%s already being used as a weapon!",
		      !is_plural(uwep) ? "That is" : "They are");
#else
		pline("それはすでに武器として構えている！");
#endif /*JP*/
		return(0);
	} else if (newquiver->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) {
#ifndef JP
		You("cannot ready that!");
#else
		pline("それは矢筒には入らない！");
#endif /*JP*/
		return (0);
	} else {
		long dummy;


		/* Check if it's the secondary weapon */
		if (newquiver == uswapwep) {
			setuswapwep((struct obj *) 0);
			untwoweapon();
		}

		/* Okay to put in quiver; print it */
		dummy = newquiver->owornmask;
		newquiver->owornmask |= W_QUIVER;
		prinv((char *)0, newquiver, 0L);
		newquiver->owornmask = dummy;
	}

	/* Finally, place it in the quiver */
	setuqwep(newquiver);
	/* Take no time since this is a convenience slot */
	return (0);
}

/* used for #rub and for applying pick-axe, whip, grappling hook, or polearm */
/* (moved from apply.c) */
boolean
wield_tool(obj, verb)
struct obj *obj;
const char *verb;	/* "rub",&c */
{
    const char *what;
    boolean more_than_1;

    if (obj == uwep) return TRUE;   /* nothing to do if already wielding it */

    if (!verb) verb = E_J("wield","構える");
    what = xname(obj);
#ifndef JP
    more_than_1 = (obj->quan > 1L ||
		   strstri(what, "pair of ") != 0 ||
		   strstri(what, "s of ") != 0);
#endif /*JP*/

    if (obj->owornmask & (W_ARMOR|W_RING|W_AMUL|W_TOOL)) {
	char yourbuf[BUFSZ];

#ifndef JP
	You_cant("%s %s %s while wearing %s.",
		 verb, shk_your(yourbuf, obj), what,
		 more_than_1 ? "them" : "it");
#else
	You("身に着けているものを%sことはできない。", verb);
#endif /*JP*/
	return FALSE;
    }
    if (welded(uwep)) {
	if (flags.verbose) {
	    const char *hand = body_part(HAND);

#ifndef JP
	    if (bimanual(uwep)) hand = makeplural(hand);
	    if (strstri(what, "pair of ") != 0) more_than_1 = FALSE;
	    pline(
	     "Since your weapon is welded to your %s, you cannot %s %s %s.",
		  hand, verb, more_than_1 ? "those" : "that", xname(obj));
#else
	    pline("武器があなたの%s%sに貼り付いているため、あなたは%sを%sことができない。",
		  bimanual(uwep) ? "両" : "", hand, xname(obj), verb);
#endif /*JP*/
	} else {
	    E_J(You_cant("do that."), pline("それはできない。"));
	}
	return FALSE;
    }
    if (cantwield(youmonst.data)) {
#ifndef JP
	You_cant("hold %s strongly enough.", more_than_1 ? "them" : "it");
#else
	You("それをしっかりと握ることができない。");
#endif /*JP*/
	return FALSE;
    }
    /* check shield */
    if (uarms && bimanual(obj)) {
#ifndef JP
	You("cannot %s a two-handed %s while wearing a shield.",
	    verb, (obj->oclass == WEAPON_CLASS) ? "weapon" : "tool");
#else
	pline("盾を装備していては、両手持ちの%sを%sことはできない。",
		(obj->oclass == WEAPON_CLASS) ? "武器" : "道具", verb);
#endif /*JP*/
	return FALSE;
    }
    if (uquiver == obj) setuqwep((struct obj *)0);
    if (uswapwep == obj) {
	(void) doswapweapon();
	/* doswapweapon might fail */
	if (uswapwep == obj) return FALSE;
    } else {
	You(E_J("now wield %s.","%sを装備した。"), doname(obj));
	setuwep(obj);
    }
    if (uwep != obj) return FALSE;	/* rewielded old object after dying */
    /* applying weapon or tool that gets wielded ends two-weapon combat */
    if (u.twoweap)
	untwoweapon();
    if (obj->oclass != WEAPON_CLASS)
	unweapon = TRUE;
    return TRUE;
}

int
can_twoweapon()
{
	struct obj *otmp;

#define NOT_WEAPON(obj) (!is_weptool(obj) && obj->oclass != WEAPON_CLASS)
	if (!could_twoweap(youmonst.data)) {
		if (Upolyd)
		    You_cant(E_J("use two weapons in your current form.",
				 "今の姿では二刀流はできない。"));
		else
#ifndef JP
		    pline("%s aren't able to use two weapons at once.",
			  makeplural((flags.female && urole.name.f) ?
				     urole.name.f : urole.name.m));
#else
		    pline("%sは一度に二つの武器を振るえない。",
			  (flags.female && urole.name.f) ?
				urole.name.f : urole.name.m);
#endif /*JP*/
	} else if (!uwep || !uswapwep)
#ifndef JP
		Your("%s%s%s empty.", uwep ? "left " : uswapwep ? "right " : "",
			body_part(HAND), (!uwep && !uswapwep) ? "s are" : " is");
#else
		Your("%s%sは空いている。", uwep ? "逆" : uswapwep ? "利き" : "両",
			body_part(HAND));
#endif /*JP*/
	else if (NOT_WEAPON(uwep) || NOT_WEAPON(uswapwep)) {
		otmp = NOT_WEAPON(uwep) ? uwep : uswapwep;
#ifndef JP
		pline("%s %s.", Yname2(otmp),
		    is_plural(otmp) ? "aren't weapons" : "isn't a weapon");
#else
		pline("%sは武器ではない。", yname(otmp));
#endif /*JP*/
	} else if (bimanual(uwep) || bimanual(uswapwep)) {
		otmp = bimanual(uwep) ? uwep : uswapwep;
#ifndef JP
		pline("%s isn't one-handed.", Yname2(otmp));
#else
		pline("%sは片手では持てない。", yname(otmp));
#endif /*JP*/
	} else if (objects[uwep->otyp    ].oc_skill == P_BOW_GROUP ||
		   objects[uswapwep->otyp].oc_skill == P_BOW_GROUP) {
		otmp = bimanual(uwep) ? uwep : uswapwep;
#ifndef JP
		You_cant("use a bow while wielding a weapon in the other %s.", body_part(HAND));
#else
		You("武器と弓を同時には使えない。");
#endif /*JP*/
	} else if (uarms)
#ifndef JP
		You_cant("use two weapons while wearing a shield.");
#else
		pline("盾を構えているため、二つの武器を持つことはできない。");
#endif /*JP*/
	else if (uswapwep->oartifact)
#ifndef JP
		pline("%s %s being held second to another weapon!",
			Yname2(uswapwep), otense(uswapwep, "resist"));
#else
		pline("%sは他の武器の下につくことを良しとしない！",
			yname(uswapwep));
#endif /*JP*/
	else if (!uarmg && !Stone_resistance && (uswapwep->otyp == CORPSE &&
		    touch_petrifies(&mons[uswapwep->corpsenm]))) {
		char kbuf[BUFSZ];

#ifndef JP
		You("wield the %s corpse with your bare %s.",
		    mons[uswapwep->corpsenm].mname, body_part(HAND));
		Sprintf(kbuf, "%s corpse", an(mons[uswapwep->corpsenm].mname));
#else
		You("%sの死体を素%sで持ってしまった。",
		    JMONNAM(uswapwep->corpsenm), body_part(HAND));
		Sprintf(kbuf, "%sの死体で", JMONNAM(uswapwep->corpsenm));
#endif /*JP*/
		instapetrify(kbuf);
//	} else if (Glib || uswapwep->cursed) {
//		if (!Glib)
//			uswapwep->bknown = TRUE;
//		drop_uswapwep();
	} else
		return (TRUE);
	return (FALSE);
}

void
drop_uswapwep()
{
	char str[BUFSZ];
	struct obj *obj = uswapwep;

	/* Avoid trashing makeplural's static buffer */
#ifndef JP
	Strcpy(str, makeplural(body_part(HAND)));
	Your("%s from your %s!",  aobjnam(obj, "slip"), str);
#else
	pline("%sがあなたの%sから滑り落ちた！",  xname(obj), body_part(HAND));
#endif /*JP*/
	dropx(obj);
}

int
dotwoweapon()
{
	struct obj *oldswep = (struct obj *)0;
	boolean selswep = FALSE;

	/* You can always toggle it off */
	if (u.twoweap) {
		You(E_J("switch to your primary weapon.",
			"利き手の武器での戦闘に切り替えた。"));
		u.twoweap = 0;
		update_inventory();
		return (0);
	}

	/**/
	if (could_twoweap(youmonst.data) &&
	    uwep && !bimanual(uwep) &&
	    !uswapwep) {
		selswep = TRUE;
		oldswep = uswapwep;
		if (!dowieldsubweapon()) return(0);
	}

	/* May we use two weapons? */
	if (can_twoweapon()) {
		/* Success! */
		You(E_J("begin two-weapon combat.",
			"二刀流で戦闘を始めた。"));
		u.twoweap = 1;
		update_inventory();
		return (rnd(20) > ACURR(A_DEX));
	} else {
		if (selswep) setuswapwep(oldswep);
	}
	return (0);
}

/*** Functions to empty a given slot ***/
/* These should be used only when the item can't be put back in
 * the slot by life saving.  Proper usage includes:
 * 1.  The item has been eaten, stolen, burned away, or rotted away.
 * 2.  Making an item disappear for a bones pile.
 */
void
uwepgone()
{
	if (uwep) {
		if (artifact_light(uwep) && uwep->lamplit) {
		    end_burn(uwep, FALSE);
#ifndef JP
		    if (!Blind) pline("%s glowing.", Tobjnam(uwep, "stop"));
#else
		    if (!Blind) pline("%sから光が消えた。", xname(uwep));
#endif /*JP*/
		}
		setworn((struct obj *)0, W_WEP);
		unweapon = TRUE;
		update_inventory();
	}
}

void
uswapwepgone()
{
	if (uswapwep) {
		setworn((struct obj *)0, W_SWAPWEP);
		update_inventory();
	}
}

void
uqwepgone()
{
	if (uquiver) {
		setworn((struct obj *)0, W_QUIVER);
		update_inventory();
	}
}

void
untwoweapon()
{
	if (u.twoweap) {
		You(E_J("can no longer use two weapons at once.",
			"同時に二つの武器を振るうことができなくなった。"));
		u.twoweap = FALSE;
		update_inventory();
	}
	return;
}

/* Maybe rust object, or corrode it if acid damage is called for */
void
erode_obj(target, acid_dmg, fade_scrolls)
struct obj *target;		/* object (e.g. weapon or armor) to erode */
boolean acid_dmg;
boolean fade_scrolls;
{
	int erosion;
	struct monst *victim;
	boolean vismon;
	boolean visobj;

	if (!target)
	    return;
	victim = carried(target) ? &youmonst :
	    mcarried(target) ? target->ocarry : (struct monst *)0;
	vismon = victim && (victim != &youmonst) && canseemon(victim);
	visobj = !victim && cansee(bhitpos.x, bhitpos.y); /* assume thrown */

	erosion = acid_dmg ? target->oeroded2 : target->oeroded;

	if (target->greased) {
	    grease_protect(target,(char *)0,victim);
	} else if (target->oclass == SCROLL_CLASS) {
	    if(fade_scrolls && target->otyp != SCR_BLANK_PAPER
#ifdef MAIL
	    && target->otyp != SCR_MAIL
#endif
					)
	    {
		if (!Blind) {
		    if (victim == &youmonst)
#ifndef JP
			Your("%s.", aobjnam(target, "fade"));
#else
			Your("%sの文字はにじんで消えた。", xname(target));
#endif /*JP*/
		    else if (vismon)
#ifndef JP
			pline("%s's %s.", Monnam(victim),
			      aobjnam(target, "fade"));
#else
			pline("%sの%sの文字はにじんで消えた。",
			      mon_nam(victim), xname(target));
#endif /*JP*/
		    else if (visobj)
#ifndef JP
			pline_The("%s.", aobjnam(target, "fade"));
#else
			pline("%sの文字はにじんで消えた。", xname(target));
#endif /*JP*/
		}
		target->otyp = SCR_BLANK_PAPER;
		target->spe = 0;
	    }
	} else rust_dmg(target, xname(target), (acid_dmg ? 3/*corrode*/ : 1/*rust*/), TRUE, victim);
}

int
chwepon(otmp, amount)
register struct obj *otmp;
register int amount;
{
	const char *color = hcolor((amount < 0) ? NH_BLACK : NH_BLUE);
	const char *xtime;
	int otyp = STRANGE_OBJECT;

	if(!uwep || (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))) {
		char buf[BUFSZ];

#ifndef JP
		Sprintf(buf, "Your %s %s.", makeplural(body_part(HAND)),
			(amount >= 0) ? "twitch" : "itch");
#else
		Sprintf(buf, "あなたの%sが%sた。", body_part(HAND),
			(amount >= 0) ? "痙攣し" : "痒くなっ");
#endif /*JP*/
		strange_feeling(otmp, buf);
		exercise(A_DEX, (boolean) (amount >= 0));
		return(0);
	}

	if (otmp && otmp->oclass == SCROLL_CLASS) otyp = otmp->otyp;

	if(uwep->otyp == WORM_TOOTH && amount >= 0) {
		uwep->otyp = CRYSKNIFE;
		uwep->oerodeproof = 0;
		Your(E_J("weapon seems sharper now.",
			 "武器はより鋭くなったようだ。"));
		uwep->cursed = 0;
		if (otyp != STRANGE_OBJECT) makeknown(otyp);
		return(1);
	}

	if(uwep->otyp == CRYSKNIFE && amount < 0) {
		uwep->otyp = WORM_TOOTH;
		uwep->oerodeproof = 0;
		Your(E_J("weapon seems duller now.",
			 "武器はより鈍くなったようだ。"));
		if (otyp != STRANGE_OBJECT && otmp->bknown) makeknown(otyp);
		return(1);
	}

	if (amount < 0 && uwep->oartifact && restrict_name(uwep, ONAME(uwep))) {
	    if (!Blind)
#ifndef JP
		Your("%s %s.", aobjnam(uwep, "faintly glow"), color);
#else
		Your("%sはかすかに%s輝いた。", xname(uwep), j_no_ni(color));
#endif /*JP*/
	    return(1);
	}
	/* there is a (soft) upper and lower limit to uwep->spe */
	if(((uwep->spe > WEP_ENCHANT_MAX && amount >= 0) ||
	    (uwep->spe < -WEP_ENCHANT_MAX && amount < 0)) && rn2(3)) {
	    if (!Blind)
#ifndef JP
	    Your("%s %s for a while and then %s.",
		 aobjnam(uwep, "violently glow"), color,
		 otense(uwep, "evaporate"));
#else
	    Your("%sは強烈な%s光を放つと、蒸発した。",
		 xname(uwep), color);
#endif /*JP*/
	    else
#ifndef JP
		Your("%s.", aobjnam(uwep, "evaporate"));
#else
		Your("%sは蒸発した。", xname(uwep));
#endif /*JP*/

	    useupall(uwep);	/* let all of them disappear */
	    return(1);
	}
	if (!Blind) {
	    xtime = (amount*amount == 1) ? E_J("moment","一瞬") : E_J("while","しばらくの間");
#ifndef JP
	    Your("%s %s for a %s.",
		 aobjnam(uwep, amount == 0 ? "violently glow" : "glow"),
		 color, xtime);
#else
	    Your("%sは%s%s%s輝いた。",
		 xname(uwep), xtime, amount == 0 ? "激しく" : "", j_no_ni(color));
#endif /*JP*/
	    if (otyp != STRANGE_OBJECT && uwep->known &&
		    (amount > 0 || (amount < 0 && otmp->bknown)))
		makeknown(otyp);
	}
	uwep->spe += amount;
	if(amount > 0) uwep->cursed = 0;

	/*
	 * Enchantment, which normally improves a weapon, has an
	 * addition adverse reaction on Magicbane whose effects are
	 * spe dependent.  Give an obscure clue here.
	 */
	if (uwep->oartifact == ART_MAGICBANE && uwep->spe >= 0) {
#ifndef JP
		Your("right %s %sches!",
			body_part(HAND),
			(((amount > 1) && (uwep->spe > 1)) ? "flin" : "it"));
#else
		Your("利き%sが%s！",
			body_part(HAND),
			(((amount > 1) && (uwep->spe > 1)) ? "たじろいだ" : "痒くなった"));
#endif /*JP*/
	}

	/* an elven magic clue, cookie@keebler */
	/* elven weapons vibrate warningly when enchanted beyond a limit */
	if ((uwep->spe > WEP_ENCHANT_WARN)
		&& (is_elven_weapon(uwep) || uwep->oartifact || !rn2(7)))
#ifndef JP
	    Your("%s unexpectedly.",
		aobjnam(uwep, "suddenly vibrate"));
#else
	    Your("%sは不意に震えた。", xname(uwep));
#endif /*JP*/

	return(1);
}

int
welded(obj)
register struct obj *obj;
{
	if (obj && obj == uwep && will_weld(obj)) {
		obj->bknown = TRUE;
		return 1;
	}
	return 0;
}

void
weldmsg(obj)
register struct obj *obj;
{
	long savewornmask;

	savewornmask = obj->owornmask;
#ifndef JP
	Your("%s %s welded to your %s!",
		xname(obj), otense(obj, "are"),
		bimanual(obj) ? (const char *)makeplural(body_part(HAND))
				: body_part(HAND));
#else
	pline("%sがあなたの%s%sに張り付いている！",
		xname(obj),
		bimanual(obj) ? "両" : "",
		body_part(HAND));
#endif /*JP*/
	obj->owornmask = savewornmask;
}

/*wield.c*/
