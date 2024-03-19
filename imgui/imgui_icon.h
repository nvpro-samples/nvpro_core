/*
 * Copyright (c) 2023, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/// @DOC_SKIP

#pragma once
#include <imgui.h>


namespace ImGuiH {
void    addIconicFont(float fontSize = 14.F);  // Call this once in the application after ImGui is initialized
void    showDemoIcons();                       // Show all icons in a separated window
ImFont* getIconicFont();                       // Return the iconic font
// Ex:  ImGui::PushFont(ImGuiH::getIconicFont());
//      ImGui::Button(ImGuiH::icon_account_login);
//      ImGui::PopFont();

[[maybe_unused]] static const char* icon_account_login            = (char*)u8"\ue000";
[[maybe_unused]] static const char* icon_account_logout           = (char*)u8"\ue001";
[[maybe_unused]] static const char* icon_action_redo              = (char*)u8"\ue002";
[[maybe_unused]] static const char* icon_action_undo              = (char*)u8"\ue003";
[[maybe_unused]] static const char* icon_align_center             = (char*)u8"\ue004";
[[maybe_unused]] static const char* icon_align_left               = (char*)u8"\ue005";
[[maybe_unused]] static const char* icon_align_right              = (char*)u8"\ue006";
[[maybe_unused]] static const char* icon_aperture                 = (char*)u8"\ue007";
[[maybe_unused]] static const char* icon_arrow_bottom             = (char*)u8"\ue008";
[[maybe_unused]] static const char* icon_arrow_circle_bottom      = (char*)u8"\ue009";
[[maybe_unused]] static const char* icon_arrow_circle_left        = (char*)u8"\ue00A";
[[maybe_unused]] static const char* icon_arrow_circle_right       = (char*)u8"\ue00B";
[[maybe_unused]] static const char* icon_arrow_circle_top         = (char*)u8"\ue00C";
[[maybe_unused]] static const char* icon_arrow_left               = (char*)u8"\ue00D";
[[maybe_unused]] static const char* icon_arrow_right              = (char*)u8"\ue00E";
[[maybe_unused]] static const char* icon_arrow_thick_bottom       = (char*)u8"\ue00F";
[[maybe_unused]] static const char* icon_arrow_thick_left         = (char*)u8"\ue010";
[[maybe_unused]] static const char* icon_arrow_thick_right        = (char*)u8"\ue011";
[[maybe_unused]] static const char* icon_arrow_thick_top          = (char*)u8"\ue012";
[[maybe_unused]] static const char* icon_arrow_top                = (char*)u8"\ue013";
[[maybe_unused]] static const char* icon_audio                    = (char*)u8"\ue014";
[[maybe_unused]] static const char* icon_audio_spectrum           = (char*)u8"\ue015";
[[maybe_unused]] static const char* icon_badge                    = (char*)u8"\ue016";
[[maybe_unused]] static const char* icon_ban                      = (char*)u8"\ue017";
[[maybe_unused]] static const char* icon_bar_chart                = (char*)u8"\ue018";
[[maybe_unused]] static const char* icon_basket                   = (char*)u8"\ue019";
[[maybe_unused]] static const char* icon_battery_empty            = (char*)u8"\ue01A";
[[maybe_unused]] static const char* icon_battery_full             = (char*)u8"\ue01B";
[[maybe_unused]] static const char* icon_beaker                   = (char*)u8"\ue01C";
[[maybe_unused]] static const char* icon_bell                     = (char*)u8"\ue01D";
[[maybe_unused]] static const char* icon_bluetooth                = (char*)u8"\ue01E";
[[maybe_unused]] static const char* icon_bold                     = (char*)u8"\ue01F";
[[maybe_unused]] static const char* icon_bolt                     = (char*)u8"\ue020";
[[maybe_unused]] static const char* icon_book                     = (char*)u8"\ue021";
[[maybe_unused]] static const char* icon_bookmark                 = (char*)u8"\ue022";
[[maybe_unused]] static const char* icon_box                      = (char*)u8"\ue023";
[[maybe_unused]] static const char* icon_briefcase                = (char*)u8"\ue024";
[[maybe_unused]] static const char* icon_british_pound            = (char*)u8"\ue025";
[[maybe_unused]] static const char* icon_browser                  = (char*)u8"\ue026";
[[maybe_unused]] static const char* icon_brush                    = (char*)u8"\ue027";
[[maybe_unused]] static const char* icon_bug                      = (char*)u8"\ue028";
[[maybe_unused]] static const char* icon_bullhorn                 = (char*)u8"\ue029";
[[maybe_unused]] static const char* icon_calculator               = (char*)u8"\ue02A";
[[maybe_unused]] static const char* icon_calendar                 = (char*)u8"\ue02B";
[[maybe_unused]] static const char* icon_camera_slr               = (char*)u8"\ue02C";
[[maybe_unused]] static const char* icon_caret_bottom             = (char*)u8"\ue02D";
[[maybe_unused]] static const char* icon_caret_left               = (char*)u8"\ue02E";
[[maybe_unused]] static const char* icon_caret_right              = (char*)u8"\ue02F";
[[maybe_unused]] static const char* icon_caret_top                = (char*)u8"\ue030";
[[maybe_unused]] static const char* icon_cart                     = (char*)u8"\ue031";
[[maybe_unused]] static const char* icon_chat                     = (char*)u8"\ue032";
[[maybe_unused]] static const char* icon_check                    = (char*)u8"\ue033";
[[maybe_unused]] static const char* icon_chevron_bottom           = (char*)u8"\ue034";
[[maybe_unused]] static const char* icon_chevron_left             = (char*)u8"\ue035";
[[maybe_unused]] static const char* icon_chevron_right            = (char*)u8"\ue036";
[[maybe_unused]] static const char* icon_chevron_top              = (char*)u8"\ue037";
[[maybe_unused]] static const char* icon_circle_check             = (char*)u8"\ue038";
[[maybe_unused]] static const char* icon_circle_x                 = (char*)u8"\ue039";
[[maybe_unused]] static const char* icon_clipboard                = (char*)u8"\ue03A";
[[maybe_unused]] static const char* icon_clock                    = (char*)u8"\ue03B";
[[maybe_unused]] static const char* icon_cloud_download           = (char*)u8"\ue03C";
[[maybe_unused]] static const char* icon_cloud_upload             = (char*)u8"\ue03D";
[[maybe_unused]] static const char* icon_cloud                    = (char*)u8"\ue03E";
[[maybe_unused]] static const char* icon_cloudy                   = (char*)u8"\ue03F";
[[maybe_unused]] static const char* icon_code                     = (char*)u8"\ue040";
[[maybe_unused]] static const char* icon_cog                      = (char*)u8"\ue041";
[[maybe_unused]] static const char* icon_collapse_down            = (char*)u8"\ue042";
[[maybe_unused]] static const char* icon_collapse_left            = (char*)u8"\ue043";
[[maybe_unused]] static const char* icon_collapse_right           = (char*)u8"\ue044";
[[maybe_unused]] static const char* icon_collapse_up              = (char*)u8"\ue045";
[[maybe_unused]] static const char* icon_command                  = (char*)u8"\ue046";
[[maybe_unused]] static const char* icon_comment_square           = (char*)u8"\ue047";
[[maybe_unused]] static const char* icon_compass                  = (char*)u8"\ue048";
[[maybe_unused]] static const char* icon_contrast                 = (char*)u8"\ue049";
[[maybe_unused]] static const char* icon_copywriting              = (char*)u8"\ue04A";
[[maybe_unused]] static const char* icon_credit_card              = (char*)u8"\ue04B";
[[maybe_unused]] static const char* icon_crop                     = (char*)u8"\ue04C";
[[maybe_unused]] static const char* icon_dashboard                = (char*)u8"\ue04D";
[[maybe_unused]] static const char* icon_data_transfer_download   = (char*)u8"\ue04E";
[[maybe_unused]] static const char* icon_data_transfer_upload     = (char*)u8"\ue04F";
[[maybe_unused]] static const char* icon_delete                   = (char*)u8"\ue050";
[[maybe_unused]] static const char* icon_dial                     = (char*)u8"\ue051";
[[maybe_unused]] static const char* icon_document                 = (char*)u8"\ue052";
[[maybe_unused]] static const char* icon_dollar                   = (char*)u8"\ue053";
[[maybe_unused]] static const char* icon_double_quote_sans_left   = (char*)u8"\ue054";
[[maybe_unused]] static const char* icon_double_quote_sans_right  = (char*)u8"\ue055";
[[maybe_unused]] static const char* icon_double_quote_serif_left  = (char*)u8"\ue056";
[[maybe_unused]] static const char* icon_double_quote_serif_right = (char*)u8"\ue057";
[[maybe_unused]] static const char* icon_droplet                  = (char*)u8"\ue058";
[[maybe_unused]] static const char* icon_eject                    = (char*)u8"\ue059";
[[maybe_unused]] static const char* icon_elevator                 = (char*)u8"\ue05A";
[[maybe_unused]] static const char* icon_ellipses                 = (char*)u8"\ue05B";
[[maybe_unused]] static const char* icon_envelope_closed          = (char*)u8"\ue05C";
[[maybe_unused]] static const char* icon_envelope_open            = (char*)u8"\ue05D";
[[maybe_unused]] static const char* icon_euro                     = (char*)u8"\ue05E";
[[maybe_unused]] static const char* icon_excerpt                  = (char*)u8"\ue05F";
[[maybe_unused]] static const char* icon_expend_down              = (char*)u8"\ue060";
[[maybe_unused]] static const char* icon_expend_left              = (char*)u8"\ue061";
[[maybe_unused]] static const char* icon_expend_right             = (char*)u8"\ue062";
[[maybe_unused]] static const char* icon_expend_up                = (char*)u8"\ue063";
[[maybe_unused]] static const char* icon_external_link            = (char*)u8"\ue064";
[[maybe_unused]] static const char* icon_eye                      = (char*)u8"\ue065";
[[maybe_unused]] static const char* icon_eyedropper               = (char*)u8"\ue066";
[[maybe_unused]] static const char* icon_file                     = (char*)u8"\ue067";
[[maybe_unused]] static const char* icon_fire                     = (char*)u8"\ue068";
[[maybe_unused]] static const char* icon_flag                     = (char*)u8"\ue069";
[[maybe_unused]] static const char* icon_flash                    = (char*)u8"\ue06A";
[[maybe_unused]] static const char* icon_folder                   = (char*)u8"\ue06B";
[[maybe_unused]] static const char* icon_fork                     = (char*)u8"\ue06C";
[[maybe_unused]] static const char* icon_fullscreen_enter         = (char*)u8"\ue06D";
[[maybe_unused]] static const char* icon_fullscreen_exit          = (char*)u8"\ue06E";
[[maybe_unused]] static const char* icon_globe                    = (char*)u8"\ue06F";
[[maybe_unused]] static const char* icon_graph                    = (char*)u8"\ue070";
[[maybe_unused]] static const char* icon_grid_four_up             = (char*)u8"\ue071";
[[maybe_unused]] static const char* icon_grid_three_up            = (char*)u8"\ue072";
[[maybe_unused]] static const char* icon_grid_two_up              = (char*)u8"\ue073";
[[maybe_unused]] static const char* icon_hard_drive               = (char*)u8"\ue074";
[[maybe_unused]] static const char* icon_header                   = (char*)u8"\ue075";
[[maybe_unused]] static const char* icon_headphones               = (char*)u8"\ue076";
[[maybe_unused]] static const char* icon_heart                    = (char*)u8"\ue077";
[[maybe_unused]] static const char* icon_home                     = (char*)u8"\ue078";
[[maybe_unused]] static const char* icon_image                    = (char*)u8"\ue079";
[[maybe_unused]] static const char* icon_inbox                    = (char*)u8"\ue07A";
[[maybe_unused]] static const char* icon_infinity                 = (char*)u8"\ue07B";
[[maybe_unused]] static const char* icon_info                     = (char*)u8"\ue07C";
[[maybe_unused]] static const char* icon_italic                   = (char*)u8"\ue07D";
[[maybe_unused]] static const char* icon_justify_center           = (char*)u8"\ue07E";
[[maybe_unused]] static const char* icon_justify_left             = (char*)u8"\ue07F";
[[maybe_unused]] static const char* icon_justify_right            = (char*)u8"\ue080";
[[maybe_unused]] static const char* icon_key                      = (char*)u8"\ue081";
[[maybe_unused]] static const char* icon_laptop                   = (char*)u8"\ue082";
[[maybe_unused]] static const char* icon_layers                   = (char*)u8"\ue083";
[[maybe_unused]] static const char* icon_lightbulb                = (char*)u8"\ue084";
[[maybe_unused]] static const char* icon_link_broken              = (char*)u8"\ue085";
[[maybe_unused]] static const char* icon_link_intact              = (char*)u8"\ue086";
[[maybe_unused]] static const char* icon_list                     = (char*)u8"\ue087";
[[maybe_unused]] static const char* icon_list_rich                = (char*)u8"\ue088";
[[maybe_unused]] static const char* icon_location                 = (char*)u8"\ue089";
[[maybe_unused]] static const char* icon_lock_locked              = (char*)u8"\ue08A";
[[maybe_unused]] static const char* icon_lock_unlocked            = (char*)u8"\ue08B";
[[maybe_unused]] static const char* icon_loop_circular            = (char*)u8"\ue08C";
[[maybe_unused]] static const char* icon_loop_square              = (char*)u8"\ue08D";
[[maybe_unused]] static const char* icon_loop                     = (char*)u8"\ue08E";
[[maybe_unused]] static const char* icon_magnifying_glass         = (char*)u8"\ue08F";
[[maybe_unused]] static const char* icon_map                      = (char*)u8"\ue090";
[[maybe_unused]] static const char* icon_map_marquer              = (char*)u8"\ue091";
[[maybe_unused]] static const char* icon_media_pause              = (char*)u8"\ue092";
[[maybe_unused]] static const char* icon_media_play               = (char*)u8"\ue093";
[[maybe_unused]] static const char* icon_media_record             = (char*)u8"\ue094";
[[maybe_unused]] static const char* icon_media_skip_backward      = (char*)u8"\ue095";
[[maybe_unused]] static const char* icon_media_skip_forward       = (char*)u8"\ue096";
[[maybe_unused]] static const char* icon_media_step_backward      = (char*)u8"\ue097";
[[maybe_unused]] static const char* icon_media_step_forward       = (char*)u8"\ue098";
[[maybe_unused]] static const char* icon_media_stop               = (char*)u8"\ue099";
[[maybe_unused]] static const char* icon_medical_cross            = (char*)u8"\ue09A";
[[maybe_unused]] static const char* icon_menu                     = (char*)u8"\ue09B";
[[maybe_unused]] static const char* icon_microphone               = (char*)u8"\ue09C";
[[maybe_unused]] static const char* icon_minus                    = (char*)u8"\ue09D";
[[maybe_unused]] static const char* icon_monitor                  = (char*)u8"\ue09E";
[[maybe_unused]] static const char* icon_moon                     = (char*)u8"\ue09F";
[[maybe_unused]] static const char* icon_move                     = (char*)u8"\ue0A0";
[[maybe_unused]] static const char* icon_musical_note             = (char*)u8"\ue0A1";
[[maybe_unused]] static const char* icon_paperclip                = (char*)u8"\ue0A2";
[[maybe_unused]] static const char* icon_pencil                   = (char*)u8"\ue0A3";
[[maybe_unused]] static const char* icon_people                   = (char*)u8"\ue0A4";
[[maybe_unused]] static const char* icon_person                   = (char*)u8"\ue0A5";
[[maybe_unused]] static const char* icon_phone                    = (char*)u8"\ue0A6";
[[maybe_unused]] static const char* icon_pie_chart                = (char*)u8"\ue0A7";
[[maybe_unused]] static const char* icon_pin                      = (char*)u8"\ue0A8";
[[maybe_unused]] static const char* icon_play_circle              = (char*)u8"\ue0A9";
[[maybe_unused]] static const char* icon_plus                     = (char*)u8"\ue0AA";
[[maybe_unused]] static const char* icon_power_standby            = (char*)u8"\ue0AB";
[[maybe_unused]] static const char* icon_print                    = (char*)u8"\ue0AC";
[[maybe_unused]] static const char* icon_project                  = (char*)u8"\ue0AD";
[[maybe_unused]] static const char* icon_pulse                    = (char*)u8"\ue0AE";
[[maybe_unused]] static const char* icon_puzzle_piece             = (char*)u8"\ue0AF";
[[maybe_unused]] static const char* icon_question_mark            = (char*)u8"\ue0B0";
[[maybe_unused]] static const char* icon_rain                     = (char*)u8"\ue0B1";
[[maybe_unused]] static const char* icon_random                   = (char*)u8"\ue0B2";
[[maybe_unused]] static const char* icon_reload                   = (char*)u8"\ue0B3";
[[maybe_unused]] static const char* icon_resize_both              = (char*)u8"\ue0B4";
[[maybe_unused]] static const char* icon_resize_height            = (char*)u8"\ue0B5";
[[maybe_unused]] static const char* icon_resize_width             = (char*)u8"\ue0B6";
[[maybe_unused]] static const char* icon_rss                      = (char*)u8"\ue0B7";
[[maybe_unused]] static const char* icon_rss_alt                  = (char*)u8"\ue0B8";
[[maybe_unused]] static const char* icon_script                   = (char*)u8"\ue0B9";
[[maybe_unused]] static const char* icon_share                    = (char*)u8"\ue0BA";
[[maybe_unused]] static const char* icon_share_boxed              = (char*)u8"\ue0BB";
[[maybe_unused]] static const char* icon_shield                   = (char*)u8"\ue0BC";
[[maybe_unused]] static const char* icon_signal                   = (char*)u8"\ue0BD";
[[maybe_unused]] static const char* icon_signpost                 = (char*)u8"\ue0BE";
[[maybe_unused]] static const char* icon_sort_ascending           = (char*)u8"\ue0BF";
[[maybe_unused]] static const char* icon_sort_descending          = (char*)u8"\ue0C0";
[[maybe_unused]] static const char* icon_spreadsheet              = (char*)u8"\ue0C1";
[[maybe_unused]] static const char* icon_star                     = (char*)u8"\ue0C2";
[[maybe_unused]] static const char* icon_sun                      = (char*)u8"\ue0C3";
[[maybe_unused]] static const char* icon_tablet                   = (char*)u8"\ue0C4";
[[maybe_unused]] static const char* icon_tag                      = (char*)u8"\ue0C5";
[[maybe_unused]] static const char* icon_tags                     = (char*)u8"\ue0C6";
[[maybe_unused]] static const char* icon_target                   = (char*)u8"\ue0C7";
[[maybe_unused]] static const char* icon_task                     = (char*)u8"\ue0C8";
[[maybe_unused]] static const char* icon_terminal                 = (char*)u8"\ue0C9";
[[maybe_unused]] static const char* icon_text                     = (char*)u8"\ue0CA";
[[maybe_unused]] static const char* icon_thumb_down               = (char*)u8"\ue0CB";
[[maybe_unused]] static const char* icon_thumb_up                 = (char*)u8"\ue0CC";
[[maybe_unused]] static const char* icon_timer                    = (char*)u8"\ue0CD";
[[maybe_unused]] static const char* icon_transfer                 = (char*)u8"\ue0CE";
[[maybe_unused]] static const char* icon_trash                    = (char*)u8"\ue0CF";
[[maybe_unused]] static const char* icon_underline                = (char*)u8"\ue0D0";
[[maybe_unused]] static const char* icon_vertical_align_bottom    = (char*)u8"\ue0D1";
[[maybe_unused]] static const char* icon_vertical_align_center    = (char*)u8"\ue0D2";
[[maybe_unused]] static const char* icon_vertical_align_top       = (char*)u8"\ue0D3";
[[maybe_unused]] static const char* icon_video                    = (char*)u8"\ue0D4";
[[maybe_unused]] static const char* icon_volume_high              = (char*)u8"\ue0D5";
[[maybe_unused]] static const char* icon_volume_low               = (char*)u8"\ue0D6";
[[maybe_unused]] static const char* icon_volume_off               = (char*)u8"\ue0D7";
[[maybe_unused]] static const char* icon_warning                  = (char*)u8"\ue0D8";
[[maybe_unused]] static const char* icon_wifi                     = (char*)u8"\ue0D9";
[[maybe_unused]] static const char* icon_wrench                   = (char*)u8"\ue0DA";
[[maybe_unused]] static const char* icon_x                        = (char*)u8"\ue0DB";
[[maybe_unused]] static const char* icon_yen                      = (char*)u8"\ue0DC";
[[maybe_unused]] static const char* icon_zoom_in                  = (char*)u8"\ue0DD";
[[maybe_unused]] static const char* icon_zoom_out                 = (char*)u8"\ue0DE";

}  // namespace ImGuiH
