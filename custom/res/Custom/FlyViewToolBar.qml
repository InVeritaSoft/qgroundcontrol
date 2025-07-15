// Resource override for QGroundControl FlyViewToolBar.qml to set custom logo
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QGroundControl 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

RowLayout {
    id: viewButtonRow
    anchors.bottomMargin: 1
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    spacing: ScreenTools.defaultFontPixelWidth / 2

    QGCToolBarButton {
        id: currentButton
        Layout.preferredHeight: viewButtonRow.height
        icon.source: "/res/icons/inverita.png" // <-- Set custom logo here
        logo: true
        onClicked: mainWindow.showToolSelectDialog()
    }
    // ... rest of the toolbar ...
} 