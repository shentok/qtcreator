/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fakevimplugin.h"

#include "fakevimhandler.h"
#include "ui_fakevimoptions.h"


#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/completionsupport.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/interactionsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textblockiterator.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <indenter.h>

#include <QtCore/QDebug>
#include <QtCore/QtPlugin>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSettings>

#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>


using namespace FakeVim::Internal;
using namespace TextEditor;
using namespace Core;
using namespace ProjectExplorer;


namespace FakeVim {
namespace Constants {

const char * const INSTALL_HANDLER        = "TextEditor.FakeVimHandler";
const char * const MINI_BUFFER            = "TextEditor.FakeVimMiniBuffer";
const char * const INSTALL_KEY            = "Alt+V,Alt+V";

} // namespace Constants
} // namespace FakeVim


///////////////////////////////////////////////////////////////////////
//
// FakeVimOptionPage
//
///////////////////////////////////////////////////////////////////////

namespace FakeVim {
namespace Internal {

class FakeVimOptionPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    FakeVimOptionPage() {}

    // IOptionsPage
    QString id() const { return QLatin1String("General"); }
    QString trName() const { return tr("General"); }
    QString category() const { return QLatin1String("FakeVim"); }
    QString trCategory() const { return tr("FakeVim"); }

    QWidget *createPage(QWidget *parent);
    void apply() { m_group.apply(ICore::instance()->settings()); }
    void finish() { m_group.finish(); }

private slots:
    void copyTextEditorSettings();
    void setQtStyle();
    void setPlainStyle();

private:
    friend class DebuggerPlugin;
    Ui::FakeVimOptionPage m_ui;

    Core::Utils::SavedActionSet m_group;
};

QWidget *FakeVimOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_group.clear();
    m_group.insert(theFakeVimSetting(ConfigUseFakeVim), 
        m_ui.checkBoxUseFakeVim);

    m_group.insert(theFakeVimSetting(ConfigExpandTab), 
        m_ui.checkBoxExpandTab);
    m_group.insert(theFakeVimSetting(ConfigHlSearch), 
        m_ui.checkBoxHlSearch);
    m_group.insert(theFakeVimSetting(ConfigShiftWidth), 
        m_ui.lineEditShiftWidth);

    m_group.insert(theFakeVimSetting(ConfigSmartTab), 
        m_ui.checkBoxSmartTab);
    m_group.insert(theFakeVimSetting(ConfigStartOfLine), 
        m_ui.checkBoxStartOfLine);
    m_group.insert(theFakeVimSetting(ConfigTabStop), 
        m_ui.lineEditTabStop);
    m_group.insert(theFakeVimSetting(ConfigBackspace), 
        m_ui.lineEditBackspace);

    m_group.insert(theFakeVimSetting(ConfigAutoIndent), 
        m_ui.checkBoxAutoIndent);

    connect(m_ui.pushButtonCopyTextEditorSettings, SIGNAL(clicked()),
        this, SLOT(copyTextEditorSetting()));
    connect(m_ui.pushButtonSetQtStyle, SIGNAL(clicked()),
        this, SLOT(setQtStyle()));
    connect(m_ui.pushButtonSetPlainStyle, SIGNAL(clicked()),
        this, SLOT(setPlainStyle()));

    return w;
}

void FakeVimOptionPage::copyTextEditorSettings()
{
    TextEditor::TabSettings ts = 
        TextEditor::TextEditorSettings::instance()->tabSettings();
    
    m_ui.checkBoxExpandTab->setChecked(ts.m_spacesForTabs);
    m_ui.lineEditTabStop->setText(QString::number(ts.m_tabSize));
    m_ui.lineEditShiftWidth->setText(QString::number(ts.m_indentSize));
    m_ui.checkBoxSmartTab->setChecked(ts.m_smartBackspace);
    m_ui.checkBoxAutoIndent->setChecked(ts.m_autoIndent);
}

void FakeVimOptionPage::setQtStyle()
{
    m_ui.checkBoxExpandTab->setChecked(true);
    m_ui.lineEditTabStop->setText("4");
    m_ui.lineEditShiftWidth->setText("4");
    m_ui.checkBoxSmartTab->setChecked(true);
    m_ui.checkBoxAutoIndent->setChecked(true);
}

void FakeVimOptionPage::setPlainStyle()
{
    m_ui.checkBoxExpandTab->setChecked(false);
    m_ui.lineEditTabStop->setText("8");
    m_ui.lineEditShiftWidth->setText("8");
    m_ui.checkBoxSmartTab->setChecked(false);
    m_ui.checkBoxAutoIndent->setChecked(false);
}

} // namespace Internal
} // namespace FakeVim


///////////////////////////////////////////////////////////////////////
//
// FakeVimPluginPrivate
//
///////////////////////////////////////////////////////////////////////

namespace FakeVim {
namespace Internal {

class FakeVimPluginPrivate : public QObject
{
    Q_OBJECT

public:
    FakeVimPluginPrivate(FakeVimPlugin *);
    ~FakeVimPluginPrivate();
    friend class FakeVimPlugin;

    bool initialize();
    void shutdown();

private slots:
    void editorOpened(Core::IEditor *);
    void editorAboutToClose(Core::IEditor *);

    void installHandlerOnCurrentEditor();
    void installHandler(Core::IEditor *editor);
    void removeHandler();
    void triggerCompletions();

    void showCommandBuffer(const QString &contents);
    void showExtraInformation(const QString &msg);
    void changeSelection(const QList<QTextEdit::ExtraSelection> &selections);
    void writeFile(bool *handled, const QString &fileName, const QString &contents);
    void moveToMatchingParenthesis(bool *moved, bool *forward, QTextCursor *cursor);
    void indentRegion(int *amount, int beginLine, int endLine,  QChar typedChar);

private:
    FakeVimPlugin *q;
    QAction *m_installHandlerAction; 
};

} // namespace Internal
} // namespace FakeVim

FakeVimPluginPrivate::FakeVimPluginPrivate(FakeVimPlugin *plugin)
{       
    q = plugin;
    m_installHandlerAction = 0;
}

FakeVimPluginPrivate::~FakeVimPluginPrivate()
{
}

void FakeVimPluginPrivate::shutdown()
{
}

bool FakeVimPluginPrivate::initialize()
{
    Core::ActionManager *actionManager = Core::ICore::instance()->actionManager();
    QTC_ASSERT(actionManager, return false);

    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    m_installHandlerAction = new QAction(this);
    m_installHandlerAction->setText(tr("Set vi-Style Keyboard Action Handler"));
    
    Core::Command *cmd = 0;
    cmd = actionManager->registerAction(m_installHandlerAction,
        Constants::INSTALL_HANDLER, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(Constants::INSTALL_KEY));

    ActionContainer *advancedMenu =
        actionManager->actionContainer(Core::Constants::M_EDIT_ADVANCED);
    advancedMenu->addAction(cmd, Core::Constants::G_EDIT_EDITOR);

    connect(m_installHandlerAction, SIGNAL(triggered()),
        this, SLOT(installHandlerOnCurrentEditor()));

    // EditorManager
    QObject *editorManager = Core::ICore::instance()->editorManager();
    connect(editorManager, SIGNAL(editorAboutToClose(Core::IEditor*)),
        this, SLOT(editorAboutToClose(Core::IEditor*)));
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        this, SLOT(editorOpened(Core::IEditor*)));

    return true;
}

void FakeVimPluginPrivate::installHandler(Core::IEditor *editor)
{
    if (!editor)
        return;

    QWidget *widget = editor->widget();
    if (!widget)
        return;

    // we can only handle QTextEdit and QPlainTextEdit
    if (!qobject_cast<QTextEdit *>(widget) && !qobject_cast<QPlainTextEdit *>(widget))
        return;
    
    FakeVimHandler *handler = new FakeVimHandler(widget, widget);

    connect(handler, SIGNAL(extraInformationChanged(QString)),
        this, SLOT(showExtraInformation(QString)));
    connect(handler, SIGNAL(commandBufferChanged(QString)),
        this, SLOT(showCommandBuffer(QString)));
    connect(handler, SIGNAL(quitRequested()),
        this, SLOT(removeHandler()), Qt::QueuedConnection);
    connect(handler, SIGNAL(writeFileRequested(bool*,QString,QString)),
        this, SLOT(writeFile(bool*,QString,QString)));
    connect(handler, SIGNAL(selectionChanged(QList<QTextEdit::ExtraSelection>)),
        this, SLOT(changeSelection(QList<QTextEdit::ExtraSelection>)));
    connect(handler, SIGNAL(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)),
        this, SLOT(moveToMatchingParenthesis(bool*,bool*,QTextCursor*)));
    connect(handler, SIGNAL(indentRegion(int*,int,int,QChar)),
        this, SLOT(indentRegion(int*,int,int,QChar)));
    connect(handler, SIGNAL(completionRequested()),
        this, SLOT(triggerCompletions()));

    handler->setupWidget();
    handler->setExtraData(editor);

    if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(widget)) {
        using namespace TextEditor;
        using namespace FakeVim::Constants;
        handler->setCurrentFileName(editor->file()->fileName());
        TabSettings settings = bt->tabSettings();
    }
}

void FakeVimPluginPrivate::installHandlerOnCurrentEditor()
{
    installHandler(EditorManager::instance()->currentEditor());
}

void FakeVimPluginPrivate::triggerCompletions()
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;
    if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget()))
        TextEditor::Internal::CompletionSupport::instance()->
            autoComplete(bt->editableInterface(), false);
   //     bt->triggerCompletions();
}

void FakeVimPluginPrivate::writeFile(bool *handled,
    const QString &fileName, const QString &contents)
{
    Q_UNUSED(contents);

    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    Core::IEditor *editor = qobject_cast<Core::IEditor *>(handler->extraData());
    if (editor && editor->file()->fileName() == fileName) {
        // Handle that as a special case for nicer interaction with core
        Core::IFile *file = editor->file();
        Core::ICore::instance()->fileManager()->blockFileChange(file);
        file->save(fileName);
        Core::ICore::instance()->fileManager()->unblockFileChange(file);
        *handled = true;
    } 
}

void FakeVimPluginPrivate::moveToMatchingParenthesis(bool *moved, bool *forward,
        QTextCursor *cursor)
{
    *moved = false;

    bool undoFakeEOL = false;
    if (cursor->atBlockEnd() && cursor->block().length() > 1) {
        cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
        undoFakeEOL = true;
    }
    TextEditor::TextBlockUserData::MatchType match
        = TextEditor::TextBlockUserData::matchCursorForward(cursor);
    if (match == TextEditor::TextBlockUserData::Match) {
        *moved = true;
        *forward = true;
   } else {
        if (undoFakeEOL)
            cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
        if (match == TextEditor::TextBlockUserData::NoMatch) {
            // backward matching is according to the character before the cursor
            bool undoMove = false;
            if (!cursor->atBlockEnd()) {
                cursor->movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                undoMove = true;
            }
            match = TextEditor::TextBlockUserData::matchCursorBackward(cursor);
            if (match == TextEditor::TextBlockUserData::Match) {
                *moved = true;
                *forward = false;
            } else if (undoMove) {
                cursor->movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            }
        }
    }
}

void FakeVimPluginPrivate::indentRegion(int *amount, int beginLine, int endLine,
      QChar typedChar)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (!handler)
        return;

    BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget());
    if (!bt)
        return;

    TextEditor::TabSettings tabSettings = 
        TextEditor::TextEditorSettings::instance()->tabSettings();
    typedef SharedTools::Indenter<TextEditor::TextBlockIterator> Indenter;
    Indenter &indenter = Indenter::instance();
    indenter.setIndentSize(tabSettings.m_indentSize);
    indenter.setTabSize(tabSettings.m_tabSize);

    const QTextDocument *doc = bt->document();
    QTextBlock begin = doc->findBlockByNumber(beginLine);
    QTextBlock end = doc->findBlockByNumber(endLine);
    const TextEditor::TextBlockIterator docStart(doc->begin());
    QTextBlock cur = begin;
    do {
        if (typedChar == 0 && cur.text().simplified().isEmpty()) {
            *amount = 0;
            if (cur != end) {
                QTextCursor cursor(cur);
                while (!cursor.atBlockEnd())
                    cursor.deleteChar();
            }
        } else {
            const TextEditor::TextBlockIterator current(cur);
            const TextEditor::TextBlockIterator next(cur.next());
            *amount = indenter.indentForBottomLine(current, docStart, next, typedChar);
            if (cur != end)
                tabSettings.indentLine(cur, *amount);
        }
        if (cur != end)
           cur = cur.next();
    } while (cur != end);
}

void FakeVimPluginPrivate::removeHandler()
{
    if (FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender())) {
        handler->restoreWidget();
        handler->deleteLater();
    }
    Core::EditorManager::instance()->hideEditorStatusBar(
        QLatin1String(Constants::MINI_BUFFER));
}

void FakeVimPluginPrivate::editorOpened(Core::IEditor *editor)
{
    //qDebug() << "OPENING: " << editor << editor->widget();
    QSettings *s = ICore::instance()->settings();
    bool automatic = s->value("textInteractionSettings/UseVim").toBool();
    if (automatic)
       installHandler(editor);
}

void FakeVimPluginPrivate::editorAboutToClose(Core::IEditor *editor)
{
    //qDebug() << "CLOSING: " << editor << editor->widget();
    Q_UNUSED(editor);
}

void FakeVimPluginPrivate::showCommandBuffer(const QString &contents)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (handler) {
        //qDebug() << "SHOW COMMAND BUFFER" << contents;
        Core::EditorManager::instance()->showEditorStatusBar( 
            QLatin1String(Constants::MINI_BUFFER), contents,
            tr("Quit FakeVim"), handler, SLOT(quit()));
    } else {
        qDebug() << "\nNO HANDLER\n";
    }
}

void FakeVimPluginPrivate::showExtraInformation(const QString &text)
{
    FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender());
    if (handler)
        QMessageBox::information(handler->widget(), tr("FakeVim Information"), text);
}

void FakeVimPluginPrivate::changeSelection
    (const QList<QTextEdit::ExtraSelection> &selection)
{
    if (FakeVimHandler *handler = qobject_cast<FakeVimHandler *>(sender()))
        if (BaseTextEditor *bt = qobject_cast<BaseTextEditor *>(handler->widget()))
            bt->setExtraSelections(BaseTextEditor::FakeVimSelection, selection);
}


///////////////////////////////////////////////////////////////////////
//
// FakeVimPlugin
//
///////////////////////////////////////////////////////////////////////

FakeVimPlugin::FakeVimPlugin()
    : d(new FakeVimPluginPrivate(this))
{}

FakeVimPlugin::~FakeVimPlugin()
{
    delete d;
}

bool FakeVimPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);
    return d->initialize();
}

void FakeVimPlugin::shutdown()
{
    d->shutdown();
}

void FakeVimPlugin::extensionsInitialized()
{
}

#include "fakevimplugin.moc"

Q_EXPORT_PLUGIN(FakeVimPlugin)
