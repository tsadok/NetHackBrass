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
			  "%sは呪文を唱えようとしたが、精神を集中することができなかった。"), Monnam(mtmp));
		return;
	    }

	    if (undirected)
		point_msg = E_J("all around, then curses","そこらじゅうを指し、呪いの言葉を吐いた");
	    else if ((Invis && !perceives(mtmp->data) &&
			(mtmp->mux != u.ux || mtmp->muy != u.uy)) ||
		    (youmonst.m_ap_type == M_AP_OBJECT &&
			youmonst.mappearance == STRANGE_OBJECT) ||
		    u.uundetected)
		point_msg = E_J("and curses in your general direction",
				"あなたがいるとおぼしき位置に向かって呪いの言葉を吐いた");
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		point_msg = E_J("and curses at your displaced image",
				"あなたの幻影に向かって呪いの言葉を吐いた");
	    else
		point_msg = E_J("at you, then curses","あなたを指し、呪いの言葉を吐いた");

	    pline(E_J("%s points %s.","%sは%s。"), Monnam(mtmp), point_msg);
	} else if ((!(moves % 4) || !rn2(4))) {
	    if (flags.soundok) Norep(E_J("You hear a mumbled curse.",
					 "何者かがぼそぼそと呪いの言葉を吐くのが聞こえた。"));
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
	    pline("%sは%sに向かって呪文を唱えた！",
		canseemon(mtmp) ? mon_nam(mtmp) : "何者か",
		levl[mtmp->mux][mtmp->muy].typ == WATER
		    ? "何もいない水中" : "虚空");
#endif /*JP*/
	    return(0);
	}

	nomul(0);
	if(rn2(ml*10) < (mtmp->mconf ? 100 : 20)) {	/* fumbled attack */
	    if (canseemon(mtmp) && flags.soundok)
		pline_The(E_J("air crackles around %s.",
			      "空気が%sの周りで乾いた音を立てた。"), mon_nam(mtmp));
	    return(0);
	}
	if (canspotmon(mtmp) || !is_undirected_spell(mattk->adtyp, spellnum)) {
	    pline(E_J("%s casts a spell%s!","%sは%s呪文を唱えた！"),
		  canspotmon(mtmp) ? Monnam(mtmp) : E_J("Something","何者か"),
		  is_undirected_spell(mattk->adtyp, spellnum) ? "" :
		  (Invisible && !perceives(mtmp->data) && 
		   (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  E_J(" at a spot near you","あなたの近くめがけて") :
		  (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  E_J(" at your displaced image","あなたの幻影めがけて") :
		  E_J(" at you","あなためがけて"));
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
			  "あなたは炎に包まれた。"));
		if(Fire_resistance) {
		    if (is_full_resist(FIRE_RES)) {
			shieldeff(u.ux, u.uy);
			pline(E_J("But you resist the effects.",
				  "しかし、あなたは影響を受けない。"));
			dmg = 0;
		    } else {
			dmg = (dmg+3) / 4;
		    }
		}
		burn_away_slime();
		break;
	    case AD_COLD:
		pline(E_J("You're covered in frost.",
			  "あなたは霜に覆われた。"));
		if(Cold_resistance) {
		    if (is_full_resist(COLD_RES)) {
			shieldeff(u.ux, u.uy);
			pline(E_J("But you resist the effects.",
				  "しかし、あなたは影響を受けない。"));
			dmg = 0;
		    } else {
			dmg = (dmg+3) / 4;
		    }
		}
		break;
	    case AD_MAGM:
		You(E_J("are hit by a shower of missiles!",
			"降り注ぐ魔力の散弾に撃たれた！"));
		if(Antimagic) {
			shieldeff(u.ux, u.uy);
			pline_The(E_J("missiles bounce off!",
				      "あなたは弾をはね返した！"));
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
	pline("なんてこった、奴は死の接触を使った！");
#endif /*JP*/
	if (nonliving(youmonst.data) || is_demon(youmonst.data)) {
	    You(E_J("seem no deader than before.",
		    "これ以上死なないようだ。"));
	} else if (!Antimagic && rn2(mtmp->m_lev) > 12) {
	    if (Hallucination) {
		You(E_J("have an out of body experience.","臨死体験をした。"));
	    } else {
		killer_format = E_J(KILLED_BY_AN,KILLED_SUFFIX);
		killer = E_J("touch of death","死の指で");
		done(DIED);
	    }
	} else {
	    if (Antimagic) shieldeff(u.ux, u.uy);
	    pline(E_J("Lucky for you, it didn't work!","幸運なことに、それは失敗した！"));
	    damage_resistant_obj(ANTIMAGIC, rnd(3));
	}
	dmg = 0;
	break;
    case MGC_CLONE_WIZ:
	if (mtmp->iswiz && flags.no_of_wizards == 1) {
	    pline(E_J("Double Trouble...","二重苦だ…。"));
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
	    verbalize("下僕どもよ、盗人を八つ裂きにせよ！");
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
		pline("怪物があなたの近くの位置を囲むように現れた！");
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("怪物があなたの幻影を囲むように現れた！");
	    else
		pline("怪物がどこからともなく現れた！");
#endif /*JP*/
	}
	dmg = 0;
	break;
    }
    case MGC_AGGRAVATION:
	You_feel(E_J("that monsters are aware of your presence.",
		     "怪物があなたの存在を感知したのに気づいた。"));
	aggravate();
	dmg = 0;
	break;
    case MGC_CURSE_ITEMS:
	You_feel(E_J("as if you need some help.","誰かの助けが必要な気がした。"));
	rndcurse();
	dmg = 0;
	break;
    case MGC_DESTRY_ARMR:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    pline(E_J("A field of force surrounds you!","力場があなたを取り囲んだ！"));
	    damage_resistant_obj(ANTIMAGIC, rnd(3));
	} else if (!destroy_arm(some_armor(&youmonst))) {
	    Your(E_J("skin itches.","肌が痒くなった。"));
	}
	dmg = 0;
	break;
    case MGC_WEAKEN_YOU:		/* drain strength */
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    You_feel(E_J("momentarily weakened.","一瞬弱められた。"));
	    damage_resistant_obj(ANTIMAGIC, 1);
	} else {
	    You(E_J("suddenly feel weaker!","突然弱々しくなった！"));
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
		pline(E_J("%s suddenly %s!","%sは突然%s！"), Monnam(mtmp),
		      !See_invisible ? E_J("disappears","消えた") : E_J("becomes transparent","半透明になった"));
	    mon_set_minvis(mtmp);
	    dmg = 0;
	} else
	    impossible("no reason for monster to cast disappear spell?");
	break;
    case MGC_STUN_YOU:
	if (Antimagic || Free_action) {
	    shieldeff(u.ux, u.uy);
	    if (!Stunned) {
		You_feel(E_J("momentarily disoriented.","一瞬方向感覚がなくなった。"));
		if (Antimagic) damage_resistant_obj(ANTIMAGIC, 1);
	    }
	    make_stunned(1L, FALSE);
	} else {
#ifndef JP
	    You(Stunned ? "struggle to keep your balance." : "reel...");
#else
	    You(Stunned ? "姿勢を保つのがさらに困難になった。" : "よろめいた…。");
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
		pline(E_J("%s looks better.","%sは元気になったようだ。"), Monnam(mtmp));
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
	    You(E_J("get a slight %sache.","少し%s痛をおぼえた。"), body_part(HEAD));
	else if (dmg <= 10)
	    Your(E_J("brain is on fire!","脳が焼けるようだ！"));
	else if (dmg <= 20)
	    Your(E_J("%s suddenly aches painfully!",
		     "%sに突然激しい痛みが走った！"), body_part(HEAD));
	else
	    Your(E_J("%s suddenly aches very painfully!",
		     "%sに突然ひどく激しい痛みが走った！"), body_part(HEAD));
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
		  "突然、濁流が噴き出してあなたを直撃した！"));
	dmg = d(8, 6);
	if (Half_physical_damage) dmg = (dmg + 1) / 2;
	break;
    case CLC_FIRE_PILLAR:
	pline(E_J("A pillar of fire strikes all around you!",
		  "炎の柱があなたの周囲に燃え盛った！"));
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
		  "一筋の稲妻があなたに打ち下ろされた！"));
	reflects = ureflects(E_J("It bounces off your %s%s.","稲妻はあなたの%s%sにはね返された。"), "");
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
	You_feel(E_J("as if you need some help.","誰かの助けが必要な気がした。"));
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
		      "%sは棒の束に呪文をかけたが、何も起こらなかった。"),
		Monnam(mtmp));
	else if (let == S_SNAKE)
	    pline(E_J("%s transforms a clump of sticks into snakes!",
		      "%sは棒の束を蛇へと変えた！"),
		Monnam(mtmp));
	else if (Invisible && !perceives(mtmp->data) &&
				(mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline(E_J("%s summons insects around a spot near you!",
		      "%sはあなたの近くの位置めがけて虫を召喚した！"),
		Monnam(mtmp));
	else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline(E_J("%s summons insects around your displaced image!",
		      "%sはあなたの幻影めがけて虫を召喚した！"),
		Monnam(mtmp));
	else
	    pline(E_J("%s summons insects!","%sは虫を召喚した！"), Monnam(mtmp));
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
	    pline("鱗があなたの%sを覆った！", body_part(EYE));
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
		You(E_J("stiffen briefly.","一瞬動けなくなった。"));
		if (Antimagic) damage_resistant_obj(ANTIMAGIC, 1);
	    }
	    nomul(-1);
	} else {
	    if (multi >= 0)
		You(E_J("are frozen in place!","その場で動けなくなった！"));
	    dmg = 4 + (int)mtmp->m_lev;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    nomul(-dmg);
	}
	dmg = 0;
	break;
    case CLC_CONFUSE_YOU:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    You_feel(E_J("momentarily dizzy.","一瞬混乱した。"));
	    damage_resistant_obj(ANTIMAGIC, 1);
	} else {
	    boolean oldprop = !!Confusion;

	    dmg = (int)mtmp->m_lev;
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    make_confused(HConfusion + dmg, TRUE);
	    if (Hallucination)
		You_feel(E_J("%s!","%sトリップした！"), oldprop ? E_J("trippier","ますます") : E_J("trippy",""));
	    else
		You_feel(E_J("%sconfused!","%s混乱した！"), oldprop ? E_J("more ","さらに") : "");
	}
	dmg = 0;
	break;
    case CLC_CURE_SELF:
	if (mtmp->mhp < mtmp->mhpmax) {
	    if (canseemon(mtmp))
		pline(E_J("%s looks better.","%sは元気になったようだ。"), Monnam(mtmp));
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
	    Your(E_J("skin itches badly for a moment.","肌が一瞬ひどく痛んだ。"));
	else if (dmg <= 10)
	    pline(E_J("Wounds appear on your body!","あなたの身体に傷が現れた！"));
	else if (dmg <= 20)
	    pline(E_J("Severe wounds appear on your body!","あなたの身体にひどい傷が現れた！"));
	else
	    Your(E_J("body is covered with painful wounds!","身体が痛々しい重傷で覆われた！"));
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
		    pline(E_J("%s zaps you with a %s!","%sはあなためがけて%sを撃った！"), Monnam(mtmp),
			  flash_types[ad_to_typ(mattk->adtyp)]);
		buzz(-ad_to_typ(mattk->adtyp), (int)mattk->damn,
		     mtmp->mx, mtmp->my, /*sgn*/(tbx), /*sgn*/(tby));
	    } else impossible("Monster spell %d cast", mattk->adtyp-1);
	}
	return(1);
}

#endif /* OVL0 */

/*mcastu.c*/
