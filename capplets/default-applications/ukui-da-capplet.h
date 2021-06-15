/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *  Modified by: zhangshuhao <zhangshuhao@kylinos.cn>
 *  Copyright 2005-2006 Luca Cavalli
 *  Copyright 2010 Perberos <perberos@gmail.com>
 *  Copyright (C) 2016,Tianjin KYLIN Information Technology Co., Ltd.
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _UKUI_DA_CAPPLET_H_
#define _UKUI_DA_CAPPLET_H_

#include <gtk/gtk.h>

#define TERMINAL_SCHEMA       "org.ukui.applications-terminal"
#define TERMINAL_KEY          "exec"

#define VISUAL_SCHEMA         "org.ukui.applications-at-visual"
#define VISUAL_KEY            "exec"
#define VISUAL_STARTUP_KEY    "startup"

#define MOBILITY_SCHEMA       "org.ukui.applications-at-mobility"
#define MOBILITY_KEY          "exec"
#define MOBILITY_STARTUP_KEY  "startup"

typedef struct _UkuiDACapplet {
	GtkBuilder* builder;

	GtkIconTheme* icon_theme;

	GtkWidget* window;

	GtkWidget* web_combo_box;
	GtkWidget* mail_combo_box;
	GtkWidget* term_combo_box;
	GtkWidget* media_combo_box;
	GtkWidget* video_combo_box;
	GtkWidget* visual_combo_box;
	GtkWidget* mobility_combo_box;
	GtkWidget* file_combo_box;
	GtkWidget* text_combo_box;
	GtkWidget* image_combo_box;
	GtkWidget* document_combo_box;
	GtkWidget* word_combo_box;
	GtkWidget* spreadsheet_combo_box;

	/* Visual Accessibility */
	GtkWidget* visual_startup_checkbutton;

	/* Mobility Accessibility */
	GtkWidget* mobility_startup_checkbutton;

	/* Lists of available apps */
	GList* web_browsers;
	GList* mail_readers;
	GList* terminals;
	GList* media_players;
	GList* video_players;
	GList* visual_ats;
	GList* mobility_ats;
	GList* file_managers;
	GList* text_editors;
	GList* image_viewers;
	GList* document_viewers;
	GList* word_editors;
	GList* spreadsheet_editors;

	/* Settings objects */
	GSettings* terminal_settings;
	GSettings* visual_settings;
	GSettings* mobility_settings;
} UkuiDACapplet;

#endif
