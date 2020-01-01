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

#include "remotelinuxcheckforfreediskspacestep.h"

#include "remotelinuxcheckforfreediskspaceservice.h"

#include <limits>

using namespace ProjectExplorer;

namespace RemoteLinux {

RemoteLinuxCheckForFreeDiskSpaceStep::RemoteLinuxCheckForFreeDiskSpaceStep(BuildStepList *bsl)
        : AbstractRemoteLinuxDeployStep(bsl, stepId())
        , m_pathToCheckAspect(this)
        , m_requiredSpaceAspect(this)
{
    setDefaultDisplayName(displayName());

    auto service = createDeployService<RemoteLinuxCheckForFreeDiskSpaceService>();

    m_pathToCheckAspect.setSettingsKey("RemoteLinux.CheckForFreeDiskSpaceStep.PathToCheck");
    m_pathToCheckAspect.setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_pathToCheckAspect.setValue("/");
    m_pathToCheckAspect.setLabelText(tr("Remote path to check for free space:"));

    m_requiredSpaceAspect.setSettingsKey("RemoteLinux.CheckForFreeDiskSpaceStep.RequiredSpace");
    m_requiredSpaceAspect.setLabel(tr("Required disk space:"));
    m_requiredSpaceAspect.setDisplayScaleFactor(1024*1024);
    m_requiredSpaceAspect.setValue(5*1024*1024);
    m_requiredSpaceAspect.setSuffix(tr("MB"));
    m_requiredSpaceAspect.setRange(1, std::numeric_limits<int>::max());

    setInternalInitializer([this, service] {
        service->setPathToCheck(m_pathToCheckAspect.value());
        service->setRequiredSpaceInBytes(m_requiredSpaceAspect.value());
        return CheckResult::success();
    });
}

RemoteLinuxCheckForFreeDiskSpaceStep::~RemoteLinuxCheckForFreeDiskSpaceStep() = default;

Core::Id RemoteLinuxCheckForFreeDiskSpaceStep::stepId()
{
    return "RemoteLinux.CheckForFreeDiskSpaceStep";
}

QString RemoteLinuxCheckForFreeDiskSpaceStep::displayName()
{
    return tr("Check for free disk space");
}

} // namespace RemoteLinux
