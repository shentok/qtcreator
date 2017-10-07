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

#include "cpplinkfinder.h"

#include "cppeditorwidget.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppsemanticinfo.h>
#include <cpptools/followsymbolinterface.h>

#include <utils/fileutils.h>

#include <texteditor/textdocument.h>

using namespace TextEditor;
using namespace CppEditor::Internal;
using namespace CppTools;

CppLinkFinder::CppLinkFinder(CppEditorWidget *editorWidget, CppModelManager *modelManager, SemanticInfo *lastSemanticInfo) :
    m_editorWidget(editorWidget),
    m_modelManager(modelManager),
    m_lastSemanticInfo(lastSemanticInfo)
{
}

LinkFinder::Link CppLinkFinder::findLinkAt(const QTextCursor &cursor, bool resolveTarget, bool inNextSplit)
{
    if (!m_modelManager)
        return Link();
    if (!m_editorWidget)
        return Link();

    const Utils::FileName &filePath = m_editorWidget->textDocument()->filePath();

    return m_modelManager->followSymbolInterface().findLink(CppTools::CursorInEditor{cursor, filePath, m_editorWidget},
                                             resolveTarget,
                                             m_modelManager->snapshot(),
                                             m_lastSemanticInfo->doc,
                                             m_modelManager->symbolFinder(),
                                             inNextSplit);
}
