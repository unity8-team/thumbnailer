import QtQuick 2.0
import QtTest 1.0
import Ubuntu.Thumbnailer 0.1

Item {
    width: 200
    height: 200

    Image {
        id: image
        width: 200
        height: 200

        SignalSpy {
            id: spy
            target: image
            signalName: "statusChanged"
        }
    }

    Canvas {
        id: canvas
        width: 200
        height: 200
        renderStrategy: Canvas.Immediate
        renderTarget: Canvas.Image
    }

    TestCase {
        name: "ThumbnailerProviderTests"
        when: windowShown

        function test_albumart() {
            var ctx = loadImage(
                "image://albumart/artist=Gotye&album=Making%20Mirrors");
            comparePixel(ctx, 0, 0, 242, 228, 209, 255);
        }

        function test_artistart() {
            var ctx = loadImage(
                "image://artistart/artist=Gotye&album=Making%20Mirrors");
            comparePixel(ctx, 0, 0, 242, 228, 209, 255);
        }

        function loadImage(uri) {
            image.source = uri
            while (image.status == Image.Loading) {
                spy.wait();
            }
            compare(image.status, Image.Ready);

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
