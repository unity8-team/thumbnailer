#ifndef THUMBNAILEXTRACTOR_H
#define THUMBNAILEXTRACTOR_H

#include <memory>
#include <string>

class ThumbnailExtractor final {
    struct Private;
public:
    ThumbnailExtractor();
    ~ThumbnailExtractor();

    void reset();
    void set_uri(const std::string &uri);
    bool extract_video_frame();
    bool extract_audio_cover_art();
    void save_screenshot(const std::string &filename);
private:
    std::unique_ptr<Private> p;
};

#endif
