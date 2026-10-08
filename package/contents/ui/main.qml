pragma ComponentBehavior: Bound
import QtMultimedia
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
    property var searchResultModel: []
    property bool hideListView
    property int historyIdx: -1
    property int historyLength: -1
    property bool songLoaded: false
    property PlasmaComponents3.SwipeView swipeView
    property bool searchLoading: false;
    property string loadingStatus: "0"; 

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
            root.searchLoading = false;
        }
        onNowPlayingUpdate: (data) => {
            root.nowPlayingTitle = data.title;
            root.nowPlayingChannel = data.channel;
            root.nowPlayingThumbnail = "https://i.ytimg.com/vi/" + data.id + "/hqdefault.jpg";
            root.searchResultModel = [];
        }
        onHistoryUpdate: (length, idx) => {
         root.historyLength = length;
            root.historyIdx = idx;
        }
        onSongStarted: (state) => {
            console.log("SONG SHOULD START");

            root.songLoaded = true
            mediaPlayer.setSource("");
            mediaPlayer.setSource("/tmp/video.mp3");
            mediaPlayer.play();
        }
        onUpdateLoadingStatus: (status) => {
            root.loadingStatus = status;
        }
     }

MediaDevices {
    id: mediaDevices
}

MediaPlayer {
    id: mediaPlayer
    audioOutput: AudioOutput {
        device: mediaDevices.defaultAudioOutput
    }
        source: "/tmp/video.mp3"
}

    fullRepresentation: Item {
        id: fullRepresentationItem

        Layout.minimumHeight: 200
        Layout.maximumHeight: 200
        Layout.maximumWidth: 325
        Layout.minimumWidth: 325


        PlasmaComponents3.SwipeView {
            id: swipeView

            Layout.fillWidth: true
            onCurrentIndexChanged: {
                if (swipeView.currentIndex === 0) {
                    fullRepresentationItem.Layout.minimumHeight = 200;
                    fullRepresentationItem.Layout.maximumHeight = 200;
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
            PlayerPage {}
            
            //Search Page
            SearchPage {}
            
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
                if (mediaPlayer.playing) {
                    mediaPlayer.pause();
                } else {
                    mediaPlayer.play();
                }
            }

            Kirigami.Icon {
                anchors.verticalCenter: parent.verticalCenter
                source: mediaPlayer.playing ? "media-playback-pause" : "media-playback-start"
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


            Label {
                id: compactTextLabel

                anchors.verticalCenter: parent.verticalCenter
                Layout.fillWidth: true
                text: root.nowPlayingTitle ? root.nowPlayingTitle : "No Audio"

                MouseArea {
                anchors.fill: parent
                onClicked: root.expanded = !root.expanded
                hoverEnabled: true
                onEntered: {
                 compactAnimation.start();
                }
                onExited: {
                  compactAnimation.restart();
                  compactAnimation.stop();
                }
              }
            }

            SequentialAnimation on contentX {
                id: compactAnimation

                onFinished: {
                    compactFlickable.contentX = 0;
                    restart();
                    compactAnimation.pause();
                }

                PropertyAnimation {
                    from: compactFlickable.originX
                    to: compactTextLabel.width > compactFlickable.width ? compactTextLabel.width - compactFlickable.width + 5 : 0
                    duration: 5000
                }

            }

        }

    }
}
