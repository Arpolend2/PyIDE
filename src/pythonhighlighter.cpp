#include "pythonhighlighter.h"

#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

namespace {
const int NonCodeProperty = QTextFormat::UserProperty + 1;
const QStringList keywords = {
    QStringLiteral("False"), QStringLiteral("None"), QStringLiteral("True"),
    QStringLiteral("and"), QStringLiteral("as"), QStringLiteral("assert"), QStringLiteral("async"),
    QStringLiteral("await"), QStringLiteral("break"), QStringLiteral("class"), QStringLiteral("continue"),
    QStringLiteral("def"), QStringLiteral("del"), QStringLiteral("elif"), QStringLiteral("else"),
    QStringLiteral("except"), QStringLiteral("finally"), QStringLiteral("for"), QStringLiteral("from"),
    QStringLiteral("global"), QStringLiteral("if"), QStringLiteral("import"), QStringLiteral("in"),
    QStringLiteral("is"), QStringLiteral("lambda"), QStringLiteral("nonlocal"), QStringLiteral("not"),
    QStringLiteral("or"), QStringLiteral("pass"), QStringLiteral("raise"), QStringLiteral("return"),
    QStringLiteral("try"), QStringLiteral("while"), QStringLiteral("with"), QStringLiteral("yield"),
    QStringLiteral("match"), QStringLiteral("case")
};
const QStringList builtins = {
    QStringLiteral("abs"), QStringLiteral("all"), QStringLiteral("any"), QStringLiteral("bool"),
    QStringLiteral("dict"), QStringLiteral("enumerate"), QStringLiteral("float"), QStringLiteral("input"),
    QStringLiteral("int"), QStringLiteral("len"), QStringLiteral("list"), QStringLiteral("max"),
    QStringLiteral("min"), QStringLiteral("open"), QStringLiteral("print"), QStringLiteral("range"),
    QStringLiteral("set"), QStringLiteral("sorted"), QStringLiteral("str"), QStringLiteral("sum"),
    QStringLiteral("tuple"), QStringLiteral("type"), QStringLiteral("zip")
};
const QStringList templates = {
    QStringLiteral("print()"), QStringLiteral("for item in items:"),
    QStringLiteral("if condition:"), QStringLiteral("while condition:"),
    QStringLiteral("def function():"), QStringLiteral("class ClassName:"),
    QStringLiteral("try:"), QStringLiteral("with open('file.txt') as file:")
};
bool isWord(QChar c) { return c.isLetterOrNumber() || c == QLatin1Char('_'); }
}

PythonHighlighter::PythonHighlighter(QObject *parent) : QObject(parent) {}
PythonHighlighter::~PythonHighlighter() { delete m_highlighter.data(); }

void PythonHighlighter::setTextDocument(QQuickTextDocument *document)
{
    if (m_quickDocument == document)
        return;
    delete m_highlighter.data();
    m_quickDocument = document;
    if (document && document->textDocument()) {
        m_highlighter = new PythonSyntaxHighlighter(document->textDocument());
        m_highlighter->lightBackground = m_lightBackground;
        m_highlighter->rehighlight();
    }
    emit textDocumentChanged();
}

void PythonHighlighter::setLightBackground(bool light)
{
    if (m_lightBackground == light)
        return;
    m_lightBackground = light;
    if (m_highlighter) {
        m_highlighter->lightBackground = light;
        m_highlighter->rehighlight();
    }
    emit lightBackgroundChanged();
}

bool PythonHighlighter::isCodePosition(int position)
{
    if (!m_highlighter || !m_highlighter->document() || position < 0
            || position >= m_highlighter->document()->characterCount() - 1)
        return false;
    const QTextBlock block = m_highlighter->document()->findBlock(position);
    if (!block.isValid() || !block.layout())
        return false;
    m_highlighter->rehighlightBlock(block);
    const int offset = position - block.position();
    for (const QTextLayout::FormatRange &range : block.layout()->formats()) {
        if (offset >= range.start && offset < range.start + range.length
                && range.format.boolProperty(NonCodeProperty))
            return false;
    }
    return true;
}

QStringList PythonHighlighter::completions(const QString &prefix) const
{
    if (prefix.isEmpty() || !m_highlighter || !m_highlighter->document())
        return {};
    QStringList candidates = templates + keywords + builtins;
    QSet<QString> seen = QSet<QString>::fromList(candidates);
    static const QRegularExpression identifiers(QStringLiteral("\\b[A-Za-z_][A-Za-z_0-9]*\\b"));
    QRegularExpressionMatchIterator matches = identifiers.globalMatch(m_highlighter->document()->toPlainText());
    while (matches.hasNext()) {
        const QString word = matches.next().captured();
        if (!seen.contains(word)) {
            seen.insert(word);
            candidates.append(word);
        }
    }
    QStringList result;
    for (const QString &word : candidates) {
        if (word.startsWith(prefix, Qt::CaseSensitive) && word != prefix) {
            result.append(word);
            if (result.size() == 5)
                break;
        }
    }
    return result;
}

void PythonSyntaxHighlighter::highlightBlock(const QString &text)
{
    QTextCharFormat keyword, builtin, number, string, comment;
    keyword.setForeground(QColor(lightBackground ? "#7040a0" : "#c792ea"));
    keyword.setFontWeight(QFont::DemiBold);
    builtin.setForeground(QColor(lightBackground ? "#005cc5" : "#82b1ff"));
    number.setForeground(QColor(lightBackground ? "#a04400" : "#ffbd72"));
    string.setForeground(QColor(lightBackground ? "#a31515" : "#ecc48d"));
    string.setProperty(NonCodeProperty, true);
    comment.setForeground(QColor(lightBackground ? "#216e39" : "#83c98b"));
    comment.setFontItalic(true);
    comment.setProperty(NonCodeProperty, true);
    static const QRegularExpression numbers(QStringLiteral(
        "(?:0[xX][0-9a-fA-F_]+|0[bB][01_]+|0[oO][0-7_]+|"
        "(?:[0-9][0-9_]*)(?:\\.[0-9_]*)?(?:[eE][+-]?[0-9_]+)?[jJ]?)"));
    int state = previousBlockState() > 0 ? previousBlockState() : 0;
    int i = 0;
    while (i < text.size()) {
        int start = i;
        if (!state && text.at(i) == QLatin1Char('#')) {
            setFormat(i, text.size() - i, comment);
            break;
        }
        if (!state && (text.at(i).isLetter() || text.at(i) == QLatin1Char('_'))) {
            while (i < text.size() && isWord(text.at(i)))
                ++i;
            const QString word = text.mid(start, i - start);
            const QString lower = word.toLower();
            if (i < text.size() && (text.at(i) == QLatin1Char('"') || text.at(i) == QLatin1Char('\''))
                    && (lower == QLatin1String("r") || lower == QLatin1String("u")
                        || lower == QLatin1String("b") || lower == QLatin1String("f")
                        || lower == QLatin1String("br") || lower == QLatin1String("rb")
                        || lower == QLatin1String("fr") || lower == QLatin1String("rf"))) {
                // Include the prefix in the string formatting below.
            } else {
                if (keywords.contains(word))
                    setFormat(start, i - start, keyword);
                else if (builtins.contains(word))
                    setFormat(start, i - start, builtin);
                continue;
            }
        }
        if (state || (i < text.size() && (text.at(i) == QLatin1Char('"') || text.at(i) == QLatin1Char('\'')))) {
            const QChar quote = state == 1 ? QLatin1Char('\'') : state == 2 ? QLatin1Char('"') : text.at(i);
            bool triple = state != 0;
            if (!state) {
                triple = i + 2 < text.size() && text.at(i + 1) == quote && text.at(i + 2) == quote;
                i += triple ? 3 : 1;
            }
            bool closed = false;
            while (i < text.size()) {
                if (text.at(i) == QLatin1Char('\\') && i + 1 < text.size()) {
                    i += 2;
                } else if (text.at(i) == quote
                           && (!triple || (i + 2 < text.size() && text.at(i + 1) == quote
                                           && text.at(i + 2) == quote))) {
                    i += triple ? 3 : 1;
                    closed = true;
                    break;
                } else {
                    ++i;
                }
            }
            state = triple && !closed ? (quote == QLatin1Char('\'') ? 1 : 2) : 0;
            setFormat(start, i - start, string);
        } else if (i < text.size() && text.at(i).isDigit()) {
            const QRegularExpressionMatch match = numbers.match(text, i, QRegularExpression::NormalMatch,
                                                                 QRegularExpression::AnchoredMatchOption);
            if (match.hasMatch()) {
                i += match.capturedLength();
                setFormat(start, i - start, number);
            } else {
                ++i;
            }
        } else {
            ++i;
        }
    }
    setCurrentBlockState(state);
}
