/*	SCCS Id: @(#)detect.c	3.4	2003/08/13	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Detection routines, including crystal ball, magic mapping, and search
 * command.
 */

#include "hack.h"
#include "artifact.h"

extern boolean known;	/* from read.c */

STATIC_DCL void FDECL(do_dknown_of, (struct obj *));
STATIC_DCL boolean FDECL(check_map_spot, (int,int,CHAR_P,unsigned));
STATIC_DCL boolean FDECL(clear_stale_map, (CHAR_P,unsigned));
STATIC_DCL void FDECL(sense_trap, (struct trap *,XCHAR_P,XCHAR_P,int));
STATIC_DCL void FDECL(show_map_spot, (int,int));
STATIC_PTR void FDECL(findone,(int,int,genericptr_t));
STATIC_PTR void FDECL(openone,(int,int,genericptr_t));

/* Recursively search obj for an object in class oclass and return 1st found */
struct obj *
o_in(obj, oclass)
struct obj* obj;
char oclass;
{
    register struct obj* otmp;
    struct obj *temp;

    if (obj->oclass == oclass) return obj;

    if (Has_contents(obj)) {
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
	    if (otmp->oclass == oclass) return otmp;
	    else if (Has_contents(otmp) && (temp = o_in(otmp, oclass)))
		return temp;
    }
    return (struct obj *) 0;
}

/* Recursively search obj for an object made of specified material and return 1st found */
struct obj *
o_material(obj, material)
struct obj* obj;
unsigned material;
{
    register struct obj* otmp;
    struct obj *temp;

    if ((unsigned)get_material(obj) == material) return obj;

    if (Has_contents(obj)) {
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
	    if ((unsigned)get_material(otmp) == material) return otmp;
	    else if (Has_contents(otmp) && (temp = o_material(otmp, material)))
		return temp;
    }
    return (struct obj *) 0;
}

STATIC_OVL void
do_dknown_of(obj)
struct obj *obj;
{
    struct obj *otmp;

    obj->dknown = 1;
    if (Has_contents(obj)) {
	for(otmp = obj->cobj; otmp; otmp = otmp->nobj)
	    do_dknown_of(otmp);
    }
}

/* Check whether the location has an outdated object displayed on it. */
STATIC_OVL boolean
check_map_spot(x, y, oclass, material)
int x, y;
register char oclass;
unsigned material;
{
	register int glyph;
	register struct obj *otmp;
	register struct monst *mtmp;

	glyph = glyph_at(x,y);
	if (glyph_is_object(glyph)) {
	    /* there's some object shown here */
	    if (oclass == ALL_CLASSES) {
		return((boolean)( !(level.objects[x][y] ||     /* stale if nothing here */
			    ((mtmp = m_at(x,y)) != 0 &&
				(
#ifndef GOLDOBJ
				 mtmp->mgold ||
#endif
						 mtmp->minvent)))));
	    } else {
		if (material && objects[glyph_to_obj(glyph)].oc_material == material) {
			/* the object shown here is of interest because material matches */
			for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
				if (o_material(otmp, GOLD)) return FALSE;
			/* didn't find it; perhaps a monster is carrying it */
			if ((mtmp = m_at(x,y)) != 0) {
				for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
					if (o_material(otmp, GOLD)) return FALSE;
		        }
			/* detection indicates removal of this object from the map */
			return TRUE;
		}
	        if (oclass && objects[glyph_to_obj(glyph)].oc_class == oclass) {
			/* the object shown here is of interest because its class matches */
			for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
				if (o_in(otmp, oclass)) return FALSE;
			/* didn't find it; perhaps a monster is carrying it */
#ifndef GOLDOBJ
			if ((mtmp = m_at(x,y)) != 0) {
				if (oclass == COIN_CLASS && mtmp->mgold)
					return FALSE;
				else for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
					if (o_in(otmp, oclass)) return FALSE;
		        }
#else
			if ((mtmp = m_at(x,y)) != 0) {
				for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
					if (o_in(otmp, oclass)) return FALSE;
		        }
#endif
			/* detection indicates removal of this object from the map */
			return TRUE;
	        }
	    }
	}
	return FALSE;
}

/*
   When doing detection, remove stale data from the map display (corpses
   rotted away, objects carried away by monsters, etc) so that it won't
   reappear after the detection has completed.  Return true if noticeable
   change occurs.
 */
STATIC_OVL boolean
clear_stale_map(oclass, material)
register char oclass;
unsigned material;
{
	register int zx, zy;
	register boolean change_made = FALSE;

	for (zx = 1; zx < COLNO; zx++)
	    for (zy = 0; zy < ROWNO; zy++)
		if (check_map_spot(zx, zy, oclass,material)) {
		    unmap_object(zx, zy);
		    change_made = TRUE;
		}

	return change_made;
}

/* look for gold, on the floor or in monsters' possession */
int
gold_detect(sobj)
register struct obj *sobj;
{
    register struct obj *obj;
    register struct monst *mtmp;
    int uw = u.uinwater;
    struct obj *temp;
    boolean stale;

    known = stale = clear_stale_map(COIN_CLASS,
				(unsigned)(sobj->blessed ? GOLD : 0));

    /* look for gold carried by monsters (might be in a container) */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
    	if (DEADMONSTER(mtmp)) continue;	/* probably not needed in this case but... */
#ifndef GOLDOBJ
	if (mtmp->mgold || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#else
	if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#endif
	    known = TRUE;
	    goto outgoldmap;	/* skip further searching */
	} else for (obj = mtmp->minvent; obj; obj = obj->nobj)
	    if (sobj->blessed && o_material(obj, GOLD)) {
	    	known = TRUE;
	    	goto outgoldmap;
	    } else if (o_in(obj, COIN_CLASS)) {
		known = TRUE;
		goto outgoldmap;	/* skip further searching */
	    }
    }
    
    /* look for gold objects */
    for (obj = fobj; obj; obj = obj->nobj) {
	if (sobj->blessed && o_material(obj, GOLD)) {
	    known = TRUE;
	    if (obj->ox != u.ux || obj->oy != u.uy) goto outgoldmap;
	} else if (o_in(obj, COIN_CLASS)) {
	    known = TRUE;
	    if (obj->ox != u.ux || obj->oy != u.uy) goto outgoldmap;
	}
    }

    if (!known) {
	/* no gold found on floor or monster's inventory.
	   adjust message if you have gold in your inventory */
	if (sobj) {
		char buf[BUFSZ];
		if (youmonst.data == &mons[PM_GOLD_GOLEM]) {
#ifndef JP
			Sprintf(buf, "You feel like a million %s!",
				currency(2L));
#else
			Sprintf(buf, "あなたは億万長者のような気分になった！");
#endif /*JP*/
		} else if (hidden_gold() ||
#ifndef GOLDOBJ
				u.ugold)
#else
			        money_cnt(invent))
#endif
			Strcpy(buf,
				E_J("You feel worried about your future financial situation.",
				    "あなたは将来の経済状況に不安を覚えた。"));
		else
			Strcpy(buf, E_J("You feel materially poor.",
					"あなたは物質的に恵まれていない気分になった。"));
		strange_feeling(sobj, buf);
        }
	return(1);
    }
    /* only under me - no separate display required */
    if (stale) docrt();
#ifndef JP
    You("notice some gold between your %s.", makeplural(body_part(FOOT)));
#else
    You("自分の%sの間にいくらかの金貨があるのに気づいた。", body_part(FOOT));
#endif /*JP*/
    return(0);

outgoldmap:
    cls();

    u.uinwater = 0;
    /* Discover gold locations. */
    for (obj = fobj; obj; obj = obj->nobj) {
    	if (sobj->blessed && (temp = o_material(obj, GOLD))) {
	    if (temp != obj) {
		temp->ox = obj->ox;
		temp->oy = obj->oy;
	    }
	    map_object(temp,1);
	} else if ((temp = o_in(obj, COIN_CLASS))) {
	    if (temp != obj) {
		temp->ox = obj->ox;
		temp->oy = obj->oy;
	    }
	    map_object(temp,1);
	}
    }
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
    	if (DEADMONSTER(mtmp)) continue;	/* probably overkill here */
#ifndef GOLDOBJ
	if (mtmp->mgold || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#else
	if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#endif
	    struct obj gold;

	    gold.otyp = GOLD_PIECE;
	    gold.ox = mtmp->mx;
	    gold.oy = mtmp->my;
	    gold.madeof = 0;
	    map_object(&gold,1);
	} else for (obj = mtmp->minvent; obj; obj = obj->nobj)
	    if (sobj->blessed && (temp = o_material(obj, GOLD))) {
		temp->ox = mtmp->mx;
		temp->oy = mtmp->my;
		map_object(temp,1);
		break;
	    } else if ((temp = o_in(obj, COIN_CLASS))) {
		temp->ox = mtmp->mx;
		temp->oy = mtmp->my;
		map_object(temp,1);
		break;
	    }
    }
    
    newsym(u.ux,u.uy);
    You_feel(E_J("very greedy, and sense gold!","とても貪欲になり、金の気配を察知した！"));
    exercise(A_WIS, TRUE);
    display_nhwindow(WIN_MAP, TRUE);
    docrt();
    u.uinwater = uw;
    if (Underwater) under_water(2);
    if (u.uburied) under_ground(2);
    return(0);
}

/* returns 1 if nothing was detected		*/
/* returns 0 if something was detected		*/
int
food_detect(sobj)
register struct obj	*sobj;
{
    register struct obj *obj;
    register struct monst *mtmp;
    register int ct = 0, ctu = 0;
    boolean confused = (Confusion || (sobj && sobj->cursed)), stale;
    char oclass = confused ? POTION_CLASS : FOOD_CLASS;
    const char *what = confused ? something : E_J("food","食物");
    int uw = u.uinwater;

    stale = clear_stale_map(oclass, 0);

    for (obj = fobj; obj; obj = obj->nobj)
	if (o_in(obj, oclass)) {
	    if (obj->ox == u.ux && obj->oy == u.uy) ctu++;
	    else ct++;
	}
    for (mtmp = fmon; mtmp && !ct; mtmp = mtmp->nmon) {
	/* no DEADMONSTER(mtmp) check needed since dmons never have inventory */
	for (obj = mtmp->minvent; obj; obj = obj->nobj)
	    if (o_in(obj, oclass)) {
		ct++;
		break;
	    }
    }
    
    if (!ct && !ctu) {
	known = stale && !confused;
	if (stale) {
	    docrt();
	    You(E_J("sense a lack of %s nearby.",
		    "近くに%sがないことを察知した。"), what);
	    if (sobj && sobj->blessed) {
		if (!u.uedibility) Your(E_J("%s starts to tingle.",
					    "%sがうずきはじめた。"), body_part(NOSE));
		u.uedibility = 1;
	    }
	} else if (sobj) {
	    char buf[BUFSZ];
#ifndef JP
	    Sprintf(buf, "Your %s twitches%s.", body_part(NOSE),
			(sobj->blessed && !u.uedibility) ? " then starts to tingle" : "");
#else
	    Sprintf(buf, "あなたの%sがひくつ%sた。", body_part(NOSE),
			(sobj->blessed && !u.uedibility) ? "き、うずきはじめ" : "い");
#endif /*JP*/
	    if (sobj->blessed && !u.uedibility) {
		boolean savebeginner = flags.beginner;	/* prevent non-delivery of */
		flags.beginner = FALSE;			/* 	message            */
		strange_feeling(sobj, buf);
		flags.beginner = savebeginner;
		u.uedibility = 1;
	    } else
		strange_feeling(sobj, buf);
	}
	return !stale;
    } else if (!ct) {
	known = TRUE;
#ifndef JP
	You("%s %s nearby.", sobj ? "smell" : "sense", what);
#else
	You("近くに%sの%s。", what, sobj ? "匂いを嗅いだ" : "存在を感知した");
#endif /*JP*/
	if (sobj && sobj->blessed) {
		if (!u.uedibility) Your(E_J("%s starts to tingle.",
					    "%sがうずきはじめた。"), body_part(NOSE));
		u.uedibility = 1;
	}
    } else {
	struct obj *temp;
	known = TRUE;
	cls();
	u.uinwater = 0;
	for (obj = fobj; obj; obj = obj->nobj)
	    if ((temp = o_in(obj, oclass)) != 0) {
		if (temp != obj) {
		    temp->ox = obj->ox;
		    temp->oy = obj->oy;
		}
		map_object(temp,1);
	    }
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    /* no DEADMONSTER(mtmp) check needed since dmons never have inventory */
	    for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if ((temp = o_in(obj, oclass)) != 0) {
		    temp->ox = mtmp->mx;
		    temp->oy = mtmp->my;
		    map_object(temp,1);
		    break;	/* skip rest of this monster's inventory */
		}
	newsym(u.ux,u.uy);
	if (sobj) {
	    if (sobj->blessed) {
#ifndef JP
	    	Your("%s %s to tingle and you smell %s.", body_part(NOSE),
	    		u.uedibility ? "continues" : "starts", what);
#else
	    	Your("%sはうずき%s、あなたは%sの匂いを嗅いだ。", body_part(NOSE),
	    		u.uedibility ? "続け" : "はじめ", what);
#endif /*JP*/
		u.uedibility = 1;
	    } else
		Your(E_J("%s tingles and you smell %s.",
			 "%sがうずき、あなたは%sの匂いを嗅いだ。"), body_part(NOSE), what);
	}
	else You(E_J("sense %s.","%sを感知した。"), what);
	display_nhwindow(WIN_MAP, TRUE);
	exercise(A_WIS, TRUE);
	docrt();
	u.uinwater = uw;
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
    }
    return(0);
}

/*
 * Used for scrolls, potions, spells, and crystal balls.  Returns:
 *
 *	1 - nothing was detected
 *	0 - something was detected
 */
int
object_detect(detector, class)
struct obj	*detector;	/* object doing the detecting */
int		class;		/* an object class, 0 for all */
{
    register int x, y;
    char stuff[BUFSZ];
    int is_cursed = (detector && detector->cursed);
    int do_dknown = (detector && (detector->oclass == POTION_CLASS ||
				    detector->oclass == SPBOOK_CLASS) &&
			detector->blessed);
    int ct = 0, ctu = 0;
    register struct obj *obj, *otmp = (struct obj *)0;
    register struct monst *mtmp;
    int uw = u.uinwater;
    int sym, boulder = 0;

    if (class < 0 || class >= MAXOCLASSES) {
	impossible("object_detect:  illegal class %d", class);
	class = 0;
    }

    /* Special boulder symbol check - does the class symbol happen
     * to match iflags.bouldersym which is a user-defined?
     * If so, that means we aren't sure what they really wanted to
     * detect. Rather than trump anything, show both possibilities.
     * We can exclude checking the buried obj chain for boulders below.
     */
    sym = class ? def_oc_syms[class] : 0;
    if (sym && iflags.bouldersym && sym == iflags.bouldersym)
    	boulder = ROCK_CLASS;

    if (Hallucination || (Confusion && class == SCROLL_CLASS))
	Strcpy(stuff, something);
    else
    	Strcpy(stuff, class ? oclass_names[class] : E_J("objects","物体"));
    if (boulder && class != ROCK_CLASS) Strcat(stuff, E_J(" and/or large stones","および大岩"));

    if (do_dknown) for(obj = invent; obj; obj = obj->nobj) do_dknown_of(obj);

    for (obj = fobj; obj; obj = obj->nobj) {
	if ((!class && !boulder) || o_in(obj, class) || o_in(obj, boulder)) {
	    if (obj->ox == u.ux && obj->oy == u.uy) ctu++;
	    else ct++;
	}
	if (do_dknown) do_dknown_of(obj);
    }

    for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
	if (!class || o_in(obj, class)) {
	    if (obj->ox == u.ux && obj->oy == u.uy) ctu++;
	    else ct++;
	}
	if (do_dknown) do_dknown_of(obj);
    }

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj) {
	    if ((!class && !boulder) || o_in(obj, class) || o_in(obj, boulder)) ct++;
	    if (do_dknown) do_dknown_of(obj);
	}
	if ((is_cursed && mtmp->m_ap_type == M_AP_OBJECT &&
	    (!class || class == objects[mtmp->mappearance].oc_class)) ||
#ifndef GOLDOBJ
	    (mtmp->mgold && (!class || class == COIN_CLASS))) {
#else
	    (findgold(mtmp->minvent) && (!class || class == COIN_CLASS))) {
#endif
	    ct++;
	    break;
	}
    }

    if (!clear_stale_map(!class ? ALL_CLASSES : class, 0) && !ct) {
	if (!ctu) {
	    if (detector)
		strange_feeling(detector, E_J("You feel a lack of something.",
					      "あなたは何かが足りないような気分になった。"));
	    return 1;
	}

	You(E_J("sense %s nearby.","近くに%sを感知した。"), stuff);
	return 0;
    }

    cls();

    u.uinwater = 0;
/*
 *	Map all buried objects first.
 */
    for (obj = level.buriedobjlist; obj; obj = obj->nobj)
	if (!class || (otmp = o_in(obj, class))) {
	    if (class) {
		if (otmp != obj) {
		    otmp->ox = obj->ox;
		    otmp->oy = obj->oy;
		}
		map_object(otmp, 1);
	    } else
		map_object(obj, 1);
	}
    /*
     * If we are mapping all objects, map only the top object of a pile or
     * the first object in a monster's inventory.  Otherwise, go looking
     * for a matching object class and display the first one encountered
     * at each location.
     *
     * Objects on the floor override buried objects.
     */
    for (x = 1; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
	    for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if ((!class && !boulder) ||
		    (otmp = o_in(obj, class)) || (otmp = o_in(obj, boulder))) {
		    if (class || boulder) {
			if (otmp != obj) {
			    otmp->ox = obj->ox;
			    otmp->oy = obj->oy;
			}
			map_object(otmp, 1);
		    } else
			map_object(obj, 1);
		    break;
		}

    /* Objects in the monster's inventory override floor objects. */
    for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj)
	    if ((!class && !boulder) ||
		 (otmp = o_in(obj, class)) || (otmp = o_in(obj, boulder))) {
		if (!class && !boulder) otmp = obj;
		otmp->ox = mtmp->mx;		/* at monster location */
		otmp->oy = mtmp->my;
		map_object(otmp, 1);
		break;
	    }
	/* Allow a mimic to override the detected objects it is carrying. */
	if (is_cursed && mtmp->m_ap_type == M_AP_OBJECT &&
		(!class || class == objects[mtmp->mappearance].oc_class)) {
	    struct obj temp;

	    temp.otyp = mtmp->mappearance;	/* needed for obj_to_glyph() */
	    temp.ox = mtmp->mx;
	    temp.oy = mtmp->my;
	    temp.corpsenm = PM_TENGU;		/* if mimicing a corpse */
	    map_object(&temp, 1);
#ifndef GOLDOBJ
	} else if (mtmp->mgold && (!class || class == COIN_CLASS)) {
#else
	} else if (findgold(mtmp->minvent) && (!class || class == COIN_CLASS)) {
#endif
	    struct obj gold;

	    gold.otyp = GOLD_PIECE;
	    gold.ox = mtmp->mx;
	    gold.oy = mtmp->my;
	    map_object(&gold, 1);
	}
    }

    newsym(u.ux,u.uy);
#ifndef JP
    You("detect the %s of %s.", ct ? "presence" : "absence", stuff);
#else
    You("%s%s。", stuff, ct ? "の存在を感知した" : "が存在しないことに気づいた");
#endif /*JP*/
    display_nhwindow(WIN_MAP, TRUE);
    /*
     * What are we going to do when the hero does an object detect while blind
     * and the detected object covers a known pool?
     */
    docrt();	/* this will correctly reset vision */

    u.uinwater = uw;
    if (Underwater) under_water(2);
    if (u.uburied) under_ground(2);
    return 0;
}

/*
 * Used by: crystal balls, potions, fountains
 *
 * Returns 1 if nothing was detected.
 * Returns 0 if something was detected.
 */
int
monster_detect(otmp, mclass)
register struct obj *otmp;	/* detecting object (if any) */
int mclass;			/* monster class, 0 for all */
{
    register struct monst *mtmp;
    int mcnt = 0;


    /* Note: This used to just check fmon for a non-zero value
     * but in versions since 3.3.0 fmon can test TRUE due to the
     * presence of dmons, so we have to find at least one
     * with positive hit-points to know for sure.
     */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
    	if (!DEADMONSTER(mtmp)) {
		mcnt++;
		break;
	}

    if (!mcnt) {
	if (otmp)
	    strange_feeling(otmp, Hallucination ?
			E_J("You get the heebie jeebies.","あなたは急にビクっとした。") :
			E_J("You feel threatened.","あなたは脅威を感じた。"));
	return 1;
    } else {
	boolean woken = FALSE;

	cls();
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if (!mclass || mtmp->data->mlet == mclass ||
		(mtmp->data == &mons[PM_LONG_WORM] && mclass == S_WORM_TAIL))
		    if (mtmp->mx > 0) {
		    	if (mclass && def_monsyms[mclass] == ' ')
				show_glyph(mtmp->mx,mtmp->my,
					detected_mon_to_glyph(mtmp));
			else
				show_glyph(mtmp->mx,mtmp->my,mon_to_glyph(mtmp));
			/* don't be stingy - display entire worm */
			if (mtmp->data == &mons[PM_LONG_WORM]) detect_wsegs(mtmp,0);
		    }
	    if (otmp && otmp->cursed &&
		(mtmp->msleeping || !mtmp->mcanmove)) {
		mtmp->msleeping = mtmp->mfrozen = 0;
		mtmp->mcanmove = 1;
		woken = TRUE;
	    }
	}
	display_self();
	You(E_J("sense the presence of monsters.","怪物の存在を感知した。"));
	if (woken)
	    pline(E_J("Monsters sense the presence of you.",
		      "怪物もあなたの存在に気づいた。"));
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
    }
    return 0;
}

STATIC_OVL void
sense_trap(trap, x, y, src_cursed)
struct trap *trap;
xchar x, y;
int src_cursed;
{
    if (Hallucination || src_cursed) {
	struct obj obj;			/* fake object */
	if (trap) {
	    obj.ox = trap->tx;
	    obj.oy = trap->ty;
	} else {
	    obj.ox = x;
	    obj.oy = y;
	}
	obj.otyp = (src_cursed) ? GOLD_PIECE : random_object();
	obj.corpsenm = random_monster();	/* if otyp == CORPSE */
	obj.madeof = 0;
	map_object(&obj,1);
    } else if (trap) {
	map_trap(trap,1);
	trap->tseen = 1;
    } else {
	struct trap temp_trap;		/* fake trap */
	temp_trap.tx = x;
	temp_trap.ty = y;
	temp_trap.ttyp = BEAR_TRAP;	/* some kind of trap */
	map_trap(&temp_trap,1);
    }

}

/* the detections are pulled out so they can	*/
/* also be used in the crystal ball routine	*/
/* returns 1 if nothing was detected		*/
/* returns 0 if something was detected		*/
int
trap_detect(sobj)
register struct obj *sobj;
/* sobj is null if crystal ball, *scroll if gold detection scroll */
{
    register struct trap *ttmp;
    register struct obj *obj;
    register int door;
    int uw = u.uinwater;
    boolean found = FALSE;
    coord cc;

    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
	if (ttmp->tx != u.ux || ttmp->ty != u.uy)
	    goto outtrapmap;
	else found = TRUE;
    }
    for (obj = fobj; obj; obj = obj->nobj) {
	if ((obj->otyp==LARGE_BOX || obj->otyp==CHEST) && obj->otrapped) {
	    if (obj->ox != u.ux || obj->oy != u.uy)
		goto outtrapmap;
	    else found = TRUE;
	}
    }
    for (door = 0; door < doorindex; door++) {
	cc = doors[door];
	if (levl[cc.x][cc.y].doormask & D_TRAPPED) {
	    if (cc.x != u.ux || cc.y != u.uy)
		goto outtrapmap;
	    else found = TRUE;
	}
    }
    if (!found) {
	char buf[42];
#ifndef JP
	Sprintf(buf, "Your %s stop itching.", makeplural(body_part(TOE)));
#else
	Sprintf(buf, "あなたの%sの痒みが止まった。", body_part(TOE));
#endif /*JP*/
	strange_feeling(sobj,buf);
	return(1);
    }
    /* traps exist, but only under me - no separate display required */
#ifndef JP
    Your("%s itch.", makeplural(body_part(TOE)));
#else
    Your("%sが痒くなった。", body_part(TOE));
#endif /*JP*/
    return(0);
outtrapmap:
    cls();

    u.uinwater = 0;
    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
	sense_trap(ttmp, 0, 0, sobj && sobj->cursed);

    for (obj = fobj; obj; obj = obj->nobj)
	if ((obj->otyp==LARGE_BOX || obj->otyp==CHEST) && obj->otrapped)
	sense_trap((struct trap *)0, obj->ox, obj->oy, sobj && sobj->cursed);

    for (door = 0; door < doorindex; door++) {
	cc = doors[door];
	if (levl[cc.x][cc.y].doormask & D_TRAPPED)
	sense_trap((struct trap *)0, cc.x, cc.y, sobj && sobj->cursed);
    }

    newsym(u.ux,u.uy);
#ifndef JP
    You_feel("%s.", sobj && sobj->cursed ? "very greedy" : "entrapped");
#else
    You_feel(sobj && sobj->cursed ? "とても貪欲になった。" : "罠にはめられた気がした。");
#endif /*JP*/
    display_nhwindow(WIN_MAP, TRUE);
    docrt();
    u.uinwater = uw;
    if (Underwater) under_water(2);
    if (u.uburied) under_ground(2);
    return(0);
}

const char *
level_distance(where)
d_level *where;
{
    register schar ll = depth(&u.uz) - depth(where);
    register boolean indun = (u.uz.dnum == where->dnum);

    if (ll < 0) {
	if (ll < (-8 - rn2(3)))
	    if (!indun)	return E_J("far away","はるか彼方");
	    else	return E_J("far below","はるかな深奥");
	else if (ll < -1)
	    if (!indun)	return E_J("away below you","彼方の下方");
	    else	return E_J("below you","あなたの下");
	else
	    if (!indun)	return E_J("in the distance","そう遠くない場所");
	    else	return E_J("just below","ちょうどこの下");
    } else if (ll > 0) {
	if (ll > (8 + rn2(3)))
	    if (!indun)	return E_J("far away","はるか彼方");
	    else	return E_J("far above","はるかな上方");
	else if (ll > 1)
	    if (!indun)	return E_J("away above you","彼方の上方");
	    else	return E_J("above you","あなたの上");
	else
	    if (!indun)	return E_J("in the distance","そう遠くない場所");
	    else	return E_J("just above","ちょうどこの上");
    } else
	    if (!indun)	return E_J("in the distance","そう遠くない場所");
	    else	return E_J("near you","あなたの近く");
}

static const struct {
    const char *what;
    d_level *where;
} level_detects[] = {
  { "Delphi", &oracle_level },
  { "Medusa's lair", &medusa_level },
  { "a castle", &stronghold_level },
  { "the Wizard of Yendor's tower", &wiz1_level },
};

void
use_crystal_ball(obj)
struct obj *obj;
{
    char ch;
    int oops;

    if (Blind) {
#ifndef JP
	pline("Too bad you can't see %s.", the(xname(obj)));
#else
	pline("%sを見られないとは、なんと残念な。", xname(obj));
#endif /*JP*/
	return;
    }
    oops = (!rn2(ACURR(A_INT)*(Role_if(PM_WIZARD) ? 2 : 1))/*rnd(20) > ACURR(A_INT)*/ || obj->cursed);
    if (oops && (obj->spe > 0)) {
	switch (rnd(obj->oartifact ? 4 : 5)) {
#ifndef JP
	case 1 : pline("%s too much to comprehend!", Tobjnam(obj, "are"));
#else
	case 1 : pline("%sを解釈するのは到底無理だ！", xname(obj));
#endif /*JP*/
	    break;
#ifndef JP
	case 2 : pline("%s you!", Tobjnam(obj, "confuse"));
#else
	case 2 : pline("%sはあなたを混乱させた！", xname(obj));
#endif /*JP*/
	    make_confused(HConfusion + rnd(100),FALSE);
	    break;
	case 3 : if (!resists_blnd(&youmonst)) {
#ifndef JP
		pline("%s your vision!", Tobjnam(obj, "damage"));
#else
		pline("%sはあなたの視力を奪った！", xname(obj));
#endif /*JP*/
		make_blinded(Blinded + rnd(100),FALSE);
		if (!Blind) Your(vision_clears);
	    } else {
#ifndef JP
		pline("%s your vision.", Tobjnam(obj, "assault"));
		You("are unaffected!");
#else
		pline("%sがあなたの視界を焼きつかせようとした。", xname(obj));
		You("影響を受けない！");
#endif /*JP*/
	    }
	    break;
#ifndef JP
	case 4 : pline("%s your mind!", Tobjnam(obj, "zap"));
#else
	case 4 : pline("%sがあなたの精神を直撃した！", xname(obj));
#endif /*JP*/
	    (void) make_hallucinated(HHallucination + rnd(100),FALSE,0L);
	    break;
#ifndef JP
	case 5 : pline("%s!", Tobjnam(obj, "explode"));
#else
	case 5 : pline("%sは爆発した！", xname(obj));
#endif /*JP*/
	    useup(obj);
	    obj = 0;	/* it's gone */
	    losehp(rnd(30), E_J("exploding crystal ball",
				"水晶玉の爆発で"), KILLED_BY_AN);
	    break;
	}
	if (obj) consume_obj_charge(obj, TRUE);
	return;
    }

    if (Hallucination) {
	if (!obj->spe) {
	    pline(E_J("All you see is funky %s haze.",
		      "見えるのはファンキーな%s靄ばかりだ。"), hcolor((char *)0));
	} else {
	    switch(rnd(6)) {
	    case 1 : You(E_J("grok some groovy globs of incandescent lava.",
			     "強烈な高熱の溶岩に共感した。"));
		break;
	    case 2 : pline(E_J("Whoa!  Psychedelic colors, %s!",
			       "うひょー！ サイケな色だぜ、%s！"),
			   poly_gender() == 1 ? E_J("babe","ベイビー") : E_J("dude","兄弟"));
		break;
	    case 3 : pline_The(E_J("crystal pulses with sinister %s light!",
				   "水晶玉は邪悪な%s光に明滅した！"),
				hcolor((char *)0));
		break;
	    case 4 : You(E_J("see goldfish swimming above fluorescent rocks.",
			     "蛍光カラーの石の上を金魚が泳いでいるのを見た。"));
		break;
	    case 5 : You(E_J("see tiny snowflakes spinning around a miniature farmhouse.",
			     "ミニチュアの農家のまわりにかわいい雪が舞っているのを見た。"));
		break;
	    default: pline(E_J("Oh wow... like a kaleidoscope!",
			       "おお、凄い…。万華鏡みたいだ！"));
		break;
	    }
	    consume_obj_charge(obj, TRUE);
	}
	return;
    }

    /* read a single character */
    if (flags.verbose) You(E_J("may look for an object or monster symbol.",
			       "物体か怪物のシンボル一つを見ることができる。"));
    ch = yn_function("What do you look for?", (char *)0, '\0');
    /* Don't filter out ' ' here; it has a use */
    if ((ch != def_monsyms[S_GHOST]) && index(quitchars,ch)) { 
	if (flags.verbose) pline(Never_mind);
	return;
    }
#ifndef JP
    You("peer into %s...", the(xname(obj)));
#else
    You("%sをのぞきこんだ…。", xname(obj));
#endif /*JP*/
    nomul(-rnd(10));
    nomovemsg = "";
    if (obj->spe <= 0)
	pline_The(E_J("vision is unclear.","景色は靄がかかったように定まらない。"));
    else {
	int class;
	int ret = 0;

	makeknown(CRYSTAL_BALL);
	consume_obj_charge(obj, TRUE);

	/* special case: accept ']' as synonym for mimic
	 * we have to do this before the def_char_to_objclass check
	 */
	if (ch == DEF_MIMIC_DEF) ch = DEF_MIMIC;

	if ((class = def_char_to_objclass(ch)) != MAXOCLASSES)
		ret = object_detect((struct obj *)0, class);
	else if ((class = def_char_to_monclass(ch)) != MAXMCLASSES)
		ret = monster_detect((struct obj *)0, class);
	else if (iflags.bouldersym && (ch == iflags.bouldersym))
		ret = object_detect((struct obj *)0, ROCK_CLASS);
	else switch(ch) {
		case '^':
		    ret = trap_detect((struct obj *)0);
		    break;
		default:
		    {
		    int i = rn2(SIZE(level_detects));
		    You(E_J("see %s, %s.","%sを%sに見た。"),
			level_detects[i].what,
			level_distance(level_detects[i].where));
		    }
		    ret = 0;
		    break;
	}

	if (ret) {
	    if (!rn2(100))  /* make them nervous */
		You(E_J("see the Wizard of Yendor gazing out at you.",
			"イェンダーの魔法使いがあなたを凝視しているのを見た。"));
	    else pline_The(E_J("vision is unclear.",
			       "景色は靄がかかったように定まらない。"));
	}
    }
    return;
}

STATIC_OVL void
show_map_spot(x, y)
register int x, y;
{
    register struct rm *lev;

    if (Confusion && rn2(7)) return;
    lev = &levl[x][y];

    lev->seenv = SVALL;

    /* Secret corridors are found, but not secret doors. */
    if (lev->typ == SCORR) {
	lev->typ = CORR;
	unblock_point(x,y);
    }

    /* if we don't remember an object or trap there, map it */
    if (lev->typ == ROOM ?
	    (glyph_is_cmap(lev->glyph) && !glyph_is_trap(lev->glyph) &&
		glyph_to_cmap(lev->glyph) != ROOM) :
	    (!glyph_is_object(lev->glyph) && !glyph_is_trap(lev->glyph))) {
	if (level.flags.hero_memory) {
	    magic_map_background(x,y,0);
	    newsym(x,y);			/* show it, if not blocked */
	} else {
	    magic_map_background(x,y,1);	/* display it */
	}
    }
}

void
do_mapping()
{
    register int zx, zy;
    int uw = u.uinwater;

    u.uinwater = 0;
    for (zx = 1; zx < COLNO; zx++)
	for (zy = 0; zy < ROWNO; zy++)
	    show_map_spot(zx, zy);
    exercise(A_WIS, TRUE);
    u.uinwater = uw;
    if (!level.flags.hero_memory || Underwater) {
	flush_screen(1);			/* flush temp screen */
	display_nhwindow(WIN_MAP, TRUE);	/* wait */
	docrt();
    }
}

void
do_vicinity_map()
{
    register int zx, zy;
    int lo_y = (u.uy-5 < 0 ? 0 : u.uy-5),
	hi_y = (u.uy+6 > ROWNO ? ROWNO : u.uy+6),
	lo_x = (u.ux-9 < 1 ? 1 : u.ux-9),	/* avoid column 0 */
	hi_x = (u.ux+10 > COLNO ? COLNO : u.ux+10);

    for (zx = lo_x; zx < hi_x; zx++)
	for (zy = lo_y; zy < hi_y; zy++)
	    show_map_spot(zx, zy);

    if (!level.flags.hero_memory || Underwater) {
	flush_screen(1);			/* flush temp screen */
	display_nhwindow(WIN_MAP, TRUE);	/* wait */
	docrt();
    }
}

/* convert a secret door into a normal door */
void
cvt_sdoor_to_door(lev)
struct rm *lev;
{
	int newmask = lev->doormask & ~WM_MASK;

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz))
	    /* rogue didn't have doors, only doorways */
	    newmask = D_NODOOR;
	else
#endif
	    /* newly exposed door is closed */
	    if (!(newmask & D_LOCKED)) newmask |= D_CLOSED;

	lev->typ = DOOR;
	lev->doormask = newmask;
}


STATIC_PTR void
findone(zx,zy,num)
int zx,zy;
genericptr_t num;
{
	register struct trap *ttmp;
	register struct monst *mtmp;

	if(levl[zx][zy].typ == SDOOR) {
		cvt_sdoor_to_door(&levl[zx][zy]);	/* .typ = DOOR */
		magic_map_background(zx, zy, 0);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if(levl[zx][zy].typ == SCORR) {
		levl[zx][zy].typ = CORR;
		unblock_point(zx,zy);
		magic_map_background(zx, zy, 0);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if ((ttmp = t_at(zx, zy)) != 0) {
		if(!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
			ttmp->tseen = 1;
			newsym(zx,zy);
			(*(int*)num)++;
		}
	} else if ((mtmp = m_at(zx, zy)) != 0) {
		if(mtmp->m_ap_type) {
			seemimic(mtmp);
			(*(int*)num)++;
		}
		if (mtmp->mundetected &&
		    (is_hider(mtmp->data) || mtmp->data->mlet == S_EEL)) {
			mtmp->mundetected = 0;
			newsym(zx, zy);
			(*(int*)num)++;
		}
		if (!canspotmons(mtmp) &&
				    !glyph_is_invisible(levl[zx][zy].glyph))
			map_invisible(zx, zy);
	} else if (glyph_is_invisible(levl[zx][zy].glyph)) {
		unmap_object(zx, zy);
		newsym(zx, zy);
		(*(int*)num)++;
	}
}

STATIC_PTR void
openone(zx,zy,num)
int zx,zy;
genericptr_t num;
{
	register struct trap *ttmp;
	register struct obj *otmp;

	if(OBJ_AT(zx, zy)) {
		for(otmp = level.objects[zx][zy];
				otmp; otmp = otmp->nexthere) {
		    if(Is_box(otmp) && otmp->olocked) {
			otmp->olocked = 0;
			(*(int*)num)++;
		    }
		}
		/* let it fall to the next cases. could be on trap. */
	}
	if(levl[zx][zy].typ == SDOOR || (levl[zx][zy].typ == DOOR &&
		      (levl[zx][zy].doormask & (D_CLOSED|D_LOCKED)))) {
		if(levl[zx][zy].typ == SDOOR)
		    cvt_sdoor_to_door(&levl[zx][zy]);	/* .typ = DOOR */
		if(levl[zx][zy].doormask & D_TRAPPED) {
		    if(distu(zx, zy) < 3) b_trapped(E_J("door","扉"), 0);
		    else Norep(E_J("You %s an explosion!",
				   "あなたは扉が爆発する%s！"),
				cansee(zx, zy) ? E_J("see","のを見た") :
				   (flags.soundok ? E_J("hear","音を聞いた") :
						E_J("feel the shock of","衝撃を感じた")));
		    wake_nearto(zx, zy, 11*11);
		    levl[zx][zy].doormask = D_NODOOR;
		} else
		    levl[zx][zy].doormask = D_ISOPEN;
		unblock_point(zx, zy);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if(levl[zx][zy].typ == SCORR) {
		levl[zx][zy].typ = CORR;
		unblock_point(zx, zy);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if ((ttmp = t_at(zx, zy)) != 0) {
		if (!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
		    ttmp->tseen = 1;
		    newsym(zx,zy);
		    (*(int*)num)++;
		}
	} else if (find_drawbridge(&zx, &zy)) {
		/* make sure it isn't an open drawbridge */
		open_drawbridge(zx, zy);
		(*(int*)num)++;
	}
}

int
findit()	/* returns number of things found */
{
	int num = 0;

	if(u.uswallow) return(0);
	do_clear_area(u.ux, u.uy, BOLT_LIM, findone, (genericptr_t) &num);
	return(num);
}

int
openit()	/* returns number of things found and opened */
{
	int num = 0;

	if(u.uswallow) {
		if (is_animal(u.ustuck->data)) {
			if (Blind) pline(E_J("Its mouth opens!","怪物の口が開いた！"));
			else pline(E_J("%s opens its mouth!",
				       "%sは口を開けた！"), Monnam(u.ustuck));
		}
		expels(u.ustuck, u.ustuck->data, TRUE);
		return(-1);
	}

	do_clear_area(u.ux, u.uy, BOLT_LIM, openone, (genericptr_t) &num);
	return(num);
}

void
find_trap(trap)
struct trap *trap;
{
    int tt = what_trap(trap->ttyp);
    boolean cleared = FALSE;

    trap->tseen = 1;
    exercise(A_WIS, TRUE);
    if (Blind)
	feel_location(trap->tx, trap->ty);
    else
	newsym(trap->tx, trap->ty);

    if (levl[trap->tx][trap->ty].glyph != trap_to_glyph(trap)) {
    	/* There's too much clutter to see your find otherwise */
	cls();
	map_trap(trap, 1);
	display_self();
	cleared = TRUE;
    }

#ifndef JP
    You("find %s.", an(defsyms[trap_to_defsym(tt)].explanation));
#else
    You("%sを見つけた。", defsyms[trap_to_defsym(tt)].explanation);
#endif /*JP*/

    if (cleared) {
	display_nhwindow(WIN_MAP, TRUE);	/* wait */
	docrt();
    }
}

int
dosearch0(aflag)
register int aflag;
{
#ifdef GCC_BUG
/* some versions of gcc seriously muck up nested loops. if you get strange
   crashes while searching in a version compiled with gcc, try putting
   #define GCC_BUG in *conf.h (or adding -DGCC_BUG to CFLAGS in the
   makefile).
 */
	volatile xchar x, y;
#else
	register xchar x, y;
#endif
	register struct trap *trap;
	register struct monst *mtmp;

	if(u.uswallow) {
		if (!aflag)
			pline(E_J("What are you looking for?  The exit?",
				  "何を探しているんだ？ 出口かい？"));
	} else {
	    int fund = (uwep && uwep->oartifact &&
		    spec_ability(uwep, SPFX_SEARCH)) ?
		    uwep->spe : 0;
#ifndef MAGIC_GLASSES
	    if (ublindf && ublindf->otyp == LENSES && !Blind)
		    fund += 2; /* JDS: lenses help searching */
#endif
	    if (fund > 5) fund = 5;
	    for(x = u.ux-1; x < u.ux+2; x++)
	      for(y = u.uy-1; y < u.uy+2; y++) {
		if(!isok(x,y)) continue;
		if(x != u.ux || y != u.uy) {
		    if (Blind && !aflag) feel_location(x,y);
		    if(levl[x][y].typ == SDOOR) {
			if(rnl(7-fund)) continue;
			cvt_sdoor_to_door(&levl[x][y]);	/* .typ = DOOR */
			exercise(A_WIS, TRUE);
			nomul(0);
			if (Blind && !aflag)
			    feel_location(x,y);	/* make sure it shows up */
			else
			    newsym(x,y);
		    } else if(levl[x][y].typ == SCORR) {
			if(rnl(7-fund)) continue;
			levl[x][y].typ = CORR;
			unblock_point(x,y);	/* vision */
			exercise(A_WIS, TRUE);
			nomul(0);
			newsym(x,y);
		    } else {
		/* Be careful not to find anything in an SCORR or SDOOR */
			if((mtmp = m_at(x, y)) && !aflag) {
			    if(mtmp->m_ap_type) {
				seemimic(mtmp);
		find:		exercise(A_WIS, TRUE);
				if (!canspotmons(mtmp)) {
				    if (glyph_is_invisible(levl[x][y].glyph)) {
					/* found invisible monster in a square
					 * which already has an 'I' in it.
					 * Logically, this should still take
					 * time and lead to a return(1), but if
					 * we did that the player would keep
					 * finding the same monster every turn.
					 */
					continue;
				    } else {
					You_feel(E_J("an unseen monster!",
						     "目に見えない怪物に気づいた！"));
					map_invisible(x, y);
				    }
				} else if (!sensemon(mtmp))
				    You(E_J("find %s.","%sを見つけた。"), a_monnam(mtmp));
				return(1);
			    }
			    if(!canspotmon(mtmp)) {
				if (mtmp->mundetected &&
				   (is_hider(mtmp->data) || mtmp->data->mlet == S_EEL))
					mtmp->mundetected = 0;
				newsym(x,y);
				goto find;
			    }
			}

			/* see if an invisible monster has moved--if Blind,
			 * feel_location() already did it
			 */
			if (!aflag && !mtmp && !Blind &&
				    glyph_is_invisible(levl[x][y].glyph)) {
			    unmap_object(x,y);
			    newsym(x,y);
			}

			if ((trap = t_at(x,y)) && !trap->tseen && !rnl(8)) {
			    nomul(0);

			    if (trap->ttyp == STATUE_TRAP) {
				if (activate_statue_trap(trap, x, y, FALSE))
				    exercise(A_WIS, TRUE);
				return(1);
			    } else {
				find_trap(trap);
			    }
			}
		    }
		}
	    }
	}
	return(1);
}

int
dosearch()
{
	return(dosearch0(0));
}

/* Pre-map the sokoban levels */
void
sokoban_detect()
{
	register int x, y;
	register struct trap *ttmp;
	register struct obj *obj;

	/* Map the background and boulders */
	for (x = 1; x < COLNO; x++)
	    for (y = 0; y < ROWNO; y++) {
	    	levl[x][y].seenv = SVALL;
	    	levl[x][y].waslit = TRUE;
	    	map_background(x, y, 1);
	    	for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
	    	    if (obj->otyp == BOULDER)
	    	    	map_object(obj, 1);
	    }

	/* Map the traps */
	for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
	    ttmp->tseen = 1;
	    map_trap(ttmp, 1);
	}
}


/*detect.c*/
