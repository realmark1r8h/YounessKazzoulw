#include "autoboot.h"
#include "ui_autoboot.h"

#include <QDebug>

#define SYSTEMPOS 2
#define ALLPOS 1
#define LOCALPOS 0

AutoBoot::AutoBoot(){
    ui = new Ui::AutoBoot;
    pluginWidget = new CustomWidget;
    pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(pluginWidget);

    pluginName = tr("autoboot");
    pluginType = SYSTEM;

    ui->tableWidget->setStyleSheet("background-color: #f5f6f7");

    // 隐藏行头
    ui->tableWidget->verticalHeader()->hide();

    // 选择整行
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 行平均填充
//    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    localconfigdir = g_build_filename(g_get_user_config_dir(), "autostart", NULL);
    //初始化添加界面
    dialog = new AddAutoBoot();

    if (ui->tableWidget->currentIndex().row() == -1)
        ui->delBtn->setEnabled(false);

    initUI();

    //选中后点亮删除按钮
    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, [=]{if (ui->tableWidget->currentRow() != -1) ui->delBtn->setEnabled(true);});

    connect(ui->addBtn, &QToolButton::clicked, this, [=]{dialog->exec();});
    connect(ui->delBtn, SIGNAL(clicked(bool)), this, SLOT(del_autoboot_realize_slot()));
    connect(dialog, SIGNAL(autoboot_adding_signals(QString,QString,QString)), this, SLOT(add_autoboot_realize_slot(QString,QString,QString)));
}

AutoBoot::~AutoBoot()
{
    delete ui;
    delete dialog;
    g_free(localconfigdir);
}

QString AutoBoot::get_plugin_name(){
    return pluginName;
}

int AutoBoot::get_plugin_type(){
    return pluginType;
}

CustomWidget *AutoBoot::get_plugin_ui(){
    return pluginWidget;
}

void AutoBoot::plugin_delay_control(){

}

void AutoBoot::initUI(){
    _walk_config_dirs();

    appgroupMultiMaps.clear();
    //设置列头
    QStringList headers;
    headers << tr("Name") << tr("Status");
    ui->tableWidget->setColumnCount(headers.count());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    //列宽
    ui->tableWidget->setColumnWidth(0, 440);
    ui->tableWidget->setColumnWidth(1, 318);
    //列头高度
    ui->tableWidget->horizontalHeader()->setMinimumHeight(38);

    ui->tableWidget->setRowCount(statusMaps.count());
    QSignalMapper * checkSignalMapper = new QSignalMapper(this);
    QMap<QString, AutoApp>::iterator it = statusMaps.begin();
    for (int i = 0; it != statusMaps.end(); it++, i++){
        QTableWidgetItem * item = new QTableWidgetItem(it.value().qicon, it.value().name);
        ui->tableWidget->setItem(i, 0, item);
        item->setData(Qt::UserRole, it.value().bname);

        SwitchButton * button = new SwitchButton();
        button->setAttribute(Qt::WA_DeleteOnClose);
        button->setChecked(it.value().enable);
        connect(button, SIGNAL(checkedChanged(bool)), checkSignalMapper, SLOT(map()));
        checkSignalMapper->setMapping(button, it.key());
        appgroupMultiMaps.insert(it.key(), button);
        ui->tableWidget->setCellWidget(i, 1, button);

        //设置行高
        ui->tableWidget->setRowHeight(i, 46);

        qDebug() << "----pos--->" << it.value().xdg_position << it.value().bname << it.value().path;
    }
    connect(checkSignalMapper, SIGNAL(mapped(QString)), this, SLOT(checkbox_changed_cb(QString)));
}

bool AutoBoot::_copy_desktop_file_to_local(QString bname){
    GFile * srcfile;
    GFile * dstfile;
    char * dstpath, * srcpath;

    //不存在则创建~/.config/autostart/
    if (!g_file_test(localconfigdir, G_FILE_TEST_EXISTS)){
        GFile * dstdirfile;
        dstdirfile = g_file_new_for_path(localconfigdir);
        g_file_make_directory(dstdirfile, NULL, NULL);
    }

    QMap<QString, AutoApp>::iterator it = appMaps.find(bname);
    dstpath = g_build_filename(localconfigdir, bname.toUtf8().data(), NULL);
    srcpath = it.value().path.toUtf8().data();

    srcfile = g_file_new_for_path(srcpath);
    dstfile = g_file_new_for_path(dstpath);

    if (!g_file_copy(srcfile, dstfile, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL)){
        qDebug() << "Could not copy desktop file for autoboot";
        g_object_unref(srcfile);
        g_object_unref(dstfile);
        g_free(dstpath);
        return false;
    }

    //更新数据
    AutoApp addapp;
    addapp = _app_new(dstpath);
    addapp.xdg_position = ALLPOS;

    localappMaps.insert(addapp.bname, addapp);

    QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
    updateit.value().xdg_position = ALLPOS;
    updateit.value().path = QString(dstpath);

    g_object_unref(srcfile);
    g_object_unref(dstfile);
    g_free(dstpath);
    return true;
}

bool AutoBoot::_delete_local_autoapp(QString bname){
    char * dstpath;

    dstpath = g_build_filename(localconfigdir, bname.toUtf8().data(), NULL);

    if (g_remove(dstpath) == -1){
        g_free(dstpath);
        return false;
    }

    //更新数据
    localappMaps.remove(bname);

    QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
    if (updateit == statusMaps.end())
        qDebug() << "statusMaps Data Error when delete local file";
    else{
        QMap<QString, AutoApp>::iterator appit = appMaps.find(bname);
        if (appit == appMaps.end())
            qDebug() << "appMaps Data Error when delete local file";
        else{
            updateit.value().enable = appit.value().enable;
            updateit.value().path = appit.value().path;
        }
        updateit.value().xdg_position = SYSTEMPOS;
    }

    g_free(dstpath);
    return true;
}

bool AutoBoot::_enable_autoapp(QString bname, bool status){
    char * dstpath;

    dstpath = g_build_filename(localconfigdir, bname.toUtf8().data(), NULL);

    //获取并修改值
    GKeyFile * keyfile;
    GError * error;
    keyfile = g_key_file_new();
    error = NULL;
    g_key_file_load_from_file(keyfile, dstpath, G_KEY_FILE_KEEP_COMMENTS, &error);


    if (error){
        g_error_free(error);
        qDebug() << "Start autoboot failed  beause keyfile load from file error";
        g_free(dstpath);
        return false;
    }

    g_key_file_set_boolean(keyfile, G_KEY_FILE_DESKTOP_GROUP, APP_KEY_FILE_DESKTOP_KEY_AUTOSTART_ENABLE, status);

    if (!_key_file_to_file(keyfile, dstpath)){
        qDebug() << "Start autoboot failed beause could not save desktop file";
        g_free(dstpath);
        return false;
    }

    g_key_file_free(keyfile);

    //更新数据
    QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
    if (updateit == statusMaps.end())
        qDebug() << "Start autoboot failed beause autoBoot Data Error";
    else{
        updateit.value().enable = status;
    }

    g_free(dstpath);
    return true;

}

bool AutoBoot::_delete_autoapp(QString bname){
    char * dstpath;

    dstpath = g_build_filename(localconfigdir, bname.toUtf8().data(), NULL);

    //获取并修改值
    GKeyFile * keyfile;
    GError * error;
    keyfile = g_key_file_new();
    error = NULL;
    g_key_file_load_from_file(keyfile, dstpath, G_KEY_FILE_KEEP_COMMENTS, &error);


    if (error){
        g_error_free(error);
        qDebug() << "Delete autoboot failed beause keyfile load from file error";
        g_free(dstpath);
        return false;
    }

    g_key_file_set_boolean(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, true);

    if (!_key_file_to_file(keyfile, dstpath)){
        qDebug() << "Delete autoboot failed beause could not save desktop file";
        g_free(dstpath);
        return false;
    }

    //刷新界面
    ui->tableWidget->clear();
    initUI();

    g_free(dstpath);
    return true;
}


bool AutoBoot::_stop_autoapp(QString bname){

    char * dstpath;

    dstpath = g_build_filename(localconfigdir, bname.toUtf8().data(), NULL);

    //获取并修改值
    GKeyFile * keyfile;
    GError * error;
    keyfile = g_key_file_new();
    error = NULL;
    g_key_file_load_from_file(keyfile, dstpath, G_KEY_FILE_KEEP_COMMENTS, &error);


    if (error){
        g_error_free(error);
        qDebug() << "Stop autoboot failed beause keyfile load from file error";
        g_free(dstpath);
        return false;
    }

    g_key_file_set_boolean(keyfile, G_KEY_FILE_DESKTOP_GROUP, APP_KEY_FILE_DESKTOP_KEY_AUTOSTART_ENABLE, false);

    if (!_key_file_to_file(keyfile, dstpath)){
        qDebug() << "Stop autoboot failed beause could not save desktop file";
        g_free(dstpath);
        return false;
    }

    g_key_file_free(keyfile);

    //更新数据
    QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
    if (updateit == statusMaps.end())
        qDebug() << "Stop autoboot failed beause AutoBoot Data Error";
    else{
        updateit.value().enable = false;
    }

    g_free(dstpath);
    return true;
}

gboolean AutoBoot::_key_file_to_file(GKeyFile *keyfile, const gchar *path){
    GError * werror;
    gchar * data;
    gsize length;
    gboolean res;

    werror = NULL;
    data = g_key_file_to_data(keyfile, &length, &werror);

    if (werror)
        return FALSE;

    res = g_file_set_contents(path, data, length, &werror);
    g_free(data);

    if (werror)
        return FALSE;

    return res;

}

gboolean AutoBoot::_key_file_get_shown(GKeyFile *keyfile, const char *currentdesktop){
    char ** only_show_in, ** not_show_in;
    gboolean found;

    if (!currentdesktop)
        return TRUE;

    only_show_in = g_key_file_get_string_list(keyfile, G_KEY_FILE_DESKTOP_GROUP,
                                              G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN, NULL, NULL);
    if (only_show_in){
        found = FALSE;
        for (int i = 0; only_show_in[i] != NULL; i++){
            if (g_strcmp0(currentdesktop, only_show_in[i]) == 0){
                found = TRUE;
                break;
            }
        }
        g_strfreev(only_show_in);
        if (!found)
            return FALSE;
    }

    not_show_in = g_key_file_get_string_list(keyfile, G_KEY_FILE_DESKTOP_GROUP,
                                             G_KEY_FILE_DESKTOP_KEY_NOT_SHOW_IN, NULL, NULL);
    if (not_show_in){
        found = FALSE;
        for (int i = 0; not_show_in[i] != NULL; i++){
            if (g_strcmp0(currentdesktop, not_show_in[i]) == 0){
                found = TRUE;
                break;
            }
        }
        g_strfreev(not_show_in);
        if (found)
            return FALSE;
    }

    return TRUE;
}

gboolean AutoBoot::_key_file_get_boolean(GKeyFile *keyfile, const gchar *key, gboolean defaultvalue){
    GError * error;
    gboolean retval;

    error = NULL;
    retval = g_key_file_get_boolean(keyfile, G_KEY_FILE_DESKTOP_GROUP, key, &error);

    if (error != NULL){
        retval = defaultvalue;
        g_error_free(error);
    }
    return retval;
}

AutoApp AutoBoot::_app_new(const char *path){
    AutoApp app;
    GKeyFile * keyfile;
    char * bname, * obpath, *name, * comment, * exec, * icon;
    bool  hidden, no_display, enable, shown;

    app.bname = "";
    keyfile = g_key_file_new();
    if (!g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)){
        g_key_file_free (keyfile);
        return app;
    }

    bname = g_path_get_basename(path);
    obpath = g_strdup(path);
    hidden = _key_file_get_boolean(keyfile, G_KEY_FILE_DESKTOP_KEY_HIDDEN, FALSE);
    no_display = _key_file_get_boolean(keyfile, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, FALSE);
    enable = _key_file_get_boolean(keyfile, APP_KEY_FILE_DESKTOP_KEY_AUTOSTART_ENABLE, TRUE);
    shown = _key_file_get_shown(keyfile, g_getenv("XDG_CURRENT_DESKTOP"));
    name = g_key_file_get_locale_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL);
    comment = g_key_file_get_locale_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_COMMENT, NULL, NULL);
    exec = g_key_file_get_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, NULL);
    icon = g_key_file_get_locale_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ICON, NULL, NULL);

    app.bname = QString::fromUtf8(bname);
    app.path = QString::fromUtf8(obpath);

    app.hidden = hidden;
    app.no_display = no_display;
    app.shown = shown;
    app.enable = enable;

    app.name = QString::fromUtf8(name);
    app.comment = QString::fromUtf8(comment);
    app.exec = QString::fromUtf8(exec);
    if (icon){
        if (QIcon::hasThemeIcon(QString(icon))){
            app.qicon = QIcon::fromTheme(QString(icon));
        }
        else{
            QString absolutepath = "/usr/share/pixmaps/" + QString(icon);
            app.qicon = QIcon(absolutepath);
        }
    }
    else
        app.qicon = QIcon(QString(":/image/default.png"));

    g_free(bname);
    g_free(obpath);
    g_key_file_free(keyfile);

    return app;
}

void AutoBoot::_walk_config_dirs(){
    const char * const * systemconfigdirs;
    GDir * dir;
    const char * name;

    appMaps.clear();

    systemconfigdirs = g_get_system_config_dirs(); //获取系统配置目录
    for (int i = 0; systemconfigdirs[i]; i++){
        char * path;
        path = g_build_filename(systemconfigdirs[i], "autostart", NULL);
        dir = g_dir_open(path, 0, NULL);
        if (!dir)
            continue;
        while ((name = g_dir_read_name(dir))) {
            AutoApp app;
            char * desktopfilepath;
            if (!g_str_has_suffix(name, ".desktop"))
                continue;
            desktopfilepath = g_build_filename(path, name, NULL);
            app = _app_new(desktopfilepath);
//            if (app.bname == "" || app.hidden || app.no_display || !app.shown ||
//                    app.exec == "/usr/bin/ukui-settings-daemon") //gtk控制面板屏蔽ukui-settings-daemon,猜测禁止用户关闭
//                continue;
            app.xdg_position = SYSTEMPOS;
            appMaps.insert(app.bname, app);
            g_free (desktopfilepath);
        }
        g_dir_close(dir);
    }

    localappMaps.clear();
    dir = g_dir_open(localconfigdir, 0, NULL);
    if (dir){
        while ((name = g_dir_read_name(dir))) {
            AutoApp localapp;
            char * localdesktopfilepath;
            if (!g_str_has_suffix(name, ".desktop"))
                continue;
            localdesktopfilepath = g_build_filename(localconfigdir, name, NULL);
            localapp = _app_new(localdesktopfilepath);
            localapp.xdg_position = LOCALPOS;
            localappMaps.insert(localapp.bname, localapp); 
            g_free(localdesktopfilepath);
        }
        g_dir_close(dir);
    }
    update_app_status();
}

void AutoBoot::update_app_status(){
    statusMaps.clear();

    QMap<QString, AutoApp>::iterator it = appMaps.begin();
    for (; it != appMaps.end(); it++){
        if (it.value().hidden || it.value().no_display || !it.value().shown ||
                it.value().exec == "/usr/bin/ukui-settings-daemon") //gtk控制面板屏蔽ukui-settings-daemon,猜测禁止用户关闭
            continue;
        statusMaps.insert(it.key(), it.value());
    }

    QMap<QString, AutoApp>::iterator localit = localappMaps.begin();
    for (; localit != localappMaps.end(); localit++){
        if (localit.value().hidden || localit.value().no_display || !localit.value().shown){
            statusMaps.remove(localit.key());
            continue;
        }

        if (statusMaps.contains(localit.key())){
            //整合状态
            QMap<QString, AutoApp>::iterator updateit = statusMaps.find(localit.key());

            if (localit.value().enable != updateit.value().enable){
                updateit.value().enable = localit.value().enable;
                updateit.value().path = localit.value().path;
                if (appMaps.contains(localit.key()))
                    updateit.value().xdg_position = ALLPOS;
            }
        }
        else{
            statusMaps.insert(localit.key(), localit.value());
        }

    }
}

void AutoBoot::add_autoboot_realize_slot(QString name, QString exec, QString comment){
    qDebug() << name << exec << comment;
    char * filename, * filepath;
    if (exec.contains("/"))
        filename = QString("%1.desktop").arg(exec.section("/", -1, -1)).toUtf8().data();
    else
        filename = QString("%1.desktop").arg(exec).toUtf8().data();

    if (!g_file_test(localconfigdir, G_FILE_TEST_EXISTS)){
        GFile * dstdirfile;
        dstdirfile = g_file_new_for_path(localconfigdir);
        g_file_make_directory(dstdirfile, NULL, NULL);
    }

    filepath = g_build_filename(localconfigdir, filename, NULL);

    GKeyFile * keyfile;
    keyfile = g_key_file_new();

    const char * locale = const_cast<const char *>(QLocale::system().name().toUtf8().data());
    char * type = QString("Application").toUtf8().data();

    g_key_file_set_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE, type);
    g_key_file_set_boolean(keyfile, G_KEY_FILE_DESKTOP_GROUP, APP_KEY_FILE_DESKTOP_KEY_AUTOSTART_ENABLE, true);
    g_key_file_set_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, exec.toUtf8().data());
    g_key_file_set_boolean(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, false);
    g_key_file_set_boolean(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, false);
    g_key_file_set_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, name.toUtf8().data());
    g_key_file_set_locale_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, locale, name.toUtf8().data());
    g_key_file_set_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_COMMENT, comment.toUtf8().data());
    g_key_file_set_locale_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_COMMENT, locale, comment.toUtf8().data());

    if (!_key_file_to_file(keyfile, filepath))
        qDebug() << "Could not save desktop file";

    g_key_file_free(keyfile);
    g_free(filepath);

    //refresh
    ui->tableWidget->clear();
    initUI();

}

void AutoBoot::del_autoboot_realize_slot(){
    QModelIndex  index = ui->tableWidget->currentIndex();
    QTableWidgetItem * item = ui->tableWidget->item(index.row(), index.column());
    QString bname = item->data(Qt::UserRole).toString();
    QMap<QString, AutoApp>::iterator it = statusMaps.find(bname);
    if (it == statusMaps.end()){
        qDebug() << "AutoBoot Data Error";
        return;
    }

    if (it.value().xdg_position == SYSTEMPOS){ //复制改值
        if (_copy_desktop_file_to_local(bname)){
            _delete_autoapp(bname);
        }
    }
    else if (it.value().xdg_position == ALLPOS){ //改值
        _delete_autoapp(bname);

    }
    else if (it.value().xdg_position == LOCALPOS){ //删除
        _delete_local_autoapp(bname);
        ui->tableWidget->clear();
        initUI();
    }
}

void AutoBoot::checkbox_changed_cb(QString bname){
    foreach (QString key, appgroupMultiMaps.keys()) {
        if (key == bname){

            QMap<QString, AutoApp>::iterator it = statusMaps.find(bname);
            if (it == statusMaps.end()){
                qDebug() << "AutoBoot Data Error";
                return;
            }

            if (((SwitchButton *)appgroupMultiMaps.value(key))->isChecked()){ //开启开机启动
                if (it.value().xdg_position == SYSTEMPOS) //
                    ;
                else if (it.value().xdg_position == ALLPOS){ //删除
                    qDebug() << "-0-->delete";
                    QMap<QString, AutoApp>::iterator appit = appMaps.find(bname);
                    if (appit.value().enable){ //直接删除
                        _delete_local_autoapp(bname);
                        //更新状态
                        QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
                        if (updateit != statusMaps.end()){
                            updateit.value().enable = true;
                            updateit.value().xdg_position = SYSTEMPOS;
                            updateit.value().path = appit.value().path;
                        }
                        else
                            qDebug() << "Update status failed when start autoboot";
                    }
                    QMap<QString, AutoApp>::iterator statusit = statusMaps.begin();
                    for (; statusit != statusMaps.end(); statusit++){
                        qDebug() << statusit.value().xdg_position << statusit.value().path;
                    }
                }
                else if (it.value().xdg_position == LOCALPOS){//改值
                    _enable_autoapp(bname, true);
                }

            }
            else{ //关闭
                if (it.value().xdg_position == SYSTEMPOS){ //复制后改值
                    if (_copy_desktop_file_to_local(bname)){
                        _stop_autoapp(bname);
                    }
                }
                else if (it.value().xdg_position == ALLPOS){//正常逻辑不应存在该情况,预防处理
                    QMap<QString, AutoApp>::iterator appit = appMaps.find(bname);
                    if (!appit.value().enable)
                        _delete_local_autoapp(bname);
                }
                else if (it.value().xdg_position == LOCALPOS){//改值
                    _enable_autoapp(bname, false);
                }

            }
        }
    }
}
