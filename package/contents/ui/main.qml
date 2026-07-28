import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import dev.hunterwhite.player 1.0
import org.kde.kirigami as Kirigami
import org.kde.kirigami.platform
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid 2.0

PlasmoidItem {
    id: root

    property string nowPlayingId
    property string nowPlayingTitle
    property string nowPlayingChannel
    property string nowPlayingThumbnail
    property bool isPlaying
    property var searchResultModel: null
    property bool hideListView
    property int historyIdx: -1
    property int historyLength: -1

    onExpandedChanged: (state) => {
        if (state === false) {
            searchResultModel = null;
            hideListView = true;
        }
    }

    Player {
        id: player

        onSearchUpdate: (searchResults) => {
            root.searchResultModel = searchResults;
            root.hideListView = false;
        }
        onNowPlayingUpdate: (data) => {
            root.nowPlayingTitle = data.title;
            root.nowPlayingChannel = data.channel;
            root.nowPlayingThumbnail = "https://i.ytimg.com/vi/" + data.id + "/hqdefault.jpg";
        }
        onPlayingStateChange: (state) => {
            root.isPlaying = state;
        }
        onHistoryUpdate: (length, idx) => {
            root.historyLength = length;
            root.historyIdx = idx;
        }
    }

    fullRepresentation: Item {
        Layout.minimumHeight: columnLayout.height
        Layout.maximumHeight: columnLayout.height
        implicitHeight: columnLayout.implicitHeight

        ColumnLayout {
            id: columnLayout

            width: parent.width
            anchors.fill: undefined

            TextField {
                id: search

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                placeholderText: "Search..."
                color: "white"
                focus: true
                Keys.onReturnPressed: {
                    player.search(search.text);
                }
            }

            Repeater {
                id: searchResultList

                model: root.searchResultModel
                visible: model !== null
                implicitHeight: hideListView ? 0 : contentHeight
                implicitWidth: 300
                Layout.maximumWidth: 300

                delegate: RowLayout {
                    id: searchResultRow

                    required property string id
                    required property string title
                    required property string channel
                    required property string duration
                    required property string url

                    Layout.alignment: Qt.AlignTop

                    Image {
                        id: searchRowImage

                        source: "https://i.ytimg.com/vi/" + id + "/hqdefault.jpg"
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: 50
                    }

                    ColumnLayout {
                        id: leftCol

                        Text {
                            id: titleText

                            Layout.preferredWidth: 200
                            wrapMode: Text.WordWrap
                            text: searchResultRow.title
                            color: "white"
                        }

                        Text {
                            id: channelText

                            Layout.preferredWidth: 200
                            wrapMode: Text.WordWrap
                            text: searchResultRow.channel
                            color: "white"
                        }

                    }

                    ColumnLayout {
                        id: rightCol

                        Layout.alignment: Qt.AlignRight

                        PlasmaComponents3.Button {
                            onClicked: player.loadVideo(searchResultRow.id, true)
                            background.visible: false
                            onHoveredChanged: {
                                background.visible = hovered;
                            }
                            Layout.alignment: Qt.AlignRight

                            Kirigami.Icon {
                                id: playButton

                                source: "media-playback-start"
                            }

                        }

                        Text {
                            id: durationText

                            color: "white"
                            text: new Date(searchResultRow.duration * 1000).toISOString().slice(11, 19)
                        }

                    }

                }

            }

            RowLayout {
                Layout.topMargin: 10
                Layout.fillWidth: true

                Item {
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true

                    Image {
                        id: nowPlayingImage

                        visible: root.nowPlayingImage
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        source: root.nowPlayingThumbnail
                        height: 25
                        width: 25
                    }

                    Text {
                        id: nowPlayingTitle

                        anchors.top: nowPlayingImage.top
                        anchors.left: nowPlayingImage.right
                        anchors.leftMargin: 5
                        anchors.topMargin: -5
                        //Cut it off of the title is too long
                        text: root.nowPlayingTitle ? root.nowPlayingTitle.slice(0, 20) : "No Audio Playing"
                        color: "white"
                    }

                    Text {
                        id: nowPlayingChannel

                        anchors.top: nowPlayingTitle.bottom
                        anchors.left: nowPlayingImage.right
                        anchors.leftMargin: 5
                        text: root.nowPlayingChannel ? root.nowPlayingChannel : "Select a track"
                        color: "#6e6973"
                    }

                }

                PlasmaComponents3.Button {
                    enabled: root.historyIdx > 0
                    Layout.alignment: Qt.AlignVCenter
                    background.visible: false
                    onHoveredChanged: {
                        background.visible = hovered;
                    }
                    onClicked: player.previous()

                    Kirigami.Icon {
                        source: "arrow-left-double"
                        width: 25
                        height: 25
                        anchors.centerIn: parent
                    }

                }

                PlasmaComponents3.Button {
                    Layout.alignment: Qt.AlignVCenter
                    background.visible: false
                    enabled: root.searchResultModel.length > 0
                    onHoveredChanged: {
                        background.visible = hovered;
                    }
                    onClicked: {
                        if (isPlaying)
                            player.pause();
                        else
                            player.play();
                    }

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        source: root.isPlaying ? "media-playback-pause" : "media-playback-start"
                        width: 25
                        height: 25
                    }

                }

                PlasmaComponents3.Button {
                    Layout.alignment: Qt.AlignVCenter
                    enabled: root.historyLength - 1 > root.historyIdx
                    background.visible: false
                    onHoveredChanged: {
                        background.visible = hovered;
                    }
                    onClicked: {
                        player.next(root.nowPlayingTitle);
                    }

                    Kirigami.Icon {
                        source: "arrow-right-double"
                        width: 25
                        height: 25
                        anchors.centerIn: parent
                    }

                }

            }

        }

    }

    compactRepresentation: RowLayout {
        id: compactRow

        implicitWidth: compactRow.implicitWidth
        Layout.minimumWidth: compactPlayButton.implicitWidth + compactTextLabel.implicitWidth + 10
        Layout.alignment: Qt.AlignCenter

        MouseArea {
            anchors.fill: parent
            onClicked: root.expanded = !root.expanded
        }

        PlasmaComponents3.Button {
            id: compactPlayButton

            Layout.alignment: Qt.AlignCenter
            anchors.verticalCenter: parent.verticalCenter
            background.visible: false
            onClicked: {
                if (root.isPlaying)
                    player.pause();
                else
                    player.play();
            }

            Kirigami.Icon {
                anchors.verticalCenter: parent.verticalCenter
                source: root.isPlaying ? "media-playback-pause" : "media-playback-start"
                width: 25
                height: 25
            }

        }

        Label {
            id: compactTextLabel

            anchors.verticalCenter: parent.verticalCenter
            Layout.alignment: Qt.AlignCenter
            Layout.fillWidth: true
            text: root.nowPlayingTitle ? root.nowPlayingTitle.length > 20 ? root.nowPlayingTitle.slice(0, 20) + "..." : root.nowPlayingTitle : "No Audio Playing"
        }

    }

}
