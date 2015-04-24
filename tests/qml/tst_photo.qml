Fixture {
    name: "Photo"

    function test_photo() {
        loadThumbnail("testimage.jpg");
        compare(size.width, 640);
        compare(size.height, 400);
        comparePixel(0, 0, "#FE0000");
        comparePixel(639, 0, "#5A00FF");
        comparePixel(0, 399, "#58FF01");
        comparePixel(639, 399, "#FE0000");
    }

    function test_scaled() {
        requestedSize = Qt.size(256, 256);
        loadThumbnail("testimage.jpg");
        compare(size.width, 256);
        compare(size.height, 160);
    }

    function test_scale_horizontal() {
        requestedSize = Qt.size(320, 0);
        loadThumbnail("testimage.jpg");
        compare(size.width, 320);
        compare(size.height, 200);
    }

    function test_scale_vertical() {
        requestedSize = Qt.size(0, 200);
        loadThumbnail("testimage.jpg");
        // FIXME: This is what we should scale to ...
        //compare(size.width, 320);
        //compare(size.height, 200);

        // But this is what we get:
        compare(size.width, 256);
        compare(size.height, 160);
    }

    /* Thumbnailer doesn't seem to be rotating unscaled images
    function test_rotation() {
        loadThumbnail("testrotate.jpg");
        compare(size.width, 428);
        compare(size.height, 640);
    }
    */

    function test_rotation_scaled() {
        requestedSize = Qt.size(256, 256);
        loadThumbnail("testrotate.jpg");
        compare(size.width, 192);
        compare(size.height, 256);
    }

}
