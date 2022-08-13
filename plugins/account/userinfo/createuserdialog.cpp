#include "createuserdialog.h"
#include "ui_createuserdialog.h"

#include <QDebug>

#define MOBILE 0
#define PC 1
#define PWD_LOW_LENGTH 6
#define PWD_HIGH_LENGTH 20
#define PIN_LENGTH 6
#define USER_LENGTH 32

CreateUserDialog::CreateUserDialog(QStringList userlist, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateUserDialog),
    ostype(PC),
    usersStringList(userlist)
{
    ui->setupUi(this);

    autologinSwitchBtn = new SwitchButton;
    autologinSwitchBtn->setAttribute(Qt::WA_DeleteOnClose);
    ui->autologinVLayout->addWidget(autologinSwitchBtn);

    //
    ui->pwdLineEdit->setEchoMode(QLineEdit::Password);
    ui->pwdsureLineEdit->setEchoMode(QLineEdit::Password);
    ui->pinLineEdit->setEchoMode(QLineEdit::Password);
    ui->pinsureLineEdit->setEchoMode(QLineEdit::Password);

    QString required = tr("(Required)");
    QString optional = tr("(Optional)");
    if (ostype == PC){
        ui->pwdLabel->setText(required);
        ui->pinLabel->setText(optional);
    }
    else{
        ui->pwdLabel->setText(optional);
        ui->pinLabel->setText(required);
    }

    ui->usernameLineEdit->setPlaceholderText(tr("UserName"));
    ui->pwdLineEdit->setPlaceholderText(tr("Password"));
    ui->pwdsureLineEdit->setPlaceholderText(tr("Password Identify"));
    ui->pinLineEdit->setPlaceholderText(tr("PIN Code"));
    ui->pinsureLineEdit->setPlaceholderText(tr("PIN Code Identify"));

    //给radiobtn设置id，id即accoutnType，方便直接返回id值
    ui->buttonGroup->setId(ui->standardRadioBtn, 0);
    ui->buttonGroup->setId(ui->adminRadioBtn, 1);

    //默认标准用户
    ui->standardRadioBtn->setChecked(true);

    //tiplabel hide
    ui->usernametipLabel->hide();
    ui->pwdtipLabel->hide();
    ui->pwdsuretipLabel->hide();
    ui->pintipLabel->hide();
    ui->pinsuretipLabel->hide();

    confirm_btn_status_refresh();

    connect(ui->cancelPushBtn, SIGNAL(clicked(bool)), this, SLOT(reject()));
    connect(ui->confirmPushBtn, SIGNAL(clicked(bool)), this, SLOT(confirm_btn_clicked_slot()));
    connect(ui->usernameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(name_legality_check_slot(QString)));
    connect(ui->pwdLineEdit, SIGNAL(textChanged(QString)), this, SLOT(pwd_legality_check_slot(QString)));
    connect(ui->pwdsureLineEdit, SIGNAL(textChanged(QString)), this, SLOT(pwdsure_legality_check_slot(QString)));

}

CreateUserDialog::~CreateUserDialog()
{
    delete ui;
//    delete process;
}

void CreateUserDialog::set_face_label(QString iconfile){
    ui->faceLabel->setPixmap(QPixmap(iconfile).scaled(QSize(80, 80)));
}

void CreateUserDialog::tiplabel_status_refresh(){
    if (ui->usernametipLabel->text().isEmpty())
        ui->usernametipLabel->hide();
    else
        ui->usernametipLabel->show();

    if (ui->pwdtipLabel->text().isEmpty())
        ui->pwdtipLabel->hide();
    else
        ui->pwdtipLabel->show();

    if (ui->pwdsuretipLabel->text().isEmpty())
        ui->pwdsuretipLabel->hide();
    else
        ui->pwdsuretipLabel->show();

    if (ui->pintipLabel->text().isEmpty())
        ui->pintipLabel->hide();
    else
        ui->pintipLabel->show();

    if (ui->pinsuretipLabel->text().isEmpty())
        ui->pinsuretipLabel->hide();
    else
        ui->pinsuretipLabel->show();
}

void CreateUserDialog::confirm_btn_status_refresh(){
    if (ostype == PC){
        if (ui->usernameLineEdit->text().isEmpty() || !ui->usernametipLabel->text().isEmpty() || \
                ui->pwdLineEdit->text().isEmpty() || !ui->pwdtipLabel->text().isEmpty() || \
                ui->pwdsureLineEdit->text().isEmpty() || !ui->pwdsuretipLabel->text().isEmpty())
            ui->confirmPushBtn->setEnabled(false);
        else
            ui->confirmPushBtn->setEnabled(true);
    }
    else{
        if (ui->usernameLineEdit->text().isEmpty() || !ui->usernametipLabel->text().isEmpty() || \
                ui->pinLineEdit->text().isEmpty() || !ui->pintipLabel->text().isEmpty() || \
                ui->pinsureLineEdit->text().isEmpty() || !ui->pinsuretipLabel->text().isEmpty())
            ui->confirmPushBtn->setEnabled(false);
        else
            ui->confirmPushBtn->setEnabled(true);
    }
}

void CreateUserDialog::pinsure_legality_check_slot(QString pinsure){
    if (pinsure != ui->pinLineEdit->text())
        ui->pinsuretipLabel->setText(tr("Inconsistency with pin code"));
    else
        ui->pinsuretipLabel->setText("");

    tiplabel_status_refresh();
    confirm_btn_status_refresh();
}

void CreateUserDialog::pin_legality_check_slot(QString pin){
    if (pin.length() != PIN_LENGTH)
        ui->pintipLabel->setText(tr("Pin code length needs to %1 character!").arg(PIN_LENGTH));
    else
        ui->pintipLabel->setText("");

    //防止先输入确认密码，再输入密码后pinsuretipLabel无法刷新
    if (ui->pinLineEdit->text() == ui->pinsureLineEdit->text())
        ui->pinsuretipLabel->setText("");

    tiplabel_status_refresh();
    confirm_btn_status_refresh();
}

void CreateUserDialog::pwdsure_legality_check_slot(QString pwdsure){
    if (pwdsure != ui->pwdLineEdit->text())
        ui->pwdsuretipLabel->setText(tr("Inconsistency with pwd"));
    else
        ui->pwdsuretipLabel->setText("");

    tiplabel_status_refresh();
    confirm_btn_status_refresh();
}

void CreateUserDialog::pwd_legality_check_slot(QString pwd){
    if (pwd.length() < PWD_LOW_LENGTH)
        ui->pwdtipLabel->setText(tr("Password length needs to more than %1 character!").arg(PWD_LOW_LENGTH - 1));
    else if (pwd.length() > PWD_HIGH_LENGTH)
        ui->pwdtipLabel->setText(tr("Password length needs to less than %1 character!").arg(PWD_HIGH_LENGTH + 1));
    else
        ui->pwdtipLabel->setText("");

    //防止先输入确认密码，再输入密码后pwdsuretipLabel无法刷新
    if (ui->pwdLineEdit->text() == ui->pwdsureLineEdit->text())
        ui->pwdsuretipLabel->setText("");

    tiplabel_status_refresh();
    confirm_btn_status_refresh();
}

void CreateUserDialog::name_legality_check_slot(QString username){
    if (username.isEmpty())
        ui->usernametipLabel->setText(tr("The user name cannot be empty"));
    else if (username.startsWith("_") || username.left(1).contains((QRegExp("[0-9]")))){
        ui->usernametipLabel->setText(tr("The first character must be lowercase letters!"));
    }
    else if (username.contains(QRegExp("[A-Z]"))){
        ui->usernametipLabel->setText(tr("User name can not contain capital letters!"));
    }
    else if (username.contains(QRegExp("[a-z]")) || username.contains(QRegExp("[0-9]")) || username.contains("_"))
        if (username.length() > 0 && username.length() < USER_LENGTH){
            back = false;
            QString cmd = QString("getent group %1").arg(username);
            process = new QProcess(this);
            connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(name_conflict_group_slot()));
            process->start(cmd);

            if (usersStringList.contains(username)){
                ui->usernametipLabel->setText(tr("The user name is already in use, please use a different one."));
            }
            else
                ui->usernametipLabel->setText("");
        }
        else
            ui->usernametipLabel->setText(tr("User name length need to less than %1 letters!").arg(USER_LENGTH));
    else
        ui->usernametipLabel->setText(tr("The user name can only be composed of letters, numbers and underline!"));

    tiplabel_status_refresh();
    confirm_btn_status_refresh();
}

void CreateUserDialog::name_conflict_group_slot(){
    QString output = QString(process->readAllStandardOutput().data());
    ui->usernametipLabel->setText(tr("The user name corresponds to the group already exists,please use a different user name!"));
}

void CreateUserDialog::confirm_btn_clicked_slot(){
    this->accept();
    emit user_info_send(ui->usernameLineEdit->text(), ui->pwdLineEdit->text(), ui->pinLineEdit->text(), ui->buttonGroup->checkedId(), autologinSwitchBtn->isChecked());
}
