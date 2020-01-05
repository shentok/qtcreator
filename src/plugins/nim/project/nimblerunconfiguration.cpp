/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimblerunconfiguration.h"
#include "nimconstants.h"
#include "nimbleproject.h"

#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/environment.h>

#include <QStandardPaths>

using namespace Nim;
using namespace ProjectExplorer;

NimbleRunConfiguration::NimbleRunConfiguration(ProjectExplorer::Target *target, Core::Id id)
    : RunConfiguration(target, id)
    , m_envAspect(this, target)
    , m_exeAspect(this)
    , m_argumentsAspect(this)
    , m_workingDirectoryAspect(this)
    , m_terminalAspect(this)
{
    setUpdater([this] {
        BuildTargetInfo bti = buildTargetInfo();
        setDisplayName(bti.displayName);
        setDefaultDisplayName(bti.displayName);
        m_exeAspect.setExecutable(bti.targetFilePath);
        m_workingDirectoryAspect.setDefaultWorkingDirectory(bti.workingDirectory);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    update();
}

RunControl *NimbleRunConfiguration::createRunControl(Core::Id runMode)
{
    auto runControl = new RunControl(runMode);
    runControl->setRunConfiguration(this);

    return runControl;
}

bool NimbleRunConfiguration::isBuildTargetValid() const
{
    return Utils::anyOf(target()->applicationTargets(), [this](const BuildTargetInfo &bti) {
        return bti.buildKey == buildKey();
    });
}

QString NimbleRunConfiguration::disabledReason() const
{
    if (!isBuildTargetValid())
        return tr("The project no longer builds the target associated with this run configuration.");
    return RunConfiguration::disabledReason();
}

NimbleRunConfigurationFactory::NimbleRunConfigurationFactory()
    : RunConfigurationFactory()
{
    registerRunConfiguration<NimbleRunConfiguration>("Nim.NimbleRunConfiguration");
    addSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

QList<RunConfigurationCreationInfo> NimbleRunConfigurationFactory::availableCreators(Target *parent) const
{
    return RunConfigurationFactory::availableCreators(parent);
}

NimbleTestConfiguration::NimbleTestConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
    , m_exeAspect(this)
    , m_argumentsAspect(this)
    , m_workingDirectoryAspect(this)
    , m_terminalAspect(this)
{
    m_exeAspect.setExecutable(Utils::FilePath::fromString(QStandardPaths::findExecutable("nimble")));
    m_argumentsAspect.setArguments("test");
    m_workingDirectoryAspect.setDefaultWorkingDirectory(project()->projectDirectory());

    setDisplayName(tr("Nimble Test"));
    setDefaultDisplayName(tr("Nimble Test"));
}

RunControl *NimbleTestConfiguration::createRunControl(Core::Id runMode)
{
    auto runControl = new RunControl(runMode);
    runControl->setRunConfiguration(this);

    return runControl;
}

NimbleTestConfigurationFactory::NimbleTestConfigurationFactory()
    : FixedRunConfigurationFactory(QString())
{
    registerRunConfiguration<NimbleTestConfiguration>("Nim.NimbleTestConfiguration");
    addSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
}
