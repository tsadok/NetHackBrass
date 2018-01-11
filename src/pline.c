/*	SCCS Id: @(#)pline.c	3.4	1999/11/28	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS /* Uses ... */	/* comment line for pre-compiled headers */
#include "hack.h"
#include "epri.h"
#ifdef WIZARD
#include "edog.h"
#endif

#ifdef OVLB

static boolean no_repeat = FALSE;

static char *FDECL(You_buf, (int));

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vpline, (const char *, va_list));

void
pline VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vpline(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vpline(const char *line, va_list the_args) {
# else
static void
vpline(line, the_args) const char *line; va_list the_args; {
# endif

#else	/* USE_STDARG | USE_VARARG */

#define vpline pline

void
pline VA_DECL(const char *, line)
#endif	/* USE_STDARG | USE_VARARG */

	char pbuf[BUFSZ];
	int l1, l2;
/* Do NOT use VA_START and VA_END in here... see above */

	if (!line || !*line) return;
	if (index(line, '%')) {
	    Vsprintf(pbuf,line,VA_ARGS);
	    line = pbuf;
	}
	if (!iflags.window_inited) {
	    raw_print(line);
	    return;
	}
#ifndef MAC
	l1 = strlen(toplines);
	l2 = strlen(line);
	if (no_repeat && l1 >= l2 && !strcmp(line, toplines + l1 - l2))
	    return;
#endif /* MAC */
	if (vision_full_recalc) vision_recalc(0);
	if (u.ux) flush_screen(1);		/* %% */
	putstr(WIN_MESSAGE, 0, line);
}

/*VARARGS1*/
void
Norep VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, const char *);
	no_repeat = TRUE;
	vpline(line, VA_ARGS);
	no_repeat = FALSE;
	VA_END();
	return;
}

/* work buffer for You(), &c and verbalize() */
static char *you_buf = 0;
static int you_buf_siz = 0;

static char *
You_buf(siz)
int siz;
{
	if (siz > you_buf_siz) {
		if (you_buf) free((genericptr_t) you_buf);
		you_buf_siz = siz + 10;
		you_buf = (char *) alloc((unsigned) you_buf_siz);
	}
	return you_buf;
}

void
free_youbuf()
{
	if (you_buf) free((genericptr_t) you_buf),  you_buf = (char *)0;
	you_buf_siz = 0;
}

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer,prefix,text) \
 Strcpy((pointer = You_buf((int)(strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer,prefix,text) \
 strcat((YouPrefix(pointer, prefix, text), pointer), text)

/*VARARGS1*/
void
You VA_DECL(const char *, line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, E_J("You ","あなたは"), line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
Your VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, E_J("Your ","あなたの"), line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_feel VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, E_J("You feel ","あなたは"), line), VA_ARGS);
	VA_END();
}


/*VARARGS1*/
void
You_cant VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, E_J("You can't ","あなたは"), line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
pline_The VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
#ifndef JP
	vpline(YouMessage(tmp, "The ", line), VA_ARGS);
#else
	vpline(line, VA_ARGS);
#endif /*JP*/
	VA_END();
}

/*VARARGS1*/
void
There VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, E_J("There ","ここには"), line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_hear VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
#ifndef JP
	if (Underwater)
		YouPrefix(tmp, "You barely hear ", line);
	else if (u.usleep)
		YouPrefix(tmp, "You dream that you hear ", line);
	else
		YouPrefix(tmp, "You hear ", line);
	vpline(strcat(tmp, line), VA_ARGS);
#else /*JP*/
	{
	    char *msg, *closing;
	    /* 文末の制御のため、line先頭の半角1文字を利用 */
	    switch (*line) {
		case '!':   closing = "！"; line++; break;
		case '?':   closing = "？"; line++; break;
		case '_':   closing = "…。"; line++; break;
		default:    closing = "。"; break;
	    }
	    if (Underwater)
		msg = "あなたは遠くに%s聞いた%s";
	    else if (u.usleep)
		msg = "あなたは夢の中で%s聞いた%s";
	    else
		msg = "あなたは%s聞いた%s";
	    tmp = You_buf((int)strlen(line) + (int)strlen(msg));
	    Sprintf(tmp, msg, line, closing);
	}
	vpline(tmp, VA_ARGS);
#endif /*JP*/
	VA_END();
}

/*VARARGS1*/
void
verbalize VA_DECL(const char *,line)
	char *tmp;
#ifdef JP
	const char *oq, *cq;
#endif /*JP*/
	if (!flags.soundok) return;
	VA_START(line);
	VA_INIT(line, const char *);
#ifdef JP
	switch (*line) {
	    case '\"':	oq = "“"; cq = "”"; line++; break;
	    default:	oq = "「"; cq = "」"; break;
	}
#endif /*JP*/
	tmp = You_buf((int)strlen(line) + sizeof E_J("\"\"",4));
	Strcpy(tmp, E_J("\"",oq));
	Strcat(tmp, line);
	Strcat(tmp, E_J("\"",cq));
	vpline(tmp, VA_ARGS);
	VA_END();
}

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vraw_printf,(const char *,va_list));

void
raw_printf VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vraw_printf(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vraw_printf(const char *line, va_list the_args) {
# else
static void
vraw_printf(line, the_args) const char *line; va_list the_args; {
# endif

#else  /* USE_STDARG | USE_VARARG */

void
raw_printf VA_DECL(const char *, line)
#endif
/* Do NOT use VA_START and VA_END in here... see above */

	if(!index(line, '%'))
	    raw_print(line);
	else {
	    char pbuf[BUFSZ];
	    Vsprintf(pbuf,line,VA_ARGS);
	    raw_print(pbuf);
	}
}


/*VARARGS1*/
void
impossible VA_DECL(const char *, s)
	VA_START(s);
	VA_INIT(s, const char *);
	if (program_state.in_impossible)
		panic("impossible called impossible");
	program_state.in_impossible = 1;
	{
	    char pbuf[BUFSZ];
	    Vsprintf(pbuf,s,VA_ARGS);
	    paniclog("impossible", pbuf);
	}
	vpline(s,VA_ARGS);
	pline(E_J("Program in disorder - perhaps you'd better #quit.",
		  "プログラムに問題発生 - #quit したほうがいいかもしれません。"));
	program_state.in_impossible = 0;
	VA_END();
}

const char *
align_str(alignment)
    aligntyp alignment;
{
    switch ((int)alignment) {
	case A_CHAOTIC: return E_J("chaotic","混沌");
	case A_NEUTRAL: return E_J("neutral","中立");
	case A_LAWFUL:	return E_J("lawful","秩序");
	case A_NONE:	return E_J("unaligned","無属性");
    }
    return "unknown";
}

void
mstatusline(mtmp)
register struct monst *mtmp;
{
	aligntyp alignment;
	char info[BUFSZ], monnambuf[BUFSZ];

	if (mtmp->ispriest || mtmp->data == &mons[PM_ALIGNED_PRIEST]
				|| mtmp->data == &mons[PM_ANGEL])
		alignment = EPRI(mtmp)->shralign;
	else
		alignment = mtmp->data->maligntyp;
	alignment = (alignment > 0) ? A_LAWFUL :
		(alignment < 0) ? A_CHAOTIC :
		A_NEUTRAL;

	info[0] = 0;
	if (mtmp->mtame) {	  Strcat(info, E_J(", tame",", ペット"));
#ifdef WIZARD
	    if (wizard) {
		Sprintf(eos(info), " (%d", mtmp->mtame);
		if (!mtmp->isminion)
		    Sprintf(eos(info), "; hungry %ld; apport %d",
			EDOG(mtmp)->hungrytime, EDOG(mtmp)->apport);
		Strcat(info, ")");
	    }
#endif
	}
	else if (mtmp->mpeaceful) Strcat(info, E_J(", peaceful",	", 友好的"));
	if (mtmp->meating) {
	    if (!is_swallowing(mtmp) || is_animal(mtmp->data))
				  Strcat(info, E_J(", eating",		", 食事中"));
	}
	if (mtmp->mcan)		  Strcat(info, E_J(", cancelled",	", 無力化されている"));
	if (mtmp->mconf)	  Strcat(info, E_J(", confused",	", 混乱"));
	if (mtmp->mblinded || !mtmp->mcansee)
				  Strcat(info, E_J(", blind",		", 盲目"));
	if (mtmp->mstun)	  Strcat(info, E_J(", stunned",		", よろめき"));
	if (mtmp->msleeping)	  Strcat(info, E_J(", asleep",		", 眠り"));
#if 0	/* unfortunately mfrozen covers temporary sleep and being busy
	   (donning armor, for instance) as well as paralysis */
	else if (mtmp->mfrozen)	  Strcat(info, E_J(", paralyzed",	", 麻痺"));
#else
	else if (mtmp->mfrozen || !mtmp->mcanmove)
				  Strcat(info, E_J(", can't move",	", 動けない"));
#endif
				  /* [arbitrary reason why it isn't moving] */
	else if (mtmp->mstrategy & STRAT_WAITMASK)
				  Strcat(info, E_J(", meditating",	", 瞑想中"));
	else if (mtmp->mflee)	  Strcat(info, E_J(", scared",		", 恐慌状態"));
	if (mtmp->mtrapped)	  Strcat(info, E_J(", trapped",		", 罠にかかっている"));
	if (mtmp->mspeed)	  Strcat(info,
					mtmp->mspeed == MFAST ? E_J(", fast", ", 速い") :
					mtmp->mspeed == MSLOW ? E_J(", slow", ", 遅い") :
					E_J(", ???? speed", ", ????速度"));
	if (mtmp->mundetected)	  Strcat(info, E_J(", concealed",	", 隠れている"));
	if (mtmp->minvis)	  Strcat(info, E_J(", invisible",	", 不可視"));
	if (mtmp == u.ustuck)	  Strcat(info,
			(sticks(youmonst.data)) ? E_J(", held by you",", あなたに捕まえられている") :
				u.uswallow ? (is_animal(u.ustuck->data) ?
				E_J(", swallowed you", ", あなたを飲み込んでいる") :
				E_J(", engulfed you", ", あなたを閉じ込めている")) :
				E_J(", holding you", ", あなたを捕まえている"));
#ifdef STEED
	if (mtmp == u.usteed)	  Strcat(info, E_J(", carrying you",	", あなたを乗せている"));
#endif

	/* avoid "Status of the invisible newt ..., invisible" */
	/* and unlike a normal mon_nam, use "saddled" even if it has a name */
	Strcpy(monnambuf, x_monnam(mtmp, ARTICLE_THE, (char *)0,
	    (SUPPRESS_IT|SUPPRESS_INVISIBLE), FALSE));

	pline(E_J("Status of %s (%s):  Level %d  HP %d(%d)  AC %d%s.",
		  "%s(%s)の状態:  Lv %d  HP %d(%d)  AC %d%s."),
		monnambuf,
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mhp,
		mtmp->mhpmax,
		find_mac(mtmp),
		info);
}

void
ustatusline()
{
	char info[BUFSZ];

	info[0] = '\0';
	if (Sick) {
#ifndef JP
		Strcat(info, ", dying from");
		if (u.usick_type & SICK_VOMITABLE)
			Strcat(info, " food poisoning");
		if (u.usick_type & SICK_NONVOMITABLE) {
			if (u.usick_type & SICK_VOMITABLE)
				Strcat(info, " and");
			Strcat(info, " illness");
#else
		Strcat(info, ", ");
		if (u.usick_type & SICK_VOMITABLE)
			Strcat(info, "食中毒");
		if (u.usick_type & SICK_NONVOMITABLE) {
			if (u.usick_type & SICK_VOMITABLE)
				Strcat(info, "と");
			Strcat(info, "病気");
		Strcat(info, "で死にかけている");
#endif /*JP*/
		}
	}
	if (Stoned)		Strcat(info, E_J(", solidifying",	", 石化中"));
	if (Slimed)		Strcat(info, E_J(", becoming slimy",	", スライム化進行中"));
	if (Strangled)		Strcat(info, (StrangledBy & SUFFO_NECK) ? E_J(", being strangled",", 首を絞められている") :
					     (StrangledBy & SUFFO_WATER) ? E_J(", drowning", ", 溺れている") :
					     E_J(", suffocating", ", 窒息しつつある"));
	if (Vomiting)		Strcat(info, E_J(", nauseated", ", 吐き気を催している")); /* !"nauseous" */
	if (Confusion)		Strcat(info, E_J(", confused", ", 混乱している"));
	if (Blind) {
#ifndef JP
	    Strcat(info, ", blind");
	    if (u.ucreamed) {
		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(youmonst.data))
		    Strcat(info, ", cover");
		Strcat(info, "ed by sticky goop");
	    }	/* note: "goop" == "glop"; variation is intentional */
#else
	    if (u.ucreamed) {
		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(youmonst.data))
		    Strcat(info, ", 目が見えない, ねばつくベトベトに覆われている");
		else
		    Strcat(info, ", ねばつくベトベトで目が見えない");
	    } else
		Strcat(info, ", 目が見えない");
#endif /*JP*/
	}
	if (Stunned)		Strcat(info, E_J(", stunned", ", よろめいている"));
#ifdef STEED
	if (!u.usteed)
#endif
	if (Wounded_legs) {
	    const char *what = body_part(LEG);
#ifndef JP
	    if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
		what = makeplural(what);
				Sprintf(eos(info), ", injured %s", what);
#else
				Sprintf(eos(info), ", %s%sを怪我している",
					((Wounded_legs & BOTH_SIDES) == BOTH_SIDES) ? "両" : "片",
					 what);
#endif /*JP*/
	}
#ifndef JP
	if (Glib)		Sprintf(eos(info), ", slippery %s",
					makeplural(body_part(HAND)));
#else
	if (Glib)		Sprintf(eos(info), ", %sが滑りやすい",
					body_part(HAND));
#endif /*JP*/
	if (u.utrap)		Strcat(info, E_J(", trapped", ", 罠にかかっている"));
	if (Fast)		Strcat(info, Very_fast ?
						E_J(", very fast", ", とても速い") :
						E_J(", fast",	   ", 速い"));
	if (u.uundetected)	Strcat(info, E_J(", concealed", ", 隠れている"));
	if (Invis)		Strcat(info, E_J(", invisible", ", 目に見えない"));
	if (u.ustuck) {
#ifndef JP
	    if (sticks(youmonst.data))
		Strcat(info, ", holding ");
	    else
		Strcat(info, u.uswallow ?
			(is_animal(u.ustuck->data) ?
				", swallowed by " : ", engulfed by ") :
				", held by ");
	    Strcat(info, mon_nam(u.ustuck));
#else
	    Sprintf(eos(info), ", %s", mon_nam(u.ustuck));
	    if (sticks(youmonst.data))
		Strcat(info, "を捕らえている");
	    else
		Strcat(info, u.uswallow ?
			(is_animal(u.ustuck->data) ?
				"に飲み込まれている" : "に閉じ込められている") :
				"に捕えられている");
#endif /*JP*/
	}

	pline(E_J("Status of %s (%s%s):  Level %d  HP %d(%d)  AC %d%s.",
		  "%s (%s%sの%s徒)の状態:  Lv %d  HP %d(%d)  AC %d%s."),
		plname,
		    (u.ualign.record >= 20) ? E_J("piously ",		"敬虔な") :
		    (u.ualign.record > 13) ?  E_J("devoutly ",		"誠実な") :
		    (u.ualign.record > 8) ?   E_J("fervently ",		"忠実な") :
		    (u.ualign.record > 3) ?   E_J("stridently ",	"大げさな") :
		    (u.ualign.record == 3) ?  "" :
		    (u.ualign.record >= 1) ?  E_J("haltingly ",		"迷い多き") :
		    (u.ualign.record == 0) ?  E_J("nominally ",		"名ばかりの") :
					      E_J("insufficiently ",	""),
		align_str(u.ualign.type),
#ifdef JP
		    (u.ualign.record < 0) ? "反" : "",
#endif /*JP*/
		Upolyd ? mons[u.umonnum].mlevel : u.ulevel,
		Upolyd ? u.mh : u.uhp,
		Upolyd ? u.mhmax : u.uhpmax,
		u.uac,
		info);
}

void
self_invis_message()
{
#ifndef JP
	pline("%s %s.",
	    Hallucination ? "Far out, man!  You" : "Gee!  All of a sudden, you",
	    See_invisible ? "can see right through yourself" :
		"can't see yourself");
#else
	pline("%s%s。",
	    Hallucination ? "すげぇ、何コレ！ あなたは" : "うわっ！ まったく突然、あなたは",
	    See_invisible ? "自分の身体が透けているのに気づいた" :
		"自分が見えなくなった");
#endif /*JP*/
}

#endif /* OVLB */
/*pline.c*/
