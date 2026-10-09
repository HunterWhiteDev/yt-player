import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3

ColumnLayout {
    id: playerView

    spacing: 5

    RowLayout {
        id: playerActions

        // width: parent.width
        Layout.alignment: Qt.AlignTop | Qt.AlignRight

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

        Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
        // anchors.top: playerActions.bottom
        // anchors.bottom: playerControls.top
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
                text: {
                    if (root.nowPlayingTitle) {
                        if (root.songLoaded)
                            return root.nowPlayingTitle;
                        else
                            return "Loading... " + root.loadingStatus + "%";
                    } else {
                        return "No Audio";
                    }
                }
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

    Item {
        id: sliderControls

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignBottom
        implicitHeight: 20

        Slider {
            id: slider

            onMoved: {
                mediaPlayer.position = value;
            }
            width: parent.width
            anchors.top: parent.bottom
            from: 0
            stepSize: 1
            to: mediaPlayer.duration
            value: mediaPlayer.position
        }

        Text {
            text: new Date(mediaPlayer.position).toISOString().slice(11, 19)
            anchors.top: slider.bottom
            anchors.left: parent.left
            color: "white"
        }

        Text {
            text: new Date(mediaPlayer.duration).toISOString().slice(11, 19)
            anchors.top: slider.bottom
            anchors.right: parent.right
            color: "white"
        }

    }

    RowLayout {
        id: playerControls

        Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter

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
                if (mediaPlayer.playing)
                    mediaPlayer.pause();
                else
                    mediaPlayer.play();
            }

            Kirigami.Icon {
                anchors.centerIn: parent
                source: mediaPlayer.playing ? "media-playback-pause" : "media-playback-start"
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
