/*	SCCS Id: @(#)mklev.c	3.4	2001/11/29	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "sp_lev.h"
/* #define DEBUG */	/* uncomment to enable code debugging */

#ifdef DEBUG
# ifdef WIZARD
#define debugpline	if (wizard) pline
# else
#define debugpline	pline
# endif
#endif

/* for UNIX, Rand #def'd to (long)lrand48() or (long)random() */
/* croom->lx etc are schar (width <= int), so % arith ensures that */
/* conversion of result to int is reasonable */


STATIC_DCL void FDECL(mkfount,(int,struct mkroom *));
#ifdef SINKS
STATIC_DCL void FDECL(mksink,(struct mkroom *));
#endif
STATIC_DCL void FDECL(mkaltar,(struct mkroom *));
STATIC_DCL void FDECL(mkgrave,(struct mkroom *));
STATIC_DCL void NDECL(makevtele);
STATIC_DCL void NDECL(clear_level_structures);
STATIC_DCL void NDECL(makelevel);
STATIC_DCL void NDECL(mineralize);
STATIC_DCL boolean FDECL(bydoor,(XCHAR_P,XCHAR_P));
STATIC_DCL struct mkroom *FDECL(find_branch_room, (coord *));
STATIC_DCL struct mkroom *FDECL(pos_to_room, (XCHAR_P, XCHAR_P));
STATIC_DCL boolean FDECL(place_niche,(struct mkroom *,int*,int*,int*));
STATIC_DCL void FDECL(makeniche,(int));
STATIC_DCL void NDECL(make_niches);

STATIC_PTR int FDECL( CFDECLSPEC do_comp,(const genericptr,const genericptr));

STATIC_DCL void FDECL(dosdoor,(XCHAR_P,XCHAR_P,struct mkroom *,int));
STATIC_DCL void FDECL(join,(int,int,BOOLEAN_P));
STATIC_DCL void FDECL(do_room_or_subroom, (struct mkroom *,int,int,int,int,
				       BOOLEAN_P,SCHAR_P,BOOLEAN_P,BOOLEAN_P));
STATIC_DCL void NDECL(makerooms);
STATIC_DCL void FDECL(finddpos,(coord *,XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P));
STATIC_DCL void FDECL(mkinvpos, (XCHAR_P,XCHAR_P,int));
STATIC_DCL void FDECL(mk_knox_portal, (XCHAR_P,XCHAR_P));
STATIC_OVL void FDECL(topologize_irregular, (struct mkroom *));

/* ghost town(Orcus) */
STATIC_OVL int FDECL (inmap, (int,int));
STATIC_OVL int FDECL (gtwn_rndloc, (int *,int *));
STATIC_OVL void NDECL (gtwn_initmap);
STATIC_OVL void NDECL (gtwn_1stpath);
STATIC_OVL void NDECL (gtwn_path);
STATIC_OVL void FDECL (gtwn_findmaxrect, (int *,int *,int *,int *,int,int));
STATIC_OVL void FDECL (gtwn_drawroom, (int,int,int,int));
STATIC_OVL int  NDECL (gtwn_findcorner);
STATIC_OVL void NDECL (gtwn_adddoor);
STATIC_OVL void NDECL (gtwn_cleanup);

/* asmodeus */
STATIC_OVL void NDECL (mkasmolev);

/* baalzebub */

/* juiblex */
STATIC_OVL void NDECL (mkjuiblev);

STATIC_OVL boolean FDECL (is_thinwall, (int,int));
STATIC_OVL void FDECL (refine_mapedge, (SCHAR_P));

extern void FDECL(mkmap, (lev_init *));

STATIC_OVL boolean NDECL (selloc_init);
STATIC_OVL void NDECL (selloc_tini);
STATIC_OVL boolean FDECL (selloc_store, (int, int));
STATIC_OVL boolean FDECL (selloc_pickrndloc, (int *, int *));

#define create_vault()	create_room(-1, -1, 2, 2, -1, -1, VAULT, TRUE)
#define init_vault()	vault_x = -1
#define do_vault()	(vault_x != -1)
static xchar		vault_x, vault_y;
boolean goldseen;
static boolean made_branch;	/* used only during level creation */

/* Args must be (const genericptr) so that qsort will always be happy. */

STATIC_PTR int CFDECLSPEC
do_comp(vx,vy)
const genericptr vx;
const genericptr vy;
{
#ifdef LINT
/* lint complains about possible pointer alignment problems, but we know
   that vx and vy are always properly aligned. Hence, the following
   bogus definition:
*/
	return (vx == vy) ? 0 : -1;
#else
	register const struct mkroom *x, *y;

	x = (const struct mkroom *)vx;
	y = (const struct mkroom *)vy;
	if(x->lx < y->lx) return(-1);
	return(x->lx > y->lx);
#endif /* LINT */
}

STATIC_OVL void
finddpos(cc, xl,yl,xh,yh)
coord *cc;
xchar xl,yl,xh,yh;
{
	register xchar x, y;

	x = (xl == xh) ? xl : (xl + rn2(xh-xl+1));
	y = (yl == yh) ? yl : (yl + rn2(yh-yl+1));
	if(okdoor(x, y))
		goto gotit;

	for(x = xl; x <= xh; x++) for(y = yl; y <= yh; y++)
		if(okdoor(x, y))
			goto gotit;

	for(x = xl; x <= xh; x++) for(y = yl; y <= yh; y++)
		if(IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR)
			goto gotit;
	/* cannot find something reasonable -- strange */
	x = xl;
	y = yh;
gotit:
	cc->x = x;
	cc->y = y;
	return;
}

void
sort_rooms()
{
#if defined(SYSV) || defined(DGUX)
	qsort((genericptr_t) rooms, (unsigned)nroom, sizeof(struct mkroom), do_comp);
#else
	qsort((genericptr_t) rooms, nroom, sizeof(struct mkroom), do_comp);
#endif
	/* restore the link between rooms and doors */
	sort_doors();
}

STATIC_OVL void
do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special, is_room)
    register struct mkroom *croom;
    int lowx, lowy;
    register int hix, hiy;
    boolean lit;
    schar rtype;
    boolean special;
    boolean is_room;
{
	register int x, y;
	struct rm *lev;

	/* locations might bump level edges in wall-less rooms */
	/* add/subtract 1 to allow for edge locations */
	if(!lowx) lowx++;
	if(!lowy) lowy++;
	if(hix >= COLNO-1) hix = COLNO-2;
	if(hiy >= ROWNO-1) hiy = ROWNO-2;

	if(lit) {
		for(x = lowx-1; x <= hix+1; x++) {
			lev = &levl[x][max(lowy-1,0)];
			for(y = lowy-1; y <= hiy+1; y++)
				lev++->lit = 1;
		}
		croom->rlit = 1;
	} else
		croom->rlit = 0;

	croom->lx = lowx;
	croom->hx = hix;
	croom->ly = lowy;
	croom->hy = hiy;
	croom->rtype = rtype;
	croom->doorct = 0;
	/* if we're not making a vault, doorindex will still be 0
	 * if we are, we'll have problems adding niches to the previous room
	 * unless fdoor is at least doorindex
	 */
	croom->fdoor = (is_room || !nroom) ? doorindex : rooms[0].fdoor;
	croom->irregular = FALSE;

	croom->nsubrooms = 0;
	croom->sbrooms[0] = (struct mkroom *) 0;
	if (!special) {
	    for(x = lowx-1; x <= hix+1; x++)
		for(y = lowy-1; y <= hiy+1; y += (hiy-lowy+2)) {
		    levl[x][y].typ = HWALL;
		    levl[x][y].horizontal = 1;	/* For open/secret doors. */
		}
	    for(x = lowx-1; x <= hix+1; x += (hix-lowx+2))
		for(y = lowy; y <= hiy; y++) {
		    levl[x][y].typ = VWALL;
		    levl[x][y].horizontal = 0;	/* For open/secret doors. */
		}
	    for(x = lowx; x <= hix; x++) {
		lev = &levl[x][lowy];
		for(y = lowy; y <= hiy; y++)
		    lev++->typ = ROOM;
	    }
	    if (is_room) {
		levl[lowx-1][lowy-1].typ = TLCORNER;
		levl[hix+1][lowy-1].typ = TRCORNER;
		levl[lowx-1][hiy+1].typ = BLCORNER;
		levl[hix+1][hiy+1].typ = BRCORNER;
	    } else {	/* a subroom */
		wallification(lowx-1, lowy-1, hix+1, hiy+1);
	    }
	}
}


void
add_room(lowx, lowy, hix, hiy, lit, rtype, special)
register int lowx, lowy, hix, hiy;
boolean lit;
schar rtype;
boolean special;
{
	register struct mkroom *croom;

	croom = &rooms[nroom];
	do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit,
					    rtype, special, (boolean) TRUE);
	croom++;
	croom->hx = -1;
	nroom++;
}

void
add_subroom(proom, lowx, lowy, hix, hiy, lit, rtype, special)
struct mkroom *proom;
register int lowx, lowy, hix, hiy;
boolean lit;
schar rtype;
boolean special;
{
	register struct mkroom *croom;

	croom = &subrooms[nsubroom];
	do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit,
					    rtype, special, (boolean) FALSE);
	proom->sbrooms[proom->nsubrooms++] = croom;
	croom++;
	croom->hx = -1;
	nsubroom++;
}

STATIC_OVL void
makerooms()
{
	boolean tried_vault = FALSE;

	/* make rooms until satisfied */
	/* rnd_rect() will returns 0 if no more rects are available... */
	while(nroom < MAXNROFROOMS && rnd_rect()) {
		if(nroom >= (MAXNROFROOMS/6) && rn2(2) && !tried_vault) {
			tried_vault = TRUE;
			if (create_vault()) {
				vault_x = rooms[nroom].lx;
				vault_y = rooms[nroom].ly;
				rooms[nroom].hx = -1;
			}
		} else
		    if (!create_room(-1, -1, -1, -1, -1, -1, OROOM, -1))
			return;
	}
	return;
}

STATIC_OVL void
join(a,b,nxcor)
register int a, b;
boolean nxcor;
{
	coord cc,tt, org, dest;
	register xchar tx, ty, xx, yy;
	register struct mkroom *croom, *troom;
	register int dx, dy;
	boolean cr, tr;

	croom = &rooms[a];
	troom = &rooms[b];

	/* find positions cc and tt for doors in croom and troom
	   and direction for a corridor between them */

	if(troom->hx < 0 || croom->hx < 0 || doorindex >= DOORMAX) return;
	if(troom->lx > croom->hx) {
	    dx = 1;
	    dy = 0;
	    xx = croom->hx+1;
	    tx = troom->lx-1;
	    if (croom->rndvault) {
		if (!finddpos_special(&cc, xx, croom->ly, xx, croom->hy)) return;
	    } else	     finddpos(&cc, xx, croom->ly, xx, croom->hy);
	    if (troom->rndvault) {
		if (!finddpos_special(&tt, tx, troom->ly, tx, troom->hy)) return;
	    } else	     finddpos(&tt, tx, troom->ly, tx, troom->hy);
	} else if(troom->hy < croom->ly) {
	    dy = -1;
	    dx = 0;
	    yy = croom->ly-1;
	    ty = troom->hy+1;
	    if (croom->rndvault) {
		if (!finddpos_special(&cc, croom->lx, yy, croom->hx, yy)) return;
	    } else	     finddpos(&cc, croom->lx, yy, croom->hx, yy);
	    if (troom->rndvault) {
		if (!finddpos_special(&tt, troom->lx, ty, troom->hx, ty)) return;
	    } else	     finddpos(&tt, troom->lx, ty, troom->hx, ty);
	} else if(troom->hx < croom->lx) {
	    dx = -1;
	    dy = 0;
	    xx = croom->lx-1;
	    tx = troom->hx+1;
	    if (croom->rndvault) {
		if (!finddpos_special(&cc, xx, croom->ly, xx, croom->hy)) return;
	    } else	     finddpos(&cc, xx, croom->ly, xx, croom->hy);
	    if (troom->rndvault) {
		if (!finddpos_special(&tt, tx, troom->ly, tx, troom->hy)) return;
	    } else	     finddpos(&tt, tx, troom->ly, tx, troom->hy);
	} else {
	    dy = 1;
	    dx = 0;
	    yy = croom->hy+1;
	    ty = troom->ly-1;
	    if (croom->rndvault) {
		if (!finddpos_special(&cc, croom->lx, yy, croom->hx, yy)) return;
	    } else	     finddpos(&cc, croom->lx, yy, croom->hx, yy);
	    if (troom->rndvault) {
		if (!finddpos_special(&tt, troom->lx, ty, troom->hx, ty)) return;
	    } else	     finddpos(&tt, troom->lx, ty, troom->hx, ty);
	}
	xx = cc.x;
	yy = cc.y;
	tx = tt.x - dx;
	ty = tt.y - dy;
	if(nxcor && levl[xx+dx][yy+dy].typ)
		return;
	if (okdoor(xx,yy) || (!nxcor && !croom->rndvault))
	    dodoor(xx,yy,croom);

	org.x  = xx+dx; org.y  = yy+dy;
	dest.x = tx; dest.y = ty;

	if (!dig_corridor(&org, &dest, nxcor,
			level.flags.arboreal ? ROOM : CORR, STONE))
	    return;

	/* we succeeded in digging the corridor */
	if (okdoor(tt.x, tt.y) || (!nxcor && !troom->rndvault))
	    dodoor(tt.x, tt.y, troom);

	if(smeq[a] < smeq[b])
		smeq[b] = smeq[a];
	else
		smeq[a] = smeq[b];
}

void
makecorridors()
{
	int a, b, i;
	boolean any = TRUE;

	for(a = 0; a < nroom-1; a++) {
		join(a, a+1, FALSE);
		if(!rn2(50)) break; /* allow some randomness */
	}
	for(a = 0; a < nroom-2; a++)
	    if(smeq[a] != smeq[a+2])
		join(a, a+2, FALSE);
	for(a = 0; any && a < nroom; a++) {
	    any = FALSE;
	    for(b = 0; b < nroom; b++)
		if(smeq[a] != smeq[b]) {
		    join(a, b, FALSE);
		    any = TRUE;
		}
	}
	if(nroom > 2)
	    for(i = rn2(nroom) + 4; i; i--) {
		a = rn2(nroom);
		b = rn2(nroom-2);
		if(b >= a) b += 2;
		join(a, b, TRUE);
	    }
}

void
add_door(x,y,aroom)
register int x, y;
register struct mkroom *aroom;
{
	register struct mkroom *broom;
	register int tmp;
	boolean is_subroom;

	aroom->doorct++;
	broom = aroom+1;

	/* door coords are arranged as room-number-ordered,
	   subroom doors first, then room doors */
	is_subroom = ((aroom - rooms) > nroom);
	if (is_subroom) {
	    if(broom->hx < 0) {
		broom = &rooms[0];
		is_subroom = FALSE;
	    }
	}
	if(broom->hx < 0)
		tmp = doorindex;
	else
		for(tmp = doorindex; tmp > broom->fdoor; tmp--)
			doors[tmp] = doors[tmp-1];
	doorindex++;
	doors[tmp].x = x;
	doors[tmp].y = y;
	for( ; broom->hx >= 0; broom++) broom->fdoor++;
	if (is_subroom)
	    for(broom = &rooms[0]; broom->hx >= 0; broom++)
		broom->fdoor++;
}

STATIC_OVL void
dosdoor(x,y,aroom,type)
register xchar x, y;
register struct mkroom *aroom;
register int type;
{
	boolean shdoor = ((*in_rooms(x, y, SHOPBASE))? TRUE : FALSE);

	if(!IS_WALL(levl[x][y].typ)) /* avoid SDOORs on already made doors */
		type = DOOR;
	levl[x][y].typ = type;
	if(type == DOOR) {
	    if(!rn2(3)) {      /* is it a locked door, closed, or a doorway? */
		if(!rn2(5))
		    levl[x][y].doormask = D_ISOPEN;
		else if(!rn2(6))
		    levl[x][y].doormask = D_LOCKED;
		else
		    levl[x][y].doormask = D_CLOSED;

		if (levl[x][y].doormask != D_ISOPEN && !shdoor &&
		    level_difficulty() >= 5 && !rn2(25))
		    levl[x][y].doormask |= D_TRAPPED;
	    } else
#ifdef STUPID
		if (shdoor)
			levl[x][y].doormask = D_ISOPEN;
		else
			levl[x][y].doormask = D_NODOOR;
#else
		levl[x][y].doormask = (shdoor ? D_ISOPEN : D_NODOOR);
#endif
	    if(levl[x][y].doormask & D_TRAPPED) {
		struct monst *mtmp;

		if (level_difficulty() >= 9 && !rn2(5) &&
		   !((mvitals[PM_SMALL_MIMIC].mvflags & G_GONE) &&
		     (mvitals[PM_LARGE_MIMIC].mvflags & G_GONE) &&
		     (mvitals[PM_GIANT_MIMIC].mvflags & G_GONE))) {
		    /* make a mimic instead */
		    levl[x][y].doormask = D_NODOOR;
		    mtmp = makemon(mkclass(S_MIMIC,0), x, y, NO_MM_FLAGS);
		    if (mtmp)
			set_mimic_sym(mtmp);
		}
	    }
	    /* newsym(x,y); */
	} else { /* SDOOR */
		if(shdoor || !rn2(5))	levl[x][y].doormask = D_LOCKED;
		else			levl[x][y].doormask = D_CLOSED;

		if(!shdoor && level_difficulty() >= 4 && !rn2(20))
		    levl[x][y].doormask |= D_TRAPPED;
	}

	add_door(x,y,aroom);
}

STATIC_OVL boolean
place_niche(aroom,dy,xx,yy)
register struct mkroom *aroom;
int *dy, *xx, *yy;
{
	coord dd;

	if(rn2(2)) {
	    *dy = 1;
	    finddpos(&dd, aroom->lx, aroom->hy+1, aroom->hx, aroom->hy+1);
	} else {
	    *dy = -1;
	    finddpos(&dd, aroom->lx, aroom->ly-1, aroom->hx, aroom->ly-1);
	}
	*xx = dd.x;
	*yy = dd.y;
	return((boolean)((isok(*xx,*yy+*dy) && levl[*xx][*yy+*dy].typ == STONE)
	    && (isok(*xx,*yy-*dy) && !IS_POOL(levl[*xx][*yy-*dy].typ)
				  && !IS_FURNITURE(levl[*xx][*yy-*dy].typ))));
}

/* there should be one of these per trap, in the same order as trap.h */
//static NEARDATA const char *trap_engravings[TRAPNUM] = {
//			(char *)0, (char *)0, (char *)0, (char *)0, (char *)0,
//			(char *)0, (char *)0, (char *)0, (char *)0, (char *)0,
//			(char *)0, (char *)0, (char *)0, (char *)0,
//			/* 14..16: trap door, teleport, level-teleport */
//			"Vlad was here", "ad aerarium", "ad aerarium",
//			(char *)0, (char *)0, (char *)0, (char *)0, (char *)0,
//			(char *)0,
//};/* moved into makeniche() */

STATIC_OVL void
makeniche(trap_type)
int trap_type;
{
	register struct mkroom *aroom;
	register struct rm *rm;
	register int vct = 8;
	int dy, xx, yy;
	register struct trap *ttmp;
	const char *trap_engr = (char *)0;

	if(doorindex < DOORMAX)
	  while(vct--) {
	    aroom = &rooms[rn2(nroom)];
	    if(aroom->rtype != OROOM) continue;	/* not an ordinary room */
	    if(aroom->doorct == 1 && rn2(5)) continue;
	    if(!place_niche(aroom,&dy,&xx,&yy)) continue;

	    rm = &levl[xx][yy+dy];
	    if(trap_type || !rn2(4)) {

		rm->typ = SCORR;
		if(trap_type) {
		    if((trap_type == HOLE || trap_type == TRAPDOOR)
			&& !Can_fall_thru(&u.uz))
			trap_type = ROCKTRAP;
		    ttmp = maketrap(xx, yy+dy, trap_type);
		    if (ttmp) {
			if (trap_type != ROCKTRAP) ttmp->once = 1;
			switch (trap_type) {
			    case TRAPDOOR:	trap_engr = E_J("Vlad was here","ヴラド参上"); break;
			    case TELEP_TRAP:	/*fallthru*/
			    case LEVEL_TELEP:	trap_engr = "ad aerarium"; break;
			    default:	break;
			}
			if (trap_engr) {
			    make_engr_at(xx, yy-dy,
				     trap_engr, 0L, DUST);
			    wipe_engr_at(xx, yy-dy, 5); /* age it a little */
			}
		    }
		}
		dosdoor(xx, yy, aroom, SDOOR);
	    } else {
		rm->typ = CORR;
		if(rn2(7))
		    dosdoor(xx, yy, aroom, rn2(5) ? SDOOR : DOOR);
		else {
		    if (!level.flags.noteleport)
			(void) mksobj_at(SCR_TELEPORTATION,
					 xx, yy+dy, TRUE, FALSE);
		    if (!rn2(3)) (void) mkobj_at(0, xx, yy+dy, TRUE);
		}
	    }
	    return;
	}
}

STATIC_OVL void
make_niches()
{
	register int ct = rnd((nroom>>1) + 1), dep = depth(&u.uz);

	boolean	ltptr = (!level.flags.noteleport && dep > 15),
		vamp = (dep > 5 && dep < 25);

	while(ct--) {
		if (ltptr && !rn2(6)) {
			ltptr = FALSE;
			makeniche(LEVEL_TELEP);
		} else if (vamp && !rn2(6)) {
			vamp = FALSE;
			makeniche(TRAPDOOR);
		} else	makeniche(NO_TRAP);
	}
}

STATIC_OVL void
makevtele()
{
	makeniche(TELEP_TRAP);
}

/* clear out various globals that keep information on the current level.
 * some of this is only necessary for some types of levels (maze, normal,
 * special) but it's easier to put it all in one place than make sure
 * each type initializes what it needs to separately.
 */
STATIC_OVL void
clear_level_structures()
{
	static struct rm zerorm = { cmap_to_glyph(S_stone),
						0, 0, 0, 0, 0, 0, 0, 0 };
	register int x,y;
	register struct rm *lev;

	for(x=0; x<COLNO; x++) {
	    lev = &levl[x][0];
	    for(y=0; y<ROWNO; y++) {
		*lev++ = zerorm;
#ifdef MICROPORT_BUG
		level.objects[x][y] = (struct obj *)0;
		level.monsters[x][y] = (struct monst *)0;
		level.clouds[x][y] = (struct cloud *)0;
#endif
	    }
	}
#ifndef MICROPORT_BUG
	(void) memset((genericptr_t)level.objects, 0, sizeof(level.objects));
	(void) memset((genericptr_t)level.monsters, 0, sizeof(level.monsters));
	(void) memset((genericptr_t)level.clouds, 0, sizeof(level.clouds));
#endif
	level.objlist = (struct obj *)0;
	level.buriedobjlist = (struct obj *)0;
	level.monlist = (struct monst *)0;
	level.damagelist = (struct damage *)0;
	level.cloudlist = (struct cloud *)0;

	level.flags.nfountains = 0;
	level.flags.nsinks = 0;
	level.flags.has_shop = 0;
	level.flags.has_vault = 0;
	level.flags.has_zoo = 0;
	level.flags.has_court = 0;
	level.flags.has_morgue = level.flags.graveyard = 0;
	level.flags.has_beehive = 0;
	level.flags.has_barracks = 0;
	level.flags.has_temple = 0;
	level.flags.has_swamp = 0;
	level.flags.noteleport = 0;
	level.flags.hardfloor = 0;
	level.flags.nommap = 0;
	level.flags.hero_memory = 1;
	level.flags.shortsighted = 0;
	level.flags.arboreal = 0;
	level.flags.is_maze_lev = 0;
	level.flags.is_cavernous_lev = 0;

	nroom = 0;
	rooms[0].hx = -1;
	nsubroom = 0;
	subrooms[0].hx = -1;
	doorindex = 0;
	init_rect();
	init_vault();
	xdnstair = ydnstair = xupstair = yupstair = 0;
	sstairs.sx = sstairs.sy = 0;
	xdnladder = ydnladder = xupladder = yupladder = 0;
	made_branch = FALSE;
	clear_regions();
}

STATIC_OVL void
makelevel()
{
	register struct mkroom *croom, *troom;
	register int tryct;
	register int x, y;
	struct monst *tmonst;	/* always put a web with a spider */
	branch *branchp;
	int room_threshold;
	coord cc;
	boolean rndvault = FALSE;
	schar river = 0;

	if(wiz1_level.dlevel == 0) init_dungeons();
	oinit();	/* assign level dependent obj probabilities */
	clear_level_structures();

	{
	    register s_level *slev = Is_special(&u.uz);

	    /* check for special levels */
#ifdef REINCARNATION
	    if (slev && !Is_rogue_level(&u.uz))
#else
	    if (slev)
#endif
	    {
		    makemaz(slev->proto);
		    return;
	    } else if (dungeons[u.uz.dnum].proto[0]) {
		    makemaz("");
		    return;
	    } else if (In_mines(&u.uz)) {
		    makemaz("minefill");
		    return;
	    } else if (In_quest(&u.uz)) {
		    char	fillname[9];
		    s_level	*loc_lev;

		    Sprintf(fillname, "%s-loca", urole.filecode);
		    loc_lev = find_level(fillname);

		    Sprintf(fillname, "%s-fil", urole.filecode);
		    Strcat(fillname,
			   (u.uz.dlevel < loc_lev->dlevel.dlevel) ? "a" : "b");
		    makemaz(fillname);
		    return;
	    } else if(In_hell(&u.uz)) {
#ifdef NEWGEHENNOM
		switch (In_which_hell(&u.uz)) {
		    case INHELL_ASMODEUS:
			mkasmolev();
			break;
		    case INHELL_JUIBLEX:
			mkjuiblev();
			break;
		    case INHELL_BAALZEBUB:
			mkbaallev(ROOM, BOG, 0);
			break;
		    case INHELL_ORCUS:
			mkghosttown(0);
			break;
		    case INHELL_HELL:
			mkhelllev();
			break;
		    default:
			makemaz("");
			break;
		}
#else  /* NEWGEHENNOM */
		makemaz("");
#endif /* NEWGEHENNOM */
		return;
	    }
	}
	if (u.uz.dnum == medusa_level.dnum &&
	    depth(&u.uz) > depth(&medusa_level)) {
		river = MOAT;
	}

	/* otherwise, fall through - it's a "regular" level. */

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) {
		makeroguerooms();
		makerogueghost();
	} else
#endif
	{
/*if(wizard && depth(&u.uz)==2) {
    char buf[BUFSZ];
    Sprintf(buf, "rndv-%03d", rn2(4));
    make_rndvault(buf);
    rndvault = TRUE;
} else {*/
		make_rndvault(0);
/*}*/
		makerooms();
	}
	sort_rooms();

	/* irregular room needs roomno, but sort_rooms() broke it... */
	for (croom = &rooms[0]; croom != &rooms[nroom]; croom++) {
	    if (croom->irregular)
		topologize_irregular(croom);
	    if (croom->rndvault && croom->needfill) {
		fill_room(croom, FALSE);
	    }
	}
	/* if a random vault already have stairs, fixup it */
	fix_stair_rooms();

	/* construct stairs (up and down in different rooms if possible) */
	for (tryct = 50; tryct; tryct--) {
	    croom = &rooms[rn2(nroom)];
	    if (croom->rtype == OROOM) break;
	}
	if (!Is_botlevel(&u.uz)) {
	    if (!xdnstair) {
		coord cc;
		somexy(croom, &cc);
		mkstairs(cc.x, cc.y, 0, croom);	/* down */
	    } else /* maybe in a random vault */
		croom = dnstairs_room;
	}
	if (nroom > 1) {
	    troom = croom;
	    for (tryct = 50; tryct; tryct--) {
		croom = &rooms[rn2(nroom-1)];
		if (croom == troom) croom++;
		if (croom->rtype == OROOM) break;
	    }
	}
	if (u.uz.dlevel != 1 && !xupstair) {
	    xchar sx, sy;
	    coord cc;
	    do {
		somexy(croom, &cc);
	    } while(occupied(cc.x, cc.y));
	    mkstairs(cc.x, cc.y, 1, croom);	/* up */
	}

	branchp = Is_branchlev(&u.uz);	/* possible dungeon branch */
	room_threshold = branchp ? 4 : 3; /* minimum number of rooms needed
					     to allow a random special room */
	if (rndvault) room_threshold++;
#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) goto skip0;
#endif
	makecorridors();
	make_niches();

	if (river) mkriver(STONE, river);

	/* make a secret treasure vault, not connected to the rest */
	if(do_vault()) {
		xchar w,h;
#ifdef DEBUG
		debugpline("trying to make a vault...");
#endif
		w = 1;
		h = 1;
		if (check_room(&vault_x, &w, &vault_y, &h, TRUE)) {
		    fill_vault:
			add_room(vault_x, vault_y, vault_x+w,
				 vault_y+h, TRUE, VAULT, FALSE);
			level.flags.has_vault = 1;
			++room_threshold;
			fill_room(&rooms[nroom - 1], FALSE);
			mk_knox_portal(vault_x+w, vault_y+h);
			if(!level.flags.noteleport && !rn2(3)) makevtele();
		} else if(rnd_rect() && create_vault()) {
			vault_x = rooms[nroom].lx;
			vault_y = rooms[nroom].ly;
			if (check_room(&vault_x, &w, &vault_y, &h, TRUE))
				goto fill_vault;
			else
				rooms[nroom].hx = -1;
		}
	}

    {
	register int u_depth = depth(&u.uz);

#ifdef WIZARD
	if(wizard && nh_getenv("SHOPTYPE")) mkroom(SHOPBASE); else
#endif
	if (u_depth > 1 &&
	    u_depth < depth(&medusa_level) &&
	    nroom >= room_threshold &&
	    rn2(u_depth) < 3) mkroom(SHOPBASE);
	else if (u_depth > 4 && !rn2(6)) mkroom(COURT);
	else if (u_depth > 5 && !rn2(8) &&
	   !(mvitals[PM_LEPRECHAUN].mvflags & G_GONE)) mkroom(LEPREHALL);
	else if (u_depth > 6 && !rn2(7)) mkroom(ZOO);
	else if (u_depth > 8 && !rn2(5)) mkroom(TEMPLE);
	else if (u_depth > 9 && !rn2(5) &&
	   !(mvitals[PM_KILLER_BEE].mvflags & G_GONE)) mkroom(BEEHIVE);
	else if (u_depth > 11 && !rn2(6)) mkroom(MORGUE);
	else if (u_depth > 12 && !rn2(8)) mkroom(ANTHOLE);
	else if (u_depth > 14 && !rn2(4) &&
	   !(mvitals[PM_SOLDIER].mvflags & G_GONE)) mkroom(BARRACKS);
	else if (u_depth > 15 && !rn2(6)) mkroom(SWAMP);
	else if (u_depth > 16 && !rn2(8) &&
	   !(mvitals[PM_COCKATRICE].mvflags & G_GONE)) mkroom(COCKNEST);
    }

#ifdef REINCARNATION
skip0:
#endif
	/* Place multi-dungeon branch. */
	place_branch(branchp, 0, 0);

	/* for each room: put things inside */
	for(croom = rooms; croom->hx > 0; croom++) {
		if(croom->rtype != OROOM) continue;

		/* put a sleeping monster inside */
		/* Note: monster may be on the stairs. This cannot be
		   avoided: maybe the player fell through a trap door
		   while a monster was on the stairs. Conclusion:
		   we have to check for monsters on the stairs anyway. */

		if(u.uhave.amulet || !rn2(3)) {
		    somexy(croom, &cc);
		    x = cc.x; y = cc.y;
		    tmonst = makemon((struct permonst *) 0, x,y, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
		    if (tmonst && tmonst->data == &mons[PM_GIANT_SPIDER] &&
			    !occupied(x, y))
			(void) maketrap(x, y, WEB);
		}
		/* put traps and mimics inside */
		goldseen = FALSE;
		x = 8 - (level_difficulty()/6);
		if (x <= 1) x = 2;
		while (!rn2(x))
		    mktrap(0,0,croom,(coord*)0);
		if (!goldseen && !rn2(3)) {
		    somexy(croom, &cc);
		    (void) mkgold(0L, cc.x, cc.y);
		}
#ifdef REINCARNATION
		if(Is_rogue_level(&u.uz)) goto skip_nonrogue;
#endif
		if(!rn2(10)) mkfount(0,croom);
#ifdef SINKS
		if(!rn2(60)) mksink(croom);
#endif
		if(!rn2(60)) mkaltar(croom);
		x = 80 - (depth(&u.uz) * 2);
		if (x < 2) x = 2;
		if(!rn2(x)) mkgrave(croom);

		/* put statues inside */
		if(!rn2(20)) {
		    somexy(croom, &cc);
		    (void) mkcorpstat(STATUE, (struct monst *)0,
				      (struct permonst *)0,
				      cc.x, cc.y, TRUE);
		}
		/* put box/chest inside;
		 *  40% chance for at least 1 box, regardless of number
		 *  of rooms; about 5 - 7.5% for 2 boxes, least likely
		 *  when few rooms; chance for 3 or more is neglible.
		 */
		if(!rn2(nroom * 5 / 2)) {
		    somexy(croom, &cc);
		    (void) mksobj_at((rn2(3)) ? LARGE_BOX : CHEST,
				     cc.x, cc.y, TRUE, FALSE);
		}

		/* maybe make some graffiti */
		if(!rn2(27 + 3 * abs(depth(&u.uz)))) {
		    char buf[BUFSZ];
		    const char *mesg = random_engraving(buf);
		    if (mesg) {
			do {
			    somexy(croom, &cc);
			    x = cc.x;  y = cc.y;
			} while(levl[x][y].typ != ROOM && !rn2(40));
			if (!(IS_POOL(levl[x][y].typ) ||
			      IS_FURNITURE(levl[x][y].typ)))
			    make_engr_at(x, y, mesg, 0L, MARK);
		    }
		}

#ifdef REINCARNATION
	skip_nonrogue:
#endif
		if(rn2(5) < 2) {
		    somexy(croom, &cc);
		    (void) mkobj_at(0, cc.x, cc.y, TRUE);
		    tryct = 0;
		    while(!rn2(5)) {
			if(++tryct > 100) {
			    impossible("tryct overflow4");
			    break;
			}
			somexy(croom, &cc);
			(void) mkobj_at(0, cc.x, cc.y, TRUE);
		    }
		}
	}
}

/*
 *	Place deposits of minerals (gold and misc gems) in the stone
 *	surrounding the rooms on the map.
 *	Also place kelp in water.
 */
STATIC_OVL void
mineralize()
{
	s_level *sp;
	struct obj *otmp;
	int goldprob, gemprob, x, y, cnt;


	/* Place kelp, except on the plane of water */
	if (In_endgame(&u.uz)) return;
	for (x = 2; x < (COLNO - 2); x++)
	    for (y = 1; y < (ROWNO - 1); y++)
		if ((levl[x][y].typ == POOL && !rn2(10)) ||
			(levl[x][y].typ == MOAT && !rn2(30)))
		    (void) mksobj_at(KELP_FROND, x, y, TRUE, FALSE);

	/* determine if it is even allowed;
	   almost all special levels are excluded */
	if (In_hell(&u.uz) || In_V_tower(&u.uz) ||
#ifdef REINCARNATION
		Is_rogue_level(&u.uz) ||
#endif
		level.flags.arboreal ||
		((sp = Is_special(&u.uz)) != 0 && !Is_oracle_level(&u.uz)
					&& (!In_mines(&u.uz) || sp->flags.town)
	    )) return;

	/* basic level-related probabilities */
	goldprob = 20 + depth(&u.uz) / 3;
	gemprob = goldprob / 4;

	/* mines have ***MORE*** goodies - otherwise why mine? */
	if (In_mines(&u.uz)) {
	    goldprob *= 2;
	    gemprob *= 3;
	} else if (In_quest(&u.uz)) {
	    goldprob /= 4;
	    gemprob /= 6;
	}

	/*
	 * Seed rock areas with gold and/or gems.
	 * We use fairly low level object handling to avoid unnecessary
	 * overhead from placing things in the floor chain prior to burial.
	 */
	for (x = 2; x < (COLNO - 2); x++)
	  for (y = 1; y < (ROWNO - 1); y++)
	    if (levl[x][y+1].typ != STONE) {	 /* <x,y> spot not eligible */
		y += 2;		/* next two spots aren't eligible either */
	    } else if (levl[x][y].typ != STONE) { /* this spot not eligible */
		y += 1;		/* next spot isn't eligible either */
	    } else if (!(levl[x][y].wall_info & W_NONDIGGABLE) &&
		  levl[x][y-1].typ   == STONE &&
		  levl[x+1][y-1].typ == STONE && levl[x-1][y-1].typ == STONE &&
		  levl[x+1][y].typ   == STONE && levl[x-1][y].typ   == STONE &&
		  levl[x+1][y+1].typ == STONE && levl[x-1][y+1].typ == STONE) {
		if (rn2(1000) < goldprob) {
		    if ((otmp = mksobj(GOLD_PIECE, FALSE, FALSE)) != 0) {
			otmp->ox = x,  otmp->oy = y;
			otmp->quan = 1L + rnd(goldprob * 3);
			otmp->owt = weight(otmp);
			if (!rn2(3)) add_to_buried(otmp);
			else place_object(otmp, x, y);
		    }
		}
		if (rn2(1000) < gemprob) {
		    for (cnt = rnd(2 + dunlev(&u.uz) / 3); cnt > 0; cnt--)
			if ((otmp = mkobj(GEM_CLASS, FALSE)) != 0) {
			    if (otmp->otyp == ROCK) {
				dealloc_obj(otmp);	/* discard it */
			    } else {
				otmp->ox = x,  otmp->oy = y;
				if (!rn2(3)) add_to_buried(otmp);
				else place_object(otmp, x, y);
			    }
		    }
		}
	    }
}

void
mklev()
{
	struct mkroom *croom;

	if(getbones()) return;
	in_mklev = TRUE;
	makelevel();
	bound_digging();
	mineralize();
	in_mklev = FALSE;
	/* has_morgue gets cleared once morgue is entered; graveyard stays
	   set (graveyard might already be set even when has_morgue is clear
	   [see fixup_special()], so don't update it unconditionally) */
	if (level.flags.has_morgue)
	    level.flags.graveyard = 1;
	if (!level.flags.is_maze_lev) {
	    for (croom = &rooms[0]; croom != &rooms[nroom]; croom++)
#ifdef SPECIALIZATION
		topologize(croom, FALSE);
#else
		topologize(croom);
#endif
	}
	set_wall_state();
}

void
#ifdef SPECIALIZATION
topologize(croom, do_ordinary)
register struct mkroom *croom;
boolean do_ordinary;
#else
topologize(croom)
register struct mkroom *croom;
#endif
{
	register int x, y, roomno = (croom - rooms) + ROOMOFFSET;
	register int lowx = croom->lx, lowy = croom->ly;
	register int hix = croom->hx, hiy = croom->hy;
#ifdef SPECIALIZATION
	register schar rtype = croom->rtype;
#endif
	register int subindex, nsubrooms = croom->nsubrooms;

	/* skip the room if already done; i.e. a shop handled out of order */
	/* also skip if this is non-rectangular (it _must_ be done already) */
	if ((int) levl[lowx][lowy].roomno == roomno || croom->irregular)
	    return;

#ifdef SPECIALIZATION
# ifdef REINCARNATION
	if (Is_rogue_level(&u.uz))
	    do_ordinary = TRUE;		/* vision routine helper */
# endif
	if ((rtype != OROOM) || do_ordinary)
#endif
	{
	    /* do innards first */
	    for(x = lowx; x <= hix; x++)
		for(y = lowy; y <= hiy; y++)
#ifdef SPECIALIZATION
		    if (rtype == OROOM)
			levl[x][y].roomno = NO_ROOM;
		    else
#endif
			levl[x][y].roomno = roomno;
	    /* top and bottom edges */
	    for(x = lowx-1; x <= hix+1; x++)
		for(y = lowy-1; y <= hiy+1; y += (hiy-lowy+2)) {
		    levl[x][y].edge = 1;
		    if (levl[x][y].roomno)
			levl[x][y].roomno = SHARED;
		    else
			levl[x][y].roomno = roomno;
		}
	    /* sides */
	    for(x = lowx-1; x <= hix+1; x += (hix-lowx+2))
		for(y = lowy; y <= hiy; y++) {
		    levl[x][y].edge = 1;
		    if (levl[x][y].roomno)
			levl[x][y].roomno = SHARED;
		    else
			levl[x][y].roomno = roomno;
		}
	}
	/* subrooms */
	for (subindex = 0; subindex < nsubrooms; subindex++)
#ifdef SPECIALIZATION
		topologize(croom->sbrooms[subindex], (rtype != OROOM));
#else
		topologize(croom->sbrooms[subindex]);
#endif
}

/* assume initialized once, but sort_room() makes it invalid... */
STATIC_OVL void
topologize_irregular(croom)
struct mkroom *croom;
{
    int x, y, n;
    int roomno = (croom - rooms) + ROOMOFFSET;
    for(x = croom->lx - 1; x <= croom->hx + 1; x++)
	for(y = croom->ly - 1; y <= croom->hy + 1; y++) {
	    n = levl[x][y].roomno - ROOMOFFSET;
	    if (n >= 0 && n < nroom)
		levl[x][y].roomno = roomno;
	}
}

/* Find an unused room for a branch location. */
STATIC_OVL struct mkroom *
find_branch_room(mp)
    coord *mp;
{
    struct mkroom *croom = 0;

    if (nroom == 0) {
	mazexy(mp);		/* already verifies location */
    } else {
	/* not perfect - there may be only one stairway */
	if(nroom > 2) {
	    int tryct = 0;

	    do
		croom = &rooms[rn2(nroom)];
	    while((croom == dnstairs_room || croom == upstairs_room ||
		  croom->rtype != OROOM) && (++tryct < 100));
	} else
	    croom = &rooms[rn2(nroom)];

	do {
	    if (!somexy(croom, mp))
		impossible("Can't place branch!");
	} while(occupied(mp->x, mp->y) ||
	    (levl[mp->x][mp->y].typ != CORR && !IS_FLOOR(levl[mp->x][mp->y].typ)));
//	    (levl[mp->x][mp->y].typ != CORR && levl[mp->x][mp->y].typ != ROOM));
    }
    return croom;
}

/* Find the room for (x,y).  Return null if not in a room. */
STATIC_OVL struct mkroom *
pos_to_room(x, y)
    xchar x, y;
{
    int i;
    struct mkroom *curr;

    for (curr = rooms, i = 0; i < nroom; curr++, i++)
	if (inside_room(curr, x, y)) return curr;;
    return (struct mkroom *) 0;
}


/* If given a branch, randomly place a special stair or portal. */
void
place_branch(br, x, y)
branch *br;	/* branch to place */
xchar x, y;	/* location */
{
	coord	      m;
	d_level	      *dest;
	boolean	      make_stairs;
	struct mkroom *br_room;

	/*
	 * Return immediately if there is no branch to make or we have
	 * already made one.  This routine can be called twice when
	 * a special level is loaded that specifies an SSTAIR location
	 * as a favored spot for a branch.
	 */
	if (!br || made_branch) return;

	if (!x) {	/* find random coordinates for branch */
	    br_room = find_branch_room(&m);
	    x = m.x;
	    y = m.y;
	} else {
	    br_room = pos_to_room(x, y);
	}

	if (on_level(&br->end1, &u.uz)) {
	    /* we're on end1 */
	    make_stairs = br->type != BR_NO_END1;
	    dest = &br->end2;
	} else {
	    /* we're on end2 */
	    make_stairs = br->type != BR_NO_END2;
	    dest = &br->end1;
	}

	if (br->type == BR_PORTAL) {
	    mkportal(x, y, dest->dnum, dest->dlevel);
	} else if (make_stairs) {
	    sstairs.sx = x;
	    sstairs.sy = y;
	    sstairs.up = (char) on_level(&br->end1, &u.uz) ?
					    br->end1_up : !br->end1_up;
	    assign_level(&sstairs.tolev, dest);
	    sstairs_room = br_room;

	    levl[x][y].ladder = sstairs.up ? LA_UP : LA_DOWN;
	    levl[x][y].typ = STAIRS;
	}
	/*
	 * Set made_branch to TRUE even if we didn't make a stairwell (i.e.
	 * make_stairs is false) since there is currently only one branch
	 * per level, if we failed once, we're going to fail again on the
	 * next call.
	 */
	made_branch = TRUE;
}

STATIC_OVL boolean
bydoor(x, y)
register xchar x, y;
{
	register int typ;

	if (isok(x+1, y)) {
		typ = levl[x+1][y].typ;
		if (IS_DOOR(typ) || typ == SDOOR) return TRUE;
	}
	if (isok(x-1, y)) {
		typ = levl[x-1][y].typ;
		if (IS_DOOR(typ) || typ == SDOOR) return TRUE;
	}
	if (isok(x, y+1)) {
		typ = levl[x][y+1].typ;
		if (IS_DOOR(typ) || typ == SDOOR) return TRUE;
	}
	if (isok(x, y-1)) {
		typ = levl[x][y-1].typ;
		if (IS_DOOR(typ) || typ == SDOOR) return TRUE;
	}
	return FALSE;
}

/* see whether it is allowable to create a door at [x,y] */
int
okdoor(x,y)
register xchar x, y;
{
	register boolean near_door = bydoor(x, y);

	return((levl[x][y].typ == HWALL || levl[x][y].typ == VWALL) &&
		((levl[x][y].wall_info & WM_MASK) != W_NODOOR) &&
			doorindex < DOORMAX && !near_door);
}

void
dodoor(x,y,aroom)
register int x, y;
register struct mkroom *aroom;
{
	if(doorindex >= DOORMAX) {
		impossible("DOORMAX exceeded?");
		return;
	}

	dosdoor(x,y,aroom,rn2(8) ? DOOR : SDOOR);
}

boolean
occupied(x, y)
register xchar x, y;
{
	return((boolean)(t_at(x, y)
		|| IS_FURNITURE(levl[x][y].typ)
		|| is_lava(x,y)
		|| is_pool(x,y)
		|| invocation_pos(x,y)
		));
}

/* make a trap somewhere (in croom if mazeflag = 0 && !tm) */
/* if tm != null, make trap at that location */
void
mktrap(num, mazeflag, croom, tm)
register int num, mazeflag;
register struct mkroom *croom;
coord *tm;
{
	register int kind;
	coord m;

	/* no traps in pools */
	if (tm && is_pool(tm->x,tm->y)) return;

	if (num > 0 && num < TRAPNUM) {
	    kind = num;
#ifdef REINCARNATION
	} else if (Is_rogue_level(&u.uz)) {
	    switch (rn2(7)) {
		default: kind = BEAR_TRAP; break; /* 0 */
		case 1: kind = ARROW_TRAP; break;
		case 2: kind = DART_TRAP; break;
		case 3: kind = TRAPDOOR; break;
		case 4: kind = PIT; break;
		case 5: kind = SLP_GAS_TRAP; break;
		case 6: kind = RUST_TRAP; break;
	    }
#endif
	} else if (Inhell && !rn2(5)) {
	    /* bias the frequency of fire traps in Gehennom */
	    kind = FIRE_TRAP;
	} else {
	    unsigned lvl = level_difficulty();

	    do {
		kind = rnd(TRAPNUM-1);
		/* reject "too hard" traps */
		switch (kind) {
		    case MAGIC_PORTAL:
			kind = NO_TRAP; break;
		    case ROLLING_BOULDER_TRAP:
		    case SLP_GAS_TRAP:
			if (lvl < 2) kind = NO_TRAP; break;
		    case LEVEL_TELEP:
			if (lvl < 5 || level.flags.noteleport)
			    kind = NO_TRAP; break;
		    case SPIKED_PIT:
			if (lvl < 5) kind = NO_TRAP; break;
		    case LANDMINE:
			if (lvl < 6) kind = NO_TRAP; break;
		    case WEB:
			if (lvl < 7) kind = NO_TRAP; break;
		    case STATUE_TRAP:
		    case POLY_TRAP:
			if (lvl < 8) kind = NO_TRAP; break;
		    case FIRE_TRAP:
			if (!Inhell) kind = NO_TRAP; break;
		    case TELEP_TRAP:
			if (level.flags.noteleport) kind = NO_TRAP; break;
		    case HOLE:
			/* make these much less often than other traps */
			if (rn2(7)) kind = NO_TRAP; break;
		}
	    } while (kind == NO_TRAP);
	}

	if ((kind == TRAPDOOR || kind == HOLE) && !Can_fall_thru(&u.uz))
		kind = ROCKTRAP;

	if (tm)
	    m = *tm;
	else {
	    register int tryct = 0;
	    boolean avoid_boulder = (kind == PIT || kind == SPIKED_PIT ||
				     kind == TRAPDOOR || kind == HOLE);

	    do {
		if (++tryct > 200)
		    return;
		if (mazeflag)
		    mazexy(&m);
		else if (!somexy(croom,&m))
		    return;
	    } while (occupied(m.x, m.y) ||
			(avoid_boulder && sobj_at(BOULDER, m.x, m.y)));
	}

	(void) maketrap(m.x, m.y, kind);
	if (kind == WEB) (void) makemon(&mons[PM_GIANT_SPIDER],
						m.x, m.y, NO_MM_FLAGS);
}

void
mkstairs(x, y, up, croom)
xchar x, y;
char  up;
struct mkroom *croom;
{
	if (!x) {
	    impossible("mkstairs:  bogus stair attempt at <%d,%d>", x, y);
	    return;
	}

	/*
	 * We can't make a regular stair off an end of the dungeon.  This
	 * attempt can happen when a special level is placed at an end and
	 * has an up or down stair specified in its description file.
	 */
	if ((dunlev(&u.uz) == 1 && up) ||
			(dunlev(&u.uz) == dunlevs_in_dungeon(&u.uz) && !up))
	    return;

	if(up) {
		xupstair = x;
		yupstair = y;
		upstairs_room = croom;
	} else {
		xdnstair = x;
		ydnstair = y;
		dnstairs_room = croom;
	}

	levl[x][y].typ = STAIRS;
	levl[x][y].ladder = up ? LA_UP : LA_DOWN;
}

STATIC_OVL
void
mkfount(mazeflag,croom)
register int mazeflag;
register struct mkroom *croom;
{
	coord m;
	register int tryct = 0;

	do {
	    if(++tryct > 200) return;
	    if(mazeflag)
		mazexy(&m);
	    else
		if (!somexy(croom, &m))
		    return;
	} while(occupied(m.x, m.y) || bydoor(m.x, m.y));

	/* Put a fountain at m.x, m.y */
	levl[m.x][m.y].typ = FOUNTAIN;
	/* Is it a "blessed" fountain? (affects drinking from fountain) */
	if(!rn2(7)) levl[m.x][m.y].blessedftn = 1;

	level.flags.nfountains++;
}

#ifdef SINKS
STATIC_OVL void
mksink(croom)
register struct mkroom *croom;
{
	coord m;
	register int tryct = 0;

	do {
	    if(++tryct > 200) return;
	    if (!somexy(croom, &m))
		return;
	} while(occupied(m.x, m.y) || bydoor(m.x, m.y));

	/* Put a sink at m.x, m.y */
	levl[m.x][m.y].typ = SINK;

	level.flags.nsinks++;
}
#endif /* SINKS */


STATIC_OVL void
mkaltar(croom)
register struct mkroom *croom;
{
	coord m;
	register int tryct = 0;
	aligntyp al;

	if (croom->rtype != OROOM) return;

	do {
	    if(++tryct > 200) return;
	    if (!somexy(croom, &m))
		return;
	} while (occupied(m.x, m.y) || bydoor(m.x, m.y));

	/* Put an altar at m.x, m.y */
	levl[m.x][m.y].typ = ALTAR;

	/* -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL */
	al = rn2((int)A_LAWFUL+2) - 1;
	/* balance - avoid getting artifact weapons in early stage */
	if (u.ualign.type == al && depth(&u.uz) < 10) {
	    al += rn2(2)*2 - 1;
	    if (al > A_LAWFUL) al = A_CHAOTIC;
	    else if (al < A_CHAOTIC) al = A_LAWFUL;
	}

	levl[m.x][m.y].altarmask = Align2amask( al );
}

static void
mkgrave(croom)
struct mkroom *croom;
{
	coord m;
	register int tryct = 0;
	register struct obj *otmp;
	boolean dobell = !rn2(10);


	if(croom->rtype != OROOM) return;

	do {
	    if(++tryct > 200) return;
	    if (!somexy(croom, &m))
		return;
	} while (occupied(m.x, m.y) || bydoor(m.x, m.y));

	/* Put a grave at m.x, m.y */
	make_grave(m.x, m.y, dobell ? E_J("Saved by the bell!","ベルに救われた！") : (char *) 0);

	/* Possibly fill it with objects */
	if (!rn2(3)) (void) mkgold(0L, m.x, m.y);
	for (tryct = rn2(5); tryct; tryct--) {
	    otmp = mkobj(RANDOM_CLASS, TRUE);
	    if (!otmp) return;
	    curse(otmp);
	    otmp->ox = m.x;
	    otmp->oy = m.y;
	    add_to_buried(otmp);
	}

	/* Leave a bell, in case we accidentally buried someone alive */
	if (dobell) (void) mksobj_at(BELL, m.x, m.y, TRUE, FALSE);
	return;
}


/* maze levels have slightly different constraints from normal levels */
#define x_maze_min 2
#define y_maze_min 2
/*
 * Major level transmutation: add a set of stairs (to the Sanctum) after
 * an earthquake that leaves behind a a new topology, centered at inv_pos.
 * Assumes there are no rooms within the invocation area and that inv_pos
 * is not too close to the edge of the map.  Also assume the hero can see,
 * which is guaranteed for normal play due to the fact that sight is needed
 * to read the Book of the Dead.
 */
void
mkinvokearea()
{
    int dist;
    xchar xmin = inv_pos.x, xmax = inv_pos.x;
    xchar ymin = inv_pos.y, ymax = inv_pos.y;
    register xchar i;

    pline_The(E_J("floor shakes violently under you!",
		  "床があなたの足下で激しく揺れ動いた！"));
    pline_The(E_J("walls around you begin to bend and crumble!",
		  "周囲の壁が倒壊してゆく！"));
    display_nhwindow(WIN_MESSAGE, TRUE);

    mkinvpos(xmin, ymin, 0);		/* middle, before placing stairs */

    for(dist = 1; dist < 7; dist++) {
	xmin--; xmax++;

	/* top and bottom */
	if(dist != 3) { /* the area is wider that it is high */
	    ymin--; ymax++;
	    for(i = xmin+1; i < xmax; i++) {
		mkinvpos(i, ymin, dist);
		mkinvpos(i, ymax, dist);
	    }
	}

	/* left and right */
	for(i = ymin; i <= ymax; i++) {
	    mkinvpos(xmin, i, dist);
	    mkinvpos(xmax, i, dist);
	}

	flush_screen(1);	/* make sure the new glyphs shows up */
	delay_output();
    }

    You(E_J("are standing at the top of a stairwell leading down!",
	    "下へと続く階段の最上段に立っている！"));
    mkstairs(u.ux, u.uy, 0, (struct mkroom *)0); /* down */
    newsym(u.ux, u.uy);
    vision_full_recalc = 1;	/* everything changed */
}

/* Change level topology.  Boulders in the vicinity are eliminated.
 * Temporarily overrides vision in the name of a nice effect.
 */
STATIC_OVL void
mkinvpos(x,y,dist)
xchar x,y;
int dist;
{
    struct trap *ttmp;
    struct obj *otmp;
    boolean make_rocks;
    register struct rm *lev = &levl[x][y];

    /* clip at existing map borders if necessary */
    if (!within_bounded_area(x, y, x_maze_min + 1, y_maze_min + 1,
				   x_maze_max - 1, y_maze_max - 1)) {
	/* only outermost 2 columns and/or rows may be truncated due to edge */
	if (dist < (7 - 2))
	    panic("mkinvpos: <%d,%d> (%d) off map edge!", x, y, dist);
	return;
    }

    /* clear traps */
    if ((ttmp = t_at(x,y)) != 0) deltrap(ttmp);

    /* clear boulders; leave some rocks for non-{moat|trap} locations */
    make_rocks = (dist != 1 && dist != 4 && dist != 5) ? TRUE : FALSE;
    while ((otmp = sobj_at(BOULDER, x, y)) != 0) {
	if (make_rocks) {
	    fracture_rock(otmp);
	    make_rocks = FALSE;		/* don't bother with more rocks */
	} else {
	    obj_extract_self(otmp);
	    obfree(otmp, (struct obj *)0);
	}
    }
    unblock_point(x,y);	/* make sure vision knows this location is open */

    /* fake out saved state */
    lev->seenv = 0;
    lev->doormask = 0;
    if(dist < 6) lev->lit = TRUE;
    lev->waslit = TRUE;
    lev->horizontal = FALSE;
    viz_array[y][x] = (dist < 6 ) ?
	(IN_SIGHT|COULD_SEE) : /* short-circuit vision recalc */
	COULD_SEE;

    switch(dist) {
    case 1: /* fire traps */
	if (is_pool(x,y)) break;
	lev->typ = ROOM;
	ttmp = maketrap(x, y, FIRE_TRAP);
	if (ttmp) ttmp->tseen = TRUE;
	break;
    case 0: /* lit room locations */
    case 2:
    case 3:
    case 6: /* unlit room locations */
	lev->typ = ROOM;
	break;
    case 4: /* pools (aka a wide moat) */
    case 5:
	lev->typ = MOAT;
	/* No kelp! */
	break;
    default:
	impossible("mkinvpos called with dist %d", dist);
	break;
    }

    /* display new value of position; could have a monster/object on it */
    newsym(x,y);
}

/*
 * The portal to Ludios is special.  The entrance can only occur within a
 * vault in the main dungeon at a depth greater than 10.  The Ludios branch
 * structure reflects this by having a bogus "source" dungeon:  the value
 * of n_dgns (thus, Is_branchlev() will never find it).
 *
 * Ludios will remain isolated until the branch is corrected by this function.
 */
STATIC_OVL void
mk_knox_portal(x, y)
xchar x, y;
{
	extern int n_dgns;		/* from dungeon.c */
	d_level *source;
	branch *br;
	schar u_depth;

	br = dungeon_branch("Fort Ludios");
	if (on_level(&knox_level, &br->end1)) {
	    source = &br->end2;
	} else {
	    /* disallow Knox branch on a level with one branch already */
	    if(Is_branchlev(&u.uz))
		return;
	    source = &br->end1;
	}

	/* Already set or 2/3 chance of deferring until a later level. */
	if (source->dnum < n_dgns || (rn2(3)
#ifdef WIZARD
				      && !wizard
#endif
				      )) return;

	if (! (u.uz.dnum == oracle_level.dnum	    /* in main dungeon */
		&& !at_dgn_entrance("The Quest")    /* but not Quest's entry */
		&& (u_depth = depth(&u.uz)) > 10    /* beneath 10 */
		&& u_depth < depth(&medusa_level))) /* and above Medusa */
	    return;

	/* Adjust source to be current level and re-insert branch. */
	*source = u.uz;
	insert_branch(br, TRUE);

#ifdef DEBUG
	pline("Made knox portal.");
#endif
	place_branch(br, x, y);
}

/*****************************************************************
   Ghost town (orcus level) random map generator
   1) Fill map with INVALID_TYPE
   2) Draw roads with CORR
   3) Draw rooms with WALL/ROOM
   4) Replace INVALID_TYPE to ROOM/graves/trees
   5) Replace CORR to ROOM
 *****************************************************************/
static int XMIN;	// 2
static int XMAX;	// (COLNO-1)
static int YMIN;	// 1
static int YMAX;	// (ROWNO)

STATIC_OVL int
inmap(x,y)
int x;
int y;
{
	return (x >= XMIN && x < XMAX && y >= YMIN && y < YMAX);
}

STATIC_OVL int
gtwn_rndloc(x,y)
int *x;
int *y;
{
	int i;
	for (i=100; i>0; i--) {
		*x = rnd(COLNO-1);
		*y = rn2(ROWNO);
		if (levl[*x][*y].typ == ROOM && !occupied(*x, *y)) return 1;
	}
	*x = *y = 0;
	return 0;
}

STATIC_OVL void
gtwn_initmap()
{
	int x,y;
	for (y=0; y<ROWNO; y++) {
	    for (x=1; x<COLNO; x++) {
		if (inmap(x,y)) {
		    levl[x][y].typ = INVALID_TYPE;
		} else if (YMIN-1 <= y && y <= YMAX && XMAX < x && x < (COLNO-1)) {
		    levl[x][y].typ = HWALL;
		    levl[x][y].wall_info = 0;
		} else {
		    levl[x][y].typ = STONE;
		    levl[x][y].wall_info = W_NONDIGGABLE | W_NONPASSWALL;
		}
	    }
	}
}

STATIC_OVL void
gtwn_1stpath()
{
	int x,y;
	int dx,dy;
	int c;
	dx = 1; dy = 0;
	x = XMIN;
	y = rn2(YMAX-YMIN)+YMIN;
	while (1) {
	    c = rn2(5)+5;
	    if (dx) c += rn2(4);
	    while (c--) {
		if (inmap(x+dx, y+dy)) {
			x += dx;
			y += dy;
			levl[x][y].typ = CORR;	/* will be replaced to ROOM */
	    	} else break;
	    }
	    if (x >= XMAX-1) break;
	    if (dy) {
		dx = 1;
		dy = 0;
	    } else {
		dx = 0;
		dy = rn2(YMAX) > y ? 1 : -1;
	    }
	}
}

STATIC_OVL void
gtwn_path()
{
	int x,y;
	int i,j;
	for (y=YMIN; y<YMAX; y++) {
	    for (x=XMIN; x<XMAX; x++) {
		if (levl[x][y].typ == INVALID_TYPE)
		    for (i=-1; i<=1; i++) {
			for (j=-1; j<=1; j++) {
			    if (inmap(x+j,y+i) &&
				IS_WALL(levl[x+j][y+i].typ)) {
				levl[x][y].typ = CORR;
				i = j = 9; /* exit loop */
			    }
			}
		    }
	    }
	}
}

STATIC_OVL void
gtwn_findmaxrect(x1, y1, x2, y2, mh, mv)
int *x1;
int *y1;
int *x2;
int *y2;
int mh;
int mv;
{
	int xl, xr, yt, yb;
	int dl, dr, dt, db;
	int i;
	char c;
	xl = *x1; xr = xl;
	yt = *y1; yb = yt;
	dl = dr = dt = db = 1; /* 0:Do not extend  1:Extend  2:Stop extend */
    while (dl || dr || dt || db) {
	/* check top line */
	if (dt && inmap(xl, yt-1)) {
	    for (i=xl; i<=xr; i++) {
		c = levl[i][yt-1].typ;
		if (IS_WALL(c)) dt++;
		else if (c != INVALID_TYPE) {
			dt = 0;
			break;
		}
	    }
	} else dt = 0;
	/* check bottom line */
	if (db && inmap(xl, yb+1)) {
	    for (i=xl; i<=xr; i++) {
		c = levl[i][yb+1].typ;
		if (IS_WALL(c)) db++;
		else if (c != INVALID_TYPE) {
			db = 0;
			break;
		}
	    }
	} else db = 0;
	/* check left line */
	if (dl && inmap(xl-1, yt)) {
	    for (i=yt; i<=yb; i++) {
		c = levl[xl-1][i].typ;
		if (IS_WALL(c)) dl++;
		else if (c != INVALID_TYPE) {
			dl = 0;
			break;
		}
	    }
	} else dl = 0;
	/* check right line */
	if (dr && inmap(xr+1, yt)) {
	    for (i=yt; i<=yb; i++) {
		c = levl[xr+1][i].typ;
		if (IS_WALL(c)) dr++;
		else if (c != INVALID_TYPE) {
			dr = 0;
			break;
		}
	    }
	} else dr = 0;
	/* left top corner */
	if (dl && dt) {
		c = levl[xl-1][yt-1].typ;
		if (!IS_WALL(c) && c!=INVALID_TYPE) {
			if (rn2(100)<50) dt = 0;
			else		 dl = 0;
		}
	}
	/* right top corner */
	if (dr && dt) {
		c = levl[xr+1][yt-1].typ;
		if (!IS_WALL(c) && c!=INVALID_TYPE) {
			if (rn2(100)<50) dt = 0;
			else		 dr = 0;
		}
	}
	/* left bottom corner */
	if (dl && db) {
		c = levl[xl-1][yb+1].typ;
		if (!IS_WALL(c) && c!=INVALID_TYPE) {
			if (rn2(100)<50) db = 0;
			else		 dl = 0;
		}
	}
	/* right bottom corner */
	if (dr && db) {
		c = levl[xr+1][yb+1].typ;
		if (!IS_WALL(c) && c!=INVALID_TYPE) {
			if (rn2(100)<50) db = 0;
			else		 dr = 0;
		}
	}
	/* extend the rect */
	if (dl) xl--;
	if (dr) xr++;
	if (dt) yt--;
	if (db) yb++;
	dl &= 1;
	dr &= 1;
	dt &= 1;
	db &= 1;
	if (xr-xl+1 >= mh) dl = dr = 0;
	if (yb-yt+1 >= mv) dt = db = 0;
    }
    *x1 = xl;
    *y1 = yt;
    *x2 = xr;
    *y2 = yb;
}

STATIC_OVL void
gtwn_drawroom(x1, y1, x2, y2)
int x1;
int y1;
int x2;
int y2;
{
	int i,j;
	char c;
	for (i=x1; i<=x2; i++) {
		c = (i==x1 || i==x2) ? VWALL : ROOM;
		levl[i][y1].typ = HWALL;
		levl[i][y1].horizontal = 1;
		for (j=y1+1; j<=y2-1; j++) {
			levl[i][j].typ = c;
			levl[i][j].horizontal = 0;
		}
		levl[i][y2].typ = HWALL;
		levl[i][y2].horizontal = 1;
	}
}

STATIC_OVL int
gtwn_findcorner()
{
	char cx[256];
	char cy[256];
	int i,j,cnt;
	int x,y;
	int x1,y1,x2,y2;
	int index = 0;
	for (i=YMIN; i<YMAX-1; i++) {
	    for (j=XMIN; j<XMAX-1; j++) {
		cnt = 1;
		if (levl[j  ][i  ].typ == INVALID_TYPE) { cnt--; x=j  ; y=i  ; }
		if (levl[j+1][i  ].typ == INVALID_TYPE) { cnt--; x=j+1; y=i  ; }
		if (levl[j  ][i+1].typ == INVALID_TYPE) { cnt--; x=j  ; y=i+1; }
		if (levl[j+1][i+1].typ == INVALID_TYPE) { cnt--; x=j+1; y=i+1; }
		if (cnt==0) {
			cx[index] = x;
			cy[index] = y;
			index++;
			if (index >= 256) break;
		}
	    if (index >= 256) break;
	    }
	}
	cnt = 0;
	for (i=0; i<index; i++) {
		x1 = cx[i];
		y1 = cy[i];
		if (levl[x1][y1].typ != INVALID_TYPE) continue;
		gtwn_findmaxrect(&x1,&y1,&x2,&y2, 4+rn2(8), 4+rn2(4));
		x = x2-x1+1;
		y = y2-y1+1;
		if (x>=4 && y>=4) {
			gtwn_drawroom(x1,y1,x2,y2);
			add_room(x1+1, y1+1, x2-1, y2-1, 0/*lit*/, OROOM/*rtype*/, 1/*special*/);
			if (nroom < MAXNROFROOMS) cnt++;
			else {
				cnt = 0;
				break;
			}
		}
	}
	return (cnt>0);
}

STATIC_OVL void
gtwn_adddoor()
{
	struct mkroom *croom;
	int i;
	int x,y,dx,dy;
	int trycnt;
	char c;
	for (i=0; i<nroom; i++) {
	    croom = &rooms[i];
	    for (trycnt=100; trycnt>0; trycnt--) {
		switch (rn2(4)) {
		  case 0: /* north */
		    x = croom->lx + rn2(croom->hx - croom->lx + 1);
		    y = croom->ly - 1;
		    dx = 0; dy = -1;
		    break;
		  case 1: /* south */
		    x = croom->lx + rn2(croom->hx - croom->lx + 1);
		    y = croom->hy + 1;
		    dx = 0; dy = 1;
		    break;
		  case 2: /* west */
		    x = croom->lx - 1;
		    y = croom->ly + rn2(croom->hy - croom->ly + 1);
		    dx = -1; dy = 0;
		    break;
		  case 3: /* east */
		    x = croom->hx + 1;
		    y = croom->ly + rn2(croom->hy - croom->ly + 1);
		    dx = 1; dy = 0;
		    break;
		}
		if (!inmap(x+dx,y+dy)) continue;
		c = levl[x+dx][y+dy].typ;
		if (okdoor(x,y) &&
		    (c == ROOM || c == CORR)) {
			dodoor(x,y,croom);
			break;
		}
	    }
	}
}

STATIC_OVL void
gtwn_cleanup()
{
	int x,y;
	char c;
	for (y=YMIN; y<YMAX; y++) {
	    for (x=XMIN; x<XMAX; x++) {
		c = levl[x][y].typ;
		if (c == CORR) {
		    levl[x][y].typ = ROOM;
		} else if (c == ROOM) {
		    levl[x][y].typ = ROOM;
		    if (x < COLNO-4 &&
			 IS_WALL(levl[x+1][y].typ) && IS_WALL(levl[x+2][y].typ) &&
			 levl[x+3][y].typ == ROOM) {
			levl[x+1][y].typ = ICE;
			levl[x+2][y].typ = ICE;
		    }
		    if (y < ROWNO-4 &&
			 IS_WALL(levl[x][y+1].typ) && IS_WALL(levl[x][y+2].typ) &&
			 levl[x][y+3].typ == ROOM) {
			levl[x][y+1].typ = ICE;
			levl[x][y+2].typ = ICE;
		    }
		} else if (c == INVALID_TYPE) {
		    switch (rn2(10)) {
			case 0:	if (inmap(x-1, y) && levl[x-1][y].typ != GRAVE &&
				    inmap(x, y-1) && levl[x][y-1].typ != GRAVE) {
					levl[x][y].typ = GRAVE;
					make_grave(x,y,(char *)0);
				} else levl[x][y].typ = ROOM;
				break;
			case 1:	levl[x][y].typ = DEADTREE;	break;
			default:levl[x][y].typ = ROOM;		break;
		    }
		}
	    }
	}
	for (y=YMIN; y<YMAX; y++) {
	    for (x=XMIN; x<XMAX; x++) {
		if (levl[x][y].typ == ICE) levl[x][y].typ = ROOM;
	    }
	}
}

void mkghosttown(xlimit)
int xlimit;
{
	int i;
	int x, y;
	struct mkroom *croom, *troom;

	XMIN = 2;
	XMAX = (COLNO-1);
	YMIN = 1;
	YMAX = (ROWNO);

	if (xlimit) {
		XMAX = xlimit;
		YMIN = 3;
		YMAX = 20;
	}

	level.flags.is_maze_lev = TRUE;
	level.flags.is_cavernous_lev = FALSE;

	gtwn_initmap();
	gtwn_1stpath();
	if (gtwn_findcorner())
	    if (gtwn_findcorner())
		gtwn_findcorner();
	gtwn_path();
	if (gtwn_findcorner())
	    if (gtwn_findcorner())
		gtwn_findcorner();
	gtwn_path();
	if (gtwn_findcorner())
	    gtwn_findcorner();
	gtwn_path();
	gtwn_adddoor();
	gtwn_cleanup();
	wallification(XMIN, YMIN, XMAX-1, YMAX-1);

	/* break some part of wall */
	for (i = rn1(10, 5); i; i--) {
	    croom = &rooms[rn2(nroom)];
	    if (rn2(2)) {
		x = croom->lx + rn2(croom->hx - croom->lx + 1);
		y = rn2(2) ? croom->ly-1 : croom->hy+1;
	    } else {
		x = rn2(2) ? croom->lx-1 : croom->hx+1;
		y = croom->ly + rn2(croom->hy - croom->ly + 1);
	    }
	    if (IS_WALL(levl[x][y].typ)) {
		dodoor(x, y, croom);
		levl[x][y].typ = DOOR;
		levl[x][y].doormask = D_NODOOR;
		if (rn2(3)) (void) mksobj_at(BOULDER, x, y, TRUE, FALSE);
	    }
	}

	if (xlimit) return;

	/* construct stairs (up and down in different rooms if possible) */
	croom = &rooms[rn2(nroom)];
	mkstairs(somex(croom), somey(croom), 0, croom);	/* down */
	troom = croom;
	croom = &rooms[rn2(nroom-1)];
	if (croom == troom) croom++;
	do {
	    x = somex(croom);
	    y = somey(croom);
	} while(occupied(x, y));
	mkstairs(x, y, 1, croom);	/* up */

	/* Place multi-dungeon branch. */
	place_branch(Is_branchlev(&u.uz), 0, 0);

	/* place morgue */
	for (i = rn2(3); i; i--) mkroom(MORGUE);

	/* objs, traps */
	for(i = rn1(5,6); i; i--) {
	    croom = &rooms[rn2(nroom)];
	    x = croom->lx + rn2(croom->hx - croom->lx + 1);
	    y = croom->ly + rn2(croom->hy - croom->ly + 1);
	    if (!occupied(x, y)) (void) mkobj_at(0, x, y, TRUE);
	}
	for(i = rn1(5,7); i; i--)
	    if (gtwn_rndloc(&x, &y))
		(void) makemon(morguemon(), x, y, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
	for(i = rn1(8,8); i; i--)
	    if (gtwn_rndloc(&x, &y))
		mktrap(0,1,(struct mkroom *) 0, (coord*) 0);
}

/*****************************************************************
   Baalzebub level random map
 *****************************************************************/
static int baal_state = 0;

void mkbaallev(field, stain, xlimit)
char field, stain;
int xlimit;
{
	int i,j;
	int x,y, sx,sy, bx,by;
	int count = 0;

	mkmap_sub(field, stain, 1, 4, 0);
	for (i=0; i<3; i++) {
	    for (j=0; j<5; j++) {
		x = j*(COLNO-2)/5+1;
		y = i*(ROWNO)/3;
		mkrndspace(x+rn2((COLNO-2)/5), y+rn2((ROWNO)/3),
			   COLNO*ROWNO/55, 1, DEADTREE, INVALID_TYPE);
	    }
	}

	/* omit frame area */
	for (x=0; x<COLNO; x++) {
	    levl[x][0].typ = STONE;
	    levl[x][0].wall_info = W_NONDIGGABLE | W_NONPASSWALL;
	    levl[x][ROWNO-1].typ = STONE;
	    levl[x][ROWNO-1].wall_info = W_NONDIGGABLE | W_NONPASSWALL;
	}
	for (y=1; y<ROWNO-1; y++) {
	    levl[1][y].typ = STONE;
	    levl[1][y].wall_info = W_NONDIGGABLE | W_NONPASSWALL;
	    levl[COLNO-1][y].typ = STONE;
	    levl[COLNO-1][y].wall_info = W_NONDIGGABLE | W_NONPASSWALL;
	}

	join_map(DEADTREE, ROOM);

	level.flags.is_maze_lev = TRUE;
	level.flags.is_cavernous_lev = FALSE;

	if (xlimit) return;

	if (IS_STWALL(field) || IS_STWALL(stain))
		wallification(2,1,COLNO-2,ROWNO-2);

	/* make stairs */
	if (!selloc_init()) panic("mkbaallev: cannot place stairs");

	for (i=0; i<ROWNO; i++) {
	    for (j=1; j<COLNO-1; j++) {
		if (levl[j][i].typ == ROOM)
			selloc_store(j,i);
	    }
	}
	selloc_pickrndloc(&bx,&by);
	selloc_pickrndloc(&sx,&sy);
	mkstairs(sx, sy, 1, 0); /* up */
	while (selloc_pickrndloc(&x,&y) && distmin(x,y, sx,sy) < 20);
	mkstairs(x, y, 0, 0);	/* down */

	/* Place multi-dungeon branch. */
	place_branch(Is_branchlev(&u.uz), bx, by);

	selloc_tini();

	/* traps */
	for (i=rn1(10,10); i; i--) {
	    coord pos;
	    for (j=100; j; j--) {
		x = rnd(COLNO-1);
		y = rn2(ROWNO);
		if ((levl[x][y].typ == ROOM || levl[x][y].typ == BOG) &&
		    !t_at(x,y)) {
			levl[x][y].typ = ROOM;
			pos.x = x; pos.y = y;
			mktrap(0, 0, (struct mkroom *) 0, &pos);
			break;
		}
	    }
	}
	/* objs */
	for(i = rn1(5,6); i; i--) {
	    for (j=100; j; j--) {
		x = rnd(COLNO-1);
		y = rn2(ROWNO);
		if (levl[x][y].typ == ROOM && !t_at(x,y)) {
			(void) mkobj_at(0, x, y, TRUE);
			break;
		}
	    }
	}
	/* monsters */
	for (i=rn1(5,15); i; i--) {
	    struct permonst *ptr = select_specific_mon();
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if (t_at(x,y)) continue;
		if (levl[x][y].typ != ROOM) continue;
		(void) makemon(ptr, x, y, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
		break;
	    }
	}
}

/*****************************************************************
   Asmodeus level random map
 *****************************************************************/
void mk_ice_cavern(flr1, flr2, lit, finishwall)
char flr1, flr2;
boolean lit, finishwall;
{
	int i,j;
	int x,y;
	uchar wm;

	mkmap_sub(STONE, ROOM, 1, 0, 1);
	join_map(STONE, ROOM);

	/* remove thin walls(not complete, but sufficient) */
	for (i=1; i<ROWNO-1; i++) {
	    for (j=2; j<COLNO-1; j++) {
		if (levl[j][i].typ == STONE) {
		    wm = 0;
		    if (is_thinwall(j,i))
			levl[j][i].typ = ROOM;
		}
	    }
	}

	for (i=0; i<rn1(5,10); i++) {
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if (levl[x][y].typ == ROOM) {
			mkrndspace(x, y, rn1(10,20), 1, flr1, ROOM);
			break;
		}
	    }
	}
	wallify_map();
	if (finishwall) wallification(1,0,COLNO-1,ROWNO-1);

	/* reverse floor/ice (since wallify_map() assumes space is ROOM) */
	for (i=0; i<ROWNO; i++) {
	    for (j=1; j<COLNO-1; j++) {
		if (levl[j][i].typ == ROOM) levl[j][i].typ = flr1;
		else if (levl[j][i].typ == flr1) levl[j][i].typ = flr2;
	    }
	}

	level.flags.is_maze_lev = FALSE;
	level.flags.is_cavernous_lev = TRUE;
}

STATIC_OVL void
mkasmolev()
{
	int i,j;
	int x,y, sx,sy, bx,by;

	mk_ice_cavern(ICE, ROOM, 0, 1);

	/* make stairs */
	if (!selloc_init()) panic("mk_ice_cavern: cannot place stairs");

	for (i=0; i<ROWNO; i++) {
	    for (j=1; j<COLNO-1; j++) {
		if (levl[j][i].typ == ROOM)
			selloc_store(j,i);
	    }
	}
	selloc_pickrndloc(&bx,&by);
	selloc_pickrndloc(&sx,&sy);
	mkstairs(sx, sy, 1, 0); /* up */
	while (selloc_pickrndloc(&x,&y) && distmin(x,y, sx,sy) < 20);
	mkstairs(x, y, 0, 0);	/* down */

	/* Place multi-dungeon branch. */
	place_branch(Is_branchlev(&u.uz), bx, by);

	selloc_tini();

	/* ice-freezed monsters */
	for (i=0; i<rn1(10,10); i++) {
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if (levl[x][y].typ == ROOM ||
		    levl[x][y].typ == ICE) {
		    switch (rn2(5)) {
			case 0:
			    maketrap(x,y,STATUE_TRAP);
			    break;
			case 1:
			    place_object(mkicefrozenmon(NON_PM), x, y);
			    break;
			default: /* random */
			    place_object(mkicefrozenmon(rn1((PM_WIZARD - PM_ARCHEOLOGIST + 1),
							     PM_ARCHEOLOGIST)), x, y);
			    break;
		    }
		    break;
		}
	    }
	}
	/* objs */
	for (i=rn1(5,5); i; i--) {
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if ((levl[x][y].typ == ROOM || levl[x][y].typ == ICE) &&
		    !t_at(x,y)) {
		    (void) mkobj_at(0, x, y, TRUE);
		    break;
		}
	    }
	}
	/* monsters */
	for (i=rn1(5,7); i; i--) {
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if ((levl[x][y].typ == ROOM || levl[x][y].typ == ICE) &&
		    !t_at(x,y)) {
		    (void) makemon(select_specific_mon(), x, y, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
		    break;
		}
	    }
	}

}

/*****************************************************************
   Juiblex level random map
 *****************************************************************/
STATIC_OVL void
mkjuiblev()
{
	int i,j;
	int x,y, sx,sy, bx,by;
	boolean nearj;
	char slimy[4] = { S_PUDDING, S_BLOB, S_JELLY, S_FUNGUS };

	nearj = (depth(&u.uz) == depth(&juiblex_level)-1);

	mkmap_sub(STONE, ROOM, 2, 2, 1);
	join_map(STONE, ROOM);

	/* put muddy area */
	for (i=0; i<rn1(5,15); i++) {
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if (levl[x][y].typ == ROOM) {
			mkrndspace(x, y, rn1(10,10), 1, BOG, INVALID_TYPE);
			break;
		}
	    }
	}

	for (j=1; j<COLNO-1; j++) {
		levl[j][0].typ = STONE;
		levl[j][0].flags = W_NONDIGGABLE;
		levl[j][ROWNO-1].typ = STONE;
		levl[j][ROWNO-1].flags = W_NONDIGGABLE;
	}
	for (i=1; i<ROWNO-2; i++) {
		levl[1][i].typ = STONE;
		levl[1][i].flags = W_NONDIGGABLE;
		levl[COLNO-1][i].typ = STONE;
		levl[COLNO-1][i].flags = W_NONDIGGABLE;
	}


	/* make river */
	mkriver(INVALID_TYPE, MOAT);
	if (nearj) mkriver(INVALID_TYPE, MOAT);

	/* remove thin walls(not complete, but sufficient) */
	for (i=1; i<ROWNO-1; i++) {
	    for (j=2; j<COLNO-1; j++) {
		if (levl[j][i].typ == STONE) {
		    uchar wm;
		    schar typ;
		    wm = 0;
		    if (is_thinwall(j,i)) {
			typ = levl[j+rn2(3)-1][i+rn2(3)-1].typ;
			if (typ == STONE) typ = ROOM;
			levl[j][i].typ = typ;
		    }
		}
	    }
	}

	wallify_map();
	wallification(1,0,COLNO-1,ROWNO-1);

	refine_mapedge(MOAT);

	/* make stairs */
	if (!selloc_init()) panic("mkjuiblev: cannot place stairs");

	for (i=0; i<ROWNO; i++) {
	    for (j=1; j<COLNO-1; j++) {
		if (levl[j][i].typ == ROOM)
			selloc_store(j,i);
	    }
	}
	selloc_pickrndloc(&bx,&by);
	selloc_pickrndloc(&sx,&sy);
	mkstairs(sx, sy, 1, 0); /* up */
	while (selloc_pickrndloc(&x,&y) && distmin(x,y, sx,sy) < 20);
	mkstairs(x, y, 0, 0);	/* down */

	/* Place multi-dungeon branch. */
	place_branch(Is_branchlev(&u.uz), bx, by);

	selloc_tini();

	/* traps */
	for (i=rn1(5,5); i; i--) {
	    coord pos;
	    for (j=100; j; j--) {
		x = rnd(COLNO-1);
		y = rn2(ROWNO);
		if (levl[x][y].typ == ROOM && !t_at(x,y)) {
			pos.x = x; pos.y = y;
			mktrap(0, 0, (struct mkroom *) 0, &pos);
			break;
		}
	    }
	}
	/* objs */
	for (i=rn1(5,5); i; i--) {
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if (levl[x][y].typ == ROOM && !t_at(x,y)) {
		    (void) mkobj_at(0, x, y, TRUE);
		    break;
		}
	    }
	}
	/* monsters */
	for (i=rn1(5,15); i; i--) {
	    struct permonst *ptr = select_specific_mon();
	    char mc = ptr->mlet;
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if (t_at(x,y)) continue;
		if (mc == S_EEL) {
		    if (levl[x][y].typ != MOAT) continue;
		} else {
		    if (levl[x][y].typ != ROOM && levl[x][y].typ != BOG) continue;
		}
		(void) makemon(ptr, x, y, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
		break;
	    }
	}

	level.flags.is_maze_lev = FALSE;
	level.flags.is_cavernous_lev = TRUE;
	level.flags.shortsighted = TRUE;
}

/* check a part of wall is thin. This routine should be called before wallified */
STATIC_OVL boolean
is_thinwall(x,y)
int x,y;
{
	uchar thinchk[] = {
		0x18, /* vertical thin wall */
		0x42, /* horizontal thin wall */
		0x32, /* L shaped thin wall */
		0x8a, /* counter-clockwize L */
		0x4c,
		0x51,
		0x8c, /* -| */
		0x31, /* |- */
		0x45  /* T */
	};
	uchar wm;
	int i;
	if (levl[x][y].typ == STONE) {
	    wm = 0;
	    if (levl[x-1][y-1].typ != STONE) wm |= 0x80; /* left top */
	    if (levl[x  ][y-1].typ != STONE) wm |= 0x40; /* top */
	    if (levl[x+1][y-1].typ != STONE) wm |= 0x20; /* right top */
	    if (levl[x-1][y  ].typ != STONE) wm |= 0x10; /* left */
	    if (levl[x+1][y  ].typ != STONE) wm |= 0x08; /* right */
	    if (levl[x-1][y+1].typ != STONE) wm |= 0x04; /* left bottom */
	    if (levl[x  ][y+1].typ != STONE) wm |= 0x02; /* bottom */
	    if (levl[x+1][y+1].typ != STONE) wm |= 0x01; /* right bottom */
	    for (i=0; i<sizeof(thinchk); i++) {
		if ((wm & thinchk[i]) == thinchk[i]) return TRUE;
	    }
	}
	return FALSE;
}

STATIC_OVL void
refine_mapedge(bg)
schar bg;
{
	int i,j;
	schar l,r,t,b;
	l = BLCORNER;
	r = BRCORNER;
	for (i=0; i<ROWNO; i+=ROWNO-1) {
	    for (j=1; j<COLNO-1; j++) {
		schar typ;
		typ = levl[j][i].typ;
		if (IS_STWALL(typ)) {
		    if (levl[j-1][i].typ == bg && levl[j+1][i].typ == bg)
			levl[j][i].typ = bg;
		} else if (typ == bg) {
		    if (levl[j-1][i].typ == HWALL) levl[j-1][i].typ = r;
		    if (levl[j+1][i].typ == HWALL) levl[j+1][i].typ = l;
		}
	    }
	    l = TLCORNER;
	    r = TRCORNER;
	}
	t = TRCORNER;
	b = BRCORNER;
	for (j=1; j<COLNO; j+=COLNO-2) {
	    for (i=0; i<ROWNO-1; i++) {
		schar typ;
		typ = levl[j][i].typ;
		if (IS_STWALL(typ)) {
		    if (levl[j][i-1].typ == bg && levl[j][i+1].typ == bg)
			levl[j][i].typ = bg;
		} else if (typ == bg) {
		    if (levl[j][i-1].typ == VWALL) levl[j][i-1].typ = b;
		    if (levl[j][i+1].typ == VWALL) levl[j][i+1].typ = t;
		}
	    }
	    t = TLCORNER;
	    b = BLCORNER;
	}

}

/*****************************************************************
   Hell level random map
 *****************************************************************/
void mkhelllev()
{
	int i,j,k,l;
	int x,y, sx,sy, bx,by;

	mkmap_sub(LAVAPOOL, ROOM, 1, 1, 2);
	join_map(LAVAPOOL, ROOM);

	/*  */
	for (y=0; y<ROWNO; y++)
	    for (x=1; x<COLNO; x++) {
		k = l = 0;
		for (i=-1; i<=1; i++)
		    for (j=-1; j<=1; j++) {
			if (!isok(x+j,y+i)) continue;
			l++;
			if (levl[x+j][y+i].typ == LAVAPOOL) k++;
		    }
		if (k == l) levl[x][y].lit = 1;
	    }
	for (y=1; y<ROWNO-1; y++)
	    for (x=2; x<COLNO-1; x++) {
		if (!levl[x][y].lit) continue;
		k = l = 0;
		for (i=-1; i<=1; i++)
		    for (j=-1; j<=1; j++) {
			l++;
			if (levl[x+j][y+i].lit) k++;
		    }
		if (k<l) levl[x][y].edge = 1;
	    }
	for (y=0; y<ROWNO; y++)
	    for (x=1; x<COLNO; x++)
		if (levl[x][y].lit) {
		    if (!levl[x][y].edge || rn2(5))
			levl[x][y].typ = STONE;
		    levl[x][y].lit = 0;
		    levl[x][y].edge = 0;
		}

	/* remove thin walls(not complete, but sufficient) */
	for (i=1; i<ROWNO-1; i++)
	    for (j=2; j<COLNO-1; j++)
		if (levl[j][i].typ == STONE) {
		    if (is_thinwall(j,i)) levl[j][i].typ = LAVAPOOL;
		}

	wallify_map();
	wallification(1,0,COLNO-1,ROWNO-1);
	refine_mapedge(LAVAPOOL);

	/* lit lava pos */
	for (i=0; i<ROWNO; i++)
	    for (j=1; j<COLNO-1; j++)
		if (levl[j][i].typ == LAVAPOOL)
			levl[j][i].lit = 1;

	level.flags.is_maze_lev = FALSE;
	level.flags.is_cavernous_lev = TRUE;
	level.flags.shortsighted = TRUE;

	/* make stairs */
	if (!selloc_init()) panic("mkhelllev: cannot place stairs");

	for (i=0; i<ROWNO; i++) {
	    for (j=1; j<COLNO-1; j++) {
		if (levl[j][i].typ == ROOM)
			selloc_store(j,i);
	    }
	}
	selloc_pickrndloc(&bx,&by);
	selloc_pickrndloc(&sx,&sy);
	mkstairs(sx, sy, 1, 0); /* up */
	while (selloc_pickrndloc(&x,&y) && distmin(x,y, sx,sy) < 20);
	mkstairs(x, y, 0, 0);	/* down */

	/* Place multi-dungeon branch. */
	place_branch(Is_branchlev(&u.uz), bx, by);

	selloc_tini();

	/* traps */
	for (i=rn1(10,10); i; i--) {
	    coord pos;
	    for (j=100; j; j--) {
		x = rnd(COLNO-1);
		y = rn2(ROWNO);
		if ((levl[x][y].typ == ROOM || levl[x][y].typ == BOG) &&
		    !t_at(x,y)) {
			levl[x][y].typ = ROOM;
			pos.x = x; pos.y = y;
			mktrap(0, 0, (struct mkroom *) 0, &pos);
			break;
		}
	    }
	}
	/* objs */
	for(i = rn1(5,5); i; i--) {
	    for (j=100; j; j--) {
		x = rnd(COLNO-1);
		y = rn2(ROWNO);
		if (levl[x][y].typ == ROOM && !t_at(x,y)) {
			(void) mkobj_at(0, x, y, TRUE);
			break;
		}
	    }
	}
	/* monsters */
	for (i=rn1(5,15); i; i--) {
	    struct permonst *ptr = select_specific_mon();
	    for (j=100; j; j--) {
		x = rn1(COLNO-1,2);
		y = rn2(ROWNO);
		if (t_at(x,y)) continue;
		if (levl[x][y].typ != ROOM) continue;
		(void) makemon(ptr, x, y, MM_MONSTEEDOK/*NO_MM_FLAGS*/);
		break;
	    }
	}
}

/*
 *  subroutines for location selection
 */
#define XYBUFSIZ ((COLNO-1)*ROWNO*2)

static char *xybuf;
static int xynum;

STATIC_OVL boolean
selloc_init()
{
	xynum = 0;
	xybuf = (char *)alloc(XYBUFSIZ + sizeof(int)*2);
	return (xybuf) ? TRUE : FALSE;
}

STATIC_OVL void
selloc_tini()
{
	if (xybuf) free(xybuf);
}

STATIC_OVL boolean
selloc_store(x,y)
int x,y;
{
	if (xynum == (COLNO-1)*ROWNO) return FALSE; /* buffer full */
	xybuf[xynum*2  ] = x;
	xybuf[xynum*2+1] = y;
	xynum++;
	return TRUE;
}

STATIC_OVL boolean
selloc_pickrndloc(x,y)
int *x,*y;
{
	int index;

	if (!xynum) return FALSE; /* buffer empty */

	/* pick a set of random location */
	index = rn2(xynum);
	*x = xybuf[index*2  ];
	*y = xybuf[index*2+1];

	xynum--;

	if (index < xynum) {
		xybuf[index*2  ] = xybuf[xynum*2  ];
		xybuf[index*2+1] = xybuf[xynum*2+1];
	}
	return TRUE;
}

/*mklev.c*/
