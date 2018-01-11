/*	SCCS Id: @(#)lock.c	3.4	2000/02/06	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(picklock);
STATIC_PTR int NDECL(forcelock);

/* at most one of `door' and `box' should be non-null at any given time */
STATIC_VAR NEARDATA struct xlock_s {
	struct rm  *door;
	struct obj *box;
	int picktyp, chance, usedtime;
} xlock;

#ifdef OVLB

STATIC_DCL const char *NDECL(lock_action);
STATIC_DCL boolean FDECL(obstructed,(int,int));
STATIC_DCL void FDECL(chest_shatter_msg, (struct obj *));
#ifdef JP
STATIC_DCL char *NDECL(occup_lock_action);
#endif /*JP*/

boolean
picking_lock(x, y)
	int *x, *y;
{
	if (occupation == picklock) {
	    *x = u.ux + u.dx;
	    *y = u.uy + u.dy;
	    return TRUE;
	} else {
	    *x = *y = 0;
	    return FALSE;
	}
}

boolean
picking_at(x, y)
int x, y;
{
	return (boolean)(occupation == picklock && xlock.door == &levl[x][y]);
}

/* produce an occupation string appropriate for the current activity */
STATIC_OVL const char *
lock_action()
{
	/* "unlocking"+2 == "locking" */
	static const char *actions[] = {
#ifndef JP
		/* [0] */	"unlocking the door",
		/* [1] */	"unlocking the chest",
		/* [2] */	"unlocking the box",
		/* [3] */	"picking the lock"
#else
		/* [0] */	"扉の鍵を開ける",
		/* [1] */	"扉に鍵をかける",
		/* [2] */	"宝箱の鍵を開ける",
		/* [3] */	"宝箱に鍵をかける",
		/* [4] */	"箱の鍵を開ける",
		/* [5] */	"箱に鍵をかける"
#endif /*JP*/
	};

#ifndef JP
	/* if the target is currently unlocked, we're trying to lock it now */
	if (xlock.door && !(xlock.door->doormask & D_LOCKED))
		return actions[0]+2;	/* "locking the door" */
	else if (xlock.box && !xlock.box->olocked)
		return xlock.box->otyp == CHEST ? actions[1]+2 : actions[2]+2;
	/* otherwise we're trying to unlock it */
	else if (xlock.picktyp == LOCK_PICK)
		return actions[3];	/* "picking the lock" */
#ifdef TOURIST
	else if (xlock.picktyp == CREDIT_CARD)
		return actions[3];	/* same as lock_pick */
#endif
	else if (xlock.door)
		return actions[0];	/* "unlocking the door" */
	else
		return xlock.box->otyp == CHEST ? actions[1] : actions[2];
#else /*JP*/
	if (xlock.door)
	    return actions[!(xlock.door->doormask & D_LOCKED)];
	if (xlock.box)
	    return actions[((xlock.box->otyp == CHEST) ? 2 : 4) +
			   !(xlock.box->olocked)];
	return actions[3];
#endif /*JP*/
}

#ifdef JP
STATIC_OVL char *
occup_lock_action()
{
	static char buf[32];
	Sprintf(buf, "%s作業", lock_action());
	return buf;
}
#endif /*JP*/

STATIC_PTR
int
picklock()	/* try to open/close a lock */
{

	if (xlock.box) {
	    if((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy)) {
		return((xlock.usedtime = 0));		/* you or it moved */
	    }
	} else {		/* door */
	    if(xlock.door != &(levl[u.ux+u.dx][u.uy+u.dy])) {
		return((xlock.usedtime = 0));		/* you moved */
	    }
	    switch (xlock.door->doormask) {
		case D_NODOOR:
		    pline(E_J("This doorway has no door.","この出入り口には扉がない。"));
		    return((xlock.usedtime = 0));
		case D_ISOPEN:
#ifndef JP
		    You("cannot lock an open door.");
#else
		    pline("開いている扉には鍵をかけられない。");
#endif /*JP*/
		    return((xlock.usedtime = 0));
		case D_BROKEN:
		    pline(E_J("This door is broken.","この扉は壊されている。"));
		    return((xlock.usedtime = 0));
	    }
	}

	if (xlock.usedtime++ >= 50 || nohands(youmonst.data)) {
	    You(E_J("give up your attempt at %s.","%sのをあきらめた。"), lock_action());
	    exercise(A_DEX, TRUE);	/* even if you don't succeed */
	    return((xlock.usedtime = 0));
	}

	if(rn2(100) >= xlock.chance) return(1);		/* still busy */

	You(E_J("succeed in %s.","%sのに成功した。"), lock_action());
	if (xlock.door) {
	    if(xlock.door->doormask & D_TRAPPED) {
		    b_trapped(E_J("door","扉"), FINGER);
		    xlock.door->doormask = D_NODOOR;
		    unblock_point(u.ux+u.dx, u.uy+u.dy);
		    if (*in_rooms(u.ux+u.dx, u.uy+u.dy, SHOPBASE))
			add_damage(u.ux+u.dx, u.uy+u.dy, 0L);
		    newsym(u.ux+u.dx, u.uy+u.dy);
	    } else if (xlock.door->doormask & D_LOCKED)
		xlock.door->doormask = D_CLOSED;
	    else xlock.door->doormask = D_LOCKED;
	} else {
	    xlock.box->olocked = !xlock.box->olocked;
	    if(xlock.box->otrapped)	
		(void) chest_trap(xlock.box, FINGER, FALSE);
	}
	exercise(A_DEX, TRUE);
	return((xlock.usedtime = 0));
}

STATIC_PTR
int
forcelock()	/* try to force a locked chest */
{

	register struct obj *otmp;

	if((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy))
		return((xlock.usedtime = 0));		/* you or it moved */

	if (xlock.usedtime++ >= 50 || !uwep || nohands(youmonst.data)) {
	    You(E_J("give up your attempt to force the lock.",
		    "錠をこじ開けるのをあきらめた。"));
	    if(xlock.usedtime >= 50)		/* you made the effort */
	      exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
	    return((xlock.usedtime = 0));
	}

	if(xlock.picktyp) {	/* blade */

	    if(rn2(1000-(int)uwep->spe) > (992-greatest_erosion(uwep)*10) &&
	       !uwep->cursed && !obj_resists(uwep, 0, 99)) {
		/* for a +0 weapon, probability that it survives an unsuccessful
		 * attempt to force the lock is (.992)^50 = .67
		 */
#ifndef JP
		pline("%sour %s broke!",
		      (uwep->quan > 1L) ? "One of y" : "Y", xname(uwep));
#else
		Your("%sが%s折れた！", xname(uwep),
		      (uwep->quan > 1L) ? "1本" : "");
#endif
		useup(uwep);
		You(E_J("give up your attempt to force the lock.",
			"錠をこじ開けるのをあきらめた。"));
		exercise(A_DEX, TRUE);
		return((xlock.usedtime = 0));
	    }
	} else			/* blunt */
	    wake_nearby();	/* due to hammering on the container */

	if(rn2(100) >= xlock.chance) return(1);		/* still busy */

	You(E_J("succeed in forcing the lock.","錠を壊すのに成功した。"));
	xlock.box->olocked = 0;
	xlock.box->obroken = 1;
	if(!xlock.picktyp && !rn2(3)) {
	    struct monst *shkp;
	    boolean costly;
	    long loss = 0L;

	    costly = (*u.ushops && costly_spot(u.ux, u.uy));
	    shkp = costly ? shop_keeper(*u.ushops) : 0;

#ifndef JP
	    pline("In fact, you've totally destroyed %s.",
		  the(xname(xlock.box)));
#else
	    pline("実際のところ、あなたは%sを完全に破壊してしまった。",
		  xname(xlock.box));
#endif /*JP*/

	    /* Put the contents on ground at the hero's feet. */
	    while ((otmp = xlock.box->cobj) != 0) {
		obj_extract_self(otmp);
		if(!rn2(3) || otmp->oclass == POTION_CLASS) {
		    chest_shatter_msg(otmp);
		    if (costly)
		        loss += stolen_value(otmp, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
		    if (otmp->quan == 1L) {
			obfree(otmp, (struct obj *) 0);
			continue;
		    }
		    useup(otmp);
		}
		if (xlock.box->otyp == ICE_BOX && otmp->otyp == CORPSE) {
		    otmp->age = monstermoves - otmp->age; /* actual age */
		    start_corpse_timeout(otmp);
		}
		place_object(otmp, u.ux, u.uy);
		stackobj(otmp);
	    }

	    if (costly)
		loss += stolen_value(xlock.box, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
	    if(loss) You(E_J("owe %ld %s for objects destroyed.",
			     "壊した品物の代金%ld%sを弁償することになった。"),
			 loss, currency(loss));
	    delobj(xlock.box);
	}
	exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
	return((xlock.usedtime = 0));
}

#endif /* OVLB */
#ifdef OVL0

void
reset_pick()
{
	xlock.usedtime = xlock.chance = xlock.picktyp = 0;
	xlock.door = 0;
	xlock.box = 0;
}

#endif /* OVL0 */
#ifdef OVLB

int
pick_lock(pick) /* pick a lock with a given object */
	register struct	obj	*pick;
{
	int picktyp, c, ch;
	coord cc;
	struct rm	*door;
	struct obj	*otmp;
	char qbuf[QBUFSZ];

	picktyp = pick->otyp;

	/* check whether we're resuming an interrupted previous attempt */
	if (xlock.usedtime && picktyp == xlock.picktyp) {
	    static char no_longer[] =
		E_J("Unfortunately, you can no longer %s %s.",
		    "残念ながら、あなたはもはや%s%s。");

	    if (nohands(youmonst.data)) {
		const char *what = (picktyp == LOCK_PICK) ? E_J("pick","鍵開け器具") : E_J("key","鍵");
#ifdef TOURIST
		if (picktyp == CREDIT_CARD) what = E_J("card","カード");
#endif
		pline(no_longer, E_J("hold the",what), E_J(what,"を持っていられない"));
		reset_pick();
		return 0;
	    } else if (xlock.box && !can_reach_floor()) {
		pline(no_longer, E_J("reach the","鍵まで"), E_J("lock","届かない。"));
		reset_pick();
		return 0;
	    } else {
		const char *action = lock_action();
		You(E_J("resume your attempt at %s.","%sのを再開した。"), action);
		set_occupation(picklock, E_J(action,occup_lock_action()), 0);
		return(1);
	    }
	}

	if(nohands(youmonst.data)) {
		You_cant(E_J("hold %s -- you have no hands!",
			     "%sをつかむことができない ─ あなたには手がない！"), doname(pick));
		return(0);
	}

	if((picktyp != LOCK_PICK &&
#ifdef TOURIST
	    picktyp != CREDIT_CARD &&
#endif
	    picktyp != SKELETON_KEY)) {
		impossible("picking lock with object %d?", picktyp);
		return(0);
	}
	ch = 0;		/* lint suppression */

	if(!get_adjacent_loc((char *)0, E_J("Invalid location!","その位置は指定できません！"), u.ux, u.uy, &cc)) return 0;
	if (cc.x == u.ux && cc.y == u.uy) {	/* pick lock on a container */
	    const char *verb;
	    boolean it;
	    int count;

	    if (u.dz < 0) {
#ifndef JP
		There("isn't any sort of lock up %s.",
		      Levitation ? "here" : "there");
#else
		pline("上%sには鍵をかけるようなものは何もない。",
		      Levitation ? "" : "のほう");
#endif /*JP*/
		return 0;
	    } else if (is_lava(u.ux, u.uy)) {
		pline(E_J("Doing that would probably melt your %s.",
			  "そんなことをすれば、%sは融けてしまうだろう。"),
		      xname(pick));
		return 0;
	    } else if (is_pool(u.ux, u.uy) && !Underwater) {
		pline_The(E_J("water has no lock.","水には錠前はない。"));
		return 0;
	    }

	    count = 0;
	    c = 'n';			/* in case there are no boxes here */
	    for(otmp = level.objects[cc.x][cc.y]; otmp; otmp = otmp->nexthere)
		if (Is_box(otmp)) {
		    ++count;
		    if (!can_reach_floor()) {
			You_cant(E_J("reach %s from up here.","今の高い位置からでは%sに届かない。"),
				 E_J(the(xname(otmp)),xname(otmp)));
			return 0;
		    }
		    it = 0;
		    if (otmp->obroken) verb = E_J("fix","壊された錠を直し");
		    else if (!otmp->olocked) verb = E_J("lock","鍵をかけ"), it = 1;
		    else if (picktyp != LOCK_PICK) verb = E_J("unlock","鍵を開け"), it = 1;
		    else verb = E_J("pick","錠をこじ開け");
#ifndef JP
		    Sprintf(qbuf, "There is %s here, %s %s?",
		    	    safe_qbuf("", sizeof("There is  here, unlock its lock?"),
			    	doname(otmp), an(simple_typename(otmp->otyp)), "a box"),
			    verb, it ? "it" : "its lock");
#else
		    Sprintf(qbuf, "ここには%sがある。%sますか？",
		    	    safe_qbuf("", sizeof("ここにはがある。壊された錠を直しますか？"),
			    	doname(otmp), simple_typename(otmp->otyp), "箱"),
			    verb);
#endif /*JP*/

		    c = ynq(qbuf);
		    if(c == 'q') return(0);
		    if(c == 'n') continue;

		    if (otmp->obroken) {
			You_cant(E_J("fix its broken lock with %s.",
				     "%sを使って壊された錠を直すことはできない。"), doname(pick));
			return 0;
		    }
#ifdef TOURIST
		    else if (picktyp == CREDIT_CARD && !otmp->olocked) {
			/* credit cards are only good for unlocking */
#ifndef JP
			You_cant("do that with %s.", doname(pick));
#else
			pline("%sで鍵をかけることはできない。",doname(pick));
#endif /*JP*/
			return 0;
		    }
#endif
		    switch(picktyp) {
#ifdef TOURIST
			case CREDIT_CARD:
			    ch = ACURR(A_DEX) + 20*Role_if(PM_ROGUE);
			    break;
#endif
			case LOCK_PICK:
			    ch = 3*ACURR(A_DEX) + 25*Role_if(PM_ROGUE);
			    break;
			case SKELETON_KEY:
			    ch = 65 + ACURR(A_DEX) + 10*Role_if(PM_ROGUE);
			    if (pick->oartifact == ART_MASTER_KEY_OF_THIEVERY)
				ch = 100;
			    break;
			default:	ch = 0;
		    }
		    if(pick->cursed) ch /= 2;
		    if(pick->oeroded || pick->oeroded2) ch /= greatest_erosion(pick)+1;
		    if (!ch || (!pick->oartifact && !rn2(ch))) {
			switch(picktyp) {
				case CREDIT_CARD:
					Your(E_J("credit card breaks in half!",
						 "クレジットカードはまっぷたつに折れた！"));
					break;
				case LOCK_PICK:
					You(E_J("break your pick!","金具を折ってしまった！"));
					break;
				case SKELETON_KEY:
					Your(E_J("key didn't quite fit the lock and snapped!",
						 "鍵は鍵穴にうまく合わず、へし折れてしまった！"));
					break;
				default:
					impossible("Your ??? broke?");
			}
			useup(pick);
			return(0);
		    }

		    xlock.picktyp = picktyp;
		    xlock.box = otmp;
		    xlock.door = 0;
		    break;
		}
	    if (c != 'y') {
		if (!count)
		    There(E_J("doesn't seem to be any sort of lock here.",
			      "施錠されているようなものは何も見当たらない。"));
		return(0);		/* decided against all boxes */
	    }
	} else {			/* pick the lock in a door */
	    struct monst *mtmp;

	    if (u.utrap && u.utraptype == TT_PIT) {
		You_cant(E_J("reach over the edge of the pit.",
			     "落し穴の中からは届かない。"));
		return(0);
	    }

	    door = &levl[cc.x][cc.y];
	    if ((mtmp = m_at(cc.x, cc.y)) && canseemon(mtmp)
			&& mtmp->m_ap_type != M_AP_FURNITURE
			&& mtmp->m_ap_type != M_AP_OBJECT) {
#ifdef TOURIST
		if (picktyp == CREDIT_CARD &&
		    (mtmp->isshk || mtmp->data == &mons[PM_ORACLE]))
		    verbalize(E_J("No checks, no credit, no problem.","いつもニコニコ現金払い。"));
		else
#endif
		    pline(E_J("I don't think %s would appreciate that.",
			      "%sにその価値がわかるとは思えない。"), mon_nam(mtmp));
		return(0);
	    }
	    if(!IS_DOOR(door->typ)) {
		if (is_drawbridge_wall(cc.x,cc.y) >= 0)
#ifndef JP
		    You("%s no lock on the drawbridge.",
				Blind ? "feel" : "see");
#else
		    pline("跳ね橋には鍵穴が%s。", Blind ? "ないようだ" : "見当たらない");
#endif /*JP*/
		else
#ifndef JP
		    You("%s no door there.",
				Blind ? "feel" : "see");
#else
		    pline("そこには扉がない%s。", Blind ? "ようだ" : "");
#endif /*JP*/
		return(0);
	    }
	    switch (door->doormask) {
		case D_NODOOR:
		    pline(E_J("This doorway has no door.","この出入り口には扉がない。"));
		    return(0);
		case D_ISOPEN:
		    You(E_J("cannot lock an open door.","開いた扉には鍵をかけられない。"));
		    return(0);
		case D_BROKEN:
		    pline(E_J("This door is broken.","この扉は壊されている。"));
		    return(0);
		default:
#ifdef TOURIST
		    /* credit cards are only good for unlocking */
		    if(picktyp == CREDIT_CARD && !(door->doormask & D_LOCKED)) {
			You_cant(E_J("lock a door with a credit card.",
				     "クレジットカードで鍵をかけることはできない。"));
			return(0);
		    }
#endif

		    Sprintf(qbuf,E_J("%sock it?","鍵を%sけますか？"),
			(door->doormask & D_LOCKED) ? E_J("Unl","開") : E_J("L","か") );

		    c = yn(qbuf);
		    if(c == 'n') return(0);

		    switch(picktyp) {
#ifdef TOURIST
			case CREDIT_CARD:
			    ch = 2*ACURR(A_DEX) + 20*Role_if(PM_ROGUE);
			    break;
#endif
			case LOCK_PICK:
			    ch = 3*ACURR(A_DEX) + 30*Role_if(PM_ROGUE);
			    break;
			case SKELETON_KEY:
			    ch = 70 + ACURR(A_DEX);
			    if (pick->oartifact == ART_MASTER_KEY_OF_THIEVERY)
				ch = 100;
			    break;
			default:
			    ch = 0;
			    break;
		    }
		    if(pick->oeroded || pick->oeroded2) ch /= greatest_erosion(pick)+1;
		    if (!ch || (!pick->oartifact && !rn2(ch))) {
			switch(picktyp) {
			    case CREDIT_CARD:
				You(E_J("break your card off in the door!",
					"扉の隙間でカードを折ってしまった！"));
				break;
			    case LOCK_PICK:
				You(E_J("break your pick!","金具を折ってしまった！"));
				break;
			    case SKELETON_KEY:
				Your(E_J("key wasn't designed for this door and broke!",
					 "鍵はこの扉の錠には合っておらず、壊れてしまった！"));
				break;
			    default:
				impossible("Your ??? broke?");
				break;
			}
			useup(pick);
			return(0);
		    }
		    xlock.door = door;
		    xlock.box = 0;
	    }
	}
	flags.move = 0;
	xlock.chance = ch;
	xlock.picktyp = picktyp;
	xlock.usedtime = 0;
	set_occupation(picklock, E_J(lock_action(),occup_lock_action()), 0);
	return(1);
}

int
doforce()		/* try to force a chest with your weapon */
{
	register struct obj *otmp;
	register int c, picktyp;
	char qbuf[QBUFSZ];

	if(!uwep ||	/* proper type test */
	   (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep) &&
	    uwep->oclass != ROCK_CLASS) ||
	   (objects[uwep->otyp].oc_skill < P_DAGGER_GROUP) ||
	   (objects[uwep->otyp].oc_skill > P_SPEAR_GROUP) ||
	   uwep->otyp == FLAIL || uwep->otyp == AKLYS
#ifdef KOPS
	   || uwep->otyp == RUBBER_HOSE
#endif
	  ) {
#ifndef JP
	    You_cant("force anything without a %sweapon.",
		  (uwep) ? "proper " : "");
#else
	    pline("錠をこじるには、%s武器が必要だ。", (uwep) ? "それなりの" : "");
#endif /*JP*/
	    return(0);
	}

	picktyp = is_blade(uwep);
	if(xlock.usedtime && xlock.box && picktyp == xlock.picktyp) {
	    You(E_J("resume your attempt to force the lock.",
		    "錠を壊す作業を再開した。"));
	    set_occupation(forcelock, E_J("forcing the lock","錠を壊す作業"), 0);
	    return(1);
	}

	/* A lock is made only for the honest man, the thief will break it. */
	xlock.box = (struct obj *)0;
	for(otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
	    if(Is_box(otmp)) {
		if (otmp->obroken || !otmp->olocked) {
		    There(E_J("is %s here, but its lock is already %s.",
			      "%sがあるが、錠はすでに%sれている。"),
			  doname(otmp), otmp->obroken ? E_J("broken","壊さ") : E_J("unlocked","開けら"));
		    continue;
		}
#ifndef JP
		Sprintf(qbuf,"There is %s here, force its lock?",
			safe_qbuf("", sizeof("There is  here, force its lock?"),
				doname(otmp), an(simple_typename(otmp->otyp)),
				"a box"));
#else
		Sprintf(qbuf,"ここには%sがある。錠を壊しますか？",
			safe_qbuf("", sizeof("ここにはがある。錠を壊しますか？"),
				doname(otmp), simple_typename(otmp->otyp), "箱"));
#endif /*JP*/

		c = ynq(qbuf);
		if(c == 'q') return(0);
		if(c == 'n') continue;

		if(picktyp)
		    You(E_J("force your %s into a crack and pry.",
			    "%sを隙間にこじ入れ、力をこめた。"), xname(uwep));
		else
		    You(E_J("start bashing it with your %s.",
			    "%sで錠を打ち壊しにかかった。"), xname(uwep));
		xlock.box = otmp;
		xlock.chance = (int)(objects[uwep->otyp].oc_wldam & 0x1f)/* * 2*/;
		xlock.picktyp = picktyp;
		xlock.usedtime = 0;
		break;
	    }

	if(xlock.box)	set_occupation(forcelock, E_J("forcing the lock","錠を壊す作業"), 0);
	else		You(E_J("decide not to force the issue.","物事の強引な解決はやめることにした。"));
	return(1);
}

int
doopen()		/* try to open a door */
{
	coord cc;
	register struct rm *door;
	struct monst *mtmp;

	if (nohands(youmonst.data)) {
	    You_cant(E_J("open anything -- you have no hands!",
			 "何も開けられない ─ あなたには手がない！"));
	    return 0;
	}

	if (u.utrap && u.utraptype == TT_PIT) {
	    You_cant(E_J("reach over the edge of the pit.",
			 "落し穴の中からは届かない。"));
	    return 0;
	}

	if(!get_adjacent_loc((char *)0, (char *)0, u.ux, u.uy, &cc)) return(0);

	if((cc.x == u.ux) && (cc.y == u.uy)) return(0);

	if ((mtmp = m_at(cc.x,cc.y))			&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp);
	    return(1);
	}

	door = &levl[cc.x][cc.y];

	if(!IS_DOOR(door->typ)) {
		if (is_db_wall(cc.x,cc.y)) {
#ifndef JP
		    There("is no obvious way to open the drawbridge.");
#else
		    pline("跳ね橋を下ろす明確な手段が見当たらない。");
#endif /*JP*/
		    return(0);
		}
#ifndef JP
		You("%s no door there.",
				Blind ? "feel" : "see");
#else
		pline("ここには扉が%s。", Blind ? "ないようだ" : "見当たらない");
#endif /*JP*/
		return(0);
	}

	if (!(door->doormask & D_CLOSED)) {
	    const char *mesg;

	    switch (door->doormask) {
	    case D_BROKEN: mesg = E_J(" is broken","扉は壊されている"); break;
	    case D_NODOOR: mesg = E_J("way has no door","この出入り口には扉がない"); break;
	    case D_ISOPEN: mesg = E_J(" is already open","扉はすでに開いている"); break;
	    default:	   mesg = E_J(" is locked","扉には鍵がかけられている"); break;
	    }
	    pline(E_J("This door%s.","%s。"), mesg);
	    if (Blind) feel_location(cc.x,cc.y);
	    return(0);
	}

	if(verysmall(youmonst.data)) {
	    pline(E_J("You're too small to pull the door open.",
		      "あなたの体は小さすぎて、扉を開けられない。"));
	    return(0);
	}

	/* door is known to be CLOSED */
	if (rnl(20) < (ACURRSTR+ACURR(A_DEX)+ACURR(A_CON))/3) {
#ifndef JP
	    pline_The("door opens.");
#else
	    pline("扉が開いた。");
#endif /*JP*/
	    if(door->doormask & D_TRAPPED) {
		b_trapped(E_J("door","扉"), FINGER);
		door->doormask = D_NODOOR;
		if (*in_rooms(cc.x, cc.y, SHOPBASE)) add_damage(cc.x, cc.y, 0L);
	    } else
		door->doormask = D_ISOPEN;
	    if (Blind)
		feel_location(cc.x,cc.y);	/* the hero knows she opened it  */
	    else
		newsym(cc.x,cc.y);
	    unblock_point(cc.x,cc.y);		/* vision: new see through there */
	} else {
	    exercise(A_STR, TRUE);
#ifndef JP
	    pline_The("door resists!");
#else
	    pline("この扉は重い！");
#endif /*JP*/
	}

	return(1);
}

STATIC_OVL
boolean
obstructed(x,y)
register int x, y;
{
	register struct monst *mtmp = m_at(x, y);

	if(mtmp && mtmp->m_ap_type != M_AP_FURNITURE) {
		if (mtmp->m_ap_type == M_AP_OBJECT) goto objhere;
		pline(E_J("%s stands in the way!","%sが立ちはだかっている！"), !canspotmon(mtmp) ?
		      E_J("Some creature","何者か") : Monnam(mtmp));
		if (!canspotmon(mtmp))
		    map_invisible(mtmp->mx, mtmp->my);
		return(TRUE);
	}
	if (OBJ_AT(x, y)) {
objhere:	pline(E_J("%s's in the way.","戸口に%sが置かれている。"), Something);
		return(TRUE);
	}
	return(FALSE);
}

int
doclose()		/* try to close a door */
{
	register int x, y;
	register struct rm *door;
	struct monst *mtmp;

	if (nohands(youmonst.data)) {
	    You_cant(E_J("close anything -- you have no hands!",
			 "何も閉められない ─ あなたには手がない！"));
	    return 0;
	}

	if (u.utrap && u.utraptype == TT_PIT) {
	    You_cant(E_J("reach over the edge of the pit.",
			 "落し穴の中からは届かない。"));
	    return 0;
	}

	if(!getdir((char *)0)) return(0);

	x = u.ux + u.dx;
	y = u.uy + u.dy;
	if((x == u.ux) && (y == u.uy)) {
#ifndef JP
		You("are in the way!");
#else
		pline("あなた自身が戸口に立っている！");
#endif /*JP*/
		return(1);
	}

	if ((mtmp = m_at(x,y))				&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp);
	    return(1);
	}

	door = &levl[x][y];

	if(!IS_DOOR(door->typ)) {
		if (door->typ == DRAWBRIDGE_DOWN)
#ifndef JP
		    There("is no obvious way to close the drawbridge.");
#else
		    pline("跳ね橋を上げる明確な手段が見当たらない。");
#endif /*JP*/
		else
#ifndef JP
		    You("%s no door there.",
				Blind ? "feel" : "see");
#else
		    pline("ここには扉が%s。", Blind ? "ないようだ" : "見当たらない");
#endif /*JP*/
		return(0);
	}

	if(door->doormask == D_NODOOR) {
	    pline(E_J("This doorway has no door.","この出入り口には扉がない。"));
	    return(0);
	}

	if(obstructed(x, y)) return(0);

	if(door->doormask == D_BROKEN) {
	    pline(E_J("This door is broken.","扉は壊されている。"));
	    return(0);
	}

	if(door->doormask & (D_CLOSED | D_LOCKED)) {
	    pline(E_J("This door is already closed.","扉はすでに閉じられている。"));
	    return(0);
	}

	if(door->doormask == D_ISOPEN) {
	    if(verysmall(youmonst.data)
#ifdef STEED
		&& !u.usteed
#endif
		) {
		 pline(E_J("You're too small to push the door closed.",
			   "あなたの体は小さすぎて、扉を閉められない。" ));
		 return(0);
	    }
	    if (
#ifdef STEED
		 u.usteed ||
#endif
		rn2(25) < (ACURRSTR+ACURR(A_DEX)+ACURR(A_CON))/3) {
#ifndef JP
		pline_The("door closes.");
#else
		pline("扉を閉めた。");
#endif /*JP*/
		door->doormask = D_CLOSED;
		if (Blind)
		    feel_location(x,y);	/* the hero knows she closed it */
		else
		    newsym(x,y);
		block_point(x,y);	/* vision:  no longer see there */
	    }
	    else {
	        exercise(A_STR, TRUE);
#ifndef JP
	        pline_The("door resists!");
#else
		pline("この扉は重い！");
#endif /*JP*/
	    }
	}

	return(1);
}

boolean			/* box obj was hit with spell effect otmp */
boxlock(obj, otmp)	/* returns true if something happened */
register struct obj *obj, *otmp;	/* obj *is* a box */
{
	register boolean res = 0;

	switch(otmp->otyp) {
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	    if (!obj->olocked) {	/* lock it; fix if broken */
		pline(E_J("Klunk!","ガチャリ！"));
		obj->olocked = 1;
		obj->obroken = 0;
		res = 1;
	    } /* else already closed and locked */
	    break;
	case WAN_OPENING:
	case SPE_KNOCK:
	    if (obj->olocked) {		/* unlock; couldn't be broken */
		pline(E_J("Klick!","カチリ！"));
		obj->olocked = 0;
		res = 1;
	    } else			/* silently fix if broken */
		obj->obroken = 0;
	    break;
	case WAN_POLYMORPH:
	case SPE_POLYMORPH:
	    /* maybe start unlocking chest, get interrupted, then zap it;
	       we must avoid any attempt to resume unlocking it */
	    if (xlock.box == obj)
		reset_pick();
	    break;
	}
	return res;
}

boolean			/* Door/secret door was hit with spell effect otmp */
doorlock(otmp,x,y)	/* returns true if something happened */
struct obj *otmp;
int x, y;
{
	register struct rm *door = &levl[x][y];
	boolean res = TRUE;
	int loudness = 0;
	const char *msg = (const char *)0;
	const char *dustcloud = E_J("A cloud of dust","埃の山");
	const char *quickly_dissipates = E_J("quickly dissipates","すぐに四散した。");
	
	if (door->typ == SDOOR) {
	    switch (otmp->otyp) {
	    case WAN_OPENING:
	    case SPE_KNOCK:
	    case WAN_STRIKING:
	    case SPE_FORCE_BOLT:
		door->typ = DOOR;
		door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
		newsym(x,y);
		if (cansee(x,y)) pline(E_J("A door appears in the wall!",
					   "壁に扉が現れた！"));
		if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK)
		    return TRUE;
		break;		/* striking: continue door handling below */
	    case WAN_LOCKING:
	    case SPE_WIZARD_LOCK:
	    default:
		return FALSE;
	    }
	}

	switch(otmp->otyp) {
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
#ifdef REINCARNATION
	    if (Is_rogue_level(&u.uz)) {
	    	boolean vis = cansee(x,y);
		/* Can't have real locking in Rogue, so just hide doorway */
		if (vis) pline(E_J("%s springs up in the older, more primitive doorway.",
				   "古びた、単純な出入り口に%が舞い上がった。"),
			dustcloud);
		else
			You_hear(E_J("a swoosh.","風の巻き起こる音を"));
		if (obstructed(x,y)) {
#ifndef JP
			if (vis) pline_The("cloud %s.",quickly_dissipates);
#else
			if (vis) pline("埃は%s",quickly_dissipates);
#endif /*JP*/
			return FALSE;
		}
		block_point(x, y);
		door->typ = SDOOR;
#ifndef JP
		if (vis) pline_The("doorway vanishes!");
#else
		if (vis) pline("出入り口は塞がれた！");
#endif /*JP*/
		newsym(x,y);
		return TRUE;
	    }
#endif
	    if (obstructed(x,y)) return FALSE;
	    /* Don't allow doors to close over traps.  This is for pits */
	    /* & trap doors, but is it ever OK for anything else? */
	    if (t_at(x,y)) {
		/* maketrap() clears doormask, so it should be NODOOR */
		pline(
		E_J("%s springs up in the doorway, but %s.",
		    "%sが戸口に舞い上がったが、%s"),
		dustcloud, quickly_dissipates);
		return FALSE;
	    }

	    switch (door->doormask & ~D_TRAPPED) {
	    case D_CLOSED:
		msg = E_J("The door locks!","扉は施錠された！");
		break;
	    case D_ISOPEN:
		msg = E_J("The door swings shut, and locks!","扉は勢いよく閉まり、施錠された！");
		break;
	    case D_BROKEN:
		msg = E_J("The broken door reassembles and locks!",
			  "壊れた扉は元通りに戻り、施錠された！");
		break;
	    case D_NODOOR:
		msg =
		E_J("A cloud of dust springs up and assembles itself into a door!",
		    "埃の山が舞い上がって集まり、扉を形作った！");
		break;
	    default:
		res = FALSE;
		break;
	    }
	    block_point(x, y);
	    door->doormask = D_LOCKED | (door->doormask & D_TRAPPED);
	    newsym(x,y);
	    break;
	case WAN_OPENING:
	case SPE_KNOCK:
	    if (door->doormask & D_LOCKED) {
		msg = E_J("The door unlocks!","扉の鍵が外れた！");
		door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
	    } else res = FALSE;
	    break;
	case WAN_STRIKING:
	case SPE_FORCE_BOLT:
	    if (door->doormask & (D_LOCKED | D_CLOSED)) {
		if (door->doormask & D_TRAPPED) {
		    if (MON_AT(x, y))
			(void) mb_trapped(m_at(x,y));
		    else if (flags.verbose) {
			if (cansee(x,y))
			    pline(E_J("KABOOM!!  You see a door explode.",
				      "ボカーン!!　あなたは扉が爆発するのを見た。"));
			else if (flags.soundok)
			    You_hear(E_J("a distant explosion.","遠くの爆発音を"));
		    }
		    door->doormask = D_NODOOR;
		    unblock_point(x,y);
		    newsym(x,y);
		    loudness = 40;
		    break;
		}
		door->doormask = D_BROKEN;
		if (flags.verbose) {
		    if (cansee(x,y))
#ifndef JP
			pline_The("door crashes open!");
#else
			pline("扉はこなごなに砕けた！");
#endif /*JP*/
		    else if (flags.soundok)
			You_hear(E_J("a crashing sound.","何かが壊れる音を"));
		}
		unblock_point(x,y);
		newsym(x,y);
		/* force vision recalc before printing more messages */
		if (vision_full_recalc) vision_recalc(0);
		loudness = 20;
	    } else res = FALSE;
	    break;
	default: impossible("magic (%d) attempted on door.", otmp->otyp);
	    break;
	}
	if (msg && cansee(x,y)) pline(msg);
	if (loudness > 0) {
	    /* door was destroyed */
	    wake_nearto(x, y, loudness);
	    if (*in_rooms(x, y, SHOPBASE)) add_damage(x, y, 0L);
	}

	if (res && picking_at(x, y)) {
	    /* maybe unseen monster zaps door you're unlocking */
	    stop_occupation();
	    reset_pick();
	}
	return res;
}

STATIC_OVL void
chest_shatter_msg(otmp)
struct obj *otmp;
{
	const char *disposition;
	const char *thing;
	long save_Blinded;

	if (otmp->oclass == POTION_CLASS) {
#ifndef JP
		You("%s %s shatter!", Blind ? "hear" : "see", an(bottlename()));
#else
		if (Blind) You_hear("!%sが割れる音を", bottlename());
		else	   pline("%sは割れた！");
#endif /*JP*/
		if (!breathless(youmonst.data) || haseyes(youmonst.data))
			potionbreathe(otmp);
		return;
	}
	/* We have functions for distant and singular names, but not one */
	/* which does _both_... */
	save_Blinded = Blinded;
	Blinded = 1;
	thing = singular(otmp, xname);
	Blinded = save_Blinded;
	switch (get_material(otmp)) {
	case PAPER:	disposition = E_J("is torn to shreds","千切れ飛んだ");
		break;
	case WAX:	disposition = E_J("is crushed","砕け散った");
		break;
	case VEGGY:	disposition = E_J("is pulped","ぐちゃぐちゃに潰れた");
		break;
	case FLESH:	disposition = E_J("is mashed","叩き潰された");
		break;
	case GLASS:	disposition = E_J("shatters","こなごなに割れた");
		break;
	case WOOD:	disposition = E_J("splinters to fragments","木っ端微塵になった");
		break;
	default:	disposition = E_J("is destroyed","破壊された");
		break;
	}
	pline(E_J("%s %s!","%sは%s！"), E_J(An(thing),thing), disposition);
}

#endif /* OVLB */

/*lock.c*/
