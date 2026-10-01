import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3

ColumnLayout {
    id: searchView

    Layout.fillWidth: true

    RowLayout {
        // Layout.alignment: Qt.AlignTop

        // Layout.preferredWidth: parent.width
        Layout.fillWidth: true
        spacing: 5

        PlasmaComponents3.Button {
            Layout.alignment: Qt.AlignLeft
            // implicitWidth: playerBackIcon.width + playerBackText.width
            onClicked: {
                swipeView.setCurrentIndex(0);
            }
            Layout.fillWidth: true

            contentItem: RowLayout {
                Kirigami.Icon {
                    id: playerBackIcon

                    Layout.alignment: Qt.AlignLeft
                    source: "arrow-left"
                    Layout.preferredWidth: 15
                    Layout.preferredHeight: 15
                }

                Text {
                    id: playerBackText

                    Layout.alignment: Qt.AlignLeft
                    text: "Player"
                    color: "white"
                }

            }

        }

        TextField {
            id: search

            Layout.fillWidth: true
            placeholderText: "Search..."
            color: "white"
            focus: true
            Keys.onReturnPressed: {
                player.search(search.text);
                root.searchLoading = true;
                root.searchResultModel = [];
            }
        }

    }

    Item {
        id: searchMessageContainer

        Layout.fillHeight: true
        Layout.preferredWidth: parent.width
        Layout.alignment: Qt.AlignVCenter
        visible: !root.searchLoading && root.searchResultModel.length < 1

        Kirigami.Icon {
            id: noResultsIcon

            anchors.centerIn: parent
            source: "search"
            width: 75
            height: 75
        }

        PlasmaComponents3.Label {
            text: "No Search Results Yet"
            anchors.top: noResultsIcon.bottom
            anchors.topMargin: 10
            anchors.horizontalCenter: noResultsIcon.horizontalCenter
            color: "lightgray"
        }

    }

    Item {
        id: searchMessageLoader

        Layout.fillHeight: true
        Layout.preferredWidth: parent.width
        Layout.alignment: Qt.AlignVCenter
        visible: root.searchLoading

        BusyIndicator {
            running: root.searchLoading
            anchors.centerIn: parent
            width: 75
            height: 75
        }

    }

    Repeater {
        id: searchResultList

        model: root.searchResultModel
        visible: !root.searchLoading && model.length > 0
        implicitHeight: root.hideListView && 0
        implicitWidth: 300
        Layout.maximumWidth: 300

        delegate: SearchResult {
        }

    }

}
