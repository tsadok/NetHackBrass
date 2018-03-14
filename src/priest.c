/*	SCCS Id: @(#)priest.c	3.4	2002/11/06	*/
/* Copyright (c) Izchak Miller, Steve Linhart, 1989.		  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "mfndpos.h"
#include "eshk.h"
#include "epri.h"
#include "emin.h"

/* this matches the categorizations shown by enlightenment */
#define ALGN_SINNED	(-4)	/* worse than strayed */

#ifdef OVLB

STATIC_DCL boolean FDECL(histemple_at,(struct monst *,XCHAR_P,XCHAR_P));
STATIC_DCL boolean FDECL(has_shrine,(struct monst *));

/*
 * Move for priests and shopkeepers.  Called from shk_move() and pri_move().
 * Valid returns are  1: moved  0: didn't  -1: let m_move do it  -2: died.
 */
int
move_special(mtmp,in_his_shop,appr,uondoor,avoid,omx,omy,gx,gy)
register struct monst *mtmp;
boolean in_his_shop;
schar appr;
boolean uondoor,avoid;
register xchar omx,omy,gx,gy;
{
	register xchar nx,ny,nix,niy;
	register schar i;
	schar chcnt,cnt;
	coord poss[9];
	long info[9];
	long allowflags;
	struct obj *ib = (struct obj *)0;

	if(omx == gx && omy == gy)
		return(0);
	if(mtmp->mconf) {
		avoid = FALSE;
		appr = 0;
	}

	nix = omx;
	niy = omy;
	if (mtmp->isshk) allowflags = ALLOW_SSM;
	else allowflags = ALLOW_SSM | ALLOW_SANCT;
	if (passes_walls(mtmp->data)) allowflags |= (ALLOW_ROCK|ALLOW_WALL);
	if (throws_rocks(mtmp->data)) allowflags |= ALLOW_ROCK;
	if (tunnels(mtmp->data)) allowflags |= ALLOW_DIG;
	if (!nohands(mtmp->data) && !verysmall(mtmp->data)) {
		allowflags |= OPENDOOR;
		if (m_carrying(mtmp, SKELETON_KEY)) allowflags |= BUSTDOOR;
	}
	if (is_giant(mtmp->data)) allowflags |= BUSTDOOR;
	cnt = mfndpos(mtmp, poss, info, allowflags);

	if(mtmp->isshk && avoid && uondoor) { /* perhaps we cannot avoid him */
		for(i=0; i<cnt; i++)
		    if(!(info[i] & NOTONL)) goto pick_move;
		avoid = FALSE;
	}

#define GDIST(x,y)	(dist2(x,y,gx,gy))
pick_move:
	chcnt = 0;
	for(i=0; i<cnt; i++) {
		nx = poss[i].x;
		ny = poss[i].y;
		if(levl[nx][ny].typ == ROOM ||
			(mtmp->ispriest &&
			    levl[nx][ny].typ == ALTAR) ||
			(mtmp->isshk &&
			    (!in_his_shop || ESHK(mtmp)->following))) {
		    if(avoid && (info[i] & NOTONL))
			continue;
		    if((!appr && !rn2(++chcnt)) ||
			(appr && GDIST(nx,ny) < GDIST(nix,niy))) {
			    nix = nx;
			    niy = ny;
		    }
		}
	}
	if(mtmp->ispriest && avoid &&
			nix == omx && niy == omy && onlineu(omx,omy)) {
		/* might as well move closer as long it's going to stay
		 * lined up */
		avoid = FALSE;
		goto pick_move;
	}

	if(nix != omx || niy != omy) {
		remove_monster(omx, omy);
		place_monster(mtmp, nix, niy);
		newsym(nix,niy);
		if (mtmp->isshk && !in_his_shop && inhishop(mtmp))
		    check_special_room(FALSE);
		if(ib) {
			if (cansee(mtmp->mx,mtmp->my))
			    pline(E_J("%s picks up %s.",
				      "%s��%s���E�����B"), Monnam(mtmp),
				distant_name(ib,doname));
			obj_extract_self(ib);
			(void) mpickobj(mtmp, ib);
		}
		return(1);
	}
	return(0);
}

#endif /* OVLB */

#ifdef OVL0

char
temple_occupied(array)
register char *array;
{
	register char *ptr;

	for (ptr = array; *ptr; ptr++)
		if (rooms[*ptr - ROOMOFFSET].rtype == TEMPLE)
			return(*ptr);
	return('\0');
}

#endif /* OVL0 */
#ifdef OVLB

STATIC_OVL boolean
histemple_at(priest, x, y)
register struct monst *priest;
register xchar x, y;
{
	return((boolean)((EPRI(priest)->shroom == *in_rooms(x, y, TEMPLE)) &&
	       on_level(&(EPRI(priest)->shrlevel), &u.uz)));
}

boolean
inhistemple(priest)
struct monst *priest;
{
    /* make sure we have a priest */
    if (!priest || !priest->ispriest)
        return FALSE;
    /* priest must be on right level and in right room */
    if (!histemple_at(priest, priest->mx, priest->my))
        return FALSE;
    /* temple room must still contain properly aligned altar */
    return has_shrine(priest);
}

/*
 * pri_move: return 1: moved  0: didn't  -1: let m_move do it  -2: died
 */
int
pri_move(priest)
register struct monst *priest;
{
	register xchar gx,gy,omx,omy;
	schar temple;
	boolean avoid = TRUE;

	omx = priest->mx;
	omy = priest->my;

	if(!histemple_at(priest, omx, omy)) return(-1);

	temple = EPRI(priest)->shroom;

	gx = EPRI(priest)->shrpos.x;
	gy = EPRI(priest)->shrpos.y;

	gx += rn1(3,-1);	/* mill around the altar */
	gy += rn1(3,-1);

	if(!priest->mpeaceful ||
	   (Conflict && !resist(priest, RING_CLASS, 0, 0))) {
		if(monnear(priest, u.ux, u.uy)) {
			if(Displaced)
				Your(E_J("displaced image doesn't fool %s!",
					 "���e��%s���\���Ȃ������I"),
					mon_nam(priest));
			(void) mattacku(priest);
			return(0);
		} else if(index(u.urooms, temple)) {
			/* chase player if inside temple & can see him */
			if(priest->mcansee && m_canseeu(priest)) {
				gx = u.ux;
				gy = u.uy;
			}
			avoid = FALSE;
		}
	} else if(Invis) avoid = FALSE;

	return(move_special(priest,FALSE,TRUE,FALSE,avoid,omx,omy,gx,gy));
}

/* exclusively for mktemple() */
void
priestini(lvl, sroom, sx, sy, sanctum)
d_level	*lvl;
struct mkroom *sroom;
int sx, sy;
boolean sanctum;   /* is it the seat of the high priest? */
{
	struct monst *priest;
	struct obj *otmp;
	int cnt;

	if(MON_AT(sx+1, sy))
		(void) rloc(m_at(sx+1, sy), FALSE); /* insurance */

	priest = makemon(&mons[sanctum ? PM_HIGH_PRIEST : PM_ALIGNED_PRIEST],
			 sx + 1, sy, NO_MM_FLAGS);
	if (priest) {
		EPRI(priest)->shroom = (sroom - rooms) + ROOMOFFSET;
		EPRI(priest)->shralign = Amask2align(levl[sx][sy].altarmask);
		EPRI(priest)->shrpos.x = sx;
		EPRI(priest)->shrpos.y = sy;
		assign_level(&(EPRI(priest)->shrlevel), lvl);
		priest->mtrapseen = ~0;	/* traps are known */
		priest->ispriest = 1;
		priest->msleeping = 0;
		setmpeaceful(priest, TRUE); /* mpeaceful may have changed */

		/* now his/her goodies... */
		if(sanctum && EPRI(priest)->shralign == A_NONE &&
		     on_level(&sanctum_level, &u.uz)) {
			(void) mongets(priest, AMULET_OF_YENDOR);
		}
		/* 2 to 4 spellbooks */
		for (cnt = rn1(3,2); cnt > 0; --cnt) {
		    (void) mpickobj(priest, mkobj(SPBOOK_CLASS, FALSE));
		}
		/* robe [via makemon()] */
		if (rn2(2) && (otmp = which_armor(priest, W_ARMC)) != 0) {
		    if (p_coaligned(priest))
			uncurse(otmp);
		    else
			curse(otmp);
		}
	}
}

/*
 * Specially aligned monsters are named specially.
 *	- aligned priests with ispriest and high priests have shrines
 *		they retain ispriest and epri when polymorphed
 *	- aligned priests without ispriest and Angels are roamers
 *		they retain isminion and access epri as emin when polymorphed
 *		(coaligned Angels are also created as minions, but they
 *		use the same naming convention)
 *	- minions do not have ispriest but have isminion and emin
 *	- caller needs to inhibit Hallucination if it wants to force
 *		the true name even when under that influence
 */
char *
priestname(mon, pname)
register struct monst *mon;
char *pname;		/* caller-supplied output buffer */
{
	const char *what = Hallucination ? rndmonnam() :
	    E_J(mon->data->mname, JMONNAM(monsndx(mon->data)));

#ifndef JP
	Strcpy(pname, "the ");
#else
	pname[0] = 0;
#endif /*JP*/
	if (mon->minvis) Strcat(pname, E_J("invisible ","�ڂɌ����Ȃ�"));
	if (mon->ispriest || mon->data == &mons[PM_ALIGNED_PRIEST] ||
					mon->data == &mons[PM_ANGEL]) {
#ifdef JP
		Strcat(pname, halu_gname((int)EPRI(mon)->shralign));
		Strcat(pname, "��");
#endif /*JP*/
		/* use epri */
		if (mon->mtame && mon->data == &mons[PM_ANGEL])
			Strcat(pname, E_J("guardian ","���"));
		if (mon->data != &mons[PM_ALIGNED_PRIEST] &&
				mon->data != &mons[PM_HIGH_PRIEST]) {
			Strcat(pname, what);
#ifndef JP
			Strcat(pname, " ");
#endif /*JP*/
		}
		if (mon->data != &mons[PM_ANGEL]) {
			if (!mon->ispriest && EPRI(mon)->renegade)
				Strcat(pname, E_J("renegade ","�j��"));
#ifndef JP
			if (mon->data == &mons[PM_HIGH_PRIEST])
				Strcat(pname, "high ");
			if (Hallucination)
				Strcat(pname, "poohbah ");
			else if (mon->female)
				Strcat(pname, "priestess ");
			else
				Strcat(pname, "priest ");
#else
			if (Hallucination) {
				Strcat(pname, "���\");
				Strcat(pname, (mon->data == &mons[PM_HIGH_PRIEST]) ?
					      "��b" : "����");
			} if (mon->data == &mons[PM_HIGH_PRIEST]) {
				Strcat(pname, mon->female ? "�����c" : "�@��");
			} else {
				Strcat(pname, mon->female ? "��m" : "�m��");
			}
#endif /*JP*/
		}
#ifndef JP
		Strcat(pname, "of ");
		Strcat(pname, halu_gname((int)EPRI(mon)->shralign));
#endif /*JP*/
		return(pname);
	}
	/* use emin instead of epri */
#ifndef JP
	Strcat(pname, what);
	Strcat(pname, " of ");
	Strcat(pname, halu_gname(EMIN(mon)->min_align));
#else
	Strcat(pname, halu_gname(EMIN(mon)->min_align));
	Strcat(pname, "��");
	Strcat(pname, what);
#endif /*JP*/
	return(pname);
}

boolean
p_coaligned(priest)
struct monst *priest;
{
	return((boolean)(u.ualign.type == ((int)EPRI(priest)->shralign)));
}

STATIC_OVL boolean
has_shrine(pri)
struct monst *pri;
{
	struct rm *lev;

	if(!pri)
		return(FALSE);
	lev = &levl[EPRI(pri)->shrpos.x][EPRI(pri)->shrpos.y];
	if (!IS_ALTAR(lev->typ) || !(lev->altarmask & AM_SHRINE))
		return(FALSE);
	return((boolean)(EPRI(pri)->shralign == Amask2align(lev->altarmask & ~AM_SHRINE)));
}

struct monst *
findpriest(roomno)
char roomno;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if(mtmp->ispriest && (EPRI(mtmp)->shroom == roomno) &&
	       histemple_at(mtmp,mtmp->mx,mtmp->my))
		return(mtmp);
	}
	return (struct monst *)0;
}

/* called from check_special_room() when the player enters the temple room */
void
intemple(roomno)
register int roomno;
{
	register struct monst *priest = findpriest((char)roomno);
	boolean tended = (priest != (struct monst *)0);
	boolean shrined, sanctum, can_speak;
	const char *msg1, *msg2;
	char buf[BUFSZ];

	if(!temple_occupied(u.urooms0)) {
	    if(tended) {
		shrined = has_shrine(priest);
		sanctum = (priest->data == &mons[PM_HIGH_PRIEST] &&
			   (Is_sanctum(&u.uz) || In_endgame(&u.uz)));
		can_speak = (priest->mcanmove && !priest->msleeping &&
			     flags.soundok);
		if (can_speak) {
		    unsigned save_priest = priest->ispriest;
		    /* don't reveal the altar's owner upon temple entry in
		       the endgame; for the Sanctum, the next message names
		       Moloch so suppress the "of Moloch" for him here too */
		    if (sanctum && !Hallucination) priest->ispriest = 0;
#ifndef JP
		    pline("%s intones:",
			canseemon(priest) ? Monnam(priest) : "A nearby voice");
#else
		    if (canseemon(priest))
			pline("%s���r������:", mon_nam(priest));
		    else
			pline("�߂��Ő�������:");
#endif /*JP*/
		    priest->ispriest = save_priest;
		}
		msg2 = 0;
		if(sanctum && Is_sanctum(&u.uz)) {
		    if(priest->mpeaceful) {
			msg1 = E_J("Infidel, you have entered Moloch's Sanctum!",
				   "�ً��k��A���O�̓��[���b�N�̐���ɓ��ݍ���ł���I");
			msg2 = E_J("Be gone!","��������I");
			setmpeaceful(priest, FALSE);
		    } else
			msg1 = E_J("You desecrate this place by your presence!",
				   "���O�͂��̑��݂ł��̐��n���������I");
		} else {
		    Sprintf(buf, E_J("Pilgrim, you enter a %s place!",
				     "����҂�A���܂���%s�n�ɂ���I"),
			    !shrined ? E_J("desecrated","�s���") : E_J("sacred","���Ȃ�"));
		    msg1 = buf;
		}
		if (can_speak) {
		    verbalize(msg1);
		    if (msg2) verbalize(msg2);
		}
		if(!sanctum) {
		    /* !tended -> !shrined */
		    if (!shrined || !p_coaligned(priest) ||
			    u.ualign.record <= ALGN_SINNED)
			You(E_J("have a%s forbidding feeling...",
				"%s�ߊ�����������c�B"),
				(!shrined) ? "" : E_J(" strange","���"));
		    else You(E_J("experience a strange sense of peace.",
				 "�s�v�c�Ȉ��炬�ɕ�܂ꂽ�B"));
		}
#ifdef DUNGEON_OVERVIEW
        /* recognize the Valley of the Dead and Moloch's Sanctum
           once hero has encountered the temple priest on those levels */
        mapseen_temple(priest);
#endif
	    } else {
		switch(rn2(3)) {
		  case 0: You(E_J("have an eerie feeling...","�����Ƃ����c�B")); break;
		  case 1: You_feel(E_J("like you are being watched.",
				       "���߂��Ă���悤�ȋC�������B")); break;
		  default: pline(E_J("A shiver runs down your %s.",
				     "���Ȃ���%s��k�����������B"),
			body_part(SPINE)); break;
		}
		if(!rn2(5)) {
		    struct monst *mtmp;

		    if(!(mtmp = makemon(&mons[PM_GHOST],u.ux,u.uy,NO_MM_FLAGS)))
			return;
		    if (!Blind || sensemon(mtmp))
			pline(E_J("An enormous ghost appears next to you!",
				  "���낵���S�삪���Ȃ��̖ڂ̑O�Ɍ��ꂽ�I"));
		    else You(E_J("sense a presence close by!",
				 "�������΂ɉ��҂��̑��݂��������I"));
		    setmpeaceful(mtmp, FALSE);
		    if(flags.verbose)
			You(E_J("are frightened to death, and unable to move.",
				"���|�̂��܂蓮���Ȃ��Ȃ����B"));
		    nomul(-3);
		    nomovemsg = E_J("You regain your composure.",
				    "���Ȃ��͕��Â����߂����B");
	       }
	   }
       }
}

void
priest_talk(priest)
register struct monst *priest;
{
	boolean coaligned = p_coaligned(priest);
	boolean strayed = (u.ualign.record < 0);

	/* KMH, conduct */
	u.uconduct.gnostic++;

	if(priest->mflee || (!priest->ispriest && coaligned && strayed)) {
	    pline(E_J("%s doesn't want anything to do with you!",
		      "%s�͂��Ȃ��Ɗւ�荇�������Ȃ��I"),
				Monnam(priest));
	    priest->mpeaceful = 0;
	    return;
	}

	/* priests don't chat unless peaceful and in their own temple */
	if(!histemple_at(priest,priest->mx,priest->my) ||
		 !priest->mpeaceful || !priest->mcanmove || priest->msleeping) {
	    static const char *cranky_msg[3] = {
#ifndef JP
		"Thou wouldst have words, eh?  I'll give thee a word or two!",
		"Talk?  Here is what I have to say!",
		"Pilgrim, I would speak no longer with thee."
#else
		"�_����������肩�A�����H ���ɂ͈ꌾ�ŏ\�����I",
		"�b���H ���Ɍ����K�v������̂͂��ꂾ�����I",
		"���Q�҂�A��͂��͂���ƌ��t�����킳�ʁB"
#endif /*JP*/
	    };

	    if(!priest->mcanmove || priest->msleeping) {
#ifndef JP
		pline("%s breaks out of %s reverie!",
		      Monnam(priest), mhis(priest));
#else
		pline("%s�͖��z�����ɕԂ����I", mon_nam(priest));
#endif /*JP*/
		priest->mfrozen = priest->msleeping = 0;
		priest->mcanmove = 1;
	    }
	    priest->mpeaceful = 0;
	    verbalize(cranky_msg[rn2(3)]);
	    return;
	}

	/* you desecrated the temple and now you want to chat? */
	if(priest->mpeaceful && *in_rooms(priest->mx, priest->my, TEMPLE) &&
		  !has_shrine(priest)) {
	    verbalize(E_J("Begone!  Thou desecratest this holy place with thy presence.",
			  "����I ���͂��̐��n�����̑��݂ŉ������̂��B"));
	    priest->mpeaceful = 0;
	    return;
	}
#ifndef GOLDOBJ
	if(!u.ugold) {
	    if(coaligned && !strayed) {
		if (priest->mgold > 0L) {
		    /* Note: two bits is actually 25 cents.  Hmm. */
#ifndef JP
		    pline("%s gives you %s for an ale.", Monnam(priest),
			(priest->mgold == 1L) ? "one bit" : "two bits");
#else
		    pline("%s�͂��Ȃ��ɃG�[����̋���%s�����߂��񂾁B", mon_nam(priest),
			(priest->mgold == 1L) ? "��" : "��");
#endif /*JP*/
		    if (priest->mgold > 1L)
			u.ugold = 2L;
		    else
			u.ugold = 1L;
		    priest->mgold -= u.ugold;
		    flags.botl = 1;
#else
	if(!money_cnt(invent)) {
	    if(coaligned && !strayed) {
                long pmoney = money_cnt(priest->minvent);
		if (pmoney > 0L) {
		    /* Note: two bits is actually 25 cents.  Hmm. */
#ifndef JP
		    pline("%s gives you %s for an ale.", Monnam(priest),
			(pmoney == 1L) ? "one bit" : "two bits");
#else
		    pline("%s�͂��Ȃ��ɃG�[����̍d��%s�����߂��񂾁B", mon_nam(priest),
			(pmoney == 1L) ? "��" : "��");
#endif /*JP*/
		     money2u(priest, pmoney > 1L ? 2 : 1);
#endif
		} else
		    pline(E_J("%s preaches the virtues of poverty.",
			      "%s�͂��Ȃ��ɐ��n�̓��ɂ��Đ������B"), Monnam(priest));
		exercise(A_WIS, TRUE);
	    } else
		pline(E_J("%s is not interested.",
			  "%s�͋����������Ȃ��B"), Monnam(priest));
	    return;
	} else {
	    long offer;

	    pline(E_J("%s asks you for a contribution for the temple.",
		      "%s�͂��Ȃ��Ɏ��@�ւ̊�t�����߂��B"),
			Monnam(priest));
	    if((offer = bribe(priest)) == 0) {
		verbalize(E_J("Thou shalt regret thine action!",
			      "���͎���̍s����������邱�ƂƂȂ낤�I"));
		if(coaligned) adjalign(-1);
	    } else if(offer < (u.ulevel * 200)) {
#ifndef GOLDOBJ
		if(u.ugold > (offer * 2L)) verbalize(E_J("Cheapskate.","�P�`�B"));
#else
		if(money_cnt(invent) > (offer * 2L)) verbalize(E_J("Cheapskate.","�P�`�B"));
#endif
		else {
		    verbalize(E_J("I thank thee for thy contribution.",
				  "���̌��g�Ɋ��ӂ������܂��B"));
		    /*  give player some token  */
		    exercise(A_WIS, TRUE);
		}
	    } else if(offer < (u.ulevel * 400)) {
		verbalize(E_J("Thou art indeed a pious individual.",
			      "���͂܂��ƂɌh�i�Ȏ҂Ȃ�B"));
#ifndef GOLDOBJ
		if(u.ugold < (offer * 2L)) {
#else
		if(money_cnt(invent) < (offer * 2L)) {
#endif
		    if (coaligned && u.ualign.record <= ALGN_SINNED)
			adjalign(1);
		    verbalize(E_J("I bestow upon thee a blessing.",
				  "���ɏj���������悤�B"));
		    incr_itimeout(&HClairvoyant, rn1(500,500));
		}
	    } else if(offer < (u.ulevel * 600) &&
		      u.ublessed < 10 &&
		      (!u.ublessed || !rn2(1 << (u.ublessed)))) {
		verbalize(E_J("Thy devotion has been rewarded.",
			      "���̌��g�͕��ꂽ�B"));
		if (!(HProtection & INTRINSIC))
			HProtection |= FROMOUTSIDE;
		u.ublessed++;
	    } else {
		verbalize("���̎��S�Ȃ����e���͑傢�ɔF�߂�ꂽ�B");
#ifndef GOLDOBJ
		if(u.ugold < (offer * 2L) && coaligned) {
#else
		if(money_cnt(invent) < (offer * 2L) && coaligned) {
#endif
		    if(strayed && (moves - u.ucleansed) > 5000L) {
			u.ualign.record = 0; /* cleanse thee */
			u.ucleansed = moves;
		    } else {
			adjalign(2);
		    }
		}
	    }
	}
}

struct monst *
mk_roamer(ptr, alignment, x, y, peaceful)
register struct permonst *ptr;
aligntyp alignment;
xchar x, y;
boolean peaceful;
{
	register struct monst *roamer;
	register boolean coaligned = (u.ualign.type == alignment);

	if (ptr != &mons[PM_ALIGNED_PRIEST] && ptr != &mons[PM_ANGEL])
		return((struct monst *)0);
	
	if (MON_AT(x, y)) (void) rloc(m_at(x, y), FALSE);	/* insurance */

	if (!(roamer = makemon(ptr, x, y, NO_MM_FLAGS)))
		return((struct monst *)0);

	EPRI(roamer)->shralign = alignment;
	if (coaligned && !peaceful)
		EPRI(roamer)->renegade = TRUE;
	/* roamer->ispriest == FALSE naturally */
	roamer->isminion = TRUE;	/* borrowing this bit */
	roamer->mtrapseen = ~0;		/* traps are known */
	roamer->msleeping = 0;
	setmpeaceful(roamer, peaceful); /* peaceful may have changed */

	/* MORE TO COME */
	return(roamer);
}

void
reset_hostility(roamer)
register struct monst *roamer;
{
	if(!(roamer->isminion && (roamer->data == &mons[PM_ALIGNED_PRIEST] ||
				  roamer->data == &mons[PM_ANGEL])))
	        return;

	if(EPRI(roamer)->shralign != u.ualign.type) {
	    roamer->mtame = 0;
	    setmpeaceful(roamer, FALSE);
	}
	newsym(roamer->mx, roamer->my);
}

boolean
in_your_sanctuary(mon, x, y)
struct monst *mon;	/* if non-null, <mx,my> overrides <x,y> */
xchar x, y;
{
	register char roomno;
	register struct monst *priest;

	if (mon) {
	    if (is_minion(mon->data) || is_rider(mon->data)) return FALSE;
	    x = mon->mx, y = mon->my;
	}
	if (u.ualign.record <= ALGN_SINNED)	/* sinned or worse */
	    return FALSE;
	if ((roomno = temple_occupied(u.urooms)) == 0 ||
		roomno != *in_rooms(x, y, TEMPLE))
	    return FALSE;
	if ((priest = findpriest(roomno)) == 0)
	    return FALSE;
	return (boolean)(has_shrine(priest) &&
			 p_coaligned(priest) &&
			 priest->mpeaceful);
}

void
ghod_hitsu(priest)	/* when attacking "priest" in his temple */
struct monst *priest;
{
	int x, y, ax, ay, roomno = (int)temple_occupied(u.urooms);
	struct mkroom *troom;

	if (!roomno || !has_shrine(priest))
		return;

	ax = x = EPRI(priest)->shrpos.x;
	ay = y = EPRI(priest)->shrpos.y;
	troom = &rooms[roomno - ROOMOFFSET];

	if((u.ux == x && u.uy == y) || !linedup(u.ux, u.uy, x, y)) {
	    if(IS_DOOR(levl[u.ux][u.uy].typ)) {

		if(u.ux == troom->lx - 1) {
		    x = troom->hx;
		    y = u.uy;
		} else if(u.ux == troom->hx + 1) {
		    x = troom->lx;
		    y = u.uy;
		} else if(u.uy == troom->ly - 1) {
		    x = u.ux;
		    y = troom->hy;
		} else if(u.uy == troom->hy + 1) {
		    x = u.ux;
		    y = troom->ly;
		}
	    } else {
		switch(rn2(4)) {
		case 0:  x = u.ux; y = troom->ly; break;
		case 1:  x = u.ux; y = troom->hy; break;
		case 2:  x = troom->lx; y = u.uy; break;
		default: x = troom->hx; y = u.uy; break;
		}
	    }
	    if(!linedup(u.ux, u.uy, x, y)) return;
	}

	switch(rn2(3)) {
	case 0:
	    pline(E_J("%s roars in anger:  \"Thou shalt suffer!\"",
		      "%s�͓{��əႦ��:�g�ꂵ�ނ��悢�I�h"),
			a_gname_at(ax, ay));
	    break;
	case 1:
	    pline(E_J("%s voice booms:  \"How darest thou harm my servant!\"",
		      "%s����������:�g�䂪�l�Ɏ��������Ƃ́A�o��͂悢�ȁI�h"),
			s_suffix(a_gname_at(ax, ay)));
	    break;
	default:
	    pline(E_J("%s roars:  \"Thou dost profane my shrine!\"",
		      "%s�̓{������������:�g���͉䂪���@��`�������I�h"),
			a_gname_at(ax, ay));
	    break;
	}

	buzz(-10-(AD_ELEC-1), 6, x, y, sgn(tbx), sgn(tby)); /* bolt of lightning */
	exercise(A_WIS, FALSE);
}

void
angry_priest()
{
	register struct monst *priest;
	struct rm *lev;

	if ((priest = findpriest(temple_occupied(u.urooms))) != 0) {
	    wakeup(priest);
	    /*
	     * If the altar has been destroyed or converted, let the
	     * priest run loose.
	     * (When it's just a conversion and there happens to be
	     *	a fresh corpse nearby, the priest ought to have an
	     *	opportunity to try converting it back; maybe someday...)
	     */
	    lev = &levl[EPRI(priest)->shrpos.x][EPRI(priest)->shrpos.y];
	    if (!IS_ALTAR(lev->typ) ||
		((aligntyp)Amask2align(lev->altarmask & AM_MASK) !=
			EPRI(priest)->shralign)) {
		priest->ispriest = 0;		/* now a roamer */
		priest->isminion = 1;		/* but still aligned */
		/* this overloads the `shroom' field, which is now clobbered */
		EPRI(priest)->renegade = 0;
	    }
	}
}

/*
 * When saving bones, find priests that aren't on their shrine level,
 * and remove them.   This avoids big problems when restoring bones.
 */
void
clearpriests()
{
    register struct monst *mtmp, *mtmp2;

    for(mtmp = fmon; mtmp; mtmp = mtmp2) {
	mtmp2 = mtmp->nmon;
	if (!DEADMONSTER(mtmp) && mtmp->ispriest && !on_level(&(EPRI(mtmp)->shrlevel), &u.uz))
	    mongone(mtmp);
    }
}

/* munge priest-specific structure when restoring -dlc */
void
restpriest(mtmp, ghostly)
register struct monst *mtmp;
boolean ghostly;
{
    if(u.uz.dlevel) {
	if (ghostly)
	    assign_level(&(EPRI(mtmp)->shrlevel), &u.uz);
    }
}

#endif /* OVLB */

/*priest.c*/
