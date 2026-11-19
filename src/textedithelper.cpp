#include "textedithelper.h"

#include <QEvent>
#include <QKeyEvent>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QTextCursor>
#include <QTextDocument>

TextEditHelper::TextEditHelper(QObject *parent) : QObject(parent) {}

void TextEditHelper::setTarget(QQuickItem *target)
{
    if (m_target == target)
        return;
    if (m_target)
        m_target->removeEventFilter(this);
    m_target = target;
    if (m_target)
        m_target->installEventFilter(this);
    emit targetChanged();
}

void TextEditHelper::setCompletionEnabled(bool enabled)
{
    if (m_completionEnabled == enabled)
        return;
    m_completionEnabled = enabled;
    emit completionEnabledChanged();
}

bool TextEditHelper::replace(int start, int end, const QString &text)
{
    QQuickTextDocument *quickDocument = m_target
            ? m_target->property("textDocument").value<QQuickTextDocument *>() : nullptr;
    QTextDocument *doc = quickDocument ? quickDocument->textDocument() : nullptr;
    if (!doc || start < 0 || end < start || end >= doc->characterCount())
        return false;
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.insertText(text);
    cursor.endEditBlock();
    return true;
}

bool TextEditHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_target || !m_completionEnabled || !m_target || !m_target->hasActiveFocus()
            || event->type() != QEvent::KeyPress)
        return false;
    QKeyEvent *key = static_cast<QKeyEvent *>(event);
    if (key->key() != Qt::Key_Tab || key->modifiers() != Qt::NoModifier)
        return false;
    emit completionAccepted();
    key->accept();
    return true;
}
