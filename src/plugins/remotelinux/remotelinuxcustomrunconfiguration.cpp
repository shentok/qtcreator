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

#include "remotelinuxcustomrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxx11forwardingaspect.h"

#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxCustomRunConfiguration::RemoteLinuxCustomRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
    , m_envAspect(this, target)
    , m_exeAspect(this)
    , m_argumentsAspect(this)
    , m_workingDirectoryAspect(this)
    , m_symbolsAspect(this)
    , m_terminalAspect()
    , m_forwardingAspect()
{
    m_exeAspect.setSettingsKey("RemoteLinux.CustomRunConfig.RemoteExecutable");
    m_exeAspect.setLabelText(tr("Remote executable:"));
    m_exeAspect.setExecutablePathStyle(OsTypeLinux);
    m_exeAspect.setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_exeAspect.setHistoryCompleter("RemoteLinux.CustomExecutable.History");
    m_exeAspect.setExpectedKind(PathChooser::Any);

    m_symbolsAspect.setSettingsKey("RemoteLinux.CustomRunConfig.LocalExecutable");
    m_symbolsAspect.setLabelText(tr("Local executable:"));
    m_symbolsAspect.setDisplayStyle(SymbolFileAspect::PathChooserDisplay);

    if (HostOsInfo::isAnyUnixHost())
        m_terminalAspect.reset(new TerminalAspect(this));
    if (Utils::HostOsInfo::isAnyUnixHost())
        m_forwardingAspect.reset(new X11ForwardingAspect(this));

    setDefaultDisplayName(runConfigDefaultDisplayName());
}

RunControl *RemoteLinuxCustomRunConfiguration::createRunControl(Core::Id runMode)
{
    auto runControl = new RunControl(runMode);
    runControl->setRunConfiguration(this);

    return runControl;
}

RemoteLinuxCustomRunConfiguration::~RemoteLinuxCustomRunConfiguration()= default;

QString RemoteLinuxCustomRunConfiguration::runConfigDefaultDisplayName()
{
    QString remoteExecutable = m_exeAspect.executable().toString();
    QString display = remoteExecutable.isEmpty()
            ? tr("Custom Executable") : tr("Run \"%1\"").arg(remoteExecutable);
    return  RunConfigurationFactory::decoratedTargetName(display, target());
}

Runnable RemoteLinuxCustomRunConfiguration::runnable() const
{
    ProjectExplorer::Runnable r = RunConfiguration::runnable();
    if (m_forwardingAspect)
        r.extraData.insert("Ssh.X11ForwardToDisplay", m_forwardingAspect->display(macroExpander()));
    return r;
}

Tasks RemoteLinuxCustomRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (m_exeAspect.executable().isEmpty()) {
        tasks << createConfigurationIssue(tr("The remote executable must be set in order to run "
                                             "a custom remote run configuration."));
    }
    return tasks;
}

// RemoteLinuxCustomRunConfigurationFactory

RemoteLinuxCustomRunConfigurationFactory::RemoteLinuxCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(RemoteLinuxCustomRunConfiguration::tr("Custom Executable"), true)
{
    registerRunConfiguration<RemoteLinuxCustomRunConfiguration>("RemoteLinux.CustomRunConfig");
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // namespace Internal
} // namespace RemoteLinux
