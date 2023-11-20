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

#pragma once
#include <imgui.h>


namespace ImGuiH {
void    addIconicFont(float fontSize = 14.F);  // Call this once in the application after ImGui is initialized
void    showDemoIcons();                       // Show all icons in a separated window
ImFont* getIconicFont();                       // Return the iconic font
// Ex:  ImGui::PushFont(ImGuiH::getIconicFont());
//      ImGui::Button(ImGuiH::icon_account_login);
//      ImGui::PopFont();

static const char* icon_account_login            = (char*)u8"\ue000";
static const char* icon_account_logout           = (char*)u8"\ue001";
static const char* icon_action_redo              = (char*)u8"\ue002";
static const char* icon_action_undo              = (char*)u8"\ue003";
static const char* icon_align_center             = (char*)u8"\ue004";
static const char* icon_align_left               = (char*)u8"\ue005";
static const char* icon_align_right              = (char*)u8"\ue006";
static const char* icon_aperture                 = (char*)u8"\ue007";
static const char* icon_arrow_bottom             = (char*)u8"\ue008";
static const char* icon_arrow_circle_bottom      = (char*)u8"\ue009";
static const char* icon_arrow_circle_left        = (char*)u8"\ue00A";
static const char* icon_arrow_circle_right       = (char*)u8"\ue00B";
static const char* icon_arrow_circle_top         = (char*)u8"\ue00C";
static const char* icon_arrow_left               = (char*)u8"\ue00D";
static const char* icon_arrow_right              = (char*)u8"\ue00E";
static const char* icon_arrow_thick_bottom       = (char*)u8"\ue00F";
static const char* icon_arrow_thick_left         = (char*)u8"\ue010";
static const char* icon_arrow_thick_right        = (char*)u8"\ue011";
static const char* icon_arrow_thick_top          = (char*)u8"\ue012";
static const char* icon_arrow_top                = (char*)u8"\ue013";
static const char* icon_audio                    = (char*)u8"\ue014";
static const char* icon_audio_spectrum           = (char*)u8"\ue015";
static const char* icon_badge                    = (char*)u8"\ue016";
static const char* icon_ban                      = (char*)u8"\ue017";
static const char* icon_bar_chart                = (char*)u8"\ue018";
static const char* icon_basket                   = (char*)u8"\ue019";
static const char* icon_battery_empty            = (char*)u8"\ue01A";
static const char* icon_battery_full             = (char*)u8"\ue01B";
static const char* icon_beaker                   = (char*)u8"\ue01C";
static const char* icon_bell                     = (char*)u8"\ue01D";
static const char* icon_bluetooth                = (char*)u8"\ue01E";
static const char* icon_bold                     = (char*)u8"\ue01F";
static const char* icon_bolt                     = (char*)u8"\ue020";
static const char* icon_book                     = (char*)u8"\ue021";
static const char* icon_bookmark                 = (char*)u8"\ue022";
static const char* icon_box                      = (char*)u8"\ue023";
static const char* icon_briefcase                = (char*)u8"\ue024";
static const char* icon_british_pound            = (char*)u8"\ue025";
static const char* icon_browser                  = (char*)u8"\ue026";
static const char* icon_brush                    = (char*)u8"\ue027";
static const char* icon_bug                      = (char*)u8"\ue028";
static const char* icon_bullhorn                 = (char*)u8"\ue029";
static const char* icon_calculator               = (char*)u8"\ue02A";
static const char* icon_calendar                 = (char*)u8"\ue02B";
static const char* icon_camera_slr               = (char*)u8"\ue02C";
static const char* icon_caret_bottom             = (char*)u8"\ue02D";
static const char* icon_caret_left               = (char*)u8"\ue02E";
static const char* icon_caret_right              = (char*)u8"\ue02F";
static const char* icon_caret_top                = (char*)u8"\ue030";
static const char* icon_cart                     = (char*)u8"\ue031";
static const char* icon_chat                     = (char*)u8"\ue032";
static const char* icon_check                    = (char*)u8"\ue033";
static const char* icon_chevron_bottom           = (char*)u8"\ue034";
static const char* icon_chevron_left             = (char*)u8"\ue035";
static const char* icon_chevron_right            = (char*)u8"\ue036";
static const char* icon_chevron_top              = (char*)u8"\ue037";
static const char* icon_circle_check             = (char*)u8"\ue038";
static const char* icon_circle_x                 = (char*)u8"\ue039";
static const char* icon_clipboard                = (char*)u8"\ue03A";
static const char* icon_clock                    = (char*)u8"\ue03B";
static const char* icon_cloud_download           = (char*)u8"\ue03C";
static const char* icon_cloud_upload             = (char*)u8"\ue03D";
static const char* icon_cloud                    = (char*)u8"\ue03E";
static const char* icon_cloudy                   = (char*)u8"\ue03F";
static const char* icon_code                     = (char*)u8"\ue040";
static const char* icon_cog                      = (char*)u8"\ue041";
static const char* icon_collapse_down            = (char*)u8"\ue042";
static const char* icon_collapse_left            = (char*)u8"\ue043";
static const char* icon_collapse_right           = (char*)u8"\ue044";
static const char* icon_collapse_up              = (char*)u8"\ue045";
static const char* icon_command                  = (char*)u8"\ue046";
static const char* icon_comment_square           = (char*)u8"\ue047";
static const char* icon_compass                  = (char*)u8"\ue048";
static const char* icon_contrast                 = (char*)u8"\ue049";
static const char* icon_copywriting              = (char*)u8"\ue04A";
static const char* icon_credit_card              = (char*)u8"\ue04B";
static const char* icon_crop                     = (char*)u8"\ue04C";
static const char* icon_dashboard                = (char*)u8"\ue04D";
static const char* icon_data_transfer_download   = (char*)u8"\ue04E";
static const char* icon_data_transfer_upload     = (char*)u8"\ue04F";
static const char* icon_delete                   = (char*)u8"\ue050";
static const char* icon_dial                     = (char*)u8"\ue051";
static const char* icon_document                 = (char*)u8"\ue052";
static const char* icon_dollar                   = (char*)u8"\ue053";
static const char* icon_double_quote_sans_left   = (char*)u8"\ue054";
static const char* icon_double_quote_sans_right  = (char*)u8"\ue055";
static const char* icon_double_quote_serif_left  = (char*)u8"\ue056";
static const char* icon_double_quote_serif_right = (char*)u8"\ue057";
static const char* icon_droplet                  = (char*)u8"\ue058";
static const char* icon_eject                    = (char*)u8"\ue059";
static const char* icon_elevator                 = (char*)u8"\ue05A";
static const char* icon_ellipses                 = (char*)u8"\ue05B";
static const char* icon_envelope_closed          = (char*)u8"\ue05C";
static const char* icon_envelope_open            = (char*)u8"\ue05D";
static const char* icon_euro                     = (char*)u8"\ue05E";
static const char* icon_excerpt                  = (char*)u8"\ue05F";
static const char* icon_expend_down              = (char*)u8"\ue060";
static const char* icon_expend_left              = (char*)u8"\ue061";
static const char* icon_expend_right             = (char*)u8"\ue062";
static const char* icon_expend_up                = (char*)u8"\ue063";
static const char* icon_external_link            = (char*)u8"\ue064";
static const char* icon_eye                      = (char*)u8"\ue065";
static const char* icon_eyedropper               = (char*)u8"\ue066";
static const char* icon_file                     = (char*)u8"\ue067";
static const char* icon_fire                     = (char*)u8"\ue068";
static const char* icon_flag                     = (char*)u8"\ue069";
static const char* icon_flash                    = (char*)u8"\ue06A";
static const char* icon_folder                   = (char*)u8"\ue06B";
static const char* icon_fork                     = (char*)u8"\ue06C";
static const char* icon_fullscreen_enter         = (char*)u8"\ue06D";
static const char* icon_fullscreen_exit          = (char*)u8"\ue06E";
static const char* icon_globe                    = (char*)u8"\ue06F";
static const char* icon_graph                    = (char*)u8"\ue070";
static const char* icon_grid_four_up             = (char*)u8"\ue071";
static const char* icon_grid_three_up            = (char*)u8"\ue072";
static const char* icon_grid_two_up              = (char*)u8"\ue073";
static const char* icon_hard_drive               = (char*)u8"\ue074";
static const char* icon_header                   = (char*)u8"\ue075";
static const char* icon_headphones               = (char*)u8"\ue076";
static const char* icon_heart                    = (char*)u8"\ue077";
static const char* icon_home                     = (char*)u8"\ue078";
static const char* icon_image                    = (char*)u8"\ue079";
static const char* icon_inbox                    = (char*)u8"\ue07A";
static const char* icon_infinity                 = (char*)u8"\ue07B";
static const char* icon_info                     = (char*)u8"\ue07C";
static const char* icon_italic                   = (char*)u8"\ue07D";
static const char* icon_justify_center           = (char*)u8"\ue07E";
static const char* icon_justify_left             = (char*)u8"\ue07F";
static const char* icon_justify_right            = (char*)u8"\ue080";
static const char* icon_key                      = (char*)u8"\ue081";
static const char* icon_laptop                   = (char*)u8"\ue082";
static const char* icon_layers                   = (char*)u8"\ue083";
static const char* icon_lightbulb                = (char*)u8"\ue084";
static const char* icon_link_broken              = (char*)u8"\ue085";
static const char* icon_link_intact              = (char*)u8"\ue086";
static const char* icon_list                     = (char*)u8"\ue087";
static const char* icon_list_rich                = (char*)u8"\ue088";
static const char* icon_location                 = (char*)u8"\ue089";
static const char* icon_lock_locked              = (char*)u8"\ue08A";
static const char* icon_lock_unlocked            = (char*)u8"\ue08B";
static const char* icon_loop_circular            = (char*)u8"\ue08C";
static const char* icon_loop_square              = (char*)u8"\ue08D";
static const char* icon_loop                     = (char*)u8"\ue08E";
static const char* icon_magnifying_glass         = (char*)u8"\ue08F";
static const char* icon_map                      = (char*)u8"\ue090";
static const char* icon_map_marquer              = (char*)u8"\ue091";
static const char* icon_media_pause              = (char*)u8"\ue092";
static const char* icon_media_play               = (char*)u8"\ue093";
static const char* icon_media_record             = (char*)u8"\ue094";
static const char* icon_media_skip_backward      = (char*)u8"\ue095";
static const char* icon_media_skip_forward       = (char*)u8"\ue096";
static const char* icon_media_step_backward      = (char*)u8"\ue097";
static const char* icon_media_step_forward       = (char*)u8"\ue098";
static const char* icon_media_stop               = (char*)u8"\ue099";
static const char* icon_medical_cross            = (char*)u8"\ue09A";
static const char* icon_menu                     = (char*)u8"\ue09B";
static const char* icon_microphone               = (char*)u8"\ue09C";
static const char* icon_minus                    = (char*)u8"\ue09D";
static const char* icon_monitor                  = (char*)u8"\ue09E";
static const char* icon_moon                     = (char*)u8"\ue09F";
static const char* icon_move                     = (char*)u8"\ue0A0";
static const char* icon_musical_note             = (char*)u8"\ue0A1";
static const char* icon_paperclip                = (char*)u8"\ue0A2";
static const char* icon_pencil                   = (char*)u8"\ue0A3";
static const char* icon_people                   = (char*)u8"\ue0A4";
static const char* icon_person                   = (char*)u8"\ue0A5";
static const char* icon_phone                    = (char*)u8"\ue0A6";
static const char* icon_pie_chart                = (char*)u8"\ue0A7";
static const char* icon_pin                      = (char*)u8"\ue0A8";
static const char* icon_play_circle              = (char*)u8"\ue0A9";
static const char* icon_plus                     = (char*)u8"\ue0AA";
static const char* icon_power_standby            = (char*)u8"\ue0AB";
static const char* icon_print                    = (char*)u8"\ue0AC";
static const char* icon_project                  = (char*)u8"\ue0AD";
static const char* icon_pulse                    = (char*)u8"\ue0AE";
static const char* icon_puzzle_piece             = (char*)u8"\ue0AF";
static const char* icon_question_mark            = (char*)u8"\ue0B0";
static const char* icon_rain                     = (char*)u8"\ue0B1";
static const char* icon_random                   = (char*)u8"\ue0B2";
static const char* icon_reload                   = (char*)u8"\ue0B3";
static const char* icon_resize_both              = (char*)u8"\ue0B4";
static const char* icon_resize_height            = (char*)u8"\ue0B5";
static const char* icon_resize_width             = (char*)u8"\ue0B6";
static const char* icon_rss                      = (char*)u8"\ue0B7";
static const char* icon_rss_alt                  = (char*)u8"\ue0B8";
static const char* icon_script                   = (char*)u8"\ue0B9";
static const char* icon_share                    = (char*)u8"\ue0BA";
static const char* icon_share_boxed              = (char*)u8"\ue0BB";
static const char* icon_shield                   = (char*)u8"\ue0BC";
static const char* icon_signal                   = (char*)u8"\ue0BD";
static const char* icon_signpost                 = (char*)u8"\ue0BE";
static const char* icon_sort_ascending           = (char*)u8"\ue0BF";
static const char* icon_sort_descending          = (char*)u8"\ue0C0";
static const char* icon_spreadsheet              = (char*)u8"\ue0C1";
static const char* icon_star                     = (char*)u8"\ue0C2";
static const char* icon_sun                      = (char*)u8"\ue0C3";
static const char* icon_tablet                   = (char*)u8"\ue0C4";
static const char* icon_tag                      = (char*)u8"\ue0C5";
static const char* icon_tags                     = (char*)u8"\ue0C6";
static const char* icon_target                   = (char*)u8"\ue0C7";
static const char* icon_task                     = (char*)u8"\ue0C8";
static const char* icon_terminal                 = (char*)u8"\ue0C9";
static const char* icon_text                     = (char*)u8"\ue0CA";
static const char* icon_thumb_down               = (char*)u8"\ue0CB";
static const char* icon_thumb_up                 = (char*)u8"\ue0CC";
static const char* icon_timer                    = (char*)u8"\ue0CD";
static const char* icon_transfer                 = (char*)u8"\ue0CE";
static const char* icon_trash                    = (char*)u8"\ue0CF";
static const char* icon_underline                = (char*)u8"\ue0D0";
static const char* icon_vertical_align_bottom    = (char*)u8"\ue0D1";
static const char* icon_vertical_align_center    = (char*)u8"\ue0D2";
static const char* icon_vertical_align_top       = (char*)u8"\ue0D3";
static const char* icon_video                    = (char*)u8"\ue0D4";
static const char* icon_volume_high              = (char*)u8"\ue0D5";
static const char* icon_volume_low               = (char*)u8"\ue0D6";
static const char* icon_volume_off               = (char*)u8"\ue0D7";
static const char* icon_warning                  = (char*)u8"\ue0D8";
static const char* icon_wifi                     = (char*)u8"\ue0D9";
static const char* icon_wrench                   = (char*)u8"\ue0DA";
static const char* icon_x                        = (char*)u8"\ue0DB";
static const char* icon_yen                      = (char*)u8"\ue0DC";
static const char* icon_zoom_in                  = (char*)u8"\ue0DD";
static const char* icon_zoom_out                 = (char*)u8"\ue0DE";

}  // namespace ImGuiH
