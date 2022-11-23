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
#include "deluserdialog.h"
#include "ui_deluserdialog.h"

#include <QDebug>

DelUserDialog::DelUserDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DelUserDialog)
{
    ui->setupUi(this);
    connect(ui->cancelPushBtn, SIGNAL(clicked()), this, SLOT(reject()));

    QSignalMapper * differSignalMapper = new QSignalMapper();
    for (QAbstractButton * button : ui->buttonGroup->buttons()){
        connect(button, SIGNAL(clicked()), differSignalMapper, SLOT(map()));
        differSignalMapper->setMapping(button, button->text());
    }
    connect(differSignalMapper, SIGNAL(mapped(QString)), this, SLOT(btn_clicked_slot(QString)));
}

DelUserDialog::~DelUserDialog()
{
    delete ui;
}

void DelUserDialog::set_face_label(QString iconfile){
    ui->faceLabel->setPixmap(QPixmap(iconfile).scaled(QSize(80, 80)));
}

void DelUserDialog::set_username_label(QString username){
    ui->usernameLabel->setText(username);
}

void DelUserDialog::btn_clicked_slot(QString key){
    this->accept();
    bool removefile;
    if (ui->removePushBtn->text() == key)
        removefile = true;
    else
        removefile = false;
    emit removefile_send(removefile, ui->usernameLabel->text());
}
