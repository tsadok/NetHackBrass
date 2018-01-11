/*	SCCS Id: @(#)do_wear.c	3.4	2003/11/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifndef OVLB

STATIC_DCL long takeoff_mask, taking_off;

#else /* OVLB */

STATIC_OVL NEARDATA long takeoff_mask = 0L;
static NEARDATA long taking_off = 0L;

static NEARDATA int todelay;
static boolean cancelled_don = FALSE;

static NEARDATA const char see_yourself[] = "see yourself";
static NEARDATA const char unknown_type[] = "Unknown type of %s (%d)";
static NEARDATA const char c_armor[]  = E_J("armor","�Z"),
			   c_suit[]   = E_J("suit","����"),
#ifdef TOURIST
			   c_shirt[]  = E_J("shirt","�V���c"),
#endif
			   c_cloak[]  = E_J("cloak","�N���[�N"),
			   c_gloves[] = E_J("gloves","�U��"),
			   c_boots[]  = E_J("boots","�u�[�c"),
			   c_helmet[] = E_J("helmet","��"),
			   c_shield[] = E_J("shield","��"),
			   c_weapon[] = E_J("weapon","����"),
			   c_sword[]  = E_J("sword","��"),
			   c_axe[]    = E_J("axe","��"),
			   c_that_[]  = E_J("that","����");

static NEARDATA const long takeoff_order[] = { WORN_BLINDF, W_WEP,
	WORN_SHIELD, WORN_GLOVES, LEFT_RING, RIGHT_RING, WORN_CLOAK,
	WORN_HELMET, WORN_AMUL, WORN_ARMOR,
#ifdef TOURIST
	WORN_SHIRT,
#endif
	WORN_BOOTS, W_SWAPWEP, W_QUIVER, 0L };

STATIC_DCL void FDECL(on_msg, (struct obj *));
STATIC_PTR void NDECL(Armor_off_sub);
STATIC_PTR int NDECL(Armor_on);
STATIC_PTR int NDECL(Boots_on);
STATIC_DCL int NDECL(Cloak_on);
STATIC_PTR int NDECL(Helmet_on);
STATIC_PTR int NDECL(Gloves_on);
STATIC_PTR int NDECL(Shield_on);
#ifdef TOURIST
STATIC_PTR int NDECL(Shirt_on);
#endif
STATIC_DCL void NDECL(Amulet_on);
STATIC_DCL void FDECL(Ring_off_or_gone, (struct obj *, BOOLEAN_P));
STATIC_PTR int FDECL(select_off, (struct obj *));
STATIC_DCL struct obj *NDECL(do_takeoff);
STATIC_PTR int NDECL(take_off);
STATIC_DCL int FDECL(menu_remarm, (int));
STATIC_DCL void FDECL(already_wearing, (const char*));
STATIC_DCL void FDECL(already_wearing2, (const char*, const char*));
STATIC_DCL void FDECL(get_armor_on_off_info, (struct obj *, boolean, int (**)(), char **));
STATIC_DCL int FDECL(getobj_filter_takeoff, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_wear, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_puton, (struct obj *));
STATIC_DCL int FDECL(getobj_filter_remove, (struct obj *));
STATIC_DCL int FDECL(see_invis_on, (long));
STATIC_PTR int NDECL(see_invis_off);

static NEARDATA const struct armor_on_off_info{
	int	(*on_func)();
	int	(*off_func)();
	struct obj **uarmptr;
	long	mask;
	char	*delayedoffmsg;
} armor_on_off_infotbl[ARM_MAXNUM] = {	/* the order must be follow the order of ARM_xxx */
    {	/* body suit */
	Armor_on, Armor_off, &uarm, WORN_ARMOR,
	E_J("You finish taking off your suit.","���Ȃ��͊Z��E���I�����B")
    },
    {	/* shield */
	Shield_on, Shield_off, &uarms, WORN_SHIELD,
	(char *)0
    },
    {	/* helmet */
	Helmet_on, Helmet_off, &uarmh, WORN_HELMET,
	E_J("You finish taking off your helmet.","���Ȃ��͊���E���I�����B")
    },
    {	/* gloves */
	Gloves_on, Gloves_off, &uarmg, WORN_GLOVES,
	E_J("You finish taking off your gloves.","���Ȃ��͘U����O���I�����B")
    },
    {	/* boots */
	Boots_on, Boots_off, &uarmf, WORN_BOOTS,
	E_J("You finish taking off your boots.","���Ȃ��̓u�[�c��E���I�����B")
    },
    {	/* cloak */
	Cloak_on, Cloak_off, &uarmc, WORN_CLOAK,
	(char *)0
    },
#ifdef TOURIST
    {	/* shirt */
	Shirt_on, Shirt_off, &uarmu, WORN_SHIRT,
	(char *)0
    }
#endif /*TOURIST*/
};
#define otyp2uarmp(otyp) (armor_on_off_infotbl[objects[otyp].oc_armcat].uarmptr)
#define otyp2mask(otyp)  (armor_on_off_infotbl[objects[otyp].oc_armcat].mask)

void
off_msg(otmp)
register struct obj *otmp;
{
	if(flags.verbose)
#ifndef JP
	    You("were wearing %s.", doname(otmp));
#else
	    You("%s��%s�B", doname(otmp),
		(otmp->oclass == ARMOR_CLASS) && !is_shield(otmp) ?
		"�E����" : "�O����");
#endif /*JP*/
}

/* for items that involve no delay */
STATIC_OVL void
on_msg(otmp)
register struct obj *otmp;
{
	if (flags.verbose) {
#ifndef JP
	    char how[BUFSZ];

	    how[0] = '\0';
	    if (otmp->otyp == TOWEL)
		Sprintf(how, " around your %s", body_part(HEAD));
	    You("are now wearing %s%s.",
		obj_is_pname(otmp) ? the(xname(otmp)) : an(xname(otmp)),
		how);
#else
	    if (otmp->otyp == TOWEL)
		You("%s��������%s�𕢂����B", xname(otmp), body_part(EYE));
#ifdef MAGIC_GLASSES
	    else if (Is_glasses(otmp))
		You("%s���������B", xname(otmp));
#endif /*MAGIC_GLASSES*/
	    else
		You("%s��g�ɂ����B", xname(otmp));
#endif /*JP*/
	}
}

/*
 * The Type_on() functions should be called *after* setworn().
 * The Type_off() functions call setworn() themselves.
 */

STATIC_PTR
int
Boots_on()
{
    long oldprop =
	u.uprops[objects[uarmf->otyp].oc_oprop].extrinsic & ~WORN_BOOTS;

    switch(uarmf->otyp) {
	case LOW_BOOTS:
	case IRON_SHOES:
	case HIGH_BOOTS:
	case JUMPING_BOOTS:
	case KICKING_BOOTS:
		break;
	case WATER_WALKING_BOOTS:
		if (u.uinwater || is_lava(u.ux,u.uy) || is_swamp(u.ux,u.uy))
			spoteffects(TRUE);
		break;
	case SPEED_BOOTS:
		/* Speed boots are still better than intrinsic speed, */
		/* though not better than potion speed */
		makeknown(uarmf->otyp);
#ifndef JP
		You_feel("yourself speed up%s.",
			 (Fast) ? " a bit more" : "");
#else
		Your("������%s�����Ȃ����B",
			 (Fast) ? "�����" : "");
#endif /*JP*/
		break;
	case ELVEN_BOOTS:
		if (!oldprop && !HStealth && !BStealth) {
			makeknown(uarmf->otyp);
			You(E_J("walk very quietly.",
				"�ƂĂ��Â��ɕ�����悤�ɂȂ����B"));
		}
		break;
	case FUMBLE_BOOTS:
		if (!oldprop && !(HFumbling & ~TIMEOUT))
			incr_itimeout(&HFumbling, rnd(20));
		break;
	case LEVITATION_BOOTS:
		if (!oldprop && !HLevitation) {
			makeknown(uarmf->otyp);
			float_up();
			spoteffects(FALSE);
		}
		break;
	default: impossible(unknown_type, c_boots, uarmf->otyp);
    }
    return 0;
}

int
Boots_off()
{
    int otyp = uarmf->otyp;
    long oldprop = u.uprops[objects[otyp].oc_oprop].extrinsic & ~WORN_BOOTS;

    takeoff_mask &= ~W_ARMF;
	/* For levitation, float_down() returns if Levitation, so we
	 * must do a setworn() _before_ the levitation case.
	 */
    setworn((struct obj *)0, W_ARMF);
    switch (otyp) {
	case SPEED_BOOTS:
		if (!cancelled_don) {
			makeknown(otyp);
#ifndef JP
			You_feel("yourself slow down%s.",
				Fast ? " a bit" : "");
#else
			Your("������%s�x���Ȃ����B",
				Fast ? "���" : "");
#endif /*JP*/
		}
		break;
	case WATER_WALKING_BOOTS:
		if ((is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy) || is_swamp(u.ux,u.uy)) &&
		    !Levitation && !Flying &&
		    !is_clinger(youmonst.data) && !cancelled_don) {
			makeknown(otyp);
			/* make boots known in case you survive the drowning */
			spoteffects(TRUE);
		}
		break;
	case ELVEN_BOOTS:
		if (!oldprop && !HStealth && !BStealth && !cancelled_don) {
			makeknown(otyp);
			You(E_J("sure are noisy.","���X�����Ȃ����B"));
		}
		break;
	case FUMBLE_BOOTS:
		if (!oldprop && !(HFumbling & ~TIMEOUT))
			HFumbling = EFumbling = 0;
		break;
	case LEVITATION_BOOTS:
		if (!oldprop && !HLevitation && !cancelled_don) {
			(void) float_down(0L, 0L);
			makeknown(otyp);
		}
		break;
	case LOW_BOOTS:
	case IRON_SHOES:
	case HIGH_BOOTS:
	case JUMPING_BOOTS:
	case KICKING_BOOTS:
		break;
	default: impossible(unknown_type, c_boots, otyp);
    }
    cancelled_don = FALSE;
    return 0;
}

STATIC_OVL int
Cloak_on()
{
    long oldprop =
	u.uprops[objects[uarmc->otyp].oc_oprop].extrinsic & ~WORN_CLOAK;

    switch(uarmc->otyp) {
	case CLOAK_OF_PROTECTION:
		u.uprotection += 2;
		/* fallthru */
	case ELVEN_CLOAK:
	case CLOAK_OF_DISPLACEMENT:
		makeknown(uarmc->otyp);
		break;
	case KITCHEN_APRON:
	case FRILLED_APRON:
		refresh_ladies_wear(0);
		break;
	case ORCISH_CLOAK:
	case DWARVISH_CLOAK:
	case CLOAK_OF_MAGIC_RESISTANCE:
//	case ROBE:
	case LEATHER_CLOAK:
		break;
	case MUMMY_WRAPPING:
		/* Note: it's already being worn, so we have to cheat here. */
		if ((HInvis || EInvis || pm_invisible(youmonst.data)) && !Blind) {
		    newsym(u.ux,u.uy);
#ifndef JP
		    You("can %s!",
			See_invisible ? "no longer see through yourself"
			: see_yourself);
#else
		    if (See_invisible)
			Your("�g�̂͂��������Ă��Ȃ��I");
		    else
			You("�����̎p��������悤�ɂȂ����I");
#endif /*JP*/
		}
		break;
	case CLOAK_OF_INVISIBILITY:
		/* since cloak of invisibility was worn, we know mummy wrapping
		   wasn't, so no need to check `oldprop' against blocked */
		if (!oldprop && !HInvis && !Blind) {
		    makeknown(uarmc->otyp);
		    newsym(u.ux,u.uy);
#ifndef JP
		    pline("Suddenly you can%s yourself.",
			See_invisible ? " see through" : "not see");
#else
		    pline("�ˑR�A���Ȃ�%s�����I",
			See_invisible ? "�̐g�̂�������" : "�͎����������Ȃ���");
#endif /*JP*/
		}
		break;
	case OILSKIN_CLOAK:
#ifndef JP
		pline("%s very tightly.", Tobjnam(uarmc, "fit"));
#else
		pline("%s�͐g�̂ɂ҂�����ƃt�B�b�g�����B", xname(uarmc));
#endif /*JP*/
		break;
	/* Alchemy smock gives poison _and_ acid resistance */
	case ALCHEMY_SMOCK:
		EAcid_resistance |= WORN_CLOAK;
  		break;
	default: impossible(unknown_type, c_cloak, uarmc->otyp);
    }
    return 0;
}

int
Cloak_off()
{
    struct obj *otmp = uarmc;
    int otyp = uarmc->otyp;
    long oldprop = u.uprops[objects[otyp].oc_oprop].extrinsic & ~WORN_CLOAK;

    takeoff_mask &= ~W_ARMC;
	/* For mummy wrapping, taking it off first resets `Invisible'. */
    setworn((struct obj *)0, W_ARMC);
    switch (otyp) {
	case CLOAK_OF_PROTECTION:
		u.uprotection -= 2;
		/* fallthru */
	case ELVEN_CLOAK:
	case ORCISH_CLOAK:
	case DWARVISH_CLOAK:
	case CLOAK_OF_MAGIC_RESISTANCE:
	case CLOAK_OF_DISPLACEMENT:
	case OILSKIN_CLOAK:
	case LEATHER_CLOAK:
		break;
	case KITCHEN_APRON:
	case FRILLED_APRON:
		refresh_ladies_wear(otmp);
		break;
	case MUMMY_WRAPPING:
		if (Invis && !Blind) {
		    newsym(u.ux,u.uy);
#ifndef JP
		    You("can %s.",
			See_invisible ? "see through yourself"
			: "no longer see yourself");
#else
		    You(See_invisible ? "�g�̂������ʂ��Č������B" :
			"�����̎p�������Ȃ��Ȃ����B");
#endif /*JP*/
		}
		break;
	case CLOAK_OF_INVISIBILITY:
		if (!oldprop && !HInvis && !Blind) {
		    makeknown(CLOAK_OF_INVISIBILITY);
		    newsym(u.ux,u.uy);
#ifndef JP
		    pline("Suddenly you can %s.",
			See_invisible ? "no longer see through yourself"
			: see_yourself);
#else
		    pline("�ˑR�A���Ȃ�%s�Ȃ����B",
			See_invisible ? "�̐g�̂̓��������Ȃ�"
			: "�͎����̎p��������悤��");
#endif /*JP*/
		}
		break;
	/* Alchemy smock gives poison _and_ acid resistance */
	case ALCHEMY_SMOCK:
		EAcid_resistance &= ~WORN_CLOAK;
  		break;
	default: impossible(unknown_type, c_cloak, otyp);
    }
    return 0;
}

STATIC_PTR
int
Helmet_on()
{
    takeoff_mask &= ~W_ARMH;

    switch(uarmh->otyp) {
	case NURSE_CAP:
		refresh_ladies_wear(0);
		break;
	case KATYUSHA:
		refresh_ladies_wear(0);
		/* fallthrough */
	case FEDORA:
                set_moreluck();       /* youkan */
                break;                /* youkan */
	case HELMET:
	case DENTED_POT:
	case ELVEN_LEATHER_HELM:
	case DWARVISH_IRON_HELM:
	case ORCISH_HELM:
	case HELM_OF_TELEPATHY:
		break;
	case HELM_OF_BRILLIANCE:
		adj_abon(uarmh, uarmh->spe);
		break;
	case CORNUTHAUM:
		/* people think marked wizards know what they're talking
		 * about, but it takes trained arrogance to pull it off,
		 * and the actual enchantment of the hat is irrelevant.
		 */
		ABON(A_CHA) += (Role_if(PM_WIZARD) ? 1 : -1);
		flags.botl = 1;
		makeknown(uarmh->otyp);
		break;
	case HELM_OF_OPPOSITE_ALIGNMENT:
		if (u.ualign.type == A_NEUTRAL)
		    u.ualign.type = rn2(2) ? A_CHAOTIC : A_LAWFUL;
		else u.ualign.type = -(u.ualign.type);
		u.ublessed = 0; /* lose your god's protection */
	     /* makeknown(uarmh->otyp);   -- moved below, after xname() */
		/*FALLTHRU*/
	case DUNCE_CAP:
		if (!uarmh->cursed) {
		    if (Blind)
#ifndef JP
			pline("%s for a moment.", Tobjnam(uarmh, "vibrate"));
#else
			pline("%s�͈�u�k�����B", xname(uarmh));
#endif /*JP*/
		    else
#ifndef JP
			pline("%s %s for a moment.",
			      Tobjnam(uarmh, "glow"), hcolor(NH_BLACK));
#else
			pline("%s�͈�u%s�P�����B",
			      xname(uarmh), j_no_ni(hcolor(NH_BLACK)));
#endif /*JP*/
		    curse(uarmh);
		}
		flags.botl = 1;		/* reveal new alignment or INT & WIS */
		if (Hallucination) {
		    pline(E_J("My brain hurts!","�́[�݂��o�[���I")); /* Monty Python's Flying Circus */
		} else if (uarmh->otyp == DUNCE_CAP) {
		    You_feel(E_J("%s.","%s�C���ɂȂ����B"),	/* track INT change; ignore WIS */
		  ACURR(A_INT) <= (ABASE(A_INT) + ABON(A_INT) + ATEMP(A_INT)) ?
			 E_J("like sitting in a corner","�L���ɗ�������Ă���悤��") :
			 E_J("giddy","�o�J���ۂ�"));
		} else {
		    Your(E_J("mind oscillates briefly.","�S�͂��΂��h���Ԃ�ꂽ�B"));
		    makeknown(HELM_OF_OPPOSITE_ALIGNMENT);
		}
		break;
	default: impossible(unknown_type, c_helmet, uarmh->otyp);
    }
    return 0;
}

int
Helmet_off()
{
    boolean recalc_luck = FALSE;           /* youkan */
    switch(uarmh->otyp) {
	case NURSE_CAP:
	    refresh_ladies_wear(uarmh);
	    break;
	case KATYUSHA:
	    refresh_ladies_wear(uarmh);
	    /* fallthrough */
	case FEDORA:
	    recalc_luck = TRUE;        /* youkan */
	    break;                     /* youkan */
	case HELMET:
	case DENTED_POT:
	case ELVEN_LEATHER_HELM:
	case DWARVISH_IRON_HELM:
	case ORCISH_HELM:
	    break;
	case DUNCE_CAP:
	    flags.botl = 1;
	    break;
	case CORNUTHAUM:
	    if (!cancelled_don) {
		ABON(A_CHA) += (Role_if(PM_WIZARD) ? -1 : 1);
		flags.botl = 1;
	    }
	    break;
	case HELM_OF_TELEPATHY:
	    /* need to update ability before calling see_monsters() */
	    setworn((struct obj *)0, W_ARMH);
	    see_monsters();
	    return 0;
	case HELM_OF_BRILLIANCE:
	    if (!cancelled_don) adj_abon(uarmh, -uarmh->spe);
	    break;
	case HELM_OF_OPPOSITE_ALIGNMENT:
	    u.ualign.type = u.ualignbase[A_CURRENT];
	    u.ublessed = 0; /* lose the other god's protection */
	    flags.botl = 1;
	    break;
	default: impossible(unknown_type, c_helmet, uarmh->otyp);
    }
    setworn((struct obj *)0, W_ARMH);
    if (recalc_luck) set_moreluck();      /* youkan */
    cancelled_don = FALSE;
    return 0;
}

STATIC_PTR
int
Gloves_on()
{
    long oldprop =
	u.uprops[objects[uarmg->otyp].oc_oprop].extrinsic & ~WORN_GLOVES;

    takeoff_mask &= ~W_ARMG;

    switch(uarmg->otyp) {
	case GAUNTLETS:
	case LEATHER_GLOVES:
		break;
	case GAUNTLETS_OF_FUMBLING:
		if (!oldprop && !(HFumbling & ~TIMEOUT))
			incr_itimeout(&HFumbling, rnd(20));
		break;
	case GAUNTLETS_OF_POWER:
	case GAUNTLETS_OF_DEXTERITY:
		makeknown(uarmg->otyp);
		flags.botl = 1; /* taken care of in attrib.c */
		break;
	default: impossible(unknown_type, c_gloves, uarmg->otyp);
    }
    return 0;
}

int
Gloves_off()
{
    long oldprop =
	u.uprops[objects[uarmg->otyp].oc_oprop].extrinsic & ~WORN_GLOVES;

    switch(uarmg->otyp) {
	case GAUNTLETS:
	case LEATHER_GLOVES:
	    break;
	case GAUNTLETS_OF_FUMBLING:
	    if (!oldprop && !(HFumbling & ~TIMEOUT))
		HFumbling = EFumbling = 0;
	    break;
	case GAUNTLETS_OF_POWER:
	case GAUNTLETS_OF_DEXTERITY:
	    makeknown(uarmg->otyp);
	    flags.botl = 1; /* taken care of in attrib.c */
	    break;
	default: impossible(unknown_type, c_gloves, uarmg->otyp);
    }
    setworn((struct obj *)0, W_ARMG);
    cancelled_don = FALSE;
    (void) encumber_msg();		/* immediate feedback for GoP */

    /* Prevent wielding cockatrice when not wearing gloves */
    if (uwep && uwep->otyp == CORPSE &&
		touch_petrifies(&mons[uwep->corpsenm])) {
	char kbuf[BUFSZ];

#ifndef JP
	You("wield the %s in your bare %s.",
	    corpse_xname(uwep, TRUE), makeplural(body_part(HAND)));
	Strcpy(kbuf, an(corpse_xname(uwep, TRUE)));
#else
	You("�f%s��%s���\�����B", body_part(HAND), corpse_xname(uwep, TRUE));
	Sprintf(kbuf, "%s�ɐG���", corpse_xname(uwep, TRUE));
#endif /*JP*/
	instapetrify(kbuf);
	uwepgone();  /* life-saved still doesn't allow touching cockatrice */
    }

    /* KMH -- ...or your secondary weapon when you're wielding it */
    if (u.twoweap && uswapwep && uswapwep->otyp == CORPSE &&
	touch_petrifies(&mons[uswapwep->corpsenm])) {
	char kbuf[BUFSZ];

#ifndef JP
	You("wield the %s in your bare %s.",
	    corpse_xname(uswapwep, TRUE), body_part(HAND));
	Strcpy(kbuf, an(corpse_xname(uswapwep, TRUE)));
#else
	You("�f%s��%s���\�����B", body_part(HAND), corpse_xname(uswapwep, TRUE));
	Sprintf(kbuf, "%s�ɐG���", corpse_xname(uswapwep, TRUE));
#endif /*JP*/
	instapetrify(kbuf);
	uswapwepgone();	/* lifesaved still doesn't allow touching cockatrice */
    }

    return 0;
}

STATIC_OVL int
Shield_on()
{
/*
    switch (uarms->otyp) {
	case SMALL_SHIELD:
	case ELVEN_SHIELD:
	case URUK_HAI_SHIELD:
	case ORCISH_SHIELD:
	case DWARVISH_ROUNDSHIELD:
	case LARGE_SHIELD:
	case SHIELD_OF_REFLECTION:
		break;
	default: impossible(unknown_type, c_shield, uarms->otyp);
    }
*/
    return 0;
}

int
Shield_off()
{
    takeoff_mask &= ~W_ARMS;
/*
    switch (uarms->otyp) {
	case SMALL_SHIELD:
	case ELVEN_SHIELD:
	case URUK_HAI_SHIELD:
	case ORCISH_SHIELD:
	case DWARVISH_ROUNDSHIELD:
	case LARGE_SHIELD:
	case SHIELD_OF_REFLECTION:
		break;
	default: impossible(unknown_type, c_shield, uarms->otyp);
    }
*/
    setworn((struct obj *)0, W_ARMS);
    return 0;
}

#ifdef TOURIST
STATIC_OVL int
Shirt_on()
{
/*
    switch (uarmu->otyp) {
	case HAWAIIAN_SHIRT:
	case T_SHIRT:
		break;
	default: impossible(unknown_type, c_shirt, uarmu->otyp);
    }
*/
    return 0;
}

int
Shirt_off()
{
    takeoff_mask &= ~W_ARMU;
/*
    switch (uarmu->otyp) {
	case HAWAIIAN_SHIRT:
	case T_SHIRT:
		break;
	default: impossible(unknown_type, c_shirt, uarmu->otyp);
    }
*/
    setworn((struct obj *)0, W_ARMU);
    return 0;
}
#endif	/*TOURIST*/

/* This must be done in worn.c, because one of the possible intrinsics conferred
 * is fire resistance, and we have to immediately set HFire_resistance in worn.c
 * since worn.c will check it before returning.
 */
STATIC_PTR
int
Armor_on()
{
    switch (uarm->otyp) {
	/* robe of protection conveys +2 protection */
	case ROBE_OF_PROTECTION:
		u.uprotection += 2;
		makeknown(uarm->otyp);
		break;
	case ROBE_OF_WEAKNESS:
		if (!uarmg ||
		    uarmg->otyp != GAUNTLETS_OF_POWER ||
		    uarmg->oartifact != ART_FIST_OF_FURY) {
			makeknown(uarm->otyp);
		}
		break;
	/* green dragon scales conveys poison _and_ sick resistance */
	case GREEN_DRAGON_SCALES:
	case GREEN_DRAGON_SCALE_MAIL:
		ESick_resistance |= W_ARM;
		break;
	/* ladies wear */
	case MAID_DRESS:
	case NURSE_UNIFORM:
		refresh_ladies_wear(0);
		break;
	default:
		break;
    }
    return 0;
}

void Armor_off_sub()
{
    switch (uarm->otyp) {
	case ROBE_OF_PROTECTION:
		u.uprotection -= 2;
		break;
	case GREEN_DRAGON_SCALES:
	case GREEN_DRAGON_SCALE_MAIL:
		ESick_resistance &= ~W_ARM;
		break;
	case MAID_DRESS:
	case NURSE_UNIFORM:
		refresh_ladies_wear(uarm);
		break;
	default:
		break;
    }
}

int
Armor_off()
{
    Armor_off_sub();
    takeoff_mask &= ~W_ARM;
    setworn((struct obj *)0, W_ARM);
    cancelled_don = FALSE;
    return 0;
}

/* The gone functions differ from the off functions in that if you die from
 * taking it off and have life saving, you still die.
 */
int
Armor_gone()
{
    Armor_off_sub();
    takeoff_mask &= ~W_ARM;
    setnotworn(uarm);
    cancelled_don = FALSE;
    return 0;
}

STATIC_OVL void
Amulet_on()
{
    switch(uamul->otyp) {
	case AMULET_OF_ESP:
	case AMULET_OF_LIFE_SAVING:
	case AMULET_VERSUS_POISON:
	case AMULET_OF_REFLECTION:
	case FAKE_AMULET_OF_YENDOR:
		break;
	case AMULET_OF_UNCHANGING:
		if (Slimed) {
		    Slimed = 0;
		    flags.botl = 1;
		}
		break;
	case AMULET_OF_CHANGE:
	    {
		int orig_sex = poly_gender();

		if (Unchanging) break;
		change_sex();
		/* Don't use same message as polymorph */
		if (orig_sex != poly_gender()) {
		    makeknown(AMULET_OF_CHANGE);
#ifndef JP
		    You("are suddenly very %s!", flags.female ? "feminine"
			: "masculine");
#else
		    You("�͋}�ɂƂĂ�%s���ۂ��Ȃ����I", flags.female ? "��" : "�j");
#endif /*JP*/
		    flags.botl = 1;
		} else
		    /* already polymorphed into single-gender monster; only
		       changed the character's base sex */
		    You(E_J("don't feel like yourself.","�����������łȂ��Ȃ����C�������B"));
		pline_The(E_J("amulet disintegrates!","�������͕��ꋎ�����I"));
		if (orig_sex == poly_gender() && uamul->dknown &&
			!objects[AMULET_OF_CHANGE].oc_name_known &&
			!objects[AMULET_OF_CHANGE].oc_uname)
		    docall(uamul);
		useup(uamul);
		break;
	    }
	case AMULET_OF_MAGICAL_BREATHING:
		if (StrangledBy & (SUFFO_WATER|SUFFO_NOAIR)) {
			makeknown(AMULET_OF_MAGICAL_BREATHING);
			end_suffocation(SUFFO_WATER|SUFFO_NOAIR);
		}
		break;
	case AMULET_OF_STRANGULATION:
		makeknown(AMULET_OF_STRANGULATION);
		if (amorphous(youmonst.data) || noncorporeal(youmonst.data) ||
		    unsolid(youmonst.data)) {
			pline(E_J("It tries to constrict your %s and passes through.",
				  "�������͂��Ȃ���%s���i�߂悤�Ƃ������A�ʂ蔲���Ă��܂����B"),
				body_part(NECK));
			setnotworn(uamul);
		} else {
			pline(E_J("It constricts your throat!",
				  "�������͂��Ȃ��̍A���i�߂����I"));
			start_suffocation(SUFFO_NECK);
		}
		break;
	case AMULET_OF_RESTFUL_SLEEP:
		HSleeping = rnd(100);
		break;
	case AMULET_OF_YENDOR:
		break;
    }
}

void
Amulet_off()
{
    takeoff_mask &= ~W_AMUL;

    switch(uamul->otyp) {
	case AMULET_OF_ESP:
		/* need to update ability before calling see_monsters() */
		setworn((struct obj *)0, W_AMUL);
		see_monsters();
		return;
	case AMULET_OF_LIFE_SAVING:
	case AMULET_VERSUS_POISON:
	case AMULET_OF_REFLECTION:
	case AMULET_OF_CHANGE:
	case AMULET_OF_UNCHANGING:
	case FAKE_AMULET_OF_YENDOR:
		break;
	case AMULET_OF_MAGICAL_BREATHING:
		if (Underwater) {
		    /* HMagical_breathing must be set off
			before calling drown() */
		    setworn((struct obj *)0, W_AMUL);
		    if (!breathless(youmonst.data) && !amphibious(youmonst.data)
						&& !Swimming) {
			You(E_J("suddenly inhale an unhealthy amount of water!",
				"�ˑR�A���N��ӂ��킵���Ȃ��ʂ̐����z�����ނ��ƂɂȂ����I"));
		    	(void) drown();
		    }
		    return;
		}
		if (drownbymon()) {
		    setworn((struct obj *)0, W_AMUL);
		    if (!Amphibious && !Breathless) {
			You_cant(E_J("breathe in water without the amulet!",
				     "���̖������Ȃ��ł͐����Ōċz�ł��Ȃ��I"));
			start_suffocation(SUFFO_WATER);
		    }
		    return;
		}
		break;
	case AMULET_OF_STRANGULATION:
		if (StrangledBy & SUFFO_NECK)
			end_suffocation(SUFFO_NECK);
		break;
	case AMULET_OF_RESTFUL_SLEEP:
		setworn((struct obj *)0, W_AMUL);
		if (!ESleeping)
			HSleeping = 0;
		return;
	case AMULET_OF_YENDOR:
		break;
    }
    setworn((struct obj *)0, W_AMUL);
    return;
}

void
Ring_on(obj)
register struct obj *obj;
{
    long oldprop = u.uprops[objects[obj->otyp].oc_oprop].extrinsic;
    int old_attrib, which;

    if (obj == uwep) setuwep((struct obj *) 0);
    if (obj == uswapwep) setuswapwep((struct obj *) 0);
    if (obj == uquiver) setuqwep((struct obj *) 0);

    /* only mask out W_RING when we don't have both
       left and right rings of the same type */
    if ((oldprop & W_RING) != W_RING) oldprop &= ~W_RING;

    switch(obj->otyp){
	case RIN_TELEPORTATION:
	case RIN_REGENERATION:
	case RIN_SEARCHING:
	case RIN_STEALTH:
	case RIN_HUNGER:
	case RIN_AGGRAVATE_MONSTER:
	case RIN_POISON_RESISTANCE:
	case RIN_FIRE_RESISTANCE:
	case RIN_COLD_RESISTANCE:
	case RIN_SHOCK_RESISTANCE:
	case RIN_CONFLICT:
	case RIN_TELEPORT_CONTROL:
	case RIN_POLYMORPH:
	case RIN_POLYMORPH_CONTROL:
	case RIN_FREE_ACTION:                
	case RIN_SLOW_DIGESTION:
	case RIN_SUSTAIN_ABILITY:
	case MEAT_RING:
		break;
	case RIN_WARNING:
		see_monsters();
		break;
	case RIN_SEE_INVISIBLE:
		/* can now see invisible monsters */
		if (see_invis_on(oldprop))
		    makeknown(RIN_SEE_INVISIBLE);
//		set_mimic_blocking(); /* do special mimic handling */
//		see_monsters();
//#ifdef INVISIBLE_OBJECTS
//		see_objects();
//#endif
//
//		if (Invis && !oldprop && !HSee_invisible &&
//				!perceives(youmonst.data) && !Blind) {
//		    newsym(u.ux,u.uy);
//		    pline(E_J("Suddenly you are transparent, but there!",
//			      "�ˑR�A���Ȃ��͎����̓����Ȏp��������悤�ɂȂ����I"));
//		    makeknown(RIN_SEE_INVISIBLE);
//		}
		break;
	case RIN_INVISIBILITY:
		if (!oldprop && !HInvis && !BInvis && !Blind) {
		    makeknown(RIN_INVISIBILITY);
		    newsym(u.ux,u.uy);
		    self_invis_message();
		}
		break;
	case RIN_LEVITATION:
		if (!oldprop && !HLevitation) {
		    float_up();
		    makeknown(RIN_LEVITATION);
		    spoteffects(FALSE);	/* for sinks */
		}
		break;
	case RIN_GAIN_STRENGTH:
		which = A_STR;
		goto adjust_attrib;
	case RIN_GAIN_CONSTITUTION:
		which = A_CON;
		goto adjust_attrib;
	case RIN_ADORNMENT:
		which = A_CHA;
 adjust_attrib:
		old_attrib = ACURR(which);
		ABON(which) += obj->spe;
		if (ACURR(which) != old_attrib ||
			(objects[obj->otyp].oc_name_known &&
			    old_attrib != 25 && old_attrib != 3)) {
		    flags.botl = 1;
		    makeknown(obj->otyp);
		    obj->known = 1;
		    update_inventory();
		}
		if (which == A_CON) recalchpmax();
		break;
	case RIN_INCREASE_ACCURACY:	/* KMH */
		u.uhitinc += obj->spe;
		break;
	case RIN_INCREASE_DAMAGE:
		u.udaminc += obj->spe;
		break;
	case RIN_PROTECTION_FROM_SHAPE_CHAN:
		if (rescham())
		    makeknown(RIN_PROTECTION_FROM_SHAPE_CHAN);
		break;
	case RIN_PROTECTION:
		if (obj->spe || objects[RIN_PROTECTION].oc_name_known) {
		    u.uprotection += obj->spe;
		    flags.botl = 1;
		    makeknown(RIN_PROTECTION);
		    obj->known = 1;
		    update_inventory();
		}
		break;
    }
}

STATIC_OVL void
Ring_off_or_gone(obj,gone)
register struct obj *obj;
boolean gone;
{
    long mask = (obj->owornmask & W_RING);
    int old_attrib, which;

    takeoff_mask &= ~mask;
    if(!(u.uprops[objects[obj->otyp].oc_oprop].extrinsic & mask))
	impossible("Strange... I didn't know you had that ring.");
    if(gone) setnotworn(obj);
    else setworn((struct obj *)0, obj->owornmask);

    switch(obj->otyp) {
	case RIN_TELEPORTATION:
	case RIN_REGENERATION:
	case RIN_SEARCHING:
	case RIN_STEALTH:
	case RIN_HUNGER:
	case RIN_AGGRAVATE_MONSTER:
	case RIN_POISON_RESISTANCE:
	case RIN_FIRE_RESISTANCE:
	case RIN_COLD_RESISTANCE:
	case RIN_SHOCK_RESISTANCE:
	case RIN_CONFLICT:
	case RIN_TELEPORT_CONTROL:
	case RIN_POLYMORPH:
	case RIN_POLYMORPH_CONTROL:
	case RIN_FREE_ACTION:
	case RIN_SLOW_DIGESTION:
	case RIN_SUSTAIN_ABILITY:
	case MEAT_RING:
		break;
	case RIN_WARNING:
		see_monsters();
		break;
	case RIN_SEE_INVISIBLE:
		/* Make invisible monsters go away */
		if (see_invis_off())
		    makeknown(RIN_SEE_INVISIBLE);
//		if (!See_invisible) {
//		    set_mimic_blocking(); /* do special mimic handling */
//		    see_monsters();
//#ifdef INVISIBLE_OBJECTS
//		    see_objects();
//#endif
//		}
//
//		if (Invisible && !Blind) {
//		    newsym(u.ux,u.uy);
//		    pline(E_J("Suddenly you cannot see yourself.",
//			      "�ˑR�A���Ȃ��͎����������Ȃ��Ȃ����B"));
//		    makeknown(RIN_SEE_INVISIBLE);
//		}
		break;
	case RIN_INVISIBILITY:
		if (!Invis && !BInvis && !Blind) {
		    newsym(u.ux,u.uy);
		    Your(E_J("body seems to unfade%s.",
			     "�g�͓̂�����������%s�B"),
			 See_invisible ? E_J(" completely","��") : E_J("..","�Ă䂭�c"));
		    makeknown(RIN_INVISIBILITY);
		}
		break;
	case RIN_LEVITATION:
		(void) float_down(0L, 0L);
		if (!Levitation) makeknown(RIN_LEVITATION);
		break;
	case RIN_GAIN_STRENGTH:
		which = A_STR;
		goto adjust_attrib;
	case RIN_GAIN_CONSTITUTION:
		which = A_CON;
		goto adjust_attrib;
	case RIN_ADORNMENT:
		which = A_CHA;
 adjust_attrib:
		old_attrib = ACURR(which);
		ABON(which) -= obj->spe;
		if (ACURR(which) != old_attrib) {
		    flags.botl = 1;
		    makeknown(obj->otyp);
		    obj->known = 1;
		    update_inventory();
		}
		if (which == A_CON) recalchpmax();
		break;
	case RIN_INCREASE_ACCURACY:	/* KMH */
		u.uhitinc -= obj->spe;
		break;
	case RIN_INCREASE_DAMAGE:
		u.udaminc -= obj->spe;
		break;
	case RIN_PROTECTION:
		/* might have forgotten it due to amnesia */
		if (obj->spe) {
		    u.uprotection -= obj->spe;
		    flags.botl = 1;
		    makeknown(RIN_PROTECTION);
		    obj->known = 1;
		    update_inventory();
		}
	case RIN_PROTECTION_FROM_SHAPE_CHAN:
		/* If you're no longer protected, let the chameleons
		 * change shape again -dgk
		 */
		if (restartcham())
		    makeknown(RIN_PROTECTION_FROM_SHAPE_CHAN);
		break;
    }
}

void
Ring_off(obj)
struct obj *obj;
{
	Ring_off_or_gone(obj,FALSE);
}

void
Ring_gone(obj)
struct obj *obj;
{
	Ring_off_or_gone(obj,TRUE);
}

void
Blindf_on(otmp)
register struct obj *otmp;
{
	long oldprop = u.uprops[objects[otmp->otyp].oc_oprop].extrinsic;
	boolean already_blind = Blind, changed = FALSE;

	if (otmp == uwep)
	    setuwep((struct obj *) 0);
	if (otmp == uswapwep)
	    setuswapwep((struct obj *) 0);
	if (otmp == uquiver)
	    setuqwep((struct obj *) 0);
	setworn(otmp, W_TOOL);
	on_msg(otmp);

	if (Blind && !already_blind) {
	    changed = TRUE;
	    if (flags.verbose) You_cant(E_J("see any more.","���������Ȃ��Ȃ����B"));
	    /* set ball&chain variables before the hero goes blind */
	    if (Punished) set_bc(0);
	} else if (already_blind && !Blind) {
	    changed = TRUE;
	    /* "You are now wearing the Eyes of the Overworld." */
	    You(E_J("can see!","�ڂ�������悤�ɂȂ����I"));
	}
#ifdef MAGIC_GLASSES
	  else if (Is_glasses(otmp) && objects[otmp->otyp].oc_oprop)
	    changed = TRUE;
	if (otmp->otyp == GLASSES_OF_SEE_INVISIBLE) {
	    if (see_invis_on(oldprop))
		makeknown(GLASSES_OF_SEE_INVISIBLE);
	    return;
	}
#endif
	if (changed) {
	    /* blindness has just been toggled */
	    if (Blind_telepat || Infravision) see_monsters();
	    vision_full_recalc = 1;	/* recalc vision limits */
	    flags.botl = 1;
	}
}

void
Blindf_off(otmp)
register struct obj *otmp;
{
	boolean was_blind = Blind, changed = FALSE;

	takeoff_mask &= ~W_TOOL;
	setworn((struct obj *)0, otmp->owornmask);
	off_msg(otmp);

	if (Blind) {
	    if (was_blind) {
		/* "still cannot see" makes no sense when removing lenses
		   since they can't have been the cause of your blindness */
#ifndef MAGIC_GLASSES
		if (otmp->otyp != LENSES)
#else
		if (Is_glasses(otmp))
#endif /*MAGIC_GLASSES*/
		    You(E_J("still cannot see.","�܂����邱�Ƃ��ł��Ȃ��B"));
	    } else {
		changed = TRUE;	/* !was_blind */
		/* "You were wearing the Eyes of the Overworld." */
		You_cant(E_J("see anything now!","���������Ȃ��Ȃ����I"));
		/* set ball&chain variables before the hero goes blind */
		if (Punished) set_bc(0);
	    }
	} else if (was_blind) {
	    changed = TRUE;	/* !Blind */
	    You(E_J("can see again.","�Ăіڂ�������悤�ɂȂ����B"));
	}
#ifdef MAGIC_GLASSES
	  else if (Is_glasses(otmp) && objects[otmp->otyp].oc_oprop) {
	    if (objects[otmp->otyp].oc_oprop == HALLUC) {
		make_hallucinated(0L, TRUE, 0L);
	    } else if (otmp->otyp == GLASSES_OF_SEE_INVISIBLE) {
		if (see_invis_off())
		    makeknown(GLASSES_OF_SEE_INVISIBLE);
		return;
	    } else
		see_monsters();
	}
#endif
	if (changed) {
	    /* blindness has just been toggled */
	    if (Blind_telepat || Infravision) see_monsters();
	    vision_full_recalc = 1;	/* recalc vision limits */
	    flags.botl = 1;
	}
}

STATIC_OVL int
see_invis_on(oldprop)
long oldprop;
{
	/* can now see invisible monsters */
	set_mimic_blocking(); /* do special mimic handling */
	see_monsters();
#ifdef INVISIBLE_OBJECTS
	see_objects();
#endif

	if (Invis && !oldprop && !HSee_invisible &&
			!perceives(youmonst.data) && !Blind) {
	    newsym(u.ux,u.uy);
	    pline(E_J("Suddenly you are transparent, but there!",
		      "�ˑR�A���Ȃ��͎����̓����Ȏp��������悤�ɂȂ����I"));
	    return TRUE;
	}
	return FALSE;
}

STATIC_OVL int
see_invis_off()
{
	/* Make invisible monsters go away */
	if (!See_invisible) {
	    set_mimic_blocking(); /* do special mimic handling */
	    see_monsters();
#ifdef INVISIBLE_OBJECTS
	    see_objects();
#endif
	}

	if (Invisible && !Blind) {
	    newsym(u.ux,u.uy);
	    pline(E_J("Suddenly you cannot see yourself.",
		      "�ˑR�A���Ȃ��͎����������Ȃ��Ȃ����B"));
	    return TRUE;
	}
	return FALSE;
}

/*
 *  Some armor/clothes are only for ladies
 */
void refresh_ladies_wear(ormv)
struct obj *ormv;
{
	const short ladies_wear[] = {
	    MAID_DRESS, NURSE_UNIFORM, FRILLED_APRON, KITCHEN_APRON,
	    KATYUSHA, NURSE_CAP, 0
	};
	const short *p = ladies_wear;
	int ladies_wear_prop[4];
	int i;
	int otyp, uwtyp;
	int lpnum;		/* num of property conveyed for ladies */
	int lprop;
	int mensprop;		/* property conveyed for men */
	int lpspe;
	long mask, oldprop;
	int rtyp = (ormv) ? ormv->otyp : 0;
	boolean lpon, mpon, isuw, nouw;
	struct obj *otmp;	/* armor to be checked */
	struct obj *ouw;	/* an underwear to be checked */
	while (otyp = *p++) {
		lpnum = 0;
		uwtyp = 0;
		mensprop = AGGRAVATE_MONSTER;	/* default */
		switch (otyp) {
		    case NURSE_UNIFORM:
			ladies_wear_prop[lpnum++] = SICK_RES;
			break;
		    case FRILLED_APRON:
			uwtyp = MAID_DRESS;
			ladies_wear_prop[lpnum++] = -ANTIMAGIC;
//			ladies_wear_prop[lpnum++] = -PROTECTION;
//			lpspe = 4;
			break;
		    case KITCHEN_APRON:
			uwtyp = MAID_DRESS;
//			ladies_wear_prop[lpnum++] = -PROTECTION;
//			lpspe = 3;
			mensprop = 0;	/* no penalty is conveyed for men by kitchen apron */
			break;
		    case KATYUSHA:
			uwtyp = MAID_DRESS;
			ladies_wear_prop[lpnum++] = ADORNED;
//			ladies_wear_prop[lpnum++] = -PROTECTION;
//			lpspe = 2;
			break;
		    case NURSE_CAP:
			uwtyp = NURSE_UNIFORM;
			ladies_wear_prop[lpnum++] = HALLUC_RES;
			ladies_wear_prop[lpnum++] = -DRAIN_RES;
			break;
		    case MAID_DRESS:
		    default:
			break;
		}
		if (rtyp == otyp) {
			/* taking off it */
			lpon = FALSE;
			mpon = FALSE;
		} else if ((otmp = *(otyp2uarmp(otyp))) && otmp->otyp == otyp) {
			/* wearing it */
			lpon = flags.female;
			mpon = !flags.female;
		} else continue; /* it's not the case... skip it */

		mask = otyp2mask(otyp);
		/* check if underwear is needed to get properties */
		if (uwtyp) {
			ouw = *(otyp2uarmp(uwtyp));
			/* wearing an appropriate underwear? */
			isuw = (ouw && ouw->otyp == uwtyp && ouw->otyp != rtyp);
		}
		/* set/reset property for ladies */
		for (i=0; i<lpnum; i++) {
			nouw = FALSE;
			lprop = ladies_wear_prop[i];
			if (lprop < 0) { /* need to check underwear */
				lprop = -lprop;
				nouw = !isuw;
			}
			oldprop = u.uprops[lprop].extrinsic & mask;
			if (lpon && !nouw) {
				u.uprops[lprop].extrinsic |= mask;
				if (!oldprop && lprop == PROTECTION)
					u.uprotection += lpspe;
			} else {
				u.uprops[lprop].extrinsic &= ~mask;
				if (oldprop && lprop == PROTECTION)
					u.uprotection -= lpspe;
			}
		}
		/* set/reset property for men */
		if (mensprop) {
			if (mpon) u.uprops[mensprop].extrinsic |= mask;
			else	  u.uprops[mensprop].extrinsic &= ~mask;
		}

		/* special handling... */
		if (otyp == KATYUSHA) set_moreluck(); /* in case of sex-changing */
	}
}

/* called in main to set intrinsics of worn start-up items */
void
set_wear()
{
#ifdef TOURIST
	if (uarmu) (void) Shirt_on();
#endif
	if (uarm)  (void) Armor_on();
	if (uarmc) (void) Cloak_on();
	if (uarmf) (void) Boots_on();
	if (uarmg) (void) Gloves_on();
	if (uarmh) (void) Helmet_on();
	if (uarms) (void) Shield_on();
}

/* check whether the target object is currently being put on (or taken off) */
boolean
donning(otmp)		/* also checks for doffing */
register struct obj *otmp;
{
 /* long what = (occupation == take_off) ? taking_off : 0L; */
    long what = taking_off;	/* if nonzero, occupation is implied */
    boolean result = FALSE;

    if (otmp == uarm)
	result = (afternmv == Armor_on || afternmv == Armor_off ||
		  what == WORN_ARMOR);
#ifdef TOURIST
    else if (otmp == uarmu)
	result = (afternmv == Shirt_on || afternmv == Shirt_off ||
		  what == WORN_SHIRT);
#endif
    else if (otmp == uarmc)
	result = (afternmv == Cloak_on || afternmv == Cloak_off ||
		  what == WORN_CLOAK);
    else if (otmp == uarmf)
	result = (afternmv == Boots_on || afternmv == Boots_off ||
		  what == WORN_BOOTS);
    else if (otmp == uarmh)
	result = (afternmv == Helmet_on || afternmv == Helmet_off ||
		  what == WORN_HELMET);
    else if (otmp == uarmg)
	result = (afternmv == Gloves_on || afternmv == Gloves_off ||
		  what == WORN_GLOVES);
    else if (otmp == uarms)
	result = (afternmv == Shield_on || afternmv == Shield_off ||
		  what == WORN_SHIELD);

    return result;
}

void
cancel_don()
{
	/* the piece of armor we were donning/doffing has vanished, so stop
	 * wasting time on it (and don't dereference it when donning would
	 * otherwise finish)
	 */
	cancelled_don = (afternmv == Boots_on || afternmv == Helmet_on ||
			 afternmv == Gloves_on || afternmv == Armor_on);
	afternmv = 0;
	nomovemsg = (char *)0;
	multi = 0;
	todelay = 0;
	taking_off = 0L;
}

static NEARDATA const char clothes[] = {ARMOR_CLASS, 0};
static NEARDATA const char accessories[] = {RING_CLASS, AMULET_CLASS, TOOL_CLASS, FOOD_CLASS, 0};

/* the 'T' command */
int
getobj_filter_takeoff(otmp)
struct obj *otmp;
{
	if (otmp->oclass != ARMOR_CLASS) return 0;
	if (!(otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL))
	     || (otmp==uarm && uarmc)
#ifdef TOURIST
	     || (otmp==uarmu && (uarm || uarmc))
#endif
	    ) return GETOBJ_ADDELSE;
	return GETOBJ_CHOOSEIT;
}

int
getobj_filter_remove(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	int ret = 0;
	switch (otmp->oclass) {
	    case TOOL_CLASS:
		if (otyp == BLINDFOLD || otyp == TOWEL ||
#ifndef MAGIC_GLASSES
		    otyp == LENSES
#else
		    Is_glasses(otmp)
#endif /*MAGIC_GLASSES*/
		   ) goto common;
		break;
	    case FOOD_CLASS:
		if (otyp != MEAT_RING) goto common;
		break;
	    case RING_CLASS:
	    case AMULET_CLASS:
common:
		ret = !(otmp->owornmask & (W_RING | W_AMUL | W_TOOL)) ?
			GETOBJ_ADDELSE : GETOBJ_CHOOSEIT;
		break;
	    default:
		break;
	}
	return ret;
}

int
getobj_filter_wear(otmp)
struct obj *otmp;
{
	long dummymask;

	if (otmp->oclass != ARMOR_CLASS) return 0;
	/* check for unworn armor that can't be worn */
	if (!canwearobj(otmp, &dummymask, FALSE))
		return (GETOBJ_ALTSET | GETOBJ_ALLOWALL);
	/* already worn? */
	if (otmp->owornmask & W_ARMOR)
		return GETOBJ_ADDELSE;
	return GETOBJ_CHOOSEIT;
}

int
getobj_filter_puton(otmp)
struct obj *otmp;
{
	int otyp = otmp->otyp;
	int ret = 0;
	switch (otmp->oclass) {
	    case TOOL_CLASS:
		if (otyp == BLINDFOLD || otyp == TOWEL ||
#ifndef MAGIC_GLASSES
		    otyp == LENSES
#else
		    Is_glasses(otmp)
#endif /*MAGIC_GLASSES*/
		   ) goto common;
		break;
	    case FOOD_CLASS:
		if (otyp == MEAT_RING) goto common;
		break;
	    case RING_CLASS:
	    case AMULET_CLASS:
common:
		ret = (otmp->owornmask & (W_RING | W_AMUL | W_TOOL)) ?	/* already worn */
			GETOBJ_ADDELSE : GETOBJ_CHOOSEIT;
		break;
	    default:
		break;
	}
	return ret;
}

#ifdef JP
	static const struct getobj_words takeoff_words = { 0, 0, "�E��", "�E��" };
#endif /*JP*/
int
dotakeoff()
{
	register struct obj *otmp = (struct obj *)0;
	int armorpieces = 0;

#define MOREARM(x) if (x) { armorpieces++; otmp = x; }
	MOREARM(uarmh);
	MOREARM(uarms);
	MOREARM(uarmg);
	MOREARM(uarmf);
	if (uarmc) {
		armorpieces++;
		otmp = uarmc;
	} else if (uarm) {
		armorpieces++;
		otmp = uarm;
#ifdef TOURIST
	} else if (uarmu) {
		armorpieces++;
		otmp = uarmu;
#endif
	}
	if (!armorpieces) {
	     /* assert( GRAY_DRAGON_SCALES > YELLOW_DRAGON_SCALE_MAIL ); */
		if (uskin)
		    pline_The(E_J("%s merged with your skin!",
				  "%s�͂��Ȃ��̔��Ɠ������Ă���I"),
			      uskin->otyp >= GRAY_DRAGON_SCALES ?
				E_J("dragon scales are",   "�h���S���̗�") :
				E_J("dragon scale mail is","�h���S���̗؊Z"));
		else
		    pline(E_J("Not wearing any armor.%s",
			      "���Ȃ��͖h��𒅂��Ă��Ȃ��B%s"), (iflags.cmdassist && 
				(uleft || uright || uamul || ublindf)) ?
			  E_J("  Use 'R' command to remove accessories.",
			      "���g����O���ɂ́A'R'�R�}���h���g�p���܂��B") : "");
		return 0;
	}
	if (armorpieces > 1)
		otmp = getobj(clothes, E_J("take off",&takeoff_words), getobj_filter_takeoff);
	if (otmp == 0) return(0);
	if (!(otmp->owornmask & W_ARMOR)) {
		You(E_J("are not wearing that.","����𒅂Ă��Ȃ��B"));
		return(0);
	}
	/* note: the `uskin' case shouldn't be able to happen here; dragons
	   can't wear any armor so will end up with `armorpieces == 0' above */
	if (otmp == uskin || ((otmp == uarm) && uarmc)
#ifdef TOURIST
			  || ((otmp == uarmu) && (uarmc || uarm))
#endif
		) {
	    You_cant(E_J("take that off.","�܂����̏�̂��̂�E���Ȃ���΁B"));
	    return 0;
	}

	reset_remarm();		/* clear takeoff_mask and taking_off */
	(void) select_off(otmp);
	if (!takeoff_mask) return 0;
	reset_remarm();		/* armoroff() doesn't use takeoff_mask */

	(void) armoroff(otmp);
	return(1);
}

/* the 'R' command */
int
doremring()
{
	register struct obj *otmp = 0;
	int Accessories = 0;
#ifdef JP
	static const struct getobj_words rmw = { 0, 0, "�O��", "�O��" };
#endif /*JP*/

#define MOREACC(x) if (x) { Accessories++; otmp = x; }
	MOREACC(uleft);
	MOREACC(uright);
	MOREACC(uamul);
	MOREACC(ublindf);

	if(!Accessories) {
		pline(E_J("Not wearing any accessories.%s",
			  "���Ȃ��͑��g���g�ɂ��Ă��Ȃ��B%s"), (iflags.cmdassist &&
			    (uarm || uarmc ||
#ifdef TOURIST
			     uarmu ||
#endif
			     uarms || uarmh || uarmg || uarmf)) ?
		      E_J("  Use 'T' command to take off armor.",
			  "�h����O���ɂ́A'T'�R�}���h���g�p���܂��B") : "");
		return(0);
	}
	if (Accessories != 1) otmp = getobj(accessories, E_J("remove",&rmw), getobj_filter_remove);
	if(!otmp) return(0);
	if(!(otmp->owornmask & (W_RING | W_AMUL | W_TOOL))) {
		You(E_J("are not wearing that.","�����g�ɂ��Ă��Ȃ��B"));
		return(0);
	}

	reset_remarm();		/* clear takeoff_mask and taking_off */
	(void) select_off(otmp);
	if (!takeoff_mask) return 0;
	reset_remarm();		/* not used by Ring_/Amulet_/Blindf_off() */

	if (otmp == uright || otmp == uleft) {
		/* Sometimes we want to give the off_msg before removing and
		 * sometimes after; for instance, "you were wearing a moonstone
		 * ring (on right hand)" is desired but "you were wearing a
		 * square amulet (being worn)" is not because of the redundant
		 * "being worn".
		 */
		off_msg(otmp);
		Ring_off(otmp);
	} else if (otmp == uamul) {
		Amulet_off();
		off_msg(otmp);
	} else if (otmp == ublindf) {
		Blindf_off(otmp);	/* does its own off_msg */
	} else {
		impossible("removing strange accessory?");
	}
	return(1);
}

/* Check if something worn is cursed _and_ unremovable. */
int
cursed(otmp)
register struct obj *otmp;
{
	/* Curses, like chickens, come home to roost. */
	if((otmp == uwep) ? welded(otmp) : (int)otmp->cursed) {
#ifndef JP
		You("can't.  %s cursed.",
			(is_boots(otmp) || is_gloves(otmp) || otmp->quan > 1L)
			? "They are" : "It is");
#else
		pline("�ʖڂ��B����͎���Ă���B");
#endif /*JP*/
		otmp->bknown = TRUE;
		return(1);
	}
	return(0);
}

int
armoroff(otmp)
register struct obj *otmp;
{
	register int delay = -objects[otmp->otyp].oc_delay;
	int NDECL((*offfunc));
	char *offmsg;

	if(cursed(otmp)) return(0);
	get_armor_on_off_info(otmp, FALSE, &offfunc, &offmsg);
	if(delay && offfunc && offmsg) {
		nomul(delay);
		nomovemsg = offmsg;
		afternmv = offfunc;
	} else {
		/* Be warned!  We want off_msg after removing the item to
		 * avoid "You were wearing ____ (being worn)."  However, an
		 * item which grants fire resistance might cause some trouble
		 * if removed in Hell and lifesaving puts it back on; in this
		 * case the message will be printed at the wrong time (after
		 * the messages saying you died and were lifesaved).  Luckily,
		 * no cloak, shield, or fast-removable armor grants fire
		 * resistance, so we can safely do the off_msg afterwards.
		 * Rings do grant fire resistance, but for rings we want the
		 * off_msg before removal anyway so there's no problem.  Take
		 * care in adding armors granting fire resistance; this code
		 * might need modification.
		 * 3.2 (actually 3.1 even): this comment is obsolete since
		 * fire resistance is not needed for Gehennom.
		 */
		if (offfunc) (void) offfunc();
		else setworn((struct obj *)0, otmp->owornmask & W_ARMOR);
		off_msg(otmp);
	}
	takeoff_mask = taking_off = 0L;
	return(1);
}

STATIC_OVL void
already_wearing(cc)
const char *cc;
{
#ifndef JP
	You("are already wearing %s%c", cc, (cc == c_that_) ? '!' : '.');
#else
	You("���ł�%s��g�ɂ��Ă���%s", cc, (cc == c_that_) ? "�I" : "�B");
#endif /*JP*/
}

STATIC_OVL void
already_wearing2(cc1, cc2)
const char *cc1, *cc2;
{
#ifndef JP
	You_cant("wear %s because you're wearing %s there already.", cc1, cc2);
#else
	You("���ł�%s��g�ɂ��Ă��邽�߁A%s��g�ɂ����Ȃ��B", cc2, cc1);
#endif /*JP*/
}

/*
 * canwearobj checks to see whether the player can wear a piece of armor
 *
 * inputs: otmp (the piece of armor)
 *         noisy (if TRUE give error messages, otherwise be quiet about it)
 * output: mask (otmp's armor type)
 */
int
canwearobj(otmp,mask,noisy)
struct obj *otmp;
long *mask;
boolean noisy;
{
    int err = 0;
    const char *which;

    which = is_cloak(otmp) ? c_cloak :
#ifdef TOURIST
	    is_shirt(otmp) ? c_shirt :
#endif
	    is_suit(otmp) ? c_suit : 0;
    if (which && cantweararm(youmonst.data) &&
	    /* same exception for cloaks as used in m_dowear() */
	    (which != c_cloak || youmonst.data->msize != MZ_SMALL) &&
	    (racial_exception(&youmonst, otmp) < 1)) {
	if (noisy) pline_The(E_J("%s will not fit on your body.",
				 "%s�͂��Ȃ��̐g�̂ɍ���Ȃ��B"), which);
	return 0;
    } else if (otmp->owornmask & W_ARMOR) {
	if (noisy) already_wearing(c_that_);
	return 0;
    }

    if (welded(uwep) && bimanual(uwep) &&
	    (is_suit(otmp)
#ifdef TOURIST
			|| is_shirt(otmp)
#endif
	    )) {
	if (noisy)
#ifndef JP
	    You("cannot do that while holding your %s.",
		is_sword(uwep) ? c_sword : c_weapon);
#else
	    pline("%s���\�����܂܂ł́A���邱�Ƃ��ł��Ȃ��B",
		is_sword(uwep) ? c_sword : c_weapon);
#endif /*JP*/
	return 0;
    }

    if (is_helmet(otmp)) {
	if (uarmh) {
	    if (noisy) already_wearing(E_J(an(c_helmet), c_helmet));
	    err++;
	} else if (Upolyd && has_horns(youmonst.data) && !is_flimsy(otmp)) {
	    /* (flimsy exception matches polyself handling) */
	    if (noisy)
#ifndef JP
		pline_The("%s won't fit over your horn%s.",
			  c_helmet, plur(num_horns(youmonst.data)));
#else
		pline("�p���ז��ŁA%s�����Ԃ邱�Ƃ��ł��Ȃ��B", c_helmet);
#endif /*JP*/
	    err++;
	} else
	    *mask = W_ARMH;
    } else if (is_shield(otmp)) {
	if (uarms) {
	    if (noisy) already_wearing(E_J(an(c_shield), c_shield));
	    err++;
	} else if (uwep && bimanual(uwep)) {
	    if (noisy) 
		You(E_J("cannot wear a shield while wielding a two-handed %s.",
			"���莝����%s���\���Ă��邽�߁A���𑕔��ł��Ȃ��B"),
		    is_sword(uwep) ? c_sword :
		    (uwep->otyp == BATTLE_AXE) ? c_axe : c_weapon);
	    err++;
	} else if (u.twoweap) {
	    if (noisy)
		You(E_J("cannot wear a shield while wielding two weapons.",
			"����ɕ�����\���Ă��邽�߁A���𑕔��ł��Ȃ��B"));
	    err++;
	} else
	    *mask = W_ARMS;
    } else if (is_boots(otmp)) {
	if (uarmf) {
	    if (noisy) already_wearing(c_boots);
	    err++;
	} else if (Upolyd && slithy(youmonst.data)) {
#ifndef JP
	    if (noisy) You("have no feet...");	/* not body_part(FOOT) */
#else
	    if (noisy) pline("���Ȃ��ɂ͑����Ȃ��c�B");
#endif /*JP*/
	    err++;
	} else if (Upolyd && youmonst.data->mlet == S_CENTAUR) {
	    /* break_armor() pushes boots off for centaurs,
	       so don't let dowear() put them back on... */
	    if (noisy) pline(E_J("You have too many hooves to wear %s.",
				 "���Ȃ���%s�𗚂��ɂ͒�����������B"),
			     c_boots);	/* makeplural(body_part(FOOT)) yields
					   "rear hooves" which sounds odd */
	    err++;
	} else if (u.utrap && (u.utraptype == TT_BEARTRAP ||
				u.utraptype == TT_INFLOOR)) {
	    if (u.utraptype == TT_BEARTRAP) {
		if (noisy) Your(E_J("%s is trapped!",
				    "%s��㩂ɂ������Ă���I"), body_part(FOOT));
	    } else {
#ifndef JP
		if (noisy) Your("%s are stuck in the %s!",
				makeplural(body_part(FOOT)),
				surface(u.ux, u.uy));
#else
		if (noisy) Your("%s��%s�ɋ��܂�Ă���I",
				body_part(FOOT), surface(u.ux, u.uy));
#endif /*JP*/
	    }
	    err++;
	} else
	    *mask = W_ARMF;
    } else if (is_gloves(otmp)) {
	if (uarmg) {
	    if (noisy) already_wearing(c_gloves);
	    err++;
	} else if (welded(uwep)) {
	    if (noisy) You(E_J("cannot wear gloves over your %s.",
			       "%s���������܂܂ł͘U��������Ȃ��B"),
			   is_sword(uwep) ? c_sword : c_weapon);
	    err++;
	} else
	    *mask = W_ARMG;
#ifdef TOURIST
    } else if (is_shirt(otmp)) {
	if (uarm || uarmc || uarmu) {
	    if (uarmu) {
		if (noisy) already_wearing(E_J(an(c_shirt), c_shirt));
	    } else {
		if (noisy) You_cant(E_J("wear that over your %s.",
					"%s�̏�ɂ���𒅂邱�Ƃ͂ł��Ȃ��B"),
			           (uarm && !uarmc) ? c_armor : cloak_simple_name(uarmc));
	    }
	    err++;
	} else
	    *mask = W_ARMU;
#endif
    } else if (is_cloak(otmp)) {
	if (uarmc) {
	    if (noisy) already_wearing(E_J(an(cloak_simple_name(uarmc)),
					   cloak_simple_name(uarmc)));
	    err++;
	} else
	    *mask = W_ARMC;
    } else if (is_suit(otmp)) {
	if (uarmc) {
#ifndef JP
	    if (noisy) You("cannot wear armor over a %s.", cloak_simple_name(uarmc));
#else
	    if (noisy) You("%s�̏��%s�𒅂邱�Ƃ͂ł��Ȃ��B",
			   cloak_simple_name(uarmc), armor_simple_name(otmp));
#endif /*JP*/
	    err++;
	} else if (uarm) {
	    if (noisy) already_wearing(E_J("some armor","�Z"));
	    err++;
	} else
	    *mask = W_ARM;
    } else {
	/* getobj can't do this after setting its allow_all flag; that
	   happens if you have armor for slots that are covered up or
	   extra armor for slots that are filled */
	if (noisy) silly_thing(E_J("wear","��������"), otmp);
	err++;
    }
/* Unnecessary since now only weapons and special items like pick-axes get
 * welded to your hand, not armor
    if (welded(otmp)) {
	if (!err++) {
	    if (noisy) weldmsg(otmp);
	}
    }
 */
    return !err;
}

/* the 'W' command */
int
dowear()
{
	struct obj *otmp;
	int delay;
	long mask = 0;
	int NDECL((*onfunc));
	char *onmsg;
#ifdef JP
	static const struct getobj_words wearw = { 0, 0, "����", "��" };
#endif /*JP*/

	/* cantweararm checks for suits of armor */
	/* verysmall or nohands checks for shields, gloves, etc... */
	if ((verysmall(youmonst.data) || nohands(youmonst.data))) {
		pline(E_J("Don't even bother.","����ɂ͋y�΂Ȃ��B"));
		return(0);
	}

	otmp = getobj(clothes, E_J("wear",&wearw), getobj_filter_wear);
	if(!otmp) return(0);

	if (!canwearobj(otmp,&mask,TRUE)) return(0);

	if (otmp->oartifact && !touch_artifact(otmp, &youmonst))
	    return 1;	/* costs a turn even though it didn't get worn */

	if (otmp->otyp == HELM_OF_OPPOSITE_ALIGNMENT &&
			qstart_level.dnum == u.uz.dnum) {	/* in quest */
		if (u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL])
			You(E_J("narrowly avoid losing all chance at your goal.",
				"�낤�������̎g����B���ł��Ȃ��Ȃ�Ƃ��낾�����B"));
		else	/* converted */
			You(E_J("are suddenly overcome with shame and change your mind.",
				"�˔@�p���������Ɉ��|����A�S��ς����B"));
		u.ublessed = 0; /* lose your god's protection */
		makeknown(otmp->otyp);
		flags.botl = 1;
		return 1;
	}

	otmp->known = TRUE;
	if(otmp == uwep)
		setuwep((struct obj *)0);
	if (otmp == uswapwep)
		setuswapwep((struct obj *) 0);
	if (otmp == uquiver)
		setuqwep((struct obj *) 0);
	setworn(otmp, mask);
	delay = -objects[otmp->otyp].oc_delay;
	get_armor_on_off_info(otmp, TRUE, &onfunc, &onmsg);
	if(delay){
		nomul(delay);
		if (onfunc) (void) onfunc();
		nomovemsg = onmsg;
	} else {
		if (onfunc) (void) onfunc();
		on_msg(otmp);
	}
	takeoff_mask = taking_off = 0L;
	if(Role_if(PM_MONK) && !Upolyd && uarm && !is_clothes(uarm))
		pline(E_J("This armor is rather cumbersome...",
			  "���̊Z�͂Ђǂ������Â炢�c�B"));
	return(1);
}

void
get_armor_on_off_info(otmp, dowear, funcptr, msgptr)
struct obj *otmp;
boolean dowear;
int (**funcptr)();
char **msgptr;
{
	const struct armor_on_off_info *ainfo;
	ainfo = &armor_on_off_infotbl[objects[otmp->otyp].oc_armcat];
	if (dowear) {
		*funcptr = ainfo->on_func;
		if (msgptr) *msgptr = E_J("You finish your dressing maneuver.",
					  "���Ȃ��͑��������������B");
	} else {
		*funcptr = ainfo->off_func;
		if (msgptr) *msgptr = ainfo->delayedoffmsg;
	}
}

int
doputon()
{
	register struct obj *otmp;
	long mask = 0L;
#ifdef JP
	static const struct getobj_words pow = { 0, 0, "�g�ɂ���", "�g�ɂ�" };
#endif /*JP*/

	if(uleft && uright && uamul && ublindf) {
#ifndef JP
		Your("%s%s are full, and you're already wearing an amulet and %s.",
			humanoid(youmonst.data) ? "ring-" : "",
			makeplural(body_part(FINGER)),
#else
		You("���łɗ�%s%s�Ɏw�ւ��͂߂Ă���A��������%s�������ς݂��B",
			humanoid(youmonst.data) ? "��" : "", body_part(FINGER),
#endif /*JP*/
#ifndef MAGIC_GLASSES
			ublindf->otyp==LENSES ? E_J("some lenses","�����Y") : E_J("a blindfold","�ډB��"));
#else
			Is_glasses(ublindf) ? E_J("glasses","�ዾ") : E_J("a blindfold","�ډB��"));
#endif /*MAGIC_GLASSES*/
		return(0);
	}
	otmp = getobj(accessories, E_J("put on",&pow), getobj_filter_puton);
	if(!otmp) return(0);
	if(otmp->owornmask & (W_RING | W_AMUL | W_TOOL)) {
		already_wearing(c_that_);
		return(0);
	}
	if(welded(otmp)) {
		weldmsg(otmp);
		return(0);
	}
	if(otmp == uwep)
		setuwep((struct obj *)0);
	if(otmp == uswapwep)
		setuswapwep((struct obj *) 0);
	if(otmp == uquiver)
		setuqwep((struct obj *) 0);
	if(otmp->oclass == RING_CLASS || otmp->otyp == MEAT_RING) {
		if(nolimbs(youmonst.data)) {
			You(E_J("cannot make the ring stick to your body.",
				"�w�ւ�g�̂ɌŒ肷�邱�Ƃ��ł��Ȃ��B"));
			return(0);
		}
		if(uleft && uright){
#ifndef JP
			There("are no more %s%s to fill.",
				humanoid(youmonst.data) ? "ring-" : "",
				makeplural(body_part(FINGER)));
#else
			You("���łɗ�%s�Ɏw�ւ��͂߂Ă���B", body_part(HAND));
#endif /*JP*/
			return(0);
		}
		if(uleft) mask = RIGHT_RING;
		else if(uright) mask = LEFT_RING;
		else do {
			char qbuf[QBUFSZ];
			char answer;

#ifndef JP
			Sprintf(qbuf, "Which %s%s, Right or Left?",
				humanoid(youmonst.data) ? "ring-" : "",
				body_part(FINGER));
#else
			Sprintf(qbuf, "�E(r)�ƍ�(l)�A�ǂ����%s�ɁH",
				body_part(HAND));
#endif /*JP*/
			if(!(answer = yn_function(qbuf, "rl", '\0')))
				return(0);
			switch(answer){
			case 'l':
			case 'L':
				mask = LEFT_RING;
				break;
			case 'r':
			case 'R':
				mask = RIGHT_RING;
				break;
			}
		} while(!mask);
		if (uarmg && uarmg->cursed) {
			uarmg->bknown = TRUE;
		    You(E_J("cannot remove your gloves to put on the ring.",
			    "�U����O���Ȃ����߁A�w�ւ��͂߂��Ȃ��B"));
			return(0);
		}
		if (welded(uwep) && bimanual(uwep)) {
			/* welded will set bknown */
	    You(E_J("cannot free your weapon hands to put on the ring.",
		    "�����������Ȃ����߁A�w�ւ��͂߂��Ȃ��B"));
			return(0);
		}
		if (welded(uwep) && mask==RIGHT_RING) {
			/* welded will set bknown */
	    You(E_J("cannot free your weapon hand to put on the ring.",
		    "�����������Ȃ����߁A�w�ւ��͂߂��Ȃ��B"));
			return(0);
		}
		if (otmp->oartifact && !touch_artifact(otmp, &youmonst))
		    return 1; /* costs a turn even though it didn't get worn */
		setworn(otmp, mask);
		Ring_on(otmp);
	} else if (otmp->oclass == AMULET_CLASS) {
		if(uamul) {
			already_wearing(E_J("an amulet","������"));
			return(0);
		}
		if (otmp->oartifact && !touch_artifact(otmp, &youmonst))
		    return 1;
		setworn(otmp, W_AMUL);
		if (otmp->otyp == AMULET_OF_CHANGE) {
			Amulet_on();
			/* Don't do a prinv() since the amulet is now gone */
			return(1);
		}
		Amulet_on();
	} else {	/* it's a blindfold, towel, or lenses */
		if (ublindf) {
			if (ublindf->otyp == TOWEL)
				Your(E_J("%s is already covered by a towel.",
					 "%s�͂��łɃ^�I���ŕ����Ă���B"),
					body_part(FACE));
			else if (ublindf->otyp == BLINDFOLD) {
#ifndef MAGIC_GLASSES
				if (otmp->otyp == LENSES)
					already_wearing2(E_J("lenses","�����Y"), E_J("a blindfold","�ډB��"));
#else
				if (Is_glasses(otmp))
					already_wearing2(E_J("glasses","�ዾ"), E_J("a blindfold","�ډB��"));
#endif /*MAGIC_GLASSES*/
				else
					already_wearing(E_J("a blindfold","�ډB��"));
#ifndef MAGIC_GLASSES
			} else if (ublindf->otyp == LENSES) {
				if (otmp->otyp == BLINDFOLD)
					already_wearing2(E_J("a blindfold","�ډB��"), E_J("some lenses","�����Y"));
				else
					already_wearing(E_J("some lenses","�����Y"));
#else
			} else if (Is_glasses(ublindf)) {
				if (otmp->otyp == BLINDFOLD)
					already_wearing2(E_J("a blindfold","�ډB��"), E_J("glasses","�ዾ"));
				else
					already_wearing(E_J("glasses","�ዾ"));
#endif /*MAGIC_GLASSES*/
			} else
				already_wearing(something); /* ??? */
			return(0);
		}
#ifndef MAGIC_GLASSES
		if (otmp->otyp != BLINDFOLD && otmp->otyp != TOWEL && otmp->otyp != LENSES) {
#else
		if (otmp->otyp != BLINDFOLD && otmp->otyp != TOWEL && !Is_glasses(otmp)) {
#endif /*MAGIC_GLASSES*/
			You_cant(E_J("wear that!","�����g�ɂ��邱�Ƃ͂ł��Ȃ��I"));
			return(0);
		}
		if (otmp->oartifact && !touch_artifact(otmp, &youmonst))
		    return 1;
		Blindf_on(otmp);
		return(1);
	}
	if (is_worn(otmp))
	    prinv((char *)0, otmp, 0L);
	return(1);
}

#endif /* OVLB */

#ifdef OVL0

const schar ac_dex_bonus[23] = {
	 2,  2,  1,  1,		/*  3- 6 */
	 1,  0,  0,  0,		/*  7-10 */
	 0,  0,  0,  0,		/* 11-14 */
	 0, -1, -1,		/* 15-17 */
	-2, -2, -2, -3,		/* 18-21 */
	-3, -4, -4, -5		/* 21-25 */
};

void
find_ac()
{
	int uac = mons[u.umonnum].ac;

	if ( ACURR(A_DEX)>=3 && ACURR(A_DEX)<=25 ) {
		uac += ac_dex_bonus[ACURR(A_DEX)-3];
	}
  
	if(uarm)  uac -= ARM_BONUS(uarm);
	if(uarmc) uac -= ARM_BONUS(uarmc);
	if(uarmh) uac -= ARM_BONUS(uarmh);
	if(uarmf) uac -= ARM_BONUS(uarmf);
	if(uarms) uac -= ARM_BONUS(uarms);
	if(uarmg) uac -= ARM_BONUS(uarmg);
#ifdef TOURIST
	if(uarmu) uac -= ARM_BONUS(uarmu);
#endif
	/* Special coding for monks and their natural defensive AC */
	if(Role_if(PM_MONK) && !uwep && !uarms &&
	   (!uarm || (uarm && is_clothes(uarm))))
		uac -= (u.ulevel / 3) + 2;

	/* maid dress brings out a special power of apron and katyusha */
	if(flags.female && uarm && uarm->otyp == MAID_DRESS) {
		if (uarmh && uarmh->otyp == KATYUSHA) uac -= 2;
		if (uarmc && uarmc->otyp == KITCHEN_APRON) uac -= 3;
		if (uarmc && uarmc->otyp == FRILLED_APRON) uac -= 4;
	}
//	if(uleft && uleft->otyp == RIN_PROTECTION) uac -= uleft->spe;
//	if(uright && uright->otyp == RIN_PROTECTION) uac -= uright->spe;
	uac -= u.uprotection;				/* rings, robe, cloak */
	if (HProtection & INTRINSIC) uac -= u.ublessed;	/* god's blessing */
	uac -= u.uspellprot;				/* spell */

	/* Beseark lowers AC by 6 points - SW */
	if (Role_special(PM_BARBARIAN)) uac += 6;

	if (uac < -128) uac = -128;	/* u.uac is an schar */
	if(uac != u.uac){
		u.uac = uac;
		flags.botl = 1;
	}
}

#endif /* OVL0 */
#ifdef OVLB

void
glibr()
{
	register struct obj *otmp;
	int xfl = 0;
	boolean leftfall, rightfall;
	const char *otherwep = 0;

	leftfall = (uleft && !uleft->cursed &&
		    (!uwep || !welded(uwep) || !bimanual(uwep)));
	rightfall = (uright && !uright->cursed && (!welded(uwep)));
	if (!uarmg && (leftfall || rightfall) && !nolimbs(youmonst.data)) {
		/* changed so cursed rings don't fall off, GAN 10/30/86 */
#ifndef JP
		Your("%s off your %s.",
			(leftfall && rightfall) ? "rings slip" : "ring slips",
			(leftfall && rightfall) ? makeplural(body_part(FINGER)) :
			body_part(FINGER));
#else
		Your("�w�ւ�%s%s���甲���������B",
			(leftfall && rightfall) ? "��" : "", body_part(HAND));
#endif /*JP*/
		xfl++;
		if (leftfall) {
			otmp = uleft;
			Ring_off(uleft);
			dropx(otmp);
		}
		if (rightfall) {
			otmp = uright;
			Ring_off(uright);
			dropx(otmp);
		}
	}

	otmp = uswapwep;
	if (u.twoweap && otmp) {
		otherwep = is_sword(otmp) ? c_sword :
		    E_J(makesingular(oclass_names[(int)otmp->oclass]),
			oclass_names[(int)otmp->oclass]);
#ifndef JP
		Your("%s %sslips from your %s.",
			otherwep,
			xfl ? "also " : "",
			makeplural(body_part(HAND)));
#else
		Your("%s%s%s���犊�藎�����B",
			otherwep, xfl ? "���܂�" : "��",
			body_part(HAND));
#endif /*JP*/
		setuswapwep((struct obj *)0);
		xfl++;
		if (otmp->otyp != LOADSTONE || !otmp->cursed)
			dropx(otmp);
	}
	otmp = uwep;
	if (otmp && !welded(otmp)) {
		const char *thiswep;

		/* nice wording if both weapons are the same type */
		thiswep = is_sword(otmp) ? c_sword :
		    E_J(makesingular(oclass_names[(int)otmp->oclass]),
			oclass_names[(int)otmp->oclass]);
		if (otherwep && strcmp(thiswep, otherwep)) otherwep = 0;

		/* changed so cursed weapons don't fall, GAN 10/30/86 */
#ifndef JP
		Your("%s%s %sslips from your %s.",
			otherwep ? "other " : "", thiswep,
			xfl ? "also " : "",
			makeplural(body_part(HAND)));
#else
		Your("%s%s%s%s���犊�藎�����B",
			otherwep ? "���������" : "", thiswep,
			xfl ? "��" : "��", body_part(HAND));
#endif /*JP*/
		setuwep((struct obj *)0);
		if (otmp->otyp != LOADSTONE || !otmp->cursed)
			dropx(otmp);
	}
}

struct obj *
some_armor(victim)
struct monst *victim;
{
	register struct obj *otmph, *otmp;

	otmph = (victim == &youmonst) ? uarmc : which_armor(victim, W_ARMC);
	if (!otmph)
	    otmph = (victim == &youmonst) ? uarm : which_armor(victim, W_ARM);
#ifdef TOURIST
	if (!otmph)
	    otmph = (victim == &youmonst) ? uarmu : which_armor(victim, W_ARMU);
#endif
	
	otmp = (victim == &youmonst) ? uarmh : which_armor(victim, W_ARMH);
	if(otmp && (!otmph || !rn2(4))) otmph = otmp;
	otmp = (victim == &youmonst) ? uarmg : which_armor(victim, W_ARMG);
	if(otmp && (!otmph || !rn2(4))) otmph = otmp;
	otmp = (victim == &youmonst) ? uarmf : which_armor(victim, W_ARMF);
	if(otmp && (!otmph || !rn2(4))) otmph = otmp;
	otmp = (victim == &youmonst) ? uarms : which_armor(victim, W_ARMS);
	if(otmp && (!otmph || !rn2(4))) otmph = otmp;
	return(otmph);
}

/* erode some arbitrary armor worn by the victim */
void
erode_armor(victim, acid_dmg)
struct monst *victim;
boolean acid_dmg;
{
	struct obj *otmph = some_armor(victim);

	if (otmph && (otmph != uarmf)) {
	    erode_obj(otmph, acid_dmg, FALSE);
	    if (carried(otmph)) update_inventory();
	}
}

/* used for praying to check and fix levitation trouble */
struct obj *
stuck_ring(ring, otyp)
struct obj *ring;
int otyp;
{
    if (ring != uleft && ring != uright) {
	impossible("stuck_ring: neither left nor right?");
	return (struct obj *)0;
    }

    if (ring && ring->otyp == otyp) {
	/* reasons ring can't be removed match those checked by select_off();
	   limbless case has extra checks because ordinarily it's temporary */
	if (nolimbs(youmonst.data) &&
		uamul && uamul->otyp == AMULET_OF_UNCHANGING && uamul->cursed)
	    return uamul;
	if (welded(uwep) && (ring == uright || bimanual(uwep))) return uwep;
	if (uarmg && uarmg->cursed) return uarmg;
	if (ring->cursed) return ring;
    }
    /* either no ring or not right type or nothing prevents its removal */
    return (struct obj *)0;
}

/* also for praying; find worn item that confers "Unchanging" attribute */
struct obj *
unchanger()
{
    if (uamul && uamul->otyp == AMULET_OF_UNCHANGING) return uamul;
    return 0;
}

/* occupation callback for 'A' */
STATIC_PTR
int
select_off(otmp)
register struct obj *otmp;
{
	struct obj *why;
	char buf[BUFSZ];

	if (!otmp) return 0;
	*buf = '\0';			/* lint suppresion */

	/* special ring checks */
	if (otmp == uright || otmp == uleft) {
	    if (nolimbs(youmonst.data)) {
		pline_The(E_J("ring is stuck.","�w�ւ͈����������Ă��Ď��Ȃ��B"));
		return 0;
	    }
	    why = 0;	/* the item which prevents ring removal */
	    if (welded(uwep) && (otmp == uright || bimanual(uwep))) {
		Sprintf(buf, E_J("free a weapon %s","����%s���󂯂��"), body_part(HAND));
		why = uwep;
	    } else if (uarmg && uarmg->cursed) {
		Sprintf(buf, E_J("take off your %s","%s���O��"), c_gloves);
		why = uarmg;
	    }
	    if (why) {
		You(E_J("cannot %s to remove the ring.",
			"%s�Ȃ����߁A�w�ւ��O�����Ƃ��ł��Ȃ��B"), buf);
		why->bknown = TRUE;
		return 0;
	    }
	}
	/* special glove checks */
	if (otmp == uarmg) {
	    if (welded(uwep)) {
#ifndef JP
		You("are unable to take off your %s while wielding that %s.",
		    c_gloves, is_sword(uwep) ? c_sword : c_weapon);
#else
		pline("%s���������܂܂ł́A%s���O�����Ƃ��ł��Ȃ��B",
		    is_sword(uwep) ? c_sword : c_weapon, c_gloves);
#endif /*JP*/
		uwep->bknown = TRUE;
		return 0;
	    } else if (Glib) {
#ifndef JP
		You_cant("take off the slippery %s with your slippery %s.",
			 c_gloves, makeplural(body_part(FINGER)));
#else
		You("%s�������Ă��܂��A%s���O�����Ƃ��ł��Ȃ��B",
			body_part(FINGER), c_gloves);
#endif /*JP*/
		return 0;
	    }
	}
	/* special boot checks */
	if (otmp == uarmf) {
	    if (u.utrap && u.utraptype == TT_BEARTRAP) {
		pline_The(E_J("bear trap prevents you from pulling your %s out.",
			      "�g���o�T�~�̂����ŁA%s�������������Ƃ��ł��Ȃ��B"),
			  body_part(FOOT));
		return 0;
	    } else if (u.utrap && u.utraptype == TT_INFLOOR) {
#ifndef JP
		You("are stuck in the %s, and cannot pull your %s out.",
		    surface(u.ux, u.uy), makeplural(body_part(FOOT)));
#else
		Your("%s��%s�ɋ��܂�Ă��āA�����������Ƃ��ł��Ȃ��B",
		     body_part(FOOT), surface(u.ux, u.uy));
#endif /*JP*/
		return 0;
	    }
	}
	/* special suit and shirt checks */
	if (otmp == uarm
#ifdef TOURIST
			|| otmp == uarmu
#endif
		) {
	    why = 0;	/* the item which prevents disrobing */
	    if (uarmc && uarmc->cursed) {
		Sprintf(buf, E_J("remove your %s",
				 "%s��E��"), cloak_simple_name(uarmc));
		why = uarmc;
#ifdef TOURIST
	    } else if (otmp == uarmu && uarm && uarm->cursed) {
		Sprintf(buf, E_J("remove your %s", "%s��E��"), c_suit);
		why = uarm;
#endif
	    } else if (welded(uwep) && bimanual(uwep)) {
		Sprintf(buf, E_J("release your %s","%s�������"),
			is_sword(uwep) ? c_sword :
			(uwep->otyp == BATTLE_AXE) ? c_axe : c_weapon);
		why = uwep;
	    }
	    if (why) {
#ifndef JP
		You("cannot %s to take off %s.", buf, the(xname(otmp)));
#else
		You("%s�Ȃ����߁A%s��E�����Ƃ��ł��Ȃ��B", buf, xname(otmp));
#endif /*JP*/
		why->bknown = TRUE;
		return 0;
	    }
	}
	/* basic curse check */
	if (otmp == uquiver || (otmp == uswapwep && !u.twoweap)) {
	    ;	/* some items can be removed even when cursed */
	} else {
	    /* otherwise, this is fundamental */
	    if (cursed(otmp)) return 0;
	}

	if(otmp == uarm) takeoff_mask |= WORN_ARMOR;
	else if(otmp == uarmc) takeoff_mask |= WORN_CLOAK;
	else if(otmp == uarmf) takeoff_mask |= WORN_BOOTS;
	else if(otmp == uarmg) takeoff_mask |= WORN_GLOVES;
	else if(otmp == uarmh) takeoff_mask |= WORN_HELMET;
	else if(otmp == uarms) takeoff_mask |= WORN_SHIELD;
#ifdef TOURIST
	else if(otmp == uarmu) takeoff_mask |= WORN_SHIRT;
#endif
	else if(otmp == uleft) takeoff_mask |= LEFT_RING;
	else if(otmp == uright) takeoff_mask |= RIGHT_RING;
	else if(otmp == uamul) takeoff_mask |= WORN_AMUL;
	else if(otmp == ublindf) takeoff_mask |= WORN_BLINDF;
	else if(otmp == uwep) takeoff_mask |= W_WEP;
	else if(otmp == uswapwep) takeoff_mask |= W_SWAPWEP;
	else if(otmp == uquiver) takeoff_mask |= W_QUIVER;

	else impossible("select_off: %s???", doname(otmp));

	return(0);
}

STATIC_OVL struct obj *
do_takeoff()
{
	register struct obj *otmp = (struct obj *)0;

	if (taking_off == W_WEP) {
	  if(!cursed(uwep)) {
	    setuwep((struct obj *) 0);
	    You(E_J("are empty %s.","����%s�ɂ��Ă��Ȃ��B"), body_part(HANDED));
	    u.twoweap = FALSE;
	  }
	} else if (taking_off == W_SWAPWEP) {
	  setuswapwep((struct obj *) 0);
	  You(E_J("no longer have a second weapon readied.",
		  "�\���̕�������߂��B"));
	  u.twoweap = FALSE;
	} else if (taking_off == W_QUIVER) {
	  setuqwep((struct obj *) 0);
	  You(E_J("no longer have ammunition readied.",
		  "�����ɂ����B"));
	} else if (taking_off == WORN_ARMOR) {
	  otmp = uarm;
	  if(!cursed(otmp)) (void) Armor_off();
	} else if (taking_off == WORN_CLOAK) {
	  otmp = uarmc;
	  if(!cursed(otmp)) (void) Cloak_off();
	} else if (taking_off == WORN_BOOTS) {
	  otmp = uarmf;
	  if(!cursed(otmp)) (void) Boots_off();
	} else if (taking_off == WORN_GLOVES) {
	  otmp = uarmg;
	  if(!cursed(otmp)) (void) Gloves_off();
	} else if (taking_off == WORN_HELMET) {
	  otmp = uarmh;
	  if(!cursed(otmp)) (void) Helmet_off();
	} else if (taking_off == WORN_SHIELD) {
	  otmp = uarms;
	  if(!cursed(otmp)) (void) Shield_off();
#ifdef TOURIST
	} else if (taking_off == WORN_SHIRT) {
	  otmp = uarmu;
	  if (!cursed(otmp)) (void) Shirt_off();
#endif
	} else if (taking_off == WORN_AMUL) {
	  otmp = uamul;
	  if(!cursed(otmp)) Amulet_off();
	} else if (taking_off == LEFT_RING) {
	  otmp = uleft;
	  if(!cursed(otmp)) Ring_off(uleft);
	} else if (taking_off == RIGHT_RING) {
	  otmp = uright;
	  if(!cursed(otmp)) Ring_off(uright);
	} else if (taking_off == WORN_BLINDF) {
	  if (!cursed(ublindf)) Blindf_off(ublindf);
	} else impossible("do_takeoff: taking off %lx", taking_off);

	return(otmp);
}

static const char *disrobing = "";

STATIC_PTR
int
take_off()
{
	register int i;
	register struct obj *otmp;

	if (taking_off) {
	    if (todelay > 0) {
		todelay--;
		return(1);	/* still busy */
	    } else {
		if ((otmp = do_takeoff())) off_msg(otmp);
	    }
	    takeoff_mask &= ~taking_off;
	    taking_off = 0L;
	}

	for(i = 0; takeoff_order[i]; i++)
	    if(takeoff_mask & takeoff_order[i]) {
		taking_off = takeoff_order[i];
		break;
	    }

	otmp = (struct obj *) 0;
	todelay = 0;

	if (taking_off == 0L) {
	  You(E_J("finish %s.","%s���I�����B"), disrobing);
	  return 0;
	} else if (taking_off == W_WEP) {
	  todelay = 1;
	} else if (taking_off == W_SWAPWEP) {
	  todelay = 1;
	} else if (taking_off == W_QUIVER) {
	  todelay = 1;
	} else if (taking_off == WORN_ARMOR) {
	  otmp = uarm;
	  /* If a cloak is being worn, add the time to take it off and put
	   * it back on again.  Kludge alert! since that time is 0 for all
	   * known cloaks, add 1 so that it actually matters...
	   */
	  if (uarmc) todelay += 2 * objects[uarmc->otyp].oc_delay + 1;
	} else if (taking_off == WORN_CLOAK) {
	  otmp = uarmc;
	} else if (taking_off == WORN_BOOTS) {
	  otmp = uarmf;
	} else if (taking_off == WORN_GLOVES) {
	  otmp = uarmg;
	} else if (taking_off == WORN_HELMET) {
	  otmp = uarmh;
	} else if (taking_off == WORN_SHIELD) {
	  otmp = uarms;
#ifdef TOURIST
	} else if (taking_off == WORN_SHIRT) {
	  otmp = uarmu;
	  /* add the time to take off and put back on armor and/or cloak */
	  if (uarm)  todelay += 2 * objects[uarm->otyp].oc_delay;
	  if (uarmc) todelay += 2 * objects[uarmc->otyp].oc_delay + 1;
#endif
	} else if (taking_off == WORN_AMUL) {
	  todelay = 1;
	} else if (taking_off == LEFT_RING) {
	  todelay = 1;
	} else if (taking_off == RIGHT_RING) {
	  todelay = 1;
	} else if (taking_off == WORN_BLINDF) {
	  todelay = 2;
	} else {
	  impossible("take_off: taking off %lx", taking_off);
	  return 0;	/* force done */
	}

	if (otmp) todelay += objects[otmp->otyp].oc_delay;

	/* Since setting the occupation now starts the counter next move, that
	 * would always produce a delay 1 too big per item unless we subtract
	 * 1 here to account for it.
	 */
	if (todelay > 0) todelay--;

	set_occupation(take_off, disrobing, 0);
	return(1);		/* get busy */
}

/* clear saved context to avoid inappropriate resumption of interrupted 'A' */
void
reset_remarm()
{
	taking_off = takeoff_mask = 0L;
	disrobing = nul;
}

/* the 'A' command -- remove multiple worn items */
int
doddoremarm()
{
    int result = 0;

    if (taking_off || takeoff_mask) {
	You(E_J("continue %s.","%s���ĊJ�����B"), disrobing);
	set_occupation(take_off, disrobing, 0);
	return 0;
    } else if (!uwep && !uswapwep && !uquiver && !uamul && !ublindf &&
		!uleft && !uright && !wearing_armor()) {
	You(E_J("are not wearing anything.",
		"�����g�ɂ��Ă��Ȃ��B"));
	return 0;
    }

    add_valid_menu_class(0); /* reset */
    if (flags.menu_style != MENU_TRADITIONAL ||
	    (result = ggetobj(E_J("take off",&takeoff_words), select_off, 0, FALSE, (unsigned *)0)) < -1)
	result = menu_remarm(result);

    if (takeoff_mask) {
	/* default activity for armor and/or accessories,
	   possibly combined with weapons */
	disrobing = E_J("disrobing","�����̉���");
	/* specific activity when handling weapons only */
	if (!(takeoff_mask & ~(W_WEP|W_SWAPWEP|W_QUIVER)))
	    disrobing = E_J("disarming","�����̉���");
	(void) take_off();
    }
    /* The time to perform the command is already completely accounted for
     * in take_off(); if we return 1, that would add an extra turn to each
     * disrobe.
     */
    return 0;
}

STATIC_OVL int
menu_remarm(retry)
int retry;
{
    int n, i = 0;
    menu_item *pick_list;
    boolean all_worn_categories = TRUE;

    if (retry) {
	all_worn_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
	all_worn_categories = FALSE;
	n = query_category(E_J("What type of things do you want to take off?",
				"�ǂ̎�ނ̑������O���܂����H"),
			   invent, WORN_TYPES|ALL_TYPES, &pick_list, PICK_ANY);
	if (!n) return 0;
	for (i = 0; i < n; i++) {
	    if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
		all_worn_categories = TRUE;
	    else
		add_valid_menu_class(pick_list[i].item.a_int);
	}
	free((genericptr_t) pick_list);
    } else if (flags.menu_style == MENU_COMBINATION) {
	all_worn_categories = FALSE;
	if (ggetobj(E_J("take off",&takeoff_words), select_off, 0, TRUE, (unsigned *)0) == -2)
	    all_worn_categories = TRUE;
    }

    n = query_objlist(E_J("What do you want to take off?",
			  "�����O���܂����H"), invent,
			SIGNAL_NOMENU|USE_INVLET|INVORDER_SORT,
			&pick_list, PICK_ANY,
			all_worn_categories ? is_worn : is_worn_by_type);
    if (n > 0) {
	for (i = 0; i < n; i++)
	    (void) select_off(pick_list[i].item.a_obj);
	free((genericptr_t) pick_list);
    } else if (n < 0 && flags.menu_style != MENU_COMBINATION) {
#ifndef JP
	There("is nothing else you can remove or unwield.");
#else
	pline("�O���鑕���͂���őS�����B");
#endif /*JP*/
    }
    return 0;
}

/* hit by destroy armor scroll/black dragon breath/monster spell */
int
destroy_arm(atmp)
register struct obj *atmp;
{
	register struct obj *otmp;
#define DESTROY_ARM(o) ((otmp = (o)) != 0 && \
			(!atmp || atmp == otmp) && \
			(!obj_resists(otmp, 0, 90)))

	if (DESTROY_ARM(uarmc)) {
		if (donning(otmp)) cancel_don();
		Your(E_J("%s crumbles and turns to dust!",
			 "%s�͂��������ɗ􂯁A�o�Ɖ������I"),
		     cloak_simple_name(uarmc));
		(void) Cloak_off();
		useup(otmp);
	} else if (DESTROY_ARM(uarm)) {
		if (donning(otmp)) cancel_don();
		Your(E_J("%s turns to dust and falls to the %s!",
			 "%s�͐o�Ɖ����A%s�̏�ɕ��ꗎ�����I"),
			armor_simple_name(otmp), surface(u.ux,u.uy));
		(void) Armor_gone();
		useup(otmp);
#ifdef TOURIST
	} else if (DESTROY_ARM(uarmu)) {
		if (donning(otmp)) cancel_don();
		Your(E_J("shirt crumbles into tiny threads and falls apart!",
			 "�V���c�͂΂�΂�̎����ƂȂ��ĕ��ꗎ�����I"));
		(void) Shirt_off();
		useup(otmp);
#endif
	} else if (DESTROY_ARM(uarmh)) {
		if (donning(otmp)) cancel_don();
		Your(E_J("%s turns to dust and is blown away!",
			 "%s�͐o�Ɖ����A������񂾁I"),
		     helm_simple_name(otmp));
		(void) Helmet_off();
		useup(otmp);
	} else if (DESTROY_ARM(uarmg)) {
		if (donning(otmp)) cancel_don();
		Your(E_J("gloves vanish!","�U�肪���ł����I"));
		(void) Gloves_off();
		useup(otmp);
		selftouch(E_J("You","���Ȃ���"));
	} else if (DESTROY_ARM(uarmf)) {
		if (donning(otmp)) cancel_don();
		Your(E_J("boots disintegrate!","�u�[�c���������ꂽ�I"));
		(void) Boots_off();
		useup(otmp);
	} else if (DESTROY_ARM(uarms)) {
		if (donning(otmp)) cancel_don();
		Your(E_J("shield crumbles away!","�����ӂ��U�����I"));
		(void) Shield_off();
		useup(otmp);
	} else {
		return 0;		/* could not destroy anything */
	}

#undef DESTROY_ARM
	stop_occupation();
	return(1);
}

void
adj_abon(otmp, delta)
register struct obj *otmp;
register schar delta;
{
	if (!otmp || !delta) return;
	if (uarmh == otmp) {
	    switch (otmp->otyp) {
		case HELM_OF_BRILLIANCE:
			makeknown(uarmh->otyp);
			ABON(A_INT) += (delta);
			ABON(A_WIS) += (delta);
			break;
		default:
			break;
	    }
	} else if (uleft == otmp || uright == otmp) {
	    boolean kn = FALSE;
	    switch (otmp->otyp) {
		case RIN_INCREASE_DAMAGE:
			u.udaminc += delta;
			break;
		case RIN_INCREASE_ACCURACY:
			u.uhitinc += delta;
			break;
		case RIN_PROTECTION:
			u.uprotection += delta;
			break;
		case RIN_GAIN_STRENGTH:
			ABON(A_STR) += delta;
			kn = TRUE;
			break;
		case RIN_GAIN_CONSTITUTION:
			ABON(A_CON) += delta;
			recalchpmax();
			kn = TRUE;
			break;
		case RIN_ADORNMENT:
			ABON(A_CHA) += delta;
			kn = TRUE;
			break;
		default:
			break;
	    }
	    if (kn) {
		makeknown(otmp->otyp);
		otmp->known = TRUE;
	    }
	}
	flags.botl = 1;
}

#endif /* OVLB */

/*do_wear.c*/
