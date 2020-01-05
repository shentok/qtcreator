/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "customexecutablerunconfiguration.h"

#include "abi.h"
#include "buildconfiguration.h"
#include "devicesupport/devicemanager.h"
#include "environmentaspect.h"
#include "project.h"
#include "runcontrol.h"
#include "target.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>

#include <utils/detailswidget.h>
#include <utils/pathchooser.h>
#include <utils/stringutils.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {

const char CUSTOM_EXECUTABLE_ID[] = "ProjectExplorer.CustomExecutableRunConfiguration";

// CustomExecutableRunConfiguration

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target)
    : CustomExecutableRunConfiguration(target, CUSTOM_EXECUTABLE_ID)
{}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
    , m_envAspect(this, target)
    , m_exeAspect(this)
    , m_argumentsAspect(this)
    , m_workingDirectoryAspect(this)
    , m_terminalAspect(this)
{
    m_exeAspect.setSettingsKey("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
    m_exeAspect.setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    m_exeAspect.setHistoryCompleter("Qt.CustomExecutable.History");
    m_exeAspect.setExpectedKind(PathChooser::ExistingCommand);
    m_exeAspect.setEnvironment(m_envAspect.environment());

    connect(&m_envAspect, &EnvironmentAspect::environmentChanged,
            this, [this] { m_exeAspect.setEnvironment(m_envAspect.environment()); });

    setDefaultDisplayName(defaultDisplayName());
}

RunControl *CustomExecutableRunConfiguration::createRunControl(Core::Id runMode)
{
    auto runControl = new RunControl(runMode);
    runControl->setRunConfiguration(this);

    return runControl;
}

QString CustomExecutableRunConfiguration::rawExecutable() const
{
    return m_exeAspect.executable().toString();
}

bool CustomExecutableRunConfiguration::isEnabled() const
{
    return true;
}

Runnable CustomExecutableRunConfiguration::runnable() const
{
    FilePath workingDirectory =
            m_workingDirectoryAspect.workingDirectory(macroExpander());

    Runnable r;
    r.setCommandLine(commandLine());
    r.environment = m_envAspect.environment();
    r.workingDirectory = workingDirectory.toString();
    r.device = DeviceManager::instance()->defaultDevice(Constants::DESKTOP_DEVICE_TYPE);

    if (!r.executable.isEmpty()) {
        const QString expanded = macroExpander()->expand(r.executable.toString());
        r.executable = r.environment.searchInPath(expanded, {workingDirectory});
    }

    return r;
}

QString CustomExecutableRunConfiguration::defaultDisplayName() const
{
    if (rawExecutable().isEmpty())
        return tr("Custom Executable");
    else
        return tr("Run %1").arg(QDir::toNativeSeparators(rawExecutable()));
}

Tasks CustomExecutableRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (rawExecutable().isEmpty()) {
        tasks << createConfigurationIssue(tr("You need to set an executable in the custom run "
                                             "configuration."));
    }
    return tasks;
}

// Factory

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory() :
    FixedRunConfigurationFactory(CustomExecutableRunConfiguration::tr("Custom Executable"))
{
    registerRunConfiguration<CustomExecutableRunConfiguration>(CUSTOM_EXECUTABLE_ID);
}

} // namespace ProjectExplorer
