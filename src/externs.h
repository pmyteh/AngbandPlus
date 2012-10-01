/* File: externs.h */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/*
 * Note that some files have their own header files
 * (z-virt.h, z-util.h, z-form.h, term.h, random.h)
 */

/*
 * Automatically generated "variable" declarations
 */

/* tables.c */
extern s16b ddd[9];
extern s16b ddx[10];
extern s16b ddy[10];
extern s16b ddx_ddd[9];
extern s16b ddy_ddd[9];
extern char hexsym[16];
extern byte adj_str_th[A_RANGE];
extern byte adj_str_td[A_RANGE];
extern byte adj_str_wgt[A_RANGE];
extern byte adj_str_hold[A_RANGE];
extern byte adj_str_dig[A_RANGE];
extern byte adj_str_blow[A_RANGE];
extern byte adj_int_dev[A_RANGE];
extern byte adj_int_dis[A_RANGE];
extern byte adj_int_alc[A_RANGE];
extern byte adj_wis_sav[A_RANGE];
extern byte adj_dex_dis[A_RANGE];
extern byte adj_dex_ta[A_RANGE];
extern byte adj_dex_th[A_RANGE];
extern byte adj_dex_blow[A_RANGE];
extern byte adj_dex_safe[A_RANGE];
extern byte adj_con_fix[A_RANGE];
extern byte adj_con_mhp[A_RANGE];
extern byte adj_chr_gold[A_RANGE];
extern byte adj_chr_calm[A_RANGE];
extern byte adj_mag_study[A_RANGE];
extern byte adj_mag_mana[A_RANGE];
extern byte adj_mag_extra_mana[A_RANGE];
extern byte adj_mag_fail[A_RANGE];
extern byte adj_mag_stat[A_RANGE];
extern byte blows_table[12][12];
extern byte extract_energy[200];
extern byte invis_chance[31];
extern s32b player_exp[PY_MAX_LEVEL];
extern player_sex sex_info[SEX_MAX];
extern spell_book instruments[SV_MUSIC_MAX];
extern spell_book books[SV_BOOK_MAX];
extern player_race_special race_special_info[2][RACE_SPECIAL_LEVELS];
extern byte max_item_plus[30];
extern byte chest_traps[64];
extern cptr color_names[16];
extern cptr stat_names[A_MAX];
extern cptr stat_names_reduced[A_MAX];
extern cptr window_flag_desc[16];
extern option_type options[OPT_NORMAL];
extern option_type options_birth[OPT_BIRTH];
extern option_type options_cheat[OPT_CHEAT];
extern option_type options_squelch[OPT_SQUELCH];
extern byte option_page[OPT_PAGE_MAX][OPT_PAGE_PER];
extern cptr inscrip_text[MAX_INSCRIP];

/* variable.c */
extern cptr copyright;
extern byte version_major;
extern byte version_minor;
extern byte version_patch;
extern byte version_extra;
extern byte sf_major;
extern byte sf_minor;
extern byte sf_patch;
extern byte sf_extra;
extern u32b sf_xtra;
extern u32b sf_when;
extern u16b sf_lives;
extern u16b sf_saves;
extern bool arg_fiddle;
extern bool arg_wizard;
extern bool arg_sound;
extern bool arg_graphics;
extern bool arg_force_original;
extern bool arg_force_roguelike;
extern bool character_generated;
extern bool character_dungeon;
extern bool character_loaded;
extern bool character_saved;
extern bool character_existed;	
extern s16b character_icky;
extern s16b character_xtra;
extern u32b seed_randart;
extern u32b seed_alchemy;
extern u32b seed_flavor;
extern u32b seed_town;
extern s16b num_repro;
extern s16b monster_level;
extern char summon_kin_type;
extern s32b turn;
extern bool use_sound;
extern bool use_graphics;
extern s16b signal_count;
extern bool msg_flag;
extern bool inkey_base;
extern bool inkey_xtra;
extern bool inkey_scan;
extern bool inkey_flag;
extern bool opening_chest;
extern bool shimmer_monsters;
extern bool shimmer_objects;
extern bool repair_mflag_born;
extern bool repair_mflag_nice;
extern bool repair_mflag_show;
extern bool repair_mflag_mark;
extern s16b o_max;
extern s16b o_cnt;
extern s16b m_max;
extern s16b m_cnt;
extern monster_race monster_temp;
extern monster_race monster_temp_fake;
extern monster_lore lore_temp;
extern s16b rating;
extern bool good_item_flag;
extern bool closing_flag;
extern int player_uid;
extern int player_euid;
extern int player_egid;
extern char savefile[1024];
extern s16b macro__num;
extern cptr *macro__pat;
extern cptr *macro__act;
extern term *angband_term[ANGBAND_TERM_MAX];
extern char angband_term_name[ANGBAND_TERM_MAX][16];
extern byte angband_color_table[256][4];
extern char angband_sound_name[MSG_MAX][16];
extern sint view_n;
extern u16b *view_g;
extern sint temp_n;
extern u16b *temp_g;
extern byte *temp_y;
extern byte *temp_x;
extern byte (*cave_info)[256];
extern byte (*cave_feat)[MAX_DUNGEON_WID];
extern s16b (*cave_o_idx)[MAX_DUNGEON_WID];
extern s16b (*cave_m_idx)[MAX_DUNGEON_WID];
extern byte (*cave_cost)[MAX_DUNGEON_WID];
extern byte (*cave_when)[MAX_DUNGEON_WID];
extern maxima *z_info;
extern object_type *o_list;
extern monster_type *m_list;
extern monster_lore *lr_list;
extern monster_lore *lu_list;
extern store_type *store;
extern object_type *inventory;
extern s16b alloc_kind_size;
extern alloc_entry *alloc_kind_table;
extern s16b alloc_ego_size;
extern alloc_entry *alloc_ego_table;
extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern byte misc_to_attr[256];
extern char misc_to_char[256];
extern byte tval_to_attr[128];
extern char macro_buffer[1024];
extern cptr keymap_act[KEYMAP_MODES][256];
extern player_sex *sp_ptr;
extern player_race *rp_ptr;
extern player_class *cp_ptr;
extern ptr_player_race_special rsp_ptr[RACE_SPECIAL_LEVELS];
extern player_other *op_ptr;
extern player_type *p_ptr;
extern vault_type *v_info;
extern char *v_name;
extern char *v_text;
extern feature_type *f_info;
extern char *f_name;
extern object_kind *k_info;
extern char *k_name;
extern artifact_type *a_info;
extern char *a_name;
extern ego_item_type *e_info;
extern char *e_name;
extern item_prefix_type *px_info;
extern char *px_name;
extern monster_race *r_info;
extern char *r_name;
extern char *r_text;
extern monster_unique *u_info;
extern char *u_name;
extern char *u_text;
extern player_class *c_info;
extern char *c_name;
extern char *c_text;
extern player_race *p_info;
extern char *p_name;
extern hist_type *h_info;
extern char *h_text;
extern owner_type *b_info;
extern char *b_name;
extern byte *g_info;
extern quest_type *q_info;
extern char *q_name;
extern cptr ANGBAND_SYS;
extern cptr ANGBAND_GRAF;
extern cptr ANGBAND_DIR;
extern cptr ANGBAND_DIR_APEX;
extern cptr ANGBAND_DIR_DATA;
extern cptr ANGBAND_DIR_EDIT;
extern cptr ANGBAND_DIR_FILE;
extern cptr ANGBAND_DIR_HELP;
extern cptr ANGBAND_DIR_SAVE;
extern cptr ANGBAND_DIR_PREF;
extern cptr ANGBAND_DIR_USER;
extern cptr ANGBAND_DIR_XTRA;
extern bool item_tester_full;
extern byte item_tester_tval;
extern bool (*item_tester_hook)(object_type*);
extern bool (*ang_sort_comp)(vptr u, vptr v, int a, int b);
extern void (*ang_sort_swap)(vptr u, vptr v, int a, int b);
extern bool (*get_mon_num_hook)(int r_idx);
extern bool (*get_obj_num_hook)(int k_idx);
extern int highscore_fd;
extern bool use_transparency;
extern bool can_save;
extern alchemy_info potion_alch[SV_POTION_MAX];


/*
 * Automatically generated "function declarations"
 */

/* birth.c */
extern void player_birth(void);

/* cave.c */
extern sint distance(int y1, int x1, int y2, int x2);
extern bool los(int y1, int x1, int y2, int x2);
extern bool no_lite(void);
extern bool cave_valid_bold(int y, int x);
extern bool feat_supports_lighting(byte feat);
#ifdef USE_TRANSPARENCY
extern void map_info(int y, int x, byte *ap, char *cp, byte *tap, char *tcp);
#else /* USE_TRANSPARENCY */
extern void map_info(int y, int x, byte *ap, char *cp);
#endif /* USE_TRANSPARENCY */
extern void move_cursor_relative(int y, int x);
extern void print_rel(char c, byte a, int y, int x);
extern void note_spot(int y, int x);
extern void lite_spot(int y, int x);
extern void prt_map(void);
extern void display_map(int *cy, int *cx);
extern errr vinfo_init(void);
extern void forget_view(void);
extern void update_view(void);
extern void forget_flow(void);
extern void update_flow(void);
extern void map_area(void);
extern void wiz_lite(void);
extern void wiz_dark(void);
extern void town_illuminate(bool daytime);
extern void cave_set_feat(int y, int x, int feat);
extern sint project_path(u16b *gp, int range, int y1, int x1, int y2, int x2, int flg);
extern bool projectable(int y1, int x1, int y2, int x2);
extern void scatter(int *yp, int *xp, int y, int x, int d);
extern void health_track(int m_idx);
extern void monster_track(int r_idx, int u_idx);
extern void object_kind_track(int k_idx, int pval);
extern void disturb(int stop_search);

/* cmd-attk.c */
extern void search(void);
extern void hit_trap(int y, int x);
extern void py_attack(int y, int x);
extern void run_step(int dir);
extern void do_cmd_go_up(void);
extern void do_cmd_go_down(void);
extern void do_cmd_search(void);
extern void do_cmd_toggle_search(void);
extern void do_cmd_walk(void);
extern void do_cmd_jump(void);
extern void do_cmd_run(void);
extern void do_cmd_hold(void);
extern void do_cmd_stay(void);
extern void do_cmd_rest(void);
extern void do_cmd_fire(void);
extern void do_cmd_throw(void);

/* cmd-book.c */
extern bool literate(void);
extern bool spellcaster(void);
extern void print_spells(int book, bool music, int lev, int y, int x);
extern byte count_spells (int book);
extern void do_cmd_browse(void);
extern void do_cmd_study(void);
extern void do_cmd_magic(void);

/* cmd-item.c */
extern void do_cmd_inven(void);
extern void do_cmd_equip(void);
extern void do_cmd_wield(void);
extern void do_cmd_takeoff(void);
extern void do_cmd_drop(void);
extern void do_cmd_destroy(void);
extern void do_cmd_observe(void);
extern void do_cmd_uninscribe(void);
extern void do_cmd_inscribe(void);
extern void do_cmd_refill(void);
extern void do_cmd_eat_food(void);
extern void do_cmd_quaff_potion(void);
extern void do_cmd_read_scroll(void);
extern void do_cmd_use_staff(void);
extern void do_cmd_aim_wand(void);
extern void do_cmd_zap_rod(void);
extern void do_cmd_invoke_talisman(void);
extern void do_cmd_activate(void);
extern void do_cmd_use(void);
extern void do_cmd_mix(void);

/* cmd-know.c */
extern void do_cmd_display_character(void);
extern void do_cmd_message_one(void);
extern void do_cmd_messages(void);
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(void);
extern void do_cmd_quest(void);
extern void do_cmd_knowledge(void);
extern void do_cmd_query_symbol(void);

/* cmd-misc.c */
extern void do_cmd_target(void);
extern void do_cmd_look(void);
extern void do_cmd_locate(void);
extern void do_cmd_open(void);
extern void do_cmd_close(void);
extern void do_cmd_tunnel(void);
extern void do_cmd_disarm(void);
extern void do_cmd_bash(void);
extern void do_cmd_alter(void);
extern void do_cmd_use_racial(void);
extern void do_cmd_view_map(void);

/* cmd-util.c */
extern void do_cmd_redraw(void);
extern void options_birth_menu(bool adult);
extern void do_cmd_options(void);
extern void do_cmd_pref(void);
extern void do_cmd_macros(void);
extern void do_cmd_visuals(void);
extern void do_cmd_colors(void);
extern void do_cmd_load_screen(void);
extern void do_cmd_save_screen(void);
extern void do_cmd_help(void);
extern void do_cmd_suicide(void);
extern void do_cmd_save_game(void);

/* dungeon.c */
extern int value_check_aux1(object_type *o_ptr);
extern int value_check_aux2(object_type *o_ptr);
extern void play_game(bool new_game);

/* effects.c */
extern bool hp_player(int num);
extern void damage_player(int dam, cptr kb_str);
extern void gain_exp(s32b amount);
extern void lose_exp(s32b amount);
extern bool restore_exp(void);
extern void scramble_stats(void);
extern bool do_dec_stat(int stat, int amount, bool permanent, bool can_sustain);
extern bool do_res_stat(int stat);
extern bool do_inc_stat(int stat);
extern bool set_blind(int v);
extern bool set_confused(int v);
extern bool set_poisoned(int v);
extern bool set_diseased(int v);
extern bool set_afraid(int v);
extern bool set_paralyzed(int v);
extern bool set_image(int v);
extern bool set_fast(int v);
extern bool set_slow(int v);
extern bool set_shield(int v);
extern bool set_blessed(int v);
extern bool set_hero(int v);
extern bool set_rage(int v);
extern bool set_protevil(int v);
extern bool set_resilient(int v);
extern bool set_absorb(int v);
extern bool set_tim_see_invis(int v);
extern bool set_tim_invis(int v);
extern bool set_tim_infra(int v);
extern bool set_oppose_acid(int v);
extern bool set_oppose_elec(int v);
extern bool set_oppose_fire(int v);
extern bool set_oppose_cold(int v);
extern bool set_oppose_pois(int v);
extern bool set_oppose_disease(int v);
extern bool set_tim_res_lite(int v);
extern bool set_tim_res_dark(int v);
extern bool set_tim_res_confu(int v);
extern bool set_tim_res_sound(int v);
extern bool set_tim_res_shard(int v);
extern bool set_tim_res_nexus(int v);
extern bool set_tim_res_nethr(int v);
extern bool set_tim_res_chaos(int v);
extern bool set_tim_res_water(int v);
extern bool set_stun(int v);
extern bool set_cut(int v);
extern bool set_food(int v);

/* files.c */
extern void safe_setuid_drop(void);
extern void safe_setuid_grab(void);
extern errr process_pref_file_command(char *buf);
extern errr process_pref_file(cptr name);
extern void reset_visuals(bool unused);
extern errr check_time(void);
extern errr check_time_init(void);
extern errr check_load(void);
extern errr check_load_init(void);
extern void player_flags(u32b *f1, u32b *f2, u32b *f3, u32b *f4);
extern void display_player_equippy(int y, int x);
extern void display_player(byte mode);
extern errr file_character(cptr name, bool full);
extern bool show_file(cptr name, cptr what, int line, int mode);
extern void process_player_name(bool sf);
extern void get_name(void);
extern void display_scores(int from, int to);
extern errr predict_score(void);
extern void close_game(void);
extern void exit_game_panic(void);
extern void signals_ignore_tstp(void);
extern void signals_handle_tstp(void);
extern void signals_init(void);
extern void display_scores_aux(int from, int to, int note, high_score *score);

/* generate.c */
extern void generate_cave(void);

/* init2.c */
extern void init_file_paths(char *path);
extern void init_angband(void);
extern void cleanup_angband(void);

/* load.c */
extern errr rd_savefile_new(void);

/* melee1.c */
extern bool make_attack_normal(int m_idx);

/* melee2.c */
extern void process_monsters(byte minimum_energy);

/* monster1.c */
extern int collect_mon_special(u32b flags2, cptr vp[64]);
extern int collect_mon_innate(u32b flags4, cptr vp[64]);
extern int collect_mon_breaths(u32b flags4, cptr vp[64]);
extern int collect_mon_immunes(u32b flags3, u32b flags4, cptr vp[64]);
extern int collect_mon_resists(u32b flags3, u32b flags4, cptr vp[64]);
extern int collect_mon_vulnerabilities(u32b flags4, cptr vp[64]);
extern int collect_mon_spells(u32b flags5, u32b flags6, cptr vp[64]);
extern int collect_mon_group(u32b flags1, cptr vp[64]);
extern void describe_mon_attacks(int method, int effect, cptr method_text[1], cptr effect_text[1]);
extern void screen_roff(int r_idx, int u_idx);
extern void roff_top(int r_idx, int u_idx);
extern void display_roff(int r_idx, int u_idx);
extern void display_visible(void);

/* monster2.c */
extern void delete_monster_idx(int i);
extern void delete_monster(int y, int x);
extern void compact_monsters(int size);
extern void wipe_m_list(void);
extern s16b m_pop(void);
extern errr get_mon_num_prep(void);
extern s16b get_mon_num(int level);
extern void monster_desc(char *desc, monster_type *m_ptr, int mode);
extern void lore_do_probe(int m_idx);
extern void lore_treasure(int m_idx, int num_item, int num_gold);
extern void update_mon(int m_idx, bool full);
extern void update_monsters(bool full);
extern s16b monster_carry(int m_idx, object_type *j_ptr);
extern void monster_swap(int y1, int x1, int y2, int x2);
extern s16b player_place(int y, int x);
extern s16b monster_place(int y, int x, monster_type *n_ptr);
extern bool place_monster_aux(int y, int x, int r_idx, bool slp, bool grp, byte mode);
extern bool place_monster(int y, int x, bool slp, bool grp);
extern bool alloc_monster(int dis, bool slp);
extern bool summon_specific(int y1, int x1, int lev, int type);
extern bool multiply_monster(int m_idx);
extern void message_pain(int m_idx, int dam);
extern void update_smart_learn(int m_idx, int what);
extern void mon_exp(int r_idx, int u_idx, u32b *exint, u32b *exfrac);

/* monster3.c */
extern cptr monster_text(int r_idx, int u_idx);
extern cptr monster_name_race(int r_idx);
extern cptr monster_name_idx(int r_idx, int u_idx);
extern cptr monster_name(monster_type *m_ptr);
extern monster_race *get_monster_real(monster_type *m_ptr);
extern monster_race *get_monster_fake(int r_idx, int u_idx);
extern monster_lore *get_lore_idx(int r_idx, int u_idx);
extern void lore_learn(monster_type *m_ptr, int mode, u32b what, bool unseen);

/* object1.c */
extern byte ring_col[SV_RING_MAX];
extern byte potion_col[SV_POTION_MAX];
extern void flavor_init(void);
extern void alchemy_init(void);
extern void object_flags(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4);
extern void object_flags_known(object_type *o_ptr, u32b *f1, u32b *f2, u32b *f3, u32b *f4);
extern void object_desc(char *buf, object_type *o_ptr, int pref, int mode);
extern void object_desc_store(char *buf, object_type *o_ptr, int pref, int mode);
extern bool item_activation(char *text, int *time1, int *time2, object_type *o_ptr);
extern int identify_random_gen(object_type *o_ptr, cptr *info);
extern bool identify_fully_aux(object_type *o_ptr);
extern char index_to_label(int i);
extern s16b label_to_inven(int c);
extern s16b label_to_equip(int c);
extern cptr mention_use(int i);
extern cptr describe_use(int i);
extern bool wearable_p(object_type *o_ptr);
extern bool ammo_p(object_type *o_ptr);
extern bool item_tester_okay(object_type *o_ptr);
extern sint scan_floor(int *items, int size, int y, int x, int mode);
extern void display_inven(void);
extern void display_equip(void);
extern void show_inven(void);
extern void show_equip(void);
extern void toggle_inven_equip(void);
extern bool get_item(int *cp, cptr pmt, cptr str, int mode);
extern void show_floor(int *floor_list, int floor_num);
extern void strip_name(char *buf, int k_idx);

/* object2.c */
extern void excise_object_idx(int o_idx);
extern void delete_object_idx(int o_idx);
extern void delete_object(int y, int x);
extern void compact_objects(int size);
extern void wipe_o_list(void);
extern s16b o_pop(void);
extern errr get_obj_num_prep(void);
extern s16b get_obj_num(int level);
extern void artifact_aware(artifact_type *a_ptr);
extern void artifact_known(artifact_type *a_ptr);
extern void object_known(object_type *o_ptr);
extern void object_aware(object_type *o_ptr);
extern void object_tried(object_type *o_ptr);
extern s32b object_value(object_type *o_ptr);
extern bool destroy_check(object_type *o_ptr);
extern bool object_similar(object_type *o_ptr, object_type *j_ptr);
extern void object_absorb(object_type *o_ptr, object_type *j_ptr);
extern s16b lookup_kind(int tval, int sval);
extern void object_wipe(object_type *o_ptr);
extern void object_copy(object_type *o_ptr, object_type *j_ptr);
extern void object_prep(object_type *o_ptr, int k_idx);
extern void apply_magic(object_type *o_ptr, int lev, bool okay, bool good, bool great, bool real_depth);
extern bool make_object(object_type *j_ptr, bool good, bool great, bool real_depth);
extern bool make_mimic(object_type *j_ptr, byte a, char c);
extern bool make_gold(object_type *j_ptr, int coin_type);
extern s16b floor_carry(int y, int x, object_type *j_ptr);
extern void drop_near(object_type *j_ptr, int chance, int y, int x);
extern void acquirement(int y1, int x1, int num, bool great, bool real_depth);
extern void place_object(int y, int x, bool good, bool great);
extern void place_gold(int y, int x);
extern void pick_trap(int y, int x);
extern void place_trap(int y, int x);
extern void place_secret_door(int y, int x);
extern void place_closed_door(int y, int x);
extern void place_random_door(int y, int x);
extern void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
extern void reduce_charges(object_type *o_ptr, int amt);
extern void alchemy_describe(char *buf, int sval);
extern void inven_item_charges(int item);
extern void inven_item_describe(int item);
extern void inven_item_increase(int item, int num);
extern void inven_item_optimize(int item);
extern void floor_item_charges(int item);
extern void floor_item_describe(int item);
extern void floor_item_increase(int item, int num);
extern void floor_item_optimize(int item);
extern bool inven_carry_okay(object_type *o_ptr);
extern s16b inven_carry(object_type *o_ptr);
extern s16b inven_takeoff(int item, int amt);
extern void inven_drop(int item, int amt);
extern void combine_pack(void);
extern void reorder_pack(void);
extern void display_spell_list(void);
extern void display_koff(int k_idx, int pval);
extern bool make_fake_artifact(object_type *o_ptr, int a_idx);
extern void create_quest_item(int ny, int nx);
extern byte wgt_factor(object_type *o_ptr);

/* powers.c */
extern info_entry power_info[POW_MAX];
extern bool do_power(int idx, int dir, int beam, int dlev, int llev, int ilev, bool *obvious);

/* quest.c */
extern cptr describe_quest(s16b level, int mode);
extern void display_guild(void);
extern void guild_purchase(void);
extern byte quest_check(int lev);
extern int quest_num(int lev);
extern int quest_item_slot(void);

/* save.c */
extern bool save_player(void);
extern bool load_player(void);

/* spells1.c */
extern void teleport_away(int m_idx, int dis);
extern void teleport_player(int dis);
extern void teleport_player_to(int ny, int nx);
extern void teleport_monster_to(int m_idx, int ny, int nx);
extern void teleport_player_level(void);
extern void dimen_door(int dis, int fail);
extern void take_hit(int dam, cptr kb_str);
extern bool ignores_acid_p(u32b f2, u32b f3, u32b f4);
extern bool ignores_fire_p(u32b f2, u32b f3, u32b f4);
extern bool ignores_cold_p(u32b f2, u32b f3, u32b f4);
extern bool ignores_elec_p(u32b f2, u32b f3, u32b f4);
extern void acid_dam(int dam, cptr kb_str);
extern void elec_dam(int dam, cptr kb_str);
extern void fire_dam(int dam, cptr kb_str);
extern void cold_dam(int dam, cptr kb_str);
extern void rust_dam(int dam, cptr kb_str);
extern void rot_dam(int dam, cptr kb_str);
extern bool apply_disenchant(void);
extern bool project(int who, int rad, int y, int x, int dam, int typ, int flg);

/* spells2.c */
extern void warding_glyph(void);
extern void identify_pack(void);
extern bool remove_curse(void);
extern bool remove_all_curse(void);
extern void self_knowledge(void);
extern bool lose_all_info(void);
extern void set_recall(void);
extern bool detect_traps(void);
extern bool detect_doors(void);
extern bool detect_stairs(void);
extern bool detect_treasure(void);
extern bool detect_objects_gold(void);
extern bool detect_objects_normal(void);
extern bool detect_objects_magic(void);
extern bool detect_monsters_normal(void);
extern bool detect_monsters_invis(void);
extern bool detect_monsters_evil(void);
extern bool detect_all(void);
extern void stair_creation(void);
extern bool enchant_spell(int num_hit, int num_dam, int num_ac);
extern bool brand_weapon(byte weapon_type, int brand_type, bool add_plus);
extern bool ident_spell(void);
extern bool identify_fully(void);
extern bool recharge(int num);
extern bool speed_monsters(void);
extern bool slow_monsters(int power);
extern bool sleep_monsters(int power);
extern bool confuse_monsters(int power);
extern bool banish_evil(int dist);
extern bool turn_undead(int power);
extern bool scare_monsters(int power);
extern bool blight(int dam);
extern bool astral_burst(int perc);
extern bool dispel_undead(int dam);
extern bool dispel_evil(int dam);
extern bool dispel_non_evil(int dam);
extern bool dispel_monsters(int dam);
extern bool calm_animals(int power);
extern bool calm_non_evil(int power);
extern bool calm_non_chaos(int power);
extern bool calm_monsters(int power);
extern void aggravate_monsters(int who);
extern void genocide(void);
extern void mass_genocide(void);
extern bool probing(void);
extern void destroy_area(int y1, int x1, int r, bool full);
extern void earthquake(int cy, int cx, int r);
extern void lite_area(int dam, int rad);
extern void unlite_area(int dam, int rad);
extern bool fire_ball(int typ, int dir, int dam, int rad);
extern bool fire_bolt(int typ, int dir, int dam);
extern bool fire_beam(int typ, int dir, int dam);
extern bool fire_bolt_or_beam(int prob, int typ, int dir, int dam);
extern bool lite_line(int dir, int dam);
extern bool drain_life(int dir, int dam);
extern bool wall_to_mud(int dir);
extern bool destroy_door(int dir);
extern bool disarm_trap(int dir);
extern bool heal_monster(int dir);
extern bool speed_monster(int dir);
extern bool slow_monster(int dir, int plev);
extern bool sleep_monster(int dir, int plev);
extern bool confuse_monster(int dir, int plev);
extern bool blind_monster(int dir, int plev);
extern bool calm_monster(int dir, int plev);
extern bool poly_monster(int dir, int plev);
extern bool clone_monster(int dir);
extern bool fear_monster(int dir, int plev);
extern bool teleport_monster(int dir);
extern bool call_monster(int dir);
extern bool door_creation(void);
extern bool trap_creation(void);
extern bool destroy_doors_touch(void);
extern bool sleep_monsters_touch(int plev);
extern bool item_tester_hook_spellbooks(object_type *o_ptr);
extern bool item_tester_hook_bookmusic(object_type *o_ptr);

/* squelch.c */
extern void do_cmd_squelch(void);
extern bool squelch_itemp(object_type *o_ptr);
extern void do_squelch_item(object_type *o_ptr);
extern void destroy_squelched_items(void);

/* store.c */
extern void do_cmd_store(void);
extern void store_shuffle(int which);
extern void store_maint(int which);
extern void store_init(int which);

/* util.c */
extern errr path_parse(char *buf, int max, cptr file);
extern errr path_build(char *buf, int max, cptr path, cptr file);
extern FILE *my_fopen(cptr file, cptr mode);
extern FILE *my_fopen_temp(char *buf, int max);
extern errr my_fclose(FILE *fff);
extern errr my_fgets(FILE *fff, char *buf, huge n);
extern errr my_fputs(FILE *fff, cptr buf, huge n);
extern errr fd_kill(cptr file);
extern errr fd_move(cptr file, cptr what);
extern errr fd_copy(cptr file, cptr what);
extern int fd_make(cptr file, int mode);
extern int fd_open(cptr file, int flags);
extern errr fd_lock(int fd, int what);
extern errr fd_seek(int fd, long n);
extern errr fd_read(int fd, char *buf, huge n);
extern errr fd_write(int fd, cptr buf, huge n);
extern errr fd_close(int fd);
extern errr check_modification_date(int fd, cptr template_file);
extern void text_to_ascii(char *buf, cptr str);
extern void ascii_to_text(char *buf, cptr str);
extern sint macro_find_exact(cptr pat);
extern errr macro_add(cptr pat, cptr act);
extern errr macro_init(void);
extern void flush(void);
extern char inkey(void);
extern void bell(cptr reason);
extern void sound(int val);
extern s16b quark_add(cptr str);
extern cptr quark_str(s16b i);
extern errr quarks_init(void);
extern errr quarks_free(void);
extern s16b message_num(void);
extern cptr message_str(s16b age);
extern u16b message_type(s16b age);
extern byte message_color(s16b age);
extern errr message_color_define(u16b type, byte color);
extern void message_add(cptr str, u16b type);
extern errr messages_init(void);
extern void messages_free(void);
extern void move_cursor(int row, int col);
extern void message(u16b message_type, s16b extra, cptr msg);
extern void message_format(u16b message_type, s16b extra, cptr fmt, ...);
extern void message_flush(void);
extern void screen_save(void);
extern void screen_load(void);
extern void c_put_str(byte attr, cptr str, int row, int col);
extern void put_str(cptr str, int row, int col);
extern void c_prt(byte attr, cptr str, int row, int col);
extern void prt(cptr str, int row, int col);
extern void c_roff(byte a, cptr str);
extern void roff(cptr str);
extern void clear_from(int row);
extern bool askfor_aux(char *buf, int len);
extern bool get_string(cptr prompt, char *buf, int len);
extern s16b get_quantity(cptr prompt, int max);
extern bool get_check(cptr prompt);
extern bool get_com(cptr prompt, char *command);
extern void pause_line(int row);
extern void request_command(bool shopping);
extern uint damroll(uint num, uint sides);
extern uint maxroll(uint num, uint sides);
extern bool is_a_vowel(int ch);
extern int color_char_to_attr(char c);

#ifdef SUPPORT_GAMMA
extern void build_gamma_table(int gamma);
extern byte gamma_table[256];
#endif /* SUPPORT_GAMMA */

/* xtra1.c */
extern byte modify_stat_value(byte value, int amount);
extern void notice_stuff(void);
extern void update_stuff(void);
extern void redraw_stuff(void);
extern void window_stuff(void);
extern void handle_stuff(void);

/* xtra2.c */
extern void check_experience(void);
extern void monster_death(int m_idx);
extern bool mon_take_hit(int m_idx, int dam, bool *fear, cptr note);
extern void verify_panel(void);
extern bool ang_mon_sort_comp_hook(vptr u, vptr v, int a, int b);
extern void ang_mon_sort_swap_hook(vptr u, vptr v, int a, int b);
extern void ang_sort(vptr u, vptr v, int n);
extern void saturate_mon_list(monster_list_entry *who, int *count, bool allow_base);
extern sint motion_dir(int y1, int x1, int y2, int x2);
extern sint target_dir(char ch);
extern bool target_okay(void);
extern void target_set_monster(int m_idx);
extern bool target_set_interactive(int mode);
extern bool get_aim_dir(int *dp);
extern bool get_rep_dir(int *dp);
extern bool confuse_dir(int *dp);

/*
 * Hack -- conditional (or "bizarre") externs
 */
 
#ifdef SET_UID
# ifndef HAS_USLEEP
/* util.c */
extern int usleep(huge usecs);
# endif
extern void user_name(char *buf, int id);
extern errr user_home(char *buf, int len);
#endif

#ifdef ALLOW_REPEAT

/* util.c */
extern void repeat_push(int what);
extern bool repeat_pull(int *what);
extern void repeat_check(void);

#endif /* ALLOW_REPEAT */

#ifdef GJW_RANDART

/* randart.c */
extern errr do_randart(u32b randart_seed, bool full);

#endif /* GJW_RANDART */

#ifdef ALLOW_SPOILERS

/* wizard1.c */
extern void do_cmd_spoilers(void);

#endif /* ALLOW_SPOILERS */

#ifdef ALLOW_DEBUG

/* wizard2.c */
extern void do_cmd_debug(void);

#endif /*ALLOW_DEBUG */

#ifdef ALLOW_BORG

/* Borg.c */
extern void do_cmd_borg(void);
# ifdef ALLOW_BORG_GRAPHICS
extern void init_translate_visuals(void);
# endif /* ALLOW_BORG_GRAPHICS */

#endif
