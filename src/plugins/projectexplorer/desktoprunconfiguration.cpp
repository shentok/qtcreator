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

#include "desktoprunconfiguration.h"

#include "buildsystem.h"
#include "project.h"
#include "target.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qbsprojectmanager/qbsprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFileInfo>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

RunControl *DesktopRunConfiguration::createRunControl(Core::Id runMode)
{
    auto runControl = new RunControl(runMode);
    runControl->setRunConfiguration(this);

    return runControl;
}

DesktopRunConfiguration::DesktopRunConfiguration(Target *target, Core::Id id, Kind kind)
    : RunConfiguration(target, id)
    , m_envAspect(this, target)
    , m_executableAspect(this)
    , m_argumentsAspect(this)
    , m_workingDirectoryAspect(this)
    , m_terminalAspect(this)
    , m_libAspect(this)
    , m_dyldAspect()
    , m_kind(kind)
{
    connect(&m_libAspect, &UseLibraryPathsAspect::changed,
            &m_envAspect, &EnvironmentAspect::environmentChanged);

    if (HostOsInfo::isMacHost()) {
        m_dyldAspect.reset(new UseDyldSuffixAspect(this));
        connect(m_dyldAspect.get(), &UseLibraryPathsAspect::changed,
                &m_envAspect, &EnvironmentAspect::environmentChanged);
        m_envAspect.addModifier([this](Environment &env) {
            if (m_dyldAspect->value())
                env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
        });
    }

    m_envAspect.addModifier([this](Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, m_libAspect.value());
    });


    setUpdater([this] { updateTargetInformation(); });

    if (m_kind == CMake)
        m_libAspect.setVisible(false);

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

void DesktopRunConfiguration::updateTargetInformation()
{
    if (!activeBuildSystem())
        return;

    BuildTargetInfo bti = buildTargetInfo();

    m_terminalAspect.setUseTerminalHint(bti.usesTerminal);

    if (m_kind == Qmake) {

        FilePath profile = FilePath::fromString(buildKey());
        if (profile.isEmpty())
            setDefaultDisplayName(tr("Qt Run Configuration"));
        else
            setDefaultDisplayName(profile.toFileInfo().completeBaseName());

        m_envAspect.environmentChanged();

        m_workingDirectoryAspect.setDefaultWorkingDirectory(bti.workingDirectory);

        m_executableAspect.setExecutable(bti.targetFilePath);

    }  else if (m_kind == Qbs) {

        setDefaultDisplayName(bti.displayName);
        const FilePath executable = executableToRun(bti);

        m_executableAspect.setExecutable(executable);

        if (!executable.isEmpty()) {
            const FilePath defaultWorkingDir = executable.absolutePath();
            if (!defaultWorkingDir.isEmpty())
                m_workingDirectoryAspect.setDefaultWorkingDirectory(defaultWorkingDir);
        }

    } else if (m_kind == CMake) {

        m_executableAspect.setExecutable(bti.targetFilePath);
        m_workingDirectoryAspect.setDefaultWorkingDirectory(bti.workingDirectory);
        m_envAspect.environmentChanged();

    }
}

Utils::FilePath DesktopRunConfiguration::executableToRun(const BuildTargetInfo &targetInfo) const
{
    const FilePath appInBuildDir = targetInfo.targetFilePath;
    const DeploymentData deploymentData = target()->deploymentData();
    if (deploymentData.localInstallRoot().isEmpty())
        return appInBuildDir;

    const QString deployedAppFilePath = deploymentData
            .deployableForLocalFile(appInBuildDir).remoteFilePath();
    if (deployedAppFilePath.isEmpty())
        return appInBuildDir;

    const FilePath appInLocalInstallDir = deploymentData.localInstallRoot() + deployedAppFilePath;
    return appInLocalInstallDir.exists() ? appInLocalInstallDir : appInBuildDir;
}

// Factory

class DesktopQmakeRunConfiguration : public DesktopRunConfiguration
{
public:
    DesktopQmakeRunConfiguration(Target *target, Core::Id id)
        : DesktopRunConfiguration(target, id, Qmake)
    {}
};

class QbsRunConfiguration : public DesktopRunConfiguration
{
public:
    QbsRunConfiguration(Target *target, Core::Id id)
        : DesktopRunConfiguration(target, id, Qbs)
    {}
};

class CMakeRunConfiguration : public DesktopRunConfiguration
{
public:
    CMakeRunConfiguration(Target *target, Core::Id id)
        : DesktopRunConfiguration(target, id, CMake)
    {}
};

const char QMAKE_RUNCONFIG_ID[] = "Qt4ProjectManager.Qt4RunConfiguration:";
const char QBS_RUNCONFIG_ID[]   = "Qbs.RunConfiguration:";
const char CMAKE_RUNCONFIG_ID[] = "CMakeProjectManager.CMakeRunConfiguration.";

CMakeRunConfigurationFactory::CMakeRunConfigurationFactory()
{
    registerRunConfiguration<CMakeRunConfiguration>(CMAKE_RUNCONFIG_ID);
    addSupportedProjectType(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

QbsRunConfigurationFactory::QbsRunConfigurationFactory()
{
    registerRunConfiguration<QbsRunConfiguration>(QBS_RUNCONFIG_ID);
    addSupportedProjectType(QbsProjectManager::Constants::PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

DesktopQmakeRunConfigurationFactory::DesktopQmakeRunConfigurationFactory()
{
    registerRunConfiguration<DesktopQmakeRunConfiguration>(QMAKE_RUNCONFIG_ID);
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace ProjectExplorer
