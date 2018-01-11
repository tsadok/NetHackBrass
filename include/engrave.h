/*	SCCS Id: @(#)engrave.h	3.4	1991/07/31	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ENGRAVE_H
#define ENGRAVE_H

struct engr {
	struct engr *nxt_engr;
	char *engr_txt;
//	xchar engr_type;
	Bitfield(engr_read,1);	/* engraving has been read once */
	Bitfield(engr_typ2,3);	/* reserve */
	Bitfield(engr_type,4);
	xchar engr_x, engr_y;
	unsigned engr_lth;	/* for save & restore; not length of text */
	long engr_time;		/* moment engraving was (will be) finished */

#define DUST	   1
#define ENGRAVE    2
#define BURN	   3
#define MARK	   4
#define ENGR_BLOOD 5
#define HEADSTONE  6
#define N_ENGRAVE  6
#define	SHOPSIGN   7	/* sign on door */
};

/* directions: should be ordered to follow xdir[],ydir[] */
#define	SIGN_WEST  0
#define	SIGN_NORTH 1
#define	SIGN_EAST  2
#define	SIGN_SOUTH 3

#define newengr(lth) (struct engr *)alloc((unsigned)(lth) + sizeof(struct engr))
#define dealloc_engr(engr) free((genericptr_t) (engr))

#endif /* ENGRAVE_H */
