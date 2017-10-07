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

#include "qmljslinkfinder.h"

#include "qmljseditor.h"
#include "qmljseditordocument.h"

#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/parser/qmljsast_p.h>

#include <qmljstools/qmljssemanticinfo.h>

#include <utils/qtcassert.h>

using namespace TextEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlJSLinkFinder::QmlJSLinkFinder(QmlJSEditorWidget *editorWidget) :
    m_editorWidget(editorWidget)
{
}

LinkFinder::Link QmlJSLinkFinder::findLinkAt(const QTextCursor &cursor,
                                             bool /*resolveTarget*/,
                                             bool /*inNextSplit*/)
{
    if (!m_editorWidget)
        return Link();

    const SemanticInfo semanticInfo = m_editorWidget->qmlJsEditorDocument()->semanticInfo();
    if (! semanticInfo.isValid())
        return Link();

    const unsigned cursorPosition = cursor.position();

    AST::Node *node = semanticInfo.astNodeAt(cursorPosition);
    QTC_ASSERT(node, return Link());

    if (AST::UiImport *importAst = cast<AST::UiImport *>(node)) {
        // if it's a file import, link to the file
        foreach (const ImportInfo &import, semanticInfo.document->bind()->imports()) {
            if (import.ast() == importAst && import.type() == ImportType::File) {
                TextEditorWidget::Link link(import.path());
                link.linkTextStart = importAst->firstSourceLocation().begin();
                link.linkTextEnd = importAst->lastSourceLocation().end();
                return link;
            }
        }
        return Link();
    }

    // string literals that could refer to a file link to them
    if (StringLiteral *literal = cast<StringLiteral *>(node)) {
        const QString &text = literal->value.toString();
        TextEditorWidget::Link link;
        link.linkTextStart = literal->literalToken.begin();
        link.linkTextEnd = literal->literalToken.end();
        if (semanticInfo.snapshot.document(text)) {
            link.targetFileName = text;
            return link;
        }
        const QString relative = QString::fromLatin1("%1/%2").arg(
                    semanticInfo.document->path(),
                    text);
        if (semanticInfo.snapshot.document(relative)) {
            link.targetFileName = relative;
            return link;
        }
    }

    const ScopeChain scopeChain = semanticInfo.scopeChain(semanticInfo.rangePath(cursorPosition));
    Evaluate evaluator(&scopeChain);
    const Value *value = evaluator.reference(node);

    QString fileName;
    int line = 0, column = 0;

    if (! (value && value->getSourceLocation(&fileName, &line, &column)))
        return Link();

    Link link;
    link.targetFileName = fileName;
    link.targetLine = line;
    link.targetColumn = column - 1; // adjust the column

    if (AST::UiQualifiedId *q = AST::cast<AST::UiQualifiedId *>(node)) {
        for (AST::UiQualifiedId *tail = q; tail; tail = tail->next) {
            if (! tail->next && cursorPosition <= tail->identifierToken.end()) {
                link.linkTextStart = tail->identifierToken.begin();
                link.linkTextEnd = tail->identifierToken.end();
                return link;
            }
        }

    } else if (AST::IdentifierExpression *id = AST::cast<AST::IdentifierExpression *>(node)) {
        link.linkTextStart = id->firstSourceLocation().begin();
        link.linkTextEnd = id->lastSourceLocation().end();
        return link;

    } else if (AST::FieldMemberExpression *mem = AST::cast<AST::FieldMemberExpression *>(node)) {
        link.linkTextStart = mem->lastSourceLocation().begin();
        link.linkTextEnd = mem->lastSourceLocation().end();
        return link;
    }

    return Link();
}
