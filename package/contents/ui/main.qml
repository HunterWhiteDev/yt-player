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
    property string nowPlayingTitle: ""
    property string nowPlayingChannel
    property string nowPlayingThumbnail: ""
    property bool isPlaying
    property var searchResultModel: []
    property bool hideListView
    property int historyIdx: -1
    property int historyLength: -1
    property PlasmaComponents3.SwipeView swipeView

    onExpandedChanged: (state) => {
        if (state === false) {
            searchResultModel = [];
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
            root.searchResultModel = [];
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
        id: fullRepresentationItem

        Layout.minimumHeight: 150
        Layout.maximumHeight: 150
        Layout.maximumWidth: 325
        Layout.minimumWidth: 325

        PlasmaComponents3.SwipeView {
            id: swipeView

            Layout.fillWidth: true
            onCurrentIndexChanged: {
                if (swipeView.currentIndex === 0) {
                    fullRepresentationItem.Layout.minimumHeight = 150;
                    fullRepresentationItem.Layout.maximumHeight = 150;
                } else if (swipeView.currentIndex === 1) {
                    fullRepresentationItem.Layout.minimumHeight = 650;
                    fullRepresentationItem.Layout.maximumHeight = 650;
                }
            }
            //A Queue view will have to be created here eventually
            spacing: 1
            currentIndex: 0
            anchors.fill: parent

            Connections {
                function onNowPlayingUpdate() {
                    swipeView.setCurrentIndex(0);
                }

                target: player
            }

            //Playing View
            Column {
                id: playerView

                RowLayout {
                    id: playerActions

                    width: parent.width

                    PlasmaComponents3.Button {
                        Layout.alignment: Qt.AlignRight
                        onClicked: {
                            swipeView.setCurrentIndex(1);
                        }

                        contentItem: RowLayout {
                            Kirigami.Icon {
                                id: searchIcon

                                color: "white"
                                source: "search"
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                            }

                            Text {
                                id: searchText

                                text: "Search"
                                color: "white"
                            }

                        }

                    }

                }

                Item {
                    id: playerMetaInfo

                    anchors.top: playerActions.bottom
                    anchors.bottom: playerControls.top
                    width: parent.width

                    Item {
                        id: nowPlayingImageContainer

                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.horizontalCenterOffset: -50
                        implicitWidth: 50
                        implicitHeight: 50
                        anchors.rightMargin: 10
                        anchors.topMargin: -10

                        Image {
                            id: nowPlayingImage

                            anchors.centerIn: parent
                            // Layout.alignment: Qt.AlignHCenter
                            source: root.nowPlayingThumbnail
                            visible: root.nowPlayingThumbnail.length > 0
                            height: 50
                            width: 50
                        }

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            color: "white"
                            source: "media-optical-album"
                            visible: root.nowPlayingThumbnail.length < 1
                            height: 50
                            width: 50
                        }

                    }

                    Flickable {
                        id: flickableTitle

                        anchors.top: nowPlayingImageContainer.top
                        anchors.left: nowPlayingImageContainer.right
                        anchors.leftMargin: 10
                        anchors.topMargin: 7.5
                        width: 150
                        height: nowPlayingTitle.height
                        contentWidth: nowPlayingTitle.width
                        clip: true
                        Component.onCompleted: {
                            animation.start();
                        }

                        Text {
                            id: nowPlayingTitle

                            //Cut it off of the title is too long
                            text: root.nowPlayingTitle ? root.nowPlayingTitle : "No Audio"
                            color: "white"
                            anchors.centerIn: parent
                        }

                        SequentialAnimation on contentX {
                            id: animation

                            onFinished: {
                                flickableTitle.contentX = 0;
                                restart();
                            }

                            PropertyAnimation {
                                from: flickableTitle.originX
                                //Only animate when the text will overflow
                                to: nowPlayingTitle.width > flickableTitle.width ? nowPlayingTitle.width - flickableTitle.width + 2 : 0
                                duration: 5000
                            }

                        }

                    }

                    Text {
                        id: nowPlayingChannel

                        anchors.top: flickableTitle.bottom
                        anchors.left: flickableTitle.left
                        text: root.nowPlayingChannel ? root.nowPlayingChannel : "Select a track"
                        color: "#a8a1b0"
                    }

                }

                RowLayout {
                    id: playerControls

                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter

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
                        enabled: root.nowPlayingTitle
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
                        enabled: root.nowPlayingTitle
                        background.visible: false
                        onHoveredChanged: {
                            background.visible = hovered;
                        }
                        onClicked: {
                            player.next();
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

            //Search View
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
                        }
                    }

                }

                Item {
                    id: searchMessageContainer

                    Layout.fillHeight: true
                    Layout.preferredWidth: parent.width
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.searchResultModel.length < 1

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

                Repeater {
                    id: searchResultList

                    model: root.searchResultModel
                    visible: model.length > 0
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
                                onClicked: player.loadVideo({
                                    "id": id,
                                    "title": title,
                                    "channel": channel,
                                    "duration": duration,
                                    "url": url
                                }, true)
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

            }

        }

    }

    compactRepresentation: RowLayout {
        id: compactRow

        implicitWidth: compactRow.implicitWidth
        Layout.minimumWidth: compactPlayButton.implicitWidth + compactTextLabel.implicitWidth + 10
        Layout.alignment: Qt.AlignCenter

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

        Flickable {
            id: compactFlickable

            anchors.verticalCenter: parent.verticalCenter
            contentWidth: 150
            clip: true
            implicitWidth: 150
            implicitHeight: parent.height
            Layout.alignment: Qt.AlignCenter
            Layout.fillHeight: true

            MouseArea {
                anchors.fill: parent
                onClicked: root.expanded = !root.expanded
            }

            Label {
                id: compactTextLabel

                anchors.verticalCenter: parent.verticalCenter
                Layout.fillWidth: true
                text: root.nowPlayingTitle ? root.nowPlayingTitle : "No Audio"
            }

            SequentialAnimation on contentX {
                id: compactAnimation

                onFinished: {
                    compactFlickable.contentX = 0;
                    restart();
                }

                PropertyAnimation {
                    from: compactFlickable.originX
                    to: compactTextLabel.width > compactFlickable.width ? compactTextLabel.width - compactFlickable.width + 2 : 0
                    duration: 5000
                }

            }

        }

    }

}
