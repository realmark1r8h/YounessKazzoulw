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
#include "ukmedia_main_widget.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QStringList>
#include <QSpacerItem>
#include <QListView>
#include <QScrollBar>
#include <QGSettings>
#include <QPixmap>
#include <qmath.h>
#define MATE_DESKTOP_USE_UNSTABLE_API
#define VERSION "1.12.1"
#define GVC_DIALOG_DBUS_NAME "org.mate.VolumeControl"
#define KEY_SOUNDS_SCHEMA   "org.mate.sound"
#define GVC_SOUND_SOUND    (xmlChar *) "sound"
#define GVC_SOUND_NAME     (xmlChar *) "name"
#define GVC_SOUND_FILENAME (xmlChar *) "filename"
#define SOUND_SET_DIR "/usr/share/ukui-media/sounds"

#define KEYBINDINGS_CUSTOM_SCHEMA "org.ukui.media.sound"
#define KEYBINDINGS_CUSTOM_DIR "/org/ukui/sound/keybindings/"

#define MAX_CUSTOM_SHORTCUTS 1000

#define FILENAME_KEY "filename"
#define NAME_KEY "name"

guint appnum = 0;
typedef enum {
    BALANCE_TYPE_RL,
    BALANCE_TYPE_FR,
    BALANCE_TYPE_LFE,
} GvcBalanceType;

enum {
    SOUND_TYPE_UNSET,
    SOUND_TYPE_OFF,
    SOUND_TYPE_DEFAULT_FROM_THEME,
    SOUND_TYPE_BUILTIN,
    SOUND_TYPE_CUSTOM
};
static void callback(ca_context *c, uint32_t id, int error, void *userdata) {
        fprintf(stderr, "callback called for id %u, error '%s', userdata=%p\n", id, ca_strerror(error), userdata);
}
UkmediaMainWidget::UkmediaMainWidget(QWidget *parent)
    : QWidget(parent)
{
    m_pOutputWidget = new UkmediaOutputWidget();
    m_pInputWidget = new UkmediaInputWidget();
    m_pSoundWidget = new UkmediaSoundEffectsWidget();

    mThemeName = UKUI_THEME_WHITE;
    QVBoxLayout *m_pvLayout = new QVBoxLayout();
    m_pvLayout->addWidget(m_pOutputWidget);
    m_pvLayout->addWidget(m_pInputWidget);
    m_pvLayout->addWidget(m_pSoundWidget);
    m_pvLayout->addSpacing(48);
    m_pvLayout->addSpacerItem(new QSpacerItem(20,0,QSizePolicy::Fixed,QSizePolicy::Expanding));
    m_pvLayout->setSpacing(48);
    this->setLayout(m_pvLayout);
    this->setMinimumWidth(582);
    this->setMaximumWidth(910);
    this->layout()->setContentsMargins(0,0,31,0);
//    this->setStyleSheet("QWidget{background: white;}");

    if (mate_mixer_init() == FALSE) {
        qDebug() << "libmatemixer initialization failed, exiting";
    }

    m_pSoundList = new QStringList;
    m_pThemeNameList = new QStringList;
    m_pThemeDisplayNameList = new QStringList;
    m_pDeviceNameList = new QStringList;
    m_pOutputStreamList = new QStringList;
    m_pInputStreamList = new QStringList;
    m_pAppVolumeList = new QStringList;
    m_pStreamControlList = new QStringList;
    m_pAppNameList = new QStringList;
    m_pInputPortList = new QStringList;
    m_pOutputPortList = new QStringList;
    m_pSoundNameList = new QStringList;
    m_pProfileNameList = new QStringList;

    eventList = new QStringList;
    eventIdNameList = new QStringList;

    eventList->append("window-close");
    eventList->append("system-setting");
    eventList->append("volume-changed");
    eventList->append("alert-sound");
    eventIdNameList->append("dialog-warning");
    eventIdNameList->append("bell");
    eventIdNameList->append("audio-volume-change");
    eventIdNameList->append("drip");

    for (int i=0;i<eventList->count();i++) {
//        getValue();
        addValue(eventList->at(i),eventIdNameList->at(i));
    }

    //??????context
    m_pContext = mate_mixer_context_new();

    mate_mixer_context_set_app_name (m_pContext,_("Volume Control"));//??????app???
    mate_mixer_context_set_app_icon(m_pContext,"multimedia-volume-control");

    //??????context
    if G_UNLIKELY (mate_mixer_context_open(m_pContext) == FALSE) {
        g_warning ("Failed to connect to a sound system**********************");
    }

    g_param_spec_object ("context",
                        "Context",
                        "MateMixer context",
                        MATE_MIXER_TYPE_CONTEXT,
                        (GParamFlags)(G_PARAM_READWRITE |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_STATIC_STRINGS));

    if (mate_mixer_context_get_state (m_pContext) == MATE_MIXER_STATE_CONNECTING) {

    }

    //??????????????????????????????????????????????????????????????????stream
    contextSetProperty(this);
    m_pInputStream = mate_mixer_context_get_default_input_stream(m_pContext);
    m_pOutputStream = mate_mixer_context_get_default_output_stream(m_pContext);

    connect(m_pInputWidget->m_pInputIconBtn,SIGNAL(clicked()),this,SLOT(inputMuteButtonSlot()));
    connect(m_pOutputWidget->m_pOutputIconBtn,SIGNAL(clicked()),this,SLOT(outputMuteButtonSlot()));
    g_signal_connect (G_OBJECT (m_pContext),
                     "notify::state",
                     G_CALLBACK (onContextStateNotify),
                     this);

    //??????????????????????????????100
    m_pInputWidget->m_pIpVolumeSlider->setMaximum(100);
    m_pOutputWidget->m_pOpVolumeSlider->setMaximum(100);
    m_pOutputWidget->m_pOpBalanceSlider->setMaximum(100);
    m_pOutputWidget->m_pOpBalanceSlider->setMinimum(-100);
    m_pOutputWidget->m_pOpBalanceSlider->setSingleStep(100);
    m_pInputWidget->m_pInputLevelSlider->setMaximum(100);
    m_pInputWidget->m_pInputLevelSlider->setEnabled(false);
    //??????????????????
    //????????????gsettings???
    m_pSoundSettings = g_settings_new (KEY_SOUNDS_SCHEMA);

    g_signal_connect (G_OBJECT (m_pSoundSettings),
                             "changed",
                             G_CALLBACK (onKeyChanged),
                             this);
    //?????????pulseaudio
    pa_glib_mainloop *m = pa_glib_mainloop_new(g_main_context_default());
    api = pa_glib_mainloop_get_api(m);

    role = "sink-input-by-media-role:event";

    setupThemeSelector(this);
    updateTheme(this);
    //????????????,???????????????????????????????????????
    populateModelFromDir(this,SOUND_SET_DIR);
    //?????????combobox??????
    comboboxCurrentTextInit();
    //??????????????????
    if (QGSettings::isSchemaInstalled(UKUI_THEME_SETTING)){
        m_pThemeSetting = new QGSettings(UKUI_THEME_SETTING);
        if (m_pThemeSetting->keys().contains("styleName")) {
            mThemeName = m_pThemeSetting->get(UKUI_THEME_NAME).toString();
        }
        connect(m_pThemeSetting, SIGNAL(changed(const QString &)),this,SLOT(ukuiThemeChangedSlot(const QString &)));
    }

    //???????????????????????????
    if (QGSettings::isSchemaInstalled(UKUI_SWITCH_SETTING)) {
        m_pBootSetting = new QGSettings(UKUI_SWITCH_SETTING);
        if (m_pBootSetting->keys().contains("bootMusic")) {
            m_hasMusic = m_pBootSetting->get(UKUI_BOOT_MUSIC_KEY).toBool();
            m_pSoundWidget->m_pBootButton->setChecked(m_hasMusic);
        }

        connect(m_pBootSetting,SIGNAL(changed(const QString &)),this,SLOT(bootMusicSettingsChanged()));
    }
    bool status = g_settings_get_boolean(m_pSoundSettings, EVENT_SOUNDS_KEY);
    m_pSoundWidget->m_pAlertSoundSwitchButton->setChecked(status);
    if (status) {
        m_pSoundWidget->m_pSoundLayout->insertWidget(5,m_pSoundWidget->m_pAlertSoundVolumeWidget);
    }
    else {
        m_pSoundWidget->m_pAlertSoundVolumeWidget->hide();
    }

    connect(m_pSoundWidget->m_pAlertIconBtn,SIGNAL(clicked()),this,SLOT(alertSoundVolumeChangedSlot()));
    connect(m_pSoundWidget->m_pBootButton,SIGNAL(checkedChanged(bool)),this,SLOT(bootButtonSwitchChangedSlot(bool)));
    connect(m_pSoundWidget->m_pAlertSoundSwitchButton,SIGNAL(checkedChanged(bool)),this,SLOT(alertSoundButtonSwitchChangedSlot(bool)));
    //??????????????????
    //???????????????????????????
    connect(m_pOutputWidget->m_pOpVolumeSlider,SIGNAL(valueChanged(int)),this,SLOT(outputWidgetSliderChangedSlot(int)));
    //???????????????????????????
    connect(m_pInputWidget->m_pIpVolumeSlider,SIGNAL(valueChanged(int)),this,SLOT(inputWidgetSliderChangedSlot(int)));

    //???????????????????????????????????????
    connect(m_pSoundWidget->m_pAlertSlider,SIGNAL(valueChanged(int)),this,SLOT(alertVolumeSliderChangedSlot(int)));
    connect(m_pSoundWidget->m_pAlertSoundCombobox,SIGNAL(currentIndexChanged(int)),this,SLOT(comboxIndexChangedSlot(int)));
    connect(m_pSoundWidget->m_pLagoutCombobox ,SIGNAL(currentIndexChanged(int)),this,SLOT(comboxIndexChangedSlot(int)));
    connect(m_pSoundWidget->m_pSoundThemeCombobox,SIGNAL(currentIndexChanged(int)),this,SLOT(themeComboxIndexChangedSlot(int)));
    connect(m_pInputWidget->m_pInputLevelSlider,SIGNAL(valueChanged(int)),this,SLOT(inputLevelValueChangedSlot()));
//    connect(m_pInputWidget->m_pInputPortCombobox,SIGNAL(currentIndexChanged(int)),this,SLOT(inputPortComboxChangedSlot(int)));
    connect(m_pSoundWidget->m_pWindowClosedCombobox,SIGNAL(currentIndexChanged (int)),this,SLOT(windowClosedComboboxChangedSlot(int)));
    connect(m_pSoundWidget->m_pVolumeChangeCombobox,SIGNAL(currentIndexChanged (int)),this,SLOT(volumeChangedComboboxChangeSlot(int)));
    connect(m_pSoundWidget->m_pSettingSoundCombobox,SIGNAL(currentIndexChanged (int)),this,SLOT(settingMenuComboboxChangedSlot(int)));
    connect(m_pOutputWidget->m_pProfileCombobox,SIGNAL(currentIndexChanged (int)),this,SLOT(profileComboboxChangedSlot(int)));

    connect(m_pOutputWidget->m_pSelectCombobox,SIGNAL(currentIndexChanged (int)),this,SLOT(selectComboboxChangedSlot(int)));

    //????????????
    ukuiInputLevelSetProperty(this);
}

QPixmap UkmediaMainWidget::drawDarkColoredPixmap(const QPixmap &source)
{
//    QColor currentcolor=HighLightEffect::getCurrentSymbolicColor();
    QColor gray(255,255,255);
    QImage img = source.toImage();
    for (int x = 0; x < img.width(); x++) {
        for (int y = 0; y < img.height(); y++) {
            auto color = img.pixelColor(x, y);
            if (color.alpha() > 0) {
                if (qAbs(color.red()-gray.red())<20 && qAbs(color.green()-gray.green())<20 && qAbs(color.blue()-gray.blue())<20) {
                    color.setRed(0);
                    color.setGreen(0);
                    color.setBlue(0);
                    img.setPixelColor(x, y, color);
                }
                else {
                    color.setRed(0);
                    color.setGreen(0);
                    color.setBlue(0);
                    img.setPixelColor(x, y, color);
                }
            }
        }
    }
    return QPixmap::fromImage(img);
}

QPixmap UkmediaMainWidget::drawLightColoredPixmap(const QPixmap &source)
{
//    QColor currentcolor=HighLightEffect::getCurrentSymbolicColor();
    QColor gray(255,255,255);
    QColor standard (0,0,0);
    QImage img = source.toImage();
    for (int x = 0; x < img.width(); x++) {
        for (int y = 0; y < img.height(); y++) {
            auto color = img.pixelColor(x, y);
            if (color.alpha() > 0) {
                if (qAbs(color.red()-gray.red())<20 && qAbs(color.green()-gray.green())<20 && qAbs(color.blue()-gray.blue())<20) {
                    color.setRed(255);
                    color.setGreen(255);
                    color.setBlue(255);
                    img.setPixelColor(x, y, color);
                }
                else {
                    color.setRed(255);
                    color.setGreen(255);
                    color.setBlue(255);
                    img.setPixelColor(x, y, color);
                }
            }
        }
    }
    return QPixmap::fromImage(img);
}

void UkmediaMainWidget::alertIconButtonSetIcon(bool state,int value)
{
    QImage image;
    QColor color = QColor(0,0,0,216);
    if (mThemeName == UKUI_THEME_WHITE) {
        color = QColor(0,0,0,216);
    }
    else if (mThemeName == UKUI_THEME_BLACK) {
        color = QColor(255,255,255,216);
    }
    m_pSoundWidget->m_pAlertIconBtn->mColor = color;
    if (state) {
        image  = QImage("/usr/share/ukui-media/img/audio-volume-muted.svg");
        m_pSoundWidget->m_pAlertIconBtn->mImage = image;
    }
    else if (value <= 0) {
        image  = QImage("/usr/share/ukui-media/img/audio-volume-muted.svg");
        m_pSoundWidget->m_pAlertIconBtn->mImage = image;
    }
    else if (value > 0 && value <= 33) {
        image = QImage("/usr/share/ukui-media/img/audio-volume-low.svg");
        m_pSoundWidget->m_pAlertIconBtn->mImage = image;
    }
    else if (value >33 && value <= 66) {
        image = QImage("/usr/share/ukui-media/img/audio-volume-medium.svg");
        m_pSoundWidget->m_pAlertIconBtn->mImage = image;
    }
    else {
        image = QImage("/usr/share/ukui-media/img/audio-volume-high.svg");
        m_pSoundWidget->m_pAlertIconBtn->mImage = image;
    }

}

void UkmediaMainWidget::createAlertSound(UkmediaMainWidget *pWidget)
{
    const GList   *list;
    connect_to_pulse(this);

    /* Find an event role stored control */
    list = mate_mixer_context_list_stored_controls (pWidget->m_pContext);
    while (list != NULL) {
        MateMixerStreamControl *control = MATE_MIXER_STREAM_CONTROL (list->data);
        MateMixerStreamControlMediaRole media_role;
        MateMixerStream *stream = mate_mixer_stream_control_get_stream(control);
        media_role = mate_mixer_stream_control_get_media_role (control);
        if (media_role == MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_EVENT) {
            pWidget->m_pMediaRoleControl = control;
            //???????????????????????????
            int volume = mate_mixer_stream_control_get_volume(m_pMediaRoleControl);
            volume = int(volume*100/65536.0+0.5);
            pWidget->m_pSoundWidget->m_pAlertSlider->setValue(volume);
            pWidget->m_pSoundWidget->m_pAlertVolumeLabel->setText(QString::number(volume).append("%"));
            qDebug() << "media role : " << mate_mixer_stream_control_get_name(control) <<"??????????????????:" <<volume;
            gboolean bIsMute = mate_mixer_stream_control_get_mute(control);
            alertIconButtonSetIcon(bIsMute,volume);
            ukuiBarSetStream(pWidget,stream);
            break;
        }

        list = list->next;
    }

}

/*
    ?????????combobox??????
*/
void UkmediaMainWidget::comboboxCurrentTextInit()
{
    QList<char *> existsPath = listExistsPath();

    for (char * path : existsPath) {

        char * prepath = QString(KEYBINDINGS_CUSTOM_DIR).toLatin1().data();
        char * allpath = strcat(prepath, path);

        const QByteArray ba(KEYBINDINGS_CUSTOM_SCHEMA);
        const QByteArray bba(allpath);
        if(QGSettings::isSchemaInstalled(ba))
        {
            QGSettings * settings = new QGSettings(ba, bba);
            QString filenameStr = settings->get(FILENAME_KEY).toString();
            QString nameStr = settings->get(NAME_KEY).toString();
            int index = 0;
            for (int i=0;i<m_pSoundList->count();i++) {
                QString str = m_pSoundList->at(i);
                if (str.contains(filenameStr,Qt::CaseSensitive)) {
                    index = i;
                    qDebug() << "str ==========" << str << filenameStr <<m_pSoundNameList->at(index) ;
                    break;
                }
            }
            if (nameStr == "alert-sound") {
                QString displayName = m_pSoundNameList->at(index);
                m_pSoundWidget->m_pAlertSoundCombobox->setCurrentText(displayName);
                continue;
            }
            if (nameStr == "window-close") {
                QString displayName = m_pSoundNameList->at(index);
                m_pSoundWidget->m_pWindowClosedCombobox->setCurrentText(displayName);
                continue;
            }
            else if (nameStr == "volume-changed") {
                QString displayName = m_pSoundNameList->at(index);
                m_pSoundWidget->m_pVolumeChangeCombobox->setCurrentText(displayName);
                continue;
            }
            else if (nameStr == "system-setting") {
                QString displayName = m_pSoundNameList->at(index);
                m_pSoundWidget->m_pSettingSoundCombobox->setCurrentText(displayName);
                continue;
            }
        }
        else {
            continue;
        }
    }
}

/*
    ???????????????????????????
*/
void UkmediaMainWidget::bootButtonSwitchChangedSlot(bool status)
{
    bool bBootStatus = true;
    if (m_pBootSetting->keys().contains("bootMusic")) {
        bBootStatus = m_pBootSetting->get(UKUI_BOOT_MUSIC_KEY).toBool();
        if (bBootStatus != status) {
            m_pBootSetting->set(UKUI_BOOT_MUSIC_KEY,status);
        }
    }
}

/*
    ??????????????????
*/
void UkmediaMainWidget::alertSoundButtonSwitchChangedSlot(bool status)
{
    g_settings_set_boolean (m_pSoundSettings, EVENT_SOUNDS_KEY, status);
    if (status == true) {
//        if (m_pSoundWidget->m_pSoundLayout->c)
        m_pSoundWidget->m_pAlertSoundVolumeWidget->show();
        m_pSoundWidget->m_pSoundLayout->insertWidget(5,m_pSoundWidget->m_pAlertSoundVolumeWidget);
    }
    else {
        m_pSoundWidget->m_pAlertSoundVolumeWidget->hide();
        m_pSoundWidget->m_pSoundLayout->removeWidget(m_pSoundWidget->m_pAlertSoundVolumeWidget);
    }
}

void UkmediaMainWidget::bootMusicSettingsChanged()
{
    bool bBootStatus = true;
    bool status = m_pSoundWidget->m_pBootButton->isChecked();
    if (m_pBootSetting->keys().contains("bootMusic")) {
        bBootStatus = m_pBootSetting->get(UKUI_BOOT_MUSIC_KEY).toBool();
        if (status != bBootStatus ) {
            m_pSoundWidget->m_pBootButton->setChecked(bBootStatus);
        }
    }
}


/*
    ??????????????????
*/
void UkmediaMainWidget::ukuiThemeChangedSlot(const QString &themeStr)
{
    if (m_pThemeSetting->keys().contains("styleName")) {
        mThemeName = m_pThemeSetting->get(UKUI_THEME_NAME).toString();
    }
    int nInputValue = getInputVolume();
    int nOutputValue = getOutputVolume();
    bool inputStatus = getInputMuteStatus();
    bool outputStatus = getOutputMuteStatus();
    inputVolumeDarkThemeImage(nInputValue,inputStatus);
    outputVolumeDarkThemeImage(nOutputValue,outputStatus);
    m_pOutputWidget->m_pOutputIconBtn->repaint();
    m_pSoundWidget->m_pAlertIconBtn->repaint();
    m_pInputWidget->m_pInputIconBtn->repaint();
}

/*
 * context????????????
*/
void UkmediaMainWidget::onContextStateNotify (MateMixerContext *m_pContext,GParamSpec *pspec,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(pspec);
    g_debug("on context state notify");
    MateMixerState state = mate_mixer_context_get_state (m_pContext);

    listDevice(m_pWidget,m_pContext);
    if (state == MATE_MIXER_STATE_READY) {
        updateDefaultInputStream (m_pWidget);
        updateIconOutput(m_pWidget);
        updateIconInput(m_pWidget);
    }
    else if (state == MATE_MIXER_STATE_FAILED) {
        UkuiMessageBox::critical(m_pWidget,tr("sound error"),tr("load sound failed"),UkuiMessageBox::Yes | UkuiMessageBox::No,UkuiMessageBox::Yes);
        g_debug(" mate mixer state failed");
    }
    m_pWidget->createAlertSound(m_pWidget);
    //    ??????????????????
    connect(m_pWidget->m_pOutputWidget->m_pOutputDeviceCombobox,SIGNAL(currentIndexChanged(QString)),m_pWidget,SLOT(outputDeviceComboxIndexChangedSlot(QString)));
    //    ??????????????????
    connect(m_pWidget->m_pInputWidget->m_pInputDeviceCombobox,SIGNAL(currentIndexChanged(QString)),m_pWidget,SLOT(inputDeviceComboxIndexChangedSlot(QString)));
}

/*
    context ??????control??????
*/
void UkmediaMainWidget::onContextStoredControlAdded(MateMixerContext *m_pContext,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    g_debug("on context stored control add");
    MateMixerStreamControl *m_pControl;
    MateMixerStreamControlMediaRole mediaRole;
    m_pControl = MATE_MIXER_STREAM_CONTROL (mate_mixer_context_get_stored_control (m_pContext, m_pName));
    if (G_UNLIKELY (m_pControl == nullptr))
        return;
    qDebug() << "on context stored control add" << mate_mixer_stream_control_get_name(m_pControl);
    m_pWidget->m_pMediaRoleControl = m_pControl;
    mediaRole = mate_mixer_stream_control_get_media_role (m_pControl);
    if (mediaRole == MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_EVENT)
        ukuiBarSetStreamControl (m_pWidget,MATE_MIXER_DIRECTION_UNKNOWN, m_pControl);
}

/*
    ????????????????????????????????????stream
*/
void UkmediaMainWidget::onContextStreamAdded (MateMixerContext *m_pContext,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    g_debug("on context stream added");
    MateMixerStream *m_pStream;
    m_pStream = mate_mixer_context_get_stream (m_pContext, m_pName);
    if (G_UNLIKELY (m_pStream == nullptr))
        return;
    addStream (m_pWidget, m_pStream,m_pContext);
}

/*
????????????
*/
void UkmediaMainWidget::listDevice(UkmediaMainWidget *m_pWidget,MateMixerContext *m_pContext)
{
    g_debug("list device");
    const GList *m_pList;
    m_pList = mate_mixer_context_list_streams (m_pContext);
    while (m_pList != nullptr) {
        addStream (m_pWidget, MATE_MIXER_STREAM (m_pList->data),m_pContext);
        m_pList = m_pList->next;
    }

    //???????????????????????????
    MateMixerStream *inputStream = mate_mixer_context_get_default_input_stream(m_pContext);
    MateMixerStream *outputStream = mate_mixer_context_get_default_output_stream(m_pContext);
    QString inputDeviceLabel = mate_mixer_stream_get_label(inputStream);
    QString outputDeviceLabel = mate_mixer_stream_get_label(outputStream);
    int index = m_pWidget->m_pOutputWidget->m_pOutputDeviceCombobox->findText(outputDeviceLabel);
    if (index >= 0) {
        m_pWidget->m_pOutputWidget->m_pOutputDeviceCombobox->setCurrentIndex(index);
    }

    index = m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->findText(inputDeviceLabel);
    if (index >= 0) {
        m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->setCurrentIndex(index);
    }

    const GList *pDeviceList;
    const GList *switches;
    const gchar *profileName;
    const gchar *profileLabel;
    pDeviceList = mate_mixer_context_list_devices(m_pContext);
    while (pDeviceList) {
        addDevice(m_pWidget, MATE_MIXER_DEVICE (pDeviceList->data));
        switches = mate_mixer_device_list_switches (MATE_MIXER_DEVICE(pDeviceList->data));
        while (switches != nullptr) {
            MateMixerDeviceSwitch *swtch = MATE_MIXER_DEVICE_SWITCH (switches->data);
            const GList *options;
            options = mate_mixer_switch_list_options ( MATE_MIXER_SWITCH(swtch));
            while (options != NULL) {   
                MateMixerSwitchOption *option = MATE_MIXER_SWITCH_OPTION (options->data);
                profileLabel = mate_mixer_switch_option_get_label (option);
                profileName = mate_mixer_switch_option_get_name(option);
                /* Select the currently active option of the switch */
                options = options->next;
            }
            switches = switches->next;
        }
        pDeviceList = pDeviceList->next;
    }
}

void UkmediaMainWidget::addStream (UkmediaMainWidget *m_pWidget, MateMixerStream *m_pStream,MateMixerContext *m_pContext)
{
    g_debug("add stream");
    const GList *m_pControls;
    MateMixerDirection direction;
    direction = mate_mixer_stream_get_direction (m_pStream);
    const gchar *m_pName;
    const gchar *m_pLabel;
    MateMixerStreamControl *m_pControl;

    const GList *switchList;
    MateMixerSwitch *swt;
    switchList = mate_mixer_stream_list_switches(m_pStream);
    while (switchList != nullptr) {
        swt = MATE_MIXER_SWITCH(switchList->data);
        MateMixerSwitchOption *opt = mate_mixer_switch_get_active_option(swt);
        const char *name = mate_mixer_switch_option_get_name(opt);
        const char *label = mate_mixer_switch_option_get_label(opt);
        m_pWidget->m_pDeviceStr = name;
        switchList = switchList->next;
    }
    if (direction == MATE_MIXER_DIRECTION_INPUT) {
        MateMixerStream *m_pInput;
        m_pInput = mate_mixer_context_get_default_input_stream (m_pContext);
        m_pName  = mate_mixer_stream_get_name (m_pStream);
        m_pLabel = mate_mixer_stream_get_label (m_pStream);
        if (m_pStream == m_pInput) {
            ukuiBarSetStream(m_pWidget,m_pStream);
            m_pControl = mate_mixer_stream_get_default_control(m_pStream);
            updateInputSettings (m_pWidget,m_pControl);
        }
        m_pName  = mate_mixer_stream_get_name (m_pStream);
        m_pLabel = mate_mixer_stream_get_label (m_pStream);
        QString deviceName = m_pName;
        qDebug() << "device name ***" << deviceName << m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->count();
        if (m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->count() == 0 && !m_pWidget->m_pInputStreamList->contains(m_pName)) {
            m_pWidget->m_pInputStreamList->append(m_pName);
            m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->addItem(m_pLabel);
        }
        else {
            if (!deviceName.contains("monitor",Qt::CaseInsensitive) && !m_pWidget->m_pInputStreamList->contains(m_pName)) {
                m_pWidget->m_pInputStreamList->append(m_pName);
                m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->addItem(m_pLabel);
            }
        }
    }
    else if (direction == MATE_MIXER_DIRECTION_OUTPUT) {
        MateMixerStream        *m_pOutput;
        MateMixerStreamControl *m_pControl;
        m_pOutput = mate_mixer_context_get_default_output_stream (m_pContext);
        m_pControl = mate_mixer_stream_get_default_control (m_pStream);
        m_pName  = mate_mixer_stream_get_name (m_pStream);
        m_pLabel = mate_mixer_stream_get_label (m_pStream);
        if (m_pStream == m_pOutput) {
            updateOutputSettings(m_pWidget,m_pControl);
            ukuiBarSetStream (m_pWidget, m_pStream);
        }
        m_pName  = mate_mixer_stream_get_name (m_pStream);
        m_pLabel = mate_mixer_stream_get_label (m_pStream);
        qDebug() << "??????????????????:" <<m_pName;
        if (!strstr(m_pName,".echo-cancel") && !(m_pWidget->m_pOutputStreamList->contains(m_pName))) {
            m_pWidget->m_pOutputStreamList->append(m_pName);
            m_pWidget->m_pOutputWidget->m_pOutputDeviceCombobox->addItem(m_pLabel);
        }

    }
    m_pControls = mate_mixer_stream_list_controls (m_pStream);
    while (m_pControls != nullptr) {
        MateMixerStreamControl    *m_pControl = MATE_MIXER_STREAM_CONTROL (m_pControls->data);
        MateMixerStreamControlRole role;
        role = mate_mixer_stream_control_get_role (m_pControl);
        const gchar *m_pStreamControlName = mate_mixer_stream_control_get_name(m_pControl);
        if (role == MATE_MIXER_STREAM_CONTROL_ROLE_APPLICATION) {
            MateMixerAppInfo *m_pAppInfo = mate_mixer_stream_control_get_app_info(m_pControl);
            const gchar *m_pAppName = mate_mixer_app_info_get_name(m_pAppInfo);
            if (strcmp(m_pAppName,"ukui-session") != 0) {
                m_pWidget->m_pStreamControlList->append(m_pStreamControlName);
                if G_UNLIKELY (m_pControl == nullptr)
                    return;
                m_pWidget->m_pStreamControlList->append(m_pName);
                if G_UNLIKELY (m_pControl == nullptr)
                    return;
                addApplicationControl (m_pWidget, m_pControl);
            }
        }
        m_pControls = m_pControls->next;
    }

    // XXX find a way to disconnect when removed
    g_signal_connect (G_OBJECT (m_pStream),
                      "control-added",
                      G_CALLBACK (onStreamControlAdded),
                      m_pWidget);
    g_signal_connect (G_OBJECT (m_pStream),
                      "control-removed",
                      G_CALLBACK (onStreamControlRemoved),
                      m_pWidget);
}

/*
    ????????????????????????
*/
void UkmediaMainWidget::addApplicationControl (UkmediaMainWidget *m_pWidget, MateMixerStreamControl *m_pControl)
{
    g_debug("add application control");
    MateMixerStream *m_pStream;
    MateMixerStreamControlMediaRole mediaRole;
    MateMixerAppInfo *m_pInfo;
    MateMixerDirection direction = MATE_MIXER_DIRECTION_UNKNOWN;
    const gchar *m_pAppId;
    const gchar *m_pAppName;
    const gchar *m_pAppIcon;
    appnum++;
    mediaRole = mate_mixer_stream_control_get_media_role (m_pControl);
    /* Add stream to the applications page, but make sure the stream qualifies
     * for the inclusion */
    m_pInfo = mate_mixer_stream_control_get_app_info (m_pControl);
    if (m_pInfo == nullptr)
        return;

    /* Skip streams with roles we don't care about */
    if (mediaRole == MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_EVENT ||
        mediaRole == MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_TEST ||
        mediaRole == MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_ABSTRACT ||
        mediaRole == MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_FILTER)
            return;

    m_pAppId = mate_mixer_app_info_get_id (m_pInfo);

    /* These applications may have associated streams because they do peak
     * level monitoring, skip these too */
    if (!g_strcmp0 (m_pAppId, "org.mate.VolumeControl") ||
        !g_strcmp0 (m_pAppId, "org.gnome.VolumeControl") ||
        !g_strcmp0 (m_pAppId, "org.PulseAudio.pavucontrol"))
        return;

    QString app_icon_name = mate_mixer_app_info_get_icon(m_pInfo);

    m_pAppName = mate_mixer_app_info_get_name (m_pInfo);

    if (m_pAppName == nullptr)
        m_pAppName = mate_mixer_stream_control_get_label (m_pControl);
    if (m_pAppName == nullptr)
        m_pAppName = mate_mixer_stream_control_get_name (m_pControl);
    if (G_UNLIKELY (m_pAppName == nullptr))
        return;

    /* By default channel bars use speaker icons, use microphone icons
     * instead for recording applications */
    m_pStream = mate_mixer_stream_control_get_stream (m_pControl);
    if (m_pStream != nullptr)
        direction = mate_mixer_stream_get_direction (m_pStream);

    if (direction == MATE_MIXER_DIRECTION_INPUT) {

    }
    m_pAppIcon = mate_mixer_app_info_get_icon (m_pInfo);
    if (m_pAppIcon == nullptr) {
        if (direction == MATE_MIXER_DIRECTION_INPUT)
            m_pAppIcon = "audio-input-microphone";
        else
            m_pAppIcon = "applications-multimedia";
    }
    ukuiBarSetStreamControl (m_pWidget,direction, m_pControl);
}

void UkmediaMainWidget::onStreamControlAdded (MateMixerStream *m_pStream,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    g_debug("on stream control added");
    MateMixerStreamControl    *m_pControl;
    MateMixerStreamControlRole role;

    m_pControl = mate_mixer_stream_get_control (m_pStream, m_pName);
    if G_UNLIKELY (m_pControl == nullptr)
        return;

    MateMixerAppInfo *m_pAppInfo = mate_mixer_stream_control_get_app_info(m_pControl);
    if (m_pAppInfo != nullptr) {
        const gchar *m_pAppName = mate_mixer_app_info_get_name(m_pAppInfo);
        if (strcmp(m_pAppName,"ukui-session") != 0) {
            m_pWidget->m_pStreamControlList->append(m_pName);
            if G_UNLIKELY (m_pControl == nullptr)
                    return;

            role = mate_mixer_stream_control_get_role (m_pControl);
            if (role == MATE_MIXER_STREAM_CONTROL_ROLE_APPLICATION) {
                addApplicationControl(m_pWidget, m_pControl);
            }
        }
    }
}

/*
    ??????control
*/
void UkmediaMainWidget::onStreamControlRemoved (MateMixerStream *m_pStream,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(m_pStream);
    g_debug("on stream control removed");
    if (m_pWidget->m_pStreamControlList->count() > 0 && m_pWidget->m_pAppNameList->count() > 0) {

        int i = m_pWidget->m_pStreamControlList->indexOf(m_pName);
        if (i < 0)
            return;
        m_pWidget->m_pStreamControlList->removeAt(i);
        m_pWidget->m_pAppNameList->removeAt(i);

    }
    else {
        m_pWidget->m_pStreamControlList->clear();
        m_pWidget->m_pAppNameList->clear();
    }
}

/*
    ??????context?????????????????????
*/
void UkmediaMainWidget::setContext(UkmediaMainWidget *m_pWidget,MateMixerContext *m_pContext)
{
    g_debug("set context");
    g_signal_connect (G_OBJECT (m_pContext),
                      "stream-added",
                      G_CALLBACK (onContextStreamAdded),
                      m_pWidget);

    g_signal_connect (G_OBJECT (m_pContext),
                    "stream-removed",
                    G_CALLBACK (onContextStreamRemoved),
                    m_pWidget);

    g_signal_connect (G_OBJECT (m_pContext),
                    "device-added",
                    G_CALLBACK (onContextDeviceAdded),
                    m_pWidget);
    g_signal_connect (G_OBJECT (m_pContext),
                    "device-removed",
                    G_CALLBACK (onContextDeviceRemoved),
                    m_pWidget);

    g_signal_connect (G_OBJECT (m_pContext),
                    "notify::default-input-stream",
                    G_CALLBACK (onContextDefaultInputStreamNotify),
                    m_pWidget);
    g_signal_connect (G_OBJECT (m_pContext),
                    "notify::default-output-stream",
                    G_CALLBACK (onContextDefaultOutputStreamNotify),
                    m_pWidget);


    g_signal_connect (G_OBJECT (m_pContext),
                    "stored-control-added",
                    G_CALLBACK (onContextStoredControlAdded),
                    m_pWidget);
    g_signal_connect (G_OBJECT (m_pContext),
                    "stored-control-removed",
                    G_CALLBACK (onContextStoredControlRemoved),
                    m_pWidget);

}

/*
    remove stream
*/
void UkmediaMainWidget::onContextStreamRemoved (MateMixerContext *m_pContext,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(m_pContext);
    Q_UNUSED(m_pName);
    g_debug("on context stream removed");

    removeStream (m_pWidget, m_pName);
}

/*
    ??????stream
*/
void UkmediaMainWidget::removeStream (UkmediaMainWidget *m_pWidget, const gchar *m_pName)
{
    g_debug("remove stream");
    int index;
    index = m_pWidget->m_pInputStreamList->indexOf(m_pName);
    if (index >= 0) {
        m_pWidget->m_pInputStreamList->removeAt(index);
        m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->removeItem(index);
    }
    else {
        index = m_pWidget->m_pOutputStreamList->indexOf(m_pName);
        if (index >= 0) {
            m_pWidget->m_pOutputStreamList->removeAt(index);
            m_pWidget->m_pOutputWidget->m_pOutputDeviceCombobox->removeItem(index);
        }
    }
    if (m_pWidget->m_pAppVolumeList != nullptr) {
        ukuiBarSetStream(m_pWidget,nullptr);
    }
}

/*
    context ?????????????????????????????????
*/
void UkmediaMainWidget::onContextDeviceAdded(MateMixerContext *m_pContext, const gchar *m_pName, UkmediaMainWidget *m_pWidget)
{
    g_debug("on context device added");
    MateMixerDevice *m_pDevice;
    m_pDevice = mate_mixer_context_get_device (m_pContext, m_pName);

    if (G_UNLIKELY (m_pDevice == nullptr))
        return;
    addDevice (m_pWidget, m_pDevice);

    int index = m_pWidget->m_pDeviceNameList->indexOf(m_pName);
    if (index >= 0 && index < m_pWidget->m_pOutputWidget->m_pSelectCombobox->count()) {
        m_pWidget->m_pOutputWidget->m_pSelectCombobox->setCurrentIndex(index);
    }
}

/*
    ????????????
*/
void UkmediaMainWidget::addDevice(UkmediaMainWidget *m_pWidget, MateMixerDevice *pDevice)
{
    g_debug("add device");
        const gchar *pName;
        const gchar *pLabel;
        const gchar *profileLabel = NULL;
        /*
         * const gchar *m_pLabel;
         * m_pLabel = mate_mixer_device_get_label (m_pDevice);
        */
        m_pWidget->m_pDevice = pDevice;
        pName  = mate_mixer_device_get_name (pDevice);
        pLabel = mate_mixer_device_get_label(pDevice);

        if (m_pWidget->m_pDeviceNameList->contains(pName) == false) {
            m_pWidget->m_pDeviceNameList->append(pName);
            m_pWidget->m_pOutputWidget->m_pSelectCombobox->addItem(pLabel);
        }
        MateMixerSwitch *profileSwitch;

        profileSwitch = findDeviceProfileSwitch(m_pWidget,pDevice);
        MateMixerSwitchOption *activeProfile;
        activeProfile = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH (profileSwitch));
        if (G_LIKELY (activeProfile != NULL))
            profileLabel = mate_mixer_switch_option_get_label(activeProfile);

        if (profileSwitch != NULL) {

            activeProfile = mate_mixer_switch_get_active_option(profileSwitch);
            if (G_LIKELY (activeProfile != NULL))
                profileLabel = mate_mixer_switch_option_get_label(activeProfile);
    //        qDebug() << "profilelabel :" << profileLabel;
            g_signal_connect (G_OBJECT (profileSwitch),
                              "notify::active-option",
                              G_CALLBACK (onDeviceProfileActiveOptionNotify),
                              m_pWidget);
        }

    MateMixerSwitch *portSwitch;

    MateMixerStream *stream = mate_mixer_context_get_default_output_stream(m_pWidget->m_pContext);
    portSwitch = findStreamPortSwitch(m_pWidget,stream);
    const GList  *options ;
    options = mate_mixer_switch_list_options(portSwitch);
//    MateMixerSwitchOption *option = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH(portSwitch));

    MateMixerSwitchOption *activePort;
    activePort = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH (portSwitch));
    if (G_LIKELY (activePort != NULL))
        profileLabel = mate_mixer_switch_option_get_label(activePort);
//    m_pWidget->m_pSwitch = profileSwitch;
    if (portSwitch != NULL) {
        if (G_LIKELY (activePort != NULL))
            profileLabel = mate_mixer_switch_option_get_label(activePort);
//        qDebug() << "profilelabel :" << profileLabel;
        g_signal_connect (G_OBJECT (portSwitch),
                          "notify::active-option",
                          G_CALLBACK(onSwitchActiveOptionNotify),
                          m_pWidget);
    }

}

/*
    ????????????
*/
void UkmediaMainWidget::onContextDeviceRemoved (MateMixerContext *m_pContext,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(m_pContext);
    g_debug("on context device removed");
    int index = m_pWidget->m_pDeviceNameList->indexOf(m_pName);
    MateMixerDevice *device = mate_mixer_context_get_device(m_pContext,m_pName);
    const gchar *label = mate_mixer_device_get_label(device);
    if (index >= 0) {
        m_pWidget->m_pDeviceNameList->removeAt(index);
        m_pWidget->m_pOutputWidget->m_pSelectCombobox->removeItem(index);
    }
}

/*
    ?????????????????????
*/
void UkmediaMainWidget::onContextDefaultInputStreamNotify (MateMixerContext *m_pContext,GParamSpec *pspec,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(pspec);
    g_debug ("on context default input stream notify");
    MateMixerStream *m_pStream;
    m_pStream = mate_mixer_context_get_default_input_stream (m_pContext);
    if (m_pStream == nullptr) {
        //????????????????????????????????????????????????????????????????????????????????????
        m_pStream = m_pWidget->m_pInputStream;
    }
    QString deviceName = mate_mixer_stream_get_label(m_pStream);
    int index = m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->findText(deviceName);
    if (index < 0)
        return;
    m_pWidget->m_pInputWidget->m_pInputDeviceCombobox->setCurrentIndex(index);
    qDebug() << "on context default input stream notify" <<deviceName;
    updateIconInput(m_pWidget);

    setInputStream(m_pWidget, m_pStream);
}

void UkmediaMainWidget::setInputStream(UkmediaMainWidget *m_pWidget, MateMixerStream *m_pStream)
{
    g_debug("set input stream");
    if (m_pStream == nullptr) {
        return;
    }

    MateMixerStreamControl *m_pControl = mate_mixer_stream_get_default_control(m_pStream);
    if (m_pControl != nullptr) {
//        mate_mixer_stream_control_set_monitor_enabled (m_pControl, false);
    }
    ukuiBarSetStream (m_pWidget, m_pStream);

    if (m_pStream != nullptr) {
        const GList *m_pControls;
        m_pControls = mate_mixer_context_list_stored_controls (m_pWidget->m_pContext);

        /* Move all stored controls to the newly selected default stream */
        while (m_pControls != nullptr) {
            MateMixerStream *parent;

            m_pControl = MATE_MIXER_STREAM_CONTROL (m_pControls->data);
            parent = mate_mixer_stream_control_get_stream (m_pControl);

            /* Prefer streamless controls to stay the way they are, forcing them to
             * a particular owning stream would be wrong for eg. event controls */
            if (parent != nullptr && parent != m_pStream) {
                MateMixerDirection direction = mate_mixer_stream_get_direction (parent);
                if (direction == MATE_MIXER_DIRECTION_INPUT)
                    mate_mixer_stream_control_set_stream (m_pControl, m_pStream);
            }
            m_pControls = m_pControls->next;
        }

        /* Enable/disable the peak level monitor according to mute state */
        g_signal_connect (G_OBJECT (m_pStream),
                          "notify::mute",
                          G_CALLBACK (onStreamControlMuteNotify),
                          m_pWidget);
    }
    m_pControl = mate_mixer_stream_get_default_control(m_pStream);
    if (G_LIKELY (m_pControl != nullptr)) {
        if (m_pWidget->m_pDeviceStr == UKUI_INPUT_REAR_MIC || m_pWidget->m_pDeviceStr == UKUI_INPUT_FRONT_MIC || m_pWidget->m_pDeviceStr == UKUI_OUTPUT_HEADPH) {
            mate_mixer_stream_control_set_monitor_enabled(m_pControl,true);
        }
    }

//    m_pControl = mate_mixer_stream_get_default_control(m_pStream);
    updateInputSettings (m_pWidget,m_pWidget->m_pInputBarStreamControl);
}

/*
    control ????????????
*/
void UkmediaMainWidget::onStreamControlMuteNotify (MateMixerStreamControl *m_pControl,GParamSpec *pspec,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(m_pWidget);
    Q_UNUSED(pspec);
    g_debug("on stream control mute notifty");
    /* Stop monitoring the input stream when it gets muted */
    if (mate_mixer_stream_control_get_mute (m_pControl) == TRUE) {
//        mate_mixer_stream_control_set_monitor_enabled (m_pControl, false);
    }
    else {
        if (m_pWidget->m_pDeviceStr == UKUI_INPUT_REAR_MIC || m_pWidget->m_pDeviceStr == UKUI_INPUT_FRONT_MIC || m_pWidget->m_pDeviceStr == UKUI_OUTPUT_HEADPH) {
            mate_mixer_stream_control_set_monitor_enabled(m_pControl,true);
        }
    }
}

/*
    ?????????????????????
*/
void UkmediaMainWidget::onContextDefaultOutputStreamNotify (MateMixerContext *m_pContext,GParamSpec *pspec,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(pspec);
    g_debug("on context default output stream notify");
    MateMixerStream *m_pStream;
    const gchar *portLabel;
    const gchar *portName;
    m_pStream = mate_mixer_context_get_default_output_stream (m_pContext);

    //?????????????????????????????????????????????
    MateMixerDevice *pDevice = mate_mixer_stream_get_device(m_pStream);
    const gchar *cardName = mate_mixer_device_get_name(pDevice);
    int cardIndex = m_pWidget->m_pDeviceNameList->indexOf(cardName);
//    qDebug() << "index " << cardIndex;
//    if (cardIndex < 0)
//        return;
//    m_pWidget->m_pOutputWidget->m_pSelectCombobox->blockSignals(true);
//    m_pWidget->m_pOutputWidget->m_pSelectCombobox->setCurrentIndex(cardIndex);
//    m_pWidget->m_pOutputWidget->m_pSelectCombobox->blockSignals(false);

    qDebug() << "on context default output steam notify:" << mate_mixer_stream_get_name(m_pStream) << cardName;

    /* Enable the port selector if the stream has one */
    MateMixerSwitch *portSwitch;
    portSwitch = findStreamPortSwitch (m_pWidget,m_pStream);
    //????????????????????????????????????
    m_pWidget->m_pOutputPortList->clear();
    m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->clear();
    MateMixerDirection direction = mate_mixer_stream_get_direction(MATE_MIXER_STREAM(m_pStream));
    if (MATE_MIXER_IS_STREAM(m_pStream)) {
        if (direction == MATE_MIXER_DIRECTION_OUTPUT) {
            if (portSwitch != nullptr) {
                const GList *options;
                options = mate_mixer_switch_list_options(MATE_MIXER_SWITCH(portSwitch));
                MateMixerSwitchOption *activePort = mate_mixer_switch_get_active_option(portSwitch);
                while (options != nullptr) {
                    portName = mate_mixer_switch_option_get_name(activePort);
                    portLabel = mate_mixer_switch_option_get_label(activePort);
                    MateMixerSwitchOption *opt = MATE_MIXER_SWITCH_OPTION(options->data);
                    QString label = mate_mixer_switch_option_get_label(opt);
                    QString name = mate_mixer_switch_option_get_name(opt);
                    qDebug() << "***********" << name << portName;
                    if (!m_pWidget->m_pOutputPortList->contains(name)) {
                        m_pWidget->m_pOutputPortList->append(name);
                        m_pWidget->m_pOutputWidget->outputWidgetAddPort();
                        m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->addItem(label);
                    }
                    options = options->next;
                }
            }
        }
    }
    int portIndex = m_pWidget->m_pOutputPortList->indexOf(portName);
    if (portIndex < 0) {
//        m_pWidget->m_pOutputWidget->m_pOutputPortWidget->hide();
        m_pWidget->m_pOutputWidget->outputWidgetRemovePort();
        return;
    }
    m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->setCurrentIndex(portIndex);
    if (m_pStream == nullptr) {
        //????????????????????????????????????????????????????????????????????????????????????
        m_pStream = m_pWidget->m_pOutputStream;
    }
    QString deviceName = mate_mixer_stream_get_label(m_pStream);
    int index = m_pWidget->m_pOutputWidget->m_pOutputDeviceCombobox->findText(deviceName);
    if (index < 0)
        return;
    m_pWidget->m_pOutputWidget->m_pOutputDeviceCombobox->setCurrentIndex(index);
    updateIconOutput(m_pWidget);
    setOutputStream (m_pWidget, m_pStream);
}

/*
    ????????????control
*/
void UkmediaMainWidget::onContextStoredControlRemoved (MateMixerContext *m_pContext,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(m_pContext);
    Q_UNUSED(m_pName);
    g_debug("on context stored control removed");
    if (m_pWidget->m_pAppVolumeList != nullptr) {
        ukuiBarSetStream (m_pWidget, nullptr);
    }
}

/*
 * context????????????
*/
void UkmediaMainWidget::contextSetProperty(UkmediaMainWidget *m_pWidget)//,guint prop_id,const GValue *value,GParamSpec *pspec)
{
    g_debug("context set property");
    setContext(m_pWidget,m_pWidget->m_pContext);
}

/*
    ?????????????????????
*/
int UkmediaMainWidget::getInputVolume()
{
    return m_pInputWidget->m_pIpVolumeSlider->value();
}

/*
    ?????????????????????
*/
int UkmediaMainWidget::getOutputVolume()
{
    return m_pOutputWidget->m_pOpVolumeSlider->value();
}

/*
   ??????????????????
*/
bool UkmediaMainWidget::getInputMuteStatus()
{
    MateMixerStream *pStream = mate_mixer_context_get_default_input_stream(m_pContext);
    MateMixerStreamControl *pControl = mate_mixer_stream_get_default_control(pStream);
    return mate_mixer_stream_control_get_mute(pControl);
}

/*
    ??????????????????
*/
bool UkmediaMainWidget::getOutputMuteStatus()
{
    MateMixerStream *pStream = mate_mixer_context_get_default_output_stream(m_pContext);
    MateMixerStreamControl *pControl = mate_mixer_stream_get_default_control(pStream);
    return mate_mixer_stream_control_get_mute(pControl);
}

/*
    ?????????????????????????????????
*/
void UkmediaMainWidget::outputVolumeDarkThemeImage(int value,bool status)
{
    QImage image;
    QColor color = QColor(0,0,0,216);
    if (mThemeName == UKUI_THEME_WHITE) {
        color = QColor(0,0,0,216);
    }
    else if (mThemeName == UKUI_THEME_BLACK) {
        color = QColor(255,255,255,216);
    }
    m_pOutputWidget->m_pOutputIconBtn->mColor = color;
    if (status) {
        image  = QImage("/usr/share/ukui-media/img/audio-volume-muted.svg");
        m_pOutputWidget->m_pOutputIconBtn->mImage = image;
    }
    else if (value <= 0) {
        image  = QImage("/usr/share/ukui-media/img/audio-volume-muted.svg");
        m_pOutputWidget->m_pOutputIconBtn->mImage = image;
    }
    else if (value > 0 && value <= 33) {
        image = QImage("/usr/share/ukui-media/img/audio-volume-low.svg");
        m_pOutputWidget->m_pOutputIconBtn->mImage = image;
    }
    else if (value >33 && value <= 66) {
        image = QImage("/usr/share/ukui-media/img/audio-volume-medium.svg");
        m_pOutputWidget->m_pOutputIconBtn->mImage = image;
    }
    else {
        image = QImage("/usr/share/ukui-media/img/audio-volume-high.svg");
        m_pOutputWidget->m_pOutputIconBtn->mImage = image;
    }

}

/*
    ??????????????????
*/
void UkmediaMainWidget::inputVolumeDarkThemeImage(int value,bool status)
{
    QImage image;
    QColor color = QColor(0,0,0,190);
    if (mThemeName == UKUI_THEME_WHITE) {
        color = QColor(0,0,0,190);
    }
    else if (mThemeName == UKUI_THEME_BLACK) {
        color = QColor(255,255,255,190);
    }
    m_pInputWidget->m_pInputIconBtn->mColor = color;
    if (status) {
        image  = QImage("/usr/share/ukui-media/img/microphone-mute.svg");
        m_pInputWidget->m_pInputIconBtn->mImage = image;
    }
    else if (value <= 0) {
        image  = QImage("/usr/share/ukui-media/img/microphone-mute.svg");
        m_pInputWidget->m_pInputIconBtn->mImage = image;
    }
    else if (value > 0 && value <= 33) {
        image = QImage("/usr/share/ukui-media/img/microphone-low.svg");
        m_pInputWidget->m_pInputIconBtn->mImage = image;
    }
    else if (value >33 && value <= 66) {
        image = QImage("/usr/share/ukui-media/img/microphone-medium.svg");
        m_pInputWidget->m_pInputIconBtn->mImage = image;
    }
    else {
        image = QImage("/usr/share/ukui-media/img/microphone-high.svg");
        m_pInputWidget->m_pInputIconBtn->mImage = image;
    }
}

/*
    ???????????????????????????
*/
void UkmediaMainWidget::updateIconInput (UkmediaMainWidget *m_pWidget)
{
    g_debug("update icon input");
    MateMixerStream *m_pStream;
    MateMixerStreamControl *m_pControl = nullptr;
    MateMixerStreamControlFlags flags;
    const gchar *m_pAppId;
    gboolean show = FALSE;
    m_pStream = mate_mixer_context_get_default_input_stream (m_pWidget->m_pContext);
    const GList *m_pInputs =mate_mixer_stream_list_controls(m_pStream);
    m_pControl = mate_mixer_stream_get_default_control(m_pStream);

    m_pWidget->m_pStream = m_pStream;
    //????????????????????????
    int volume = mate_mixer_stream_control_get_volume(m_pControl);
    bool status = mate_mixer_stream_control_get_mute(m_pControl);
    int value = volume *100 /65536.0+0.5;
    m_pWidget->m_pInputWidget->m_pIpVolumeSlider->setValue(value);
    QString percent = QString::number(value);
    percent.append("%");
    m_pWidget->m_pInputWidget->m_pIpVolumePercentLabel->setText(percent);
    m_pWidget->m_pInputWidget->m_pInputIconBtn->setFocusPolicy(Qt::NoFocus);
//    m_pWidget->m_pInputWidget->m_pInputIconBtn->setStyleSheet("QPushButton{background:transparent;border:0px;padding-left:0px;}");

    const QSize icon_size = QSize(24,24);
    m_pWidget->m_pInputWidget->m_pInputIconBtn->setIconSize(icon_size);
    //?????????????????????????????????
    m_pWidget->inputVolumeDarkThemeImage(value,status);
    m_pWidget->m_pInputWidget->m_pInputIconBtn->repaint();
    while (m_pInputs != nullptr) {
        MateMixerStreamControl *input = MATE_MIXER_STREAM_CONTROL (m_pInputs->data);
        MateMixerStreamControlRole role = mate_mixer_stream_control_get_role (input);
        if (role == MATE_MIXER_STREAM_CONTROL_ROLE_APPLICATION) {
            MateMixerAppInfo *app_info = mate_mixer_stream_control_get_app_info (input);
            m_pAppId = mate_mixer_app_info_get_id (app_info);
            if (m_pAppId == nullptr) {
                /* A recording application which has no
                 * identifier set */
                g_debug ("Found a recording application control %s",
                         mate_mixer_stream_control_get_label (input));

                if G_UNLIKELY (m_pControl == nullptr) {
                    /* In the unlikely case when there is no
                     * default input control, use the application
                     * control for the icon */
                    m_pControl = input;
                }
                show = TRUE;
                break;
            }
            if (strcmp (m_pAppId, "org.mate.VolumeControl") != 0 &&
                    strcmp (m_pAppId, "org.gnome.VolumeControl") != 0 &&
                    strcmp (m_pAppId, "org.PulseAudio.pavucontrol") != 0) {
                g_debug ("Found a recording application %s", m_pAppId);

                if G_UNLIKELY (m_pControl == nullptr)
                    m_pControl = input;

                show = TRUE;
                break;
            }
        }
        m_pInputs = m_pInputs->next;
    }
    //????????????????????????????????????????????????
    if (show == TRUE) {
        mate_mixer_stream_control_set_monitor_enabled(m_pControl,true);
        g_debug ("Input icon enabled");
    }
    else {
        mate_mixer_stream_control_set_monitor_enabled(m_pControl,false);
        g_debug ("There is no recording application, input icon disabled");
    }
    streamStatusIconSetControl(m_pWidget, m_pControl);

    if (m_pControl != nullptr) {
        g_debug ("Output icon enabled");
    }
    else {
        g_debug ("There is no output stream/control, output icon disabled");
    }

//    if (show == TRUE ) {
//        flags = mate_mixer_stream_control_get_flags(m_pControl);
//        if (m_pWidget->m_pDeviceStr == UKUI_INPUT_REAR_MIC || m_pWidget->m_pDeviceStr == UKUI_INPUT_FRONT_MIC || m_pWidget->m_pDeviceStr == UKUI_OUTPUT_HEADPH) {
//            mate_mixer_stream_control_set_monitor_enabled(m_pControl,true);
//        }
        /* Enable level bar only if supported by the control */
        if (flags & MATE_MIXER_STREAM_CONTROL_HAS_MONITOR) {
        }
//    }
//    else if(show == FALSE) {
//        mate_mixer_stream_control_set_monitor_enabled(m_pControl,false);
//    }
}

/*
    ???????????????????????????
*/
void UkmediaMainWidget::updateIconOutput(UkmediaMainWidget *m_pWidget)
{
    g_debug("update icon output");
    MateMixerStream *m_Stream;
    MateMixerStreamControl *m_pControl = nullptr;

    m_Stream = mate_mixer_context_get_default_output_stream (m_pWidget->m_pContext);
    if (m_Stream != nullptr)
        m_pControl = mate_mixer_stream_get_default_control (m_Stream);

    streamStatusIconSetControl(m_pWidget, m_pControl);
    //????????????????????????
    int volume = mate_mixer_stream_control_get_volume(m_pControl);
    bool status = mate_mixer_stream_control_get_mute(m_pControl);
    int value = volume *100 /65536.0+0.5;

    m_pWidget->m_pOutputWidget->m_pOpVolumeSlider->setValue(value);
    QString percent = QString::number(value);
    percent.append("%");
    m_pWidget->m_pOutputWidget->m_pOpVolumePercentLabel->setText(percent);
    m_pWidget->m_pOutputWidget->m_pOutputIconBtn->setFocusPolicy(Qt::NoFocus);
//    m_pWidget->m_pOutputWidget->m_pOutputIconBtn->setStyleSheet("QPushButton{background:transparent;border:0px;padding-left:0px;}");

    const QSize icon_size = QSize(24,24);
    m_pWidget->m_pOutputWidget->m_pOutputIconBtn->setIconSize(icon_size);
    m_pWidget->outputVolumeDarkThemeImage(value,status);
    m_pWidget->m_pOutputWidget->m_pOutputIconBtn->repaint();

    gdouble balance_value = mate_mixer_stream_control_get_balance(m_pControl);
    m_pWidget->m_pOutputWidget->m_pOpBalanceSlider->setValue(balance_value*100);
//    //??????????????????
//    //??????????????????????????????
//    connect(m_pWidget->m_pOutputWidget->m_pOpVolumeSlider,&QSlider::valueChanged,[=](int value){
//        QString percent;

//        percent = QString::number(value);
//        int volume = value*65536/100;
//        mate_mixer_stream_control_set_volume(m_pControl,guint(volume));
//        if (value <= 0) {
//            mate_mixer_stream_control_set_mute(m_pControl,TRUE);
//            mate_mixer_stream_control_set_volume(m_pControl,0);
//            percent = QString::number(0);
//        }
//        bool status = mate_mixer_stream_control_get_mute(m_pControl);
//        m_pWidget->outputVolumeDarkThemeImage(value,status);
//        m_pWidget->m_pOutputWidget->m_pOutputIconBtn->repaint();
//        mate_mixer_stream_control_set_mute(m_pControl,FALSE);
//        percent.append("%");
//        m_pWidget->m_pOutputWidget->m_pOpVolumePercentLabel->setText(percent);
//    });

    if (m_pControl != nullptr) {
        g_debug ("Output icon enabled");
    }
    else {
        g_debug ("There is no output stream/control, output icon disabled");
    }
}

void UkmediaMainWidget::streamStatusIconSetControl(UkmediaMainWidget *m_pWidget,MateMixerStreamControl *m_pControl)
{
    g_debug("stream status icon set control");
    qDebug() << "stream status icon set control" << mate_mixer_stream_control_get_label(m_pControl);
    g_signal_connect ( G_OBJECT (m_pControl),
                      "notify::volume",
                      G_CALLBACK (onStreamControlVolumeNotify),
                      m_pWidget);
    g_signal_connect (G_OBJECT (m_pControl),
                      "notify::mute",
                      G_CALLBACK (onStreamControlMuteNotify),
                      m_pWidget);

    MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags(m_pControl);
    if (flags & MATE_MIXER_STREAM_CONTROL_MUTE_READABLE) {
        g_signal_connect (G_OBJECT (m_pControl),
                          "notify::mute",
                          G_CALLBACK (onControlMuteNotify),
                          m_pWidget);
    }
}

/*
    ????????????
*/
void UkmediaMainWidget::onControlMuteNotify (MateMixerStreamControl *m_pControl,GParamSpec *pspec,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(pspec);
    g_debug("on control mute notify");
    gboolean mute = mate_mixer_stream_control_get_mute (m_pControl);
    int volume = int(mate_mixer_stream_control_get_volume(m_pControl));
    volume = int(volume*100/65536.0+0.5);
    MateMixerStream *stream = mate_mixer_stream_control_get_stream(m_pControl);
    MateMixerDirection direction = mate_mixer_stream_get_direction(stream);

    if (direction == MATE_MIXER_DIRECTION_OUTPUT) {
        m_pWidget->outputVolumeDarkThemeImage(volume,mute);
        m_pWidget->m_pOutputWidget->m_pOutputIconBtn->repaint();
    }
    else if (direction == MATE_MIXER_DIRECTION_INPUT) {
        m_pWidget->inputVolumeDarkThemeImage(volume,mute);
        m_pWidget->m_pInputWidget->m_pInputIconBtn->repaint();
    }

}

/*
    stream control ????????????
*/
void UkmediaMainWidget::onStreamControlVolumeNotify (MateMixerStreamControl *m_pControl,GParamSpec *pspec,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(pspec);
    g_debug("on stream control volume notify");
    qDebug() << "volume notify" << mate_mixer_stream_control_get_name(m_pControl);
    MateMixerStreamControlFlags flags;
    guint volume = 0;
    QString decscription;

    if (m_pControl != nullptr)
        flags = mate_mixer_stream_control_get_flags(m_pControl);

    if (flags&MATE_MIXER_STREAM_CONTROL_VOLUME_READABLE) {
        volume = mate_mixer_stream_control_get_volume(m_pControl);
    }

    decscription = mate_mixer_stream_control_get_label(m_pControl);
    MateMixerDirection direction;
    MateMixerStream *m_pStream = mate_mixer_stream_control_get_stream(m_pControl);

    MateMixerSwitch *portSwitch;
    MateMixerStream *stream = mate_mixer_stream_control_get_stream(m_pControl);
    /* Enable the port selector if the stream has one */
     portSwitch = findStreamPortSwitch (m_pWidget,stream);
     direction = mate_mixer_stream_get_direction(MATE_MIXER_STREAM(m_pStream));

    if (MATE_MIXER_IS_STREAM(m_pStream)) {
        if (direction == MATE_MIXER_DIRECTION_OUTPUT) {

            if (portSwitch != nullptr) {
                const GList *options;
                options = mate_mixer_switch_list_options(MATE_MIXER_SWITCH(portSwitch));
                if (options != nullptr) {
                    //????????????????????????????????????
                    m_pWidget->m_pOutputPortList->clear();
                    m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->clear();
                }
                MateMixerSwitchOption *option = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH(portSwitch));
                const gchar *outputPortLabel = mate_mixer_switch_option_get_label(option);
                while (options != nullptr) {
                    MateMixerSwitchOption *opt = MATE_MIXER_SWITCH_OPTION(options->data);
                    QString label = mate_mixer_switch_option_get_label(opt);
                    QString name = mate_mixer_switch_option_get_name(opt);
                    if (!m_pWidget->m_pOutputPortList->contains(name)) {
                        m_pWidget->m_pOutputPortList->append(name);
                        m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->addItem(label);
                    }
                    options = options->next;
                }
//                m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->blockSignals(true);
                m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->setCurrentText(outputPortLabel);
//                m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->blockSignals(false);
            }
        }
    }
    else {
        m_pStream = m_pWidget->m_pStream;
        if (direction == MATE_MIXER_DIRECTION_OUTPUT) {
//            portSwitch = findStreamPortSwitch (m_pWidget,m_pStream);
//            mate_mixer_context_set_default_output_stream(m_pWidget->m_pContext,m_pStream);
//            if (portSwitch!= nullptr) {
//                MateMixerSwitchOption *option = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH(portSwitch));
//                QString label = mate_mixer_switch_option_get_label(option);
//                m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->setCurrentText(label);
//                qDebug() << "get stream correct" << mate_mixer_stream_control_get_label(m_pControl) << mate_mixer_stream_get_label(m_pStream) <<label;

//            }
            setOutputStream(m_pWidget,m_pStream);
//            qDebug() << "???control ?????????stream??????output stream" << mate_mixer_stream_get_label(m_pStream);
        }
        else if (direction == MATE_MIXER_DIRECTION_INPUT) {
//            mate_mixer_context_set_default_input_stream(m_pWidget->m_pContext,m_pStream);
            qDebug() << "???control ?????????stream??????input stream" << mate_mixer_stream_get_label(m_pStream);
            setInputStream(m_pWidget,m_pStream);
        }
    }

    direction = mate_mixer_stream_get_direction(m_pStream);
    //???????????????????????????
    int value = volume*100/65536.0 + 0.5;
    if (direction == MATE_MIXER_DIRECTION_OUTPUT) {
//        m_pWidget->m_pOutputWidget->m_pOpVolumeSlider->blockSignals(true);
        m_pWidget->m_pOutputWidget->m_pOpVolumeSlider->setValue(value);
//        m_pWidget->m_pOutputWidget->m_pOpVolumeSlider->blockSignals(true);
    }
    else if (direction == MATE_MIXER_DIRECTION_INPUT) {
//        m_pWidget->m_pInputWidget->m_pIpVolumeSlider->blockSignals(true);
        m_pWidget->m_pInputWidget->m_pIpVolumeSlider->setValue(value);
//        m_pWidget->m_pInputWidget->m_pIpVolumeSlider->blockSignals(true);
    }
}

/*
    ??????????????????
*/
void UkmediaMainWidget::ukuiBalanceBarSetProperty (UkmediaMainWidget *m_pWidget,MateMixerStreamControl *m_pControl)
{
    g_debug("ukui balance bar set property");
    ukuiBalanceBarSetControl(m_pWidget,m_pControl);
}

/*
    ????????????control
*/
void UkmediaMainWidget::ukuiBalanceBarSetControl (UkmediaMainWidget *m_pWidget, MateMixerStreamControl *m_pControl)
{
    g_debug("ukui balance bar set control");
    g_signal_connect (G_OBJECT (m_pControl),
                      "notify::balance",
                      G_CALLBACK (onBalanceValueChanged),
                      m_pWidget);
}

/*
    ???????????????
*/
void UkmediaMainWidget::onBalanceValueChanged (MateMixerStreamControl *m_pControl,GParamSpec *pspec,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(pspec);
    g_debug("on balance value changed");
    gdouble value = mate_mixer_stream_control_get_balance(m_pControl);
    m_pWidget->m_pOutputWidget->m_pOpBalanceSlider->setValue(value*100);
}

/*
    ??????????????????
*/
void UkmediaMainWidget::updateOutputSettings (UkmediaMainWidget *m_pWidget,MateMixerStreamControl *m_pControl)
{
    qDebug() << "update output settings";
    g_debug("update output settings");
    QString outputPortLabel;
    MateMixerStreamControlFlags flags;
    if (m_pControl == nullptr) {
        return;
    }
    if(m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->count() != 0 || m_pWidget->m_pOutputPortList->count() != 0) {
        qDebug() << "?????????????????????:" << m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->count() ;
        m_pWidget->m_pOutputPortList->clear();
        m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->clear();
        m_pWidget->m_pOutputWidget->outputWidgetRemovePort();
    }

    MateMixerSwitch *portSwitch;
    flags = mate_mixer_stream_control_get_flags(m_pControl);

    if (flags & MATE_MIXER_STREAM_CONTROL_CAN_BALANCE) {
        ukuiBalanceBarSetProperty(m_pWidget,m_pControl);
    }
    MateMixerStream *stream = mate_mixer_stream_control_get_stream(m_pControl);
    /* Enable the port selector if the stream has one */
    portSwitch = findStreamPortSwitch (m_pWidget,stream);
    MateMixerDirection direction = mate_mixer_stream_get_direction(MATE_MIXER_STREAM(stream));
    if (direction == MATE_MIXER_DIRECTION_OUTPUT) {
        if (portSwitch != nullptr) {
            const GList *options;
            options = mate_mixer_switch_list_options(MATE_MIXER_SWITCH(portSwitch));
            MateMixerSwitchOption *option = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH(portSwitch));
            outputPortLabel = mate_mixer_switch_option_get_label(option);
            while (options != nullptr) {
                MateMixerSwitchOption *opt = MATE_MIXER_SWITCH_OPTION(options->data);
                QString label = mate_mixer_switch_option_get_label(opt);
                QString name = mate_mixer_switch_option_get_name(opt);
                qDebug() << "opt label******: "<< label << "opt name :" << mate_mixer_switch_option_get_name(opt);
                if (!m_pWidget->m_pOutputPortList->contains(name)) {

                    m_pWidget->m_pOutputPortList->append(name);
                    m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->addItem(label);
                }
                options = options->next;
            }
        }
    }

    qDebug() << "?????????????????????:" <<outputPortLabel << m_pWidget->m_pOutputPortList->count();
    if (m_pWidget->m_pOutputPortList->count() > 0) {
        m_pWidget->m_pOutputWidget->outputWidgetAddPort();
//        m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->blockSignals(true);
        m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->setCurrentText(outputPortLabel);
//        m_pWidget->m_pOutputWidget->m_pOutputPortCombobox->blockSignals(false);
    }
    connect(m_pWidget->m_pOutputWidget->m_pOutputPortCombobox,SIGNAL(currentIndexChanged(int)),m_pWidget,SLOT(outputPortComboxChangedSlot(int)));
    connect(m_pWidget->m_pOutputWidget->m_pOpBalanceSlider,&QSlider::valueChanged,[=](int volume){
        gdouble value = volume/100.0;
        mate_mixer_stream_control_set_balance(m_pControl,value);
    });
//    m_pWidget->updateProfileOption();
}

void UkmediaMainWidget::onKeyChanged (GSettings *settings,gchar *key,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(settings);
    g_debug("on key changed");
    if (!strcmp (key, EVENT_SOUNDS_KEY) ||
        !strcmp (key, SOUND_THEME_KEY) ||
        !strcmp (key, INPUT_SOUNDS_KEY)) {
        updateTheme (m_pWidget);
    }
}

/*
    ????????????
*/
void UkmediaMainWidget::updateTheme (UkmediaMainWidget *m_pWidget)
{
    g_debug("update theme");
    char *pThemeName;
    gboolean feedBackEnabled;
    gboolean eventsEnabled;
    feedBackEnabled = g_settings_get_boolean(m_pWidget->m_pSoundSettings, INPUT_SOUNDS_KEY);
    eventsEnabled = g_settings_get_boolean(m_pWidget->m_pSoundSettings,EVENT_SOUNDS_KEY);
//    eventsEnabled = FALSE;
    if (eventsEnabled) {
        pThemeName = g_settings_get_string (m_pWidget->m_pSoundSettings, SOUND_THEME_KEY);
    } else {
        pThemeName = g_strdup (NO_SOUNDS_THEME_NAME);
    }
    qDebug() << "update theme????????????" << pThemeName << eventsEnabled;
    //??????combox?????????
    setComboxForThemeName (m_pWidget, pThemeName);
    updateAlertsFromThemeName (m_pWidget, pThemeName);
}

/*
    ??????????????????combox
*/
void UkmediaMainWidget::setupThemeSelector (UkmediaMainWidget *m_pWidget)
{
    g_debug("setup theme selector");
    GHashTable  *hash;
    const char * const *dataDirs;
    const char *m_pDataDir;
    char *dir;
    guint i;

    /* Add the theme names and their display name to a hash table,
     * makes it easy to avoid duplicate themes */
    hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    dataDirs = g_get_system_data_dirs ();
    for (i = 0; dataDirs[i] != nullptr; i++) {
        dir = g_build_filename (dataDirs[i], "sounds", nullptr);
        soundThemeInDir (m_pWidget,hash, dir);
    }

    m_pDataDir = g_get_user_data_dir ();
    dir = g_build_filename (m_pDataDir, "sounds", nullptr);
    soundThemeInDir (m_pWidget,hash, dir);

    /* If there isn't at least one theme, make everything
     * insensitive, LAME! */
    if (g_hash_table_size (hash) == 0) {
        g_warning ("Bad setup, install the freedesktop sound theme");
        g_hash_table_destroy (hash);
        return;
    }
    /* Add the themes to a combobox */
    g_hash_table_destroy (hash);
}

/*
    ?????????????????????
*/
void UkmediaMainWidget::soundThemeInDir (UkmediaMainWidget *m_pWidget,GHashTable *hash,const char *dir)
{
    Q_UNUSED(hash);
    g_debug("sound theme in dir");
    GDir *d;
    const char *m_pName;
    d = g_dir_open (dir, 0, nullptr);
    if (d == nullptr) {
        return;
    }
    while ((m_pName = g_dir_read_name (d)) != nullptr) {
        char *m_pDirName, *m_pIndex, *m_pIndexName;
        /* Look for directories */
        m_pDirName = g_build_filename (dir, m_pName, nullptr);
        if (g_file_test (m_pDirName, G_FILE_TEST_IS_DIR) == FALSE) {
            continue;
        }

        /* Look for index files */
        m_pIndex = g_build_filename (m_pDirName, "index.theme", nullptr);

        /* Check the name of the theme in the index.theme file */
        m_pIndexName = loadIndexThemeName (m_pIndex, nullptr);
        if (m_pIndexName == nullptr) {
            continue;
        }
        //???????????????combox???
//        if(m_pName != NO_SOUNDS_THEME_NAME) {
            qDebug() << "sound theme in dir" << "displayname:" << m_pIndexName << "theme name:" << m_pName;
            m_pWidget->m_pThemeDisplayNameList->append(m_pIndexName);
            m_pWidget->m_pThemeNameList->append(m_pName);
            m_pWidget->m_pSoundWidget->m_pSoundThemeCombobox->addItem(m_pIndexName);
//        }
    }
    g_dir_close (d);
}

/*
    ????????????????????????
*/
char *UkmediaMainWidget::loadIndexThemeName (const char *index,char **parent)
{
    g_debug("load index theme name");
    GKeyFile *file;
    char *indexname = nullptr;
    gboolean hidden;

    file = g_key_file_new ();
    if (g_key_file_load_from_file (file, index, G_KEY_FILE_KEEP_TRANSLATIONS, nullptr) == FALSE) {
        g_key_file_free (file);
        return nullptr;
    }
    /* Don't add hidden themes to the list */
    hidden = g_key_file_get_boolean (file, "Sound Theme", "Hidden", nullptr);
    if (!hidden) {
        indexname = g_key_file_get_locale_string (file,"Sound Theme","Name",nullptr,nullptr);
        /* Save the parent theme, if there's one */
        if (parent != nullptr) {
            *parent = g_key_file_get_string (file,"Sound Theme","Inherits",nullptr);
        }
    }
    g_key_file_free (file);
    return indexname;
}

/*
    ??????combox????????????
*/
void UkmediaMainWidget::setComboxForThemeName (UkmediaMainWidget *m_pWidget,const char *name)
{
    g_debug("set combox for theme name");
//    qDebug() << "set combox for theme name" << name;
    gboolean      found;
    int count = 0;
    /* If the name is empty, use "freedesktop" */
    if (name == nullptr || *name == '\0') {
        name = "freedesktop";
    }
    QString value;
    int index = -1;
    while(!found) {
        value = m_pWidget->m_pThemeNameList->at(count);
        found = (value != "" && value == name);
        count++;
        if( count >= m_pWidget->m_pThemeNameList->size() || found) {
            count = 0;
            break;
        }
    }
    if (m_pWidget->m_pThemeNameList->contains(name)) {
        index = m_pWidget->m_pThemeNameList->indexOf(name);
//        if (index == -1) {
//            return;
//        }
        value = m_pWidget->m_pThemeNameList->at(index);
        m_pWidget->m_pSoundWidget->m_pSoundThemeCombobox->setCurrentIndex(index);
    }
    /* When we can't find the theme we need to set, try to set the default
     * one "freedesktop" */
    if (found) {
    } else if (strcmp (name, "freedesktop") != 0) {//????????????????????????
        qDebug() << "????????????????????????" << "freedesktop";
        g_debug ("not found, falling back to fdo");
        setComboxForThemeName (m_pWidget, "freedesktop");
    }
}

/*
    ???????????????
*/
void UkmediaMainWidget::updateAlertsFromThemeName (UkmediaMainWidget *m_pWidget,const gchar *m_pName)
{
    g_debug("update alerts from theme name");
    if (strcmp (m_pName, CUSTOM_THEME_NAME) != 0) {
            /* reset alert to default */
        updateAlert (m_pWidget, DEFAULT_ALERT_ID);
    } else {
        int   sound_type;
        char *linkname;
        linkname = nullptr;
        sound_type = getFileType ("bell-terminal", &linkname);
        g_debug ("Found link: %s", linkname);
        if (sound_type == SOUND_TYPE_CUSTOM) {
            updateAlert (m_pWidget, linkname);
        }
    }
}

/*
    ??????????????????
*/
void UkmediaMainWidget::updateAlert (UkmediaMainWidget *pWidget,const char *alertId)
{
    Q_UNUSED(alertId)
    g_debug("update alert");
    QString themeStr;
    char *theme;
    char *parent;
    gboolean      is_custom;
    gboolean      is_default;
    gboolean add_custom = false;
    gboolean remove_custom = false;
    QString nameStr;
    int index = -1;
    /* Get the current theme's name, and set the parent */
    index = pWidget->m_pSoundWidget->m_pSoundThemeCombobox->currentIndex();
    if (index != -1) {
        themeStr = pWidget->m_pThemeNameList->at(index);
        nameStr = pWidget->m_pThemeNameList->at(index);
    }
    else {
        themeStr = "freedesktop";
        nameStr = "freedesktop";
    }
    QByteArray ba = themeStr.toLatin1();
    theme = ba.data();

    QByteArray baParent = nameStr.toLatin1();
    parent = baParent.data();

    is_custom = strcmp (theme, CUSTOM_THEME_NAME) == 0;
    is_default = strcmp (alertId, DEFAULT_ALERT_ID) == 0;

//    qDebug() << "namestr:" << nameStr << "themeStr:" << themeStr << "parent:" << parent << "theme:" << theme;
    if (! is_custom && is_default) {
        /* remove custom just in case */
        remove_custom = TRUE;
    } else if (! is_custom && ! is_default) {
        createCustomTheme (parent);
        saveAlertSounds(pWidget->m_pSoundWidget->m_pSoundThemeCombobox, alertId);
        add_custom = TRUE;
    } else if (is_custom && is_default) {
        saveAlertSounds(pWidget->m_pSoundWidget->m_pSoundThemeCombobox, alertId);
        /* after removing files check if it is empty */
        if (customThemeDirIsEmpty ()) {
            remove_custom = TRUE;
        }
    } else if (is_custom && ! is_default) {
        saveAlertSounds(pWidget->m_pSoundWidget->m_pSoundThemeCombobox, alertId);
    }

    if (add_custom) {
        qDebug() << "add custom ????????????";
        setComboxForThemeName (pWidget, CUSTOM_THEME_NAME);
    } else if (remove_custom) {
        qDebug() << "remove custom ????????????";
        setComboxForThemeName (pWidget, parent);
    }
}

/*
    ????????????????????????
*/
int UkmediaMainWidget::getFileType (const char *sound_name,char **linked_name)
{
    g_debug("get file type");
    char *name, *filename;
    *linked_name = nullptr;
    name = g_strdup_printf ("%s.disabled", sound_name);
    filename = customThemeDirPath (name);
    if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) != FALSE) {
        return SOUND_TYPE_OFF;
    }
    /* We only check for .ogg files because those are the
     * only ones we create */
    name = g_strdup_printf ("%s.ogg", sound_name);
    filename = customThemeDirPath (name);
    g_free (name);

    if (g_file_test (filename, G_FILE_TEST_IS_SYMLINK) != FALSE) {
        *linked_name = g_file_read_link (filename, nullptr);
        g_free (filename);
        return SOUND_TYPE_CUSTOM;
    }
    g_free (filename);
    return SOUND_TYPE_BUILTIN;
}

/*
    ?????????????????????
*/
char *UkmediaMainWidget::customThemeDirPath (const char *child)
{
    g_debug("custom theme dir path");
    static char *dir = nullptr;
    const char *data_dir;

    if (dir == nullptr) {
        data_dir = g_get_user_data_dir ();
        dir = g_build_filename (data_dir, "sounds", CUSTOM_THEME_NAME, nullptr);
    }
    if (child == nullptr)
        return g_strdup (dir);

    return g_build_filename (dir, child, nullptr);
}

/*
    ?????????????????????????????????
*/
void UkmediaMainWidget::populateModelFromDir (UkmediaMainWidget *m_pWidget,const char *dirname)//?????????????????????????????????
{
    g_debug("populate model from dir");
    GDir *d;
    const char *name;
    d = g_dir_open (dirname, 0, nullptr);
    if (d == nullptr) {
        return;
    }
    while ((name = g_dir_read_name (d)) != nullptr) {
        char *path;

        if (! g_str_has_suffix (name, ".xml")) {
            continue;
        }
        path = g_build_filename (dirname, name, nullptr);
        populateModelFromFile (m_pWidget, path);
        g_free (path);
    }
    g_dir_close (d);
}

/*
    ????????????????????????
*/
void UkmediaMainWidget::populateModelFromFile (UkmediaMainWidget *m_pWidget,const char *filename)
{
    g_debug("populate model from file");
    xmlDocPtr  doc;
    xmlNodePtr root;
    xmlNodePtr child;
    gboolean   exists;

    exists = g_file_test (filename, G_FILE_TEST_EXISTS);
    if (! exists) {
        return;
    }
    doc = xmlParseFile (filename);
    if (doc == nullptr) {
        return;
    }
    root = xmlDocGetRootElement (doc);
    for (child = root->children; child; child = child->next) {
        if (xmlNodeIsText (child)) {
                continue;
        }
        if (xmlStrcmp (child->name, GVC_SOUND_SOUND) != 0) {
                continue;
        }
        populateModelFromNode (m_pWidget, child);
    }
    xmlFreeDoc (doc);
}

/*
    ???????????????????????????????????????????????????
*/
void UkmediaMainWidget::populateModelFromNode (UkmediaMainWidget *m_pWidget,xmlNodePtr node)
{
    g_debug("populate model from node");
    xmlNodePtr child;
    xmlChar   *filename;
    xmlChar   *name;

    filename = nullptr;
    name = xmlGetAndTrimNames (node);
    for (child = node->children; child; child = child->next) {
        if (xmlNodeIsText (child)) {
            continue;
        }

        if (xmlStrcmp (child->name, GVC_SOUND_FILENAME) == 0) {
            filename = xmlNodeGetContent (child);
        } else if (xmlStrcmp (child->name, GVC_SOUND_NAME) == 0) {
                /* EH? should have been trimmed */
        }
    }

    //????????????????????????????????????combox???
    if (filename != nullptr && name != nullptr) {
        m_pWidget->m_pSoundList->append((const char *)filename);
        m_pWidget->m_pSoundNameList->append((const char *)name);
        m_pWidget->m_pSoundWidget->m_pAlertSoundCombobox->addItem((char *)name);
        m_pWidget->m_pSoundWidget->m_pLagoutCombobox->addItem((char *)name);
        m_pWidget->m_pSoundWidget->m_pWindowClosedCombobox->addItem((char *)name);
        m_pWidget->m_pSoundWidget->m_pVolumeChangeCombobox->addItem((char *)name);
        m_pWidget->m_pSoundWidget->m_pSettingSoundCombobox->addItem((char *)name);
    }
    xmlFree (filename);
    xmlFree (name);
}

/* Adapted from yelp-toc-pager.c */
xmlChar *UkmediaMainWidget::xmlGetAndTrimNames (xmlNodePtr node)
{
    g_debug("xml get and trim names");
    xmlNodePtr cur;
    xmlChar *keep_lang = nullptr;
    xmlChar *value;
    int j, keep_pri = INT_MAX;
    const gchar * const * langs = g_get_language_names ();

    value = nullptr;
    for (cur = node->children; cur; cur = cur->next) {
        if (! xmlStrcmp (cur->name, GVC_SOUND_NAME)) {
            xmlChar *cur_lang = nullptr;
            int cur_pri = INT_MAX;
            cur_lang = xmlNodeGetLang (cur);

            if (cur_lang) {
                for (j = 0; langs[j]; j++) {
                    if (g_str_equal (cur_lang, langs[j])) {
                        cur_pri = j;
                        break;
                    }
                }
            } else {
                cur_pri = INT_MAX - 1;
            }

            if (cur_pri <= keep_pri) {
                if (keep_lang)
                    xmlFree (keep_lang);
                if (value)
                    xmlFree (value);

                value = xmlNodeGetContent (cur);

                keep_lang = cur_lang;
                keep_pri = cur_pri;
            } else {
                if (cur_lang)
                    xmlFree (cur_lang);
            }
        }
    }

    /* Delete all GVC_SOUND_NAME nodes */
    cur = node->children;
    while (cur) {
            xmlNodePtr p_this = cur;
            cur = cur->next;
            if (! xmlStrcmp (p_this->name, GVC_SOUND_NAME)) {
                xmlUnlinkNode (p_this);
                xmlFreeNode (p_this);
            }
    }
    return value;
}

/*
 * ??????????????????
*/
void UkmediaMainWidget::playAlretSoundFromPath (UkmediaMainWidget *w,QString path)
{
    g_debug("play alert sound from path");

   gchar * themeName = g_settings_get_string (w->m_pSoundSettings, SOUND_THEME_KEY);

   qDebug() << "????????????:" << themeName << "id :" << path.toLatin1().data();
   if (strcmp (path.toLatin1().data(), DEFAULT_ALERT_ID) == 0) {
       if (themeName != NULL) {
           caPlayForWidget (w, 0,
                            CA_PROP_APPLICATION_NAME, _("Sound Preferences"),
                            CA_PROP_EVENT_ID, "bell-window-system",
                            CA_PROP_CANBERRA_XDG_THEME_NAME, themeName,
                            CA_PROP_EVENT_DESCRIPTION, _("Testing event sound"),
                            CA_PROP_CANBERRA_CACHE_CONTROL, "never",
                            CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
                 #ifdef CA_PROP_CANBERRA_ENABLE
                            CA_PROP_CANBERRA_ENABLE, "1",
                 #endif
                            NULL);
       } else {
           caPlayForWidget (w, 0,
                            CA_PROP_APPLICATION_NAME, _("Sound Preferences"),
                            CA_PROP_EVENT_ID, "bell-window-system",
                            CA_PROP_EVENT_DESCRIPTION, _("Testing event sound"),
                            CA_PROP_CANBERRA_CACHE_CONTROL, "never",
                            CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
                 #ifdef CA_PROP_CANBERRA_ENABLE
                            CA_PROP_CANBERRA_ENABLE, "1",
                 #endif
                            NULL);
       }
   } else {
       caPlayForWidget (w, 0,
                        CA_PROP_APPLICATION_NAME, _("Sound Preferences"),
                        CA_PROP_MEDIA_FILENAME, path.toLatin1().data(),
                        CA_PROP_EVENT_DESCRIPTION, _("Testing event sound"),
                        CA_PROP_CANBERRA_CACHE_CONTROL, "never",
                        CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
                 #ifdef CA_PROP_CANBERRA_ENABLE
                        CA_PROP_CANBERRA_ENABLE, "1",
                 #endif
                        NULL);
   }
}

/*
    ??????combox????????????
*/
void UkmediaMainWidget::comboxIndexChangedSlot(int index)
{
    g_debug("combox index changed slot");
    QString sound_name = m_pSoundList->at(index);
    updateAlert(this,sound_name.toLatin1().data());
    playAlretSoundFromPath(this,sound_name);

    QString fileName = m_pSoundList->at(index);
    QStringList list = fileName.split("/");
    QString soundName = list.at(list.count()-1);
    QStringList eventIdList = soundName.split(".");
    QString eventId = eventIdList.at(0);
    QList<char *> existsPath = listExistsPath();

    for (char * path : existsPath) {

        char * prepath = QString(KEYBINDINGS_CUSTOM_DIR).toLatin1().data();
        char * allpath = strcat(prepath, path);

        const QByteArray ba(KEYBINDINGS_CUSTOM_SCHEMA);
        const QByteArray bba(allpath);
        if(QGSettings::isSchemaInstalled(ba))
        {
            QGSettings * settings = new QGSettings(ba, bba);
//            QString filenameStr = settings->get(FILENAME_KEY).toString();
            QString nameStr = settings->get(NAME_KEY).toString();
            if (nameStr == "alert-sound") {
                qDebug() << "??????????????????" <<  nameStr << eventId;
                settings->set(FILENAME_KEY,eventId);
                return;
            }
        }
        else {
            continue;
        }
    }

}

/*
    ??????????????????????????????
*/
void UkmediaMainWidget::windowClosedComboboxChangedSlot(int index)
{
    QString fileName = m_pSoundList->at(index);
    QStringList list = fileName.split("/");
    QString soundName = list.at(list.count()-1);
    QStringList eventIdList = soundName.split(".");
    QString eventId = eventIdList.at(0);
    QList<char *> existsPath = listExistsPath();

    for (char * path : existsPath) {

        char * prepath = QString(KEYBINDINGS_CUSTOM_DIR).toLatin1().data();
        char * allpath = strcat(prepath, path);

        const QByteArray ba(KEYBINDINGS_CUSTOM_SCHEMA);
        const QByteArray bba(allpath);
        if(QGSettings::isSchemaInstalled(ba))
        {
            QGSettings * settings = new QGSettings(ba, bba);
//            QString filenameStr = settings->get(FILENAME_KEY).toString();
            QString nameStr = settings->get(NAME_KEY).toString();
            if (nameStr == "window-close") {
                qDebug() << "??????????????????" <<  nameStr << eventId;
                settings->set(FILENAME_KEY,eventId);
                return;
            }
        }
        else {
            continue;
        }
    }
}

/*
    ?????????????????????????????????
*/
void UkmediaMainWidget::volumeChangedComboboxChangeSlot(int index)
{
    QString fileName = m_pSoundList->at(index);
    QStringList list = fileName.split("/");
    QString soundName = list.at(list.count()-1);
    QStringList eventIdList = soundName.split(".");
    QString eventId = eventIdList.at(0);
    QList<char *> existsPath = listExistsPath();
    for (char * path : existsPath) {

        char * prepath = QString(KEYBINDINGS_CUSTOM_DIR).toLatin1().data();
        char * allpath = strcat(prepath, path);
        const QByteArray ba(KEYBINDINGS_CUSTOM_SCHEMA);
        const QByteArray bba(allpath);
        if(QGSettings::isSchemaInstalled(ba))
        {
            QGSettings * settings = new QGSettings(ba, bba);
//            QString filenameStr = settings->get(FILENAME_KEY).toString();
            QString nameStr = settings->get(NAME_KEY).toString();
            if (nameStr == "volume-changed") {
                qDebug() << "??????????????????volume changed event id" << eventId;
                settings->set(FILENAME_KEY,eventId);
                return;
            }
        }
        else {
            continue;
        }
    }
}

void UkmediaMainWidget::settingMenuComboboxChangedSlot(int index)
{
    QString fileName = m_pSoundList->at(index);
    QStringList list = fileName.split("/");
    QString soundName = list.at(list.count()-1);
    QStringList eventIdList = soundName.split(".");
    QString eventId = eventIdList.at(0);
    QList<char *> existsPath = listExistsPath();
    for (char * path : existsPath) {

        char * prepath = QString(KEYBINDINGS_CUSTOM_DIR).toLatin1().data();
        char * allpath = strcat(prepath, path);
        const QByteArray ba(KEYBINDINGS_CUSTOM_SCHEMA);
        const QByteArray bba(allpath);
        if(QGSettings::isSchemaInstalled(ba))
        {
            QGSettings * settings = new QGSettings(ba, bba);
//            QString filenameStr = settings->get(FILENAME_KEY).toString();
            QString nameStr = settings->get(NAME_KEY).toString();
            if (nameStr == "system-setting") {
                qDebug() << "??????????????????system-setting event id" << eventId;
                settings->set(FILENAME_KEY,eventId);
                return;
            }
        }
        else {
            continue;
        }
    }
}

void UkmediaMainWidget::profileComboboxChangedSlot(int index)
{
    if (index >= m_pProfileNameList->count() )
        return;
    if (index < 0)
        return;
    QString profileName = m_pProfileNameList->at(index);
    QByteArray ba = profileName.toLatin1();
    const gchar *optionName =  ba.data();
    qDebug() << "profile combox changed ****************" << index << m_pProfileNameList->count() <<"option name" <<optionName << "???????????????" << m_pOutputWidget->m_pSelectCombobox->currentText();
    int devIndex = m_pOutputWidget->m_pSelectCombobox->currentIndex();
    QString deviceStr = m_pDeviceNameList->at(devIndex);
    QByteArray bba = deviceStr.toLatin1();
//    qDebug() << "device name" << m_pDeviceNameList->at(devIndex);
    const gchar * deviceName = bba.data();
    if (m_pSwitch == nullptr)
        qDebug() << "switch is null ===============";
    MateMixerStream *mStream = mate_mixer_context_get_default_output_stream(m_pContext);
    MateMixerDevice *mDevice = mate_mixer_context_get_device(m_pContext,deviceName);
//     = mate_mixer_stream_get_device(mStream);
    m_pSwitch = findDeviceProfileSwitch(this,mDevice);

    MateMixerSwitchOption *opt = mate_mixer_switch_get_option(m_pSwitch,optionName);
//    qDebug() << "combox: " << mate_mixer_switch_option_get_label(opt) << "option name :" << optionName << "device name :" << mate_mixer_device_get_label(m_pDevice);
    mate_mixer_switch_set_active_option(m_pSwitch,opt);
}

void UkmediaMainWidget::selectComboboxChangedSlot(int index)
{
    if (index > m_pProfileNameList->count() && index < 0)
        return;
    qDebug() << "index changed :" << index;//  << m_pProfileNameList->at(index);
    QString deviceStr = m_pDeviceNameList->at(index);
    QByteArray ba = deviceStr.toLatin1();
    const gchar *deviceName = ba.data();
    const gchar *profileLabel = nullptr;
    const gchar *profileName = nullptr;
    const gchar *setProfileLabel = nullptr;
    MateMixerSwitchOption *activeOption;
    MateMixerDevice *pDevice = mate_mixer_context_get_device(m_pContext,deviceName);
    const GList *switches;
    switches = mate_mixer_device_list_switches (MATE_MIXER_DEVICE(pDevice));
    m_pOutputWidget->m_pProfileCombobox->clear();
    m_pProfileNameList->clear();
    while (switches != nullptr) {
        MateMixerDeviceSwitch *swtch = MATE_MIXER_DEVICE_SWITCH (switches->data);
        const GList *options;
//        options = mate_mixer_switch_list_options ( MATE_MIXER_SWITCH(swtch));
        MateMixerSwitch *swtch1 = findDeviceProfileSwitch(this,pDevice);
        options = mate_mixer_switch_list_options(swtch1);
//        activeOption = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH(swtch));
        activeOption = mate_mixer_switch_get_active_option(swtch1);
        setProfileLabel  = mate_mixer_switch_option_get_label(activeOption) ;

        while (options != NULL) {
            MateMixerSwitchOption *option = MATE_MIXER_SWITCH_OPTION (options->data);
            profileLabel = mate_mixer_switch_option_get_label (option);
            profileName = mate_mixer_switch_option_get_name(option);
            qDebug() <<"?????????????????? ============ :" <<profileLabel;
            m_pProfileNameList->append(profileName);
            m_pOutputWidget->m_pProfileCombobox->addItem(profileLabel);
            /* Select the currently active option of the switch */
            options = options->next;
        }
        switches = switches->next;
    }
    if (setProfileLabel != nullptr)
        m_pOutputWidget->m_pProfileCombobox->setCurrentText(setProfileLabel);
//    qDebug() << "???????????????????????????:" << setProfileLabel;
}

/*
    ??????????????????????????????
*/
void UkmediaMainWidget::inputMuteButtonSlot()
{
    MateMixerStreamControl *pControl;
    MateMixerStream *pStream = mate_mixer_context_get_default_input_stream(m_pContext);
    if (pStream != nullptr)
        pControl = mate_mixer_stream_get_default_control(pStream);
    int volume = int(mate_mixer_stream_control_get_volume(pControl));
    volume = int(volume*100/65536.0 + 0.5);
    bool status = mate_mixer_stream_control_get_mute(pControl);
    if (status) {
        status = false;
       mate_mixer_stream_control_set_mute(pControl,status);
    }
    else {
        status =true;
        mate_mixer_stream_control_set_mute(pControl,status);
    }
}

/*
    ??????????????????????????????
*/
void UkmediaMainWidget::outputMuteButtonSlot()
{
    MateMixerStreamControl *pControl;
    MateMixerStream *pStream = mate_mixer_context_get_default_output_stream(m_pContext);
    if (pStream != nullptr)
        pControl = mate_mixer_stream_get_default_control(pStream);
    int volume = int(mate_mixer_stream_control_get_volume(pControl));
    volume = int(volume*100/65536.0 + 0.5);
    bool status = mate_mixer_stream_control_get_mute(pControl);
    if (status) {
        status = false;
        mate_mixer_stream_control_set_mute(pControl,status);
    }
    else {
        status =true;
        mate_mixer_stream_control_set_mute(pControl,status);
    }
}


/*
    ????????????????????????????????????
*/
void UkmediaMainWidget::themeComboxIndexChangedSlot(int index)
{
    Q_UNUSED(index);
    g_debug("theme combox index changed slot");
    if (index == -1) {
        return;
    }
    //??????????????????
    QString theme = m_pThemeNameList->at(index);
    QByteArray ba = theme.toLatin1();
    const char *m_pThemeName = ba.data();
    gboolean ok = g_settings_set_string (m_pSoundSettings, SOUND_THEME_KEY, m_pThemeName);
    qDebug() << "index changed:" << index << m_pThemeNameList->at(index) << m_pThemeName << "????????????????????????" << ok;

    if (strcmp(m_pThemeName,"freedesktop") == 0) {
//        m_pSoundWidget->m_pAlertSoundCombobox->setCurrentIndex();
        int index = 0;
        for (int i=0;i<m_pSoundList->count();i++) {
            QString str = m_pSoundList->at(i);
            if (str.contains("drip",Qt::CaseSensitive)) {
                index = i;
                break;
            }
        }

        QString displayName = m_pSoundNameList->at(index);
        m_pSoundWidget->m_pAlertSoundCombobox->setCurrentText(displayName);
    }

    /* special case for no sounds */
    if (strcmp (m_pThemeName, NO_SOUNDS_THEME_NAME) == 0) {
        //?????????????????????
        g_settings_set_boolean (m_pSoundSettings, EVENT_SOUNDS_KEY, FALSE);
        return;
    } else {
        g_settings_set_boolean (m_pSoundSettings, EVENT_SOUNDS_KEY, TRUE);
    }

}

/*
    ??????????????????combox????????????
*/
void UkmediaMainWidget::outputDeviceComboxIndexChangedSlot(QString str)
{
    g_debug("output device combox index changed slot");
    MateMixerBackendFlags flags;
    int index = m_pOutputWidget->m_pOutputDeviceCombobox->findText(str);
    if (index == -1)
        return;
    const QString str1 =  m_pOutputStreamList->at(index);
    const gchar *name = str1.toLocal8Bit();
    MateMixerStream *stream = mate_mixer_context_get_stream(m_pContext,name);
    if (G_UNLIKELY (stream == nullptr)) {
       g_warn_if_reached ();
//       g_free (name);
       return;
    }

    flags = mate_mixer_context_get_backend_flags (m_pContext);

    if (flags & MATE_MIXER_BACKEND_CAN_SET_DEFAULT_OUTPUT_STREAM) {

        mate_mixer_context_set_default_output_stream (m_pContext, stream);
        m_pStream = stream;
        MateMixerStreamControl *c = mate_mixer_stream_get_default_control(stream);
        int(mate_mixer_stream_control_get_volume(c) *100 /65536.0+0.5);
        /*miniWidget->masterVolumeSlider->setValue(volume);*/
    }
    else {
        setOutputStream(this, stream);
    }
}

/*
    ??????????????????combox??????
*/
void UkmediaMainWidget::inputDeviceComboxIndexChangedSlot(QString str)
{
    g_debug("input device combox index changed slot");
    MateMixerBackendFlags flags;
    int index = m_pInputWidget->m_pInputDeviceCombobox->findText(str);
    if (index == -1)
        return;
    const QString str1 =  m_pInputStreamList->at(index);
    const gchar *name = str1.toLocal8Bit();
    MateMixerStream *stream = mate_mixer_context_get_stream(m_pContext,name);
    if (G_UNLIKELY (stream == nullptr)) {
       g_warn_if_reached ();
//       g_free (name);
       return;
    }

    flags = mate_mixer_context_get_backend_flags (m_pContext);

    if (flags & MATE_MIXER_BACKEND_CAN_SET_DEFAULT_INPUT_STREAM) {
        m_pStream = stream;
        mate_mixer_context_set_default_input_stream (m_pContext, stream);
        MateMixerStreamControl *c = mate_mixer_stream_get_default_control(stream);
    }
    else {
        setInputStream(this, stream);
    }
}

void UkmediaMainWidget::setOutputStream (UkmediaMainWidget *m_pWidget, MateMixerStream *m_pStream)
{
    g_debug("set output stream");

    int i = 0;
    if (m_pStream == nullptr) {
        return;
    }
    MateMixerStreamControl *m_pControl;
    ukuiBarSetStream(m_pWidget,m_pStream);
    if (m_pStream != nullptr) {
        const GList *controls;
        controls = mate_mixer_context_list_stored_controls (m_pWidget->m_pContext);
        if (controls == nullptr) {
            return;
        }
        /* Move all stored controls to the newly selected default stream */
        while (controls != nullptr) {
            MateMixerStream        *parent;
            MateMixerStreamControl *m_pControl;
            m_pControl = MATE_MIXER_STREAM_CONTROL (controls->data);
            parent  = mate_mixer_stream_control_get_stream (m_pControl);

            /* Prefer streamless controls to stay the way they are, forcing them to
            * a particular owning stream would be wrong for eg. event controls */
            if (parent != nullptr && parent != m_pStream) {
                MateMixerDirection direction = mate_mixer_stream_get_direction (parent);

                if (direction == MATE_MIXER_DIRECTION_OUTPUT)
                    mate_mixer_stream_control_set_stream (m_pControl, m_pStream);
            }
            controls = controls->next;
        }
    }
    updateOutputStreamList (m_pWidget, m_pStream);
    if (m_pControl == nullptr) {
        return;
    }
    updateOutputSettings(m_pWidget,m_pWidget->m_pOutputBarStreamControl);
}

/*
    ????????????stream ??????
*/
void UkmediaMainWidget::updateOutputStreamList(UkmediaMainWidget *m_pWidget,MateMixerStream *m_pStream)
{
    Q_UNUSED(m_pWidget);
    g_debug("update output stream list");
    const gchar *m_pName = nullptr;
    if (m_pStream != nullptr) {
        m_pName = mate_mixer_stream_get_name(m_pStream);
    }
}

/*
    bar??????stream
*/
void UkmediaMainWidget::ukuiBarSetStream (UkmediaMainWidget  *w,MateMixerStream *m_pStream)
{
    g_debug("ukui bar set stream");
    MateMixerStreamControl *m_pControl = nullptr;

    if (m_pStream != nullptr)
        m_pControl = mate_mixer_stream_get_default_control (m_pStream);
    MateMixerDirection direction = mate_mixer_stream_get_direction(m_pStream);
    ukuiBarSetStreamControl (w,direction,m_pControl);
}

void UkmediaMainWidget::ukuiBarSetStreamControl (UkmediaMainWidget *m_pWidget,MateMixerDirection direction,MateMixerStreamControl *m_pControl)
{
    Q_UNUSED(m_pWidget);
    g_debug("ukui bar set stream control");
    const gchar *m_pName;
    if (m_pControl != nullptr) {
        if (direction == MATE_MIXER_DIRECTION_OUTPUT) {
            m_pWidget->m_pOutputBarStreamControl = m_pControl;
        }
        else if (direction == MATE_MIXER_DIRECTION_INPUT) {
            m_pWidget->m_pInputBarStreamControl = m_pControl;
        }
        m_pName = mate_mixer_stream_control_get_name (m_pControl);
//        qDebug() << "ukuiBarSetStreamControl*********" << m_pName << direction;
    }
}


void UkmediaMainWidget::ukuiInputLevelSetProperty (UkmediaMainWidget *m_pWidget)
{
    g_debug("ukui input level set property");
    scale = GVC_LEVEL_SCALE_LINEAR;
    ukuiInputLevelSetScale (m_pWidget, m_pWidget->scale);
}

void UkmediaMainWidget::ukuiInputLevelSetScale (UkmediaMainWidget *m_pWidget, LevelScale scale)
{
    g_debug("ukui input level set scale");
    if (scale != m_pWidget->scale) {
        ukuiUpdatePeakValue (m_pWidget);
    }
}

void UkmediaMainWidget::ukuiUpdatePeakValue (UkmediaMainWidget *m_pWidget)
{
    g_debug("ukui update peak value");
    gdouble value = ukuiFractionFromAdjustment(m_pWidget);
    m_pWidget->peakFraction = value;

    if (value > m_pWidget->maxPeak) {
        if (m_pWidget->maxPeakId > 0)
            g_source_remove (m_pWidget->maxPeakId);
        m_pWidget->maxPeak = value;
    }
}

/*
    ???????????????????????????
*/
void UkmediaMainWidget::outputWidgetSliderChangedSlot(int value)
{
    m_pStream = mate_mixer_context_get_default_output_stream(m_pContext);
    if (m_pStream != nullptr)
        m_pControl = mate_mixer_stream_get_default_control(m_pStream);

    QString percent;
    bool status = false;
    percent = QString::number(value);
    int volume = value*65536/100;
    mate_mixer_stream_control_set_volume(m_pControl,guint(volume));
    if (value <= 0) {
        status = true;
        mate_mixer_stream_control_set_mute(m_pControl,status);
        mate_mixer_stream_control_set_volume(m_pControl,0);
        percent = QString::number(0);
    }
    else {
        if (firstEnterSystem) {
            bool status = mate_mixer_stream_control_get_mute(m_pControl);
            mate_mixer_stream_control_set_mute(m_pControl,status);
        }
        else {
            mate_mixer_stream_control_set_mute(m_pControl,status);
        }
    }
    firstEnterSystem = false;
    outputVolumeDarkThemeImage(value,status);
    percent.append("%");
    m_pOutputWidget->m_pOpVolumePercentLabel->setText(percent);
    m_pOutputWidget->m_pOutputIconBtn->repaint();

}

/*
    ?????????????????????
*/
void UkmediaMainWidget::inputWidgetSliderChangedSlot(int value)
{
    m_pStream = mate_mixer_context_get_default_input_stream(m_pContext);
    m_pControl = mate_mixer_stream_get_default_control(m_pStream);

    QString percent;
    bool status = false;
    if (value <= 0) {
        status = true;
        mate_mixer_stream_control_set_mute(m_pControl,status);
        mate_mixer_stream_control_set_volume(m_pControl,0);
        percent = QString::number(0);
    }
    //?????????????????????????????????

    inputVolumeDarkThemeImage(value,status);
    m_pInputWidget->m_pInputIconBtn->repaint();
    percent = QString::number(value);
    value = value * 65536 / 100;
    mate_mixer_stream_control_set_mute(m_pControl,status);
    mate_mixer_stream_control_set_volume(m_pControl,value);
    percent.append("%");
    m_pInputWidget->m_pInputIconBtn->repaint();
    m_pInputWidget->m_pIpVolumePercentLabel->setText(percent);
}

/*
    ???????????????????????????
*/
void UkmediaMainWidget::alertVolumeSliderChangedSlot(int value)
{
    if (m_pMediaRoleControl != nullptr) {
        mate_mixer_stream_control_set_volume(m_pMediaRoleControl,value*65535/100);
        this->m_pSoundWidget->m_pAlertVolumeLabel->setText(QString::number(value).append("%"));

        alertIconButtonSetIcon(false,value);
        m_pSoundWidget->m_pAlertIconBtn->repaint();
    }
    else {
        volume.channels = 1;
        volume.values[0] = value*65536/100;
        info.volume = volume;
        updateRole(info);
    }
}

/*
    ?????????????????????
*/
void UkmediaMainWidget::alertSoundVolumeChangedSlot()
{
    bool states = mate_mixer_stream_control_get_mute(m_pMediaRoleControl);
    int volume = m_pSoundWidget->m_pAlertSlider->value();

    mate_mixer_stream_control_set_mute(m_pMediaRoleControl,!states);
    alertIconButtonSetIcon(!states,volume);
    m_pSoundWidget->m_pAlertIconBtn->repaint();
}

void UkmediaMainWidget::inputPortComboxChangedSlot(int index)
{
    if (index < 0)
        return;
    QString portStr = m_pInputPortList->at(index);
    QByteArray ba = portStr.toLatin1();
    const char *portName = ba.data();
    MateMixerStream *stream = mate_mixer_context_get_default_input_stream(m_pContext);
    MateMixerSwitch *portSwitch = findStreamPortSwitch (this,stream);
    if (portSwitch != nullptr) {

        MateMixerSwitchOption *opt = mate_mixer_switch_get_option(portSwitch,portName);
        mate_mixer_switch_set_active_option(MATE_MIXER_SWITCH(portSwitch),opt);
    }
}

void UkmediaMainWidget::outputPortComboxChangedSlot(int index)
{
    if (index < 0)
        return;
    QString portStr = m_pOutputPortList->at(index);
    QByteArray ba = portStr.toLatin1();
    const char *portName = ba.data();
    MateMixerStream *stream = mate_mixer_context_get_default_output_stream(m_pContext);
    MateMixerSwitch *portSwitch = findStreamPortSwitch (this,stream);
    if (portSwitch != nullptr) {

        MateMixerSwitchOption *opt = mate_mixer_switch_get_option(portSwitch,portName);
        m_pOutputWidget->m_pOutputPortCombobox->blockSignals(true);
        mate_mixer_switch_set_active_option(MATE_MIXER_SWITCH(portSwitch),opt);
        m_pOutputWidget->m_pOutputPortCombobox->blockSignals(false);
    }
}

void UkmediaMainWidget::inputLevelValueChangedSlot()
{
    g_debug("input level value changed slot");
    ukuiUpdatePeakValue(this);
}

gdouble UkmediaMainWidget::ukuiFractionFromAdjustment (UkmediaMainWidget *m_pWidget)
{
    g_debug("ukui fraction from adjustment");
    gdouble level;
    gdouble fraction = 0.0;
    gdouble min;
    gdouble max;

    level = m_pWidget->m_pInputWidget->m_pInputLevelSlider->value();
    min = m_pWidget->m_pInputWidget->m_pInputLevelSlider->minimum();
    max = m_pWidget->m_pInputWidget->m_pInputLevelSlider->maximum();

    switch (m_pWidget->scale) {
    case GVC_LEVEL_SCALE_LINEAR:
            fraction = (level - min) / (max - min);
            break;
    case GVC_LEVEL_SCALE_LOG:
            fraction = log10 ((level - min + 1) / (max - min + 1));
            break;
    }
    return fraction;
}

/*
    ??????????????????w
*/
void UkmediaMainWidget::updateInputSettings (UkmediaMainWidget *m_pWidget,MateMixerStreamControl *m_pControl)
{
    g_debug ("updating input settings");
    MateMixerStream            *stream;
    MateMixerStreamControlFlags flags;
    MateMixerSwitch            *portSwitch;
    qDebug() << "??????????????????---------------------";
    /* Get the control currently associated with the input slider */
    if (m_pControl == nullptr)
        return;
    /* Get owning stream of the control */
    qDebug() << "control name is :" << mate_mixer_stream_control_get_label(m_pControl);
    stream = mate_mixer_stream_control_get_stream (m_pControl);
    if (G_UNLIKELY (stream == nullptr))
        return;

    if(m_pWidget->m_pInputWidget->m_pInputPortCombobox->count() != 0 || m_pWidget->m_pInputPortList->count() != 0) {
        m_pWidget->m_pInputPortList->clear();
        m_pWidget->m_pInputWidget->m_pInputPortCombobox->clear();
        m_pWidget->m_pInputWidget->inputWidgetRemovePort();
    }

    flags = mate_mixer_stream_control_get_flags (m_pControl);

    /* Enable level bar only if supported by the control */
    if (flags & MATE_MIXER_STREAM_CONTROL_HAS_MONITOR) {
        g_signal_connect (G_OBJECT (m_pControl),
                          "monitor-value",
                          G_CALLBACK (onStreamControlMonitorValue),
                          m_pWidget);
    }

    /* Enable the port selector if the stream has one */
    portSwitch = findStreamPortSwitch (m_pWidget,stream);
    if (portSwitch != nullptr) {
        const GList *options;
        options = mate_mixer_switch_list_options(MATE_MIXER_SWITCH(portSwitch));
        while (options != nullptr) {
            MateMixerSwitchOption *opt = MATE_MIXER_SWITCH_OPTION(options->data);
            QString label = mate_mixer_switch_option_get_label(opt);
            QString name = mate_mixer_switch_option_get_name(opt);
            m_pWidget->m_pInputPortList->append(name);
            m_pWidget->m_pInputWidget->m_pInputPortCombobox->addItem(label);
            options = options->next;
        }
        MateMixerSwitchOption *option = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH(portSwitch));
        QString label = mate_mixer_switch_option_get_label(option);
        if (m_pWidget->m_pInputPortList->count() > 0) {
            qDebug() << "??????????????????????????????:" << label;
            m_pWidget->m_pInputWidget->inputWidgetAddPort();
            m_pWidget->m_pInputWidget->m_pInputPortCombobox->setCurrentText(label);
        }
        connect(m_pWidget->m_pInputWidget->m_pInputPortCombobox,SIGNAL(currentIndexChanged(int)),m_pWidget,SLOT(inputPortComboxChangedSlot(int)));
    }

}

MateMixerSwitch* UkmediaMainWidget::findStreamPortSwitch (UkmediaMainWidget *widget,MateMixerStream *stream)
{
    const GList *switches;
//    stream = mate_mixer_context_get_default_input_stream(widget->m_pContext);
    switches = mate_mixer_stream_list_switches (stream);
    while (switches != nullptr) {
        MateMixerStreamSwitch *swtch = MATE_MIXER_STREAM_SWITCH (switches->data);
        if (!MATE_MIXER_IS_STREAM_TOGGLE (swtch) &&
                mate_mixer_stream_switch_get_role (swtch) == MATE_MIXER_STREAM_SWITCH_ROLE_PORT) {
            return MATE_MIXER_SWITCH (swtch);
        }
        switches = switches->next;
    }
    return NULL;
}

void UkmediaMainWidget::onStreamControlMonitorValue (MateMixerStream *m_pStream,gdouble value,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(m_pStream);
    g_debug("on stream control monitor value");
    value = value*100;
    if (value >= 0) {
        m_pWidget->m_pInputWidget->m_pInputLevelSlider->setValue(value);
    }
    else {
        m_pWidget->m_pInputWidget->m_pInputLevelSlider->setValue(0);
    }
}

/*
    ??????stream control add
*/
void UkmediaMainWidget::onInputStreamControlAdded (MateMixerStream *m_pStream,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    g_debug("on input stream control added");
    MateMixerStreamControl *m_pControl;
    m_pControl = mate_mixer_stream_get_control (m_pStream, m_pName);
    if G_LIKELY (m_pControl != nullptr) {
        MateMixerStreamControlRole role = mate_mixer_stream_control_get_role (m_pControl);

        /* Non-application input control doesn't affect the icon */
        if (role != MATE_MIXER_STREAM_CONTROL_ROLE_APPLICATION) {
            return;
        }
    }

    /* Either an application control has been added or we couldn't
     * read the control, this shouldn't happen but let's revalidate the
     * icon to be sure if it does */
    updateIconInput (m_pWidget);
}

/*
    ??????stream control removed
*/
void UkmediaMainWidget::onInputStreamControlRemoved (MateMixerStream *m_pStream,const gchar *m_pName,UkmediaMainWidget *m_pWidget)
{
    Q_UNUSED(m_pStream);
    Q_UNUSED(m_pName);
    g_debug("on input stream control removed");
    updateIconInput (m_pWidget);
}

/*
    ?????????????????????stream
*/
gboolean UkmediaMainWidget::updateDefaultInputStream (UkmediaMainWidget *m_pWidget)
{
    g_debug("update default input stream");
    MateMixerStream *m_pStream;
    m_pStream = mate_mixer_context_get_default_input_stream (m_pWidget->m_pContext);

    m_pWidget->m_pInput = (m_pStream == nullptr) ? nullptr : m_pStream;
    if (m_pWidget->m_pInput != nullptr) {
        g_signal_connect (G_OBJECT (m_pWidget->m_pInput),
                          "control-added",
                          G_CALLBACK (onInputStreamControlAdded),
                          m_pWidget);
        g_signal_connect (G_OBJECT (m_pWidget->m_pInput),
                          "control-removed",
                          G_CALLBACK (onInputStreamControlRemoved),
                          m_pWidget);
    }

    /* Return TRUE if the default input stream has changed */
    return TRUE;
}

gboolean UkmediaMainWidget::saveAlertSounds (QComboBox *combox,const char *id)
{
    const char *sounds[3] = { "bell-terminal", "bell-window-system", NULL };
    char *path;

    if (strcmp (id, DEFAULT_ALERT_ID) == 0) {
        deleteOldFiles (sounds);
        deleteDisabledFiles (sounds);
    } else {
        deleteOldFiles (sounds);
        deleteDisabledFiles (sounds);
        addCustomFile (sounds, id);
    }

    /* And poke the directory so the theme gets updated */
    path = customThemeDirPath(NULL);
    if (utime (path, NULL) != 0) {
        g_warning ("Failed to update mtime for directory '%s': %s",
                   path, g_strerror (errno));
    }
    g_free (path);

    return FALSE;
}

void UkmediaMainWidget::deleteOldFiles (const char **sounds)
{
    guint i;
    for (i = 0; sounds[i] != NULL; i++) {
        deleteOneFile (sounds[i], "%s.ogg");
    }
}

void UkmediaMainWidget::deleteOneFile (const char *sound_name, const char *pattern)
{
        GFile *file;
        char *name, *filename;

        name = g_strdup_printf (pattern, sound_name);
        filename = customThemeDirPath(name);
        g_free (name);
        file = g_file_new_for_path (filename);
        g_free (filename);
        cappletFileDeleteRecursive (file, NULL);
        g_object_unref (file);
}


void UkmediaMainWidget::deleteDisabledFiles (const char **sounds)
{
    guint i;
    for (i = 0; sounds[i] != NULL; i++) {
        deleteOneFile (sounds[i], "%s.disabled");
    }
}

void UkmediaMainWidget::addCustomFile (const char **sounds, const char *filename)
{
    guint i;

    for (i = 0; sounds[i] != NULL; i++) {
        GFile *file;
        char *name, *path;

        /* We use *.ogg because it's the first type of file that
                 * libcanberra looks at */
        name = g_strdup_printf ("%s.ogg", sounds[i]);
        path = customThemeDirPath(name);
        g_free (name);
        /* In case there's already a link there, delete it */
        g_unlink (path);
        file = g_file_new_for_path (path);
        g_free (path);

        /* Create the link */
        g_file_make_symbolic_link (file, filename, NULL, NULL);
        g_object_unref (file);
    }
}

/**
 * capplet_file_delete_recursive :
 * @file :
 * @error  :
 *
 * A utility routine to delete files and/or directories,
 * including non-empty directories.
 **/
gboolean UkmediaMainWidget::cappletFileDeleteRecursive (GFile *file, GError **error)
{
    GFileInfo *info;
    GFileType type;

    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    info = g_file_query_info (file,
                              G_FILE_ATTRIBUTE_STANDARD_TYPE,
                              G_FILE_QUERY_INFO_NONE,
                              NULL, error);
    if (info == NULL) {
        return FALSE;
    }

    type = g_file_info_get_file_type (info);
    g_object_unref (info);

    if (type == G_FILE_TYPE_DIRECTORY) {
        return directoryDeleteRecursive (file, error);
    }
    else {
        return g_file_delete (file, NULL, error);
    }
}

gboolean UkmediaMainWidget::directoryDeleteRecursive (GFile *directory, GError **error)
{
    GFileEnumerator *enumerator;
    GFileInfo *info;
    gboolean success = TRUE;

    enumerator = g_file_enumerate_children (directory,
                                            G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                            G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                            G_FILE_QUERY_INFO_NONE,
                                            NULL, error);
    if (enumerator == NULL)
        return FALSE;

    while (success &&
           (info = g_file_enumerator_next_file (enumerator, NULL, NULL))) {
        GFile *child;

        child = g_file_get_child (directory, g_file_info_get_name (info));

        if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
            success = directoryDeleteRecursive (child, error);
        }
        g_object_unref (info);

        if (success)
            success = g_file_delete (child, NULL, error);
    }
    g_file_enumerator_close (enumerator, NULL, NULL);

    if (success)
        success = g_file_delete (directory, NULL, error);

    return success;
}

void UkmediaMainWidget::createCustomTheme (const char *parent)
{
    GKeyFile *keyfile;
    char     *data;
    char     *path;

    /* Create the custom directory */
    path = customThemeDirPath(NULL);
    g_mkdir_with_parents (path, 0755);
    g_free (path);

    qDebug() << "create_custom_theme" << parent;
    /* Set the data for index.theme */
    keyfile = g_key_file_new ();
    g_key_file_set_string (keyfile, "Sound Theme", "Name", _("Custom"));
    g_key_file_set_string (keyfile, "Sound Theme", "Inherits", parent);
    g_key_file_set_string (keyfile, "Sound Theme", "Directories", ".");
    data = g_key_file_to_data (keyfile, NULL, NULL);
    g_key_file_free (keyfile);

    /* Save the index.theme */
    path = customThemeDirPath ("index.theme");
    g_file_set_contents (path, data, -1, NULL);
    g_free (path);
    g_free (data);

    customThemeUpdateTime ();
}

/* This function needs to be called after each individual
 * changeset to the theme */
void UkmediaMainWidget::customThemeUpdateTime (void)
{
    char *path;
    path = customThemeDirPath (NULL);
    utime (path, NULL);
    g_free (path);
}

gboolean UkmediaMainWidget::customThemeDirIsEmpty (void)
{
    char            *dir;
    GFile           *file;
    gboolean         is_empty;
    GFileEnumerator *enumerator;
    GFileInfo       *info;
    GError          *error = NULL;

    dir = customThemeDirPath(NULL);
    file = g_file_new_for_path (dir);
    g_free (dir);

    is_empty = TRUE;

    enumerator = g_file_enumerate_children (file,
                                            G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                            G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                            G_FILE_QUERY_INFO_NONE,
                                            NULL, &error);
    if (enumerator == NULL) {
        g_warning ("Unable to enumerate files: %s", error->message);
        g_error_free (error);
        goto out;
    }

    while (is_empty &&
           (info = g_file_enumerator_next_file (enumerator, NULL, NULL))) {

        if (strcmp ("index.theme", g_file_info_get_name (info)) != 0) {
            is_empty = FALSE;
        }

        g_object_unref (info);
    }
    g_file_enumerator_close (enumerator, NULL, NULL);

out:
    g_object_unref (file);

    return is_empty;
}

int UkmediaMainWidget::caPlayForWidget(UkmediaMainWidget *w, uint32_t id, ...)
{
    va_list ap;
    int ret;
    ca_proplist *p;

    if ((ret = ca_proplist_create(&p)) < 0)
        return ret;

    if ((ret = caProplistSetForWidget(p, w)) < 0)
        return -1;

    va_start(ap, id);
    ret = caProplistMergeAp(p, ap);
    va_end(ap);

    if (ret < 0)
        return -1;
    ca_context *c ;
    ca_context_create(&c);
    ret = ca_context_play_full(c, id, p, NULL, NULL);

    return ret;
}

int UkmediaMainWidget::caProplistMergeAp(ca_proplist *p, va_list ap)
{
    int ret;
    for (;;) {
        const char *key, *value;

        if (!(key = va_arg(ap, const char*)))
            break;

        if (!(value = va_arg(ap, const char*)))
            return CA_ERROR_INVALID;

        if ((ret = ca_proplist_sets(p, key, value)) < 0)
            return ret;
    }

    return CA_SUCCESS;
}

int UkmediaMainWidget::caProplistSetForWidget(ca_proplist *p, UkmediaMainWidget *widget)
{
    int ret;
    const char *t;
    QScreen *screen;
    gint x = -1;
    gint y = -1;
    gint width = -1;
    gint height = -1;
    gint screen_width = -1;
    gint screen_height = -1;

    if ((t = widget->windowTitle().toLatin1().data()))
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_NAME, t)) < 0)
            return ret;

    if (t)
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_ID, t)) < 0)
            return ret;

    if ((t = widget->windowIconText().toLatin1().data()))
        if ((ret = ca_proplist_sets(p, CA_PROP_WINDOW_ICON_NAME, t)) < 0)
            return ret;
    if (screen = qApp->primaryScreen()) {
        if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_X11_SCREEN, "%i", 0)) < 0)
            return ret;
    }

    width = widget->size().width();
    height = widget->size().height();

    if (width > 0)
        if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_WIDTH, "%i", width)) < 0)
            return ret;
    if (height > 0)
        if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_HEIGHT, "%i", height)) < 0)
            return ret;

    if (x >= 0 && width > 0) {
        screen_width = qApp->primaryScreen()->size().width();

        x += width/2;
        x = CA_CLAMP(x, 0, screen_width-1);

        /* We use these strange format strings here to avoid that libc
                         * applies locale information on the formatting of floating
                         * numbers. */

        if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_HPOS, "%i.%03i",
                                    (int) (x/(screen_width-1)), (int) (1000.0*x/(screen_width-1)) % 1000)) < 0)
            return ret;
    }

    if (y >= 0 && height > 0) {
        screen_height = qApp->primaryScreen()->size().height();

        y += height/2;
        y = CA_CLAMP(y, 0, screen_height-1);

        if ((ret = ca_proplist_setf(p, CA_PROP_WINDOW_VPOS, "%i.%03i",
                                    (int) (y/(screen_height-1)), (int) (1000.0*y/(screen_height-1)) % 1000)) < 0)
            return ret;
    }
    return CA_SUCCESS;
}

QList<char *> UkmediaMainWidget::listExistsPath()
{
    char ** childs;
    int len;

    DConfClient * client = dconf_client_new();
    childs = dconf_client_list (client, KEYBINDINGS_CUSTOM_DIR, &len);
    g_object_unref (client);

    QList<char *> vals;

    for (int i = 0; childs[i] != NULL; i++){
        if (dconf_is_rel_dir (childs[i], NULL)){
            char * val = g_strdup (childs[i]);

            vals.append(val);
        }
    }
    g_strfreev (childs);
    return vals;
}

QString UkmediaMainWidget::findFreePath(){
    int i = 0;
    char * dir;
    bool found;
    QList<char *> existsdirs;

    existsdirs = listExistsPath();

    for (; i < MAX_CUSTOM_SHORTCUTS; i++){
        found = true;
        dir = QString("custom%1/").arg(i).toLatin1().data();
        for (int j = 0; j < existsdirs.count(); j++)
            if (!g_strcmp0(dir, existsdirs.at(j))){
                found = false;
                break;
            }
        if (found)
            break;
    }

    if (i == MAX_CUSTOM_SHORTCUTS){
        qDebug() << "Keyboard Shortcuts" << "Too many custom shortcuts";
        return "";
    }

    return QString("%1%2").arg(KEYBINDINGS_CUSTOM_DIR).arg(QString(dir));
}

void UkmediaMainWidget::addValue(QString name,QString filename)
{
    //?????????setting????????????????????????????????????????????????????????????
    QList<char *> existsPath = listExistsPath();

    for (char * path : existsPath) {

        char * prepath = QString(KEYBINDINGS_CUSTOM_DIR).toLatin1().data();
        char * allpath = strcat(prepath, path);

        const QByteArray ba(KEYBINDINGS_CUSTOM_SCHEMA);
        const QByteArray bba(allpath);
        if(QGSettings::isSchemaInstalled(ba))
        {
            QGSettings * settings = new QGSettings(ba, bba);
            QString filenameStr = settings->get(FILENAME_KEY).toString();
            QString nameStr = settings->get(NAME_KEY).toString();

            g_warning("full path: %s", allpath);
            qDebug() << filenameStr << FILENAME_KEY <<NAME_KEY << nameStr;
            if (nameStr == name) {
                qDebug() << "??????????????????????????????" ;
                return;
            }
            delete settings;
        }
        else {
            continue;
        }

    }
    QString availablepath = findFreePath();

    qDebug() << "Add Path" << availablepath;

    const QByteArray id(KEYBINDINGS_CUSTOM_SCHEMA);
    const QByteArray idd(availablepath.toUtf8().data());
    if(QGSettings::isSchemaInstalled(id))
    {
        QGSettings * settings = new QGSettings(id, idd);
        settings->set(FILENAME_KEY, filename);
        settings->set(NAME_KEY, name);
    }
//    delete settings;
}

MateMixerSwitch * UkmediaMainWidget::findDeviceProfileSwitch (UkmediaMainWidget *w,MateMixerDevice *device)
{
    const GList *switches;
    const gchar *profileLabel = nullptr;
    const gchar *devName = nullptr;
    devName = mate_mixer_device_get_name(device);
    switches = mate_mixer_device_list_switches (device);
    while (switches != nullptr) {
        MateMixerDeviceSwitch *swtch = MATE_MIXER_DEVICE_SWITCH (switches->data);
        MateMixerSwitchOption *active;
        active = mate_mixer_switch_get_active_option (MATE_MIXER_SWITCH (swtch));
//        w->m_pOutputWidget->m_pProfileCombobox->setCurrentText(profileLabel);
        if (G_LIKELY (active != NULL)) {
            profileLabel = mate_mixer_switch_option_get_label (active);
        }
//        if (w->m_pOutputWidget->m_pProfileCombobox)
        int devIndex = w->m_pOutputWidget->m_pSelectCombobox->currentIndex();
        QString deviceStr = w->m_pDeviceNameList->at(devIndex);
        QByteArray bba = deviceStr.toLatin1();
        const gchar * deviceName = bba.data();
        qDebug() << "profilelabel :" << devName << "device name :" << mate_mixer_device_get_name(device) <<deviceName;
        if (strcmp(deviceName,devName) == 0) {
            qDebug() << "??????????????????*****************";
//            w->m_pOutputWidget->m_pProfileCombobox->setCurrentText(profileLabel);
        }
        if (mate_mixer_device_switch_get_role (swtch) == MATE_MIXER_DEVICE_SWITCH_ROLE_PROFILE)
            return MATE_MIXER_SWITCH (swtch);

        switches =  switches->next;
    }
    return nullptr;
}

gchar * UkmediaMainWidget::deviceStatus (MateMixerDevice *device)
{
    guint        inputs = 0;
    guint        outputs = 0;
    gchar       *inputs_str = NULL;
    gchar       *outputs_str = NULL;
    const GList *streams;

    /* Get number of input and output streams in the device */
    streams = mate_mixer_device_list_streams (device);
    while (streams != NULL) {
        MateMixerStream   *stream = MATE_MIXER_STREAM (streams->data);
        MateMixerDirection direction;

        direction = mate_mixer_stream_get_direction (stream);

        if (direction == MATE_MIXER_DIRECTION_INPUT)
            inputs++;
        else if (direction == MATE_MIXER_DIRECTION_OUTPUT)
            outputs++;

        streams = streams->next;
    }

    if (inputs == 0 && outputs == 0) {
        /* translators:
                 * The device has been disabled */
        return g_strdup (_("Disabled"));
    }

    if (outputs > 0) {
        /* translators:
                 * The number of sound outputs on a particular device */
        outputs_str = g_strdup_printf (ngettext ("%u Output",
                                                 "%u Outputs",
                                                 outputs),
                                       outputs);
    }

    if (inputs > 0) {
        /* translators:
                 * The number of sound inputs on a particular device */
        inputs_str = g_strdup_printf (ngettext ("%u Input",
                                                "%u Inputs",
                                                inputs),
                                      inputs);
    }

    if (inputs_str != NULL && outputs_str != NULL) {
        gchar *ret = g_strdup_printf ("%s / %s",
                                      outputs_str,
                                      inputs_str);
        g_free (outputs_str);
        g_free (inputs_str);
        return ret;
    }

    if (inputs_str != NULL)
        return inputs_str;

    return outputs_str;
}

void UkmediaMainWidget::updateProfileOption()
{
    int index = m_pOutputWidget->m_pSelectCombobox->currentIndex();
    if (index < 0)
        return;
    QString deviceStr = m_pDeviceNameList->at(index);
    QByteArray ba = deviceStr.toLatin1();
    const gchar *deviceName = ba.data();
    const gchar *profileLabel = nullptr;
    const gchar *profileName = nullptr;
    const gchar *setProfileLabel = nullptr;
    MateMixerSwitchOption *activeOption;
    MateMixerDevice *pDevice = mate_mixer_context_get_device(m_pContext,deviceName);
    const GList *switches;
    switches = mate_mixer_device_list_switches (MATE_MIXER_DEVICE(pDevice));
    m_pOutputWidget->m_pProfileCombobox->clear();
    m_pProfileNameList->clear();
    while (switches != nullptr) {
        MateMixerDeviceSwitch *swtch = MATE_MIXER_DEVICE_SWITCH (switches->data);
        const GList *options;
        options = mate_mixer_switch_list_options ( MATE_MIXER_SWITCH(swtch));
        activeOption = mate_mixer_switch_get_active_option(MATE_MIXER_SWITCH(swtch));
        setProfileLabel  = mate_mixer_switch_option_get_label(activeOption) ;

        while (options != NULL) {
            MateMixerSwitchOption *option = MATE_MIXER_SWITCH_OPTION (options->data);
            profileLabel = mate_mixer_switch_option_get_label (option);
            profileName = mate_mixer_switch_option_get_name(option);
            qDebug() <<"???????????????????????? ============ :" <<profileLabel;
            m_pProfileNameList->append(profileName);
            m_pOutputWidget->m_pProfileCombobox->addItem(profileLabel);
            /* Select the currently active option of the switch */
            options = options->next;
        }
        switches = switches->next;
    }
//    if (setProfileLabel != nullptr)
//        m_pOutputWidget->m_pProfileCombobox->setCurrentText(setProfileLabel);
}

void UkmediaMainWidget::updateDeviceInfo (UkmediaMainWidget *w, MateMixerDevice *device)
{
    const gchar     *label;
    const gchar     *profileLabel = NULL;
    gchar           *status;
    MateMixerSwitch *profileSwitch;

    label = mate_mixer_device_get_label (device);
    profileSwitch = findDeviceProfileSwitch (w,device);
//    w->m_pSwitch = profileSwitch;
    if (profileSwitch != NULL) {
        MateMixerSwitchOption *active;

        active = mate_mixer_switch_get_active_option (profileSwitch);
        if (G_LIKELY (active != NULL))
            profileLabel = mate_mixer_switch_option_get_label (active);

        qDebug() << "update device info ,??????combobox profile:" << profileLabel;
        w->m_pOutputWidget->m_pProfileCombobox->setCurrentText(profileLabel);
    }

    status = deviceStatus (device);
    g_free (status);
}

void UkmediaMainWidget::onSwitchActiveOptionNotify (MateMixerSwitch *swtch,GParamSpec *pspec,UkmediaMainWidget *w)
{
    MateMixerSwitchOption *action = mate_mixer_switch_get_active_option(swtch);
    mate_mixer_switch_option_get_label(action);
    const gchar *outputPortLabel = mate_mixer_switch_option_get_label(action);
    w->m_pOutputWidget->m_pOutputPortCombobox->setCurrentText(outputPortLabel);
}

void UkmediaMainWidget::onDeviceProfileActiveOptionNotify (MateMixerDeviceSwitch *swtch,GParamSpec *pspec,UkmediaMainWidget *w)
{
    MateMixerDevice *device;
    device = mate_mixer_device_switch_get_device (swtch);
    updateDeviceInfo (w, device);
}

void UkmediaMainWidget::setConnectingMessage(const char *string) {
    QByteArray markup = "<i>";
    if (!string)
        markup += tr("Establishing connection to PulseAudio. Please wait...").toUtf8().constData();
    else
        markup += string;
    markup += "</i>";
}

gboolean UkmediaMainWidget::connect_to_pulse(gpointer userdata)
{
    UkmediaMainWidget *w = static_cast<UkmediaMainWidget*>(userdata);

    pa_proplist *proplist = pa_proplist_new();
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, QObject::tr("PulseAudio Volume Control").toUtf8().constData());
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, "org.PulseAudio.pavucontrol");
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_ICON_NAME, "audio-card");
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_VERSION, "PACKAGE_VERSION");

    context = pa_context_new_with_proplist(api, nullptr, proplist);
    g_assert(context);

    pa_proplist_free(proplist);

    pa_context_set_state_callback(context, context_state_callback, w);
    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFAIL, nullptr) < 0) {
        if (pa_context_errno(context) == PA_ERR_INVALID) {
            w->setConnectingMessage(QObject::tr("Connection to PulseAudio failed. Automatic retry in 5s\n\n"
                "In this case this is likely because PULSE_SERVER in the Environment/X11 Root Window Properties\n"
                "or default-server in client.conf is misconfigured.\n"
                "This situation can also arrise when PulseAudio crashed and left stale details in the X11 Root Window.\n"
                "If this is the case, then PulseAudio should autospawn again, or if this is not configured you should\n"
                "run start-pulseaudio-x11 manually.").toUtf8().constData());
        }
    }

    return false;
}

void UkmediaMainWidget::createEventRole()
{
    pa_channel_map cm = {
        1, { PA_CHANNEL_POSITION_MONO }
    };
    channelMap = cm;
    executeVolumeUpdate(false);
}

void UkmediaMainWidget::context_state_callback(pa_context *c, void *userdata) {
    UkmediaMainWidget *w = static_cast<UkmediaMainWidget*>(userdata);
    g_assert(c);

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY: {
            pa_operation *o;

            /* Create event widget immediately so it's first in the list */
            w->createEventRole();
            if (!(o = pa_context_subscribe(c, (pa_subscription_mask_t)
                                           (PA_SUBSCRIPTION_MASK_SINK|
                                            PA_SUBSCRIPTION_MASK_SOURCE|
                                            PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                            PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT|
                                            PA_SUBSCRIPTION_MASK_CLIENT|
                                            PA_SUBSCRIPTION_MASK_SERVER|
                                            PA_SUBSCRIPTION_MASK_CARD), nullptr, nullptr))) {
                w->show_error(QObject::tr("pa_context_subscribe() failed").toUtf8().constData());
                return;
            }
            pa_operation_unref(o);

            /* These calls are not always supported */
            if ((o = pa_ext_stream_restore_read(c, ext_stream_restore_read_cb, w))) {
                pa_operation_unref(o);

                if ((o = pa_ext_stream_restore_subscribe(c, 1, nullptr, nullptr)))
                    pa_operation_unref(o);

            } else
                g_debug(QObject::tr("Failed to initialize stream_restore extension: %s").toUtf8().constData(), pa_strerror(pa_context_errno(w->context)));
            break;
        }
        case PA_CONTEXT_TERMINATED:
        default:
//            qApp->quit();
            return;
    }
}

void UkmediaMainWidget::ext_stream_restore_subscribe_cb(pa_context *c, void *userdata)
{
    UkmediaMainWidget *w = static_cast<UkmediaMainWidget*>(userdata);
    pa_operation *o;
    if (!(o = pa_ext_stream_restore_read(c, w->ext_stream_restore_read_cb, w))) {
        w->show_error(QObject::tr("pa_ext_stream_restore_read() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void UkmediaMainWidget::ext_stream_restore_read_cb(pa_context *,const pa_ext_stream_restore_info *i,int eol,void *userdata)
{
    UkmediaMainWidget *w = static_cast<UkmediaMainWidget*>(userdata);

    if (eol < 0) {
//        w->deleteEventRoleWidget();
        return;
    }

    if (eol > 0) {
        qDebug() << "Failed to initialize stream_restore extension";
        return;
    }

    w->updateRole(*i);
}

void UkmediaMainWidget::executeVolumeUpdate(bool isMuted)
{
    info.name = role;
    info.channel_map.channels = 1;
    info.channel_map.map[0] = PA_CHANNEL_POSITION_MONO;
    volume.channels = 1;
    volume.values[0] = m_pSoundWidget->m_pAlertSlider->value()*65536/100;
    info.volume = volume;
    qDebug() <<"executeVolumeUpdate"  << m_pSoundWidget->m_pAlertSlider->value();
    info.device = device == "" ? nullptr : device.constData();
    info.mute = isMuted;

    pa_operation* o;
    if (!(o = pa_ext_stream_restore_write(get_context(), PA_UPDATE_REPLACE, &info, 1, true, nullptr, nullptr))) {
        show_error(tr("pa_ext_stream_restore_write() failed").toUtf8().constData());
        return;
    }
    pa_operation_unref(o);
}



void UkmediaMainWidget::show_error(const char *txt) {
    char buf[256];

    snprintf(buf, sizeof(buf), "%s: %s", txt, pa_strerror(pa_context_errno(context)));
    qDebug() << "show error:" << QString::fromUtf8(buf);
//    QMessageBox::critical(nullptr, QObject::tr("Error"), QString::fromUtf8(buf));
//    qApp->quit();
//    QApplication::quit();
}

pa_context* UkmediaMainWidget::get_context()
{
    return context;
}

void UkmediaMainWidget::updateRole(const pa_ext_stream_restore_info &info)
{
    if (strcmp(info.name, "sink-input-by-media-role:event") != 0)
        return;
    createEventRole();
}

UkmediaMainWidget::~UkmediaMainWidget()
{
//    delete player;
}
