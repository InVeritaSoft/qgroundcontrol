/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CustomFirmwarePluginFactory.h"
#include "CustomFirmwarePlugin.h"
#include "../../Utilities/QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CustomFirmwarePluginFactoryLog, "CustomFirmwarePluginFactoryLog")

// Global static instance to register the factory
CustomFirmwarePluginFactory CustomFirmwarePluginFactory;

CustomFirmwarePluginFactory::CustomFirmwarePluginFactory()
{
    qCDebug(CustomFirmwarePluginFactoryLog) << "CustomFirmwarePluginFactory created";
}

QList<QGCMAVLink::FirmwareClass_t> CustomFirmwarePluginFactory::supportedFirmwareClasses() const
{
    QList<QGCMAVLink::FirmwareClass_t> list;
    list.append(QGCMAVLink::FirmwareClassPX4);
    return list;
}

FirmwarePlugin* CustomFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    Q_UNUSED(vehicleType)
    
    if (autopilotType == MAV_AUTOPILOT_PX4) {
        qCDebug(CustomFirmwarePluginFactoryLog) << "Creating CustomFirmwarePlugin for PX4 autopilot";
        return new CustomFirmwarePlugin();
    }
    
    // For non-PX4 autopilots, return nullptr to use default plugins
    return nullptr;
}
