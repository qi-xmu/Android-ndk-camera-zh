//
// Created by 36014 on 2023/4/2.
//


#include "native-log.h"

#include "camera_image_reader.h"


CameraImageReader::CameraImageReader(
        ImageFormat image_format,
        AImageReader_ImageListener listener,
        int max_img_cnt
)
        : _max_image_count(max_img_cnt),
          _image_listener(listener),
          _image_format(image_format) {
    media_status_t res;

    // 创建一个图像读取
    res = AImageReader_new(
            _image_format.width,
            _image_format.height,
            _image_format.format,
            _max_image_count,
            &_image_reader
    );
    if (res != AMEDIA_OK) {
        LOG_ERR("create Image Reader error.");
    }

    // 添加图像的回调
    res = AImageReader_setImageListener(_image_reader, &_image_listener);
    if (res != AMEDIA_OK) {
        LOG_ERR("set Image Listener error.");
    }

    // acquire native window
    res = AImageReader_getWindow(_image_reader, &_image_reader_window);
    ANativeWindow_acquire(_image_reader_window);
    if (res != AMEDIA_OK) {
        LOG_ERR("AImageReader_getWindow error.");
    }
}


ANativeWindow *CameraImageReader::getImageNativeWindow() {
    return _image_reader_window;
}


