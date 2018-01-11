/*	SCCS Id: @(#)eat.c	3.4	2003/02/13	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
/* #define DEBUG */	/* uncomment to enable new eat code debugging */

#ifdef DEBUG
# ifdef WIZARD
#define debugpline	if (wizard) pline
# else
#define debugpline	pline
# endif
#endif

STATIC_PTR int NDECL(eatmdone);
STATIC_PTR int NDECL(eatfood);
STATIC_PTR void FDECL(costly_tin, (const char*));
STATIC_PTR int NDECL(opentin);
STATIC_PTR int NDECL(unfaint);

#ifdef OVLB
STATIC_DCL const char *FDECL(food_xname, (struct obj *,BOOLEAN_P));
STATIC_DCL void FDECL(choke, (struct obj *));
STATIC_DCL void NDECL(recalc_wt);
STATIC_DCL struct obj *FDECL(touchfood, (struct obj *));
STATIC_DCL void NDECL(do_reset_eat);
STATIC_DCL void FDECL(done_eating, (BOOLEAN_P));
STATIC_DCL void FDECL(cprefx, (int));
STATIC_DCL int FDECL(intrinsic_possible, (int,struct permonst *));
STATIC_DCL void FDECL(givit, (int,struct permonst *));
STATIC_DCL void FDECL(cpostfx, (int));
STATIC_DCL void FDECL(start_tin, (struct obj *));
STATIC_DCL int FDECL(eatcorpse, (struct obj *));
STATIC_DCL void FDECL(start_eating, (struct obj *));
STATIC_DCL void FDECL(fprefx, (struct obj *));
STATIC_DCL void FDECL(accessory_has_effect, (struct obj *));
STATIC_DCL void FDECL(fpostfx, (struct obj *));
STATIC_DCL int NDECL(bite);
STATIC_DCL int FDECL(edibility_prompts, (struct obj *));
STATIC_DCL int FDECL(rottenfood, (struct obj *));
STATIC_DCL void NDECL(eatspecial);
STATIC_DCL void FDECL(eataccessory, (struct obj *));
STATIC_DCL const char *FDECL(foodword, (struct obj *));
STATIC_DCL boolean FDECL(maybe_cannibal, (int,BOOLEAN_P));
STATIC_DCL int FDECL(getobj_filter_eat, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_tin, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_sacrifice, (struct obj *));
STATIC_DCL boolean FDECL(isstoneorslime, (int));
STATIC_DCL int FDECL(check_food_conduct, (struct obj *));

char msgbuf[BUFSZ];

char *stoptingle = E_J("%s stops tingling and your sense of smell returns to normal.",
		       "%s�̂������������A���Ȃ��̚k�o�͒ʏ�̏�Ԃɖ߂����B");

#endif /* OVLB */

STATIC_DCL void FDECL(set_rotten_flag, (struct obj *));

/* hunger texts used on bottom line (each 8 chars long) */
#define SATIATED	0
#define NOT_HUNGRY	1
#define HUNGRY		2
#define WEAK		3
#define FAINTING	4
#define FAINTED		5
#define STARVED		6

/* also used to see if you're allowed to eat cats and dogs */
#define CANNIBAL_ALLOWED() (Role_if(PM_CAVEMAN) || Race_if(PM_ORC))

#ifndef OVLB

STATIC_DCL NEARDATA const char comestibles[];
STATIC_DCL NEARDATA const char allobj[];
STATIC_DCL boolean force_save_hs;

#else

STATIC_OVL NEARDATA const char comestibles[] = { FOOD_CLASS, 0 };

/* Gold must come first for getobj(). */
STATIC_OVL NEARDATA const char allobj[] = {
	COIN_CLASS, WEAPON_CLASS, ARMOR_CLASS, POTION_CLASS, SCROLL_CLASS,
	WAND_CLASS, RING_CLASS, AMULET_CLASS, FOOD_CLASS, TOOL_CLASS,
	GEM_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, SPBOOK_CLASS, 0 };

STATIC_OVL boolean force_save_hs = FALSE;

const char *hu_stat[] = {
	E_J("Satiated", "����"),
	    "        ",
	E_J("Hungry  ", "��"),
	E_J("Weak    ", "����"),
	E_J("Fainting", "�Q��"),
	E_J("Fainted ", "���|"),
	E_J("Starved ", "�쎀")
};

#endif /* OVLB */
#ifdef OVL1

/*
 * Decide whether a particular object can be eaten by the possibly
 * polymorphed character.  Not used for monster checks.
 */
boolean
is_edible(obj)
register struct obj *obj;
{
	/* protect invocation tools but not Rider corpses (handled elsewhere)*/
     /* if (obj->oclass != FOOD_CLASS && obj_resists(obj, 0, 0)) */
	if (objects[obj->otyp].oc_unique)
		return FALSE;
	/* above also prevents the Amulet from being eaten, so we must never
	   allow fake amulets to be eaten either [which is already the case] */

	if (metallivorous(youmonst.data) && is_metallic(obj) &&
	    (youmonst.data != &mons[PM_RUST_MONSTER] || is_rustprone(obj)))
		return TRUE;
	if (u.umonnum == PM_GELATINOUS_CUBE && is_organic(obj) &&
		/* [g.cubes can eat containers and retain all contents
		    as engulfed items, but poly'd player can't do that] */
	    !Has_contents(obj))
		return TRUE;
	/* a sheaf of straw is VEGGY, but only edible for herbivorous animals */
	if (obj->otyp == SHEAF_OF_STRAW && herbivorous(youmonst.data))
		return !carnivorous(youmonst.data);
	if (herbivorous(youmonst.data) && get_material(obj) == VEGGY)
		return TRUE;

     /* return((boolean)(!!index(comestibles, obj->oclass))); */
	return (boolean)(obj->oclass == FOOD_CLASS);
}

#endif /* OVL1 */
#ifdef OVLB

void
init_uhunger()
{
	u.uhunger = 900;
	u.uhs = NOT_HUNGRY;
}

static const struct { const char *txt; int nut; } tintxts[] = {
	{E_J("deep fried",	"%s�̓��g��"),		 60},
	{E_J("pickled",		"%s�̐|�Ђ�"),		 40},
	{E_J("soup made from",	"%s�̃X�[�v"),		 20},
	{E_J("pureed",		"%s�̃s���[��"),	500},
#define ROTTEN_TIN 4
	{E_J("rotten",		"������%s�̊ʋl"),	-50},
#define HOMEMADE_TIN 5
	{E_J("homemade",	"���Ɛ���%s�̊ʋl"),	 50},
	{E_J("stir fried",	"%s���u�ߕ�"),		 80},
	{E_J("candied",		"%s�̊ØI��"),		100},
	{E_J("boiled",		"%s�̎ϕ�"),		 50},
	{E_J("dried",		"%s�̊���"),		 55},
	{E_J("szechuan",	"�l�앗%s"),		 70},
#define FRENCH_FRIED_TIN 11
	{E_J("french fried",	"%s�̃t���C"),		 40},
	{E_J("sauteed",		"%s�̃\�e�["),		 95},
	{E_J("broiled",		"%s�̒Y�ΏĂ�"),	 80},
	{E_J("smoked",		"%s������"),		 50},
	{"", 0}
};
#define TTSZ	SIZE(tintxts)

static NEARDATA struct {
	struct	obj *tin;
	int	usedtime, reqtime;
} tin;

static NEARDATA struct {
	struct	obj *piece;	/* the thing being eaten, or last thing that
				 * was partially eaten, unless that thing was
				 * a tin, which uses the tin structure above,
				 * in which case this should be 0 */
	/* doeat() initializes these when piece is valid */
	int	usedtime,	/* turns spent eating */
		reqtime;	/* turns required to eat */
	int	nmod;		/* coded nutrition per turn */
	Bitfield(canchoke,1);	/* was satiated at beginning */

	/* start_eating() initializes these */
	Bitfield(fullwarn,1);	/* have warned about being full */
	Bitfield(eating,1);	/* victual currently being eaten */
	Bitfield(doreset,1);	/* stop eating at end of turn */
	Bitfield(tainted,1);	/* food(corpse) is tainted */
	Bitfield(rotsick,1);	/* food(corpse) will make you sick */
	Bitfield(rotten,1);	/* food(corpse) is rotten */
	Bitfield(poison,1);	/* food(corpse) is poisonous */
} victual;

static char *eatmbuf = 0;	/* set by cpostfx() */

STATIC_PTR
int
eatmdone()		/* called after mimicing is over */
{
	/* release `eatmbuf' */
	if (eatmbuf) {
	    if (nomovemsg == eatmbuf) nomovemsg = 0;
	    free((genericptr_t)eatmbuf),  eatmbuf = 0;
	}
	/* update display */
	if (youmonst.m_ap_type) {
	    youmonst.m_ap_type = M_AP_NOTHING;
	    newsym(u.ux,u.uy);
	}
	return 0;
}

/* ``[the(] singular(food, xname) [)]'' with awareness of unique monsters */
STATIC_OVL const char *
food_xname(food, the_pfx)
struct obj *food;
boolean the_pfx;
{
	const char *result;
	int mnum = food->corpsenm;

#ifndef JP
	if (food->otyp == CORPSE && (mons[mnum].geno & G_UNIQ)) {
	    /* grab xname()'s modifiable return buffer for our own use */
	    char *bufp = xname(food);
	    Sprintf(bufp, "%s%s corpse",
		    (the_pfx && !type_is_pname(&mons[mnum])) ? "the " : "",
		    s_suffix(mons[mnum].mname));
	    result = bufp;
	} else {
	    /* the ordinary case */
	    result = singular(food, xname);
	    if (the_pfx) result = the(result);
	}
#else
	result = singular(food, xname);
#endif /*JP*/
	return result;
}

/* Created by GAN 01/28/87
 * Amended by AKP 09/22/87: if not hard, don't choke, just vomit.
 * Amended by 3.  06/12/89: if not hard, sometimes choke anyway, to keep risk.
 *		  11/10/89: if hard, rarely vomit anyway, for slim chance.
 */
STATIC_OVL void
choke(food)	/* To a full belly all food is bad. (It.) */
	register struct obj *food;
{
	/* only happens if you were satiated */
	if (u.uhs != SATIATED) {
		if (!food || food->otyp != AMULET_OF_STRANGULATION)
			return;
	} else if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL) {
			adjalign(-1);		/* gluttony is unchivalrous */
			You_feel(E_J("like a glutton!","�\�H�̋���Ƃ����C�������I"));
	}

	exercise(A_CON, FALSE);

	if (Breathless || (!Strangled && !rn2(20))) {
		/* choking by eating AoS doesn't involve stuffing yourself */
		if (food && food->otyp == AMULET_OF_STRANGULATION) {
			You(E_J("choke, but recover your composure.",
				"�A���l�܂点�����A���Â����߂����B"));
			return;
		}
		You(E_J("stuff yourself and then vomit voluminously.",
			"�������ς��ɋl�ߍ��񂾌�A��ʂɚq�f�����B"));
		morehungry(1000);	/* you just got *very* sick! */
		nomovemsg = 0;
		vomit();
	} else {
		killer_format = KILLED_BY_AN;
		/*
		 * Note all "killer"s below read "Choked on %s" on the
		 * high score list & tombstone.  So plan accordingly.
		 */
		if(food) {
			You(E_J("choke over your %s.", "%s���A�ɋl�܂点���B"), foodword(food));
			if (food->oclass == COIN_CLASS) {
				killer = E_J("a very rich meal","�ƂĂ������ȐH����");
			} else {
				killer = food_xname(food, FALSE);
#ifndef JP
				if (food->otyp == CORPSE &&
				    (mons[food->corpsenm].geno & G_UNIQ)) {
				    if (!type_is_pname(&mons[food->corpsenm]))
					killer = the(killer);
				    killer_format = KILLED_BY;
				}
#endif /*JP*/
			}
		} else {
			You(E_J("choke over it.","�A���l�܂点���B"));
			killer = E_J("quick snack","�܂ݐH��������");
		}
		You(E_J("die...","���ɂ܂����c�B"));
		done(CHOKING);
	}
}

/* modify object wt. depending on time spent consuming it */
STATIC_OVL void
recalc_wt()
{
	struct obj *piece = victual.piece;

#ifdef DEBUG
	debugpline("Old weight = %d", piece->owt);
	debugpline("Used time = %d, Req'd time = %d",
		victual.usedtime, victual.reqtime);
#endif
	piece->owt = weight(piece);
#ifdef DEBUG
	debugpline("New weight = %d", piece->owt);
#endif
}

void
reset_eat()		/* called when eating interrupted by an event */
{
    /* we only set a flag here - the actual reset process is done after
     * the round is spent eating.
     */
	if(victual.eating && !victual.doreset) {
#ifdef DEBUG
	    debugpline("reset_eat...");
#endif
	    victual.doreset = TRUE;
	}
	return;
}

STATIC_OVL struct obj *
touchfood(otmp)
register struct obj *otmp;
{
	if (otmp->quan > 1L) {
	    if(!carried(otmp))
		(void) splitobj(otmp, otmp->quan - 1L);
	    else
		otmp = splitobj(otmp, 1L);
#ifdef DEBUG
	    debugpline("split object,");
#endif
	}

	if (otmp->oclass == FOOD_CLASS && !otmp->oeaten) {
	    if(((!carried(otmp) && costly_spot(otmp->ox, otmp->oy) &&
		 !otmp->no_charge)
		 || otmp->unpaid)) {
		/* create a dummy duplicate to put on bill */
		verbalize(E_J("You bit it, you bought it!",
			      "�����������͔̂����Ă��炤��I"));
		bill_dummy_object(otmp);
	    }
	    otmp->oeaten = (otmp->otyp == CORPSE ?
				mons[otmp->corpsenm].cnutrit :
				objects[otmp->otyp].oc_nutrition);
	}

	if (carried(otmp)) {
	    freeinv(otmp);
	    if (inv_cnt() >= 52) {
		sellobj_state(SELL_DONTSELL);
		dropy(otmp);
		sellobj_state(SELL_NORMAL);
	    } else {
		otmp->oxlth++;		/* hack to prevent merge */
		otmp = addinv(otmp);
		otmp->oxlth--;
	    }
	}
	return(otmp);
}

/* When food decays, in the middle of your meal, we don't want to dereference
 * any dangling pointers, so set it to null (which should still trigger
 * do_reset_eat() at the beginning of eatfood()) and check for null pointers
 * in do_reset_eat().
 */
void
food_disappears(obj)
register struct obj *obj;
{
	if (obj == victual.piece) victual.piece = (struct obj *)0;
	if (obj->timed) obj_stop_timers(obj);
}

/* renaming an object usually results in it having a different address;
   so the sequence start eating/opening, get interrupted, name the food,
   resume eating/opening would restart from scratch */
void
food_substitution(old_obj, new_obj)
struct obj *old_obj, *new_obj;
{
	if (old_obj == victual.piece) victual.piece = new_obj;
	if (old_obj == tin.tin) tin.tin = new_obj;
}

STATIC_OVL void
do_reset_eat()
{
#ifdef DEBUG
	debugpline("do_reset_eat...");
#endif
	if (victual.piece) {
		victual.piece = touchfood(victual.piece);
		recalc_wt();
	}
	victual.fullwarn = victual.eating = victual.doreset = FALSE;
	/* Do not set canchoke to FALSE; if we continue eating the same object
	 * we need to know if canchoke was set when they started eating it the
	 * previous time.  And if we don't continue eating the same object
	 * canchoke always gets recalculated anyway.
	 */
	stop_occupation();
	newuhs(FALSE);
}

STATIC_PTR
int
eatfood()		/* called each move during eating process */
{
	if(!victual.piece ||
	 (!carried(victual.piece) && !obj_here(victual.piece, u.ux, u.uy))) {
		/* maybe it was stolen? */
		do_reset_eat();
		return(0);
	}
	if(!victual.eating) return(0);

	if(++victual.usedtime <= victual.reqtime) {
	    if(bite()) return(0);
	    return(1);	/* still busy */
	} else {	/* done */
	    done_eating(TRUE);
	    return(0);
	}
}

STATIC_OVL void
done_eating(message)
boolean message;
{
	victual.piece->in_use = TRUE;
	occupation = 0; /* do this early, so newuhs() knows we're done */
	newuhs(FALSE);
	if (nomovemsg) {
		if (message) pline(nomovemsg);
		nomovemsg = 0;
	} else if (message)
		You(E_J("finish eating %s.","%s��H�׏I�����B"), food_xname(victual.piece, TRUE));

	if(victual.piece->otyp == CORPSE)
		cpostfx(victual.piece->corpsenm);
	else
		fpostfx(victual.piece);

	if (carried(victual.piece)) useup(victual.piece);
	else useupf(victual.piece, 1L);
	victual.piece = (struct obj *) 0;
	victual.fullwarn = victual.eating = victual.doreset = FALSE;
}

STATIC_OVL boolean
maybe_cannibal(pm, allowmsg)
int pm;
boolean allowmsg;
{
	if (!CANNIBAL_ALLOWED() && your_race(&mons[pm])) {
		if (allowmsg) {
			if (Upolyd)
#ifndef JP
				You("have a bad feeling deep inside.");
#else
				Your("�S�̉���ŋ������������B");
#endif /*JP*/
#ifndef JP
			You("cannibal!  You will regret this!");
#else
			pline("���H�����I�@������邼�I");
#endif /*JP*/
		}
		HAggravate_monster |= FROMOUTSIDE;
		change_luck(-rn1(4,2));		/* -5..-2 */
		return TRUE;
	}
	return FALSE;
}

STATIC_OVL void
cprefx(pm)
register int pm;
{
	(void) maybe_cannibal(pm,TRUE);
	if (touch_petrifies(&mons[pm]) || pm == PM_MEDUSA) {
	    if (!Stone_resistance &&
		!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
#ifndef JP
		Sprintf(killer_buf, "tasting %s meat", mons[pm].mname);
#else
		Sprintf(killer_buf, "%s�̓��𖡌�����", JMONNAM(pm));
#endif /*JP*/
		killer_format = KILLED_BY;
		killer = killer_buf;
		You(E_J("turn to stone.","�΂ɂȂ����B"));
		done(STONING);
		if (victual.piece)
		    victual.eating = FALSE;
		return; /* lifesaved */
	    }
	}

	switch(pm) {
	    case PM_LITTLE_DOG:
	    case PM_DOG:
	    case PM_LARGE_DOG:
	    case PM_KITTEN:
	    case PM_HOUSECAT:
	    case PM_LARGE_CAT:
		if (!CANNIBAL_ALLOWED()) {
#ifndef JP
		    You_feel("that eating the %s was a bad idea.", mons[pm].mname);
#else
		    pline("%s��H�ׂ�̂͗ǂ��Ȃ����Ƃ��Ǝv��ꂽ�B", JMONNAM(pm));
#endif
		    HAggravate_monster |= FROMOUTSIDE;
		}
		break;
	    case PM_LIZARD:
		if (Stoned) fix_petrification();
		break;
	    case PM_DEATH:
	    case PM_PESTILENCE:
	    case PM_FAMINE:
		{ char buf[BUFSZ];
		    pline(E_J("Eating that is instantly fatal.",
			      "�����H�ׂ邱�Ƃ͎����Ӗ�����B"));
#ifndef JP
		    Sprintf(buf, "unwisely ate the body of %s",
			    mons[pm].mname);
#else
		    Sprintf(buf, "�����ɂ�%s�̎��̂�H�ׂ�", JMONNAM(pm));
#endif /*JP*/
		    killer = buf;
		    killer_format = E_J(NO_KILLER_PREFIX,KILLED_BY);
		    done(DIED);
		    /* It so happens that since we know these monsters */
		    /* cannot appear in tins, victual.piece will always */
		    /* be what we want, which is not generally true. */
		    if (revive_corpse(victual.piece))
			victual.piece = (struct obj *)0;
		    return;
		}
	    case PM_GREEN_SLIME:
		if (!Slimed && !Unchanging && !flaming(youmonst.data) &&
			youmonst.data != &mons[PM_GREEN_SLIME]) {
		    You(E_J("don't feel very well.","�ƂĂ��C���������B"));
		    Slimed = 10L;
		    flags.botl = 1;
		}
		/* Fall through */
	    default:
		if (acidic(&mons[pm]) && Stoned)
		    fix_petrification();
		break;
	}
}

void
fix_petrification()
{
	Stoned = 0;
	delayed_killer = 0;
	if (Hallucination)
	    pline(E_J("What a pity - you just ruined a future piece of %sart!",
		      "�Ȃ�Ă����� �� �㐢�Ɏc��%s�|�p��i�ɂȂꂽ�̂ɁI"),
		  ACURR(A_CHA) > 15 ? E_J("fine ","�f���炵��") : "");
	else
	    You_feel(E_J("limber!","���_�炩���Ȃ����悤���I"));
}

/*
 * If you add an intrinsic that can be gotten by eating a monster, add it
 * to intrinsic_possible() and givit().  (It must already be in prop.h to
 * be an intrinsic property.)
 * It would be very easy to make the intrinsics not try to give you one
 * that you already had by checking to see if you have it in
 * intrinsic_possible() instead of givit().
 */

/* intrinsic_possible() returns TRUE iff a monster can give an intrinsic. */
STATIC_OVL int
intrinsic_possible(type, ptr)
int type;
register struct permonst *ptr;
{
	switch (type) {
	    case FIRE_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_FIRE) {
			debugpline("can get fire resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_FIRE);
#endif
	    case SLEEP_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_SLEEP) {
			debugpline("can get sleep resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_SLEEP);
#endif
	    case COLD_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_COLD) {
			debugpline("can get cold resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_COLD);
#endif
	    case DISINT_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_DISINT) {
			debugpline("can get disintegration resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_DISINT);
#endif
	    case SHOCK_RES:	/* shock (electricity) resistance */
#ifdef DEBUG
		if (ptr->mconveys & MR_ELEC) {
			debugpline("can get shock resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_ELEC);
#endif
	    case POISON_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_POISON) {
			debugpline("can get poison resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_POISON);
#endif
	    case TELEPORT:
#ifdef DEBUG
		if (can_teleport(ptr)) {
			debugpline("can get teleport");
			return(TRUE);
		} else  return(FALSE);
#else
		return(can_teleport(ptr));
#endif
	    case TELEPORT_CONTROL:
#ifdef DEBUG
		if (control_teleport(ptr)) {
			debugpline("can get teleport control");
			return(TRUE);
		} else  return(FALSE);
#else
		return(control_teleport(ptr));
#endif
	    case TELEPAT:
#ifdef DEBUG
		if (telepathic(ptr)) {
			debugpline("can get telepathy");
			return(TRUE);
		} else  return(FALSE);
#else
		return(telepathic(ptr));
#endif
	    default:
		return(FALSE);
	}
	/*NOTREACHED*/
}

/* givit() tries to give you an intrinsic based on the monster's level
 * and what type of intrinsic it is trying to give you.
 */
STATIC_OVL void
givit(type, ptr)
int type;
register struct permonst *ptr;
{
	register int chance;

#ifdef DEBUG
	debugpline("Attempting to give intrinsic %d", type);
#endif
	/* some intrinsics are easier to get than others */
	switch (type) {
		case POISON_RES:
			if ((ptr == &mons[PM_KILLER_BEE] ||
					ptr == &mons[PM_SCORPION]) && !rn2(4))
				chance = 1;
			else
				chance = 25;
			break;
		case TELEPORT:
			chance = 10;
			break;
		case TELEPORT_CONTROL:
			chance = 12;
			break;
		case TELEPAT:
			chance = 1;
			break;
		default:
			chance = 15;
			break;
	}

	if (ptr->mlevel <= rn2(chance))
		return;		/* failed die roll */

	switch (type) {
	    case FIRE_RES:
#ifdef DEBUG
		debugpline("Trying to give fire resistance");
#endif
		if(!(HFire_resistance & FROMOUTSIDE)) {
#ifndef JP
			You(Hallucination ? "be chillin'." :
			    "feel a momentary chill.");
#else /*JP*/
			if (Hallucination) pline("���񂽁A�N�[�������B");
			else You("��u�₽�����������B");
#endif /*JP*/
			HFire_resistance |= FROMOUTSIDE;
		}
		break;
	    case SLEEP_RES:
#ifdef DEBUG
		debugpline("Trying to give sleep resistance");
#endif
		if(!(HSleep_resistance & FROMOUTSIDE)) {
			You_feel(E_J("wide awake.","��������ڂ��o�߂��B"));
			HSleep_resistance |= FROMOUTSIDE;
		}
		break;
	    case COLD_RES:
#ifdef DEBUG
		debugpline("Trying to give cold resistance");
#endif
		if(!(HCold_resistance & FROMOUTSIDE)) {
			You_feel(E_J("full of hot air.","�S�g�ɔM�C���������B"));
			HCold_resistance |= FROMOUTSIDE;
		}
		break;
	    case DISINT_RES:
#ifdef DEBUG
		debugpline("Trying to give disintegration resistance");
#endif
		if(!(HDisint_resistance & FROMOUTSIDE)) {
			You_feel(Hallucination ?
			    E_J("totally together, man.","�z���g�Ɉ�ɂȂ����݂������B") :
			    E_J("very firm.","�ƂĂ����ɂȂ����悤���B"));
			HDisint_resistance |= FROMOUTSIDE;
		}
		break;
	    case SHOCK_RES:	/* shock (electricity) resistance */
#ifdef DEBUG
		debugpline("Trying to give shock resistance");
#endif
		if(!(HShock_resistance & FROMOUTSIDE)) {
			if (Hallucination)
				You_feel(E_J("grounded in reality.","���̓A�[�X����Ă���C�������B"));
			else
				Your(E_J("health currently feels amplified!",
					 "���N�͑�������Ă���悤���I"));
			HShock_resistance |= FROMOUTSIDE;
		}
		break;
	    case POISON_RES:
#ifdef DEBUG
		debugpline("Trying to give poison resistance");
#endif
		if(!(HPoison_resistance & FROMOUTSIDE)) {
			You_feel(Poison_resistance ?
				 E_J("especially healthy.","����߂Č��N�ɂȂ����C�������B") :
				 E_J("healthy.","���N�ɂȂ����C�������B"));
			HPoison_resistance |= FROMOUTSIDE;
		}
		break;
	    case TELEPORT:
#ifdef DEBUG
		debugpline("Trying to give teleport");
#endif
		if(!(HTeleportation & FROMOUTSIDE)) {
			You_feel(Hallucination ? E_J("diffuse.","����Ɋg�U���Ă����C�������B") :
			    E_J("very jumpy.","�ƂĂ����������Ȃ��Ȃ����B"));
			HTeleportation |= FROMOUTSIDE;
		}
		break;
	    case TELEPORT_CONTROL:
#ifdef DEBUG
		debugpline("Trying to give teleport control");
#endif
		if(!(HTeleport_control & FROMOUTSIDE)) {
			You_feel(Hallucination ?
			    E_J("centered in your personal space.","�����̃p�[�\�i���E�X�y�[�X�̒��S�ɂ���C�������B") :
			    E_J("in control of yourself.","�������g�𐧌�ł���悤�ɂȂ����C�������B"));
			HTeleport_control |= FROMOUTSIDE;
		}
		break;
	    case TELEPAT:
#ifdef DEBUG
		debugpline("Trying to give telepathy");
#endif
		if(!(HTelepat & FROMOUTSIDE)) {
			You_feel(Hallucination ?
			    E_J("in touch with the cosmos.","�F���̐_��ɐG�ꂽ�悤�ȋC�������B") :
			    E_J("a strange mental acuity.","���_���s�v�c�Ɖs�q�ɂȂ�̂��������B"));
			HTelepat |= FROMOUTSIDE;
			/* If blind, make sure monsters show up. */
			if (Blind) see_monsters();
		}
		break;
	    default:
#ifdef DEBUG
		debugpline("Tried to give an impossible intrinsic");
#endif
		break;
	}
}

STATIC_OVL void
cpostfx(pm)		/* called after completely consuming a corpse */
register int pm;
{
	register int tmp = 0;
	boolean catch_lycanthropy = FALSE;

	/* in case `afternmv' didn't get called for previously mimicking
	   gold, clean up now to avoid `eatmbuf' memory leak */
	if (eatmbuf) (void)eatmdone();

	switch(pm) {
	    case PM_NEWT:
		/* MRKR: "eye of newt" may give small magical energy boost */
		if (rn2(3) || 3 * u.uen <= 2 * u.uenmax) {
		    int old_uen = u.uen;
		    u.uen += rnd(3);
		    if (u.uen > u.uenmax) {
			if (!rn2(3)) u.uenmax++;
			u.uen = u.uenmax;
		    }
		    if (old_uen != u.uen) {
			    You_feel(E_J("a mild buzz.","�Â��ȍ��܂���������B"));
			    flags.botl = 1;
		    }
		}
		break;
/* STEHPEN WHITE'S NEW CODE */
	    case PM_WRAITH:
		switch(rn2(10)) {
		case 0:
			You(E_J("feel that was a bad idea.",
				"��߂Ă����ׂ��������ƋC�Â����B"));
			losexp(E_J("life drainage","�����͂�D����"));
			break;
		case 1:
		case 2:
			You(E_J("don't feel so good ...",
				"���Ȃ�C���������Ȃ����c�B"));
			addhpmax(-rnd(6));
			u.uenmax -= rnd(8);
			if (u.uenmax < 0) u.uenmax = 0;
			if (u.uenmax < u.uen) u.uen = u.uenmax;
			break;
		case 3:
		case 4:
			You(E_J("feel something strange for a moment.",
				"��u�Ȃ�Ƃ������Ȃ��C���ɂȂ����B"));
			break;
		case 5:
		case 6:
		case 7:
			You(E_J("feel physically and mentally stronger!",
				"���̓I�ɂ����_�I�ɂ������Ȃ����I"));
			addhpmax(rnd(6));
			u.uhp = u.uhpmax;
			u.uenmax += rnd(8);
			u.uen = u.uenmax;
			break;
		case 8:
		case 9:
			You(E_J("feel that was a smart thing to do.",
				"�����������s�����Ƃ������ƂɋC�Â����B"));
			pluslvl(FALSE);
			break;
		default:
			break;
		}
		flags.botl = 1;
		break;
	    case PM_HUMAN_WERERAT:
		catch_lycanthropy = TRUE;
		u.ulycn = PM_WERERAT;
		break;
	    case PM_HUMAN_WEREJACKAL:
		catch_lycanthropy = TRUE;
		u.ulycn = PM_WEREJACKAL;
		break;
	    case PM_HUMAN_WEREWOLF:
		catch_lycanthropy = TRUE;
		u.ulycn = PM_WEREWOLF;
		break;
	    case PM_NURSE:
		if (Upolyd) u.mh = u.mhmax;
		else u.uhp = u.uhpmax;
		flags.botl = 1;
		break;
	    case PM_STALKER:
		if(!Invis) {
			set_itimeout(&HInvis, (long)rn1(100, 50));
			if (!Blind && !BInvis) self_invis_message();
		} else {
			if (!(HInvis & INTRINSIC)) You_feel(E_J("hidden!","�g���B�����C���ɂȂ����I"));
			HInvis |= FROMOUTSIDE;
			HSee_invisible |= FROMOUTSIDE;
		}
		newsym(u.ux, u.uy);
		/* fall into next case */
	    case PM_YELLOW_LIGHT:
		/* fall into next case */
	    case PM_GIANT_BAT:
		make_stunned(HStun + 30,FALSE);
		/* fall into next case */
	    case PM_BAT:
		make_stunned(HStun + 30,FALSE);
		break;
	    case PM_GIANT_MIMIC:
		tmp += 10;
		/* fall into next case */
	    case PM_LARGE_MIMIC:
		tmp += 20;
		/* fall into next case */
	    case PM_SMALL_MIMIC:
		tmp += 20;
		if (youmonst.data->mlet != S_MIMIC && !Unchanging) {
		    char buf[BUFSZ];
#ifndef JP
		    You_cant("resist the temptation to mimic %s.",
			Hallucination ? "an orange" : "a pile of gold");
#else
		    You("%s�ɉ��������Ƃ����~����}������Ȃ��Ȃ����I",
			Hallucination ? "�݂���" : "����̎R");
#endif /*JP*/
#ifdef STEED
                    /* A pile of gold can't ride. */
		    if (u.usteed) dismount_steed(DISMOUNT_FELL);
#endif
		    nomul(-tmp);
#ifndef JP
		    Sprintf(buf, Hallucination ?
			"You suddenly dread being peeled and mimic %s again!" :
			"You now prefer mimicking %s again.",
			an(Upolyd ? youmonst.data->mname : urace.noun));
#else
		    Sprintf(buf, Hallucination ?
			"�ˑR�A���Ȃ��͔���ނ����̂��|���Ȃ�A�Ă�%s�̂ӂ�������I" :
			"���Ȃ��͂�͂�%s�̂ӂ�����邱�Ƃɂ����B",
			Upolyd ? JMONNAM(monsndx(youmonst.data)) : urace.noun);
#endif /*JP*/
		    eatmbuf = strcpy((char *) alloc(strlen(buf) + 1), buf);
		    nomovemsg = eatmbuf;
		    afternmv = eatmdone;
		    /* ??? what if this was set before? */
		    youmonst.m_ap_type = M_AP_OBJECT;
		    youmonst.mappearance = Hallucination ? ORANGE : GOLD_PIECE;
		    newsym(u.ux,u.uy);
		    curs_on_u();
		    /* make gold symbol show up now */
		    display_nhwindow(WIN_MAP, TRUE);
		}
		break;
	    case PM_QUANTUM_MECHANIC:
		Your(E_J("velocity suddenly seems very uncertain!",
			 "���x�͓ˑR����߂ĕs�m��ɂȂ����I"));
		if (HFast & INTRINSIC) {
			HFast &= ~INTRINSIC;
			You(E_J("seem slower.","�x���Ȃ����悤���B"));
		} else {
			HFast |= FROMOUTSIDE;
			You(E_J("seem faster.","�����Ȃ����悤���B"));
		}
		break;
	    case PM_LIZARD:
		if (HStun > 2)  make_stunned(2L,FALSE);
		if (HConfusion > 2)  make_confused(2L,FALSE);
		break;
	    case PM_CHAMELEON:
	    case PM_DOPPELGANGER:
	 /* case PM_SANDESTIN: */
		if (!Unchanging) {
		    E_J(You_feel("a change coming over you."),
			pline("�ω������Ȃ��ɖK�ꂽ�B"));
		    polyself(FALSE);
		}
		break;
	    case PM_DISENCHANTER:
		{
		    struct obj *equipment[10];
		    int i = 0;
		    if (uwep &&
			(uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
			equipment[i++] = uwep;
		    if (uarm) equipment[i++] = uarm;
		    if (uarmc) equipment[i++] = uarmc;
		    if (uarmg) equipment[i++] = uarmg;
		    if (uarms) equipment[i++] = uarms;
		    if (uarmf) equipment[i++] = uarmf;
		    if (uarmh) equipment[i++] = uarmh;
#ifdef TOURIST
		    if (uarmu) equipment[i++] = uarmu;
#endif

		    if (uleft && uleft->oclass == RING_CLASS &&
			objects[uleft->otyp].oc_charged) equipment[i++] = uleft;
		    if (uright && uright->oclass == RING_CLASS &&
			objects[uright->otyp].oc_charged) equipment[i++] = uright;
		    if (!Blind)
#ifndef JP
			You("are surrounded by a bright blue glow%s",
				i ? "!" : " for a moment.");
#else
			You("%s�̋P���ɕ�܂ꂽ%s",
				i ? "�܂΂䂢" : "��u�A��������", i ? "�I" : "�B");
#endif /*JP*/
		    if (i) {
			struct obj *otmp = equipment[rn2(i)];
			if ((otmp->spe <
				((otmp->oclass == RING_CLASS || otmp->oclass == WEAPON_CLASS ||
				  is_weptool(otmp) || is_special_armor(otmp)) ? 7 : 5)) &&
			    (otmp->spe < 2 || !rn2(otmp->spe/2 + 1))) {
			    if (!Blind)
				pline_The(E_J("blue glow converges on your %s, and disappears.",
					      "�����͂��Ȃ���%s�Ɏ������A�����ď������B"), xname(otmp));
			    else
#ifndef JP
				Your("%s %s warm for a moment.",
					xname(otmp), otense(otmp, "feel"));
#else
				Your("%s����u�g�����Ȃ����B", xname(otmp));
#endif /*JP*/
			    otmp->spe++;
			    adj_abon(otmp, 1);
			    update_inventory();
			    flags.botl = 1;
			} else {
			    if (!Blind)
				pline_The(E_J("blue glow disappears.","�����͏������B"));
			}
		    }
		}
		break;
	    case PM_MIND_FLAYER:
	    case PM_MASTER_MIND_FLAYER:
		if (ABASE(A_INT) < ATTRMAX(A_INT)) {
			if (!rn2(2)) {
				pline(E_J("Yum! That was real brain food!",
					"�������I ���ꂼ�܂��ɓ��̂悭�Ȃ�H�ו����I"));
				(void) adjattrib(A_INT, 1, FALSE);
				break;	/* don't give them telepathy, too */
			}
		}
		else {
			pline(E_J("For some reason, that tasted bland.",
				  "�ǂ������킯���A���}�Ȗ�������B"));
		}
		/* fall through to default case */
	    default: {
		register struct permonst *ptr = &mons[pm];
		int i, count;

		if (dmgtype(ptr, AD_STUN) || dmgtype(ptr, AD_HALU) ||
		    pm == PM_VIOLET_FUNGUS) {
			pline (E_J("Oh wow!  Great stuff!","���킠�I ����Ⴗ������I"));
			make_hallucinated(HHallucination + 200,FALSE,0L);
		}
		if(is_giant(ptr)) gainstr((struct obj *)0, 0);

		/* Check the monster for all of the intrinsics.  If this
		 * monster can give more than one, pick one to try to give
		 * from among all it can give.
		 *
		 * If a monster can give 4 intrinsics then you have
		 * a 1/1 * 1/2 * 2/3 * 3/4 = 1/4 chance of getting the first,
		 * a 1/2 * 2/3 * 3/4 = 1/4 chance of getting the second,
		 * a 1/3 * 3/4 = 1/4 chance of getting the third,
		 * and a 1/4 chance of getting the fourth.
		 *
		 * And now a proof by induction:
		 * it works for 1 intrinsic (1 in 1 of getting it)
		 * for 2 you have a 1 in 2 chance of getting the second,
		 *	otherwise you keep the first
		 * for 3 you have a 1 in 3 chance of getting the third,
		 *	otherwise you keep the first or the second
		 * for n+1 you have a 1 in n+1 chance of getting the (n+1)st,
		 *	otherwise you keep the previous one.
		 * Elliott Kleinrock, October 5, 1990
		 */

		 count = 0;	/* number of possible intrinsics */
		 tmp = 0;	/* which one we will try to give */
		 for (i = 1; i <= LAST_PROP; i++) {
			if (intrinsic_possible(i, ptr)) {
				count++;
				/* a 1 in count chance of replacing the old
				 * one with this one, and a count-1 in count
				 * chance of keeping the old one.  (note
				 * that 1 in 1 and 0 in 1 are what we want
				 * for the first one
				 */
				if (!rn2(count)) {
#ifdef DEBUG
					debugpline("Intrinsic %d replacing %d",
								i, tmp);
#endif
					tmp = i;
				}
			}
		 }

		 /* if any found try to give them one */
		 if (count) givit(tmp, ptr);
	    }
	    break;
	}

	if (catch_lycanthropy && defends(AD_WERE, uwep)) {
	    if (!touch_artifact(uwep, &youmonst)) {
		dropx(uwep);
		uwepgone();
	    }
	}

	return;
}

void
violated_vegetarian()
{
    u.uconduct.unvegetarian++;
    if (Role_if(PM_MONK)) {
	You_feel(E_J("guilty.","�߂̈ӎ���������B"));
	adjalign(-1);
    }
    return;
}

/* common code to check and possibly charge for 1 context.tin.tin,
 * will split() context.tin.tin if necessary */
STATIC_PTR
void
costly_tin(verb)
	const char* verb;		/* if 0, the verb is "open" */
{
	if(((!carried(tin.tin) &&
	     costly_spot(tin.tin->ox, tin.tin->oy) &&
	     !tin.tin->no_charge)
	    || tin.tin->unpaid)) {
	    verbalize(E_J("You %s it, you bought it!",
			  "%s�����Ă��炤��I"), verb ? verb : E_J("open","�J�����̂Ȃ�"));
	    if(tin.tin->quan > 1L) tin.tin = splitobj(tin.tin, 1L);
	    bill_dummy_object(tin.tin);
	}
}

STATIC_PTR
int
opentin()		/* called during each move whilst opening a tin */
{
	register int r;
	const char *what;
	int which;
	char buf[BUFSZ];
	boolean edibility = (u.uedibility ||
		(uarmc && uarmc->otyp == KITCHEN_APRON && !uarmc->cursed));
	boolean dangerfood = FALSE;

	if(!carried(tin.tin) && !obj_here(tin.tin, u.ux, u.uy))
					/* perhaps it was stolen? */
		return(0);		/* %% probably we should use tinoid */
	if(tin.usedtime++ >= 50) {
		You(E_J("give up your attempt to open the tin.",
			"�ʋl���J����̂�������߂��B"));
		return(0);
	}
	if(tin.usedtime < tin.reqtime)
		return(1);		/* still busy */
	if(tin.tin->otrapped ||
	   (tin.tin->cursed && tin.tin->spe != -1 && !rn2(8))) {
		b_trapped(E_J("tin","�ʋl"), 0);
		costly_tin(E_J("destroyed","�䖳���ɂ����񂾂���A"));
		goto use_me;
	}
	You(E_J("succeed in opening the tin.","�ʂ��J���邱�Ƃɐ��������B"));
	if(tin.tin->spe != 1) {
	    if (tin.tin->corpsenm == NON_PM) {
		pline(E_J("It turns out to be empty.","���ɂ͉��������Ă��Ȃ������B"));
		tin.tin->dknown = tin.tin->known = TRUE;
		costly_tin((const char*)0);
		goto use_me;
	    }
	    r = tin.tin->cursed ? ROTTEN_TIN :	/* always rotten if cursed */
		    (tin.tin->spe == -1) ? HOMEMADE_TIN :  /* player made it */
			rn2(TTSZ-1);		/* else take your pick */
	    if (r == ROTTEN_TIN && (tin.tin->corpsenm == PM_LIZARD ||
			tin.tin->corpsenm == PM_LICHEN))
		r = HOMEMADE_TIN;		/* lizards don't rot */
	    else if (tin.tin->spe == -1 && !tin.tin->blessed && !rn2(7))
		r = ROTTEN_TIN;			/* some homemade tins go bad */
	    which = 0;	/* 0=>plural, 1=>as-is, 2=>"the" prefix */
	    if (Hallucination) {
		what = rndmonnam();
	    } else {
#ifndef JP
		what = mons[tin.tin->corpsenm].mname;
		if (mons[tin.tin->corpsenm].geno & G_UNIQ)
		    which = type_is_pname(&mons[tin.tin->corpsenm]) ? 1 : 2;
#else
		what = JMONNAM(tin.tin->corpsenm);
#endif /*JP*/
	    }
#ifndef JP
	    if (which == 0) what = makeplural(what);
	    pline("It smells like %s%s%s.",
			(which == 2) ? "the " : "",
			dangerfood ? "rotten " : "",
			what);
#else
	    pline("%s%s�̂悤��%s��������B",
			dangerfood ? "������" : "",
			what,
			dangerfood ? "�L" : "��");
#endif /*JP*/
	    if (edibility) {
		if (r == ROTTEN_TIN) dangerfood = TRUE;
		if (isstoneorslime(tin.tin->corpsenm)) {
		    pline(E_J("It could be very dangerous!",
			      "����͂ƂĂ��댯�Ȃ��̂ɈႢ�Ȃ��I"));
		    dangerfood = TRUE;
		} else if (!vegetarian(&mons[tin.tin->corpsenm])) {
		    if (Role_if(PM_MONK)) {
			pline(E_J("It could be unhealthy.",
				  "����͑̂ɗǂ��Ȃ��悤���B"));
			dangerfood = TRUE;
		    } else if (!u.uconduct.unvegetarian) {
			pline(E_J("It could be foul and unfamiliar to you.",
				  "����͑S������݂̂Ȃ��A�s���Ȃ��̂̂悤���B"));
			dangerfood = TRUE;
		    }
		}
	    }
	    Sprintf(buf, E_J("Eat it%s?","%s�H�ׂ܂����H"), dangerfood ? E_J(" anyway","����ł�") : "");
	    if (yn(buf) == 'n') {
		if (!Hallucination) tin.tin->dknown = tin.tin->known = TRUE;
		if (flags.verbose) You(E_J("discard the open tin.","�J�����ʋl���̂Ă��B"));
		costly_tin((const char*)0);
		goto use_me;
	    }
	    /* in case stop_occupation() was called on previous meal */
	    victual.piece = (struct obj *)0;
	    victual.fullwarn = victual.eating = victual.doreset = FALSE;

#ifndef JP
	    You("consume %s %s.", tintxts[r].txt,
			mons[tin.tin->corpsenm].mname);
#else
	    Sprintf(buf, tintxts[r].txt, JMONNAM(tin.tin->corpsenm));
	    You("%s��H�׏I�����B", buf);
#endif /*JP*/

	    /* KMH, conduct */
	    u.uconduct.food++;
	    if (!vegan(&mons[tin.tin->corpsenm]))
		u.uconduct.unvegan++;
	    if (!vegetarian(&mons[tin.tin->corpsenm]))
		violated_vegetarian();

	    tin.tin->dknown = tin.tin->known = TRUE;
	    cprefx(tin.tin->corpsenm); cpostfx(tin.tin->corpsenm);

	    /* charge for one at pre-eating cost */
	    costly_tin((const char*)0);

	    /* check for vomiting added by GAN 01/16/87 */
	    if(tintxts[r].nut < 0) make_vomiting((long)rn1(15,10), FALSE);
	    else lesshungry(tintxts[r].nut);

	    if(r == 0 || r == FRENCH_FRIED_TIN) {
	        /* Assume !Glib, because you can't open tins when Glib. */
		incr_itimeout(&Glib, rnd(15));
#ifndef JP
		pline("Eating deep fried food made your %s very slippery.",
			makeplural(body_part(FINGER)));
#else
		pline("�g������H�ׂ������ŁA���Ȃ���%s�͊���₷���Ȃ����B",
			body_part(FINGER));
#endif /*JP*/
	    }
	} else {
	    if (tin.tin->cursed)
#ifndef JP
		pline("It contains some decaying%s%s substance.",
			Blind ? "" : " ", Blind ? "" : hcolor(NH_GREEN));
#else
		pline("���ɂ͉���������%s���̂������Ă���B", Blind ? "" : hcolor(NH_GREEN));
#endif /*JP*/
	    else
		pline(E_J("It contains spinach.","�ق���񑐂������Ă���B"));

	    if (yn(E_J("Eat it?","�H�ׂ܂����H")) == 'n') {
		if (!Hallucination && !tin.tin->cursed)
		    tin.tin->dknown = tin.tin->known = TRUE;
		if (flags.verbose)
		    You(E_J("discard the open tin.","�J�����ʋl���̂Ă��B"));
		costly_tin((const char*)0);
		goto use_me;
	    }

	    tin.tin->dknown = tin.tin->known = TRUE;
	    costly_tin((const char*)0);

	    if (!tin.tin->cursed)
		pline(E_J("This makes you feel like %s!","%s�̂悤�ȋC�����I"),
		      Hallucination ? E_J("Swee'pea","�X�E�B�[�s�[") : E_J("Popeye","�|�p�C"));
	    else pline(E_J("Yuck. You feel sick.","�I�G�b�B���Ȃ��͋C���������Ȃ����B"));
	    lesshungry(600);
	    gainstr(tin.tin, 0);
	    u.uconduct.food++;
	}
use_me:
	if (carried(tin.tin)) useup(tin.tin);
	else useupf(tin.tin, 1L);
	tin.tin = (struct obj *) 0;
	if (u.uedibility && dangerfood) {
	    Your(stoptingle, body_part(NOSE));
	    u.uedibility = 0;
	}
	return(0);
}

STATIC_OVL void
start_tin(otmp)		/* called when starting to open a tin */
	register struct obj *otmp;
{
	register int tmp;

	if (metallivorous(youmonst.data)) {
		You(E_J("bite right into the metal tin...",
			"�����̊ʂɊ��݂����c�B"));
		tmp = 1;
	} else if (nolimbs(youmonst.data)) {
		You(E_J("cannot handle the tin properly to open it.",
			"�ʂ��J�����Ƃ𐳂������Ȃ����Ƃ��ł��Ȃ��B"));
		return;
	} else if (otmp->blessed) {
		pline_The(E_J("tin opens like magic!","�ʂ͂܂�Ŗ��@�̂悤�ɊJ�����I"));
		tmp = 1;
	} else if(uwep) {
		switch(uwep->otyp) {
		case TIN_OPENER:
			tmp = 1;
			break;
		case DAGGER:
		/*case SILVER_DAGGER: (material patch) */
		case ELVEN_DAGGER:
		case ORCISH_DAGGER:
		case ATHAME:
		case KNIFE:
		case CRYSKNIFE:
			tmp = 3;
			break;
		case PICK_AXE:
		case AXE:
			tmp = 6;
			break;
		default:
			goto no_opener;
		}
		pline(E_J("Using your %s you try to open the tin.",
			  "���Ȃ���%s���g���Ċʋl���J���n�߂��B"),
			E_J(aobjnam(uwep, (char *)0),xname(uwep)));
	} else {
no_opener:
		pline(E_J("It is not so easy to open this tin.",
			  "���̊ʋl���J����̂͗e�ՂȂ��Ƃł͂Ȃ��B"));
		if(Glib) {
			pline_The(E_J("tin slips from your %s.","�ʋl�͂��Ȃ���%s���炷�ׂ藎�����B"),
			      E_J(makeplural(body_part(FINGER)),body_part(FINGER)));
			if(otmp->quan > 1L) {
			    otmp = splitobj(otmp, 1L);
			}
			if (carried(otmp)) dropx(otmp);
			else stackobj(otmp);
			return;
		}
		tmp = rn1(1 + 500/((int)(ACURR(A_DEX) + ACURRSTR)), 10);
	}
	tin.reqtime = tmp;
	tin.usedtime = 0;
	tin.tin = otmp;
	set_occupation(opentin, E_J("opening the tin","�ʋl���J�����"), 0);
	return;
}

int
Hear_again()		/* called when waking up after fainting */
{
	flags.soundok = 1;
	return 0;
}

/* called on the "first bite" of rotten food */
STATIC_OVL int
rottenfood(obj)
struct obj *obj;
{
	pline(E_J("Blecch!  Rotten %s!","�������I ����%s�͕����Ă���I"), foodword(obj));
	if(!rn2(4)) {
		if (Hallucination) You_feel(E_J("rather trippy.","�L���L���ɂȂ����B"));
		else You_feel(E_J("rather %s.","����%s�B"), body_part(LIGHT_HEADED));
		make_confused(HConfusion + d(2,4),FALSE);
	} else if(!rn2(4) && !Blind) {
		pline(E_J("Everything suddenly goes dark.",
			  "�ˑR�����������^���ÂɂȂ����B"));
		make_blinded((long)d(2,10),FALSE);
		if (!Blind) Your(vision_clears);
	} else if(!rn2(3)) {
#ifndef JP
		const char *what, *where;
		if (!Blind)
		    what = "goes",  where = "dark";
		else if (Levitation || Is_airlevel(&u.uz) ||
			 Is_waterlevel(&u.uz))
		    what = "you lose control of",  where = "yourself";
		else
		    what = "you slap against the", where =
#ifdef STEED
			   (u.usteed) ? "saddle" :
#endif
			   surface(u.ux,u.uy);
		pline_The("world spins and %s %s.", what, where);
#else /*JP*/
		const char *w;
		char tmp[BUFSZ];
		if (!Blind)
		    w = "�^���ÂɂȂ���";
		else if (Levitation || Is_airlevel(&u.uz) ||
			 Is_waterlevel(&u.uz))
		    w = "�̂��������Ƃ������Ȃ��Ȃ���";
		else {
		    w = tmp;
#ifdef STEED
		    if (u.usteed)
			w = "���Ȃ��͈Ƃ̏�ō��|����";
		    else
#endif
		    Sprintf(tmp, "���Ȃ���%s�ɓ|�ꂱ��", surface(u.ux,u.uy));
		}
		pline("���E�����o���A%s�B", w);
#endif /*JP*/
		flags.soundok = 0;
		nomul(-rnd(10));
		nomovemsg = E_J("You are conscious again.","���Ȃ��͈ӎ������߂����B"); /* see unconscious() */
		afternmv = Hear_again;
		return(1);
	}
	return(0);
}

STATIC_OVL int
eatcorpse(otmp)		/* called when a corpse is selected as food */
	register struct obj *otmp;
{
	int tp = 0, mnum = otmp->corpsenm;
	long rotted = 0L;
	boolean uniq = !!(mons[mnum].geno & G_UNIQ);
	int retcode = 0;
	boolean stoneable = (touch_petrifies(&mons[mnum]) && !Stone_resistance &&
				!poly_when_stoned(youmonst.data));

	/* KMH, conduct */
	if (!vegan(&mons[mnum])) u.uconduct.unvegan++;
	if (!vegetarian(&mons[mnum])) violated_vegetarian();

	if (mnum != PM_LIZARD && mnum != PM_LICHEN) {
		long age = peek_at_iced_corpse_age(otmp);

		rotted = (monstermoves - age)/(10L + rn2(20));
		if (otmp->cursed) rotted += 2L;
		else if (otmp->blessed) rotted -= 2L;
	}

	if (victual.tainted) {
		boolean cannibal = maybe_cannibal(mnum, FALSE);
		pline(E_J("Ulch - that %s was tainted%s!",
			  "�E�F�c ����%s�͉�������Ă�%s�I"),
		      mons[mnum].mlet == S_FUNGUS ? E_J("fungoid vegetation","�ۑ���") :
		      !vegetarian(&mons[mnum]) ? E_J("meat","��") : E_J("protoplasm","���`��"),
		      cannibal ? E_J(" cannibal","�āA���̏㋤�H����") : E_J("","��"));
		if (Sick_resistance) {
			pline(E_J("It doesn't seem at all sickening, though...",
				  "�܂��A���̂Ƃ��낻��قǍ������̂ł��Ȃ��悤�����c�B"));
		} else {
			char buf[BUFSZ];
			long sick_time;

			sick_time = (long) rn1(10, 10);
			/* make sure new ill doesn't result in improvement */
			if (Sick && (sick_time > Sick))
			    sick_time = (Sick > 1L) ? Sick - 1L : 1L;
			if (!uniq)
			    Sprintf(buf, E_J("rotted %s","������%s"), corpse_xname(otmp,TRUE));
			else
#ifndef JP
			    Sprintf(buf, "%s%s rotted corpse",
				    !type_is_pname(&mons[mnum]) ? "the " : "",
				    s_suffix(mons[mnum].mname));
#else
			    Sprintf(buf, "%s�̕���������", JMONNAM(mnum));
#endif /*JP*/
			make_sick(sick_time, buf, TRUE, SICK_VOMITABLE);
		}
		if (carried(otmp)) useup(otmp);
		else useupf(otmp, 1L);
		return(2);
	} else if (acidic(&mons[mnum]) && !Acid_resistance) {
		tp++;
		You(E_J("have a very bad case of stomach acid.",
			"�d�x�̈ݎ_�ߑ��ǂƂȂ����B")); /* not body_part() */
		losehp(rnd(15), E_J("acidic corpse","���_���̎��̂�H�ׂ�"), KILLED_BY_AN);
	} else if (victual.poison) {
		pline(E_J("Ecch - that must have been poisonous!",
			  "�Q�F�c �L�ł������ɈႢ�Ȃ��I"));
		if(!Poison_resistance) {
			losestr(rnd(4));
			losehp(rnd(15), E_J("poisonous corpse","�L�łȎ��̂�H�ׂ�"), KILLED_BY_AN);
		} else	You(E_J("seem unaffected by the poison.",
				"�ł̉e�����󂯂Ȃ��悤���B"));
	/* now any corpse left too long will make you mildly ill */
	} else if (victual.rotsick) {
		You_feel(E_J("%ssick.","%s�C���������Ȃ����B"), (Sick) ? E_J("very ","�ƂĂ�") : "");
		losehp(rnd(8), E_J("cadaver","�������̂�H�ׂ�"), KILLED_BY_AN);
	}

	/* delay is weight dependent */
	victual.reqtime = 3 + (mons[mnum].cwt >> 6);

	if (victual.rotten) {
	    if (rottenfood(otmp)) {
		(void)touchfood(otmp);
		retcode = 1;
	    }

	    if (!mons[otmp->corpsenm].cnutrit) {
		/* no nutrution: rots away, no message if you passed out */
		if (!retcode) pline_The("corpse rots away completely.");
		if (carried(otmp)) useup(otmp);
		else useupf(otmp, 1L);
		retcode = 2;
	    }
		    
	    if (!retcode) consume_oeaten(otmp, 2);	/* oeaten >>= 2 */
	} else {
#ifndef JP
	    pline("%s%s %s!",
		  !uniq ? "This " : !type_is_pname(&mons[mnum]) ? "The " : "",
		  food_xname(otmp, FALSE),
		  (vegan(&mons[mnum]) ?
		   (!carnivorous(youmonst.data) && herbivorous(youmonst.data)) :
		   (carnivorous(youmonst.data) && !herbivorous(youmonst.data)))
		  ? "is delicious" : "tastes terrible");
#else
	    pline("%s��%s�I",
		  food_xname(otmp, FALSE),
		  (vegan(&mons[mnum]) ?
		   (!carnivorous(youmonst.data) && herbivorous(youmonst.data)) :
		   (carnivorous(youmonst.data) && !herbivorous(youmonst.data)))
		  ? "���܂�" : "�Ђǂ�����");
#endif /*JP*/
	}

	return(retcode);
}

STATIC_OVL void
start_eating(otmp)		/* called as you start to eat */
	register struct obj *otmp;
{
#ifdef DEBUG
	debugpline("start_eating: %lx (victual = %lx)", otmp, victual.piece);
	debugpline("reqtime = %d", victual.reqtime);
	debugpline("(original reqtime = %d)", objects[otmp->otyp].oc_delay);
	debugpline("nmod = %d", victual.nmod);
	debugpline("oeaten = %d", otmp->oeaten);
#endif
	victual.fullwarn = victual.doreset = FALSE;
	victual.eating = TRUE;

	if (otmp->otyp == CORPSE) {
	    cprefx(victual.piece->corpsenm);
	    if (!victual.piece || !victual.eating) {
		/* rider revived, or died and lifesaved */
		return;
	    }
	}

	if (bite()) return;

	if (++victual.usedtime >= victual.reqtime) {
	    /* print "finish eating" message if they just resumed -dlc */
	    done_eating(victual.reqtime > 1 ? TRUE : FALSE);
	    return;
	}

	Sprintf(msgbuf, E_J("eating %s","%s��H�ׂ��"), food_xname(otmp, TRUE));
	set_occupation(eatfood, msgbuf, 0);
}


/*
 * called on "first bite" of (non-corpse) food.
 * used for non-rotten non-tin non-corpse food
 */
STATIC_OVL void
fprefx(otmp)
struct obj *otmp;
{
	switch(otmp->otyp) {
	    case FOOD_RATION:
		if(u.uhunger <= 200)
		    pline(Hallucination ? E_J("Oh wow, like, superior, man!",
			    "���[�ށA�܂�����Ƃ��āA����ł��Ă������Ȃ��I") :
			  E_J("That food really hit the spot!","���̐H���͂܂��ɐ\�����Ȃ��I"));
#ifndef JP
		else if(u.uhunger <= 700) pline("That satiated your %s!",
						body_part(STOMACH));
#else
		else if(u.uhunger <= 700) pline("�����ɂȂ����I");
#endif /*JP*/
		break;
	    case TRIPE_RATION:
		if (carnivorous(youmonst.data) && !humanoid(youmonst.data))
		    pline(E_J("That tripe ration was surprisingly good!",
			      "���̃��c���͂��ǂ낭�قǎ|���I"));
		else if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC)))
		    pline(Hallucination ? E_J("Tastes great! Less filling!","���܂��I �����Ƃ���I") :
			  E_J("Mmm, tripe... not bad!","���A���c���c�����Ȃ��I"));
		else {
		    pline(E_J("Yak - dog food!","�����c�h�b�O�t�[�h���I"));
		    more_experienced(1,0);
		    newexplevel();
		    /* not cannibalism, but we use similar criteria
		       for deciding whether to be sickened by this meal */
		    if (rn2(2) && !CANNIBAL_ALLOWED())
			make_vomiting((long)rn1(victual.reqtime, 14), FALSE);
		}
		break;
	    case MEATBALL:
	    case MEAT_STICK:
	    case HUGE_CHUNK_OF_MEAT:
	    case MEAT_RING:
		goto give_feedback;
	     /* break; */
	    case CLOVE_OF_GARLIC:
		if (is_undead(youmonst.data)) {
			make_vomiting((long)rn1(victual.reqtime, 5), FALSE);
			break;
		}
		/* Fall through otherwise */
	    default:
		if (otmp->otyp==SLIME_MOLD && !otmp->cursed
			&& otmp->spe == current_fruit)
		    pline(E_J("My, that was a %s %s!","����A�Ȃ��%s%s���I"),
			  Hallucination ? E_J("primo","�ɏ��") : E_J("yummy","��������"),
			  singular(otmp, xname));
		else
#ifdef UNIX
		if (otmp->otyp == APPLE || otmp->otyp == PEAR) {
		    if (!Hallucination) pline("Core dumped.");
		    else {
/* This is based on an old Usenet joke, a fake a.out manual page */
			int x = rnd(100);
			if (x <= 75)
			    pline("Segmentation fault -- core dumped.");
			else if (x <= 99)
			    pline("Bus error -- core dumped.");
			else pline("Yo' mama -- core dumped.");
		    }
		} else
#endif
#ifdef MAC	/* KMH -- Why should Unix have all the fun? */
		if (otmp->otyp == APPLE) {
			pline(E_J("Delicious!  Must be a Macintosh!",
				  "���������I �}�b�L���g�b�V���ɈႢ�Ȃ��I"));
		} else
#endif
		if (otmp->otyp == EGG && stale_egg(otmp)) {
		    pline(E_J("Ugh.  Rotten egg.",
			      "�����B�����������B"));	/* perhaps others like it */
		    make_vomiting(Vomiting+d(10,4), TRUE);
		} else
 give_feedback:
		    pline(E_J("This %s is %s","����%s��%s"), singular(otmp, xname),
		      otmp->cursed ? (Hallucination ? E_J("grody!","�C�P�ĂȂ��I") : E_J("terrible!","�Ђǂ������I")) :
		      (otmp->otyp == CRAM_RATION
		      || otmp->otyp == K_RATION
		      || otmp->otyp == C_RATION)
		      ? E_J("bland.","���C�Ȃ��B") :
		      Hallucination ? E_J("gnarly!","�C�P�Ă�I") : E_J("delicious!","���������I"));
		break;
	}

	/* KMH, conduct */
	switch (check_food_conduct(otmp)) {
	    case 0: /* strict vegan diet */
		break;
	    case 1: /* vegetarian */
		u.uconduct.unvegan++;
		break;
	    case 2: /* meat */
		u.uconduct.unvegan++;
		violated_vegetarian();
		break;
	    default:
		break;
	}
}

/*
 * check food type
 *	0: strict vegan diet
 *	1: vegetarian
 *	2: meat
 */
STATIC_OVL int
check_food_conduct(otmp)
struct obj *otmp;
{
	int ftyp = 0;
	/* KMH, conduct */
	if (otmp->otyp == CORPSE || otmp->otyp == TIN) {
	    ftyp = vegetarian(&mons[otmp->corpsenm]) ? 0 : 2;
	} else {
	  switch (get_material(otmp)) {
	    case WAX: /* let's assume bees' wax */
		ftyp = 1;
		break;

	    case FLESH:
		if (otmp->otyp == EGG) {
			ftyp = 1;
			break;
		}
		/* fallthru */
	    case LEATHER:
	    case BONE:
	    case DRAGON_HIDE:
		ftyp = 2;
		break;

	    default:
		if (otmp->otyp == PANCAKE ||
		    otmp->otyp == FORTUNE_COOKIE || /* eggs */
		    otmp->otyp == CREAM_PIE || otmp->otyp == CANDY_BAR || /* milk */
		    otmp->otyp == LUMP_OF_ROYAL_JELLY)
		ftyp = 1;
		break;
	  }
	}
	return ftyp;
}

STATIC_OVL void
accessory_has_effect(otmp)
struct obj *otmp;
{
	pline(E_J("Magic spreads through your body as you digest the %s.",
		  "%s�����������ƂƂ��ɁA���͂����Ȃ��̑̂ɐ��ݓn�����B"),
	    otmp->oclass == RING_CLASS ? E_J("ring","�w��") : E_J("amulet","������"));
}

STATIC_OVL void
eataccessory(otmp)
struct obj *otmp;
{
	int typ = otmp->otyp;
	long oldprop;

	/* Note: rings are not so common that this is unbalancing. */
	/* (How often do you even _find_ 3 rings of polymorph in a game?) */
	oldprop = u.uprops[objects[typ].oc_oprop].intrinsic;
	if (otmp == uleft || otmp == uright) {
	    Ring_gone(otmp);
	    if (u.uhp <= 0) return; /* died from sink fall */
	}
	otmp->known = otmp->dknown = 1; /* by taste */
	if (!rn2(otmp->oclass == RING_CLASS ? 3 : 5)) {
	  switch (otmp->otyp) {
	    default:
	        if (!objects[typ].oc_oprop) break; /* should never happen */

		if (!(u.uprops[objects[typ].oc_oprop].intrinsic & FROMOUTSIDE))
		    accessory_has_effect(otmp);

		u.uprops[objects[typ].oc_oprop].intrinsic |= FROMOUTSIDE;

		switch (typ) {
		  case RIN_SEE_INVISIBLE:
		    set_mimic_blocking();
		    see_monsters();
		    if (Invis && !oldprop && !ESee_invisible &&
				!perceives(youmonst.data) && !Blind) {
			newsym(u.ux,u.uy);
			pline(E_J("Suddenly you can see yourself.",
				  "�ˑR�A���Ȃ��͎�����������悤�ɂȂ����B"));
			makeknown(typ);
		    }
		    break;
		  case RIN_INVISIBILITY:
		    if (!oldprop && !EInvis && !BInvis &&
					!See_invisible && !Blind) {
			newsym(u.ux,u.uy);
			Your(E_J("body takes on a %s transparency...",
				 "�̂�%s��������тт��c�B"),
				Hallucination ? E_J("normal","���ʂ�") : E_J("strange","�s�v�c��"));
			makeknown(typ);
		    }
		    break;
		  case RIN_PROTECTION_FROM_SHAPE_CHAN:
		    rescham();
		    break;
		  case RIN_LEVITATION:
		    /* undo the `.intrinsic |= FROMOUTSIDE' done above */
		    u.uprops[LEVITATION].intrinsic = oldprop;
		    if (!Levitation) {
			float_up();
			incr_itimeout(&HLevitation, d(10,20));
			makeknown(typ);
		    }
		    break;
		}
		break;
	    case RIN_ADORNMENT:
		accessory_has_effect(otmp);
		if (adjattrib(A_CHA, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_GAIN_STRENGTH:
		accessory_has_effect(otmp);
		if (adjattrib(A_STR, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_GAIN_CONSTITUTION:
		accessory_has_effect(otmp);
		if (adjattrib(A_CON, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_INCREASE_ACCURACY:
		accessory_has_effect(otmp);
		u.uhitinc += otmp->spe;
		break;
	    case RIN_INCREASE_DAMAGE:
		accessory_has_effect(otmp);
		u.udaminc += otmp->spe;
		break;
	    case RIN_PROTECTION:
		accessory_has_effect(otmp);
		HProtection |= FROMOUTSIDE;
		u.ublessed += otmp->spe;
		flags.botl = 1;
		break;
	    case RIN_FREE_ACTION:
		/* Give sleep resistance instead */
		if (!(HSleep_resistance & FROMOUTSIDE))
		    accessory_has_effect(otmp);
		if (!Sleep_resistance)
		    You_feel("wide awake.");
		HSleep_resistance |= FROMOUTSIDE;
		break;
	    case AMULET_OF_CHANGE:
		accessory_has_effect(otmp);
		makeknown(typ);
		change_sex();
		You(E_J("are suddenly very %s!","�ˑR�ƂĂ�%s�炵���Ȃ����I"),
		    flags.female ? E_J("feminine","��") : E_J("masculine","�j"));
		flags.botl = 1;
		break;
	    case AMULET_OF_UNCHANGING:
		/* un-change: it's a pun */
		if (!Unchanging && Upolyd) {
		    accessory_has_effect(otmp);
		    makeknown(typ);
		    rehumanize();
		}
		break;
	    case AMULET_OF_STRANGULATION: /* bad idea! */
		/* no message--this gives no permanent effect */
		choke(otmp);
		break;
	    case AMULET_OF_RESTFUL_SLEEP: /* another bad idea! */
		if (!(HSleeping & FROMOUTSIDE))
		    accessory_has_effect(otmp);
		HSleeping = FROMOUTSIDE | rnd(100);
		break;
	    case RIN_SUSTAIN_ABILITY:
	    case AMULET_OF_LIFE_SAVING:
	    case AMULET_OF_REFLECTION: /* nice try */
	    /* can't eat Amulet of Yendor or fakes,
	     * and no oc_prop even if you could -3.
	     */
		break;
	  }
	}
}

STATIC_OVL void
eatspecial() /* called after eating non-food */
{
	register struct obj *otmp = victual.piece;

	/* lesshungry wants an occupation to handle choke messages correctly */
	set_occupation(eatfood, E_J("eating non-food","�H�ׂ��"), 0);
	lesshungry(victual.nmod);
	occupation = 0;
	victual.piece = (struct obj *)0;
	victual.eating = 0;
	if (otmp->oclass == COIN_CLASS) {
#ifdef GOLDOBJ
		if (carried(otmp))
		    useupall(otmp);
#else
		if (otmp->where == OBJ_FREE)
		    dealloc_obj(otmp);
#endif
		else
		    useupf(otmp, otmp->quan);
		return;
	}
	if (otmp->oclass == POTION_CLASS) {
		otmp->quan++; /* dopotion() does a useup() */
		(void)dopotion(otmp);
	}
	if (otmp->oclass == RING_CLASS || otmp->oclass == AMULET_CLASS)
		eataccessory(otmp);
	else if (otmp->otyp == LEASH && otmp->leashmon)
		o_unleash(otmp);

	/* KMH -- idea by "Tommy the Terrorist" */
	if ((otmp->otyp == TRIDENT) && !otmp->cursed)
	{
		pline(Hallucination ? E_J("Four out of five dentists agree.",
					  "5�l��4�l�̎��Ȉオ�����߂��Ă��܂��B") :
				E_J("That was pure chewing satisfaction!",
				    "����͂܂��Ɋ��މ����I"));
		exercise(A_WIS, TRUE);
	}
	if ((otmp->otyp == FLINT) && !otmp->cursed)
	{
		pline(E_J("Yabba-dabba delicious!","���b�o�_�b�o���܂��I"));
		exercise(A_CON, TRUE);
	}

	if (otmp == uwep && otmp->quan == 1L) uwepgone();
	if (otmp == uquiver && otmp->quan == 1L) uqwepgone();
	if (otmp == uswapwep && otmp->quan == 1L) uswapwepgone();

	if (otmp == uball) unpunish();
	if (otmp == uchain) unpunish(); /* but no useup() */
	else if (carried(otmp)) useup(otmp);
	else useupf(otmp, 1L);
}

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
static const char *foodwords[] = {
#ifndef JP
	"meal", "liquid", "wax", "food", "meat",
	"paper", "cloth", "leather", "wood", "bone", "scale",
	"metal", "metal", "metal", "silver", "gold", "platinum", "mithril",
	"plastic", "glass", "rich food", "stone"
#else
	"�H��", "�t��", "�X", "�H��", "��",
	"��", "�z", "�v", "��", "��", "��",
	"����", "����", "����", "��", "��", "�v���`�i", "�~�X����",
	"�v���X�`�b�N", "�K���X", "��������", "��"
#endif /*JP*/
};

STATIC_OVL const char *
foodword(otmp)
register struct obj *otmp;
{
	if (otmp->oclass == FOOD_CLASS) return E_J("food","�H�ו�");
	if (otmp->oclass == GEM_CLASS &&
	    get_material(otmp) == GLASS &&
	    otmp->dknown)
		makeknown(otmp->otyp);
	return foodwords[get_material(otmp)];
}

STATIC_OVL void
fpostfx(otmp)		/* called after consuming (non-corpse) food */
register struct obj *otmp;
{
	switch(otmp->otyp) {
	    case SPRIG_OF_WOLFSBANE:
		if (u.ulycn >= LOW_PM || is_were(youmonst.data))
		    you_unwere(TRUE);
		break;
	    case CARROT:
		make_blinded((long)u.ucreamed,TRUE);
		break;
	    case FORTUNE_COOKIE:
		outrumor(bcsign(otmp), BY_COOKIE);
		if (!Blind) u.uconduct.literate++;
		break;
	    case LUMP_OF_ROYAL_JELLY:
		/* This stuff seems to be VERY healthy! */
		gainstr(otmp, 1);
		if (Upolyd) {
		    u.mh += otmp->cursed ? -rnd(20) : rnd(20);
		    if (u.mh > u.mhmax) {
			if (!rn2(17)) u.mhmax++;
			u.mh = u.mhmax;
		    } else if (u.mh <= 0) {
			rehumanize();
		    }
		} else {
		    u.uhp += otmp->cursed ? -rnd(20) : rnd(20);
		    if (u.uhp > u.uhpmax) {
			if(!rn2(17)) addhpmax(1);
			u.uhp = u.uhpmax;
		    } else if (u.uhp <= 0) {
			killer_format = KILLED_BY_AN;
			killer = E_J("rotten lump of royal jelly","���������C�����[���[�̓łɂ�����");
			done(POISONING);
		    }
		}
		if(!otmp->cursed) heal_legs();
		break;
	    case EGG:
		if (touch_petrifies(&mons[otmp->corpsenm])) {
		    if (!Stone_resistance &&
			!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
			if (!Stoned) Stoned = 5;
			killer_format = KILLED_BY_AN;
			Sprintf(killer_buf, E_J("%s egg","%s�̗���H�ׂ�"),
				E_J(mons[otmp->corpsenm].mname,JMONNAM(otmp->corpsenm)));
			delayed_killer = killer_buf;
		    }
		}
		break;
	    case EUCALYPTUS_LEAF:
		if (Sick && !otmp->cursed)
		    make_sick(0L, (char *)0, TRUE, SICK_ALL);
		if (Vomiting && !otmp->cursed)
		    make_vomiting(0L, TRUE);
		break;
	}
	return;
}

boolean
isstoneorslime(mnum)
int mnum;
{
	return ((touch_petrifies(&mons[mnum]) &&
		 !Stone_resistance && !poly_when_stoned(youmonst.data)) ||
		(mnum == PM_GREEN_SLIME && !Unchanging &&
			!flaming(youmonst.data) &&
			youmonst.data != &mons[PM_GREEN_SLIME]));
}

/*
 * return 0 if the food was not dangerous.
 * return 1 if the food was dangerous and you chose to stop.
 * return 2 if the food was dangerous and you chose to eat it anyway.
 */
STATIC_OVL int
edibility_prompts(otmp)
struct obj *otmp;
{
	/* blessed food detection granted you a one-use
	   ability to detect food that is unfit for consumption
	   or dangerous and avoid it. */

	char buf[BUFSZ], foodsmell[BUFSZ],
	     it_or_they[QBUFSZ], eat_it_anyway[QBUFSZ];
	boolean cadaver = (otmp->otyp == CORPSE),
		stoneorslime = FALSE;
	int material = get_material(otmp),
	    mnum = otmp->corpsenm;
	int fcnd = check_food_conduct(otmp);
	long rotted = 0L;

#ifndef JP
	Strcpy(foodsmell, Tobjnam(otmp, "smell"));
	Strcpy(it_or_they, (otmp->quan == 1L) ? "it" : "they");
	Sprintf(eat_it_anyway, "Eat %s anyway?",
		(otmp->quan == 1L) ? "it" : "one");
#else /*JP*/
	Strcpy(foodsmell, xname(otmp));
	Strcpy(it_or_they, (otmp->quan == 1L) ? "it" : "they");
	Sprintf(eat_it_anyway, "����ł��H�ׂ܂����H");
#endif /*JP*/

	if (cadaver || otmp->otyp == EGG || otmp->otyp == TIN) {
		mnum = otmp->corpsenm;
		/* These checks must match those in eatcorpse() */
		stoneorslime = isstoneorslime(mnum);
	}

	/*
	 * These problems with food should be checked in
	 * order from most detrimental to least detrimental.
	 */

	if (victual.tainted && !Sick_resistance) {
		/* Tainted meat */
#ifndef JP
		Sprintf(buf, "%s like %s could be tainted! %s",
			foodsmell, it_or_they, eat_it_anyway);
#else
		Sprintf(buf, "%s�͉�������Ă���悤���I %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (stoneorslime) {
#ifndef JP
		Sprintf(buf, "%s like %s could be something very dangerous! %s",
			foodsmell, it_or_they, eat_it_anyway);
#else
		Sprintf(buf, "%s�͔��Ɋ댯�ȏL��������Ă���I %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf, ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (victual.rotsick || victual.rotten) {
		/* Rotten */
#ifndef JP
		Sprintf(buf, "%s like %s could be rotten! %s",
			foodsmell, it_or_they, eat_it_anyway);
#else
		Sprintf(buf, "%s�͕������悤�ȏL��������I %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (victual.poison && !Poison_resistance) {
		/* poisonous */
#ifndef JP
		Sprintf(buf, "%s like %s might be poisonous! %s",
			foodsmell, it_or_they, eat_it_anyway);
#else
		Sprintf(buf, "%s�͓ł������Ă������ȏL��������I %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (fcnd >= 2 && !u.uconduct.unvegetarian && Role_if(PM_MONK)) {
#ifndef JP
		Sprintf(buf, "%s unhealthy. %s",
			foodsmell, eat_it_anyway);
#else
		Sprintf(buf, "%s����͌��N�Ɉ������ȏL��������B %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (cadaver && acidic(&mons[mnum]) && !Acid_resistance) {
#ifndef JP
		Sprintf(buf, "%s rather acidic. %s",
			foodsmell, eat_it_anyway);
#else
		Sprintf(buf, "%s����͂��Ȃ�_���ς��L��������B %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (Upolyd && u.umonnum == PM_RUST_MONSTER &&
	    is_metallic(otmp) && otmp->oerodeproof) {
#ifndef JP
		Sprintf(buf, "%s disgusting to you right now. %s",
			foodsmell, eat_it_anyway);
#else
		Sprintf(buf, "%s�͓f���C���Â��L��������Ă���B %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}

	/*
	 * Breaks conduct, but otherwise safe.
	 */
	 
	if (!u.uconduct.unvegan && fcnd >= 1) {
#ifndef JP
		Sprintf(buf, "%s foul and unfamiliar to you. %s",
			foodsmell, eat_it_anyway);
#else
		Sprintf(buf, "%s����͐H�ׂ����Ƃ̂Ȃ��A�q�ꂽ�L��������B %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	if (!u.uconduct.unvegetarian && fcnd >= 2) {
#ifndef JP
		Sprintf(buf, "%s unfamiliar to you. %s",
			foodsmell, eat_it_anyway);
#else
		Sprintf(buf, "%s����͐H�ׂ����Ƃ̂Ȃ��L��������B %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}

//	if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && Sick_resistance) {
	if (victual.tainted && Sick_resistance) {
		/* Tainted meat with Sick_resistance */
#ifndef JP
		Sprintf(buf, "%s like %s could be tainted! %s",
			foodsmell, it_or_they, eat_it_anyway);
#else
		Sprintf(buf, "%s�͉�������Ă���悤���I %s",
			foodsmell, eat_it_anyway);
#endif /*JP*/
		if (yn_function(buf,ynchars,'n')=='n') return 1;
		else return 2;
	}
	return 0;
}

int
doeat()		/* generic "eat" command funtion (see cmd.c) */
{
	register struct obj *otmp;
	int basenutrit;			/* nutrition of full item */
	boolean dont_start = FALSE;
#ifdef JP
	static const struct getobj_words eatw = { 0, 0, "�H�ׂ�", "�H��" };
#endif /*JP*/

	if (Strangled) {
		pline(E_J("If you can't breathe air, how can you consume solids?",
			  "��C���z���Ȃ��̂ɁA�ǂ�����ĕ������ݍ��ނ񂾂��H"));
		return 0;
	}
	if (getobj_override) {
	    otmp = getobj_override;
	    getobj_override = 0;
	} else
	if (!(otmp = floorfood(E_J("eat",&eatw), 0))) return 0;
	if (check_capacity((char *)0)) return 0;

	set_rotten_flag(otmp);

	if (otmp->otyp != TIN) {
	    if (u.uedibility) {
		int res = edibility_prompts(otmp);
		if (res) {
		    Your(stoptingle, body_part(NOSE));
		    u.uedibility = 0;
		    if (res == 1) return 0;
		}
	    } else if (uarmc && uarmc->otyp == KITCHEN_APRON && !uarmc->cursed) {
		if (edibility_prompts(otmp) == 1) return 0;
	    }
	}

	/* We have to make non-foods take 1 move to eat, unless we want to
	 * do ridiculous amounts of coding to deal with partly eaten plate
	 * mails, players who polymorph back to human in the middle of their
	 * metallic meal, etc....
	 */
	if (!is_edible(otmp)) {
	    You(E_J("cannot eat that!","����Ȃ��̂�H�ׂ��Ȃ��I"));
	    return 0;
	} else if ((otmp->owornmask & (W_ARMOR|W_TOOL|W_AMUL
#ifdef STEED
			|W_SADDLE
#endif
			)) != 0) {
	    /* let them eat rings */
#ifndef JP
	    You_cant("eat %s you're wearing.", something);
#else
	    You_cant("�g�ɂ��Ă�����̂�H�ׂ邱�Ƃ͂ł��Ȃ��B");
#endif /*JP*/
	    return 0;
	}
	if (is_metallic(otmp) &&
	    u.umonnum == PM_RUST_MONSTER && otmp->oerodeproof) {
	    	otmp->rknown = TRUE;
		if (otmp->quan > 1L) {
		    if(!carried(otmp))
			(void) splitobj(otmp, otmp->quan - 1L);
		    else
			otmp = splitobj(otmp, 1L);
		}
		pline(E_J("Ulch - That %s was rustproofed!",
			  "�E�F�c ����%s�͖h�K��������Ă���I"), xname(otmp));
		/* The regurgitated object's rustproofing is gone now */
		otmp->oerodeproof = 0;
		make_stunned(HStun + rn2(10), TRUE);
#ifndef JP
		You("spit %s out onto the %s.", the(xname(otmp)),
			surface(u.ux, u.uy));
#else
		You("%s��%s�ɓf���̂Ă��B", xname(otmp),
			surface(u.ux, u.uy));
#endif /*JP*/
		if (carried(otmp)) {
			freeinv(otmp);
			dropy(otmp);
		}
		stackobj(otmp);
		return 1;
	}
	/* KMH -- Slow digestion is... indigestible */
	if (otmp->otyp == RIN_SLOW_DIGESTION) {
		pline(E_J("This ring is indigestible!","���̎w�ւ͏����ł��Ȃ��I"));
		(void) rottenfood(otmp);
		if (otmp->dknown && !objects[otmp->otyp].oc_name_known
				&& !objects[otmp->otyp].oc_uname)
			docall(otmp);
		return (1);
	}
	if (otmp->oclass != FOOD_CLASS) {
	    int material;
	    victual.reqtime = 1;
	    victual.piece = otmp;
		/* Don't split it, we don't need to if it's 1 move */
	    victual.usedtime = 0;
	    victual.canchoke = (u.uhs == SATIATED);
		/* Note: gold weighs 1 pt. for each 1000 pieces (see */
		/* pickup.c) so gold and non-gold is consistent. */
	    if (otmp->oclass == COIN_CLASS)
		basenutrit = ((otmp->quan > 200000L) ? 2000
			: (int)(otmp->quan/100L));
	    else if(otmp->oclass == BALL_CLASS || otmp->oclass == CHAIN_CLASS)
		basenutrit = weight(otmp);
	    /* oc_nutrition is usually weight anyway */
	    else basenutrit = objects[otmp->otyp].oc_nutrition;
	    victual.nmod = basenutrit;
	    victual.eating = TRUE; /* needed for lesshungry() */

	    material = get_material(otmp);
	    if (material == LEATHER || material == BONE ||
	        material == DRAGON_HIDE) {
	 	u.uconduct.unvegan++;
	    	violated_vegetarian();
	    } else if (material == WAX)
		u.uconduct.unvegan++;
	    u.uconduct.food++;
	    
//	    if (otmp->cursed)
	    if (victual.rotten)
		(void) rottenfood(otmp);

//	    if (otmp->oclass == WEAPON_CLASS && otmp->opoisoned) {
	    if (victual.poison) {
#ifndef JP
		pline("Ecch - that must have been poisonous!");
#else
		pline("�����c ����%s�͗L�ł������ɈႢ�Ȃ��I", foodword(otmp));
#endif /*JP*/
		if(!Poison_resistance) {
		    losestr(rnd(4));
		    losehp(rnd(15), xname(otmp), KILLED_BY_AN);
		} else
		    You(E_J("seem unaffected by the poison.","�ł̉e�����󂯂Ȃ��悤���B"));
	    } else if (!otmp->cursed)
		pline(E_J("This %s is delicious!","����%s�͂��܂��I"),
		      otmp->oclass == COIN_CLASS ? foodword(otmp) :
		      singular(otmp, xname));

	    eatspecial();
	    return 1;
	}

	if(otmp == victual.piece) {
	/* If they weren't able to choke, they don't suddenly become able to
	 * choke just because they were interrupted.  On the other hand, if
	 * they were able to choke before, if they lost food it's possible
	 * they shouldn't be able to choke now.
	 */
	    if (u.uhs != SATIATED) victual.canchoke = FALSE;
	    victual.piece = touchfood(otmp);
	    You(E_J("resume your meal.","�H�����ĊJ�����B"));
	    start_eating(victual.piece);
	    return(1);
	}

	/* nothing in progress - so try to find something. */
	/* tins are a special case */
	/* tins must also check conduct separately in case they're discarded */
	if(otmp->otyp == TIN) {
	    start_tin(otmp);
	    return(1);
	}

	/* KMH, conduct */
	u.uconduct.food++;

	victual.piece = otmp = touchfood(otmp);
	victual.usedtime = 0;

	/* Now we need to calculate delay and nutritional info.
	 * The base nutrition calculated here and in eatcorpse() accounts
	 * for normal vs. rotten food.  The reqtime and nutrit values are
	 * then adjusted in accordance with the amount of food left.
	 */
	if(otmp->otyp == CORPSE) {
	    int tmp = eatcorpse(otmp);
	    if (tmp == 2) {
		/* used up */
		victual.piece = (struct obj *)0;
		return(1);
	    } else if (tmp)
		dont_start = TRUE;
	    /* if not used up, eatcorpse sets up reqtime and may modify
	     * oeaten */
	} else {
	    /* No checks for WAX, LEATHER, BONE, DRAGON_HIDE.  These are
	     * all handled in the != FOOD_CLASS case, above */
	    switch (objects[otmp->otyp].oc_material) {
	    case FLESH:
		u.uconduct.unvegan++;
		if (otmp->otyp != EGG) {
		    violated_vegetarian();
		}
		break;

	    default:
		if (otmp->otyp == PANCAKE ||
		    otmp->otyp == FORTUNE_COOKIE || /* eggs */
		    otmp->otyp == CREAM_PIE ||
		    otmp->otyp == CANDY_BAR || /* milk */
		    otmp->otyp == LUMP_OF_ROYAL_JELLY)
		    u.uconduct.unvegan++;
		break;
	    }

	    victual.reqtime = objects[otmp->otyp].oc_delay;
	    if (victual.rotten) {

		if (rottenfood(otmp)) {
		    dont_start = TRUE;
		}
		consume_oeaten(otmp, 1);	/* oeaten >>= 1 */
	    } else fprefx(otmp);
	}

	/* re-calc the nutrition */
	if (otmp->otyp == CORPSE) basenutrit = mons[otmp->corpsenm].cnutrit;
	else basenutrit = objects[otmp->otyp].oc_nutrition;

#ifdef DEBUG
	debugpline("before rounddiv: victual.reqtime == %d", victual.reqtime);
	debugpline("oeaten == %d, basenutrit == %d", otmp->oeaten, basenutrit);
#endif
	victual.reqtime = (basenutrit == 0 ? 0 :
		rounddiv(victual.reqtime * (long)otmp->oeaten, basenutrit));
#ifdef DEBUG
	debugpline("after rounddiv: victual.reqtime == %d", victual.reqtime);
#endif
	/* calculate the modulo value (nutrit. units per round eating)
	 * note: this isn't exact - you actually lose a little nutrition
	 *	 due to this method.
	 * TODO: add in a "remainder" value to be given at the end of the
	 *	 meal.
	 */
	if (victual.reqtime == 0 || otmp->oeaten == 0)
	    /* possible if most has been eaten before */
	    victual.nmod = 0;
	else if ((int)otmp->oeaten >= victual.reqtime)
	    victual.nmod = -((int)otmp->oeaten / victual.reqtime);
	else
	    victual.nmod = victual.reqtime % otmp->oeaten;
	victual.canchoke = (u.uhs == SATIATED);

	if (!dont_start) start_eating(otmp);
	return(1);
}

/* Take a single bite from a piece of food, checking for choking and
 * modifying usedtime.  Returns 1 if they choked and survived, 0 otherwise.
 */
STATIC_OVL int
bite()
{
	if(victual.canchoke && u.uhunger >= 2000) {
		choke(victual.piece);
		return 1;
	}
	if (victual.doreset) {
		do_reset_eat();
		return 0;
	}
	force_save_hs = TRUE;
	if(victual.nmod < 0) {
		lesshungry(-victual.nmod);
		consume_oeaten(victual.piece, victual.nmod); /* -= -nmod */
	} else if(victual.nmod > 0 && (victual.usedtime % victual.nmod)) {
		lesshungry(1);
		consume_oeaten(victual.piece, -1);		  /* -= 1 */
	}
	force_save_hs = FALSE;
	recalc_wt();
	return 0;
}

#endif /* OVLB */
#ifdef OVL0

void
gethungry()	/* as time goes by - called by moveloop() and domove() */
{
	if (u.uinvulnerable) return;	/* you don't feel hungrier */

	if ((!u.usleep || !rn2(10))	/* slow metabolic rate while asleep */
		&& (carnivorous(youmonst.data) || herbivorous(youmonst.data))
		&& !Slow_digestion)
	    u.uhunger--;		/* ordinary food consumption */

	if (moves % 2) {	/* odd turns */
	    /* Regeneration uses up food, unless due to an artifact */
	    if (HRegeneration ||
		 ((ERegeneration & (~W_ART)) &&
		  (ERegeneration != W_WEP || !uwep->oartifact)))
			u.uhunger--;
	    if (near_capacity() > SLT_ENCUMBER) u.uhunger--;
	} else {		/* even turns */
	    if (Hunger) u.uhunger--;
	    /* Conflict uses up food too */
	    if (HConflict || (EConflict & (~W_ARTI))) u.uhunger--;
	    /* +0 charged rings don't do anything, so don't affect hunger */
	    /* Slow digestion still uses ring hunger */
	    switch ((int)(moves % 20)) {	/* note: use even cases only */
	     case  4: if (uleft &&
			  (uleft->spe || !objects[uleft->otyp].oc_charged))
			    u.uhunger--;
		    break;
	     case  8: if (uamul) u.uhunger--;
		    break;
	     case 12: if (uright &&
			  (uright->spe || !objects[uright->otyp].oc_charged))
			    u.uhunger--;
		    break;
	     case 16: if (u.uhave.amulet) u.uhunger--;
		    break;
	     default: break;
	    }
	}
	newuhs(TRUE);
}

#endif /* OVL0 */
#ifdef OVLB

void
morehungry(num)	/* called after vomiting and after performing feats of magic */
register int num;
{
	u.uhunger -= num;
	newuhs(TRUE);
}


void
lesshungry(num)	/* called after eating (and after drinking fruit juice) */
register int num;
{
	/* See comments in newuhs() for discussion on force_save_hs */
	boolean iseating = (occupation == eatfood) || force_save_hs;
#ifdef DEBUG
	debugpline("lesshungry(%d)", num);
#endif
	u.uhunger += num;
	if(u.uhunger >= 2000) {
	    if (!iseating || victual.canchoke) {
		if (iseating) {
		    choke(victual.piece);
		    reset_eat();
		} else
		    choke(occupation == opentin ? tin.tin : (struct obj *)0);
		/* no reset_eat() */
	    }
	} else {
	    /* Have lesshungry() report when you're nearly full so all eating
	     * warns when you're about to choke.
	     */
	    if (u.uhunger >= 1500) {
		if (!victual.eating || (victual.eating && !victual.fullwarn)) {
		    pline(E_J("You're having a hard time getting all of it down.",
			      "���Ȃ��͑S�Ă����ݍ��ނ̂ɂƂĂ���J���Ă���B"));
		    nomovemsg = E_J("You're finally finished.","���Ȃ��͂悤�₭�H�׏I�����B");
		    if (!victual.eating)
			multi = -2;
		    else {
			victual.fullwarn = TRUE;
			if (victual.canchoke && victual.reqtime > 1) {
			    /* a one-gulp food will not survive a stop */
			    if (yn_function(E_J("Stop eating?","�H�ׂ�̂𒆒f���܂����H"),ynchars,'y')=='y') {
				reset_eat();
				nomovemsg = (char *)0;
			    }
			}
		    }
		}
	    }
	}
	newuhs(FALSE);
}

STATIC_PTR
int
unfaint()
{
	(void) Hear_again();
	if(u.uhs > FAINTING)
		u.uhs = FAINTING;
	stop_occupation();
	flags.botl = 1;
	return 0;
}

#endif /* OVLB */
#ifdef OVL0

boolean
is_fainted()
{
	return((boolean)(u.uhs == FAINTED));
}

void
reset_faint()	/* call when a faint must be prematurely terminated */
{
	if(is_fainted()) nomul(0);
}

#if 0
void
sync_hunger()
{

	if(is_fainted()) {

		flags.soundok = 0;
		nomul(-10+(u.uhunger/10));
		nomovemsg = E_J("You regain consciousness.","���Ȃ��͈ӎ������߂����B"); /* see unconscious() */
		afternmv = unfaint;
	}
}
#endif

void
newuhs(incr)		/* compute and comment on your (new?) hunger status */
boolean incr;
{
	unsigned newhs;
	static unsigned save_hs;
	static boolean saved_hs = FALSE;
	int h = u.uhunger;

	newhs = (h > 1000) ? SATIATED :
		(h > 150) ? NOT_HUNGRY :
		(h > 50) ? HUNGRY :
		(h > 0) ? WEAK : FAINTING;

	/* While you're eating, you may pass from WEAK to HUNGRY to NOT_HUNGRY.
	 * This should not produce the message "you only feel hungry now";
	 * that message should only appear if HUNGRY is an endpoint.  Therefore
	 * we check to see if we're in the middle of eating.  If so, we save
	 * the first hunger status, and at the end of eating we decide what
	 * message to print based on the _entire_ meal, not on each little bit.
	 */
	/* It is normally possible to check if you are in the middle of a meal
	 * by checking occupation == eatfood, but there is one special case:
	 * start_eating() can call bite() for your first bite before it
	 * sets the occupation.
	 * Anyone who wants to get that case to work _without_ an ugly static
	 * force_save_hs variable, feel free.
	 */
	/* Note: If you become a certain hunger status in the middle of the
	 * meal, and still have that same status at the end of the meal,
	 * this will incorrectly print the associated message at the end of
	 * the meal instead of the middle.  Such a case is currently
	 * impossible, but could become possible if a message for SATIATED
	 * were added or if HUNGRY and WEAK were separated by a big enough
	 * gap to fit two bites.
	 */
	if (occupation == eatfood || force_save_hs) {
		if (!saved_hs) {
			save_hs = u.uhs;
			saved_hs = TRUE;
		}
		u.uhs = newhs;
		return;
	} else {
		if (saved_hs) {
			u.uhs = save_hs;
			saved_hs = FALSE;
		}
	}

	if(newhs == FAINTING) {
		if(is_fainted()) newhs = FAINTED;
		if(u.uhs <= WEAK || rn2(20-u.uhunger/10) >= 19) {
			if(!is_fainted() && multi >= 0 /* %% */) {
				/* stop what you're doing, then faint */
				stop_occupation();
				You(E_J("faint from lack of food.","�󕠂̂��܂�C�₵���B"));
				flags.soundok = 0;
				nomul(-10+(u.uhunger/10));
				nomovemsg = E_J("You regain consciousness.","���Ȃ��͈ӎ������߂����B"); /* see unconscious() */
				afternmv = unfaint;
				newhs = FAINTED;
			}
		} else
		if(u.uhunger < -(int)(200 + 20*ACURR(A_CON))) {
			u.uhs = STARVED;
			flags.botl = 1;
			bot();
			You(E_J("die from starvation.","�쎀�����B"));
			killer_format = KILLED_BY;
			killer = E_J("starvation","�Q����");
			done(STARVING);
			/* if we return, we lifesaved, and that calls newuhs */
			return;
		}
	}

	if(newhs != u.uhs) {
		if(newhs >= WEAK && u.uhs < WEAK)
			losestr(1);	/* this may kill you -- see below */
		else if(newhs < WEAK && u.uhs >= WEAK)
			losestr(-1);
		switch(newhs){
		case HUNGRY:
			if (Hallucination) {
			    You((!incr) ?
				E_J("now have a lesser case of the munchies.",
				    "���y�R���x���͌y�x�ɂȂ����B") :
				E_J("are getting the munchies.",
				    "���y�R�ɂȂ����B"));
			} else
			    You((!incr) ? E_J("only feel hungry now.",
					      "���シ��قǋ󕠂ł͂Ȃ��Ȃ����B") :
				  (u.uhunger < 145) ? E_J("feel hungry.","�󕠂������Ă���B") :
				   E_J("are beginning to feel hungry.",
				       "�󕠂������n�߂��B"));
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
			break;
		case WEAK:
			if (Hallucination)
			    pline((!incr) ?
				  E_J("You still have the munchies.",
				      "���Ȃ��͂܂��܂����y�R���B") :
      E_J("The munchies are interfering with your motor capabilities.",
	  "���y�R���x���̏㏸�ŁA���Ȃ��̋쓮���\�ɉe�����o�n�߂��B"));
			else if (incr &&
				(Role_if(PM_WIZARD) || Race_if(PM_ELF) ||
				 Role_if(PM_VALKYRIE)))
#ifndef JP
			    pline("%s needs food, badly!",
				  (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE)) ?
				  urole.name.m : "Elf");
#else
			    pline("%s needs food, badly!",
				  Role_if(PM_WIZARD) ? "Wizard" :
				  Role_if(PM_VALKYRIE) ? "Valkyrie" : "Elf");
#endif /*JP*/
			else
			    You((!incr) ? E_J("feel weak now.","�܂����サ�Ă���B") :
				  (u.uhunger < 45) ? E_J("feel weak.","���サ�Ă���B") :
				   E_J("are beginning to feel weak.",
				       "���サ�Ă����B"));
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
			break;
		}
		u.uhs = newhs;
		flags.botl = 1;
		bot();
		if ((Upolyd ? u.mh : u.uhp) < 1) {
			You(E_J("die from hunger and exhaustion.",
				"�󕠂ɂ�鐊��Ŏ��񂾁B"));
			killer_format = E_J(KILLED_BY,NO_KILLER_PREFIX);
			killer = E_J("exhaustion","�󕠂ɂ�鐊���");
			done(STARVING);
			return;
		}
	}
}

#endif /* OVL0 */
#ifdef OVLB

int
getobj_filter_eat(otmp)
struct obj *otmp;
{
	return (is_edible(otmp) ? GETOBJ_CHOOSEIT : 0);
}
int
getobj_filter_sacrifice(otmp)
struct obj *otmp;
{
	return ((otmp->otyp == CORPSE) ? GETOBJ_CHOOSEIT : 0);
}
int
getobj_filter_tin(otmp)
struct obj *otmp;
{
	return ((otmp->otyp == CORPSE && tinnable(otmp)) ? GETOBJ_CHOOSEIT : 0);
}

/* Returns an object representing food.  Object may be either on floor or
 * in inventory.
 */
struct obj *
floorfood(verb,corpsecheck)	/* get food from floor or pack */
#ifndef JP
	const char *verb;
#else
	const struct getobj_words *verb;
#endif /*JP*/
	int corpsecheck; /* 0, no check, 1, corpses, 2, tinnable corpses */
{
	register struct obj *otmp;
	char qbuf[QBUFSZ];
	char c;
	boolean feeding = (E_J(!strcmp(verb, "eat"),!strncmp(verb->suru, "�H�ׂ�", 6)));
	int (*filter)(struct obj *);

	/* if we can't touch floor objects then use invent food only */
	if (!can_reach_floor() ||
#ifdef STEED
		(feeding && u.usteed) || /* can't eat off floor while riding */
#endif
		((is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) &&
		    (Wwalking || is_clinger(youmonst.data) ||
			(Flying && !Breathless))))
	    goto skipfloor;

	if (feeding && metallivorous(youmonst.data)) {
	    struct obj *gold;
	    struct trap *ttmp = t_at(u.ux, u.uy);

	    if (ttmp && ttmp->tseen && ttmp->ttyp == BEAR_TRAP) {
		/* If not already stuck in the trap, perhaps there should
		   be a chance to becoming trapped?  Probably not, because
		   then the trap would just get eaten on the _next_ turn... */
		Sprintf(qbuf, E_J("There is a bear trap here (%s); eat it?",
				  "�����ɂ�%s�g���o�T�~������B�H�ׂ܂����H"),
			(u.utrap && u.utraptype == TT_BEARTRAP) ?
				E_J("holding you","���Ȃ���߂炦�Ă���") : E_J("armed","�d�|����ꂽ"));
		if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		    u.utrap = u.utraptype = 0;
		    deltrap(ttmp);
		    return mksobj(BEARTRAP, TRUE, FALSE);
		} else if (c == 'q') {
		    return (struct obj *)0;
		}
	    }

	    if (youmonst.data != &mons[PM_RUST_MONSTER] &&
		(gold = g_at(u.ux, u.uy)) != 0) {
#ifndef JP
		if (gold->quan == 1L)
		    Sprintf(qbuf, "There is 1 gold piece here; eat it?");
		else
		    Sprintf(qbuf, "There are %ld gold pieces here; eat them?",
			    gold->quan);
#else
		Sprintf(qbuf, "�����ɂ�%ld���̋��݂�����B�H�ׂ܂����H",
			gold->quan);
#endif /*JP*/
		if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		    return gold;
		} else if (c == 'q') {
		    return (struct obj *)0;
		}
	    }
	}

	/* Is there some food (probably a heavy corpse) here on the ground? */
	for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
		if(corpsecheck ?
		(otmp->otyp==CORPSE && (corpsecheck == 1 || tinnable(otmp))) :
		    feeding ? (otmp->oclass != COIN_CLASS && is_edible(otmp)) :
						otmp->oclass==FOOD_CLASS) {
#ifndef JP
			Sprintf(qbuf, "There %s %s here; %s %s?",
				otense(otmp, "are"),
				doname(otmp), verb,
				(otmp->quan == 1L) ? "it" : "one");
#else
			Sprintf(qbuf, "�����ɂ�%s������B%s%s�܂����H",
				doname(otmp),
				(otmp->quan == 1L) ? "" : "���", verb->shimasu);
#endif /*JP*/
			if((c = yn_function(qbuf,ynqchars,'n')) == 'y')
				return(otmp);
			else if(c == 'q')
				return((struct obj *) 0);
		}
	}
 skipfloor:
	/* We cannot use ALL_CLASSES since that causes getobj() to skip its
	 * "ugly checks" and we need to check for inedible items.
	 */
	filter = (corpsecheck == 0) ? getobj_filter_eat :
		 (corpsecheck == 1) ? getobj_filter_sacrifice :
				      getobj_filter_tin;
	otmp = getobj(feeding ? (const char *)allobj :
				(const char *)comestibles, verb, filter);
	if (corpsecheck && otmp)
	    if (otmp->otyp != CORPSE || (corpsecheck == 2 && !tinnable(otmp))) {
#ifndef JP
		You_cant("%s that!", verb);
#else
		You("�����%s���Ƃ͂ł��Ȃ��I", verb->suru);
#endif /*JP*/
		return (struct obj *)0;
	    }
	return otmp;
}

/* Side effects of vomiting */
/* added nomul (MRS) - it makes sense, you're too busy being sick! */
void
vomit()		/* A good idea from David Neves */
{
	make_sick(0L, (char *) 0, TRUE, SICK_VOMITABLE);
	nomul(-2);
}

int
eaten_stat(base, obj)
register int base;
register struct obj *obj;
{
	long uneaten_amt, full_amount;

	uneaten_amt = (long)obj->oeaten;
	full_amount = (obj->otyp == CORPSE) ? (long)mons[obj->corpsenm].cnutrit
					: (long)objects[obj->otyp].oc_nutrition;
	if (uneaten_amt > full_amount) {
	    impossible(
	  "partly eaten food (%ld) more nutritious than untouched food (%ld)",
		       uneaten_amt, full_amount);
	    uneaten_amt = full_amount;
	}

	base = (int)(full_amount ? (long)base * uneaten_amt / full_amount : 0L);
	return (base < 1) ? 1 : base;
}

/* reduce obj's oeaten field, making sure it never hits or passes 0 */
void
consume_oeaten(obj, amt)
struct obj *obj;
int amt;
{
    /*
     * This is a hack to try to squelch several long standing mystery
     * food bugs.  A better solution would be to rewrite the entire
     * victual handling mechanism from scratch using a less complex
     * model.  Alternatively, this routine could call done_eating()
     * or food_disappears() but its callers would need revisions to
     * cope with victual.piece unexpectedly going away.
     *
     * Multi-turn eating operates by setting the food's oeaten field
     * to its full nutritional value and then running a counter which
     * independently keeps track of whether there is any food left.
     * The oeaten field can reach exactly zero on the last turn, and
     * the object isn't removed from inventory until the next turn
     * when the "you finish eating" message gets delivered, so the
     * food would be restored to the status of untouched during that
     * interval.  This resulted in unexpected encumbrance messages
     * at the end of a meal (if near enough to a threshold) and would
     * yield full food if there was an interruption on the critical
     * turn.  Also, there have been reports over the years of food
     * becoming massively heavy or producing unlimited satiation;
     * this would occur if reducing oeaten via subtraction attempted
     * to drop it below 0 since its unsigned type would produce a
     * huge positive value instead.  So far, no one has figured out
     * _why_ that inappropriate subtraction might sometimes happen.
     */

    if (amt > 0) {
	/* bit shift to divide the remaining amount of food */
	obj->oeaten >>= amt;
    } else {
	/* simple decrement; value is negative so we actually add it */
	if ((int) obj->oeaten > -amt)
	    obj->oeaten += amt;
	else
	    obj->oeaten = 0;
    }

    if (obj->oeaten == 0) {
	if (obj == victual.piece)	/* always true unless wishing... */
	    victual.reqtime = victual.usedtime;	/* no bites left */
	obj->oeaten = 1;	/* smallest possible positive value */
    }
}

#endif /* OVLB */
#ifdef OVL1

/* called when eatfood occupation has been interrupted,
   or in the case of theft, is about to be interrupted */
boolean
maybe_finished_meal(stopping)
boolean stopping;
{
	/* in case consume_oeaten() has decided that the food is all gone */
	if (occupation == eatfood && victual.usedtime >= victual.reqtime) {
	    if (stopping) occupation = 0;	/* for do_reset_eat */
	    (void) eatfood(); /* calls done_eating() to use up victual.piece */
	    return TRUE;
	}
	return FALSE;
}

#endif /* OVL1 */

void
set_rotten_flag(otmp)
struct obj *otmp;
{
	long rotted = 0L;
	int mnum = otmp->corpsenm;
	boolean stoneable = (touch_petrifies(&mons[mnum]) && !Stone_resistance &&
				!poly_when_stoned(youmonst.data));

	victual.tainted = 0;
	victual.rotsick = 0;
	victual.rotten  = 0;
	victual.poison  = 0;

	if (otmp->oclass != FOOD_CLASS) {
	    if (otmp->cursed) victual.rotten = 1;
	    if (otmp->oclass == WEAPON_CLASS && otmp->opoisoned) victual.poison = 1;
	} else {
	    if (otmp->otyp == CORPSE) {
		if (mnum != PM_LIZARD && mnum != PM_LICHEN) {
			long age = peek_at_iced_corpse_age(otmp);

			rotted = (monstermoves - age)/(10L + rn2(20));
			if (otmp->cursed) rotted += 2L;
			else if (otmp->blessed) rotted -= 2L;
		}

		if ((mnum != PM_ACID_BLOB && !stoneable && rotted > 5L) ||
		    otmp->orotten == 3) {
			victual.tainted = 1;
			otmp->orotten = 3;
		} else if (acidic(&mons[mnum]) && !Acid_resistance) {
		} else if ((poisonous(&mons[mnum]) && rn2(5)) ||
			   otmp->opoisoned) {
			victual.poison = 1;
			otmp->opoisoned = 1;
		/* now any corpse left too long will make you mildly ill */
		} else if (((rotted > 5L || (rotted > 3L && rn2(5)))
						&& !Sick_resistance) ||
			   otmp->orotten == 2) {
			victual.rotsick = 1;
			otmp->orotten = 2;
		} else if ((mnum != PM_LIZARD && mnum != PM_LICHEN &&
				(otmp->orotten || !rn2(7))) ||
			   otmp->orotten == 1) {
			victual.rotten = 1;
			otmp->orotten = 1;
		}
	    } else {
		if (otmp->otyp != FORTUNE_COOKIE &&
		    (otmp->cursed ||
		     (((monstermoves - otmp->age) > (int) otmp->blessed ? 50:30) &&
		      (otmp->orotten || !rn2(7)))))
			victual.rotten = 1;
	    }
	}
}
/*eat.c*/
