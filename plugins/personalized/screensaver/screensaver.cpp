#include "screensaver.h"
#include "ui_screensaver.h"

#include <QDebug>

#define SSTHEMEPATH "/usr/share/applications/screensavers/"
#define ID_PREFIX "screensavers-ukui-"

#define SCREENSAVER_SCHEMA "org.ukui.screensaver"
#define MODE_KEY "mode"
#define THEMES_KEY "themes"
#define LOCK_KEY "lock-enabled"
#define ACTIVE_KEY "idle-activation-enabled"

#define SESSION_SCHEMA "org.ukui.session"
#define IDLE_DELAY_KEY "idle-delay"

typedef enum
{
    MODE_BLANK_ONLY,
    MODE_RANDOM,
    MODE_SINGLE
}SaverMode;

Screensaver::Screensaver()
{
    ui = new Ui::Screensaver;
    pluginWidget = new CustomWidget;
    pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(pluginWidget);

    pluginName = tr("screensaver");
    pluginType = PERSONALIZED;

    init_theme_info_map();
    component_init();
    status_init();

    process = new QProcess();
}

Screensaver::~Screensaver()
{
    delete ui;
    delete process;
    process = nullptr;
}

QString Screensaver::get_plugin_name(){
    return pluginName;
}

int Screensaver::get_plugin_type(){
    return pluginType;
}

CustomWidget *Screensaver::get_plugin_ui(){
    return pluginWidget;
}

void Screensaver::plugin_delay_control(){
    // 启动屏保预览程序
    kill_and_start();
}

void Screensaver::component_init(){

    //设置屏保预览widget的背景为黑色
    ui->previewWidget->setStyleSheet("#previewWidget{background: black}");

    //
    activeswitchbtn = new SwitchButton();
    activeswitchbtn->setAttribute(Qt::WA_DeleteOnClose);
    ui->activeHLayout->addWidget(activeswitchbtn);
    ui->activeHLayout->addStretch();

    lockswitchbtn = new SwitchButton();
    lockswitchbtn->setAttribute(Qt::WA_DeleteOnClose);
    ui->lockHLayout->addWidget(lockswitchbtn);
    ui->lockHLayout->addStretch();

    ui->comboBox->addItem(tr("Blank_Only"));
    ui->comboBox->addItem(tr("Random"));

    QMap<QString, SSThemeInfo>::iterator it = infoMap.begin();
    for (int itemsindex = 2; it != infoMap.end(); it++, itemsindex++){
        SSThemeInfo info = (SSThemeInfo)it.value();
        ui->comboBox->addItem(info.name);
        ui->comboBox->setItemData(itemsindex, QVariant::fromValue(info));
    }

    //init slider
    int min = 1;
    int max = 120;
    int singlestep = 1;

    ui->idleSlider->setMinimum(min);
    ui->idleSlider->setMaximum(max);
    ui->idleSlider->setSingleStep(singlestep);
    ui->idleSlider->installEventFilter(this);

    connect(this, SIGNAL(kill_signals()), this, SLOT(kill_screensaver_preview()));
}

void Screensaver::status_init(){
    int mode;
    char * name;

    screensaver_settings = g_settings_new(SCREENSAVER_SCHEMA);
    mode = g_settings_get_enum(screensaver_settings, MODE_KEY);
    if (mode == MODE_BLANK_ONLY){
        ui->comboBox->setCurrentIndex(0); //Black_Only
    }
    else if (mode == MODE_RANDOM){
        ui->comboBox->setCurrentIndex(1); //Random
    }
    else{
        gchar ** strv;
        strv = g_settings_get_strv(screensaver_settings, THEMES_KEY);

        if (strv != NULL){
            name = g_strdup(strv[0]);
            QString nameqt = QString(name);
            SSThemeInfo info = (SSThemeInfo)infoMap.find(name).value();
            ui->comboBox->setCurrentText(info.name);
        }
        else
            ui->comboBox->setCurrentIndex(0); //no data, default Blank_Only
        g_strfreev(strv);
    }

    //init
    bool activation; bool lockable;
    activation = g_settings_get_boolean(screensaver_settings, ACTIVE_KEY);
    activeswitchbtn->setChecked(activation);
    if (activation){
        lockable = g_settings_get_boolean(screensaver_settings, LOCK_KEY);
        lockswitchbtn->setChecked(lockable);
        ui->widget->show();
    }
    else{
        lockswitchbtn->setChecked(false);
        ui->widget->hide();
    }

    g_object_unref(screensaver_settings);

    //获取空闲时间
    int minutes;
    session_settings = g_settings_new(SESSION_SCHEMA);
    minutes = g_settings_get_int(session_settings, IDLE_DELAY_KEY);
    ui->idleSlider->setValue(minutes);
    ui->idleLabel->setText(QString("%1%2").arg(minutes).arg(tr("minutes")));

    g_object_unref(session_settings);

    //获取功能列表
    PublicData * publicdata = new PublicData();
    QStringList tmpList = publicdata->subfuncList[SYSTEM];

    //connect
    connect(ui->powerBtn, &QPushButton::clicked, this, [=]{pluginWidget->emitting_toggle_signal(tmpList.at(2), SYSTEM, 0);});

    connect(ui->idleSlider, SIGNAL(valueChanged(int)), this, SLOT(slider_changed_slot(int))); //改label
    connect(ui->idleSlider, SIGNAL(sliderReleased()), this, SLOT(slider_released_slot())); //改gsettings
    connect(activeswitchbtn, SIGNAL(checkedChanged(bool)), this, SLOT(activebtn_changed_slot(bool)));
    connect(lockswitchbtn, SIGNAL(checkedChanged(bool)), this, SLOT(lockbtn_changed_slot(bool)));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(combobox_changed_slot(int)));

    connect(ui->previewWidget, SIGNAL(destroyed(QObject*)), this, SLOT(kill_screensaver_preview()));

    delete publicdata;
}

void Screensaver::kill_and_start(){
    emit kill_signals(); //如果有屏保先杀死
    if (ui->comboBox->currentIndex() == 0){//黑屏
        ui->previewWidget->update();
    }
    else if (ui->comboBox->currentIndex() == 1){//随机
        ui->previewWidget->update();
    }
    else{//屏保
        SSThemeInfo info = ui->comboBox->currentData().value<SSThemeInfo>();
        QStringList args;
        args << "-window-id" << QString::number(ui->previewWidget->winId());
        //启动屏保
        process->startDetached(info.exec, args);
        killList.clear(); killList.append(info.exec);
    }
}


void Screensaver::set_idle_gsettings_value(int value){
    session_settings = g_settings_new(SESSION_SCHEMA);
    g_settings_set_int(session_settings, IDLE_DELAY_KEY, value);
}

void Screensaver::slider_released_slot(){
    int minutes;
    minutes = ui->idleSlider->value();
    set_idle_gsettings_value(minutes);
}

void Screensaver::slider_changed_slot(int value){
    ui->idleLabel->setText(QString("%1%2").arg(value).arg(tr("minutes")));
}

void Screensaver::lockbtn_changed_slot(bool status){
    //REVIEW***  setchecked(false) -> g_object_unref faild
//    screensaver_settings = g_settings_new(SCREENSAVER_SCHEMA);
//    g_settings_set_boolean(screensaver_settings, LOCK_KEY, status);
//    if (screensaver_settings)
//            g_object_unref(screensaver_settings);
    const QByteArray ba(SCREENSAVER_SCHEMA);
    QGSettings * settings = new QGSettings(ba);
    settings->set(LOCK_KEY, status);
    delete settings;
}

void Screensaver::activebtn_changed_slot(bool status){
    screensaver_settings = g_settings_new(SCREENSAVER_SCHEMA);
    g_settings_set_boolean(screensaver_settings, ACTIVE_KEY, status);

    if (status){
        ui->widget->show();
    }
    else{
        lockswitchbtn->setChecked(false);
        ui->widget->hide();

    }
    g_object_unref(screensaver_settings);

}

void Screensaver::combobox_changed_slot(int index){
    char ** strv = NULL;


    screensaver_settings = g_settings_new(SCREENSAVER_SCHEMA);
    if (index == 0){ //Blank_Only
        g_settings_set_enum(screensaver_settings, MODE_KEY, MODE_BLANK_ONLY);
    }
    else if (index == 1){ //Random
//        int mode = MODE_RANDOM;
        g_settings_set_enum(screensaver_settings, MODE_KEY, MODE_RANDOM);
        //REVIEW*** 二维字符数组赋值字符串段错误？
//        QMap<QString, SSThemeInfo>::iterator it = infoMap.begin();
//        for (guint num = 0; it != infoMap.end(); it++){
//            QString id = QString(it.key());
//            QByteArray ba = id.toLatin1();
//            char * info_id = ba.data();
//            strv[num++] = g_strdup)(info_id);
//        }
        //改用qt的gsetting
        QStringList valueStringList;
        const QByteArray ba(SCREENSAVER_SCHEMA);
        QGSettings * settings = new QGSettings(ba);
        QMap<QString, SSThemeInfo>::iterator it = infoMap.begin();
        for (; it != infoMap.end(); it++){
            QString id = QString(it.key());
            valueStringList.append(id);
        }
        settings->set(THEMES_KEY, QVariant(valueStringList));
        delete settings;
    }
    else{
        g_settings_set_enum(screensaver_settings, MODE_KEY, MODE_SINGLE);
        //获取当前屏保的id
        QVariant variant = ui->comboBox->itemData(index);
        SSThemeInfo info = variant.value<SSThemeInfo>();
        QByteArray ba = info.id.toLatin1();
        strv = g_strsplit(ba.data(), "%%%", 1);
        g_settings_set_strv(screensaver_settings, THEMES_KEY, (const gchar * const*)strv);
    }
    //set mode
//    g_settings_set_enum(screensaver_settings, MODE_KEY, mode);
    //set themes
//    g_settings_set_strv(screensaver_settings, THEMES_KEY, (const gchar * const*)strv);

    g_object_unref(screensaver_settings);
    g_strfreev(strv);

    //启动屏保
    kill_and_start();
}

void Screensaver::kill_screensaver_preview(){
    //需要手动杀死分离启动的屏保预览程序
    if (!killList.isEmpty()){
        process->start(QString("killall"), killList);
        process->waitForStarted();
        process->waitForFinished(2000);

        killList.clear();
    }
}

SSThemeInfo Screensaver::_info_new(const char *path){
    SSThemeInfo info;
    GKeyFile * keyfile;
    char * name, * exec;

    keyfile = g_key_file_new();
    if (!g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL)){
        g_key_file_free (keyfile);
        return info;
    }

    name = g_key_file_get_locale_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL);
    exec = g_key_file_get_string(keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TRY_EXEC, NULL);

    info.name = QString::fromUtf8(name);
    info.exec = QString::fromUtf8(exec);
    info.id = ID_PREFIX + info.name.toLower();

    g_key_file_free(keyfile);

    return info;
}

void Screensaver::init_theme_info_map(){
    GDir * dir;
    const char * name;

    infoMap.clear();

    dir = g_dir_open(SSTHEMEPATH, 0, NULL);
    if (!dir)
        return;
    while ((name = g_dir_read_name(dir))) {
        SSThemeInfo info;
        char * desktopfilepath;
        if (!g_str_has_suffix(name, ".desktop"))
            continue;
        desktopfilepath = g_build_filename(SSTHEMEPATH, name, NULL);
        info = _info_new(desktopfilepath);
        infoMap.insert(info.id, info);
        g_free (desktopfilepath);
    }
    g_dir_close(dir);
}
