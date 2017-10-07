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

#include "profilelinkfinder.h"

#include "profileeditor.h"

#include <texteditor/textdocument.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QTextBlock>

#include <algorithm>

using namespace QmakeProjectManager::Internal;
using namespace TextEditor;

static bool isValidFileNameChar(const QChar &c)
{
    return c.isLetterOrNumber()
            || c == QLatin1Char('.')
            || c == QLatin1Char('_')
            || c == QLatin1Char('-')
            || c == QLatin1Char('/')
            || c == QLatin1Char('\\');
}

ProFileLinkFinder::ProFileLinkFinder(TextEditorWidget *editorWidget) :
    m_editorWidget(editorWidget)
{
}

LinkFinder::Link ProFileLinkFinder::findLinkAt(const QTextCursor &cursor,
                                               bool /*resolveTarget*/,
                                               bool /*inNextSplit*/)
{
    if (!m_editorWidget)
        return Link();

    int lineNumber = 0, positionInBlock = 0;
    m_editorWidget->convertPosition(cursor.position(), &lineNumber, &positionInBlock);

    const QString block = cursor.block().text();

    // check if the current position is commented out
    const int hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < positionInBlock)
        return Link();

    // find the beginning of a filename
    QString buffer;
    int beginPos = positionInBlock - 1;
    int endPos = positionInBlock;

    // Check is cursor is somewhere on $${PWD}:
    const int chunkStart = std::max(0, positionInBlock - 7);
    const int chunkLength = 14 + std::min(0, positionInBlock - 7);
    QString chunk = block.mid(chunkStart, chunkLength);

    const QString curlyPwd = "$${PWD}";
    const QString pwd = "$$PWD";
    const int posCurlyPwd = chunk.indexOf(curlyPwd);
    const int posPwd = chunk.indexOf(pwd);
    bool doBackwardScan = true;

    if (posCurlyPwd >= 0) {
        const int end = chunkStart + posCurlyPwd + curlyPwd.count();
        const int start = chunkStart + posCurlyPwd;
        if (start <= positionInBlock && end >= positionInBlock) {
            buffer = pwd;
            beginPos = chunkStart + posCurlyPwd - 1;
            endPos = end;
            doBackwardScan = false;
        }
    } else if (posPwd >= 0) {
        const int end = chunkStart + posPwd + pwd.count();
        const int start = chunkStart + posPwd;
        if (start <= positionInBlock && end >= positionInBlock) {
            buffer = pwd;
            beginPos = start - 1;
            endPos = end;
            doBackwardScan = false;
        }
    }

    while (doBackwardScan && beginPos >= 0) {
        QChar c = block.at(beginPos);
        if (isValidFileNameChar(c)) {
            buffer.prepend(c);
            beginPos--;
        } else {
            break;
        }
    }

    if (doBackwardScan
            && beginPos > 0
            && block.mid(beginPos - 1, pwd.count()) == pwd
            && (block.at(beginPos + pwd.count() - 1) == '/' || block.at(beginPos + pwd.count() - 1) == '\\')) {
        buffer.prepend("$$");
        beginPos -= 2;
    } else if (doBackwardScan
               && beginPos >= curlyPwd.count() - 1
               && block.mid(beginPos - curlyPwd.count() + 1, curlyPwd.count()) == curlyPwd) {
        buffer.prepend(pwd);
        beginPos -= curlyPwd.count();
    }

    // find the end of a filename
    while (endPos < block.count()) {
        QChar c = block.at(endPos);
        if (isValidFileNameChar(c)) {
            buffer.append(c);
            endPos++;
        } else {
            break;
        }
    }

    if (buffer.isEmpty())
        return Link();

    // remove trailing '\' since it can be line continuation char
    if (buffer.at(buffer.size() - 1) == QLatin1Char('\\')) {
        buffer.chop(1);
        endPos--;
    }

    // if the buffer starts with $$PWD accept it
    if (buffer.startsWith("$$PWD/") || buffer.startsWith("$$PWD\\"))
        buffer = buffer.mid(6);

    const QDir dir(m_editorWidget->textDocument()->filePath().toFileInfo().absolutePath());
    QString fileName = dir.filePath(buffer);
    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            QDir subDir(fi.absoluteFilePath());
            QString subProject = subDir.filePath(subDir.dirName() + QLatin1String(".pro"));
            if (QFileInfo::exists(subProject))
                fileName = subProject;
            else
                return Link();
        }

        Link link;
        link.targetFileName = QDir::cleanPath(fileName);
        link.linkTextStart = cursor.position() - positionInBlock + beginPos + 1;
        link.linkTextEnd = cursor.position() - positionInBlock + endPos;
        return link;
    }

    return Link();
}
