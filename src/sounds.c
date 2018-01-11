/*	SCCS Id: @(#)sounds.c	3.4	2002/05/06	*/
/*	Copyright (c) 1989 Janet Walz, Mike Threepoint */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"
#ifdef USER_SOUNDS
# ifdef USER_SOUNDS_REGEX
#include <regex.h>
# endif
#endif

#ifdef OVLB

static int FDECL(domonnoise,(struct monst *));
static int NDECL(dochat);

#endif /* OVLB */

#ifdef OVL0

static int FDECL(mon_in_room, (struct monst *,int));

/* this easily could be a macro, but it might overtax dumb compilers */
static int
mon_in_room(mon, rmtyp)
struct monst *mon;
int rmtyp;
{
    int rno = levl[mon->mx][mon->my].roomno;

    return rooms[rno - ROOMOFFSET].rtype == rmtyp;
}

void
dosounds()
{
    register struct mkroom *sroom;
    register int hallu, vx, vy;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
    int xx;
#endif
    struct monst *mtmp;

    if (!flags.soundok || u.uswallow || Underwater) return;

    hallu = Hallucination ? 1 : 0;

    if (level.flags.nfountains && !rn2(400)) {
	static const char * const fountain_msg[4] = {
	    E_J("bubbling water.",	    "水のせせらぎを"),
	    E_J("water falling on coins.",  "コインに降り注ぐ水の音を"),
	    E_J("the splashing of a naiad.","泉の乙女が水浴びする音を"),
	    E_J("a soda fountain!",	    "!ラムネの泉の音を"),
	};
	You_hear(fountain_msg[rn2(3)+hallu]);
    }
#ifdef SINK
    if (level.flags.nsinks && !rn2(300)) {
	static const char * const sink_msg[3] = {
	    E_J("a slow drip.",		"水滴の落ちる音を"),
	    E_J("a gurgling noise.",	"水が排水口に流れる音を"),
	    E_J("dishes being washed!",	"!皿が洗われる音を"),
	};
	You_hear(sink_msg[rn2(2)+hallu]);
    }
#endif
    if (level.flags.has_court && !rn2(200)) {
	static const char * const throne_msg[4] = {
	    E_J("the tones of courtly conversation.",	"宮廷風の話し声を"),
	    E_J("a sceptre pounded in judgment.",	"裁判で王錫が叩き鳴らされるのを"),
	    E_J("Someone shouts \"Off with %s head!\"",	"誰かが「その者の首を刎ねよ！」と叫ぶ声を"),
	    E_J("Queen Beruthiel's cats!",		"ベルシエル王妃の猫の鳴き声を"),
	};
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if ((mtmp->msleeping ||
			is_lord(mtmp->data) || is_prince(mtmp->data)) &&
		!is_animal(mtmp->data) &&
		mon_in_room(mtmp, COURT)) {
		/* finding one is enough, at least for now */
		int which = rn2(3)+hallu;

#ifndef JP
		if (which != 2) You_hear(throne_msg[which]);
		else		pline(throne_msg[2], uhis());
#else
		You_hear(throne_msg[which]);
#endif /*JP*/
		return;
	    }
	}
    }
    if (level.flags.has_swamp && !rn2(200)) {
	static const char * const swamp_msg[3] = {
	    E_J("hear mosquitoes!", "蚊の羽音を聞いた！"),
	    E_J("smell marsh gas!", "沼地の腐敗臭を感じた！"),	/* so it's a smell...*/
	    E_J("hear Donald Duck!","ドナルドダックの声を聞いた！"),
	};
	You(swamp_msg[rn2(2)+hallu]);
	return;
    }
    if (level.flags.has_vault && !rn2(200)) {
	if (!(sroom = search_special(VAULT))) {
	    /* strange ... */
	    level.flags.has_vault = 0;
	    return;
	}
	if(gd_sound())
	    switch (rn2(2)+hallu) {
		case 1: {
		    boolean gold_in_vault = FALSE;

		    for (vx = sroom->lx;vx <= sroom->hx; vx++)
			for (vy = sroom->ly; vy <= sroom->hy; vy++)
			    if (g_at(vx, vy))
				gold_in_vault = TRUE;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
		    /* Bug in aztec assembler here. Workaround below */
		    xx = ROOM_INDEX(sroom) + ROOMOFFSET;
		    xx = (xx != vault_occupied(u.urooms));
		    if(xx)
#else
		    if (vault_occupied(u.urooms) !=
			 (ROOM_INDEX(sroom) + ROOMOFFSET))
#endif /* AZTEC_C_WORKAROUND */
		    {
			if (gold_in_vault)
			    You_hear(!hallu ? E_J("someone counting money.",
						  "誰かがお金を数えているのを") :
				E_J("the quarterback calling the play.",
				    "クォーターバックが指示をする声を"));
			else
			    You_hear(E_J("someone searching.","誰かが捜索している音を"));
			break;
		    }
		    /* fall into... (yes, even for hallucination) */
		}
		case 0:
		    You_hear(E_J("the footsteps of a guard on patrol.",
				 "警備員が見回る足音を"));
		    break;
		case 2:
		    You_hear(E_J("Ebenezer Scrooge!","!エベネーザ・スクルージの声を"));
		    break;
	    }
	return;
    }
    if (level.flags.has_beehive && !rn2(200)) {
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if ((mtmp->data->mlet == S_ANT && is_flyer(mtmp->data)) &&
		mon_in_room(mtmp, BEEHIVE)) {
		switch (rn2(2)+hallu) {
		    case 0:
			You_hear(E_J("a low buzzing.","くぐもった羽音を"));
			break;
		    case 1:
			You_hear(E_J("an angry drone.","興奮したオス蜂の羽音を"));
			break;
		    case 2:
			You_hear(E_J("bees in your %sbonnet!","!蜂が帽子%sの中にいる音を"),
			    uarmh ? "" : E_J("(nonexistent) ","(被ってないけど)"));
			break;
		}
		return;
	    }
	}
    }
    if (level.flags.has_morgue && !rn2(200)) {
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if (is_undead(mtmp->data) &&
		mon_in_room(mtmp, MORGUE)) {
		switch (rn2(2)+hallu) {
		    case 0:
			You(E_J("suddenly realize it is unnaturally quiet.",
				"突然、周りが不自然なまでに静かなことに気づいた。"));
			break;
		    case 1:
#ifndef JP
			pline_The("%s on the back of your %s stands up.",
				body_part(HAIR), body_part(NECK));
#else /*JP*/
			pline("あなたの%sの後ろの%sが逆立った。",
				body_part(NECK), body_part(HAIR));
#endif /*JP*/
			break;
		    case 2:
#ifndef JP
			pline_The("%s on your %s seems to stand up.",
				body_part(HAIR), body_part(HEAD));
#else /*JP*/
			pline("あなたの%sの%sが立ち上がった。",
				body_part(HEAD), body_part(HAIR));
#endif /*JP*/
			break;
		}
		return;
	    }
	}
    }
    if (level.flags.has_barracks && !rn2(200)) {
	static const char * const barracks_msg[4] = {
	    E_J("blades being honed.","剣の研がれる音を"),
	    E_J("loud snoring.",      "大きないびきを"),
	    E_J("dice being thrown.", "ダイスの振られる音を"),
	    E_J("General MacArthur!", "!マッカーサー将軍の声を"),
	};
	int count = 0;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if (is_mercenary(mtmp->data) &&
#if 0		/* don't bother excluding these */
		!strstri(mtmp->data->mname, "watch") &&
		!strstri(mtmp->data->mname, "guard") &&
#endif
		mon_in_room(mtmp, BARRACKS) &&
		/* sleeping implies not-yet-disturbed (usually) */
		(mtmp->msleeping || ++count > 5)) {
		You_hear(barracks_msg[rn2(3)+hallu]);
		return;
	    }
	}
    }
    if (level.flags.has_zoo && !rn2(200)) {
	static const char * const zoo_msg[3] = {
	    E_J("a sound reminiscent of an elephant stepping on a peanut.",
		"象がピーナッツの上で踊っているような音を"),
	    E_J("a sound reminiscent of a seal barking.",
		"アシカが吠えるような声を"),
	    E_J("Doctor Doolittle!","!ドリトル先生の声を"),
	};
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if ((mtmp->msleeping || is_animal(mtmp->data)) &&
		    mon_in_room(mtmp, ZOO)) {
		You_hear(zoo_msg[rn2(2)+hallu]);
		return;
	    }
	}
    }
    if (level.flags.has_shop && !rn2(200)) {
	if (!(sroom = search_special(ANY_SHOP))) {
	    /* strange... */
	    level.flags.has_shop = 0;
	    return;
	}
	if (tended_shop(sroom) &&
		!index(u.ushops, ROOM_INDEX(sroom) + ROOMOFFSET)) {
	    static const char * const shop_msg[3] = {
		E_J("someone cursing shoplifters.", "誰かが万引きをののしる声を"),
		E_J("the chime of a cash register.","レジがチーンと鳴る音を"),
		E_J("Neiman and Marcus arguing!",   "!イトーとヨーカドーの口ゲンカを"),
	    };
	    You_hear(shop_msg[rn2(2)+hallu]);
	}
	return;
    }
    if (Is_oracle_level(&u.uz) && !rn2(400)) {
	/* make sure the Oracle is still here */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp) && mtmp->data == &mons[PM_ORACLE])
		break;
	/* and don't produce silly effects when she's clearly visible */
	if (mtmp && (hallu || !canseemon(mtmp))) {
	    static const char * const ora_msg[5] = {
#ifndef JP
		"a strange wind.",	/* Jupiter at Dodona */
		"convulsive ravings.",	/* Apollo at Delphi */
		"snoring snakes.",	/* AEsculapius at Epidaurus */
		"someone say \"No more woodchucks!\"",
		"a loud ZOT!"		/* both rec.humor.oracle */
#else /*JP*/
		"不思議な風の音を",
		"半狂乱の声を",
		"蛇のいびきを",
		"誰かが「もうウッドチャックはいらない！」と言っている声を",
		"!大きなZOTを",
#endif /*JP*/
	    };
	    You_hear(ora_msg[rn2(3)+hallu*2]);
	}
	return;
    }
}

#endif /* OVL0 */
#ifdef OVLB

static const char * const h_sounds[] = {
#ifndef JP
    "beep", "boing", "sing", "belche", "creak", "cough", "rattle",
    "ululate", "pop", "jingle", "sniffle", "tinkle", "eep"
#else
    "ブザーを鳴らした", "ボョーンと跳ねた", "歌った", "ゲップした", "きしんだ", "咳をした", "ガラガラを鳴らした",
    "ホーホー鳴いた", "弾けた", "ジリリと鳴った", "鼻をすすった", "チリンチリンと鳴った", "チューチュー鳴いた"
#endif /*JP*/
};

const char *
growl_sound(mtmp)
register struct monst *mtmp;
{
	const char *ret;

	switch (mtmp->data->msound) {
	case MS_MEW:
	case MS_HISS:
	    ret = E_J("hiss","シューッと鳴いた");
	    break;
	case MS_BARK:
	case MS_GROWL:
	    ret = E_J("growl","うなった");
	    break;
	case MS_ROAR:
	    ret = E_J("roar","吼えた");
	    break;
	case MS_BUZZ:
	    ret = E_J("buzz","羽音を立てた");
	    break;
	case MS_SQEEK:
	    ret = E_J("squeal","金切り声をあげた");
	    break;
	case MS_SQAWK:
	    ret = E_J("screech","甲高く鳴いた");
	    break;
	case MS_NEIGH:
	    ret = E_J("neigh","いなないた");
	    break;
	case MS_WAIL:
	    ret = E_J("wail","悲痛な叫びをあげた");
	    break;
	case MS_SILENT:
		ret = E_J("commotion","ざわめいた");
		break;
	default:
		ret = E_J("scream","叫んだ");
	}
	return ret;
}

/* the sounds of a seriously abused pet, including player attacking it */
void
growl(mtmp)
register struct monst *mtmp;
{
    register const char *growl_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
	return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
	growl_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
	growl_verb = growl_sound(mtmp);
    if (growl_verb) {
#ifndef JP
	pline("%s %s!", Monnam(mtmp), vtense((char *)0, growl_verb));
#else
	pline("%sは%s！", mon_nam(mtmp), growl_verb);
#endif /*JP*/
	if(flags.run) nomul(0);
	wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 18);
    }
}

/* the sounds of mistreated pets */
void
yelp(mtmp)
register struct monst *mtmp;
{
    register const char *yelp_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
	return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
	yelp_verb = h_sounds[rn2(SIZE(h_sounds))];
    else switch (mtmp->data->msound) {
	case MS_MEW:
	    yelp_verb = E_J("yowl","苦しげに鳴いた");
	    break;
	case MS_BARK:
	case MS_GROWL:
	    yelp_verb = E_J("yelp","苦しげに吠えた");
	    break;
	case MS_ROAR:
	    yelp_verb = E_J("snarl","うなった");
	    break;
	case MS_SQEEK:
	    yelp_verb = E_J("squeal","甲高い鳴き声をあげた");
	    break;
	case MS_SQAWK:
	    yelp_verb = E_J("screak","金切り声をあげた");
	    break;
	case MS_WAIL:
	    yelp_verb = E_J("wail","悲痛な叫びをあげた");
	    break;
    }
    if (yelp_verb) {
#ifndef JP
	pline("%s %s!", Monnam(mtmp), vtense((char *)0, yelp_verb));
#else
	pline("%sは%s！", mon_nam(mtmp), yelp_verb);
#endif /*JP*/
	if(flags.run) nomul(0);
	wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 12);
    }
}

/* the sounds of distressed pets */
void
whimper(mtmp)
register struct monst *mtmp;
{
    register const char *whimper_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
	return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
	whimper_verb = h_sounds[rn2(SIZE(h_sounds))];
    else switch (mtmp->data->msound) {
	case MS_MEW:
	case MS_GROWL:
	    whimper_verb = E_J("whimper","不満げにうなった");
	    break;
	case MS_BARK:
	    whimper_verb = E_J("whine","不満げにうなった");
	    break;
	case MS_SQEEK:
	    whimper_verb = E_J("squeal","キーキーと鳴いた");
	    break;
    }
    if (whimper_verb) {
	pline(E_J("%s %s.","%s%s。"), Monnam(mtmp), vtense((char *)0, whimper_verb));
	if(flags.run) nomul(0);
	wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 6);
    }
}

/* pet makes "I'm hungry" noises */
void
beg(mtmp)
register struct monst *mtmp;
{
    if (mtmp->msleeping || !mtmp->mcanmove ||
	    !(carnivorous(mtmp->data) || herbivorous(mtmp->data)))
	return;

    /* presumably nearness and soundok checks have already been made */
    if (!is_silent(mtmp->data) && mtmp->data->msound <= MS_ANIMAL)
	(void) domonnoise(mtmp);
    else if (mtmp->data->msound >= MS_HUMANOID) {
	if (!canspotmons(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);
	verbalize(E_J("I'm hungry.","空腹だ。"));
    }
}

static int
domonnoise(mtmp)
register struct monst *mtmp;
{
    register const char *pline_msg = 0,	/* Monnam(mtmp) will be prepended */
			*verbl_msg = 0;	/* verbalize() */
    struct permonst *ptr = mtmp->data;
    char verbuf[BUFSZ];

    /* presumably nearness and sleep checks have already been made */
    if (!flags.soundok) return(0);
    if (is_silent(ptr)) {
#ifdef JP
	if (Hallucination && ptr == &mons[PM_GHOUL])
	    verbl_msg = "GHOULちゃん、でぇぇぇーす！"; /* HYDLIDE 2 */
	else
#endif
	    return(0);
    }

    /* Make sure its your role's quest quardian; adjust if not */
    if (ptr->msound == MS_GUARDIAN && ptr != &mons[urole.guardnum]) {
    	int mndx = monsndx(ptr);
    	ptr = &mons[genus(mndx,1)];
    }

    /* be sure to do this before talking; the monster might teleport away, in
     * which case we want to check its pre-teleport position
     */
    if (!canspotmons(mtmp))
	map_invisible(mtmp->mx, mtmp->my);

    switch (ptr->msound) {
	case MS_ORACLE:
	    return doconsult(mtmp);
	case MS_PRIEST:
	    priest_talk(mtmp);
	    break;
	case MS_LEADER:
	case MS_NEMESIS:
	case MS_GUARDIAN:
	    quest_chat(mtmp);
	    break;
	case MS_SELL: /* pitch, pay, total */
	    shk_chat(mtmp);
	    break;
	case MS_VAMPIRE:
	    {
	    /* vampire messages are varied by tameness, peacefulness, and time of night */
		boolean isnight = night();
		boolean kindred =    (Upolyd && (u.umonnum == PM_VAMPIRE ||
				       u.umonnum == PM_VAMPIRE_LORD));
		boolean nightchild = (Upolyd && (u.umonnum == PM_WOLF ||
				       u.umonnum == PM_WINTER_WOLF ||
	    			       u.umonnum == PM_WINTER_WOLF_CUB));
		const char *racenoun = (flags.female && urace.individual.f) ?
					urace.individual.f : (urace.individual.m) ?
					urace.individual.m : urace.noun;

		if (mtmp->mtame) {
			if (kindred) {
				Sprintf(verbuf, E_J("Good %s to you Master%s",
						    "よき%sを、わが主%s"),
					isnight ? E_J("evening","晩") : E_J("day","日"),
					isnight ? E_J("!","！") :
						  E_J(".  Why do we not rest?","。お休みにならないので？"));
				verbl_msg = verbuf;
		    	} else {
		    	    Sprintf(verbuf,"%s%s",
				nightchild ? E_J("Child of the night, ","夜の子よ、") : "",
				midnight() ?
					E_J("I can stand this craving no longer!",
					    "最早この渇望を抑えられぬ！") :
				isnight ?
					E_J("I beg you, help me satisfy this growing craving!",
					    "頼む、このたぎる渇望を満たす手助けをしてくれ！") :
					E_J("I find myself growing a little weary.",
					    "私は少々疲れてきたようだ。"));
				verbl_msg = verbuf;
			}
		} else if (mtmp->mpeaceful) {
			if (kindred && isnight) {
#ifndef JP
				Sprintf(verbuf, "Good feeding %s!",
	    				flags.female ? "sister" : "brother");
#else
				Sprintf(verbuf, "ごきげんよう、我がきょうだいよ！");
#endif /*JP*/
				verbl_msg = verbuf;
 			} else if (nightchild && isnight) {
				Sprintf(verbuf,
				    E_J("How nice to hear you, child of the night!",
					"会えて嬉しいぞ、夜の子よ！"));
				verbl_msg = verbuf;
	    		} else
		    		verbl_msg = E_J("I only drink... potions.","私は…薬しか飲まない。");
    	        } else {
			int vampindex;
	    		static const char * const vampmsg[] = {
			       /* These first two (0 and 1) are specially handled below */
	    			E_J("I vant to suck your %s!","お前の%sをよこせ！"),
				E_J("I vill come after %s without regret!","%sを狩るのにためらいなどないぞ！"),
		    	       /* other famous vampire quotes can follow here if desired */
	    		};
			if (kindred)
			    verbl_msg = E_J("This is my hunting ground that you dare to prowl!",
					    "お前がうろつきまわっているこの一帯は、私の狩場だ！");
			else if (youmonst.data == &mons[PM_SILVER_DRAGON] ||
				 youmonst.data == &mons[PM_BABY_SILVER_DRAGON]) {
			    /* Silver dragons are silver in color, not made of silver */
			    Sprintf(verbuf, E_J("%s! Your silver sheen does not frighten me!",
						"愚か%sめ！　お前の銀色の皮で私を威すことなどできぬぞ！"),
					youmonst.data == &mons[PM_SILVER_DRAGON] ?
					E_J("Fool","者") : E_J("Young Fool","な若造"));
			    verbl_msg = verbuf; 
			} else {
			    vampindex = rn2(SIZE(vampmsg));
			    if (vampindex == 0) {
				Sprintf(verbuf, vampmsg[vampindex], body_part(BLOOD));
	    			verbl_msg = verbuf;
			    } else if (vampindex == 1) {
				Sprintf(verbuf, vampmsg[vampindex],
#ifndef JP
					Upolyd ? an(mons[u.umonnum].mname) : an(racenoun));
#else
					Upolyd ? JMONNAM(u.umonnum) : racenoun);
#endif /*JP*/
	    			verbl_msg = verbuf;
		    	    } else
			    	verbl_msg = vampmsg[vampindex];
			}
	        }
	    }
	    break;
	case MS_WERE:
	    if (flags.moonphase == FULL_MOON && (night() ^ !rn2(13))) {
#ifndef JP
		pline("%s throws back %s head and lets out a blood curdling %s!",
		      Monnam(mtmp), mhis(mtmp),
		      ptr == &mons[PM_HUMAN_WERERAT] ? "shriek" : "howl");
#else
		pline("%sは大きくのけぞると、血も凍るような%sを上げた！",
		      Monnam(mtmp), ptr == &mons[PM_HUMAN_WERERAT] ? "金切り声の叫び" : "雄叫び");
#endif /*JP*/
		wake_nearto(mtmp->mx, mtmp->my, 11*11);
	    } else
		pline_msg =
		     E_J("whispers inaudibly.  All you can make out is \"moon\".",
			 "聞き取りがたい声で囁いた。かろうじて判ったのは「月」という言葉だけだった。");
	    break;
	case MS_BARK:
	    if (flags.moonphase == FULL_MOON && night()) {
		pline_msg = E_J("howls.","遠吠えをした。");
	    } else if (mtmp->mpeaceful) {
		if (mtmp->mtame &&
			(mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
			 moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
		    pline_msg = E_J("whines.","不満げに吠えた。");
		else if (mtmp->mtame && EDOG(mtmp)->hungrytime > moves + 1000)
		    pline_msg = E_J("yips.","嬉しそうに吠えた。");
		else {
		    if (mtmp->data != &mons[PM_DINGO])	/* dingos do not actually bark */
			    pline_msg = E_J("barks.","吠えた。");
		}
	    } else {
		pline_msg = E_J("growls.","低い唸り声をあげた。");
	    }
	    break;
	case MS_MEW:
	    if (mtmp->mtame) {
		if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
			mtmp->mtame < 5)
		    pline_msg = E_J("yowls.","苦しそうな鳴き声をあげた。");
		else if (moves > EDOG(mtmp)->hungrytime)
		    pline_msg = E_J("meows.","大きく鳴いた。");
		else if (EDOG(mtmp)->hungrytime > moves + 1000)
		    pline_msg = E_J("purrs.","のどを鳴らした。");
		else
		    pline_msg = E_J("mews.","ニャアと鳴いた。");
		break;
	    } /* else FALLTHRU */
	case MS_GROWL:
	    pline_msg = mtmp->mpeaceful ? E_J("snarls.","うなった。") : E_J("growls!","威嚇の唸り声をあげた！");
	    break;
	case MS_ROAR:
	    pline_msg = mtmp->mpeaceful ? E_J("snarls.","うなった。") : E_J("roars!","吼えた！");
	    break;
	case MS_SQEEK:
	    pline_msg = E_J("squeaks.","キーキーと鳴いた。");
	    break;
	case MS_SQAWK:
	    if (ptr == &mons[PM_RAVEN] && !mtmp->mpeaceful)
	    	verbl_msg = E_J("Nevermore!","またとなけめ"); /* エドガー・アラン・ポー「大鴉」日夏 耿之介 訳*/
	    else
	    	pline_msg = E_J("squawks.","鳴き声を上げた。");
	    break;
	case MS_HISS:
	    if (!mtmp->mpeaceful)
		pline_msg = E_J("hisses!","シューッという音を出した！");
	    else return 0;	/* no sound */
	    break;
	case MS_BUZZ:
	    pline_msg = mtmp->mpeaceful ? E_J("drones.","低い羽音をたてている。") :
					  E_J("buzzes angrily.","怒ったような羽音をたてた。");
	    break;
	case MS_GRUNT:
	    pline_msg = E_J("grunts.","うなり声をあげた。");
	    break;
	case MS_NEIGH:
	    if (mtmp->mtame < 5)
		pline_msg = E_J("neighs.","荒々しくいなないた。");
	    else if (moves > EDOG(mtmp)->hungrytime)
		pline_msg = E_J("whinnies.","弱々しくいなないた。");
	    else
		pline_msg = E_J("whickers.","いなないた。");
	    break;
	case MS_WAIL:
	    pline_msg = E_J("wails mournfully.","悲嘆にくれた叫びを上げた。");
	    break;
	case MS_GURGLE:
	    pline_msg = E_J("gurgles.","ぶくぶくと音を立てた。");
	    break;
	case MS_BURBLE:
	    pline_msg = E_J("burbles.","わけのわからないことを言った。");
	    break;
	case MS_SHRIEK:
	    pline_msg = E_J("shrieks.","金切り声を上げた。");
	    aggravate();
	    break;
	case MS_IMITATE:
	    pline_msg = E_J("imitates you.","あなたの真似をした。");
	    break;
	case MS_BONES:
	    pline(E_J("%s rattles noisily.","%sはガラガラとうるさく鳴った。"), Monnam(mtmp));
	    You(E_J("freeze for a moment.","一瞬凍りついた。"));
	    nomul(-2);
	    break;
	case MS_LAUGH:
	    {
		static const char * const laugh_msg[4] = {
#ifndef JP
		    "giggles.", "chuckles.", "snickers.", "laughs.",
#else
		    "ニヤニヤ笑った。", "くすくす笑った。", "嘲笑を浴びせた。", "声を上げて笑った。",
#endif
		};
		pline_msg = laugh_msg[rn2(4)];
	    }
	    break;
	case MS_MUMBLE:
	    pline_msg = E_J("mumbles incomprehensibly.","理解しがたい言葉を呟いた。");
	    break;
	case MS_DJINNI:
	    if (mtmp->mtame) {
		verbl_msg = E_J("Sorry, I'm all out of wishes.","ごめんよ、願いは品切れなんだ。");
	    } else if (mtmp->mpeaceful) {
		if (ptr == &mons[PM_WATER_DEMON])
		    pline_msg = E_J("gurgles.","ゴボゴボという音を立てた。");
		else
		    verbl_msg = E_J("I'm free!","自由だ！");
	    } else verbl_msg = E_J("This will teach you not to disturb me!",
				   "私を煩わす者がどうなるか知るがいい！");
	    break;
	case MS_BOAST:	/* giants */
	    if (!mtmp->mpeaceful) {
		switch (rn2(4)) {
		case 0: pline(E_J("%s boasts about %s gem collection.",
				  "%sは%s宝石コレクションを自慢した。"),
			      Monnam(mtmp), mhis(mtmp));
			break;
		case 1: pline_msg = E_J("complains about a diet of mutton.",
					"羊肉の食事に不平をもらした。");
			break;
	       default: pline_msg = E_J("shouts \"Fee Fie Foe Foo!\" and guffaws.",
					"\"Fee Fie Foe Foo!\"と叫ぶと、大声で笑った。"); /* Adventure? */
			wake_nearto(mtmp->mx, mtmp->my, 7*7);
			break;
		}
		break;
	    }
	    /* else FALLTHRU */
	case MS_HUMANOID:
	    if (!mtmp->mpeaceful) {
		if (In_endgame(&u.uz) && is_mplayer(ptr)) {
		    mplayer_talk(mtmp);
		    break;
		} else return 0;	/* no sound */
	    }
	    /* Generic peaceful humanoid behaviour. */
	    if (mtmp->mflee)
		pline_msg = E_J("wants nothing to do with you.","あなたに関わりたくない。");
	    else if (mtmp->mhp < mtmp->mhpmax/4)
		pline_msg = E_J("moans.","うめいた。");
	    else if (mtmp->mconf || mtmp->mstun)
		verbl_msg = !rn2(3) ? E_J("Huh?","は？") : rn2(2) ? E_J("What?","何？") : E_J("Eh?","え？");
	    else if (!mtmp->mcansee)
		verbl_msg = E_J("I can't see!","何も見えない！");
	    else if (mtmp->mtrapped) {
		struct trap *t = t_at(mtmp->mx, mtmp->my);

		if (t) t->tseen = 1;
		verbl_msg = E_J("I'm trapped!","罠にはまってしまった！");
	    } else if (mtmp->mhp < mtmp->mhpmax/2)
		pline_msg = E_J("asks for a potion of healing.","回復の薬を分けてくれないかと尋ねた。");
	    else if (mtmp->mtame && !mtmp->isminion &&
						moves > EDOG(mtmp)->hungrytime)
		verbl_msg = E_J("I'm hungry.","空腹だ。");
	    /* Specific monsters' interests */
	    else if (is_elf(ptr))
		pline_msg = E_J("curses orcs.","オークに悪態をついた。");
	    else if (is_dwarf(ptr))
		pline_msg = E_J("talks about mining.","採掘について話した。");
	    else if (likes_magic(ptr))
		pline_msg = E_J("talks about spellcraft.","魔術のことを話した。");
	    else if (ptr->mlet == S_CENTAUR)
		pline_msg = E_J("discusses hunting.","狩猟について議論した。");
	    else switch (monsndx(ptr)) {
		case PM_HOBBIT:
		    pline_msg = (mtmp->mhpmax - mtmp->mhp >= 10) ?
				E_J("complains about unpleasant dungeon conditions.",
				    "ダンジョン内の不愉快な状態に不満を述べた。")
				: E_J("asks you about the One Ring.","一つの指輪についてたずねた。");
		    break;
		case PM_ARCHEOLOGIST:
    pline_msg = E_J("describes a recent article in \"Spelunker Today\" magazine.",
		    "“今日の洞窟探検”誌の最近の記事を教えてくれた。");
		    break;
#ifdef TOURIST
		case PM_TOURIST:
		    verbl_msg = E_J("Aloha.","アローハ。");
		    break;
#endif
		default:
		    pline_msg = E_J("discusses dungeon exploration.",
				    "地下迷宮の探検について意見を述べた。");
		    break;
	    }
	    break;
	case MS_SEDUCE:
#ifdef SEDUCE
	    if (ptr->mlet != S_NYMPH &&
		could_seduce(mtmp, &youmonst, (struct attack *)0) == 1) {
			(void) doseduce(mtmp);
			break;
	    }
	    switch ((poly_gender() != (int) mtmp->female) ? rn2(3) : 0)
#else
	    switch ((poly_gender() == 0) ? rn2(3) : 0)
#endif
	    {
		case 2:
			verbl_msg = E_J("Hello, sailor.","こんにちは、水兵さん。");
			break;
		case 1:
			pline_msg = E_J("comes on to you.","すり寄ってきた。");
			break;
		default:
			pline_msg = E_J("cajoles you.","あなたをおだてた。");
	    }
	    break;
#ifdef KOPS
	case MS_ARREST:
	    if (mtmp->mpeaceful)
		/* Sergeant Joe Friday: Dragnet  */
		verbalize(E_J("Just the facts, %s.","事実が肝心です、%s。"),
		      flags.female ? E_J("Ma'am","ご婦人") : E_J("Sir","ご主人"));
	    else {
		static const char * const arrest_msg[3] = {
		    E_J("Anything you say can be used against you.",
			"発言はすべて不利な証拠として採用される可能性がある。"),
		    E_J("You're under arrest!","逮捕する！"),
		    E_J("Stop in the name of the Law!","法の名の下に中止せよ！"),
		};
		verbl_msg = arrest_msg[rn2(3)];
	    }
	    break;
#endif
	case MS_BRIBE:
	    if (mtmp->mpeaceful && !mtmp->mtame) {
		(void) demon_talk(mtmp);
		break;
	    }
	    /* fall through */
	case MS_CUSS:
	    if (!mtmp->mpeaceful)
		cuss(mtmp);
	    break;
	case MS_SPELL:
	    /* deliberately vague, since it's not actually casting any spell */
	    pline_msg = "seems to mutter a cantrip.";
	    break;
	case MS_NURSE:
	    if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
		verbl_msg = E_J("Put that weapon away before you hurt someone!",
				"人を傷つける前に、その武器をしまいなさい！");
	    else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
		verbl_msg = Role_if(PM_HEALER) ?
			  E_J("Doc, I can't help you unless you cooperate.",
			      "先生、協力していただかないとお手伝いできません。") :
			  E_J("Please undress so I can examine you.","診察しますので、服を脱いでください。");
#ifdef TOURIST
	    else if (uarmu)
		verbl_msg = E_J("Take off your shirt, please.","シャツを脱いでくださいね。");
#endif
	    else verbl_msg = E_J("Relax, this won't hurt a bit.","大丈夫、ちっとも痛くありませんよ。");
	    break;
	case MS_GUARD:
#ifndef GOLDOBJ
	    if (u.ugold)
#else
	    if (money_cnt(invent))
#endif
		verbl_msg = E_J("Please drop that gold and follow me.",
				"金を置いて、ついて来なさい。");
	    else
		verbl_msg = E_J("Please follow me.","ついて来なさい。");
	    break;
	case MS_SOLDIER:
	    {
		static const char * const soldier_foe_msg[3] = {
		    E_J("Resistance is useless!","無駄な抵抗は止めろ！"),
		    E_J("You're dog meat!","犬のエサになれ！"),
		    E_J("Surrender!","投降しろ！"),
		},		  * const soldier_pax_msg[3] = {
		    E_J("What lousy pay we're getting here!","ここの給料のシケっぷりったらないぜ！"),
		    E_J("The food's not fit for Orcs!","オークだってこんな飯は喰っとらんだろうよ！"),
		    E_J("My feet hurt, I've been on them all day!","足が痛むよ、一日中立ちっぱなしだ！"),
		};
		verbl_msg = mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
					    : soldier_foe_msg[rn2(3)];
	    }
	    break;
	case MS_RIDER:
	    if (ptr == &mons[PM_DEATH] && !rn2(10))
		pline_msg = E_J("is busy reading a copy of Sandman #8.",
				"は「サンドマン」の8巻を読むのに忙しい。");
	    else verbl_msg = E_J("Who do you think you are, War?",
				 "自分が誰だと思っているのかね、“戦争”よ？");
	    break;
    }

    if (pline_msg) pline(E_J("%s %s","%sは%s"), Monnam(mtmp), pline_msg);
    else if (verbl_msg) verbalize(verbl_msg);
    return(1);
}


int
dotalk()
{
    int result;
    boolean save_soundok = flags.soundok;
    flags.soundok = 1;	/* always allow sounds while chatting */
    result = dochat();
    flags.soundok = save_soundok;
    return result;
}

static int
dochat()
{
    register struct monst *mtmp;
    register int tx,ty;
    struct obj *otmp;

    if (is_silent(youmonst.data)) {
#ifndef JP
	pline("As %s, you cannot speak.", an(youmonst.data->mname));
#else
	pline("%sの姿では、あなたは話すことができない。", JMONNAM(u.umonnum));
#endif /*JP*/
	return(0);
    }
    if (Strangled) {
	You_cant(E_J("speak.  You're choking!","喋れない。あなたは息ができない！"));
	return(0);
    }
    if (u.uswallow) {
	pline(E_J("They won't hear you out there.",
		  "こんなところでは、誰も話を聞いてくれない。"));
	return(0);
    }
    if (Underwater) {
	Your(E_J("speech is unintelligible underwater.",
		 "言葉は水中のため、理解不能になった。"));
	return(0);
    }

    if (!Blind && (otmp = shop_object(u.ux, u.uy)) != (struct obj *)0) {
	/* standing on something in a shop and chatting causes the shopkeeper
	   to describe the price(s).  This can inhibit other chatting inside
	   a shop, but that shouldn't matter much.  shop_object() returns an
	   object iff inside a shop and the shopkeeper is present and willing
	   (not angry) and able (not asleep) to speak and the position contains
	   any objects other than just gold.
	*/
	price_quote(otmp);
	return(1);
    }

    if (!getdir(E_J("Talk to whom? (in what direction)",
		    "誰と話しますか？(どの方向)"))) {
	/* decided not to chat */
	return(0);
    }

#ifdef STEED
    if (u.usteed && u.dz > 0)
	return (domonnoise(u.usteed));
#endif
    if (u.dz) {
#ifndef JP
	pline("They won't hear you %s there.", u.dz < 0 ? "up" : "down");
#else
	pline("%sの方向には誰もいない。", u.dz < 0 ? "上" : "下");
#endif /*JP*/
	return(0);
    }

    if (u.dx == 0 && u.dy == 0) {
/*
 * Let's not include this.  It raises all sorts of questions: can you wear
 * 2 helmets, 2 amulets, 3 pairs of gloves or 6 rings as a marilith,
 * etc...  --KAA
	if (u.umonnum == PM_ETTIN) {
	    You("discover that your other head makes boring conversation.");
	    return(1);
	}
*/
	pline(E_J("Talking to yourself is a bad habit for a dungeoneer.",
		  "独り言は迷宮住人の悪習だ。"));
	return(0);
    }

    tx = u.ux+u.dx; ty = u.uy+u.dy;
    mtmp = m_at(tx, ty);

    if (!mtmp || mtmp->mundetected ||
		mtmp->m_ap_type == M_AP_FURNITURE ||
		mtmp->m_ap_type == M_AP_OBJECT)
	return(0);

    /* sleeping monsters won't talk, except priests (who wake up) */
    if ((!mtmp->mcanmove || mtmp->msleeping) && !mtmp->ispriest) {
	/* If it is unseen, the player can't tell the difference between
	   not noticing him and just not existing, so skip the message. */
	if (canspotmon(mtmp))
	    pline(E_J("%s seems not to notice you.",
		      "%sはあなたに気づいていないようだ。"), Monnam(mtmp));
	return(0);
    }

    /* if this monster is waiting for something, prod it into action */
    mtmp->mstrategy &= ~STRAT_WAITMASK;

    if (mtmp->mtame && mtmp->meating) {
	if (!canspotmons(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);
	pline(E_J("%s is eating noisily.",
		  "%sは騒々しく食べている。"), Monnam(mtmp));
	return (0);
    }

    return domonnoise(mtmp);
}

#ifdef USER_SOUNDS

extern void FDECL(play_usersound, (const char*, int));

typedef struct audio_mapping_rec {
#ifdef USER_SOUNDS_REGEX
	struct re_pattern_buffer regex;
#else
	char *pattern;
#endif
	char *filename;
	int volume;
	struct audio_mapping_rec *next;
} audio_mapping;

static audio_mapping *soundmap = 0;

char* sounddir = ".";

/* adds a sound file mapping, returns 0 on failure, 1 on success */
int
add_sound_mapping(mapping)
const char *mapping;
{
	char text[256];
	char filename[256];
	char filespec[256];
	int volume;

	if (sscanf(mapping, "MESG \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d",
		   text, filename, &volume) == 3) {
	    const char *err;
	    audio_mapping *new_map;

	    if (strlen(sounddir) + strlen(filename) > 254) {
		raw_print("sound file name too long");
		return 0;
	    }
	    Sprintf(filespec, "%s/%s", sounddir, filename);

	    if (can_read_file(filespec)) {
		new_map = (audio_mapping *)alloc(sizeof(audio_mapping));
#ifdef USER_SOUNDS_REGEX
		new_map->regex.translate = 0;
		new_map->regex.fastmap = 0;
		new_map->regex.buffer = 0;
		new_map->regex.allocated = 0;
		new_map->regex.regs_allocated = REGS_FIXED;
#else
		new_map->pattern = (char *)alloc(strlen(text) + 1);
		Strcpy(new_map->pattern, text);
#endif
		new_map->filename = strdup(filespec);
		new_map->volume = volume;
		new_map->next = soundmap;

#ifdef USER_SOUNDS_REGEX
		err = re_compile_pattern(text, strlen(text), &new_map->regex);
#else
		err = 0;
#endif
		if (err) {
		    raw_print(err);
		    free(new_map->filename);
		    free(new_map);
		    return 0;
		} else {
		    soundmap = new_map;
		}
	    } else {
		Sprintf(text, "cannot read %.243s", filespec);
		raw_print(text);
		return 0;
	    }
	} else {
	    raw_print("syntax error in SOUND");
	    return 0;
	}

	return 1;
}

void
play_sound_for_message(msg)
const char* msg;
{
	audio_mapping* cursor = soundmap;

	while (cursor) {
#ifdef USER_SOUNDS_REGEX
	    if (re_search(&cursor->regex, msg, strlen(msg), 0, 9999, 0) >= 0) {
#else
	    if (pmatch(cursor->pattern, msg)) {
#endif
		play_usersound(cursor->filename, cursor->volume);
	    }
	    cursor = cursor->next;
	}
}

#endif /* USER_SOUNDS */

#endif /* OVLB */

/*sounds.c*/
