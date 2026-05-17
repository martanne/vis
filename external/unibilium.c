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
	char *alloc;
} unibi_term;

static const char *unibi_names_str[][2] = {
	{"cbt"     , "back_tab"                 },
	{"bel"     , "bell"                     },
	{"cr"      , "carriage_return"          },
	{"csr"     , "change_scroll_region"     },
	{"tbc"     , "clear_all_tabs"           },
	{"clear"   , "clear_screen"             },
	{"el"      , "clr_eol"                  },
	{"ed"      , "clr_eos"                  },
	{"hpa"     , "column_address"           },
	{"cmdch"   , "command_character"        },
	{"cup"     , "cursor_address"           },
	{"cud1"    , "cursor_down"              },
	{"home"    , "cursor_home"              },
	{"civis"   , "cursor_invisible"         },
	{"cub1"    , "cursor_left"              },
	{"mrcup"   , "cursor_mem_address"       },
	{"cnorm"   , "cursor_normal"            },
	{"cuf1"    , "cursor_right"             },
	{"ll"      , "cursor_to_ll"             },
	{"cuu1"    , "cursor_up"                },
	{"cvvis"   , "cursor_visible"           },
	{"dch1"    , "delete_character"         },
	{"dl1"     , "delete_line"              },
	{"dsl"     , "dis_status_line"          },
	{"hd"      , "down_half_line"           },
	{"smacs"   , "enter_alt_charset_mode"   },
	{"blink"   , "enter_blink_mode"         },
	{"bold"    , "enter_bold_mode"          },
	{"smcup"   , "enter_ca_mode"            },
	{"smdc"    , "enter_delete_mode"        },
	{"dim"     , "enter_dim_mode"           },
	{"smir"    , "enter_insert_mode"        },
	{"invis"   , "enter_secure_mode"        },
	{"prot"    , "enter_protected_mode"     },
	{"rev"     , "enter_reverse_mode"       },
	{"smso"    , "enter_standout_mode"      },
	{"smul"    , "enter_underline_mode"     },
	{"ech"     , "erase_chars"              },
	{"rmacs"   , "exit_alt_charset_mode"    },
	{"sgr0"    , "exit_attribute_mode"      },
	{"rmcup"   , "exit_ca_mode"             },
	{"rmdc"    , "exit_delete_mode"         },
	{"rmir"    , "exit_insert_mode"         },
	{"rmso"    , "exit_standout_mode"       },
	{"rmul"    , "exit_underline_mode"      },
	{"flash"   , "flash_screen"             },
	{"ff"      , "form_feed"                },
	{"fsl"     , "from_status_line"         },
	{"is1"     , "init_1string"             },
	{"is2"     , "init_2string"             },
	{"is3"     , "init_3string"             },
	{"if"      , "init_file"                },
	{"ich1"    , "insert_character"         },
	{"il1"     , "insert_line"              },
	{"ip"      , "insert_padding"           },
	{"kbs"     , "key_backspace"            },
	{"ktbc"    , "key_catab"                },
	{"kclr"    , "key_clear"                },
	{"kctab"   , "key_ctab"                 },
	{"kdch1"   , "key_dc"                   },
	{"kdl1"    , "key_dl"                   },
	{"kcud1"   , "key_down"                 },
	{"krmir"   , "key_eic"                  },
	{"kel"     , "key_eol"                  },
	{"ked"     , "key_eos"                  },
	{"kf0"     , "key_f0"                   },
	{"kf1"     , "key_f1"                   },
	{"kf10"    , "key_f10"                  },
	{"kf2"     , "key_f2"                   },
	{"kf3"     , "key_f3"                   },
	{"kf4"     , "key_f4"                   },
	{"kf5"     , "key_f5"                   },
	{"kf6"     , "key_f6"                   },
	{"kf7"     , "key_f7"                   },
	{"kf8"     , "key_f8"                   },
	{"kf9"     , "key_f9"                   },
	{"khome"   , "key_home"                 },
	{"kich1"   , "key_ic"                   },
	{"kil1"    , "key_il"                   },
	{"kcub1"   , "key_left"                 },
	{"kll"     , "key_ll"                   },
	{"knp"     , "key_npage"                },
	{"kpp"     , "key_ppage"                },
	{"kcuf1"   , "key_right"                },
	{"kind"    , "key_sf"                   },
	{"kri"     , "key_sr"                   },
	{"khts"    , "key_stab"                 },
	{"kcuu1"   , "key_up"                   },
	{"rmkx"    , "keypad_local"             },
	{"smkx"    , "keypad_xmit"              },
	{"lf0"     , "lab_f0"                   },
	{"lf1"     , "lab_f1"                   },
	{"lf10"    , "lab_f10"                  },
	{"lf2"     , "lab_f2"                   },
	{"lf3"     , "lab_f3"                   },
	{"lf4"     , "lab_f4"                   },
	{"lf5"     , "lab_f5"                   },
	{"lf6"     , "lab_f6"                   },
	{"lf7"     , "lab_f7"                   },
	{"lf8"     , "lab_f8"                   },
	{"lf9"     , "lab_f9"                   },
	{"rmm"     , "meta_off"                 },
	{"smm"     , "meta_on"                  },
	{"nel"     , "newline"                  },
	{"pad"     , "pad_char"                 },
	{"dch"     , "parm_dch"                 },
	{"dl"      , "parm_delete_line"         },
	{"cud"     , "parm_down_cursor"         },
	{"ich"     , "parm_ich"                 },
	{"indn"    , "parm_index"               },
	{"il"      , "parm_insert_line"         },
	{"cub"     , "parm_left_cursor"         },
	{"cuf"     , "parm_right_cursor"        },
	{"rin"     , "parm_rindex"              },
	{"cuu"     , "parm_up_cursor"           },
	{"pfkey"   , "pkey_key"                 },
	{"pfloc"   , "pkey_local"               },
	{"pfx"     , "pkey_xmit"                },
	{"mc0"     , "print_screen"             },
	{"mc4"     , "prtr_off"                 },
	{"mc5"     , "prtr_on"                  },
	{"rep"     , "repeat_char"              },
	{"rs1"     , "reset_1string"            },
	{"rs2"     , "reset_2string"            },
	{"rs3"     , "reset_3string"            },
	{"rf"      , "reset_file"               },
	{"rc"      , "restore_cursor"           },
	{"vpa"     , "row_address"              },
	{"sc"      , "save_cursor"              },
	{"ind"     , "scroll_forward"           },
	{"ri"      , "scroll_reverse"           },
	{"sgr"     , "set_attributes"           },
	{"hts"     , "set_tab"                  },
	{"wind"    , "set_window"               },
	{"ht"      , "tab"                      },
	{"tsl"     , "to_status_line"           },
	{"uc"      , "underline_char"           },
	{"hu"      , "up_half_line"             },
	{"iprog"   , "init_prog"                },
	{"ka1"     , "key_a1"                   },
	{"ka3"     , "key_a3"                   },
	{"kb2"     , "key_b2"                   },
	{"kc1"     , "key_c1"                   },
	{"kc3"     , "key_c3"                   },
	{"mc5p"    , "prtr_non"                 },
	{"rmp"     , "char_padding"             },
	{"acsc"    , "acs_chars"                },
	{"pln"     , "plab_norm"                },
	{"kcbt"    , "key_btab"                 },
	{"smxon"   , "enter_xon_mode"           },
	{"rmxon"   , "exit_xon_mode"            },
	{"smam"    , "enter_am_mode"            },
	{"rmam"    , "exit_am_mode"             },
	{"xonc"    , "xon_character"            },
	{"xoffc"   , "xoff_character"           },
	{"enacs"   , "ena_acs"                  },
	{"smln"    , "label_on"                 },
	{"rmln"    , "label_off"                },
	{"kbeg"    , "key_beg"                  },
	{"kcan"    , "key_cancel"               },
	{"kclo"    , "key_close"                },
	{"kcmd"    , "key_command"              },
	{"kcpy"    , "key_copy"                 },
	{"kcrt"    , "key_create"               },
	{"kend"    , "key_end"                  },
	{"kent"    , "key_enter"                },
	{"kext"    , "key_exit"                 },
	{"kfnd"    , "key_find"                 },
	{"khlp"    , "key_help"                 },
	{"kmrk"    , "key_mark"                 },
	{"kmsg"    , "key_message"              },
	{"kmov"    , "key_move"                 },
	{"knxt"    , "key_next"                 },
	{"kopn"    , "key_open"                 },
	{"kopt"    , "key_options"              },
	{"kprv"    , "key_previous"             },
	{"kprt"    , "key_print"                },
	{"krdo"    , "key_redo"                 },
	{"kref"    , "key_reference"            },
	{"krfr"    , "key_refresh"              },
	{"krpl"    , "key_replace"              },
	{"krst"    , "key_restart"              },
	{"kres"    , "key_resume"               },
	{"ksav"    , "key_save"                 },
	{"kspd"    , "key_suspend"              },
	{"kund"    , "key_undo"                 },
	{"kBEG"    , "key_sbeg"                 },
	{"kCAN"    , "key_scancel"              },
	{"kCMD"    , "key_scommand"             },
	{"kCPY"    , "key_scopy"                },
	{"kCRT"    , "key_screate"              },
	{"kDC"     , "key_sdc"                  },
	{"kDL"     , "key_sdl"                  },
	{"kslt"    , "key_select"               },
	{"kEND"    , "key_send"                 },
	{"kEOL"    , "key_seol"                 },
	{"kEXT"    , "key_sexit"                },
	{"kFND"    , "key_sfind"                },
	{"kHLP"    , "key_shelp"                },
	{"kHOM"    , "key_shome"                },
	{"kIC"     , "key_sic"                  },
	{"kLFT"    , "key_sleft"                },
	{"kMSG"    , "key_smessage"             },
	{"kMOV"    , "key_smove"                },
	{"kNXT"    , "key_snext"                },
	{"kOPT"    , "key_soptions"             },
	{"kPRV"    , "key_sprevious"            },
	{"kPRT"    , "key_sprint"               },
	{"kRDO"    , "key_sredo"                },
	{"kRPL"    , "key_sreplace"             },
	{"kRIT"    , "key_sright"               },
	{"kRES"    , "key_srsume"               },
	{"kSAV"    , "key_ssave"                },
	{"kSPD"    , "key_ssuspend"             },
	{"kUND"    , "key_sundo"                },
	{"rfi"     , "req_for_input"            },
	{"kf11"    , "key_f11"                  },
	{"kf12"    , "key_f12"                  },
	{"kf13"    , "key_f13"                  },
	{"kf14"    , "key_f14"                  },
	{"kf15"    , "key_f15"                  },
	{"kf16"    , "key_f16"                  },
	{"kf17"    , "key_f17"                  },
	{"kf18"    , "key_f18"                  },
	{"kf19"    , "key_f19"                  },
	{"kf20"    , "key_f20"                  },
	{"kf21"    , "key_f21"                  },
	{"kf22"    , "key_f22"                  },
	{"kf23"    , "key_f23"                  },
	{"kf24"    , "key_f24"                  },
	{"kf25"    , "key_f25"                  },
	{"kf26"    , "key_f26"                  },
	{"kf27"    , "key_f27"                  },
	{"kf28"    , "key_f28"                  },
	{"kf29"    , "key_f29"                  },
	{"kf30"    , "key_f30"                  },
	{"kf31"    , "key_f31"                  },
	{"kf32"    , "key_f32"                  },
	{"kf33"    , "key_f33"                  },
	{"kf34"    , "key_f34"                  },
	{"kf35"    , "key_f35"                  },
	{"kf36"    , "key_f36"                  },
	{"kf37"    , "key_f37"                  },
	{"kf38"    , "key_f38"                  },
	{"kf39"    , "key_f39"                  },
	{"kf40"    , "key_f40"                  },
	{"kf41"    , "key_f41"                  },
	{"kf42"    , "key_f42"                  },
	{"kf43"    , "key_f43"                  },
	{"kf44"    , "key_f44"                  },
	{"kf45"    , "key_f45"                  },
	{"kf46"    , "key_f46"                  },
	{"kf47"    , "key_f47"                  },
	{"kf48"    , "key_f48"                  },
	{"kf49"    , "key_f49"                  },
	{"kf50"    , "key_f50"                  },
	{"kf51"    , "key_f51"                  },
	{"kf52"    , "key_f52"                  },
	{"kf53"    , "key_f53"                  },
	{"kf54"    , "key_f54"                  },
	{"kf55"    , "key_f55"                  },
	{"kf56"    , "key_f56"                  },
	{"kf57"    , "key_f57"                  },
	{"kf58"    , "key_f58"                  },
	{"kf59"    , "key_f59"                  },
	{"kf60"    , "key_f60"                  },
	{"kf61"    , "key_f61"                  },
	{"kf62"    , "key_f62"                  },
	{"kf63"    , "key_f63"                  },
	{"el1"     , "clr_bol"                  },
	{"mgc"     , "clear_margins"            },
	{"smgl"    , "set_left_margin"          },
	{"smgr"    , "set_right_margin"         },
	{"fln"     , "label_format"             },
	{"sclk"    , "set_clock"                },
	{"dclk"    , "display_clock"            },
	{"rmclk"   , "remove_clock"             },
	{"cwin"    , "create_window"            },
	{"wingo"   , "goto_window"              },
	{"hup"     , "hangup"                   },
	{"dial"    , "dial_phone"               },
	{"qdial"   , "quick_dial"               },
	{"tone"    , "tone"                     },
	{"pulse"   , "pulse"                    },
	{"hook"    , "flash_hook"               },
	{"pause"   , "fixed_pause"              },
	{"wait"    , "wait_tone"                },
	{"u0"      , "user0"                    },
	{"u1"      , "user1"                    },
	{"u2"      , "user2"                    },
	{"u3"      , "user3"                    },
	{"u4"      , "user4"                    },
	{"u5"      , "user5"                    },
	{"u6"      , "user6"                    },
	{"u7"      , "user7"                    },
	{"u8"      , "user8"                    },
	{"u9"      , "user9"                    },
	{"op"      , "orig_pair"                },
	{"oc"      , "orig_colors"              },
	{"initc"   , "initialize_color"         },
	{"initp"   , "initialize_pair"          },
	{"scp"     , "set_color_pair"           },
	{"setf"    , "set_foreground"           },
	{"setb"    , "set_background"           },
	{"cpi"     , "change_char_pitch"        },
	{"lpi"     , "change_line_pitch"        },
	{"chr"     , "change_res_horz"          },
	{"cvr"     , "change_res_vert"          },
	{"defc"    , "define_char"              },
	{"swidm"   , "enter_doublewide_mode"    },
	{"sdrfq"   , "enter_draft_quality"      },
	{"sitm"    , "enter_italics_mode"       },
	{"slm"     , "enter_leftward_mode"      },
	{"smicm"   , "enter_micro_mode"         },
	{"snlq"    , "enter_near_letter_quality"},
	{"snrmq"   , "enter_normal_quality"     },
	{"sshm"    , "enter_shadow_mode"        },
	{"ssubm"   , "enter_subscript_mode"     },
	{"ssupm"   , "enter_superscript_mode"   },
	{"sum"     , "enter_upward_mode"        },
	{"rwidm"   , "exit_doublewide_mode"     },
	{"ritm"    , "exit_italics_mode"        },
	{"rlm"     , "exit_leftward_mode"       },
	{"rmicm"   , "exit_micro_mode"          },
	{"rshm"    , "exit_shadow_mode"         },
	{"rsubm"   , "exit_subscript_mode"      },
	{"rsupm"   , "exit_superscript_mode"    },
	{"rum"     , "exit_upward_mode"         },
	{"mhpa"    , "micro_column_address"     },
	{"mcud1"   , "micro_down"               },
	{"mcub1"   , "micro_left"               },
	{"mcuf1"   , "micro_right"              },
	{"mvpa"    , "micro_row_address"        },
	{"mcuu1"   , "micro_up"                 },
	{"porder"  , "order_of_pins"            },
	{"mcud"    , "parm_down_micro"          },
	{"mcub"    , "parm_left_micro"          },
	{"mcuf"    , "parm_right_micro"         },
	{"mcuu"    , "parm_up_micro"            },
	{"scs"     , "select_char_set"          },
	{"smgb"    , "set_bottom_margin"        },
	{"smgbp"   , "set_bottom_margin_parm"   },
	{"smglp"   , "set_left_margin_parm"     },
	{"smgrp"   , "set_right_margin_parm"    },
	{"smgt"    , "set_top_margin"           },
	{"smgtp"   , "set_top_margin_parm"      },
	{"sbim"    , "start_bit_image"          },
	{"scsd"    , "start_char_set_def"       },
	{"rbim"    , "stop_bit_image"           },
	{"rcsd"    , "stop_char_set_def"        },
	{"subcs"   , "subscript_characters"     },
	{"supcs"   , "superscript_characters"   },
	{"docr"    , "these_cause_cr"           },
	{"zerom"   , "zero_motion"              },
	{"csnm"    , "char_set_names"           },
	{"kmous"   , "key_mouse"                },
	{"minfo"   , "mouse_info"               },
	{"reqmp"   , "req_mouse_pos"            },
	{"getm"    , "get_mouse"                },
	{"setaf"   , "set_a_foreground"         },
	{"setab"   , "set_a_background"         },
	{"pfxl"    , "pkey_plab"                },
	{"devt"    , "device_type"              },
	{"csin"    , "code_set_init"            },
	{"s0ds"    , "set0_des_seq"             },
	{"s1ds"    , "set1_des_seq"             },
	{"s2ds"    , "set2_des_seq"             },
	{"s3ds"    , "set3_des_seq"             },
	{"smglr"   , "set_lr_margin"            },
	{"smgtb"   , "set_tb_margin"            },
	{"birep"   , "bit_image_repeat"         },
	{"binel"   , "bit_image_newline"        },
	{"bicr"    , "bit_image_carriage_return"},
	{"colornm" , "color_names"              },
	{"defbi"   , "define_bit_image_region"  },
	{"endbi"   , "end_bit_image_region"     },
	{"setcolor", "set_color_band"           },
	{"slines"  , "set_page_length"          },
	{"dispc"   , "display_pc_char"          },
	{"smpch"   , "enter_pc_charset_mode"    },
	{"rmpch"   , "exit_pc_charset_mode"     },
	{"smsc"    , "enter_scancode_mode"      },
	{"rmsc"    , "exit_scancode_mode"       },
	{"pctrm"   , "pc_term_options"          },
	{"scesc"   , "scancode_escape"          },
	{"scesa"   , "alt_scancode_esc"         },
	{"ehhlm"   , "enter_horizontal_hl_mode" },
	{"elhlm"   , "enter_left_hl_mode"       },
	{"elohlm"  , "enter_low_hl_mode"        },
	{"erhlm"   , "enter_right_hl_mode"      },
	{"ethlm"   , "enter_top_hl_mode"        },
	{"evhlm"   , "enter_vertical_hl_mode"   },
	{"sgr1"    , "set_a_attributes"         },
	{"slength" , "set_pglen_inch"           },
	{"OTi2"    , "termcap_init2"            },
	{"OTrs"    , "termcap_reset"            },
	{"OTnl"    , "linefeed_if_not_lf"       },
	{"OTbc"    , "backspace_if_not_bs"      },
	{"OTko"    , "other_non_function_keys"  },
	{"OTma"    , "arrow_key_map"            },
	{"OTG2"    , "acs_ulcorner"             },
	{"OTG3"    , "acs_llcorner"             },
	{"OTG1"    , "acs_urcorner"             },
	{"OTG4"    , "acs_lrcorner"             },
	{"OTGR"    , "acs_ltee"                 },
	{"OTGL"    , "acs_rtee"                 },
	{"OTGU"    , "acs_btee"                 },
	{"OTGD"    , "acs_ttee"                 },
	{"OTGH"    , "acs_hline"                },
	{"OTGV"    , "acs_vline"                },
	{"OTGC"    , "acs_plus"                 },
	{"meml"    , "memory_lock"              },
	{"memu"    , "memory_unlock"            },
	{"box1"    , "box_chars_1"              }
};

static const char *
unibi_x_name_str(enum unibi_string v, int long_name)
{
	assert(v > unibi_string_begin_ && v < unibi_string_end_);
	if (v <= unibi_string_begin_ || v >= unibi_string_end_)
		return 0;
	return unibi_names_str[v - unibi_string_begin_ - 1][long_name ? 1 : 0];
}

UNIBI_EXPORT const char *
unibi_name_str(enum unibi_string v)
{
	return unibi_x_name_str(v, 1);
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
	free(t->alloc);
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

	unibi_term *result = calloc(1, sizeof(unibi_term));
	if (!result)
		return 0;

	result->alloc = calloc(1, namco * sizeof(*result->aliases) + tablsz + namlen + 1);
	if (!result->alloc) {
		free(result);
		return 0;
	}
	result->aliases = (void *)result->alloc;

	char *strp = result->alloc + namco * sizeof(*result->aliases);
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
		/* overflow */
		errno = ENOMEM;
		return 0;
	}

	unibi_term *result = 0;
	char *path = calloc(1, path_size);
	if (path) {
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
		free(path);
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
