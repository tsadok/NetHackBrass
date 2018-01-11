/*	SCCS Id: @(#)region.c	3.4	2002/10/15	*/
/* Copyright (c) 1996 by Jean-Christophe Collet	 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

/*
 * This should really go into the level structure, but
 * I'll start here for ease. It *WILL* move into the level
 * structure eventually.
 */

static NhRegion **regions;
static int n_regions = 0;
static int max_regions = 0;

#define NO_CALLBACK (-1)

boolean FDECL(inside_gas_cloud, (genericptr,genericptr));
boolean FDECL(expire_gas_cloud, (genericptr,genericptr));
boolean FDECL(inside_rect, (NhRect *,int,int));
boolean FDECL(inside_region, (NhRegion *,int,int));
NhRegion *FDECL(create_region, (NhRect *,int));
void FDECL(add_rect_to_reg, (NhRegion *,NhRect *));
void FDECL(add_mon_to_reg, (NhRegion *,struct monst *));
void FDECL(remove_mon_from_reg, (NhRegion *,struct monst *));
boolean FDECL(mon_in_region, (NhRegion *,struct monst *));

#if 0
NhRegion *FDECL(clone_region, (NhRegion *));
#endif
void FDECL(free_region, (NhRegion *));
void FDECL(add_region, (NhRegion *));
void FDECL(remove_region, (NhRegion *));

#if 0
void FDECL(replace_mon_regions, (struct monst *,struct monst *));
void FDECL(remove_mon_from_regions, (struct monst *));
NhRegion *FDECL(create_msg_region, (XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,
				    const char *,const char *));
boolean FDECL(enter_force_field, (genericptr,genericptr));
NhRegion *FDECL(create_force_field, (XCHAR_P,XCHAR_P,int,int));
#endif

static void FDECL(reset_region_mids, (NhRegion *));

static callback_proc callbacks[] = {
#define INSIDE_GAS_CLOUD 0
    inside_gas_cloud,
#define EXPIRE_GAS_CLOUD 1
    expire_gas_cloud
};

/* Should be inlined. */
boolean
inside_rect(r, x, y)
NhRect *r;
int x, y;
{
    return (x >= r->lx && x <= r->hx && y >= r->ly && y <= r->hy);
}

/*
 * Check if a point is inside a region.
 */
boolean
inside_region(reg, x, y)
NhRegion *reg;
int x, y;
{
    int i;

    if (reg == NULL || !inside_rect(&(reg->bounding_box), x, y))
	return FALSE;
    for (i = 0; i < reg->nrects; i++)
	if (inside_rect(&(reg->rects[i]), x, y))
	    return TRUE;
    return FALSE;
}

/*
 * Create a region. It does not activate it.
 */
NhRegion *
create_region(rects, nrect)
NhRect *rects;
int nrect;
{
    int i;
    NhRegion *reg;

    reg = (NhRegion *) alloc(sizeof (NhRegion));
    /* Determines bounding box */
    if (nrect > 0) {
	reg->bounding_box = rects[0];
    } else {
	reg->bounding_box.lx = 99;
	reg->bounding_box.ly = 99;
	reg->bounding_box.hx = 0;
	reg->bounding_box.hy = 0;
    }
    reg->nrects = nrect;
    reg->rects = nrect > 0 ? (NhRect *)alloc((sizeof (NhRect)) * nrect) : NULL;
    for (i = 0; i < nrect; i++) {
	if (rects[i].lx < reg->bounding_box.lx)
	    reg->bounding_box.lx = rects[i].lx;
	if (rects[i].ly < reg->bounding_box.ly)
	    reg->bounding_box.ly = rects[i].ly;
	if (rects[i].hx > reg->bounding_box.hx)
	    reg->bounding_box.hx = rects[i].hx;
	if (rects[i].hy > reg->bounding_box.hy)
	    reg->bounding_box.hy = rects[i].hy;
	reg->rects[i] = rects[i];
    }
    reg->ttl = -1;		/* Defaults */
    reg->attach_2_u = FALSE;
    reg->attach_2_m = 0;
    /* reg->attach_2_o = NULL; */
    reg->enter_msg = NULL;
    reg->leave_msg = NULL;
    reg->expire_f = NO_CALLBACK;
    reg->enter_f = NO_CALLBACK;
    reg->can_enter_f = NO_CALLBACK;
    reg->leave_f = NO_CALLBACK;
    reg->can_leave_f = NO_CALLBACK;
    reg->inside_f = NO_CALLBACK;
    clear_hero_inside(reg);
    clear_heros_fault(reg);
    reg->n_monst = 0;
    reg->max_monst = 0;
    reg->monsters = NULL;
    reg->arg = NULL;
    return reg;
}

/*
 * Add rectangle to region.
 */
void
add_rect_to_reg(reg, rect)
NhRegion *reg;
NhRect *rect;
{
    NhRect *tmp_rect;

    tmp_rect = (NhRect *) alloc(sizeof (NhRect) * (reg->nrects + 1));
    if (reg->nrects > 0) {
	(void) memcpy((genericptr_t) tmp_rect, (genericptr_t) reg->rects,
		      (sizeof (NhRect) * reg->nrects));
	free((genericptr_t) reg->rects);
    }
    tmp_rect[reg->nrects] = *rect;
    reg->nrects++;
    reg->rects = tmp_rect;
    /* Update bounding box if needed */
    if (reg->bounding_box.lx > rect->lx)
	reg->bounding_box.lx = rect->lx;
    if (reg->bounding_box.ly > rect->ly)
	reg->bounding_box.ly = rect->ly;
    if (reg->bounding_box.hx < rect->hx)
	reg->bounding_box.hx = rect->hx;
    if (reg->bounding_box.hy < rect->hy)
	reg->bounding_box.hy = rect->hy;
}

/*
 * Add a monster to the region
 */
void
add_mon_to_reg(reg, mon)
NhRegion *reg;
struct monst *mon;
{
    int i;
    unsigned *tmp_m;

    if (reg->max_monst <= reg->n_monst) {
	tmp_m = (unsigned *)
		    alloc(sizeof (unsigned) * (reg->max_monst + MONST_INC));
	if (reg->max_monst > 0) {
	    for (i = 0; i < reg->max_monst; i++)
		tmp_m[i] = reg->monsters[i];
	    free((genericptr_t) reg->monsters);
	}
	reg->monsters = tmp_m;
	reg->max_monst += MONST_INC;
    }
    reg->monsters[reg->n_monst++] = mon->m_id;
}

/*
 * Remove a monster from the region list (it left or died...)
 */
void
remove_mon_from_reg(reg, mon)
NhRegion *reg;
struct monst *mon;
{
    register int i;

    for (i = 0; i < reg->n_monst; i++)
	if (reg->monsters[i] == mon->m_id) {
	    reg->n_monst--;
	    reg->monsters[i] = reg->monsters[reg->n_monst];
	    return;
	}
}

/*
 * Check if a monster is inside the region.
 * It's probably quicker to check with the region internal list
 * than to check for coordinates.
 */
boolean
mon_in_region(reg, mon)
NhRegion *reg;
struct monst *mon;
{
    int i;

    for (i = 0; i < reg->n_monst; i++)
	if (reg->monsters[i] == mon->m_id)
	    return TRUE;
    return FALSE;
}

#if 0
/* not yet used */

/*
 * Clone (make a standalone copy) the region.
 */
NhRegion *
clone_region(reg)
NhRegion *reg;
{
    NhRegion *ret_reg;

    ret_reg = create_region(reg->rects, reg->nrects);
    ret_reg->ttl = reg->ttl;
    ret_reg->attach_2_u = reg->attach_2_u;
    ret_reg->attach_2_m = reg->attach_2_m;
 /* ret_reg->attach_2_o = reg->attach_2_o; */
    ret_reg->expire_f = reg->expire_f;
    ret_reg->enter_f = reg->enter_f;
    ret_reg->can_enter_f = reg->can_enter_f;
    ret_reg->leave_f = reg->leave_f;
    ret_reg->can_leave_f = reg->can_leave_f;
    ret_reg->player_flags = reg->player_flags;	/* set/clear_hero_inside,&c*/
    ret_reg->n_monst = reg->n_monst;
    if (reg->n_monst > 0) {
	ret_reg->monsters = (unsigned *)
				alloc((sizeof (unsigned)) * reg->n_monst);
	(void) memcpy((genericptr_t) ret_reg->monsters, (genericptr_t) reg->monsters,
		      sizeof (unsigned) * reg->n_monst);
    } else
	ret_reg->monsters = NULL;
    return ret_reg;
}

#endif	/*0*/

/*
 * Free mem from region.
 */
void
free_region(reg)
NhRegion *reg;
{
    if (reg) {
	if (reg->rects)
	    free((genericptr_t) reg->rects);
	if (reg->monsters)
	    free((genericptr_t) reg->monsters);
	free((genericptr_t) reg);
    }
}

/*
 * Add a region to the list.
 * This actually activates the region.
 */
void
add_region(reg)
NhRegion *reg;
{
    NhRegion **tmp_reg;
    int i, j;

    if (max_regions <= n_regions) {
	tmp_reg = regions;
	regions = (NhRegion **)alloc(sizeof (NhRegion *) * (max_regions + 10));
	if (max_regions > 0) {
	    (void) memcpy((genericptr_t) regions, (genericptr_t) tmp_reg,
			  max_regions * sizeof (NhRegion *));
	    free((genericptr_t) tmp_reg);
	}
	max_regions += 10;
    }
    regions[n_regions] = reg;
    n_regions++;
    /* Check for monsters inside the region */
    for (i = reg->bounding_box.lx; i <= reg->bounding_box.hx; i++)
	for (j = reg->bounding_box.ly; j <= reg->bounding_box.hy; j++) {
	    /* Some regions can cross the level boundaries */
	    if (!isok(i,j))
		continue;
	    if (MON_AT(i, j) && inside_region(reg, i, j))
		add_mon_to_reg(reg, level.monsters[i][j]);
	    if (reg->visible && cansee(i, j))
		newsym(i, j);
	}
    /* Check for player now... */
    if (inside_region(reg, u.ux, u.uy)) 
	set_hero_inside(reg);
    else
	clear_hero_inside(reg);
}

/*
 * Remove a region from the list & free it.
 */
void
remove_region(reg)
NhRegion *reg;
{
    register int i, x, y;

    for (i = 0; i < n_regions; i++)
	if (regions[i] == reg)
	    break;
    if (i == n_regions)
	return;

    /* Update screen if necessary */
    if (reg->visible)
	for (x = reg->bounding_box.lx; x <= reg->bounding_box.hx; x++)
	    for (y = reg->bounding_box.ly; y <= reg->bounding_box.hy; y++)
		if (isok(x,y) && inside_region(reg, x, y) && cansee(x, y))
		    newsym(x, y);

    free_region(reg);
    regions[i] = regions[n_regions - 1];
    regions[n_regions - 1] = (NhRegion *) 0;
    n_regions--;
}

/*
 * Remove all regions and clear all related data (This must be down
 * when changing level, for instance).
 */
void
clear_regions()
{
    register int i;

    for (i = 0; i < n_regions; i++)
	free_region(regions[i]);
    n_regions = 0;
    if (max_regions > 0)
	free((genericptr_t) regions);
    max_regions = 0;
    regions = NULL;
}

/*
 * This function is called every turn.
 * It makes the regions age, if necessary and calls the appropriate
 * callbacks when needed.
 */
void
run_regions()
{
    register int i, j, k;
    int f_indx;

    /* End of life ? */
    /* Do it backward because the array will be modified */
    for (i = n_regions - 1; i >= 0; i--) {
	if (regions[i]->ttl == 0) {
	    if ((f_indx = regions[i]->expire_f) == NO_CALLBACK ||
		(*callbacks[f_indx])(regions[i], (genericptr_t) 0))
		remove_region(regions[i]);
	}
    }

    /* Process remaining regions */
    for (i = 0; i < n_regions; i++) {
	/* Make the region age */
	if (regions[i]->ttl > 0)
	    regions[i]->ttl--;
	/* Check if player is inside region */
	f_indx = regions[i]->inside_f;
	if (f_indx != NO_CALLBACK && hero_inside(regions[i]))
	    (void) (*callbacks[f_indx])(regions[i], (genericptr_t) 0);
	/* Check if any monster is inside region */
	if (f_indx != NO_CALLBACK) {
	    for (j = 0; j < regions[i]->n_monst; j++) {
		struct monst *mtmp = find_mid(regions[i]->monsters[j], FM_FMON);

		if (!mtmp || mtmp->mhp <= 0 ||
				(*callbacks[f_indx])(regions[i], mtmp)) {
		    /* The monster died, remove it from list */
		    k = (regions[i]->n_monst -= 1);
		    regions[i]->monsters[j] = regions[i]->monsters[k];
		    regions[i]->monsters[k] = 0;
		    --j;    /* current slot has been reused; recheck it next */
		}
	    }
	}
    }
}

/*
 * check whether player enters/leaves one or more regions.
 */
boolean
in_out_region(x, y)
xchar
    x, y;
{
    int i, f_indx;

    /* First check if we can do the move */
    for (i = 0; i < n_regions; i++) {
	if (inside_region(regions[i], x, y)
	    && !hero_inside(regions[i]) && !regions[i]->attach_2_u) {
	    if ((f_indx = regions[i]->can_enter_f) != NO_CALLBACK)
		if (!(*callbacks[f_indx])(regions[i], (genericptr_t) 0))
		    return FALSE;
	} else
	    if (hero_inside(regions[i])
		&& !inside_region(regions[i], x, y)
		&& !regions[i]->attach_2_u) {
	    if ((f_indx = regions[i]->can_leave_f) != NO_CALLBACK)
		if (!(*callbacks[f_indx])(regions[i], (genericptr_t) 0))
		    return FALSE;
	}
    }

    /* Callbacks for the regions we do leave */
    for (i = 0; i < n_regions; i++)
	if (hero_inside(regions[i]) &&
		!regions[i]->attach_2_u && !inside_region(regions[i], x, y)) {
	    clear_hero_inside(regions[i]);
	    if (regions[i]->leave_msg != NULL)
		pline(regions[i]->leave_msg);
	    if ((f_indx = regions[i]->leave_f) != NO_CALLBACK)
		(void) (*callbacks[f_indx])(regions[i], (genericptr_t) 0);
	}

    /* Callbacks for the regions we do enter */
    for (i = 0; i < n_regions; i++)
	if (!hero_inside(regions[i]) &&
		!regions[i]->attach_2_u && inside_region(regions[i], x, y)) {
	    set_hero_inside(regions[i]);
	    if (regions[i]->enter_msg != NULL)
		pline(regions[i]->enter_msg);
	    if ((f_indx = regions[i]->enter_f) != NO_CALLBACK)
		(void) (*callbacks[f_indx])(regions[i], (genericptr_t) 0);
	}
    return TRUE;
}

/*
 * check wether a monster enters/leaves one or more region.
*/
boolean
m_in_out_region(mon, x, y)
struct monst *mon;
xchar x, y;
{
    int i, f_indx;

    /* First check if we can do the move */
    for (i = 0; i < n_regions; i++) {
	if (inside_region(regions[i], x, y) &&
		!mon_in_region(regions[i], mon) &&
		regions[i]->attach_2_m != mon->m_id) {
	    if ((f_indx = regions[i]->can_enter_f) != NO_CALLBACK)
		if (!(*callbacks[f_indx])(regions[i], mon))
		    return FALSE;
	} else if (mon_in_region(regions[i], mon) &&
		!inside_region(regions[i], x, y) &&
		regions[i]->attach_2_m != mon->m_id) {
	    if ((f_indx = regions[i]->can_leave_f) != NO_CALLBACK)
		if (!(*callbacks[f_indx])(regions[i], mon))
		    return FALSE;
	}
    }

    /* Callbacks for the regions we do leave */
    for (i = 0; i < n_regions; i++)
	if (mon_in_region(regions[i], mon) &&
		regions[i]->attach_2_m != mon->m_id &&
		!inside_region(regions[i], x, y)) {
	    remove_mon_from_reg(regions[i], mon);
	    if ((f_indx = regions[i]->leave_f) != NO_CALLBACK)
		(void) (*callbacks[f_indx])(regions[i], mon);
	}

    /* Callbacks for the regions we do enter */
    for (i = 0; i < n_regions; i++)
	if (!hero_inside(regions[i]) &&
		!regions[i]->attach_2_u && inside_region(regions[i], x, y)) {
	    add_mon_to_reg(regions[i], mon);
	    if ((f_indx = regions[i]->enter_f) != NO_CALLBACK)
		(void) (*callbacks[f_indx])(regions[i], mon);
	}
    return TRUE;
}

/*
 * Checks player's regions after a teleport for instance.
 */
void
update_player_regions()
{
    register int i;

    for (i = 0; i < n_regions; i++)
	if (!regions[i]->attach_2_u && inside_region(regions[i], u.ux, u.uy))
	    set_hero_inside(regions[i]);
	else
	    clear_hero_inside(regions[i]);
}

/*
 * Ditto for a specified monster.
 */
void
update_monster_region(mon)
struct monst *mon;
{
    register int i;

    for (i = 0; i < n_regions; i++) {
	if (inside_region(regions[i], mon->mx, mon->my)) {
	    if (!mon_in_region(regions[i], mon))
		add_mon_to_reg(regions[i], mon);
	} else {
	    if (mon_in_region(regions[i], mon))
		remove_mon_from_reg(regions[i], mon);
	}
    }
}

#if 0
/* not yet used */

/*
 * Change monster pointer in regions
 * This happens, for instance, when a monster grows and
 * need a new structure (internally that is).
 */
void
replace_mon_regions(monold, monnew)
struct monst *monold, *monnew;
{
    register int i;

    for (i = 0; i < n_regions; i++)
	if (mon_in_region(regions[i], monold)) {
	    remove_mon_from_reg(regions[i], monold);
	    add_mon_to_reg(regions[i], monnew);
	}
}

/*
 * Remove monster from all regions it was in (ie monster just died)
 */
void
remove_mon_from_regions(mon)
struct monst *mon;
{
    register int i;

    for (i = 0; i < n_regions; i++)
	if (mon_in_region(regions[i], mon))
	    remove_mon_from_reg(regions[i], mon);
}

#endif	/*0*/

/*
 * Check if a spot is under a visible region (eg: gas cloud).
 * Returns NULL if not, otherwise returns region.
 */
NhRegion *
visible_region_at(x, y)
xchar x, y;
{
    register int i;

    for (i = 0; i < n_regions; i++)
	if (inside_region(regions[i], x, y) && regions[i]->visible &&
		regions[i]->ttl != 0)
	    return regions[i];
    return (NhRegion *) 0;
}

void
show_region(reg, x, y)
NhRegion *reg;
xchar x, y;
{
    show_glyph(x, y, reg->glyph);
}

/*
 * save_regions :
 */
void
save_regions(fd, mode)
int fd;
int mode;
{
    int i, j;
    unsigned n;

    if (!perform_bwrite(mode)) goto skip_lots;

    bwrite(fd, (genericptr_t) &moves, sizeof (moves));	/* timestamp */
    bwrite(fd, (genericptr_t) &n_regions, sizeof (n_regions));
    for (i = 0; i < n_regions; i++) {
	bwrite(fd, (genericptr_t) &regions[i]->bounding_box, sizeof (NhRect));
	bwrite(fd, (genericptr_t) &regions[i]->nrects, sizeof (short));
	for (j = 0; j < regions[i]->nrects; j++)
	    bwrite(fd, (genericptr_t) &regions[i]->rects[j], sizeof (NhRect));
	bwrite(fd, (genericptr_t) &regions[i]->attach_2_u, sizeof (boolean));
	n = 0;
	bwrite(fd, (genericptr_t) &regions[i]->attach_2_m, sizeof (unsigned));
	n = regions[i]->enter_msg != NULL ? strlen(regions[i]->enter_msg) : 0;
	bwrite(fd, (genericptr_t) &n, sizeof n);
	if (n > 0)
	    bwrite(fd, (genericptr_t) regions[i]->enter_msg, n);
	n = regions[i]->leave_msg != NULL ? strlen(regions[i]->leave_msg) : 0;
	bwrite(fd, (genericptr_t) &n, sizeof n);
	if (n > 0)
	    bwrite(fd, (genericptr_t) regions[i]->leave_msg, n);
	bwrite(fd, (genericptr_t) &regions[i]->ttl, sizeof (short));
	bwrite(fd, (genericptr_t) &regions[i]->expire_f, sizeof (short));
	bwrite(fd, (genericptr_t) &regions[i]->can_enter_f, sizeof (short));
	bwrite(fd, (genericptr_t) &regions[i]->enter_f, sizeof (short));
	bwrite(fd, (genericptr_t) &regions[i]->can_leave_f, sizeof (short));
	bwrite(fd, (genericptr_t) &regions[i]->leave_f, sizeof (short));
	bwrite(fd, (genericptr_t) &regions[i]->inside_f, sizeof (short));
	bwrite(fd, (genericptr_t) &regions[i]->player_flags, sizeof (boolean));
	bwrite(fd, (genericptr_t) &regions[i]->n_monst, sizeof (short));
	for (j = 0; j < regions[i]->n_monst; j++)
	    bwrite(fd, (genericptr_t) &regions[i]->monsters[j],
	     sizeof (unsigned));
	bwrite(fd, (genericptr_t) &regions[i]->visible, sizeof (boolean));
	bwrite(fd, (genericptr_t) &regions[i]->glyph, sizeof (int));
	bwrite(fd, (genericptr_t) &regions[i]->arg, sizeof (genericptr_t));
    }

skip_lots:
    if (release_data(mode))
	clear_regions();
}

void
rest_regions(fd, ghostly)
int fd;
boolean ghostly; /* If a bones file restore */
{
    int i, j;
    unsigned n;
    long tmstamp;
    char *msg_buf;

    clear_regions();		/* Just for security */
    mread(fd, (genericptr_t) &tmstamp, sizeof (tmstamp));
    if (ghostly) tmstamp = 0;
    else tmstamp = (moves - tmstamp);
    mread(fd, (genericptr_t) &n_regions, sizeof (n_regions));
    max_regions = n_regions;
    if (n_regions > 0)
	regions = (NhRegion **) alloc(sizeof (NhRegion *) * n_regions);
    for (i = 0; i < n_regions; i++) {
	regions[i] = (NhRegion *) alloc(sizeof (NhRegion));
	mread(fd, (genericptr_t) &regions[i]->bounding_box, sizeof (NhRect));
	mread(fd, (genericptr_t) &regions[i]->nrects, sizeof (short));

	if (regions[i]->nrects > 0)
	    regions[i]->rects = (NhRect *)
				  alloc(sizeof (NhRect) * regions[i]->nrects);
	for (j = 0; j < regions[i]->nrects; j++)
	    mread(fd, (genericptr_t) &regions[i]->rects[j], sizeof (NhRect));
	mread(fd, (genericptr_t) &regions[i]->attach_2_u, sizeof (boolean));
	mread(fd, (genericptr_t) &regions[i]->attach_2_m, sizeof (unsigned));

	mread(fd, (genericptr_t) &n, sizeof n);
	if (n > 0) {
	    msg_buf = (char *) alloc(n + 1);
	    mread(fd, (genericptr_t) msg_buf, n);
	    msg_buf[n] = '\0';
	    regions[i]->enter_msg = (const char *) msg_buf;
	} else
	    regions[i]->enter_msg = NULL;

	mread(fd, (genericptr_t) &n, sizeof n);
	if (n > 0) {
	    msg_buf = (char *) alloc(n + 1);
	    mread(fd, (genericptr_t) msg_buf, n);
	    msg_buf[n] = '\0';
	    regions[i]->leave_msg = (const char *) msg_buf;
	} else
	    regions[i]->leave_msg = NULL;

	mread(fd, (genericptr_t) &regions[i]->ttl, sizeof (short));
	/* check for expired region */
	if (regions[i]->ttl >= 0)
	    regions[i]->ttl =
		(regions[i]->ttl > tmstamp) ? regions[i]->ttl - tmstamp : 0;
	mread(fd, (genericptr_t) &regions[i]->expire_f, sizeof (short));
	mread(fd, (genericptr_t) &regions[i]->can_enter_f, sizeof (short));
	mread(fd, (genericptr_t) &regions[i]->enter_f, sizeof (short));
	mread(fd, (genericptr_t) &regions[i]->can_leave_f, sizeof (short));
	mread(fd, (genericptr_t) &regions[i]->leave_f, sizeof (short));
	mread(fd, (genericptr_t) &regions[i]->inside_f, sizeof (short));
	mread(fd, (genericptr_t) &regions[i]->player_flags, sizeof (boolean));
	if (ghostly) {	/* settings pertained to old player */
	    clear_hero_inside(regions[i]);
	    clear_heros_fault(regions[i]);
	}
	mread(fd, (genericptr_t) &regions[i]->n_monst, sizeof (short));
	if (regions[i]->n_monst > 0)
	    regions[i]->monsters =
		(unsigned *) alloc(sizeof (unsigned) * regions[i]->n_monst);
	else
	    regions[i]->monsters = NULL;
	regions[i]->max_monst = regions[i]->n_monst;
	for (j = 0; j < regions[i]->n_monst; j++)
	    mread(fd, (genericptr_t) &regions[i]->monsters[j],
		  sizeof (unsigned));
	mread(fd, (genericptr_t) &regions[i]->visible, sizeof (boolean));
	mread(fd, (genericptr_t) &regions[i]->glyph, sizeof (int));
	mread(fd, (genericptr_t) &regions[i]->arg, sizeof (genericptr_t));
    }
    /* remove expired regions, do not trigger the expire_f callback (yet!);
       also update monster lists if this data is coming from a bones file */
    for (i = n_regions - 1; i >= 0; i--)
	if (regions[i]->ttl == 0)
	    remove_region(regions[i]);
	else if (ghostly && regions[i]->n_monst > 0)
	    reset_region_mids(regions[i]);
}

/* update monster IDs for region being loaded from bones; `ghostly' implied */
static void
reset_region_mids(reg)
NhRegion *reg;
{
    int i = 0, n = reg->n_monst;
    unsigned *mid_list = reg->monsters;

    while (i < n)
	if (!lookup_id_mapping(mid_list[i], &mid_list[i])) {
	    /* shrink list to remove missing monster; order doesn't matter */
	    mid_list[i] = mid_list[--n];
	} else {
	    /* move on to next monster */
	    ++i;
	}
    reg->n_monst = n;
    return;
}

#if 0
/* not yet used */

/*--------------------------------------------------------------*
 *								*
 *			Create Region with just a message	*
 *								*
 *--------------------------------------------------------------*/

NhRegion *
create_msg_region(x, y, w, h, msg_enter, msg_leave)
xchar x, y;
xchar w, h;
const char *msg_enter;
const char *msg_leave;
{
    NhRect tmprect;
    NhRegion *reg = create_region((NhRect *) 0, 0);

    reg->enter_msg = msg_enter;
    reg->leave_msg = msg_leave;
    tmprect.lx = x;
    tmprect.ly = y;
    tmprect.hx = x + w;
    tmprect.hy = y + h;
    add_rect_to_reg(reg, &tmprect);
    reg->ttl = -1;
    return reg;
}


/*--------------------------------------------------------------*
 *								*
 *			Force Field Related Code		*
 *			(unused yet)				*
 *--------------------------------------------------------------*/

boolean
enter_force_field(p1, p2)
genericptr_t p1;
genericptr_t p2;
{
    struct monst *mtmp;

    if (p2 == NULL) {		/* That means the player */
	if (!Blind)
		You("bump into %s. Ouch!",
		    Hallucination ? "an invisible tree" :
			"some kind of invisible wall");
	else
	    pline("Ouch!");
    } else {
	mtmp = (struct monst *) p2;
	if (canseemon(mtmp))
	    pline("%s bumps into %s!", Monnam(mtmp), something);
    }
    return FALSE;
}

NhRegion *
create_force_field(x, y, radius, ttl)
xchar x, y;
int radius, ttl;
{
    int i;
    NhRegion *ff;
    int nrect;
    NhRect tmprect;

    ff = create_region((NhRect *) 0, 0);
    nrect = radius;
    tmprect.lx = x;
    tmprect.hx = x;
    tmprect.ly = y - (radius - 1);
    tmprect.hy = y + (radius - 1);
    for (i = 0; i < nrect; i++) {
	add_rect_to_reg(ff, &tmprect);
	tmprect.lx--;
	tmprect.hx++;
	tmprect.ly++;
	tmprect.hy--;
    }
    ff->ttl = ttl;
    if (!in_mklev && !flags.mon_moving)
	set_heros_fault(ff);		/* assume player has created it */
 /* ff->can_enter_f = enter_force_field; */
 /* ff->can_leave_f = enter_force_field; */
    add_region(ff);
    return ff;
}

#endif	/*0*/

/*--------------------------------------------------------------*
 *								*
 *			Gas cloud related code			*
 *								*
 *--------------------------------------------------------------*/

/*
 * Here is an example of an expire function that may prolong
 * region life after some mods...
 */
boolean
expire_gas_cloud(p1, p2)
genericptr_t p1;
genericptr_t p2;
{
    NhRegion *reg;
    int damage;

    reg = (NhRegion *) p1;
    damage = (int) reg->arg;

    /* If it was a thick cloud, it dissipates a little first */
    if (damage >= 5) {
	damage /= 2;		/* It dissipates, let's do less damage */
	reg->arg = (genericptr_t) damage;
	reg->ttl = 2;		/* Here's the trick : reset ttl */
	return FALSE;		/* THEN return FALSE, means "still there" */
    }
    return TRUE;		/* OK, it's gone, you can free it! */
}

boolean
inside_gas_cloud(p1, p2)
genericptr_t p1;
genericptr_t p2;
{
    NhRegion *reg;
    struct monst *mtmp;
    int dam;

    reg = (NhRegion *) p1;
    dam = (int) reg->arg;
    if (p2 == NULL) {		/* This means *YOU* Bozo! */
	if (nonliving(youmonst.data) || Breathless)
	    return FALSE;
	if (!Blind)
	    make_blinded(1L, FALSE);
	if (!Poison_resistance) {
	    pline("%s is burning your %s!", Something, makeplural(body_part(LUNG)));
	    You("cough and spit blood!");
	    losehp(rnd(dam) + 5, "gas cloud", KILLED_BY_AN);
	    return FALSE;
	} else {
	    You("cough!");
	    return FALSE;
	}
    } else {			/* A monster is inside the cloud */
	mtmp = (struct monst *) p2;

	/* Non living and non breathing monsters are not concerned */
	if (!nonliving(mtmp->data) && !breathless(mtmp->data)) {
	    if (cansee(mtmp->mx, mtmp->my))
		pline("%s coughs!", Monnam(mtmp));
	    setmangry(mtmp);
	    if (haseyes(mtmp->data) && mtmp->mcansee) {
		mtmp->mblinded = 1;
		mtmp->mcansee = 0;
	    }
	    if (resists_poison(mtmp))
		return FALSE;
	    mtmp->mhp -= rnd(dam) + 5;
	    if (mtmp->mhp <= 0) {
		if (heros_fault(reg))
		    killed(mtmp);
		else
		    monkilled(mtmp, "gas cloud", AD_DRST);
		if (mtmp->mhp <= 0) {	/* not lifesaved */
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;		/* Monster is still alive */
}

NhRegion *
create_gas_cloud(x, y, radius, damage)
xchar x, y;
int radius;
int damage;
{
    NhRegion *cloud;
    int i, nrect;
    NhRect tmprect;

    cloud = create_region((NhRect *) 0, 0);
    nrect = radius;
    tmprect.lx = x;
    tmprect.hx = x;
    tmprect.ly = y - (radius - 1);
    tmprect.hy = y + (radius - 1);
    for (i = 0; i < nrect; i++) {
	add_rect_to_reg(cloud, &tmprect);
	tmprect.lx--;
	tmprect.hx++;
	tmprect.ly++;
	tmprect.hy--;
    }
    cloud->ttl = rn1(3,4);
    if (!in_mklev && !flags.mon_moving)
	set_heros_fault(cloud);		/* assume player has created it */
    cloud->inside_f = INSIDE_GAS_CLOUD;
    cloud->expire_f = EXPIRE_GAS_CLOUD;
    cloud->arg = (genericptr_t) damage;
    cloud->visible = TRUE;
    cloud->glyph = cmap_to_glyph(S_cloud);
    add_region(cloud);
    return cloud;
}

/* ---------------------------- *
 *  alternative gas cloud code  *
 * ---------------------------- */
STATIC_DCL struct cloud *NDECL(newcloud);
STATIC_DCL void FDECL(dealloc_cloud, (struct cloud *));
STATIC_DCL void FDECL(place_cloud, (struct cloud *, int, int));
STATIC_DCL void FDECL(remove_cloud, (struct cloud *));
STATIC_DCL void FDECL(diffuse_cloud, (int, int, int, int));
STATIC_DCL void FDECL(update_cloud_attrib, (struct cloud *));
STATIC_DCL struct cloud *FDECL(cloud_at_i, (int,int,int,int));

struct cloud *
newcloud(void)
{
    struct cloud *ctmp;
    ctmp = (struct cloud *)alloc(sizeof(struct cloud));
    ctmp->cx = ctmp->cy = 0;
    ctmp->ncloud = 0;
    ctmp->nchere = 0;
    return ctmp;
}
void
dealloc_cloud(ctmp)
struct cloud *ctmp;
{
    if (ctmp->cx) remove_cloud(ctmp);
    free((genericptr_t) ctmp);
}

void
place_cloud(ctmp, x, y)
struct cloud *ctmp;
int x;
int y;
{
    struct cloud *ctmp2, **cchn;
    if (x) {
	ctmp->cx = x;
	ctmp->cy = y;
	ctmp->ncloud = level.cloudlist;
	level.cloudlist = ctmp;
    } else {
	/* if x==0, only update level.clouds[][] for restoring */
	x = ctmp->cx;
	y = ctmp->cy;
    }
    cchn = &(level.clouds[x][y]);
    ctmp2 = level.clouds[x][y];
    while (ctmp2 && ctmp->hlev < ctmp2->hlev) {
	cchn = &(ctmp2->nchere);
	ctmp2 = ctmp2->nchere;
    }
    *cchn = ctmp;
    ctmp->nchere = ctmp2;
}

void
remove_cloud(ctmp)
struct cloud *ctmp;
{
    struct cloud **cchn;
    cchn = &(level.cloudlist);
    while (ctmp != *cchn)
	cchn = &((*cchn)->ncloud);
    if (*cchn) {
	*cchn = ctmp->ncloud;
    } else {
	impossible("remove_cloud: Not in level.cloudlist???");
	return;
    }
    cchn = &(level.clouds[ctmp->cx][ctmp->cy]);
    while (ctmp != *cchn)
	cchn = &((*cchn)->nchere);
    if (*cchn) {
	*cchn = ctmp->nchere;
    } else {
	impossible("remove_cloud: Not in level.clouds[x][y]???");
	return;
    }
    ctmp->cx = 0;
    ctmp->cy = 0;
}

struct cloud *
cloud_at_i(x, y, typ, init)
int x, y, typ;
{
    struct cloud *ctmp;
    for (ctmp = level.clouds[x][y]; ctmp; ctmp = ctmp->nchere)
	if (!typ ||
	    ((ctmp->ctyp == typ) && (ctmp->initial == init))) return ctmp;
    return (struct cloud *)0;
}

struct cloud *
cloud_at(x, y, typ)
int x, y, typ;
{
    return cloud_at_i(x, y, typ, 0);
}

boolean
show_cloud(x, y, heightlev)
int x;
int y;
int heightlev;
{
    struct cloud *ctmp;
    for (ctmp = level.clouds[x][y]; ctmp; ctmp = ctmp->nchere) {
	if (ctmp->hlev == heightlev && ctmp->visible) {
	    show_glyph(x, y, ctmp->glyph);
	    if (level.flags.hero_memory)
		levl[x][y].glyph = ctmp->glyph;
	    return TRUE;
	}
    }
    return FALSE;
}

void
update_cloud_attrib(ctmp)
struct cloud *ctmp;
{
    if (!ctmp) return;
    switch (ctmp->ctyp) {
	case STINKING_CLOUD:
	    if (ctmp->density > 5) {
		ctmp->glyph = GLYPH_CLOUD_OFF + S_densepcloud - S_thinpcloud;
		ctmp->doesblock = 0;
	    } else {
		ctmp->glyph = GLYPH_CLOUD_OFF + S_thinpcloud - S_thinpcloud;
		ctmp->doesblock = 0;
	    }
	    break;
	default:
	    break;
    }
}

/*
    diffuse gas cloud.
    if (during_init == 1) then diffusion is separately done from existing clouds.
    if (during_init >  1) then this is the last initial diffusion.
 */
void
diffuse_cloud(typ, th, centerwt, during_init)
int typ, th, centerwt, during_init;
{
    struct cloud *ctmp, *ctmp2, *ctmp3;
    int i, j;
    int dif, d, dd;
    int x, y;
    int cleanup_init;
    schar ltyp;
    cleanup_init = (during_init > 1);
    during_init = !!during_init;
    for (ctmp = level.cloudlist; ctmp; ctmp = ctmp->ncloud)
	if (ctmp->ctyp == typ) ctmp->tmp = 0;
    for (ctmp = level.cloudlist; ctmp; ctmp = ctmp->ncloud) {
	if (ctmp->ctyp != typ) continue;
	dif = ctmp->density - th;
	if (dif <= 0) continue;
	/* check positions to diffuse */
	dd = 0;
	for (i=0; i<8; i++) {
	    x = ctmp->cx + xdir[i];
	    y = ctmp->cy + ydir[i];
	    ltyp = levl[x][y].typ;
	    if (isok(x, y) && (!IS_ROCK(ltyp) || IS_TREES(ltyp)) && ltyp != DRAWBRIDGE_UP &&
		(ltyp != DOOR || !(levl[x][y].doormask & (D_CLOSED | D_LOCKED | D_TRAPPED)))) {
		dd += (!xdir[i] || !ydir[i]) ? 10 : 7;
		ctmp2 = cloud_at_i(x, y, typ, during_init);
		if (!ctmp2) {
		    ctmp2 = newcloud();
		    *ctmp2 = *ctmp;
		    ctmp2->ncloud = ctmp2->nchere = 0;
		    ctmp2->density = 0;
		    ctmp2->tmp = 0;
		    ctmp2->initial = during_init;
		    place_cloud(ctmp2, x, y);
		}
	    }
	}
	if (dd == 0) continue; /* no space to diffuse */
	dd += centerwt;
	/* diffuse */
	for (i=0; i<8; i++) {
	    x = ctmp->cx + xdir[i];
	    y = ctmp->cy + ydir[i];
	    ctmp2 = cloud_at_i(x, y, typ, during_init);
	    if (!ctmp2) continue;
	    d = (!xdir[i] || !ydir[i]) ? ((dif * 10) / dd) : ((dif *  7) / dd);
	    if (d > dif) d = dif;
	    dif -= d;
	    if (dif && rn2(2)) { d++; dif--; }
	    ctmp2->tmp += d;
	}
	ctmp->density = th + dif;
    }
    /* cleanup */
    for (ctmp = level.cloudlist; ctmp; ctmp = ctmp2) {
	ctmp2 = ctmp->ncloud;
	if (ctmp->ctyp == typ && ctmp->initial == during_init) {
	    x = ctmp->cx;
	    y = ctmp->cy;
	    ctmp->density += ctmp->tmp - 1;
	    if (cleanup_init) {
		/* check if the same type of cloud is here */
		ctmp3 = cloud_at_i(x, y, typ, 0);
		if (ctmp3) {
		    /* if exist, merge into existing cloud */
		    ctmp3->density += ctmp->density;
		    ctmp->density = 0; /* initial cloud is removed */
		    update_cloud_attrib(ctmp3);
		} else {
		    /* if not exist, just clear the initial flag */
		    ctmp->initial = 0;
		}
	    }
	    if (ctmp->density <= 0) {
		remove_cloud(ctmp);
		dealloc_cloud(ctmp);
	    } else {
		update_cloud_attrib(ctmp);
	    }
	    newsym(x, y);
	}
    }
}

int testth, testcw;

void
run_cloud()
{
    struct cloud *ctmp, *nc;
    struct monst *mtmp;
    int tmp;
    for (ctmp = level.cloudlist; ctmp; ctmp = nc) {
	nc = ctmp->ncloud;
	switch (ctmp->ctyp) {
	    case STINKING_CLOUD:
		if (u.ux == ctmp->cx && u.uy == ctmp->cy &&
		    !u.uswallow && !Underwater) {
		    if (nonliving(youmonst.data) || Breathless)
			break;
		    if (!Blind)
			make_blinded(1L, FALSE);
		    if (!Poison_resistance) {
#ifndef JP
			pline("%s is burning your %s!", Something, makeplural(body_part(LUNG)));
#else
			pline("%s‚ª‚ ‚È‚½‚Ì%s‚ðÄ‚¢‚½I", Something, body_part(LUNG));
#endif /*JP*/
			You(E_J("cough and spit blood!","ŠP‚«ž‚ÝAŒŒ‚ð“f‚¢‚½I"));
			losehp(rnd(ctmp->density > 5 ? ctmp->damage : ctmp->damage/2) + 5,
				E_J("gas cloud","“ÅƒKƒX‚Å"), KILLED_BY_AN);
		    } else if (!is_full_resist(POISON_RES) &&
			       u.umonnum != PM_GREEN_DRAGON &&
			       u.umonnum != PM_BABY_GREEN_DRAGON) {
			You(E_J("cough!","ŠP‚«ž‚ñ‚¾I"));
		    }
#ifdef STEED
		    if (u.usteed) {
			mtmp = u.usteed;
			goto run_cloud_mon;
		    }
#endif
		} else if ((mtmp = m_at(ctmp->cx, ctmp->cy)) != 0) {
run_cloud_mon:
#ifdef MONSTEED
		  do {
#endif
		    /* Non living and non breathing monsters are not concerned */
		    if (!nonliving(mtmp->data) && !breathless(mtmp->data)) {
			/* green dragons are immune to poisonous gas */
			if (mtmp->data == &mons[PM_GREEN_DRAGON] ||
			    mtmp->data == &mons[PM_BABY_GREEN_DRAGON])
				continue;
			if (cansee(mtmp->mx, mtmp->my))
			    pline(E_J("%s coughs!","%s‚ÍŠP‚«ž‚ñ‚¾I"), Monnam(mtmp));
			setmangry(mtmp);
			if (haseyes(mtmp->data) && mtmp->mcansee) {
			    mtmp->mblinded = 1;
			    mtmp->mcansee = 0;
			}
			if (resists_poison(mtmp))
#ifdef MONSTEED
			    continue;
#else
			    break;
#endif
			tmp = rnd(ctmp->density > 5 ? ctmp->damage : ctmp->damage/2) + 5;
			mtmp->mhp -= tmp;
#ifdef SHOWDMG
			if (flags.showdmg && mtmp->mhp > 0 &&
			    canseemon(mtmp)) printdmg(tmp);
#endif
			if (mtmp->mhp <= 0) {
			    if (ctmp->madebyu && canseemon(mtmp))
				killed_showdmg(mtmp, tmp);
			    else
				monkilled(mtmp, E_J("gas cloud","“ÅƒKƒX"), AD_DRST);
			}
		    }
#ifdef MONSTEED
		  } while (is_mriding(mtmp) && (mtmp = mtmp->mlink) != 0);
#endif
		}
		break;

	    case CLOUD_LIGHT:
		if (ctmp->density) {
		    ctmp->density--;
		    if (ctmp->density <= 0) {
			remove_cloud(ctmp);
			dealloc_cloud(ctmp);
			vision_full_recalc = 1;
		    }
		}
		break;

	    default:
		break;
	}
    }
    diffuse_cloud(STINKING_CLOUD, 3, 10, 0);
}

void
create_stinking_cloud(x, y, radius, damage, madebyu)
xchar x, y;
int radius;
int damage;
boolean madebyu;
{
    struct cloud *ctmp;
    int i, j, d;
    int cx, cy;
    schar ltyp;
    for (i=-1; i<=1; i++) {
	for (j=-1; j<=1; j++) {
	    cx = x + j;
	    cy = y + i;
	    if (!isok(cx, cy)) continue;
	    ltyp = levl[cx][cy].typ;
	    if ((!IS_ROCK(ltyp) || IS_TREES(ltyp)) && ltyp != DRAWBRIDGE_UP &&
		(ltyp != DOOR || (levl[cx][cy].doormask & (D_NODOOR | D_BROKEN | D_ISOPEN)))) {
		d = (radius <= 2) ? 5 : (radius == 3) ? 15 : 30;
		if (i != 0 && j != 0) d -= rn2(4);
		if (i == 0 && j == 0) d += rn2(d/3);
		ctmp = newcloud();
		ctmp->visible = 1;
		ctmp->doeslit = 0;
		ctmp->hlev = CLOUD_OVEROBJ;
		ctmp->ctyp = STINKING_CLOUD;
		ctmp->density = d;
		ctmp->damage = damage;
		ctmp->madebyu = madebyu;
		ctmp->age = monstermoves;
		ctmp->initial = 1;
		update_cloud_attrib(ctmp);
		place_cloud(ctmp, cx, cy);
		newsym(cx, cy);
	    }
	}
    }
    for (i=radius; i>1; i--) {
	diffuse_cloud(STINKING_CLOUD, 3, 10, 1+(i==2));
    }
}

#ifdef WIZARD
int wiz_cloud_list()
{
    winid win;
    char buf[BUFSZ];
    struct cloud *ctmp;
    int x, y;

    win = create_nhwindow(NHW_MENU);	/* corner text window */
    if (win == WIN_ERR) return 0;

    putstr(win, 0, "List of clouds");
    putstr(win, 0, "");

    if (!level.cloudlist) 
	putstr(win, 0, "<none>");
    for (ctmp = level.cloudlist; ctmp; ctmp = ctmp->ncloud) {
	Sprintf(buf, "(%d,%d) Typ:%d Density:%d Initial:%d",
		ctmp->cx, ctmp->cy, ctmp->ctyp, ctmp->density, ctmp->initial);
	putstr(win, 0, buf);
    }
    for (y=0; y<ROWNO; y++)
	for (x=1; x<COLNO; x++) {
	    for (ctmp = level.clouds[x][y]; ctmp; ctmp = ctmp->nchere) {
		Sprintf(buf, "{%d,%d} Typ:%d Density:%d Initial:%d",
			x, y, ctmp->ctyp, ctmp->density, ctmp->initial);
	    }
	}
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);

    return 0;
}
#endif /*WIZARD*/

// Th=3,Cw=10,Den=5/15/30
/* imported from vision.c, for small circles */
extern char circle_data[];
extern char circle_start[];
void
test_cloud()
{
    struct cloud *ctmp;

    int x, y, min_x, max_x, max_y, offset, d;
    char *limits;

    limits = circle_ptr(5);
    if ((max_y = (u.uy + 5)) >= ROWNO) max_y = ROWNO-1;
    if ((y = (u.uy - 5)) < 0) y = 0;
    for (; y <= max_y; y++) {
	offset = limits[abs(y - u.uy)];
	if ((min_x = (u.ux - offset)) < 1) min_x = 1;
	if ((max_x = (u.ux + offset)) >= COLNO) max_x = COLNO-1;
	for (x = min_x; x <= max_x; x++) {
	    if (!couldsee(x, y)) continue;
	    d = dist2(u.ux, u.uy, x, y);
	    if (d > 5*5) continue;
	    ctmp = cloud_at(x, y, CLOUD_LIGHT);
	    if (ctmp) {
		ctmp->density = max(5*5 - d + 10, ctmp->density);
		continue;
	    }
	    ctmp = newcloud();
	    ctmp->visible = 0;
	    ctmp->doesblock = 0;
	    ctmp->doeslit = 1;
	    ctmp->hlev = CLOUD_OVERFLOOR;
	    ctmp->ctyp = CLOUD_LIGHT;
	    ctmp->glyph = 0;
	    ctmp->density = 5*5 - d + 10;
	    ctmp->damage = 0;
	    ctmp->madebyu = 1;
	    place_cloud(ctmp, x, y);
	}
    }
    vision_full_recalc = 1;

//int i, x, y, den;
//schar ltyp;
//char buf[BUFSZ], qbuf[QBUFSZ];
//coord cc;
//    pline("Spot the center of the cloud.");
//    cc.x = u.ux; cc.y = u.uy;
//    if (getpos(&cc, TRUE, "the center spot of cloud") < 0)
//	return;	/* user pressed ESC */
//    testth = 5;
//    testcw = 20;
//    den = 30;
//    Sprintf(qbuf, "Threshold, Centerweight, Density ? ");
//    getlin(qbuf, buf);
//    sscanf(buf, " %d , %d , %d ", &testth, &testcw, &den);
//    pline("Threshold, Centerweight, Density = %d, %d, %d", testth, testcw, den);
//
//    ctmp = newcloud();
//    ctmp->visible = 1;
//    ctmp->doesblock = 0;
//    ctmp->hlev = CLOUD_OVEROBJ;
//    ctmp->ctyp = STINKING_CLOUD;
//    ctmp->glyph = GLYPH_CLOUD_OFF + S_densepcloud - S_thinpcloud;
//    ctmp->density = den;
//    ctmp->damage = 4;
//    ctmp->madebyu = 1;
//    place_cloud(ctmp, cc.x, cc.y);
//    newsym(cc.x,cc.y);
//  for (i=0; i<8; i++) {
//    x = cc.x + xdir[i];
//    y = cc.y + ydir[i];
//    if (!isok(x,y)) continue;
//    ltyp = levl[x][y].typ;
//    if ((!IS_ROCK(ltyp) || IS_TREES(ltyp)) && ltyp != DRAWBRIDGE_UP &&
//	(ltyp != DOOR || (levl[x][y].doormask & (D_NODOOR | D_BROKEN | D_ISOPEN)))) {
//    ctmp = newcloud();
//    ctmp->visible = 1;
//    ctmp->doesblock = 0;
//    ctmp->hlev = CLOUD_OVEROBJ;
//    ctmp->ctyp = STINKING_CLOUD;
//    ctmp->glyph = GLYPH_CLOUD_OFF + S_densepcloud - S_thinpcloud;
//    ctmp->density = den;
//    ctmp->damage = 4;
//    ctmp->madebyu = 1;
//    place_cloud(ctmp, x, y);
//    newsym(x,y);
//    }
//  }
}
//    #           
//   ###       #  
//  #####     ### 
// #######   #####
//  #####     ### 
//   ###       #  
//    #           

void
save_cloud(fd, ctmp, mode)
int fd, mode;
struct cloud *ctmp;
{
	struct cloud *ctmp2;
	int cnt = 0;

	/* count cloud structures */
	for (ctmp2 = ctmp; ctmp2; ctmp2 = ctmp2->ncloud)
	    cnt++;
	/* write the count */
	bwrite(fd, (genericptr_t) &cnt, sizeof(cnt));
	/* write clouds */
	while(cnt--) {
	    ctmp2 = ctmp->ncloud;
	    if (perform_bwrite(mode)) {
		bwrite(fd, (genericptr_t) ctmp, sizeof(struct cloud));
	    }
	    ctmp = ctmp2;
	}
	if (release_data(mode))
	    while (ctmp) {
		ctmp2 = ctmp->ncloud;
		dealloc_cloud(ctmp);
		ctmp = ctmp2;
	    }
}

void
restore_cloud(fd, adjust)
int fd;
long adjust;
{
	struct cloud *ctmp, *ctmp2 = 0;
	int cnt;
	int x, y;

	for (y=0; y<ROWNO; y++)
	    for (x=0; x<COLNO; x++)
		level.clouds[x][y] = (struct cloud *)0;

	mread(fd, (genericptr_t) &cnt, sizeof(cnt));
	while(cnt--) {
	    ctmp = newcloud();
	    mread(fd, (genericptr_t) ctmp, sizeof(struct cloud));
	    ctmp->ncloud = (struct cloud *)0;
	    ctmp->nchere = (struct cloud *)0;
	    if (adjust) {
		if (ctmp->density > adjust) {
		    ctmp->density -= adjust;
		} else {
		    /* dissipated */
		    ctmp->cx = ctmp->cy = 0;
		    dealloc_cloud(ctmp);
		    ctmp = (struct cloud *)0;
		}
	    }
	    if (ctmp) {
		/* add ctmp to link */
		if(!ctmp2) level.cloudlist = ctmp;
		else ctmp2->ncloud = ctmp;
		/* place ctmp on the map */
		place_cloud(ctmp, 0, 0); /* update level.clouds[][] only */
		update_cloud_attrib(ctmp);
		ctmp2 = ctmp;
	    }
	}
}
/*region.c*/
