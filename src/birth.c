/* File: birth.c */

/* Purpose: create a player character */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

/*
 * How often the autoroller will update the display and pause
 * to check for user interuptions.
 * Bigger values will make the autoroller faster, but slower
 * system may have problems because the user can't stop the
 * autoroller for this number of rolls.
 */
#define AUTOROLLER_STEP 25L


/*
 * Forward declare
 */
typedef struct birther birther;

/*
 * A structure to hold "rolled" information
 */
struct birther
{
	s16b age;
	s16b wt;
	s16b ht;
	s16b sc;

	s32b au;

	s16b stat[6];

	char history[4][60];

	byte weapon;
	s16b patron;
};



/*
 * The last character displayed
 */
static birther prev;



/*
 * Forward declare
 */
typedef struct hist_type hist_type;

/*
 * Player background information
 */
struct hist_type
{
	cptr info;			    /* Textual History */
	byte roll;			    /* Frequency of this entry */
	byte chart;			    /* Chart index */
	byte next;			    /* Next chart index */
	byte bonus;			    /* Social Class Bonus + 50 */
};


/*
 * Background information (see below)
 *
 * Chart progression by race:
 * Human      -->   1 -->   2 -->   3 -->  50 -->  51 -->  52 -->  53
 * Elf        -->   7 -->   8 -->   9 -->  54 -->  55 -->  56
 * Hobbit     -->  10 -->  11 -->   3 -->  50 -->  51 -->  52 -->  53
 * Gnome      -->  13 -->  14 -->   3 -->  50 -->  51 -->  52 -->  53
 * Dwarf      -->  16 -->  17 -->  18 -->  57 -->  58 -->  59 -->  60 --> 61
 * Half-Orc   -->  19 -->  20 -->   2 -->   3 -->  50 -->  51 -->  52 --> 53
 * Half-Troll -->  22 -->  23 -->  62 -->  63 -->  64 -->  65 -->  66
 * Gambolt    -->  30 -->  31 -->  32
 * High-Elf   -->   7 -->   8 -->   9 -->  54 -->  55 -->  56
 * Barbarian  -->   1 -->   2 -->   3 -->  50 -->  51 -->  52 -->  53
 * Half-Giant -->  75 -->  20 -->   2 -->   3 -->  50 -->  51 -->  52 --> 53
 * Half-Titan -->  76 -->  20 -->   2 -->   3 -->  50 -->  51 -->  52 --> 53
 * Klackon    -->  84 -->  85 -->  86
 * Kobold     -->  82 -->  83 -->  80 -->  81 -->  65 -->  66
 * Draconian  -->  89 -->  90 -->  91
 * Mindflayer -->  92 -->  93
 * Golem      -->  98 -->  99 --> 100 --> 101
 * Vampire    --> 113 --> 114 --> 115 --> 116 --> 117
 * Spectre    --> 118 --> 119 --> 134 --> 120 --> 121 --> 122 --> 123
 * Beastman   --> 129 --> 130 --> 131 --> 132 --> 133
 * Yeek       -->  77 -->  78 -->  80 -->  81 -->  65 -->  66
 * Melnibonean-->  24 -->  25 -->   3 -->  50 -->  51 -->  52 --> 53
 * Vadhagh    -->   4 -->   5 -->  55 -->  56
 *
 * XXX XXX XXX This table *must* be correct or drastic errors may occur!
 */

static hist_type bg[] =
{
	{"You are the illegitimate and unacknowledged child ", 10, 1, 2, 25},
	{"You are the illegitimate but acknowledged child ",   20, 1, 2, 35},
	{"You are one of several children ",                   95, 1, 2, 45},
	{"You are the first child ",                          100, 1, 2, 50},

	{"of a Serf.  ",                                     40, 2, 3,  65},
	{"of a Yeoman.  ",                                   65, 2, 3,  80},
	{"of a Townsman.  ",                                 80, 2, 3,  90},
	{"of a Guildsman.  ",                                90, 2, 3, 105},
	{"of a Landed Knight.  ",                            96, 2, 3, 120},
	{"of a Duke.  ",                                     99, 2, 3, 130},
	{"of a King.  ",                                    100, 2, 3, 140},

	{"You are the black sheep of the family.  ",         20, 3, 50, 20},
	{"You are a credit to the family.  ",                80, 3, 50, 55},
	{"You are a well liked child.  ",                   100, 3, 50, 60},

	{"You are one of several children of a Vadhagh ",    50, 4, 5, 50},
	{"You are the eldest of several children of a Vadhagh ", 75, 4, 5, 75},
	{"You are the only child of a Vadhagh ",             100, 4, 5, 95},

	{"commoner.  You have ",                             50, 5, 55,  75},
	{"Prince.  You have ",                              100, 5, 55, 100},

	/* Line 6 unused */

	{"You are one of several children ",                  60, 7, 8, 50},
	{"You are the only child ",                          100, 7, 8, 55},

	{"of a Teleri ",                                      75, 8, 9, 50},
	{"of a Noldor ",                                      95, 8, 9, 55},
	{"of a Vanyar ",                                     100, 8, 9, 60},

	{"Ranger.  ",                                       40, 9, 54,  80},
	{"Archer.  ",                                       70, 9, 54,  90},
	{"Warrior.  ",                                      87, 9, 54, 110},
	{"Mage.  ",                                         95, 9, 54, 125},
	{"Prince.  ",                                       99, 9, 54, 140},
	{"King.  ",                                        100, 9, 54, 145},

	{"You are one of several children of a Hobbit ",    85, 10, 11, 45},
	{"You are the only child of a Hobbit ",            100, 10, 11, 55},

	{"Bum.  ",                                          20, 11, 3,  55},
	{"Tavern Owner.  ",                                 30, 11, 3,  80},
	{"Miller.  ",                                       40, 11, 3,  90},
	{"Home Owner.  ",                                   50, 11, 3, 100},
	{"Burglar.  ",                                      80, 11, 3, 110},
	{"Warrior.  ",                                      95, 11, 3, 115},
	{"Mage.  ",                                         99, 11, 3, 125},
	{"Clan Elder.  ",                                  100, 11, 3, 140},

	/* Line 12 unused */

	{"You are one of several children of a Gnome ",     85, 13, 14, 45},
	{"You are the only child of a Gnome ",             100, 13, 14, 55},

	{"Beggar.  ",     20, 14, 3,  55},
	{"Braggart.  ",   50, 14, 3,  70},
	{"Prankster.  ",  75, 14, 3,  85},
	{"Warrior.  ",    95, 14, 3, 100},
	{"Mage.  ",      100, 14, 3, 125},

	/* Line 15 unused */

	{"You are one of two children of a Dwarven ",  25, 16, 17, 40},
	{"You are the only child of a Dwarven ",      100, 16, 17, 50},

	{"Thief.  ",         10, 17, 18,  60},
	{"Prison Guard.  ",  25, 17, 18,  75},
	{"Miner.  ",         75, 17, 18,  90},
	{"Warrior.  ",       90, 17, 18, 110},
	{"Priest.  ",        99, 17, 18, 130},
	{"King.  ",         100, 17, 18, 150},

	{"You are the black sheep of the family.  ",  15, 18, 57, 10},
	{"You are a credit to the family.  ",         85, 18, 57, 50},
	{"You are a well liked child.  ",            100, 18, 57, 55},

	{"Your mother was an Orc, but it is unacknowledged.  ",  25, 19, 20, 25},
	{"Your father was an Orc, but it is unacknowledged.  ", 100, 19, 20, 25},

	{"You are the adopted child ", 100, 20, 2, 50},

	/* Line 21 unused */

	{"Your mother was a Cave-Troll ",   30, 22, 23, 20},
	{"Your father was a Cave-Troll ",   60, 22, 23, 25},
	{"Your mother was a Hill-Troll ",   75, 22, 23, 30},
	{"Your father was a Hill-Troll ",   90, 22, 23, 35},
	{"Your mother was a Water-Troll ",  95, 22, 23, 40},
	{"Your father was a Water-Troll ", 100, 22, 23, 45},

	{"Cook.  ",         5, 23, 62, 60},
	{"Warrior.  ",     95, 23, 62, 55},
	{"Shaman.  ",      99, 23, 62, 65},
	{"Clan Chief.  ", 100, 23, 62, 80},

	{"You are the illegitimate and unacknowledged child ", 10, 24, 25, 25},
	{"You are the illegitimate but acknowledged child ",   20, 24, 25, 35},
	{"You are one of several children ",                   95, 24, 25, 45},
	{"You are the first child ",                          100, 24, 25, 50},

	{"of a Melnibonean commoner.  ",		       25, 25, 3, 30},
	{"of a Melnibonean soldier.  ",			       50, 25, 3, 50},
	{"of a Melnibonean priest.  ",			       65, 25, 3, 65},
	{"of a Melnibonean Dragonmaster.  ",		       85, 25, 3, 70},
	{"of a Melnibonean noble.  ",			      100, 25, 3, 100},

	/* Lines 26-29 unused */

	{"You were the smallest kitten in the litter.  ",   25, 30, 31, 40},
	{"You were just one of a large number of kittens in the litter.  ",
                                                            95, 30, 31, 50},
	{"You were the largest kitten in the litter.  ",   100, 30, 31, 60},

	{"Your fur is entirely white.",                     40, 31,  0, 30},
	{"Your fur is creamy and orange striped",           50, 31, 32, 40},
	{"Your fur is creamy and black striped",            60, 31, 32, 50},
	{"Your fur is light gray and striped dark gray",    70, 31, 32, 60},
	{"Your fur is gray and black striped",              80, 31, 32, 70},
	{"Your fur is a solid steel-gray",                  90, 31, 32, 80},
	{"Your fur is entirely black",                     100, 31, 32, 90},

	{", with a white chest and paws.",                  30, 32, 0,  30},
	{", with white paws.",                              70, 32, 0,  50},
	{", with a white chest.",                           90, 32, 0,  75},
	{".",                                              100, 32, 0, 100},

	/* Lines 33-49 unused */

	{"You have dark brown eyes, ",  20, 50, 51, 50},
	{"You have brown eyes, ",       60, 50, 51, 50},
	{"You have hazel eyes, ",       70, 50, 51, 50},
	{"You have green eyes, ",       80, 50, 51, 50},
	{"You have blue eyes, ",        90, 50, 51, 50},
	{"You have blue-gray eyes, ",  100, 50, 51, 50},

	{"straight ",  70, 51, 52, 50},
	{"wavy ",      90, 51, 52, 50},
	{"curly ",    100, 51, 52, 50},

	{"black hair, ",   30, 52, 53, 50},
	{"brown hair, ",   70, 52, 53, 50},
	{"auburn hair, ",  80, 52, 53, 50},
	{"red hair, ",     90, 52, 53, 50},
	{"blond hair, ",  100, 52, 53, 50},

	{"and a very dark complexion.",  10, 53, 0, 50},
	{"and a dark complexion.",       30, 53, 0, 50},
	{"and an average complexion.",   80, 53, 0, 50},
	{"and a fair complexion.",       90, 53, 0, 50},
	{"and a very fair complexion.", 100, 53, 0, 50},

	{"You have light grey eyes, ",   85, 54, 55, 50},
	{"You have light blue eyes, ",   95, 54, 55, 50},
	{"You have light green eyes, ", 100, 54, 55, 50},

	{"straight ",  75, 55, 56, 50},
	{"wavy ",     100, 55, 56, 50},

	{"black hair, and a fair complexion.",   75, 56, 0, 50},
	{"brown hair, and a fair complexion.",   85, 56, 0, 50},
	{"blond hair, and a fair complexion.",   95, 56, 0, 50},
	{"silver hair, and a fair complexion.", 100, 56, 0, 50},

	{"You have dark brown eyes, ",   99, 57, 58, 50},
	{"You have glowing red eyes, ", 100, 57, 58, 60},

	{"straight ",  90, 58, 59, 50},
	{"wavy ",     100, 58, 59, 50},

	{"black hair, ",  75, 59, 60, 50},
	{"brown hair, ", 100, 59, 60, 50},

	{"a one foot beard, ",    25, 60, 61, 52},
	{"a two foot beard, ",    60, 60, 61, 55},
	{"a three foot beard, ",  90, 60, 61, 58},
	{"a four foot beard, ",  100, 60, 61, 60},

	{"and a dark complexion.", 100, 61, 0, 50},

	{"You have slime green eyes, ",     60, 62, 63, 50},
	{"You have puke yellow eyes, ",     85, 62, 63, 50},
	{"You have blue-bloodshot eyes, ",  99, 62, 63, 50},
	{"You have glowing red eyes, ",    100, 62, 63, 55},

	{"dirty ",  33, 63, 64, 50},
	{"mangy ",  66, 63, 64, 50},
	{"oily ",  100, 63, 64, 50},

	{"sea-weed green hair, ",  33, 64, 65, 50},
	{"bright red hair, ",      66, 64, 65, 50},
	{"dark purple hair, ",    100, 64, 65, 50},

	{"and green ",  25, 65, 66, 50},
	{"and blue ",   50, 65, 66, 50},
	{"and white ",  75, 65, 66, 50},
	{"and black ", 100, 65, 66, 50},

	{"ulcerous skin.",					 33, 66, 0, 50},
	{"scabby skin.",					 66, 66, 0, 50},
	{"leprous skin.",       		                100, 66, 0, 50},

	/* Lines 67-74 unused */

	{"Your mother was a Hill Giant.  ",			 10, 75, 20, 50},
	{"Your mother was a Fire Giant.  ",			 12, 75, 20, 55},
	{"Your mother was a Frost Giant.  ",			 20, 75, 20, 60},
	{"Your mother was a Cloud Giant.  ",			 23, 75, 20, 65},
	{"Your mother was a Storm Giant.  ",			 25, 75, 20, 70},
	{"Your mother was a Mountain Giant.  ",		30, 75, 20, 60},
	{"Your father was a Mountain Giant.  ",		35, 75, 20, 60},
	{"Your father was a Hill Giant.  ",			 60, 75, 20, 50},
	{"Your father was a Fire Giant.  ",			 70, 75, 20, 55},
	{"Your father was a Frost Giant.  ",			 80, 75, 20, 60},
	{"Your father was a Cloud Giant.  ",			 90, 75, 20, 65},
	{"Your father was a Storm Giant.  ",			100, 75, 20, 70},

	{"Your father was an unknown Titan.  ",			 75, 76, 20,  50},
	{"Your mother was Themis.  ",				 80, 76, 20, 100},
	{"Your mother was Mnemosyne.  ",			 85, 76, 20, 100},
	{"Your father was Okeanoas.  ",				 90, 76, 20, 100},
	{"Your father was Crius.  ",				 95, 76, 20, 100},
	{"Your father was Hyperion.  ",				 98, 76, 20, 125},
	{"Your father was Kronos.  ",				100, 76, 20, 150},

	{"You are one of several children of ",			100, 77, 78,  50 },

	{"a Brown Yeek. ",					 50, 78, 80,  50 },
	{"a Blue Yeek.  ",					 75, 78, 80,  50 },
	{"a Master Yeek.  ",					 95, 78, 80,  85 },
	{"Boldor, King of the Yeeks.  ",			100, 78, 80, 120 },

	/* Line 79 unused */

	{"You have pale eyes, ",				 25, 80, 81, 50},
	{"You have glowing eyes, ",				 50, 80, 81, 50},
	{"You have tiny black eyes, ",				 75, 80, 81, 50},
	{"You have shining black eyes, ",			100, 80, 81, 50},

	{"no hair at all, ",					 20, 81, 65, 50},
	{"short black hair, ",					 40, 81, 65, 50},
	{"long black hair, ",					 60, 81, 65, 50},
	{"bright red hair, ",					 80, 81, 65, 50},
	{"colourless albino hair, ",				100, 81, 65, 50},

	{"You are one of several children of ",			100, 82, 83, 50},

	{"a Small Kobold.  ",					 40, 83, 80, 50},
	{"a Kobold.  ",						 75, 83, 80, 55},
	{"a Large Kobold.  ",					 95, 83, 80, 65},
	{"Mughash, the Kobold Lord.  ",				100, 83, 80, 100},

	{"You are one of several children of a Klackon hive queen.  ", 100, 84, 85, 50},

	{"You have red skin, ",					 40, 85, 86, 50},
	{"You have black skin, ",				 90, 85, 86, 50},
	{"You have yellow skin, ",				100, 85, 86, 50},

	{"and black eyes.",					100, 86, 0, 50},

	/* Lines 87-88 unused */

	{"You are one of several children of a Draconian ",	 85, 89, 90, 50},
	{"You are the only child of a Draconian ",		100, 89, 90, 55},

	{"Warrior.  ",						 50, 90, 91, 50},
	{"Priest.  ",						 65, 90, 91, 65},
	{"Mage.  ",						 85, 90, 91, 70},
	{"Noble.  ",						100, 90, 91, 100},

	{"You have green wings, green skin and yellow belly.",	 30, 91, 0, 50},
	{"You have green wings, and green skin.",		 55, 91, 0, 50},
	{"You have red wings, and red skin.",			 80, 91, 0, 50},
	{"You have black wings, and black skin.",		 90, 91, 0, 50},
	{"You have metallic skin, and shining wings.",		100, 91, 0, 50},

	{"You have slimy skin, empty glowing eyes, and ",	100, 92, 93, 80},

	{"three tentacles around your mouth.",			 20, 93, 0, 45},
	{"four tentacles around your mouth.",			 80, 93, 0, 50},
	{"five tentacles around your mouth.",			100, 93, 0, 55},

	/* Lines 94-97 unused */

	{"You were shaped from ",				100, 98, 99, 50},

	{"clay ",						 40, 99, 100,  40},
	{"stone ",						 75, 99, 100,  45},
	{"copper ",						 85, 99, 100,  50},
	{"iron ",						 95, 99, 100,  70},
	{"silver ",						 99, 99, 100,  85},
	{"gold ",						100, 99, 100, 100},

	{"by a Kabbalist",					 40, 100, 101, 50},
	{"by a Wizard",						 65, 100, 101, 50},
	{"by an Alchemist",					 90, 100, 101, 50},
	{"by a Priest",						100, 100, 101, 60},

	{" to fight evil.",					 10, 101, 0, 80},
	{" to do heavy labour.",				 30, 101, 0, 60},
	{" to do household chores.",				 50, 101, 0, 40},
	{" to keep the crows out of the corn.",			 60, 101, 0, 30},
	{".",							100, 101, 0, 20},

	/* Lines 102-112 unused */

	{"You arose from an unmarked grave.  ",			 20, 113, 114, 50},
	{"In life you were a simple peasant.  ",		 40, 113, 114, 50},
	{"In life you were a Vampire Hunter, but they got you.  ", 60, 113, 114, 50},
	{"In life you were a Necromancer.  ",			 80, 113, 114, 50},
	{"In life you were a powerful noble.  ",		 95, 113, 114, 50},
	{"In life you were a powerful and cruel tyrant.  ",	100, 113, 114, 50},

	{"You have ",						100, 114, 115, 50},

	{"jet-black hair, ",					 25, 115, 116, 50},
	{"matted brown hair, ",					 50, 115, 116, 50},
	{"white hair, ",					 75, 115, 116, 50},
	{"a hairless head, ",					100, 115, 116, 50},

	{"eyes like red coals, ",				 25, 116, 117, 50},
	{"blank white eyes, ",					 50, 116, 117, 50},
	{"feral yellow eyes, ",					 75, 116, 117, 50},
	{"bloodshot red eyes, ",				100, 116, 117, 50},

	{"and a deathly pale complexion.",			100, 117, 0, 50},

	{"You were created by ",				100, 118, 119, 50},

	{"a Necromancer.  ",					 30, 119, 134, 50},
	{"a magical experiment.  ",				 50, 119, 134, 50},
	{"an Evil Priest.  ",					 70, 119, 134, 50},
	{"a pact with the demons.  ",				 75, 119, 134, 50},
	{"a restless spirit.  ",				 85, 119, 134, 50},
	{"a curse.  ",						 95, 119, 134, 30},
	{"an oath.  ",						100, 119, 134, 50},

	{"jet-black hair, ",					 25, 120, 121, 50},
	{"matted brown hair, ",					 50, 120, 121, 50},
	{"white hair, ",					 75, 120, 121, 50},
	{"a hairless head, ",					100, 120, 121, 50},

	{"eyes like red coals, ",				 25, 121, 122, 50},
	{"blank white eyes, ",					 50, 121, 122, 50},
	{"feral yellow eyes, ",					 75, 121, 122, 50},
	{"bloodshot red eyes, ",				100, 121, 122, 50},

	{" and a deathly gray complexion. ",			100, 122, 123, 50},

	{"An eerie green aura surrounds you.",			100, 123, 0, 50},

	/* Lines 124-128 unused */

	{"You were produced by a magical experiment.  ",	 30, 129, 130, 40},
	{"As a child, you were stupid enough to stick your head in raw Chaos.  ",
								 50, 129, 130, 50},
	{"A Lord of Chaos wanted some fun, so he created you.  ",
								 60, 129, 130, 60},
	{"You are the magical crossbreed of an animal and a man.  ",
								 75, 129, 130, 50},
	{"You are the blasphemous crossbreed of unspeakable creatures of Chaos.  ",
								100, 129, 130, 30},

	{"You have green reptilian eyes, ",			 60, 130, 131, 50},
	{"You have the black eyes of a bird, ",			 85, 130, 131, 50},
	{"You have the orange eyes of a cat, ",			 99, 130, 131, 50},
	{"You have the fiery eyes of a demon, ",		100, 130, 131, 55},

	{"no hair at all, ",					 10, 131, 133, 50},
	{"dirty ",						 33, 131, 132, 50},
	{"mangy ",						 66, 131, 132, 50},
	{"oily ",						100, 131, 132, 50},

	{"brown fur, ",						 33, 132, 133, 50},
	{"gray fur, ",						 66, 132, 133, 50},
	{"albino fur, ",					100, 132, 133, 50},

	{"and the hooves of a goat.",				 50, 133, 0, 50},
	{"and human feet.",					 75, 133, 0, 50},
	{"and bird's feet.",					 85, 133, 0, 50},
	{"and reptilian feet.",					 90, 133, 0, 50},
	{"and bovine feet.",					 95, 133, 0, 50},
	{"and feline feet.",					 97, 133, 0, 50},
	{"and canine feet.",					100, 133, 0, 50},

	{"You have ",						100, 134, 120, 50},
};



/*
 * Current stats
 */
static s16b stat_use[6];

/*
 * Autoroll limit
 */
static s16b stat_limit[6];

/*
 * Autoroll matches
 */
static s32b stat_match[6];

/*
 * Autoroll round
 */
static s32b auto_round;

/*
 * Last round
 */
static s32b last_round;

byte choose_realm(byte choices)
{

	int picks[MAX_REALM] = {0};
	int k, n;

	char c;

	char p2 = ')';

	char buf[80];

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"The realm of magic will determine which spells you can learn.");
	Term_putstr(5, 16, -1, TERM_WHITE,
		"Life and Sorcery are protective, Chaos and Death are destructive.");
	Term_putstr(5, 17, -1, TERM_WHITE,
		"Nature has both defensive and offensive spells.");

	n = 0;

/* Hack: Allow priests to specialize in Life or Death magic */

	if ((choices & CH_LIFE) && p_ptr->realm1 != REALM_LIFE)
	{
		sprintf(buf, "%c%c %s", I2A(n), p2, "Life");
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
		picks[n] = REALM_LIFE;
		n++;
	}

	if ((choices & CH_SORCERY) && p_ptr->realm1 != REALM_SORCERY)
	{
		sprintf(buf, "%c%c %s", I2A(n), p2, "Sorcery");
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
		picks[n] = REALM_SORCERY;
		n++;
	}

	if ((choices & CH_NATURE) && p_ptr->realm1 != REALM_NATURE)
	{
		sprintf(buf, "%c%c %s", I2A(n), p2, "Nature");
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
		picks[n] = REALM_NATURE;
		n++;
	}

	if ((choices & CH_CHAOS) && p_ptr->realm1 != REALM_CHAOS)
	{
		sprintf(buf, "%c%c %s", I2A(n), p2, "Chaos");
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
		picks[n] = REALM_CHAOS;
		n++;
	}

	if ((choices & CH_DEATH) && p_ptr->realm1 != REALM_DEATH)
	{
		sprintf(buf, "%c%c %s", I2A(n), p2, "Death");
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
		picks[n] = REALM_DEATH;
		n++;
	}

	if ((choices & CH_TRUMP) && p_ptr->realm1 != REALM_TRUMP)
	{
		sprintf(buf, "%c%c %s", I2A(n), p2, "Trump");
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
		picks[n] = REALM_TRUMP;
		n++;
	}

	if ((choices & CH_ARCANE) && p_ptr->realm1 != REALM_ARCANE)
	{
		sprintf(buf, "%c%c %s", I2A(n), p2, "Arcane");
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
		picks[n] = REALM_ARCANE;
		n++;
	}

	/* Get a class */
	while (1)
	{
		sprintf(buf, "Choose a realm (%c-%c): ", I2A(0), I2A(n-1));
		put_str(buf, 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		k = (islower(c) ? A2I(c) : -1);
		if ((k >= 0) && (k < n)) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}


	/* Clean up */

	clear_from(15);

	return (picks[k]);
}

void get_realms()
{
	int pclas=p_ptr->pclass;

	/* First we have null realms */
	p_ptr->realm1=p_ptr->realm2=REALM_NONE;

	/* Warriors and certain others get no realms */

	if (realm_choices[pclas] == (CH_NONE)) return;

	/* Other characters get at least one realm */

	switch (pclas)
	{
	case CLASS_PRIEST:
		p_ptr->realm1 = choose_realm( CH_LIFE | CH_DEATH);
		/*
		 * Hack... priests can be 'dark' priests and choose death
		 * instead of life, but not both
		 */
		break;
	case CLASS_RANGER:
		p_ptr->realm1 = REALM_NATURE;
		break;
	case CLASS_WARRIOR_MAGE:
		p_ptr->realm1 = REALM_ARCANE;
		break;
	case CLASS_CHAOS_WARRIOR:
		p_ptr->realm1 = REALM_CHAOS;
		break;
	default:
		p_ptr->realm1 = choose_realm(realm_choices[pclas]);
	}

	/* Some classes get no second realm */
	if (pclas == CLASS_ROGUE || pclas == CLASS_RANGER ||
	    pclas == CLASS_PALADIN || pclas == CLASS_WARRIOR_MAGE ||
	    pclas == CLASS_CHAOS_WARRIOR || pclas == CLASS_MONK ||
	    pclas == CLASS_HIGH_MAGE) return;
	else
		p_ptr->realm2 = choose_realm(realm_choices[pclas]);
}


/*
 * Name segments for random player names
 * Transplanted and modified from CthAngband -- Gumby
 */

/* Dwarves */
static char *dwarf_syllable1[] =
{
	"B", "D", "F", "G", "Gl", "H", "K", "L", "M", "N", "R", "S", "T",
	"Th", "V",
};

static char *dwarf_syllable2[] =
{
	"a", "e", "i", "o", "oi", "u",
};

static char *dwarf_syllable3[] =
{
	"bur", "fur", "gan", "gnus", "gnar", "li", "lin", "lir", "mli",
	"nar", "nus", "rin", "ran", "sin", "sil", "sur",
};

/* Elves */
static char *elf_syllable1[] =
{
	"Al", "An", "Bal", "Bel", "Cal", "Cel", "El", "Elr", "Elv", "Eow",
	"Ear", "F", "Fal", "Fel", "Fin", "G", "Gal", "Gel", "Gl", "Is",
	"Lan", "Leg", "Lom", "N", "Nal", "Nel",  "S", "Sal", "Sel", "T",
	"Tal", "Tel", "Thr", "Tin",
};

static char *elf_syllable2[] =
{
	"a", "adrie", "ara", "e", "ebri", "ele", "ere", "i", "io", "ithra",
	"ilma", "il-Ga", "ili", "o", "orfi", "u", "y",
};

static char *elf_syllable3[] =
{
	"l", "las", "lad", "ldor", "ldur", "linde", "lith", "mir", "n",
	"nd", "ndel", "ndil", "ndir", "nduil", "ng", "mbor", "r", "rith",
	"ril", "riand", "rion", "s", "thien", "viel", "wen", "wyn",
};

/* Gnomes */
static char *gnome_syllable1[] =
{
	"Aar", "An", "Ar", "As", "C", "H", "Han", "Har", "Hel", "Iir", "J",
	"Jan", "Jar", "K", "L", "M", "Mar", "N", "Nik", "Os", "Ol", "P",
	"R", "S", "Sam", "San", "T", "Ter", "Tom", "Ul", "V", "W", "Y",
};

static char *gnome_syllable2[] =
{
	"a", "aa",  "ai", "e", "ei", "i", "o", "uo", "u", "uu",
};

static char *gnome_syllable3[] =
{
	"ron", "re", "la", "ki", "kseli", "ksi", "ku", "ja", "ta", "na",
	"namari", "neli", "nika", "nikki", "nu", "nukka", "ka", "ko", "li",
	"kki", "rik", "po", "to", "pekka", "rjaana", "rjatta", "rjukka",
	"la", "lla", "lli", "mo", "nni",
};

/* Hobbit */
static char *hobbit_syllable1[] =
{
	"B", "Ber", "Br", "D", "Der", "Dr", "F", "Fr", "G", "H", "L", "Ler",
	"M", "Mer", "N", "P", "Pr", "Per", "R", "S", "T", "W",
};

static char *hobbit_syllable2[] =
{
	"a", "e", "i", "ia", "o", "oi", "u",
};

static char *hobbit_syllable3[] =
{
	"bo", "ck", "decan", "degar", "do", "doc", "go", "grin", "lba",
	"lbo", "lda", "ldo", "lla", "ll", "lo", "m", "mwise", "nac", "noc",
	"nwise", "p", "ppin", "pper", "tho", "to",
};

/* Human */
static char *human_syllable1[] =
{
	"Ab", "Ac", "Ad", "Af", "Agr", "Ast", "As", "Al", "Adw", "Adr",
	"Ar", "B", "Br", "C", "Cr", "Ch", "Cad", "D", "Dr", "Dw", "Ed",
	"Eth", "Et", "Er", "El", "Eow", "F", "Fr", "G", "Gr", "Gw", "Gal",
	"Gl", "H", "Ha", "Ib", "Jer", "K", "Ka", "Ked", "L", "Loth", "Lar",
	"Leg", "M", "Mir", "N", "Nyd", "Ol", "Oc", "On", "P", "Pr", "R",
	"Rh", "S", "Sev", "T", "Tr", "Th", "V", "Y", "Z", "W", "Wic",
};

static char *human_syllable2[] =
{
	"a", "ae", "au", "ao", "are", "ale", "ali", "ay", "ardo", "e", "ei",
	"ea", "eri", "era", "ela", "eli", "enda", "erra", "i", "ia", "ie",
	"ire", "ira", "ila", "ili", "ira", "igo", "o", "oa", "oi", "oe",
	"ore", "u", "y",
};

static char *human_syllable3[] =
{
	"a", "and", "b", "bwyn", "baen", "bard", "c", "ctred", "cred", "ch",
	"can", "d", "dan", "don", "der", "dric", "dfrid", "dus", "f", "g",
	"gord", "gan", "l", "li", "lgrin", "lin", "lith", "lath", "loth",
	"ld", "ldric", "ldan", "m", "mas", "mos", "mar", "mond", "n",
	"nydd", "nidd", "nnon", "nwan", "nyth", "nad", "nn", "nnor", "nd",
	"p", "r", "ron", "rd", "s", "sh", "seth", "sean", "t", "th", "tha",
	"tlan", "trem", "tram", "v", "vudd", "w", "wan", "win", "wyn",
	"wyr", "wyr", "wyth",
};

/* Orc */
static char *orc_syllable1[] =
{
	"B", "Er", "G", "Gr", "H", "P", "Pr", "R", "V", "Vr", "T", "Tr",
	"M", "Dr",
};

static char *orc_syllable2[] =
{
	"a", "i", "o", "oo", "u", "ui",
};

static char *orc_syllable3[] =
{
	"dash", "dish", "dush", "gar", "gor", "gdush", "lo", "gdish", "k",
	"lg", "nak", "rag", "rbag", "rg", "rk", "ng", "nk", "rt", "ol",
	"urk", "shnak", "mog", "mak", "rak",
};

/* Klackon */
static char *klackon_syllable1[] =
{
	"K'", "K", "Kri", "Kir", "Kiri", "Iriki", "Irik", "Karik", "Iri",
	"Akri",
};

static char *klackon_syllable2[] =
{
	"arak", "i", "iri", "ikki", "ki", "kiri", "ikir", "irak", "arik",
	"k'", "r",
};

static char *klackon_syllable3[] =
{
	"akkak", "ak", "ik", "ikkik", "irik", "arik", "kidik", "kii", "k",
	"ki","riki","irk",
};

static char *cthuloid_syllable1[] =
{
	"Cth", "Az", "Fth", "Ts", "Xo", "Q'N", "R'L", "Ghata", "L", "Zz",
	"Fl", "Cl", "S", "Y",
};

static char *cthuloid_syllable2[] =
{
	"nar", "loi", "ul", "lu", "noth", "thon", "ath", "'N", "rhy", "oth",
	"aza", "agn", "oa", "og",
};

static char *cthuloid_syllable3[] =
{
	"l", "a", "u", "oa", "oggua", "oth", "ath", "aggua", "lu", "lo",
	"loth", "lotha", "agn", "axl",
};


/*
 * Random Name Generator
 * based on a Javascript by Michael Hensley
 * "http://geocities.com/timessquare/castle/6274/"
 *
 * Transplanted and modified from CthAngband -- Gumby
 */
static void create_random_name(int race, char *name)
{
	/* Paranoia */
	if (!name) return;

	/* Select the monster type */
	switch (race)
	{
	/* Create the monster name */
	case RACE_DWARF:	case RACE_HALF_GIANT:	case RACE_GOLEM:
		strcpy(name, dwarf_syllable1[rand_int(sizeof(dwarf_syllable1) / sizeof(char*))]);
		strcat(name, dwarf_syllable2[rand_int(sizeof(dwarf_syllable2) / sizeof(char*))]);
		strcat(name, dwarf_syllable3[rand_int(sizeof(dwarf_syllable3) / sizeof(char*))]);
		break;
	case RACE_ELF:		case RACE_HIGH_ELF:
	case RACE_MELNIBONEAN:	case RACE_VADHAGH:
		strcpy(name, elf_syllable1[rand_int(sizeof(elf_syllable1) / sizeof(char*))]);
		strcat(name, elf_syllable2[rand_int(sizeof(elf_syllable2) / sizeof(char*))]);
		strcat(name, elf_syllable3[rand_int(sizeof(elf_syllable3) / sizeof(char*))]);
		break;
	case RACE_DRACONIAN:	case RACE_GNOME:
		strcpy(name, gnome_syllable1[rand_int(sizeof(gnome_syllable1) / sizeof(char*))]);
		strcat(name, gnome_syllable2[rand_int(sizeof(gnome_syllable2) / sizeof(char*))]);
		strcat(name, gnome_syllable3[rand_int(sizeof(gnome_syllable3) / sizeof(char*))]);
		break;
	case RACE_HOBBIT:	case RACE_YEEK:
		strcpy(name, hobbit_syllable1[rand_int(sizeof(hobbit_syllable1) / sizeof(char*))]);
		strcat(name, hobbit_syllable2[rand_int(sizeof(hobbit_syllable2) / sizeof(char*))]);
		strcat(name, hobbit_syllable3[rand_int(sizeof(hobbit_syllable3) / sizeof(char*))]);
		break;
	case RACE_BARBARIAN:	case RACE_HALF_TITAN:	case RACE_HUMAN:
	case RACE_SPECTRE:	case RACE_VAMPIRE:	case RACE_GAMBOLT:
		strcpy(name, human_syllable1[rand_int(sizeof(human_syllable1) / sizeof(char*))]);
		strcat(name, human_syllable2[rand_int(sizeof(human_syllable2) / sizeof(char*))]);
		strcat(name, human_syllable3[rand_int(sizeof(human_syllable3) / sizeof(char*))]);
		break;
	case RACE_HALF_ORC:	case RACE_HALF_TROLL:	case RACE_KOBOLD:
	case RACE_BEASTMAN:
		strcpy(name, orc_syllable1[rand_int(sizeof(orc_syllable1) / sizeof(char*))]);
		strcat(name, orc_syllable2[rand_int(sizeof(orc_syllable2) / sizeof(char*))]);
		strcat(name, orc_syllable3[rand_int(sizeof(orc_syllable3) / sizeof(char*))]);
		break;
	case RACE_KLACKON:
		strcpy(name, klackon_syllable1[rand_int(sizeof(klackon_syllable1) / sizeof(char*))]);
		strcat(name, klackon_syllable2[rand_int(sizeof(klackon_syllable2) / sizeof(char*))]);
		strcat(name, klackon_syllable3[rand_int(sizeof(klackon_syllable3) / sizeof(char*))]);
		break;
	case RACE_MIND_FLAYER:
		strcpy(name, cthuloid_syllable1[rand_int(sizeof(cthuloid_syllable1) / sizeof(char*))]);
		strcat(name, cthuloid_syllable2[rand_int(sizeof(cthuloid_syllable2) / sizeof(char*))]);
		strcat(name, cthuloid_syllable3[rand_int(sizeof(cthuloid_syllable3) / sizeof(char*))]);
		break;
		/* Create an empty name */
	default:
		name[0] = '\0';
		break;
	}
}


/*
 * Allow player to modify the character by spending points
 * Transplanted from CthAngband -- Gumby
 */
static bool point_mod_player(void)
{
	char b1 = '[';
	char b2 = ']';
	char stat;
	char modpts[4] = "none";
	int x = 0;
	int i, points;
 
 
	points = 34;

	sprintf(modpts,"%d",points);

	clear_from(23);

	while(1)
	{ 
		/* reset variable */
		i = 0; 

		/* Calculate the bonuses and hitpoints and mana */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);

		/* Update stuff */
		update_stuff();

		/* Fully healed */
		p_ptr->chp = p_ptr->mhp;

		/* Fully rested */
		p_ptr->csp = p_ptr->msp;

		/* Display the player */
		display_player(0);

		/* Display Stat Menu */
		clear_from(23);

		sprintf(modpts,"%d",points);
		
		Term_putstr(73,2,-1,TERM_WHITE,"<-S/s->");
		Term_putstr(73,3,-1,TERM_WHITE,"<-I/i->");
		Term_putstr(73,4,-1,TERM_WHITE,"<-W/w->");
		Term_putstr(73,5,-1,TERM_WHITE,"<-D/d->");
		Term_putstr(73,6,-1,TERM_WHITE,"<-C/c->");
		Term_putstr(73,7,-1,TERM_WHITE,"<-H/h->");

		Term_gotoxy(2, 23);
		Term_addch(TERM_WHITE, b1);

		if (points == 0)	Term_addstr(-1, TERM_GREEN, modpts);
		else if (points > 0)	Term_addstr(-1, TERM_YELLOW, modpts);
		else			Term_addstr(-1, TERM_RED, modpts);

/*		Term_addstr(-1, TERM_WHITE, " points left. Press 'ESC' whilst on 0 points to finish."); */
		Term_addstr(-1, TERM_WHITE, " points left. Press 'ESC' with 0 or more points left to finish.");
		Term_addch(TERM_WHITE, b2);

		/* Get an entry */
		stat = inkey();

		/* ESC goes back to previous menu */
/*		if ((stat == ESCAPE) && (points == 0)) break; */
		if ((stat == ESCAPE) && (points >= 0)) break;

		/* Assign values to entries, stats 0 to 5 */
		switch (stat)
		{
			/* The index to a specific stat is retrieved */
			case 's':	i = 1;	break;
			case 'S':	i = 1;	break;
			case 'i':	i = 2;	break;
			case 'I':	i = 2;	break;
			case 'w':	i = 3;	break;
			case 'W':	i = 3;	break;
			case 'd':	i = 4;	break;
			case 'D':	i = 4;	break;
			case 'c':	i = 5;	break;
			case 'C':	i = 5;	break;
			case 'h':	i = 6;	break;
			case 'H':	i = 6;	break;
			default:	i = 0;
		}  

		/* Test for invalid key */
		if (!i) continue;
		i--;

		/* Test for lower case (add to stat) or 
		upper case (subtract stat) */

		if (islower(stat)) /* ('a' < stat) */
		{
			/* different conditions for maximize on */
			if (p_ptr->maximize)
			{
				/* Max stat increase */
				if (p_ptr->stat_max[i] < 17)
				{
					p_ptr->stat_cur[i] = ++p_ptr->stat_max[i];
				}
				else
				{
					continue;
				}
			}
			else
			{
				/* Max stat increase, maximize off */
				x = rp_ptr->r_adj[i] + cp_ptr->c_adj[i];

				if (x > 8) x = 8;
				if (x > 0) x *= 13;

				if (p_ptr->stat_max[i] < 18 + x)    
				{
					if (p_ptr->stat_max[i]> 17)
					{
						p_ptr->stat_max[i] += 10;
						p_ptr->stat_cur[i] += 10;
					}
					else
					{
						p_ptr->stat_cur[i] = ++p_ptr->stat_max[i];
					}
				}
				else
				{
					continue;
				}
			}

			/* Higher stats linearly cost more */
			if (p_ptr->stat_max[i] > 97)	points--; 
			if (p_ptr->stat_max[i] > 67)	points--;
			if (p_ptr->stat_max[i] > 18)	points--;
			if (p_ptr->stat_max[i] > 14)	points--;
			if (p_ptr->stat_max[i] > 3)	points--;
			continue;
		}
		else    /* Reduce stat case */
		{ 
			if (p_ptr->stat_use[i] > 3)
			{
				if (p_ptr->stat_max[i] > 27)
				{ 
					p_ptr->stat_max[i] -= 10;
					p_ptr->stat_cur[i] -= 10;
				}
				else
				{
					p_ptr->stat_cur[i] = --p_ptr->stat_max[i];
				}
			}
			else
			{
				continue;
			}

			/* Higher stats yield more mod points */
			if (p_ptr->stat_max[i] > 87)	points++; 
			if (p_ptr->stat_max[i] > 57)	points++;
			if (p_ptr->stat_max[i] > 17)	points++;
			if (p_ptr->stat_max[i] > 13)	points++;
			if (p_ptr->stat_max[i] > 2)	points++;
			continue;
		}
	}
	return TRUE;
}


/*
 * Save the current data for later
 */
static void save_prev_data(void)
{
	int i;


	/*** Save the current data ***/

	/* Save the data */
	prev.age = p_ptr->age;
	prev.wt = p_ptr->wt;
	prev.ht = p_ptr->ht;
	prev.sc = p_ptr->sc;
	prev.au = p_ptr->au;
	prev.weapon = p_ptr->wm_choice;
	prev.patron = p_ptr->chaos_patron;

	/* Save the stats */
	for (i = 0; i < 6; i++)
	{
		prev.stat[i] = p_ptr->stat_max[i];
	}

	/* Save the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(prev.history[i], history[i]);
	}
}


/*
 * Load the previous data
 */
static void load_prev_data(void)
{
	int i;

	birther	temp;


	/*** Save the current data ***/

	/* Save the data */
	temp.age = p_ptr->age;
	temp.wt = p_ptr->wt;
	temp.ht = p_ptr->ht;
	temp.sc = p_ptr->sc;
	temp.au = p_ptr->au;
	temp.weapon = p_ptr->wm_choice;
	temp.patron = p_ptr->chaos_patron;

	/* Save the stats */
	for (i = 0; i < 6; i++)
	{
		temp.stat[i] = p_ptr->stat_max[i];
	}

	/* Save the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(temp.history[i], history[i]);
	}


	/*** Load the previous data ***/

	/* Load the data */
	p_ptr->age = prev.age;
	p_ptr->wt = prev.wt;
	p_ptr->ht = prev.ht;
	p_ptr->sc = prev.sc;
	p_ptr->au = prev.au;
	p_ptr->wm_choice = prev.weapon;
	p_ptr->chaos_patron = prev.patron;

	/* Load the stats */
	for (i = 0; i < 6; i++)
	{
		p_ptr->stat_max[i] = prev.stat[i];
		p_ptr->stat_cur[i] = prev.stat[i];
	}

	/* Load the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(history[i], prev.history[i]);
	}


	/*** Save the current data ***/

	/* Save the data */
	prev.age = temp.age;
	prev.wt = temp.wt;
	prev.ht = temp.ht;
	prev.sc = temp.sc;
	prev.au = temp.au;
	prev.weapon = temp.weapon;
	prev.patron = temp.patron;

	/* Save the stats */
	for (i = 0; i < 6; i++)
	{
		prev.stat[i] = temp.stat[i];
	}

	/* Save the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(prev.history[i], temp.history[i]);
	}
}




/*
 * Returns adjusted stat -JK-  Algorithm by -JWT-
 *
 * auto_roll is boolean and states maximum changes should be used rather
 * than random ones to allow specification of higher values to wait for
 *
 * The "p_ptr->maximize" code is important	-BEN-
 */
static int adjust_stat(int value, s16b amount, int auto_roll)
{
	int i;

	/* Negative amounts */
	if (amount < 0)
	{
		/* Apply penalty */
		for (i = 0; i < (0 - amount); i++)
		{
			if (value >= 18+10)
			{
				value -= 10;
			}
			else if (value > 18)
			{
				value = 18;
			}
			else if (value > 3)
			{
				value--;
			}
		}
	}

	/* Positive amounts */
	else if (amount > 0)
	{
		/* Apply reward */
		for (i = 0; i < amount; i++)
		{
			if (value < 18)
			{
				value++;
			}
			else if (p_ptr->maximize)
			{
				value += 10;
			}
			else if (value < 18+70)
			{
				value += ((auto_roll ? 15 : randint(15)) + 5);
			}
			else if (value < 18+90)
			{
				value += ((auto_roll ? 6 : randint(6)) + 2);
			}
			else if (value < 18+100)
			{
				value++;
			}
		}
	}

	/* Return the result */
	return (value);
}




/*
 * Roll for a characters stats
 *
 * For efficiency, we include a chunk of "calc_bonuses()".
 */
static void get_stats(void)
{
	int		i, j;

	int		bonus;

	int		dice[18];


	/* Roll and verify some stats */
	while (TRUE)
	{
		/* Roll some dice */
		for (j = i = 0; i < 18; i++)
		{
			/* Roll the dice */
			dice[i] = randint(3 + i % 3);

			/* Collect the maximum */
			j += dice[i];
		}

		/* Verify totals */
		if ((j > 42) && (j < 60)) break;
		/* 57 was 54... I hate 'magic numbers' :< TY */
		/* 60 was 54... I love being a munckin :) - Gumby */
	}

	/* Acquire the stats */
	for (i = 0; i < 6; i++)
	{
		/* Extract 5 + 1d3 + 1d4 + 1d5 */
		j = 5 + dice[3*i] + dice[3*i+1] + dice[3*i+2];

		/* Save that value */
		p_ptr->stat_max[i] = j;

		/* Obtain a "bonus" for "race" and "class" */
		bonus = rp_ptr->r_adj[i] + cp_ptr->c_adj[i];

		/* Variable stat maxes */
		if (p_ptr->maximize)
		{
			/* Start fully healed */
			p_ptr->stat_cur[i] = p_ptr->stat_max[i];

			/* Efficiency -- Apply the racial/class bonuses */
			stat_use[i] = modify_stat_value(p_ptr->stat_max[i], bonus);
		}

		/* Fixed stat maxes */
		else
		{
			/* Apply the bonus to the stat (somewhat randomly) */
			stat_use[i] = adjust_stat(p_ptr->stat_max[i], bonus, FALSE);

			/* Save the resulting stat maximum */
			p_ptr->stat_cur[i] = p_ptr->stat_max[i] = stat_use[i];
		}
	}
}


/*
 * Roll for some info that the auto-roller ignores
 */
static void get_extra(void)
{
	int i, j, min_value, max_value;
#ifdef SHOW_LIFE_RATE
	int percent;
#endif

	/* Level one */
	p_ptr->max_plv = p_ptr->lev = 1;

	/* Experience factor */
	p_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

	/* Set number of Beastmaster pets to 0 */
	p_ptr->number_pets = 0;

	/* Hitdice */
	p_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp;

	/* Initial hitpoints */
	p_ptr->mhp = p_ptr->hitdie;

	/* Minimum hitpoints at highest level */
	min_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 3) / 8;
	min_value += PY_MAX_LEVEL;

	/* Maximum hitpoints at highest level */
	max_value = (PY_MAX_LEVEL * (p_ptr->hitdie - 1) * 5) / 8;
	max_value += PY_MAX_LEVEL;

	/* Pre-calculate level 1 hitdice */
	player_hp[0] = p_ptr->hitdie;

	/* Roll out the hitpoints */
	while (TRUE)
	{
		/* Roll the hitpoint values */
		for (i = 1; i < PY_MAX_LEVEL; i++)
		{
			j = randint(p_ptr->hitdie);
			player_hp[i] = player_hp[i-1] + j;
		}

		/* XXX Could also require acceptable "mid-level" hitpoints */

		/* Require "valid" hitpoints at highest level */
		if (player_hp[PY_MAX_LEVEL-1] < min_value) continue;
		if (player_hp[PY_MAX_LEVEL-1] > max_value) continue;

		/* Acceptable */
		break;
	}

#ifdef SHOW_LIFE_RATE
	percent = (int)(((long)player_hp[PY_MAX_LEVEL - 1] * 200L) /
	(p_ptr->hitdie + ((PY_MAX_LEVEL - 1) * p_ptr->hitdie)));

	msg_format("Current Life Rating is %d/100.", percent);
	msg_print(NULL);
#endif /* SHOW_LIFE_RATE */

	if (quick_start)
	{
		p_ptr->max_plv = p_ptr->lev = 5;
		p_ptr->max_exp = p_ptr->exp = 1 + (player_exp[p_ptr->lev - 2] * p_ptr->expfact / 100L);
	}
}


/*
 * Get the racial history, and social class, using the "history charts".
 */
static void get_history(void)
{
	int i, n, chart, roll, social_class;
	char *s, *t;
	char buf[240];

	/* Clear the previous history strings */
	for (i = 0; i < 4; i++) history[i][0] = '\0';

	/* Clear the history text */
	buf[0] = '\0';

	/* Initial social class */
	social_class = randint(4);

	/* Starting place */
	switch (p_ptr->prace)
	{
		case RACE_HUMAN:
		case RACE_BARBARIAN:
			chart = 1;	break;
		case RACE_ELF:
		case RACE_HIGH_ELF:
			chart = 7;	break;
		case RACE_HOBBIT:
			chart = 10;	break;
		case RACE_GNOME:
			chart = 13;	break;
		case RACE_DWARF:
			chart = 16;	break;
		case RACE_HALF_ORC:
			chart = 19;	break;
		case RACE_HALF_TROLL:
			chart = 22;	break;
		case RACE_GAMBOLT:
			chart = 30;	break;
		case RACE_HALF_GIANT:
			chart = 75;	break;
		case RACE_HALF_TITAN:
			chart = 76;	break;
		case RACE_KOBOLD:
			chart = 82;	break;
		case RACE_KLACKON:
			chart = 84;	break;
		case RACE_DRACONIAN:
			chart = 89;	break;
		case RACE_MIND_FLAYER:
			chart = 92;	break;
		case RACE_GOLEM:
			chart = 98;	break;
		case RACE_VAMPIRE:
			chart = 113;	break;
		case RACE_SPECTRE:
			chart = 118;	break;
		case RACE_BEASTMAN:
			chart = 129;	break;
		case RACE_YEEK:
			chart = 77;	break;
		case RACE_MELNIBONEAN:
			chart = 24;	break;
		case RACE_VADHAGH:
			chart = 4;	break;
		default:
			chart = 0;	break;
	}

	/* Process the history */
	while (chart)
	{
		/* Start over */
		i = 0;

		/* Roll for nobility */
		roll = randint(100);

		/* Access the proper entry in the table */
		while ((chart != bg[i].chart) || (roll > bg[i].roll)) i++;

		/* Acquire the textual history */
		(void)strcat(buf, bg[i].info);

		/* Add in the social class */
		social_class += (int)(bg[i].bonus) - 50;

		/* Enter the next chart */
		chart = bg[i].next;
	}

	/* Verify social class */
	if (social_class > 100) social_class = 100;
	else if (social_class < 1) social_class = 1;

	/* Save the social class */
	p_ptr->sc = social_class;

	/* Skip leading spaces */
	for (s = buf; *s == ' '; s++) /* loop */;

	/* Get apparent length */
	n = strlen(s);

	/* Kill trailing spaces */
	while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';

	/* Start at first line */
	i = 0;

	/* Collect the history */
	while (TRUE)
	{
		/* Extract remaining length */
		n = strlen(s);

		/* All done */
		if (n < 60)
		{
			/* Save one line of history */
			strcpy(history[i++], s);

			/* All done */
			break;
		}

		/* Find a reasonable break-point */
		for (n = 60; ((n > 0) && (s[n-1] != ' ')); n--) /* loop */;

		/* Save next location */
		t = s + n;

		/* Wipe trailing spaces */
		while ((n > 0) && (s[n-1] == ' ')) s[--n] = '\0';

		/* Save one line of history */
		strcpy(history[i++], s);

		/* Start next line */
		for (s = t; *s == ' '; s++) /* loop */;
	}
}


/*
 * Computes character's age, height, and weight
 */
static void get_ahw(void)
{
	/* Calculate the age */
	p_ptr->age = rp_ptr->b_age + randint(rp_ptr->m_age);

	/* Calculate the height/weight for males */
	if (p_ptr->psex == SEX_MALE)
	{
		p_ptr->ht = randnor(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
		p_ptr->wt = randnor(rp_ptr->m_b_wt, rp_ptr->m_m_wt);
	}

	/* Calculate the height/weight for females */
	else if (p_ptr->psex == SEX_FEMALE)
	{
		p_ptr->ht = randnor(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
		p_ptr->wt = randnor(rp_ptr->f_b_wt, rp_ptr->f_m_wt);
	}
}


/*
 * Get the player's starting money
 */
static void get_money(void)
{
	int i, gold;

	/* Social Class determines starting gold */
	gold = (p_ptr->sc * 6) + randint(100) + 300;

	/* Process the stats */
	for (i = 0; i < 6; i++)
	{
		/* Mega-Hack -- reduce gold for high stats */
		if (stat_use[i] >= 18+50) gold -= 300;
		else if (stat_use[i] >= 18+20) gold -= 200;
		else if (stat_use[i] > 18) gold -= 150;
		else gold -= (stat_use[i] - 8) * 10;
	}

	/* Minimum gold */
	if (gold < 99 + p_ptr->sc) gold = 99 + p_ptr->sc;

	if (quick_start) gold *= 10;

	/* Save the gold */
	p_ptr->au = gold;
}



/*
 * Display stat values, subset of "put_stats()"
 *
 * See 'display_player()' for basic method.
 */
static void birth_put_stats(void)
{
	int	i, p;
	byte	attr;
	char	buf[80];


	/* Put the stats (and percents) */
	for (i = 0; i < 6; i++)
	{
		/* Put the stat */
		cnv_stat(stat_use[i], buf);
		c_put_str(TERM_L_GREEN, buf, 2 + i, 66);

		/* Put the percent */
		if (stat_match[i])
		{
			p = 1000L * stat_match[i] / auto_round;
			attr = (p < 100) ? TERM_YELLOW : TERM_L_GREEN;
			sprintf(buf, "%3d.%d%%", p/10, p%10);
			c_put_str(attr, buf, 2 + i, 73);
		}

		/* Never happened */
		else
		{
			c_put_str(TERM_RED, "(NONE)", 2 + i, 73);
		}
	}
}


/*
 * Clear all the global "character" data
 */
static void player_wipe(void)
{
	int i;


	/* Hack -- zero the struct */
	WIPE(p_ptr, player_type);

	/* Wipe the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(history[i], "");
	}

	/* No weight */
	total_weight = 0;

	/* No items */
	inven_cnt = 0;
	equip_cnt = 0;

	/* Clear the inventory */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_wipe(&inventory[i]);
	}

	/* Start with no artifacts made yet */
	for (i = 0; i < MAX_A_IDX; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		a_ptr->cur_num = 0;
	}

	/* Initialize quests (Heino Vander Sanden and Jimmy De Laet) */
	initialise_quests();

	/* Reset the "objects" */
	for (i = 1; i < MAX_K_IDX; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Reset "tried" */
		k_ptr->tried = FALSE;

		/* Reset "aware" */
		k_ptr->aware = FALSE;
	}

	/* Reset the "monsters" */
	for (i = 1; i < MAX_R_IDX; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Hack -- Reset the counter */
		r_ptr->cur_num = 0;

		/* Hack -- Reset the max counter */
		r_ptr->max_num = 100;

		/* Hack -- Reset the max counter */
		if (r_ptr->flags1 & (RF1_UNIQUE)) r_ptr->max_num = 1;

		/* Clear player kills */
		r_ptr->r_pkills = 0;
	}

	/* Hack -- no ghosts */
	r_info[MAX_R_IDX-1].max_num = 0;

	/* Hack -- Well fed player */
	p_ptr->food = PY_FOOD_FULL - 1;

	/* Wipe the spells */
	spell_learned1 = spell_learned2 = 0L;
	spell_worked1 = spell_worked2 = 0L;
	spell_forgotten1 = spell_forgotten2 = 0L;
	for (i = 0; i < 64; i++) spell_order[i] = 99;

	/* Clear "cheat" options */
	cheat_peek = FALSE;
	cheat_hear = FALSE;
	cheat_room = FALSE;
	cheat_xtra = FALSE;
	cheat_know = FALSE;
	cheat_live = FALSE;

	/* Assume no winning game */
	total_winner = FALSE;

	/* Assume no panic save */
	panic_save = 0;

	/* Assume no cheating */
	noscore = 0;
}


/*
 * Each player starts out with a few items, given as tval/sval pairs.
 * In addition, he always has some food and a few torches.
 */

static byte player_init[MAX_CLASS][3][2] =
{
	{
		/* Warrior */
		{ TV_RING, SV_RING_RES_FEAR }, /* Warriors need it! */
		{ TV_SWORD, SV_LONG_SWORD },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL }
	},

	{
		/* Mage */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_SWORD, SV_DAGGER },
		{ TV_DEATH_BOOK, 0 } /* Hack: for realm2 book */
	},

	{
		/* Priest */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for Life / Death book */
		{ TV_HAFTED, SV_MACE },
		{ TV_DEATH_BOOK, 0 } /* Hack: for realm2 book */
	},

	{
		/* Rogue */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_SWORD, SV_DAGGER },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR }
	},

	{
		/* Ranger */
		{ TV_NATURE_BOOK, 0 },
		{ TV_SWORD, SV_SHORT_SWORD },
		{ TV_BOW, SV_SHORT_BOW }
	},

	{
		/* Paladin */
		{ TV_SORCERY_BOOK, 0 },
		{ TV_SWORD, SV_BROAD_SWORD },
		{ TV_HARD_ARMOR, SV_SCALE_MAIL }
	},

	{
		/* Warrior-Mage */
		{ TV_ARCANE_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_SWORD, SV_BROAD_SWORD },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR }
	},

	{
		/* Chaos Warrior */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: For realm1 book */
		{ TV_SWORD, SV_BROAD_SWORD },
		{ TV_HARD_ARMOR, SV_SCALE_MAIL }
	},

	{
		/* Monk */
		{ TV_SORCERY_BOOK, 0 },
		{ TV_POTION, SV_POTION_HEALING },
		{ TV_SOFT_ARMOR, SV_ROBE },
	},

	{
		/* Mindcrafter */
		{ TV_SWORD, SV_SHORT_SWORD },
		{ TV_POTION, SV_POTION_RESTORE_MANA },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
	},

	{
		/* High Mage */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_SWORD, SV_DAGGER },
		{ TV_RING, SV_RING_SUSTAIN_INT}
	},

	{
		/*
		 * Weaponmaster - they also get a weapon appropriate to
		 * their specialty.  See below. -- Gumby
		 */
		{ TV_RING, SV_RING_RES_FEAR },
		{ TV_POTION, SV_POTION_HEROISM },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL }
	},

	{
		/* Archer - they also get some pebbles. -- Gumby */
		{ TV_RING, SV_RING_RES_FEAR },
		{ TV_BOW, SV_SLING },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR }
	},

	{
		/* Beastmaster */
		{TV_POTION, SV_POTION_BESERK_STRENGTH},
		{TV_SWORD, SV_BROAD_SWORD},
		{TV_HARD_ARMOR, SV_CHAIN_MAIL}
	},
};


/*
 * Init players with some belongings
 *
 * Having an item makes the player "aware" of its purpose.
 */
static void player_outfit(void)
{
	int		i, tv, sv;
	object_type	forge;
	object_type	*q_ptr;
	ego_item_type	*e_ptr;
	
	/* Get local object */
	q_ptr = &forge;

	if ((p_ptr->prace >= RACE_GOLEM) && (p_ptr->prace <= RACE_SPECTRE))
	{
		/* Hack -- Give the player scrolls of satisfy hunger */
		object_prep(q_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_SATISFY_HUNGER));
		q_ptr->number = rand_range(5, 8);
		object_aware(q_ptr);
		object_known(q_ptr);

		/* These objects are "storebought" */
		q_ptr->ident |= IDENT_STOREB;

		(void)inven_carry(q_ptr, FALSE);
	}
	else
	{
		/* Hack -- Give the player some food */
		object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
		q_ptr->number = rand_range(5, 8);
		if (p_ptr->astral) q_ptr->number += 5;
		object_aware(q_ptr);
		object_known(q_ptr);
		(void)inven_carry(q_ptr, FALSE);
	}

	/* Start off with lots of ID scrolls if a ghost... -- Gumby */
	if (p_ptr->astral)
	{
		/* Get local object */
		q_ptr = &forge;

		object_prep(q_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_IDENTIFY));
		q_ptr->number = 75;
		object_aware(q_ptr);
		object_known(q_ptr);
		(void)inven_carry(q_ptr, FALSE);
	}

	/* Start off With a wand of Magic Missiles if you're a High-Mage
	 * who chose Sorcery. -- Gumby
	 */
	if ((p_ptr->pclass == CLASS_HIGH_MAGE) && (p_ptr->realm1 == REALM_SORCERY))
	{
		q_ptr = &forge;

		object_prep(q_ptr, lookup_kind(TV_WAND, SV_WAND_MAGIC_MISSILE));
		q_ptr->pval = 10 + randint(10);
		object_aware(q_ptr);
		object_known(q_ptr);
		(void)inven_carry(q_ptr, FALSE);
	}

	/* Gotta give Weaponmasters a weapon they can use! -- Gumby */
	if (p_ptr->pclass == CLASS_WEAPONMASTER)
	{
		q_ptr = &forge;

		switch (p_ptr->wm_choice)
		{
			case TV_HAFTED:
				object_prep(q_ptr, lookup_kind(TV_HAFTED, SV_MACE));
				break;
			case TV_POLEARM:
				object_prep(q_ptr, lookup_kind(TV_POLEARM, SV_PIKE));
				break;
			case TV_AXE:
				object_prep(q_ptr, lookup_kind(TV_AXE, SV_LIGHT_WAR_AXE));
				break;
			case TV_SWORD:
				object_prep(q_ptr, lookup_kind(TV_SWORD, SV_BROAD_SWORD));
				break;
		}

		q_ptr->number = 1;

		/* Get a quick start with an Elemental Brand or Slay. - G */
		if (quick_start)
		{
			if (randint(2)==1)
				/* a basic Brand or Slay Elemental */
				q_ptr->name2 = rand_range(71,75);
			else
				/* a basic Slay */
				q_ptr->name2 = rand_range(80,87);

			/* Give it a few plusses */
			q_ptr->to_h += 1 + randint(4);
			q_ptr->to_d += 1 + randint(2);

			/* Determine its pval, if any */
			e_ptr = &e_info[q_ptr->name2];

			if (e_ptr->max_pval) q_ptr->pval = randint(e_ptr->max_pval);

			if (e_ptr->flags2 & (TR2_RAND_SUSTAIN))
				q_ptr->xtra1 = EGO_XTRA_SUSTAIN;

			if (e_ptr->flags2 & (TR2_RAND_ABILITY))
				q_ptr->xtra1 = EGO_XTRA_ABILITY;

			if (e_ptr->flags2 & (TR2_RAND_RESIST))
				q_ptr->xtra1 = EGO_XTRA_POWER;

			/* Randomize the "xtra" power */
			if (q_ptr->xtra1) q_ptr->xtra2 = randint(256);

			/* You know its properties */
			q_ptr->ident |= IDENT_MENTAL;
		}

		object_aware(q_ptr);
		object_known(q_ptr);

		/* These objects are "storebought" */
		q_ptr->ident |= IDENT_STOREB;

		(void)inven_carry(q_ptr, FALSE);
	}


	/* Get local object */
	q_ptr = &forge;

	/* Start off with a bunch of rounded pebbles if an Archer -- Gumby */
	if (p_ptr->pclass == CLASS_ARCHER)
	{
		object_prep(q_ptr, lookup_kind(TV_SHOT, SV_AMMO_LIGHT));
		q_ptr->number = 20;
		object_aware(q_ptr);
		object_known(q_ptr);

		if (quick_start)
		{
			/* Plusses only */
			q_ptr->to_h += 2 + randint(3);
			q_ptr->to_d += 2 + randint(3);

			/* You know its properties */
			q_ptr->ident |= IDENT_MENTAL;
		}

		/* They are "storebought" */
		q_ptr->ident |= IDENT_STOREB;

		(void)inven_carry(q_ptr, FALSE);
	}


	/* Get local object */
	q_ptr = &forge;

	if (p_ptr->prace == RACE_VAMPIRE)
	{
		/* Hack -- Give the player scrolls of Darkness. They used to
		 * get scrolls of Light as well, for some reason, but I felt
		 * that that was just plain wrong. - Gumby
		 */
		object_prep(q_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_DARKNESS));
		q_ptr->number = rand_range(5,8);
		object_aware(q_ptr);
		object_known(q_ptr);

		/* These objects are "storebought" */
		q_ptr->ident |= IDENT_STOREB;

		(void)inven_carry(q_ptr, FALSE);
	}
	else if (!p_ptr->astral)
	{
		/* Hack -- Give the player some torches */
		object_prep(q_ptr, lookup_kind(TV_LITE, SV_LITE_TORCH));
		q_ptr->number = rand_range(4, 8);
		q_ptr->pval = 2000;
		object_aware(q_ptr);
		object_known(q_ptr);
		(void)inven_carry(q_ptr, FALSE);
	}


	/* Hack -- Give the player three useful objects */
	for (i = 0; i < 3; i++)
	{
		/* Look up standard equipment */
		tv = player_init[p_ptr->pclass][i][0];
		sv = player_init[p_ptr->pclass][i][1];

		/* Hack to initialize spellbooks */
		if (tv  == TV_SORCERY_BOOK)
		{
			tv = TV_LIFE_BOOK + p_ptr->realm1 - 1;
		}
		else if (tv == TV_DEATH_BOOK)
		{
			tv = TV_LIFE_BOOK + p_ptr->realm2 - 1;
		}
		/* Barbarians have no need for a Ring of Resist Fear */
		else if ((tv == TV_RING) && (sv == SV_RING_RES_FEAR) &&
			 (p_ptr->prace == RACE_BARBARIAN))
		{
			sv = SV_RING_SUSTAIN_STR;
		}
		/* Mind Flayers have no need for rings of Sustain Int - G */
		else if ((tv == TV_RING) && (sv == SV_RING_SUSTAIN_INT) &&
			 (p_ptr->prace == RACE_MIND_FLAYER))
		{
			sv = SV_RING_FEATHER_FALL;
		}

		/* Get local object */
		q_ptr = &forge;

		/* Hack -- Give the player an object */
		object_prep(q_ptr, lookup_kind(tv, sv));

		/*
		 * Give players minor ego-weapons and armour to start, with
		 * the weapon (a minor Slaying or Branded weapon) being
		 * based on magical realm, if any. -- Gumby
		 */
		if (quick_start)
		{
			if ((tv >= TV_HAFTED) && (tv <= TV_SWORD))
			{
				switch (p_ptr->realm1)
				{
					case REALM_LIFE:
						if (randint(5)==1)
							q_ptr->name2 = EGO_BRAND_COLD;
						else
							q_ptr->name2 = EGO_SLAY_EVIL;
						break;
					case REALM_NATURE:
						if (randint(4)==1)
							q_ptr->name2 = EGO_BRAND_ELEC;
						else
							q_ptr->name2 = EGO_SLAY_ANIMAL;
						break;
					case REALM_CHAOS:
						if (randint(4)==1)
							q_ptr->name2 = EGO_BRAND_FIRE;
						else
							q_ptr->name2 = EGO_SLAY_DEMON;
						break;
					case REALM_DEATH:
						if (randint(5)==1)
							q_ptr->name2 = EGO_BRAND_POIS;
						else
							q_ptr->name2 = EGO_SLAY_UNDEAD;
						break;
					case REALM_TRUMP:
						if (randint(4)==1)
							/* a basic Brand */
							q_ptr->name2 = rand_range(72,75);
						else
						{	/* a Trump weapon */
							q_ptr->name2 = EGO_TRUMP;
						}
						break;
					default:
						if (randint(2)==1)
							/* a basic Brand */
							q_ptr->name2 = rand_range(71,75);
						else
							/* a basic Slay */
							q_ptr->name2 = rand_range(80,87);
						break;
				}

				/* Give it some plusses */
				q_ptr->to_h += 1 + randint(4);
				q_ptr->to_d += 1 + randint(2);

				/* Determine its pval, if any */
				e_ptr = &e_info[q_ptr->name2];

				if (e_ptr->max_pval) q_ptr->pval = randint(e_ptr->max_pval);

				if (e_ptr->flags2 & (TR2_RAND_SUSTAIN))
					q_ptr->xtra1 = EGO_XTRA_SUSTAIN;

				if (e_ptr->flags2 & (TR2_RAND_ABILITY))
					q_ptr->xtra1 = EGO_XTRA_ABILITY;

				if (e_ptr->flags2 & (TR2_RAND_RESIST))
					q_ptr->xtra1 = EGO_XTRA_POWER;

				/* Randomize the "xtra" power */
				if (q_ptr->xtra1) q_ptr->xtra2 = randint(256);

			}

			if ((tv >= TV_SOFT_ARMOR) && (tv <= TV_HARD_ARMOR))
			{
				/* Resist Lightning, Cold or Fire */
				q_ptr->name2 = rand_range(5,7);

				/* A few plusses */
				q_ptr->to_a += 2 + randint(3);
			}

			if (tv == TV_BOW)
			{
				/* Plusses only */
				q_ptr->to_h += 1 + randint(5);
				q_ptr->to_d += 1 + randint(2);
			}

			/* You know these items */
			q_ptr->ident |= IDENT_MENTAL;
		}

		/* These objects are "storebought" */
		q_ptr->ident |= IDENT_STOREB;

		object_aware(q_ptr);
		object_known(q_ptr);
		(void)inven_carry(q_ptr, FALSE);
	}
}


/*
 * Helper function for 'player_birth()'
 *
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 */
static bool player_birth_aux()
{
	int i, j, k, m, n, v;
	int mode = 0;
	bool flag = FALSE;
	bool prev = FALSE;
	cptr str;
	char c;
#if 0
	char p1 = '(';
#endif
	char p2 = ')';
	char b1 = '[';
	char b2 = ']';
	char buf[80];
	char inp[80];

	bool autoroll = FALSE;
	bool point_mod = FALSE;


	/*** Intro ***/

	/* Clear screen */
	Term_clear();

	/* Title everything */
	put_str("Name        :", 2, 1);
	put_str("Sex         :", 3, 1);
	put_str("Race        :", 4, 1);
	put_str("Class       :", 5, 1);


	/*** Instructions ***/

	/* Display some helpful information */
	Term_putstr(5, 10, -1, TERM_WHITE,
		"Please answer the following questions.  Most of the questions");
	Term_putstr(5, 11, -1, TERM_WHITE,
		"display a set of standard answers, and many will also accept");
	Term_putstr(5, 12, -1, TERM_WHITE,
		"some special responses, including 'Q' to quit, 'S' to restart,");
	Term_putstr(5, 13, -1, TERM_WHITE,
		"'?' for help, and '*' for a random choice.");


	/*** Player sex ***/

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"Your 'sex' does not have any significant gameplay effects.");

	/* Prompt for "Sex" */
	for (n = 0; n < MAX_SEXES; n++)
	{
		/* Analyze */
		p_ptr->psex = n;
		sp_ptr = &sex_info[p_ptr->psex];
		str = sp_ptr->title;

		/* Display */
		sprintf(buf, "%c%c %s", I2A(n), p2, str);
		put_str(buf, 21 + (n/5), 2 + 15 * (n%5));
	}

	/* Choose */
	while (1)
	{
		sprintf(buf, "Choose a sex (%c-%c, *): ", I2A(0), I2A(n-1));
		put_str(buf, 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == '*')
		{
			k = rand_int(MAX_SEXES);
		}
		else
		{
			k = (islower(c) ? A2I(c) : -1);
		}
		if ((k >= 0) && (k < n)) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}

	/* Set sex */
	p_ptr->psex = k;
	sp_ptr = &sex_info[p_ptr->psex];
	str = sp_ptr->title;

	/* Display */
	c_put_str(TERM_L_BLUE, str, 3, 15);

	/* Clean up */
	clear_from(15);


	/*** Player race ***/

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"Your 'race' determines various intrinsic factors and bonuses.");
	hack_mutation = FALSE;

	/* Dump races */
	for (n = 0; n < MAX_RACES; n++)
	{
		/* Analyze */
		p_ptr->prace = n;
		rp_ptr = &race_info[p_ptr->prace];
		str = rp_ptr->title;
		
		/* Display */
		sprintf(buf, "%c%c %s", I2A(n), p2, str);
		put_str(buf, 18 + (n/5), 2 + 15 * (n%5));
	}

	/* Choose */
	while (1)
	{
	        sprintf(buf, "Choose a race (%c-%c, *): ", I2A(0), I2A(n-1));
	        put_str(buf, 17, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == '*')
	        {
			k = rand_int(MAX_RACES);
			if (k == RACE_BEASTMAN)
			{
				hack_mutation = TRUE;
			}
		}
		else
		{
	        	k = (islower(c) ? A2I(c) : -1);
			if (k == RACE_BEASTMAN)
			{
				hack_mutation = TRUE;
			}
		}
		if ((k >= 0) && (k < n)) break;
	       	if (c == '?') do_cmd_help("help.hlp");
	       	else bell();
	}

	/* Set race */
	p_ptr->prace = k;
	rp_ptr = &race_info[p_ptr->prace];
	str = rp_ptr->title;

	/* Display */
	c_put_str(TERM_L_BLUE, str, 4, 15);

	/* Get a random name now we have a race*/
	create_random_name(p_ptr->prace, player_name);

	/* Display */
	c_put_str(TERM_L_BLUE, player_name, 2, 15);

	/* Clean up */
	clear_from(15);


	/*** Player class ***/

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"Your 'class' determines various intrinsic abilities and bonuses.");
	Term_putstr(5, 16, -1, TERM_WHITE,
        "Any entries in parentheses should only be used by advanced players.");

	/* Dump classes */
	for (n = 0; n < MAX_CLASS; n++)
	{
		cptr mod = "";

		/* Analyze */
		p_ptr->pclass = n;
		cp_ptr = &class_info[p_ptr->pclass];
		mp_ptr = &magic_info[p_ptr->pclass];
		str = cp_ptr->title;

#if 0
        /* Verify legality */
        if (!(rp_ptr->choice & (1L << n))) mod = " (*)";
#endif

    if (!(rp_ptr->choice & (1L << n )))
        sprintf(buf, "%c%c (%s)%s", I2A(n), p2, str, mod);
    else
		/* Display */
		sprintf(buf, "%c%c %s%s", I2A(n), p2, str, mod);

        put_str(buf, 19 + (n/3), 2 + 20 * (n%3));
	}

	/* Get a class */
	while (1)
	{
		sprintf(buf, "Choose a class (%c-%c, *): ", I2A(0), I2A(n-1));
	        put_str(buf, 18, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == '*')
		{
			k = rand_int(MAX_CLASS);
		}
		else
		{
			k = (islower(c) ? A2I(c) : -1);
		}
		if ((k >= 0) && (k < n)) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}

	/* Set class */
	p_ptr->pclass = k;
	cp_ptr = &class_info[p_ptr->pclass];
	mp_ptr = &magic_info[p_ptr->pclass];
	str = cp_ptr->title;

	/* Display */
	c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 15);

	/* Clean up */

	clear_from(15);

	get_realms();

        if (p_ptr->realm1 || p_ptr->realm2)
          put_str("Magic       :", 6, 1);
        if (p_ptr->realm1)
          c_put_str(TERM_L_BLUE, realm_names[p_ptr->realm1],6,15);
        if (p_ptr->realm2)
          c_put_str(TERM_L_BLUE, realm_names[p_ptr->realm2],7,15);

	/*** Maximize mode ***/

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"Using 'maximize' mode makes the game harder at the start,");
	Term_putstr(5, 16, -1, TERM_WHITE,
        "but often makes it easier to win. In Gumband, 'maximize'");
    Term_putstr(5, 17, -1, TERM_WHITE,
        "mode is recommended for spellcasters.");

	/* Ask about "maximize" mode */
	while (1)
	{
		put_str("Use 'maximize' mode? (y/n) ", 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == ESCAPE) break;
		if ((c == 'y') || (c == 'n')) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}

	/* Set "maximize" mode */
	p_ptr->maximize = (c == 'y');

	/* Clear */
	clear_from(15);


	/*** Preserve mode ***/

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"Using 'preserve' mode makes it difficult to 'lose' artifacts,");
	Term_putstr(5, 16, -1, TERM_WHITE,
		"but eliminates the 'special' feelings about some levels.");

	/* Ask about "preserve" mode */
	while (1)
	{
		put_str("Use 'preserve' mode? (y/n) ", 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == ESCAPE) break;
		if ((c == 'y') || (c == 'n')) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}

	/* Set "preserve" mode */
	p_ptr->preserve = (c == 'y');

	/* Clear */
	clear_from(20);


#ifdef ALLOW_AUTOROLLER

	/*** Autoroll ***/

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"The 'autoroller' allows you to specify certain 'minimal' stats,");
	Term_putstr(5, 16, -1, TERM_WHITE,
		"but be warned that your various stats may not be independant!");
	Term_putstr(5, 17, -1, TERM_WHITE,
		"Point-based generation lets you spend points to modify your");
	Term_putstr(5, 18, -1, TERM_WHITE,
		"statistics instead of determining them randomly.");

	/* Ask about "auto-roller" mode */
	while (1)
	{
		put_str("Generate with (a)utoroller, (p)oint-based, or (n)either? (a/p/n) ", 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == ESCAPE) break;
		if ((c == 'a') || (c == 'p') || (c == 'n')) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}

	/* Set "autoroll" */
	autoroll = (c == 'a');
	point_mod = (c == 'p');

	/* Clear */
	clear_from(15);


	/* Initialize */
	if (autoroll)
	{
		int mval[6];


		/* Clear fields */
		auto_round = 0L;
		last_round = 0L;

		/* Clean up */
		clear_from(10);

		/* Prompt for the minimum stats */
		put_str("Enter minimum attribute for: ", 15, 2);

		/* Output the maximum stats */
		for (i = 0; i < 6; i++)
		{
			/* Reset the "success" counter */
			stat_match[i] = 0;

			/* Race/Class bonus */
			j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i];

			/* Obtain the "maximal" stat */
			m = adjust_stat(17, j, TRUE);

			/* Save the maximum */
			mval[i] = m;

			/* Extract a textual format */
			/* cnv_stat(m, inp); */

			/* Above 18 */
			if (m > 18)
			{
				sprintf(inp, "(Max of 18/%02d):", (m - 18));
			}

			/* From 3 to 18 */
			else
			{
				sprintf(inp, "(Max of %2d):", m);
			}

			/* Prepare a prompt */
			sprintf(buf, "%-5s%-20s", stat_names[i], inp);

			/* Dump the prompt */
			put_str(buf, 16 + i, 5);
		}

		/* Input the minimum stats */
		for (i = 0; i < 6; i++)
		{
			/* Get a minimum stat */
			while (TRUE)
			{
				char *s;

				/* Move the cursor */
				put_str("", 16 + i, 30);

				/* Default */
				strcpy(inp, "");

				/* Get a response (or escape) */
				if (!askfor_aux(inp, 8)) inp[0] = '\0';

				/* Hack -- add a fake slash */
				strcat(inp, "/");

				/* Hack -- look for the "slash" */
				s = strchr(inp, '/');

				/* Hack -- Nuke the slash */
				*s++ = '\0';

				/* Hack -- Extract an input */
				v = atoi(inp) + atoi(s);

				/* Break on valid input */
				if (v <= mval[i]) break;
			}

			/* Save the minimum stat */
			stat_limit[i] = (v > 0) ? v : 0;
		}
	}

#endif /* ALLOW_AUTOROLLER */

	/* Clean up */
	clear_from(10);

	/*** User enters number of quests ***/
	/* Heino Vander Sanden and Jimmy De Laet */

	/* Extra info */
	Term_putstr(5, 15, -1, TERM_WHITE,
		"You can input yourself the number of quest you'd like to");
	Term_putstr(5, 16, -1, TERM_WHITE,
		"perform next to three obligatory ones (Arioch, Xiombarg and Mabelrode)");
	Term_putstr(5, 17, -1, TERM_WHITE,
		"In case you do not want any additional quest, just enter 0");

	/* Ask the number of additional quests */
	while (1)
	{
		put_str(format("Number of additional quest? (<%u) ", MAX_QUESTS + 1), 20, 2);

		/* Get a the number of additional quest */
		while (TRUE)
		{
			/* Move the cursor */
			put_str("", 20, 37);

			/* Default */
			strcpy(inp, "40");

			/* Get a response (or escape) */
			if (!askfor_aux(inp, 2)) inp[0] = '\0';
			v = atoi(inp);

			/* Break on valid input */
			if ( (v <= MAX_QUESTS) && ( v >= 0 )) break;
		}
		break;
	}

	/* Set maxnumber of quest */
	MAX_Q_IDX = v + DEFAULT_QUESTS;

	/* Clear */
	clear_from(15);

	/* Generate quests */
	player_birth_quests();


	/*** Quick Start -- Gumby ***/

	/* Extra info */
	Term_putstr(5, 13, -1, TERM_WHITE,
		"A quick start gives the character starting equipment that");
	Term_putstr(5, 14, -1, TERM_WHITE,
		"is of better quality than they would otherwise get.  This");
	Term_putstr(5, 15, -1, TERM_WHITE,
		"will make the early part of the game much easier.");
	Term_putstr(5, 17, -1, TERM_L_RED,
		"Using this option will cut your final score in half.");

	/* Ask about quick start */
	while (1)
	{
		put_str("Quick Start? (y/n) ", 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == ESCAPE) break;
		if ((c == 'y') || (c == 'n')) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}

	/* Set quick start */
	if (c == 'y')
	{
		quick_start = TRUE;
	}
	else
	{
		quick_start = FALSE;
	}

	/* Clear */
	clear_from(13);


	/*** Ghostly Status -- Gumby ***/

	/* Extra info */
	Term_putstr(5, 13, -1, TERM_WHITE,
		"Starting as an astral being makes you begin the game on");
	Term_putstr(5, 14, -1, TERM_WHITE,
		"dungeon level 96.  You must make your way from there to the");
	Term_putstr(5, 15, -1, TERM_WHITE,
		"town on foot, where you will finally regain your corporeal");
	Term_putstr(5, 16, -1, TERM_WHITE,
		"form.  You will then have to make your way back down to confront");
	Term_putstr(5, 17, -1, TERM_WHITE,
		"the Swords Rulers to win the game.");

	while (1)
	{
		put_str("Start as an astral being? (y/n) ", 20, 2);
		c = inkey();
		if (c == 'Q') quit(NULL);
		if (c == 'S') return (FALSE);
		if (c == ESCAPE) break;
		if ((c == 'y') || (c == 'n')) break;
		if (c == '?') do_cmd_help("help.hlp");
		else bell();
	}

	/* Set "ghost" mode */
	if (c == 'y')
	{
		p_ptr->astral = TRUE;
		p_ptr->was_astral = FALSE;
		p_ptr->astral_start = TRUE;
	}
	else
	{
		p_ptr->astral = FALSE;
		p_ptr->was_astral = FALSE;
		p_ptr->astral_start = FALSE;
	}

	/* Clear */
	clear_from(13);


	/*** Generate ***/

	/* Roll */
	while (TRUE)
	{
		/* Feedback */
		if (autoroll)
		{
			Term_clear();

			put_str("Name        :", 2, 1);
			put_str("Sex         :", 3, 1);
			put_str("Race        :", 4, 1);
			put_str("Class       :", 5, 1);

			c_put_str(TERM_L_BLUE, player_name, 2, 15);
			c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 15);
			c_put_str(TERM_L_BLUE, rp_ptr->title, 4, 15);
			c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 15);

			/* Label stats */
			put_str("STR:", 2 + A_STR, 61);
			put_str("INT:", 2 + A_INT, 61);
			put_str("WIS:", 2 + A_WIS, 61);
			put_str("DEX:", 2 + A_DEX, 61);
			put_str("CON:", 2 + A_CON, 61);
			put_str("CHR:", 2 + A_CHR, 61);

			/* Note when we started */
			last_round = auto_round;

			/* Indicate the state */
			put_str("(Hit ESC to abort)", 11, 61);

			/* Label count */
			put_str("Round:", 9, 61);
		}

		/* Otherwise just get a character */
		else
		{
			/* Get a new character */
			if (point_mod)
			{
				for (i = 0; i < 6; i++)
				{
					p_ptr->stat_cur[i] = p_ptr->stat_max[i] = 8;
				}
				point_mod_player();  
			}
			else
			{
				get_stats();
			}
		}

		/* Auto-roll */
		while (autoroll)
		{
			bool accept = TRUE;

			/* Get a new character */
			get_stats();

			/* Advance the round */
			auto_round++;

			/* Hack -- Prevent overflow */
			if (auto_round >= 500000L) break;

			/* Check and count acceptable stats */
			for (i = 0; i < 6; i++)
			{
				/* This stat is okay */
				if (stat_use[i] >= stat_limit[i])
				{
					stat_match[i]++;
				}

				/* This stat is not okay */
				else
				{
					accept = FALSE;
				}
			}

			/* Break if "happy" */
			if (accept) break;

			/* Take note every 25 rolls */
			flag = (!(auto_round % AUTOROLLER_STEP));

			/* Update display occasionally */
			if (flag || (auto_round < last_round + 100))
			{
				/* Dump data */
				birth_put_stats();

				/* Dump round */
				put_str(format("%6ld", auto_round), 9, 73);

				/* Make sure they see everything */
				Term_fresh();
#ifndef FAST_AUTOROLLER
				/* Delay 1/10 second */
				if (flag) Term_xtra(TERM_XTRA_DELAY, 100);
#endif
				/* Do not wait for a key */
				inkey_scan = TRUE;

				/* Check for a keypress */
				if (inkey()) break;
			}
		}

		/* Flush input */
		flush();


		/*** Display ***/

		/* Mode */
		mode = 0;

		/* Roll for base hitpoints */
		get_extra();

		/* Roll for age/height/weight */
		get_ahw();

		/* Roll for social class */
		get_history();

		/* Roll for gold */
		get_money();

		/* Prevent quick-starting, point-based characters from
		 * having 10000+ gold :) -- Gumby
		 */
		if (point_mod && quick_start) p_ptr->au /= 10;

		/* Choose specialty for Weaponmasters -- Gumby
		 * Considered putting probabilities in here, but I think
		 * I'll keep it simple and trust people not to reroll the
		 * character just because they didn't get Swords.
		 */
		if (p_ptr->pclass == CLASS_WEAPONMASTER)
		{
			p_ptr->wm_choice = rand_range(TV_HAFTED, TV_SWORD);
		}

		/* Hack -- get a chaos patron even if you are not a chaos
		 * warrior.
		 *
		 * Melniboneans have been pledged *ONLY* to Arioch
		 * throughout their history. -- Gumby
		 */
		if (p_ptr->prace == RACE_MELNIBONEAN)
		{
			p_ptr->chaos_patron = PATRON_ARIOCH;
		}
		else
		{
			p_ptr->chaos_patron = (randint(MAX_PATRON)) - 1;
		}

		p_ptr->muta1 = 0;
		p_ptr->muta2 = 0;
		p_ptr->muta3 = 0;

		/* Input loop */
		while (TRUE)
		{
			/* Calculate the bonuses and hitpoints and mana */
			p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA);

			/* Update stuff */
			update_stuff();

			/* Fully healed */
			p_ptr->chp = p_ptr->mhp;

			/* Fully rested */
			p_ptr->csp = p_ptr->msp;

			/* Display the player */
			display_player(mode);

			/* Prepare a prompt (must squeeze everything in) */
			Term_gotoxy(2, 23);
			Term_addch(TERM_WHITE, b1);
			Term_addstr(-1, TERM_WHITE, "'r' to reroll");
			if (prev) Term_addstr(-1, TERM_WHITE, ", 'p' for prev");
			if (mode) Term_addstr(-1, TERM_WHITE, ", 'h' for Misc.");
			else Term_addstr(-1, TERM_WHITE, ", 'h' for History");
			Term_addstr(-1, TERM_WHITE, ", or ESC to accept");
			Term_addch(TERM_WHITE, b2);

			/* Prompt and get a command */
			c = inkey();

			/* Quit */
			if (c == 'Q') quit(NULL);

			/* Start over */
		    if (c == 'S') return (FALSE);

			/* Escape accepts the roll */
			if (c == ESCAPE) break;

			/* Reroll this character */
			if ((c == ' ') || (c == 'r')) break;

			/* Previous character */
			if (prev && (c == 'p'))
			{
				load_prev_data();
				continue;
			}

			/* Toggle the display */
			if ((c == 'H') || (c == 'h'))
			{
				mode = ((mode != 0) ? 0 : 1);
				continue;
			}

			/* Help */
			if (c == '?')
			{
				do_cmd_help("help.hlp");
				continue;
			}

			/* Warning */
			bell();
		}

		/* Are we done? */
		if (c == ESCAPE) break;

		/* Save this for the "previous" character */
		save_prev_data();

		/* Note that a previous roll exists */
		prev = TRUE;
	}

	/* Clear prompt */
	clear_from(23);


	/*** Finish up ***/

	/* Get a name, recolor it, prepare savefile */

	get_name();


	/* Prompt for it */
	prt("['Q' to suicide, 'S' to start over, or ESC to continue]", 23, 10);

	/* Get a key */
	c = inkey();

	/* Quit */
	if (c == 'Q') quit(NULL);

	/* Start over */
	if (c == 'S') return (FALSE);

	/* Check for patron-granted powers */
	if (p_ptr->pclass == CLASS_CHAOS_WARRIOR)
	{
		switch(p_ptr->chaos_patron)
		{
			/* Resist magic */
			case PATRON_SLORTAR:
			{
				p_ptr->muta3 |= MUT3_MAGIC_RES;
				break;
			}
			/* Mabelrode the Faceless */
			case PATRON_MABELRODE:
			{
				if (randint(2) == 1)
					p_ptr->muta3 |= MUT3_BLANK_FAC;
				else
					p_ptr->muta3 |= MUT3_ILL_NORM;
				break;
			}
			/* Random telepathy and warning, or extra legs:
			 * Pyaray's earthly form is that of a giant
			 * octopus, after all. :) - Gumby
			 */
			case PATRON_PYARAY:
			{
				if (randint(1000) == 7)
					p_ptr->muta3 |= MUT3_ESP;
				else if (randint(100) == 7)
					p_ptr->muta3 |= MUT3_XTRA_LEGS;
				else
					p_ptr->muta2 |= MUT2_WEIRD_MIND | MUT2_WARNING;
				break;
			}
			/* Detect curse */
			case PATRON_BALAAN:
			{
				if (randint(10) == 7)
					p_ptr->muta1 |= MUT1_DET_CURSE;
				else
					p_ptr->muta3 |= MUT3_XTRA_EYES;
				break;
			}
			/* Arioch misses Elric's loyalty :) */
			case PATRON_ARIOCH:
			{
				p_ptr->muta3 |= MUT3_ALBINO;
				break;
			}
			/* Random dispel all */
			case PATRON_EEQUOR:
			{
				p_ptr->muta2 |= MUT2_DISPEL_ALL;
				break;
			}
			/* Random mutation */
			case PATRON_BALO:
			{
				gain_random_mutation(0);
				break;
			}
			/* Random berserk */
			case PATRON_KHORNE:
			{
				p_ptr->muta2 |= MUT2_BERS_RAGE;
				break;
			}
			/* Random polymorph wounds */
			case PATRON_NURGLE:
			{
				p_ptr->muta2 |= MUT2_POLY_WOUND;
				break;
			}
			/* Augmented intelligence */
			case PATRON_TZEENTCH:
			{
				p_ptr->muta3 |= MUT3_HYPER_INT;
				break;
			}
			/* Random chaos */
			case PATRON_KHAINE:
			{
				p_ptr->muta2 |= MUT2_RAW_CHAOS;
				break;
			}
		}
	}

	/* Accept */
	return (TRUE);
}


/*
 * Create a new character.
 *
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 */
void player_birth(void)
{
	int i, n;

	/* Create a new character */
	while (1)
	{
		/* Wipe the player */
		player_wipe();

		/* Roll up a new character */
		if (player_birth_aux()) break;
	}


	/* Note player birth in the message recall */
	message_add(" ");
	message_add("  ");
	message_add("====================");
	message_add("  ");
	message_add(" ");


	/* Hack -- outfit the player */
	player_outfit();

	/* Shops */
	for (n = 0; n < MAX_STORES; n++)
	{
		/* Initialize */
		store_init(n);

		/* Ignore home */
		if (n == 7) continue;

		/* Maintain the shop (ten times) */
		for (i = 0; i < 10; i++) store_maint(n);
	}
}
