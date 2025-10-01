/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CustomFirmwarePlugin.h"
#include "../../Vehicle/Vehicle.h"
#include "../../Utilities/QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CustomFirmwarePluginLog, "CustomFirmwarePluginLog")

CustomFirmwarePlugin::CustomFirmwarePlugin()
{
    qCDebug(CustomFirmwarePluginLog) << "CustomFirmwarePlugin created";
}

QString CustomFirmwarePlugin::vehicleImageOpaque(const Vehicle* vehicle) const
{
    if (vehicle && vehicle->id() == 1) {
        qCDebug(CustomFirmwarePluginLog) << "Vehicle ID 1 detected - returning golden vehicle image";
        return QStringLiteral("/qmlimages/vehicleArrowOpaqueGolden.svg");
    }
    
    // For all other vehicles, use the default PX4 image
    return PX4FirmwarePlugin::vehicleImageOpaque(vehicle);
}

QString CustomFirmwarePlugin::vehicleImageOutline(const Vehicle* vehicle) const
{
    if (vehicle && vehicle->id() == 1) {
        qCDebug(CustomFirmwarePluginLog) << "Vehicle ID 1 detected - returning golden vehicle outline";
        return QStringLiteral("/qmlimages/vehicleArrowOutlineGolden.svg");
    }
    
    // For all other vehicles, use the default PX4 image
    return PX4FirmwarePlugin::vehicleImageOutline(vehicle);
}
