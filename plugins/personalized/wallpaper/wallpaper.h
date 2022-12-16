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
#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <QObject>
#include <QtPlugin>

#include <QThread>
#include <QPixmap>
#include <QListWidgetItem>
#include <QFileDialog>

#include <QGSettings/QGSettings>

#include "shell/interface.h"

#include "xmlhandle.h"
#include "component/custdomitemmodel.h"
#include "simplethread.h"
#include "workerobject.h"

/* qt会将glib里的signals成员识别为宏，所以取消该宏
 * 后面如果用到signals时，使用Q_SIGNALS代替即可
 **/
#ifdef signals
#undef signals
#endif

extern "C" {
#include <glib.h>
#include <gio/gio.h>
}

#define BACKGROUND "org.mate.background"

//图片文件路径
#define FILENAME "picture-filename"
//图片不透明度
#define OPACITY "picture-opacity"
//图片放置方式
#define OPTIONS "picture-options"
//主色
#define PRIMARY "primary-color"
//副色
#define SECONDARY "secondary-color"

namespace Ui {
class Wallpaper;
}

class Wallpaper : public QObject, CommonInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kycc.CommonInterface")
    Q_INTERFACES(CommonInterface)

public:
    Wallpaper();
    ~Wallpaper();

    QString get_plugin_name() Q_DECL_OVERRIDE;
    int get_plugin_type() Q_DECL_OVERRIDE;
    QWidget * get_plugin_ui() Q_DECL_OVERRIDE;
    void plugin_delay_control() Q_DECL_OVERRIDE;

public:
    void initComponent();
    void initBgFormStatus();
    void initPreviewStatus();

    void appendPicWpItem(QPixmap pixmap, QString filename);

public:
    void append_item(QPixmap pixmap, QString filename);

private:
    Ui::Wallpaper *ui;

    QString pluginName;
    int pluginType;
    QWidget * pluginWidget;

    QMap<QString, QMap<QString, QString> > wallpaperinfosMap;

    XmlHandle * xmlhandleObj;
    QGSettings * bgsettings;
    QString localwpconf;
    QMap<QString, QListWidgetItem*> delItemsMap;

    CustdomItemModel wpListModel;

    void component_init();

    //尝试mode view
    void setlistview();
    void setModeldata();

private:
    QThread * pThread;
    WorkerObject * pObject;

    QMap<QString, QListWidgetItem*> picWpItemMap;

public slots:
    void picWallpaperChangedSlot(QListWidgetItem * current, QListWidgetItem * previous);
    void resetDefaultWallpaperSlot();
    void wpOptionsChangedSlot(QString op);

public slots:
    void colorwp_item_clicked(QListWidgetItem * current, QListWidgetItem * previous);

    void add_custom_wallpaper();
    void del_wallpaper();
};

#endif // WALLPAPER_H
