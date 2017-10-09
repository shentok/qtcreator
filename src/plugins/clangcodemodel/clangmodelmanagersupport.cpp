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

#include "clangmodelmanagersupport.h"

#include "clangconstants.h"
#include "clangeditordocumentprocessor.h"
#include "clangutils.h"
#include "clangfollowsymbol.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppfollowsymbolundercursor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/editordocumenthandle.h>
#include <cpptools/projectinfo.h>

#include <texteditor/quickfix.h>

#include <projectexplorer/project.h>

#include <clangsupport/cmbregisterprojectsforeditormessage.h>
#include <clangsupport/filecontainer.h>
#include <clangsupport/projectpartcontainer.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QMenu>
#include <QTextBlock>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

static ModelManagerSupportClang *m_instance = 0;

static bool useClangFollowSymbol()
{
    static bool use = qEnvironmentVariableIntValue("QTC_CLANG_FOLLOW_SYMBOL");
    return use;
}

ModelManagerSupportClang::ModelManagerSupportClang()
    : m_modelManager(CppTools::CppModelManager::instance())
    , m_communicator(m_modelManager)
    , m_completionAssistProvider(m_communicator)
{
    QTC_CHECK(!m_instance);
    m_instance = this;

    if (useClangFollowSymbol())
        m_followSymbol.reset(new ClangFollowSymbol);
    else
        m_followSymbol.reset(new CppTools::FollowSymbolUnderCursor);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::editorOpened,
            this, &ModelManagerSupportClang::onEditorOpened);
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ModelManagerSupportClang::onCurrentEditorChanged,
            Qt::QueuedConnection);
    connect(editorManager, &Core::EditorManager::editorsClosed,
            this, &ModelManagerSupportClang::onEditorClosed,
            Qt::QueuedConnection);

    connect(m_modelManager, &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
            this, &ModelManagerSupportClang::onAbstractEditorSupportContentsUpdated);
    connect(m_modelManager, &CppTools::CppModelManager::abstractEditorSupportRemoved,
            this, &ModelManagerSupportClang::onAbstractEditorSupportRemoved);
    connect(m_modelManager, &CppTools::CppModelManager::projectPartsUpdated,
            this, &ModelManagerSupportClang::onProjectPartsUpdated);
    connect(m_modelManager, &CppTools::CppModelManager::projectPartsRemoved,
            this, &ModelManagerSupportClang::onProjectPartsRemoved);

    m_communicator.registerFallbackProjectPart();
}

ModelManagerSupportClang::~ModelManagerSupportClang()
{
    m_instance = 0;
}

CppTools::CppCompletionAssistProvider *ModelManagerSupportClang::completionAssistProvider()
{
    return &m_completionAssistProvider;
}

CppTools::FollowSymbolInterface &ModelManagerSupportClang::followSymbolInterface()
{
    return *m_followSymbol;
}

CppTools::BaseEditorDocumentProcessor *ModelManagerSupportClang::editorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new ClangEditorDocumentProcessor(m_communicator, baseTextDocument);
}

void ModelManagerSupportClang::onCurrentEditorChanged(Core::IEditor *)
{
    m_communicator.updateTranslationUnitVisiblity();
}

void ModelManagerSupportClang::connectTextDocumentToTranslationUnit(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ModelManagerSupportClang::onCppDocumentAboutToReloadOnTranslationUnit,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ModelManagerSupportClang::onCppDocumentReloadFinishedOnTranslationUnit,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForTranslationUnit(textDocument);
}

void ModelManagerSupportClang::connectTextDocumentToUnsavedFiles(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ModelManagerSupportClang::onCppDocumentAboutToReloadOnUnsavedFile,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ModelManagerSupportClang::onCppDocumentReloadFinishedOnUnsavedFile,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
}

void ModelManagerSupportClang::connectToTextDocumentContentsChangedForTranslationUnit(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnTranslationUnit,
            Qt::UniqueConnection);
}

void ModelManagerSupportClang::connectToTextDocumentContentsChangedForUnsavedFile(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnUnsavedFile,
            Qt::UniqueConnection);
}

void ModelManagerSupportClang::connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget)
{
    const auto widget = qobject_cast<TextEditor::TextEditorWidget *>(editorWidget);
    if (widget) {
        connect(widget, &TextEditor::TextEditorWidget::markContextMenuRequested,
                this, &ModelManagerSupportClang::onTextMarkContextMenuRequested);
    }
}

void ModelManagerSupportClang::onEditorOpened(Core::IEditor *editor)
{
    QTC_ASSERT(editor, return);
    Core::IDocument *document = editor->document();
    QTC_ASSERT(document, return);
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(document);

    if (textDocument && m_modelManager->isCppEditor(editor)) {
        connectTextDocumentToTranslationUnit(textDocument);
        connectToWidgetsMarkContextMenuRequested(editor->widget());

        // TODO: Ensure that not fully loaded documents are updated?
    }
}

void ModelManagerSupportClang::onEditorClosed(const QList<Core::IEditor *> &)
{
    m_communicator.updateTranslationUnitVisiblity();
}

void ModelManagerSupportClang::onCppDocumentAboutToReloadOnTranslationUnit()
{
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnTranslationUnit);
}

void ModelManagerSupportClang::onCppDocumentReloadFinishedOnTranslationUnit(bool success)
{
    if (success) {
        TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
        connectToTextDocumentContentsChangedForTranslationUnit(textDocument);
        m_communicator.updateTranslationUnitWithRevisionCheck(textDocument);
    }
}

namespace {
void clearDiagnosticFixIts(const QString &filePath)
{
    auto processor = ClangEditorDocumentProcessor::get(filePath);
    if (processor)
        processor->clearDiagnosticsWithFixIts();
}
}

void ModelManagerSupportClang::onCppDocumentContentsChangedOnTranslationUnit(int position,
                                                                             int /*charsRemoved*/,
                                                                             int /*charsAdded*/)
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());

    m_communicator.updateChangeContentStartPosition(document->filePath().toString(),
                                                       position);
    m_communicator.updateTranslationUnitIfNotCurrentDocument(document);

    clearDiagnosticFixIts(document->filePath().toString());
}

void ModelManagerSupportClang::onCppDocumentAboutToReloadOnUnsavedFile()
{
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ModelManagerSupportClang::onCppDocumentContentsChangedOnUnsavedFile);
}

void ModelManagerSupportClang::onCppDocumentReloadFinishedOnUnsavedFile(bool success)
{
    if (success) {
        TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
        connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
        m_communicator.updateUnsavedFile(textDocument);
    }
}

void ModelManagerSupportClang::onCppDocumentContentsChangedOnUnsavedFile()
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());
    m_communicator.updateUnsavedFile(document);
}

void ModelManagerSupportClang::onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                                      const QByteArray &content)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    const QString mappedPath = m_uiHeaderOnDiskManager.createIfNeeded(filePath);
    m_communicator.updateUnsavedFile(mappedPath, content, 0);
}

void ModelManagerSupportClang::onAbstractEditorSupportRemoved(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    if (!m_modelManager->cppEditorDocument(filePath)) {
        const QString mappedPath = m_uiHeaderOnDiskManager.remove(filePath);
        const QString projectPartId = Utils::projectPartIdForFile(filePath);
        m_communicator.unregisterUnsavedFilesForEditor({{mappedPath, projectPartId}});
    }
}

void addFixItsActionsToMenu(QMenu *menu, const TextEditor::QuickFixOperations &fixItOperations)
{
    foreach (const auto &fixItOperation, fixItOperations) {
        QAction *action = menu->addAction(fixItOperation->description());
        QObject::connect(action, &QAction::triggered, [fixItOperation]() {
            fixItOperation->perform();
        });
    }
}

static int lineToPosition(const QTextDocument *textDocument, int lineNumber)
{
    QTC_ASSERT(textDocument, return 0);
    const QTextBlock textBlock = textDocument->findBlockByLineNumber(lineNumber);
    return textBlock.isValid() ? textBlock.position() - 1 : 0;
}

static TextEditor::AssistInterface createAssistInterface(TextEditor::TextEditorWidget *widget,
                                                         int lineNumber)
{
    return TextEditor::AssistInterface(widget->document(),
                                       lineToPosition(widget->document(), lineNumber),
                                       widget->textDocument()->filePath().toString(),
                                       TextEditor::IdleEditor);
}

void ModelManagerSupportClang::onTextMarkContextMenuRequested(TextEditor::TextEditorWidget *widget,
                                                              int lineNumber,
                                                              QMenu *menu)
{
    QTC_ASSERT(widget, return);
    QTC_ASSERT(lineNumber >= 1, return);
    QTC_ASSERT(menu, return);

    const auto filePath = widget->textDocument()->filePath().toString();
    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(filePath);
    if (processor) {
        const auto assistInterface = createAssistInterface(widget, lineNumber);
        const auto fixItOperations = processor->extraRefactoringOperations(assistInterface);

        addFixItsActionsToMenu(menu, fixItOperations);
    }
}

void ModelManagerSupportClang::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    const CppTools::ProjectInfo projectInfo = m_modelManager->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return);

    m_communicator.registerProjectsParts(projectInfo.projectParts());
    m_communicator.registerFallbackProjectPart();
}

void ModelManagerSupportClang::onProjectPartsRemoved(const QStringList &projectPartIds)
{
    if (!projectPartIds.isEmpty()) {
        unregisterTranslationUnitsWithProjectParts(projectPartIds);
        m_communicator.unregisterProjectPartsForEditor(projectPartIds);
        m_communicator.registerFallbackProjectPart();
    }
}

static QVector<ClangEditorDocumentProcessor *>
clangProcessorsWithProjectParts(const QStringList &projectPartIds, const CppTools::CppModelManager &modelManager)
{
    QVector<ClangEditorDocumentProcessor *> result;

    foreach (auto *editorDocument, modelManager.cppEditorDocuments()) {
        auto *processor = editorDocument->processor();
        auto *clangProcessor = qobject_cast<ClangEditorDocumentProcessor *>(processor);
        if (clangProcessor && clangProcessor->hasProjectPart()) {
            if (projectPartIds.contains(clangProcessor->projectPart()->id()))
                result.append(clangProcessor);
        }
    }

    return result;
}

void ModelManagerSupportClang::unregisterTranslationUnitsWithProjectParts(
        const QStringList &projectPartIds)
{
    const auto processors = clangProcessorsWithProjectParts(projectPartIds, *m_modelManager);
    foreach (ClangEditorDocumentProcessor *processor, processors) {
        m_communicator.unregisterTranslationUnitsForEditor({processor->fileContainerWithArguments()});
        processor->clearProjectPart();
        processor->run();
    }
}

ModelManagerSupportClang *ModelManagerSupportClang::instance()
{
    return m_instance;
}

BackendCommunicator &ModelManagerSupportClang::communicator()
{
    return m_communicator;
}

QString ModelManagerSupportClang::dummyUiHeaderOnDiskPath(const QString &filePath) const
{
    return m_uiHeaderOnDiskManager.mapPath(filePath);
}

QString ModelManagerSupportClang::dummyUiHeaderOnDiskDirPath() const
{
    return m_uiHeaderOnDiskManager.directoryPath();
}

QString ModelManagerSupportProviderClang::id() const
{
    return QLatin1String(Constants::CLANG_MODELMANAGERSUPPORT_ID);
}

QString ModelManagerSupportProviderClang::displayName() const
{
    //: Display name
    return QCoreApplication::translate("ClangCodeModel::Internal::ModelManagerSupport",
                                       "Clang");
}

CppTools::ModelManagerSupport::Ptr ModelManagerSupportProviderClang::createModelManagerSupport()
{
    return CppTools::ModelManagerSupport::Ptr(new ModelManagerSupportClang);
}
