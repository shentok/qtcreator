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

#include "qdbrunconfiguration.h"

#include "qdbconstants.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb {
namespace Internal {

// FullCommandLineAspect

FullCommandLineAspect::FullCommandLineAspect(RunConfiguration *rc)
    : ProjectExplorer::BaseStringAspect(rc)
{
    setLabelText(QdbRunConfiguration::tr("Full command line:"));

    auto exeAspect = rc->aspect<ExecutableAspect>();
    auto argumentsAspect = rc->aspect<ArgumentsAspect>();

    auto updateCommandLine = [this, rc, exeAspect, argumentsAspect] {
        const QString usedExecutable = exeAspect->executable().toString();
        const QString args = argumentsAspect->arguments(rc->macroExpander());
        setValue(QString(Constants::AppcontrollerFilepath)
                    + ' ' + usedExecutable + ' ' + args);
    };

    connect(argumentsAspect, &ArgumentsAspect::argumentsChanged, this, updateCommandLine);
    connect(exeAspect, &ExecutableAspect::changed, this, updateCommandLine);
    updateCommandLine();
}

// QdbRunConfiguration

QdbRunConfiguration::QdbRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
    , m_envAspect(this, target)
    , m_exeAspect(this)
    , m_argumentsAspect(this)
    , m_workingDirectoryAspect(this)
    , m_symbolsAspect(this)
    , m_commandLineAspect(this)
{
    m_exeAspect.setSettingsKey("QdbRunConfig.RemoteExecutable");
    m_exeAspect.setLabelText(tr("Executable on device:"));
    m_exeAspect.setExecutablePathStyle(OsTypeLinux);
    m_exeAspect.setPlaceHolderText(tr("Remote path not set"));
    m_exeAspect.makeOverridable("QdbRunConfig.AlternateRemoteExecutable",
                               "QdbRunCofig.UseAlternateRemoteExecutable");

    m_symbolsAspect.setSettingsKey("QdbRunConfig.LocalExecutable");
    m_symbolsAspect.setLabelText(tr("Executable on host:"));
    m_symbolsAspect.setDisplayStyle(SymbolFileAspect::LabelDisplay);

    setUpdater([this, target] {
        const BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        const DeployableFile depFile = target->deploymentData().deployableForLocalFile(localExecutable);

        m_exeAspect.setExecutable(FilePath::fromString(depFile.remoteFilePath()));
        m_symbolsAspect.setFilePath(localExecutable);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);

    setDefaultDisplayName(tr("Run on Boot2Qt Device"));
}

RunControl *QdbRunConfiguration::createRunControl(Core::Id runMode)
{
    auto runControl = new RunControl(runMode);
    runControl->setRunConfiguration(this);

    return runControl;
}

Tasks QdbRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (m_exeAspect.executable().toString().isEmpty()) {
        tasks << createConfigurationIssue(tr("The remote executable must be set "
                                             "in order to run on a Boot2Qt device."));
    }
    return tasks;
}

QString QdbRunConfiguration::defaultDisplayName() const
{
    return RunConfigurationFactory::decoratedTargetName(buildKey(), target());
}

// QdbRunConfigurationFactory

QdbRunConfigurationFactory::QdbRunConfigurationFactory()
{
    registerRunConfiguration<QdbRunConfiguration>("QdbLinuxRunConfiguration:");
    addSupportedTargetDeviceType(Constants::QdbLinuxOsType);
}

} // namespace Internal
} // namespace Qdb
