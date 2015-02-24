import QtQuick 2.0
import QtTest 1.0
import Ubuntu.Thumbnailer 0.1

Item {
    Image {
        id: image
        width: 200
        height: 200
        source: thumbnailer.thumbnail

        SignalSpy {
            id: imageSpy
            target: image
            signalName: "statusChanged"
        }
    }

    Thumbnailer {
        id: thumbnailer
        size {
            width: image.width
            height: image.height
        }
    }

    SignalSpy {
        id: thumbnailerSpy
        target: thumbnailer
        signalName: "thumbnailChanged"
    }

    Canvas {
        id: canvas
        width: image.width
        height: image.height
        renderStrategy: Canvas.Immediate
        renderTarget: Canvas.Image
    }

    TestCase {
        name: "ThumbnailerTests"
        when: windowShown

        function cleanup() {
            thumbnailer.source = "";
            thumbnailerSpy.clear();
            imageSpy.clear();
        }

        function test_initialState() {
            compare(thumbnailerSpy.count, 0);
            compare(thumbnailer.size.width, 200);
            compare(thumbnailer.size.height, 200);
            compare(thumbnailer.thumbnail, "");
        }

        function test_localjpeg() {
            thumbnailer.source = "testimage.jpg";
            thumbnailerSpy.wait();
            compare(thumbnailerSpy.count, 1);
            verify(thumbnailer.thumbnail != "");
            tryCompare(image, "status", Image.Ready)

            var ctx = getContextForImage(image);
            comparePixel(ctx, 0, 0, 255, 1, 1, 255);
            comparePixel(ctx, 100, 100, 4, 2, 1, 255);
        }

        function test_nonExistingFile() {
            thumbnailer.source = "idontexist.jpg";
            compare(thumbnailerSpy.count, 0);
            compare(thumbnailer.thumbnail, "");
            tryCompare(image, "status", Image.Null)
        }

        function getContextForImage(image) {
            var ctx = canvas.getContext("2d");
            ctx.drawImage(image, 0, 0);
            return ctx;
        }

        function comparePixel(ctx,x,y,r,g,b,a, d) {
            var c = ctx.getImageData(x,y,1,1).data;
            if (d === undefined)
                d = 0;
            r = Math.round(r);
            g = Math.round(g);
            b = Math.round(b);
            a = Math.round(a);
            var notSame = Math.abs(c[0]-r)>d || Math.abs(c[1]-g)>d || Math.abs(c[2]-b)>d || Math.abs(c[3]-a)>d;
            if (notSame)
                qtest_fail('Pixel compare fail:\nactual :[' + c[0]+','+c[1]+','+c[2]+','+c[3] + ']\nexpected:['+r+','+g+','+b+','+a+'] +/- '+d, 1);

        }
    }
}
