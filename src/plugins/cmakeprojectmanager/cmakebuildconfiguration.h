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

#include "cmakeconfigitem.h"
#include "cmakeproject.h"
#include "configmodel.h"

#include <cpptools/cpprawprojectpart.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/abi.h>

#include <memory>

namespace ProjectExplorer { class ToolChain; }

namespace CMakeProjectManager {
class CMakeBuildInfo;
class CMakeProject;

namespace Internal {

class BuildDirManager;
class CMakeBuildConfigurationFactory;
class CMakeBuildSettingsWidget;
class CMakeProjectNode;

class CMakeBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    CMakeBuildConfiguration(ProjectExplorer::Target *parent);

    bool isEnabled() const override;
    QString disabledReason() const override;

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    QVariantMap toMap() const override;

    BuildType buildType() const override;

    void emitBuildTypeChanged();

    CMakeConfig configurationForCMake() const;
    CMakeConfig configurationFromCMake() const;

    QString error() const;
    QString warning() const;

    QList<CMakeBuildTarget> buildTargets() const;

    static Utils::FileName
    shadowBuildDirectory(const Utils::FileName &projectFilePath, const ProjectExplorer::Kit *k,
                         const QString &bcName, BuildConfiguration::BuildType buildType);

    // Context menu action:
    void buildTarget(const QString &buildTarget);

signals:
    void errorOccured(const QString &message);
    void warningOccured(const QString &message);

    void configurationForCMakeChanged();

protected:
    CMakeBuildConfiguration(ProjectExplorer::Target *parent, CMakeBuildConfiguration *source);
    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    bool isParsing() const;

    enum ForceEnabledChanged : quint8 { False, True };
    void clearError(ForceEnabledChanged fec = ForceEnabledChanged::False);

    void setBuildTargets(const QList<CMakeBuildTarget> &targets);
    void setConfigurationFromCMake(const CMakeConfig &config);
    void setConfigurationForCMake(const QList<ConfigModel::DataItem> &items);
    void setConfigurationForCMake(const CMakeConfig &config);

    void setError(const QString &message);
    void setWarning(const QString &message);

    CMakeConfig m_configurationForCMake;
    QString m_error;
    QString m_warning;

    CMakeConfig m_configurationFromCMake;
    QList<CMakeBuildTarget> m_buildTargets;

    friend class CMakeBuildConfigurationFactory;
    friend class CMakeBuildSettingsWidget;
    friend class CMakeProjectManager::CMakeProject;
    friend class BuildDirManager;
};

class CMakeProjectImporter;

class CMakeBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    CMakeBuildConfigurationFactory(QObject *parent = 0);

    enum BuildType { BuildTypeNone = 0,
                     BuildTypeDebug = 1,
                     BuildTypeRelease = 2,
                     BuildTypeRelWithDebInfo = 3,
                     BuildTypeMinSizeRel = 4,
                     BuildTypeLast = 5 };
    static BuildType buildTypeFromByteArray(const QByteArray &in);
    static ProjectExplorer::BuildConfiguration::BuildType cmakeBuildTypeToBuildType(const BuildType &in);

    int priority(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const override;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const override;
    CMakeBuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    CMakeBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;

private:
    bool canHandle(const ProjectExplorer::Target *t) const;

    CMakeBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k,
                                    const QString &sourceDir,
                                    BuildType buildType) const;

    friend class CMakeProjectImporter;
};

} // namespace Internal
} // namespace CMakeProjectManager
