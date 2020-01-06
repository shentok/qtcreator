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

#include "qbscleanstep.h"

#include "qbsbuildconfiguration.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbssession.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonObject>

using namespace ProjectExplorer;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsCleanStep:
// --------------------------------------------------------------------

QbsCleanStep::QbsCleanStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Constants::QBS_CLEANSTEP_ID),
    m_dryRunAspect(this),
    m_keepGoingAspect(this),
    m_effectiveCommandAspect(this)
{
    setDisplayName(tr("Qbs Clean"));

    m_dryRunAspect.setSettingsKey("Qbs.DryRun");
    m_dryRunAspect.setLabel(tr("Dry run:"), BaseBoolAspect::LabelPlacement::InExtraLabel);

    m_keepGoingAspect.setSettingsKey("Qbs.DryKeepGoing");
    m_keepGoingAspect.setLabel(tr("Keep going:"), BaseBoolAspect::LabelPlacement::InExtraLabel);

    m_effectiveCommandAspect.setDisplayStyle(BaseStringAspect::TextEditDisplay);
    m_effectiveCommandAspect.setLabelText(tr("Equivalent command line:"));

    setSummaryUpdater([this] {
        QString command = static_cast<QbsBuildConfiguration *>(buildConfiguration())
                 ->equivalentCommandLine(this);
        m_effectiveCommandAspect.setValue(command);
        return tr("<b>Qbs:</b> %1").arg(command);
    });
}

QbsCleanStep::~QbsCleanStep()
{
    doCancel();
    if (m_session)
        m_session->disconnect(this);
}

bool QbsCleanStep::init()
{
    if (buildSystem()->isParsing() || m_session)
        return false;
    const auto bc = static_cast<QbsBuildConfiguration *>(buildConfiguration());
    if (!bc)
        return false;
    m_products = bc->products();
    return true;
}

void QbsCleanStep::doRun()
{
    m_session = static_cast<QbsBuildSystem*>(buildSystem())->session();
    if (!m_session) {
        emit addOutput(tr("No qbs session exists for this target."), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    QJsonObject request;
    request.insert("type", "clean-project");
    if (!m_products.isEmpty())
        request.insert("products", QJsonArray::fromStringList(m_products));
    request.insert("dry-run", m_dryRunAspect.value());
    request.insert("keep-going", m_keepGoingAspect.value());
    m_session->sendRequest(request);
    m_maxProgress = 0;
    connect(m_session, &QbsSession::projectCleaned, this, &QbsCleanStep::cleaningDone);
    connect(m_session, &QbsSession::taskStarted, this, &QbsCleanStep::handleTaskStarted);
    connect(m_session, &QbsSession::taskProgress, this, &QbsCleanStep::handleProgress);
    connect(m_session, &QbsSession::errorOccurred, this, [this] {
        cleaningDone(ErrorInfo(tr("Cleaning canceled: Qbs session failed.")));
    });
}

void QbsCleanStep::doCancel()
{
    if (m_session)
        m_session->cancelCurrentJob();
}

void QbsCleanStep::cleaningDone(const ErrorInfo &error)
{
    m_session->disconnect(this);
    m_session = nullptr;

    for (const ErrorInfoItem &item : error.items)
        createTaskAndOutput(Task::Error, item.description, item.filePath.toString(), item.line);
    emit finished(!error.hasError());
}

void QbsCleanStep::handleTaskStarted(const QString &desciption, int max)
{
    Q_UNUSED(desciption)
    m_maxProgress = max;
}

void QbsCleanStep::handleProgress(int value)
{
    if (m_maxProgress > 0)
        emit progress(value * 100 / m_maxProgress, m_description);
}

void QbsCleanStep::createTaskAndOutput(ProjectExplorer::Task::TaskType type, const QString &message, const QString &file, int line)
{
    Task task(type, message, Utils::FilePath::fromString(file), line,
              ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    emit addTask(task, 1);
    emit addOutput(message, OutputFormat::Stdout);
}

QbsBuildSystem *QbsCleanStep::qbsBuildSystem() const
{
    return static_cast<QbsBuildSystem *>(buildSystem());
}

// --------------------------------------------------------------------
// QbsCleanStepFactory:
// --------------------------------------------------------------------

QbsCleanStepFactory::QbsCleanStepFactory()
{
    registerStep<QbsCleanStep>(Constants::QBS_CLEANSTEP_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    setSupportedConfiguration(Constants::QBS_BC_ID);
    setDisplayName(QbsCleanStep::tr("Qbs Clean"));
}

} // namespace Internal
} // namespace QbsProjectManager
