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
#include "ukmedia_input_widget.h"
#include <QDebug>
#include <QFile>
#include <QApplication>

UkmediaInputWidget::UkmediaInputWidget(QWidget *parent) : QWidget(parent)
{
    m_pInputWidget = new QWidget(this);
    m_pInputDeviceWidget = new QFrame(m_pInputWidget);
    m_pVolumeWidget = new QFrame(m_pInputWidget);
    m_pInputLevelWidget = new QFrame(m_pInputWidget);
    m_pInputPortWidget = new QFrame(m_pInputWidget);
    m_pselectWidget = new QFrame(m_pInputWidget);

    m_pInputDeviceWidget->setFrameShape(QFrame::Shape::Box);
    m_pVolumeWidget->setFrameShape(QFrame::Shape::Box);
    m_pInputLevelWidget->setFrameShape(QFrame::Shape::Box);
    m_pInputPortWidget->setFrameShape(QFrame::Shape::Box);
    m_pselectWidget->setFrameShape(QFrame::Shape::Box);
    //设置大小
    m_pInputWidget->setMinimumSize(550,152);
    m_pInputWidget->setMaximumSize(960,152);
    m_pInputDeviceWidget->setMinimumSize(550,50);
    m_pInputDeviceWidget->setMaximumSize(960,50);
    m_pVolumeWidget->setMinimumSize(550,50);
    m_pVolumeWidget->setMaximumSize(960,50);
    m_pInputLevelWidget->setMinimumSize(550,50);
    m_pInputLevelWidget->setMaximumSize(960,50);
    m_pInputPortWidget->setMinimumSize(550,50);
    m_pInputPortWidget->setMaximumSize(960,50);
    m_pselectWidget->setMinimumSize(550,50);
    m_pselectWidget->setMaximumSize(960,50);

    m_pInputLabel = new QLabel(tr("Input"),this);
    m_pInputLabel->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");
    m_pInputDeviceLabel = new QLabel(tr("Input Device"),m_pInputWidget);
    m_pInputDeviceCombobox = new QComboBox(m_pInputDeviceWidget);

    m_pIpVolumeLabel = new QLabel(tr("Volume"),m_pVolumeWidget);
    m_pInputIconBtn = new UkuiButtonDrawSvg(m_pVolumeWidget);
    m_pIpVolumeSlider = new AudioSlider(m_pVolumeWidget);
    m_pIpVolumePercentLabel = new QLabel(m_pVolumeWidget);
    m_pInputLevelLabel = new QLabel(tr("Input Level"),m_pInputLevelWidget);
    m_pLowLevelLabel = new QLabel(tr("Low"),m_pInputLevelWidget);
    m_pInputLevelSlider = new AudioSlider(m_pInputLevelWidget);
    m_pHighLevelLabel = new QLabel(tr("High"),m_pInputLevelWidget);

    m_pInputPortCombobox = new QComboBox(m_pInputPortWidget);
    m_pInputPortLabel = new QLabel(tr("Connector"),m_pInputPortWidget);

    m_pSelectDeviceLabel = new QLabel(tr("Select Device"),m_pselectWidget);
    m_pSelectCombobox = new QComboBox(m_pselectWidget);

    m_pIpVolumeSlider->setOrientation(Qt::Horizontal);
    m_pInputLevelSlider->setOrientation(Qt::Horizontal);
    m_pIpVolumeSlider->setRange(0,100);
    m_pInputIconBtn->setFocusPolicy(Qt::NoFocus);
    //输入设备添加布局
    QHBoxLayout *m_pInputDeviceLayout = new QHBoxLayout(m_pInputDeviceWidget);
    m_pInputLabel->setFixedSize(83,24);
    m_pInputDeviceCombobox->setMinimumSize(50,32);
    m_pInputDeviceCombobox->setMaximumSize(900,32);
    m_pInputDeviceLabel->setFixedSize(115,24);
    m_pInputDeviceLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pInputDeviceLayout->addWidget(m_pInputDeviceLabel);
    m_pInputDeviceLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pInputDeviceLayout->addWidget(m_pInputDeviceCombobox);
    m_pInputDeviceLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pInputDeviceLayout->setSpacing(0);
    m_pInputDeviceWidget->setLayout(m_pInputDeviceLayout);
    m_pInputDeviceLayout->layout()->setContentsMargins(0,0,0,0);
    //主音量添加布局
    QHBoxLayout *m_pMasterLayout = new QHBoxLayout(m_pVolumeWidget);
    m_pIpVolumeLabel->setFixedSize(115,24);
    m_pInputIconBtn->setFixedSize(24,24);
    m_pIpVolumeSlider->setFixedHeight(20);
    m_pIpVolumePercentLabel->setFixedSize(40,24);
    m_pMasterLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pMasterLayout->addWidget(m_pIpVolumeLabel);
    m_pMasterLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pMasterLayout->addWidget(m_pInputIconBtn);
    m_pMasterLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pMasterLayout->addWidget(m_pIpVolumeSlider);
    m_pMasterLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pMasterLayout->addWidget(m_pIpVolumePercentLabel);
    m_pMasterLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pMasterLayout->setSpacing(0);
    m_pVolumeWidget->setLayout(m_pMasterLayout);
    m_pVolumeWidget->layout()->setContentsMargins(0,0,0,0);
    //声道平衡添加布局
    QHBoxLayout *m_pSoundLayout = new QHBoxLayout(m_pInputLevelWidget);
    m_pInputLevelLabel->setFixedSize(115,24);
    m_pLowLevelLabel->setFixedSize(24,24);
    m_pInputLevelSlider->setFixedHeight(20);
    m_pHighLevelLabel->setFixedSize(36,24);
    m_pSoundLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pSoundLayout->addWidget(m_pInputLevelLabel);
    m_pSoundLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pSoundLayout->addWidget(m_pLowLevelLabel);
    m_pSoundLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pSoundLayout->addWidget(m_pInputLevelSlider);
    m_pSoundLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pSoundLayout->addWidget(m_pHighLevelLabel);
    m_pSoundLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pSoundLayout->setSpacing(0);
    m_pInputLevelWidget->setLayout(m_pSoundLayout);
    m_pInputLevelWidget->layout()->setContentsMargins(0,0,0,0);

    //连接器添加布局
    QHBoxLayout *pConnectLayout = new QHBoxLayout(m_pInputLevelWidget);
    m_pInputPortLabel->setFixedSize(115,24);
    m_pInputPortCombobox->setMinimumSize(50,32);
    m_pInputPortCombobox->setMaximumSize(900,32);
    pConnectLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    pConnectLayout->addWidget(m_pInputPortLabel);
    pConnectLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    pConnectLayout->addWidget(m_pInputPortCombobox);
    pConnectLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    pConnectLayout->setSpacing(0);
    m_pInputPortWidget->setLayout(pConnectLayout);
    pConnectLayout->layout()->setContentsMargins(0,0,0,0);

    //选择的设备布局
    QHBoxLayout *pSelectDeviceLayout = new QHBoxLayout(m_pInputLevelWidget);
    m_pSelectDeviceLabel->setFixedSize(115,24);
    m_pSelectCombobox->setMinimumSize(50,32);
    m_pSelectCombobox->setMaximumSize(900,32);
    pSelectDeviceLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    pSelectDeviceLayout->addWidget(m_pSelectDeviceLabel);
    pSelectDeviceLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    pSelectDeviceLayout->addWidget(m_pSelectCombobox);
    pSelectDeviceLayout->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    pSelectDeviceLayout->setSpacing(0);
    m_pselectWidget->setLayout(pSelectDeviceLayout);
    pSelectDeviceLayout->layout()->setContentsMargins(0,0,0,0);
    m_pselectWidget->setVisible(false);
    //进行整体布局
    m_pVlayout = new QVBoxLayout(m_pInputWidget);
    m_pVlayout->addWidget(m_pInputDeviceWidget);
    m_pVlayout->addWidget(m_pVolumeWidget);
    m_pVlayout->addWidget(m_pInputLevelWidget);
//    m_pVlayout->addWidget(m_pselectWidget);
    m_pVlayout->setSpacing(1);
    m_pInputWidget->setLayout(m_pVlayout);
    m_pInputWidget->layout()->setContentsMargins(0,0,0,0);
//    inputWidgetAddPort();
    m_pInputPortWidget->hide();
    QVBoxLayout *m_pVlayout1 = new QVBoxLayout(this);
    m_pVlayout1->addWidget(m_pInputLabel);
    m_pVlayout1->addItem(new QSpacerItem(16,20,QSizePolicy::Fixed));
    m_pVlayout1->addWidget(m_pInputWidget);
    this->setLayout(m_pVlayout1);
    this->layout()->setContentsMargins(0,0,0,0);

    //设置样式
    m_pIpVolumeSlider->setStyleSheet("QSlider::groove:horizontal {"
                                  "border: 0px solid #bbb; }"
                                  "QSlider::sub-page:horizontal {"
                                  "background: #3D6BE5;border-radius: 2px;"
                                  "margin-top:8px;margin-bottom:9px;}"
                                  "QSlider::add-page:horizontal {"
                                  "background:  rgba(52,70,80,90%);"
                                  "border: 0px solid #777;"
                                  "border-radius: 2px;"
                                  "margin-top:8px;"
                                  "margin-bottom:9px;}"
                                  "QSlider::handle:horizontal {"
                                  "width: 20px;"
                                  "height: 20px;"
                                  "background: rgb(61,107,229);"
                                  "border-radius:10px;}");
    m_pInputLevelSlider->setStyleSheet("QSlider::groove:horizontal {"
                                   "border: 0px solid #bbb; }"
                                   "QSlider::sub-page:horizontal {"
                                   "background: #3D6BE5;border-radius: 2px;"
                                   "margin-top:8px;margin-bottom:9px;}"
                                   "QSlider::add-page:horizontal {"
                                   "background:  rgba(52,70,80,90%);"
                                   "border: 0px solid #777;"
                                   "border-radius: 2px;"
                                   "margin-top:8px;"
                                   "margin-bottom:9px;}"
                                   "QSlider::handle:horizontal {"
                                   "width: 20px;"
                                   "height: 20px;"
                                   "background: rgb(61,107,229);"
                                   "border-radius:10px;}");


}

void UkmediaInputWidget::inputWidgetAddPort()
{
    m_pInputWidget->setMinimumSize(550,203);
    m_pInputWidget->setMaximumSize(960,203);
//    m_pVlayout->addSpacing(1);
    m_pVlayout->addWidget(m_pInputPortWidget);
    m_pInputPortWidget->show();
}

void UkmediaInputWidget::inputWidgetRemovePort()
{
//    m_pVlayout->addSpacing(1);
    m_pInputWidget->setMinimumSize(550,152);
    m_pInputWidget->setMaximumSize(960,152);
    m_pVlayout->removeWidget(m_pInputPortWidget);
    m_pInputPortWidget->hide();
}

UkmediaInputWidget::~UkmediaInputWidget()
{

}
