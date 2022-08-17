#ifndef KEYBOARDCONTROL_H
#define KEYBOARDCONTROL_H

#include <QObject>
#include <QtPlugin>
#include "mainui/interface.h"

#include <QWidget>

#include "kylin-keyboard-interface.h"
#include "kylin-interface-interface.h"
#include <QGSettings/QGSettings>

#include "../../pluginsComponent/switchbutton.h"
#include "../../pluginsComponent/customwidget.h"

//#include "keyboardlayout.h"
#include "kbdlayoutmanager.h"
#include "customlineedit.h"
#include "keymap.h"

/* qt会将glib里的signals成员识别为宏，所以取消该宏
 * 后面如果用到signals时，使用Q_SIGNALS代替即可
 **/
#ifdef signals
#undef signals
#endif

extern "C" {
#include <glib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
}

namespace Ui {
class KeyboardControl;
}

enum KeyMapping{

};

class KeyboardControl : public QObject, CommonInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kycc.CommonInterface")
    Q_INTERFACES(CommonInterface)

public:
    KeyboardControl();
    ~KeyboardControl() Q_DECL_OVERRIDE;

    QString get_plugin_name() Q_DECL_OVERRIDE;
    int get_plugin_type() Q_DECL_OVERRIDE;
    CustomWidget *get_plugin_ui() Q_DECL_OVERRIDE;
    void plugin_delay_control() Q_DECL_OVERRIDE;

    void component_init();
    void status_init();

    void rebuild_layouts_component();

    void append_keys_from_desktop();
    void append_keys_from_system();
    void append_keys_from_custom();
    bool key_is_forbidden(QString key);

    QString binding_from_string(QString keyString);
    QString binding_name(QList<int> shortcutList);

    QStringList forbidden_keys;

private:
    Ui::KeyboardControl *ui;

    QString pluginName;
    int pluginType;
    CustomWidget * pluginWidget;

    QGSettings * kbdsettings;
    QGSettings * desktopsettings;
    QGSettings * syskeysettings;

    SwitchButton * keySwitchBtn;
    SwitchButton * cursorSwitchBtn;

//    KeyboardLayout * kbl;
    KbdLayoutManager * layoutmanagerObj;

    QStringList desktopshortcut;

    KeyMap * keymapobj;

private slots:
//    void activate_key_repeat_changed_slot(bool status);
    void layout_combobox_changed_slot(int index);
    void receive_shortcut_slot(QList<int> shortcutList);
};

#endif // KEYBOARDCONTROL_H
