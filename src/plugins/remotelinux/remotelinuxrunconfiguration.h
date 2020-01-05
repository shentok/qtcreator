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

#include "remotelinux_export.h"

#include "remotelinuxenvironmentaspect.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <memory>

namespace RemoteLinux {

class X11ForwardingAspect;

namespace Internal {

class RemoteLinuxRunConfiguration final : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    RemoteLinuxRunConfiguration(ProjectExplorer::Target *target, Core::Id id);
    ~RemoteLinuxRunConfiguration() override;

    ProjectExplorer::RunControl *createRunControl(Core::Id runMode) override;

private:
    ProjectExplorer::Runnable runnable() const override;

    RemoteLinuxEnvironmentAspect m_envAspect;
    ProjectExplorer::ExecutableAspect m_exeAspect;
    ProjectExplorer::ArgumentsAspect m_argumentsAspect;
    ProjectExplorer::WorkingDirectoryAspect m_workingDirectoryAspect;
    ProjectExplorer::SymbolFileAspect m_symbolsAspect;
    std::unique_ptr<ProjectExplorer::TerminalAspect> m_terminalAspect;
    std::unique_ptr<X11ForwardingAspect> m_forwardingAspect;
};

class RemoteLinuxRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    RemoteLinuxRunConfigurationFactory();
};

} // namespace Internal
} // namespace RemoteLinux
