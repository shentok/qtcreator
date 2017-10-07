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

#include "profileeditor.h"

#include "profilecompletionassist.h"
#include "profilehighlighter.h"
#include "profilehoverhandler.h"
#include "profilelinkfinder.h"
#include "qmakeprojectmanager.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagerconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <extensionsystem/pluginmanager.h>
#include <qtsupport/qtsupportconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textdocument.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

using namespace TextEditor;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class ProFileEditorWidget : public TextEditorWidget
{
public:
    ProFileEditorWidget();

protected:
    void contextMenuEvent(QContextMenuEvent *) override;
};

ProFileEditorWidget::ProFileEditorWidget()
{
    addHoverHandler(new ProFileHoverHandler(this));
    setLinkFinder(new ProFileLinkFinder(this));
}

void ProFileEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

static TextDocument *createProFileDocument()
{
    auto doc = new TextDocument;
    doc->setId(Constants::PROFILE_EDITOR_ID);
    doc->setMimeType(QLatin1String(Constants::PROFILE_MIMETYPE));
    // qmake project files do not support UTF8-BOM
    // If the BOM would be added qmake would fail and Qt Creator couldn't parse the project file
    doc->setSupportsUtf8Bom(false);
    return doc;
}

//
// ProFileEditorFactory
//

ProFileEditorFactory::ProFileEditorFactory()
{
    setId(Constants::PROFILE_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::PROFILE_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::PROFILE_MIMETYPE);
    addMimeType(Constants::PROINCLUDEFILE_MIMETYPE);
    addMimeType(Constants::PROFEATUREFILE_MIMETYPE);
    addMimeType(Constants::PROCONFIGURATIONFILE_MIMETYPE);
    addMimeType(Constants::PROCACHEFILE_MIMETYPE);
    addMimeType(Constants::PROSTASHFILE_MIMETYPE);

    setDocumentCreator(createProFileDocument);
    setEditorWidgetCreator([]() { return new ProFileEditorWidget; });

    setCompletionAssistProvider(new KeywordsCompletionAssistProvider(qmakeKeywords()));

    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setEditorActionHandlers(TextEditorActionHandler::UnCommentSelection
                | TextEditorActionHandler::JumpToFileUnderCursor);

    setSyntaxHighlighterCreator([]() { return new ProFileHighlighter; });

    const QString defaultOverlay = QLatin1String(ProjectExplorer::Constants::FILEOVERLAY_QT);
    Core::FileIconProvider::registerIconOverlayForSuffix(
                creatorTheme()->imageFile(Theme::IconOverlayPro, defaultOverlay), "pro");
    Core::FileIconProvider::registerIconOverlayForSuffix(
                creatorTheme()->imageFile(Theme::IconOverlayPri, defaultOverlay), "pri");
    Core::FileIconProvider::registerIconOverlayForSuffix(
                creatorTheme()->imageFile(Theme::IconOverlayPrf, defaultOverlay), "prf");
}

} // namespace Internal
} // namespace QmakeProjectManager
