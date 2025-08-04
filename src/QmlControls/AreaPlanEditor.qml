/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
	id: _root

	Rectangle {
		id: background
		anchors.fill: parent
		color: qgcPal.window
		
		ScrollView {
			anchors.fill: parent
			anchors.margins: ScreenTools.defaultFontPixelHeight * 0.5
			clip: true
			ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
			ScrollBar.vertical.policy: ScrollBar.AsNeeded
			
			ColumnLayout {
				width: parent.width
				spacing: ScreenTools.defaultFontPixelHeight * 0.5
				
				QGCLabel {
					text: qsTr("I am Area Plan Editor")
					font.pointSize: ScreenTools.largeFontPointSize
					Layout.alignment: Qt.AlignHCenter
					Layout.fillWidth: true
					wrapMode: Text.WordWrap
				}
				
				// Add some sample content to demonstrate scrolling
				Repeater {
					model: 20
					
					Rectangle {
						Layout.fillWidth: true
						height: ScreenTools.defaultFontPixelHeight * 2
						color: index % 2 === 0 ? qgcPal.button : qgcPal.buttonHighlight
						radius: ScreenTools.defaultFontPixelWidth * 0.25
						
						QGCLabel {
							anchors.centerIn: parent
							text: qsTr("Area Plan Item %1").arg(index + 1)
							color: parent.color === qgcPal.button ? qgcPal.buttonText : qgcPal.buttonHighlightText
							horizontalAlignment: Text.AlignHCenter
							width: parent.width - ScreenTools.defaultFontPixelWidth
							elide: Text.ElideRight
						}
					}
				}
			}
		}
	}
} 