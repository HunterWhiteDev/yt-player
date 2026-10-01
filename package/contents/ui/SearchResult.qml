import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents3

RowLayout {
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
            onClicked: () => {
                root.songLoaded = false;
                player.loadVideo({
                    "id": id,
                    "title": title,
                    "channel": channel,
                    "duration": duration,
                    "url": url
                }, true);
            }
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
