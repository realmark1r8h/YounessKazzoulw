/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2016 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef _APPEARANCE_H
#define _APPEARANCE_H
#include <glib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-desktop-thumbnail.h>

#define APPEARANCE_SCHEMA            "org.ukui.control-center.appearance"

#define WP_SCHEMA                    "org.mate.background"
#define WP_FILE_KEY                  "picture-filename"
#define WP_OPTIONS_KEY               "picture-options"
#define WP_SHADING_KEY               "color-shading-type"
#define WP_PCOLOR_KEY                "primary-color"
#define WP_SCOLOR_KEY                "secondary-color"

#define INTERFACE_SCHEMA             "org.mate.interface"
#define GTK_FONT_KEY                 "font-name"
#define MONOSPACE_FONT_KEY           "monospace-font-name"
#define DOCUMENT_FONT_KEY            "document-font-name"
#define GTK_THEME_KEY                "gtk-theme"
#define ICON_THEME_KEY               "icon-theme"
#define COLOR_SCHEME_KEY             "gtk-color-scheme"
#define GTK_FONT_DEFAULT_VALUE       "Sans 10"

#define LOCKDOWN_SCHEMA              "org.mate.lockdown"
#define DISABLE_THEMES_SETTINGS_KEY  "disable-theme-settings"

#define CAJA_SCHEMA                  "org.ukui.peony.desktop"
#define DESKTOP_FONT_KEY             "font"

#define MARCO_SCHEMA                 "org.mate.Marco.general"
#define MARCO_THEME_KEY              "theme"
#define WINDOW_TITLE_FONT_KEY        "titlebar-font"
#define WINDOW_TITLE_USES_SYSTEM_KEY "titlebar-uses-system-font"

#define NOTIFICATION_SCHEMA          "org.mate.NotificationDaemon"
#define NOTIFICATION_THEME_KEY       "theme"

#define MOUSE_SCHEMA                 "org.mate.peripherals-mouse"
#define CURSOR_THEME_KEY             "cursor-theme"
#define CURSOR_SIZE_KEY              "cursor-size"

#define FONT_RENDER_SCHEMA           "org.mate.font-rendering"
#define FONT_ANTIALIASING_KEY        "antialiasing"
#define FONT_HINTING_KEY             "hinting"
#define FONT_RGBA_ORDER_KEY          "rgba-order"
#define FONT_DPI_KEY                 "dpi"

typedef struct {
    GSettings* settings;
    GSettings* wp_settings;
    GSettings* caja_settings;
    GSettings* interface_settings;
    GSettings* marco_settings;
    GSettings* mouse_settings;
    GSettings* font_settings;
    GtkBuilder* ui;
    MateDesktopThumbnailFactory* thumb_factory;
    gulong screen_size_handler;
    gulong screen_monitors_handler;

    /* desktop */
    GHashTable* wp_hash;
    GtkIconView* wp_view;
    GtkTreeModel* wp_model;
    GtkWidget* wp_style_menu;
    GtkWidget* wp_rem_button;
    GtkFileChooser* wp_filesel;
    GtkWidget* wp_image;
	//GtkWidget * menu;
	//GtkTreePath * path; //current path
    GSList* wp_uris;
    gint frame;
    gint thumb_width;
    gint thumb_height;
    GtkWidget * dialog_vbox2;
    /* font */
    GtkWidget* font_details;
    GSList* font_groups;

    gchar* revert_application_font;
    gchar* revert_documents_font;
    gchar* revert_desktop_font;
    gchar* revert_windowtitle_font;
    gchar* revert_monospace_font;

    /* style */
    GdkPixbuf* gtk_theme_icon;
    GdkPixbuf* window_theme_icon;
    GdkPixbuf* icon_theme_icon;
    GtkWidget* style_message_area;
    GtkWidget* style_message_label;
    GtkWidget* style_install_button;
} AppearanceData;

#define appearance_capplet_get_widget(x, y) (GtkWidget*) gtk_builder_get_object(x->ui, y)
#endif
