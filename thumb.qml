import QtQuick 2.0
import Qt.labs.folderlistmodel 2.1
import Ubuntu.Thumbnailer 0.1

Rectangle {
    width: 800
    height: 600
    color: "gray"

    GridView {
        anchors.fill: parent
        model: FolderListModel {
            folder: "/home/kaleo/Videos/"
            showDirs: false
            showDotAndDotDot: false
            showOnlyReadable: true
//            nameFilters: ["*.jpg", "*.png", "*.mp4",]
        }

        delegate: Image {
            id: delegate
            source: thumbnailer.thumbnail
            asynchronous: true

            Thumbnailer {
                id: thumbnailer
                source: filePath
                size: Thumbnailer.Large
//                size: Thumbnailer.Small
            }
        }
    }
}
