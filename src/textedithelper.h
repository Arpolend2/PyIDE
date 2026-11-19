#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickItem>

class TextEditHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool completionEnabled READ completionEnabled WRITE setCompletionEnabled NOTIFY completionEnabledChanged)
public:
    explicit TextEditHelper(QObject *parent = nullptr);
    QQuickItem *target() const { return m_target.data(); }
    void setTarget(QQuickItem *target);
    bool completionEnabled() const { return m_completionEnabled; }
    void setCompletionEnabled(bool enabled);
    Q_INVOKABLE bool replace(int start, int end, const QString &text);
signals:
    void targetChanged();
    void completionEnabledChanged();
    void completionAccepted();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    QPointer<QQuickItem> m_target;
    bool m_completionEnabled = false;
};
