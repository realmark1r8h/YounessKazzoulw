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
#include "userinfo.h"
#include "ui_userinfo.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusReply>

#include <QDebug>

#include "SwitchButton/switchbutton.h"
#include "HoverWidget/hoverwidget.h"

#define DEFAULTFACE "://img/plugins/userinfo/defaultface.png"
#define ITEMHEIGH 58

UserInfo::UserInfo()
{
    ui = new Ui::UserInfo;
    pluginWidget = new QWidget;
    pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(pluginWidget);

    pluginName = tr("userinfo");
    pluginType = ACCOUNT;

    //构建System dbus调度对象
    sysdispatcher = new SystemDbusDispatcher;


    //获取系统全部用户信息，用户Uid大于等于1000的
    _acquireAllUsersInfo();


    initComponent();
    initAllUserStatus();
    //设置界面用户信息
    _refreshUserInfoUI();


//    pwdSignalMapper = new QSignalMapper(this);
//    faceSignalMapper = new QSignalMapper(this);
//    typeSignalMapper = new QSignalMapper(this);
//    delSignalMapper = new QSignalMapper(this);

//    faceSize = QSize(64, 64);
//    itemSize = QSize(230, 106); //?需要比btnsize大多少？否则显示不全
//    btnSize = QSize(222, 92);


//    get_all_users();
//    ui_component_init();
//    ui_status_init();

}

UserInfo::~UserInfo()
{
    delete ui;
}

QString UserInfo::get_plugin_name(){
    return pluginName;
}

int UserInfo::get_plugin_type(){
    return pluginType;
}

QWidget *UserInfo::get_plugin_ui(){
    return pluginWidget;
}

void UserInfo::plugin_delay_control(){

}

QString UserInfo::_accountTypeIntToString(int type){
    QString atype;
    if (type == STANDARDUSER)
        atype = tr("standard user");
    else if (type == ADMINISTRATOR)
        atype = tr("administrator");
    else if (type == ROOT)
        atype = tr("root");

    return atype;
}

QString UserInfo::accounttype_enum_to_string(int id){
    QString type;
    if (id == STANDARDUSER)
        type = tr("standard user");
    else if (id == ADMINISTRATOR)
        type = tr("administrator");
    else if (id == ROOT)
        type = tr("root");
    return type;
}

QString UserInfo::login_status_bool_to_string(bool status){
    QString logined;
    if (status)
        logined = tr("logined");
    else
        logined = tr("unlogined");
    return logined;
}

void UserInfo::_acquireAllUsersInfo(){
    QStringList objectpaths = sysdispatcher->list_cached_users();

    //初始化用户信息QMap
    allUserInfoMap.clear();
    //初始化管理员数目为0
    adminnum = 0;

    for (QString objectpath : objectpaths){
        UserInfomation user;
        user = _acquireUserInfo(objectpath);
        allUserInfoMap.insert(user.username, user);
    }

    //处理root登录
    if (!getuid())
        ;

}

void UserInfo::get_all_users(){
    QStringList objectpaths = sysdispatcher->list_cached_users();

    //清空allUserInfoMap
    allUserInfoMap.clear();
    adminnum = 0; //reset

    for (QString objectpath : objectpaths){
        UserInfomation user;
        user = init_user_info(objectpath);
        allUserInfoMap.insert(user.username, user);
    }
    if (!getuid())
        init_root_info();
}

UserInfomation UserInfo::_acquireUserInfo(QString objpath){
    UserInfomation user;

    //默认值
    user.current = false;
    user.logined = false;
    user.autologin = false;

    QDBusInterface * iproperty = new QDBusInterface("org.freedesktop.Accounts",
                                            objpath,
                                            "org.freedesktop.DBus.Properties",
                                            QDBusConnection::systemBus());
    QDBusReply<QMap<QString, QVariant> > reply = iproperty->call("GetAll", "org.freedesktop.Accounts.User");
    if (reply.isValid()){
        QMap<QString, QVariant> propertyMap;
        propertyMap = reply.value();
        user.username = propertyMap.find("UserName").value().toString();
        if (user.username == QString(g_get_user_name())){
            user.current = true;
            user.logined = true;
        }
        user.accounttype = propertyMap.find("AccountType").value().toInt();
        if (user.accounttype == ADMINISTRATOR)
            adminnum++;
        user.iconfile = propertyMap.find("IconFile").value().toString();
        user.passwdtype = propertyMap.find("PasswordMode").value().toInt();
        user.uid = propertyMap.find("Uid").value().toInt();
        user.autologin = propertyMap.find("AutomaticLogin").value().toBool();
        user.objpath = objpath;
    }
    else
        qDebug() << "reply failed";

    delete iproperty;

    return user;
}

UserInfomation UserInfo::init_user_info(QString objpath){
    UserInfomation user;

    //default set
    user.current = false;
    user.logined = false;
    user.autologin = false;

//    QDBusInterface * iface = new QDBusInterface("org.freedesktop.Accounts",
//                                         objpath,
//                                         "org.freedesktop.Accounts.User",
//                                         QDBusConnection::systemBus());
    QDBusInterface * iproperty = new QDBusInterface("org.freedesktop.Accounts",
                                            objpath,
                                            "org.freedesktop.DBus.Properties",
                                            QDBusConnection::systemBus());
    QDBusReply<QMap<QString, QVariant> > reply = iproperty->call("GetAll", "org.freedesktop.Accounts.User");
    if (reply.isValid()){
        QMap<QString, QVariant> propertyMap;
        propertyMap = reply.value();
        user.username = propertyMap.find("UserName").value().toString();
        if (user.username == QString(g_get_user_name())){
            user.current = true;
            user.logined = true;
        }
        user.accounttype = propertyMap.find("AccountType").value().toInt();
        if (user.accounttype == ADMINISTRATOR)
            adminnum++;
        user.iconfile = propertyMap.find("IconFile").value().toString();
        user.passwdtype = propertyMap.find("PasswordMode").value().toInt();
        user.uid = propertyMap.find("Uid").value().toInt();
        user.autologin = propertyMap.find("AutomaticLogin").value().toBool();
        user.objpath = objpath;
    }
    else
        qDebug() << "reply failed";

    delete iproperty;

    return user;

}

void UserInfo::initComponent(){
    //样式表
    pluginWidget->setStyleSheet("background: #ffffff;");

    ui->currentUserWidget->setStyleSheet("QWidget{background: #F4F4F4; border-top-left-radius: 6px; border-top-right-radius: 6px;}");
    ui->autoLoginWidget->setStyleSheet("QWidget{background: #F4F4F4; border-bottom-left-radius: 6px; border-bottom-right-radius: 6px;}");

    QString btnQss = QString("QPushButton{background: #FFFFFF; border-radius: 4px;}");
    ui->changePwdBtn->setStyleSheet(btnQss);
    ui->changeTypeBtn->setStyleSheet(btnQss);

    ui->addUserWidget->setStyleSheet("QWidget{background: #F4F4F4; border-radius: 6px;}");

    nopwdSwitchBtn = new SwitchButton(ui->nopwdLoginWidget);
    ui->nopwdHorLayout->addWidget(nopwdSwitchBtn);

    autoLoginSwitchBtn = new SwitchButton(ui->autoLoginWidget);
    ui->autoLoginHorLayout->addWidget(autoLoginSwitchBtn);


    ui->listWidget->setStyleSheet("QListWidget{border: none}");
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget->setSpacing(0);

    //设置添加用户的图标
    ui->addBtn->setIcon(QIcon("://img/plugins/userinfo/add.png"));
    ui->addBtn->setIconSize(ui->addBtn->size());

    //修改当前用户密码的回调
    connect(ui->changePwdBtn, &QPushButton::clicked, this, [=](bool checked){
        UserInfomation user = allUserInfoMap.value(g_get_user_name());

        showChangePwdDialog(user.username);
    });

    //修改当前用户类型的回调
    connect(ui->changeTypeBtn, &QPushButton::clicked, this, [=](bool checked){

    });

    //成功删除用户的回调
    connect(sysdispatcher, &SystemDbusDispatcher::deleteuserdone, this, [=](QString objPath){
        deleteUserDone(objPath);
    });

    //新建用户的回调
    connect(ui->addBtn, &QPushButton::clicked, this, [=](bool checked){
        Q_UNUSED(checked)
        showCreateUserDialog();
    });

    //成功新建用户的回调
    connect(sysdispatcher, &SystemDbusDispatcher::createuserdone, this, [=](QString objPath){
        createUserDone(objPath);
    });
}

void UserInfo::_resetListWidgetHeigh(){
    //设置其他用户控件的总高度
    ui->listWidget->setFixedHeight((allUserInfoMap.count() - 1) * ITEMHEIGH);
}

void UserInfo::initAllUserStatus(){
    _resetListWidgetHeigh();

    //每次初始化用户状态清空其他用户QMap
    otherUserItemMap.clear();

    QMap<QString, UserInfomation>::iterator it = allUserInfoMap.begin();
    for (; it != allUserInfoMap.end(); it++){
        UserInfomation user = it.value();

        //当前用户
        if (user.username == QString(g_get_user_name())){

        } else { //其他用户
            _buildWidgetForItem(user);

        }
    }
}

void UserInfo::_refreshUserInfoUI(){
    QMap<QString, UserInfomation>::iterator it = allUserInfoMap.begin();
    for (; it != allUserInfoMap.end(); it++){
        UserInfomation user = it.value();

        //用户头像筛选
        QString iconfile;
        if (user.iconfile.isEmpty() || user.iconfile.endsWith(".face"))
            iconfile = DEFAULTFACE;
        else
            iconfile = user.iconfile;

        //当前用户
        if (user.username == QString(g_get_user_name())){
            //设置用户头像
            QPixmap iconPixmap = QPixmap(iconfile);
            iconPixmap.scaled(ui->currentUserFaceLabel->size(), Qt::KeepAspectRatio);
            ui->currentUserFaceLabel->setScaledContents(true);
            ui->currentUserFaceLabel->setPixmap(iconPixmap);

            //设置用户名
            ui->userNameLabel->setText(user.username);
            //设置用户类型
            ui->userTypeLabel->setText(_accountTypeIntToString(user.accounttype));
            //设置登录状态

        } else { //其他用户
            QListWidgetItem * item = otherUserItemMap.value(user.objpath); //是否需要判断？？

            QWidget * widget = ui->listWidget->itemWidget(item);

            QPushButton * faceBtn = widget->findChild<QPushButton *>("faceBtn");
            faceBtn->setIcon(QIcon(user.iconfile));

        }
    }
}

void UserInfo::ui_status_init(){
    /*
    QMap<QString, UserInfomation>::iterator it = allUserInfoMap.begin();
    for (; it != allUserInfoMap.end(); it++){
        UserInfomation user = (UserInfomation) it.value();
        QString iconfile = DEFAULTFACE;
        if (user.iconfile !="" && !user.iconfile.endsWith(".face")) //如果存在头像文件，覆盖默认值
            iconfile = user.iconfile;
        if (user.username == QString(g_get_user_name())){ //当前用户      
            ui->faceLabel->setPixmap(QPixmap(iconfile).scaled(QSize(80, 80)));
            ui->usernameLabel->setText(user.username);
            ui->accounttypeLabel->setText(accounttype_enum_to_string(user.accounttype));
            ui->loginLabel->setText(login_status_bool_to_string(user.logined));
        }
        else{ //其他用户
            QMap<QString, QListWidgetItem *>::iterator itemit = otherItemMap.find(user.username);

            if (itemit != otherItemMap.end()){
                QWidget * widget = ui->listWidget->itemWidget((QListWidgetItem *) itemit.value());
                QToolButton * button = widget->findChild<QToolButton *>(user.username);
                // 获取账户类型
                QString type = accounttype_enum_to_string(user.accounttype);
                // 获取登录状态
                QString logined = login_status_bool_to_string(user.logined);
                button->setIcon(QIcon(iconfile));
                button->setText(QString("%1\n%2\n%3").arg(user.username, type, logined));
            }
            else
                qDebug() << QString(it.key()) << "QToolBtn init failed!";
        }
    }
    */
}

void UserInfo::_buildWidgetForItem(UserInfomation user){
    HoverWidget * baseWidget = new HoverWidget(user.username);
    baseWidget->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout * baseVerLayout = new QVBoxLayout(baseWidget);
    baseVerLayout->setSpacing(0);
    baseVerLayout->setMargin(0);

    QHBoxLayout * baseHorLayout = new QHBoxLayout();
    baseHorLayout->setSpacing(16);
    baseHorLayout->setMargin(0);

    QWidget * widget = new QWidget(baseWidget);
    widget->setFixedHeight(50);
    widget->setStyleSheet("background: #F4F4F4; border-radius: 4px;");

    QHBoxLayout * mainHorLayout = new QHBoxLayout(widget);
    mainHorLayout->setSpacing(16);
    mainHorLayout->setContentsMargins(16, 0, 16, 0);

    QPushButton * faceBtn = new QPushButton(widget);
    faceBtn->setObjectName("faceBtn");
    faceBtn->setFixedSize(32, 32);
//    faceBtn->setIcon(QIcon(user.iconfile));
    faceBtn->setIconSize(faceBtn->size());

    QLabel * nameLabel = new QLabel(widget);
    QSizePolicy nameSizePolicy = nameLabel->sizePolicy();
    nameSizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
    nameSizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    nameLabel->setSizePolicy(nameSizePolicy);
    nameLabel->setText(user.username);

    QString btnQss = QString("QPushButton{background: #ffffff; border-radius: 4px;}");

    QPushButton * typeBtn = new QPushButton(widget);
    typeBtn->setFixedSize(88, 36);
    typeBtn->setText(tr("Change type"));
    typeBtn->setStyleSheet(btnQss);
    typeBtn->hide();

    QPushButton * pwdBtn = new QPushButton(widget);
    pwdBtn->setFixedSize(88, 36);
    pwdBtn->setText(tr("Change pwd"));
    pwdBtn->setStyleSheet(btnQss);
    connect(pwdBtn, &QPushButton::clicked, this, [=](bool checked){
        showChangePwdDialog(user.username);
    });
    pwdBtn->hide();

    mainHorLayout->addWidget(faceBtn);
    mainHorLayout->addWidget(nameLabel);
    mainHorLayout->addStretch();
    mainHorLayout->addWidget(typeBtn);
    mainHorLayout->addWidget(pwdBtn);

    widget->setLayout(mainHorLayout);

    QPushButton * delBtn = new QPushButton(baseWidget);
    delBtn->setFixedSize(60, 36);
    delBtn->setText(tr("Delete"));
    delBtn->setStyleSheet("QPushButton{background: #FA6056; border-radius: 4px}");
    delBtn->hide();
    connect(delBtn, &QPushButton::clicked, this, [=](bool checked){
        showDeleteUserDialog(user.username);
    });

    connect(baseWidget, &HoverWidget::enterWidget, this, [=](QString name){
        Q_UNUSED(name)
        typeBtn->show();
        pwdBtn->show();
        delBtn->show();
    });
    connect(baseWidget, &HoverWidget::leaveWidget, this, [=](QString name){
        Q_UNUSED(name)
        typeBtn->hide();
        pwdBtn->hide();
        delBtn->hide();
    });

    baseHorLayout->addWidget(widget);
    baseHorLayout->addWidget(delBtn, Qt::AlignVCenter);

    baseVerLayout->addLayout(baseHorLayout);
    baseVerLayout->addStretch();

    baseWidget->setLayout(baseVerLayout);

    QListWidgetItem * item = new QListWidgetItem(ui->listWidget);
    item->setSizeHint(QSize(ui->listWidget->width(), ITEMHEIGH));
    item->setData(Qt::UserRole, QVariant(user.objpath));
    ui->listWidget->setItemWidget(item, baseWidget);

    otherUserItemMap.insert(user.objpath, item);

}

void UserInfo::build_item_with_widget(UserInfomation user){

    QWidget * otherWidget = new QWidget();
    otherWidget->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout * otherVerLayout = new QVBoxLayout(otherWidget);
    QToolButton * otherToolBtn = new QToolButton(otherWidget);
    otherToolBtn->setFixedSize(btnSize);
    otherToolBtn->setObjectName(user.username);
    otherToolBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//            otherToolBtn->setIcon(QIcon(user.iconfile));
    otherToolBtn->setIconSize(faceSize);
//            otherToolBtn->setText(QString("%1\n%2\n%3").arg(user.username, type, logined));
    otherbtnMap.insert(user.username, otherToolBtn);

    QMenu * menu = new QMenu(otherToolBtn);
    QAction * chpwd = menu->addAction(tr("change pwd"));
    QAction * chface = menu->addAction(tr("change face"));
    QAction * chtype = menu->addAction(tr("change accounttype"));
    QAction * deluer = menu->addAction(tr("delete user"));
    connect(chpwd, SIGNAL(triggered()), pwdSignalMapper, SLOT(map()));
    connect(chface, SIGNAL(triggered()), faceSignalMapper, SLOT(map()));
    connect(chtype, SIGNAL(triggered()), typeSignalMapper, SLOT(map()));
    connect(deluer, SIGNAL(triggered()), delSignalMapper, SLOT(map()));

    pwdSignalMapper->setMapping(chpwd, user.username);
    faceSignalMapper->setMapping(chface, user.username);
    typeSignalMapper->setMapping(chtype, user.username);
    delSignalMapper->setMapping(deluer, user.username);

    otherToolBtn->setMenu(menu);
    otherToolBtn->setPopupMode(QToolButton::InstantPopup);
//            otherToolBtn->setStyleSheet("QToolButton::menu-indicator{image:none;}");
    otherVerLayout->addWidget(otherToolBtn);
    otherWidget->setLayout(otherVerLayout);

    QListWidgetItem * item = new QListWidgetItem(ui->listWidget);
    item->setSizeHint(itemSize);
    item->setData(Qt::UserRole, QVariant(user.objpath));
    ui->listWidget->addItem(item);
    ui->listWidget->setItemWidget(item, otherWidget);

    otherItemMap.insert(user.username, item);
}

void UserInfo::ui_component_init(){

    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setSpacing(0);

    //设置创建用户按钮
    QWidget * newuserWidget = new QWidget();
    newuserWidget->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout * newVLayout = new QVBoxLayout(newuserWidget);
    QToolButton * newToolBtn = new QToolButton(newuserWidget);
    newToolBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    newToolBtn->setFixedSize(btnSize);
    newToolBtn->setIcon(QIcon(":/userinfo/add.png"));
    newToolBtn->setIconSize(faceSize);
    newToolBtn->setText(tr("add new user"));
    newVLayout->addWidget(newToolBtn);
    newuserWidget->setLayout(newVLayout);
    connect(newToolBtn, SIGNAL(clicked()), this, SLOT(show_create_user_dialog_slot()));

    QListWidgetItem * newitem = new QListWidgetItem(ui->listWidget);
    newitem->setSizeHint(itemSize);
    newitem->setData(Qt::UserRole, QVariant(""));
    ui->listWidget->addItem(newitem);
    ui->listWidget->setItemWidget(newitem, newuserWidget);


    QMap<QString, UserInfomation>::iterator it = allUserInfoMap.begin();
    for (; it != allUserInfoMap.end(); it++){
        UserInfomation user = (UserInfomation)it.value();

        //当前用户
        if (user.current){
//            connect(ui->chpwdPushBtn, SIGNAL(clicked(bool)), pwdSignalMapper, SLOT(map()));
//            connect(ui->chfacePushBtn, SIGNAL(clicked(bool)), faceSignalMapper, SLOT(map()));
//            connect(ui->chtypePushBtn, SIGNAL(clicked(bool)), typeSignalMapper, SLOT(map()));

//            pwdSignalMapper->setMapping(ui->chpwdPushBtn, user.username);
//            faceSignalMapper->setMapping(ui->chfacePushBtn, user.username);
//            typeSignalMapper->setMapping(ui->chtypePushBtn, user.username);
//            continue;
        }
        //设置其他用户
//        build_item_with_widget(user);
    }
    connect(pwdSignalMapper, SIGNAL(mapped(QString)), this, SLOT(show_change_pwd_dialog_slot(QString)));
    connect(faceSignalMapper, SIGNAL(mapped(QString)), this, SLOT(show_change_face_dialog_slot(QString)));
    connect(typeSignalMapper, SIGNAL(mapped(QString)), this, SLOT(show_change_accounttype_dialog_slot(QString)));
    connect(delSignalMapper, SIGNAL(mapped(QString)), this, SLOT(show_del_user_dialog_slot(QString)));
}

void UserInfo::showCreateUserDialog(){
    //获取系统所有用户名列表，创建时判断重名
    QStringList usersStringList;
    for (QVariant tmp : allUserInfoMap.keys()){
        usersStringList << tmp.toString();
    }

    CreateUserDialog * dialog = new CreateUserDialog(usersStringList);
    dialog->set_face_label(DEFAULTFACE);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &CreateUserDialog::user_info_send, this, [=](QString username, QString pwd, QString pin, int atype, bool autologin){
        createUser(username, pwd, pin, atype, autologin);
    });
    dialog->exec();
}

void UserInfo::show_create_user_dialog_slot(){
    allUserInfoMap.keys();
    QStringList usersStringList;

    for (QVariant tmp : allUserInfoMap.keys()){
        usersStringList << tmp.toString();
    }

    CreateUserDialog * dialog = new CreateUserDialog(usersStringList);
    dialog->set_face_label(DEFAULTFACE);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, SIGNAL(user_info_send(QString,QString,QString,int, bool)), this, SLOT(create_user_slot(QString,QString,QString,int, bool)));
    dialog->exec();
}

void UserInfo::createUser(QString username, QString pwd, QString pin, int atype, bool autologin){
    Q_UNUSED(pin); Q_UNUSED(autologin);
    sysdispatcher->create_user(username, "", atype);

    //使用全局变量传递新建用户密码
    _newUserPwd = pwd;
}

void UserInfo::create_user_slot(QString username, QString pwd, QString pin, int atype, bool autologin){
    Q_UNUSED(pin); Q_UNUSED(autologin);
    sysdispatcher->create_user(username, "", atype);

    pwdcreate = ""; //重置
    pwdcreate = pwd;
}

void UserInfo::createUserDone(QString objpath){
    UserDispatcher * userdispatcher  = new UserDispatcher(objpath);
    //设置默认头像
    userdispatcher->change_user_face(DEFAULTFACE);
    //设置默认密码
    userdispatcher->change_user_pwd(_newUserPwd, "");

    //刷新全部用户信息
    _acquireAllUsersInfo();
    //重建其他用户ListWidget高度
    _resetListWidgetHeigh();

    //获取新建用户信息
    UserInfomation user;
    user = _acquireUserInfo(objpath);

    //构建Item
    _buildWidgetForItem(user);
}

void UserInfo::create_user_done_slot(QString objpath){
    //设置默认头像
    UserDispatcher * userdispatcher  = new UserDispatcher(objpath);
    userdispatcher->change_user_face(DEFAULTFACE);
    userdispatcher->change_user_pwd(pwdcreate, "");

    UserInfomation user;
    user = init_user_info(objpath);

    build_item_with_widget(user);

    //刷新界面
    get_all_users();
    ui_status_init();

}

void UserInfo::showDeleteUserDialog(QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    DelUserDialog * dialog = new DelUserDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->set_face_label(user.iconfile);
    dialog->set_username_label(user.username);
    connect(dialog, &DelUserDialog::removefile_send, this, [=](bool removeFile, QString userName){
        deleteUser(removeFile, userName);
    });
    dialog->exec();
}

void UserInfo::show_del_user_dialog_slot(QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    DelUserDialog * dialog = new DelUserDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->set_face_label(user.iconfile);
    dialog->set_username_label(user.username);
    connect(dialog, SIGNAL(removefile_send(bool, QString)), this, SLOT(delete_user_slot(bool, QString)));
    dialog->exec();
}

void UserInfo::deleteUser(bool removefile, QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    sysdispatcher->delete_user(user.uid, removefile);
}

void UserInfo::delete_user_slot(bool removefile, QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    sysdispatcher->delete_user(user.uid, removefile);
}

void UserInfo::deleteUserDone(QString objpath){
    QListWidgetItem * item = otherUserItemMap.value(objpath);

    //删除Item
    ui->listWidget->takeItem(ui->listWidget->row(item));

    //更新其他用户QMap
    otherUserItemMap.remove(objpath);

    //更新所有用户信息
    _acquireAllUsersInfo();

    //重置其他用户ListWidget高度
    _resetListWidgetHeigh();
}

void UserInfo::delete_user_done_slot(QString objpath){
    QMap<QString, QListWidgetItem *>::iterator it = otherItemMap.begin();
    QString del;

    for (; it != otherItemMap.end(); it++){
        QListWidgetItem * item = (QListWidgetItem *) it.value();
        if (item->data(Qt::UserRole).toString() == objpath){
            del = QString(it.key());
            ui->listWidget->takeItem(ui->listWidget->row(item));
            break;
        }
    }

    if (otherItemMap.contains(del))
        otherItemMap.remove(del);

    //
    get_all_users();
}

void UserInfo::showChangeTypeDialog(QString username){
    if (allUserInfoMap.keys().contains(username)){
        UserInfomation user = allUserInfoMap.value(username);

        ChangeTypeDialog * dialog = new ChangeTypeDialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->set_face_label(user.iconfile);
        dialog->set_username_label(user.username);
        dialog->set_account_type_label(accounttype_enum_to_string(user.accounttype));
        dialog->set_current_account_type(user.accounttype);
        dialog->set_autologin_status(user.autologin);
//        connect(dialog, SIGNAL(type_send(int,QString,bool)), this, SLOT(change_accounttype_slot(int,QString,bool)));
        connect(dialog, &ChangeTypeDialog::type_send, this, [=](int atype, QString userName){
            changeUserType(atype, userName);
        });
        dialog->exec();

    } else {
        qDebug() << "User Data Error When Change User type";
    }
}

void UserInfo::show_change_accounttype_dialog_slot(QString username = g_get_user_name()){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    ChangeTypeDialog * dialog = new ChangeTypeDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->set_face_label(user.iconfile);
    dialog->set_username_label(user.username);
    dialog->set_account_type_label(accounttype_enum_to_string(user.accounttype));
    dialog->set_current_account_type(user.accounttype);
    dialog->set_autologin_status(user.autologin);
    connect(dialog, SIGNAL(type_send(int,QString,bool)), this, SLOT(change_accounttype_slot(int,QString)));
    dialog->exec();
}

void UserInfo::changeUserType(int atype, QString username){
    UserInfomation user = allUserInfoMap.value(username);

    //构建dbus调度对象
    UserDispatcher * userdispatcher  = new UserDispatcher(user.objpath); //继承QObject不再删除

    //更改用户类型
    userdispatcher->change_user_type(atype);

    //重新获取全部用户QMap
    _acquireAllUsersInfo();

    //更新界面显示
    _refreshUserInfoUI();
}

void UserInfo::change_accounttype_slot(int atype, QString username, bool status){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    UserDispatcher * userdispatcher  = new UserDispatcher(user.objpath);

    if (user.accounttype != atype){
        userdispatcher->change_user_type(atype);
    }

    if (user.autologin != status)
        userdispatcher->change_user_autologin(status);

    //刷新界面
    get_all_users();
    ui_status_init();
}

void UserInfo::change_accounttype_done_slot(){
    qDebug() << "**********";
}

void UserInfo::show_change_face_dialog_slot(QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    ChangeFaceDialog * dialog = new ChangeFaceDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->set_face_label(user.iconfile);
    dialog->set_username_label(user.username);
    dialog->set_account_type_label(accounttype_enum_to_string(user.accounttype));
    dialog->set_face_list_status(user.iconfile);
    connect(dialog, SIGNAL(face_file_send(QString,QString)), this, SLOT(change_face_slot(QString,QString)));
    dialog->exec();
}

void UserInfo::change_face_slot(QString facefile, QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    UserDispatcher * userdispatcher  = new UserDispatcher(user.objpath);
    userdispatcher->change_user_face(facefile);

    get_all_users();
    ui_status_init();

    //拷贝设置的头像文件到~/.face
    sysinterface = new QDBusInterface("com.control.center.qt.systemdbus",
                                     "/",
                                     "com.control.center.interface",
                                     QDBusConnection::systemBus());

    if (!sysinterface->isValid()){
        qCritical() << "Create Client Interface Failed When Copy Face File: " << QDBusConnection::systemBus().lastError();
        return;
    }

    QString cmd = QString("cp %1 /home/%2/.face").arg(facefile).arg(user.username);

    QDBusReply<QString> reply =  sysinterface->call("systemRun", QVariant(cmd));
}

void UserInfo::showChangePwdDialog(QString username){
    if (allUserInfoMap.keys().contains(username)){
        UserInfomation user = allUserInfoMap.value(username);

        ChangePwdDialog * dialog = new ChangePwdDialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->set_faceLabel(user.iconfile);
        dialog->set_usernameLabel(user.username);
        connect(dialog, &ChangePwdDialog::passwd_send, this, [=](QString pwd, QString userName){
            changeUserPwd(pwd, userName);
        });
        dialog->exec();

    } else {
        qDebug() << "User Info Data Error When Change User type";
    }

}

void UserInfo::show_change_pwd_dialog_slot(QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    ChangePwdDialog * dialog = new ChangePwdDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->set_faceLabel(user.iconfile);
    dialog->set_usernameLabel(user.username);
    connect(dialog, SIGNAL(passwd_send(QString, QString)), this, SLOT(change_pwd_slot(QString, QString)));
    dialog->exec();
}

void UserInfo::changeUserPwd(QString pwd, QString username){
    //上层已做判断，这里不去判断而直接获取
    UserInfomation user = allUserInfoMap.value(username);

    UserDispatcher * userdispatcher  = new UserDispatcher(user.objpath); //继承QObject不再删除
    QString result = userdispatcher->change_user_pwd(pwd, "");

    Q_UNUSED(result)
}

void UserInfo::change_pwd_slot(QString pwd, QString username){
    UserInfomation user = (UserInfomation)(allUserInfoMap.find(username).value());

    UserDispatcher * userdispatcher  = new UserDispatcher(user.objpath);
    QString result = userdispatcher->change_user_pwd(pwd, "");
}
