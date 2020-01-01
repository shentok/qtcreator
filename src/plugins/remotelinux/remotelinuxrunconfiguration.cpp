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

#include "remotelinuxrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxx11forwardingaspect.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
    , m_envAspect(this, target)
    , m_exeAspect(this)
    , m_argumentsAspect(this)
    , m_workingDirectoryAspect(this)
    , m_symbolsAspect(this)
    , m_terminalAspect()
    , m_forwardingAspect()
{
    m_exeAspect.setLabelText(tr("Executable on device:"));
    m_exeAspect.setExecutablePathStyle(OsTypeLinux);
    m_exeAspect.setPlaceHolderText(tr("Remote path not set"));
    m_exeAspect.makeOverridable("RemoteLinux.RunConfig.AlternateRemoteExecutable",
                               "RemoteLinux.RunConfig.UseAlternateRemoteExecutable");
    m_exeAspect.setHistoryCompleter("RemoteLinux.AlternateExecutable.History");

    m_symbolsAspect.setLabelText(tr("Executable on host:"));
    m_symbolsAspect.setDisplayStyle(SymbolFileAspect::LabelDisplay);

    if (HostOsInfo::isAnyUnixHost())
        m_terminalAspect.reset(new TerminalAspect(this));
    if (HostOsInfo::isAnyUnixHost())
        m_forwardingAspect.reset(new X11ForwardingAspect(this));

    setUpdater([this, target] {
        BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        DeployableFile depFile = target->deploymentData().deployableForLocalFile(localExecutable);

        m_exeAspect.setExecutable(FilePath::fromString(depFile.remoteFilePath()));
        m_symbolsAspect.setFilePath(localExecutable);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    connect(target, &Target::kitChanged, this, &RunConfiguration::update);
}

RemoteLinuxRunConfiguration::~RemoteLinuxRunConfiguration() = default;

Runnable RemoteLinuxRunConfiguration::runnable() const
{
    Runnable r = RunConfiguration::runnable();
    if (m_forwardingAspect)
        r.extraData.insert("Ssh.X11ForwardToDisplay", m_forwardingAspect->display(macroExpander()));
    return r;
}

// RemoteLinuxRunConfigurationFactory

RemoteLinuxRunConfigurationFactory::RemoteLinuxRunConfigurationFactory()
{
    registerRunConfiguration<RemoteLinuxRunConfiguration>("RemoteLinuxRunConfiguration:");
    setDecorateDisplayNames(true);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // namespace Internal
} // namespace RemoteLinux
