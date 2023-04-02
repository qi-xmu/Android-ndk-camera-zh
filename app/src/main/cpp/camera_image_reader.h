//
// Created by 36014 on 2023/4/2.
//

#ifndef TCAMERA_CAMERA_IMAGE_READER_H
#define TCAMERA_CAMERA_IMAGE_READER_H

#include <media/NdkImageReader.h>
#include "ncamera.h"


#define MAX_OUT_BUF_COUNT 10

class CameraImageReader {
private:
    /**
     * 图像读取
     */
    AImageReader *_image_reader{};
    /**
     * 图像读取监听
     */
    AImageReader_ImageListener _image_listener{};

    /**
     * 图像的格式
     */
    ImageFormat _image_format{};

    /**
     *  图像缓冲区大小
     */
    int32_t _max_image_count;
    /**
     *
     */
    ANativeWindow *_image_reader_window;

public:
    /**
     * 初始化一个 Camera Image Reader
     * @param image_format
     */
    CameraImageReader(
            ImageFormat image_format,
            AImageReader_ImageListener listener,
            int max_img_cnt = MAX_OUT_BUF_COUNT
    );

    ANativeWindow *getImageNativeWindow();
};


#endif //TCAMERA_CAMERA_IMAGE_READER_H
