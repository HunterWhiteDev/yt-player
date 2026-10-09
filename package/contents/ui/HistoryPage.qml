import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3

Column {
    id: historyColumn

    spacing: 0

    RowLayout {
        id: headerRow

        // Layout.alignment: Qt.AlignTop
        // Layout.fillWidth: true
        width: parent.width
        spacing: 5

        Text {
            text: "History"
            color: "white"
            focus: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignLeft
        }

        PlasmaComponents3.Button {
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            // implicitWidth: playerBackIcon.width + playerBackText.width
            onClicked: {
                swipeView.setCurrentIndex(1);
            }

            contentItem: RowLayout {
                Text {
                    id: playerForwardText

                    text: "Player"
                    color: "white"
                }

                Kirigami.Icon {
                    id: playerForwardIcon

                    source: "arrow-right"
                    Layout.preferredWidth: 15
                    Layout.preferredHeight: 15
                }

            }

        }

    }

    ScrollView {
        implicitWidth: 300
        // ScrollBar.vertical.interactive: true
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerRow.bottom
        anchors.bottom: parent.bottom
        //These don't seem to actually work? Idk
        ScrollBar.vertical.anchors.left: parent.right
        ScrollBar.vertical.anchors.leftMargin: 10

        ListView {
            id: searchResultList

            anchors.fill: parent
            // implicitHeight: root.hideListView && 0
            height: parent.height
            model: root.history

            delegate: HistoryItem {
            }

        }

    }

}
