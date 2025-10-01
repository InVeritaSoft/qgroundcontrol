/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "../../FirmwarePlugin/PX4/PX4FirmwarePlugin.h"

class CustomFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT

public:
    CustomFirmwarePlugin();

    // Override vehicle image to make ID 1 golden
    QString vehicleImageOpaque(const Vehicle* vehicle) const override;
    QString vehicleImageOutline(const Vehicle* vehicle) const override;
};
