////////////////////////////////////////////////////
// NOTE: this is an amalgamation of the original
// unibilium made for the vis editor project
//
// - removes all functions not needed by termkey
// - namespaces everything to avoid conflicts when
//   #included
////////////////////////////////////////////////////

/*

Copyright 2008, 2010, 2012, 2013, 2015 Lukas Mai.

This file is part of unibilium.

Unibilium is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unibilium is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with unibilium.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef UNIBI_EXPORT
#define UNIBI_EXPORT
#endif

#ifndef TERMINFO
#define TERMINFO      "/usr/share/terminfo"
#endif
#ifndef TERMINFO_DIRS
#define TERMINFO_DIRS "/etc/terminfo:/usr/share/terminfo"
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum unibi_boolean {
	unibi_boolean_begin_,
	unibi_auto_left_margin,
	unibi_auto_right_margin,
	unibi_no_esc_ctlc,
	unibi_ceol_standout_glitch,
	unibi_eat_newline_glitch,
	unibi_erase_overstrike,
	unibi_generic_type,
	unibi_hard_copy,
	unibi_has_meta_key,
	unibi_has_status_line,
	unibi_insert_null_glitch,
	unibi_memory_above,
	unibi_memory_below,
	unibi_move_insert_mode,
	unibi_move_standout_mode,
	unibi_over_strike,
	unibi_status_line_esc_ok,
	unibi_dest_tabs_magic_smso,
	unibi_tilde_glitch,
	unibi_transparent_underline,
	unibi_xon_xoff,
	unibi_needs_xon_xoff,
	unibi_prtr_silent,
	unibi_hard_cursor,
	unibi_non_rev_rmcup,
	unibi_no_pad_char,
	unibi_non_dest_scroll_region,
	unibi_can_change,
	unibi_back_color_erase,
	unibi_hue_lightness_saturation,
	unibi_col_addr_glitch,
	unibi_cr_cancels_micro_mode,
	unibi_has_print_wheel,
	unibi_row_addr_glitch,
	unibi_semi_auto_right_margin,
	unibi_cpi_changes_res,
	unibi_lpi_changes_res,
	unibi_backspaces_with_bs,
	unibi_crt_no_scrolling,
	unibi_no_correctly_working_cr,
	unibi_gnu_has_meta_key,
	unibi_linefeed_is_newline,
	unibi_has_hardware_tabs,
	unibi_return_does_clr_eol,
	unibi_boolean_end_
};

enum unibi_numeric {
	unibi_numeric_begin_ = unibi_boolean_end_,
	unibi_columns,
	unibi_init_tabs,
	unibi_lines,
	unibi_lines_of_memory,
	unibi_magic_cookie_glitch,
	unibi_padding_baud_rate,
	unibi_virtual_terminal,
	unibi_width_status_line,
	unibi_num_labels,
	unibi_label_height,
	unibi_label_width,
	unibi_max_attributes,
	unibi_maximum_windows,
	unibi_max_colors,
	unibi_max_pairs,
	unibi_no_color_video,
	unibi_buffer_capacity,
	unibi_dot_vert_spacing,
	unibi_dot_horz_spacing,
	unibi_max_micro_address,
	unibi_max_micro_jump,
	unibi_micro_col_size,
	unibi_micro_line_size,
	unibi_number_of_pins,
	unibi_output_res_char,
	unibi_output_res_line,
	unibi_output_res_horz_inch,
	unibi_output_res_vert_inch,
	unibi_print_rate,
	unibi_wide_char_size,
	unibi_buttons,
	unibi_bit_image_entwining,
	unibi_bit_image_type,
	unibi_magic_cookie_glitch_ul,
	unibi_carriage_return_delay,
	unibi_new_line_delay,
	unibi_backspace_delay,
	unibi_horizontal_tab_delay,
	unibi_number_of_function_keys,
	unibi_numeric_end_
};

enum unibi_string {
	unibi_string_begin_ = unibi_numeric_end_,
	unibi_back_tab,
	unibi_bell,
	unibi_carriage_return,
	unibi_change_scroll_region,
	unibi_clear_all_tabs,
	unibi_clear_screen,
	unibi_clr_eol,
	unibi_clr_eos,
	unibi_column_address,
	unibi_command_character,
	unibi_cursor_address,
	unibi_cursor_down,
	unibi_cursor_home,
	unibi_cursor_invisible,
	unibi_cursor_left,
	unibi_cursor_mem_address,
	unibi_cursor_normal,
	unibi_cursor_right,
	unibi_cursor_to_ll,
	unibi_cursor_up,
	unibi_cursor_visible,
	unibi_delete_character,
	unibi_delete_line,
	unibi_dis_status_line,
	unibi_down_half_line,
	unibi_enter_alt_charset_mode,
	unibi_enter_blink_mode,
	unibi_enter_bold_mode,
	unibi_enter_ca_mode,
	unibi_enter_delete_mode,
	unibi_enter_dim_mode,
	unibi_enter_insert_mode,
	unibi_enter_secure_mode,
	unibi_enter_protected_mode,
	unibi_enter_reverse_mode,
	unibi_enter_standout_mode,
	unibi_enter_underline_mode,
	unibi_erase_chars,
	unibi_exit_alt_charset_mode,
	unibi_exit_attribute_mode,
	unibi_exit_ca_mode,
	unibi_exit_delete_mode,
	unibi_exit_insert_mode,
	unibi_exit_standout_mode,
	unibi_exit_underline_mode,
	unibi_flash_screen,
	unibi_form_feed,
	unibi_from_status_line,
	unibi_init_1string,
	unibi_init_2string,
	unibi_init_3string,
	unibi_init_file,
	unibi_insert_character,
	unibi_insert_line,
	unibi_insert_padding,
	unibi_key_backspace,
	unibi_key_catab,
	unibi_key_clear,
	unibi_key_ctab,
	unibi_key_dc,
	unibi_key_dl,
	unibi_key_down,
	unibi_key_eic,
	unibi_key_eol,
	unibi_key_eos,
	unibi_key_f0,
	unibi_key_f1,
	unibi_key_f10,
	unibi_key_f2,
	unibi_key_f3,
	unibi_key_f4,
	unibi_key_f5,
	unibi_key_f6,
	unibi_key_f7,
	unibi_key_f8,
	unibi_key_f9,
	unibi_key_home,
	unibi_key_ic,
	unibi_key_il,
	unibi_key_left,
	unibi_key_ll,
	unibi_key_npage,
	unibi_key_ppage,
	unibi_key_right,
	unibi_key_sf,
	unibi_key_sr,
	unibi_key_stab,
	unibi_key_up,
	unibi_keypad_local,
	unibi_keypad_xmit,
	unibi_lab_f0,
	unibi_lab_f1,
	unibi_lab_f10,
	unibi_lab_f2,
	unibi_lab_f3,
	unibi_lab_f4,
	unibi_lab_f5,
	unibi_lab_f6,
	unibi_lab_f7,
	unibi_lab_f8,
	unibi_lab_f9,
	unibi_meta_off,
	unibi_meta_on,
	unibi_newline,
	unibi_pad_char,
	unibi_parm_dch,
	unibi_parm_delete_line,
	unibi_parm_down_cursor,
	unibi_parm_ich,
	unibi_parm_index,
	unibi_parm_insert_line,
	unibi_parm_left_cursor,
	unibi_parm_right_cursor,
	unibi_parm_rindex,
	unibi_parm_up_cursor,
	unibi_pkey_key,
	unibi_pkey_local,
	unibi_pkey_xmit,
	unibi_print_screen,
	unibi_prtr_off,
	unibi_prtr_on,
	unibi_repeat_char,
	unibi_reset_1string,
	unibi_reset_2string,
	unibi_reset_3string,
	unibi_reset_file,
	unibi_restore_cursor,
	unibi_row_address,
	unibi_save_cursor,
	unibi_scroll_forward,
	unibi_scroll_reverse,
	unibi_set_attributes,
	unibi_set_tab,
	unibi_set_window,
	unibi_tab,
	unibi_to_status_line,
	unibi_underline_char,
	unibi_up_half_line,
	unibi_init_prog,
	unibi_key_a1,
	unibi_key_a3,
	unibi_key_b2,
	unibi_key_c1,
	unibi_key_c3,
	unibi_prtr_non,
	unibi_char_padding,
	unibi_acs_chars,
	unibi_plab_norm,
	unibi_key_btab,
	unibi_enter_xon_mode,
	unibi_exit_xon_mode,
	unibi_enter_am_mode,
	unibi_exit_am_mode,
	unibi_xon_character,
	unibi_xoff_character,
	unibi_ena_acs,
	unibi_label_on,
	unibi_label_off,
	unibi_key_beg,
	unibi_key_cancel,
	unibi_key_close,
	unibi_key_command,
	unibi_key_copy,
	unibi_key_create,
	unibi_key_end,
	unibi_key_enter,
	unibi_key_exit,
	unibi_key_find,
	unibi_key_help,
	unibi_key_mark,
	unibi_key_message,
	unibi_key_move,
	unibi_key_next,
	unibi_key_open,
	unibi_key_options,
	unibi_key_previous,
	unibi_key_print,
	unibi_key_redo,
	unibi_key_reference,
	unibi_key_refresh,
	unibi_key_replace,
	unibi_key_restart,
	unibi_key_resume,
	unibi_key_save,
	unibi_key_suspend,
	unibi_key_undo,
	unibi_key_sbeg,
	unibi_key_scancel,
	unibi_key_scommand,
	unibi_key_scopy,
	unibi_key_screate,
	unibi_key_sdc,
	unibi_key_sdl,
	unibi_key_select,
	unibi_key_send,
	unibi_key_seol,
	unibi_key_sexit,
	unibi_key_sfind,
	unibi_key_shelp,
	unibi_key_shome,
	unibi_key_sic,
	unibi_key_sleft,
	unibi_key_smessage,
	unibi_key_smove,
	unibi_key_snext,
	unibi_key_soptions,
	unibi_key_sprevious,
	unibi_key_sprint,
	unibi_key_sredo,
	unibi_key_sreplace,
	unibi_key_sright,
	unibi_key_srsume,
	unibi_key_ssave,
	unibi_key_ssuspend,
	unibi_key_sundo,
	unibi_req_for_input,
	unibi_key_f11,
	unibi_key_f12,
	unibi_key_f13,
	unibi_key_f14,
	unibi_key_f15,
	unibi_key_f16,
	unibi_key_f17,
	unibi_key_f18,
	unibi_key_f19,
	unibi_key_f20,
	unibi_key_f21,
	unibi_key_f22,
	unibi_key_f23,
	unibi_key_f24,
	unibi_key_f25,
	unibi_key_f26,
	unibi_key_f27,
	unibi_key_f28,
	unibi_key_f29,
	unibi_key_f30,
	unibi_key_f31,
	unibi_key_f32,
	unibi_key_f33,
	unibi_key_f34,
	unibi_key_f35,
	unibi_key_f36,
	unibi_key_f37,
	unibi_key_f38,
	unibi_key_f39,
	unibi_key_f40,
	unibi_key_f41,
	unibi_key_f42,
	unibi_key_f43,
	unibi_key_f44,
	unibi_key_f45,
	unibi_key_f46,
	unibi_key_f47,
	unibi_key_f48,
	unibi_key_f49,
	unibi_key_f50,
	unibi_key_f51,
	unibi_key_f52,
	unibi_key_f53,
	unibi_key_f54,
	unibi_key_f55,
	unibi_key_f56,
	unibi_key_f57,
	unibi_key_f58,
	unibi_key_f59,
	unibi_key_f60,
	unibi_key_f61,
	unibi_key_f62,
	unibi_key_f63,
	unibi_clr_bol,
	unibi_clear_margins,
	unibi_set_left_margin,
	unibi_set_right_margin,
	unibi_label_format,
	unibi_set_clock,
	unibi_display_clock,
	unibi_remove_clock,
	unibi_create_window,
	unibi_goto_window,
	unibi_hangup,
	unibi_dial_phone,
	unibi_quick_dial,
	unibi_tone,
	unibi_pulse,
	unibi_flash_hook,
	unibi_fixed_pause,
	unibi_wait_tone,
	unibi_user0,
	unibi_user1,
	unibi_user2,
	unibi_user3,
	unibi_user4,
	unibi_user5,
	unibi_user6,
	unibi_user7,
	unibi_user8,
	unibi_user9,
	unibi_orig_pair,
	unibi_orig_colors,
	unibi_initialize_color,
	unibi_initialize_pair,
	unibi_set_color_pair,
	unibi_set_foreground,
	unibi_set_background,
	unibi_change_char_pitch,
	unibi_change_line_pitch,
	unibi_change_res_horz,
	unibi_change_res_vert,
	unibi_define_char,
	unibi_enter_doublewide_mode,
	unibi_enter_draft_quality,
	unibi_enter_italics_mode,
	unibi_enter_leftward_mode,
	unibi_enter_micro_mode,
	unibi_enter_near_letter_quality,
	unibi_enter_normal_quality,
	unibi_enter_shadow_mode,
	unibi_enter_subscript_mode,
	unibi_enter_superscript_mode,
	unibi_enter_upward_mode,
	unibi_exit_doublewide_mode,
	unibi_exit_italics_mode,
	unibi_exit_leftward_mode,
	unibi_exit_micro_mode,
	unibi_exit_shadow_mode,
	unibi_exit_subscript_mode,
	unibi_exit_superscript_mode,
	unibi_exit_upward_mode,
	unibi_micro_column_address,
	unibi_micro_down,
	unibi_micro_left,
	unibi_micro_right,
	unibi_micro_row_address,
	unibi_micro_up,
	unibi_order_of_pins,
	unibi_parm_down_micro,
	unibi_parm_left_micro,
	unibi_parm_right_micro,
	unibi_parm_up_micro,
	unibi_select_char_set,
	unibi_set_bottom_margin,
	unibi_set_bottom_margin_parm,
	unibi_set_left_margin_parm,
	unibi_set_right_margin_parm,
	unibi_set_top_margin,
	unibi_set_top_margin_parm,
	unibi_start_bit_image,
	unibi_start_char_set_def,
	unibi_stop_bit_image,
	unibi_stop_char_set_def,
	unibi_subscript_characters,
	unibi_superscript_characters,
	unibi_these_cause_cr,
	unibi_zero_motion,
	unibi_char_set_names,
	unibi_key_mouse,
	unibi_mouse_info,
	unibi_req_mouse_pos,
	unibi_get_mouse,
	unibi_set_a_foreground,
	unibi_set_a_background,
	unibi_pkey_plab,
	unibi_device_type,
	unibi_code_set_init,
	unibi_set0_des_seq,
	unibi_set1_des_seq,
	unibi_set2_des_seq,
	unibi_set3_des_seq,
	unibi_set_lr_margin,
	unibi_set_tb_margin,
	unibi_bit_image_repeat,
	unibi_bit_image_newline,
	unibi_bit_image_carriage_return,
	unibi_color_names,
	unibi_define_bit_image_region,
	unibi_end_bit_image_region,
	unibi_set_color_band,
	unibi_set_page_length,
	unibi_display_pc_char,
	unibi_enter_pc_charset_mode,
	unibi_exit_pc_charset_mode,
	unibi_enter_scancode_mode,
	unibi_exit_scancode_mode,
	unibi_pc_term_options,
	unibi_scancode_escape,
	unibi_alt_scancode_esc,
	unibi_enter_horizontal_hl_mode,
	unibi_enter_left_hl_mode,
	unibi_enter_low_hl_mode,
	unibi_enter_right_hl_mode,
	unibi_enter_top_hl_mode,
	unibi_enter_vertical_hl_mode,
	unibi_set_a_attributes,
	unibi_set_pglen_inch,
	unibi_termcap_init2,
	unibi_termcap_reset,
	unibi_linefeed_if_not_lf,
	unibi_backspace_if_not_bs,
	unibi_other_non_function_keys,
	unibi_arrow_key_map,
	unibi_acs_ulcorner,
	unibi_acs_llcorner,
	unibi_acs_urcorner,
	unibi_acs_lrcorner,
	unibi_acs_ltee,
	unibi_acs_rtee,
	unibi_acs_btee,
	unibi_acs_ttee,
	unibi_acs_hline,
	unibi_acs_vline,
	unibi_acs_plus,
	unibi_memory_lock,
	unibi_memory_unlock,
	unibi_box_chars_1,
	unibi_string_end_
};

#ifndef countof
#define countof(a) (sizeof (a) / sizeof *(a))
#endif

#ifndef S16_MAX
#define S16_MAX (0x7FFF)
#endif
#ifndef S32_MAX
#define S32_MAX (0x7FFFFFFF)
#endif

#define NCONTAINERS(n, csize) (((n) - 1) / (csize) + 1u)

enum {
	MAGIC_16BIT = 00432,
	MAGIC_32BIT = 01036
};

typedef struct {
	const char *name;
	const char **aliases;

	unsigned char bools[NCONTAINERS(unibi_boolean_end_ - unibi_boolean_begin_ - 1, CHAR_BIT)];
	int nums[unibi_numeric_end_ - unibi_numeric_begin_ - 1];
	const char *strs[unibi_string_end_ - unibi_string_begin_ - 1];
} unibi_term;

static const char *unibi_names_str[] = {
	"back_tab",
	"bell",
	"carriage_return",
	"change_scroll_region",
	"clear_all_tabs",
	"clear_screen",
	"clr_eol",
	"clr_eos",
	"column_address",
	"command_character",
	"cursor_address",
	"cursor_down",
	"cursor_home",
	"cursor_invisible",
	"cursor_left",
	"cursor_mem_address",
	"cursor_normal",
	"cursor_right",
	"cursor_to_ll",
	"cursor_up",
	"cursor_visible",
	"delete_character",
	"delete_line",
	"dis_status_line",
	"down_half_line",
	"enter_alt_charset_mode",
	"enter_blink_mode",
	"enter_bold_mode",
	"enter_ca_mode",
	"enter_delete_mode",
	"enter_dim_mode",
	"enter_insert_mode",
	"enter_secure_mode",
	"enter_protected_mode",
	"enter_reverse_mode",
	"enter_standout_mode",
	"enter_underline_mode",
	"erase_chars",
	"exit_alt_charset_mode",
	"exit_attribute_mode",
	"exit_ca_mode",
	"exit_delete_mode",
	"exit_insert_mode",
	"exit_standout_mode",
	"exit_underline_mode",
	"flash_screen",
	"form_feed",
	"from_status_line",
	"init_1string",
	"init_2string",
	"init_3string",
	"init_file",
	"insert_character",
	"insert_line",
	"insert_padding",
	"key_backspace",
	"key_catab",
	"key_clear",
	"key_ctab",
	"key_dc",
	"key_dl",
	"key_down",
	"key_eic",
	"key_eol",
	"key_eos",
	"key_f0",
	"key_f1",
	"key_f10",
	"key_f2",
	"key_f3",
	"key_f4",
	"key_f5",
	"key_f6",
	"key_f7",
	"key_f8",
	"key_f9",
	"key_home",
	"key_ic",
	"key_il",
	"key_left",
	"key_ll",
	"key_npage",
	"key_ppage",
	"key_right",
	"key_sf",
	"key_sr",
	"key_stab",
	"key_up",
	"keypad_local",
	"keypad_xmit",
	"lab_f0",
	"lab_f1",
	"lab_f10",
	"lab_f2",
	"lab_f3",
	"lab_f4",
	"lab_f5",
	"lab_f6",
	"lab_f7",
	"lab_f8",
	"lab_f9",
	"meta_off",
	"meta_on",
	"newline",
	"pad_char",
	"parm_dch",
	"parm_delete_line",
	"parm_down_cursor",
	"parm_ich",
	"parm_index",
	"parm_insert_line",
	"parm_left_cursor",
	"parm_right_cursor",
	"parm_rindex",
	"parm_up_cursor",
	"pkey_key",
	"pkey_local",
	"pkey_xmit",
	"print_screen",
	"prtr_off",
	"prtr_on",
	"repeat_char",
	"reset_1string",
	"reset_2string",
	"reset_3string",
	"reset_file",
	"restore_cursor",
	"row_address",
	"save_cursor",
	"scroll_forward",
	"scroll_reverse",
	"set_attributes",
	"set_tab",
	"set_window",
	"tab",
	"to_status_line",
	"underline_char",
	"up_half_line",
	"init_prog",
	"key_a1",
	"key_a3",
	"key_b2",
	"key_c1",
	"key_c3",
	"prtr_non",
	"char_padding",
	"acs_chars",
	"plab_norm",
	"key_btab",
	"enter_xon_mode",
	"exit_xon_mode",
	"enter_am_mode",
	"exit_am_mode",
	"xon_character",
	"xoff_character",
	"ena_acs",
	"label_on",
	"label_off",
	"key_beg",
	"key_cancel",
	"key_close",
	"key_command",
	"key_copy",
	"key_create",
	"key_end",
	"key_enter",
	"key_exit",
	"key_find",
	"key_help",
	"key_mark",
	"key_message",
	"key_move",
	"key_next",
	"key_open",
	"key_options",
	"key_previous",
	"key_print",
	"key_redo",
	"key_reference",
	"key_refresh",
	"key_replace",
	"key_restart",
	"key_resume",
	"key_save",
	"key_suspend",
	"key_undo",
	"key_sbeg",
	"key_scancel",
	"key_scommand",
	"key_scopy",
	"key_screate",
	"key_sdc",
	"key_sdl",
	"key_select",
	"key_send",
	"key_seol",
	"key_sexit",
	"key_sfind",
	"key_shelp",
	"key_shome",
	"key_sic",
	"key_sleft",
	"key_smessage",
	"key_smove",
	"key_snext",
	"key_soptions",
	"key_sprevious",
	"key_sprint",
	"key_sredo",
	"key_sreplace",
	"key_sright",
	"key_srsume",
	"key_ssave",
	"key_ssuspend",
	"key_sundo",
	"req_for_input",
	"key_f11",
	"key_f12",
	"key_f13",
	"key_f14",
	"key_f15",
	"key_f16",
	"key_f17",
	"key_f18",
	"key_f19",
	"key_f20",
	"key_f21",
	"key_f22",
	"key_f23",
	"key_f24",
	"key_f25",
	"key_f26",
	"key_f27",
	"key_f28",
	"key_f29",
	"key_f30",
	"key_f31",
	"key_f32",
	"key_f33",
	"key_f34",
	"key_f35",
	"key_f36",
	"key_f37",
	"key_f38",
	"key_f39",
	"key_f40",
	"key_f41",
	"key_f42",
	"key_f43",
	"key_f44",
	"key_f45",
	"key_f46",
	"key_f47",
	"key_f48",
	"key_f49",
	"key_f50",
	"key_f51",
	"key_f52",
	"key_f53",
	"key_f54",
	"key_f55",
	"key_f56",
	"key_f57",
	"key_f58",
	"key_f59",
	"key_f60",
	"key_f61",
	"key_f62",
	"key_f63",
	"clr_bol",
	"clear_margins",
	"set_left_margin",
	"set_right_margin",
	"label_format",
	"set_clock",
	"display_clock",
	"remove_clock",
	"create_window",
	"goto_window",
	"hangup",
	"dial_phone",
	"quick_dial",
	"tone",
	"pulse",
	"flash_hook",
	"fixed_pause",
	"wait_tone",
	"user0",
	"user1",
	"user2",
	"user3",
	"user4",
	"user5",
	"user6",
	"user7",
	"user8",
	"user9",
	"orig_pair",
	"orig_colors",
	"initialize_color",
	"initialize_pair",
	"set_color_pair",
	"set_foreground",
	"set_background",
	"change_char_pitch",
	"change_line_pitch",
	"change_res_horz",
	"change_res_vert",
	"define_char",
	"enter_doublewide_mode",
	"enter_draft_quality",
	"enter_italics_mode",
	"enter_leftward_mode",
	"enter_micro_mode",
	"enter_near_letter_quality",
	"enter_normal_quality",
	"enter_shadow_mode",
	"enter_subscript_mode",
	"enter_superscript_mode",
	"enter_upward_mode",
	"exit_doublewide_mode",
	"exit_italics_mode",
	"exit_leftward_mode",
	"exit_micro_mode",
	"exit_shadow_mode",
	"exit_subscript_mode",
	"exit_superscript_mode",
	"exit_upward_mode",
	"micro_column_address",
	"micro_down",
	"micro_left",
	"micro_right",
	"micro_row_address",
	"micro_up",
	"order_of_pins",
	"parm_down_micro",
	"parm_left_micro",
	"parm_right_micro",
	"parm_up_micro",
	"select_char_set",
	"set_bottom_margin",
	"set_bottom_margin_parm",
	"set_left_margin_parm",
	"set_right_margin_parm",
	"set_top_margin",
	"set_top_margin_parm",
	"start_bit_image",
	"start_char_set_def",
	"stop_bit_image",
	"stop_char_set_def",
	"subscript_characters",
	"superscript_characters",
	"these_cause_cr",
	"zero_motion",
	"char_set_names",
	"key_mouse",
	"mouse_info",
	"req_mouse_pos",
	"get_mouse",
	"set_a_foreground",
	"set_a_background",
	"pkey_plab",
	"device_type",
	"code_set_init",
	"set0_des_seq",
	"set1_des_seq",
	"set2_des_seq",
	"set3_des_seq",
	"set_lr_margin",
	"set_tb_margin",
	"bit_image_repeat",
	"bit_image_newline",
	"bit_image_carriage_return",
	"color_names",
	"define_bit_image_region",
	"end_bit_image_region",
	"set_color_band",
	"set_page_length",
	"display_pc_char",
	"enter_pc_charset_mode",
	"exit_pc_charset_mode",
	"enter_scancode_mode",
	"exit_scancode_mode",
	"pc_term_options",
	"scancode_escape",
	"alt_scancode_esc",
	"enter_horizontal_hl_mode",
	"enter_left_hl_mode",
	"enter_low_hl_mode",
	"enter_right_hl_mode",
	"enter_top_hl_mode",
	"enter_vertical_hl_mode",
	"set_a_attributes",
	"set_pglen_inch",
	"termcap_init2",
	"termcap_reset",
	"linefeed_if_not_lf",
	"backspace_if_not_bs",
	"other_non_function_keys",
	"arrow_key_map",
	"acs_ulcorner",
	"acs_llcorner",
	"acs_urcorner",
	"acs_lrcorner",
	"acs_ltee",
	"acs_rtee",
	"acs_btee",
	"acs_ttee",
	"acs_hline",
	"acs_vline",
	"acs_plus",
	"memory_lock",
	"memory_unlock",
	"box_chars_1",
};

UNIBI_EXPORT const char *
unibi_name_str(enum unibi_string v)
{
	assert(v > unibi_string_begin_ && v < unibi_string_end_);
	if (v <= unibi_string_begin_ || v >= unibi_string_end_)
		return 0;
	return unibi_names_str[v - unibi_string_begin_ - 1];
}

static uint16_t
unibi_read_u16(const char *p)
{
	const unsigned char *q = (const unsigned char *)p;
	uint16_t result = 0;
	result |= q[1]; result <<= 8;
	result |= q[0];
	return result;
}

static int16_t
unibi_read_s16(const char *p)
{
	unsigned short n = unibi_read_u16(p);
	return n <= S16_MAX ? n : -1;
}

static uint32_t
unibi_read_u32(const char *p)
{
	const unsigned char *q = (const unsigned char *)p;
	uint32_t result = 0;
	result |= q[3]; result <<= 8;
	result |= q[2]; result <<= 8;
	result |= q[1]; result <<= 8;
	result |= q[0];
	return result;
}

static int32_t
unibi_read_s32(const char *p)
{
	uint32_t n = unibi_read_u32(p);
	return n <= S32_MAX ? (int)n : -1;
}

UNIBI_EXPORT void
unibi_destroy(unibi_term *t)
{
	free(t);
}

#define FAIL_IF_(c, e, f) do { if (c) { f; errno = (e); return 0; } } while (0)
#define FAIL_IF(c, e) FAIL_IF_(c, e, (void)0)
#define DEL_FAIL_IF(c, e, x) FAIL_IF_(c, e, unibi_destroy(x))

UNIBI_EXPORT unibi_term *
unibi_from_mem(const char *p, size_t n)
{
	if (n < 12)
		return 0;

	uint16_t magic = unibi_read_u16(p + 0);
	FAIL_IF(magic != MAGIC_16BIT && magic != MAGIC_32BIT, EINVAL);
	size_t numsize = magic == MAGIC_16BIT ? 2 : 4;

	uint16_t namlen  = unibi_read_u16(p + 2);
	uint16_t boollen = unibi_read_u16(p + 4);
	uint16_t numlen  = unibi_read_u16(p + 6);
	uint16_t strslen = unibi_read_u16(p + 8);
	uint16_t tablsz  = unibi_read_u16(p + 10);
	p += 12;
	n -= 12;

	if (n < namlen)
		return 0;

	size_t namco = 1;
	for (size_t j = 0; j < namlen; j++)
		if (p[j] == '|')
			namco++;

	unibi_term *result = calloc(1, sizeof(unibi_term) + namco * sizeof(*result->aliases) + tablsz + namlen + 1);
	if (!result)
		return 0;
	result->aliases = (void *)(result + 1);

	char *strp = (char *)(result + 1) + namco * sizeof(*result->aliases);
	char *namp = strp + tablsz;
	memcpy(namp, p, namlen);
	namp[namlen] = 0;
	p += namlen;
	n -= namlen;

	{
		size_t k = 0;
		char *a = namp, *z;
		while ((z = strchr(a, '|'))) {
			*z = 0;
			result->aliases[k++] = a;
			a = z + 1;
		}
		assert(k < namco);
		result->aliases[k] = 0;
		result->name = a;
	}

	DEL_FAIL_IF(n < boollen, EFAULT, result);

	for (size_t i = 0; i < boollen && i / CHAR_BIT < countof(result->bools); i++)
		if (p[i])
			result->bools[i / CHAR_BIT] |= 1 << i % CHAR_BIT;

	p += boollen;
	n -= boollen;

	if ((namlen + boollen) % 2 && n > 0) {
		p += 1;
		n -= 1;
	}

	DEL_FAIL_IF(n < numlen * numsize, EFAULT, result);

	{
		size_t i;
		for (i = 0; i < numlen && i < countof(result->nums); i++) {
			if (numsize == 2) {
				result->nums[i] = unibi_read_s16(p + i * 2);
			} else {
				result->nums[i] = unibi_read_s32(p + i * 4);
			}
		}
		for (size_t j = 0; j < countof(result->nums) - i; j++)
			result->nums[i + j] = -1;
	}

	p += numlen * numsize;
	n -= numlen * numsize;

	DEL_FAIL_IF(n < strslen * 2u, EFAULT, result);
	{
		size_t i;
		for (i = 0; i < strslen && i < countof(result->strs); i++) {
			int16_t value = unibi_read_s16(p + i * 2);
			result->strs[i] = (value < 0 || (size_t)value >= tablsz) ? 0 : strp + value;
		}
		for (size_t j = 0; j < countof(result->strs) - i; j++)
			result->strs[i + j] = 0;
	}

	p += strslen * 2;
	n -= strslen * 2;

	DEL_FAIL_IF(n < tablsz, EFAULT, result);
	if (tablsz) {
		memcpy(strp, p, tablsz);
		strp[tablsz - 1] = 0;
	}
	return result;
}

#undef FAIL_IF
#undef FAIL_IF_
#undef DEL_FAIL_IF

UNIBI_EXPORT unibi_term *
unibi_from_fd(int fd)
{
	char buf[4096];
	int64_t r;

	size_t n = 0;
	while (n < sizeof(buf) && (r = read(fd, buf + n, sizeof buf - n)) > 0)
		n += r;

	if (r < 0)
		return 0;

	return unibi_from_mem(buf, n);
}

UNIBI_EXPORT unibi_term *
unibi_from_file(const char *file)
{
	unibi_term *result = 0;
	int fd = open(file, O_RDONLY);
	if (fd >= 0) {
		result = unibi_from_fd(fd);
		close(fd);
	}
	return result;
}

static unibi_term *
unibi_from_dir(const char *dir_begin, const char *dir_end, const char *mid, const char *term)
{
	size_t dir_len  = dir_end ? (size_t)(dir_end - dir_begin) : strlen(dir_begin);
	size_t mid_len  = mid ? strlen(mid) + 1 : 0;
	size_t term_len = strlen(term);

	size_t path_size = 0;
	if (!addu(path_size, dir_len,  &path_size) ||
	    !addu(path_size, mid_len,  &path_size) ||
	    !addu(path_size, term_len, &path_size) ||
	    !addu(path_size, 4,        &path_size))
	{
		return 0;
	}

	unibi_term *result = 0;
	char path[PATH_MAX];
	if (path_size < countof(path)) {
		size_t off = dir_len;
		memcpy(path, dir_begin, off);
		path[off++] = '/';
		if (mid) {
			memcpy(path + off, mid, mid_len - 1);
			off += mid_len - 1;
			path[off++] = '/';
		}
		path[off++] = term[0];
		path[off++] = '/';
		memcpy(path + off, term, term_len);
		path[off + term_len] = 0;

		errno = 0;
		result = unibi_from_file(path);
		if (!result && errno == ENOENT) {
			/* OS X likes to use /usr/share/terminfo/<hexcode>/name instead of the first letter */
			const char *hex = "0123456789abcdef";
			off = dir_len + 1 + mid_len;
			path[off++] = hex[(unsigned char)term[0] & 0xf0u];
			path[off++] = hex[(unsigned char)term[0] & 0x0fu];
			path[off++] = '/';
			memcpy(path + off, term, term_len);
			path[off + term_len] = 0;
			result = unibi_from_file(path);
		}
	}
	return result;
}

static unibi_term *
unibi_from_dirs(const char *list, const char *term)
{
	if (list[0] == 0) {
		errno = ENOENT;
		return 0;
	}

	const char *const unibi_terminfo = TERMINFO;
	int unibi_terminfo_searched      = unibi_terminfo[0] == 0;

	const char *a = list;
	unibi_term *result = 0;
	for (;;) {
		const char *z = strchr(a, ':');

		/* Empty entry can be either:
		 *  - iterator is on the delimiter, i.e. delimiter starts the list or adjacent,
		 *  - list ends with delimiter. */
		if (a != z && *a != 0) {
			result = unibi_from_dir(a, z, 0, term);
		} else if (!unibi_terminfo_searched) {
			result = unibi_from_dir(unibi_terminfo, 0, 0, term);
			unibi_terminfo_searched = 1;
		} else {
			errno = 0;
		}

		if (result)
			break;

		/* If end of list */
		if (!z) {
			errno = ENOENT;
			break;
		}

		a = z + 1;
	}
	return result;
}

UNIBI_EXPORT unibi_term *
unibi_from_term(const char *term)
{
	assert(term != 0);

	unibi_term *result = 0;

	if (term[0] == 0 || term[0] == '.' || strchr(term, '/')) {
		errno = EINVAL;
		return 0;
	}

	const char *env;
	if ((env = getenv("TERMINFO"))) {
		result = unibi_from_dir(env, 0, 0, term);
		if (result) return result;
	}

	if ((env = getenv("HOME"))) {
		result = unibi_from_dir(env, 0, ".terminfo", term);
		if (result) return result;
	}

	if ((env = getenv("TERMINFO_DIRS"))) {
		result =  unibi_from_dirs(env, term);
		if (result) return result;
	}

	return unibi_from_dirs(TERMINFO_DIRS, term);
}

UNIBI_EXPORT const char *
unibi_get_str(const unibi_term *t, enum unibi_string v)
{
	assert(v > unibi_string_begin_ && v < unibi_string_end_);
	if (v > unibi_string_begin_ && v < unibi_string_end_)
		return t->strs[v - unibi_string_begin_ - 1];
	return 0;
}
