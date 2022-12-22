/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
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
#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QtPlugin>

#include <QMap>
#include <QDir>

#include "shell/interface.h"


class QToolButton;
class SwitchButton;
class QGSettings;

namespace Ui {
class Theme;
}

class Theme : public QObject, CommonInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kycc.CommonInterface")
    Q_INTERFACES(CommonInterface)

public:
    Theme();
    ~Theme();

    QString get_plugin_name() Q_DECL_OVERRIDE;
    int get_plugin_type() Q_DECL_OVERRIDE;
    QWidget * get_plugin_ui() Q_DECL_OVERRIDE;
    void plugin_delay_control() Q_DECL_OVERRIDE;

public:

    void initComponent();
    void initIconTheme();
    void initControlTheme();
    void initCursorTheme();
    void initEffectSettings();

    QStringList _getSystemCursorThemes();

    void status_init();
    void component_init();
    void refresh_btn_select_status();

private:
    Ui::Theme *ui;

    QString pluginName;
    int pluginType;
    QWidget * pluginWidget;

    QGSettings * ifsettings;
    QGSettings * marcosettings;
    QGSettings * desktopsettings;

    QMap<QString, QToolButton *> delbtnMap;
    QMap<QString, SwitchButton *> delsbMap;

//    QMap<QString, IconThemeWidget *> delframeMap;

public slots:
    void set_theme_slots(QString value);
    void desktop_icon_settings_slots(QString key);
};

#endif // THEME_H
