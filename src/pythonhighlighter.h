#pragma once

#include <QColor>
#include <QPointer>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>

class PythonSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    explicit PythonSyntaxHighlighter(QTextDocument *document) : QSyntaxHighlighter(document) {}
    bool lightBackground = false;
protected:
    void highlightBlock(const QString &text) override;
};

class PythonHighlighter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(bool lightBackground READ lightBackground WRITE setLightBackground NOTIFY lightBackgroundChanged)
public:
    explicit PythonHighlighter(QObject *parent = nullptr);
    ~PythonHighlighter() override;
    QQuickTextDocument *textDocument() const { return m_quickDocument.data(); }
    void setTextDocument(QQuickTextDocument *document);
    bool lightBackground() const { return m_lightBackground; }
    void setLightBackground(bool light);
    Q_INVOKABLE bool isCodePosition(int position);
    Q_INVOKABLE QStringList completions(const QString &prefix) const;

signals:
    void textDocumentChanged();
    void lightBackgroundChanged();

private:
    QPointer<QQuickTextDocument> m_quickDocument;
    QPointer<PythonSyntaxHighlighter> m_highlighter;
    bool m_lightBackground = false;
};
