#include "nhw.h"

/* ================================================
	Menu
   ================================================ */
/* allocate memory for menu */
BYTE *NHWCreateMenu(void) {
	BYTE *buf;
	NHWMenuHeader *mp;

	buf = malloc(DEFMENUSIZE);
	if (buf == NULL) return NULL;

	mp = (NHWMenuHeader *)buf;
	mp->bufsize = DEFMENUSIZE;
	mp->storeoffset = sizeof(NHWMenuHeader);
	mp->firstoffset = 0;
	mp->lastoffset = 0;
	mp->curroffset = 0;

	return buf;
}

/* add string to menu */
BYTE *NHWAddMenuItem(BYTE *menu, NHWMenuItem *ma, char *str, int totop) {
	long reqsiz, fsiz;
	long ofs;
	NHWMenuHeader *mh = (NHWMenuHeader *)menu;
	NHWMenuItem *mi, *ml;

	if (!str) str = "";
	reqsiz = sizeof(NHWMenuItem) - 4 + strlen(str) + 1/*EOS*/;
	reqsiz = (reqsiz + 3) & (~0UL-3);	/* alignment */
	fsiz = mh->bufsize - mh->storeoffset;
	if (reqsiz > fsiz) {
	    BYTE *newmenu;
	    newmenu = realloc(menu, mh->bufsize + DEFMENUSIZE);
	    if (newmenu == NULL) return menu;	/* out of memory, do nothing */
	    menu = newmenu;
	    mh = (NHWMenuHeader *)menu;
	    mh->bufsize += DEFMENUSIZE;
	}
	/* maintain link list */
	if (mh->lastoffset == 0) {
	    mh->lastoffset = mh->firstoffset = mh->storeoffset;
	    mi = (NHWMenuItem *)(menu + mh->storeoffset);
	    mi->nextoffset = 0;
	    mi->prevoffset = 0;
	    mh->storeoffset += reqsiz;
	} else {
	    if (totop) {
		ml = (NHWMenuItem *)(menu + mh->firstoffset);
		ml->prevoffset = mh->storeoffset;
		mi = (NHWMenuItem *)(menu + mh->storeoffset);
		mi->nextoffset = mh->firstoffset;
		mi->prevoffset = 0;
		mh->firstoffset = mh->storeoffset;
		mh->storeoffset += reqsiz;
	    } else {
		ml = (NHWMenuItem *)(menu + mh->lastoffset);
		ml->nextoffset = mh->storeoffset;
		mi = (NHWMenuItem *)(menu + mh->storeoffset);
		mi->prevoffset = mh->lastoffset;
		mi->nextoffset = 0;
		mh->lastoffset = mh->storeoffset;
		mh->storeoffset += reqsiz;
	    }
	}
	/* copy contents */
	strcpy((char *)(mi->str), str);
	mi->id		= ma->id;
	mi->attr	= ma->attr;
	mi->count	= ma->count;
	mi->accelerator = ma->accelerator;
	mi->groupacc	= ma->groupacc;
	mi->selected	= ma->selected;

	mi->fmt = NULL;

	return menu;
}

/* dispose menu */
void NHWDisposeMenu(BYTE *menu) {
	free(menu);
}

/* menu item utils */
NHWMenuItem *NHWFirstMenuItem(BYTE *menu) {
	NHWMenuHeader *mh;
	NHWMenuItem *mi;
	long o;
	mh = (NHWMenuHeader *)menu;
	o = mh->firstoffset;
	if (o == 0) return NULL;	/* no menu item */
	mh->curroffset = o;
	return (NHWMenuItem *)(menu + o);
}
NHWMenuItem *NHWNextMenuItem(BYTE *menu) {
	NHWMenuHeader *mh;
	NHWMenuItem *mi;
	long o;
	mh = (NHWMenuHeader *)menu;
	if (mh->firstoffset == 0) return NULL;	/* no menu item */
	mi = (NHWMenuItem *)(menu + mh->curroffset);
	o = mi->nextoffset;
	if (o == 0) return NULL;	/* no next item */
	mh->curroffset = o;
	return (NHWMenuItem *)(menu + o);
}
NHWMenuItem *NHWPrevMenuItem(BYTE *menu) {
	NHWMenuHeader *mh;
	NHWMenuItem *mi;
	long o;
	mh = (NHWMenuHeader *)menu;
	if (mh->firstoffset == 0) return NULL;	/* no menu item */
	mi = (NHWMenuItem *)(menu + mh->curroffset);
	o = mi->prevoffset;
	if (o == 0) return NULL;	/* no previous item */
	mh->curroffset = o;
	return (NHWMenuItem *)(menu + o);
}

