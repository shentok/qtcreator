/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "androidrunconfiguration.h"

#include "androidconstants.h"
#include "androidglobal.h"
#include "androidtoolchain.h"
#include "androidmanager.h"
#include "adbcommandswidget.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QWidget>

using namespace Android::Internal;
using namespace ProjectExplorer;
using namespace Utils;

namespace Android {

BaseStringListAspect::BaseStringListAspect(ProjectExplorer::ProjectConfiguration *parent,
                                           const QString &settingsKey,
                                           Core::Id id)
    : ProjectExplorer::ProjectConfigurationAspect(parent)
{
    setSettingsKey(settingsKey);
    setId(id);
}

BaseStringListAspect::~BaseStringListAspect() = default;

void BaseStringListAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!m_widget);
    m_widget = new AdbCommandsWidget;
    m_widget->setCommandList(m_value);
    m_widget->setTitleText(m_label);
    builder.addItem(m_widget.data());
    connect(m_widget.data(), &AdbCommandsWidget::commandsChanged, this, [this] {
        m_value = m_widget->commandsList();
        emit changed();
    });
}

void BaseStringListAspect::fromMap(const QVariantMap &map)
{
    m_value = map.value(settingsKey()).toStringList();
}

void BaseStringListAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), m_value);
}

QStringList BaseStringListAspect::value() const
{
    return m_value;
}

void BaseStringListAspect::setValue(const QStringList &value)
{
    m_value = value;
    if (m_widget)
        m_widget->setCommandList(m_value);
}

void BaseStringListAspect::setLabel(const QString &label)
{
    m_label = label;
}


AndroidRunConfiguration::AndroidRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
    , m_envAspect(this)
    , m_argumentsAspect(this)
    , m_amStartArgsAspect(this)
    , m_warning(this)
    , m_preStartShellCmdAspect(this)
    , m_postStartShellCmdAspect(this)
{
    m_envAspect.addSupportedBaseEnvironment(tr("Clean Environment"), {});

    m_amStartArgsAspect.setId(Constants::ANDROID_AMSTARTARGS);
    m_amStartArgsAspect.setSettingsKey("Android.AmStartArgsKey");
    m_amStartArgsAspect.setLabelText(tr("Activity manager start options:"));
    m_amStartArgsAspect.setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_amStartArgsAspect.setHistoryCompleter("Android.AmStartArgs.History");

    m_warning.setLabelPixmap(Icons::WARNING.pixmap());
    m_warning.setValue(tr("If the \"am start\" options conflict, the application might not start."));

    m_preStartShellCmdAspect.setId(Constants::ANDROID_PRESTARTSHELLCMDLIST);
    m_preStartShellCmdAspect.setSettingsKey("Android.PreStartShellCmdListKey");
    m_preStartShellCmdAspect.setLabel(tr("Shell commands to run on Android device before application launch."));

    m_postStartShellCmdAspect.setId(Constants::ANDROID_POSTFINISHSHELLCMDLIST);
    m_postStartShellCmdAspect.setSettingsKey("Android.PostStartShellCmdListKey");
    m_postStartShellCmdAspect.setLabel(tr("Shell commands to run on Android device after application quits."));

    setUpdater([this, target] {
        const BuildTargetInfo bti = buildTargetInfo();
        setDisplayName(bti.displayName);
        setDefaultDisplayName(bti.displayName);
        AndroidManager::updateGradleProperties(target, buildKey());
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

} // namespace Android
