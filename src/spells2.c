/* File: spells2.c */

/* Purpose: Spell code (part 2) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

/* Chance of using syllables to form the name instead of the "template" files */
#define TABLE_NAME      45
#define A_CURSED        13
#define WEIRD_LUCK      12
#define BIAS_LUCK       20
/*
 * Bias luck needs to be higher than weird luck,
 * since it is usually tested several times...
 */

#define ACTIVATION_CHANCE 3

void summon_dragon_riders();


/*
 * Grow trees
 */
void grow_trees(int rad)
{
        int a,i,j;

        for(a=0;a<rad*rad+11;a++){
                i=(Rand_mod((rad*2)+1)-rad+Rand_mod((rad*2)+1)-rad)/2;
                j=(Rand_mod((rad*2)+1)-rad+Rand_mod((rad*2)+1)-rad)/2;
                if (in_bounds(py + i, px + j) && (cave_clean_bold(py+i, px+j)))
                {
                        cave_set_feat(py+i, px+j, FEAT_TREES);
                }
        }
}

/*
 * Increase players hit points, notice effects
 */
bool hp_player(int num)
{
	/* Healing needed */
	if (p_ptr->chp < p_ptr->mhp)
	{
		/* Gain hitpoints */
		p_ptr->chp += num;

		/* Enforce maximum */
		if (p_ptr->chp >= p_ptr->mhp)
		{
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;
		}

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Heal 0-4 */
		if (num < 5)
		{
			msg_print("You feel a little better.");
		}

		/* Heal 5-14 */
		else if (num < 15)
		{
			msg_print("You feel better.");
		}

		/* Heal 15-34 */
		else if (num < 35)
		{
			msg_print("You feel much better.");
		}

		/* Heal 35+ */
		else
		{
			msg_print("You feel very good.");
		}

		/* Notice */
		return (TRUE);
	}

	/* Ignore */
	return (FALSE);
}



/*
 * Leave a "glyph of warding" which prevents monster movement
 */
void warding_glyph(void)
{
	/* XXX XXX XXX */
	if (!cave_clean_bold(py, px))
	{
		msg_print("The object resists the spell.");
		return;
	}
	if (special_flag)
	{
		msg_print("No glyph of warding on special level.");
		return;
	}

	/* Create a glyph */
	cave_set_feat(py, px, FEAT_GLYPH);
}

void explosive_rune(void)
{
	/* XXX XXX XXX */
	if (!cave_clean_bold(py, px))
	{
		msg_print("The object resists the spell.");
		return;
	}

	/* Create a glyph */
	cave_set_feat(py, px, FEAT_MINOR_GLYPH);
}



/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_pos[] =
{
	"strong",
	"smart",
	"wise",
	"dextrous",
	"healthy",
	"cute"
};


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_neg[] =
{
	"weak",
	"stupid",
	"naive",
	"clumsy",
	"sickly",
	"ugly"
};


/*
 * Lose a "point"
 */
bool do_dec_stat(int stat, int mode)
{
	bool sust = FALSE;

	/* Access the "sustain" */
	switch (stat)
	{
		case A_STR: if (p_ptr->sustain_str) sust = TRUE; break;
		case A_INT: if (p_ptr->sustain_int) sust = TRUE; break;
		case A_WIS: if (p_ptr->sustain_wis) sust = TRUE; break;
		case A_DEX: if (p_ptr->sustain_dex) sust = TRUE; break;
		case A_CON: if (p_ptr->sustain_con) sust = TRUE; break;
		case A_CHR: if (p_ptr->sustain_chr) sust = TRUE; break;
	}

	/* Sustain */
	if (sust)
	{
		/* Message */
		msg_format("You feel %s for a moment, but the feeling passes.",
			   desc_stat_neg[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Attempt to reduce the stat */
	if (dec_stat(stat, 10, mode))
	{
		/* Message */
		msg_format("You feel very %s.", desc_stat_neg[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Restore lost "points" in a stat
 */
bool do_res_stat(int stat)
{
	/* Attempt to increase */
	if (res_stat(stat))
	{
		/* Message */
		msg_format("You feel less %s.", desc_stat_neg[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Gain a "point" in a stat
 */
bool do_inc_stat(int stat)
{
	bool res;

	/* Restore strength */
	res = res_stat(stat);

	/* Attempt to increase */
	if (inc_stat(stat))
	{
		/* Message */
		msg_format("Wow!  You feel very %s!", desc_stat_pos[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Restoration worked */
	if (res)
	{
		/* Message */
		msg_format("You feel less %s.", desc_stat_neg[stat]);

		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}



/*
 * Identify everything being carried.
 * Done by a potion of "self knowledge".
 */
void identify_pack(void)
{
	int i;

	/* Simply identify and know every item */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Aware and Known */
		object_aware(o_ptr);
		object_known(o_ptr);
	}
}






/*
 * Used by the "enchant" function (chance of failure)
 * (modified for Zangband, we need better stuff there...) -- TY
 */
static int enchant_table[16] =
{
	0, 10,  50, 100, 200,
	300, 400, 500, 650, 800,
	950, 987, 993, 995, 998,
	1000
};


/*
 * Removes curses from items in inventory
 *
 * Note that Items which are "Perma-Cursed" (The One Ring,
 * The Crown of Morgoth) can NEVER be uncursed.
 *
 * Note that if "all" is FALSE, then Items which are
 * "Heavy-Cursed" (Mormegil, Calris, and Weapons of Morgul)
 * will not be uncursed.
 */
static int remove_curse_aux(int all)
{
	int i, cnt = 0;

	/* Attempt to uncurse items being worn */
	for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
	{
                u32b f1, f2, f3, f4;

		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Uncursed already */
		if (!cursed_p(o_ptr)) continue;

		/* Extract the flags */
                object_flags(o_ptr, &f1, &f2, &f3, &f4);

		/* Heavily Cursed Items need a special spell */
		if (!all && (f3 & (TR3_HEAVY_CURSE))) continue;

		/* Perma-Cursed Items can NEVER be uncursed */
		if (f3 & (TR3_PERMA_CURSE)) continue;

		/* Uncurse it */
		o_ptr->ident &= ~(IDENT_CURSED);

		/* Hack -- Assume felt */
		o_ptr->ident |= (IDENT_SENSE);

		if (o_ptr->art_flags3 & (TR3_CURSED))
			o_ptr->art_flags3 &= ~(TR3_CURSED);

		if (o_ptr->art_flags3 & (TR3_HEAVY_CURSE))
			o_ptr->art_flags3 &= ~(TR3_HEAVY_CURSE);

		/* Take note */
		o_ptr->note = quark_add("uncursed");

                /* Reverse the curse effect */
 /* jk - scrolls of *remove curse* have a 1 in (55-level chance to */
 /* reverse the curse effects - a ring of damage(-15) {cursed} then */
 /* becomes a ring of damage (+15) */
 /* this does not go for artifacts - a Sword of Mormegil +40,+60 would */
 /* be somewhat unbalancing */
 /* due to the nature of this procedure, it only works on cursed items */
 /* ie you get only one chance! */
                if ((randint(55-p_ptr->lev)==1) && !artifact_p(o_ptr))
                {
                        if (o_ptr->to_a<0) o_ptr->to_a=-o_ptr->to_a;
                        if (o_ptr->to_h<0) o_ptr->to_h=-o_ptr->to_h;
                        if (o_ptr->to_d<0) o_ptr->to_d=-o_ptr->to_d;
                        if (o_ptr->pval<0) o_ptr->pval=-o_ptr->pval;
                }

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);

		/* Count the uncursings */
		cnt++;
	}

	/* Return "something uncursed" */
	return (cnt);
}


/*
 * Remove most curses
 */
bool remove_curse(void)
{
	return (remove_curse_aux(FALSE));
}

/*
 * Remove all curses
 */
bool remove_all_curse(void)
{
	return (remove_curse_aux(TRUE));
}



/*
 * Restores any drained experience
 */
bool restore_level(void)
{
	/* Restore experience */
	if (p_ptr->exp < p_ptr->max_exp)
	{
		/* Message */
		msg_print("You feel your life energies returning.");

		/* Restore the experience */
		p_ptr->exp = p_ptr->max_exp;

		/* Check the experience */
		check_experience();

		/* Did something */
		return (TRUE);
	}

	/* No effect */
	return (FALSE);
}


bool alchemy(void) /* Turns an object into gold, gain some of its value in a shop */
{
	int item, amt = 1;
	int old_number;
	long price;
	bool force = FALSE;
	object_type *o_ptr;
	char o_name[80];
	char out_val[160];

	cptr q, s;

	/* Hack -- force destruction */
	if (command_arg > 0) force = TRUE;

	/* Get an item */
	q = "Turn which item to gold? ";
	s = "You have nothing to turn to gold.";
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* See how many items */
	if (o_ptr->number > 1)
	{
		/* Get a quantity */
		amt = get_quantity(NULL, o_ptr->number);

		/* Allow user abort */
	if (amt <= 0) return FALSE;
	}


	/* Describe the object */
	old_number = o_ptr->number;
	o_ptr->number = amt;
	object_desc(o_name, o_ptr, TRUE, 3);
	o_ptr->number = old_number;

	/* Verify unless quantity given */
	if (!force)
	{
		if (!((auto_destroy) && (object_value(o_ptr)<1)))
		{
			/* Make a verification */
			sprintf(out_val, "Really turn %s to gold? ", o_name);
			if (!get_check(out_val)) return FALSE;
		}
	}

	/* Artifacts cannot be destroyed */
	if (artifact_p(o_ptr) || o_ptr->art_name)
	{
		cptr feel = "special";

		/* Message */
		msg_format("You fail to turn %s to gold!", o_name);

		/* Hack -- Handle icky artifacts */
		if (cursed_p(o_ptr) || broken_p(o_ptr)) feel = "terrible";

		/* Hack -- inscribe the artifact */
		o_ptr->note = quark_add(feel);

		/* We have "felt" it (again) */
		o_ptr->ident |= (IDENT_SENSE);

		/* Combine the pack */
		p_ptr->notice |= (PN_COMBINE);

		/* Window stuff */
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Done */
	return FALSE;
	}

	price = object_value_real(o_ptr);

	if (price <= 0)
		/* Message */
		msg_format("You turn %s to fool's gold.", o_name);
	else
	{
		price /= 3;

		if (amt > 1) price *= amt;

		if (price > 30000) price = 30000;
		msg_format("You turn %s to %ld coins worth of gold.", o_name, price);
		p_ptr->au += price;

		/* Redraw gold */
		p_ptr->redraw |= (PR_GOLD);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

	}

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -amt);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

	return TRUE;
}




/*
 * self-knowledge... idea from nethack.  Useful for determining powers and
 * resistences of items.  It saves the screen, clears it, then starts listing
 * attributes, a screenful at a time.  (There are a LOT of attributes to
 * list.  It will probably take 2 or 3 screens for a powerful character whose
 * using several artifacts...) -CFT
 *
 * It is now a lot more efficient. -BEN-
 *
 * See also "identify_fully()".
 *
 * XXX XXX XXX Use the "show_file()" method, perhaps.
 */
void self_knowledge(FILE *fff)
{
	int i = 0, j, k;

	u32b f1 = 0L, f2 = 0L, f3 = 0L;

	object_type *o_ptr;

	char Dummy[80];

	cptr info[200];

	int plev = p_ptr->lev;

	strcpy (Dummy, "");

	/* Acquire item flags from equipment */
	for (k = INVEN_WIELD; k < INVEN_TOTAL; k++)
	{
                u32b t1, t2, t3, t4;

		o_ptr = &inventory[k];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Extract the flags */
                object_flags(o_ptr, &t1, &t2, &t3, &t4);

		/* Extract flags */
		f1 |= t1;
		f2 |= t2;
		f3 |= t3;
	}


	/* Racial powers... */
	if (p_ptr->body_monster != 0)
	{
		monster_race *r_ptr = &r_info[p_ptr->body_monster];

		if (r_ptr->flags1 & RF1_CHAR_CLEAR ||
		    r_ptr->flags1 & RF1_ATTR_CLEAR)
			info[i++] = "You are clear.";
		if ((r_ptr->flags1 & RF1_CHAR_MULTI) ||
		    (r_ptr->flags2 & RF2_SHAPECHANGER))
			info[i++] = "Your form constantly change.";
		if (r_ptr->flags1 & RF1_ATTR_MULTI)
			info[i++] = "Your color constantly change.";
		if (r_ptr->flags1 & RF1_NEVER_BLOW)
			info[i++] = "You do not have any physical weapon";
		if (r_ptr->flags1 & RF1_NEVER_MOVE)
			info[i++] = "You cannot move.";
		if ((r_ptr->flags1 & RF1_RAND_25) &&
		    (r_ptr->flags1 & RF1_RAND_50))
			info[i++] = "You move extremely erratically.";
		else if (r_ptr->flags1 & RF1_RAND_50)
		        info[i++] = "You move somewhat erratically.";
		else if (r_ptr->flags1 & RF1_RAND_25)
			info[i++] = "You move a bit erratically.";
		if (r_ptr->flags2 & RF2_STUPID)
			info[i++] = "You are very stupid (INT -4).";
		if (r_ptr->flags2 & RF2_SMART)
			info[i++] = "You are very smart (INT +4).";
		/* Not implemented */
		if (r_ptr->flags2 & RF2_CAN_SPEAK)
			info[i++] = "You can speak.";
		else
			info[i++] = "You cannot speak.";
		/* Not implemented */
		if (r_ptr->flags2 & RF2_COLD_BLOOD)
			info[i++] = "You are cold blooded.";
		/* Not implemented */
		if (r_ptr->flags2 & RF2_EMPTY_MIND)
			info[i++] = "You have empty mind.";
		if (r_ptr->flags2 & RF2_WEIRD_MIND)
			info[i++] = "You have weird mind.";
		if (r_ptr->flags2 & RF2_MULTIPLY)
			info[i++] = "You can multiply.";	
		if (r_ptr->flags2 & RF2_POWERFUL)
			info[i++] = "You have strong breath.";
		/* Not implemented */
		if (r_ptr->flags2 & RF2_ELDRITCH_HORROR)
			info[i++] = "You are an eldritch horror.";
		if (r_ptr->flags2 & RF2_OPEN_DOOR)
			info[i++] = "You can open doors.";
		else
			info[i++] = "You cannot open doors.";
		if (r_ptr->flags2 & RF2_BASH_DOOR)
			info[i++] = "You can bash doors.";
		else
			info[i++] = "You cannot bash doors.";
		if (r_ptr->flags2 & RF2_PASS_WALL)
			info[i++] = "You can pass walls.";
		if (r_ptr->flags2 & RF2_KILL_WALL)
			info[i++] = "You destroy walls.";
		/* Not implemented */
		if (r_ptr->flags2 & RF2_MOVE_BODY)
			info[i++] = "You can move monsters.";
		/* Not implemented */
/* #ifdef 0 */
/*                 They are disabled, because the r_info.txt array has to    */
/*                  few RF2_TAKE_ITEM flags...                              */
/*                if (r_ptr->flags2 & RF2_TAKE_ITEM)                           */
/*                        info[i++] = "You can pick up items.";                */
/*                else                                                         */
/*                        info[i++] = "You cannot pick up items.";             */
/* #endif                                                                      */
		/* Not implemented */
		if (r_ptr->flags3 & RF3_ORC)
			info[i++] = "You have orc blood in your veins.";
		/* Not implemented */
		else if (r_ptr->flags3 & RF3_TROLL)
			info[i++] = "You have troll blood in your veins.";
		/* Not implemented */
		else if (r_ptr->flags3 & RF3_GIANT)
			info[i++] = "You have giant blood in your veins.";
		/* Not implemented */
		else if (r_ptr->flags3 & RF3_DRAGON)
			info[i++] = "You have dragon blood in your veins.";
		/* Not implemented */
		else if (r_ptr->flags3 & RF3_DEMON)
			info[i++] = "You have demon blood in your veins.";
		/* Not implemented */
		else if (r_ptr->flags3 & RF3_UNDEAD)
			info[i++] = "You are an undead.";
		/* Not implemented */
		else if (r_ptr->flags3 & RF3_ANIMAL)
			info[i++] = "You are an animal.";
		/* Not implemented */
		else if (r_ptr->flags3 & RF3_DRAGONRIDER)
			info[i++] = "You have dragonrider blood in your veins.";
		if (r_ptr->flags3 & RF3_EVIL)
			info[i++] = "You are inherently evil.";
		else if (r_ptr->flags3 & RF3_GOOD)
			info[i++] = "You are inherently good.";
		if (r_ptr->flags3 & RF3_AURA_COLD)
			info[i++] = "You are surrounded by a chilly aura.";
		/* Not implemented */
		if (r_ptr->flags3 & RF3_NONLIVING)
			info[i++] = "You are not living.";
		/* Not implemented */
		if (r_ptr->flags3 & RF3_HURT_LITE)
			info[i++] = "Your eyes are sensible to bright light.";
		/* Not implemented */
		if (r_ptr->flags3 & RF3_HURT_ROCK)
			info[i++] = "You can be hurt by rock remover.";
		if (r_ptr->flags3 & RF3_SUSCEP_FIRE)
			info[i++] = "You are sensitive to fire.";
		if (r_ptr->flags3 & RF3_SUSCEP_COLD)
			info[i++] = "You are sensitive to cold.";
		if (r_ptr->flags3 & RF3_RES_TELE)
			info[i++] = "You are resistant to teleportation.";
		if (r_ptr->flags3 & RF3_RES_WATE)
			info[i++] = "You are resistant to water.";
		if (r_ptr->flags3 & RF3_RES_DISE)
			info[i++] = "You are resistant to disease.";
		/* Not implemented */
		if (r_ptr->flags3 & RF3_NO_SLEEP)
			info[i++] = "You cannot be slept.";
		if (r_ptr->flags3 & RF3_NO_FEAR)
			info[i++] = "You are immune to fear.";
		if (r_ptr->flags3 & RF3_NO_STUN)
			info[i++] = "You are immune to stun.";
		if (r_ptr->flags3 & RF3_NO_CONF)
			info[i++] = "You are immune to confusion.";
		if (r_ptr->flags3 & RF3_NO_SLEEP)
			info[i++] = "You are immune to sleep.";

		if (r_ptr->flags4 & RF4_SHRIEK)
			info[i++] = "You can aggrevate monsters.";
		if (r_ptr->flags4 & RF4_ROCKET)
			info[i++] = "You can fire a rocket.";
		if (r_ptr->flags4 & RF4_ARROW_1)
			info[i++] = "You can fire a light arrow.";
		if (r_ptr->flags4 & RF4_ARROW_2)
			info[i++] = "You can fire a heavy arrow.";
		if (r_ptr->flags4 & RF4_ARROW_3)
			info[i++] = "You can fire a light missile.";
		if (r_ptr->flags4 & RF4_ARROW_4)
			info[i++] = "You can fire a heavy missile.";
		if (r_ptr->flags4 & RF4_BR_ACID)
			info[i++] = "You can breathe acid.";
		if (r_ptr->flags4 & RF4_BR_ELEC)
			info[i++] = "You can breathe electricity.";
		if (r_ptr->flags4 & RF4_BR_FIRE)
			info[i++] = "You can breathe fire.";
		if (r_ptr->flags4 & RF4_BR_COLD)
			info[i++] = "You can breathe cold.";
		if (r_ptr->flags4 & RF4_BR_POIS)
			info[i++] = "You can breathe poison.";
		if (r_ptr->flags4 & RF4_BR_NETH)
			info[i++] = "You can breathe nether.";
		if (r_ptr->flags4 & RF4_BR_LITE)
			info[i++] = "You can breathe light.";
		if (r_ptr->flags4 & RF4_BR_DARK)
			info[i++] = "You can breathe darkness.";
		if (r_ptr->flags4 & RF4_BR_CONF)
			info[i++] = "You can breathe confusion.";
		if (r_ptr->flags4 & RF4_BR_SOUN)
			info[i++] = "You can breathe sound.";
		if (r_ptr->flags4 & RF4_BR_CHAO)
			info[i++] = "You can breathe chaos.";
		if (r_ptr->flags4 & RF4_BR_DISE)
			info[i++] = "You can breathe disenchantment.";
		if (r_ptr->flags4 & RF4_BR_NEXU)
			info[i++] = "You can breathe nexus.";
		if (r_ptr->flags4 & RF4_BR_TIME)
			info[i++] = "You can breathe time.";
		if (r_ptr->flags4 & RF4_BR_INER)
			info[i++] = "You can breathe inertia.";
		if (r_ptr->flags4 & RF4_BR_GRAV)
			info[i++] = "You can breathe gravity.";
		if (r_ptr->flags4 & RF4_BR_SHAR)
			info[i++] = "You can breathe shards.";
		if (r_ptr->flags4 & RF4_BR_PLAS)
			info[i++] = "You can breathe plasma.";
		if (r_ptr->flags4 & RF4_BR_WALL)
			info[i++] = "You can breathe force.";
		if (r_ptr->flags4 & RF4_BR_MANA)
			info[i++] = "You can breathe mana.";
		if (r_ptr->flags4 & RF4_BR_NUKE)
			info[i++] = "You can breathe nuke.";
		if (r_ptr->flags4 & RF4_BR_DISI)
			info[i++] = "You can breathe disitegration.";
		if (r_ptr->flags5 & RF5_BA_ACID)
			info[i++] = "You can cast a ball of acid.";
		if (r_ptr->flags5 & RF5_BA_ELEC)
			info[i++] = "You can cast a ball of electricity.";
		if (r_ptr->flags5 & RF5_BA_FIRE)
			info[i++] = "You can cast a ball of fire.";
		if (r_ptr->flags5 & RF5_BA_COLD)
			info[i++] = "You can cast a ball of cold.";
		if (r_ptr->flags5 & RF5_BA_POIS)
			info[i++] = "You can cast a ball of poison.";
		if (r_ptr->flags5 & RF5_BA_NETH)
			info[i++] = "You can cast a ball of nether.";
		if (r_ptr->flags5 & RF5_BA_WATE)
			info[i++] = "You can cast a ball of water.";
		/* Not implemented */
		if (r_ptr->flags5 & RF5_DRAIN_MANA)
			info[i++] = "You can drain mana.";
		if (r_ptr->flags5 & RF5_MIND_BLAST)
			info[i++] = "You can cause mind blasting.";
		if (r_ptr->flags5 & RF5_BRAIN_SMASH)
			info[i++] = "You can cause brain smashing.";
		if (r_ptr->flags5 & RF5_CAUSE_1)
			info[i++] = "You can cause light wounds.";
		if (r_ptr->flags5 & RF5_CAUSE_2)
			info[i++] = "You can cause serious wounds.";
		if (r_ptr->flags5 & RF5_CAUSE_3)
			info[i++] = "You can cause critical wounds.";
		if (r_ptr->flags5 & RF5_CAUSE_4)
			info[i++] = "You can cause mortal wounds.";
		if (r_ptr->flags5 & RF5_BO_ACID)
			info[i++] = "You can cast a bolt of acid.";
		if (r_ptr->flags5 & RF5_BO_ELEC)
			info[i++] = "You can cast a bolt of electricity.";
		if (r_ptr->flags5 & RF5_BO_FIRE)
			info[i++] = "You can cast a bolt of fire.";
		if (r_ptr->flags5 & RF5_BO_COLD)
			info[i++] = "You can cast a bolt of cold.";
		if (r_ptr->flags5 & RF5_BO_POIS)
			info[i++] = "You can cast a bolt of poison.";
		if (r_ptr->flags5 & RF5_BO_NETH)
			info[i++] = "You can cast a bolt of nether.";
		if (r_ptr->flags5 & RF5_BO_WATE)
			info[i++] = "You can cast a bolt of water.";
		if (r_ptr->flags5 & RF5_BO_MANA)
			info[i++] = "You can cast a bolt of mana.";
		if (r_ptr->flags5 & RF5_BO_PLAS)
			info[i++] = "You can cast a bolt of plasma.";
		if (r_ptr->flags5 & RF5_BO_ICEE)
			info[i++] = "You can cast a bolt of ice.";
		if (r_ptr->flags5 & RF5_MISSILE)
			info[i++] = "You can cast magic missile.";
		if (r_ptr->flags5 & RF5_SCARE)
			info[i++] = "You can terrify.";
		if (r_ptr->flags5 & RF5_BLIND)
			info[i++] = "You can blind.";
		if (r_ptr->flags5 & RF5_CONF)
			info[i++] = "You can use confusion.";
		if (r_ptr->flags5 & RF5_SLOW)
			info[i++] = "You can cast slow.";
		if (r_ptr->flags5 & RF5_HOLD)
			info[i++] = "You can touch to paralyze.";
		if (r_ptr->flags6 & RF6_HASTE)
			info[i++] = "You can haste yourself.";
		if (r_ptr->flags6 & RF6_HAND_DOOM)
			info[i++] = "You can invoke Hand of Doom.";
		if (r_ptr->flags6 & RF6_HEAL)
			info[i++] = "You can heal yourself.";
		if (r_ptr->flags6 & RF6_BLINK)
			info[i++] = "You can blink.";
		if (r_ptr->flags6 & RF6_TPORT)
			info[i++] = "You can teleport.";
		if (r_ptr->flags6 & RF6_TELE_TO)
			info[i++] = "You can go between.";
		if (r_ptr->flags6 & RF6_TELE_AWAY)
			info[i++] = "You can teleport away.";
		if (r_ptr->flags6 & RF6_TELE_LEVEL)
			info[i++] = "You can teleport level.";
		if (r_ptr->flags6 & RF6_DARKNESS)
			info[i++] = "You can create darkness.";
		if (r_ptr->flags6 & RF6_TRAPS)
			info[i++] = "You can create traps.";
		/* Not implemented */
		if (r_ptr->flags6 & RF6_FORGET)
			info[i++] = "You can fade memories.";
		if (r_ptr->flags6 & RF6_RAISE_DEAD)
			info[i++] = "You can Raise the Dead.";
		if (r_ptr->flags6 & RF6_S_BUG)
			info[i++] = "You can magically summon a Software Bugs.";
		if (r_ptr->flags6 & RF6_S_RNG)
			info[i++] = "You can magically summon the RNG.";
		if (r_ptr->flags6 & RF6_S_DRAGONRIDER)
			info[i++] = "You can magically summon some Dragonriders.";
		if (r_ptr->flags6 & RF6_S_KIN)
			info[i++] = "You can magically summon some Kins.";
		if (r_ptr->flags6 & RF6_S_CYBER)
			info[i++] = "You can magically summon a Cyberdemon.";
		if (r_ptr->flags6 & RF6_S_MONSTER)
			info[i++] = "You can magically summon a monster.";
		if (r_ptr->flags6 & RF6_S_MONSTERS)
			info[i++] = "You can magically summon monsters.";
		if (r_ptr->flags6 & RF6_S_ANT)
			info[i++] = "You can magically summon ants.";
		if (r_ptr->flags6 & RF6_S_SPIDER)
			info[i++] = "You can magically summon spiders.";
		if (r_ptr->flags6 & RF6_S_HOUND)
			info[i++] = "You can magically summon hounds.";
		if (r_ptr->flags6 & RF6_S_HYDRA)
			info[i++] = "You can magically summon hydras.";
		if (r_ptr->flags6 & RF6_S_ANGEL)
			info[i++] = "You can magically summon an angel.";
		if (r_ptr->flags6 & RF6_S_DEMON)
			info[i++] = "You can magically summon a demon.";
		if (r_ptr->flags6 & RF6_S_UNDEAD)
			info[i++] = "You can magically summon an undead.";
		if (r_ptr->flags6 & RF6_S_DRAGON)
			info[i++] = "You can magically summon a dragon.";
		if (r_ptr->flags6 & RF6_S_HI_UNDEAD)
			info[i++] = "You can magically summon greater undeads.";
		if (r_ptr->flags6 & RF6_S_HI_DRAGON)
			info[i++] = "You can magically summon greater dragons.";
		if (r_ptr->flags6 & RF6_S_WRAITH)
			info[i++] = "You can magically summon a wraith.";
		/* Not implemented */
		if (r_ptr->flags6 & RF6_S_UNIQUE)
			info[i++] = "You can magically summon an unique monster.";
		/* Not implemented */
		if (r_ptr->flags7 & RF7_AQUATIC)
			info[i++] = "You are aquatic.";
		/* Not implemented */
		if (r_ptr->flags7 & RF7_CAN_SWIM)
			info[i++] = "You can swim.";
		/* Not implemented */
		if (r_ptr->flags7 & RF7_CAN_FLY)
			info[i++] = "You can fly.";
		if ((r_ptr->flags7 & RF7_MORTAL) == 0)
			info[i++] = "You are immortal.";
		/* Not implemented */
		if (r_ptr->flags7 & RF7_NAZGUL)
			info[i++] = "You are a Nazgul.";

                if (r_ptr->flags8 & RF8_WILD_TOWN)
			info[i++] = "You appear in towns.";
		if (r_ptr->flags8 & RF8_WILD_SHORE)
			info[i++] = "You appear in shores.";
		if (r_ptr->flags8 & RF8_WILD_OCEAN)
			info[i++] = "You appear in the ocean.";
		if (r_ptr->flags8 & RF8_WILD_WASTE)
			info[i++] = "You appear in the waste.";
		if (r_ptr->flags8 & RF8_WILD_WOOD)
			info[i++] = "You appear in woods.";
		if (r_ptr->flags8 & RF8_WILD_VOLCANO)
			info[i++] = "You appear in volcanos.";
		if (r_ptr->flags8 & RF8_WILD_MOUNTAIN)
			info[i++] = "You appear in the mountains.";
		if (r_ptr->flags8 & RF8_WILD_GRASS)
			info[i++] = "You appear in grasses.";
		
		if (r_ptr->flags9 & RF9_SUSCEP_ACID)
			info[i++] = "You are sensitive to acid.";
		if (r_ptr->flags9 & RF9_SUSCEP_ELEC)
			info[i++] = "You are sensitive to electricity.";
		if (r_ptr->flags9 & RF9_SUSCEP_POIS)
			info[i++] = "You are sensitive to poison.";
		if (r_ptr->flags9 & RF9_KILL_TREES)
			info[i++] = "You can eat trees.";
		if (r_ptr->flags9 & RF9_WYRM_PROTECT)
			info[i++] = "You are protected by great wyrms of power.";
	}
	else
	switch (p_ptr->prace)
	{
		case RACE_VAMPIRE:
			if (plev > 1)
			{
				sprintf(Dummy, "You can steal life from a foe, dam. %d-%d (cost %d).",
				    plev+MAX(1, plev/10), plev+plev*MAX(1, plev/10), 1+(plev/3));
				info[i++] = Dummy;
			}
			break;


		default:
			break;
	}

	if (p_ptr->muta1)
	{
		/*if (p_ptr->muta1 & MUT1_SPIT_ACID)
		{
			info[i++] = "You can spit acid (dam lvl).";
		}*/
		
	}

	if (p_ptr->muta2)
	{
		
	}

	if (p_ptr->muta3)
	{
		
	}

        if (p_ptr->allow_one_death)
        {
                info[i++] = "The blood of life flows through your veins.";
        }
	if (p_ptr->blind)
	{
		info[i++] = "You cannot see.";
	}
	if (p_ptr->confused)
	{
		info[i++] = "You are confused.";
	}
	if (p_ptr->afraid)
	{
		info[i++] = "You are terrified.";
	}
	if (p_ptr->cut)
	{
		info[i++] = "You are bleeding.";
	}
	if (p_ptr->stun)
	{
		info[i++] = "You are stunned.";
	}
	if (p_ptr->poisoned)
	{
		info[i++] = "You are poisoned.";
	}
	if (p_ptr->image)
	{
		info[i++] = "You are hallucinating.";
	}
	if (p_ptr->aggravate)
	{
		info[i++] = "You aggravate monsters.";
	}
	if (p_ptr->teleport)
	{
		info[i++] = "Your position is very uncertain.";
	}
	if (p_ptr->blessed)
	{
		info[i++] = "You feel rightous.";
	}
	if (p_ptr->hero)
	{
		info[i++] = "You feel heroic.";
	}
	if (p_ptr->shero)
	{
		info[i++] = "You are in a battle rage.";
	}
	if (p_ptr->shield)
	{
		info[i++] = "You are protected by a mystic shield.";
	}
	if (p_ptr->wraith_form)
	{
		info[i++] = "You are temporarily incorporeal.";
	}
	if (p_ptr->confusing)
	{
		info[i++] = "Your hands are glowing dull red.";
	}
	if (p_ptr->searching)
	{
		info[i++] = "You are looking around very carefully.";
	}
	if (p_ptr->word_recall)
	{
		info[i++] = "You will soon be recalled.";
	}
	if (p_ptr->see_infra)
	{
		info[i++] = "Your eyes are sensitive to infrared light.";
	}
	if (p_ptr->see_inv)
	{
		info[i++] = "You can see invisible creatures.";
	}
	if (p_ptr->ffall)
	{
		info[i++] = "You can fly.";
	}
        if (p_ptr->climb)
	{
                info[i++] = "You can climb hight mountains.";
	}
	if (p_ptr->free_act)
	{
		info[i++] = "You have free action.";
	}
	if (p_ptr->regenerate)
	{
		info[i++] = "You regenerate quickly.";
	}
	if (p_ptr->slow_digest)
	{
		info[i++] = "Your appetite is small.";
	}
	if (p_ptr->telepathy)
	{
		info[i++] = "You have ESP.";
	}
	if (p_ptr->hold_life)
	{
		info[i++] = "You have a firm hold on your life force.";
	}
	if (p_ptr->reflect)
	{
		info[i++] = "You reflect arrows and bolts.";
	}
	if (p_ptr->sh_fire)
	{
		info[i++] = "You are surrounded with a fiery aura.";
	}
	if (p_ptr->sh_elec)
	{
		info[i++] = "You are surrounded with electricity.";
	}
	if (p_ptr->lite)
	{
		info[i++] = "You are carrying a permanent light.";
	}
	if (p_ptr->resist_conf)
	{
		info[i++] = "You are resistant to confusion.";
	}
	if (p_ptr->resist_fear)
	{
		info[i++] = "You are completely fearless.";
	}
	if (p_ptr->resist_blind)
	{
		info[i++] = "Your eyes are resistant to blindness.";
	}

	if (p_ptr->sustain_str)
	{
		info[i++] = "Your strength is sustained.";
	}
	if (p_ptr->sustain_int)
	{
		info[i++] = "Your intelligence is sustained.";
	}
	if (p_ptr->sustain_wis)
	{
		info[i++] = "Your wisdom is sustained.";
	}
	if (p_ptr->sustain_con)
	{
		info[i++] = "Your constitution is sustained.";
	}
	if (p_ptr->sustain_dex)
	{
		info[i++] = "Your dexterity is sustained.";
	}
	if (p_ptr->sustain_chr)
	{
		info[i++] = "Your charisma is sustained.";
	}

	if (f1 & (TR1_STR))
	{
		info[i++] = "Your strength is affected by your equipment.";
	}
	if (f1 & (TR1_INT))
	{
		info[i++] = "Your intelligence is affected by your equipment.";
	}
	if (f1 & (TR1_WIS))
	{
		info[i++] = "Your wisdom is affected by your equipment.";
	}
	if (f1 & (TR1_DEX))
	{
		info[i++] = "Your dexterity is affected by your equipment.";
	}
	if (f1 & (TR1_CON))
	{
		info[i++] = "Your constitution is affected by your equipment.";
	}
	if (f1 & (TR1_CHR))
	{
		info[i++] = "Your charisma is affected by your equipment.";
	}

	if (f1 & (TR1_STEALTH))
	{
		info[i++] = "Your stealth is affected by your equipment.";
	}
	if (f1 & (TR1_INFRA))
	{
		info[i++] = "Your infravision is affected by your equipment.";
	}
	if (f1 & (TR1_SPEED))
	{
		info[i++] = "Your speed is affected by your equipment.";
	}
	if (f1 & (TR1_BLOWS))
	{
		info[i++] = "Your attack speed is affected by your equipment.";
	}


	/* Access the current weapon */
	o_ptr = &inventory[INVEN_WIELD];

	/* Analyze the weapon */
	if (o_ptr->k_idx)
	{
		/* Indicate Blessing */
		if (f3 & (TR3_BLESSED))
		{
			info[i++] = "Your weapon has been blessed by the gods.";
		}

		if (f1 & (TR1_CHAOTIC))
		{
                        info[i++] = "Your weapon is branded with the Sign of Chaos.";
		}

		/* Hack */
		if (f1 & (TR1_IMPACT))
		{
			info[i++] = "The impact of your weapon can cause earthquakes.";
		}

		if (f1 & (TR1_VORPAL))
		{
			info[i++] = "Your weapon is very sharp.";
		}

		if (f1 & (TR1_VAMPIRIC))
		{
			info[i++] = "Your weapon drains life from your foes.";
		}

		/* Special "Attack Bonuses" */
		if (f1 & (TR1_BRAND_ACID))
		{
			info[i++] = "Your weapon melts your foes.";
		}
		if (f1 & (TR1_BRAND_ELEC))
		{
			info[i++] = "Your weapon shocks your foes.";
		}
		if (f1 & (TR1_BRAND_FIRE))
		{
			info[i++] = "Your weapon burns your foes.";
		}
		if (f1 & (TR1_BRAND_COLD))
		{
			info[i++] = "Your weapon freezes your foes.";
		}
		if (f1 & (TR1_BRAND_POIS))
		{
			info[i++] = "Your weapon poisons your foes.";
		}

		/* Special "slay" flags */
		if (f1 & (TR1_SLAY_ANIMAL))
		{
			info[i++] = "Your weapon strikes at animals with extra force.";
		}
		if (f1 & (TR1_SLAY_EVIL))
		{
			info[i++] = "Your weapon strikes at evil with extra force.";
		}
		if (f1 & (TR1_SLAY_UNDEAD))
		{
			info[i++] = "Your weapon strikes at undead with holy wrath.";
		}
		if (f1 & (TR1_SLAY_DEMON))
		{
			info[i++] = "Your weapon strikes at demons with holy wrath.";
		}
		if (f1 & (TR1_SLAY_ORC))
		{
			info[i++] = "Your weapon is especially deadly against orcs.";
		}
		if (f1 & (TR1_SLAY_TROLL))
		{
			info[i++] = "Your weapon is especially deadly against trolls.";
		}
		if (f1 & (TR1_SLAY_GIANT))
		{
			info[i++] = "Your weapon is especially deadly against giants.";
		}
		if (f1 & (TR1_SLAY_DRAGON))
		{
			info[i++] = "Your weapon is especially deadly against dragons.";
		}

		/* Special "kill" flags */
		if (f1 & (TR1_KILL_DRAGON))
		{
			info[i++] = "Your weapon is a great bane of dragons.";
		}
	}

        /* Print on screen or in a file ? */
        if (fff == NULL)
        {
                /* Save the screen */
                Term_save();

                /* Erase the screen */
                for (k = 1; k < 24; k++) prt("", k, 13);

                /* Label the information */
                prt("     Your Attributes:", 1, 15);

                /* We will print on top of the map (column 13) */
                for (k = 2, j = 0; j < i; j++)
                {
                        /* Show the info */
                        prt(info[j], k++, 15);

                        /* Every 20 entries (lines 2 to 21), start over */
                        if ((k == 22) && (j+1 < i))
                        {
                                prt("-- more --", k, 15);
                                inkey();
                                for (; k > 2; k--) prt("", k, 15);
                        }
                }

                /* Pause */
                prt("[Press any key to continue]", k, 13);
                inkey();

                /* Restore the screen */
                Term_load();
        }
        else
        {
                /* Label the information */
                fprintf(fff, "     Your Attributes:\n");

                /* We will print on top of the map (column 13) */
                for (j = 0; j < i; j ++)
                {
                        /* Show the info */
                        fprintf(fff, "%s\n", info[j]);
                }
        }
}


static int report_magics_aux(int dur)
{
    if (dur <= 5)
    {
        return 0;
    }
    else if (dur <= 10)
    {
        return 1;
    }
    else if (dur <= 20)
    {
        return 2;
    }
    else if (dur <= 50)
    {
        return 3;
    }
    else if (dur <= 100)
    {
        return 4;
    }
    else if (dur <= 200)
    {
        return 5;
    }
    else
    {
        return 6;
    }
}

static cptr report_magic_durations[] =
{
	"for a short time",
	"for a little while",
	"for a while",
	"for a long while",
	"for a long time",
	"for a very long time",
	"for an incredibly long time",
	"until you hit a monster"
};


void report_magics(void)
{
	int             i = 0, j, k;
	
	char Dummy[80];
	
	cptr    info[128];
	int     info2[128];
	
	if (p_ptr->blind)
	{
		info2[i]  = report_magics_aux(p_ptr->blind);
		info[i++] = "You cannot see";
	}
	if (p_ptr->confused)
	{
		info2[i]  = report_magics_aux(p_ptr->confused);
		info[i++] = "You are confused";
	}
	if (p_ptr->afraid)
	{
		info2[i]  = report_magics_aux(p_ptr->afraid);
		info[i++] = "You are terrified";
	}
	if (p_ptr->poisoned)
	{
		info2[i]  = report_magics_aux(p_ptr->poisoned);
		info[i++] = "You are poisoned";
	}
	if (p_ptr->image)
	{
		info2[i]  = report_magics_aux(p_ptr->image);
		info[i++] = "You are hallucinating";
	}
	
	if (p_ptr->blessed)
	{
		info2[i]  = report_magics_aux(p_ptr->blessed);
		info[i++] = "You feel rightous";
	}
	if (p_ptr->hero)
	{
		info2[i]  = report_magics_aux(p_ptr->hero);
		info[i++] = "You feel heroic";
	}
	if (p_ptr->shero)
	{
		info2[i]  = report_magics_aux(p_ptr->shero);
		info[i++] = "You are in a battle rage";
	}
	if (p_ptr->shield)
	{
		info2[i]  = report_magics_aux(p_ptr->shield);
		info[i++] = "You are protected by a mystic shield";
	}
	if (p_ptr->wraith_form)
	{
		info2[i]  = report_magics_aux(p_ptr->wraith_form);
		info[i++] = "You are incorporeal";
	}
	if (p_ptr->confusing)
	{
		info2[i]  = 7;
		info[i++] = "Your hands are glowing dull red.";
	}
	if (p_ptr->word_recall)
	{
		info2[i]  = report_magics_aux(p_ptr->word_recall);
		info[i++] = "You waiting to be recalled";
	}

	/* Save the screen */
	Term_save();
	
	/* Erase the screen */
	for (k = 1; k < 24; k++) prt("", k, 13);
	
	/* Label the information */
	prt("     Your Current Magic:", 1, 15);
	
	/* We will print on top of the map (column 13) */
	for (k = 2, j = 0; j < i; j++)
	{
		/* Show the info */
		sprintf( Dummy, "%s %s.", info[j],
			report_magic_durations[info2[j]] );
		prt(Dummy, k++, 15);
		
		/* Every 20 entries (lines 2 to 21), start over */
		if ((k == 22) && (j + 1 < i))
		{
			prt("-- more --", k, 15);
			inkey();
			for (; k > 2; k--) prt("", k, 15);
		}
	}
	
	/* Pause */
	prt("[Press any key to continue]", k, 13);
	inkey();

	/* Restore the screen */
	Term_load();
}



/*
 * Forget everything
 */
bool lose_all_info(void)
{
	int i;

	/* Forget info about objects */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Allow "protection" by the MENTAL flag */
		if (o_ptr->ident & (IDENT_MENTAL)) continue;

		/* Remove "default inscriptions" */
		if (o_ptr->note && (o_ptr->ident & (IDENT_SENSE)))
		{
			/* Access the inscription */
			cptr q = quark_str(o_ptr->note);

			/* Hack -- Remove auto-inscriptions */
			if ((streq(q, "cursed")) ||
			    (streq(q, "broken")) ||
			    (streq(q, "good")) ||
			    (streq(q, "average")) ||
			    (streq(q, "excellent")) ||
			    (streq(q, "worthless")) ||
			    (streq(q, "special")) ||
			    (streq(q, "terrible")))
			{
				/* Forget the inscription */
				o_ptr->note = 0;
			}
		}

		/* Hack -- Clear the "empty" flag */
		o_ptr->ident &= ~(IDENT_EMPTY);

		/* Hack -- Clear the "known" flag */
		o_ptr->ident &= ~(IDENT_KNOWN);

		/* Hack -- Clear the "felt" flag */
		o_ptr->ident &= ~(IDENT_SENSE);
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Mega-Hack -- Forget the map */
	wiz_dark();

	/* It worked */
	return (TRUE);
}




/*
 * Detect all traps on current panel
 */
bool detect_traps(void)
{
	int             x, y;
	bool            detect = FALSE;
	cave_type       *c_ptr;


	/* Scan the current panel */
	for (y = panel_row_min; y <= panel_row_max; y++)
	{
		for (x = panel_col_min; x <= panel_col_max; x++)
		{
			/* Access the grid */
			c_ptr = &cave[y][x];

			/* Detect invisible traps */
			if (c_ptr->t_idx != 0)
			{
				/* Pick a trap */
				pick_trap(y, x);

				/* Hack -- Memorize */
				c_ptr->info |= (CAVE_MARK);

				/* Redraw */
				lite_spot(y, x);

				/* Obvious */
				detect = TRUE;
			}
		}
	}

	/* Describe */
	if (detect)
	{
		msg_print("You sense the presence of traps!");
	}

	/* Result */
	return (detect);
}



/*
 * Detect all doors on current panel
 */
bool detect_doors(void)
{
	int y, x;

	bool detect = FALSE;

	cave_type *c_ptr;


	/* Scan the panel */
	for (y = panel_row_min; y <= panel_row_max; y++)
	{
		for (x = panel_col_min; x <= panel_col_max; x++)
		{
			c_ptr = &cave[y][x];

			/* Detect secret doors */
			if (c_ptr->feat == FEAT_SECRET)
			{
				/* Pick a door XXX XXX XXX */
				cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
			}

			/* Detect doors */
			if (((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			     (c_ptr->feat <= FEAT_DOOR_HEAD)) ||
			    ((c_ptr->feat == FEAT_OPEN) ||
			     (c_ptr->feat == FEAT_BROKEN)))
			{
				/* Hack -- Memorize */
				c_ptr->info |= (CAVE_MARK);

				/* Redraw */
				lite_spot(y, x);

				/* Obvious */
				detect = TRUE;
			}
		}
	}

	/* Describe */
	if (detect)
	{
		msg_print("You sense the presence of doors!");
	}

	/* Result */
	return (detect);
}


/*
 * Detect all stairs on current panel
 */
bool detect_stairs(void)
{
	int y, x;

	bool detect = FALSE;

	cave_type *c_ptr;


	/* Scan the panel */
	for (y = panel_row_min; y <= panel_row_max; y++)
	{
		for (x = panel_col_min; x <= panel_col_max; x++)
		{
			c_ptr = &cave[y][x];

			/* Detect stairs */
			if ((c_ptr->feat == FEAT_LESS) ||
			    (c_ptr->feat == FEAT_MORE))
			{
				/* Hack -- Memorize */
				c_ptr->info |= (CAVE_MARK);

				/* Redraw */
				lite_spot(y, x);

				/* Obvious */
				detect = TRUE;
			}
		}
	}

	/* Describe */
	if (detect)
	{
		msg_print("You sense the presence of stairs!");
	}

	/* Result */
	return (detect);
}


/*
 * Detect any treasure on the current panel
 */
bool detect_treasure(void)
{
	int y, x;

	bool detect = FALSE;

	cave_type *c_ptr;


	/* Scan the current panel */
	for (y = panel_row_min; y <= panel_row_max; y++)
	{
		for (x = panel_col_min; x <= panel_col_max; x++)
		{
			c_ptr = &cave[y][x];

			/* Notice embedded gold */
			if ((c_ptr->feat == FEAT_MAGMA_H) ||
			    (c_ptr->feat == FEAT_QUARTZ_H))
			{
				/* Expose the gold */
				c_ptr->feat += 0x02;
			}

			/* Magma/Quartz + Known Gold */
			if ((c_ptr->feat == FEAT_MAGMA_K) ||
			    (c_ptr->feat == FEAT_QUARTZ_K))
			{
				/* Hack -- Memorize */
				c_ptr->info |= (CAVE_MARK);

				/* Redraw */
				lite_spot(y, x);

				/* Detect */
				detect = TRUE;
			}
		}
	}

	/* Describe */
	if (detect)
	{
		msg_print("You sense the presence of buried treasure!");
	}



	/* Result */
	return (detect);
}



/*
 * Detect all "gold" objects on the current panel
 */
bool detect_objects_gold(void)
{
	int i, y, x;

	bool detect = FALSE;


	/* Scan objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (!panel_contains(y, x)) continue;
		
		/* Detect "gold" objects */
		if (o_ptr->tval == TV_GOLD)
		{
			/* Hack -- memorize it */
			o_ptr->marked = TRUE;

			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			detect = TRUE;
		}
	}

	/* Describe */
	if (detect)
	{
		msg_print("You sense the presence of treasure!");
	}

	if (detect_monsters_string("$*"))
	{
		detect = TRUE;
	}

	/* Result */
	return (detect);
}


/*
 * Detect all "normal" objects on the current panel
 */
bool detect_objects_normal(void)
{
	int i, y, x;

	bool detect = FALSE;


	/* Scan objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (!panel_contains(y, x)) continue;
		
		/* Detect "real" objects */
		if (o_ptr->tval != TV_GOLD)
		{
			/* Hack -- memorize it */
			o_ptr->marked = TRUE;

			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			detect = TRUE;
		}
	}

	/* Describe */
	if (detect)
	{
		msg_print("You sense the presence of objects!");
	}

	if (detect_monsters_string("!=?|"))
	{
		detect = TRUE;
	}

	/* Result */
	return (detect);
}


/*
 * Detect all "magic" objects on the current panel.
 *
 * This will light up all spaces with "magic" items, including artifacts,
 * ego-items, potions, scrolls, books, rods, wands, staves, amulets, rings,
 * and "enchanted" items of the "good" variety.
 *
 * It can probably be argued that this function is now too powerful.
 */
bool detect_objects_magic(void)
{
	int i, y, x, tv;

	bool detect = FALSE;


	/* Scan all objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];
		
		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		/* Location */
		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (!panel_contains(y,x)) continue;
		
		/* Examine the tval */
		tv = o_ptr->tval;

		/* Artifacts, misc magic items, or enchanted wearables */
		if (artifact_p(o_ptr) || ego_item_p(o_ptr) || o_ptr->art_name ||
                    (tv == TV_AMULET) || (tv == TV_RING) || (tv == TV_BATERIE) ||
		    (tv == TV_STAFF) || (tv == TV_WAND) || (tv == TV_ROD) ||
                    (tv == TV_SCROLL) || (tv == TV_POTION) || (tv == TV_POTION2) ||
                    (tv == TV_VALARIN_BOOK) || (tv == TV_MAGERY_BOOK) ||
                    (tv == TV_SHADOW_BOOK) || (tv == TV_CHAOS_BOOK) ||
                    (tv == TV_NETHER_BOOK) || (tv == TV_MIMIC_BOOK) ||
                    (tv == TV_CRUSADE_BOOK) || (tv == TV_SIGALDRY_BOOK) ||
                    (tv == TV_SYMBIOTIC_BOOK) || (tv == TV_MUSIC_BOOK) ||
		    ((o_ptr->to_a > 0) || (o_ptr->to_h + o_ptr->to_d > 0)))
		{
			/* Memorize the item */
			o_ptr->marked = TRUE;

			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			detect = TRUE;
		}
	}

	/* Describe */
	if (detect)
	{
		msg_print("You sense the presence of magic objects!");
	}

	/* Return result */
	return (detect);
}


/*
 * Detect all "normal" monsters on the current panel
 */
bool detect_monsters_normal(void)
{
	int i, y, x;

	bool flag = FALSE;


	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (!panel_contains(y, x)) continue;
		
		/* Detect all non-invisible monsters */
		if ((!(r_ptr->flags2 & (RF2_INVISIBLE))) ||
		    p_ptr->see_inv || p_ptr->tim_invis)
		{
			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Hack -- See monster */
			m_ptr->ml = TRUE;
			
			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			flag = TRUE;
		}
	}

	/* Describe */
	if (flag)
	{
		/* Describe result */
		msg_print("You sense the presence of monsters!");
	}

	/* Result */
	return (flag);
}


/*
 * Detect all "invisible" monsters on current panel
 */
bool detect_monsters_invis(void)
{
	int i, y, x;
	bool flag = FALSE;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (!panel_contains(y, x)) continue;
		
		/* Detect invisible monsters */
		if (r_ptr->flags2 & (RF2_INVISIBLE))
		{
			/* Take note that they are invisible */
			r_ptr->r_flags2 |= (RF2_INVISIBLE);

			/* Update monster recall window */
			if (monster_race_idx == m_ptr->r_idx)
			{
				/* Window stuff */
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Hack -- See monster */
			m_ptr->ml = TRUE;
			
			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			flag = TRUE;
		}
	}

	/* Describe */
	if (flag)
	{
		/* Describe result */
		msg_print("You sense the presence of invisible creatures!");
	}

	/* Result */
	return (flag);
}



/*
 * Detect all "evil" monsters on current panel
 */
bool detect_monsters_evil(void)
{
	int i, y, x;
	bool flag = FALSE;


	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (!panel_contains(y, x)) continue;
		
		/* Detect evil monsters */
		if (r_ptr->flags3 & (RF3_EVIL))
		{
			/* Take note that they are evil */
			r_ptr->r_flags3 |= (RF3_EVIL);

			/* Update monster recall window */
			if (monster_race_idx == m_ptr->r_idx)
			{
				/* Window stuff */
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Hack -- See monster */
			m_ptr->ml = TRUE;
			
			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			flag = TRUE;
		}
	}

	/* Describe */
	if (flag)
	{
		/* Describe result */
		msg_print("You sense the presence of evil creatures!");
	}

	/* Result */
	return (flag);
}




/*
 * Detect all (string) monsters on current panel
 */
bool detect_monsters_string(cptr Match)
{
	int i, y, x;
	bool flag = FALSE;


	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (!panel_contains(y, x)) continue;
		
		/* Detect evil monsters */
		if (strchr(Match, r_ptr->d_char))
		{

			/* Update monster recall window */
			if (monster_race_idx == m_ptr->r_idx)
			{
				/* Window stuff */
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Hack -- See monster */
			m_ptr->ml = TRUE;
			
			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			flag = TRUE;
		}
	}

	/* Describe */
	if (flag)
	{
		/* Describe result */
		msg_print("You sense the presence of monsters!");
	}

	/* Result */
	return (flag);
}


/*
 * A "generic" detect monsters routine, tagged to flags3
 */
bool detect_monsters_xxx(u32b match_flag)
{
	int  i, y, x;
	bool flag = FALSE;
	cptr desc_monsters = "weird monsters";


	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (!panel_contains(y, x)) continue;
		
		/* Detect evil monsters */
		if (r_ptr->flags3 & (match_flag))
		{
			/* Take note that they are something */
			r_ptr->r_flags3 |= (match_flag);

			/* Update monster recall window */
			if (monster_race_idx == m_ptr->r_idx)
			{
				/* Window stuff */
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Hack -- See monster */
			m_ptr->ml = TRUE;
			
			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			flag = TRUE;
		}
	}

	/* Describe */
	if (flag)
	{
		switch (match_flag)
		{
			case RF3_DEMON:
				desc_monsters = "demons";
				break;
			case RF3_UNDEAD:
				desc_monsters = "the undead";
				break;
		}

		/* Describe result */
		msg_format("You sense the presence of %s!", desc_monsters);
		msg_print(NULL);
	}

	/* Result */
	return (flag);
}


/*
 * Detect everything
 */
bool detect_all(void)
{
	bool detect = FALSE;

	/* Detect everything */
	if (detect_traps()) detect = TRUE;
	if (detect_doors()) detect = TRUE;
	if (detect_stairs()) detect = TRUE;
	if (detect_treasure()) detect = TRUE;
	if (detect_objects_gold()) detect = TRUE;
	if (detect_objects_normal()) detect = TRUE;
	if (detect_monsters_invis()) detect = TRUE;
	if (detect_monsters_normal()) detect = TRUE;
	
	/* Result */
	return (detect);
}



/*
 * Create stairs at the player location
 */
void stair_creation(void)
{
	/* XXX XXX XXX */
	if (!cave_valid_bold(py, px))
	{
		msg_print("The object resists the spell.");
		return;
	}
	if (special_flag)
	{
		msg_print("No stair creation on special levels...");
		return;
	}

	/* XXX XXX XXX */
	delete_object(py, px);

	/* Create a staircase */
	if (p_ptr->inside_quest)
	{
		/* arena or quest */
		msg_print("There is no effect!");
	}
	else if (!dun_level)
	{
		/* Town/wilderness */
		cave_set_feat(py, px, FEAT_MORE);
	}
	else if (is_quest(dun_level) || (dun_level >= MAX_DEPTH-1))
	{
		/* Quest level */
		cave_set_feat(py, px, FEAT_LESS);
	}
	else if (rand_int(100) < 50)
	{
		cave_set_feat(py, px, FEAT_MORE);
	}
	else
	{
		cave_set_feat(py, px, FEAT_LESS);
	}
}




/*
 * Hook to specify "weapon"
 */
static bool item_tester_hook_weapon(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
                case TV_MSTAFF:
                case TV_BOOMERANG:
                case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
                case TV_DAGGER:
                case TV_AXE:
		case TV_DIGGING:
		case TV_BOW:
		case TV_BOLT:
		case TV_ARROW:
		case TV_SHOT:
                case TV_ROD:
                case TV_SWORD_DEVASTATION:
                case TV_HELL_STAFF:
                case TV_VALKYRIE_SPEAR:
                case TV_RING:
                case TV_ZELAR_WEAPON:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Hook to specify "armour"
 */
bool item_tester_hook_armour(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_CROWN:
		case TV_HELM:
		case TV_BOOTS:
		case TV_GLOVES:
                case TV_AMULET:
                case TV_ARM_BAND:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Check if an object is weapon or armour (but not arrow, bolt, or shot)
 */
bool item_tester_hook_weapon_armour(object_type *o_ptr)
{
	return(item_tester_hook_weapon(o_ptr) ||
	       item_tester_hook_armour(o_ptr));
}

/*
 * Check if an object is artifactable
 */
bool item_tester_hook_artifactable(object_type *o_ptr)
{
	return(item_tester_hook_weapon(o_ptr) ||
               item_tester_hook_armour(o_ptr) ||
               (o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET));
}


/*
 * Enchants a plus onto an item. -RAK-
 *
 * Revamped!  Now takes item pointer, number of times to try enchanting,
 * and a flag of what to try enchanting.  Artifacts resist enchantment
 * some of the time, and successful enchantment to at least +0 might
 * break a curse on the item. -CFT-
 *
 * Note that an item can technically be enchanted all the way to +15 if
 * you wait a very, very, long time.  Going from +9 to +10 only works
 * about 5% of the time, and from +10 to +11 only about 1% of the time.
 *
 * Note that this function can now be used on "piles" of items, and
 * the larger the pile, the lower the chance of success.
 */
bool enchant(object_type *o_ptr, int n, int eflag)
{
	int     i, chance, prob;
	bool    res = FALSE;
	bool    a = (artifact_p(o_ptr) || o_ptr->art_name);
        u32b    f1, f2, f3, f4;


	/* Extract the flags */
        object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Large piles resist enchantment */
	prob = o_ptr->number * 100;

	/* Missiles are easy to enchant */
	if ((o_ptr->tval == TV_BOLT) ||
	    (o_ptr->tval == TV_ARROW) ||
	    (o_ptr->tval == TV_SHOT))
	{
		prob = prob / 20;
	}

	/* Try "n" times */
	for (i=0; i<n; i++)
	{
		/* Hack -- Roll for pile resistance */
		if (rand_int(prob) >= 100) continue;

		/* Enchant to hit */
		if (eflag & (ENCH_TOHIT))
		{
			if (o_ptr->to_h < 0) chance = 0;
			else if (o_ptr->to_h > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_h];

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				o_ptr->to_h++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & (TR3_PERMA_CURSE))) &&
				    (o_ptr->to_h >= 0) && (rand_int(100) < 25))
				{
					msg_print("The curse is broken!");
					o_ptr->ident &= ~(IDENT_CURSED);
					o_ptr->ident |= (IDENT_SENSE);

					if (o_ptr->art_flags3 & (TR3_CURSED))
					    o_ptr->art_flags3 &= ~(TR3_CURSED);
					if (o_ptr->art_flags3 & (TR3_HEAVY_CURSE))
					    o_ptr->art_flags3 &= ~(TR3_HEAVY_CURSE);

					o_ptr->note = quark_add("uncursed");
				}
			}
		}

		/* Enchant to damage */
		if (eflag & (ENCH_TODAM))
		{
			if (o_ptr->to_d < 0) chance = 0;
			else if (o_ptr->to_d > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_d];

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				o_ptr->to_d++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & (TR3_PERMA_CURSE))) &&
				    (o_ptr->to_d >= 0) && (rand_int(100) < 25))
				{
					msg_print("The curse is broken!");
					o_ptr->ident &= ~(IDENT_CURSED);
					o_ptr->ident |= (IDENT_SENSE);

					if (o_ptr->art_flags3 & (TR3_CURSED))
					    o_ptr->art_flags3 &= ~(TR3_CURSED);
					if (o_ptr->art_flags3 & (TR3_HEAVY_CURSE))
					    o_ptr->art_flags3 &= ~(TR3_HEAVY_CURSE);

					o_ptr->note = quark_add("uncursed");
				}
			}
		}


		/* Enchant to damage */
                if (eflag & (ENCH_PVAL))
		{
                        if (o_ptr->pval < 0) chance = 0;
                        else if (o_ptr->pval > 8) chance = 1000;
                        else chance = enchant_table[o_ptr->pval];

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
                                o_ptr->pval++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & (TR3_PERMA_CURSE))) &&
                                    (o_ptr->pval >= 0) && (rand_int(100) < 25))
				{
					msg_print("The curse is broken!");
					o_ptr->ident &= ~(IDENT_CURSED);
					o_ptr->ident |= (IDENT_SENSE);

					if (o_ptr->art_flags3 & (TR3_CURSED))
					    o_ptr->art_flags3 &= ~(TR3_CURSED);
					if (o_ptr->art_flags3 & (TR3_HEAVY_CURSE))
					    o_ptr->art_flags3 &= ~(TR3_HEAVY_CURSE);

					o_ptr->note = quark_add("uncursed");
				}
			}
		}

		/* Enchant to armor class */
		if (eflag & (ENCH_TOAC))
		{
			if (o_ptr->to_a < 0) chance = 0;
			else if (o_ptr->to_a > 15) chance = 1000;
			else chance = enchant_table[o_ptr->to_a];

			if ((randint(1000) > chance) && (!a || (rand_int(100) < 50)))
			{
				o_ptr->to_a++;
				res = TRUE;

				/* only when you get it above -1 -CFT */
				if (cursed_p(o_ptr) &&
				    (!(f3 & (TR3_PERMA_CURSE))) &&
				    (o_ptr->to_a >= 0) && (rand_int(100) < 25))
				{
					msg_print("The curse is broken!");
					o_ptr->ident &= ~(IDENT_CURSED);
					o_ptr->ident |= (IDENT_SENSE);

					if (o_ptr->art_flags3 & (TR3_CURSED))
					    o_ptr->art_flags3 &= ~(TR3_CURSED);
					if (o_ptr->art_flags3 & (TR3_HEAVY_CURSE))
					    o_ptr->art_flags3 &= ~(TR3_HEAVY_CURSE);

					o_ptr->note = quark_add("uncursed");
				}
			}
		}
	}

	/* Failure */
	if (!res) return (FALSE);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Success */
	return (TRUE);
}



/*
 * Enchant an item (in the inventory or on the floor)
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
bool enchant_spell(int num_hit, int num_dam, int num_ac, int num_pval)
{
	int         item;
	bool        okay = FALSE;
	object_type *o_ptr;
	char        o_name[80];
	cptr        q, s;


	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon;

	/* Enchant armor if requested */
	if (num_ac) item_tester_hook = item_tester_hook_armour;

	/* Get an item */
	q = "Enchant which item? ";
	s = "You have nothing to enchant.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Describe */
	msg_format("%s %s glow%s brightly!",
		   ((item >= 0) ? "Your" : "The"), o_name,
		   ((o_ptr->number > 1) ? "" : "s"));

	/* Enchant */
	if (enchant(o_ptr, num_hit, ENCH_TOHIT)) okay = TRUE;
	if (enchant(o_ptr, num_dam, ENCH_TODAM)) okay = TRUE;
        if (enchant(o_ptr, num_ac, ENCH_TOAC)) okay = TRUE;
        if (enchant(o_ptr, num_pval, ENCH_PVAL)) okay = TRUE;

	/* Failure */
	if (!okay)
	{
		/* Flush */
		if (flush_failure) flush();

		/* Message */
		msg_print("The enchantment failed.");
	}

	/* Something happened */
	return (TRUE);
}

void curse_artifact(object_type * o_ptr)
{
	if (o_ptr->pval) o_ptr->pval = 0 - ((o_ptr->pval) + randint(4));
	if (o_ptr->to_a) o_ptr->to_a = 0 - ((o_ptr->to_a) + randint(4));
	if (o_ptr->to_h) o_ptr->to_h = 0 - ((o_ptr->to_h) + randint(4));
	if (o_ptr->to_d) o_ptr->to_d = 0 - ((o_ptr->to_d) + randint(4));
	o_ptr->art_flags3 |= ( TR3_HEAVY_CURSE | TR3_CURSED );
	if (randint(4)==1) o_ptr-> art_flags3 |= TR3_PERMA_CURSE;
	if (randint(3)==1) o_ptr-> art_flags3 |= TR3_TY_CURSE;
	if (randint(2)==1) o_ptr-> art_flags3 |= TR3_AGGRAVATE;
	if (randint(3)==1) o_ptr-> art_flags3 |= TR3_DRAIN_EXP;
	if (randint(2)==1) o_ptr-> art_flags3 |= TR3_TELEPORT;
	else if (randint(3)==1) o_ptr->art_flags3 |= TR3_NO_TELE;
	if (p_ptr->pclass != CLASS_WARRIOR && (randint(3)==1))
		o_ptr->art_flags3 |= TR3_NO_MAGIC;
	o_ptr->ident |= IDENT_CURSED;
}


void random_plus(object_type * o_ptr, bool is_scroll)
{
	int this_type = (o_ptr->tval<TV_BOOTS?23:19);

	switch (randint(23))
	{
	case 1: case 2:
		o_ptr->art_flags1 |= TR1_STR;
		/*  if (is_scroll) msg_print("It makes you feel strong!"); */
		if (!(artifact_bias) && randint(13)!=1)
			artifact_bias = BIAS_STR;
		else if (!(artifact_bias) && randint(7)==1)
			artifact_bias = BIAS_WARRIOR;
		break;
	case 3: case 4:
		o_ptr->art_flags1 |= TR1_INT;
		/*  if (is_scroll) msg_print("It makes you feel smart!"); */
		if (!(artifact_bias) && randint(13)!=1)
			artifact_bias = BIAS_INT;
		else if (!(artifact_bias) && randint(7)==1)
			artifact_bias = BIAS_MAGE;
		break;
	case 5: case 6:
		o_ptr->art_flags1 |= TR1_WIS;
		/*  if (is_scroll) msg_print("It makes you feel wise!"); */
		if (!(artifact_bias) && randint(13)!=1)
			artifact_bias = BIAS_WIS;
		else if (!(artifact_bias) && randint(7)==1)
			artifact_bias = BIAS_PRIESTLY;
		break;
	case 7: case 8:
		o_ptr->art_flags1 |= TR1_DEX;
		/*  if (is_scroll) msg_print("It makes you feel nimble!"); */
		if (!(artifact_bias) && randint(13)!=1)
			artifact_bias = BIAS_DEX;
		else if (!(artifact_bias) && randint(7)==1)
			artifact_bias = BIAS_ROGUE;
		break;
	case 9: case 10:
		o_ptr->art_flags1 |= TR1_CON;
		/*  if (is_scroll) msg_print("It makes you feel healthy!"); */
		if (!(artifact_bias) && randint(13)!=1)
			artifact_bias = BIAS_CON;
		else if (!(artifact_bias) && randint(9)==1)
			artifact_bias = BIAS_RANGER;
		break;
	case 11: case 12:
		o_ptr->art_flags1 |= TR1_CHR;
		/*  if (is_scroll) msg_print("It makes you look great!"); */
		if (!(artifact_bias) && randint(13)!=1)
			artifact_bias = BIAS_CHR;
		break;
	case 13: case 14:
		o_ptr->art_flags1 |= TR1_STEALTH;
		/*  if (is_scroll) msg_print("It looks muffled."); */
		if (!(artifact_bias) && randint(3)==1)
			artifact_bias = BIAS_ROGUE;
		break;
        case 15: case 16: case 17: case 18:
		o_ptr->art_flags1 |= TR1_INFRA;
		/*  if (is_scroll) msg_print("It makes you see tiny red animals.");*/
		break;
        case 19: case 20: case 21:
		o_ptr->art_flags1 |= TR1_SPEED;
		/*  if (is_scroll) msg_print("It makes you move faster!"); */
		if (!(artifact_bias) && randint(11)==1)
			artifact_bias = BIAS_ROGUE;
		break;
	case 22: case 23:
		if (o_ptr->tval == TV_BOW) random_plus(o_ptr, is_scroll);
		else
		{
			o_ptr->art_flags1 |= TR1_BLOWS;
			/*  if (is_scroll) msg_print("It seems faster!"); */
			if (!(artifact_bias) && randint(11)==1)
				artifact_bias = BIAS_WARRIOR;
		}
		break;
	}
}

/* Simplified for NewAngband 1.8.0. */
void random_resistance (object_type * o_ptr)
{
	int whichres = randint(19);
	int resamount;

	resamount = o_ptr->pval * 2;

	switch (whichres)
	{
		case 1:
			o_ptr->fireres += resamount;
			if (o_ptr->fireres > 100) o_ptr->fireres = 100;
			break;
		case 2:
			o_ptr->coldres += resamount;
			if (o_ptr->coldres > 100) o_ptr->coldres = 100;
			break;
		case 3:
			o_ptr->elecres += resamount;
			if (o_ptr->elecres > 100) o_ptr->elecres = 100;
			break;
		case 4:
			o_ptr->acidres += resamount;
			if (o_ptr->acidres > 100) o_ptr->acidres = 100;
			break;
		case 5:
			o_ptr->poisres += resamount;
			if (o_ptr->poisres > 100) o_ptr->poisres = 100;
			break;
		case 6:
			o_ptr->lightres += resamount;
			if (o_ptr->lightres > 100) o_ptr->lightres = 100;
			break;
		case 7:
			o_ptr->darkres += resamount;
			if (o_ptr->darkres > 100) o_ptr->darkres = 100;
			break;
		case 8:
			o_ptr->warpres += resamount;
			if (o_ptr->warpres > 100) o_ptr->warpres = 100;
			break;
		case 9:
			o_ptr->waterres += resamount;
			if (o_ptr->waterres > 100) o_ptr->waterres = 100;
			break;
		case 10:
			o_ptr->windres += resamount;
			if (o_ptr->windres > 100) o_ptr->windres = 100;
			break;
		case 11:
			o_ptr->earthres += resamount;
			if (o_ptr->earthres > 100) o_ptr->earthres = 100;
			break;
		case 12:
			o_ptr->soundres += resamount;
			if (o_ptr->soundres > 100) o_ptr->soundres = 100;
			break;
		case 13:
			o_ptr->chaosres += resamount;
			if (o_ptr->chaosres > 100) o_ptr->chaosres = 100;
			break;
		case 14:
			o_ptr->radiores += resamount;
			if (o_ptr->radiores > 100) o_ptr->radiores = 100;
			break;
		case 15:
			o_ptr->physres += resamount;
			if (o_ptr->physres > 100) o_ptr->physres = 100;
			break;
		case 16:
			o_ptr->art_flags2 |= TR2_RES_FEAR;
			break;
		case 17:
			o_ptr->art_flags2 |= TR2_RES_BLIND;
			break;
		case 18:
			o_ptr->art_flags2 |= TR2_RES_CONF;
			break;
		case 19:
			o_ptr->art_flags4 |= TR4_SAFETY;
			break;
	}
}

void random_misc (object_type * o_ptr, bool is_scroll)
{

    if (artifact_bias == BIAS_RANGER)
    {
	if (!(o_ptr->art_flags2 & TR2_SUST_CON))
	{
	    o_ptr->art_flags2 |= TR2_SUST_CON;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_STR)
    {
	if (!(o_ptr->art_flags2 & TR2_SUST_STR))
	{
	    o_ptr->art_flags2 |= TR2_SUST_STR;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_WIS)
    {
	if (!(o_ptr->art_flags2 & TR2_SUST_WIS))
	{
	    o_ptr->art_flags2 |= TR2_SUST_WIS;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_INT)
    {
	if (!(o_ptr->art_flags2 & TR2_SUST_INT))
	{
	    o_ptr->art_flags2 |= TR2_SUST_INT;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_DEX)
    {
	if (!(o_ptr->art_flags2 & TR2_SUST_DEX))
	{
	    o_ptr->art_flags2 |= TR2_SUST_DEX;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_CON)
    {
	if (!(o_ptr->art_flags2 & TR2_SUST_CON))
	{
	    o_ptr->art_flags2 |= TR2_SUST_CON;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_CHR)
    {
	if (!(o_ptr->art_flags2 & TR2_SUST_CHR))
	{
	    o_ptr->art_flags2 |= TR2_SUST_CHR;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_CHAOS)
    {
	if (!(o_ptr->art_flags3 & TR3_TELEPORT))
	{
	    o_ptr->art_flags3 |= TR3_TELEPORT;
	    if (randint(2)==1) return;
	}
    }
    else if (artifact_bias == BIAS_FIRE)
    {
	if (!(o_ptr->art_flags3 & TR3_LITE))
	{
	    o_ptr->art_flags3 |= TR3_LITE; /* Freebie */
	}
    }


    switch (randint(29))
    {
    case 1:
	o_ptr->art_flags2 |= TR2_SUST_STR;
/*  if (is_scroll) msg_print("It makes you feel you cannot become weaker."); */
	if (!artifact_bias)
	    artifact_bias = BIAS_STR;
	break;
    case 2:
	o_ptr->art_flags2 |= TR2_SUST_INT;
/*  if (is_scroll) msg_print("It makes you feel you cannot become more stupid.");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_INT;
	break;
    case 3:
	o_ptr->art_flags2 |= TR2_SUST_WIS;
/*  if (is_scroll) msg_print("It makes you feel you cannot become simpler.");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_WIS;
	break;
    case 4:
	o_ptr->art_flags2 |= TR2_SUST_DEX;
/*  if (is_scroll) msg_print("It makes you feel you cannot become clumsier.");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_DEX;
	break;
    case 5:
	o_ptr->art_flags2 |= TR2_SUST_CON;
/*  if (is_scroll) msg_print("It makes you feel you cannot become less healthy.");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_CON;
	break;
    case 6:
	o_ptr->art_flags2 |= TR2_SUST_CHR;
/*  if (is_scroll) msg_print("It makes you feel you cannot become uglier.");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_CHR;
	break;
    case 7: case 8: case 14:
	o_ptr->art_flags2 |= TR2_FREE_ACT;
/*  if (is_scroll) msg_print("It makes you feel like a young rebel!");*/
	break;
    case 9:
	o_ptr->art_flags2 |= TR2_HOLD_LIFE;
/*  if (is_scroll) msg_print("It makes you feel immortal.");*/
	if (!artifact_bias && (randint(5)==1))
	    artifact_bias = BIAS_PRIESTLY;
	else if (!artifact_bias && (randint(6)==1))
	    artifact_bias = BIAS_NECROMANTIC;
	break;
    case 10: case 11:
	o_ptr->art_flags3 |= TR3_LITE;
/*  if (is_scroll) msg_print("It starts shining.");*/
	break;
    case 12: case 13:
	o_ptr->art_flags3 |= TR3_FEATHER;
/*  if (is_scroll) msg_print("It feels lighter.");*/
	break;
    case 15: case 16: case 17:
	o_ptr->art_flags3 |= TR3_SEE_INVIS;
/*  if (is_scroll) msg_print("It makes you see the air!");*/
	break;
    case 18:
	o_ptr->art_flags3 |= TR3_TELEPATHY;
/*  if (is_scroll) msg_print("It makes you hear voices inside your head!");*/
	if (!artifact_bias && (randint(9)==1))
	    artifact_bias = BIAS_MAGE;
	break;
    case 19: case 20:
	o_ptr->art_flags3 |= TR3_SLOW_DIGEST;
/*  if (is_scroll) msg_print("It makes you feel less hungry.");*/
	break;
    case 21: case 22:
	o_ptr->art_flags3 |= TR3_REGEN;
/*  if (is_scroll) msg_print("It looks as good as new.");*/
	break;
    case 23:
	o_ptr->art_flags3 |= TR3_TELEPORT;
/*  if (is_scroll) msg_print("Its position feels uncertain!");*/
	break;
    case 24: case 25: case 26:
	if (o_ptr->tval>=TV_BOOTS) random_misc(o_ptr, is_scroll);
	else
	{
		o_ptr->art_flags3 |= TR3_SHOW_MODS;
		o_ptr->to_a = 4 + (randint(11));
	}
	break;
    case 27: case 28: case 29:
	o_ptr->art_flags3 |= TR3_SHOW_MODS;
	o_ptr->to_h += 4 + (randint(11));
	o_ptr->to_d += 4 + (randint(11));
	break;
    }


}


void random_slay (object_type * o_ptr, bool is_scroll)
{
	if (artifact_bias == BIAS_CHAOS && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_CHAOTIC))
		{
			o_ptr->art_flags1 |= TR1_CHAOTIC;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_PRIESTLY &&
	   (o_ptr->tval == TV_SWORD || o_ptr->tval == TV_POLEARM) &&
	  !(o_ptr->art_flags3 & TR3_BLESSED))
	{
		/* A free power for "priestly" random artifacts */
		o_ptr->art_flags3 |= TR3_BLESSED;
	}

	else if (artifact_bias == BIAS_NECROMANTIC && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_VAMPIRIC))
		{
			o_ptr->art_flags1 |= TR1_VAMPIRIC;
			if (randint(2)==1) return;
		}
		if (!(o_ptr->art_flags1 & TR1_BRAND_POIS) && (randint(2)==1))
		{
			o_ptr->art_flags1 |= TR1_BRAND_POIS;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_RANGER && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_SLAY_ANIMAL))
		{
			o_ptr->art_flags1 |= TR1_SLAY_ANIMAL;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_ROGUE && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_POIS))
		{
			o_ptr->art_flags1 |= TR1_BRAND_POIS;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_POIS && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_POIS))
		{
			o_ptr->art_flags1 |= TR1_BRAND_POIS;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_FIRE && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_FIRE))
		{
			o_ptr->art_flags1 |= TR1_BRAND_FIRE;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_COLD && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_COLD))
		{
			o_ptr->art_flags1 |= TR1_BRAND_COLD;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_ELEC && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_ELEC))
		{
			o_ptr->art_flags1 |= TR1_BRAND_ELEC;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_ACID && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_ACID))
		{
			o_ptr->art_flags1 |= TR1_BRAND_ACID;
			if (randint(2)==1) return;
		}
	}

	else if (artifact_bias == BIAS_LAW && !(o_ptr->tval == TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_SLAY_EVIL))
		{
			o_ptr->art_flags1 |= TR1_SLAY_EVIL;
			if (randint(2)==1) return;
		}
		if (!(o_ptr->art_flags1 & TR1_SLAY_UNDEAD))
		{
			o_ptr->art_flags1 |= TR1_SLAY_UNDEAD;
			if (randint(2)==1) return;
		}
		if (!(o_ptr->art_flags1 & TR1_SLAY_DEMON))
		{
			o_ptr->art_flags1 |= TR1_SLAY_DEMON;
			if (randint(2)==1) return;
		}
	}

  if (!(o_ptr->tval == TV_BOW))
  {
    switch (randint(34))
    {
    case 1: case 2:
	o_ptr->art_flags1 |= TR1_SLAY_ANIMAL;
/*  if (is_scroll) msg_print("You start hating animals.");*/
	break;
    case 3: case 4:
	o_ptr->art_flags1 |= TR1_SLAY_EVIL;
/*  if (is_scroll) msg_print("You hate evil creatures.");*/
	if (!artifact_bias && (randint(2)==1))
	    artifact_bias = BIAS_LAW;
	else if (!artifact_bias && (randint(9)==1))
	    artifact_bias = BIAS_PRIESTLY;
	break;
    case 5: case 6:
	o_ptr->art_flags1 |= TR1_SLAY_UNDEAD;
/*  if (is_scroll) msg_print("You hate undead creatures.");*/
	if (!artifact_bias && (randint(9)==1))
	    artifact_bias = BIAS_PRIESTLY;
	break;
    case 7: case 8:
	o_ptr->art_flags1 |= TR1_SLAY_DEMON;
/*  if (is_scroll) msg_print("You hate demons.");*/
	if (!artifact_bias && (randint(9)==1))
	    artifact_bias = BIAS_PRIESTLY;
	break;
    case 9: case 10:
	o_ptr->art_flags1 |= TR1_SLAY_ORC;
/*  if (is_scroll) msg_print("You hate orcs.");*/
	break;
    case 11: case 12:
	o_ptr->art_flags1 |= TR1_SLAY_TROLL;
/*  if (is_scroll) msg_print("You hate trolls.");*/
	break;
    case 13: case 14:
	o_ptr->art_flags1 |= TR1_SLAY_GIANT;
/*  if (is_scroll) msg_print("You hate giants.");*/
	break;
    case 15: case 16:
	o_ptr->art_flags1 |= TR1_SLAY_DRAGON;
/*  if (is_scroll) msg_print("You hate dragons.");*/
	break;
    case 17: 
	o_ptr->art_flags1 |= TR1_KILL_DRAGON;
/*  if (is_scroll) msg_print("You feel an intense hatred of dragons.");*/
	break;
    case 18:  case 19:
	if (o_ptr->tval == TV_SWORD)
	    {   o_ptr->art_flags1 |= TR1_VORPAL;
/*      if (is_scroll) msg_print("It looks extremely sharp!");*/
		if (!artifact_bias && (randint(9)==1))
		    artifact_bias = BIAS_WARRIOR;
	    }
	else random_slay(o_ptr, is_scroll);
	break;
    case 20:
	o_ptr->art_flags1 |= TR1_IMPACT;
/*  if (is_scroll) msg_print("The ground trembles beneath you.");*/
	break;
    case 21: case 22:
	o_ptr->art_flags1 |= TR1_BRAND_FIRE;
/*  if (is_scroll) msg_print("It feels hot!");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_FIRE;
	break;
    case 23: case 24:
	o_ptr->art_flags1 |= TR1_BRAND_COLD;
/*  if (is_scroll) msg_print("It feels cold!");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_COLD;
	break;
    case 25: case 26:
	o_ptr->art_flags1 |= TR1_BRAND_ELEC;
/*  if (is_scroll) msg_print("Ouch! You get zapped!");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_ELEC;
	break;
    case 27: case 28:
	o_ptr->art_flags1 |= TR1_BRAND_ACID;
/*  if (is_scroll) msg_print("Its smell makes you feel dizzy.");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_ACID;
	break;
    case 29: case 30:
	o_ptr->art_flags1 |= TR1_BRAND_POIS;
/*  if (is_scroll) msg_print("It smells rotten.");*/
	if (!artifact_bias && (randint(3)!=1))
	    artifact_bias = BIAS_POIS;
	else if (!artifact_bias && randint(6)==1)
	    artifact_bias = BIAS_NECROMANTIC;
	else if (!artifact_bias)
	    artifact_bias = BIAS_ROGUE;
	break;
    case 31: case 32:
	o_ptr->art_flags1 |= TR1_VAMPIRIC;
/*  if (is_scroll) msg_print("You think it bit you!");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_NECROMANTIC;
	break;
	default:
	o_ptr->art_flags1 |= TR1_CHAOTIC;
/*  if (is_scroll) msg_print("It looks very confusing.");*/
	if (!artifact_bias)
	    artifact_bias = BIAS_CHAOS;
	break;
      }
  }
  else
  {
    switch (randint(6))
    {
	case 1: case 2: case 3:
	o_ptr->art_flags3 |= TR3_XTRA_MIGHT;
/*  if (is_scroll) msg_print("It looks mightier than before."); */
	if (!artifact_bias && randint(9)==1)
	    artifact_bias = BIAS_RANGER;
	break;
	default:
	o_ptr->art_flags3 |= TR3_XTRA_SHOTS;
/*  if (is_scroll) msg_print("It seems faster!"); */
	if (!artifact_bias && randint(9)==1)
	    artifact_bias = BIAS_RANGER;
	break;
    }
  }
}


void give_activation_power (object_type * o_ptr)
{
	int type = 0, chance = 0;

	if (artifact_bias)
	{
		if (artifact_bias == BIAS_ELEC)
		{
			if (randint(3)!=1)
			{
				type = ACT_BO_ELEC_1;
			}
			else if (randint(5)!=1)
			{
				type = ACT_BA_ELEC_2;
			}
			else
			{
				type = ACT_BA_ELEC_3;
			}
			chance = 101;
		}
		else if (artifact_bias == BIAS_POIS)
		{
			type = ACT_BA_POIS_1;
			chance = 101;
		}
		else if (artifact_bias == BIAS_FIRE)
		{
			if (randint(3)!=1)
			{
				type = ACT_BO_FIRE_1;
			}
			else if (randint(5)!=1)
			{
				type = ACT_BA_FIRE_1;
			}
			else
			{
				type = ACT_BA_FIRE_2;
			}
			chance = 101;
		}
		else if (artifact_bias == BIAS_COLD)
		{
			chance = 101;
			if (randint(3)!=1)
				type = ACT_BO_COLD_1;
			else if (randint(3)!=1)
				type = ACT_BA_COLD_1;
			else if (randint(3)!=1)
				type = ACT_BA_COLD_2;
			else
				type = ACT_BA_COLD_3;
		}
		else if (artifact_bias == BIAS_CHAOS)
		{
			chance = 50;
			if (randint(6)==1)
				type = ACT_SUMMON_DEMON;
			else
				type = ACT_CALL_CHAOS;
		}
		else if (artifact_bias == BIAS_PRIESTLY)
		{
			chance = 101;

			if (randint(13)==1)
				type = ACT_CHARM_UNDEAD;
			else if (randint(12)==1)
				type = ACT_BANISH_EVIL;
			else if (randint(11)==1)
				type = ACT_DISP_EVIL;
			else if (randint(10)==1)
				type = ACT_PROT_EVIL;
			else if (randint(9)==1)
				type = ACT_CURE_1000;
			else if (randint(8)==1)
				type = ACT_CURE_700;
			else if (randint(7)==1)
				type = ACT_REST_ALL;
			else if (randint(6)==1)
				type = ACT_REST_LIFE;
			else
				type = ACT_CURE_MW;
		}
		else if (artifact_bias == BIAS_NECROMANTIC)
		{
			chance = 101;
			if (randint(66)==1)
				type = ACT_WRAITH;
			else if (randint(13)==1)
				type = ACT_DISP_GOOD;
			else if (randint(9)==1)
				type = ACT_MASS_GENO;
			else if (randint(8)==1)
				type = ACT_GENOCIDE;
			else if (randint(13)==1)
				type = ACT_SUMMON_UNDEAD;
			else if (randint(9)==1)
				type = ACT_VAMPIRE_2;
			else if (randint(6)==1)
				type = ACT_CHARM_UNDEAD;
			else
				type = ACT_VAMPIRE_1;
		}
		else if (artifact_bias == BIAS_LAW)
		{
			chance = 101;
			if (randint(8)==1)
				type = ACT_BANISH_EVIL;
			else if (randint(4)==1)
				type = ACT_DISP_EVIL;
			else
				type = ACT_PROT_EVIL;
		}
		else if (artifact_bias == BIAS_ROGUE)
		{
			chance = 101;
			if (randint(50)==1)
				type = ACT_SPEED;
			else if (randint(4)==1)
				type = ACT_SLEEP;
			else if (randint(3)==1)
				type = ACT_DETECT_ALL;
			else if (randint(8)==1)
				type = ACT_ID_FULL;
			else
				type = ACT_ID_PLAIN;
		}
		else if (artifact_bias == BIAS_MAGE)
		{
			chance = 66;
			if (randint(20)==1)
				type = SUMMON_ELEMENTAL;
			else if (randint(10)==1)
				type = SUMMON_PHANTOM;
			else if (randint(5)==1)
				type = ACT_RUNE_EXPLO;
			else
				type = ACT_ESP;
		}
		else if (artifact_bias == BIAS_WARRIOR)
		{
			chance = 80;
			if (randint(100)==1)
				type = ACT_INVULN;
			else
				type = ACT_BERSERK;
		}
		else if (artifact_bias == BIAS_RANGER)
		{
			chance = 101;
			if (randint(20)==1)
				type = ACT_CHARM_ANIMALS;
			else if (randint(7)==1)
				type = ACT_SUMMON_ANIMAL;
			else if (randint(6)==1)
				type = ACT_CHARM_ANIMAL;
			else if (randint(4)==1)
				type = ACT_RESIST_ALL;
			else if (randint(3)==1)
				type = ACT_SATIATE;
			else
				type = ACT_CURE_POISON;
		}
	}

	while (!(type) || (randint(100)>=chance))
	{
		type = randint(255);
		switch (type)
		{
			case ACT_SUNLIGHT: case ACT_BO_MISS_1:
			case ACT_BA_POIS_1: case ACT_BO_ELEC_1:
			case ACT_BO_ACID_1: case ACT_BO_COLD_1: case ACT_BO_FIRE_1:
			case ACT_CONFUSE: case ACT_SLEEP: case ACT_QUAKE:
			case ACT_CURE_LW: case ACT_CURE_MW: case ACT_CURE_POISON:
			case ACT_BERSERK: case ACT_LIGHT: case ACT_MAP_LIGHT:
			case ACT_DEST_DOOR: case ACT_STONE_MUD: case ACT_TELEPORT:
				chance = 101;
				break;
			case ACT_BA_COLD_1: case ACT_BA_FIRE_1: case ACT_DRAIN_1:
			case ACT_TELE_AWAY: case ACT_ESP: case ACT_RESIST_ALL:
			case ACT_DETECT_ALL: case ACT_RECALL:
			case ACT_SATIATE: case ACT_RECHARGE:
				chance = 85;
				break;
			case ACT_TERROR: case ACT_PROT_EVIL: case ACT_ID_PLAIN:
				chance = 75;
				break;
			case ACT_DRAIN_2: case ACT_VAMPIRE_1: case ACT_BO_MISS_2:
			case ACT_BA_FIRE_2: case ACT_REST_LIFE:
				chance = 66;
				break;
			case ACT_BA_COLD_3: case ACT_BA_ELEC_3: case ACT_WHIRLWIND:
			case ACT_VAMPIRE_2: case ACT_CHARM_ANIMAL:
				chance = 50;
				break;
			case ACT_SUMMON_ANIMAL:
				chance = 40;
				break;
			case ACT_DISP_EVIL: case ACT_BA_MISS_3: case ACT_DISP_GOOD:
			case ACT_BANISH_EVIL: case ACT_GENOCIDE: case ACT_MASS_GENO:
			case ACT_CHARM_UNDEAD: case ACT_CHARM_OTHER: case ACT_SUMMON_PHANTOM:
			case ACT_REST_ALL:
			case ACT_RUNE_EXPLO:
				chance = 33;
				break;
			case ACT_CALL_CHAOS: case ACT_ROCKET:
			case ACT_CHARM_ANIMALS: case ACT_CHARM_OTHERS:
			case ACT_SUMMON_ELEMENTAL: case ACT_CURE_700:
			case ACT_SPEED: case ACT_ID_FULL: case ACT_RUNE_PROT:
				chance = 25;
				break;
			case ACT_CURE_1000: case ACT_XTRA_SPEED:
			case ACT_DETECT_XTRA: case ACT_DIM_DOOR:
				chance = 10;
				break;
			case ACT_SUMMON_UNDEAD: case ACT_SUMMON_DEMON:
			case ACT_WRAITH: case ACT_INVULN: case ACT_ALCHEMY:
				chance = 5;
				break;
			default:
				chance = 0;
		}
	}

	/* A type was chosen... */
	o_ptr->xtra2 = type;
	o_ptr->art_flags3 |= TR3_ACTIVATE;
	o_ptr->timeout = 0;
}

int get_activation_power()
{
        object_type *o_ptr, forge;

        o_ptr = &forge;

        artifact_bias = 0;

        give_activation_power(o_ptr);

        return o_ptr->xtra2;
}

void get_random_name(char * return_name, bool armour, int power)
{
	if (randint(100)<=TABLE_NAME)
		get_table_name(return_name);
	else
	{
		char NameFile[16];
		switch (armour)
		{
			case 1:
				switch(power)
				{
					case 0:
						strcpy(NameFile, "a_cursed.txt");
						break;
					case 1:
						strcpy(NameFile, "a_low.txt");
						break;
					case 2:
						strcpy(NameFile, "a_med.txt");
						break;
					default:
						strcpy(NameFile, "a_high.txt");
				}
				break;
			default:
				switch(power)
				{
					case 0:
						strcpy(NameFile, "w_cursed.txt");
						break;
					case 1:
						strcpy(NameFile, "w_low.txt");
						break;
					case 2:
						strcpy(NameFile, "w_med.txt");
						break;
					default:
						strcpy(NameFile, "w_high.txt");
				}
		}

		get_rnd_line(NameFile, return_name);
	}
}


bool create_artifact(object_type *o_ptr, bool a_scroll, bool get_name)
{
	char new_name[80];
	int has_pval = 0;
	int powers = randint(5) + 1;
	int max_type = (o_ptr->tval<TV_BOOTS?7:5);
	int power_level;
	s32b total_flags;
	bool a_cursed = FALSE;

	int warrior_artifact_bias = 0;

	artifact_bias = 0;

	if (a_scroll && (randint(4)==1))
	{
		switch (p_ptr->pclass)
		{
			case CLASS_WARRIOR:
				artifact_bias = BIAS_WARRIOR;
				break;
			case CLASS_MAGE:
			case CLASS_HIGH_MAGE:
				artifact_bias = BIAS_MAGE;
				break;
			case CLASS_PRIEST:
				artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_ROGUE:
				artifact_bias = BIAS_ROGUE;
				warrior_artifact_bias = 25;
				break;
			case CLASS_RANGER:
				artifact_bias = BIAS_RANGER;
				warrior_artifact_bias = 30;
				break;
			case CLASS_PALADIN:
				artifact_bias = BIAS_PRIESTLY;
				warrior_artifact_bias = 40;
				break;
			case CLASS_WARRIOR_MAGE:
				artifact_bias = BIAS_MAGE;
				warrior_artifact_bias = 40;
				break;
			case CLASS_MONK:
				artifact_bias = BIAS_PRIESTLY;
				break;
		}
	}

	if ((randint(100) <= warrior_artifact_bias) && a_scroll) artifact_bias = BIAS_WARRIOR;

	strcpy(new_name,"");

	if ((!a_scroll) && (randint(A_CURSED)==1)) a_cursed = TRUE;

	while ((randint(powers) == 1) || (randint(7)==1) || randint(10)==1) powers++;

	if ((!a_cursed) && (randint(WEIRD_LUCK)==1)) powers *= 2;

	if (a_cursed) powers /= 2;

	/* Main loop */
	while(powers--)
	{
		switch (randint(max_type))
		{
			case 1: case 2:
				random_plus(o_ptr, a_scroll);
				has_pval = TRUE;
				break;
			case 3: case 4:
				random_resistance(o_ptr);
				break;
			case 5:
				random_misc(o_ptr, a_scroll);
				break;
			case 6: case 7:
				random_slay(o_ptr, a_scroll);
				break;
			default:
				if (wizard) msg_print("Switch error in create_artifact!");
				powers++;
		}
	};

	if (has_pval)
	{
#if 0
		o_ptr->art_flags3 |= TR3_SHOW_MODS;

		/* This one commented out by gw's request... */
		if (!a_scroll)
			o_ptr->art_flags3 |= TR3_HIDE_TYPE;
#endif

		if (o_ptr->art_flags1 & TR1_BLOWS)
		    o_ptr->pval = randint(2) + 1;
		else
		{
			do
			{
				o_ptr->pval++;
			}
			while (o_ptr->pval<randint(5) || randint(o_ptr->pval)==1);
		}

		if (o_ptr->pval > 4 && (randint(WEIRD_LUCK)!=1))
			o_ptr->pval = 4;
	}

	/* give it some plusses... */
	if (o_ptr->tval>=TV_BOOTS)
		o_ptr->to_a += randint(o_ptr->to_a>19?1:20-o_ptr->to_a);
	else
	{
		o_ptr->to_h += randint(o_ptr->to_h>19?1:20-o_ptr->to_h);
		o_ptr->to_d += randint(o_ptr->to_d>19?1:20-o_ptr->to_d);
	}

	/* Just to be sure */
	o_ptr->art_flags3 |= ( TR3_IGNORE_ACID | TR3_IGNORE_ELEC |
	                       TR3_IGNORE_FIRE | TR3_IGNORE_COLD);

	total_flags = flag_cost(o_ptr, o_ptr->pval);
	if (cheat_peek) msg_format("%ld", total_flags);

	if (a_cursed) curse_artifact(o_ptr);

	if ((!a_cursed) &&
	    (randint((o_ptr->tval>=TV_BOOTS)
	    ?ACTIVATION_CHANCE * 2 : ACTIVATION_CHANCE)==1))
	{
		o_ptr->xtra2 = 0;
		give_activation_power(o_ptr);
	}


	if (o_ptr->tval>=TV_BOOTS)
	{
		if (a_cursed) power_level = 0;
		else if (total_flags<10000) power_level = 1;
		else if (total_flags<20000) power_level = 2;
		else power_level = 3;
	}

	else
	{
		if (a_cursed) power_level = 0;
		else if (total_flags<15000) power_level = 1;
		else if (total_flags<30000) power_level = 2;
		else power_level = 3;
	}

        if(get_name)
        {
	if (a_scroll)
	{
		char dummy_name[80];
		strcpy(dummy_name, "");
		identify_fully_aux(o_ptr);
		o_ptr->ident |= IDENT_STOREB; /* This will be used later on... */
		if (!(get_string("What do you want to call the artifact? ", dummy_name, 80)))
			strcpy(new_name,"(a DIY Artifact)");
		else
		{
			strcpy(new_name,"called '");
			strcat(new_name,dummy_name);
			strcat(new_name,"'");
		}
		/* Identify it fully */
		object_aware(o_ptr);
		object_known(o_ptr);

		/* Mark the item as fully known */
		o_ptr->ident |= (IDENT_MENTAL);
	}
	else
	{
		get_random_name(new_name, (bool)(o_ptr->tval >= TV_BOOTS), power_level);
	}
        }

	if (cheat_xtra)
	{
		if (artifact_bias)
			msg_format("Biased artifact: %d.", artifact_bias);
		else
			msg_print("No bias in artifact.");
	}

	/* Save the inscription */
	o_ptr->art_name = quark_add(new_name);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	return TRUE;
}


bool artifact_scroll(void)
{
	int             item;
	bool            okay = FALSE;
	object_type     *o_ptr;
	char            o_name[80];

	cptr q, s;


	/* Enchant weapon/armour */
        item_tester_hook = item_tester_hook_artifactable;

	/* Get an item */
	q = "Enchant which item? ";
	s = "You have nothing to enchant.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Describe */
	msg_format("%s %s radiate%s a blinding light!",
	          ((item >= 0) ? "Your" : "The"), o_name,
	          ((o_ptr->number > 1) ? "" : "s"));

	if (o_ptr->name1 || o_ptr->art_name)
	{
		msg_format("The %s %s already %s!",
		    o_name, ((o_ptr->number > 1) ? "are" : "is"),
		    ((o_ptr->number > 1) ? "artifacts" : "an artifact"));
		okay = FALSE;
	}

	else if (o_ptr->name2)
	{
		msg_format("The %s %s already %s!",
		    o_name, ((o_ptr->number > 1) ? "are" : "is"),
		    ((o_ptr->number > 1) ? "ego items" : "an ego item"));
		okay = FALSE;
	}

	else
	{
		if (o_ptr->number > 1)
		{
			msg_print("Not enough enough energy to enchant more than one object!");
			msg_format("%d of your %s %s destroyed!",(o_ptr->number)-1, o_name, (o_ptr->number>2?"were":"was"));
			o_ptr->number = 1;
		}
                okay = create_artifact(o_ptr, TRUE, TRUE);
	}

	/* Failure */
	if (!okay)
	{
		/* Flush */
		if (flush_failure) flush();

		/* Message */
		msg_print("The enchantment failed.");
	}

	/* Something happened */
	return (TRUE);
}


/*
 * Identify an object in the inventory (or on the floor)
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell(void)
{
	int             item;

	object_type     *o_ptr;

	char            o_name[80];

	cptr q, s;

	/* Get an item */
	q = "Identify which item? ";
	s = "You have nothing to identify.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Identify it fully */
	object_aware(o_ptr);
	object_known(o_ptr);

        /* CAN fully identify magic items, but not artifacts */
        if (o_ptr->name1 == 0) o_ptr->ident |= (IDENT_MENTAL);
        
	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Description */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

	/* Something happened */
	return (TRUE);
}



/*
 * Fully "identify" an object in the inventory  -BEN-
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully(void)
{
	int             item;
	object_type     *o_ptr;
	char            o_name[80];

	cptr q, s;

	/* Get an item */
	q = "Identify which item? ";
	s = "You have nothing to identify.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Identify it fully */
	object_aware(o_ptr);
	object_known(o_ptr);

	/* Mark the item as fully known */
	o_ptr->ident |= (IDENT_MENTAL);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Handle stuff */
	handle_stuff();

	/* Description */
	object_desc(o_name, o_ptr, TRUE, 3);

	/* Describe */
	if (item >= INVEN_WIELD)
	{
		msg_format("%^s: %s (%c).",
			   describe_use(item), o_name, index_to_label(item));
	}
	else if (item >= 0)
	{
		msg_format("In your pack: %s (%c).",
			   o_name, index_to_label(item));
	}
	else
	{
		msg_format("On the ground: %s.",
			   o_name);
	}

	/* Describe it fully */
	identify_fully_aux(o_ptr);

	/* Success */
	return (TRUE);
}




/*
 * Hook for "get_item()".  Determine if something is rechargable.
 */
bool item_tester_hook_recharge(object_type *o_ptr)
{
        /* Recharge staffs -- hack -- not the Staffs of Wishing*/
        if ((o_ptr->tval == TV_STAFF) && (o_ptr->sval != SV_STAFF_WISHING)) return (TRUE);

	/* Recharge wands */
	if (o_ptr->tval == TV_WAND) return (TRUE);

	/* Hack -- Recharge rods */
	if (o_ptr->tval == TV_ROD) return (TRUE);

	/* Nope */
	return (FALSE);
}


/*
 * Recharge a wand/staff/rod from the pack or on the floor.
 * This function has been rewritten in Oangband. -LM-
 *
 * Mage -- Recharge I --> recharge(90)
 * Mage -- Recharge II --> recharge(150)
 * Mage -- Recharge III --> recharge(220)
 *
 * Priest or Necromancer -- Recharge --> recharge(140)
 *
 * Scroll of recharging --> recharge(130)
 * Scroll of *recharging* --> recharge(200)
 *
 * It is harder to recharge high level, and highly charged wands, 
 * staffs, and rods.  The more wands in a stack, the more easily and 
 * strongly they recharge.  Staffs, however, each get fewer charges if 
 * stacked.
 *
 * XXX XXX XXX Beware of "sliding index errors".
 */
bool recharge(int power)
{
	int recharge_strength, recharge_amount;
        int item, lev;

	bool fail = FALSE;
	byte fail_type = 1;


	cptr q, s;

        u32b f1, f2, f3, f4;
	char o_name[80];

	object_type *o_ptr;
	object_kind *k_ptr;

	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	/* Get an item */
	q = "Recharge which item? ";
	s = "You have nothing to recharge.";
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

        /* Extract the flags */
        object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Extract the object "level" */
	lev = k_info[o_ptr->k_idx].level;
        k_ptr = &k_info[o_ptr->k_idx];

	/* Recharge a rod */
	if (o_ptr->tval == TV_ROD)
	{
		/* Extract a recharge strength by comparing object level to power. */
		recharge_strength = ((power > lev) ? (power - lev) : 0) / 5;
                
                /* Paranoia */
                if (recharge_strength < 0) recharge_strength = 0;

		/* Back-fire */
                if ((rand_int(recharge_strength) == 0)&&(!(f4 & TR4_RECHARGE)))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}

		/* Recharge */
		else
		{
			/* Recharge amount */
			recharge_amount = (power * damroll(3, 2));

			/* Recharge by that amount */
			if (o_ptr->timeout > recharge_amount)
				o_ptr->timeout -= recharge_amount;
			else
				o_ptr->timeout = 0;
		}
	}


	/* Recharge wand/staff */
	else
	{
		/* Extract a recharge strength by comparing object level to power. 
		 * Divide up a stack of wands' charges to calculate charge penalty.
		 */
		if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
			recharge_strength = (100 + power - lev - 
			(8 * o_ptr->pval / o_ptr->number)) / 15;

		/* All staffs, unstacked wands. */
		else recharge_strength = (100 + power - lev - 
			(8 * o_ptr->pval)) / 15;


		/* Back-fire XXX XXX XXX */
                if (((rand_int(recharge_strength) == 0)&&(!(f4 & TR4_RECHARGE))) ||
                    ((o_ptr->tval == TV_STAFF) && o_ptr->sval == SV_STAFF_WISHING))
		{
			/* Activate the failure code. */
			fail = TRUE;
		}

		/* If the spell didn't backfire, recharge the wand or staff. */
		else
		{
			/* Recharge based on the standard number of charges. */
			recharge_amount = randint(1 + k_ptr->pval / 2);

			/* Multiple wands in a stack increase recharging somewhat. */
			if ((o_ptr->tval == TV_WAND) && (o_ptr->number > 1))
			{
				recharge_amount += 
					(randint(recharge_amount * (o_ptr->number - 1))) / 2;
				if (recharge_amount < 1) recharge_amount = 1;
				if (recharge_amount > 12) recharge_amount = 12;
			}

			/* But each staff in a stack gets fewer additional charges, 
			 * although always at least one.
			 */
			if ((o_ptr->tval == TV_STAFF) && (o_ptr->number > 1))
			{
				recharge_amount /= o_ptr->number;
				if (recharge_amount < 1) recharge_amount = 1;
			}

			/* Recharge the wand or staff. */
			o_ptr->pval += recharge_amount;

                        if(!(f4 & TR4_RECHARGE))
                        {
                                /* Hack -- we no longer "know" the item */
                                o_ptr->ident &= ~(IDENT_KNOWN);
                        }

			/* Hack -- we no longer think the item is empty */
			o_ptr->ident &= ~(IDENT_EMPTY);
		}
	}

	/* Inflict the penalties for failing a recharge. */
	if (fail)
	{
		/* Artifacts are never destroyed. */
		if (artifact_p(o_ptr))
		{
			object_desc(o_name, o_ptr, TRUE, 0);
			msg_format("The recharging backfires - %s is completely drained!", o_name);

			/* Artifact rods. */
			if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout < 10000)) 
				o_ptr->timeout = (o_ptr->timeout + 100) * 2;

			/* Artifact wands and staffs. */
			else if ((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_STAFF)) 
				o_ptr->pval = 0;
		}
		else 
		{
			/* Get the object description */
			object_desc(o_name, o_ptr, FALSE, 0);

			/*** Determine Seriousness of Failure ***/

			/* Mages recharge objects more safely. */
			if (p_ptr->pclass == CLASS_MAGE)
			{
				/* 10% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (randint(10) == 1) fail_type = 2;
					else fail_type = 1;
				}
				/* 75% chance to blow up one wand, otherwise draining. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (randint(3) != 1) fail_type = 2;
					else fail_type = 1;
				}
				/* 50% chance to blow up one staff, otherwise no effect. */
				else if (o_ptr->tval == TV_STAFF)
				{
					if (randint(2) == 1) fail_type = 2;
					else fail_type = 0;
				}
			}

			/* All other classes get no special favors. */
			else
			{
				/* 33% chance to blow up one rod, otherwise draining. */
				if (o_ptr->tval == TV_ROD)
				{
					if (randint(3) == 1) fail_type = 2;
					else fail_type = 1;
				}
				/* 20% chance of the entire stack, else destroy one wand. */
				else if (o_ptr->tval == TV_WAND)
				{
					if (randint(5) == 1) fail_type = 3;
					else fail_type = 2;
				}
				/* Blow up one staff. */
				else if (o_ptr->tval == TV_STAFF)
				{
					fail_type = 2;
				}
			}

			/*** Apply draining and destruction. ***/

			/* Drain object or stack of objects. */
			if (fail_type == 1)
			{
				if (o_ptr->tval == TV_ROD)
				{
					msg_print("The recharge backfires, draining the rod further!");
					if (o_ptr->timeout < 10000) 
						o_ptr->timeout = (o_ptr->timeout + 100) * 2;
				}
				else if (o_ptr->tval == TV_WAND)
				{
					msg_format("You save your %s from destruction, but all charges are lost.", o_name);
					o_ptr->pval = 0;
				}
				/* Staffs aren't drained. */
			}

			/* Destroy an object or one in a stack of objects. */
			if (fail_type == 2)
			{
				if (o_ptr->number > 1)
					msg_format("Wild magic consumes one of your %s!", o_name);
				else
					msg_format("Wild magic consumes your %s!", o_name);

				/* Reduce rod stack maximum timeout, drain wands. */
				if (o_ptr->tval == TV_ROD) o_ptr->pval -= k_ptr->pval;
				if (o_ptr->tval == TV_WAND) o_ptr->pval = 0;

				/* Reduce and describe inventory */
				if (item >= 0)
				{
					inven_item_increase(item, -1);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -1);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}

			/* Destroy all memebers of a stack of objects. */
			if (fail_type == 3)
			{
				if (o_ptr->number > 1)
					msg_format("Wild magic consumes all your %s!", o_name);
				else
					msg_format("Wild magic consumes your %s!", o_name);


				/* Reduce and describe inventory */
				if (item >= 0)
				{
					inven_item_increase(item, -999);
					inven_item_describe(item);
					inven_item_optimize(item);
				}

				/* Reduce and describe floor item */
				else
				{
					floor_item_increase(0 - item, -999);
					floor_item_describe(0 - item);
					floor_item_optimize(0 - item);
				}
			}
		}
	}


	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN);

	/* Something was done */
	return (TRUE);
}



/*
 * Apply a "project()" directly to all viewable monsters
 *
 * Note that affected monsters are NOT auto-tracked by this usage.
 */
bool project_hack(int typ, s32b dam)
{
	int     i, x, y;
	int     flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
	bool    obvious = FALSE;


	/* Affect all (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Require line of sight */
		if (!player_has_los_bold(y, x)) continue;

		/* Jump directly to the target monster */
		if (project(0, 0, y, x, dam, typ, flg)) obvious = TRUE;
	}

	/* Result */
	return (obvious);
}

/*
 * Apply a "project()" a la meteor shower
 */
void project_meteor(int radius, int typ, s32b dam, u32b flg)
{
        int x, y, dx, dy, d, count = 0, i;
        int b = radius + randint(radius); 
        for (i = 0; i < b; i++) {
                do {
                        count++;
                        if (count > 1000)  break;
                        x = px - 5 + randint(10);
                        y = py - 5 + randint(10);
                        dx = (px > x) ? (py - x) : (x - px);
                        dy = (py > y) ? (py - y) : (y - py);
                        /* Approximate distance */
                        d = (dy > dx) ? (dy + (dx>>1)) : (dx + (dy>>1));
                } while ((d > 5) || (!(player_has_los_bold(y, x))));
			   
                if (count > 1000)   break;
                count = 0;
                project(0, 2, y, x, dam, typ, PROJECT_JUMP | flg);
        }
}


/*
 * Speed monsters
 */
bool speed_monsters(void)
{
	return (project_hack(GF_OLD_SPEED, p_ptr->lev));
}

/*
 * Slow monsters
 */
bool slow_monsters(void)
{
	return (project_hack(GF_OLD_SLOW, p_ptr->lev));
}

/*
 * Paralyzation monsters
 */
bool conf_monsters(void)
{
        return (project_hack(GF_OLD_CONF, p_ptr->lev));
}

/*
 * Sleep monsters
 */
bool sleep_monsters(void)
{
	return (project_hack(GF_OLD_SLEEP, p_ptr->lev));
}

/*
 * Scare monsters
 */
bool scare_monsters(void)
{
        return (project_hack(GF_FEAR, p_ptr->lev));
}


/*
 * Banish evil monsters
 */
bool banish_evil(int dist)
{
	return (project_hack(GF_AWAY_EVIL, dist));
}


/*
 * Turn undead
 */
bool turn_undead(void)
{
	return (project_hack(GF_TURN_UNDEAD, p_ptr->lev));
}


/*
 * Dispel undead monsters
 */
bool dispel_undead(s32b dam)
{
	return (project_hack(GF_DISP_UNDEAD, dam));
}

/*
 * Dispel evil monsters
 */
bool dispel_evil(s32b dam)
{
	return (project_hack(GF_DISP_EVIL, dam));
}

/*
 * Dispel good monsters
 */
bool dispel_good(s32b dam)
{
    return (project_hack(GF_DISP_GOOD, dam));
}

/*
 * Dispel all monsters
 */
bool dispel_monsters(s32b dam)
{
	return (project_hack(GF_DISP_ALL, dam));
}

/*
 * Dispel 'living' monsters
 */
bool dispel_living(s32b dam)
{
	return (project_hack(GF_DISP_LIVING, dam));
}

/*
 * Dispel demons
 */
bool dispel_demons(s32b dam)
{
	return (project_hack(GF_DISP_DEMON, dam));
}


/*
 * Wake up all monsters, and speed up "los" monsters.
 */
void aggravate_monsters(int who)
{
	int     i;
	bool    sleep = FALSE;
	bool    speed = FALSE;


	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];
		monster_race    *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* Wake up nearby sleeping monsters */
		if (m_ptr->cdis < MAX_SIGHT * 2)
		{
			/* Wake up */
			if (m_ptr->csleep)
			{
				/* Wake up */
				m_ptr->csleep = 0;
				sleep = TRUE;
			}
		}

		/* Speed up monsters in line of sight */
		if (player_has_los_bold(m_ptr->fy, m_ptr->fx))
		{
			/* Speed up (instantly) to racial base + 10 */
			if (m_ptr->mspeed < r_ptr->speed + 10)
			{
				/* Speed up */
				m_ptr->mspeed = r_ptr->speed + 10;
				speed = TRUE;
			}

			/* Pets may get angry (50% chance) */
			if (is_pet(m_ptr))
			{
				if (randint(2)==1)
				{
					set_pet(m_ptr, FALSE);
				}
			}
		}
	}

	/* Messages */
	if (speed) msg_print("You feel a sudden stirring nearby!");
	else if (sleep) msg_print("You hear a sudden stirring in the distance!");
}

/*
 * Inflict dam damage of type typee to all monster of the given race
 */
bool invoke(s32b dam, int typee)
{
	int     i;
	char    typ;
	bool    result = FALSE;
	int     msec = delay_factor * delay_factor * delay_factor;

        if (special_flag) return(FALSE);

	/* Mega-Hack -- Get a monster symbol */
	(void)(get_com("Choose a monster race (by symbol) to genocide: ", &typ));

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];
		monster_race    *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip Unique Monsters */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;

		/* Hack -- Skip Quest Monsters */
		if (r_ptr->flags1 & RF1_QUESTOR) continue;

		/* Skip "wrong" monsters */
		if (r_ptr->d_char != typ) continue;

                project_m(0, 0, m_ptr->fy, m_ptr->fx, dam, typee);

		/* Visual feedback */
		move_cursor_relative(py, px);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);

		/* Take note */
		result = TRUE;
	}

	return (result);
}


/*
 * Delete all non-unique/non-quest monsters of a given "type" from the level
 */
bool genocide(bool player_cast)
{
	int     i;
	char    typ;
	bool    result = FALSE;
	int     msec = delay_factor * delay_factor * delay_factor;

        if (special_flag) return(FALSE);

	/* Mega-Hack -- Get a monster symbol */
	(void)(get_com("Choose a monster race (by symbol) to genocide: ", &typ));

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];
		monster_race    *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip Unique Monsters */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;

		/* Hack -- Skip Quest Monsters */
		if (r_ptr->flags1 & RF1_QUESTOR) continue;

		/* Skip "wrong" monsters */
		if (r_ptr->d_char != typ) continue;

		/* Delete the monster */
		delete_monster_idx(i);

		if (player_cast)
		{
			/* Take damage */
			take_hit(randint(4), "the strain of casting Genocide");
		}

		/* Visual feedback */
		move_cursor_relative(py, px);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);

		/* Take note */
		result = TRUE;
	}

	return (result);
}


/*
 * Delete all nearby (non-unique) monsters
 */
bool mass_genocide(bool player_cast)
{
	int     i;

	bool    result = FALSE;

	int     msec = delay_factor * delay_factor * delay_factor;

        if (special_flag) return(FALSE);

	/* Delete the (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];
		monster_race    *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Hack -- Skip unique monsters */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;

		/* Hack -- Skip Quest Monsters */
		if (r_ptr->flags1 & RF1_QUESTOR) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > MAX_SIGHT) continue;

		/* Delete the monster */
		delete_monster_idx(i);

		if (player_cast)
		{
			/* Hack -- visual feedback */
			take_hit(randint(3), "the strain of casting Mass Genocide");
		}

		move_cursor_relative(py, px);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);

		/* Note effect */
		result = TRUE;
	}

	return (result);
}



/*
 * Probe nearby monsters
 */
bool probing(void)
{
	int     i;

	bool    probe = FALSE;


	/* Probe all (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Require line of sight */
		if (!player_has_los_bold(m_ptr->fy, m_ptr->fx)) continue;

		/* Probe visible monsters */
		if (m_ptr->ml)
		{
			char m_name[80];

			/* Start the message */
			if (!probe) msg_print("Probing...");

			/* Get "the monster" or "something" */
			monster_desc(m_name, m_ptr, 0x04);

			/* Describe the monster */
			msg_format("%^s has %d hit points.", m_name, m_ptr->hp);

			/* Learn all of the non-spell, non-treasure flags */
			lore_do_probe(i);

			/* Probe worked */
			probe = TRUE;
		}
	}

	/* Done */
	if (probe)
	{
		msg_print("That's all.");
	}

	/* Result */
	return (probe);
}



/*
 * The spell of destruction
 *
 * This spell "deletes" monsters (instead of "killing" them).
 *
 * Later we may use one function for both "destruction" and
 * "earthquake" by using the "full" to select "destruction".
 */
void destroy_area(int y1, int x1, int r, bool full)
{
	int y, x, k, t;

	cave_type *c_ptr;

	bool flag = FALSE;


	/* XXX XXX */
	full = full ? full : 0;

        if(special_flag){msg_print("Not on special levels!");return;}

	/* Big area of affect */
	for (y = (y1 - r); y <= (y1 + r); y++)
	{
		for (x = (x1 - r); x <= (x1 + r); x++)
		{
			/* Skip illegal grids */
			if (!in_bounds(y, x)) continue;

			/* Extract the distance */
			k = distance(y1, x1, y, x);

			/* Stay in the circle of death */
			if (k > r) continue;

			/* Access the grid */
			c_ptr = &cave[y][x];

			/* Lose room and vault */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

			/* Lose light and knowledge */
			c_ptr->info &= ~(CAVE_MARK | CAVE_GLOW);

			/* Hack -- Notice player affect */
			if ((x == px) && (y == py))
			{
				/* Hurt the player later */
				flag = TRUE;

				/* Do not hurt this grid */
				continue;
			}

			/* Hack -- Skip the epicenter */
			if ((y == y1) && (x == x1)) continue;

			/* Delete the monster (if any) */
			delete_monster(y, x);

			/* Destroy "valid" grids */
			if (cave_valid_bold(y, x))
			{
				/* Delete objects */
				delete_object(y, x);

				/* Wall (or floor) type */
				t = rand_int(200);

				/* Granite */
				if (t < 20)
				{
					/* Create granite wall */
					c_ptr->feat = FEAT_WALL_EXTRA;
				}

				/* Quartz */
				else if (t < 70)
				{
					/* Create quartz vein */
					c_ptr->feat = FEAT_QUARTZ;
				}

				/* Magma */
				else if (t < 100)
				{
					/* Create magma vein */
					c_ptr->feat = FEAT_MAGMA;
				}

				/* Floor */
				else
				{
					/* Create floor */
					c_ptr->feat = FEAT_FLOOR;
				}
			}
		}
	}


	/* Hack -- Affect player */
	if (flag)
	{
		/* Message */
		msg_print("There is a searing blast of light!");

		/* Blind the player */
		if (!p_ptr->resist_blind)
		{
			/* Become blind */
			(void)set_blind(p_ptr->blind + 10 + randint(10));
		}
	}


	/* Mega-Hack -- Forget the view and lite */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}


/*
 * Induce an "earthquake" of the given radius at the given location.
 *
 * This will turn some walls into floors and some floors into walls.
 *
 * The player will take damage and "jump" into a safe grid if possible,
 * otherwise, he will "tunnel" through the rubble instantaneously.
 *
 * Monsters will take damage, and "jump" into a safe grid if possible,
 * otherwise they will be "buried" in the rubble, disappearing from
 * the level in the same way that they do when genocided.
 *
 * Note that thus the player and monsters (except eaters of walls and
 * passers through walls) will never occupy the same grid as a wall.
 * Note that as of now (2.7.8) no monster may occupy a "wall" grid, even
 * for a single turn, unless that monster can pass_walls or kill_walls.
 * This has allowed massive simplification of the "monster" code.
 */
void earthquake(int cy, int cx, int r)
{
	int             i, t, y, x, yy, xx, dy, dx, oy, ox;
        s32b            damage = 0;
	int             sn = 0, sy = 0, sx = 0;
	bool            hurt = FALSE;
	cave_type       *c_ptr;
	bool            map[32][32];


	/* Paranoia -- Enforce maximum range */
	if (r > 12) r = 12;

	/* Clear the "maximal blast" area */
	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 32; x++)
		{
			map[y][x] = FALSE;
		}
	}

	/* Check around the epicenter */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip illegal grids */
			if (!in_bounds(yy, xx)) continue;

			/* Skip distant grids */
			if (distance(cy, cx, yy, xx) > r) continue;

			/* Access the grid */
			c_ptr = &cave[yy][xx];

			/* Lose room and vault */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

			/* Lose light and knowledge */
			c_ptr->info &= ~(CAVE_GLOW | CAVE_MARK);

			/* Skip the epicenter */
			if (!dx && !dy) continue;

			/* Skip most grids */
			if (rand_int(100) < 85) continue;

			/* Damage this grid */
			map[16+yy-cy][16+xx-cx] = TRUE;

			/* Hack -- Take note of player damage */
			if ((yy == py) && (xx == px)) hurt = TRUE;
		}
	}

	/* First, affect the player (if necessary) */
	if (hurt)
	{
		/* Check around the player */
		for (i = 0; i < 8; i++)
		{
			/* Access the location */
			y = py + ddy[i];
			x = px + ddx[i];

			/* Skip non-empty grids */
			if (!cave_empty_bold(y, x)) continue;

			/* Important -- Skip "quake" grids */
			if (map[16+y-cy][16+x-cx]) continue;

			/* Count "safe" grids */
			sn++;

			/* Randomize choice */
			if (rand_int(sn) > 0) continue;

			/* Save the safe location */
			sy = y; sx = x;
		}

		/* Random message */
		switch (randint(3))
		{
			case 1:
			{
				msg_print("The cave ceiling collapses!");
				break;
			}
			case 2:
			{
				msg_print("The cave floor twists in an unnatural way!");
				break;
			}
			default:
			{
				msg_print("The cave quakes!  You are pummeled with debris!");
				break;
			}
		}

		/* Hurt the player a lot */
		if (!sn)
		{
			/* Message and damage */
			msg_print("You are severely crushed!");
			damage = 300;
		}

		/* Destroy the grid, and push the player to safety */
		else
		{
			/* Calculate results */
			switch (randint(3))
			{
				case 1:
				{
					msg_print("You nimbly dodge the blast!");
					damage = 0;
					break;
				}
				case 2:
				{
					msg_print("You are bashed by rubble!");
					damage = damroll(10, 4);
					(void)set_stun(p_ptr->stun + randint(50));
					break;
				}
				case 3:
				{
					msg_print("You are crushed between the floor and ceiling!");
					damage = damroll(10, 4);
					(void)set_stun(p_ptr->stun + randint(50));
					break;
				}
			}

			/* Save the old location */
			oy = py;
			ox = px;

			/* Move the player to the safe location */
			py = sy;
			px = sx;

			/* Redraw the old spot */
			lite_spot(oy, ox);

			/* Redraw the new spot */
			lite_spot(py, px);

			/* Check for new panel */
			verify_panel();
		}

		/* Important -- no wall on player */
		map[16+py-cy][16+px-cx] = FALSE;

		/* Take some damage */
		if (damage) take_hit(damage, "an earthquake");
	}


	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16+yy-cy][16+xx-cx]) continue;

			/* Access the grid */
			c_ptr = &cave[yy][xx];

			/* Process monsters */
			if (c_ptr->m_idx)
			{
				monster_type *m_ptr = &m_list[c_ptr->m_idx];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				/* Most monsters cannot co-exist with rock */
				if (!(r_ptr->flags2 & (RF2_KILL_WALL)) &&
				    !(r_ptr->flags2 & (RF2_PASS_WALL)))
				{
					char m_name[80];

					/* Assume not safe */
					sn = 0;

					/* Monster can move to escape the wall */
					if (!(r_ptr->flags1 & (RF1_NEVER_MOVE)))
					{
						/* Look for safety */
						for (i = 0; i < 8; i++)
						{
							/* Access the grid */
							y = yy + ddy[i];
							x = xx + ddx[i];

							/* Skip non-empty grids */
							if (!cave_empty_bold(y, x)) continue;

							/* Hack -- no safety on glyph of warding */
							if (cave[y][x].feat == FEAT_GLYPH) continue;
							if (cave[y][x].feat == FEAT_MINOR_GLYPH) continue;

							/* ... nor on the Pattern */
							if ((cave[y][x].feat <= FEAT_PATTERN_XTRA2) &&
							    (cave[y][x].feat >= FEAT_PATTERN_START))
								continue;

							/* Important -- Skip "quake" grids */
							if (map[16+y-cy][16+x-cx]) continue;

							/* Count "safe" grids */
							sn++;

							/* Randomize choice */
							if (rand_int(sn) > 0) continue;

							/* Save the safe grid */
							sy = y; sx = x;
						}
					}

					/* Describe the monster */
					monster_desc(m_name, m_ptr, 0);

					/* Scream in pain */
					msg_format("%^s wails out in pain!", m_name);

					/* Take damage from the quake */
					damage = (sn ? damroll(4, 8) : 200);

					/* Monster is certainly awake */
					m_ptr->csleep = 0;

					/* Apply damage directly */
					m_ptr->hp -= damage;

					/* Delete (not kill) "dead" monsters */
					if (m_ptr->hp < 0)
					{
						/* Message */
						msg_format("%^s is embedded in the rock!", m_name);

						/* Delete the monster */
						delete_monster(yy, xx);

						/* No longer safe */
						sn = 0;
					}

					/* Hack -- Escape from the rock */
					if (sn)
					{
						int m_idx = cave[yy][xx].m_idx;

						/* Update the new location */
						cave[sy][sx].m_idx = m_idx;

						/* Update the old location */
						cave[yy][xx].m_idx = 0;

						/* Move the monster */
						m_ptr->fy = sy;
						m_ptr->fx = sx;

						/* Update the monster (new location) */
						update_mon(m_idx, TRUE);

						/* Redraw the old grid */
						lite_spot(yy, xx);

						/* Redraw the new grid */
						lite_spot(sy, sx);
					}
				}
			}
		}
	}


	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16+yy-cy][16+xx-cx]) continue;

			/* Access the cave grid */
			c_ptr = &cave[yy][xx];

			/* Paranoia -- never affect player */
			if ((yy == py) && (xx == px)) continue;

			/* Destroy location (if valid) */
			if (cave_valid_bold(yy, xx))
			{
				bool floor = cave_floor_bold(yy, xx);

				/* Delete objects */
				delete_object(yy, xx);

				/* Wall (or floor) type */
				t = (floor ? rand_int(100) : 200);

				/* Granite */
				if (t < 20)
				{
					/* Create granite wall */
					c_ptr->feat = FEAT_WALL_EXTRA;
				}

				/* Quartz */
				else if (t < 70)
				{
					/* Create quartz vein */
					c_ptr->feat = FEAT_QUARTZ;
				}

				/* Magma */
				else if (t < 100)
				{
					/* Create magma vein */
					c_ptr->feat = FEAT_MAGMA;
				}

				/* Floor */
				else
				{
					/* Create floor */
					c_ptr->feat = FEAT_FLOOR;
				}
			}
		}
	}


	/* Mega-Hack -- Forget the view and lite */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Update the health bar */
	p_ptr->redraw |= (PR_HEALTH);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);
}



/*
 * This routine clears the entire "temp" set.
 *
 * This routine will Perma-Lite all "temp" grids.
 *
 * This routine is used (only) by "lite_room()"
 *
 * Dark grids are illuminated.
 *
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 1/4 the time when illuminated
 * STUPID monsters wake up 1/10 the time when illuminated
 */
static void cave_temp_room_lite(void)
{
	int i;

	/* Clear them all */
	for (i = 0; i < temp_n; i++)
	{
		int y = temp_y[i];
		int x = temp_x[i];

		cave_type *c_ptr = &cave[y][x];

		/* No longer in the array */
		c_ptr->info &= ~(CAVE_TEMP);

		/* Update only non-CAVE_GLOW grids */
		/* if (c_ptr->info & (CAVE_GLOW)) continue; */

		/* Perma-Lite */
		c_ptr->info |= (CAVE_GLOW);

		/* Process affected monsters */
		if (c_ptr->m_idx)
		{
			int chance = 25;

			monster_type    *m_ptr = &m_list[c_ptr->m_idx];

			monster_race    *r_ptr = &r_info[m_ptr->r_idx];

			/* Update the monster */
			update_mon(c_ptr->m_idx, FALSE);

			/* Stupid monsters rarely wake up */
			if (r_ptr->flags2 & (RF2_STUPID)) chance = 10;

			/* Smart monsters always wake up */
			if (r_ptr->flags2 & (RF2_SMART)) chance = 100;

			/* Sometimes monsters wake up */
			if (m_ptr->csleep && (rand_int(100) < chance))
			{
				/* Wake up! */
				m_ptr->csleep = 0;

				/* Notice the "waking up" */
				if (m_ptr->ml)
				{
					char m_name[80];

					/* Acquire the monster name */
					monster_desc(m_name, m_ptr, 0);

					/* Dump a message */
					msg_format("%^s wakes up.", m_name);
				}
			}
		}

		/* Note */
		note_spot(y, x);

		/* Redraw */
		lite_spot(y, x);
	}

	/* None left */
	temp_n = 0;
}



/*
 * This routine clears the entire "temp" set.
 *
 * This routine will "darken" all "temp" grids.
 *
 * In addition, some of these grids will be "unmarked".
 *
 * This routine is used (only) by "unlite_room()"
 *
 * Also, process all affected monsters
 */
static void cave_temp_room_unlite(void)
{
	int i;

	/* Clear them all */
	for (i = 0; i < temp_n; i++)
	{
		int y = temp_y[i];
		int x = temp_x[i];

		cave_type *c_ptr = &cave[y][x];

		/* No longer in the array */
		c_ptr->info &= ~(CAVE_TEMP);

		/* Darken the grid */
		c_ptr->info &= ~(CAVE_GLOW);

		/* Hack -- Forget "boring" grids */
		if (c_ptr->feat == FEAT_FLOOR)
		{
			/* Forget the grid */
			c_ptr->info &= ~(CAVE_MARK);

			/* Notice */
			note_spot(y, x);
		}

		/* Process affected monsters */
		if (c_ptr->m_idx)
		{
			/* Update the monster */
			update_mon(c_ptr->m_idx, FALSE);
		}

		/* Redraw */
		lite_spot(y, x);
	}

	/* None left */
	temp_n = 0;
}




/*
 * Aux function -- see below
 */
static void cave_temp_room_aux(int y, int x)
{
	cave_type *c_ptr = &cave[y][x];

	/* Avoid infinite recursion */
	if (c_ptr->info & (CAVE_TEMP)) return;

	/* Do not "leave" the current room */
	if (!(c_ptr->info & (CAVE_ROOM))) return;

	/* Paranoia -- verify space */
	if (temp_n == TEMP_MAX) return;

	/* Mark the grid as "seen" */
	c_ptr->info |= (CAVE_TEMP);

	/* Add it to the "seen" set */
	temp_y[temp_n] = y;
	temp_x[temp_n] = x;
	temp_n++;
}




/*
 * Illuminate any room containing the given location.
 */
void lite_room(int y1, int x1)
{
	int i, x, y;

	/* Add the initial grid */
	cave_temp_room_aux(y1, x1);

	/* While grids are in the queue, add their neighbors */
	for (i = 0; i < temp_n; i++)
	{
		x = temp_x[i], y = temp_y[i];

		/* Walls get lit, but stop light */
		if (!cave_floor_bold(y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(y + 1, x);
		cave_temp_room_aux(y - 1, x);
		cave_temp_room_aux(y, x + 1);
		cave_temp_room_aux(y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(y + 1, x + 1);
		cave_temp_room_aux(y - 1, x - 1);
		cave_temp_room_aux(y - 1, x + 1);
		cave_temp_room_aux(y + 1, x - 1);
	}

	/* Now, lite them all up at once */
	cave_temp_room_lite();
}


/*
 * Darken all rooms containing the given location
 */
void unlite_room(int y1, int x1)
{
	int i, x, y;

	/* Add the initial grid */
	cave_temp_room_aux(y1, x1);

	/* Spread, breadth first */
	for (i = 0; i < temp_n; i++)
	{
		x = temp_x[i], y = temp_y[i];

		/* Walls get dark, but stop darkness */
		if (!cave_floor_bold(y, x)) continue;

		/* Spread adjacent */
		cave_temp_room_aux(y + 1, x);
		cave_temp_room_aux(y - 1, x);
		cave_temp_room_aux(y, x + 1);
		cave_temp_room_aux(y, x - 1);

		/* Spread diagonal */
		cave_temp_room_aux(y + 1, x + 1);
		cave_temp_room_aux(y - 1, x - 1);
		cave_temp_room_aux(y - 1, x + 1);
		cave_temp_room_aux(y + 1, x - 1);
	}

	/* Now, darken them all at once */
	cave_temp_room_unlite();
}



/*
 * Hack -- call light around the player
 * Affect all monsters in the projection radius
 */
bool lite_area(s32b dam, int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hack -- Message */
	if (!p_ptr->blind)
	{
		msg_print("You are surrounded by a white light.");
	}

	/* Hook into the "project()" function */
	(void)project(0, rad, py, px, dam, GF_LITE_WEAK, flg);

	/* Lite up the room */
	lite_room(py, px);

	/* Assume seen */
	return (TRUE);
}


/*
 * Hack -- call darkness around the player
 * Affect all monsters in the projection radius
 */
bool unlite_area(s32b dam, int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hack -- Message */
	if (!p_ptr->blind)
	{
		msg_print("Darkness surrounds you.");
	}

	/* Hook into the "project()" function */
        (void)project(0, rad, py, px, dam, GF_DARK_WEAK, flg);

	/* Lite up the room */
        unlite_room(py, px);


	/* Assume seen */
	return (TRUE);
}

bool dark_lord_aura(s32b dam, int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        int x;
        s32b dambonus = 0;
        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Hook into the "project()" function */
        (void)project(0, rad, py, px, 0, GF_LOWER_POWER, flg);
        (void)project(0, rad, py, px, 0, GF_LOWER_MAGIC, flg);
        (void)project(0, rad, py, px, 0, GF_SLOW_DOWN, flg);
        (void)project(0, rad, py, px, dam, GF_MANA, flg);

	/* Assume seen */
	return (TRUE);
}

bool valkyrie_aura(s32b dam, int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        int x;
        s32b dambonus = 0;
        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Hook into the "project()" function */
        (void)project(0, rad, py, px, dam, GF_LITE, flg);

	/* Assume seen */
	return (TRUE);
}

bool elem_lord_aura(s32b dam, int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        int x;
        s32b dambonus = 0;

        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Hook into the "project()" function */
        no_magic_return = TRUE;
        (void)project(0, rad, py, px, dam, p_ptr->elemlord, flg);
        no_magic_return = FALSE;

	/* Assume seen */
	return (TRUE);
}

bool energy_spin()
{
	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hook into the "project()" function */
        (void)project(0, 3, py, px, (p_ptr->to_d / 2), GF_MANA, flg);
        update_and_handle();

	/* Assume seen */
	return (TRUE);
}

bool firelord_fireaura()
{
	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hook into the "project()" function */
        (void)project(0, 3, py, px, (p_ptr->chp / 2), GF_FIRE, flg);
        p_ptr->chp -= p_ptr->chp / 3;
        update_and_handle();

	/* Assume seen */
	return (TRUE);
}

bool circle_of_force()
{
	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hook into the "project()" function */
        (void)project(0, 3, py, px, (p_ptr->to_d * 5), GF_MANA, flg);
        p_ptr->chp -= p_ptr->chp / 2;
        update_and_handle();

	/* Assume seen */
	return (TRUE);
}


bool mega_lite()
{
	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hook into the "project()" function */
        (void)project(0, 100, py, px, 0, GF_LITE_WEAK, flg);

	/* Assume seen */
	return (TRUE);
}

bool attack_aura(int typ, s32b dam, int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        int x;
        s32b dambonus = 0;
        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Hook into the "project()" function */
        (void)project(0, rad, py, px, dam, typ, flg);

	/* Assume seen */
	return (TRUE);
}

/* Does physical-type damages */
bool spin_kick(s32b dam, int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;

        if (dam < 0) dam = 0;
        
	/* Hook into the "project()" function */
        (void)project(0, rad, py, px, dam, GF_PHYSICAL, flg);

	/* Assume seen */
	return (TRUE);
}

/* Warrior's Spin Attack */
/* Was once a samurai ability, now it's a warrior ability. */
bool spin_attack()
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        s32b dam;
        object_type *o_ptr = &inventory[INVEN_WIELD];

        if (!o_ptr)
        {
                msg_print("You must have a weapon!");
                return (TRUE);
        }
        
	call_lua("weapon_damages", "", "l", &dam);
        dam += ((dam * ((p_ptr->abilities[(CLASS_WARRIOR * 10)] - 1) * 20)) / 100);
        if (dam < 0) dam = 0;

	/* Hook into the "project()" function */
        (void)project(0, 1, py, px, dam, GF_PHYSICAL, flg);

	/* Assume seen */
	return (TRUE);
}
/* Ice Lord's Spin Attack */
/* Similar to the previous one, but it's a cold attack, not physical. */
/* Also a little more powerful */
bool glacial_spin()
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        s32b dam;
        object_type *o_ptr = &inventory[INVEN_WIELD];

        if (!o_ptr)
        {
                msg_print("You must have a weapon!");
                return (TRUE);
        }
        
	call_lua("weapon_damages", "", "l", &dam);
        dam *= 4;
        if (dam < 0) dam = 0;

	/* Hook into the "project()" function */
        (void)project(0, 1, py, px, dam, GF_COLD, flg);

	/* Assume seen */
	return (TRUE);
}

/* Sword spin feat! */
/* Gained with swords skill >= 20 */
bool sword_spin()
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        s32b dam;
        object_type *o_ptr = &inventory[INVEN_WIELD];

        if (!o_ptr)
        {
                msg_print("You must have a sword!");
                return (TRUE);
        }
        
	call_lua("weapon_damages", "", "l", &dam);
        dam += (dam / 2);
        if (dam < 0) dam = 0;

	/* Hook into the "project()" function */
        (void)project(0, 1, py, px, dam, GF_PHYSICAL, flg);

	/* Assume seen */
	return (TRUE);
}



/*
 * Cast a ball spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_ball(int typ, int dir, s32b dam, int rad)
{
        int tx, ty, x;

	int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        s32b dambonus = 0;

	/* Use the given direction */
	tx = px + 99 * ddx[dir];
	ty = py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		flg &= ~(PROJECT_STOP);
		tx = target_col;
		ty = target_row;
	}
        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
        return (project(0, (rad > 16)?16:rad, ty, tx, dam, typ, flg));
}

/*
 * Cast a druidistic ball spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_druid_ball(int typ, int dir, s32b dam, int rad)
{
        int tx, ty, x;

        int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_MANA_PATH;
        s32b dambonus = 0;

	/* Use the given direction */
	tx = px + 99 * ddx[dir];
	ty = py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		flg &= ~(PROJECT_STOP);
		tx = target_col;
		ty = target_row;
	}
        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
        return (project(0, (rad > 16)?16:rad, ty, tx, dam, typ, flg));
}


/*
 * Cast a ball-beamed spell
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 */
bool fire_ball_beam(int typ, int dir, s32b dam, int rad)
{
        int tx, ty, x;

        int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_BEAM;
        s32b dambonus = 0;

	/* Use the given direction */
	tx = px + 99 * ddx[dir];
	ty = py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		flg &= ~(PROJECT_STOP);
		tx = target_col;
		ty = target_row;
	}
        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
        return (project(0, (rad > 16)?16:rad, ty, tx, dam, typ, flg));
}


void teleport_swap(int dir)
{
	int tx, ty;
	cave_type * c_ptr;
	monster_type * m_ptr;
	monster_race * r_ptr;

	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}
	else
	{
		tx = px + ddx[dir];
		ty = py + ddy[dir];
	}
	c_ptr = &cave[ty][tx];
	
	if (!c_ptr->m_idx)
	{
		msg_print("You can't trade places with that!");
	}
	else
	{
		m_ptr = &m_list[c_ptr->m_idx];
		r_ptr = &r_info[m_ptr->r_idx];
		
		if (r_ptr->flags3 & RF3_RES_TELE)
		{
			msg_print("Your teleportation is blocked!");
		}
		else
		{
			sound(SOUND_TELEPORT);
			
			cave[py][px].m_idx = c_ptr->m_idx;
			
			/* Update the old location */
			c_ptr->m_idx = 0;
			
			/* Move the monster */
			m_ptr->fy = py;
			m_ptr->fx = px;
			
			/* Move the player */
			px = tx;
			py = ty;
			
			tx = m_ptr->fx;
			ty = m_ptr->fy;
			
			/* Update the monster (new location) */
			update_mon(cave[ty][tx].m_idx, TRUE);
			
			/* Redraw the old grid */
			lite_spot(ty, tx);
			
			/* Redraw the new grid */
			lite_spot(py, px);                        
			
			/* Check for new panel (redraw map) */
			verify_panel();
			
			/* Update stuff */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
			
			/* Update the monsters */
			p_ptr->update |= (PU_DISTANCE);
			
			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD);
			
			/* Handle stuff XXX XXX XXX */
			handle_stuff();
		}
	}
}

void swap_position(int lty, int ltx)
{
        int tx = ltx, ty = lty;
	cave_type * c_ptr;
	monster_type * m_ptr;
	monster_race * r_ptr;
	
	c_ptr = &cave[ty][tx];
	
	if (!c_ptr->m_idx)
	{
			sound(SOUND_TELEPORT);
			
                        /* Keep trace of the old location */
                        tx = px;
                        ty = py;
			
			/* Move the player */
                        px = ltx;
                        py = lty;
			
			/* Redraw the old grid */
			lite_spot(ty, tx);
			
			/* Redraw the new grid */
			lite_spot(py, px);
			
			/* Check for new panel (redraw map) */
			verify_panel();
			
			/* Update stuff */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
			
			/* Update the monsters */
			p_ptr->update |= (PU_DISTANCE);
			
			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD);
			
			/* Handle stuff XXX XXX XXX */
			handle_stuff();
	}
	else
	{
		m_ptr = &m_list[c_ptr->m_idx];
		r_ptr = &r_info[m_ptr->r_idx];
		
			sound(SOUND_TELEPORT);
			
			cave[py][px].m_idx = c_ptr->m_idx;
			
			/* Update the old location */
			c_ptr->m_idx = 0;
			
			/* Move the monster */
			m_ptr->fy = py;
			m_ptr->fx = px;
			
			/* Move the player */
			px = tx;
			py = ty;
			
			tx = m_ptr->fx;
			ty = m_ptr->fy;
			
			/* Update the monster (new location) */
			update_mon(cave[ty][tx].m_idx, TRUE);
			
			/* Redraw the old grid */
			lite_spot(ty, tx);
			
			/* Redraw the new grid */
			lite_spot(py, px);
			
			/* Check for new panel (redraw map) */
			verify_panel();
			
			/* Update stuff */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
			
			/* Update the monsters */
			p_ptr->update |= (PU_DISTANCE);
			
			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD);
			
			/* Handle stuff XXX XXX XXX */
			handle_stuff();
	}
}


/*
 * Hack -- apply a "projection()" in a direction (or at the target)
 */
bool project_hook(int typ, int dir, s32b dam, int flg)
{
	int tx, ty;

	/* Pass through the target if needed */
	flg |= (PROJECT_THRU);

	/* Use the given direction */
	tx = px + ddx[dir];
	ty = py + ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target", do NOT explode */
	return (project(0, 0, ty, tx, dam, typ, flg));
}


/*
 * Cast a bolt spell
 * Stop if we hit a monster, as a "bolt"
 * Affect monsters (not grids or objects)
 */
bool fire_bolt(int typ, int dir, s32b dam)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
        int x;
        s32b dambonus = 0;

        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        dam += dambonus;
	return (project_hook(typ, dir, dam, flg));
}

/*
 * Cast a druidistic bolt spell
 * Stop if we hit a monster, as a "bolt"
 * Affect monsters (not grids or objects)
 */
bool fire_druid_bolt(int typ, int dir, s32b dam)
{
        int flg = PROJECT_STOP | PROJECT_KILL | PROJECT_MANA_PATH;
        int x;
        s32b dambonus = 0;

        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        dam += dambonus;
	return (project_hook(typ, dir, dam, flg));
}


/*
 * Cast a druidistic beam spell
 * Pass through monsters, as a "beam"
 * Affect monsters (not grids or objects)
 */
bool fire_druid_beam(int typ, int dir, s32b dam)
{
        int flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_MANA_PATH;
        int x;
        s32b dambonus = 0;

        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        dam += dambonus;
	return (project_hook(typ, dir, dam, flg));
}

/*
 * Cast a beam spell
 * Pass through monsters, as a "beam"
 * Affect monsters (not grids or objects)
 */
bool fire_beam(int typ, int dir, s32b dam)
{
	int flg = PROJECT_BEAM | PROJECT_KILL;
        int x;
        s32b dambonus = 0;

        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        dam += dambonus;
	return (project_hook(typ, dir, dam, flg));
}


/*
 * Cast a bolt spell, or rarely, a beam spell
 */
bool fire_bolt_or_beam(int prob, int typ, int dir, s32b dam)
{
	if (rand_int(100) < prob)
	{
		return (fire_beam(typ, dir, dam));
	}
	else
	{
		return (fire_bolt(typ, dir, dam));
	}
}

bool fire_godly_wrath(int y, int x, int typ, int rad, s32b dam) {
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

  return (project(0, rad, y, x, dam, typ, flg));
}

bool fire_explosion(int y, int x, int typ, int rad, s32b dam) {
  int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

  return (project(0, rad, y, x, dam, typ, flg));
}

/*
 * Some of the old functions
 */
bool lite_line(int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
	return (project_hook(GF_LITE_WEAK, dir, damroll(6, 8), flg));
}


bool drain_life(int dir, s32b dam)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_DRAIN, dir, dam, flg));
}


bool wall_to_mud(int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
	return (project_hook(GF_KILL_WALL, dir, 20 + randint(30), flg));
}


bool wizard_lock(int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    return (project_hook(GF_JAM_DOOR, dir, 20 + randint(30), flg));
}


bool destroy_door(int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(GF_KILL_DOOR, dir, 0, flg));
}


bool disarm_trap(int dir)
{
	int flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(GF_KILL_TRAP, dir, 0, flg));
}


bool heal_monster(int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_HEAL, dir, damroll(4, 6), flg));
}


bool speed_monster(int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_SPEED, dir, p_ptr->lev, flg));
}


bool slow_monster(int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_SLOW, dir, p_ptr->lev, flg));
}


bool sleep_monster(int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_SLEEP, dir, p_ptr->lev, flg));
}


bool stasis_monster(int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_STASIS, dir, p_ptr->lev, flg));
}


bool confuse_monster(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_CONF, dir, plev, flg));
}


bool stun_monster(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_STUN, dir, plev, flg));
}


bool poly_monster(int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_POLY, dir, p_ptr->lev, flg));
}


bool clone_monster(int dir)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_OLD_CLONE, dir, 0, flg));
}


bool fear_monster(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_TURN_ALL, dir, plev, flg));
}


bool death_ray(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_DEATH_RAY, dir, plev, flg));
}


bool teleport_monster(int dir)
{
	int flg = PROJECT_BEAM | PROJECT_KILL;

	return (project_hook(GF_WARP, dir, MAX_SIGHT * 5, flg));
}


/*
 * Hooks -- affect adjacent grids (radius 1 ball attack)
 */
bool door_creation(void)
{
	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, py, px, 0, GF_MAKE_DOOR, flg));
}


bool trap_creation(void)
{
	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, py, px, 0, GF_MAKE_TRAP, flg));
}


bool glyph_creation(void)
{
	int flg = PROJECT_GRID | PROJECT_ITEM;
	return (project(0, 1, py, px, 0, GF_MAKE_GLYPH, flg));
}


bool wall_stone(void)
{
	int flg = PROJECT_GRID | PROJECT_ITEM;

	bool dummy = (project(0, 1, py, px, 0, GF_STONE_WALL, flg));

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	return dummy;
}


bool destroy_doors_touch(void)
{
	int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, py, px, 0, GF_KILL_DOOR, flg));
}


bool sleep_monsters_touch(void)
{
	int flg = PROJECT_KILL | PROJECT_HIDE;
	return (project(0, 1, py, px, p_ptr->lev, GF_OLD_SLEEP, flg));
}


/*
 * Activate the evil Topi Ylinen curse
 * rr9: Stop the nasty things when a Cyberdemon is summoned
 * or the player gets paralyzed.
 */
void activate_ty_curse(void)
{
	int     i = 0;
	bool    stop_ty = FALSE;

	do
	{
		switch(randint(27))
		{
		case 1: case 2: case 3: case 16: case 17:
			aggravate_monsters(1);
			if (randint(6) != 1) break;
		case 4: case 5: case 6:
			activate_hi_summon();
			if (randint(6) != 1) break;
		case 7: case 8: case 9: case 18:
			(void) summon_specific(py, px, dun_level, 0, 0);
			if (randint(6) != 1) break;
		case 10: case 11: case 12:
			msg_print("You feel your life draining away...");
			lose_exp(p_ptr->exp / 16);
			if (randint(6) != 1) break;
		case 13: case 14: case 15: case 19: case 20:
                        if (p_ptr->free_act && (randint(100) < p_ptr->stat_ind[A_WIS]))
			{
				/* Do nothing */ ;
			}
			else
			{
				msg_print("You feel like a statue!");
				if (p_ptr->free_act)
					set_paralyzed (p_ptr->paralyzed + randint(3));
				else
					set_paralyzed (p_ptr->paralyzed + randint(13));
				stop_ty = TRUE;
			}
			if (randint(6) != 1) break;
		case 21: case 22: case 23:
			(void)do_dec_stat((randint(6))-1, STAT_DEC_NORMAL);
			if (randint(6) != 1) break;
		case 24:
			msg_print("Huh? Who am I? What am I doing here?");
			lose_all_info();
			break;
		case 25:
			/*
			 * Only summon Cyberdemons deep in the dungeon.
			 */
			if ((dun_level > 65) && !stop_ty)
			{
				summon_cyber();
				stop_ty = TRUE;
				break;
			}
		default:
			while (i<6)
			{
				do
				{
					(void)do_dec_stat(i, STAT_DEC_NORMAL);
				}
				while (randint(2)==1);

				i++;
			}
		}
	}
	while ((randint(3) == 1) && !stop_ty);
}

/*
 * Activate the ultra evil Dark God curse
 */
void activate_dg_curse(void)
{
	int     i = 0;
        bool    stop_dg = FALSE;

	do
	{
                switch(randint(30))
		{
		case 1: case 2: case 3: case 16: case 17:
			aggravate_monsters(1);
                        if (randint(8) != 1) break;
		case 4: case 5: case 6:
                        msg_print("Oh ! You feel that the curse is replicating itself!");
                        curse_equipment_dg(100, 50 * randint(2));
                        if (randint(8) != 1) break;
		case 7: case 8: case 9: case 18:
                        curse_equipment(100, 50 * randint(2));
                        if (randint(8) != 1) break;
		case 10: case 11: case 12:
			msg_print("You feel your life draining away...");
                        lose_exp(p_ptr->exp / 12);
                        if (randint(8) != 1) break;
                case 13: case 14: case 15:
                        if (p_ptr->free_act && (randint(100) < p_ptr->stat_ind[A_WIS]))
			{
				/* Do nothing */ ;
			}
			else
			{
				msg_print("You feel like a statue!");
				if (p_ptr->free_act)
					set_paralyzed (p_ptr->paralyzed + randint(3));
				else
					set_paralyzed (p_ptr->paralyzed + randint(13));
                                stop_dg = TRUE;
			}
                        if (randint(7) != 1) break;
                case 19: case 20:
			{
                                msg_print("Wohhh! you see 10 little Morgoths dancing before you!");
                                set_confused(p_ptr->confused + randint(13 * 2));
                                if(rand_int(2)) stop_dg = TRUE;
			}
                        if (randint(7) != 1) break;
		case 21: case 22: case 23:
                        (void)do_dec_stat((randint(6))-1, STAT_DEC_PERMANENT);
                        if (randint(7) != 1) break;
		case 24:
			msg_print("Huh? Who am I? What am I doing here?");
			lose_all_info();
			break;
                case 27: case 28: case 29:
                        if(inventory[INVEN_WIELD].k_idx)
                        {
                                msg_print("Your weapon now seems useless...");
                                inventory[INVEN_WIELD].art_flags4 = TR4_NEVER_BLOW;
                        }
                        break;
		case 25:
			/*
                         * Only summon Dragon Riders not too shallow in the dungeon.
			 */
                        if ((dun_level > 25) && !stop_dg)
			{
                                msg_print("Oh! You attracted some evil DragonRiders!");
                                summon_dragon_riders();

                                /* This is evil -- DG */
                                if(rand_int(2)) stop_dg = TRUE;
				break;
			}
		default:
			while (i<6)
			{
				do
				{
                                        (void)do_dec_stat(i, STAT_DEC_NORMAL);
				}
				while (randint(2)==1);

				i++;
			}
		}
	}
        while ((randint(4) == 1) && !stop_dg);
}


void activate_hi_summon(void)
{
	int i;

	for (i = 0; i < (randint(9) + (dun_level / 40)); i++)
	{
		switch(randint(26) + (dun_level / 20) )
		{
			case 1: case 2:
				(void) summon_specific(py, px, dun_level, SUMMON_ANT, 0);
				break;
			case 3: case 4:
				(void) summon_specific(py, px, dun_level, SUMMON_SPIDER, 0);
				break;
			case 5: case 6:
				(void) summon_specific(py, px, dun_level, SUMMON_HOUND, 0);
				break;
			case 7: case 8:
				(void) summon_specific(py, px, dun_level, SUMMON_HYDRA, 0);
				break;
			case 9: case 10:
				(void) summon_specific(py, px, dun_level, SUMMON_ANGEL, 0);
				break;
			case 11: case 12:
				(void) summon_specific(py, px, dun_level, SUMMON_UNDEAD, 0);
				break;
			case 13: case 14:
				(void) summon_specific(py, px, dun_level, SUMMON_DRAGON, 0);
				break;
			case 15: case 16:
				(void) summon_specific(py, px, dun_level, SUMMON_DEMON, 0);
				break;
			case 17:
				(void) summon_specific(py, px, dun_level, SUMMON_WRAITH, 0);
				break;
			case 18: case 19:
				(void) summon_specific(py, px, dun_level, SUMMON_UNIQUE, 0);
				break;
			case 20: case 21:
				(void) summon_specific(py, px, dun_level, SUMMON_HI_UNDEAD, 0);
				break;
			case 22: case 23:
				(void) summon_specific(py, px, dun_level, SUMMON_HI_DRAGON, 0);
				break;
			case 24: case 25:
				(void) summon_specific(py, px, 100, SUMMON_CYBER, 0);
				break;
			default:
				(void) summon_specific(py, px,(((dun_level * 3) / 2) + 5), 0, 0);
		}
	}
}


void summon_cyber(void)
{
	int i;
	int max_cyber = (dun_level / 50) + randint(6);

	for (i = 0; i < max_cyber; i++)
	{
		(void)summon_specific(py, px, 100, SUMMON_CYBER, 0);
	}
}

void summon_dragon_riders()
{
	int i;
        int max_dr = (dun_level / 50) + randint(6);

        for (i = 0; i < max_dr; i++)
	{
                (void)summon_specific(py, px, 100, SUMMON_DRAGONRIDER, 0);
	}
}


void wall_breaker(void)
{
	int dummy = 5;

	if (randint(80 + p_ptr->lev) < 70)
	{
		do
		{
			dummy = randint(9);
		}
		while ((dummy == 5) || (dummy == 0));

		wall_to_mud (dummy);
	}
	else if (randint(100) > 30)
	{
		/* Prevent destruction of quest levels and town */
		if (!is_quest(dun_level) && dun_level)
			earthquake(py, px, 1);
	}
	else
	{
		for (dummy = 1; dummy < 10; dummy++)
		{
			if (dummy - 5) wall_to_mud(dummy);
		}
	}
}


void bless_weapon(void)
{
	int             item;
	object_type     *o_ptr;
        u32b            f1, f2, f3, f4;
	char            o_name[80];
	cptr            q, s;

	/* Assume enchant weapon */
	item_tester_hook = item_tester_hook_weapon;

	/* Get an item */
	q = "Bless which weapon? ";
	s = "You have weapon to bless.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}


	/* Description */
	object_desc(o_name, o_ptr, FALSE, 0);

        /* Extract the flags */
        object_flags(o_ptr, &f1, &f2, &f3, &f4);

	if (o_ptr->ident & (IDENT_CURSED))
	{

		if (((f3 & (TR3_HEAVY_CURSE)) && (randint (100) < 33)) ||
		    (f3 & (TR3_PERMA_CURSE)))
		{

			msg_format("The black aura on %s %s disrupts the blessing!",
			    ((item >= 0) ? "your" : "the"), o_name);
			return;
		}

		msg_format("A malignant aura leaves %s %s.",
		    ((item >= 0) ? "your" : "the"), o_name);

		/* Uncurse it */
		o_ptr->ident &= ~(IDENT_CURSED);

		/* Hack -- Assume felt */
		o_ptr->ident |= (IDENT_SENSE);

		/* Take note */
		o_ptr->note = quark_add("uncursed");

		/* Recalculate the bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Window stuff */
		p_ptr->window |= (PW_EQUIP);
	}

	/*
	 * Next, we try to bless it. Artifacts have a 1/3 chance of
	 * being blessed, otherwise, the operation simply disenchants
	 * them, godly power negating the magic. Ok, the explanation
	 * is silly, but otherwise priests would always bless every
	 * artifact weapon they find. Ego weapons and normal weapons
	 * can be blessed automatically.
	 */
	if (f3 & TR3_BLESSED)
	{
		msg_format("%s %s %s blessed already.",
		    ((item >= 0) ? "Your" : "The"), o_name,
		    ((o_ptr->number > 1) ? "were" : "was"));
		return;
	}

	if (!(o_ptr->art_name || o_ptr->name1) || (randint(3) == 1))
	{
		/* Describe */
		msg_format("%s %s shine%s!",
		    ((item >= 0) ? "Your" : "The"), o_name,
		    ((o_ptr->number > 1) ? "" : "s"));
		o_ptr->art_flags3 |= TR3_BLESSED;
	}
	else
	{
		bool dis_happened = FALSE;

		msg_print("The artifact resists your blessing!");

		/* Disenchant tohit */
		if (o_ptr->to_h > 0)
		{
			o_ptr->to_h--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_h > 5) && (rand_int(100) < 33)) o_ptr->to_h--;

		/* Disenchant todam */
		if (o_ptr->to_d > 0)
		{
			o_ptr->to_d--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_d > 5) && (rand_int(100) < 33)) o_ptr->to_d--;

		/* Disenchant toac */
		if (o_ptr->to_a > 0)
		{
			o_ptr->to_a--;
			dis_happened = TRUE;
		}

		if ((o_ptr->to_a > 5) && (rand_int(100) < 33)) o_ptr->to_a--;

		if (dis_happened)
		{
			msg_print("There is a static feeling in the air...");
			msg_format("%s %s %s disenchanted!",
			    ((item >= 0) ? "Your" : "The"), o_name,
			    ((o_ptr->number > 1) ? "were" : "was"));
		}
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER);
}


/*
 * Detect all "nonliving", "undead" or "demonic" monsters on current panel
 */
bool detect_monsters_nonliving(void)
{
	int     i, y, x;
	bool    flag = FALSE;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (!panel_contains(y, x)) continue;

		/* Detect evil monsters */
		if ((r_ptr->flags3 & (RF3_NONLIVING)) ||
		    (r_ptr->flags3 & (RF3_UNDEAD)) ||
		    (r_ptr->flags3 & (RF3_DEMON)))
		{
			/* Update monster recall window */
			if (monster_race_idx == m_ptr->r_idx)
			{
				/* Window stuff */
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag |= (MFLAG_MARK | MFLAG_SHOW);

			/* Hack -- See monster */
			m_ptr->ml = TRUE;

			/* Redraw */
			lite_spot(y, x);

			/* Detect */
			flag = TRUE;
		}
	}

	/* Describe */
	if (flag)
	{
		/* Describe result */
		msg_print("You sense the presence of unnatural beings!");
	}

	/* Result */
	return (flag);
}


/*
 * Confuse monsters
 */
bool confuse_monsters(s32b dam)
{
	return (project_hack(GF_OLD_CONF, dam));
}


/*
 * Charm monsters
 */
bool charm_monsters(s32b dam)
{
	return (project_hack(GF_CHARM, dam));
}


/*
 * Charm animals
 */
bool charm_animals(s32b dam)
{
	return (project_hack(GF_CONTROL_ANIMAL, dam));
}


/*
 * Stun monsters
 */
bool stun_monsters(s32b dam)
{
	return (project_hack(GF_STUN, dam));
}


/*
 * Stasis monsters
 */
bool stasis_monsters(s32b dam)
{
	return (project_hack(GF_STASIS, dam));
}


/*
 * Mindblast monsters
 */
bool mindblast_monsters(s32b dam)
{
	return (project_hack(GF_PSI, dam));
}


/*
 * Banish all monsters
 */
bool banish_monsters(int dist)
{
	return (project_hack(GF_WARP, dist));
}


/*
 * Turn evil
 */
bool turn_evil(s32b dam)
{
	return (project_hack(GF_TURN_EVIL, dam));
}


/*
 * Turn everyone
 */
bool turn_monsters(s32b dam)
{
	return (project_hack(GF_TURN_ALL, dam));
}


/*
 * Death-ray all monsters (note: OBSCENELY powerful)
 */
bool deathray_monsters(void)
{
	return (project_hack(GF_DEATH_RAY, p_ptr->lev));
}


bool charm_monster(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_CHARM, dir, plev, flg));
}

bool star_charm_monster(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
        return (project_hook(GF_STAR_CHARM, dir, plev, flg));
}


bool control_one_undead(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_CONTROL_UNDEAD, dir, plev, flg));
}


bool charm_animal(int dir, int plev)
{
	int flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_CONTROL_ANIMAL, dir, plev, flg));
}

void change_wild_mode(void)
{
        /*
         * A mold can't go into small scale mode, it's impossible for me to find
         * a good way to handle blinking on such a small map
         */
                p_ptr->wild_mode = !p_ptr->wild_mode;

                if (autosave_l)
                {
                        is_autosave = TRUE;
                        msg_print("Autosaving the game...");
                        do_cmd_save_game();
                        is_autosave = FALSE;
                }

                /* Leaving */
                p_ptr->leaving = TRUE;
}


void alter_reality(void)
{
	msg_print("The world changes!");

	if (autosave_l)
	{
		is_autosave = TRUE;
		msg_print("Autosaving the game...");
		do_cmd_save_game();
		is_autosave = FALSE;
	}

	/* Leaving */
	p_ptr->leaving = TRUE;
}

/*
 * Helper function for passwall()
 *
 * Handles moving the player to the final location, and
 * what happens when he gets there.
 */
static void passwall_finish(int y, int x, bool safe)
{
	byte feat;
	bool door;

	/* Did the player actually move? */
	if (py == y && px == x)
		return;

	if (!cave_floor_bold(y, x))
	{
                feat = cave[y][x].feat;
		door = ((feat >= FEAT_DOOR_HEAD && feat <= FEAT_DOOR_TAIL) ||
				feat == FEAT_SECRET);

		if (!safe)
		{
			if (!door)
			{
				msg_print("You emerge in the wall!");
				take_hit(damroll(10, 8), "becoming one with a wall");
				cave_set_feat(y, x, FEAT_FLOOR);
			}
			else
			{
				msg_print("You emerge in a door!");
				take_hit(damroll(5, 5), "becoming one with a door");
				cave_set_feat(y, x, FEAT_BROKEN);
			}
		}
		else
		{
			if (door)
				cave_set_feat(y, x, FEAT_OPEN);
			else
				cave_set_feat(y, x, FEAT_FLOOR);
		}
	}

        /* Move player */
        py = y;
        px = x;

	/* Take care of traps/objects/stores/etc */
	step_effects(y, x, always_pickup);
}

/*
 * Send the player shooting through walls in the given direction until
 * they reach a non-wall space, or a monster, or a permanent wall.
 */
bool passwall(int dir, bool safe, bool local)
{
	int y1, x1, y2, x2;
	int ty, tx, dy, dx;
        int oy, ox, ny = 1, nx = 1;

	int i, path_n = 0;

	/* We want to stop when we hit a monster */
        int flg = (PROJECT_WALL | PROJECT_STOP | PROJECT_KILL);

	/* Actual grids in the "path" */
	u16b path_g[512];

	bool in_wall = FALSE;

	/* Use the given direction */
	tx = px + 99 * ddx[dir];
	ty = py + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
                tx = target_col;
                ty = target_row;
	}

	/* In case we need to relocate the target */
	dy = ty - py;
	dx = tx - px;

	/* Starting location */
	y1 = py;
	x1 = px;

	/* Default "destination" */
	y2 = ty;
	x2 = tx;

	/* Make sure we're actually going somewhere */
	if (x1 == x2 && y1 == y2)
		return FALSE;

	/* Project until done */
	while (1)
	{
		/* Calculate the projection path */
		path_n = project_path(path_g, MAX_RANGE, y1, x1, y2, x2, flg);

		oy = y1;
		ox = x1;

		/* Project along the path */
		for (i = 0; i < path_n; i++)
		{
			ny = GRID_Y(path_g[i]);
			nx = GRID_X(path_g[i]);

			/* Stop if we tried to go through a monster or a 
			 * permanent wall */
                        if (cave[ny][nx].m_idx || (!cave_floor_bold(ny, nx) &&
									   cave_perma_bold(ny, nx)))
			{
				passwall_finish(oy, ox, safe);

                                /* Check for new panel (redraw map) */
                                verify_panel();

                                /* Update stuff */
                                p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_HP);

                                /* Update the monsters */
                                p_ptr->update |= (PU_DISTANCE);

                                /* Window stuff */
                                p_ptr->window |= (PW_OVERHEAD);
				return TRUE;
			}

			if (in_wall)
			{
				/* Stop when we reach an empty floor */
				if (cave_floor_bold(ny, nx))
				{
					passwall_finish(ny, nx, safe);

                                        /* Check for new panel (redraw map) */
                                        verify_panel();

                                        /* Update stuff */
                                        p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_HP);

                                        /* Update the monsters */
                                        p_ptr->update |= (PU_DISTANCE);

                                        /* Window stuff */
                                        p_ptr->window |= (PW_OVERHEAD);
					return TRUE;
				}
			}
			else
			{
				/* Check if we've entered the walls yet */
				if (!cave_floor_bold(ny, nx))
					in_wall = TRUE;
				else
				{
					/* Abort if we needed to start by a wall and didn't */
					if (local)
						return FALSE;
				}
			}

			oy = ny;
			ox = nx;
		}

		/* Continue */
		y1 = ny;
		x1 = nx;

		/* If we reached the target, but aren't done yet, move it. */
		if (y1 == y2 && x1 == x2)
		{
			y2 = y1 + dy;
			x2 = x1 + dx;
		}
	}
}

/* Target an attack from a friendly monster */
bool project_hook_pets(monster_type *m_ptr, int typ, int dir, s32b dam, int flg)
{
	int tx, ty;

	/* Pass through the target if needed */
	flg |= (PROJECT_THRU);

        msg_print("Target? ");
        if (!tgt_pt(&tx, &ty)) return FALSE;

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
        return (project(0, 0, ty, tx, dam, typ, flg));
}

bool fire_ball_pets(monster_type *m_ptr, int typ, s32b dam, int rad)
{
	int tx, ty;

        int flg = PROJECT_GRID | PROJECT_KILL;

        msg_print("Target? ");
        if (!tgt_pt(&tx, &ty)) return FALSE;

	/* Hook into the "project()" function */
        (void)project(0, rad, ty, tx, dam, typ, flg);

	/* Assume seen */
	return (TRUE);
}

bool fire_ball_spot(int tx, int ty, int typ, s32b dam, int rad)
{
        int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hook into the "project()" function */
        (void)project(0, rad, ty, tx, dam, typ, flg);

	/* Assume seen */
	return (TRUE);
}

/* NEW TO NEWANGBAND: CHAIN ATTACKS! */
bool chain_attack(int dir, int typ, s32b dam, int rad, int range)
{
        int flg = PROJECT_GRID | PROJECT_KILL;
        int x;
        s32b dambonus = 0;

        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (typ != GF_PHYSICAL && typ != GF_STEALTH_ATTACK && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        dam += dambonus;

        if (c_ptr->feat == 224)
        {
                msg_print("Can't use this while in a barrier shield!");
                return (TRUE);
        }
        if (dir == 1)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx -= 1;
                        tmpy += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 2)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpy += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 3)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx += 1;
                        tmpy += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 4)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 5)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < 0)
                {
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 6)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 7)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx -= 1;
                        tmpy -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 8)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpy -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 9)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx += 1;
                        tmpy -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }

        return (TRUE);
}

bool chain_attack_fields(int dir, int typ, s32b dam, int rad, int range, int fldtype, int fldam)
{
        int flg = PROJECT_GRID | PROJECT_KILL;
        int x;
        s32b dambonus = 0;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (typ != GF_PHYSICAL && typ != GF_LIFE_BLAST)
        {
                if (typ == GF_MISSILE)
                {
                        dambonus = (dam * (p_ptr->skill[1] * 5)) / 100;
                }
                else
                {
                        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;
                }
        }
        dam += dambonus;

        if (c_ptr->feat == 224)
        {
                msg_print("Can't use this while in a barrier shield!");
                return (TRUE);
        }
        if (dir == 1)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx -= 1;
                        tmpy += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }
        if (dir == 2)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpy += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }
        if (dir == 3)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx += 1;
                        tmpy += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }
        if (dir == 4)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }
        if (dir == 5)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < 0)
                {
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        tmpvar += 1;
                }
        }
        if (dir == 6)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx += 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }
        if (dir == 7)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx -= 1;
                        tmpy -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }
        if (dir == 8)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpy -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }
        if (dir == 9)
        {
                int tmpx = px;
                int tmpy = py;
                int tmpvar = 0;
                while (tmpvar < range)
                {
                        tmpx += 1;
                        tmpy -= 1;
                        c_ptr = &cave[tmpy][tmpx];
                        if (solid_block(c_ptr)) return (TRUE);
                        (void)project(0, rad, tmpy, tmpx, dam, typ, flg);
                        place_field(fldtype, rad, tmpx, tmpy, fldam);
                        tmpvar += 1;
                }
        }

        return (TRUE);
}

bool lava_burst()
{
        cave_type *c_ptr;
	int tx, ty;
        int typ = GF_METEOR;
        int rad = 3;
        s32b dam = 0;

        int flg = PROJECT_GRID | PROJECT_KILL;

        msg_print("Target? ");
        if (!tgt_pt(&tx, &ty)) return FALSE;

        c_ptr = &cave[ty][tx];
        if (c_ptr->feat == FEAT_SHAL_LAVA) dam = 7500;
        else if (c_ptr->feat == FEAT_DEEP_LAVA) dam = 20000;
        else
        {
                msg_print("You must target a space with lava!");
                return (FALSE);
        }

	/* Hook into the "project()" function */
        (void)project(0, rad, ty, tx, dam, typ, flg);

        /* Lava burst cost 20 mp! */
        p_ptr->csp -= 20;
        update_and_handle();

	/* Assume seen */
	return (TRUE);
}

/* New and useful Monk ability, but may also be used by other classes... */
void hard_kick(int dir, s32b dam, int range)
{
	int hit;
        int thecount = 0;
        int m_idx = 0;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx + 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx + 1, m_ptr->fy);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy - 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx, m_ptr->fy - 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx + 1, m_ptr->fy - 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }

        return (TRUE);
}
/* The Smash feat! */
bool smash(int dir, s32b dam, int range)
{
	int hit;
        int thecount = 0;
        int m_idx = 0;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				m_ptr->defense -= (m_ptr->defense / 10);
                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }

        return (TRUE);
}
/* The Dizzy Smash feat! */
bool dizzy_smash(int dir, s32b dam, int range)
{
	int hit;
	int ppower;
	int mpower;
        int thecount = 0;
        int m_idx = 0;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[13] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
				if (randint(ppower) >= randint(mpower))
				{
                        		if (m_ptr->boss <= 0 && !(r_ptr->flags1 & (RF1_UNIQUE)))
					{
						msg_print("Enemy has been confused!");
						m_ptr->confused = 5;
					}
				}

                        	/* Can't push away if dead */
                        	if (m_ptr->hp <= 0) return (TRUE);
                        	while (thecount < range)
                        	{
                                	thecount += 1;
                                	c_ptr = &cave[m_ptr->fy + thecount][m_ptr->fx - thecount];
                                	if (!c_ptr->m_idx && (c_ptr->feat == FEAT_FLOOR || c_ptr->feat == FEAT_SHAL_WATER ||
                                	c_ptr->feat == FEAT_DEEP_WATER || c_ptr->feat == FEAT_SHAL_LAVA || c_ptr->feat == FEAT_DEEP_LAVA ||
                                	c_ptr->feat == FEAT_GRASS))
                                	{       
                                        	move_monster_spot(m_idx, m_ptr->fx - 1, m_ptr->fy + 1);
                                	}
                                	else return (TRUE);
                        	}
			}
			else msg_print("You miss the monster.");
                        update_and_handle();
                }
        }

        return (TRUE);
}
/* The Shattering Blow feat! */
bool shattering_blow(int dir)
{
        int m_idx = 0;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        m_ptr->defense = 0;
                        msg_print("You give the monster a Shattering Blow!");
                        update_and_handle();
                }
        }

        return (TRUE);
}
/* The Power Punch feat! */
bool power_punch(int dir)
{
	int hit;
        int m_idx = 0;
	s32b mdam;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
	call_lua("monk_damages", "", "l", &mdam);
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_idx = c_ptr->m_idx;
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam * 2, 0);
				nevermiss = FALSE;
                        	m_ptr->defense -= (m_ptr->defense / 4);
                        	update_and_handle();
			}
                }
        }

        return (TRUE);
}
/* The Stunning Blow feat! */
bool stunning_blow(int dir)
{
	int hit;
        int m_idx = 0;
	int ppower = 0;
	int mpower = 0;
	s32b mdam;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
	call_lua("monk_damages", "", "l", &mdam);
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        m_idx = c_ptr->m_idx;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				ppower = p_ptr->skill[18] + p_ptr->stat_ind[A_STR];
				mpower = m_ptr->level + m_ptr->str;
                        	if (m_ptr->boss <= 0 && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & (RF3_NO_STUN)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
                                        	msg_print("You give the monster a Stunning Blow!");
                                        	m_ptr->seallight = 3;
					}
                        	}
				nevermiss = TRUE;
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, mdam, 0);
				nevermiss = FALSE;
                        	update_and_handle();
			}
                }
        }

        return (TRUE);
}
/* The Eye Stab feat! */
bool eye_stab(int dir)
{
	int hit;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				if (!(m_ptr->boss >= 1 || (r_ptr->flags1 & (RF1_UNIQUE))))
				{
                        		if (!(m_ptr->abilities & (EYE_STABBED)))
                        		{
                                		msg_print("You stab the monster's eyes!");
                                		m_ptr->hitrate -= (m_ptr->hitrate / 2);
                                		m_ptr->defense -= (m_ptr->defense / 4);
                                		m_ptr->abilities |= (EYE_STABBED);
                        		}
                        		else msg_print("You already blinded this monster!");
				}
				else msg_print("This monster is unaffected.");
			}
			else msg_print("You miss.");
                        update_and_handle();
                }
        }

        return (TRUE);
}

/* The Fatal Stab feat! */
/* Dangerous... */
bool fatal_stab(int dir, object_type *o_ptr)
{
	int hit;
        int percentage = 10;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	percentage += (o_ptr->pval * 10);
                                	if (percentage > 90) percentage = 90;
                                	if (((r_ptr->flags1 & (RF1_UNIQUE)) || m_ptr->boss >= 1) && percentage > 30) percentage = 30;
                        	}
                        	msg_print("You force the dagger into the monster's body!");
                        	m_ptr->hitrate -= ((m_ptr->hitrate * percentage) / 100);
                        	m_ptr->defense -= ((m_ptr->defense * percentage) / 100);
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        	{
                                	m_ptr->mspeed -= ((m_ptr->mspeed * percentage) / 100);
                        	}
                        	m_ptr->hp -= ((m_ptr->hp * percentage) / 100);
                        	update_and_handle();
			}
			else msg_print("You miss.");
                }
        }

        return (TRUE);
}

/* The Chop feat! */
bool axe_chop(int dir, s32b dam)
{
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        m_ptr->defense -= (m_ptr->defense / 4);
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }

        return (TRUE);
}

/* The Mutilate Legs feat! */
bool mutilate_legs(int dir, s32b dam)
{
	int hit;
	int ppower = 0;
	int mpower = 0;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_LEGS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }

        return (TRUE);
}

/* The Mutilate Arms feat! */
bool mutilate_arms(int dir, s32b dam)
{
	int hit;
	int ppower = 0;
	int mpower = 0;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			ppower = p_ptr->skill[16] + p_ptr->stat_ind[A_STR];
			mpower = m_ptr->level + m_ptr->str;
			call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
			{
				nevermiss = TRUE; 
                        	fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
				nevermiss = FALSE;
                        	if (m_ptr->boss < 1 && !(r_ptr->flags1 & (RF1_UNIQUE)))
                        	{
					if (randint(ppower) >= randint(mpower))
					{
						m_ptr->abilities |= (MUTILATE_ARMS);
						msg_print("You mutilate the enemy!");
					}
                        	}
			}
                        update_and_handle();
                }
        }

        return (TRUE);
}


/* Slice is a nice ability for the Samurai... */
bool samurai_slice(int dir)
{
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        slice_kill(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }

        return (TRUE);
}

/* The complement to samurai_slice */
void slice_kill(monster_type *m_ptr, int m_idx)
{
        int slicechance = 100;
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        slicechance -= m_ptr->level;
        if (slicechance < 15) slicechance = 15;
        if (randint(100) <= slicechance && !m_ptr->boss && !(r_ptr->flags1 & RF1_UNIQUE))
        {
                msg_print("You slice the monster in half!");
                m_ptr->hp = -1;
                mon_take_hit(m_idx, 1, FALSE, NULL);
        }
        else msg_print("Your slice didn't work.");
}

/* All characters may attempt this. */
/* Depending mostly on charisma and money, it may or may not work. */
bool hire_befriend(int dir)
{
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        makefriend(m_ptr, c_ptr->m_idx);
                }
                else msg_print("No monster here.");
        }

        return (TRUE);
}

/* The complement to hire_befriend */
void makefriend(monster_type *m_ptr, int m_idx)
{
        int recruitbonus = 1;
        int recruitprice = 0;
        char ch;
        monster_race *r_ptr = &r_info[m_ptr->r_idx];

        recruitprice = m_ptr->level * 100;

        if (p_ptr->skill[9] >= 50) recruitbonus += 1;

        /* Must have a minimal charisma... */
        /* Bosses cannot be befriended */
        if ((p_ptr->stat_ind[A_CHR] < m_ptr->level) || m_ptr->boss >= 1 || (r_ptr->flags1 & RF1_UNIQUE))
        {
                give_refusal_message();
                return;
        }
        /* Then, it's your charisma versus the monster's level */
        if (randint((p_ptr->stat_ind[A_CHR] * recruitbonus)) >= randint(m_ptr->level))
        {
                /* Let's see if your charisma is very high... */
                /* I mean, VERY high! */
                /* Your charisma must be real high to befriend high level monsters */
                /* this way! */
                if ((p_ptr->stat_ind[A_CHR] * recruitbonus) >= m_ptr->level * 3)
                {
                        give_approval_message();
                        set_pet(m_ptr, TRUE);
                        m_ptr->imprinted = 1;
                        m_ptr->friend = 1;
                }
                /* Otherwise, the monster will help you if you give cash... */
                else
                {
                        msg_format("I will help you for %d golds...", recruitprice);
                        get_com("Accept? [y/n] ", &ch);
                        if (ch == 'y' || ch == 'Y')
                        {
                                if (p_ptr->au >= recruitprice)
                                {
                                        msg_print("Okay, but don't piss me off!");
                                        p_ptr->au -= recruitprice;
                                        set_pet(m_ptr, TRUE);
                                }
                                else msg_print("You don't have enough money, you liar!");
                        }
                        else msg_print("Then you shall die!");
                }
        }
        else give_refusal_message();
        update_and_handle();
}

void give_refusal_message()
{
        int whichone = 0;
        whichone = randint(100);
        if (whichone >= 90) msg_print("I would rather kill you instead!");
        else if (whichone >= 80) msg_print("Killing you will be more fun!");
        else if (whichone >= 70) msg_print("Never will I betray Variaz...");
        else if (whichone >= 60) msg_print("It is YOU who should join us!");
        else if (whichone >= 50) msg_print("Forget it!");
        else if (whichone >= 40) msg_print("Friends? Are you mad or what?");
        else if (whichone >= 30) msg_print("Your cause is lost. No point in joining you!");
        else if (whichone >= 20) msg_print("I do not follow weaklings like you!");
        else if (whichone >= 10) msg_print("Yeah, like I would trust YOU!");
        else msg_print("Get out of my sight!");
}

void give_approval_message()
{
        int whichone = 0;
        whichone = randint(100);
        if (whichone >= 90) msg_print("You seems to be good for me...Let us be friends!");
        else if (whichone >= 80) msg_print("Let's be friends, and crush our foes!");
        else if (whichone >= 70) msg_print("Together we shall become powerful!");
        else if (whichone >= 60) msg_print("May our frienship be eternal!");
        else if (whichone >= 50) msg_print("Let's be friends and have fun!");
        else if (whichone >= 40) msg_print("All right, I will accompany you!");
        else if (whichone >= 30) msg_print("Together we shall defeat all enemies!");
        else if (whichone >= 20) msg_print("We're friends now! No one can defeat us!");
        else if (whichone >= 10) msg_print("Let's make peace and become friends!");
        else msg_print("Sure! I'l be glad to be your partner!");
}

bool solid_block(cave_type *c_ptr)
{
        if (c_ptr->feat == FEAT_WALL_EXTRA || c_ptr->feat == FEAT_WALL_INNER ||
        c_ptr->feat == FEAT_WALL_OUTER || c_ptr->feat == FEAT_WALL_SOLID ||
        c_ptr->feat == FEAT_PERM_EXTRA || c_ptr->feat == FEAT_PERM_INNER ||
        c_ptr->feat == FEAT_PERM_OUTER || c_ptr->feat == FEAT_PERM_SOLID ||
        c_ptr->feat == FEAT_MAGMA || c_ptr->feat == FEAT_QUARTZ ||
        c_ptr->feat == FEAT_MAGMA_H || c_ptr->feat == FEAT_QUARTZ_H ||
        c_ptr->feat == FEAT_MAGMA_K || c_ptr->feat == FEAT_QUARTZ_K ||
        c_ptr->feat == FEAT_TREES || c_ptr->feat == FEAT_MOUNTAIN ||
        c_ptr->feat == 0x20 || c_ptr->feat == 0x21 || c_ptr->feat == 0x22 ||
        c_ptr->feat == 0x23 || c_ptr->feat == 0x24 || c_ptr->feat == 0x25 ||
        c_ptr->feat == 0x26 || c_ptr->feat == 0x27 || c_ptr->feat == 0x28 ||
        c_ptr->feat == 0x29 || c_ptr->feat == 0x2A || c_ptr->feat == 0x2B ||
        c_ptr->feat == 0x2C || c_ptr->feat == 0x2D || c_ptr->feat == 0x2E ||
        c_ptr->feat == 0x2F || c_ptr->feat == 188) return (TRUE);

        else return (FALSE);
}

/* Based on Genocide, destroy *ALL* monsters!! */
/* Used when you defeat Variaz! */
void anihilate_monsters()
{
	int     i;
	bool    result = FALSE;
	int     msec = delay_factor * delay_factor * delay_factor;

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Delete the monster */
		delete_monster_idx(i);

		/* Visual feedback */
		move_cursor_relative(py, px);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);

		/* Take note */
		result = TRUE;
	}

	return;
}

/* Blood suck from Vampires! */
bool vampire_drain(int dir)
{
        cave_type *c_ptr;
        c_ptr = &cave[py][px];
        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");
                }
                else msg_print("No monster here.");
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");                        
                }
                else msg_print("No monster here.");
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");                        
                }
                else msg_print("No monster here.");
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");                        
                }
                else msg_print("No monster here.");
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");                        
                }
                else msg_print("No monster here.");
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");                        
                }
                else msg_print("No monster here.");
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");                        
                }
                else msg_print("No monster here.");
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        if (!(m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)))
                        {
                                msg_print("You bite the monster!");
                                nevermiss = TRUE;
                                fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, p_ptr->lev * 30, 0);
                                nevermiss = FALSE;
                                p_ptr->chp += p_ptr->lev * 30;
                                if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
                                p_ptr->food += p_ptr->lev * 100;
                                if (p_ptr->food > PY_FOOD_MAX) p_ptr->food = PY_FOOD_MAX;
                        }
                        else msg_print("The monster is immune to your bite!");                        
                }
                else msg_print("No monster here.");
        }
        update_and_handle();
        energy_use = 100;
        return (TRUE);
}

/* The Accurate Strike warrior ability! */
bool accurate_strike(int dir)
{
        s32b dam;
        cave_type *c_ptr;
        object_type *o_ptr = &inventory[INVEN_WIELD];
        c_ptr = &cave[py][px];

        if (!o_ptr)
        {
                msg_print("You must have a weapon!");
                return (TRUE);
        }
        
        call_lua("weapon_damages", "", "l", &dam);
        dam += ((dam * ((p_ptr->abilities[(CLASS_WARRIOR * 10) + 4] - 1) * 10)) / 100);
        if (dam < 0) dam = 0;

        /* Never miss! :) */
        nevermiss = TRUE; /* This global is found in angband.h */

        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
                        update_and_handle();
                }
        }

        /* Yes, it's only temporary... */
        nevermiss = FALSE;

        return (TRUE);
}

void counter_attack(monster_type *m_ptr)
{
        s32b dam;
        object_type *o_ptr = &inventory[INVEN_WIELD];

        if (!o_ptr)
        {
                return;
        }
        
        call_lua("weapon_damages", "", "l", &dam);
        dam += ((dam * ((p_ptr->abilities[(CLASS_WARRIOR * 10) + 7] - 1) * 20)) / 100);
        if (dam < 0) dam = 0;

        msg_print("You counter attack!");
        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_PHYSICAL, dam, 0);
        update_and_handle();
}

/* Warrior Leaping Spin attack! :) */
void leaping_spin()
{
        int     ii = 0, ij = 0;
        int flg = PROJECT_GRID | PROJECT_KILL;
        s32b dam;
        cave_type *c_ptr;
        object_type *o_ptr = &inventory[INVEN_WIELD];
        c_ptr = &cave[py][px];

        if (!o_ptr)
        {
                msg_print("You must have a weapon!");
                return;
        }
        
        call_lua("weapon_damages", "", "l", &dam);
        dam += ((dam * ((p_ptr->abilities[(CLASS_WARRIOR * 10) + 8] - 1) * 10)) / 100);
        if (dam < 0) dam = 0;

        msg_print("You jump high!");
        if (!tgt_pt(&ii,&ij)) return;
        if (!cave_empty_bold(ij,ii) || (cave[ij][ii].info & CAVE_ICKY) || (distance(ij,ii,py,px) > (3 + (p_ptr->abilities[(CLASS_WARRIOR * 10) + 8] / 10))))
        {
              msg_print("You can't jump there...");
        }
        else
        {
                if (!(cave[ij][ii].info & CAVE_MARK))
                {
                        if (cave[ij][ii].info & CAVE_LITE) teleport_player_to(ij,ii);
                        else msg_print("You can't jump there...");
                }
                else teleport_player_to(ij,ii);
        }


	/* Hook into the "project()" function */
        (void)project(0, 1, py, px, dam, GF_PHYSICAL, flg);
}

/* Based on Genocide, curse *ALL* monsters!! */
bool godly_wrath()
{
        int     i, amt;
	bool    result = FALSE;
	int     msec = delay_factor * delay_factor * delay_factor;
	int         item;
	object_type *o_ptr;
	cptr        q, s;

	/* Get an item */
        q = "Offer which item? ";
        s = "You have nothing to offer.";
	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

        amt = o_ptr->number;

        if (!(o_ptr->tval == TV_FOOD || o_ptr->tval == TV_LITE || o_ptr->tval == TV_CRYSTAL || o_ptr->tval == TV_LICIALHYD || o_ptr->tval == TV_POTION || o_ptr->tval == TV_POTION2 || (o_ptr->ident & (IDENT_BROKEN))))
        {
                if (!(o_ptr->pval >= (22 - (p_ptr->abilities[(CLASS_PRIEST * 10) + 9] / 5))))
                {
                        msg_format("You must use an item with pval %d or higher.", (22 - (p_ptr->abilities[(CLASS_PRIEST * 10) + 9] / 5)));
                        return FALSE;
                }
        }
        else
        {
                msg_print("You cannot offer this item.");
                return FALSE;
        }

        /* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -amt);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -amt);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}

        msg_print("You call upon the wrath of your god!!");
        msg_print("The air is filled with powerful magical energy, and your enemies start losing their strength!");
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];
                monster_race    *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

                /* Skip friendly monsters! */
                if (is_pet(m_ptr)) continue;

                /* Skip questors */
                if (r_ptr->flags1 & RF1_QUESTOR) continue;

                /* Otherwise, curse the monsters! */
                m_ptr->level = m_ptr->level / 2;
                m_ptr->hp = m_ptr->hp / 2;
                m_ptr->maxhp = m_ptr->maxhp / 2;
                m_ptr->hitrate = m_ptr->hitrate / 2;
                m_ptr->defense = 0;
                if (!(r_ptr->flags1 & RF1_UNIQUE)) m_ptr->abilities |= CURSE_LOCK; 

		/* Visual feedback */
		move_cursor_relative(py, px);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);

		/* Take note */
		result = TRUE;
	}

	return (result);
}

/* Ranger's Wilderness Lore */
void wilderness_lore()
{
        int rad = ((p_ptr->abilities[(CLASS_RANGER * 10)]) / 4) + 3;
        reveal_spell(px, py, rad);
}

/* Paladin's Aura Of Life! */
bool aura_of_life()
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        int x, rad;
        s32b dam, dambonus = 0;
	int spellstat;

	spellstat = (p_ptr->stat_ind[A_WIS] - 5);
	if (spellstat < 0) spellstat = 0;

        /* Heal yourself a little! :) */
        p_ptr->chp += p_ptr->abilities[(CLASS_PALADIN * 10) + 2] * 3;
        if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;

        dam = (p_ptr->abilities[(CLASS_PALADIN * 10) + 2] * 10) * spellstat;
        rad = 3 + (p_ptr->abilities[(CLASS_PALADIN * 10) + 2] / 20);

        dambonus = (dam * (p_ptr->skill[1] * 10)) / 100;

        rad += p_ptr->skill[1] / 30;
        dam += dambonus;

	/* Hook into the "project()" function */
        (void)project(0, rad, py, px, dam, GF_AURA_LIFE, flg);

	/* Assume seen */
        update_and_handle();
	return (TRUE);
}

/* The Smite Evil paladin ability! */
bool smite_evil(int dir)
{
        s32b dam;
        cave_type *c_ptr;
        object_type *o_ptr = &inventory[INVEN_WIELD];
        c_ptr = &cave[py][px];

        if (!o_ptr)
        {
                msg_print("You must have a weapon!");
                return (TRUE);
        }
        
        call_lua("weapon_damages", "", "l", &dam);
	dam += ((dam * ((p_ptr->abilities[(CLASS_PALADIN * 10) + 3] - 1) * 50)) / 100);
        if (dam < 0) dam = 0;

        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, GF_SMITE_EVIL, dam, 0);
                        update_and_handle();
                }
        }

        return (TRUE);
}

/* All evil monsters becomes friendly! :) */
bool word_of_peace()
{
        int     i;
	bool    result = FALSE;
	int     msec = delay_factor * delay_factor * delay_factor;

        msg_print("You formulate a wish of peace, and suddently, there is divine energy in the air...");
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];
                monster_race    *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

                /* Skip friendly monsters! */
                if (is_pet(m_ptr)) continue;

                /* Skip uniques */
                if (r_ptr->flags1 & RF1_UNIQUE) continue;

                /* Skip undeads and demons */
                if (r_ptr->flags3 & RF3_UNDEAD) continue;
                if (r_ptr->flags3 & RF3_DEMON) continue;

                /* Skip non-evil creatures */
                if (!(r_ptr->flags3 & RF3_EVIL)) continue;

                /* Skip bosses */
                if (m_ptr->boss >= 1) continue;

                /* Finally, skip strong monsters */
                if (m_ptr->level > (p_ptr->abilities[(CLASS_PALADIN * 10) + 9] * 3)) continue; 

                /* All right, make it friendly! :) */
                set_pet(m_ptr, TRUE);

		/* Visual feedback */
		move_cursor_relative(py, px);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);

		/* Take note */
		result = TRUE;
	}

	return (result);
}

/* The Element Strike! */
bool element_strike(int dir)
{
        s32b dam;
        cave_type *c_ptr;
        object_type *o_ptr = &inventory[INVEN_WIELD];
        c_ptr = &cave[py][px];

        if (!o_ptr)
        {
                msg_print("You must have a weapon!");
                return (TRUE);
        }
        
        call_lua("weapon_damages", "", "l", &dam);
	dam += ((dam * ((p_ptr->abilities[(CLASS_ELEM_LORD * 10) + 1] - 1) * 10)) / 100);
        if (dam < 0) dam = 0;

        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        fire_ball_spot(m_ptr->fx, m_ptr->fy, p_ptr->elemlord, dam, 0);
                        update_and_handle();
                }
        }

        return (TRUE);
}

/* Elemental Lord's Wave Of Elements! */
void elem_wave()
{
        s32b dam;
        int dir;
        object_type *o_ptr = &inventory[INVEN_WIELD];

        if (!o_ptr)
        {
                msg_print("You must have a weapon!");
                return;
        }


        call_lua("weapon_damages", "", "l", &dam);
	dam += ((dam * ((p_ptr->abilities[(CLASS_ELEM_LORD * 10) + 7] - 1) * 10)) / 100);
        if (dam < 0) dam = 0;

        if (!get_rep_dir(&dir)) return;
        chain_attack(dir, p_ptr->elemlord, dam, 0, 30);
}

bool aura_repulse_evil(int rad)
{
	int flg = PROJECT_GRID | PROJECT_KILL;
        int x;
	/* Hook into the "project()" function */
        no_magic_return = TRUE;
        (void)project(0, rad, py, px, 0, GF_REPULSE_EVIL, flg);
        no_magic_return = FALSE;

	/* Assume seen */
	return (TRUE);
}

/* Zelar's leg breaking throw! */
bool zelar_leg_throw(int dir)
{
	int hit;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];

        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                zelar_leg_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }

        return (TRUE);
}

/* Remove all non-friendly monsters of a given type. */
void anihilate_monsters_specific(int r_idx)
{
	int     i;
	int     msec = delay_factor * delay_factor * delay_factor;

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Delete the monster if we must */
		if (m_ptr->r_idx == r_idx) delete_monster_idx(i);

		/* Visual feedback */
		move_cursor_relative(py, px);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);
	}

}

void fire_jump_ball(int typ, s32b dam, int rad, int x, int y, bool nomagic)
{
        int flg = PROJECT_GRID | PROJECT_KILL;
	if (nomagic) no_magic_return = TRUE;
	(void)project(0, rad, y, x, dam, typ, flg);
	no_magic_return = FALSE;
}

/* Fighter's throw! */
bool fighter_throw(int dir)
{
	int hit;
        cave_type *c_ptr;
        c_ptr = &cave[py][px];

        if (dir == 1)
        {
                c_ptr = &cave[py + 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 2)
        {
                c_ptr = &cave[py + 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 3)
        {
                c_ptr = &cave[py + 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 4)
        {
                c_ptr = &cave[py][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 6)
        {
                c_ptr = &cave[py][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 7)
        {
                c_ptr = &cave[py - 1][px - 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 8)
        {
                c_ptr = &cave[py - 1][px];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }
        if (dir == 9)
        {
                c_ptr = &cave[py - 1][px + 1];
                if (c_ptr->m_idx)
                {
			monster_type    *m_ptr = &m_list[c_ptr->m_idx];
                        call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
			if (hit == 1)
                        {
                                fighter_throw_execute(m_ptr);
                        }
			else msg_print("You missed the monster.");
                        update_and_handle();
                }
        }

        return (TRUE);
}

/* Make all monsters of a given kind friendly or hostile */
void mass_change_allegiance(int r_idx, bool friendly)
{
	int     i;
	int     msec = delay_factor * delay_factor * delay_factor;

	/* Change allegiance! */
	for (i = 1; i < m_max; i++)
	{
		monster_type    *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Delete the monster if we must */
		if (m_ptr->r_idx == r_idx)
		{
			if (friendly) set_pet(m_ptr, TRUE);
			else set_pet(m_ptr, FALSE);
		}

		/* Visual feedback */
		move_cursor_relative(py, px);

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle */
		handle_stuff();

		/* Fresh */
		Term_fresh();

		/* Delay */
		Term_xtra(TERM_XTRA_DELAY, msec);
	}

}

/* Projection on a spcific grid */
void fire_ball_specific_grid(s32b dam, int x, int y, int rad, int typ)
{
	int flg = PROJECT_GRID | PROJECT_KILL;

	/* Hook into the "project()" function */
        (void)project(0, rad, y, x, dam, typ, flg);

	/* Return */
	return;
}