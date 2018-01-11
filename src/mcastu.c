/*	SCCS Id: @(#)mcastu.c	3.4	2003/01/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* monster mage spells */
#define MGC_PSI_BOLT	 0
#define MGC_CURE_SELF	 1
#define MGC_HASTE_SELF	 2
#define MGC_STUN_YOU	 3
#define MGC_DISAPPEAR	 4
#define MGC_WEAKEN_YOU	 5
#define MGC_DESTRY_ARMR	 6
#define MGC_CURSE_ITEMS	 7
#define MGC_AGGRAVATION	 8
#define MGC_SUMMON_MONS	 9
#define MGC_CLONE_WIZ	10
#define MGC_DEATH_TOUCH	11

/* monster cleric spells */
#define CLC_OPEN_WOUNDS	 0
#define CLC_CURE_SELF	 1
#define CLC_CONFUSE_YOU	 2
#define CLC_PARALYZE	 3
#define CLC_BLIND_YOU	 4
#define CLC_INSECTS	 5
#define CLC_CURSE_ITEMS	 6
#define CLC_LIGHTNING	 7
#define CLC_FIRE_PILLAR	 8
#define CLC_GEYSER	 9

STATIC_DCL void FDECL(cursetxt,(struct monst *,BOOLEAN_P));
STATIC_DCL int FDECL(choose_magic_spell, (int));
STATIC_DCL int FDECL(choose_clerical_spell, (int));
STATIC_DCL void FDECL(cast_wizard_spell,(struct monst *, int,int));
STATIC_DCL void FDECL(cast_cleric_spell,(struct monst *, int,int));
STATIC_DCL boolean FDECL(is_undirected_spell,(unsigned int,int));
STATIC_DCL boolean FDECL(spell_would_be_useless,(struct monst *,unsigned int,int));

#ifdef OVL0

extern const char * const flash_types[];	/* from zap.c */

/* feedback when frustrated monster couldn't cast a spell */
STATIC_OVL
void
cursetxt(mtmp, undirected)
struct monst *mtmp;
boolean undirected;
{
	if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
	    const char *point_msg;  /* spellcasting monsters are impolite */

	    if (mtmp->mstun || mtmp->mconf) {
		pline(E_J("%s tries to cast a spell, but cannot concentrate.",
			  "%s�͎����������悤�Ƃ������A���_���W�����邱�Ƃ��ł��Ȃ������B"), Monnam(mtmp));
		return;
	    }

	    if (undirected)
		point_msg = E_J("all around, then curses","�����炶�イ���w���A�􂢂̌��t��f����");
	    else if ((Invis && !perceives(mtmp->data) &&
			(mtmp->mux != u.ux || mtmp->muy != u.uy)) ||
		    (youmonst.m_ap_type == M_AP_OBJECT &&
			youmonst.mappearance == STRANGE_OBJECT) ||
		    u.uundetected)
		point_msg = E_J("and curses in your general direction",
				"���Ȃ�������Ƃ��ڂ����ʒu�Ɍ������Ď􂢂̌��t��f����");
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		point_msg = E_J("and curses at your displaced image",
				"���Ȃ��̌��e�Ɍ������Ď􂢂̌��t��f����");
	    else
		point_msg = E_J("at you, then curses","���Ȃ����w���A�􂢂̌��t��f����");

	    pline(E_J("%s points %s.","%s��%s�B"), Monnam(mtmp), point_msg);
	} else if ((!(moves % 4) || !rn2(4))) {
	    if (flags.soundok) Norep(E_J("You hear a mumbled curse.",
					 "���҂����ڂ��ڂ��Ǝ􂢂̌��t��f���̂����������B"));
	}
}

#endif /* OVL0 */
#ifdef OVLB

/* convert a level based random selection into a specific mage spell;
   inappropriate choices will be screened out by spell_would_be_useless() */
STATIC_OVL int
choose_magic_spell(spellval)
int spellval;
{
    switch (spellval) {
    case 22:
    case 21:
    case 20:
	return MGC_DEATH_TOUCH;
    case 19:
    case 18:
	return MGC_CLONE_WIZ;
    case 17:
    case 16:
    case 15:
	return MGC_SUMMON_MONS;
    case 14:
    case 13:
	return MGC_AGGRAVATION;
    case 12:
    case 11:
    case 10:
	return MGC_CURSE_ITEMS;
    case 9:
    case 8:
	return MGC_DESTRY_ARMR;
    case 7:
    case 6:
	return MGC_WEAKEN_YOU;
    case 5:
    case 4:
	return MGC_DISAPPEAR;
    case 3:
	return MGC_STUN_YOU;
    case 2:
	return MGC_HASTE_SELF;
    case 1:
	return MGC_CURE_SELF;
    case 0:
    default:
	return MGC_PSI_BOLT;
    }
}

/* convert a level based random selection into a specific cleric spell */
STATIC_OVL int
choose_clerical_spell(spellnum)
int spellnum;
{
    switch (spellnum) {
    case 13:
	return CLC_GEYSER;
    case 12:
	return CLC_FIRE_PILLAR;
    case 11:
	return CLC_LIGHTNING;
    case 10:
    case 9:
	return CLC_CURSE_ITEMS;
    case 8:
	return CLC_INSECTS;
    case 7:
    case 6:
	return CLC_BLIND_YOU;
    case 5:
    case 4:
	return CLC_PARALYZE;
    case 3:
    case 2:
	return CLC_CONFUSE_YOU;
    case 1:
	return CLC_CURE_SELF;
    case 0:
    default:
	return CLC_OPEN_WOUNDS;
    }
}

/* return values:
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmu(mtmp, mattk, thinks_it_foundyou, foundyou)
	register struct monst *mtmp;
	register struct attack *mattk;
	boolean thinks_it_foundyou;
	boolean foundyou;
{
	int	dmg, ml = mtmp->m_lev;
	int ret;
	int spellnum = 0;

	/* Three cases:
	 * -- monster is attacking you.  Search for a useful spell.
	 * -- monster thinks it's attacking you.  Search for a useful spell,
	 *    without checking for undirected.  If the spell found is directed,
	 *    it fails with cursetxt() and loss of mspec_used.
	 * -- monster isn't trying to attack.  Select a spell once.  Don't keep
	 *    searching; if that spell is not useful (or if it's directed),
	 *    return and do something else. 
	 * Since most spells are directed, this means that a monster that isn't
	 * attacking casts spells only a small portion of the time that an
	 * attacking monster does.
	 */
	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
	    int cnt = 40;

	    do {
		spellnum = rn2(ml);
		if (mattk->adtyp == AD_SPEL)
		    spellnum = choose_magic_spell(spellnum);
		else
		    spellnum = choose_clerical_spell(spellnum);
		/* not trying to attack?  don't allow directed spells */
		if (!thinks_it_foundyou) {
		    if (!is_undirected_spell(mattk->adtyp, spellnum) ||
			spell_would_be_useless(mtmp, mattk->adtyp, spellnum)) {
			if (foundyou)
			    impossible("spellcasting monster found you and doesn't know it?");
			return 0;
		    }
		    break;
		}
	    } while(--cnt > 0 &&
		    spell_would_be_useless(mtmp, mattk->adtyp, spellnum));
	    if (cnt == 0) return 0;
	}

	/* monster unable to cast spells? */
	if(mtmp->mcan || mtmp->mspec_used || mtmp->mstun || mtmp->mconf || !ml) {
	    cursetxt(mtmp, is_undirected_spell(mattk->adtyp, spellnum));
	    return(0);
	}

	if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
	    mtmp->mspec_used = 10 - mtmp->m_lev;
	    if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
	}

	/* monster can cast spells, but is casting a directed spell at the
	   wrong place?  If so, give a message, and return.  Do this *after*
	   penalizing mspec_used. */
	if (!foundyou && thinks_it_foundyou &&
		!is_undirected_spell(mattk->adtyp, spellnum)) {
#ifndef JP
	    pline("%s casts a spell at %s!",
		canseemon(mtmp) ? Monnam(mtmp) : "Something",
		levl[mtmp->mux][mtmp->muy].typ == WATER
		    ? "empty water" : "thin air");
#else
	    pline("%s��%s�Ɍ������Ď������������I",
		canseemon(mtmp) ? mon_nam(mtmp) : "���҂�",
		levl[mtmp->mux][mtmp->muy].typ == WATER
		    ? "�������Ȃ�����" : "����");
#endif /*JP*/
	    return(0);
	}

	nomul(0);
	if(rn2(ml*10) < (mtmp->mconf ? 100 : 20)) {	/* fumbled attack */
	    if (canseemon(mtmp) && flags.soundok)
		pline_The(E_J("air crackles around %s.",
			      "��C��%s�̎���Ŋ��������𗧂Ă��B"), mon_nam(mtmp));
	    return(0);
	}
	if (canspotmon(mtmp) || !is_undirected_spell(mattk->adtyp, spellnum)) {
	    pline(E_J("%s casts a spell%s!","%s��%s�������������I"),
		  canspotmon(mtmp) ? Monnam(mtmp) : E_J("Something","���҂�"),
		  is_undirected_spell(mattk->adtyp, spellnum) ? "" :
		  (Invisible && !perceives(mtmp->data) && 
		   (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  E_J(" at a spot near you","���Ȃ��̋߂��߂�����") :
		  (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  E_J(" at your displaced image","���Ȃ��̌��e�߂�����") :
		  E_J(" at you","���Ȃ��߂�����"));
	}

/*
 *	As these are spells, the damage is related to the level
 *	of the monster casting the spell.
 */
	if (!foundyou) {
	    dmg = 0;
	    if (mattk->adtyp != AD_SPEL && mattk->adtyp != AD_CLRC) {
		impossible(
	      "%s casting non-hand-to-hand version of hand-to-hand spell %d?",
			   Monnam(mtmp), mattk->adtyp);
		return(0);
	    }
	} else if (mattk->damd)
	    dmg = d((int)((ml/2) + mattk->damn), (int)mattk->damd);
	else dmg = d((int)((ml/2) + 1), 6);
	if (Half_spell_damage) dmg = (dmg+1) / 2;

	ret = 1;

	switch (mattk->adtyp) {

	    case AD_FIRE:
		pline(E_J("You're enveloped in flames.",
			  "���Ȃ��͉��ɕ�܂ꂽ�B"));
		if(Fire_resistance) {
		    if (is_full_resist(FIRE_RES)) {
			shieldeff(u.ux, u.uy);
			pline(E_J("But you resist the effects.",
				  "�������A���Ȃ��͉e�����󂯂Ȃ��B"));
			dmg = 0;
		    } else {
			dmg = (dmg+3) / 4;
		    }
		}
		burn_away_slime();
		break;
	    case AD_COLD:
		pline(E_J("You're covered in frost.",
			  "���Ȃ��͑��ɕ���ꂽ�B"));
		if(Cold_resistance) {
		    if (is_full_resist(COLD_RES)) {
			shieldeff(u.ux, u.uy);
			pline(E_J("But you resist the effects.",
				  "�������A���Ȃ��͉e�����󂯂Ȃ��B"));
			dmg = 0;
		    } else {
			dmg = (dmg+3) / 4;
		    }
		}
		break;
	    case AD_MAGM:
		You(E_J("are hit by a shower of missiles!",
			"�~�蒍�����͂̎U�e�Ɍ����ꂽ�I"));
		if(Antimagic) {
			shieldeff(u.ux, u.uy);
			pline_The(E_J("missiles bounce off!",
				      "���Ȃ��͒e���͂˕Ԃ����I"));
			damage_resistant_obj(ANTIMAGIC, 1);
			dmg = 0;
		} else dmg = d((int)mtmp->m_lev/2 + 1,6);
		break;
	    case AD_SPEL:	/* wizard spell */
	    case AD_CLRC:       /* clerical spell */
	    {
		if (mattk->adtyp == AD_SPEL)
		    cast_wizard_spell(mtmp, dmg, spellnum);
		else
		    cast_cleric_spell(mtmp, dmg, spellnum);
		dmg = 0; /* done by the spell casting functions */
		break;
	    }
	}
	if(dmg) mdamageu(mtmp, dmg);
	return(ret);
}

/* monster wizard and cleric spellcasting functions */
/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and spellnum should always be a valid
   undirected spell.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
STATIC_OVL
void
cast_wizard_spell(mtmp, dmg, spellnum)
struct monst *mtmp;
int dmg;
int spellnum;
{
    if (dmg == 0 && !is_undirected_spell(AD_SPEL, spellnum)) {
	impossible("cast directed wizard spell (%d) with dmg=0?", spellnum);
	return;
    }

    switch (spellnum) {
    case MGC_DEATH_TOUCH:
#ifndef JP
	pline("Oh no, %s's using the touch of death!", mhe(mtmp));
#else
	pline("�Ȃ�Ă������A�z�͎��̐ڐG���g�����I");
#endif /*JP*/
	if (nonliving(youmonst.data) || is_demon(youmonst.data)) {
	    You(E_J("seem no deader than before.",
		    "����ȏ㎀�ȂȂ��悤���B"));
	} else if (!Antimagic && rn2(mtmp->m_lev) > 12) {
	    if (Hallucination) {
		You(E_J("have an out of body experience.","�Վ��̌��������B"));
	    } else {
		killer_format = E_J(KILLED_BY_AN,KILLED_SUFFIX);
		killer = E_J("touch of death","���̎w��");
		done(DIED);
	    }
	} else {
	    if (Antimagic) shieldeff(u.ux, u.uy);
	    pline(E_J("Lucky for you, it didn't work!","�K�^�Ȃ��ƂɁA����͎��s�����I"));
	    damage_resistant_obj(ANTIMAGIC, rnd(3));
	}
	dmg = 0;
	break;
    case MGC_CLONE_WIZ:
	if (mtmp->iswiz && flags.no_of_wizards == 1) {
	    pline(E_J("Double Trouble...","��d�ꂾ�c�B"));
	    clonewiz();
	    dmg = 0;
	} else
	    impossible("bad wizard cloning?");
	break;
    case MGC_SUMMON_MONS:
    {
	int count;

	count = nasty(mtmp);	/* summon something nasty */
	if (mtmp->iswiz)
#ifndef JP
	    verbalize("Destroy the thief, my pet%s!", plur(count));
#else
	    verbalize("���l�ǂ���A���l�𔪂􂫂ɂ���I");
#endif /*JP*/
	else {
#ifndef JP
	    const char *mappear =
		(count == 1) ? "A monster appears" : "Monsters appear";

	    /* messages not quite right if plural monsters created but
	       only a single monster is seen */
	    if (Invisible && !perceives(mtmp->data) &&
				    (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("%s around a spot near you!", mappear);
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("%s around your displaced image!", mappear);
	    else
		pline("%s from nowhere!", mappear);
#else
	    if (Invisible && !perceives(mtmp->data) &&
				    (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("���������Ȃ��̋߂��̈ʒu���͂ނ悤�Ɍ��ꂽ�I");
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("���������Ȃ��̌��e���͂ނ悤�Ɍ��ꂽ�I");
	    else
		pline("�������ǂ�����Ƃ��Ȃ����ꂽ�I");
#endif /*JP*/
	}
	dmg = 0;
	break;
    }
    case MGC_AGGRAVATION:
	You_feel(E_J("that monsters are aware of your presence.",
		     "���������Ȃ��̑��݂����m�����̂ɋC�Â����B"));
	aggravate();
	dmg = 0;
	break;
    case MGC_CURSE_ITEMS:
	You_feel(E_J("as if you need some help.","�N���̏������K�v�ȋC�������B"));
	rndcurse();
	dmg = 0;
	break;
    case MGC_DESTRY_ARMR:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    pline(E_J("A field of force surrounds you!","�͏ꂪ���Ȃ������͂񂾁I"));
	    damage_resistant_obj(ANTIMAGIC, rnd(3));
	} else if (!destroy_arm(some_armor(&youmonst))) {
	    Your(E_J("skin itches.","�����y���Ȃ����B"));
	}
	dmg = 0;
	break;
    case MGC_WEAKEN_YOU:		/* drain strength */
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    You_feel(E_J("momentarily weakened.","��u��߂�ꂽ�B"));
	    damage_resistant_obj(ANTIMAGIC, 1);
	} else {
	    You(E_J("suddenly feel weaker!","�ˑR��X�����Ȃ����I"));
	    dmg = mtmp->m_lev - 6;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    losestr(rnd(dmg));
	    if (u.uhp < 1)
		done_in_by(mtmp);
	}
	dmg = 0;
	break;
    case MGC_DISAPPEAR:		/* makes self invisible */
	if (!mtmp->minvis && !mtmp->invis_blkd) {
	    if (canseemon(mtmp))
		pline(E_J("%s suddenly %s!","%s�͓ˑR%s�I"), Monnam(mtmp),
		      !See_invisible ? E_J("disappears","������") : E_J("becomes transparent","�������ɂȂ���"));
	    mon_set_minvis(mtmp);
	    dmg = 0;
	} else
	    impossible("no reason for monster to cast disappear spell?");
	break;
    case MGC_STUN_YOU:
	if (Antimagic || Free_action) {
	    shieldeff(u.ux, u.uy);
	    if (!Stunned) {
		You_feel(E_J("momentarily disoriented.","��u�������o���Ȃ��Ȃ����B"));
		if (Antimagic) damage_resistant_obj(ANTIMAGIC, 1);
	    }
	    make_stunned(1L, FALSE);
	} else {
#ifndef JP
	    You(Stunned ? "struggle to keep your balance." : "reel...");
#else
	    You(Stunned ? "�p����ۂ̂�����ɍ���ɂȂ����B" : "���߂����c�B");
#endif /*JP*/
	    dmg = d(ACURR(A_DEX) < 12 ? 6 : 4, 4);
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    make_stunned(HStun + dmg, FALSE);
	}
	dmg = 0;
	break;
    case MGC_HASTE_SELF:
	mon_adjust_speed(mtmp, 1, (struct obj *)0);
	dmg = 0;
	break;
    case MGC_CURE_SELF:
	if (mtmp->mhp < mtmp->mhpmax) {
	    if (canseemon(mtmp))
		pline(E_J("%s looks better.","%s�͌��C�ɂȂ����悤���B"), Monnam(mtmp));
	    /* note: player healing does 6d4; this used to do 1d8 */
	    if ((mtmp->mhp += d(3,6)) > mtmp->mhpmax)
		mtmp->mhp = mtmp->mhpmax;
	    dmg = 0;
	}
	break;
    case MGC_PSI_BOLT:
	/* prior to 3.4.0 Antimagic was setting the damage to 1--this
	   made the spell virtually harmless to players with magic res. */
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    dmg = (dmg + 1) / 2;
	    damage_resistant_obj(ANTIMAGIC, 1);
	}
	if (dmg <= 5)
	    You(E_J("get a slight %sache.","����%s�ɂ����ڂ����B"), body_part(HEAD));
	else if (dmg <= 10)
	    Your(E_J("brain is on fire!","�]���Ă���悤���I"));
	else if (dmg <= 20)
	    Your(E_J("%s suddenly aches painfully!",
		     "%s�ɓˑR�������ɂ݂��������I"), body_part(HEAD));
	else
	    Your(E_J("%s suddenly aches very painfully!",
		     "%s�ɓˑR�Ђǂ��������ɂ݂��������I"), body_part(HEAD));
	break;
    default:
	impossible("mcastu: invalid magic spell (%d)", spellnum);
	dmg = 0;
	break;
    }

    if (dmg) mdamageu(mtmp, dmg);
}

STATIC_OVL
void
cast_cleric_spell(mtmp, dmg, spellnum)
struct monst *mtmp;
int dmg;
int spellnum;
{
    if (dmg == 0 && !is_undirected_spell(AD_CLRC, spellnum)) {
	impossible("cast directed cleric spell (%d) with dmg=0?", spellnum);
	return;
    }

    switch (spellnum) {
    case CLC_GEYSER:
	/* this is physical damage, not magical damage */
	pline(E_J("A sudden geyser slams into you from nowhere!",
		  "�ˑR�A�����������o���Ă��Ȃ��𒼌������I"));
	dmg = d(8, 6);
	if (Half_physical_damage) dmg = (dmg + 1) / 2;
	break;
    case CLC_FIRE_PILLAR:
	pline(E_J("A pillar of fire strikes all around you!",
		  "���̒������Ȃ��̎��͂ɔR���������I"));
	if (Fire_resistance) {
	    if (is_full_resist(FIRE_RES)) {
		shieldeff(u.ux, u.uy);
		dmg = 0;
	    } else dmg = d(2, 6);
	} else
	    dmg = d(8, 6);
	if (Half_spell_damage) dmg = (dmg + 1) / 2;
	burn_away_slime();
	(void) burnarmor(&youmonst);
	destroy_item(SCROLL_CLASS, AD_FIRE);
	destroy_item(POTION_CLASS, AD_FIRE);
	destroy_item(SPBOOK_CLASS, AD_FIRE);
	destroy_item(TOOL_CLASS, AD_FIRE);
	(void) burn_floor_paper(u.ux, u.uy, TRUE, FALSE);
	break;
    case CLC_LIGHTNING:
    {
	boolean reflects;

	pline(E_J("A bolt of lightning strikes down at you from above!",
		  "��؂̈�Ȃ����Ȃ��ɑł����낳�ꂽ�I"));
	reflects = ureflects(E_J("It bounces off your %s%s.","��Ȃ͂��Ȃ���%s%s�ɂ͂˕Ԃ��ꂽ�B"), "");
	if (reflects || is_full_resist(SHOCK_RES)) {
	    shieldeff(u.ux, u.uy);
	    dmg = 0;
	} else
	    dmg = d(Shock_resistance ? 2 : 8, 6);
	if (reflects) {
	    damage_resistant_obj(REFLECTING, d(2,4));
	    break;
	}
	if (Half_spell_damage) dmg = (dmg + 1) / 2;
	destroy_item(WAND_CLASS, AD_ELEC);
	destroy_item(RING_CLASS, AD_ELEC);
	break;
    }
    case CLC_CURSE_ITEMS:
	You_feel(E_J("as if you need some help.","�N���̏������K�v�ȋC�������B"));
	rndcurse();
	dmg = 0;
	break;
    case CLC_INSECTS:
      {
	/* Try for insects, and if there are none
	   left, go for (sticks to) snakes.  -3. */
	struct permonst *pm = mkclass(S_ANT,0);
	struct monst *mtmp2 = (struct monst *)0;
	char let = (pm ? S_ANT : S_SNAKE);
	boolean success;
	int i;
	coord bypos;
	int quan;

	quan = (mtmp->m_lev < 2) ? 1 : rnd((int)mtmp->m_lev / 2);
	if (quan < 3) quan = 3;
	success = pm ? TRUE : FALSE;
	for (i = 0; i <= quan; i++) {
	    if (!enexto(&bypos, mtmp->mux, mtmp->muy, mtmp->data))
		break;
	    if ((pm = mkclass(let,0)) != 0 &&
		    (mtmp2 = makemon(pm, bypos.x, bypos.y, NO_MM_FLAGS)) != 0) {
		success = TRUE;
		mtmp2->msleeping = mtmp2->mpeaceful = mtmp2->mtame = 0;
		set_malign(mtmp2);
	    }
	}
	/* Not quite right:
         * -- message doesn't always make sense for unseen caster (particularly
	 *    the first message)
         * -- message assumes plural monsters summoned (non-plural should be
         *    very rare, unlike in nasty())
         * -- message assumes plural monsters seen
         */
	if (!success)
	    pline(E_J("%s casts at a clump of sticks, but nothing happens.",
		      "%s�͖_�̑��Ɏ��������������A�����N����Ȃ������B"),
		Monnam(mtmp));
	else if (let == S_SNAKE)
	    pline(E_J("%s transforms a clump of sticks into snakes!",
		      "%s�͖_�̑����ււƕς����I"),
		Monnam(mtmp));
	else if (Invisible && !perceives(mtmp->data) &&
				(mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline(E_J("%s summons insects around a spot near you!",
		      "%s�͂��Ȃ��̋߂��̈ʒu�߂����Ē������������I"),
		Monnam(mtmp));
	else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline(E_J("%s summons insects around your displaced image!",
		      "%s�͂��Ȃ��̌��e�߂����Ē������������I"),
		Monnam(mtmp));
	else
	    pline(E_J("%s summons insects!","%s�͒������������I"), Monnam(mtmp));
	dmg = 0;
	break;
      }
    case CLC_BLIND_YOU:
	/* note: resists_blnd() doesn't apply here */
	if (!Blinded) {
	    int num_eyes = eyecount(youmonst.data);
#ifndef JP
	    pline("Scales cover your %s!",
		  (num_eyes == 1) ?
		  body_part(EYE) : makeplural(body_part(EYE)));
#else
	    pline("�؂����Ȃ���%s�𕢂����I", body_part(EYE));
#endif /*JP*/
	    make_blinded(Half_spell_damage ? 100L : 200L, FALSE);
	    if (!Blind) Your(vision_clears);
	    dmg = 0;
	} else
	    impossible("no reason for monster to cast blindness spell?");
	break;
    case CLC_PARALYZE:
	if (Antimagic || Free_action) {
	    shieldeff(u.ux, u.uy);
	    if (multi >= 0) {
		You(E_J("stiffen briefly.","��u�����Ȃ��Ȃ����B"));
		if (Antimagic) damage_resistant_obj(ANTIMAGIC, 1);
	    }
	    nomul(-1);
	} else {
	    if (multi >= 0)
		You(E_J("are frozen in place!","���̏�œ����Ȃ��Ȃ����I"));
	    dmg = 4 + (int)mtmp->m_lev;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    nomul(-dmg);
	}
	dmg = 0;
	break;
    case CLC_CONFUSE_YOU:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    You_feel(E_J("momentarily dizzy.","��u���������B"));
	    damage_resistant_obj(ANTIMAGIC, 1);
	} else {
	    boolean oldprop = !!Confusion;

	    dmg = (int)mtmp->m_lev;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    make_confused(HConfusion + dmg, TRUE);
	    if (Hallucination)
		You_feel(E_J("%s!","%s�g���b�v�����I"), oldprop ? E_J("trippier","�܂��܂�") : E_J("trippy",""));
	    else
		You_feel(E_J("%sconfused!","%s���������I"), oldprop ? E_J("more ","�����") : "");
	}
	dmg = 0;
	break;
    case CLC_CURE_SELF:
	if (mtmp->mhp < mtmp->mhpmax) {
	    if (canseemon(mtmp))
		pline(E_J("%s looks better.","%s�͌��C�ɂȂ����悤���B"), Monnam(mtmp));
	    /* note: player healing does 6d4; this used to do 1d8 */
	    if ((mtmp->mhp += d(3,6)) > mtmp->mhpmax)
		mtmp->mhp = mtmp->mhpmax;
	    dmg = 0;
	}
	break;
    case CLC_OPEN_WOUNDS:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    dmg = (dmg + 1) / 2;
	    damage_resistant_obj(ANTIMAGIC, rnd(3));
	}
	if (dmg <= 5)
	    Your(E_J("skin itches badly for a moment.","������u�Ђǂ��ɂ񂾁B"));
	else if (dmg <= 10)
	    pline(E_J("Wounds appear on your body!","���Ȃ��̐g�̂ɏ������ꂽ�I"));
	else if (dmg <= 20)
	    pline(E_J("Severe wounds appear on your body!","���Ȃ��̐g�̂ɂЂǂ��������ꂽ�I"));
	else
	    Your(E_J("body is covered with painful wounds!","�g�̂��ɁX�����d���ŕ���ꂽ�I"));
	break;
    default:
	impossible("mcastu: invalid clerical spell (%d)", spellnum);
	dmg = 0;
	break;
    }

    if (dmg) mdamageu(mtmp, dmg);
}

STATIC_DCL
boolean
is_undirected_spell(adtyp, spellnum)
unsigned int adtyp;
int spellnum;
{
    if (adtyp == AD_SPEL) {
	switch (spellnum) {
	case MGC_CLONE_WIZ:
	case MGC_SUMMON_MONS:
	case MGC_AGGRAVATION:
	case MGC_DISAPPEAR:
	case MGC_HASTE_SELF:
	case MGC_CURE_SELF:
	    return TRUE;
	default:
	    break;
	}
    } else if (adtyp == AD_CLRC) {
	switch (spellnum) {
	case CLC_INSECTS:
	case CLC_CURE_SELF:
	    return TRUE;
	default:
	    break;
	}
    }
    return FALSE;
}

/* Some spells are useless under some circumstances. */
STATIC_DCL
boolean
spell_would_be_useless(mtmp, adtyp, spellnum)
struct monst *mtmp;
unsigned int adtyp;
int spellnum;
{
    /* Some spells don't require the player to really be there and can be cast
     * by the monster when you're invisible, yet still shouldn't be cast when
     * the monster doesn't even think you're there.
     * This check isn't quite right because it always uses your real position.
     * We really want something like "if the monster could see mux, muy".
     */
    boolean mcouldseeu = couldsee(mtmp->mx, mtmp->my);

    if (adtyp == AD_SPEL) {
	/* aggravate monsters, etc. won't be cast by peaceful monsters */
	if (mtmp->mpeaceful && (spellnum == MGC_AGGRAVATION ||
		spellnum == MGC_SUMMON_MONS || spellnum == MGC_CLONE_WIZ))
	    return TRUE;
	/* haste self when already fast */
	if (mtmp->permspeed == MFAST && spellnum == MGC_HASTE_SELF)
	    return TRUE;
	/* invisibility when already invisible */
	if ((mtmp->minvis || mtmp->invis_blkd) && spellnum == MGC_DISAPPEAR)
	    return TRUE;
	/* peaceful monster won't cast invisibility if you can't see invisible,
	   same as when monsters drink potions of invisibility.  This doesn't
	   really make a lot of sense, but lets the player avoid hitting
	   peaceful monsters by mistake */
	if (mtmp->mpeaceful && !See_invisible && spellnum == MGC_DISAPPEAR)
	    return TRUE;
	/* healing when already healed */
	if (mtmp->mhp == mtmp->mhpmax && spellnum == MGC_CURE_SELF)
	    return TRUE;
	/* don't summon monsters if it doesn't think you're around */
	if (!mcouldseeu && (spellnum == MGC_SUMMON_MONS ||
		(!mtmp->iswiz && spellnum == MGC_CLONE_WIZ)))
	    return TRUE;
	if ((!mtmp->iswiz || flags.no_of_wizards > 1)
						&& spellnum == MGC_CLONE_WIZ)
	    return TRUE;
    } else if (adtyp == AD_CLRC) {
	/* summon insects/sticks to snakes won't be cast by peaceful monsters */
	if (mtmp->mpeaceful && spellnum == CLC_INSECTS)
	    return TRUE;
	/* healing when already healed */
	if (mtmp->mhp == mtmp->mhpmax && spellnum == CLC_CURE_SELF)
	    return TRUE;
	/* don't summon insects if it doesn't think you're around */
	if (!mcouldseeu && spellnum == CLC_INSECTS)
	    return TRUE;
	/* blindness spell on blinded player */
	if (Blinded && spellnum == CLC_BLIND_YOU)
	    return TRUE;
    }
    return FALSE;
}

#endif /* OVLB */
#ifdef OVL0

/* convert 1..10 to 0..9; add 10 for second group (spell casting) */
#define ad_to_typ(k) (10 + (int)k - 1)

int
buzzmu(mtmp, mattk)		/* monster uses spell (ranged) */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	/* don't print constant stream of curse messages for 'normal'
	   spellcasting monsters at range */
	if (mattk->adtyp > AD_SPC2)
	    return(0);

	if (mtmp->mcan || mtmp->mstun || mtmp->mconf) {
	    cursetxt(mtmp, FALSE);
	    return(0);
	}
	if(/*lined_up(mtmp)*/lined_up2(mtmp) && rn2(3)) {
	    nomul(0);
	    if(mattk->adtyp && (mattk->adtyp < 11)) { /* no cf unsigned >0 */
		if(canseemon(mtmp))
		    pline(E_J("%s zaps you with a %s!","%s�͂��Ȃ��߂�����%s���������I"), Monnam(mtmp),
			  flash_types[ad_to_typ(mattk->adtyp)]);
		buzz(-ad_to_typ(mattk->adtyp), (int)mattk->damn,
		     mtmp->mx, mtmp->my, /*sgn*/(tbx), /*sgn*/(tby));
	    } else impossible("Monster spell %d cast", mattk->adtyp-1);
	}
	return(1);
}

#endif /* OVL0 */

/*mcastu.c*/
