/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QShortcut>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(ShortcutManagerLog)

class ShortcutManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ShortcutManager(QObject* parent = nullptr);
    ~ShortcutManager();

    // QML-invokable methods
    Q_INVOKABLE void registerShortcut(const QString& name, const QString& keySequence, const QString& description = "");
    Q_INVOKABLE void unregisterShortcut(const QString& name);
    Q_INVOKABLE void clearAllShortcuts();
    Q_INVOKABLE bool isShortcutRegistered(const QString& name) const;
    Q_INVOKABLE QStringList getRegisteredShortcuts() const;
    Q_INVOKABLE QString getShortcutDescription(const QString& name) const;

    // Predefined shortcuts for common actions
    Q_INVOKABLE void registerMissionGenerationShortcuts();
    Q_INVOKABLE void registerVehicleControlShortcuts();
    Q_INVOKABLE void registerCollisionDetectionShortcuts();

signals:
    void shortcutTriggered(const QString& name);
    void shortcutRegistered(const QString& name, const QString& keySequence);
    void shortcutUnregistered(const QString& name);

private slots:
    void onShortcutActivated();

private:
    struct ShortcutInfo {
        QShortcut* shortcut;
        QString description;
        QString keySequence;
    };

    QMap<QString, ShortcutInfo> m_shortcuts;
    QObject* m_parent;
};
