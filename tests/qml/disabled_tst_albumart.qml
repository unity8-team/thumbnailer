Fixture {
    function test_albumart() {
        loadAlbumArt("Gotye", "Making Mirrors")
        comparePixel(0, 0, "#F1E4D3");
    }

    function test_artistart() {
        loadArtistArt("Gotye", "Making Mirrors")
        comparePixel(0, 0, "#F8F1EB");
    }
}
