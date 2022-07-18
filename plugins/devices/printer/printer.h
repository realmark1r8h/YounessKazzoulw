#ifndef PRINTER_H
#define PRINTER_H


#include <QObject>
#include <QtPlugin>
#include "mainui/interface.h"

#include <QWidget>
#include <QLabel>
#include <QtPrintSupport/QPrinterInfo>
#include <QProcess>

namespace Ui {
class Printer;
}

class Printer : public QObject, CommonInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kycc.CommonInterface")
    Q_INTERFACES(CommonInterface)

public:
    Printer();
    QString get_plugin_name() Q_DECL_OVERRIDE;
    int get_plugin_type() Q_DECL_OVERRIDE;
    QWidget * get_plugin_ui() Q_DECL_OVERRIDE;

    ~Printer();

private:
    Ui::Printer *ui;
    QString pluginName;
    int pluginType;
    QWidget * pluginWidget;

    QLabel * label;

private slots:
    void run_external_app_slot();
};

#endif // PRINTER_H
