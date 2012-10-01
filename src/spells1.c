/* File: spells1.c */

/* Purpose: Spell code (part 1) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

/* 1/x chance of reducing stats (for elemental attacks) */
#define HURT_CHANCE 16

/* 1/x chance of hurting even if invulnerable!*/
#define PENETRATE_INVULNERABILITY 13

/* Maximum number of tries for teleporting */
#define MAX_TRIES 100


/*
 * Helper function -- return a "nearby" race for polymorphing
 *
 * Note that this function is one of the more "dangerous" ones...
 */
s16b poly_r_idx(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

        int i, r;

#if 0 /* No more -- hehehe -- DG */
	/* Allowable range of "levels" for resulting monster */
        int lev1 = r_ptr->level - ((randint(20) / randint(9)) + 1);
        int lev2 = r_ptr->level + ((randint(20) / randint(9)) + 1);
#endif

	/* Hack -- Uniques/Questors never polymorph */
	if ((r_ptr->flags1 & RF1_UNIQUE) ||
	    (r_ptr->flags1 & RF1_QUESTOR))
		return (r_idx);

	/* Pick a (possibly new) non-unique race */
	for (i = 0; i < 1000; i++)
	{
		/* Pick a new race, using a level calculation */
		r = get_mon_num((dun_level + r_ptr->level) / 2 + 5);

		/* Handle failure */
		if (!r) break;

		/* Obtain race */
		r_ptr = &r_info[r];

		/* Ignore unique monsters */
		if (r_ptr->flags1 & (RF1_UNIQUE)) continue;

#if 0
		/* Ignore monsters with incompatible levels */
		if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) continue;
#endif

		/* Use that index */
		r_idx = r;

		/* Done */
		break;
	}

	/* Result */
	return (r_idx);
}

/*
 * Teleport player, using a distance and a direction as a rough guide.
 *
 * This function is not at all obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
void teleport_player_directed(int rad, int dir)
{
  int y = py;
  int x = px;
  int yfoo = ddy[dir];
  int xfoo = ddx[dir];
  int min = rad / 4;
  int dis = rad;
  int i, d;
  bool look = TRUE;
  bool y_major = FALSE;
  bool x_major = FALSE;
  int y_neg = 1;
  int x_neg = 1;
  cave_type *c_ptr;

  if (xfoo == 0 && yfoo == 0) {
    teleport_player(rad);
    return;
  }
  if (yfoo == 0) {
    x_major = TRUE;
  }
  if (xfoo == 0) {
    y_major = TRUE;
  }
  if (yfoo < 0) {
    y_neg = -1;
  }
  if (xfoo < 0) {
    x_neg = -1;
  }
  /* Look until done */
  while (look) {

    /* Verify max distance */
    if (dis > 200) {
      teleport_player(rad);
      return;
    }
    /* Try several locations */
    for (i = 0; i < 500; i++) {

      /* Pick a (possibly illegal) location */
      while (1) {
	if (y_major) {
	  y = rand_spread(py + y_neg * dis / 2, dis / 2);
	} else {
	  y = rand_spread(py, dis / 3);
	}

	if (x_major) {
	  x = rand_spread(px + x_neg * dis / 2, dis / 2);
	} else {
	  x = rand_spread(px, dis / 3);
	}

	d = distance(py, px, y, x);
	if ((d >= min) && (d <= dis))
	  break;
      }

      /* Ignore illegal locations */
      if (!in_bounds(y, x))
	continue;

      /* Require "naked" floor space */
      if (!cave_empty_bold(y, x))
	continue;

      /* This grid looks good */
      look = FALSE;

      /* Stop looking */
      break;
    }

    /* Increase the maximum distance */
    dis = dis * 2;

    /* Decrease the minimum distance */
    min = min / 2;

  }

  /* Sound */
  sound(SOUND_TELEPORT);

  /* Move player */
  teleport_player_to(y,x);

  /* Handle stuff XXX XXX XXX */
  handle_stuff();

  c_ptr = &cave[y][x];
  /* Hack -- enter a store if we are on one */
  if ((c_ptr->feat >= FEAT_SHOP_HEAD) &&
      (c_ptr->feat <= FEAT_SHOP_TAIL)) {
    /* Disturb */
    disturb(0, 0);

    /* Hack -- enter store */
    command_new = '_';
  }
  /* Hack -- enter a building if we are on one -KMW- */
  if ((c_ptr->feat >= FEAT_BLDG_HEAD) &&
      (c_ptr->feat <= FEAT_BLDG_TAIL)) {
    /* Disturb */
    disturb(0, 0);

    /* Hack -- enter building */
    command_new = ']';
  }
  /* Exit a quest if reach the quest exit -KMW */
  if (c_ptr->feat == FEAT_QUEST_EXIT) {
			if (quest[p_ptr->inside_quest].type == 4)
			{
				quest[p_ptr->inside_quest].status = QUEST_STATUS_COMPLETED;
				msg_print("You accomplished your quest!");
				msg_print(NULL);
			}

			leaving_quest = p_ptr->inside_quest;
			p_ptr->inside_quest = cave[y][x].special;
			dun_level = 0;
			p_ptr->oldpx = 0;
			p_ptr->oldpy = 0;
			p_ptr->leaving = TRUE;
  }
}


/*
 * Teleport a monster, normally up to "dis" grids away.
 *
 * Attempt to move the monster at least "dis/2" grids away.
 *
 * But allow variation to prevent infinite loops.
 */
void teleport_away(int m_idx, int dis)
{
	int ny, nx, oy, ox, d, i, min;
	int tries = 0;

	bool look = TRUE;

	monster_type *m_ptr = &m_list[m_idx];

	/* Paranoia */
	if (!m_ptr->r_idx) return;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look)
	{
		tries++;

		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(oy, dis);
				nx = rand_spread(ox, dis);
				d = distance(oy, ox, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (cave[ny][nx].feat == FEAT_GLYPH) continue;
			if (cave[ny][nx].feat == FEAT_MINOR_GLYPH) continue;

			/* ...nor onto the Pattern */
			if ((cave[ny][nx].feat >= FEAT_PATTERN_START) &&
			    (cave[ny][nx].feat <= FEAT_PATTERN_XTRA2)) continue;

			/* No teleporting into vaults and such */
			if (!(p_ptr->inside_quest))
				if (cave[ny][nx].info & (CAVE_ICKY)) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;

		/* Stop after MAX_TRIES tries */
		if (tries > MAX_TRIES) return;
	}

	/* Sound */
	sound(SOUND_TPOTHER);

	/* Update the new location */
	cave[ny][nx].m_idx = m_idx;

	/* Update the old location */
	cave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	lite_spot(oy, ox);

	/* Redraw the new grid */
	lite_spot(ny, nx);
}



/*
 * Teleport monster next to the player
 */
void teleport_to_player(int m_idx)
{
	int ny, nx, oy, ox, d, i, min;
	int dis = 2;

	bool look = TRUE;

	monster_type *m_ptr = &m_list[m_idx];
	int attempts = 500;

	/* Paranoia */
	if (!m_ptr->r_idx) return;

	/* "Skill" test */
	if (randint(100) > r_info[m_ptr->r_idx].level) return;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look && --attempts)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(py, dis);
				nx = rand_spread(px, dis);
				d = distance(py, px, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (cave[ny][nx].feat == FEAT_GLYPH) continue;
			if (cave[ny][nx].feat == FEAT_MINOR_GLYPH) continue;

			/* ...nor onto the Pattern */
			if ((cave[ny][nx].feat >= FEAT_PATTERN_START) &&
			    (cave[ny][nx].feat <= FEAT_PATTERN_XTRA2)) continue;

			/* No teleporting into vaults and such */
			/* if (cave[ny][nx].info & (CAVE_ICKY)) continue; */

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	if (attempts < 1) return;

	/* Sound */
	sound(SOUND_TPOTHER);

	/* Update the new location */
	cave[ny][nx].m_idx = m_idx;

	/* Update the old location */
	cave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	lite_spot(oy, ox);

	/* Redraw the new grid */
	lite_spot(ny, nx);
}


/*
 * Teleport the player to a location up to "dis" grids away.
 *
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 */
void teleport_player(int dis)
{
	int d, i, min, ox, oy, x, y;
	int tries = 0;

	int xx = -1, yy = -1;

	bool look = TRUE;

	if (p_ptr->inside_quest)
	{
		msg_print("You cannot teleport here.");
		return;
	}

	if (dis > 200) dis = 200; /* To be on the safe side... */

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look)
	{
		tries++;

		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				y = rand_spread(py, dis);
				x = rand_spread(px, dis);
				d = distance(py, px, y, x);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(y, x)) continue;

			/* Require "naked" floor space */
			if (!cave_naked_bold(y, x)) continue;

			/* No teleporting into vaults and such */
			if (cave[y][x].info & (CAVE_ICKY)) continue;

			/* If there is ANY events, do not teleport */
			if (cave[y][x].event > 0) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;

		/* Stop after MAX_TRIES tries */
		if (tries > MAX_TRIES) return;
	}

	/* Sound */
	sound(SOUND_TELEPORT);

	/* Save the old location */
	oy = py;
	ox = px;

	/* Move the player */
	py = y;
	px = x;

	/* Redraw the old spot */
	lite_spot(oy, ox);

	while (xx < 2)
	{
		yy = -1;

		while (yy < 2)
		{
			if (xx == 0 && yy == 0)
			{
				/* Do nothing */
			}
			else
			{
				if (cave[oy+yy][ox+xx].m_idx)
				{
					if ((r_info[m_list[cave[oy+yy][ox+xx].m_idx].r_idx].flags6
					    & RF6_TPORT) &&
					    !(r_info[m_list[cave[oy+yy][ox+xx].m_idx].r_idx].flags3
					    & RF3_RES_TELE))
						/*
						 * The latter limitation is to avoid
						 * totally unkillable suckers...
						 */
					{
						if (!(m_list[cave[oy+yy][ox+xx].m_idx].csleep))
							teleport_to_player(cave[oy+yy][ox+xx].m_idx);
					}
				}
			}
			yy++;
		}
		xx++;
	}

	/* Redraw the new spot */
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

/*
 * get a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 */
void get_pos_player(int dis, int *ny, int *nx)
{
        int d, i, min, x, y;
	int tries = 0;

	bool look = TRUE;

	if (dis > 200) dis = 200; /* To be on the safe side... */

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look)
	{
		tries++;

		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				y = rand_spread(py, dis);
				x = rand_spread(px, dis);
				d = distance(py, px, y, x);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(y, x)) continue;

			/* Require "naked" floor space */
			if (!cave_naked_bold(y, x)) continue;

			/* No teleporting into vaults and such */
			if (cave[y][x].info & (CAVE_ICKY)) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;

		/* Stop after MAX_TRIES tries */
		if (tries > MAX_TRIES) return;
	}

        *ny = y;
        *nx = x;
}

/*
 * Teleport a monster to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 */
void teleport_monster_to(int m_idx, int ny, int nx)
{
	int y, x, oy, ox, dis = 0, ctr = 0;
        monster_type *m_ptr = &m_list[m_idx];

	/* Find a usable location */
	while (1)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = rand_spread(ny, dis);
			x = rand_spread(nx, dis);
			if (in_bounds(y, x)) break;
		}

                /* Not on the player's grid */
		/* Accept "naked" floor grids */
                if (cave_naked_bold(y, x) && (y != py) && (x != px)) break;

		/* Occasionally advance the distance */
		if (++ctr > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
		}
	}

	/* Sound */
        sound(SOUND_TPOTHER);

        /* Save the old position */
        oy = m_ptr->fy;
        ox = m_ptr->fx;
        cave[oy][ox].m_idx = 0;

        /* Move the monster */
        m_ptr->fy = y;
        m_ptr->fx = x;
        cave[y][x].m_idx = m_idx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old spot */
	lite_spot(oy, ox);

	/* Redraw the new spot */
        lite_spot(m_ptr->fy, m_ptr->fx);
}

/*
 * Teleport player to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
void teleport_player_to(int ny, int nx)
{
	int y, x, oy, ox, dis = 0, ctr = 0;

	/* Find a usable location */
	while (1)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = rand_spread(ny, dis);
			x = rand_spread(nx, dis);
			if (in_bounds(y, x)) break;
		}

		/* Accept "naked" floor grids */
                /*if (cave_naked_bold(y, x)) break; */
                break;

		/* Occasionally advance the distance */
                if (++ctr > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
                } 
	}

	/* Sound */
	sound(SOUND_TELEPORT);

	/* Save the old location */
	oy = py;
	ox = px;

	/* Move the player */
	py = y;
	px = x;

	/* Redraw the old spot */
	lite_spot(oy, ox);

	/* Redraw the new spot */
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



/*
 * Teleport the player one level up or down (random when legal)
 */
void teleport_player_level(void)
{
	/* No effect in arena or quest */
	if (p_ptr->inside_quest)
	{
		msg_print("There is no effect.");
		return;
	}

	if (!dun_level)
	{
		msg_print("You sink through the floor.");

		if (autosave_l)
		{
			is_autosave = TRUE;
			msg_print("Autosaving the game...");
			do_cmd_save_game();
			is_autosave = FALSE;
		}

		dun_level++;

		/* Leaving */
		p_ptr->leaving = TRUE;
	}
	else if (is_quest(dun_level) || (dun_level >= MAX_DEPTH-1))
	{
		msg_print("You rise up through the ceiling.");

		if (autosave_l)
		{
			is_autosave = TRUE;
			msg_print("Autosaving the game...");
			do_cmd_save_game();
			is_autosave = FALSE;
		}

		dun_level--;

		/* Leaving */
		p_ptr->leaving = TRUE;
	}
	else if (rand_int(100) < 50)
	{
		msg_print("You rise up through the ceiling.");

		if (autosave_l)
		{
			is_autosave = TRUE;
			msg_print("Autosaving the game...");
			do_cmd_save_game();
			is_autosave = FALSE;
		}

		dun_level--;

		/* Leaving */
		p_ptr->leaving = TRUE;
	}
	else
	{
		msg_print("You sink through the floor.");

		if (autosave_l)
		{
			is_autosave = TRUE;
			msg_print("Autosaving the game...");
			do_cmd_save_game();
			is_autosave = FALSE;
		}

		dun_level++;

		/* Leaving */
		p_ptr->leaving = TRUE;
	}

	/* Sound */
	sound(SOUND_TPLEVEL);
}



/*
 * Recall the player to town or dungeon
 */
void recall_player(void)
{
#if 0
        if (!p_ptr->town_num)
	{
		/* TODO: Recall the player to the last visited town */
		msg_print("Nothing happens.");
		return;
	}
#endif
        if (dun_level && (max_dlv[dungeon_type] > dun_level) && (!p_ptr->inside_quest))
	{
		if (get_check("Reset recall depth? "))
                        max_dlv[dungeon_type] = dun_level;
		
	}
	if (!p_ptr->word_recall)
	{
		p_ptr->word_recall = rand_int(21) + 15;
		msg_print("The air about you becomes charged...");
	}
	else
	{
		p_ptr->word_recall = 0;
		msg_print("A tension leaves the air around you...");
	}
}



/*
 * Get a legal "multi-hued" color for drawing "spells"
 */
static byte mh_attr(int max)
{
	switch (randint(max))
	{
		case  1: return (TERM_RED);
		case  2: return (TERM_GREEN);
		case  3: return (TERM_BLUE);
		case  4: return (TERM_YELLOW);
		case  5: return (TERM_ORANGE);
		case  6: return (TERM_VIOLET);
		case  7: return (TERM_L_RED);
		case  8: return (TERM_L_GREEN);
		case  9: return (TERM_L_BLUE);
		case 10: return (TERM_UMBER);
		case 11: return (TERM_L_UMBER);
		case 12: return (TERM_SLATE);
		case 13: return (TERM_WHITE);
		case 14: return (TERM_L_WHITE);
		case 15: return (TERM_L_DARK);
	}

	return (TERM_WHITE);
}


/*
 * Return a color to use for the bolt/ball spells
 */
static byte spell_color(int type)
{
	/* Check if A.B.'s new graphics should be used (rr9) */
	if (strcmp(ANGBAND_GRAF, "new") == 0)
	{
		/* Analyze */
		switch (type)
		{
			case GF_MISSILE:        return (0x0F);
			case GF_ACID:           return (0x04);
		        case GF_ELEC:           return (0x02);
			case GF_FIRE:           return (0x00);
			case GF_COLD:           return (0x01);
			case GF_FROSTFIRE:      return (0x01);
			case GF_POIS:           return (0x03);
			case GF_TOXIC:          return (0x03);
			case GF_HOLY_FIRE:      return (0x00);
			case GF_HELL_FIRE:      return (0x00);
			case GF_MANA:           return (0x0E);
			case GF_ARROW:          return (0x0F);
			case GF_WATER:          return (0x04);
			case GF_NETHER:         return (0x07);
			case GF_CHAOS:          return (mh_attr(15));
			case GF_DISENCHANT:     return (0x05);
			case GF_NEXUS:          return (0x0C);
			case GF_CONFUSION:      return (mh_attr(4));
			case GF_SOUND:          return (0x09);
			case GF_EARTH:         return (0x08);
			case GF_FORCE:          return (0x09);
			case GF_INERTIA:        return (0x09);
			case GF_GRAVITY:        return (0x09);
			case GF_TIME:           return (0x09);
			case GF_GREY:           return (0x09);
			case GF_LITE_WEAK:      return (0x06);
			case GF_LITE:           return (0x06);
			case GF_DARK_WEAK:      return (0x07);
			case GF_DARK:           return (0x07);
			case GF_PLASMA:         return (0x0B);
			case GF_METEOR:         return (0x00);
			case GF_ICE:            return (0x01);
			case GF_ROCKET:         return (0x0F);
			case GF_DEATH_RAY:      return (0x07);
			case GF_RADIO:           return (mh_attr(2));
			case GF_DISINTEGRATE:   return (0x05);
			case GF_PSI:
			case GF_PSI_DRAIN:
			case GF_TELEKINESIS:
			case GF_DOMINATION:
                        case GF_PSI_HITRATE:
                        case GF_PSI_FEAR:
						return (0x09);
		}

	}
	/* Normal tiles or ASCII */
	else if (use_color)
	{
		/* Analyze */
		switch (type)
		{
			case GF_MISSILE:        return (TERM_SLATE);
			case GF_ACID:           return (randint(5)<3?TERM_YELLOW:TERM_L_GREEN);
		        case GF_ELEC:           return (randint(7)<6?TERM_WHITE:(randint(4)==1?TERM_BLUE:TERM_L_BLUE));
			case GF_FIRE:           return (randint(6)<4?TERM_YELLOW:(randint(4)==1?TERM_RED:TERM_L_RED));
			case GF_COLD:           return (randint(6)<4?TERM_WHITE:TERM_L_WHITE);
			case GF_FROSTFIRE:      return (randint(6)<4?TERM_WHITE:TERM_L_WHITE);
			case GF_POIS:           return (randint(5)<3?TERM_L_GREEN:TERM_GREEN);
			case GF_TOXIC:           return (randint(5)<3?TERM_L_GREEN:TERM_GREEN);
			case GF_HOLY_FIRE:      return (randint(5)==1?TERM_ORANGE:TERM_WHITE);
			case GF_HELL_FIRE:      return (randint(6)==1?TERM_RED:TERM_L_DARK);
			case GF_MANA:           return (randint(5)!=1?TERM_VIOLET:TERM_L_BLUE);
			case GF_ARROW:          return (TERM_L_UMBER);
			case GF_WATER:          return (randint(4)==1?TERM_L_BLUE:TERM_BLUE);
			case GF_NETHER:         return (randint(4)==1?TERM_SLATE:TERM_L_DARK);
			case GF_CHAOS:          return (mh_attr(15));
			case GF_DISENCHANT:     return (randint(5)!=1?TERM_L_BLUE:TERM_VIOLET);
			case GF_NEXUS:          return (randint(5)<3?TERM_L_RED:TERM_VIOLET);
			case GF_CONFUSION:      return (mh_attr(4));
			case GF_SOUND:          return (randint(4)==1?TERM_VIOLET:TERM_WHITE);
			case GF_EARTH:         return (randint(5)<3?TERM_UMBER:TERM_SLATE);
			case GF_FORCE:          return (randint(5)<3?TERM_L_WHITE:TERM_ORANGE);
			case GF_INERTIA:        return (randint(5)<3?TERM_SLATE:TERM_L_WHITE);
			case GF_GREY:        return (randint(5)<3?TERM_SLATE:TERM_L_WHITE);
			case GF_GRAVITY:        return (randint(3)==1?TERM_L_UMBER:TERM_UMBER);
			case GF_TIME:           return (randint(2)==1?TERM_WHITE:TERM_L_DARK);
			case GF_LITE_WEAK:      return (randint(3)==1?TERM_ORANGE:TERM_YELLOW);
			case GF_LITE:           return (randint(4)==1?TERM_ORANGE:TERM_YELLOW);
			case GF_DARK_WEAK:      return (randint(3)==1?TERM_DARK:TERM_L_DARK);
			case GF_DARK:           return (randint(4)==1?TERM_DARK:TERM_L_DARK);
			case GF_PLASMA:         return (randint(5)==1?TERM_RED:TERM_L_RED);
			case GF_METEOR:         return (randint(3)==1?TERM_RED:TERM_UMBER);
			case GF_ICE:            return (randint(4)==1?TERM_L_BLUE:TERM_WHITE);
			case GF_ROCKET:         return (randint(6)<4?TERM_L_RED:(randint(4)==1?TERM_RED:TERM_L_UMBER));
                        case GF_DEATH:
			case GF_DEATH_RAY:
                                                return (TERM_L_DARK);
			case GF_RADIO:           return (mh_attr(2));
			case GF_DISINTEGRATE:   return (randint(3)!=1?TERM_L_DARK:(randint(2)==1?TERM_ORANGE:TERM_L_UMBER));
			case GF_PSI:
			case GF_PSI_DRAIN:
			case GF_TELEKINESIS:
			case GF_DOMINATION:
                        case GF_PSI_HITRATE:
                        case GF_PSI_FEAR:
						return (randint(3)!=1?TERM_L_BLUE:TERM_WHITE);
		}
	}

	/* Standard "color" */
	return (TERM_WHITE);
}


/*
 * Find the attr/char pair to use for a spell effect
 *
 * It is moving (or has moved) from (x,y) to (nx,ny).
 *
 * If the distance is not "one", we (may) return "*".
 */
static u16b bolt_pict(int y, int x, int ny, int nx, int typ)
{
	int base;

	byte k;

	byte a;
	char c;

	/* No motion (*) */
	if ((ny == y) && (nx == x)) base = 0x30;

	/* Vertical (|) */
	else if (nx == x) base = 0x40;

	/* Horizontal (-) */
	else if (ny == y) base = 0x50;

	/* Diagonal (/) */
	else if ((ny-y) == (x-nx)) base = 0x60;

	/* Diagonal (\) */
	else if ((ny-y) == (nx-x)) base = 0x70;

	/* Weird (*) */
	else base = 0x30;

	/* Basic spell color */
	k = spell_color(typ);

	/* Obtain attr/char */
	a = misc_to_attr[base+k];
	c = misc_to_char[base+k];

	/* Create pict */
	return (PICT(a,c));
}


/*
 * Decreases players hit points and sets death flag if necessary
 *
 * XXX XXX XXX Invulnerability needs to be changed into a "shield"
 *
 * XXX XXX XXX Hack -- this function allows the user to save (or quit)
 * the game when he dies, since the "You die." message is shown before
 * setting the player to "dead".
 */
void take_hit(s32b damage, cptr hit_from)
{
        int old_chp = p_ptr->chp;

        bool monster_take = FALSE;

	char death_message[80];

	int warning = (p_ptr->mhp * hitpoint_warn / 10);


	/* Paranoia */
	if (death) return;

	/* Disturb */
	disturb(1, 0);

        /* Protection item? */
        if (protection_check()) damage = damage / 2;

        /* Great Guard Defender ability! */
        if (p_ptr->abilities[(CLASS_DEFENDER * 10) + 5] >= 1 && !(p_ptr->confused) && damage > 0)
        {
                int chance = p_ptr->guardconfuse;
                s32b reduction = (p_ptr->abilities[(CLASS_DEFENDER * 10) + 5] * 1000);
                char ch;

                msg_format("You are about to take %ld damages...", damage);
                get_com("Guard? [y/n] ", &ch);
                if (ch == 'y' || ch == 'Y')
                {
                        damage -= reduction;
                        if (damage < 0) damage = 0;

                        msg_print("You guard the damages!");

                        p_ptr->guardconfuse += 20;

                        if (randint(100) <= chance)
                        {
                                msg_print("The impact daze your head...");
                                (void)set_confused(3);
                        }
                        update_and_handle();
                }
        }
        /* Soul Guard ability! */
        if (p_ptr->abilities[(CLASS_SOUL_GUARDIAN * 10) + 8] >= 1)
        {                
                s32b reduction;
                char ch;

                msg_format("You are about to take %ld damages...", damage);
                get_com("Use a soul to guard? [y/n] ", &ch);
                if (ch == 'y' || ch == 'Y')
                {
                        monster_race *r_ptr;
                        int item;
                        object_type *o_ptr;

                        cptr q, s;

                        /* Restrict choices to monsters */
                        item_tester_tval = TV_SOUL;

                        /* Get an item */
                        q = "Use which soul? ";
                        s = "You have no souls!.";
                        if (!get_item(&item, q, s, (USE_INVEN))) return;

                        o_ptr = &inventory[item]; 

                        if (o_ptr->timeout >= 1)
                        {
                                msg_print("This soul still need to recharge.");
                        }
                        else
                        {
                                /* Get the monster */
                                r_ptr = &r_info[o_ptr->pval];

                                reduction = maxroll(r_ptr->hdice, r_ptr->hside);
                                reduction = reduction + ((reduction * (p_ptr->abilities[(CLASS_SOUL_GUARDIAN * 10) + 8] * 20)) / 100);

                                damage -= reduction;
                                if (damage < 0) damage = 0;

                                msg_print("The soul guarded you!");
                                o_ptr->timeout = 60;

                                update_and_handle();
                        }
                }
        }

	/* Hurt the player */
        if(!monster_take) p_ptr->chp -= damage;

	/* Display the hitpoints */
	p_ptr->redraw |= (PR_HP);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Dead player */
	if (p_ptr->chp < 0)
	{
		if ((p_ptr->inside_quest > 0) && (p_ptr->death_dialog > 0))
		{
			p_ptr->chp = 1;
			death = FALSE;
			if (p_ptr->eventdeath > 0) p_ptr->events[p_ptr->eventdeath] = p_ptr->eventdeathset;
			show_dialog(p_ptr->death_dialog);
		}
		else
		{
                	cptr str;

                	/* You're dying! */
                	dying = TRUE;

                	/* Sound */
                	sound(SOUND_DEATH);

                	/* Hack -- Note death */
                	if (!last_words)
                	{
                        	msg_print("You die.");
                        	msg_print(NULL);
                	}
                	else
                	{
                        	(void)get_rnd_line("death.txt", death_message);
                        	msg_print(death_message);
                	}

                	/* Note cause of death */
                	(void)strcpy(died_from, hit_from);

                	if (p_ptr->image) strcat(died_from,"(?)");

                	/* Increase death count! */
                	p_ptr->deathcount += 1;

                	/* No longer a winner */
                	total_winner = FALSE;

                	/* Leaving */
                	p_ptr->leaving = TRUE;

			/* We won't resurrect in the wild... */
			p_ptr->wild_mode = FALSE;

                	/* Note death */
                	death = TRUE;

                	if (get_check("Dump the screen? "))
                	{
                        	do_cmd_save_screen();
                	}

                	/* Dead */
                	return;
		}
	}

	/* Hitpoint warning */
	if (p_ptr->chp < warning)
	{
		/* Hack -- bell on first notice */
		if (alert_hitpoint && (old_chp > warning)) bell();

		sound(SOUND_WARN);		

		
                msg_print("*** LOW HITPOINT WARNING! ***");
		msg_print(NULL);
	}
}

/*
 * Note that amulets, rods, and high-level spell books are immune
 * to "inventory damage" of any kind.  Also sling ammo and shovels.
 */


/*
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
static bool hates_acid(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable items */
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
                case TV_DAGGER:
                case TV_AXE:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
                case TV_ZELAR_WEAPON:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls are wood/paper */
		case TV_STAFF:
		case TV_SCROLL:
		{
			return (TRUE);
		}

		/* Ouch */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Junk is useless */
		case TV_SKELETON:
		case TV_BOTTLE:
                case TV_EGG:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate electricity?
 */
static bool hates_elec(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_RING:
		case TV_WAND:
                case TV_EGG:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate fire?
 * Hafted/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
static bool hates_fire(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable */
		case TV_LITE:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		{
			return (TRUE);
		}

		/* Books */
                case TV_VALARIN_BOOK:
                case TV_MAGERY_BOOK:
                case TV_SHADOW_BOOK:
		case TV_CHAOS_BOOK:
                case TV_NETHER_BOOK:
                case TV_CRUSADE_BOOK:
                case TV_SIGALDRY_BOOK:
                case TV_SYMBIOTIC_BOOK:
                case TV_MUSIC_BOOK:
                case TV_MIMIC_BOOK:
		{
			return (TRUE);
		}

		/* Chests */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls burn */
		case TV_STAFF:
		case TV_SCROLL:
                case TV_EGG:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate cold?
 */
static bool hates_cold(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
                case TV_POTION2:
		case TV_POTION:
		case TV_FLASK:
		case TV_BOTTLE:
                case TV_EGG:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}









/*
 * Melt something
 */
static int set_acid_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3, f4;
	if (!hates_acid(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3, &f4);
	if (f3 & (TR3_IGNORE_ACID)) return (FALSE);
	return (TRUE);
}


/*
 * Electrical damage
 */
static int set_elec_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3, f4;
	if (!hates_elec(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3, &f4);
	if (f3 & (TR3_IGNORE_ELEC)) return (FALSE);
	return (TRUE);
}


/*
 * Burn something
 */
static int set_fire_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3, f4;
	if (!hates_fire(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3, &f4);
	if (f3 & (TR3_IGNORE_FIRE)) return (FALSE);
	return (TRUE);
}


/*
 * Freeze things
 */
static int set_cold_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3, f4;
	if (!hates_cold(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3, &f4);
	if (f3 & (TR3_IGNORE_COLD)) return (FALSE);
	return (TRUE);
}




/*
 * This seems like a pretty standard "typedef"
 */
typedef int (*inven_func)(object_type *);

/*
 * Destroys a type of item on a given percent chance
 * Note that missiles are no longer necessarily all destroyed
 * Destruction taken from "melee.c" code for "stealing".
 * Returns number of items destroyed.
 */
static int inven_damage(inven_func typ, int perc)
{
	int             i, j, k, amt;

	object_type     *o_ptr;

	char    o_name[80];


	/* Count the casualties */
	k = 0;

	/* Scan through the slots backwards */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Hack -- for now, skip artifacts */
		if (artifact_p(o_ptr) || o_ptr->art_name) continue;

		/* Give this item slot a shot at death */
		if ((*typ)(o_ptr))
		{
			/* Count the casualties */
			for (amt = j = 0; j < o_ptr->number; ++j)
			{
				if (rand_int(100) < perc) amt++;
			}

			/* Some casualities */
			if (amt)
			{
				/* Get a description */
				object_desc(o_name, o_ptr, FALSE, 3);

				/* Message */
				msg_format("%sour %s (%c) %s destroyed!",
				    ((o_ptr->number > 1) ?
				    ((amt == o_ptr->number) ? "All of y" :
				    (amt > 1 ? "Some of y" : "One of y")) : "Y"),
				    o_name, index_to_label(i),
				    ((amt > 1) ? "were" : "was"));

				/* Potions smash open */
				if (k_info[o_ptr->k_idx].tval == TV_POTION)
				{
					(void)potion_smash_effect(0, py, px, o_ptr->sval);
				}
                
				/* Hack -- If rods or wand are destroyed, the total maximum 
				 * timeout or charges of the stack needs to be reduced, 
				 * unless all the items are being destroyed. -LM-
				 */
				if (((o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD)) 
					&& (amt < o_ptr->number))
				{
					o_ptr->pval -= o_ptr->pval * amt / o_ptr->number;
				}

				/* Destroy "amt" items */
				inven_item_increase(i, -amt);
				inven_item_optimize(i);

				/* Count the casualties */
				k += amt;
			}
		}
	}

	/* Return the casualty count */
	return (k);
}




/*
 * Acid has hit the player, attempt to affect some armor.
 *
 * Note that the "base armor" of an object never changes.
 *
 * If any armor is damaged (or resists), the player takes less damage.
 */
static int minus_ac(void)
{
	object_type     *o_ptr = NULL;

        u32b            f1, f2, f3, f4;

	char            o_name[80];


	/* Pick a (possibly empty) inventory slot */
	switch (randint(6))
	{
		case 1: o_ptr = &inventory[INVEN_BODY]; break;
		case 2: o_ptr = &inventory[INVEN_ARM]; break;
		case 3: o_ptr = &inventory[INVEN_OUTER]; break;
		case 4: o_ptr = &inventory[INVEN_HANDS]; break;
		case 5: o_ptr = &inventory[INVEN_HEAD]; break;
		case 6: o_ptr = &inventory[INVEN_FEET]; break;
	}

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0) return (FALSE);


	/* Describe */
	object_desc(o_name, o_ptr, FALSE, 0);

	/* Extract the flags */
        object_flags(o_ptr, &f1, &f2, &f3, &f4);

	/* Object resists */
	if (f3 & (TR3_IGNORE_ACID))
	{
		msg_format("Your %s is unaffected!", o_name);

		return (TRUE);
	}

	/* Message */
	msg_format("Your %s is damaged!", o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER);

	/* Item was damaged */
	return (TRUE);
}


/*
 * Hurt the player with Acid
 */
void acid_dam(s32b dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total Immunity */
	if ((dam <= 0)) return;

	/* If any armor gets hit, defend the player */
	if (minus_ac()) dam = (dam + 1) / 2;

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_acid_destroy, inv);
}


/*
 * Hurt the player with electricity
 */
void elec_dam(s32b dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if ((dam <= 0)) return;

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_elec_destroy, inv);
}




/*
 * Hurt the player with Fire
 */
void fire_dam(s32b dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Totally immune */
	if ((dam <= 0)) return;

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_fire_destroy, inv);
}


/*
 * Hurt the player with Cold
 */
void cold_dam(s32b dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if ((dam <= 0)) return;

	/* Take damage */
	take_hit(dam, kb_str);

	/* Inventory damage */
	inven_damage(set_cold_destroy, inv);
}





/*
 * Increases a stat by one randomized level             -RAK-
 *
 * Note that this function (used by stat potions) now restores
 * the stat BEFORE increasing it.
 */
bool inc_stat(int stat)
{
        int value;

	/* Then augment the current/max stat */
	value = p_ptr->stat_cur[stat];
        value++;

        /* Save the new value */
        p_ptr->stat_cur[stat] = value;

        /* Bring up the maximum too */
        if (value > p_ptr->stat_max[stat])
        {
                p_ptr->stat_max[stat] = value;
        }

        /* Recalculate bonuses */
        p_ptr->update |= (PU_BONUS);

        /* Success */
        return (TRUE);
}



/*
 * Decreases a stat by an amount indended to vary from 0 to 100 percent.
 *
 * Amount could be a little higher in extreme cases to mangle very high
 * stats from massive assaults.  -CWS
 *
 * Note that "permanent" means that the *given* amount is permanent,
 * not that the new value becomes permanent.  This may not work exactly
 * as expected, due to "weirdness" in the algorithm, but in general,
 * if your stat is already drained, the "max" value will not drop all
 * the way down to the "cur" value.
 */
bool dec_stat(int stat, int amount, int mode)
{
        int cur, max, loss = 0, same, res = FALSE;


	/* Acquire current value */
	cur = p_ptr->stat_cur[stat];        
        max = p_ptr->stat_max[stat];

	/* Note when the values are identical */
	same = (cur == max);

	/* Damage "current" value */
        if (cur > 1)
	{
                cur -= (amount / 3);

		/* Prevent illegal values */
                if (cur < 1) cur = 1;

		/* Something happened */
		if (cur != p_ptr->stat_cur[stat]) res = TRUE;
	}

	/* Damage "max" value */
        if ((mode==STAT_DEC_PERMANENT) && (max > 1))
	{
                max -= (amount / 3);

		/* Hack -- keep it clean */
		if (same || (max < cur)) max = cur;

		/* Something happened */
		if (max != p_ptr->stat_max[stat]) res = TRUE;
	}

	/* Apply changes */
	if (res)
	{
		/* Actually set the stat to its new value. */
                p_ptr->stat_cur[stat] = cur;
                p_ptr->stat_max[stat] = max;

		if (mode==STAT_DEC_TEMPORARY)
		{
			u16b dectime;

			/* a little crude, perhaps */
			dectime = rand_int(max_dlv[dungeon_type]*50) + 50;
			
			/* prevent overflow, stat_cnt = u16b */
			/* or add another temporary drain... */
			if ( ((p_ptr->stat_cnt[stat]+dectime)<p_ptr->stat_cnt[stat]) ||
			    (p_ptr->stat_los[stat]>0) )

			{
				p_ptr->stat_cnt[stat] += dectime;
				p_ptr->stat_los[stat] += loss;
			}
			else
			{
				p_ptr->stat_cnt[stat] = dectime;
				p_ptr->stat_los[stat] = loss;
			}
		}

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
	}

	/* Done */
	return (res);
}


/*
 * Restore a stat.  Return TRUE only if this actually makes a difference.
 */
bool res_stat(int stat)
{
	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat])
	{
		/* Restore */
		p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		/* Remove temporary drain */
		p_ptr->stat_cnt[stat] = 0;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}




/*
 * Apply disenchantment to the player's stuff
 *
 * XXX XXX XXX This function is also called from the "melee" code
 *
 * If "mode is set to 0 then a random slot will be used, if not the "mode"
 * slot will be used.
 *
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(int mode)
{
        int             t = mode;
	object_type     *o_ptr;
	char            o_name[80];

        if(!mode)
        {
                /* Pick a random slot */
                switch (randint(8))
                {
                        case 1: t = INVEN_WIELD; break;
                        case 2: t = INVEN_BOW; break;
                        case 3: t = INVEN_BODY; break;
                        case 4: t = INVEN_OUTER; break;
                        case 5: t = INVEN_ARM; break;
                        case 6: t = INVEN_HEAD; break;
                        case 7: t = INVEN_HANDS; break;
                        case 8: t = INVEN_FEET; break;
                }
        }

	/* Get the item */
	o_ptr = &inventory[t];

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);


	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0))
	{
		/* Nothing to notice */
		return (FALSE);
	}


	/* Describe the object */
	object_desc(o_name, o_ptr, FALSE, 0);


	/* Artifacts have 71% chance to resist */
	if ((artifact_p(o_ptr) || o_ptr->art_name) && (rand_int(100) < 71))
	{
		/* Message */
		msg_format("Your %s (%c) resist%s disenchantment!",
			   o_name, index_to_label(t),
			   ((o_ptr->number != 1) ? "" : "s"));

		/* Notice */
		return (TRUE);
	}


	/* Disenchant tohit */
	if (o_ptr->to_h > 0) o_ptr->to_h--;
	if ((o_ptr->to_h > 5) && (rand_int(100) < 20)) o_ptr->to_h--;

	/* Disenchant todam */
	if (o_ptr->to_d > 0) o_ptr->to_d--;
	if ((o_ptr->to_d > 5) && (rand_int(100) < 20)) o_ptr->to_d--;

	/* Disenchant toac */
	if (o_ptr->to_a > 0) o_ptr->to_a--;
	if ((o_ptr->to_a > 5) && (rand_int(100) < 20)) o_ptr->to_a--;

	/* Message */
	msg_format("Your %s (%c) %s disenchanted!",
		   o_name, index_to_label(t),
		   ((o_ptr->number != 1) ? "were" : "was"));

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Window stuff */
	p_ptr->window |= (PW_EQUIP | PW_PLAYER);

	/* Notice */
	return (TRUE);
}


void mutate_player(void)
{
	int max1, cur1, max2, cur2, ii, jj;

	/* Pick a pair of stats */
	ii = rand_int(6);
	for (jj = ii; jj == ii; jj = rand_int(6)) /* loop */;

	max1 = p_ptr->stat_max[ii];
	cur1 = p_ptr->stat_cur[ii];
	max2 = p_ptr->stat_max[jj];
	cur2 = p_ptr->stat_cur[jj];

	p_ptr->stat_max[ii] = max2;
	p_ptr->stat_cur[ii] = cur2;
	p_ptr->stat_max[jj] = max1;
	p_ptr->stat_cur[jj] = cur1;

	p_ptr->update |= (PU_BONUS);
}


/*
 * Apply Nexus
 */
static void apply_nexus(monster_type *m_ptr)
{
	if (!special_flag)
	{
	switch (randint(7))
	{
		case 1: case 2: case 3:
		{
			teleport_player(200);
			break;
		}

		case 4: case 5:
		{
			teleport_player_to(m_ptr->fy, m_ptr->fx);
			break;
		}

		case 6:
		{
                        if (rand_int(100) < p_ptr->stat_ind[A_WIS])
			{
				msg_print("You resist the effects!");
				break;
			}

			/* Teleport Level */
			teleport_player_level();
			break;
		}

		case 7:
		{
                        if (rand_int(100) < p_ptr->stat_ind[A_WIS])
			{
				msg_print("You resist the effects!");
				break;
			}

			msg_print("Your body starts to scramble...");
			mutate_player();
			break;
		}
	}
        }
}

/*
 * Convert 2 couples of coordonates to a direction
 */
int yx_to_dir(int y2, int x2, int y1, int x1)
{
        int y = y2 - y1, x = x2 - x1;

        if((y == 0) && (x == 1)) return 6;
        if((y == 0) && (x == -1)) return 4;
        if((y == -1) && (x == 0)) return 8;
        if((y == 1) && (x == 0)) return 2;
        if((y == -1) && (x == -1)) return 7;
        if((y == -1) && (x == 1)) return 9;
        if((y == 1) && (x == 1)) return 3;
        if((y == 1) && (x == -1)) return 1;

        return 5;
}

/*
 * Give the opposate direction of the given one
 */
int invert_dir(int dir)
{
        if(dir == 4) return 6;
        if(dir == 6) return 4;
        if(dir == 8) return 2;
        if(dir == 2) return 8;
        if(dir == 7) return 3;
        if(dir == 9) return 1;
        if(dir == 1) return 9;
        if(dir == 3) return 7;
        return 5;
}

/*
 * Determine which way the mana path follow
 */
int get_mana_path_dir(int y, int x, int oy, int ox, int pdir, int mana)
{
        int dir[8] = {5, 5, 5, 5, 5, 5, 5, 5}, n = 0, i;
        int r = 0;

        /* Check which case are allowed */
        if(cave[y - 1][x].mana == mana) dir[n++] = 8;
        if(cave[y + 1][x].mana == mana) dir[n++] = 2;
        if(cave[y][x - 1].mana == mana) dir[n++] = 4;
        if(cave[y][x + 1].mana == mana) dir[n++] = 6;

        /* If only 2 possibilities select the only good one */
        if(n == 2)
        {
                if(invert_dir(yx_to_dir(y, x, oy, ox)) != dir[0]) return dir[0];
                if(invert_dir(yx_to_dir(y, x, oy, ox)) != dir[1]) return dir[1];

                /* Should never happen */
                return 5;
        }


        /* Check if it's not your last place */
        for(i = 0; i < n; i++)
        {
                if((oy == y + ddy[dir[i]]) && (ox == x + ddx[dir[i]]))
                {
                        if(dir[i] == 8) dir[i] = 2;
                        else if(dir[i] == 2) dir[i] = 8;
                        else if(dir[i] == 6) dir[i] = 4;
                        else if(dir[i] == 4) dir[i] = 6;
                }
        }

        /* Select the desired one if possible */
        for(i = 0; i < n; i++)
        {
                if((dir[i] == pdir) && (cave[y + ddy[dir[i]]][x + ddx[dir[i]]].mana == mana)) return dir[i];
        }

        /* If not select a random one */
        if(n > 2)
        {
                byte nb = 200;

                while(nb)
                {
                        nb--;

                        r = rand_int(n);
                        if((dir[r] != 5) && (yx_to_dir(y, x, oy, ox) != dir[r])) break;
                }
                return dir[r];
        }
        /* If nothing is found return 5 */
        else return 5;
}

/*
 * Determine the path taken by a projection.
 *
 * The projection will always start from the grid (y1,x1), and will travel
 * towards the grid (y2,x2), touching one grid per unit of distance along
 * the major axis, and stopping when it enters the destination grid or a
 * wall grid, or has travelled the maximum legal distance of "range".
 *
 * Note that "distance" in this function (as in the "update_view()" code)
 * is defined as "MAX(dy,dx) + MIN(dy,dx)/2", which means that the player
 * actually has an "octagon of projection" not a "circle of projection".
 *
 * The path grids are saved into the grid array pointed to by "gp", and
 * there should be room for at least "range" grids in "gp".  Note that
 * due to the way in which distance is calculated, this function normally
 * uses fewer than "range" grids for the projection path, so the result
 * of this function should never be compared directly to "range".  Note
 * that the initial grid (y1,x1) is never saved into the grid array, not
 * even if the initial grid is also the final grid.  XXX XXX XXX
 *
 * The "flg" flags can be used to modify the behavior of this function.
 *
 * In particular, the "PROJECT_STOP" and "PROJECT_THRU" flags have the same
 * semantics as they do for the "project" function, namely, that the path
 * will stop as soon as it hits a monster, or that the path will continue
 * through the destination grid, respectively.
 *
 * The "PROJECT_JUMP" flag, which for the "project()" function means to
 * start at a special grid (which makes no sense in this function), means
 * that the path should be "angled" slightly if needed to avoid any wall
 * grids, allowing the player to "target" any grid which is in "view".
 * This flag is non-trivial and has not yet been implemented, but could
 * perhaps make use of the "vinfo" array (above).  XXX XXX XXX
 *
 * This function returns the number of grids (if any) in the path.  This
 * function will return zero if and only if (y1,x1) and (y2,x2) are equal.
 *
 * This algorithm is similar to, but slightly different from, the one used
 * by "update_view_los()", and very different from the one used by "los()".
 */
sint project_path(u16b *gp, int range, int y1, int x1, int y2, int x2, int flg)
{
        int y, x, mana = 0, dir = 0;

	int n = 0;
	int k = 0;

	/* Absolute */
	int ay, ax;

	/* Offsets */
	int sy, sx;

	/* Fractions */
	int frac;

	/* Scale factors */
	int full, half;

	/* Slope */
	int m;


	/* No path necessary (or allowed) */
	if ((x1 == x2) && (y1 == y2)) return (0);

        /* Hack -- to make a bolt/beam/ball follow a mana path */
        if(flg & PROJECT_MANA_PATH)
        {
                int oy = y1, ox = x1, pdir = yx_to_dir(y2, x2, y1, x1);

                /* Get the mana path level to follow */
                mana = cave[y1][x1].mana;

		/* Start */
                dir = get_mana_path_dir(y1, x1, y1, x1, pdir, mana);
                y = y1 + ddy[dir];
                x = x1 + ddx[dir];

		/* Create the projection path */
		while (1)
		{
			/* Save grid */
			gp[n++] = GRID(y,x);

			/* Hack -- Check maximum range */
                        if (n >= range + 10) return n;

			/* Always stop at non-initial wall grids */
                        if ((n > 0) && !cave_floor_bold_project(y, x)) return n;

			/* Sometimes stop at non-initial monsters/players */
			if (flg & (PROJECT_STOP))
			{
                                if ((n > 0) && (cave[y][x].m_idx != 0)) return n;
			}

                        /* Get the new direction */
                        dir = get_mana_path_dir(y, x, oy, ox, pdir, mana);
                        if(dir == 5) return n;
                        oy = y;
                        ox = x;
                        y += ddy[dir];
                        x += ddx[dir];
		}
        }

	/* Analyze "dy" */
	if (y2 < y1)
	{
		ay = (y1 - y2);
		sy = -1;
	}
	else
	{
		ay = (y2 - y1);
		sy = 1;
	}

	/* Analyze "dx" */
	if (x2 < x1)
	{
		ax = (x1 - x2);
		sx = -1;
	}
	else
	{
		ax = (x2 - x1);
		sx = 1;
	}


	/* Number of "units" in one "half" grid */
	half = (ay * ax);

	/* Number of "units" in one "full" grid */
	full = half << 1;


	/* Vertical */
	if (ay > ax)
	{
		/* Start at tile edge */
		frac = ax * ax;

		/* Let m = ((dx/dy) * full) = (dx * dx * 2) = (frac * 2) */
		m = frac << 1;

		/* Start */
		y = y1 + sy;
		x = x1;

		/* Create the projection path */
		while (1)
		{
			/* Save grid */
			gp[n++] = GRID(y,x);

			/* Hack -- Check maximum range */
			if ((n + (k >> 1)) >= range) break;

			/* Sometimes stop at destination grid */
			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			/* Always stop at non-initial wall grids */
                        if ((n > 0) && !cave_floor_bold_project(y, x) && !(flg & PROJECT_WALL)) break;

			/* Sometimes stop at non-initial monsters/players */
			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) && (cave[y][x].m_idx != 0)) break;
			}

			/* Slant */
			if (m)
			{
				/* Advance (X) part 1 */
				frac += m;

				/* Horizontal change */
				if (frac >= half)
				{
					/* Advance (X) part 2 */
					x += sx;

					/* Advance (X) part 3 */
					frac -= full;

					/* Track distance */
					k++;
				}
			}

			/* Advance (Y) */
			y += sy;
		}
	}

	/* Horizontal */
	else if (ax > ay)
	{
		/* Start at tile edge */
		frac = ay * ay;

		/* Let m = ((dy/dx) * full) = (dy * dy * 2) = (frac * 2) */
		m = frac << 1;

		/* Start */
		y = y1;
		x = x1 + sx;

		/* Create the projection path */
		while (1)
		{
			/* Save grid */
			gp[n++] = GRID(y,x);

			/* Hack -- Check maximum range */
			if ((n + (k >> 1)) >= range) break;

			/* Sometimes stop at destination grid */
			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			/* Always stop at non-initial wall grids */
                        if ((n > 0) && !cave_floor_bold_project(y, x) && !(flg & PROJECT_WALL)) break;

			/* Sometimes stop at non-initial monsters/players */
			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) && (cave[y][x].m_idx != 0)) break;
			}

			/* Slant */
			if (m)
			{
				/* Advance (Y) part 1 */
				frac += m;

				/* Vertical change */
				if (frac >= half)
				{
					/* Advance (Y) part 2 */
					y += sy;

					/* Advance (Y) part 3 */
					frac -= full;

					/* Track distance */
					k++;
				}
			}

			/* Advance (X) */
			x += sx;
		}
	}

	/* Diagonal */
	else
	{
		/* Start */
		y = y1 + sy;
		x = x1 + sx;

		/* Create the projection path */
		while (1)
		{
			/* Save grid */
			gp[n++] = GRID(y,x);

			/* Hack -- Check maximum range */
			if ((n + (n >> 1)) >= range) break;

			/* Sometimes stop at destination grid */
			if (!(flg & (PROJECT_THRU)))
			{
				if ((x == x2) && (y == y2)) break;
			}

			/* Always stop at non-initial wall grids */
                        if ((n > 0) && !cave_floor_bold_project(y, x) && !(flg & PROJECT_WALL)) break;

			/* Sometimes stop at non-initial monsters/players */
			if (flg & (PROJECT_STOP))
			{
				if ((n > 0) && (cave[y][x].m_idx != 0)) break;
			}

			/* Advance (Y) */
			y += sy;

			/* Advance (X) */
			x += sx;
		}
	}


	/* Length */
	return (n);
}



/*
 * Mega-Hack -- track "affected" monsters (see "project()" comments)
 */
static int project_m_n;
static int project_m_x;
static int project_m_y;



/*
 * We are called from "project()" to "damage" terrain features
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * XXX XXX XXX Perhaps we should affect doors?
 */
static bool project_f(int who, int r, int y, int x, s32b dam, int typ)
{
	cave_type       *c_ptr = &cave[y][x];

	bool            obvious = FALSE;

        bool flag = FALSE;

	/* XXX XXX XXX */
	who = who ? who : 0;

	/* Analyze the type */
	switch (typ)
	{
		/* Ignore most effects */
		case GF_ACID:
		case GF_ELEC:
		case GF_COLD:
		case GF_PLASMA:
		case GF_ICE:
		case GF_EARTH:
		case GF_FORCE:
		case GF_SOUND:
		case GF_MANA:
		case GF_HOLY_FIRE:
		case GF_HELL_FIRE:
		case GF_DISINTEGRATE:
		case GF_PSI:
		case GF_PSI_DRAIN:
		case GF_TELEKINESIS:
		case GF_DOMINATION:
                case GF_PSI_FEAR:
                case GF_PSI_HITRATE:
		{
			break;
		}

                case GF_BETWEEN_GATE:
                {
                        int y1 = randint(cur_hgt) - 1, x1 = randint(cur_wid) - 1, y2 = y1, x2 = x1;
                        int try = 1000;

                        if(!((f_info[cave[y][x].feat].flags1 & FF1_FLOOR) && !(f_info[cave[y][x].feat].flags1 & FF1_REMEMBER) && !(f_info[cave[y][x].feat].flags1 & FF1_PERMANENT))) break;

                        while(!((f_info[cave[y2][x2].feat].flags1 & FF1_FLOOR) && !(f_info[cave[y][x].feat].flags1 & FF1_REMEMBER) && !(f_info[cave[y][x].feat].flags1 & FF1_PERMANENT)) && try)
                        {
                                y2 = y1 = randint(cur_hgt) - 1;
                                x2 = x1 = randint(cur_wid) - 1;
                                scatter(&y2, &x2, y1, x1, 20, 0);
                                try --;
                        }
                        if(!try) break;

                        cave_set_feat(y, x, FEAT_BETWEEN);
                        cave[y][x].special = x2 + (y2 << 8);

                        cave_set_feat(y2, x2, FEAT_BETWEEN);
                        cave[y2][x2].special = x + (y << 8);
                        break;
                }

                /* Burn trees */
		case GF_FIRE:
		case GF_METEOR:
                {
                        if(c_ptr->feat == FEAT_TREES)
                        {
				/* Forget the trap */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the trap */
                                cave_set_feat(y, x, FEAT_GRASS);
                        }
                        break;
                }

		/* Destroy Traps (and Locks) */
		case GF_KILL_TRAP:
		{
			/* Destroy traps */
			if (c_ptr->t_idx != 0)
			{
				/* Check line of sight */
				if (player_has_los_bold(y, x))
				{
					msg_print("There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Forget the trap */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the trap */
				c_ptr->t_idx = 0;
			}

			/* Secret / Locked doors are found and unlocked */
			else if ((c_ptr->feat == FEAT_SECRET) ||
				 ((c_ptr->feat >= FEAT_DOOR_HEAD + 0x01) &&
				  (c_ptr->feat <= FEAT_DOOR_HEAD + 0x07)))
			{
				/* Unlock the door */
				cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);

				/* Check line of sound */
				if (player_has_los_bold(y, x))
				{
					msg_print("Click!");
					obvious = TRUE;
				}
			}

			break;
		}

		/* Destroy Doors (and traps) */
		case GF_KILL_DOOR:
		{
			/* Destroy all doors and traps */
			if ((c_ptr->feat == FEAT_OPEN) ||
			    (c_ptr->feat == FEAT_BROKEN) ||
			    (c_ptr->t_idx != 0) ||
			   ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			    (c_ptr->feat <= FEAT_DOOR_TAIL)))
			{
				/* Check line of sight */
				if (player_has_los_bold(y, x))
				{
					/* Message */
					msg_print("There is a bright flash of light!");
					obvious = TRUE;

					/* Visibility change */
					if ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
					    (c_ptr->feat <= FEAT_DOOR_TAIL))
					{
						/* Update some things */
						p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);
					}
				}

				/* Forget the door */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the feature */
				cave_set_feat(y, x, FEAT_FLOOR);

				/* Remove traps */
				c_ptr->t_idx = 0;
			}

			break;
		}

		case GF_JAM_DOOR: /* Jams a door (as if with a spike) */
		{
			if ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			    (c_ptr->feat <= FEAT_DOOR_TAIL))
			{
				/* Convert "locked" to "stuck" XXX XXX XXX */
				if (c_ptr->feat < FEAT_DOOR_HEAD + 0x08) c_ptr->feat += 0x08;

				/* Add one spike to the door */
				if (c_ptr->feat < FEAT_DOOR_TAIL) c_ptr->feat++;

				/* Check line of sight */
				if (player_has_los_bold(y, x))
				{
					/* Message */
					msg_print("The door seems stuck.");
					obvious = TRUE;
				}
			}
			break;
		}

		/* Destroy walls (and doors) */
		case GF_KILL_WALL:
		{
			/* Non-walls (etc) */
			if (cave_floor_bold_project(y, x)) break;

			/* Permanent walls */
			if (c_ptr->feat >= FEAT_PERM_EXTRA) break;

			/* Granite */
			if (c_ptr->feat >= FEAT_WALL_EXTRA)
			{
				/* Message */
				if (c_ptr->info & (CAVE_MARK))
				{
					msg_print("The wall turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the wall */
				cave_set_feat(y, x, FEAT_FLOOR);
			}

			/* Quartz / Magma with treasure */
			else if (c_ptr->feat >= FEAT_MAGMA_H)
			{
				/* Message */
				if (c_ptr->info & (CAVE_MARK))
				{
					msg_print("The vein turns into mud!");
					msg_print("You have found something!");
					obvious = TRUE;
				}

				/* Forget the wall */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the wall */
				cave_set_feat(y, x, FEAT_FLOOR);

				/* Place some gold */
				place_gold(y, x);
			}

			/* Quartz / Magma */
			else if (c_ptr->feat >= FEAT_MAGMA)
			{
				/* Message */
				if (c_ptr->info & (CAVE_MARK))
				{
					msg_print("The vein turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the wall */
				cave_set_feat(y, x, FEAT_FLOOR);
			}

			/* Rubble */
			else if (c_ptr->feat == FEAT_RUBBLE)
			{
				/* Message */
				if (c_ptr->info & (CAVE_MARK))
				{
					msg_print("The rubble turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the rubble */
				cave_set_feat(y, x, FEAT_FLOOR);

				/* Hack -- place an object */
				if (rand_int(100) < 10)
				{
					/* Found something */
					if (player_can_see_bold(y, x))
					{
						msg_print("There was something buried in the rubble!");
						obvious = TRUE;
					}

					/* Place gold */
					place_object(y, x, FALSE, FALSE);
				}
			}

			/* Destroy doors (and secret doors) */
			else /* if (c_ptr->feat >= FEAT_DOOR_HEAD) */
			{
				/* Hack -- special message */
				if (c_ptr->info & (CAVE_MARK))
				{
					msg_print("The door turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				c_ptr->info &= ~(CAVE_MARK);

				/* Destroy the feature */
				cave_set_feat(y, x, FEAT_FLOOR);
			}

			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);

			break;
		}

		/* Make doors */
		case GF_MAKE_DOOR:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(y, x)) break;

			/* Create a closed door */
			cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);

			/* Observe */
			if (c_ptr->info & (CAVE_MARK)) obvious = TRUE;

			/* Update some things */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_MONSTERS);

			break;
		}

		/* Make traps */
		case GF_MAKE_TRAP:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(y, x)) break;

			/* Place a trap */
			place_trap(y, x);

			break;
		}


		case GF_MAKE_GLYPH:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(y, x)) break;

			cave_set_feat(y, x, FEAT_GLYPH);

			break;
		}



		case GF_STONE_WALL:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(y, x)) break;

                        /* Place a wall */
			cave_set_feat(y, x, FEAT_WALL_EXTRA);

			break;
		}

                case GF_WINDS_MANA:
                {
                        if(dam >= 256)
                        {
                                /* With erase mana */

                                /* Absorb some of the mana of the grid */
                                p_ptr->csp += cave[y][x].mana / 80;
                                if(p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;

                                /* Set the new amount */
                                cave[y][x].mana = dam - 256;
                        }
                        else
                        {
                                /* Without erase mana */
                                int amt = cave[y][x].mana + dam;

                                /* Check if not overflow */
                                if(amt > 255) amt = 255;

                                /* Set the new amount */
                                cave[y][x].mana = amt;
                        }
                        break;
                }

                case GF_LAVA_FLOW:
		{
                        /* Shallow Lava */
                        if(dam == 1)
                        {
                                /* Require a "naked" floor grid */
                                if (!cave_naked_bold(y, x)) break;

                                /* Place a shallow lava */
                                cave_set_feat(y, x, FEAT_SHAL_LAVA);
                        }
                        /* Deep Lava */
                        else
                        {
                                /* Require a "naked" floor grid */
                                if (cave_perma_bold(y, x) || !dam) break;

                                /* Place a deep lava */
                                cave_set_feat(y, x, FEAT_DEEP_LAVA);

                                /* Dam is used as a counter for the number of grid to convert */
                                dam--;
                        }
			break;
		}

		/* Lite up the grid */
		case GF_LITE_WEAK:
		case GF_LITE:
		{
			/* Turn on the light */
			c_ptr->info |= (CAVE_GLOW);

			/* Notice */
			note_spot(y, x);

			/* Redraw */
			lite_spot(y, x);

			/* Observe */
			if (player_can_see_bold(y, x)) obvious = TRUE;

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx) update_mon(c_ptr->m_idx, FALSE);

			break;
		}

		/* Darken the grid */
		case GF_DARK_WEAK:
		case GF_DARK:
		{
			/* Notice */
			if (player_can_see_bold(y, x)) obvious = TRUE;

			/* Turn off the light. */
			c_ptr->info &= ~(CAVE_GLOW);

			/* Hack -- Forget "boring" grids */
			if (c_ptr->feat == FEAT_FLOOR)
			{
				/* Forget */
				c_ptr->info &= ~(CAVE_MARK);

				/* Notice */
				note_spot(y, x);
			}

			/* Redraw */
			lite_spot(y, x);

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx) update_mon(c_ptr->m_idx, FALSE);

			/* All done */
			break;
		}
                case GF_DESTRUCTION:
                {
                        int t;

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
                                break;;
			}

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
                        obvious = TRUE;
                        break;
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

	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * We are called from "project()" to "damage" objects
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Perhaps we should only SOMETIMES damage things on the ground.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 */
static bool project_o(int who, int r, int y, int x, s32b dam, int typ)
{
	cave_type *c_ptr = &cave[y][x];

	s16b this_o_idx, next_o_idx = 0;

	bool obvious = FALSE;

        u32b f1, f2, f3, f4;

	char o_name[80];

	int o_sval = 0;
	bool is_potion = FALSE;
        int xx, yy;


	/* XXX XXX XXX */
	who = who ? who : 0;

	/* Scan all objects in the grid */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
	
		bool is_art = FALSE;
		bool ignore = FALSE;
		bool plural = FALSE;
		bool do_kill = FALSE;

		cptr note_kill = NULL;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Extract the flags */
                object_flags(o_ptr, &f1, &f2, &f3, &f4);

		/* Get the "plural"-ness */
		if (o_ptr->number > 1) plural = TRUE;

		/* Check for artifact */
		if ((artifact_p(o_ptr) || o_ptr->art_name)) is_art = TRUE;

		/* Analyze the type */
		switch (typ)
		{
			/* Acid -- Lots of things */
			case GF_ACID:
			{
				if (hates_acid(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " melt!" : " melts!");
					if (f3 & (TR3_IGNORE_ACID)) ignore = TRUE;
				}
				break;
			}

			/* Elec -- Rings and Wands */
			case GF_ELEC:
			{
				if (hates_elec(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " are destroyed!" : " is destroyed!");
					if (f3 & (TR3_IGNORE_ELEC)) ignore = TRUE;
				}
				break;
			}

			/* Fire -- Flammable objects */
			case GF_FIRE:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " burn up!" : " burns up!");
					if (f3 & (TR3_IGNORE_FIRE)) ignore = TRUE;
				}
				break;
			}

			/* Cold -- potions and flasks */
			case GF_COLD:
			{
				if (hates_cold(o_ptr))
				{
					note_kill = (plural ? " shatter!" : " shatters!");
					do_kill = TRUE;
					if (f3 & (TR3_IGNORE_COLD)) ignore = TRUE;
				}
				break;
			}

			/* Fire + Elec */
			case GF_PLASMA:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " burn up!" : " burns up!");
					if (f3 & (TR3_IGNORE_FIRE)) ignore = TRUE;
				}
				if (hates_elec(o_ptr))
				{
					ignore = FALSE;
					do_kill = TRUE;
					note_kill = (plural ? " are destroyed!" : " is destroyed!");
					if (f3 & (TR3_IGNORE_ELEC)) ignore = TRUE;
				}
				break;
			}

			/* Fire + Cold */
			case GF_METEOR:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " burn up!" : " burns up!");
					if (f3 & (TR3_IGNORE_FIRE)) ignore = TRUE;
				}
				if (hates_cold(o_ptr))
				{
					ignore = FALSE;
					do_kill = TRUE;
					note_kill = (plural ? " shatter!" : " shatters!");
					if (f3 & (TR3_IGNORE_COLD)) ignore = TRUE;
				}
				break;
			}

			/* Hack -- break potions and such */
			case GF_ICE:
			case GF_EARTH:
			case GF_FORCE:
			case GF_SOUND:
			{
				if (hates_cold(o_ptr))
				{
					note_kill = (plural ? " shatter!" : " shatters!");
					do_kill = TRUE;
				}
				break;
			}

			/* Mana and Chaos -- destroy everything */
			case GF_MANA: 
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				break;
			}

			case GF_DISINTEGRATE:
			{
				do_kill = TRUE;
				note_kill = (plural ? " evaporate!" : " evaporates!");
				break;
			}

			case GF_CHAOS:
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f2 & (TR2_RES_CHAOS)) ignore = TRUE;
				break;
			}

			/* Holy Fire and Hell Fire -- destroys cursed non-artifacts */
			case GF_HOLY_FIRE:
			case GF_HELL_FIRE:
			{
				if (cursed_p(o_ptr))
				{
					do_kill = TRUE;
					note_kill = (plural ? " are destroyed!" : " is destroyed!");
				}
				break;
			}

			/* Unlock chests */
			case GF_KILL_TRAP:
			case GF_KILL_DOOR:
			{
				/* Chests are noticed only if trapped or locked */
				if (o_ptr->tval == TV_CHEST)
				{
					/* Disarm/Unlock traps */
					if (o_ptr->pval > 0)
					{
						/* Disarm or Unlock */
						o_ptr->pval = (0 - o_ptr->pval);

						/* Identify */
						object_known(o_ptr);

						/* Notice */
						if (o_ptr->marked)
						{
							msg_print("Click!");
							obvious = TRUE;
						}
					}
				}

				break;
			}
                        case GF_STAR_IDENTIFY:
                        {
                                /* Identify it fully */
                                object_aware(o_ptr);
                                object_known(o_ptr);

                                /* Mark the item as fully known */
                                o_ptr->ident |= (IDENT_MENTAL);
                                break;
                        }
                        case GF_IDENTIFY:
                        {
                                object_aware(o_ptr);
                                object_known(o_ptr);
                                break;
                        }
                        case GF_RAISE:
                        {
                                xx=x;
                                yy=y;
                                get_pos_player(100, &y, &x);
                                if(place_monster_one(y, x, o_ptr->pval2, FALSE, TRUE, 0))
                                        msg_print("A monster raise from the grave!");
				do_kill = TRUE;
                                break;
                        }
		}


		/* Attempt to destroy the object */
		if (do_kill)
		{
			/* Effect "observed" */
			if (o_ptr->marked)
			{
				obvious = TRUE;
				object_desc(o_name, o_ptr, FALSE, 0);
			}

			/* Artifacts, and other objects, get to resist */
			if (is_art || ignore)
			{
				/* Observe the resist */
				if (o_ptr->marked)
				{
					msg_format("The %s %s unaffected!",
						   o_name, (plural ? "are" : "is"));
				}
			}

			/* Kill it */
			else
			{
				/* Describe if needed */
				if (o_ptr->marked && note_kill)
				{
					msg_format("The %s%s", o_name, note_kill);
				}

				o_sval = o_ptr->sval;
                                is_potion = ((k_info[o_ptr->k_idx].tval == TV_POTION)||(k_info[o_ptr->k_idx].tval == TV_POTION2));


				/* Delete the object */
				delete_object_idx(this_o_idx);

				/* Potions produce effects when 'shattered' */
				if (is_potion)
				{
					(void)potion_smash_effect(who, y, x, o_sval);
				}
                

				/* Redraw */
				lite_spot(y, x);
			}
		}
	}

	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to a monster.
 *
 * This routine takes a "source monster" (by index) which is mostly used to
 * determine if the player is causing the damage, and a "radius" (see below),
 * which is used to decrease the power of explosions with distance, and a
 * location, via integers which are modified by certain types of attacks
 * (polymorph and teleport being the obvious ones), a default damage, which
 * is modified as needed based on various properties, and finally a "damage
 * type" (see below).
 *
 * Note that this routine can handle "no damage" attacks (like teleport) by
 * taking a "zero" damage, and can even take "parameters" to attacks (like
 * confuse) by accepting a "damage", using it to calculate the effect, and
 * then setting the damage to zero.  Note that the "damage" parameter is
 * divided by the radius, so monsters not at the "epicenter" will not take
 * as much damage (or whatever)...
 *
 * Note that "polymorph" is dangerous, since a failure in "place_monster()"'
 * may result in a dereference of an invalid pointer.  XXX XXX XXX
 *
 * Various messages are produced, and damage is applied.
 *
 * Just "casting" a substance (i.e. plasma) does not make you immune, you must
 * actually be "made" of that substance, or "breathe" big balls of it.
 *
 * We assume that "Plasma" monsters, and "Plasma" breathers, are immune
 * to plasma.
 *
 * We assume "Nether" is an evil, necromantic force, so it doesn't hurt undead,
 * and hurts evil less.  If can breath nether, then it resists it as well.
 *
 * Damage reductions use the following formulas:
 *   Note that "dam = dam * 6 / (randint(6) + 6);"
 *     gives avg damage of .655, ranging from .858 to .500
 *   Note that "dam = dam * 5 / (randint(6) + 6);"
 *     gives avg damage of .544, ranging from .714 to .417
 *   Note that "dam = dam * 4 / (randint(6) + 6);"
 *     gives avg damage of .444, ranging from .556 to .333
 *   Note that "dam = dam * 3 / (randint(6) + 6);"
 *     gives avg damage of .327, ranging from .427 to .250
 *   Note that "dam = dam * 2 / (randint(6) + 6);"
 *     gives something simple.
 *
 * In this function, "result" messages are postponed until the end, where
 * the "note" string is appended to the monster name, if not NULL.  So,
 * to make a spell have "no effect" just set "note" to NULL.  You should
 * also set "notice" to FALSE, or the player will learn what the spell does.
 *
 * We attempt to return "TRUE" if the player saw anything "useful" happen.
 */
bool project_m(int who, int r, int y, int x, s32b dam, int typ)
{
	int tmp;

	cave_type *c_ptr = &cave[y][x];

	monster_type *m_ptr = &m_list[c_ptr->m_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int m_idx = c_ptr->m_idx;

	char killer [80];

        /* cptr name = (r_name + r_ptr->name); */

	s32b            div, new_exp, new_exp_frac;

	/* Is the monster "seen"? */
	bool seen = m_ptr->ml;

	/* Were the effects "obvious" (if seen)? */
	bool obvious = FALSE;

	/* Were the effects "irrelevant"? */
	bool skipped = FALSE;


	/* Polymorph setting (true or false) */
	int do_poly = 0;

	/* Teleport setting (max distance) */
	int do_dist = 0;

	/* Confusion setting (amount to confuse) */
	int do_conf = 0;

	/* Stunning setting (amount to stun) */
	int do_stun = 0;

	/* Sleep amount (amount to sleep) */
	int do_sleep = 0;

	/* Fear amount (amount to fear) */
	int do_fear = 0;


	/* Hold the monster name */
	char m_name[80];

	/* Assume no note */
	cptr note = NULL;

	/* Assume a default death */
	cptr note_dies = " dies.";



	/* Walls protect monsters */
	/* (No, they don't)  */
#if 0
	if (!cave_floor_bold(y,x)) return (FALSE);
#endif

	/* Nobody here */
	if (!c_ptr->m_idx) return (FALSE);

	/* Never affect projector */
	if (who && (c_ptr->m_idx == who)) return (FALSE);

	/* Don't affect already death monsters */
	/* Prevents problems with chain reactions of exploding monsters */
	if (m_ptr->hp < 0) return (FALSE);

	/* Get the monster name (BEFORE polymorphing) */
	monster_desc(m_name, m_ptr, 0);

	/* If the player used a ranged attack... */
	if (monster_ranged)
	{
		int hit;
		call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
		if (hit == 1)
		{
			if ((r_ptr->countertype == 16 || r_ptr->countertype == 17 || r_ptr->countertype == 18 || r_ptr->countertype == 19) && randint(100) <= r_ptr->counterchance)
			{
				if (randint(m_ptr->dex) >= randint(p_ptr->stat_ind[A_DEX]))
				{
                                	msg_format("%s blocked your ammo!", m_name);
					return (FALSE);
				}
			}
			if ((r_ptr->countertype == 20 || r_ptr->countertype == 21 || r_ptr->countertype == 22 || r_ptr->countertype == 23) && randint(100) <= r_ptr->counterchance)
			{
                        	msg_format("%s blocked your ammo!", m_name);
				return (FALSE);
			}
		}
		else
		{
			msg_format("You miss %s.", m_name);
			return (FALSE);
		}
	}


	/* Some monsters get "destroyed" */
	if ((r_ptr->flags3 & (RF3_DEMON)) ||
	    (r_ptr->flags3 & (RF3_UNDEAD)) ||
	    (r_ptr->flags2 & (RF2_STUPID)) ||
	    (r_ptr->flags3 & (RF3_NONLIVING)) ||
	    (strchr("Evg", r_ptr->d_char)))
	{
		/* Special note at death */
		note_dies = " is destroyed.";
	}

	if (!who && is_pet(m_ptr))
	{
		bool get_angry = FALSE;
		/* Grrr? */
		switch (typ)
		{
			case GF_AWAY_UNDEAD:
			case GF_AWAY_EVIL:
			case GF_CHARM:
                        case GF_STAR_CHARM:
                        case GF_LITE_CONTROL:
			case GF_CONTROL_UNDEAD:
			case GF_CONTROL_ANIMAL:
			case GF_OLD_HEAL:
			case GF_OLD_SPEED:
			case GF_DARK_WEAK:
			case GF_JAM_DOOR:
                        case GF_RAISE:
                        case GF_WAR_BLESSING:
                        case GF_MORALE_BOOST:
                        case GF_UNSUMMON:
                        case GF_ANIMAL_EMPATHY:
                        case GF_AURA_LIFE:
                        case GF_EVOLVE:
                        case GF_UNEVOLVE:
				break;             /* none of the above anger */
			case GF_KILL_WALL:
				if (r_ptr->flags3 & (RF3_HURT_ROCK))
					get_angry = TRUE;
				break;
			case GF_HOLY_FIRE:
				if (!(r_ptr->flags3 & (RF3_GOOD)))
					get_angry = TRUE;
				break;
			case GF_TURN_UNDEAD:
			case GF_DISP_UNDEAD:
				if (r_ptr->flags3 & RF3_UNDEAD)
					get_angry = TRUE;
				break;
			case GF_TURN_EVIL:
			case GF_DISP_EVIL:
				if (r_ptr->flags3 & RF3_EVIL)
					get_angry = TRUE;
				break;
			case GF_DISP_GOOD:
				if (r_ptr->flags3 & RF3_GOOD)
					get_angry = TRUE;
				break;
			case GF_DISP_DEMON:
				if (r_ptr->flags3 & RF3_DEMON)
					get_angry = TRUE;
				break;
			case GF_DISP_LIVING:
				if (!(r_ptr->flags3 & (RF3_UNDEAD)) &&
				    !(r_ptr->flags3 & (RF3_NONLIVING)))
					get_angry = TRUE;
				break;
			case GF_PSI:
			case GF_PSI_DRAIN:
                        case GF_PSI_HITRATE:
                        case GF_PSI_FEAR:
				if (!(r_ptr->flags2 & (RF2_EMPTY_MIND)))
					get_angry = TRUE;
				break;
			case GF_DOMINATION:
				if (!(r_ptr->flags3 & (RF3_NO_CONF)))
					get_angry = TRUE;
				break;
			case GF_OLD_POLY:
			case GF_OLD_CLONE:
				if (randint(8) == 1)
					get_angry = TRUE;
				break;
			case GF_LITE:
			case GF_LITE_WEAK:
				if (r_ptr->flags3 & RF3_HURT_LITE)
					get_angry = TRUE;
				break;
			default:
				get_angry = TRUE;
		}

		/* Now anger it if appropriate */
                if (get_angry && !(who) && m_ptr->friend == 0 && p_ptr->prace != RACE_MONSTER)
		{
                        m_ptr->angered_pet = 1;
			msg_format("%^s gets angry!", m_name);
			set_pet(m_ptr, FALSE);
		}
	}


	/* Analyze the damage type */
	switch (typ)
	{
		/* Magic Missile -- pure damage */
                case GF_DEATH:
		{
                        if (seen) obvious = TRUE;

                        if(r_ptr->r_flags1 & RF1_UNIQUE)
                        {
                                note = " resists.";
                                dam = 0;
                        }
                        else
                        {
                                /* It KILLS */
                                dam = m_ptr->hp + 1;
                        }
                        break;
		}
		/* Magic Missile */
		/* Does both physical and magical damages! */
		case GF_MISSILE:
		{
			s32b phydam = dam / 2;
			s32b magdam = dam / 2;
			if (seen) obvious = TRUE;
			phydam -= ((phydam * r_ptr->physres) / 100);
			magdam -= ((magdam * r_ptr->manares) / 100);
			dam = phydam + magdam;
			r_ptr->r_resist[GF_PHYSICAL] = 1;
			r_ptr->r_resist[GF_MANA] = 1;
			break;
		}

		/* Acid */
		case GF_ACID:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->acidres) / 100);
			r_ptr->r_resist[GF_ACID] = 1;
			break;
		}

		/* Electricity */
		case GF_ELEC:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->elecres) / 100);
			r_ptr->r_resist[GF_ELEC] = 1;
			break;
		}

		/* Fire damage */
		case GF_FIRE:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->fireres) / 100);
			r_ptr->r_resist[GF_FIRE] = 1;
			break;
		}

		/* Cold */
		case GF_COLD:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->coldres) / 100);
			r_ptr->r_resist[GF_COLD] = 1;
			break;
		}

		/* Poison */
		case GF_POIS:
		{
			if (seen) obvious = TRUE;
                        /* Kobolds get a bonus to Poison damages! */
                        if (p_ptr->prace == RACE_KOBOLD) dam = dam + (dam / 2);
			dam -= ((dam * r_ptr->poisres) / 100);
			r_ptr->r_resist[GF_POIS] = 1;
			break;
		}

		/* Nuclear waste */
		case GF_RADIO:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->radiores) / 100);
			r_ptr->r_resist[GF_RADIO] = 1;
			break;
		}

		/* Toxic! */
		/* A combination of Acid, Poison and Radio! */
		case GF_TOXIC:
		{
			s32b aciddam = dam / 3;
			s32b poisdam = dam / 3;
			s32b radiodam = dam / 3;
			if (seen) obvious = TRUE;
			aciddam -= ((aciddam * r_ptr->acidres) / 100);
			poisdam -= ((poisdam * r_ptr->poisres) / 100);
			radiodam -= ((radiodam * r_ptr->radiores) / 100);
			dam = aciddam + poisdam + radiodam;
			r_ptr->r_resist[GF_ACID] = 1;
			r_ptr->r_resist[GF_POIS] = 1;
			r_ptr->r_resist[GF_RADIO] = 1;
			break;
		}

		/* FrostFire! */
		/* A combination of Fire and Cold! */
		case GF_FROSTFIRE:
		{
			s32b firedam = dam / 2;
			s32b colddam = dam / 2;
			if (seen) obvious = TRUE;
			firedam -= ((firedam * r_ptr->fireres) / 100);
			colddam -= ((colddam * r_ptr->coldres) / 100);
			dam = firedam + colddam;
			r_ptr->r_resist[GF_FIRE] = 1;
			r_ptr->r_resist[GF_COLD] = 1;
			break;
		}

		/* Grey! */
		/* A combination of Light and Dark! */
		case GF_GREY:
		{
			s32b litedam = dam / 2;
			s32b darkdam = dam / 2;
			if (seen) obvious = TRUE;
			litedam -= ((litedam * r_ptr->lightres) / 100);
			darkdam -= ((darkdam * r_ptr->darkres) / 100);
			dam = litedam + darkdam;
			r_ptr->r_resist[GF_LITE] = 1;
			r_ptr->r_resist[GF_DARK] = 1;
			break;
		}

		/* Holy Orb -- hurts Evil (replaced with Hellfire) */
		case GF_HELL_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				dam *= 2;
				note = " is hit hard.";
				if (seen) r_ptr->r_flags3 |= (RF3_EVIL);
			}
			break;
		}

		/* Holy Fire -- hurts Evil, Good are immune, others _resist_ */
		case GF_HOLY_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_GOOD))
			{
				dam = 0;
				note = " is immune.";
				if (seen) r_ptr->r_flags3 |= (RF3_GOOD);
			}
			else if (r_ptr->flags3 & (RF3_EVIL))
			{
				dam *= 2;
				note = " is hit hard.";
				if (seen) r_ptr->r_flags3 |= (RF3_EVIL);
			}
			else
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
			}
			break;
		}

		/* Arrow -- XXX no defense */
		case GF_ARROW:
		{
			if (seen) obvious = TRUE;
			break;
		}


		/* Water */
		case GF_WATER:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->waterres) / 100);
			r_ptr->r_resist[GF_WATER] = 1;
			break;
		}

		/* Chaos -- Chaos breathers resist */
		case GF_CHAOS:
		{
			if (seen) obvious = TRUE;
                        /* Zulgors get a bonus to Chaos damages! */
                        if (p_ptr->prace == RACE_ZULGOR) dam = dam + (dam / 4);
			dam -= ((dam * r_ptr->chaosres) / 100);
			r_ptr->r_resist[GF_CHAOS] = 1;
			break;
		}

		/* Shards -- Shard breathers resist */
		case GF_EARTH:
		{			
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->earthres) / 100);
			r_ptr->r_resist[GF_EARTH] = 1;
			break;
		}


		/* Sound -- Sound breathers resist */
		case GF_SOUND:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->soundres) / 100);
			r_ptr->r_resist[GF_SOUND] = 1;
			break;
		}

		/* Confusion */
		case GF_CONFUSION:
		{
			int ppower = (p_ptr->lev);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_NO_CONF))
			{
                                dam /= 3;
			}
			dam -= ((dam * (p_ptr->chaosres)) / 100);
			dam -= ((dam * (p_ptr->manares)) / 100);
			if (dam > 0 && !(r_ptr->flags3 & (RF3_NO_CONF)) && !(r_ptr->flags1 * (RF1_UNIQUE)) && m_ptr->boss <= 0)
			{
				if (randint(ppower) >= randint(mpower))
                                	do_conf = 10;
			}
			break;
		}

		/* Disenchantment -- Breathers and Disenchanters resist */
		case GF_DISENCHANT:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_RES_DISE))
			{
				note = " resists.";
				dam *= 3; dam /= (randint(6)+6);
				if (seen) r_ptr->r_flags3 |= (RF3_RES_DISE);
			}
			break;
		}

                /* Wind */
                case GF_WIND:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->windres) / 100);
			r_ptr->r_resist[GF_WIND] = 1;
			break;
		}


		/* Pure damage */
		/* Can theorically be resisted now... */
		/* But please don't abuse mana resistance! */
		case GF_MANA:
		{
			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->manares) / 100);
			r_ptr->r_resist[GF_MANA] = 1;
			break;
		}


		/* Pure damage */
		case GF_DISINTEGRATE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & (RF3_HURT_ROCK))
			{
				if (seen) r_ptr->r_flags3 |= (RF3_HURT_ROCK);
				note = " loses some skin!";
				note_dies = " evaporates!";
				dam *= 2;
			}

			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				if (rand_int(r_ptr->level + 10) > rand_int(p_ptr->lev))
				{
					note = " resists.";
					dam >>= 3;
				}
			}
			break;
		}

                case GF_FEAR:
                {
			int ppower = (dam + p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS]);
			int mpower = (m_ptr->level + m_ptr->mind);
                        if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags3 & (RF3_NO_FEAR)))
                                note = " is unaffected.";
                        else
			{
				if (randint(ppower) >= randint(mpower))
                                	do_fear = dam;
				else note = " resists.";
			}

                        /* No damage */
                        dam = 0;
                        break;
                }

		/* These doesn't do anything on other monsters. */
		case GF_LOSE_STR:
		case GF_LOSE_INT:
		case GF_LOSE_WIS:
		case GF_LOSE_DEX:
		case GF_LOSE_CON:
		case GF_LOSE_CHR:
		case GF_LOSE_ALL:
		case GF_LOSE_EXP:
		{
			dam = 0;
			break;
		}

		case GF_PSI:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				dam = 0;
				note = " is immune!";
			}
                        /* else if ((r_ptr->flags2 & RF2_STUPID) ||    */
                        /*         (r_ptr->flags2 & RF2_WEIRD_MIND) || */
                        /*         (r_ptr->flags3 & RF3_ANIMAL) ||     */
                        /*         (r_ptr->level > randint(3 * dam)))  */
                        /*                                             */
                        else
                        {
                                if ((randint(100) >= 50) && !(r_ptr->flags3 & (RF3_NO_CONF)) && !(r_ptr->flags1 & (RF1_UNIQUE)) && m_ptr->boss <= 0)
                                {
                                        do_conf = 10;
                                }
                        }
			break;
		}

                case GF_PSI_HITRATE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				dam = 0;
				note = " is immune!";
			}
                        else
                        {
                                if (randint(100) <= 50 && !(m_ptr->abilities & (PSYCHIC_HITRATE)))
                                {
                                        note = " is blinded by the illusions!";
                                        m_ptr->hitrate = m_ptr->hitrate / 2;
                                        m_ptr->abilities |= (PSYCHIC_HITRATE);
                                }
                        }
			break;
		}
                case GF_PSI_FEAR:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				dam = 0;
				note = " is immune!";
			}
                        else
                        {
                                if ((randint(100) >= 50) && !(r_ptr->flags3 & (RF3_NO_FEAR)) && !(r_ptr->flags1 & (RF1_UNIQUE)) && m_ptr->boss <= 0)
                                {
                                        do_fear = 10;
                                }
                        }
			break;
		}

	    
		case GF_PSI_DRAIN:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				dam = 0;
				note = " is immune!";
			}
			else if ((r_ptr->flags2 & RF2_STUPID) ||
			         (r_ptr->flags2 & RF2_WEIRD_MIND) ||
			         (r_ptr->flags3 & RF3_ANIMAL) || 
			         (r_ptr->level > randint(3 * dam)))
			{
				dam /= 3;
				note = " resists.";

				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if (((r_ptr->flags3 & RF3_UNDEAD) ||
				     (r_ptr->flags3 & RF3_DEMON)) &&
				     (r_ptr->level > p_ptr->lev/2) && 
				     (randint(2) == 1))
				{
					note = NULL;
					msg_format("%^s%s corrupted mind backlashes your attack!",
					    m_name, (seen ? "'s" : "s"));
					/* Saving throw */
                                        if (rand_int(100) < p_ptr->stat_ind[A_WIS])
					{
						msg_print("You resist the effects!");
					}
					else
					{
						/* Injure + mana drain */
						monster_desc(killer, m_ptr, 0x88);
						msg_print("Your psychic energy is drained!");
						p_ptr->csp = MAX(0, p_ptr->csp - damroll(5, dam)/2);
						p_ptr->redraw |= PR_MANA;
						p_ptr->window |= (PW_SPELL);
						take_hit(dam, killer);  /* has already been /3 */
					}
					dam = 0;
				}
			}
			else if (dam > 0)
			{
				int b = damroll(5, dam) / 4;
				msg_format("You convert %s%s pain into psychic energy!",
				    m_name, (seen ? "'s" : "s"));
				b = MIN(p_ptr->msp, p_ptr->csp + b);
				p_ptr->csp = b;
				p_ptr->redraw |= PR_MANA;
				p_ptr->window |= (PW_SPELL);
			}

			note_dies = " collapses, a mindless husk.";
			break;
		}

		case GF_TELEKINESIS:
		{
			if (seen) obvious = TRUE;
			do_dist = 7;
			/* 1. stun */
			do_stun = damroll((p_ptr->lev / 10) + 3 , (dam)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->level > 5 + randint(dam)))
			{
				/* Resist */
				do_stun = 0;
				/* No obvious effect */
				obvious = FALSE;
			}
			break;
		}

		/* Meteor -- powerful magic missile */
		case GF_METEOR:
		{
			if (seen) obvious = TRUE;
			break;
		}

		case GF_DOMINATION:
		{
			if (is_pet(m_ptr)) break;
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->flags3 & (RF3_NO_CONF)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				/* Resist */
				do_conf = 0;
			        
				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if (((r_ptr->flags3 & RF3_UNDEAD) ||
				     (r_ptr->flags3 & RF3_DEMON)) &&
				     (r_ptr->level > p_ptr->lev/2) &&
				     (randint(2) == 1))
				{
					note = NULL;
					msg_format("%^s%s corrupted mind backlashes your attack!",
					    m_name, (seen ? "'s" : "s"));
					/* Saving throw */
                                        if (rand_int(100) < p_ptr->stat_ind[A_WIS])
					{
						msg_print("You resist the effects!");
					}
					else
					{
						/* Confuse, stun, terrify */
						switch (randint(4))
						{
							case 1:
								set_stun(p_ptr->stun + dam / 2);
								break;
							case 2:
								set_confused(p_ptr->confused + dam / 2);
								break;
							default:
							{
								if (r_ptr->flags3 & (RF3_NO_FEAR))
									note = " is unaffected.";
								else
									set_afraid(p_ptr->afraid + dam);
							}
						}
					}
				}
				else
				{
					/* No obvious effect */
					note = " is unaffected!";
					obvious = FALSE;
				}
			}
			else
			{
				if ((dam > 29) && (randint(100) < dam))
				{
					note = " is in your thrall!";
					set_pet(m_ptr, TRUE);
				}
				else
				{
					switch (randint(4))
					{
						case 1:
							do_stun = dam/2;
							break;
						case 2:
							do_conf = dam/2;
							break;
						default:
							do_fear = dam;
					}
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}



		/* Ice -- Cold + Cuts + Stun */
		case GF_ICE:
		{
			if (seen) obvious = TRUE;
			do_stun = (randint(15) + 1) / (r + 1);
                        if (r_ptr->flags3 & (RF3_SUSCEP_COLD))
			{
                                note = " is hit hard.";
                                dam *= 3;
                                if (seen) r_ptr->r_flags3 |= (RF3_SUSCEP_COLD);
			}
			if (r_ptr->flags3 & (RF3_IM_COLD))
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen) r_ptr->r_flags3 |= (RF3_IM_COLD);
			}
			break;
		}


		/* Drain Life */
		case GF_OLD_DRAIN:
		{
			if (seen) obvious = TRUE;

			if ((r_ptr->flags3 & (RF3_UNDEAD)) ||
			    (r_ptr->flags3 & (RF3_DEMON)) ||
			    (r_ptr->flags3 & (RF3_NONLIVING)) ||
			    (strchr("Egv", r_ptr->d_char)))
			{
				if (r_ptr->flags3 & (RF3_UNDEAD))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_UNDEAD);
				}
				if (r_ptr->flags3 & (RF3_DEMON))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_DEMON);
				}

				note = " is unaffected!";
				obvious = FALSE;
				dam = 0;
			}

			break;
		}

		/* Death Ray */
		case GF_DEATH_RAY:
		{
#if 0
			dam = 0;
#endif
			if (seen) obvious = TRUE;
			if ((r_ptr->flags3 & (RF3_UNDEAD)) ||
			    (r_ptr->flags3 & (RF3_NONLIVING)))
			{
				if (r_ptr->flags3 & (RF3_UNDEAD))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_UNDEAD);
				}

				note = " is immune.";
				obvious = FALSE;
				dam = 0;
			}
			else if (((r_ptr->flags1 & (RF1_UNIQUE)) &&
			    (randint(888) != 666)) ||
			    (((r_ptr->level + randint(20))> randint((dam)+randint(10))) &&
			    randint(100) != 66 ))
			{
				note = " resists!";
				obvious = FALSE;
				dam = 0;
			}

			else dam = (p_ptr->lev) * 200;

			break;
		}

		/* Polymorph monster (Use "dam" as "power") */
		case GF_OLD_POLY:
		{
			int ppower = (dam + p_ptr->lev);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

			/* Attempt to polymorph (see below) */
			do_poly = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags1 & RF1_QUESTOR) ||
			    m_ptr->boss >= 1)
			{
				note = " is unaffected!";
				do_poly = FALSE;
				obvious = FALSE;
			}
			else
			{
				if (randint(ppower) < randint(mpower))
				{
					note = " resists.";
					do_poly = FALSE;
					obvious = FALSE;
				}
			}

			/* No "real" damage */
			dam = 0;

			break;
		}


		/* Clone monsters (Ignore "dam") */
		case GF_OLD_CLONE:
		{
			bool is_friend = FALSE;

			if (seen) obvious = TRUE;
			if (is_pet(m_ptr) && (randint(3)!=1))
				is_friend = TRUE;

			/* Heal fully */
			m_ptr->hp = m_ptr->maxhp;

			/* Speed up */
			if (m_ptr->mspeed < 150) m_ptr->mspeed += 10;

			/* Attempt to clone. */
			if (multiply_monster(c_ptr->m_idx, is_friend, TRUE))
			{
				note = " spawns!";
			}

			/* No "real" damage */
			dam = 0;

			break;
		}


		/* Heal Monster (use "dam" as amount of healing) */
		case GF_OLD_HEAL:
		{
			if (seen) obvious = TRUE;

			/* Wake up */
			m_ptr->csleep = 0;

			/* Heal */
			m_ptr->hp += dam;

			/* No overflow */
                        if ((m_ptr->hp > m_ptr->maxhp) || (m_ptr->hp < 0)) m_ptr->hp = m_ptr->maxhp;

			/* Redraw (later) if needed */
			if (health_who == c_ptr->m_idx) p_ptr->redraw |= (PR_HEALTH);

			/* Message */
			note = " looks healthier.";

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Speed Monster (Ignore "dam") */
		case GF_OLD_SPEED:
		{
			if (seen) obvious = TRUE;

			/* Speed up */
			m_ptr->mspeed += dam;
			if (m_ptr->mspeed > 180) m_ptr->mspeed = 180;
			note = " starts moving faster.";

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Slow Monster (Use "dam" as "power") */
		case GF_OLD_SLOW:
		{
			int ppower = (dam + p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    m_ptr->boss >= 1)
			{
				note = " is unaffected!";
				obvious = FALSE;
			}

			/* Normal monsters slow down */
			else
			{
				if (randint(ppower) >= randint(mpower))
				{
					if (m_ptr->mspeed > 60) m_ptr->mspeed -= 10;
					note = " starts moving slower.";
				}
				else note = " resists.";
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Sleep (Use "dam" as "power") */
		case GF_OLD_SLEEP:
		{
			int ppower = (dam + p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->flags3 & (RF3_NO_SLEEP)) ||
			    m_ptr->boss >= 1)
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_SLEEP))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_SLEEP);
				}

                                note = " is unaffected!";
                                obvious = FALSE;
			}
			else
			{
				if (randint(ppower) >= randint(mpower))
				{
					/* Go to sleep (much) later */
					note = " falls asleep!";
					do_sleep = 500;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Sleep (Use "dam" as "power") */
		case GF_STASIS:
		{
			int ppower = (dam + p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				obvious = FALSE;
			}
			else
			{
				if (randint(ppower) >= randint(mpower))
				{
					/* Go to sleep (much) later */
					note = " is suspended!";
					do_sleep = 500;
				}
				else note = " resists.";
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Charm monster */
		case GF_CHARM:
		{
			int ppower = (dam + p_ptr->lev);
			int mpower = (m_ptr->level + m_ptr->mind);
			dam += (adj_con_fix[p_ptr->stat_ind[A_CHR]] - 1);

			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags1 & RF1_QUESTOR) ||
                            (r_ptr->flags3 & RF3_NO_CONF) || m_ptr->boss >= 1)
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				/* Resist */
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else if (p_ptr->aggravate)
			{
				note = " hates you too much!";
			}
			else
			{
				if (randint(ppower) >= randint(mpower))
				{
					note = " suddenly seems friendly!";
					set_pet(m_ptr, TRUE);
				}
				else note = " resists.";
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* *Charm* monster */
                case GF_STAR_CHARM:
		{
			dam += (adj_con_fix[p_ptr->stat_ind[A_CHR]] - 1);

			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags1 & RF1_QUESTOR) ||
			    (r_ptr->flags3 & RF3_NO_CONF) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 5))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				/* Resist */
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else if (p_ptr->aggravate)
			{
				note = " hates you too much!";
			}
			else
			{
				note = " suddenly seems friendly!";
				set_pet(m_ptr, TRUE);
                                m_ptr->imprinted = TRUE;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

                case GF_LITE_CONTROL:
		{
			dam += (adj_con_fix[p_ptr->stat_ind[A_CHR]] - 1);

			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
                        if ((r_ptr->flags1 & RF1_QUESTOR) || (r_ptr->flags1 & RF1_UNIQUE) || (m_ptr->boss != 0))
			{
				/* Resist */
				/* No obvious effect */
                                note = " cannot be recruited in your gang!";
				obvious = FALSE;
			}
                        else
			{
                                int hpper, chance;
                                /* Basic 100% chance */
                                chance = 100;
                                /* The stronger, the less likely it will follow... */
                                hpper = (m_ptr->hp * 100) / m_ptr->maxhp;
                                chance -= hpper;
                                /* Add your level to the chance! */
                                chance += p_ptr->lev;
                                /* But the monster's level also affect your chances... */
                                chance -= m_ptr->level;
                                if (rand_int(100) <= chance)
                                {
                                        note = " was recruited in your gang!";
                                        set_pet(m_ptr, TRUE);
                                        m_ptr->imprinted = TRUE;
                                        m_ptr->friend = TRUE;
                                        /* msg_print("Turn the monster into a crystal? y/n");
                                        b = inkey();
                                        if (b == 'y' || b == 'Y')
                                        {
                                                turn_in_crystal(m_ptr);
                                        } */
                                }
                                else note = " does not want to join you!";
                                
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

                /* It is called DRAGON_CONTROL for *HUGE* */
                /* Historical reasons... :) */
                case GF_DRAGON_CONTROL:
		{
			dam += (adj_con_fix[p_ptr->stat_ind[A_CHR]] - 1);

			if (seen) obvious = TRUE;

                        note = " is hit!";
			break;
		}

                /* And now our little and fun curses! */
                case GF_WEAKEN:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 1)
                        {
				if (randint(ppower) >= randint(mpower))
				{
                        		m_ptr->level -= dam;
                                	if (m_ptr->level < 1) m_ptr->level = 1;
					apply_monster_level_stats(m_ptr);
                                	note = " has been weakened.";
				}
				else note = " resists.";
                        }
                        else note = " cannot be cursed!";
                        dam = 0;
			break;
		}

                case GF_LOWER_POWER:
		{

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
                                if (m_ptr->abilities & (CURSE_LOWER_POWER))
                                {
                                        if (p_ptr->pclass != CLASS_DARK_LORD) note = " has already received this curse!";
                                }
                                else
                                {
                                        m_ptr->abilities |= (CURSE_LOWER_POWER);
                                        note = "'s power is lowered!";
                                }
                        }
                        else note = " cannot be cursed!";
			break;
		}
                case GF_LOWER_MAGIC:
		{

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
                                if (m_ptr->abilities & (CURSE_LOWER_MAGIC))
                                {
                                        if (p_ptr->pclass != CLASS_DARK_LORD) note = " has already received this curse!";
                                }
                                else
                                {
                                        m_ptr->abilities |= (CURSE_LOWER_MAGIC);
                                        note = "'s power is lowered!";
                                }
                        }
                        else note = " cannot be cursed!";
			break;
		}
                case GF_SLOW_DOWN:
		{
			int ppower, mpower;
			ppower = p_ptr->abilities[(CLASS_MAGE * 10) + 4] * 10;
			mpower = m_ptr->level + m_ptr->mind;
			if (seen) obvious = TRUE;

			if (randint(ppower) >= randint(mpower))
			{
                        	/* Give the curse */
                        	if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 1)
                        	{
                                	if (m_ptr->abilities & (CURSE_SLOW_DOWN))
                                	{
                                        	if (p_ptr->pclass != CLASS_DARK_LORD) note = " has already been slowed down!";
                                	}
                                	else
                                	{
                                        	int speedfrac;
                                        	speedfrac = (m_ptr->mspeed * (5 + (p_ptr->abilities[(CLASS_MAGE * 10) + 4] / 2))) / 100;
                                        	m_ptr->mspeed -= speedfrac;
                                        	m_ptr->abilities |= (CURSE_SLOW_DOWN);
                                        	note = " is now slower.";
                                	}
                        	}
                        	else note = " cannot be cursed!";
			}
			else note = " resists.";
			break;
		}
                case GF_LIFE_BLAST:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (randint(ppower) >= randint(mpower))
			{
                        	dam = (m_ptr->maxhp * dam) / 100;
				if ((m_ptr->hp - dam) == 0) dam += 1;
                        	note = "'s hp has been blasted!";
                        }
                        else
			{
				note = " resists.";
				dam = 0;
			}
			break;
		}
                case GF_HALVE_DAMAGES:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
				if (randint(ppower) >= randint(mpower))
				{
                                	if (m_ptr->abilities & (CURSE_HALVE_DAMAGES)) note = " has already received this curse!";
                                	else
                                	{
                                        	m_ptr->abilities |= (CURSE_HALVE_DAMAGES);
                                        	note = "'s power has been greatly lowered!";
                                	}
				}
				else note = " resists.";
                        }
                        else note = " cannot be cursed!";
			break;
		}
                case GF_HALVE_MAGIC:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
				if (randint(ppower) >= randint(mpower))
				{
                                	if (m_ptr->abilities & (CURSE_HALVE_MAGIC)) note = " has already received this curse!";
                                	else
                                	{
                                        	m_ptr->abilities |= (CURSE_HALVE_MAGIC);
                                        	note = "'s magic powers have been greatly lowered!";
                                	}
				}
				else note = " resists.";
                        }
                        else note = " cannot be cursed!";
			break;
		}
                /* Slow Down and Halve Speed use the same variables, because they */
                /* are not cumulative! */
                case GF_HALVE_SPEED:
		{

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
                                if (m_ptr->abilities & (CURSE_SLOW_DOWN)) note = " has already received this curse!";
                                else
                                {
                                        m_ptr->mspeed -= m_ptr->mspeed / 4;
                                        m_ptr->abilities |= (CURSE_SLOW_DOWN);
                                        note = " is now much slower!";
                                }
                        }
                        else note = " cannot be cursed!";
			break;
		}
                case GF_HALVE_LEVEL:
		{

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
                                if (m_ptr->abilities & (CURSE_HALVE_LEVEL)) note = " has already received this curse!";
                                else
                                {
                                        m_ptr->level -= m_ptr->level / 2;
                                        m_ptr->abilities |= (CURSE_HALVE_LEVEL);
                                        note = "'s level has been halved!";
                                }
                        }
                        else note = " cannot be cursed!";
			break;
		}
                case GF_LOCK:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + p_ptr->lev);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_UNIQUE)) && m_ptr->boss < 1)
                        {
                                if (m_ptr->abilities & (CURSE_LOCK)) note = " has already received this curse!";
                                else
                                {
					if (randint(ppower) >= randint(mpower))
					{
                                        	m_ptr->abilities |= (CURSE_LOCK);
                                        	note = "'s spells have been locked!";
					}
					else note = " resists.";
                                }
                        }
                        else note = " cannot be cursed!";
			dam = 0;
			break;
		}
                case GF_DAMAGES_CURSE:
		{
			int ppower;
			int mpower;
			if (seen) obvious = TRUE;

			ppower = p_ptr->abilities[(CLASS_MAGE * 10) + 6] * 10;
			mpower = m_ptr->level + m_ptr->mind;

			if (randint(ppower) >= randint(mpower))
			{
                        	if (m_ptr->abilities & (CURSE_DAMAGES_CURSE)) note = " has already been cursed!";
                        	else
                        	{
                                	m_ptr->abilities |= (CURSE_DAMAGES_CURSE);
                                	note = "'s attacks are now cursed!";
                        	}
			}
			else note = " resisted the curse.";
			break;
		}
                case GF_INCOMPETENCE:
		{

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && !(r_ptr->flags1 & (RF1_UNIQUE)) && m_ptr->boss == 0)
                        {
                                /* Do not always work... */
                                if (randint(100) >= 50)
                                {
                                m_ptr->level = 1;
                                note = " became incompetent!";
                                }
                                else note = " resisted the curse!";
                        }
                        else note = " is unaffected!";
			break;
		}
                case GF_RETROGRADE:
		{
			
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + p_ptr->lev);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (m_ptr->boss >= 1)
                        {
				if (randint(ppower) >= randint(mpower))
				{
                                	m_ptr->boss = 0;
                                	m_ptr->abilities &= (BOSS_IMMUNE_WEAPONS);
                                	m_ptr->abilities &= (BOSS_IMMUNE_MAGIC);
                                	m_ptr->abilities &= (BOSS_DOUBLE_DAMAGES);
                                	m_ptr->abilities &= (BOSS_RETURNING);
                                	m_ptr->abilities &= (BOSS_CURSED_HITS);
                                	m_ptr->abilities &= (BOSS_DOUBLE_MAGIC);
                                	m_ptr->abilities &= (BOSS_HALVE_DAMAGES);
                                	m_ptr->abilities &= (BOSS_MAGIC_RETURNING);
                                	note = " has been retrograded!";
				}
				else note = " resists.";
                        }
                        else note = " is unaffected!";
			dam = 0;
			break;
		}
                case GF_RETROGRADE_DARKNESS:
		{
			int ppower, mpower;
			ppower = p_ptr->abilities[(CLASS_PALADIN * 10) + 7] * 10;
			mpower = m_ptr->level + m_ptr->mind;
			if (seen) obvious = TRUE;

			if (randint(ppower) >= randint(mpower))
			{
                        	/* Give the curse */
                        	if (m_ptr->boss >= 1 && ((r_ptr->flags3 & (RF3_UNDEAD)) || (r_ptr->flags3 & (RF3_DEMON))))
                        	{
                                        m_ptr->boss = 0;
                                        m_ptr->abilities &= (BOSS_IMMUNE_WEAPONS);
                                        m_ptr->abilities &= (BOSS_IMMUNE_MAGIC);
                                        m_ptr->abilities &= (BOSS_DOUBLE_DAMAGES);
                                        m_ptr->abilities &= (BOSS_RETURNING);
                                        m_ptr->abilities &= (BOSS_CURSED_HITS);
                                        m_ptr->abilities &= (BOSS_DOUBLE_MAGIC);
                                        m_ptr->abilities &= (BOSS_HALVE_DAMAGES);
                                        m_ptr->abilities &= (BOSS_MAGIC_RETURNING);
                                        note = " has been retrograded!";
                        	}
                        	else note = " is unaffected!";
			}
			else note = " resists.";
			dam = 0;
			break;
		}
		case GF_TAUNT:
		{
			int ppower = p_ptr->abilities[(CLASS_FIGHTER * 10) + 2] * 10;
			int mpower = (m_ptr->level + m_ptr->mind);
			int chabonus = (p_ptr->stat_ind[A_CHR] - 5) * 5;

			if (chabonus < 0) chabonus = 0;
			if (chabonus > ppower) chabonus = ppower;

			ppower += chabonus;

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_UNIQUE)))
                        {
				if (randint(ppower) >= randint(mpower))
				{
                                	if (m_ptr->abilities & (CURSE_HALVE_DAMAGES)) note = " has already been taunted!";
                                	else
                                	{
						m_ptr->hitrate -= m_ptr->hitrate / 3;
						m_ptr->defense -= m_ptr->hitrate / 3;
						m_ptr->mspeed += 3;
                                        	m_ptr->abilities |= (TAUNTED);
                                        	note = " becomes furious!";
                                	}
				}
				else note = " resists.";
                        }
                        else note = " cannot be taunted.";
			dam = 0;
			break;
		}

		/* Physical! */
                case GF_PHYSICAL:
		{
			if (seen) obvious = TRUE;

                        /* NOT always hit! */
                        if (monster_physical != TRUE && monster_ranged != TRUE && nevermiss != TRUE)
                        {
				int hit;
				call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
                                if (hit == 0)
                                {
                                        note = " blocked your attack!";
                                        dam = 0;
                                }
                        }
			if (dam > 0)
			{
				dam -= ((dam * r_ptr->physres) / 100);
			}
			r_ptr->r_resist[GF_PHYSICAL] = 1;
			break;
		}
                case GF_SMITE_EVIL:
		{
			if (seen) obvious = TRUE;

                        if (!(r_ptr->flags3 & (RF3_EVIL)))
                        {
                                note = " is unaffected.";
                                dam = 0;
                        }
                        else
                        {

                        /* NOT always hit! */
                        if (monster_physical != TRUE)
                        {
				int hit;
				call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
                                if (hit == 0)
                                {
                                        note = " blocked your attack!";
                                        dam = 0;
                                }
                                else
                                {
                                        if (m_ptr->level <= (p_ptr->abilities[(CLASS_PALADIN * 10) + 3] * 3) && !(r_ptr->flags1 & (RF1_QUESTOR)))
                                        {
                                                m_ptr->monfear = 10;
                                                note = " becomes scared!";
                                                update_and_handle();
                                        }
                                }                                                        
                        }

                        }
			break;
		}
                
                case GF_STEALTH_ATTACK:
		{
			if (seen) obvious = TRUE;
                        if (r_ptr->flags2 & (RF2_INVISIBLE))
                        {
                                note = " is unaffected.";
                                dam = 0;
                        }
                        else
                        {


                        /* NOT always hit! */
                        if (monster_physical != TRUE)
                        {
				int hit;
				call_lua("player_hit_monster", "(Md)", "d", m_ptr, 0, &hit);
                                if (hit == 0)
                                {
                                        note = " blocked your attack!";
                                        dam = 0;
                                }
                        }
                        }
			break;
		}

                case GF_DOMINATION_CURSE:
		{

			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
                        if ((r_ptr->flags1 & RF1_QUESTOR) || (r_ptr->flags1 & RF1_UNIQUE) || (m_ptr->boss != 0))
			{
				/* Resist */
				/* No obvious effect */
                                note = " cannot be dominated!";
				obvious = FALSE;
			}
                        else
			{
                                if (m_ptr->level <= (p_ptr->lev + (p_ptr->lev / 4)))
                                {
                                        if (m_ptr->level < p_ptr->lev)
                                        {
                                                note = " is charmed, and is now a trusted friend!";
                                                set_pet(m_ptr, TRUE);
                                                m_ptr->imprinted = TRUE;
                                                m_ptr->friend = 1;
                                        }
                                        else
                                        {
                                                note = " is now under your control!";
                                                set_pet(m_ptr, TRUE);
                                        }        
                                }
                                else note = " resists!";
                                
			}

			/* No "real" damage */
			dam = 0;
			break;
		}
                case GF_DOMINATE_MONSTER:
		{
			int ppower, mpower;
			ppower = p_ptr->abilities[(CLASS_MONSTER_MAGE * 10) + 5] * 10;
			mpower = m_ptr->level + m_ptr->mind;
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
                        if ((r_ptr->flags1 & RF1_QUESTOR) || (r_ptr->flags1 & RF1_UNIQUE) || (m_ptr->boss != 0))
			{
				/* Resist */
				/* No obvious effect */
                                note = " cannot be dominated!";
				obvious = FALSE;
			}
                        else
			{
                                        if (randint(ppower) >= randint(mpower))
                                        {
                                                note = " is now under your control!";
                                                set_pet(m_ptr, TRUE);
                                        }
                                        else
                                        {
                                                note = " resists!";
                                        }
                                
			}

			/* No "real" damage */
			dam = 0;
			break;
		}
                case GF_SHATTER_EVIL:
		{
			int ppower, mpower;
			ppower = p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10)] * 10;
			mpower = m_ptr->level + m_ptr->mind;
			if (seen) obvious = TRUE;
                        if (!(r_ptr->flags3 & RF3_EVIL))
			{
				dam = 0;
				note = " is immune!";
			}
                        else
                        {
				if ((randint(ppower) * 2) >= randint(mpower))
				{
                                	if ((r_ptr->flags3 & RF3_DEMON || r_ptr->flags3 & RF3_UNDEAD) && !(r_ptr->flags1 & RF1_UNIQUE) && m_ptr->boss < 1)
                                	{
                                        	do_fear = 10;
                                	}
				}
                        }
			break;
		}
                case GF_ANGELIC_VOICE:
		{
			int ppower, mpower;
			ppower = p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10) + 1] * 10;
			mpower = m_ptr->level + m_ptr->mind;
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
                        if ((r_ptr->flags1 & RF1_UNIQUE) || (m_ptr->boss >= 1) || !(r_ptr->flags3 & RF3_EVIL))
			{
				/* Resist */
				/* No obvious effect */
                                note = " is immune.";
				obvious = FALSE;
			}
                        else
			{
                                int chance = (randint(ppower) + p_ptr->stat_ind[A_CHR]);
                                if (chance >= randint(mpower))
                                {
                                        note = " becomes friendly!";
                                        set_pet(m_ptr, TRUE);
                                }
                                else
                                {
                                        note = " resists!";
                                }        
			}

			/* No "real" damage */
			dam = 0;
			break;
		}
                case GF_REPULSE_EVIL:
		{
			int ppower, mpower;
			ppower = p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10) + 2] * 10;
			mpower = m_ptr->level + m_ptr->mind;
			if (seen) obvious = TRUE;
                        if (!(r_ptr->flags3 & RF3_EVIL) || (r_ptr->flags1 & RF1_UNIQUE) || m_ptr->boss >= 1)
			{
				dam = 0;
				note = " cannot be affected.";
			}
                        else
                        {
				if (randint(ppower) >= randint(mpower))
				{
                                	do_fear = 5 + (p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10) + 2] / 2);
				}
                        }
			break;
		}
                case GF_SLAY_EVIL:
		{
			int ppower, mpower;
			ppower = p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10) + 6] * 10;
			mpower = m_ptr->level + m_ptr->mind;
			if (seen) obvious = TRUE;
                        if (!(r_ptr->flags3 & RF3_EVIL))
			{
				dam = 0;
				note = " is immune!";
			}
                        else
                        {
                                if (!(r_ptr->flags1 & RF1_UNIQUE) && m_ptr->boss < 1)
                                {
                                        if (randint(ppower) <= mpower)
                                        {
                                                note = " is disintegrated by your holy spell!";
                                                dam = (m_ptr->hp + 1);
                                        }
                                }
                        }
			break;
		}
                case GF_SEAL_LIGHT:
                {
                        int chance = dam;
                        int resist = m_ptr->level + m_ptr->mind;

                        chance += ((chance * (p_ptr->abilities[(CLASS_SOUL_GUARDIAN * 10) + 3] * 20)) / 100);
                        if ((r_ptr->flags1 & RF1_UNIQUE) || m_ptr->boss >= 1)
                        {
                                resist *= 3;
                        }

                        if ((r_ptr->flags1 & RF1_QUESTOR))
                        {
                                note = " is immune.";
                        }
                        else
                        {
                                if (randint(chance) >= randint(resist))
                                {
                                        note = " has been sealed!";
                                        m_ptr->seallight = 10 + (p_ptr->abilities[(CLASS_SOUL_GUARDIAN * 10) + 3] / 2);
                                }
                                else note = " resists.";
                        }

                        /* No real damages */
                        dam = 0;

			break;
                }        
                case GF_PARALYZE:
                {
                        int chance = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + p_ptr->lev + p_ptr->skill[23]);
                        int resist = m_ptr->level + m_ptr->mind;

                        if ((r_ptr->flags1 & RF1_UNIQUE) || m_ptr->boss >= 1)
                        {
                                resist *= 3;
                        }

                        if ((r_ptr->flags1 & RF1_QUESTOR) || (r_ptr->flags3 & RF3_NO_STUN))
                        {
                                note = " is immune.";
                        }
                        else
                        {
                                if (randint(chance) >= randint(resist))
                                {
                                        note = " has been paralyzed!";
                                        m_ptr->seallight = dam;
                                }
                                else note = " resists.";
                        }

                        /* No real damages */
                        dam = 0;

			break;
                }        
                

                case GF_SLEEP_POLLEN:
		{
			int chance = dam;
                        int resist = m_ptr->level + m_ptr->mind;

                        chance += (p_ptr->abilities[(CLASS_RANGER * 10) + 8] * 10);
			if (seen) obvious = TRUE;

                        if ((randint(chance) >= randint(resist)) && (m_ptr->boss == 0) && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & RF3_NO_SLEEP))
			{
				/* Go to sleep (much) later */
				note = " falls asleep!";
				do_sleep = 500;
			}
                        else note = " wasen't affected!";

			/* No "real" damage */
			dam = 0;
			break;
		}
                case GF_SLEEP_GAS:
		{
			if (seen) obvious = TRUE;

                        if (((randint(p_ptr->abilities[(CLASS_ROGUE * 10) + 6]) * 10) >= randint(m_ptr->level + m_ptr->mind)) && (m_ptr->boss < 2) && !(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags3 & RF3_NO_SLEEP))
			{
				/* Go to sleep (much) later */
				note = " falls asleep!";
				do_sleep = 500;
			}
                        else note = " wasen't affected!";

			/* No "real" damage */
			dam = 0;
			break;
		}


                case GF_WAR_BLESSING:
		{
			if (seen) obvious = TRUE;

                        /* Only boost pets...and only once! */
                        if (is_pet(m_ptr))
			{
                                if (m_ptr->abilities & (WAR_BLESSED))
                                {
                                        note = " has already been blessed!";
                                        return (FALSE);
                                }
                                else
                                {
                                        /* Boost them! */
                                        note = " is now blessed!";
                                        m_ptr->hitrate *= 2;
                                        m_ptr->defense *= 2;
                                        m_ptr->abilities |= (WAR_BLESSED);
                                }
			}
                        else note = " wasen't affected!";

			/* No "real" damage */
			dam = 0;
			break;
		}
                case GF_FRAILNESS:
		{

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
                                if (m_ptr->abilities & (CURSE_FRAILNESS))
                                {
                                        note = " has already received this curse!";
                                }
                                else
                                {
                                        m_ptr->abilities |= (CURSE_FRAILNESS);
                                        m_ptr->defense = m_ptr->defense / 2;
                                        note = "'s defense is lowered!";
                                }
                        }
                        else note = " cannot be cursed!";
			break;
		}
                case GF_INEPTITUDE:
		{

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)) && m_ptr->boss < 2)
                        {
                                if (m_ptr->abilities & (CURSE_INEPTITUDE))
                                {
                                        note = " has already received this curse!";
                                }
                                else
                                {
                                        m_ptr->abilities |= (CURSE_INEPTITUDE);
                                        m_ptr->hitrate = m_ptr->hitrate / 2;
                                        note = "'s hit rate is lowered!";
                                }
                        }
                        else note = " cannot be cursed!";
			break;
		}
                case GF_FEAR_CURSE:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_UNIQUE)) && m_ptr->boss < 1 && !(r_ptr->flags3 & (RF3_NONLIVING)) && !(r_ptr->flags3 & (RF3_NO_FEAR)))
                        {
				if (randint(ppower) >= randint(mpower))
				{
                                	if (m_ptr->monfear > 0)
                                	{
                                        	note = " is already scared!";
                                	}
                                	else
                                	{
                                        	m_ptr->monfear = dam;
                                        	note = " becomes scared!";
                                        	update_and_handle();
                                	}
				}
				else note = " resists.";
                        }
                        else note = " is unaffected.";
                        dam = 0;
			break;
		}
		case GF_WARCRY:
		{
			int ppower, mpower;
			if (seen) obvious = TRUE;

			ppower = p_ptr->abilities[(CLASS_WARRIOR * 10) + 6] * 10;
			mpower = m_ptr->level + m_ptr->mind;

			if (randint(ppower) >= randint(mpower))
			{

                        	/* Give the curse */
                        	if (!(r_ptr->flags1 & (RF1_UNIQUE)) && m_ptr->boss < 1 && !(r_ptr->flags3 & (RF3_NONLIVING)) && !(r_ptr->flags3 & (RF3_NO_FEAR)))
                        	{
                                	if (m_ptr->monfear > 0)
                                	{
                                        	note = " is already scared!";
                                	}
                                	else
                                	{
                                        	m_ptr->monfear = dam;
                                        	note = " becomes scared!";
                                        	update_and_handle();
                                	}
                        	}
                        	else note = " is unaffected.";
			}
			else note = " resists.";
                        dam = 0;
			break;
		}
                /* Reduce defense. No limits. */
                case GF_REDUCE_DEF:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Reduce defense */
			if (randint(ppower) >= randint(mpower))
			{
                        	m_ptr->defense -= dam;
                        	if (m_ptr->defense < 0) m_ptr->defense = 0;
                        	note = " has lost defense.";
			}
			else note = " resists.";
                        dam = 0;
			break;
		}
                /* Reduce hit rate. No limits. */
                case GF_REDUCE_HIT:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Reduce defense */
			if (randint(ppower) >= randint(mpower))
			{
                        	m_ptr->hitrate -= dam;
                        	if (m_ptr->hitrate < 0) m_ptr->hitrate = 0;
                        	note = " has lost hit rate.";
			}
			else note = " resists.";
                        dam = 0;
			break;
		}
                case GF_REDUCE_SPEED:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_QUESTOR)))
                        {
				if (randint(ppower) >= randint(mpower))
				{
                        		m_ptr->mspeed -= dam;
                                	if (m_ptr->mspeed <= 0) m_ptr->mspeed = 0;
                                	note = " is moving slower.";
				}
				else note = " resists.";
                        }
                        else note = " cannot be slowed!";
                        dam = 0;
			break;
		}

                case GF_MORALE_BOOST:
		{
			if (seen) obvious = TRUE;

                        /* Only boost pets...and only once! */
                        if (is_pet(m_ptr))
			{
                                if (m_ptr->abilities & (MORALE_BOOST))
                                {
                                        note = " has already been boosted!";
                                        return (FALSE);
                                }
                                else
                                {
                                        /* Boost them! */
                                        note = " is now determined to fight at best!";
                                        m_ptr->hitrate += (m_ptr->hitrate / 2);
                                        m_ptr->mspeed += (m_ptr->mspeed / 4);
                                        m_ptr->abilities |= (MORALE_BOOST);
                                }
			}
                        else note = " wasen't affected!";

			/* No "real" damage */
			dam = 0;
			break;
		}
                case GF_AURA_LIFE:
		{                        
			if (seen) obvious = TRUE;

                        /* Heal friendly monsters */
                        if (is_pet(m_ptr))
			{
                                m_ptr->hp += dam;
                                if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;
                                dam = 0;
			}
                        else if (!(r_ptr->flags3 & RF3_UNDEAD)) dam = 0;
			break;
		}



            case GF_EVOLVE:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);

			/* Dragons and demons are tough to evolve. */
			if ((r_ptr->flags3 & (RF3_DRAGON)) || (r_ptr->flags3 & (RF3_DEMON))) mpower = ((mpower + 50) * 3);

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_UNIQUE)))
                        {
				if (randint(ppower) >= randint(mpower))
				{
                                	do_cmd_evolve_monster(m_ptr);
				}
				else note = " resists.";
                        }
                        else note = " cannot be evolved!";
                        dam = 0;
			break;
		}
            case GF_UNEVOLVE:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[23] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);

			/* Dragons and demons are tough to un-evolve. */
			if ((r_ptr->flags3 & (RF3_DRAGON)) || (r_ptr->flags3 & (RF3_DEMON))) mpower = ((mpower + 50) * 3);

			if (seen) obvious = TRUE;

                        /* Give the curse */
                        if (!(r_ptr->flags1 & (RF1_UNIQUE)) && m_ptr->boss < 1)
                        {
                                if (!is_pet(m_ptr))
				{
					if (randint(ppower) >= randint(mpower))
					{
                                		do_cmd_unevolve_monster(m_ptr);
					}
					else note = " resists.";
				}
				else do_cmd_unevolve_monster(m_ptr);
                        }
                        else note = " cannot be un-evolved!";
                        dam = 0;
			break;
		}            


		/* Control undead */
		case GF_CONTROL_UNDEAD:
		{
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags1 & RF1_QUESTOR) ||
			  (!(r_ptr->flags3 & RF3_UNDEAD)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{

#if 0
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}
#endif

				/* Resist */
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else if (p_ptr->aggravate)
			{
				note = " hates you too much!";
			}
			else
			{
				note = " is in your thrall!";
				set_pet(m_ptr, TRUE);
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Tame animal */
		case GF_CONTROL_ANIMAL:
		{
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->flags1 & (RF1_QUESTOR)) ||
			  (!(r_ptr->flags3 & (RF3_ANIMAL))) ||
			    (r_ptr->flags3 & (RF3_NO_CONF)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				/* Resist */
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else if (p_ptr->aggravate)
			{
				note = " hates you too much!";
			}
			else
			{
				note = " is tamed!";
				set_pet(m_ptr, TRUE);
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Confusion (Use "dam" as "power") */
		case GF_OLD_CONF:
		{
			int ppower = (dam + p_ptr->lev);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

			/* Get confused later */
			do_conf = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
                            (r_ptr->flags3 & (RF3_NO_CONF)) || m_ptr->boss >= 1)
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				/* Resist */
				do_conf = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else
			{
				if (randint(ppower) < randint(mpower))
				{
					note = " resists.";
					do_conf = 0;
					obvious = FALSE;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		case GF_STUN:
		{
			int ppower = (dam + p_ptr->lev);
			int mpower = (m_ptr->level + m_ptr->mind);
			if (seen) obvious = TRUE;

			do_stun = damroll((p_ptr->lev / 10) + 3 , (dam)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags3 & (RF3_NO_STUN)) || 
			    m_ptr->boss >= 1)
			{
				/* Resist */
				do_stun = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else
			{
				if (randint(ppower) < randint(mpower))
				{
					note = " resists.";
					do_conf = 0;
					obvious = FALSE;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Confusion (Use "dam" as "power") */
                case GF_CONF_DAM:
		{
			if (seen) obvious = TRUE;

			/* Get confused later */
			do_conf = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->flags3 & (RF3_NO_CONF)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				/* Resist */
				do_conf = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			break;
		}

                case GF_STUN_DAM:
		{
			if (seen) obvious = TRUE;

			do_stun = damroll((p_ptr->lev / 10) + 3 , (dam)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Resist */
				do_stun = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			break;
		}

                /* Implosion is the same than Stun_dam but only affect the living */
                case GF_IMPLOSION:
		{
			if (seen) obvious = TRUE;

			do_stun = damroll((p_ptr->lev / 10) + 3 , (dam)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Resist */
				do_stun = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}

                        /* Non_living resists */
                        if (r_ptr->flags3 & (RF3_NONLIVING))
			{
				/* Resist */
				do_stun = 0;
                                dam = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			break;
		}

                /* Confusion & Stunning (Use "dam" as "power") */
                case GF_STUN_CONF:
		{
			if (seen) obvious = TRUE;

			/* Get confused later */
			do_conf = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->flags3 & (RF3_NO_CONF)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (seen) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				/* Resist */
				do_conf = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}

			do_stun = damroll((p_ptr->lev / 10) + 3 , (dam)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Resist */
				do_stun = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			break;
		}


		/* Lite, but only hurts susceptible creatures */
		case GF_LITE_WEAK:
		{
			/* Hurt by light */
			if (r_ptr->flags3 & (RF3_HURT_LITE))
			{
				/* Obvious effect */
				if (seen) obvious = TRUE;

				/* Memorize the effects */
				if (seen) r_ptr->r_flags3 |= (RF3_HURT_LITE);

				/* Special effect */
				note = " cringes from the light!";
				note_dies = " shrivels away in the light!";
			}

			/* Normally no damage */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}



		/* Lite -- opposite of Dark */
		case GF_LITE:
		{
			if (seen) obvious = TRUE;
			if (p_ptr->prace == RACE_CELESTIAL) dam = dam + (dam / 4);
			dam -= ((dam * r_ptr->lightres) / 100);
			r_ptr->r_resist[GF_LITE] = 1;
			break;
		}


		/* Dark -- opposite of Lite */
		case GF_DARK:
		{
			if (seen) obvious = TRUE;
                        /* Demons get a bonus to Darkness damages! */
                        if (p_ptr->prace == RACE_DEMON) dam = dam + (dam / 4);
			dam -= ((dam * r_ptr->darkres) / 100);
			r_ptr->r_resist[GF_DARK] = 1;
			break;
		}


		/* Stone to Mud */
		case GF_KILL_WALL:
		{
			/* Hurt by rock remover */
			if (r_ptr->flags3 & (RF3_HURT_ROCK))
			{
				/* Notice effect */
				if (seen) obvious = TRUE;

				/* Memorize the effects */
				if (seen) r_ptr->r_flags3 |= (RF3_HURT_ROCK);

				/* Cute little message */
				note = " loses some skin!";
				note_dies = " dissolves!";
			}

			/* Usually, ignore the effects */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}


		/* Teleport undead (Use "dam" as "power") */
		case GF_AWAY_UNDEAD:
		{

                        if (special_flag) break;/* No teleport on special levels */
			/* Only affect undead */
			if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				bool resists_tele = FALSE;

				if (r_ptr->flags3 & (RF3_RES_TELE))
				{
					if (r_ptr->flags1 & (RF1_UNIQUE))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " is unaffected!";
						resists_tele = TRUE;
					}
					else if (r_ptr->level > randint(100))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " resists!";
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (seen) obvious = TRUE;
					if (seen) r_ptr->r_flags3 |= (RF3_UNDEAD);
					do_dist = dam;
				}
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Teleport evil (Use "dam" as "power") */
		case GF_AWAY_EVIL:
		{
                        if (special_flag) break;/* No teleport on special levels */
			/* Only affect evil */
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				bool resists_tele = FALSE;

				if (r_ptr->flags3 & (RF3_RES_TELE))
				{
					if (r_ptr->flags1 & (RF1_UNIQUE))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " is unaffected!";
						resists_tele = TRUE;
					}
					else if (r_ptr->level > randint(100))
					{
						if (seen) r_ptr->r_flags3 |= RF3_RES_TELE;
						note = " resists!";
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (seen) obvious = TRUE;
					if (seen) r_ptr->r_flags3 |= (RF3_EVIL);
					do_dist = dam;
				}
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}

		/* Warp! */
		/* Teleport monster (Use "dam" as "power") */
		case GF_WARP:
		{
			int ppower = (p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS] + (p_ptr->skill[22] * 3) + p_ptr->skill[1]);
			int mpower = (m_ptr->level + m_ptr->mind);
			bool resists_tele = FALSE;

                        if (special_flag) break;/* No teleport on special levels */
                        /* Cannot send away Uniques, Elites and Bosses... */
                        if (r_ptr->flags1 & (RF1_UNIQUE))
                        {
                                resists_tele = TRUE;
                        }

			if (m_ptr->boss > 0) mpower *= 3;

			if (seen) obvious = TRUE;
			dam -= ((dam * r_ptr->warpres) / 100);

			if (!resists_tele)
			{
				/* Obvious */
				if (seen) obvious = TRUE;
				
                                if (randint(ppower) >= randint(mpower))
                                {
                                        do_dist = 20;
                                }
				
			}
			
			r_ptr->r_resist[GF_WARP] = 1;
			break;
		}


		/* Turn undead (Use "dam" as "power") */
		case GF_TURN_UNDEAD:
		{
			/* Only affect undead */
                        if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				if (!(r_ptr->flags1 & (RF1_UNIQUE)) && !(m_ptr->boss >= 1))
                                {
					int turnpower = p_ptr->abilities[(CLASS_PRIEST * 10) + 1] * 10;
					int resistpower = m_ptr->level + m_ptr->mind;
					int tpow, rpow;
					tpow = randint(turnpower);
					rpow = randint(resistpower);
					if (tpow > rpow)
					{
						if (tpow > (rpow * 2))
						{
							note = " has been turned!";
							set_pet(m_ptr, TRUE);
							dam = 0;
						}
						else dam = (m_ptr->hp + 1);
					}        
                                }
                        }

			/* Others ignore */
			else
			{
                                note = " is unaffected.";
                                dam = 0;
                        }

			break;
		}


		/* Turn evil (Use "dam" as "power") */
		case GF_TURN_EVIL:
		{
			/* Only affect evil */
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= (RF3_EVIL);

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
				{
					/* No obvious effect */
					note = " is unaffected!";
					obvious = FALSE;
					do_fear = 0;
				}
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Turn monster (Use "dam" as "power") */
		case GF_TURN_ALL:
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Apply some fear */
			do_fear = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
			    (r_ptr->flags3 & (RF3_NO_FEAR)) ||
			    (r_ptr->level > randint((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
				do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Dispel undead */
		case GF_DISP_UNDEAD:
		{
			/* Only affect undead */
			if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= (RF3_UNDEAD);

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders.";
				note_dies = " dissolves!";
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;

				/* No damage */
				dam = 0;
			}

			break;
		}

                /* Animal Empathy! */
                case GF_ANIMAL_EMPATHY:
		{
                        int successrate;

                        successrate = (p_ptr->abilities[(CLASS_RANGER * 10) + 3] * 2) + 23;
                        if (successrate > 100) successrate = 100;

                        /* Only affect animals */
                        if (r_ptr->flags3 & (RF3_ANIMAL))
			{
                                if (!(r_ptr->flags1 & (RF1_UNIQUE)) && !(m_ptr->boss >= 1))
                                {
                                        if (randint(100) <= successrate)
                                        {
                                                note = " has been tamed.";
                                                set_pet(m_ptr, TRUE);
                                        }
                                        else note = " resists.";
                                }
                                else if (randint(100) <= successrate)
                                {
                                        note = " starts fleeing from you!";
                                        m_ptr->monfear = 15;
                                }
                                else note = " resists.";

                        }

			/* Others ignore */
			else
			{
                                note = " is unaffected.";
                        }

                        /* No damage */
                        dam = 0;
			break;
		}


		/* Dispel evil */
		case GF_DISP_EVIL:
		{
			/* Only affect evil */
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= (RF3_EVIL);

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders.";
				note_dies = " dissolves!";
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;

				/* No damage */
				dam = 0;
			}

			break;
		}

		/* Dispel good */
		case GF_DISP_GOOD:
		{
			/* Only affect good */
			if (r_ptr->flags3 & (RF3_GOOD))
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= (RF3_GOOD);

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders.";
				note_dies = " dissolves!";
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;

				/* No damage */
				dam = 0;
			}

			break;
		}

		/* Dispel living */
		case GF_DISP_LIVING:
		{
			/* Only affect non-undead */
			if (!(r_ptr->flags3 & (RF3_UNDEAD)) &&
			    !(r_ptr->flags3 & (RF3_NONLIVING)))
			{
				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders.";
				note_dies = " dissolves!";
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;

				/* No damage */
				dam = 0;
			}

			break;
		}

		/* Dispel demons */
		case GF_DISP_DEMON:
		{
			/* Only affect demons */
			if (r_ptr->flags3 & (RF3_DEMON))
			{
				/* Learn about type */
				if (seen) r_ptr->r_flags3 |= (RF3_DEMON);

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders.";
				note_dies = " dissolves!";
			}

			/* Others ignore */
			else
			{
				/* Irrelevant */
				skipped = TRUE;

				/* No damage */
				dam = 0;
			}

			break;
		}

		/* Dispel monster */
		case GF_DISP_ALL:
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";

			break;
		}

                /* Dispel friendly monster */
                case GF_UNSUMMON:
		{
			/* Obvious */
                        if (is_pet(m_ptr))
                        {
				if ((r_ptr->r_flags7 |= (RF7_TOWNSFOLK)) || (r_ptr->r_flags7 |= (RF7_GUARD)))
				{
					msg_print("You can't use this on citizens or town guards!");
				}
				else
				{
                                	if (seen) obvious = TRUE;
                                	dam = m_ptr->hp + 1;

                                	/* Message */
                                	note = " shudders.";
                                	note_dies = " dissolves!";
				}

                        }
                        break;
		}


                /* Raise Death -- Heal monster */
                case GF_RAISE:
		{
			if (seen) obvious = TRUE;

			/* Wake up */
			m_ptr->csleep = 0;

			/* Heal */
			m_ptr->hp += dam;

			/* No overflow */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Redraw (later) if needed */
			if (health_who == c_ptr->m_idx) p_ptr->redraw |= (PR_HEALTH);

			/* Message */
			note = " looks healthier.";

			/* No "real" damage */
			dam = 0;
			break;
		}


		/* Default */
		default:
		{
			/* Irrelevant */
			skipped = TRUE;

			/* No damage */
			dam = 0;

			break;
		}
	}


	/* Absolutely no effect */
	if (skipped) return (FALSE);


	/* "Unique" monsters cannot be polymorphed */
	if (r_ptr->flags1 & (RF1_UNIQUE)) do_poly = FALSE;

	/*
	 * "Quest" monsters cannot be polymorphed
	 */
	if (r_ptr->flags1 & RF1_QUESTOR)
		do_poly = FALSE;
        /* Elites/Bosses can't be polymorphed... */
        if (m_ptr->boss != 0) do_poly = 0;

        /* Mastery Of Elements! */
        if ((p_ptr->abilities[(CLASS_ELEM_LORD * 10) + 9] >= 1) && typ == p_ptr->elemlord)
        {
                dam += ((dam * (p_ptr->abilities[(CLASS_ELEM_LORD * 10) + 9] * 5)) / 100);
        }

        /* The PHYSICAL type is a physical attack after all... */
        /* Same for Smite Evil */
        /*if (typ == GF_PHYSICAL || typ == GF_SMITE_EVIL || typ == GF_STEALTH_ATTACK) dam = monster_damage_reduction(dam, m_ptr, FALSE);*/

        /* The MISSILE type is a little more complicated... */
        if (typ == GF_MISSILE)
        {
                s32b magichalf = dam / 2;
                s32b physicalhalf = dam / 2;
                /*physicalhalf = monster_damage_reduction(physicalhalf, m_ptr, FALSE);*/
                if (m_ptr->abilities & (BOSS_IMMUNE_MAGIC)) magichalf = 0;
                if (m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)) physicalhalf = 0;
                dam = magichalf + physicalhalf;
        }
        /* Arg!! Magic Returning bosses?? :| */
        if ((typ != GF_PHYSICAL) && (typ != GF_TURN_UNDEAD) && (typ != GF_AURA_LIFE) && (typ != GF_SMITE_EVIL) && (typ != GF_STEALTH_ATTACK) && !(no_magic_return) && !(lord_piercing(5, 2, typ, m_ptr, 1)) && (m_ptr->abilities & (BOSS_MAGIC_RETURNING)))
        {
                msg_print("You take damages from your spell!");
		(void)project(m_idx, 0, py, px, (dam / 2), typ, (PROJECT_JUMP | PROJECT_GRID | PROJECT_KILL));
        }
	/* Returning counters! */
	if ((typ != GF_PHYSICAL) && (typ != GF_TURN_UNDEAD) && (typ != GF_AURA_LIFE) && (typ != GF_SMITE_EVIL) && (typ != GF_STEALTH_ATTACK) && !(no_magic_return) && ((r_ptr->countertype == 8 || r_ptr->countertype == 9) && randint(100) <= r_ptr->counterchance))
        {
		msg_print("You suffer from your own spell!");
                (void)project(m_idx, 0, py, px, dam, typ, (PROJECT_JUMP | PROJECT_GRID | PROJECT_KILL));
	}
	/* Counters that block magic! */
        if ((typ != GF_PHYSICAL) && (typ != GF_TURN_UNDEAD) && (typ != GF_AURA_LIFE) && (typ != GF_SMITE_EVIL) && (typ != GF_STEALTH_ATTACK) && !(no_magic_return) && ((r_ptr->countertype == 2 || r_ptr->countertype == 3 || r_ptr->countertype == 18 || r_ptr->countertype == 19) && randint(100) <= r_ptr->counterchance))
        {
		/* The test is based on the monster's mind */
		/* Mind vs (Intelligence + Wisdom) */
		int playerscore = p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS];
		if (randint(m_ptr->mind) >= randint(playerscore))
		{
                	note = " blocked your magic!";
                	dam = 0;
		}
        }
	if ((typ != GF_PHYSICAL) && (typ != GF_TURN_UNDEAD) && (typ != GF_AURA_LIFE) && (typ != GF_SMITE_EVIL) && (typ != GF_STEALTH_ATTACK) && !(no_magic_return) && ((r_ptr->countertype == 5 || r_ptr->countertype == 6 || r_ptr->countertype == 22 || r_ptr->countertype == 23) && randint(100) <= r_ptr->counterchance))
        {
		/* 100% block! */
                note = " blocked your magic!";
                dam = 0;
        }
	/* Counters that block AND return magic! */
        if ((typ != GF_PHYSICAL) && (typ != GF_TURN_UNDEAD) && (typ != GF_AURA_LIFE) && (typ != GF_SMITE_EVIL) && (typ != GF_STEALTH_ATTACK) && !(no_magic_return) && ((r_ptr->countertype == 11 || r_ptr->countertype == 12) && randint(100) <= r_ptr->counterchance))
        {
		/* The test is based on the monster's mind */
		/* Mind vs (Intelligence + Wisdom) */
		int playerscore = p_ptr->stat_ind[A_INT] + p_ptr->stat_ind[A_WIS];
		if (randint(m_ptr->mind) >= randint(playerscore))
		{
                	note = " blocked your magic!";
			msg_print("You suffer from your own spell!");
                	(void)project(m_idx, 0, py, px, dam, typ, (PROJECT_JUMP | PROJECT_GRID | PROJECT_KILL));
                	dam = 0;
		}
        }
	if ((typ != GF_PHYSICAL) && (typ != GF_TURN_UNDEAD) && (typ != GF_AURA_LIFE) && (typ != GF_SMITE_EVIL) && (typ != GF_STEALTH_ATTACK) && !(no_magic_return) && ((r_ptr->countertype == 14 || r_ptr->countertype == 15) && randint(100) <= r_ptr->counterchance))
        {
		/* 100% block! */
                note = " blocked your magic!";
		msg_print("You suffer from your own spell!");
                (void)project(m_idx, 0, py, px, dam, typ, (PROJECT_JUMP | PROJECT_GRID | PROJECT_KILL));
                dam = 0;
        }

        /* Some elites/bosses are immune to magic... */
        if (((m_ptr->abilities & (BOSS_HALVE_DAMAGES)) && !(lord_piercing(10, 3, typ, m_ptr, 1))) || m_ptr->r_idx == 982 || m_ptr->r_idx == 1030)
        {
                dam = dam / 2;
        }

        /* GF_PHYSICAL is actually physical damages... */
        if ((m_ptr->abilities & (BOSS_IMMUNE_MAGIC)) && typ != GF_PHYSICAL && typ != GF_MISSILE && typ != GF_SMITE_EVIL && typ != GF_STEALTH_ATTACK && !(lord_piercing(1, 1, typ, m_ptr, 1)) && dam > 0)
        {
                note = " is immune!";
                dam = 0;
        }

        /* Yeah, but then, physical immunes are immune to GF_PHYSICAL! */
        if ((m_ptr->abilities & (BOSS_IMMUNE_WEAPONS)) && (typ == GF_PHYSICAL || typ == GF_SMITE_EVIL || typ == GF_STEALTH_ATTACK) && typ != GF_MISSILE && dam > 0)
        {
                note = " is immune!";
                dam = 0;
        }

        /* Variaz has a damage limit ability! */
        /*if (m_ptr->r_idx == 1030 && dam > 1000000 && typ != GF_PHYSICAL && typ != GF_SMITE_EVIL && typ != GF_STEALTH_ATTACK) dam = 1000000;*/

	/*
	 * "Quest" monsters can only be "killed" by the player
	 */
        /*if (r_ptr->flags1 & RF1_QUESTOR)                            */
        /*{                                                           */
        /*        if ((who > 0) && (dam > m_ptr->hp)) dam = m_ptr->hp;*/
        /*}                                                           */

        /* Bosses are NEVER confused. */
        if (m_ptr->boss >= 1) do_conf = 0;

	/* Check for death */
	if (dam > m_ptr->hp)
	{
		/* Extract method of death */
		note = note_dies;
	}

	/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
	else if (do_poly && (randint(90) > r_ptr->level))
	{
		bool charm = FALSE;
		int new_r_idx;
		int old_r_idx = m_ptr->r_idx;

		/* Default -- assume no polymorph */
		note = " is unaffected!";

		charm = is_pet(m_ptr) ? TRUE : FALSE;

		/* Pick a "new" monster race */
		new_r_idx = poly_r_idx(old_r_idx);

		/* Handle polymorh */
		if (new_r_idx != old_r_idx)
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Monster polymorphs */
			note = " changes!";

			/* Turn off the damage */
			dam = 0;

			/* "Kill" the "old" monster */
			delete_monster_idx(c_ptr->m_idx);

			/* Create a new monster (no groups) */
			if (!place_monster_aux(y, x, new_r_idx, FALSE, FALSE, charm, 0))
			{
				/* Placing the new monster failed */
				place_monster_aux(y, x, old_r_idx, FALSE, FALSE, charm, 0);
				obvious = FALSE;
				note = " is unaffected!";
			}

			/* XXX XXX XXX Hack -- Assume success */

			/* Hack -- Get new monster */
			m_ptr = &m_list[c_ptr->m_idx];

			/* Hack -- Get new race */
			r_ptr = &r_info[m_ptr->r_idx];
		}
	}

	/* Handle "teleport" */
	else if (do_dist)
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Message */
		note = " disappears!";

		/* Teleport */
		teleport_away(c_ptr->m_idx, do_dist);

		/* Hack -- get new location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Hack -- get new grid */
		c_ptr = &cave[y][x];
	}

	/* Sound and Impact breathers never stun */
	else if (do_stun &&
	    !(r_ptr->flags4 & (RF4_BR_SOUN)) &&
	    !(r_ptr->flags4 & (RF4_BR_WALL)))
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Get confused */
		if (m_ptr->stunned)
		{
			note = " is more dazed.";
			tmp = m_ptr->stunned + (do_stun / 2);
		}
		else
		{
			note = " is dazed.";
			tmp = do_stun;
		}

		/* Apply stun */
		m_ptr->stunned = (tmp < 200) ? tmp : 200;
	}

	/* Confusion and Chaos breathers (and sleepers) never confuse */
	else if (do_conf &&
		 !(r_ptr->flags3 & (RF3_NO_CONF)) &&
		 !(r_ptr->flags4 & (RF4_BR_CONF)) &&
		 !(r_ptr->flags4 & (RF4_BR_CHAO)))
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Already partially confused */
		if (m_ptr->confused)
		{
			note = " looks more confused.";
			tmp = m_ptr->confused + (do_conf / 2);
		}

		/* Was not confused */
		else
		{
			note = " looks confused.";
			tmp = do_conf;
		}

		/* Apply confusion */
		m_ptr->confused = (tmp < 200) ? tmp : 200;
	}


	/* Fear */
	if (do_fear)
	{
		/* Increase fear */
		tmp = m_ptr->monfear + do_fear;

		/* Set fear */
		m_ptr->monfear = (tmp < 200) ? tmp : 200;
	}


	/* If another monster did the damage, hurt the monster by hand */
	if (who)
	{
                /* Yeah, I know, the name of this one is stupid! :) */
                int stupidvariable = 1;

		/* Redraw (later) if needed */
		if (health_who == c_ptr->m_idx) p_ptr->redraw |= (PR_HEALTH);

		/* Wake the monster up */
		m_ptr->csleep = 0;

		/* Hurt the monster */
		m_ptr->hp -= dam;

		/* Dead monster */
		if (m_ptr->hp < 0)
		{
			bool sad = FALSE;

			if (is_pet(m_ptr) && !(m_ptr->ml))
				sad = TRUE;

#ifdef PET_GAIN_EXP
                if(stupidvariable == 1)
                {
		/* Maximum player level */
		div = p_ptr->max_plv;

		/* Give some experience for the kill */
		new_exp = ((long)r_ptr->mexp * r_ptr->level) / div;

		/* Handle fractional experience */
		new_exp_frac = ((((long)r_ptr->mexp * r_ptr->level) % div)
		                * 0x10000L / div) + p_ptr->exp_frac;

		/* Keep track of experience */
		if (new_exp_frac >= 0x10000L)
		{
			new_exp++;
			p_ptr->exp_frac = new_exp_frac - 0x10000;
		}
		else
		{
			p_ptr->exp_frac = new_exp_frac;
		}

                /* 1.5.0, revised experience system... */
                new_exp = new_exp / 10;

                /* Friends are dead, no experience! */
                if (is_pet(m_ptr))
                {
                        new_exp = 0;
                }

		/* Gain experience */
                gain_exp_kill(new_exp, m_ptr);
                }
#endif

			/* Generate treasure, etc */
			monster_death(c_ptr->m_idx);

			/* Delete the monster */
			delete_monster_idx(c_ptr->m_idx);

			/* Give detailed messages if destroyed */
			if (note) msg_format("%^s%s", m_name, note);

			if (sad)
			{
				msg_print("You feel sad for a moment.");
			}
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (note && seen) msg_format("%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (dam > 0) message_pain(c_ptr->m_idx, dam);

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}

	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;

		/* Hurt the monster, check for fear and death */
		if (mon_take_hit(c_ptr->m_idx, dam, &fear, note_dies))
		{
			/* Dead monster */
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (note && seen) msg_format("%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (dam > 0) message_pain(c_ptr->m_idx, dam);

			/* Take note */
			if ((fear || do_fear) && (m_ptr->ml))
			{
				/* Sound */
				sound(SOUND_FLEE);

				/* Message */
				msg_format("%^s flees in terror!", m_name);
			}

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}


	/* XXX XXX XXX Verify this code */

	/* Update the monster */
	update_mon(c_ptr->m_idx, FALSE);

	/* Redraw the monster grid */
	lite_spot(y, x);


	/* Update monster recall window */
	if (monster_race_idx == m_ptr->r_idx)
	{
		/* Window stuff */
		p_ptr->window |= (PW_MONSTER);
	}


	/* Track it */
	project_m_n++;
	project_m_x = x;
	project_m_y = y;


	/* Return "Anything seen?" */
	return (obvious);
}


/* Is the spell unsafe for the player ? */
bool unsafe = FALSE;


/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to the player.
 *
 * This routine takes a "source monster" (by index), a "distance", a default
 * "damage", and a "damage type".  See "project_m()" above.
 *
 * If "rad" is non-zero, then the blast was centered elsewhere, and the damage
 * is reduced (see "project_m()" above).  This can happen if a monster breathes
 * at the player and hits a wall instead.
 *
 * NOTE (Zangband): 'Bolt' attacks can be reflected back, so we need to know
 * if this is actually a ball or a bolt spell
 *
 *
 * We return "TRUE" if any "obvious" effects were observed.  XXX XXX Actually,
 * we just assume that the effects were obvious, for historical reasons.
 */
static bool project_p(int who, int r, int y, int x, s32b dam, int typ, int a_rad)
{
	int k = 0;

	/* Hack -- assume obvious */
	bool obvious = TRUE;

	/* Player blind-ness */
	bool blind = (p_ptr->blind ? TRUE : FALSE);

	/* Player needs a "description" (he is blind) */
	bool fuzzy = FALSE;

	/* Source monster */
	monster_type *m_ptr;

	/* Monster race */
	monster_race *r_ptr;

	/* Monster name (for attacks) */
	char m_name[80];

	/* Monster name (for damage) */
	char killer[80];

	/* Hack -- messages */
	cptr act = NULL;

	/* Player is not here */
	if ((x != px) || (y != py)) return (FALSE);

	/* Player cannot hurt himself */
        if ((!who)&&(!unsafe)) return (FALSE);

	/* If the player is blind, be more descriptive */
	if (blind) fuzzy = TRUE;

	/* If the player is hit by a trap, be more descritive */
	if (who == -2) fuzzy = TRUE;

	/* Get the source monster */
	m_ptr = &m_list[who];

	/* Extract monster race data. */
	r_ptr = &r_info[m_ptr->r_idx];

	/* Get the monster name */
	monster_desc(m_name, m_ptr, 0);

	/* Get the monster's real name */
	monster_desc(killer, m_ptr, 0x88);

	/* Reflect ability */
	if (p_ptr->reflect && !(monster_physical))
	{
		u32b f1, f2, f3, f4;
        	int i;
		int ppower = 0;
		int mpower = 0;
        	object_type *o_ptr;

        	i = 24;
        	while (i <= 52)
        	{
                	/* Get the item */
                	o_ptr = &inventory[i];

                	/* Examine the item */
                	object_flags(o_ptr, &f1, &f2, &f3, &f4);

                	/* Check for the REFLECT flag */
                	if (o_ptr->k_idx && (f2 & (TR2_REFLECT)))
                	{
                        	ppower += (10 * (o_ptr->pval + 1));
                	}

                	i++;
        	}

		if (ppower < 0) ppower = 0;

		mpower = (m_ptr->level + m_ptr->mind) * (a_rad + 1);
		if (mpower < 0) mpower = 0;

		if (randint(ppower) >= randint(mpower))
		{

                	int t_y, t_x;
			int max_attempts = 10;

			if (blind) msg_print("Something bounces!");
			else msg_print("The attack bounces!");

			/* Choose 'new' target */
			do
			{
				t_y = m_list[who].fy - 1 + randint(3);
				t_x = m_list[who].fx - 1 + randint(3);
				max_attempts--;
			}
			while (max_attempts && in_bounds2(t_y, t_x) &&
		     		!(player_has_los_bold(t_y, t_x)));

			if (max_attempts < 1)
			{
				t_y = m_list[who].fy;
				t_x = m_list[who].fx;
			}

			project(0, 0, t_y, t_x, dam, typ, (PROJECT_STOP|PROJECT_KILL));

			disturb(1, 0);
			return TRUE;
		}
	}

        /* Elites/Bosses may cause double damages with spells... */
        if (m_ptr->abilities & (BOSS_DOUBLE_MAGIC)) dam *= 2;

	/* Can you block magic attacks? */
        if (p_ptr->abilities[(CLASS_DEFENDER * 10) + 4] >= 1 && shield_has())
        {
                int blockchance;
                object_type *o_ptr = &inventory[INVEN_ARM];

                blockchance = ((o_ptr->sval * 10) / 2);
                blockchance += (o_ptr->pval * 2);
                if (p_ptr->abilities[(CLASS_DEFENDER * 10) + 3] >= 1) blockchance += (p_ptr->abilities[(CLASS_DEFENDER * 10) + 3] * 2);
                if (blockchance > 75) blockchance = 75;
                if (randint(100) < blockchance)
                {
                        msg_print("You block the magic attack!");
                        dam -= (p_ptr->abilities[(CLASS_DEFENDER * 10) + 4] * 500);
                        if (dam < 0) dam = 0;
                }
        }

        /* Justice Warrior's protection from evil! :) */
        if (p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10) + 8] >= 1 && r_ptr->flags3 & (RF3_EVIL))
        {
                int reduction = p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10) + 8];
                if (reduction > 75) reduction = 75;
                dam -= ((dam * reduction) / 100);
                dam -= p_ptr->abilities[(CLASS_JUSTICE_WARRIOR * 10) + 8] * 5;
        }

        /* Magic resistance! :) */
        if (p_ptr->mres_dur > 0)
        {
                s32b damfract;
                damfract = (dam * p_ptr->mres) / 100;
                dam -= damfract;
        }

        /* Paldin's Resist Impure! :) */
        if ((typ == GF_POIS || typ == GF_RADIO || typ == GF_DARK || typ == GF_NETHER || typ == GF_CHAOS) && p_ptr->abilities[(CLASS_PALADIN * 10) + 5] >= 1)
        {
                s32b damfract;
                damfract = (dam * ((p_ptr->abilities[(CLASS_PALADIN * 10) + 5] * 2) + 25)) / 100;
                dam -= damfract;
                if (dam < 0)
                {
                        dam *= -1;
                        p_ptr->chp += dam;
                        if (p_ptr->chp > p_ptr->mhp) p_ptr->chp = p_ptr->mhp;
			dam = 0;
                        update_and_handle();
                }
        }

        /* Absorb the mana! :) */
        if (p_ptr->abilities[(CLASS_MAGE * 10) + 3] >= 1)
        {
                s32b damfract;
                damfract = (dam * (p_ptr->abilities[(CLASS_MAGE * 10) + 3] * 10)) / 100;
                p_ptr->csp += damfract;
                if (p_ptr->csp > p_ptr->msp) p_ptr->csp = p_ptr->msp;
                update_and_handle();
        }

        /* Rogue's Evasion ability! */
        if (p_ptr->abilities[(CLASS_ROGUE * 10) + 4] >= 1 && dam > 0 && !(monster_physical) && !(monster_ranged))
        {
                int evadechance;
                evadechance = 5 + p_ptr->abilities[(CLASS_ROGUE * 10) + 4];
                if (evadechance > 75) evadechance = 75;

                if (randint(100) <= evadechance)
                {
                        msg_print("You evade the attack!");
                        dam = 0;
                }
        }

        /* Enough agility can provide a chance to avoid damages... */
        if (p_ptr->skill[5] >= 70 && dam > 0 && !(monster_physical) && !(monster_ranged))
        {
                if (randint(100) <= 33)
                {
                        msg_print("You evade the attack!");
                        dam = 0;
                }
        }

	/* And of course, the Magic Defense skill! */
	if (p_ptr->skill[27] >= 1 && dam > 0 && !(monster_physical) && !(monster_ranged))
	{
		int ppower = (p_ptr->skill[27] * 10);
		int mpower = (m_ptr->level + m_ptr->mind);

		if (randint(ppower) >= randint(mpower))
		{
			msg_print("You evade the attack!");
			dam = 0;
		}
		else
		{
			int percentred;

			percentred = (p_ptr->skill[27] / 2);
			if (percentred > 75) percentred = 75;
			dam -= ((dam * percentred) / 100);
			if (dam < 0) dam = 0;
		}
	}


	/* Analyze the damage */
	switch (typ)
	{
		/* Standard damage -- hurts inventory too */
		case GF_ACID:
		{
			if (fuzzy) msg_print("You are hit by acid!");
			dam -= ((dam * (p_ptr->acidres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) take_hit(dam, killer);
			break;
		}

		/* Standard damage -- hurts inventory too */
		case GF_FIRE:
		{
			if (fuzzy) msg_print("You are hit by fire!");
			dam -= ((dam * (p_ptr->fireres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) take_hit(dam, killer);
			break;
		}

		/* Standard damage -- hurts inventory too */
		case GF_COLD:
		{
			if (fuzzy) msg_print("You are hit by cold!");
			dam -= ((dam * (p_ptr->coldres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) take_hit(dam, killer);
			break;
		}

		/* Standard damage -- hurts inventory too */
		case GF_ELEC:
		{
			if (fuzzy) msg_print("You are hit by electricity!");
			dam -= ((dam * (p_ptr->elecres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) take_hit(dam, killer);
			break;
		}

		/* Standard damage -- also poisons player */
		case GF_POIS:
		{
			if (fuzzy) msg_print("You are hit by poison!");
			dam -= ((dam * (p_ptr->poisres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) 
			{
				take_hit(dam, killer);
				if (randint(100) > p_ptr->poisres && (randint(p_ptr->stat_ind[A_CON]) < randint(100))) set_poisoned(p_ptr->poisoned + rand_int(dam) + 10);
			}

			break;
		}

		/* Standard damage -- also poisons / mutates player */
		case GF_RADIO:
		{
			if (fuzzy) msg_print("You are hit by radiation!");
			dam -= ((dam * (p_ptr->radiores)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) 
			{
				take_hit(dam, killer);
				if (randint(100) > p_ptr->radiores && (randint(p_ptr->stat_ind[A_CON]) < randint(100)))
				{
					msg_print("Your body changes from radiations!");
					p_ptr->stat_mut[A_STR] += randint(3) - randint(4);
					p_ptr->stat_mut[A_INT] += randint(3) - randint(4);
					p_ptr->stat_mut[A_WIS] += randint(3) - randint(4);
					p_ptr->stat_mut[A_DEX] += randint(3) - randint(4);
					p_ptr->stat_mut[A_CON] += randint(3) - randint(4);
					p_ptr->stat_mut[A_CHR] += randint(3) - randint(4);
					update_and_handle();	
				}
			}
			break;
		}

		/* Magic Missile */
		/* Being both magic/physical, it is resisted by both! */
		case GF_MISSILE:
		{
			s32b misphys = dam / 2;
			s32b mismagic = dam / 2;
			misphys -= ((misphys * (p_ptr->pres)) / 100);
			misphys -= ((misphys * (p_ptr->physres)) / 100);
			mismagic -= ((mismagic * (p_ptr->manares)) / 100);
			dam = misphys + mismagic;
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && ((p_ptr->elemlord == GF_PHYSICAL) || (p_ptr->elemlord == GF_MANA)))
        		{
                		dam = dam / 2;
        		}
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam < 0) dam = 0; 
			if (fuzzy) msg_print("You are hit by a magic missile!");
			take_hit(dam, killer);
			break;
		}

		/* FrostFire */
		case GF_FROSTFIRE:
		{
			s32b firedam = dam / 2;
			s32b colddam = dam / 2;
			firedam -= ((firedam * (p_ptr->fireres)) / 100);
			colddam -= ((colddam * (p_ptr->coldres)) / 100);
			dam = firedam + colddam;
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && ((p_ptr->elemlord == GF_FIRE) || (p_ptr->elemlord == GF_COLD)))
        		{
                		dam = dam / 2;
        		}
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam < 0) dam = 0; 
			if (fuzzy) msg_print("You are hit by a blue, cold flame!");
			take_hit(dam, killer);
			break;
		}

		/* Grey */
		case GF_GREY:
		{
			s32b litedam = dam / 2;
			s32b darkdam = dam / 2;
			litedam -= ((litedam * (p_ptr->lightres)) / 100);
			darkdam -= ((darkdam * (p_ptr->darkres)) / 100);
			dam = litedam + darkdam;
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && ((p_ptr->elemlord == GF_LITE) || (p_ptr->elemlord == GF_DARK)))
        		{
                		dam = dam / 2;
        		}
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam < 0) dam = 0;
			if (fuzzy) msg_print("You are hit by a grey energy!");
			take_hit(dam, killer);
			break;
		}

		/* Toxic */
		case GF_TOXIC:
		{
			s32b aciddam = dam / 3;
			s32b poisdam = dam / 3;
			s32b radiodam = dam / 3;
			aciddam -= ((aciddam * (p_ptr->acidres)) / 100);
			poisdam -= ((poisdam * (p_ptr->poisres)) / 100);
			radiodam -= ((poisdam * (p_ptr->radiores)) / 100);
			dam = aciddam + poisdam + radiodam;
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && ((p_ptr->elemlord == GF_ACID) || (p_ptr->elemlord == GF_POIS)  || (p_ptr->elemlord == GF_RADIO)))
        		{
                		dam = dam / 3;
        		}
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam < 0) dam = 0;
			if (dam > 0 && poisdam > 0)
			{
				if (randint(100) > p_ptr->poisres && (randint(p_ptr->stat_ind[A_CON]) < randint(100))) set_poisoned(p_ptr->poisoned + rand_int(dam) + 10);
			}
			if (dam > 0 && radiodam > 0) 
			{
				if (randint(100) > p_ptr->radiores && (randint(p_ptr->stat_ind[A_CON]) < randint(100)))
				{
					msg_print("Your body changes from radiations!");
					p_ptr->stat_mut[A_STR] += randint(3) - randint(4);
					p_ptr->stat_mut[A_INT] += randint(3) - randint(4);
					p_ptr->stat_mut[A_WIS] += randint(3) - randint(4);
					p_ptr->stat_mut[A_DEX] += randint(3) - randint(4);
					p_ptr->stat_mut[A_CON] += randint(3) - randint(4);
					p_ptr->stat_mut[A_CHR] += randint(3) - randint(4);
					update_and_handle();	
				}
			} 
			if (fuzzy) msg_print("You are hit by highly toxic matter!");
			take_hit(dam, killer);
			break;
		}

		/* Arrow -- XXX no dodging */
		case GF_ARROW:
		{
			dam -= ((dam * (p_ptr->pres)) / 100);
			dam -= ((dam * (p_ptr->physres)) / 100);
			if (dam < 0) dam = 0;
			if (fuzzy) msg_print("You are hit by something sharp!");
			take_hit(dam, killer);
			break;
		}

		/* Water -- stun/confuse */
		case GF_WATER:
		{
			if (fuzzy) msg_print("You are hit by water!");
			dam -= ((dam * (p_ptr->waterres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}

		/* Chaos -- many effects */
		case GF_CHAOS:
		{
			if (fuzzy) msg_print("You are hit by chaos!");
			dam -= ((dam * (p_ptr->chaosres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}

		/* Shards -- mostly cutting */
		case GF_EARTH:
		{
			if (fuzzy) msg_print("You are hit by earth!");
			dam -= ((dam * (p_ptr->earthres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}

		/* Sound -- mostly stunning */
		case GF_SOUND:
		{
			if (fuzzy) msg_print("You are hit by a loud noise!");
			dam -= ((dam * (p_ptr->soundres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}

		/* Pure confusion */
		case GF_CONFUSION:
		{
			if (fuzzy) msg_print("You are hit by pure confusion!");
			if (p_ptr->resist_conf) dam = dam / 3;
			dam -= ((dam * (p_ptr->chaosres)) / 100);
			dam -= ((dam * (p_ptr->manares)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) 
			{
				if (!p_ptr->resist_conf)
				{
					if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS] + p_ptr->lev)) (void)set_confused(p_ptr->confused + randint(20) + 10);
				}
				take_hit(dam, killer);
			}
			break;
		}

		/* Lite -- blinding */
		case GF_LITE:
		{
			if (fuzzy) msg_print("You are hit by light!");
			dam -= ((dam * (p_ptr->lightres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			/*if (!blind && !p_ptr->resist_blind)
			{
				(void)set_blind(p_ptr->blind + randint(5) + 2);
			}*/
			break;
		}

		/* Dark -- blinding */
		case GF_DARK:
		{
			if (fuzzy) msg_print("You are hit by darkness!");
			dam -= ((dam * (p_ptr->darkres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}

		case GF_OLD_HEAL:
		{
			if (fuzzy) msg_print("You are hit by something invigorating!");
			(void)hp_player(dam);
			dam = 0;
			break;
		}

		case GF_OLD_SPEED:
		{
			if (fuzzy) msg_print("You are hit by something!");
			(void)set_fast(p_ptr->fast + randint(5));
			dam = 0;
			break;
		}

		case GF_OLD_SLOW:
		{
			if (fuzzy) msg_print("You are hit by something slow!");
			if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS])) (void)set_slow(p_ptr->slow + rand_int(4) + 4);
			break;
		}

		case GF_OLD_SLEEP:
		{
			if (p_ptr->free_act)  break;
			if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
			{
				if (fuzzy) msg_print("You fall asleep!");
				set_paralyzed(p_ptr->paralyzed + dam);
			}
			dam = 0;
			break;
		}

		/* Warp damage */
		case GF_WARP:
		{
			if (fuzzy) msg_print("You are hit by space-time disruption!");
			dam -= ((dam * (p_ptr->warpres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			if (dam > 0) 
			{
				take_hit(dam, killer);
				if (randint(100) <= p_ptr->warpres) teleport_player(3);
			}
			break;
		}

		/* Wind damage */
		case GF_WIND:
		{
			if (fuzzy) msg_print("You are hit by strong winds!");
			dam -= ((dam * (p_ptr->windres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}
       
		/* Pure damage */
		case GF_MANA:
		{
			if (fuzzy) msg_print("You are hit by an aura of magic!");
			dam -= ((dam * (p_ptr->manares)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}

		/* Physical damage */
		case GF_PHYSICAL:
		{
			if (fuzzy) msg_print("You are hit by a physical attack!");
			if (p_ptr->pres_dur > 0) dam -= ((dam * (p_ptr->pres)) / 100);
			dam -= ((dam * (p_ptr->physres)) / 100);
			/* Elemental Lord's Shield Of Element! */
        		if (p_ptr->elem_shield > 0 && typ == p_ptr->elemlord)
        		{
                		dam = 0;
        		}
			take_hit(dam, killer);
			break;
		}

		/* In case it is returned... */
		case GF_LIFE_BLAST:
		{
			if (fuzzy) msg_print("Your life is blasted!");
			take_hit(dam, killer);
			break;
		}

		/* Fear! */
                case GF_FEAR:
		{
                        if (fuzzy) msg_print("You are hit by fear!");
			if (!p_ptr->resist_fear)
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		set_afraid(p_ptr->afraid + dam);
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}

		/* Confusion! */
                case GF_OLD_CONF:
		{
                        if (fuzzy) msg_print("You are hit by confusion!");
			if (!p_ptr->resist_conf)
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		set_confused(p_ptr->confused + dam);
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
            
                /* Statis -- paralyse */
                case GF_STASIS:
		{
                        if (fuzzy) msg_print("You are hit by something paralysing!");
                        if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS])) set_paralyzed(p_ptr->paralyzed + dam);
			break;
		}

		/* Stats reduction */
                case GF_LOSE_STR:
		{
                        if (fuzzy) msg_print("You are hit by strength reduction!");
			if (!(p_ptr->sustain_str))
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		msg_print("Your strength has been reduced!");
					dec_stat(A_STR, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
		case GF_LOSE_INT:
		{
                        if (fuzzy) msg_print("You are hit by intelligence reduction!");
			if (!(p_ptr->sustain_int))
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		msg_print("Your intelligence has been reduced!");
					dec_stat(A_INT, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
		case GF_LOSE_WIS:
		{
                        if (fuzzy) msg_print("You are hit by wisdom reduction!");
			if (!(p_ptr->sustain_wis))
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		msg_print("Your wisdom has been reduced!");
					dec_stat(A_WIS, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
		case GF_LOSE_DEX:
		{
                        if (fuzzy) msg_print("You are hit by dexterity reduction!");
			if (!(p_ptr->sustain_dex))
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		msg_print("Your dexterity has been reduced!");
					dec_stat(A_DEX, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
		case GF_LOSE_CON:
		{
                        if (fuzzy) msg_print("You are hit by constitution reduction!");
			if (!(p_ptr->sustain_con))
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		msg_print("Your constitution has been reduced!");
					dec_stat(A_CON, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
		case GF_LOSE_CHR:
		{
                        if (fuzzy) msg_print("You are hit by charisma reduction!");
			if (!(p_ptr->sustain_chr))
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
                        		msg_print("Your charisma has been reduced!");
					dec_stat(A_CHR, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
		case GF_LOSE_ALL:
		{
                        if (fuzzy) msg_print("You are hit by stats reduction!");
			if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
			{
				if (!(p_ptr->sustain_str))
				{
                        		msg_print("Your strength has been reduced!");
					dec_stat(A_STR, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				if (!(p_ptr->sustain_int))
				{
                        		msg_print("Your intelligence has been reduced!");
					dec_stat(A_INT, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				if (!(p_ptr->sustain_wis))
				{
                        		msg_print("Your wisdom has been reduced!");
					dec_stat(A_WIS, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				if (!(p_ptr->sustain_dex))
				{
                        		msg_print("Your dexterity has been reduced!");
					dec_stat(A_DEX, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				if (!(p_ptr->sustain_con))
				{
                        		msg_print("Your constitution has been reduced!");
					dec_stat(A_CON, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
				if (!(p_ptr->sustain_chr))
				{
                        		msg_print("Your charisma has been reduced!");
					dec_stat(A_CHR, dam, STAT_DEC_NORMAL);
					update_and_handle();
				}
			}
			else msg_print("You resist the effects!");
			break;
		}
		case GF_LOSE_EXP:
		{
                        if (fuzzy) msg_print("You are hit by experience reduction!");
			if (!(p_ptr->hold_life))
			{
				if (randint(dam) >= randint(p_ptr->stat_ind[A_WIS]))
				{
					msg_print("Your experience has been reduced!");
					lose_exp(dam);
					update_and_handle();
				}
				else msg_print("You resist the effects!");
			}
			else msg_print("You are unaffected!");
			break;
		}
            
                /* Raise Death -- restore life */
                case GF_RAISE:
		{
                        if (fuzzy) msg_print("You are hit by pure anti-death energy!");
                        restore_level();
			break;
		}

                /* Make Glyph -- Shield */
                case GF_MAKE_GLYPH:
		{
                        if (fuzzy) msg_print("You are hit by pure protection!");
                        set_shield(p_ptr->shield + dam, 50);
			break;
		}

		/* Default */
		default:
		{
			/* No damage */
			dam = 0;

			break;
		}
	}


	/* Disturb */
	disturb(1, 0);


	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * Generic "beam"/"bolt"/"ball" projection routine.
 *
 * Input:
 *   who: Index of "source" monster (negative for "player")
 *        jk -- -2 for traps, only used with project_jump
 *   rad: Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 *   y,x: Target location (or location to travel "towards")
 *   dam: Base damage roll to apply to affected monsters (or player)
 *   typ: Type of damage to apply to monsters (and objects)
 *   flg: Extra bit flags (see PROJECT_xxxx in "defines.h")
 *
 * Return:
 *   TRUE if any "effects" of the projection were observed, else FALSE
 *
 * Allows a monster (or player) to project a beam/bolt/ball of a given kind
 * towards a given location (optionally passing over the heads of interposing
 * monsters), and have it do a given amount of damage to the monsters (and
 * optionally objects) within the given radius of the final location.
 *
 * A "bolt" travels from source to target and affects only the target grid.
 * A "beam" travels from source to target, affecting all grids passed through.
 * A "ball" travels from source to the target, exploding at the target, and
 *   affecting everything within the given radius of the target location.
 *
 * Traditionally, a "bolt" does not affect anything on the ground, and does
 * not pass over the heads of interposing monsters, much like a traditional
 * missile, and will "stop" abruptly at the "target" even if no monster is
 * positioned there, while a "ball", on the other hand, passes over the heads
 * of monsters between the source and target, and affects everything except
 * the source monster which lies within the final radius, while a "beam"
 * affects every monster between the source and target, except for the casting
 * monster (or player), and rarely affects things on the ground.
 *
 * Two special flags allow us to use this function in special ways, the
 * "PROJECT_HIDE" flag allows us to perform "invisible" projections, while
 * the "PROJECT_JUMP" flag allows us to affect a specific grid, without
 * actually projecting from the source monster (or player).
 *
 * The player will only get "experience" for monsters killed by himself
 * Unique monsters can only be destroyed by attacks from the player
 *
 * Only 256 grids can be affected per projection, limiting the effective
 * "radius" of standard ball attacks to nine units (diameter nineteen).
 *
 * One can project in a given "direction" by combining PROJECT_THRU with small
 * offsets to the initial location (see "line_spell()"), or by calculating
 * "virtual targets" far away from the player.
 *
 * One can also use PROJECT_THRU to send a beam/bolt along an angled path,
 * continuing until it actually hits somethings (useful for "stone to mud").
 *
 * Bolts and Beams explode INSIDE walls, so that they can destroy doors.
 *
 * Balls must explode BEFORE hitting walls, or they would affect monsters
 * on both sides of a wall.  Some bug reports indicate that this is still
 * happening in 2.7.8 for Windows, though it appears to be impossible.
 *
 * We "pre-calculate" the blast area only in part for efficiency.
 * More importantly, this lets us do "explosions" from the "inside" out.
 * This results in a more logical distribution of "blast" treasure.
 * It also produces a better (in my opinion) animation of the explosion.
 * It could be (but is not) used to have the treasure dropped by monsters
 * in the middle of the explosion fall "outwards", and then be damaged by
 * the blast as it spreads outwards towards the treasure drop location.
 *
 * Walls and doors are included in the blast area, so that they can be
 * "burned" or "melted" in later versions.
 *
 * This algorithm is intended to maximize simplicity, not necessarily
 * efficiency, since this function is not a bottleneck in the code.
 *
 * We apply the blast effect from ground zero outwards, in several passes,
 * first affecting features, then objects, then monsters, then the player.
 * This allows walls to be removed before checking the object or monster
 * in the wall, and protects objects which are dropped by monsters killed
 * in the blast, and allows the player to see all affects before he is
 * killed or teleported away.  The semantics of this method are open to
 * various interpretations, but they seem to work well in practice.
 *
 * We process the blast area from ground-zero outwards to allow for better
 * distribution of treasure dropped by monsters, and because it provides a
 * pleasing visual effect at low cost.
 *
 * Note that the damage done by "ball" explosions decreases with distance.
 * This decrease is rapid, grids at radius "dist" take "1/dist" damage.
 *
 * Notice the "napalm" effect of "beam" weapons.  First they "project" to
 * the target, and then the damage "flows" along this beam of destruction.
 * The damage at every grid is the same as at the "center" of a "ball"
 * explosion, since the "beam" grids are treated as if they ARE at the
 * center of a "ball" explosion.
 *
 * Currently, specifying "beam" plus "ball" means that locations which are
 * covered by the initial "beam", and also covered by the final "ball", except
 * for the final grid (the epicenter of the ball), will be "hit twice", once
 * by the initial beam, and once by the exploding ball.  For the grid right
 * next to the epicenter, this results in 150% damage being done.  The center
 * does not have this problem, for the same reason the final grid in a "beam"
 * plus "bolt" does not -- it is explicitly removed.  Simply removing "beam"
 * grids which are covered by the "ball" will NOT work, as then they will
 * receive LESS damage than they should.  Do not combine "beam" with "ball".
 *
 * The array "gy[],gx[]" with current size "grids" is used to hold the
 * collected locations of all grids in the "blast area" plus "beam path".
 *
 * Note the rather complex usage of the "gm[]" array.  First, gm[0] is always
 * zero.  Second, for N>1, gm[N] is always the index (in gy[],gx[]) of the
 * first blast grid (see above) with radius "N" from the blast center.  Note
 * that only the first gm[1] grids in the blast area thus take full damage.
 * Also, note that gm[rad+1] is always equal to "grids", which is the total
 * number of blast grids.
 *
 * Note that once the projection is complete, (y2,x2) holds the final location
 * of bolts/beams, and the "epicenter" of balls.
 *
 * Note also that "rad" specifies the "inclusive" radius of projection blast,
 * so that a "rad" of "one" actually covers 5 or 9 grids, depending on the
 * implementation of the "distance" function.  Also, a bolt can be properly
 * viewed as a "ball" with a "rad" of "zero".
 *
 * Note that if no "target" is reached before the beam/bolt/ball travels the
 * maximum distance allowed (MAX_RANGE), no "blast" will be induced.  This
 * may be relevant even for bolts, since they have a "1x1" mini-blast.
 *
 * Note that for consistency, we "pretend" that the bolt actually takes "time"
 * to move from point A to point B, even if the player cannot see part of the
 * projection path.  Note that in general, the player will *always* see part
 * of the path, since it either starts at the player or ends on the player.
 *
 * Hack -- we assume that every "projection" is "self-illuminating".
 *
 * Hack -- when only a single monster is affected, we automatically track
 * (and recall) that monster, unless "PROJECT_JUMP" is used.
 *
 * Note that all projections now "explode" at their final destination, even
 * if they were being projected at a more distant destination.  This means
 * that "ball" spells will *always* explode.
 *
 * Note that we must call "handle_stuff()" after affecting terrain features
 * in the blast radius, in case the "illumination" of the grid was changed,
 * and "update_view()" and "update_monsters()" need to be called.
 */
bool project(int who, int rad, int y, int x, s32b dam, int typ, int flg)
{
	int i, t, dist;

	int y1, x1;
	int y2, x2;

	int dist_hack = 0;

	int y_saver, x_saver; /* For reflecting monsters */

	int msec = delay_factor * delay_factor * delay_factor;

	/* Assume the player sees nothing */
	bool notice = FALSE;

	/* Assume the player has seen nothing */
	bool visual = FALSE;

	/* Assume the player has seen no blast grids */
	bool drawn = FALSE;

	/* Is the player blind? */
	bool blind = (p_ptr->blind ? TRUE : FALSE);

	/* Number of grids in the "path" */
	int path_n = 0;

	/* Actual grids in the "path" */
        u16b path_g[1024];

	/* Number of grids in the "blast area" (including the "beam" path) */
	int grids = 0;

	/* Coordinates of the affected grids */
        byte gx[1024], gy[1024];

	/* Encoded "radius" info (see above) */
        byte gm[64];

	/* First, if the player is the source, and the type is not Physical */
	/* and the player is a Zulgor, turn it into Chaos! */
	if (who <= 0 && p_ptr->prace == RACE_ZULGOR)
	{
		if (typ == GF_FIRE || typ == GF_COLD || typ == GF_ELEC || typ == GF_ACID
		|| typ == GF_POIS || typ == GF_LITE || typ == GF_DARK || typ == GF_WARP ||
		typ == GF_WATER || typ == GF_WIND || typ == GF_EARTH || typ == GF_RADIO ||
		typ == GF_SOUND || typ == GF_MANA || typ == GF_MISSILE || typ == GF_CONFUSION) typ = GF_CHAOS;
	}


	/* Hack -- Jump to target */
	if (flg & (PROJECT_JUMP))
	{
		x1 = x;
		y1 = y;

		/* Clear the flag */
		flg &= ~(PROJECT_JUMP);
	}

	/* Start at player */
	else if (who <= 0)
	{
		x1 = px;
		y1 = py;
	}

	/* Start at monster */
	else if (who > 0)
	{
		x1 = m_list[who].fx;
		y1 = m_list[who].fy;
	}

	/* Oops */
	else
	{
		x1 = x;
		y1 = y;
	}

	y_saver = y1;
	x_saver = x1;

	/* Default "destination" */
	y2 = y;
	x2 = x;


	/* Hack -- verify stuff */
	if (flg & (PROJECT_THRU))
	{
		if ((x1 == x2) && (y1 == y2))
		{
			flg &= ~(PROJECT_THRU);
		}
	}


	/* Hack -- Assume there will be no blast (max radius 16) */
        for (dist = 0; dist < 64; dist++) gm[dist] = 0;


	/* Initial grid */
	y = y1;
	x = x1;
	dist = 0;

	/* Collect beam grids */
	if (flg & (PROJECT_BEAM))
	{
		gy[grids] = y;
		gx[grids] = x;
		grids++;
	}


	/* Calculate the projection path */
	path_n = project_path(path_g, MAX_RANGE, y1, x1, y2, x2, flg);


	/* Hack -- Handle stuff */
	handle_stuff();

	/* Project along the path */
	for (i = 0; i < path_n; ++i)
	{
		int oy = y;
		int ox = x;

		int ny = GRID_Y(path_g[i]);
		int nx = GRID_X(path_g[i]);

		/* Hack -- Balls explode before reaching walls */
		if (!cave_floor_bold_project(ny, nx) && (rad > 0)) break;

		/* Advance */
		y = ny;
		x = nx;

		/* Collect beam grids */
		if (flg & (PROJECT_BEAM))
		{
			gy[grids] = y;
			gx[grids] = x;
			grids++;
		}

		/* Only do visuals if requested */
		if (!blind && !(flg & (PROJECT_HIDE)))
		{
			/* Only do visuals if the player can "see" the bolt */
			if (panel_contains(y, x) && player_has_los_bold(y, x))
			{
				u16b p;

				byte a;
				char c;

				/* Obtain the bolt pict */
				p = bolt_pict(oy, ox, y, x, typ);

				/* Extract attr/char */
				a = PICT_A(p);
				c = PICT_C(p);

				/* Visual effects */
				print_rel(c, a, y, x);
				move_cursor_relative(y, x);
				if (fresh_before) Term_fresh();
				Term_xtra(TERM_XTRA_DELAY, msec);
				lite_spot(y, x);
				if (fresh_before) Term_fresh();

				/* Display "beam" grids */
				if (flg & (PROJECT_BEAM))
				{
					/* Obtain the explosion pict */
					p = bolt_pict(y, x, y, x, typ);

					/* Extract attr/char */
					a = PICT_A(p);
					c = PICT_C(p);

					/* Visual effects */
					print_rel(c, a, y, x);
				}

				/* Hack -- Activate delay */
				visual = TRUE;
			}

			/* Hack -- delay anyway for consistency */
			else if (visual)
			{
				/* Delay for consistency */
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}
	}


	/* Save the "blast epicenter" */
	y2 = y;
	x2 = x;

	/* Start the "explosion" */
	gm[0] = 0;

	/* Hack -- make sure beams get to "explode" */
	gm[1] = grids;

	dist_hack = dist;

	/* Explode */
	if (TRUE)
	{
		/* Hack -- remove final beam grid */
		if (flg & (PROJECT_BEAM))
		{
			grids--;
		}

		/* Determine the blast area, work from the inside out */
		for (dist = 0; dist <= rad; dist++)
		{
			/* Scan the maximal blast area of radius "dist" */
			for (y = y2 - dist; y <= y2 + dist; y++)
			{
				for (x = x2 - dist; x <= x2 + dist; x++)
				{
					/* Ignore "illegal" locations */
					if (!in_bounds(y, x)) continue;

					/* Enforce a "circular" explosion */
					if (distance(y2, x2, y, x) != dist) continue;

					/* Ball explosions are stopped by walls */
					if (typ == GF_DISINTEGRATE)
					{
						if (cave_valid_bold(y,x) &&
						     (cave[y][x].feat < FEAT_PATTERN_START
						   || cave[y][x].feat > FEAT_PATTERN_XTRA2))
							cave_set_feat(y, x, FEAT_FLOOR);

						/* Update some things -- similar to GF_KILL_WALL */
						p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS);
					}
					else
					{
						if (!los(y2, x2, y, x)) continue;
					}

					/* Save this grid */
					gy[grids] = y;
					gx[grids] = x;
					grids++;
				}
			}

			/* Encode some more "radius" info */
			gm[dist+1] = grids;
		}
	}


	/* Speed -- ignore "non-explosions" */
	if (!grids) return (FALSE);


	/* Display the "blast area" if requested */
	if (!blind && !(flg & (PROJECT_HIDE)))
	{
		/* Then do the "blast", from inside out */
		for (t = 0; t <= rad; t++)
		{
			/* Dump everything with this radius */
			for (i = gm[t]; i < gm[t+1]; i++)
			{
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				/* Only do visuals if the player can "see" the blast */
				if (panel_contains(y, x) && player_has_los_bold(y, x))
				{
					u16b p;

					byte a;
					char c;

					drawn = TRUE;

					/* Obtain the explosion pict */
					p = bolt_pict(y, x, y, x, typ);

					/* Extract attr/char */
					a = PICT_A(p);
					c = PICT_C(p);

					/* Visual effects -- Display */
					print_rel(c, a, y, x);
				}
			}

			/* Hack -- center the cursor */
			move_cursor_relative(y2, x2);

			/* Flush each "radius" seperately */
			if (fresh_before) Term_fresh();

			/* Delay (efficiently) */
			if (visual || drawn)
			{
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}

		/* Flush the erasing */
		if (drawn)
		{
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++)
			{
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				/* Hack -- Erase if needed */
				if (panel_contains(y, x) && player_has_los_bold(y, x))
				{
					lite_spot(y, x);
				}
			}

			/* Hack -- center the cursor */
			move_cursor_relative(y2, x2);

			/* Flush the explosion */
			if (fresh_before) Term_fresh();
		}
	}


	/* Check features */
	if (flg & (PROJECT_GRID))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for features */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Affect the feature in that grid */
			if (project_f(who, dist, y, x, dam, typ)) notice = TRUE;
		}
	}


	/* Update stuff if needed */
	if (p_ptr->update) update_stuff();


	/* Check objects */
	if (flg & (PROJECT_ITEM))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for objects */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Affect the object in the grid */
			if (project_o(who, dist, y, x, dam, typ)) notice = TRUE;
		}
	}


	/* Check monsters */
	if (flg & (PROJECT_KILL))
	{
		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for monsters */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			if (grids > 1)
			{
				/* Affect the monster in the grid */
				if (project_m(who, dist, y, x, dam, typ)) notice = TRUE;
			}
			else
			{
				monster_race *ref_ptr = &r_info[m_list[cave[y][x].m_idx].r_idx];

				if ((ref_ptr->flags2 & (RF2_REFLECTING)) && (randint(10)!=1)
				    && (dist_hack > 1))
				{
                                        int t_y, t_x;
					int max_attempts = 10;

					/* Choose 'new' target */
					do
					{
						t_y = y_saver - 1 + randint(3);
						t_x = x_saver - 1 + randint(3);
						max_attempts--;
					}

					while (max_attempts && in_bounds2(t_y, t_x) &&
					    !(los(y, x, t_y, t_x)));

					if (max_attempts < 1)
					{
						t_y = y_saver;
						t_x = x_saver;
					}

					if (m_list[cave[y][x].m_idx].ml)
					{
						msg_print("The attack bounces!");
						ref_ptr->r_flags2 |= RF2_REFLECTING;
					}

					project(cave[y][x].m_idx, 0, t_y, t_x,  dam, typ, flg);
				}
				else
				{
					if (project_m(who, dist, y, x, dam, typ)) notice = TRUE;
				}
			}
		}

		/* Player affected one monster (without "jumping") */
		if ((who < 0) && (project_m_n == 1) && !(flg & (PROJECT_JUMP)))
		{
			/* Location */
			x = project_m_x;
			y = project_m_y;

			/* Track if possible */
			if (cave[y][x].m_idx > 0)
			{
				monster_type *m_ptr = &m_list[cave[y][x].m_idx];

				/* Hack -- auto-recall */
				if (m_ptr->ml) monster_race_track(m_ptr->r_idx);

				/* Hack - auto-track */
				if (m_ptr->ml) health_track(cave[y][x].m_idx);
			}
		}
	}


	/* Check player */
	if (flg & (PROJECT_KILL))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for player */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Affect the player */
			if (project_p(who, dist, y, x, dam, typ, rad)) notice = TRUE;
		}
	}

	/* Return "something was noticed" */
	return (notice);
}



 /*
  * Potions "smash open" and cause an area effect when
  * (1) they are shattered while in the player's inventory,
  * due to cold (etc) attacks;
  * (2) they are thrown at a monster, or obstacle;
  * (3) they are shattered by a "cold ball" or other such spell
  * while lying on the floor.
  *
  * Arguments:
  *    who   ---  who caused the potion to shatter (0=player)
  *          potions that smash on the floor are assumed to
  *          be caused by no-one (who = 1), as are those that
  *          shatter inside the player inventory.
  *          (Not anymore -- I changed this; TY)
  *    y, x  --- coordinates of the potion (or player if
  *          the potion was in her inventory);
  *    o_ptr --- pointer to the potion object.
  */
bool potion_smash_effect(int who, int y, int x, int o_sval)
 {
	int     radius = 2;
	int     dt = 0;
	int     dam = 0;
	bool    ident = FALSE;
	bool    angry = FALSE;

	switch(o_sval)
	{
		case SV_POTION_SALT_WATER:
		case SV_POTION_SLIME_MOLD:
		case SV_POTION_LOSE_MEMORIES:
		case SV_POTION_DEC_STR:
		case SV_POTION_DEC_INT:
		case SV_POTION_DEC_WIS:
		case SV_POTION_DEC_DEX:
		case SV_POTION_DEC_CON:
		case SV_POTION_DEC_CHR:
		case SV_POTION_WATER:   /* perhaps a 'water' attack? */
		case SV_POTION_APPLE_JUICE:
			return TRUE;

		case SV_POTION_INFRAVISION:
		case SV_POTION_DETECT_INVIS:
		case SV_POTION_SLOW_POISON:
		case SV_POTION_CURE_POISON:
		case SV_POTION_BOLDNESS:
		case SV_POTION_RESIST_HEAT:
		case SV_POTION_RESIST_COLD:
		case SV_POTION_HEROISM:
		case SV_POTION_BESERK_STRENGTH:
		case SV_POTION_RESTORE_EXP:
		case SV_POTION_RES_STR:
		case SV_POTION_RES_INT:
		case SV_POTION_RES_WIS:
		case SV_POTION_RES_DEX:
		case SV_POTION_RES_CON:
		case SV_POTION_RES_CHR:
		case SV_POTION_INC_STR:
		case SV_POTION_INC_INT:
		case SV_POTION_INC_WIS:
		case SV_POTION_INC_DEX:
		case SV_POTION_INC_CON:
		case SV_POTION_INC_CHR:
		case SV_POTION_AUGMENTATION:
		case SV_POTION_ENLIGHTENMENT:
		case SV_POTION_STAR_ENLIGHTENMENT:
		case SV_POTION_SELF_KNOWLEDGE:
		case SV_POTION_EXPERIENCE:
		case SV_POTION_RESISTANCE:
		case SV_POTION_INVULNERABILITY:
                case SV_POTION_STONESKIN:
                case SV_POTION_RESTORE_MANA:
                case SV_POTION_RUINATION:
			/* All of the above potions have no effect when shattered */
			return FALSE;
		case SV_POTION_SLOWNESS:
			dt = GF_OLD_SLOW;
			dam = 5;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_POISON:
			dt = GF_POIS;
                        dam = (20 * p_ptr->skill[10]) * p_ptr->skill[3];
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_BLINDNESS:
			dt = GF_DARK;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_CONFUSION: /* Booze */
			dt = GF_OLD_CONF;
			ident = TRUE;
			angry = TRUE;
			break;
		case SV_POTION_SLEEP:
			dt = GF_OLD_SLEEP;
			angry = TRUE;
			ident = TRUE;
			break;                
		case SV_POTION_DETONATIONS:
                        dt = GF_FIRE;
                        dam = (300 * p_ptr->skill[10]) * p_ptr->skill[3];
                        radius = 5;
                        angry = TRUE;
			ident = TRUE;
			break;
		case SV_POTION_DEATH:
                        dt = GF_DARK;
                        dam = (40 * p_ptr->skill[10]) * p_ptr->skill[3];
			angry = TRUE;
			ident = TRUE;
			break;
		case SV_POTION_SPEED:
			dt = GF_OLD_SPEED;
			ident = TRUE;
			break;
		case SV_POTION_CURE_LIGHT:
			dt = GF_OLD_HEAL;
                        dam = 10;
			ident = TRUE;
			break;
		case SV_POTION_CURE_SERIOUS:
			dt = GF_OLD_HEAL;
                        dam = 20;
			ident = TRUE;
			break;
		case SV_POTION_CURE_CRITICAL:
		case SV_POTION_CURING:
			dt = GF_OLD_HEAL;
                        dam = 30;
			ident = TRUE;
			break;
		case SV_POTION_HEALING:
			dt = GF_OLD_HEAL;
                        dam = 40;
			ident = TRUE;
			break;
		case SV_POTION_STAR_HEALING:
		case SV_POTION_LIFE:
                case SV_POTION_FULL_RESTORE:
			dt = GF_OLD_HEAL;
                        dam = 2000;
			radius = 1;
			ident = TRUE;
			break;
                case SV_POTION_MOLOTOV:
                        dt = GF_FIRE;
                        dam = (30 * p_ptr->skill[10]) * p_ptr->skill[3];
			ident = TRUE;
			angry = TRUE;
			break;

		default:
			/* Do nothing */  ;
	}

	(void) project(who, radius, y, x, dam, dt,
	    (PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL));

	/* XXX  those potions that explode need to become "known" */
	return angry;
}

static const int destructive_attack_types[10] = {
  GF_KILL_WALL,
  GF_KILL_DOOR,
  GF_KILL_TRAP,
  GF_STONE_WALL,
  GF_MAKE_DOOR,
  GF_MAKE_TRAP,
  GF_DESTRUCTION,
  GF_DESTRUCTION,
  GF_DESTRUCTION,
  GF_DESTRUCTION,
};

static const int attack_types[25] = {
  GF_ARROW, 
  GF_MISSILE, 
  GF_MANA,
  GF_WATER, 
  GF_PLASMA, 
  GF_METEOR, 
  GF_ICE, 
  GF_GRAVITY,
  GF_INERTIA, 
  GF_FORCE, 
  GF_TIME,
  GF_ACID, 
  GF_ELEC, 
  GF_FIRE, 
  GF_COLD, 
  GF_POIS, 
  GF_LITE, 
  GF_DARK, 
  GF_CONFUSION, 
  GF_SOUND, 
  GF_EARTH, 
  GF_NEXUS, 
  GF_NETHER, 
  GF_CHAOS,
  GF_DISENCHANT,
};

/*
 * Describe the attack using normal names. 
 */

void describe_attack_fully(int type, char* r) {
  switch (type) {
  case GF_ARROW:       strcpy(r, "arrows"); break; 
  case GF_MISSILE:     strcpy(r, "magic missiles"); break;
  case GF_MANA:        strcpy(r, "mana"); break;
  case GF_LITE_WEAK:   strcpy(r, "light"); break;
  case GF_DARK_WEAK:   strcpy(r, "dark"); break;
  case GF_WATER:       strcpy(r, "water"); break;
  case GF_PLASMA:      strcpy(r, "plasma"); break;
  case GF_METEOR:      strcpy(r, "meteors"); break;
  case GF_ICE:         strcpy(r, "ice"); break;
  case GF_GRAVITY:     strcpy(r, "gravity"); break;
  case GF_INERTIA:     strcpy(r, "inertia"); break;
  case GF_FORCE:       strcpy(r, "force"); break;
  case GF_TIME:        strcpy(r, "pure time"); break;
  case GF_ACID:        strcpy(r, "acid"); break;
  case GF_ELEC:        strcpy(r, "lightning"); break;
  case GF_FIRE:        strcpy(r, "flames"); break;
  case GF_COLD:        strcpy(r, "cold"); break;
  case GF_POIS:        strcpy(r, "poison"); break;
  case GF_LITE:        strcpy(r, "pure light"); break;
  case GF_DARK:        strcpy(r, "pure dark"); break;
  case GF_CONFUSION:   strcpy(r, "confusion"); break;
  case GF_SOUND:       strcpy(r, "sound"); break;
  case GF_EARTH:      strcpy(r, "shards"); break;
  case GF_NEXUS:       strcpy(r, "nexus"); break;
  case GF_NETHER:      strcpy(r, "nether"); break;
  case GF_CHAOS:       strcpy(r, "chaos"); break;
  case GF_DISENCHANT:  strcpy(r, "disenchantment"); break;
  case GF_KILL_WALL:   strcpy(r, "wall destruction"); break;
  case GF_KILL_DOOR:   strcpy(r, "door destruction"); break;
  case GF_KILL_TRAP:   strcpy(r, "trap destruction"); break;
  case GF_STONE_WALL:  strcpy(r, "wall creation"); break;
  case GF_MAKE_DOOR:   strcpy(r, "door creation"); break;
  case GF_MAKE_TRAP:   strcpy(r, "trap creation"); break;
  case GF_DESTRUCTION: strcpy(r, "destruction"); break;
    
  default:
    strcpy(r, "something unknown");
    break;
  }
}

/*
 * Give a randomly-generated spell a name.
 * Note that it only describes the first effect!
 */

static void name_spell(random_spell* s_ptr) {
  char buff[30];
  cptr buff2 = "???";

  if (s_ptr->proj_flags & PROJECT_STOP && s_ptr->radius == 0) {
    buff2 = "Bolt";

  } else if (s_ptr->proj_flags & PROJECT_BEAM) {
    buff2 = "Beam";

  } else if (s_ptr->proj_flags & PROJECT_STOP && s_ptr->radius > 0) {
    buff2 = "Ball";

  } else if (s_ptr->proj_flags & PROJECT_BLAST) {
    buff2 = "Blast";

  } else if (s_ptr->proj_flags & PROJECT_METEOR_SHOWER) {
    buff2 = "Area";

  } else if (s_ptr->proj_flags & PROJECT_VIEWABLE) {
    buff2 = "View";
  }

  describe_attack_fully(s_ptr->GF, buff);
  strnfmt(s_ptr->name, 30, "%s - %s", buff2, buff);
}

void generate_spell(int plev)
{
  random_spell* rspell;

  int dice, sides, chance, mana, power;

  bool destruc_gen = FALSE;
  bool simple_gen = TRUE;

  if (spell_num == MAX_SPELLS) return;

  rspell = &random_spells[spell_num];

  power = randnor(0, 10);

  dice = plev/5;
  sides = plev*2;
  mana = plev;

  /* Make the spell more or less powerful. */
  dice += power/5;
  sides += power/2;
  mana += (plev*power)/8;

  /* Stay within reasonable bounds. */
  if (dice < 1) dice = 1;
  if (dice > 10) dice = 10;

  if (sides < 1) sides = 1;
  if (sides > 100) sides = 100;

  if (mana < 1) mana = 1;

  rspell->level = plev;
  rspell->mana = mana;
  rspell->untried = TRUE;

  /* Spells are always maximally destructive. */
  rspell->proj_flags = PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID;

  chance = randint(100);

  /* Hack -- Always start with Magic Missile or derivative at lev. 1 */
  if (plev == 1 || chance < 25) {
    rspell->proj_flags |= PROJECT_STOP;
    rspell->dam_dice = dice;
    rspell->dam_sides = sides;
    rspell->radius = 0;

  } else if (chance < 50) {
    rspell->proj_flags |= PROJECT_BEAM;
    rspell->dam_dice = dice;
    rspell->dam_sides = sides;
    rspell->radius = 0;

  } else if (chance < 76) {
    rspell->proj_flags |= PROJECT_STOP;
    rspell->radius = dice;
    rspell->dam_dice = sides;
    rspell->dam_sides = 1;
    
  } else if (chance < 83) {
    rspell->proj_flags |= PROJECT_BLAST;
    rspell->radius = sides/3;
    rspell->dam_dice = dice;
    rspell->dam_sides = sides;

    destruc_gen = TRUE;
    simple_gen = FALSE;
      
  } else if (chance < 90) {
    rspell->proj_flags |= PROJECT_METEOR_SHOWER;
    rspell->dam_dice = dice;
    rspell->dam_sides = sides;
    rspell->radius = sides/3;
    if(rspell->radius < 4) rspell->radius = 4;

    destruc_gen = TRUE;

  } else {
    rspell->proj_flags |= PROJECT_VIEWABLE;
    rspell->dam_dice = dice;
    rspell->dam_sides = sides;
  }

  /* Both a destructive and a simple spell requested -- 
   * pick one or the other. */
  if (destruc_gen && simple_gen) {
    if (magik(25)) {
      simple_gen = FALSE;
    } else {
      destruc_gen = FALSE;
    }
  }

  /* Pick a simple spell */
  if (simple_gen) {
    rspell->GF = attack_types[rand_int(25)];

  /* Pick a destructive spell */
  } else {
    rspell->GF = destructive_attack_types[rand_int(10)];
  }

  /* Give the spell a name. */
  name_spell(rspell);
  sprintf(rspell->desc, "Damage: %dd%d, Power: %d", dice, sides, power);

  spell_num++;
}

/* Move the monster to a specific place */
void move_monster_spot(int m_idx, int xspot, int yspot)
{
        int ny, nx, oy, ox;

	monster_type *m_ptr = &m_list[m_idx];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Paranoia */
	if (!m_ptr->r_idx) return;

        /* Questors cannot be moved this way */
        if (r_ptr->flags1 & RF1_QUESTOR) return;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

        ny = yspot;
        nx = xspot;

	/* Update the new location */
	cave[ny][nx].m_idx = m_idx;

	/* Update the old location */
	cave[oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	lite_spot(oy, ox);

	/* Redraw the new grid */
	lite_spot(ny, nx);
}

bool lord_piercing(int basechance, int factor, int typ, monster_type *m_ptr, int checktype)
{
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (typ != p_ptr->elemlord) return (FALSE);
        else
        {
                 int chance = basechance + (factor * (p_ptr->abilities[(CLASS_ELEM_LORD * 10) + 4] - 1));

                 if (randint(100) <= chance) return (TRUE);
        }
	/* Check type 0: elemental check */
	/* Check 1: Boss abilities check */
	/* If the resistance is negative, ALWAYS return true! */
	if (checktype == 0)
	{
		switch (typ)
		{
			case GF_FIRE:
			{
				if (r_ptr->fireres < 0) return (TRUE);
				break;
			}
			case GF_COLD:
			{
				if (r_ptr->coldres < 0) return (TRUE);
				break;
			}
			case GF_ELEC:
			{
				if (r_ptr->elecres < 0) return (TRUE);
				break;
			}
			case GF_ACID:
			{
				if (r_ptr->acidres < 0) return (TRUE);
				break;
			}
			case GF_POIS:
			{
				if (r_ptr->poisres < 0) return (TRUE);
				break;
			}
			case GF_LITE:
			{
				if (r_ptr->lightres < 0) return (TRUE);
				break;
			}
			case GF_DARK:
			{
				if (r_ptr->darkres < 0) return (TRUE);
				break;
			}
			case GF_WARP:
			{
				if (r_ptr->warpres < 0) return (TRUE);
				break;
			}
			case GF_WATER:
			{
				if (r_ptr->waterres < 0) return (TRUE);
				break;
			}
			case GF_EARTH:
			{
				if (r_ptr->earthres < 0) return (TRUE);
				break;
			}
			case GF_SOUND:
			{
				if (r_ptr->soundres < 0) return (TRUE);
				break;
			}
			case GF_WIND:
			{
				if (r_ptr->windres < 0) return (TRUE);
				break;
			}
			case GF_RADIO:
			{
				if (r_ptr->radiores < 0) return (TRUE);
				break;
			}
			case GF_CHAOS:
			{
				if (r_ptr->chaosres < 0) return (TRUE);
				break;
			}
			case GF_PHYSICAL:
			{
				if (r_ptr->physres < 0) return (TRUE);
				break;
			}
			case GF_MANA:
			{
				if (r_ptr->manares < 0) return (TRUE);
				break;
			}
		}
	}

        /* Default */
        return (FALSE);
}
