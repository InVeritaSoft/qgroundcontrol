/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ShortcutManager.h"
#include "QGCLoggingCategory.h"
#include <QDebug>
#include <QApplication>

QGC_LOGGING_CATEGORY(ShortcutManagerLog, "ShortcutManagerLog")

ShortcutManager::ShortcutManager(QObject* parent)
    : QObject(parent)
    , m_parent(parent)
{
    qCDebug(ShortcutManagerLog) << "ShortcutManager initialized";
}

ShortcutManager::~ShortcutManager()
{
    clearAllShortcuts();
    qCDebug(ShortcutManagerLog) << "ShortcutManager destroyed";
}

void ShortcutManager::registerShortcut(const QString& name, const QString& keySequence, const QString& description)
{
    qCDebug(ShortcutManagerLog) << "Registering shortcut:" << name << "with key sequence:" << keySequence;

    // Check if shortcut already exists
    if (m_shortcuts.contains(name)) {
        qCWarning(ShortcutManagerLog) << "Shortcut already exists:" << name << "- replacing";
        unregisterShortcut(name);
    }

    // Create new shortcut
    QShortcut* shortcut = new QShortcut(QKeySequence(keySequence), m_parent);
    shortcut->setContext(Qt::ApplicationShortcut); // Global shortcut
    
    // Connect to activation signal
    connect(shortcut, &QShortcut::activated, this, &ShortcutManager::onShortcutActivated);
    
    // Store shortcut info
    ShortcutInfo info;
    info.shortcut = shortcut;
    info.description = description;
    info.keySequence = keySequence;
    
    m_shortcuts[name] = info;
    
    qCDebug(ShortcutManagerLog) << "Successfully registered shortcut:" << name << "->" << keySequence;
    emit shortcutRegistered(name, keySequence);
}

void ShortcutManager::unregisterShortcut(const QString& name)
{
    qCDebug(ShortcutManagerLog) << "Unregistering shortcut:" << name;

    if (m_shortcuts.contains(name)) {
        ShortcutInfo info = m_shortcuts[name];
        info.shortcut->deleteLater();
        m_shortcuts.remove(name);
        
        qCDebug(ShortcutManagerLog) << "Successfully unregistered shortcut:" << name;
        emit shortcutUnregistered(name);
    } else {
        qCWarning(ShortcutManagerLog) << "Shortcut not found:" << name;
    }
}

void ShortcutManager::clearAllShortcuts()
{
    qCDebug(ShortcutManagerLog) << "Clearing all shortcuts, count:" << m_shortcuts.size();

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        it.value().shortcut->deleteLater();
    }
    m_shortcuts.clear();
    
    qCDebug(ShortcutManagerLog) << "All shortcuts cleared";
}

bool ShortcutManager::isShortcutRegistered(const QString& name) const
{
    return m_shortcuts.contains(name);
}

QStringList ShortcutManager::getRegisteredShortcuts() const
{
    return m_shortcuts.keys();
}

QString ShortcutManager::getShortcutDescription(const QString& name) const
{
    if (m_shortcuts.contains(name)) {
        return m_shortcuts[name].description;
    }
    return QString();
}

void ShortcutManager::registerMissionGenerationShortcuts()
{
    qCDebug(ShortcutManagerLog) << "Registering mission generation shortcuts";

    // Mission Generation Shortcuts
    registerShortcut("GenerateMission", "Ctrl+G", "Generate Mission");
    registerShortcut("OpenMissionDialog", "Ctrl+M", "Open Mission Generation Dialog");
    registerShortcut("TestCollisionAlert", "Ctrl+T", "Test Collision Alert");
    registerShortcut("StartCollisionMonitoring", "Ctrl+Shift+C", "Start Collision Monitoring");
    registerShortcut("StopCollisionMonitoring", "Ctrl+Shift+X", "Stop Collision Monitoring");
}

void ShortcutManager::registerVehicleControlShortcuts()
{
    qCDebug(ShortcutManagerLog) << "Registering vehicle control shortcuts";

    // Single Vehicle Control Shortcuts
    registerShortcut("ArmVehicle", "Ctrl+A", "Arm Vehicle");
    registerShortcut("DisarmVehicle", "Ctrl+D", "Disarm Vehicle");
    registerShortcut("Takeoff", "Ctrl+Shift+T", "Takeoff");
    registerShortcut("Land", "Ctrl+Shift+L", "Land");
    registerShortcut("RTL", "Ctrl+R", "Return to Launch");
    registerShortcut("EmergencyStop", "Ctrl+E", "Emergency Stop");

    // All Vehicles Control Shortcuts
    registerShortcut("SetAllVehiclesAUTO", "A", "Set All Vehicles to AUTO Mode");
    registerShortcut("ArmAllVehicles", "R", "Arm All Vehicles");
    registerShortcut("DisarmAllVehicles", "D", "Disarm All Vehicles");
    registerShortcut("LandAllVehicles", "L", "Land All Vehicles");
    registerShortcut("RTLAllVehicles", "H", "RTL All Vehicles");
}

void ShortcutManager::registerCollisionDetectionShortcuts()
{
    qCDebug(ShortcutManagerLog) << "Registering collision detection shortcuts";

    // Collision Detection Shortcuts
    registerShortcut("ShowCollisionAlert", "Ctrl+Alt+C", "Show Collision Alert");
    registerShortcut("ClearCollisionAlerts", "Ctrl+Alt+X", "Clear Collision Alerts");
    registerShortcut("ToggleCollisionMonitoring", "Ctrl+Alt+M", "Toggle Collision Monitoring");
}

void ShortcutManager::onShortcutActivated()
{
    QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
    if (!shortcut) {
        qCWarning(ShortcutManagerLog) << "Invalid shortcut sender";
        return;
    }

    // Find the shortcut name by comparing key sequences
    QString shortcutName;
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it.value().shortcut == shortcut) {
            shortcutName = it.key();
            break;
        }
    }

    if (shortcutName.isEmpty()) {
        qCWarning(ShortcutManagerLog) << "Shortcut not found in registry";
        return;
    }

    qCDebug(ShortcutManagerLog) << "Shortcut activated:" << shortcutName;
    emit shortcutTriggered(shortcutName);
}
