/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl
import QGroundControl.FlightDisplay

GuidedToolStripAction {
    text: _guidedController.explodeTitle
    iconSource: "/res/Stop.svg"
    visible: true
    enabled: _activeVehicle && _activeVehicle.id === 1
    actionID: _guidedController.actionExplode
}
