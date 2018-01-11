/*	SCCS Id: @(#)explode.c	3.4	2002/11/10	*/
/*	Copyright (C) 1990 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef OVL0

/* Note: Arrays are column first, while the screen is row first */
static int expl[3][3] = {
	{ S_explode1, S_explode4, S_explode7 },
	{ S_explode2, S_explode5, S_explode8 },
	{ S_explode3, S_explode6, S_explode9 }
};

/* Note: I had to choose one of three possible kinds of "type" when writing
 * this function: a wand type (like in zap.c), an adtyp, or an object type.
 * Wand types get complex because they must be converted to adtyps for
 * determining such things as fire resistance.  Adtyps get complex in that
 * they don't supply enough information--was it a player or a monster that
 * did it, and with a wand, spell, or breath weapon?  Object types share both
 * these disadvantages....
 */
void
explode(x, y, type, dam, olet, expltype)
int x, y;
int type; /* the same as in zap.c */
int dam;
char olet;
int expltype;
{
	int i, j, k, damu = dam;
	boolean starting = 1;
	boolean visible, any_shield;
	int uhurt = 0; /* 0=unhurt, 1=items damaged, 2=you and items damaged */
	const char *str;
	int idamres, idamnonres;
	struct monst *mtmp;
	uchar adtyp;
	int explmask[3][3];
#define EXPL_YOURESIST	0x001
#define EXPL_STEEDHERE	0x002
#define EXPL_STEEDRES	0x004
#define EXPL_MONHERE	0x008
#define EXPL_MONRESIST	0x010
#define EXPL_MSTEEDHERE	0x020
#define EXPL_MSTEEDRES	0x040
#define EXPL_SHIELDEFF	0x080
#define EXPL_NONE	0x100
	int res, monres, monhere;
	boolean shopdamage = FALSE;
	boolean generic = FALSE;

	if (olet == WAND_CLASS)		/* retributive strike */
		switch (Role_switch) {
			case PM_PRIEST:
			case PM_MONK:
			case PM_WIZARD: damu /= 5;
				  break;
			case PM_HEALER:
			case PM_KNIGHT: damu /= 2;
				  break;
			default:  break;
		}

	if (olet == MON_EXPLODE) {
	    str = killer;
	    killer = 0;		/* set again later as needed */
	    adtyp = AD_PHYS;
	} else
	switch (abs(type) % 10) {
		case 0: str = E_J("magical blast","魔法の爆発");
			adtyp = AD_MAGM;
			break;
		case 1: str =   olet == BURNING_OIL ?	E_J("burning oil","燃える油") :
				olet == SCROLL_CLASS ?	E_J("tower of flame","炎の柱") :
							E_J("fireball","ファイアボール");
			adtyp = AD_FIRE;
			break;
		case 2: str = E_J("ball of cold","冷気の塊");
			adtyp = AD_COLD;
			break;
		case 4: str =  (olet == WAND_CLASS) ? E_J("death field","死の空間") :
							E_J("disintegration field","分解空間");
			adtyp = AD_DISN;
			break;
		case 5: str = E_J("ball of lightning","電撃の球体");
			adtyp = AD_ELEC;
			break;
		case 6: str = E_J("poison gas cloud","毒ガスの雲");
			adtyp = AD_DRST;
			break;
		case 7: str = E_J("splash of acid","酸の飛沫");
			adtyp = AD_ACID;
			break;
		default: impossible("explosion base type %d?", type); return;
	}

	any_shield = visible = FALSE;
	for (i=0; i<3; i++) for (j=0; j<3; j++) {
		if (!isok(i+x-1, j+y-1)) {
			explmask[i][j] |= EXPL_NONE;
			continue;
		} else
			explmask[i][j] = 0;

		if (i+x-1 == u.ux && j+y-1 == u.uy) {
		    switch(adtyp) {
			case AD_PHYS:
				res = 0;
				break;
			case AD_MAGM:
				res = Antimagic;
				break;
			case AD_FIRE:
				res = is_full_resist(FIRE_RES);
				break;
			case AD_COLD:
				res = is_full_resist(COLD_RES);
				break;
			case AD_DISN:
				res = (olet == WAND_CLASS) ?
					(nonliving(youmonst.data) || is_demon(youmonst.data)) :
					Disint_resistance;
				break;
			case AD_ELEC:
				res = is_full_resist(SHOCK_RES);
				break;
			case AD_DRST:
				res = Poison_resistance;
				break;
			case AD_ACID:
				res = Acid_resistance;
				break;
			default:
				impossible("explosion type %d?", adtyp);
				break;
		    }
		    if (res) explmask[i][j] |= (EXPL_YOURESIST | EXPL_SHIELDEFF);
		}
		/* can be both you and mtmp if you're swallowed */
		mtmp = m_at(i+x-1, j+y-1);

		if (mtmp && cansee(i+x-1,j+y-1) && !canspotmons(mtmp))
		    map_invisible(i+x-1, j+y-1);
		else if (!mtmp && glyph_is_invisible(levl[i+x-1][j+y-1].glyph)) {
		    unmap_object(i+x-1, j+y-1);
		    newsym(i+x-1, j+y-1);
		}

		monres = EXPL_MONRESIST;
		monhere = EXPL_MONHERE;
#ifdef STEED
		if (!mtmp && u.usteed && i+x-1 == u.ux && j+y-1 == u.uy) {
		    monres = EXPL_STEEDRES;
		    monhere = EXPL_STEEDHERE;
		    mtmp = u.usteed;
		}
#endif
		while (mtmp) {
		    res = 0;
		    if (mtmp->mhp < 1) explmask[i][j] |= EXPL_NONE;
		    else {
			switch(adtyp) {
			    case AD_PHYS:
				break;
			    case AD_MAGM:
				res = resists_magm(mtmp);
				break;
			    case AD_FIRE:
				res = resists_fire(mtmp);
				break;
			    case AD_COLD:
				res = resists_cold(mtmp);
				break;
			    case AD_DISN:
				res = (olet == WAND_CLASS) ?
					(nonliving(mtmp->data) || is_demon(mtmp->data)) :
					resists_disint(mtmp);
				break;
			    case AD_ELEC:
				res = resists_elec(mtmp);
				break;
			    case AD_DRST:
				res = resists_poison(mtmp);
				break;
			    case AD_ACID:
				res = resists_acid(mtmp);
				break;
			    default:
				impossible("explosion type %d?", adtyp);
				break;
			}
			explmask[i][j] |= monhere;
			if (res) explmask[i][j] |= (monres|EXPL_SHIELDEFF);
		    }
#ifdef MONSTEED
		    if (is_mriding(mtmp)) {
			monres = EXPL_MSTEEDRES;
			monhere = EXPL_MSTEEDHERE;
			mtmp = mtmp->mlink;
			continue;
		    }
#endif
		    break;
		}
		if (cansee(i+x-1, j+y-1)) visible = TRUE;
		if (explmask[i][j] & EXPL_SHIELDEFF) any_shield = TRUE;
	}

	if (visible) {
		/* Start the explosion */
		for (i=0; i<3; i++) for (j=0; j<3; j++) {
			if (explmask[i][j] & EXPL_NONE) continue;
			tmp_at(starting ? DISP_BEAM : DISP_CHANGE,
				explosion_to_glyph(expltype,expl[i][j]));
			tmp_at(i+x-1, j+y-1);
			starting = 0;
		}
		curs_on_u();	/* will flush screen and output */

		if (any_shield && flags.sparkle) { /* simulate shield effect */
		    for (k = 0; k < SHIELD_COUNT; k++) {
			for (i=0; i<3; i++) for (j=0; j<3; j++) {
			    if (explmask[i][j] & EXPL_SHIELDEFF)
				/*
				 * Bypass tmp_at() and send the shield glyphs
				 * directly to the buffered screen.  tmp_at()
				 * will clean up the location for us later.
				 */
				show_glyph(i+x-1, j+y-1,
					cmap_to_glyph(shield_static[k]));
			}
			curs_on_u();	/* will flush screen and output */
			delay_output();
		    }

		    /* Cover last shield glyph with blast symbol. */
		    for (i=0; i<3; i++) for (j=0; j<3; j++) {
			if (explmask[i][j] & EXPL_SHIELDEFF)
			    show_glyph(i+x-1,j+y-1,
					explosion_to_glyph(expltype, expl[i][j]));
		    }

		} else {		/* delay a little bit. */
		    delay_output();
		    delay_output();
		}

		tmp_at(DISP_END, 0); /* clear the explosion */
	} else {
	    if (olet == MON_EXPLODE) {
		str = E_J("explosion","爆風");
		generic = TRUE;
	    }
	    if (flags.soundok) You_hear(E_J("a blast.","爆発音を"));
	}

    if (dam)
	for (i=0; i<3; i++) for (j=0; j<3; j++) {
		if (explmask[i][j] & EXPL_NONE) continue;
		if (i+x-1 == u.ux && j+y-1 == u.uy)
			uhurt = (explmask[i][j] & EXPL_YOURESIST) ? 1 : 2;
		idamres = idamnonres = 0;
		if (type >= 0)
		    (void)zap_over_floor((xchar)(i+x-1), (xchar)(j+y-1),
		    		type, &shopdamage);

		monhere = explmask[i][j] & (EXPL_MONHERE|EXPL_STEEDHERE|EXPL_MSTEEDHERE);
		while (monhere) {
		    if (monhere & EXPL_MONHERE) {
			monhere &= ~EXPL_MONHERE;
			mtmp = m_at(i+x-1, j+y-1);
			monres = explmask[i][j] & EXPL_MONRESIST;
#ifdef MONSTEED
		    } else if (monhere & EXPL_MSTEEDHERE) {
			monhere &= ~EXPL_MSTEEDHERE;
			mtmp = m_at(i+x-1, j+y-1);
			if (mtmp && is_mriding(mtmp)) mtmp = mtmp->mlink;
			monres = explmask[i][j] & EXPL_MSTEEDRES;
#endif
#ifdef STEED
		    } else if (monhere & EXPL_STEEDHERE) {
			monhere &= ~EXPL_STEEDHERE;
			mtmp = u.usteed;
			monres = explmask[i][j] & EXPL_STEEDRES;
#endif
		    } else break; /* NotReached */
		    if (!mtmp) continue; /* may be killed? */
		    if (u.uswallow && mtmp == u.ustuck) {
			if (is_animal(u.ustuck->data))
				pline(E_J("%s gets %s!","%sは%s！"),
				      Monnam(u.ustuck),
				      (adtyp == AD_FIRE) ? E_J("heartburn","胸焼けをおこした") :
				      (adtyp == AD_COLD) ? E_J("chilly","体を冷やした！") :
				      (adtyp == AD_DISN) ? ((olet == WAND_CLASS) ?
				       E_J("irradiated by pure energy","純粋な死のエネルギーに曝された") :
				       E_J("perforated","胃に穴が開いた")) :
				      (adtyp == AD_ELEC) ? E_J("shocked","感電した") :
				      (adtyp == AD_DRST) ? E_J("poisoned","毒に冒された") :
				      (adtyp == AD_ACID) ? E_J("an upset stomach","胃がムカムカした") :
				       E_J("fried","焼かれた"));
			else
				pline(E_J("%s gets slightly %s!","%sは%s！"),
				      Monnam(u.ustuck),
				      (adtyp == AD_FIRE) ? E_J("toasted","熱せられた") :
				      (adtyp == AD_COLD) ? E_J("chilly","冷やされた") :
				      (adtyp == AD_DISN) ? ((olet == WAND_CLASS) ?
				       E_J("overwhelmed by pure energy","純粋な死のエネルギーに覆われた") :
				       E_J("perforated","穴が開いた")) :
				      (adtyp == AD_ELEC) ? E_J("shocked","電撃を受けた") :
				      (adtyp == AD_DRST) ? E_J("intoxicated","毒物を混入させられた") :
				      (adtyp == AD_ACID) ? E_J("burned","灼かれた") :
				       E_J("fried","燃やされた"));
		    } else if (cansee(i+x-1, j+y-1)) {
			if(mtmp->m_ap_type) seemimic(mtmp);
			pline(E_J("%s is caught in the %s!",
				  "%sは%sに包まれた！"), Monnam(mtmp), str);
		    }

		    idamres += destroy_mitem(mtmp, SCROLL_CLASS, (int) adtyp);
		    idamres += destroy_mitem(mtmp, SPBOOK_CLASS, (int) adtyp);
		    idamnonres += destroy_mitem(mtmp, POTION_CLASS, (int) adtyp);
		    idamnonres += destroy_mitem(mtmp, WAND_CLASS, (int) adtyp);
		    idamnonres += destroy_mitem(mtmp, RING_CLASS, (int) adtyp);

		    if (monres) {
			golemeffects(mtmp, (int) adtyp, dam + idamres);
			mtmp->mhp -= idamnonres;
		    } else {
		    /* call resist with 0 and do damage manually so 1) we can
		     * get out the message before doing the damage, and 2) we can
		     * call mondied, not killed, if it's not your blast
		     */
			int mdam = dam;

			if (resist(mtmp, olet, 0, FALSE)) {
			    if (cansee(i+x-1,j+y-1))
				pline(E_J("%s resists the %s!",
					  "%sは%sに耐えている！"), Monnam(mtmp), str);
			    mdam = dam/2;
			}
			if (mtmp == u.ustuck)
				mdam *= 2;
			if (resists_cold(mtmp) && adtyp == AD_FIRE)
				mdam *= 2;
			else if (resists_fire(mtmp) && adtyp == AD_COLD)
				mdam *= 2;
			mtmp->mhp -= mdam;
			mtmp->mhp -= (idamres + idamnonres);
		    }
		    if (mtmp->mhp <= 0) {
			/* KMH -- Don't blame the player for pets killing gas spores */
			if (!flags.mon_moving) killed(mtmp);
			else monkilled(mtmp, "", (int)adtyp);
		    } else if (!flags.mon_moving) setmangry(mtmp);
		}
	}

	/* Do your injury last */
	if (uhurt) {
		if ((type >= 0 || adtyp == AD_PHYS) &&	/* gas spores */
				flags.verbose && olet != SCROLL_CLASS)
			You(E_J("are caught in the %s!",
				"%sに包まれた！"), str);
		/* do property damage first, in case we end up leaving bones */
		if (adtyp == AD_FIRE) burn_away_slime();
		if (Invulnerable) {
		    damu = 0;
		    You(E_J("are unharmed!","傷つかない！"));
		} else {
		    if (uhurt == 2) {
			boolean resist = 0;
			char *mes;
			switch (adtyp) {
			    case AD_FIRE:
				resist |= !!Fire_resistance;
				mes = E_J("burning blast","燃えさかる炎");
				break;
			    case AD_COLD:
				resist |= !!Cold_resistance;
				mes = E_J("freezing blast","凍気の嵐");
				break;
			    case AD_ELEC:
				resist |= !!Shock_resistance;
				mes = E_J("vast electric shock","強烈な電撃");
				break;
			}
			if (resist) {
			    You(E_J("endure the %s!","%sに耐えている！"), mes);
			    damu = (damu+3) / 4;
			}
		    }
		    if (Half_physical_damage && adtyp == AD_PHYS)
			damu = (damu+1) / 2;
		}
		if (adtyp == AD_FIRE) (void) burnarmor(&youmonst);
		destroy_item(SCROLL_CLASS, (int) adtyp);
		destroy_item(SPBOOK_CLASS, (int) adtyp);
		destroy_item(POTION_CLASS, (int) adtyp);
		destroy_item(RING_CLASS, (int) adtyp);
		destroy_item(WAND_CLASS, (int) adtyp);
		destroy_item(TOOL_CLASS, (int) adtyp);

		ugolemeffects((int) adtyp, damu);
		if (uhurt == 2) {
		    if (Upolyd)
		    	u.mh  -= damu;
		    else
			u.uhp -= damu;
#ifdef SHOWDMG
		    if (flags.showdmg)  pline("(%dpt%s)", damu, (damu == 1 ? "" : "s"));
#endif /*SHOWDMG*/
		    flags.botl = 1;
		}

		if (u.uhp <= 0 || (Upolyd && u.mh <= 0)) {
		    if (Upolyd) {
			rehumanize();
		    } else {
			if (olet == MON_EXPLODE) {
			    /* killer handled by caller */
			    if (str != killer_buf && !generic)
				Strcpy(killer_buf, str);
#ifdef JP
			    Strcat(killer_buf, "で");
#endif /*JP*/
			    killer_format = KILLED_BY_AN;
			} else if (type >= 0 && olet != SCROLL_CLASS) {
			    killer_format = NO_KILLER_PREFIX;
#ifndef JP
			    Sprintf(killer_buf, "caught %sself in %s own %s",
				    uhim(), uhis(), str);
#else
			    Sprintf(killer_buf, "自分で自分を%sに巻き込んで", str);
#endif /*JP*/
#ifndef JP
			} else if (!strncmpi(str,"tower of flame", 8) ||
				   !strncmpi(str,"fireball", 8)) {
			    killer_format = KILLED_BY_AN;
			    Strcpy(killer_buf, str);
#endif /*JP*/
			} else {
			    killer_format = KILLED_BY;
#ifndef JP
			    Strcpy(killer_buf, str);
#else
			    Sprintf(killer_buf, "%sに巻き込まれて", str);
#endif /*JP*/
			}
			killer = killer_buf;
			/* Known BUG: BURNING suppresses corpse in bones data,
			   but done does not handle killer reason correctly */
			done((adtyp == AD_FIRE) ? BURNING : DIED);
		    }
		}
		exercise(A_STR, FALSE);
	}

	if (shopdamage) {
		pay_for_damage(adtyp == AD_FIRE ? E_J("burn away","扉を燃やした") :
			       adtyp == AD_COLD ? E_J("shatter","扉を砕いた") :
			       adtyp == AD_DISN ? E_J("disintegrate","扉を分解した") :
						  E_J("destroy","扉を壊した"),
			       FALSE);
	}

	/* explosions are noisy */
	i = dam * dam;
	if (i < 50) i = 50;	/* in case random damage is very small */
	wake_nearto(x, y, i);
}
#endif /* OVL0 */
#ifdef OVL1

struct scatter_chain {
	struct scatter_chain *next;	/* pointer to next scatter item	*/
	struct obj *obj;		/* pointer to the object	*/
	xchar ox;			/* location of			*/
	xchar oy;			/*	item			*/
	schar dx;			/* direction of			*/
	schar dy;			/*	travel			*/
	int range;			/* range of object		*/
	boolean stopped;		/* flag for in-motion/stopped	*/
};

/*
 * scflags:
 *	VIS_EFFECTS	Add visual effects to display
 *	MAY_HITMON	Objects may hit monsters
 *	MAY_HITYOU	Objects may hit hero
 *	MAY_HIT		Objects may hit you or monsters
 *	MAY_DESTROY	Objects may be destroyed at random
 *	MAY_FRACTURE	Stone objects can be fractured (statues, boulders)
 */

/* returns number of scattered objects */
long
scatter(sx,sy,blastforce,scflags, obj)
int sx,sy;				/* location of objects to scatter */
int blastforce;				/* force behind the scattering	*/
unsigned int scflags;
struct obj *obj;			/* only scatter this obj        */
{
	register struct obj *otmp;
	register int tmp;
	int farthest = 0;
	uchar typ;
	long qtmp;
	boolean used_up;
	boolean individual_object = obj ? TRUE : FALSE;
	struct monst *mtmp;
	struct scatter_chain *stmp, *stmp2 = 0;
	struct scatter_chain *schain = (struct scatter_chain *)0;
	long total = 0L;

	while ((otmp = individual_object ? obj : level.objects[sx][sy]) != 0) {
	    if (otmp->quan > 1L) {
		qtmp = otmp->quan - 1;
		if (qtmp > LARGEST_INT) qtmp = LARGEST_INT;
		qtmp = (long)rnd((int)qtmp);
		otmp = splitobj(otmp, qtmp);
	    } else {
		obj = (struct obj *)0; /* all used */
	    }
	    obj_extract_self(otmp);
	    used_up = FALSE;

	    /* 9 in 10 chance of fracturing boulders or statues */
	    if ((scflags & MAY_FRACTURE)
			&& ((otmp->otyp == BOULDER) || (otmp->otyp == STATUE))
			&& rn2(10)) {
		if (otmp->otyp == BOULDER) {
#ifndef JP
		    pline("%s apart.", Tobjnam(otmp, "break"));
#else
		    pline("%sは粉々に砕けた。", xname(otmp));
#endif /*JP*/
		    fracture_rock(otmp);
		    place_object(otmp, sx, sy);	/* put fragments on floor */
		    if ((otmp = sobj_at(BOULDER, sx, sy)) != 0) {
			/* another boulder here, restack it to the top */
			obj_extract_self(otmp);
			place_object(otmp, sx, sy);
		    }
		} else {
		    struct trap *trap;

		    if ((trap = t_at(sx,sy)) && trap->ttyp == STATUE_TRAP)
			    deltrap(trap);
#ifndef JP
		    pline("%s.", Tobjnam(otmp, "crumble"));
#else
		    pline("%sは砕けた。", xname(otmp));
#endif /*JP*/
		    (void) break_statue(otmp);
		    place_object(otmp, sx, sy);	/* put fragments on floor */
		}
		used_up = TRUE;

	    /* 1 in 10 chance of destruction of obj; glass, egg destruction */
	    } else if ((scflags & MAY_DESTROY) && (!rn2(10)
			|| (get_material(otmp) == GLASS
			|| otmp->otyp == EGG))) {
		if (breaks(otmp, (xchar)sx, (xchar)sy)) used_up = TRUE;
	    }

	    if (!used_up) {
		stmp = (struct scatter_chain *)
					alloc(sizeof(struct scatter_chain));
		stmp->next = (struct scatter_chain *)0;
		stmp->obj = otmp;
		stmp->ox = sx;
		stmp->oy = sy;
		tmp = rn2(8);		/* get the direction */
		stmp->dx = xdir[tmp];
		stmp->dy = ydir[tmp];
		tmp = blastforce - (otmp->owt/40);
		if (tmp < 1) tmp = 1;
		stmp->range = rnd(tmp); /* anywhere up to that determ. by wt */
		if (farthest < stmp->range) farthest = stmp->range;
		stmp->stopped = FALSE;
		if (!schain)
		    schain = stmp;
		else
		    stmp2->next = stmp;
		stmp2 = stmp;
	    }
	}

	while (farthest-- > 0) {
		for (stmp = schain; stmp; stmp = stmp->next) {
		   if ((stmp->range-- > 0) && (!stmp->stopped)) {
			bhitpos.x = stmp->ox + stmp->dx;
			bhitpos.y = stmp->oy + stmp->dy;
			typ = levl[bhitpos.x][bhitpos.y].typ;
			if(!isok(bhitpos.x, bhitpos.y)) {
				bhitpos.x -= stmp->dx;
				bhitpos.y -= stmp->dy;
				stmp->stopped = TRUE;
			} else if(!ZAP_POS(typ) ||
					closed_door(bhitpos.x, bhitpos.y)) {
				bhitpos.x -= stmp->dx;
				bhitpos.y -= stmp->dy;
				stmp->stopped = TRUE;
			} else if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
				if (scflags & MAY_HITMON) {
				    stmp->range--;
				    if (ohitmon(mtmp, stmp->obj, 1, FALSE)) {
					stmp->obj = (struct obj *)0;
					stmp->stopped = TRUE;
				    }
				}
			} else if (bhitpos.x==u.ux && bhitpos.y==u.uy) {
				if (scflags & MAY_HITYOU) {
				    int hitvalu, hitu;

				    if (multi) nomul(0);
				    hitvalu = 8 + stmp->obj->spe;
				    if (bigmonst(youmonst.data)) hitvalu++;
				    hitu = thitu(hitvalu,
						 dmgval(stmp->obj, &youmonst),
						 stmp->obj, (char *)0);
				    if (hitu) {
					stmp->range -= 3;
					stop_occupation();
				    }
				}
			} else {
				if (scflags & VIS_EFFECTS) {
				    /* tmp_at(bhitpos.x, bhitpos.y); */
				    /* delay_output(); */
				}
			}
			stmp->ox = bhitpos.x;
			stmp->oy = bhitpos.y;
		   }
		}
	}
	for (stmp = schain; stmp; stmp = stmp2) {
		int x,y;

		stmp2 = stmp->next;
		x = stmp->ox; y = stmp->oy;
		if (stmp->obj) {
			if ( x!=sx || y!=sy )
			    total += stmp->obj->quan;
			place_object(stmp->obj, x, y);
			stackobj(stmp->obj);
		}
		free((genericptr_t)stmp);
		newsym(x,y);
	}

	return total;
}


/*
 * Splatter burning oil from x,y to the surrounding area.
 *
 * This routine should really take a how and direction parameters.
 * The how is how it was caused, e.g. kicked verses thrown.  The
 * direction is which way to spread the flaming oil.  Different
 * "how"s would give different dispersal patterns.  For example,
 * kicking a burning flask will splatter differently from a thrown
 * flask hitting the ground.
 *
 * For now, just perform a "regular" explosion.
 */
void
splatter_burning_oil(x, y)
    int x, y;
{
/* ZT_SPELL(ZT_FIRE) = ZT_SPELL(AD_FIRE-1) = 10+(2-1) = 11 */
#define ZT_SPELL_O_FIRE 11 /* value kludge, see zap.c */
    explode(x, y, ZT_SPELL_O_FIRE, d(4,4), BURNING_OIL, EXPL_FIERY);
}

#endif /* OVL1 */

/*explode.c*/
