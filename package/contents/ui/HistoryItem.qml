import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3

RowLayout {
    id: historyRow

    required property string id
    required property string title
    required property string channel
    required property string duration
    required property string url
    required property int index

    Layout.alignment: Qt.AlignTop

    Image {
        id: historyRowImage

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
            text: historyRow.title
            color: "white"
        }

        Text {
            id: historyChannelText

            Layout.preferredWidth: 200
            wrapMode: Text.WordWrap
            text: historyRow.channel
            color: "white"
        }

    }

    ColumnLayout {
        id: rightCol

        Layout.alignment: Qt.AlignRight

        PlasmaComponents3.Button {
            onClicked: () => {
                if (id === root.nowPlayingId) {
                    if (mediaPlayer.playing)
                        mediaPlayer.pause();
                    else
                        mediaPlayer.play();
                    return ;
                }
                root.songLoaded = false;
                player.setHistoryIdx(index);
                player.loadVideo({
                    "id": id,
                    "title": title,
                    "channel": channel,
                    "duration": duration,
                    "url": url
                }, false);
            }
            background.visible: false
            onHoveredChanged: background.visible = hovered
            Layout.alignment: Qt.AlignRight

            Kirigami.Icon {
                id: playButton

                source: root.nowPlayingId === id && mediaPlayer.playing ? "media-playback-pause" : "media-playback-start"
            }

        }

        Text {
            id: historyDurationText

            color: "white"
            text: new Date(historyRow.duration * 1000).toISOString().slice(11, 19)
        }

    }

}
