/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "../../pch.h"
#include <QtCore/QStringList>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>
#include "../AreaPlanEditor.h"

// QGroundControl includes
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MissionManager.h"
#include "MissionItem.h"
#include "MissionController.h"

/**
 * @file AreaPlanEditor_Validation.cc
 * @brief Validation and error handling for AreaPlanEditor
 * 
 * This file contains all validation, error handling, and input
 * validation methods for the AreaPlanEditor class.
 */

// Validation methods
bool AreaPlanEditor::validateAreaParameters()
{
    QStringList errors;
    
    // Validate area center
    if (!areaCenter().isValid()) {
        errors << "Area center must be a valid coordinate";
    }
    
    // Validate area dimensions
    if (areaWidth() <= 0) {
        errors << "Area width must be greater than 0";
    }
    
    if (areaHeight() <= 0) {
        errors << "Area height must be greater than 0";
    }
    
    // Validate line spacing
    if (lineSpacing() <= 0) {
        errors << "Line spacing must be greater than 0";
    }
    
    if (lineSpacing() > areaWidth() && lineSpacing() > areaHeight()) {
        errors << "Line spacing is too large for the area dimensions";
    }
    
    // Validate number of points
    if (numPoints() <= 0) {
        errors << "Number of points must be greater than 0";
    }
    
    // Validate mission altitude
    if (missionAltitude() < 0) {
        errors << "Mission altitude must be non-negative";
    }
    
    if (missionAltitude() > 1000) {
        errors << "Mission altitude is very high - please verify this is correct";
    }
    
    // Validate area rotation
    if (areaRotation() < -360 || areaRotation() > 360) {
        errors << "Area rotation must be between -360 and 360 degrees";
    }
    
    // Validate home location
    if (homeLocation().isValid() && homeLocation().distanceTo(areaCenter()) > 10000) {
        errors << "Home location is very far from area center - please verify this is correct";
    }
    
    if (errors.isEmpty()) {
        clearValidationError();
        return true;
    } else {
        handleError("Area parameter validation failed", errors.join("; "));
        return false;
    }
}

bool AreaPlanEditor::validateWaypointGeneration()
{
    QStringList errors;
    
    // Check if area parameters are valid
    if (!validateAreaParameters()) {
        return false;
    }
    
    // Validate waypoint generation specific parameters
    if (numPoints() < 2 && areaWidth() > 0) {
        errors << "At least 2 points are recommended for waypoint generation";
    }
    
    // Check if line spacing is reasonable
    if (lineSpacing() > areaWidth() * 0.5) {
        errors << "Line spacing is very large compared to area width - this may result in poor coverage";
    }
    
    // Validate altitude banding parameters
    if (droneCount() > 1) {
        if (altitudeBandStep() <= 0) {
            errors << "Altitude band step must be greater than 0 for multi-drone missions";
        }
        
        if (altitudeBandStart() < 0) {
            errors << "Altitude band start must be non-negative";
        }
        
        // Check if altitude bands are reasonable
        qreal maxAltitude = missionAltitude() + altitudeBandStart() + ((droneCount() - 1) * altitudeBandStep());
        if (maxAltitude > 1000) {
            errors << "Maximum altitude for multi-drone mission is very high - please verify this is correct";
        }
    }
    
    if (errors.isEmpty()) {
        clearValidationError();
        return true;
    } else {
        handleError("Waypoint generation validation failed", errors.join("; "));
        return false;
    }
}

bool AreaPlanEditor::validateMissionUpload()
{
    QStringList errors;
    
    // Check if mission controller is available
    if (!planMasterController()) {
        errors << "No mission controller available";
    } else {
        MissionController* missionController = qobject_cast<MissionController*>(planMasterController());
        if (!missionController) {
            errors << "Invalid mission controller";
        } else {
            // Check if mission has waypoints
            if (missionController->visualItems() && missionController->visualItems()->count() == 0) {
                errors << "No waypoints in mission - please generate waypoints first";
            }
        }
    }
    
    // Check if vehicles are available
    MultiVehicleManager* vehicleManager = qgcApp()->toolbox()->multiVehicleManager();
    if (!vehicleManager) {
        errors << "Vehicle manager not available";
    } else {
        int connectedVehicles = 0;
        for (int i = 0; i < vehicleManager->vehicles()->count(); ++i) {
            Vehicle* vehicle = qobject_cast<Vehicle*>(vehicleManager->vehicles()->get(i));
            if (vehicle && vehicle->connected()) {
                connectedVehicles++;
            }
        }
        
        if (connectedVehicles == 0) {
            errors << "No connected vehicles available for mission upload";
        }
        
        if (droneCount() > 0 && connectedVehicles < droneCount()) {
            errors << QString("Only %1 vehicles connected, but %2 drones configured")
                      .arg(connectedVehicles)
                      .arg(droneCount());
        }
    }
    
    if (errors.isEmpty()) {
        clearValidationError();
        return true;
    } else {
        handleError("Mission upload validation failed", errors.join("; "));
        return false;
    }
}

bool AreaPlanEditor::validateMissionFileSaving()
{
    QStringList errors;
    
    // Check if mission controller is available
    if (!planMasterController()) {
        errors << "No mission controller available";
    } else {
        MissionController* missionController = qobject_cast<MissionController*>(planMasterController());
        if (!missionController) {
            errors << "Invalid mission controller";
        } else {
            // Check if mission has waypoints
            if (missionController->visualItems() && missionController->visualItems()->count() == 0) {
                errors << "No waypoints in mission - please generate waypoints first";
            }
        }
    }
    
    // Check if area parameters are valid
    if (!validateAreaParameters()) {
        return false;
    }
    
    if (errors.isEmpty()) {
        clearValidationError();
        return true;
    } else {
        handleError("Mission file saving validation failed", errors.join("; "));
        return false;
    }
}

bool AreaPlanEditor::validateSwarmConfiguration() const
{
    QStringList errors;
    
    // Validate drone count
    if (droneCount() <= 0) {
        errors << "Drone count must be greater than 0";
    }
    
    if (droneCount() > 10) {
        errors << "Drone count is very high - please verify this is correct";
    }
    
    // Validate altitude banding
    if (droneCount() > 1) {
        if (altitudeBandStep() <= 0) {
            errors << "Altitude band step must be greater than 0 for multi-drone missions";
        }
        
        if (altitudeBandStart() < 0) {
            errors << "Altitude band start must be non-negative";
        }
        
        // Check if altitude bands are reasonable
        qreal maxAltitude = missionAltitude() + altitudeBandStart() + ((droneCount() - 1) * altitudeBandStep());
        if (maxAltitude > 1000) {
            errors << "Maximum altitude for swarm mission is very high - please verify this is correct";
        }
    }
    
    // Validate timing parameters
    if (timeOffsetPerDrone() < 0) {
        errors << "Time offset per drone must be non-negative";
    }
    
    if (perTargetSeparationS() < 0) {
        errors << "Per target separation must be non-negative";
    }
    
    // Validate formation parameters
    if (formationSpacing() < 1.0) {
        errors << "Formation spacing must be at least 1 meter";
    }
    
    if (formationSpacing() > 100) {
        errors << "Formation spacing is very large - please verify this is correct";
    }
    
    if (errors.isEmpty()) {
        return true;
    } else {
        qDebug() << "Swarm configuration validation failed:" << errors.join("; ");
        return false;
    }
}

QString AreaPlanEditor::validateInput(const QString& fieldName, const QVariant& value) const
{
    if (fieldName == "areaWidth") {
        qreal width = value.toReal();
        if (width <= 0) {
            return "Area width must be greater than 0";
        }
        if (width > 10000) {
            return "Area width is very large - please verify this is correct";
        }
    } else if (fieldName == "areaHeight") {
        qreal height = value.toReal();
        if (height <= 0) {
            return "Area height must be greater than 0";
        }
        if (height > 10000) {
            return "Area height is very large - please verify this is correct";
        }
    } else if (fieldName == "lineSpacing") {
        qreal spacing = value.toReal();
        if (spacing <= 0) {
            return "Line spacing must be greater than 0";
        }
        if (spacing > 1000) {
            return "Line spacing is very large - please verify this is correct";
        }
    } else if (fieldName == "numPoints") {
        int points = value.toInt();
        if (points <= 0) {
            return "Number of points must be greater than 0";
        }
        if (points > 1000) {
            return "Number of points is very high - this may cause performance issues";
        }
    } else if (fieldName == "missionAltitude") {
        qreal altitude = value.toReal();
        if (altitude < 0) {
            return "Mission altitude must be non-negative";
        }
        if (altitude > 1000) {
            return "Mission altitude is very high - please verify this is correct";
        }
    } else if (fieldName == "droneCount") {
        int count = value.toInt();
        if (count <= 0) {
            return "Drone count must be greater than 0";
        }
        if (count > 10) {
            return "Drone count is very high - please verify this is correct";
        }
    } else if (fieldName == "altitudeBandStep") {
        qreal step = value.toReal();
        if (step <= 0) {
            return "Altitude band step must be greater than 0";
        }
        if (step > 100) {
            return "Altitude band step is very large - please verify this is correct";
        }
    } else if (fieldName == "formationSpacing") {
        qreal spacing = value.toReal();
        if (spacing < 1.0) {
            return "Formation spacing must be at least 1 meter";
        }
        if (spacing > 100) {
            return "Formation spacing is very large - please verify this is correct";
        }
    }
    
    return QString(); // No error
}

bool AreaPlanEditor::isInputValid(const QString& fieldName, const QVariant& value) const
{
    return validateInput(fieldName, value).isEmpty();
}

QString AreaPlanEditor::getValidationError() const
{
    return validationError();
}

void AreaPlanEditor::clearValidationError()
{
    if (validationError().isEmpty()) {
        return;
    }
    
    // Note: This method needs to clear the validation error, but since we're using getters,
    // we need to access the private member directly or add a setter method
    // For now, we'll leave this as a placeholder that needs to be implemented in the main class
    emit validationErrorChanged();
}

// Error handling methods
void AreaPlanEditor::logError(const QString& errorMessage, const QString& context)
{
    QString fullMessage = context.isEmpty() ? errorMessage : QString("%1: %2").arg(context, errorMessage);
    qDebug() << "AreaPlanEditor Error:" << fullMessage;
    
    // Log to file or other logging system if needed
    // This is a placeholder for more sophisticated logging
}

void AreaPlanEditor::handleError(const QString& errorMessage, const QString& recoverySuggestion)
{
    QString fullMessage = errorMessage;
    if (!recoverySuggestion.isEmpty()) {
        fullMessage += QString(" Suggestion: %1").arg(recoverySuggestion);
    }
    
    // Note: This method needs to set the validation error, but since we're using getters,
    // we need to access the private member directly or add a setter method
    // For now, we'll leave this as a placeholder that needs to be implemented in the main class
    emit validationErrorChanged();
    
    logError(errorMessage, "AreaPlanEditor");
    
    // Emit error signal for QML handling
    emit errorOccurred(errorMessage, recoverySuggestion);
}
