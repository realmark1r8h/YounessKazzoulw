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
#ifndef CHANGETYPEDIALOG_H
#define CHANGETYPEDIALOG_H

#include <QDialog>

#include "../../pluginsComponent/switchbutton.h"

namespace Ui {
class ChangeTypeDialog;
}

class ChangeTypeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeTypeDialog(QWidget *parent = 0);
    ~ChangeTypeDialog();

    void set_face_label(QString iconfile);
    void set_username_label(QString username);
    void set_account_type_label(QString atype);
    void set_current_account_type(int id);
    void set_autologin_status(bool status);

private:
    Ui::ChangeTypeDialog *ui;

    SwitchButton * autologinSwitchBtn;

    int currenttype;
    bool currentloginstatus;

private slots:
    void autologin_status_changed_slot(bool status);
    void radioBtn_clicked_slot(int id);
    void confirm_slot();

Q_SIGNALS:
    void type_send(int type, QString username, bool autologin);
};

#endif // CHANGETYPEDIALOG_H
