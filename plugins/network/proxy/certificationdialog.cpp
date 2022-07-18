#include "certificationdialog.h"
#include "ui_certificationdialog.h"

#include <QDebug>

CertificationDialog::CertificationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CertificationDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Certification"));

    const QByteArray id(HTTP_PROXY_SCHEMA);
    cersettings = new QGSettings(id);

    component_init();
    status_init();
}

CertificationDialog::~CertificationDialog()
{
    delete ui;
    delete cersettings;
}

void CertificationDialog::component_init(){
    ui->pwdLineEdit->setEchoMode(QLineEdit::Password);
}

void CertificationDialog::status_init(){
    //获取认证状态
    bool status = cersettings->get(HTTP_AUTH_KEY).toBool();
    ui->certificationCheckBox->setChecked(status);
    ui->widget->setEnabled(status);

    //获取用户名密码
    QString user = cersettings->get(HTTP_AUTH_USER_KEY).toString();
    ui->userLineEdit->setText(user);
    QString pwd = cersettings->get(HTTP_AUTH_PASSWD_KEY).toString();
    ui->pwdLineEdit->setText(pwd);

    connect(ui->certificationCheckBox, SIGNAL(clicked(bool)), this, SLOT(cerCheckBox_clicked_slot(bool)));
    connect(ui->closePushBtn, SIGNAL(released()), this, SLOT(close()));

    connect(ui->userLineEdit, SIGNAL(textChanged(QString)), this, SLOT(user_edit_changed_slot(QString)));
    connect(ui->pwdLineEdit, SIGNAL(textChanged(QString)), this, SLOT(pwd_edit_changed_slot(QString)));
}

void CertificationDialog::cerCheckBox_clicked_slot(bool status){
    ui->widget->setEnabled(status);
    cersettings->set(HTTP_AUTH_KEY, QVariant(status));
}

void CertificationDialog::user_edit_changed_slot(QString edit){
    cersettings->set(HTTP_AUTH_USER_KEY, QVariant(edit));
}

void CertificationDialog::pwd_edit_changed_slot(QString edit){
    cersettings->set(HTTP_AUTH_PASSWD_KEY, QVariant(edit));
}
