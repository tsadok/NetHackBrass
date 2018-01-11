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
	    E_J("bubbling water.",	    "���̂����炬��"),
	    E_J("water falling on coins.",  "�R�C���ɍ~�蒍�����̉���"),
	    E_J("the splashing of a naiad.","��̉����������т��鉹��"),
	    E_J("a soda fountain!",	    "!�����l�̐�̉���"),
	};
	You_hear(fountain_msg[rn2(3)+hallu]);
    }
#ifdef SINK
    if (level.flags.nsinks && !rn2(300)) {
	static const char * const sink_msg[3] = {
	    E_J("a slow drip.",		"���H�̗����鉹��"),
	    E_J("a gurgling noise.",	"�����r�����ɗ���鉹��"),
	    E_J("dishes being washed!",	"!�M������鉹��"),
	};
	You_hear(sink_msg[rn2(2)+hallu]);
    }
#endif
    if (level.flags.has_court && !rn2(200)) {
	static const char * const throne_msg[4] = {
	    E_J("the tones of courtly conversation.",	"�{�앗�̘b������"),
	    E_J("a sceptre pounded in judgment.",	"�ٔ��ŉ������@���炳���̂�"),
	    E_J("Someone shouts \"Off with %s head!\"",	"�N�����u���̎҂̎�𙆂˂�I�v�Ƌ��Ԑ���"),
	    E_J("Queen Beruthiel's cats!",		"�x���V�G�����܂̔L�̖�����"),
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
	    E_J("hear mosquitoes!", "��̉H���𕷂����I"),
	    E_J("smell marsh gas!", "���n�̕��s�L���������I"),	/* so it's a smell...*/
	    E_J("hear Donald Duck!","�h�i���h�_�b�N�̐��𕷂����I"),
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
						  "�N���������𐔂��Ă���̂�") :
				E_J("the quarterback calling the play.",
				    "�N�H�[�^�[�o�b�N���w�������鐺��"));
			else
			    You_hear(E_J("someone searching.","�N�����{�����Ă��鉹��"));
			break;
		    }
		    /* fall into... (yes, even for hallucination) */
		}
		case 0:
		    You_hear(E_J("the footsteps of a guard on patrol.",
				 "�x����������鑫����"));
		    break;
		case 2:
		    You_hear(E_J("Ebenezer Scrooge!","!�G�x�l�[�U�E�X�N���[�W�̐���"));
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
			You_hear(E_J("a low buzzing.","�����������H����"));
			break;
		    case 1:
			You_hear(E_J("an angry drone.","���������I�X�I�̉H����"));
			break;
		    case 2:
			You_hear(E_J("bees in your %sbonnet!","!�I���X�q%s�̒��ɂ��鉹��"),
			    uarmh ? "" : E_J("(nonexistent) ","(����ĂȂ�����)"));
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
				"�ˑR�A���肪�s���R�Ȃ܂łɐÂ��Ȃ��ƂɋC�Â����B"));
			break;
		    case 1:
#ifndef JP
			pline_The("%s on the back of your %s stands up.",
				body_part(HAIR), body_part(NECK));
#else /*JP*/
			pline("���Ȃ���%s�̌���%s���t�������B",
				body_part(NECK), body_part(HAIR));
#endif /*JP*/
			break;
		    case 2:
#ifndef JP
			pline_The("%s on your %s seems to stand up.",
				body_part(HAIR), body_part(HEAD));
#else /*JP*/
			pline("���Ȃ���%s��%s�������オ�����B",
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
	    E_J("blades being honed.","���̌�����鉹��"),
	    E_J("loud snoring.",      "�傫�Ȃ��т���"),
	    E_J("dice being thrown.", "�_�C�X�̐U���鉹��"),
	    E_J("General MacArthur!", "!�}�b�J�[�T�[���R�̐���"),
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
		"�ۂ��s�[�i�b�c�̏�ŗx���Ă���悤�ȉ���"),
	    E_J("a sound reminiscent of a seal barking.",
		"�A�V�J���i����悤�Ȑ���"),
	    E_J("Doctor Doolittle!","!�h���g���搶�̐���"),
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
		E_J("someone cursing shoplifters.", "�N�������������̂̂��鐺��"),
		E_J("the chime of a cash register.","���W���`�[���Ɩ鉹��"),
		E_J("Neiman and Marcus arguing!",   "!�C�g�[�ƃ��[�J�h�[�̌��Q���J��"),
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
		"�s�v�c�ȕ��̉���",
		"�������̐���",
		"�ւ̂��т���",
		"�N�����u�����E�b�h�`���b�N�͂���Ȃ��I�v�ƌ����Ă��鐺��",
		"!�傫��ZOT��",
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
    "�u�U�[��炵��", "�{���[���ƒ��˂�", "�̂���", "�Q�b�v����", "������", "�P������", "�K���K����炵��",
    "�z�[�z�[����", "�e����", "�W�����Ɩ���", "�@����������", "�`�����`�����Ɩ���", "�`���[�`���[����"
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
	    ret = E_J("hiss","�V���[�b�Ɩ���");
	    break;
	case MS_BARK:
	case MS_GROWL:
	    ret = E_J("growl","���Ȃ���");
	    break;
	case MS_ROAR:
	    ret = E_J("roar","�Ⴆ��");
	    break;
	case MS_BUZZ:
	    ret = E_J("buzz","�H���𗧂Ă�");
	    break;
	case MS_SQEEK:
	    ret = E_J("squeal","���؂萺��������");
	    break;
	case MS_SQAWK:
	    ret = E_J("screech","�b��������");
	    break;
	case MS_NEIGH:
	    ret = E_J("neigh","���ȂȂ���");
	    break;
	case MS_WAIL:
	    ret = E_J("wail","�ߒɂȋ��т�������");
	    break;
	case MS_SILENT:
		ret = E_J("commotion","����߂���");
		break;
	default:
		ret = E_J("scream","����");
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
	pline("%s��%s�I", mon_nam(mtmp), growl_verb);
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
	    yelp_verb = E_J("yowl","�ꂵ���ɖ���");
	    break;
	case MS_BARK:
	case MS_GROWL:
	    yelp_verb = E_J("yelp","�ꂵ���ɖi����");
	    break;
	case MS_ROAR:
	    yelp_verb = E_J("snarl","���Ȃ���");
	    break;
	case MS_SQEEK:
	    yelp_verb = E_J("squeal","�b����������������");
	    break;
	case MS_SQAWK:
	    yelp_verb = E_J("screak","���؂萺��������");
	    break;
	case MS_WAIL:
	    yelp_verb = E_J("wail","�ߒɂȋ��т�������");
	    break;
    }
    if (yelp_verb) {
#ifndef JP
	pline("%s %s!", Monnam(mtmp), vtense((char *)0, yelp_verb));
#else
	pline("%s��%s�I", mon_nam(mtmp), yelp_verb);
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
	    whimper_verb = E_J("whimper","�s�����ɂ��Ȃ���");
	    break;
	case MS_BARK:
	    whimper_verb = E_J("whine","�s�����ɂ��Ȃ���");
	    break;
	case MS_SQEEK:
	    whimper_verb = E_J("squeal","�L�[�L�[�Ɩ���");
	    break;
    }
    if (whimper_verb) {
	pline(E_J("%s %s.","%s%s�B"), Monnam(mtmp), vtense((char *)0, whimper_verb));
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
	verbalize(E_J("I'm hungry.","�󕠂��B"));
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
	    verbl_msg = "GHOUL�����A�ł������[���I"; /* HYDLIDE 2 */
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
						    "�悫%s���A�킪��%s"),
					isnight ? E_J("evening","��") : E_J("day","��"),
					isnight ? E_J("!","�I") :
						  E_J(".  Why do we not rest?","�B���x�݂ɂȂ�Ȃ��̂ŁH"));
				verbl_msg = verbuf;
		    	} else {
		    	    Sprintf(verbuf,"%s%s",
				nightchild ? E_J("Child of the night, ","��̎q��A") : "",
				midnight() ?
					E_J("I can stand this craving no longer!",
					    "�ő����̊��]��}�����ʁI") :
				isnight ?
					E_J("I beg you, help me satisfy this growing craving!",
					    "���ށA���̂����銉�]�𖞂����菕�������Ă���I") :
					E_J("I find myself growing a little weary.",
					    "���͏��X���Ă����悤���B"));
				verbl_msg = verbuf;
			}
		} else if (mtmp->mpeaceful) {
			if (kindred && isnight) {
#ifndef JP
				Sprintf(verbuf, "Good feeding %s!",
	    				flags.female ? "sister" : "brother");
#else
				Sprintf(verbuf, "��������悤�A�䂪���傤������I");
#endif /*JP*/
				verbl_msg = verbuf;
 			} else if (nightchild && isnight) {
				Sprintf(verbuf,
				    E_J("How nice to hear you, child of the night!",
					"��Ċ��������A��̎q��I"));
				verbl_msg = verbuf;
	    		} else
		    		verbl_msg = E_J("I only drink... potions.","���́c�򂵂����܂Ȃ��B");
    	        } else {
			int vampindex;
	    		static const char * const vampmsg[] = {
			       /* These first two (0 and 1) are specially handled below */
	    			E_J("I vant to suck your %s!","���O��%s���悱���I"),
				E_J("I vill come after %s without regret!","%s�����̂ɂ��߂炢�ȂǂȂ����I"),
		    	       /* other famous vampire quotes can follow here if desired */
	    		};
			if (kindred)
			    verbl_msg = E_J("This is my hunting ground that you dare to prowl!",
					    "���O��������܂���Ă��邱�̈�т́A���̎�ꂾ�I");
			else if (youmonst.data == &mons[PM_SILVER_DRAGON] ||
				 youmonst.data == &mons[PM_BABY_SILVER_DRAGON]) {
			    /* Silver dragons are silver in color, not made of silver */
			    Sprintf(verbuf, E_J("%s! Your silver sheen does not frighten me!",
						"����%s�߁I�@���O�̋�F�̔�Ŏ����Ђ����ƂȂǂł��ʂ��I"),
					youmonst.data == &mons[PM_SILVER_DRAGON] ?
					E_J("Fool","��") : E_J("Young Fool","�Ȏᑢ"));
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
		pline("%s�͑傫���̂�����ƁA��������悤��%s���グ���I",
		      Monnam(mtmp), ptr == &mons[PM_HUMAN_WERERAT] ? "���؂萺�̋���" : "�Y����");
#endif /*JP*/
		wake_nearto(mtmp->mx, mtmp->my, 11*11);
	    } else
		pline_msg =
		     E_J("whispers inaudibly.  All you can make out is \"moon\".",
			 "������肪�������Ś������B���낤���Ĕ������̂́u���v�Ƃ������t�����������B");
	    break;
	case MS_BARK:
	    if (flags.moonphase == FULL_MOON && night()) {
		pline_msg = E_J("howls.","���i���������B");
	    } else if (mtmp->mpeaceful) {
		if (mtmp->mtame &&
			(mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
			 moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
		    pline_msg = E_J("whines.","�s�����ɖi�����B");
		else if (mtmp->mtame && EDOG(mtmp)->hungrytime > moves + 1000)
		    pline_msg = E_J("yips.","���������ɖi�����B");
		else {
		    if (mtmp->data != &mons[PM_DINGO])	/* dingos do not actually bark */
			    pline_msg = E_J("barks.","�i�����B");
		}
	    } else {
		pline_msg = E_J("growls.","�Ⴂ�X�萺���������B");
	    }
	    break;
	case MS_MEW:
	    if (mtmp->mtame) {
		if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
			mtmp->mtame < 5)
		    pline_msg = E_J("yowls.","�ꂵ�����Ȗ������������B");
		else if (moves > EDOG(mtmp)->hungrytime)
		    pline_msg = E_J("meows.","�傫�������B");
		else if (EDOG(mtmp)->hungrytime > moves + 1000)
		    pline_msg = E_J("purrs.","�̂ǂ�炵���B");
		else
		    pline_msg = E_J("mews.","�j���A�Ɩ����B");
		break;
	    } /* else FALLTHRU */
	case MS_GROWL:
	    pline_msg = mtmp->mpeaceful ? E_J("snarls.","���Ȃ����B") : E_J("growls!","�Њd�̚X�萺���������I");
	    break;
	case MS_ROAR:
	    pline_msg = mtmp->mpeaceful ? E_J("snarls.","���Ȃ����B") : E_J("roars!","�Ⴆ���I");
	    break;
	case MS_SQEEK:
	    pline_msg = E_J("squeaks.","�L�[�L�[�Ɩ����B");
	    break;
	case MS_SQAWK:
	    if (ptr == &mons[PM_RAVEN] && !mtmp->mpeaceful)
	    	verbl_msg = E_J("Nevermore!","�܂��ƂȂ���"); /* �G�h�K�[�E�A�����E�|�[�u����v���� �ԔV�� ��*/
	    else
	    	pline_msg = E_J("squawks.","�������グ���B");
	    break;
	case MS_HISS:
	    if (!mtmp->mpeaceful)
		pline_msg = E_J("hisses!","�V���[�b�Ƃ��������o�����I");
	    else return 0;	/* no sound */
	    break;
	case MS_BUZZ:
	    pline_msg = mtmp->mpeaceful ? E_J("drones.","�Ⴂ�H�������ĂĂ���B") :
					  E_J("buzzes angrily.","�{�����悤�ȉH�������Ă��B");
	    break;
	case MS_GRUNT:
	    pline_msg = E_J("grunts.","���Ȃ萺���������B");
	    break;
	case MS_NEIGH:
	    if (mtmp->mtame < 5)
		pline_msg = E_J("neighs.","�r�X�������ȂȂ����B");
	    else if (moves > EDOG(mtmp)->hungrytime)
		pline_msg = E_J("whinnies.","��X�������ȂȂ����B");
	    else
		pline_msg = E_J("whickers.","���ȂȂ����B");
	    break;
	case MS_WAIL:
	    pline_msg = E_J("wails mournfully.","�ߒQ�ɂ��ꂽ���т��グ���B");
	    break;
	case MS_GURGLE:
	    pline_msg = E_J("gurgles.","�Ԃ��Ԃ��Ɖ��𗧂Ă��B");
	    break;
	case MS_BURBLE:
	    pline_msg = E_J("burbles.","�킯�̂킩��Ȃ����Ƃ��������B");
	    break;
	case MS_SHRIEK:
	    pline_msg = E_J("shrieks.","���؂萺���グ���B");
	    aggravate();
	    break;
	case MS_IMITATE:
	    pline_msg = E_J("imitates you.","���Ȃ��̐^���������B");
	    break;
	case MS_BONES:
	    pline(E_J("%s rattles noisily.","%s�̓K���K���Ƃ��邳�������B"), Monnam(mtmp));
	    You(E_J("freeze for a moment.","��u��������B"));
	    nomul(-2);
	    break;
	case MS_LAUGH:
	    {
		static const char * const laugh_msg[4] = {
#ifndef JP
		    "giggles.", "chuckles.", "snickers.", "laughs.",
#else
		    "�j���j���΂����B", "���������΂����B", "�}�΂𗁂т����B", "�����グ�ď΂����B",
#endif
		};
		pline_msg = laugh_msg[rn2(4)];
	    }
	    break;
	case MS_MUMBLE:
	    pline_msg = E_J("mumbles incomprehensibly.","���������������t��ꂢ���B");
	    break;
	case MS_DJINNI:
	    if (mtmp->mtame) {
		verbl_msg = E_J("Sorry, I'm all out of wishes.","���߂��A�肢�͕i�؂�Ȃ񂾁B");
	    } else if (mtmp->mpeaceful) {
		if (ptr == &mons[PM_WATER_DEMON])
		    pline_msg = E_J("gurgles.","�S�{�S�{�Ƃ������𗧂Ă��B");
		else
		    verbl_msg = E_J("I'm free!","���R���I");
	    } else verbl_msg = E_J("This will teach you not to disturb me!",
				   "����ς킷�҂��ǂ��Ȃ邩�m�邪�����I");
	    break;
	case MS_BOAST:	/* giants */
	    if (!mtmp->mpeaceful) {
		switch (rn2(4)) {
		case 0: pline(E_J("%s boasts about %s gem collection.",
				  "%s��%s��΃R���N�V���������������B"),
			      Monnam(mtmp), mhis(mtmp));
			break;
		case 1: pline_msg = E_J("complains about a diet of mutton.",
					"�r���̐H���ɕs�������炵���B");
			break;
	       default: pline_msg = E_J("shouts \"Fee Fie Foe Foo!\" and guffaws.",
					"\"Fee Fie Foe Foo!\"�Ƌ��ԂƁA�吺�ŏ΂����B"); /* Adventure? */
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
		pline_msg = E_J("wants nothing to do with you.","���Ȃ��Ɋւ�肽���Ȃ��B");
	    else if (mtmp->mhp < mtmp->mhpmax/4)
		pline_msg = E_J("moans.","���߂����B");
	    else if (mtmp->mconf || mtmp->mstun)
		verbl_msg = !rn2(3) ? E_J("Huh?","�́H") : rn2(2) ? E_J("What?","���H") : E_J("Eh?","���H");
	    else if (!mtmp->mcansee)
		verbl_msg = E_J("I can't see!","���������Ȃ��I");
	    else if (mtmp->mtrapped) {
		struct trap *t = t_at(mtmp->mx, mtmp->my);

		if (t) t->tseen = 1;
		verbl_msg = E_J("I'm trapped!","㩂ɂ͂܂��Ă��܂����I");
	    } else if (mtmp->mhp < mtmp->mhpmax/2)
		pline_msg = E_J("asks for a potion of healing.","�񕜂̖�𕪂��Ă���Ȃ����Ɛq�˂��B");
	    else if (mtmp->mtame && !mtmp->isminion &&
						moves > EDOG(mtmp)->hungrytime)
		verbl_msg = E_J("I'm hungry.","�󕠂��B");
	    /* Specific monsters' interests */
	    else if (is_elf(ptr))
		pline_msg = E_J("curses orcs.","�I�[�N�Ɉ��Ԃ������B");
	    else if (is_dwarf(ptr))
		pline_msg = E_J("talks about mining.","�̌@�ɂ��Ęb�����B");
	    else if (likes_magic(ptr))
		pline_msg = E_J("talks about spellcraft.","���p�̂��Ƃ�b�����B");
	    else if (ptr->mlet == S_CENTAUR)
		pline_msg = E_J("discusses hunting.","��ɂ��ċc�_�����B");
	    else switch (monsndx(ptr)) {
		case PM_HOBBIT:
		    pline_msg = (mtmp->mhpmax - mtmp->mhp >= 10) ?
				E_J("complains about unpleasant dungeon conditions.",
				    "�_���W�������̕s�����ȏ�Ԃɕs�����q�ׂ��B")
				: E_J("asks you about the One Ring.","��̎w�ւɂ��Ă����˂��B");
		    break;
		case PM_ARCHEOLOGIST:
    pline_msg = E_J("describes a recent article in \"Spelunker Today\" magazine.",
		    "�g�����̓��A�T���h���̍ŋ߂̋L���������Ă��ꂽ�B");
		    break;
#ifdef TOURIST
		case PM_TOURIST:
		    verbl_msg = E_J("Aloha.","�A���[�n�B");
		    break;
#endif
		default:
		    pline_msg = E_J("discusses dungeon exploration.",
				    "�n�����{�̒T���ɂ��Ĉӌ����q�ׂ��B");
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
			verbl_msg = E_J("Hello, sailor.","����ɂ��́A��������B");
			break;
		case 1:
			pline_msg = E_J("comes on to you.","�������Ă����B");
			break;
		default:
			pline_msg = E_J("cajoles you.","���Ȃ��������Ă��B");
	    }
	    break;
#ifdef KOPS
	case MS_ARREST:
	    if (mtmp->mpeaceful)
		/* Sergeant Joe Friday: Dragnet  */
		verbalize(E_J("Just the facts, %s.","�������̐S�ł��A%s�B"),
		      flags.female ? E_J("Ma'am","���w�l") : E_J("Sir","����l"));
	    else {
		static const char * const arrest_msg[3] = {
		    E_J("Anything you say can be used against you.",
			"�����͂��ׂĕs���ȏ؋��Ƃ��č̗p�����\��������B"),
		    E_J("You're under arrest!","�ߕ߂���I"),
		    E_J("Stop in the name of the Law!","�@�̖��̉��ɒ��~����I"),
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
				"�l��������O�ɁA���̕�������܂��Ȃ����I");
	    else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
		verbl_msg = Role_if(PM_HEALER) ?
			  E_J("Doc, I can't help you unless you cooperate.",
			      "�搶�A���͂��Ă��������Ȃ��Ƃ���`���ł��܂���B") :
			  E_J("Please undress so I can examine you.","�f�@���܂��̂ŁA����E���ł��������B");
#ifdef TOURIST
	    else if (uarmu)
		verbl_msg = E_J("Take off your shirt, please.","�V���c��E���ł��������ˁB");
#endif
	    else verbl_msg = E_J("Relax, this won't hurt a bit.","���v�A�����Ƃ��ɂ�����܂����B");
	    break;
	case MS_GUARD:
#ifndef GOLDOBJ
	    if (u.ugold)
#else
	    if (money_cnt(invent))
#endif
		verbl_msg = E_J("Please drop that gold and follow me.",
				"����u���āA���ė��Ȃ����B");
	    else
		verbl_msg = E_J("Please follow me.","���ė��Ȃ����B");
	    break;
	case MS_SOLDIER:
	    {
		static const char * const soldier_foe_msg[3] = {
		    E_J("Resistance is useless!","���ʂȒ�R�͎~�߂�I"),
		    E_J("You're dog meat!","���̃G�T�ɂȂ�I"),
		    E_J("Surrender!","���~����I"),
		},		  * const soldier_pax_msg[3] = {
		    E_J("What lousy pay we're getting here!","�����̋����̃V�P���Ղ������Ȃ����I"),
		    E_J("The food's not fit for Orcs!","�I�[�N�����Ă���Ȕт͋���Ƃ�񂾂낤��I"),
		    E_J("My feet hurt, I've been on them all day!","�����ɂނ�A������������ςȂ����I"),
		};
		verbl_msg = mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
					    : soldier_foe_msg[rn2(3)];
	    }
	    break;
	case MS_RIDER:
	    if (ptr == &mons[PM_DEATH] && !rn2(10))
		pline_msg = E_J("is busy reading a copy of Sandman #8.",
				"�́u�T���h�}���v��8����ǂނ̂ɖZ�����B");
	    else verbl_msg = E_J("Who do you think you are, War?",
				 "�������N���Ǝv���Ă���̂��ˁA�g�푈�h��H");
	    break;
    }

    if (pline_msg) pline(E_J("%s %s","%s��%s"), Monnam(mtmp), pline_msg);
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
	pline("%s�̎p�ł́A���Ȃ��͘b�����Ƃ��ł��Ȃ��B", JMONNAM(u.umonnum));
#endif /*JP*/
	return(0);
    }
    if (Strangled) {
	You_cant(E_J("speak.  You're choking!","����Ȃ��B���Ȃ��͑����ł��Ȃ��I"));
	return(0);
    }
    if (u.uswallow) {
	pline(E_J("They won't hear you out there.",
		  "����ȂƂ���ł́A�N���b�𕷂��Ă���Ȃ��B"));
	return(0);
    }
    if (Underwater) {
	Your(E_J("speech is unintelligible underwater.",
		 "���t�͐����̂��߁A����s�\�ɂȂ����B"));
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
		    "�N�Ƙb���܂����H(�ǂ̕���)"))) {
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
	pline("%s�̕����ɂ͒N�����Ȃ��B", u.dz < 0 ? "��" : "��");
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
		  "�Ƃ茾�͖��{�Z�l�̈��K���B"));
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
		      "%s�͂��Ȃ��ɋC�Â��Ă��Ȃ��悤���B"), Monnam(mtmp));
	return(0);
    }

    /* if this monster is waiting for something, prod it into action */
    mtmp->mstrategy &= ~STRAT_WAITMASK;

    if (mtmp->mtame && mtmp->meating) {
	if (!canspotmons(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);
	pline(E_J("%s is eating noisily.",
		  "%s�͑��X�����H�ׂĂ���B"), Monnam(mtmp));
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
