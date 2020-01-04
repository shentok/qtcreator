/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qtbuildaspects.h"

#include "baseqtversion.h"

#include <projectexplorer/buildpropertiessettings.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/kitmanager.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QLabel>
#include <QLayout>

using namespace ProjectExplorer;

namespace QtSupport {

QmlDebuggingAspect::QmlDebuggingAspect(ProjectConfiguration *parent)
    : ProjectExplorer::BaseTriStateAspect(parent)
{
    setSettingsKey("EnableQmlDebugging");
    setDisplayName(tr("QML debugging and profiling:"));
    setSetting(ProjectExplorerPlugin::buildPropertiesSettings().qmlDebugging);
}

void QmlDebuggingAspect::addToLayout(LayoutBuilder &builder)
{
    BaseSelectionAspect::addToLayout(builder);
    const auto warningIconLabel = new QLabel;
    warningIconLabel->setAlignment(Qt::AlignTop);
    warningIconLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    const auto warningTextLabel = new QLabel;
    warningTextLabel->setAlignment(Qt::AlignTop);
    builder.startNewRow().addItems(QString(), warningIconLabel, warningTextLabel);
    const auto changeHandler = [this, warningIconLabel, warningTextLabel] {
        QString warningText;
        const bool supported = m_kit && BaseQtVersion::isQmlDebuggingSupported(m_kit, &warningText);
        if (!supported) {
            setSetting(TriState::Default);
        } else if (setting() == TriState::Enabled) {
            warningText = tr("Might make your application vulnerable.<br/>"
                             "Only use in a safe environment.");
        }
        warningTextLabel->setText(warningText);
        setVisibleDynamic(supported);
        const bool warningLabelsVisible = supported && !warningText.isEmpty();
        warningIconLabel->setVisible(warningLabelsVisible);
        warningTextLabel->setVisible(warningLabelsVisible);
    };
    connect(KitManager::instance(), &KitManager::kitsChanged, builder.layout(), changeHandler);
    connect(this, &QmlDebuggingAspect::changed, builder.layout(), changeHandler);
    changeHandler();
}

QtQuickCompilerAspect::QtQuickCompilerAspect(ProjectExplorer::ProjectConfiguration *parent)
    : ProjectExplorer::BaseTriStateAspect(parent)
{
    setSettingsKey("QtQuickCompiler");
    setDisplayName(tr("Qt Quick Compiler:"));
    setSetting(ProjectExplorerPlugin::buildPropertiesSettings().qtQuickCompiler);
}

void QtQuickCompilerAspect::addToLayout(LayoutBuilder &builder)
{
    BaseSelectionAspect::addToLayout(builder);
    const auto warningIconLabel = new QLabel;
    warningIconLabel->setAlignment(Qt::AlignTop);
    warningIconLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    const auto warningTextLabel = new QLabel;
    warningTextLabel->setAlignment(Qt::AlignTop);
    builder.startNewRow().addItems(QString(), warningIconLabel, warningTextLabel);
    const auto changeHandler = [this, warningIconLabel, warningTextLabel] {
        QString warningText;
        const bool supported = m_kit
                && BaseQtVersion::isQtQuickCompilerSupported(m_kit, &warningText);
        if (!supported)
            setSetting(TriState::Default);
        if (setting() == TriState::Enabled
                && m_qmlDebuggingAspect && m_qmlDebuggingAspect->setting() == TriState::Enabled) {
            warningText = tr("Disables QML debugging. QML profiling will still work.");
        }
        warningTextLabel->setText(warningText);
        setVisibleDynamic(supported);
        const bool warningLabelsVisible = supported && !warningText.isEmpty();
        warningIconLabel->setVisible(warningLabelsVisible);
        warningTextLabel->setVisible(warningLabelsVisible);
    };
    connect(KitManager::instance(), &KitManager::kitsChanged, builder.layout(), changeHandler);
    connect(this, &QmlDebuggingAspect::changed, builder.layout(), changeHandler);
    connect(this, &QtQuickCompilerAspect::changed, builder.layout(), changeHandler);
    if (m_qmlDebuggingAspect) {
        connect(m_qmlDebuggingAspect, &QmlDebuggingAspect::changed, builder.layout(),
                changeHandler);
    }
    changeHandler();
}

void QtQuickCompilerAspect::acquaintSiblings(const ProjectConfigurationAspects &siblings)
{
    m_qmlDebuggingAspect = siblings.aspect<QmlDebuggingAspect>();
}

} // namespace QtSupport
