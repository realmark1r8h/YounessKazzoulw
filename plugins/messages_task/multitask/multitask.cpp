#include "multitask.h"
#include "ui_multitask.h"

Multitask::Multitask()
{
    ui = new Ui::Multitask;
    pluginWidget = new QWidget;
    pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(pluginWidget);

    pluginName = tr("multitask");
    pluginType = MESSAGES_TASK;
}

Multitask::~Multitask()
{
    delete ui;
}

QString Multitask::get_plugin_name(){
    return pluginName;
}

int Multitask::get_plugin_type(){
    return pluginType;
}

QWidget * Multitask::get_plugin_ui(){
    return pluginWidget;
}
