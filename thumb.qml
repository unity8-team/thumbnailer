import QtQuick 2.0
import Qt.labs.folderlistmodel 2.1
import Ubuntu.Thumbnailer 0.1

Rectangle {
    width: 1024
    height: 800
    color: "gray"

    GridView {
        anchors.fill: parent
        cellWidth: 128
        cellHeight: 100
        model: FolderListModel {
            folder: "/home/kaleo/Videos/"
            showDirs: false
            showDotAndDotDot: false
            showOnlyReadable: true
//            nameFilters: ["*.jpg", "*.png", "*.mp4",]
        }

        delegate: Item {
            width: GridView.view.cellWidth
            height: GridView.view.cellHeight

            Image {
                id: image
                anchors.fill: parent
                anchors.margins: 10

                fillMode: Image.PreserveAspectFit
                source: thumbnailer.thumbnail
                sourceSize {
                    width: image.width
                    height: image.height
                }

                asynchronous: true

                Thumbnailer {
                    id: thumbnailer
                    source: filePath
                    size: image.sourceSize
                }
            }
        }
    }
}
