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

#pragma once

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

namespace Nim {

class NimbleRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    NimbleRunConfiguration(ProjectExplorer::Target *target, Core::Id id);

    QString disabledReason() const override;

private:
    ProjectExplorer::LocalEnvironmentAspect m_envAspect;
    ProjectExplorer::ExecutableAspect m_exeAspect;
    ProjectExplorer::ArgumentsAspect m_argumentsAspect;
    ProjectExplorer::WorkingDirectoryAspect m_workingDirectoryAspect;
    ProjectExplorer::TerminalAspect m_terminalAspect;

    bool isBuildTargetValid() const;
};

class NimbleRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    NimbleRunConfigurationFactory();

protected:
    QList<ProjectExplorer::RunConfigurationCreationInfo> availableCreators(ProjectExplorer::Target *parent) const override;
};

class NimbleTestConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    NimbleTestConfiguration(ProjectExplorer::Target *target, Core::Id id);

private:
    ProjectExplorer::ExecutableAspect m_exeAspect;
    ProjectExplorer::ArgumentsAspect m_argumentsAspect;
    ProjectExplorer::WorkingDirectoryAspect m_workingDirectoryAspect;
    ProjectExplorer::TerminalAspect m_terminalAspect;
};

class NimbleTestConfigurationFactory : public ProjectExplorer::FixedRunConfigurationFactory
{
public:
    NimbleTestConfigurationFactory();
};

}
