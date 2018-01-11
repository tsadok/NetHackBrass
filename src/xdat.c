#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*====Test Structure====*/
typedef unsigned char uchar;
typedef void * genericptr_t;
struct obj {
	int	dummy;
	uchar	onamelth;
	short	oxlth;
	long	oextra[1];
};
#define ONAME(otmp)	(((char *)(otmp)->oextra) + (otmp)->oxlth)

struct obj *realloc_obj(struct obj *obj, int oextra_size, genericptr_t oextra_src, int oname_size, char *name) {
	struct obj *newobj;
	newobj = malloc(sizeof(struct obj) + oextra_size + oname_size);
	*newobj = *obj;
	memcpy((genericptr_t)newobj->oextra, (genericptr_t)oextra_src, obj->oxlth);
	newobj->oxlth = oextra_size;
	memcpy((genericptr_t)((char *)newobj->oextra + oextra_size), name, obj->onamelth);
	newobj->onamelth = oname_size;
	free(obj);
	return newobj;
}
/*======================*/


/*size of data chunk = oxlth, mxlth*/

#define ALIGNMENT 4
#define XDATHSIZ (sizeof(struct xdat) - sizeof(long))

/* attachment data structure */

struct xdat {
	uchar	id;		/* xdat id: 0 for sentinel */
	uchar	reserved;
	short	siz;		/* size of this chunk including the header: 0 for sentinel */
	long	dat[1];
};

/* attachment handling code */

/*------------------------------------*/
genericptr_t next_xdat(struct xdat *);
struct obj * add_xdat_obj(struct obj *, uchar, int, genericptr_t);
genericptr_t get_xdat_obj(struct obj *, uchar);
int get_xdat_size(struct xdat *);
struct xdat *get_xdat(struct xdat *, uchar);
void remove_xdat(struct xdat *, uchar);
/*------------------------------------*/


struct obj *
add_xdat_obj(otmp, a_id, siz, dat)
struct obj *otmp;
uchar a_id;
int siz;
genericptr_t dat;
{
	struct xdat *xhp;
	int reqsiz;
	int needinit = 0;

	siz = (siz + XDATHSIZ + (ALIGNMENT-1)) & ~(ALIGNMENT-1);
	if (otmp->oxlth) {
	    reqsiz = siz + get_xdat_size((struct xdat *)&otmp->oextra);
	} else {
	    /* no terminator at this moment */
	    reqsiz = siz;
	    needinit = 1;
	}

	/* if we don't have a space for attachment, reallocate otmp */
	if (reqsiz > otmp->oxlth)
	    otmp = realloc_obj(otmp, reqsiz, otmp->oextra, otmp->onamelth, ONAME(otmp));

	/* get the terminator of attachments */
	xhp = (struct xdat *)&(otmp->oextra);
	if (!needinit)
	    for (; xhp->id; xhp = next_xdat(xhp));

	/* update with the new attachment data */
	xhp->id  = a_id;
	xhp->siz = siz;
	if (dat) (void) memcpy((genericptr_t)xhp->dat, dat, siz);

	/* new terminator */
	xhp = next_xdat(xhp);
	xhp->id  = 0;
	xhp->siz = 0;

	return otmp;
}

genericptr_t
get_xdat_obj(otmp, a_id)
struct obj *otmp;
uchar a_id;
{
	struct xdat *xhp;
	if (!otmp->oxlth) return 0;
	xhp = get_xdat((struct xdat *)&(otmp->oextra), a_id);
	if (!xhp) return 0;
	return (genericptr_t)&(xhp->dat);
}

/*------------------------------------*/

genericptr_t
next_xdat(xhp)
struct xdat *xhp;
{
	return (struct xdat *)((uchar *)xhp + xhp->siz);
}

int
get_xdat_size(xhp)
struct xdat *xhp;
{
	int sz = XDATHSIZ;
	while (xhp->id) {
	    sz += xhp->siz;
	    xhp = next_xdat(xhp);
	}
	return sz;
}

struct xdat *
get_xdat(xhp, a_id)
struct xdat *xhp;
uchar a_id;
{
	while (xhp->id != a_id) {
	    if (!xhp->id || !xhp->siz) return (genericptr_t)0;
	    xhp = next_xdat(xhp);
	}
	return xhp;
}

void
remove_xdat(xhp, a_id)
struct xdat *xhp;
uchar a_id;
{
	struct xdat *psrc, *pdst;

	/* search the attachment to be removed */
	pdst = get_xdat(xhp, a_id);
	if (!pdst) return; /* not found */
	psrc = next_xdat(pdst);

	/* do compaction */
	(void) memcpy((genericptr_t)pdst, (genericptr_t)psrc, get_xdat_size(psrc));
}


/*=====================================================================*/

void print_xdat(struct xdat *xhp) {
	printf("ID  :\t%8d\n", xhp->id);
	printf("Size:\t%8d\n", xhp->siz);
	printf("Ptr :\t%08X\n", (long)xhp);
	printf("\n");
}

void print_xdat_list(struct xdat *xhp) {
	printf("------------------------\n");
	for (;;) {
		print_xdat(xhp);
		if (!xhp->id) return;
		xhp = next_xdat(xhp);
	}
}

int main(int argc, char **argv) {
	struct obj *otmp;
	otmp = malloc(sizeof(struct obj));
	otmp->onamelth = 0;
	otmp->oxlth = 0;

	otmp = add_xdat_obj(otmp, 1, 15, 0);
	print_xdat_list((struct xdat *)&otmp->oextra);

	otmp = add_xdat_obj(otmp, 2, 33, 0);
	sprintf((char *)get_xdat_obj(otmp, 2), "TestString");
	print_xdat_list((struct xdat *)&otmp->oextra);

	remove_xdat((struct xdat *)&otmp->oextra, 1);
	print_xdat_list((struct xdat *)&otmp->oextra);

	otmp = add_xdat_obj(otmp, 1, 15, 0);
	print_xdat_list((struct xdat *)&otmp->oextra);

	remove_xdat((struct xdat *)&otmp->oextra, 1);
	print_xdat_list((struct xdat *)&otmp->oextra);

	printf("%s\n", (char *)get_xdat_obj(otmp, 2));

	return 0;
}
