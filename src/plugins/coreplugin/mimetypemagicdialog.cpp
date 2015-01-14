/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "mimetypemagicdialog.h"
#include "mimedatabase.h"

#include <utils/headerviewstretcher.h>

#include <QMessageBox>

using namespace Core;
using namespace Internal;

MimeTypeMagicDialog::MimeTypeMagicDialog(QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);
    setWindowTitle(tr("Add Magic Header"));
    connect(ui.useRecommendedGroupBox, SIGNAL(clicked(bool)),
            this, SLOT(applyRecommended(bool)));
    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(validateAccept()));
}

void MimeTypeMagicDialog::applyRecommended(bool checked)
{
    if (checked) {
        ui.startRangeSpinBox->setValue(0);
        ui.endRangeSpinBox->setValue(0);
        ui.prioritySpinBox->setValue(50);
    }
}

void MimeTypeMagicDialog::validateAccept()
{
    if (ui.valueLineEdit->text().isEmpty()
            || (ui.byteRadioButton->isChecked()
                && !Core::MagicByteRule::validateByteSequence(ui.valueLineEdit->text()))) {
        QMessageBox::critical(0, tr("Error"), tr("Not a valid byte pattern."));
        return;
    }
    accept();
}

void MimeTypeMagicDialog::setMagicData(const MagicData &data)
{
    ui.valueLineEdit->setText(data.m_value);
    if (data.m_type == Core::MagicStringRule::kMatchType)
        ui.stringRadioButton->setChecked(true);
    else
        ui.byteRadioButton->setChecked(true);
    ui.startRangeSpinBox->setValue(data.m_start);
    ui.endRangeSpinBox->setValue(data.m_end);
    ui.prioritySpinBox->setValue(data.m_priority);
}

MagicData MimeTypeMagicDialog::magicData() const
{
    MagicData data;
    data.m_value = ui.valueLineEdit->text();
    if (ui.stringRadioButton->isChecked())
        data.m_type = Core::MagicStringRule::kMatchType;
    else
        data.m_type = Core::MagicByteRule::kMatchType;
    data.m_start = ui.startRangeSpinBox->value();
    data.m_end = ui.endRangeSpinBox->value();
    data.m_priority = ui.prioritySpinBox->value();
    return data;
}
