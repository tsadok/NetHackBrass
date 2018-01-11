/*	SCCS Id: @(#)vault.c	3.4	2003/01/15	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "vault.h"

STATIC_DCL struct monst *NDECL(findgd);

#define g_monnam(mtmp) \
	x_monnam(mtmp, ARTICLE_NONE, (char *)0, SUPPRESS_IT, FALSE)

#ifdef OVLB

STATIC_DCL boolean FDECL(clear_fcorr, (struct monst *,BOOLEAN_P));
STATIC_DCL void FDECL(restfakecorr,(struct monst *));
STATIC_DCL boolean FDECL(in_fcorridor, (struct monst *,int,int));
STATIC_DCL void FDECL(move_gold,(struct obj *,int));
STATIC_DCL void FDECL(wallify_vault,(struct monst *));

STATIC_OVL boolean
clear_fcorr(grd, forceshow)
register struct monst *grd;
register boolean forceshow;
{
	register int fcx, fcy, fcbeg;
	register struct monst *mtmp;

	if (!on_level(&(EGD(grd)->gdlevel), &u.uz)) return TRUE;

	while((fcbeg = EGD(grd)->fcbeg) < EGD(grd)->fcend) {
		fcx = EGD(grd)->fakecorr[fcbeg].fx;
		fcy = EGD(grd)->fakecorr[fcbeg].fy;
		if((grd->mhp <= 0 || !in_fcorridor(grd, u.ux, u.uy)) &&
				   EGD(grd)->gddone)
			forceshow = TRUE;
		if((u.ux == fcx && u.uy == fcy && grd->mhp > 0)
			|| (!forceshow && couldsee(fcx,fcy))
			|| (Punished && !carried(uball)
				&& uball->ox == fcx && uball->oy == fcy))
			return FALSE;

		if ((mtmp = m_at(fcx,fcy)) != 0) {
			if(mtmp->isgd) return(FALSE);
			else if(!in_fcorridor(grd, u.ux, u.uy)) {
			    if(mtmp->mtame) yelp(mtmp);
			    (void) rloc(mtmp, FALSE);
			}
		}
		levl[fcx][fcy].typ = EGD(grd)->fakecorr[fcbeg].ftyp;
		map_location(fcx, fcy, 1);	/* bypass vision */
		if(!ACCESSIBLE(levl[fcx][fcy].typ)) block_point(fcx,fcy);
		EGD(grd)->fcbeg++;
	}
	if(grd->mhp <= 0) {
	    pline_The(E_J("corridor disappears.","通路は消え失せた。"));
	    if(IS_ROCK(levl[u.ux][u.uy].typ)) You(E_J("are encased in rock.","岩に閉じ込められた。"));
	}
	return(TRUE);
}

STATIC_OVL void
restfakecorr(grd)
register struct monst *grd;
{
	/* it seems you left the corridor - let the guard disappear */
	if(clear_fcorr(grd, FALSE)) mongone(grd);
}

boolean
grddead(grd)				/* called in mon.c */
register struct monst *grd;
{
	register boolean dispose = clear_fcorr(grd, TRUE);

	if(!dispose) {
		/* see comment by newpos in gd_move() */
		remove_monster(grd->mx, grd->my);
		newsym(grd->mx, grd->my);
		place_monster(grd, 0, 0);
		EGD(grd)->ogx = grd->mx;
		EGD(grd)->ogy = grd->my;
		dispose = clear_fcorr(grd, TRUE);
	}
	return(dispose);
}

STATIC_OVL boolean
in_fcorridor(grd, x, y)
register struct monst *grd;
int x, y;
{
	register int fci;

	for(fci = EGD(grd)->fcbeg; fci < EGD(grd)->fcend; fci++)
		if(x == EGD(grd)->fakecorr[fci].fx &&
				y == EGD(grd)->fakecorr[fci].fy)
			return(TRUE);
	return(FALSE);
}

STATIC_OVL
struct monst *
findgd()
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if(mtmp->isgd && !DEADMONSTER(mtmp) && on_level(&(EGD(mtmp)->gdlevel), &u.uz))
		return(mtmp);
	return((struct monst *)0);
}

#endif /* OVLB */
#ifdef OVL0

char
vault_occupied(array)
char *array;
{
	register char *ptr;

	for (ptr = array; *ptr; ptr++)
		if (rooms[*ptr - ROOMOFFSET].rtype == VAULT)
			return(*ptr);
	return('\0');
}

void
invault()
{
#ifdef BSD_43_BUG
    int dummy;		/* hack to avoid schain botch */
#endif
    struct monst *guard;
    int trycount, vaultroom = (int)vault_occupied(u.urooms);

    if(!vaultroom) {
	u.uinvault = 0;
	return;
    }

    vaultroom -= ROOMOFFSET;

    guard = findgd();
    if(++u.uinvault % 30 == 0 && !guard) { /* if time ok and no guard now. */
	char buf[BUFSZ];
	register int x, y, dd, gx, gy;
	int lx = 0, ly = 0;
#ifdef GOLDOBJ
        long umoney;
#endif
	/* first find the goal for the guard */
	for(dd = 2; (dd < ROWNO || dd < COLNO); dd++) {
	  for(y = u.uy-dd; y <= u.uy+dd; ly = y, y++) {
	    if(y < 0 || y > ROWNO-1) continue;
	    for(x = u.ux-dd; x <= u.ux+dd; lx = x, x++) {
	      if(y != u.uy-dd && y != u.uy+dd && x != u.ux-dd)
		x = u.ux+dd;
	      if(x < 1 || x > COLNO-1) continue;
	      if(levl[x][y].typ == CORR) {
		  if(x < u.ux) lx = x + 1;
		  else if(x > u.ux) lx = x - 1;
		  else lx = x;
		  if(y < u.uy) ly = y + 1;
		  else if(y > u.uy) ly = y - 1;
		  else ly = y;
		  if(levl[lx][ly].typ != STONE && levl[lx][ly].typ != CORR)
		      goto incr_radius;
		  goto fnd;
	      }
	    }
	  }
incr_radius: ;
	}
	impossible("Not a single corridor on this level??");
	tele();
	return;
fnd:
	gx = x; gy = y;

	/* next find a good place for a door in the wall */
	x = u.ux; y = u.uy;
	if(levl[x][y].typ != ROOM) {  /* player dug a door and is in it */
		if(levl[x+1][y].typ == ROOM)  x = x + 1;
		else if(levl[x][y+1].typ == ROOM) y = y + 1;
		else if(levl[x-1][y].typ == ROOM) x = x - 1;
		else if(levl[x][y-1].typ == ROOM) y = y - 1;
		else if(levl[x+1][y+1].typ == ROOM) {
			x = x + 1;
			y = y + 1;
		} else if (levl[x-1][y-1].typ == ROOM) {
			x = x - 1;
			y = y - 1;
		} else if (levl[x+1][y-1].typ == ROOM) {
			x = x + 1;
			y = y - 1;
		} else if (levl[x-1][y+1].typ == ROOM) {
			x = x - 1;
			y = y + 1;
		}
	}
	while(levl[x][y].typ == ROOM) {
		register int dx,dy;

		dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
		dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
		if(abs(gx-x) >= abs(gy-y))
			x += dx;
		else
			y += dy;
	}
	if(x == u.ux && y == u.uy) {
		if(levl[x+1][y].typ == HWALL || levl[x+1][y].typ == DOOR)
			x = x + 1;
		else if(levl[x-1][y].typ == HWALL || levl[x-1][y].typ == DOOR)
			x = x - 1;
		else if(levl[x][y+1].typ == VWALL || levl[x][y+1].typ == DOOR)
			y = y + 1;
		else if(levl[x][y-1].typ == VWALL || levl[x][y-1].typ == DOOR)
			y = y - 1;
		else return;
	}

	/* make something interesting happen */
	if(!(guard = makemon(&mons[PM_GUARD], x, y, NO_MM_FLAGS))) return;
	guard->isgd = 1;
	setmpeaceful(guard, TRUE);
	EGD(guard)->gddone = 0;
	EGD(guard)->ogx = x;
	EGD(guard)->ogy = y;
	assign_level(&(EGD(guard)->gdlevel), &u.uz);
	EGD(guard)->vroom = vaultroom;
	EGD(guard)->warncnt = 0;

	reset_faint();			/* if fainted - wake up */
	if (canspotmon(guard))
#ifndef JP
	    pline("Suddenly one of the Vault's %s enters!",
		  makeplural(g_monnam(guard)));
#else
	    pline("突然、金庫の%sの一人が入ってきた！",
		  g_monnam(guard));
#endif /*JP*/
	else
	    pline(E_J("Someone else has entered the Vault.",
		      "他の誰かが金庫に入ってきた。"));
	newsym(guard->mx,guard->my);
	if (youmonst.m_ap_type == M_AP_OBJECT || u.uundetected) {
	    if (youmonst.m_ap_type == M_AP_OBJECT &&
			youmonst.mappearance != GOLD_PIECE)
	    	verbalize(E_J("Hey! Who left that %s in here?",
			      "おい！ ここに%sを置いていったのは誰だ？"), mimic_obj_name(&youmonst));
	    /* You're mimicking some object or you're hidden. */
	    pline(E_J("Puzzled, %s turns around and leaves.",
		      "不思議そうな顔のまま、%sは出ていった。"), mhe(guard));
	    mongone(guard);
	    return;
	}
	if (Strangled || is_silent(youmonst.data) || multi < 0) {
	    /* [we ought to record whether this this message has already
	       been given in order to vary it upon repeat visits, but
	       discarding the monster and its egd data renders that hard] */
	    verbalize(E_J("I'll be back when you're ready to speak to me!",
			  "お前が喋れるようになったら、また来るからな！"));
	    mongone(guard);
	    return;
	}

	stop_occupation();		/* if occupied, stop it *now* */
	if (multi > 0) { nomul(0); unmul((char *)0); }
	trycount = 5;
	do {
	    getlin(E_J("\"Hello stranger, who are you?\" -",
		       "「こんにちは、見知らぬ方。お前は誰だ？」-"), buf);
	    (void) mungspaces(buf);
	} while (!letter(buf[0]) && --trycount > 0
#ifdef JP
		 && !is_kanji(buf[0])
#endif /*JP*/
		);

	if (u.ualign.type == A_LAWFUL &&
	    /* ignore trailing text, in case player includes character's rank */
	    strncmpi(buf, plname, (int) strlen(plname)) != 0) {
		adjalign(-1);		/* Liar! */
	}

	if (!strcmpi(buf, "Croesus") || !strcmpi(buf, "Kroisos")
#ifdef TOURIST
		|| !strcmpi(buf, "Creosote")
#endif
#ifdef JP
		|| !strcmp(buf, "クロイソス")
#endif /*JP*/
	    ) {
	    if (!mvitals[PM_CROESUS].died) {
		verbalize(E_J("Oh, yes, of course.  Sorry to have disturbed you.",
			      "あ、ええ、もちろんですとも。お邪魔して申し訳ありません。"));
		mongone(guard);
	    } else {
		setmangry(guard);
		verbalize(E_J("Back from the dead, are you?  I'll remedy that!",
			      "墓場から甦ったてわけか、え？ 私が送り返してやろう！"));
		/* don't want guard to waste next turn wielding a weapon */
		if (!MON_WEP(guard)) {
		    guard->weapon_check = NEED_HTH_WEAPON;
		    (void) mon_wield_item(guard);
		}
	    }
	    return;
	}
	verbalize(E_J("I don't know you.","知らないな。"));
#ifndef GOLDOBJ
	if (!u.ugold && !hidden_gold())
	    verbalize(E_J("Please follow me.","私について来なさい。"));
	else {
	    if (!u.ugold)
		verbalize(E_J("You have hidden gold.","お前は金を隠しているな。"));
	    verbalize(E_J("Most likely all your gold was stolen from this vault.",
			  "お前の所持金はほとんどこの金庫から盗んだもののようだ。"));
	    verbalize(E_J("Please drop that gold and follow me.",
			  "金をすべて置いて、私について来なさい。"));
	}
#else
        umoney = money_cnt(invent);
	if (!umoney && !hidden_gold())
	    verbalize(E_J("Please follow me.","ついて来なさい。"));
	else {
	    if (!umoney)
		verbalize(E_J("You have hidden money.","お前は金を隠しているな。"));
	    verbalize(E_J("Most likely all your money was stolen from this vault.",
			  "お前の所持金はほとんどこの金庫から盗んだもののようだ。"));
	    verbalize(E_J("Please drop that money and follow me.",
			  "金をすべて置いて、私について来なさい。"));
	}
#endif
	EGD(guard)->gdx = gx;
	EGD(guard)->gdy = gy;
	EGD(guard)->fcbeg = 0;
	EGD(guard)->fakecorr[0].fx = x;
	EGD(guard)->fakecorr[0].fy = y;
	if(IS_WALL(levl[x][y].typ))
	    EGD(guard)->fakecorr[0].ftyp = levl[x][y].typ;
	else { /* the initial guard location is a dug door */
	    int vlt = EGD(guard)->vroom;
	    xchar lowx = rooms[vlt].lx, hix = rooms[vlt].hx;
	    xchar lowy = rooms[vlt].ly, hiy = rooms[vlt].hy;

	    if(x == lowx-1 && y == lowy-1)
		EGD(guard)->fakecorr[0].ftyp = TLCORNER;
	    else if(x == hix+1 && y == lowy-1)
		EGD(guard)->fakecorr[0].ftyp = TRCORNER;
	    else if(x == lowx-1 && y == hiy+1)
		EGD(guard)->fakecorr[0].ftyp = BLCORNER;
	    else if(x == hix+1 && y == hiy+1)
		EGD(guard)->fakecorr[0].ftyp = BRCORNER;
	    else if(y == lowy-1 || y == hiy+1)
		EGD(guard)->fakecorr[0].ftyp = HWALL;
	    else if(x == lowx-1 || x == hix+1)
		EGD(guard)->fakecorr[0].ftyp = VWALL;
	}
	levl[x][y].typ = DOOR;
	levl[x][y].doormask = D_NODOOR;
	unblock_point(x, y);		/* doesn't block light */
	EGD(guard)->fcend = 1;
	EGD(guard)->warncnt = 1;
    }
}

#endif /* OVL0 */
#ifdef OVLB

STATIC_OVL void
move_gold(gold, vroom)
struct obj *gold;
int vroom;
{
	xchar nx, ny;

	remove_object(gold);
	newsym(gold->ox, gold->oy);
	nx = rooms[vroom].lx + rn2(2);
	ny = rooms[vroom].ly + rn2(2);
	place_object(gold, nx, ny);
	stackobj(gold);
	newsym(nx,ny);
}

STATIC_OVL void
wallify_vault(grd)
struct monst *grd;
{
	int x, y, typ;
	int vlt = EGD(grd)->vroom;
	char tmp_viz;
	xchar lox = rooms[vlt].lx - 1, hix = rooms[vlt].hx + 1,
	      loy = rooms[vlt].ly - 1, hiy = rooms[vlt].hy + 1;
	struct monst *mon;
	struct obj *gold;
	struct trap *trap;
	boolean fixed = FALSE;
	boolean movedgold = FALSE;

	for (x = lox; x <= hix; x++)
	    for (y = loy; y <= hiy; y++) {
		/* if not on the room boundary, skip ahead */
		if (x != lox && x != hix && y != loy && y != hiy) continue;

		if (!IS_WALL(levl[x][y].typ) && !in_fcorridor(grd, x, y)) {
		    if ((mon = m_at(x, y)) != 0 && mon != grd) {
			if (mon->mtame) yelp(mon);
			(void) rloc(mon, FALSE);
		    }
		    if ((gold = g_at(x, y)) != 0) {
			move_gold(gold, EGD(grd)->vroom);
			movedgold = TRUE;
		    }
		    if ((trap = t_at(x, y)) != 0)
			deltrap(trap);
		    if (x == lox)
			typ = (y == loy) ? TLCORNER :
			      (y == hiy) ? BLCORNER : VWALL;
		    else if (x == hix)
			typ = (y == loy) ? TRCORNER :
			      (y == hiy) ? BRCORNER : VWALL;
		    else  /* not left or right side, must be top or bottom */
			typ = HWALL;
		    levl[x][y].typ = typ;
		    levl[x][y].doormask = 0;
		    /*
		     * hack: player knows walls are restored because of the
		     * message, below, so show this on the screen.
		     */
		    tmp_viz = viz_array[y][x];
		    viz_array[y][x] = IN_SIGHT|COULD_SEE;
		    newsym(x,y);
		    viz_array[y][x] = tmp_viz;
		    block_point(x,y);
		    fixed = TRUE;
		}
	    }

	if(movedgold || fixed) {
	    if(in_fcorridor(grd, grd->mx, grd->my) || cansee(grd->mx, grd->my))
		pline_The(E_J("%s whispers an incantation.",
			      "%sは呪文を囁いた。"), g_monnam(grd));
	    else You_hear(E_J("a distant chant.","遠くに呪文の詠唱を"));
	    if(movedgold)
		pline(E_J("A mysterious force moves the gold into the vault.",
			  "不思議な力が金貨を金庫の中に移動させた。"));
	    if(fixed)
		pline_The(E_J("damaged vault's walls are magically restored!",
			      "壊された金庫の壁が、魔法で修復された！"));
	}
}

/*
 * return  1: guard moved,  0: guard didn't,  -1: let m_move do it,  -2: died
 */
int
gd_move(grd)
register struct monst *grd;
{
	int x, y, nx, ny, m, n;
	int dx, dy, gx, gy, fci;
	uchar typ;
	struct fakecorridor *fcp;
	register struct egd *egrd = EGD(grd);
	register struct rm *crm;
	register boolean goldincorridor = FALSE,
			 u_in_vault = vault_occupied(u.urooms)? TRUE : FALSE,
			 grd_in_vault = *in_rooms(grd->mx, grd->my, VAULT)?
					TRUE : FALSE;
	boolean disappear_msg_seen = FALSE, semi_dead = (grd->mhp <= 0);
#ifndef GOLDOBJ
	register boolean u_carry_gold = ((u.ugold + hidden_gold()) > 0L);
#else
        long umoney = money_cnt(invent);
	register boolean u_carry_gold = ((umoney + hidden_gold()) > 0L);
#endif
	boolean see_guard;

	if(!on_level(&(egrd->gdlevel), &u.uz)) return(-1);
	nx = ny = m = n = 0;
	if(!u_in_vault && !grd_in_vault)
	    wallify_vault(grd);
	if(!grd->mpeaceful) {
	    if(semi_dead) {
		egrd->gddone =1;
		goto newpos;
	    }
	    if(!u_in_vault &&
	       (grd_in_vault ||
		(in_fcorridor(grd, grd->mx, grd->my) &&
		 !in_fcorridor(grd, u.ux, u.uy)))) {
		(void) rloc(grd, FALSE);
		wallify_vault(grd);
		(void) clear_fcorr(grd, TRUE);
		goto letknow;
	    }
	    if(!in_fcorridor(grd, grd->mx, grd->my))
		(void) clear_fcorr(grd, TRUE);
	    return(-1);
	}
	if(abs(egrd->ogx - grd->mx) > 1 ||
			abs(egrd->ogy - grd->my) > 1)
		return(-1);	/* teleported guard - treat as monster */
	if(egrd->fcend == 1) {
	    if(u_in_vault &&
			(u_carry_gold || um_dist(grd->mx, grd->my, 1))) {
		if(egrd->warncnt == 3)
			verbalize(E_J("I repeat, %sfollow me!",
				      "繰り返す、%sついて来い！"),
				u_carry_gold ? (
#ifndef GOLDOBJ
					  !u.ugold ?
					E_J("drop that hidden gold and ","隠した金を置いて") :
					E_J("drop that gold and ","金を置いて")) : "");
#else
					  !umoney ?
					E_J("drop that hidden money and ","隠した金を置いて") :
					E_J("drop that money and ","金を置いて")) : "");
#endif
		if(egrd->warncnt == 7) {
			m = grd->mx;
			n = grd->my;
			verbalize(E_J("You've been warned, knave!",
				      "警告したはずだ、ごろつきめ！"));
			mnexto(grd);
			levl[m][n].typ = egrd->fakecorr[0].ftyp;
			newsym(m,n);
			grd->mpeaceful = 0;
			return(-1);
		}
		/* not fair to get mad when (s)he's fainted or paralyzed */
		if(!is_fainted() && multi >= 0) egrd->warncnt++;
		return(0);
	    }

	    if (!u_in_vault) {
		if (u_carry_gold) {	/* player teleported */
		    m = grd->mx;
		    n = grd->my;
		    (void) rloc(grd, FALSE);
		    levl[m][n].typ = egrd->fakecorr[0].ftyp;
		    newsym(m,n);
		    grd->mpeaceful = 0;
letknow:
		    if (!cansee(grd->mx, grd->my) || !mon_visible(grd))
			You_hear(E_J("the shrill sound of a guard's whistle.",
				     "見張りの吹いたかん高い笛の音を"));
		    else
			You(um_dist(grd->mx, grd->my, 2) ?
			    E_J("see an angry %s approaching.","怒った%sが迫ってくるのを見た。") :
			    E_J("are confronted by an angry %s.","怒った%sに相対した。"),
			    g_monnam(grd));
		    return(-1);
		} else {
		    verbalize(E_J("Well, begone.","いいだろう、去れ。"));
		    wallify_vault(grd);
		    egrd->gddone = 1;
		    goto cleanup;
		}
	    }
	}

	if(egrd->fcend > 1) {
	    if(egrd->fcend > 2 && in_fcorridor(grd, grd->mx, grd->my) &&
		  !egrd->gddone && !in_fcorridor(grd, u.ux, u.uy) &&
		  levl[egrd->fakecorr[0].fx][egrd->fakecorr[0].fy].typ
				 == egrd->fakecorr[0].ftyp) {
		pline_The(E_J("%s, confused, disappears.",
			      "%sは、困ったように、消え失せた。"), g_monnam(grd));
		disappear_msg_seen = TRUE;
		goto cleanup;
	    }
	    if(u_carry_gold &&
		    (in_fcorridor(grd, u.ux, u.uy) ||
		    /* cover a 'blind' spot */
		    (egrd->fcend > 1 && u_in_vault))) {
		if(!grd->mx) {
			restfakecorr(grd);
			return(-2);
		}
		if(egrd->warncnt < 6) {
			egrd->warncnt = 6;
			verbalize(E_J("Drop all your gold, scoundrel!",
				      "金をすべて置くんだ、悪党め！"));
			return(0);
		} else {
			verbalize(E_J("So be it, rogue!","後悔するぞ、チンピラが！"));
			grd->mpeaceful = 0;
			return(-1);
		}
	    }
	}
	for(fci = egrd->fcbeg; fci < egrd->fcend; fci++)
	    if(g_at(egrd->fakecorr[fci].fx, egrd->fakecorr[fci].fy)){
		m = egrd->fakecorr[fci].fx;
		n = egrd->fakecorr[fci].fy;
		goldincorridor = TRUE;
	    }
	if(goldincorridor && !egrd->gddone) {
		x = grd->mx;
		y = grd->my;
		if (m == u.ux && n == u.uy) {
		    struct obj *gold = g_at(m,n);
		    /* Grab the gold from between the hero's feet.  */
#ifndef GOLDOBJ
		    grd->mgold += gold->quan;
		    delobj(gold);
#else
		    obj_extract_self(gold);
		    add_to_minv(grd, gold);
#endif
		    newsym(m,n);
		} else if (m == x && n == y) {
		    mpickgold(grd);	/* does a newsym */
		} else {
		    /* just for insurance... */
		    if (MON_AT(m, n) && m != grd->mx && n != grd->my) {
			verbalize(E_J("Out of my way, scum!","どけ、クズ野郎！"));
			(void) rloc(m_at(m, n), FALSE);
		    }
		    remove_monster(grd->mx, grd->my);
		    newsym(grd->mx, grd->my);
		    place_monster(grd, m, n);
		    mpickgold(grd);	/* does a newsym */
		}
		if(cansee(m,n))
#ifndef JP
		    pline("%s%s picks up the gold.", Monnam(grd),
				grd->mpeaceful ? " calms down and" : "");
#else
		    pline("%sは%s金を拾い上げた。", mon_nam(grd),
				grd->mpeaceful ? "怒りを収め、" : "");
#endif /*JP*/
		if(x != grd->mx || y != grd->my) {
		    remove_monster(grd->mx, grd->my);
		    newsym(grd->mx, grd->my);
		    place_monster(grd, x, y);
		    newsym(x, y);
		}
		if(!grd->mpeaceful) return(-1);
		else {
		    egrd->warncnt = 5;
		    return(0);
		}
	}
	if(um_dist(grd->mx, grd->my, 1) || egrd->gddone) {
		if(!egrd->gddone && !rn2(10)) verbalize(E_J("Move along!","早く来い！"));
		restfakecorr(grd);
		return(0);	/* didn't move */
	}
	x = grd->mx;
	y = grd->my;

	if(u_in_vault) goto nextpos;

	/* look around (hor & vert only) for accessible places */
	for(nx = x-1; nx <= x+1; nx++) for(ny = y-1; ny <= y+1; ny++) {
	  if((nx == x || ny == y) && (nx != x || ny != y) && isok(nx, ny)) {

	    typ = (crm = &levl[nx][ny])->typ;
	    if(!IS_STWALL(typ) && !IS_POOL(typ)) {

		if(in_fcorridor(grd, nx, ny))
			goto nextnxy;

		if(*in_rooms(nx,ny,VAULT))
			continue;

		/* seems we found a good place to leave him alone */
		egrd->gddone = 1;
		if(ACCESSIBLE(typ)) goto newpos;
#ifdef STUPID
		if (typ == SCORR)
		    crm->typ = CORR;
		else
		    crm->typ = DOOR;
#else
		crm->typ = (typ == SCORR) ? CORR : DOOR;
#endif
		if(crm->typ == DOOR) crm->doormask = D_NODOOR;
		goto proceed;
	    }
	  }
nextnxy:	;
	}
nextpos:
	nx = x;
	ny = y;
	gx = egrd->gdx;
	gy = egrd->gdy;
	dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
	dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
	if(abs(gx-x) >= abs(gy-y)) nx += dx; else ny += dy;

	while((typ = (crm = &levl[nx][ny])->typ) != 0) {
	/* in view of the above we must have IS_WALL(typ) or typ == POOL */
	/* must be a wall here */
		if(isok(nx+nx-x,ny+ny-y) && !IS_POOL(typ) &&
		    IS_ROOM(levl[nx+nx-x][ny+ny-y].typ)){
			crm->typ = DOOR;
			crm->doormask = D_NODOOR;
			goto proceed;
		}
		if(dy && nx != x) {
			nx = x; ny = y+dy;
			continue;
		}
		if(dx && ny != y) {
			ny = y; nx = x+dx; dy = 0;
			continue;
		}
		/* I don't like this, but ... */
		if(IS_ROOM(typ)) {
			crm->typ = DOOR;
			crm->doormask = D_NODOOR;
			goto proceed;
		}
		break;
	}
	crm->typ = CORR;
proceed:
	unblock_point(nx, ny);	/* doesn't block light */
	if (cansee(nx,ny))
	    newsym(nx,ny);

	fcp = &(egrd->fakecorr[egrd->fcend]);
	if(egrd->fcend++ == FCSIZ) panic("fakecorr overflow");
	fcp->fx = nx;
	fcp->fy = ny;
	fcp->ftyp = typ;
newpos:
	if(egrd->gddone) {
		/* The following is a kludge.  We need to keep    */
		/* the guard around in order to be able to make   */
		/* the fake corridor disappear as the player      */
		/* moves out of it, but we also need the guard    */
		/* out of the way.  We send the guard to never-   */
		/* never land.  We set ogx ogy to mx my in order  */
		/* to avoid a check at the top of this function.  */
		/* At the end of the process, the guard is killed */
		/* in restfakecorr().				  */
cleanup:
		x = grd->mx; y = grd->my;

		see_guard = canspotmon(grd);
		wallify_vault(grd);
		remove_monster(grd->mx, grd->my);
		newsym(grd->mx,grd->my);
		place_monster(grd, 0, 0);
		egrd->ogx = grd->mx;
		egrd->ogy = grd->my;
		restfakecorr(grd);
		if(!semi_dead && (in_fcorridor(grd, u.ux, u.uy) ||
				     cansee(x, y))) {
		    if (!disappear_msg_seen && see_guard)
			pline(E_J("Suddenly, the %s disappears.",
				  "突然、%sは消え失せた。"), g_monnam(grd));
		    return(1);
		}
		return(-2);
	}
	egrd->ogx = grd->mx;	/* update old positions */
	egrd->ogy = grd->my;
	remove_monster(grd->mx, grd->my);
	place_monster(grd, nx, ny);
	newsym(grd->mx,grd->my);
	restfakecorr(grd);
	return(1);
}

/* Routine when dying or quitting with a vault guard around */
void
paygd()
{
	register struct monst *grd = findgd();
#ifndef GOLDOBJ
	struct obj *gold;
#else
        long umoney = money_cnt(invent);
	struct obj *coins, *nextcoins;
#endif
	int gx,gy;
	char buf[BUFSZ];

#ifndef GOLDOBJ
	if (!u.ugold || !grd) return;
#else
	if (!umoney || !grd) return;
#endif

	if (u.uinvault) {
	    Your(E_J("%ld %s goes into the Magic Memory Vault.",
		     "%ld %sは魔法記念金庫に入った。"),
#ifndef GOLDOBJ
		u.ugold,
		currency(u.ugold));
#else
		umoney,
		currency(umoney));
#endif
	    gx = u.ux;
	    gy = u.uy;
	} else {
	    if(grd->mpeaceful) { /* guard has no "right" to your gold */
		mongone(grd);
		return;
	    }
	    mnexto(grd);
	    pline(E_J("%s remits your gold to the vault.",
		      "%sはあなたの金を金庫に送った。"), Monnam(grd));
	    gx = rooms[EGD(grd)->vroom].lx + rn2(2);
	    gy = rooms[EGD(grd)->vroom].ly + rn2(2);
#ifndef JP
	    Sprintf(buf,
		"To Croesus: here's the gold recovered from %s the %s.",
		plname, mons[u.umonster].mname);
#else
	    Sprintf(buf,
		"クロイソス殿へ: %s%sから取り戻した金です。",
		JMONNAM(u.umonster), plname);
#endif /*JP*/
	    make_grave(gx, gy, buf);
	}
#ifndef GOLDOBJ
	place_object(gold = mkgoldobj(u.ugold), gx, gy);
	stackobj(gold);
#else
        for (coins = invent; coins; coins = nextcoins) {
            nextcoins = coins->nobj;
	    if (objects[coins->otyp].oc_class == COIN_CLASS) {
	        freeinv(coins);
                place_object(coins, gx, gy);
		stackobj(coins);
	    }
        }
#endif
	mongone(grd);
}

long
hidden_gold()
{
	register long value = 0L;
	register struct obj *obj;

	for (obj = invent; obj; obj = obj->nobj)
	    if (Has_contents(obj))
		value += contained_gold(obj);
	/* unknown gold stuck inside statues may cause some consternation... */

	return(value);
}

boolean
gd_sound()  /* prevent "You hear footsteps.." when inappropriate */
{
	register struct monst *grd = findgd();

	if (vault_occupied(u.urooms)) return(FALSE);
	else return((boolean)(grd == (struct monst *)0));
}

#endif /* OVLB */

/*vault.c*/
