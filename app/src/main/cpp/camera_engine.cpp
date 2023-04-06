//
// Created by 36014 on 2023/3/31.
//

#include "camera_engine.h"
#include "native-log.h"

#include <cstring>
#include <media/NdkImageReader.h>


CameraEngine::CameraEngine(JNIEnv *env, jobject instance, jint w, jint h) :
        _env(env),
        _java_instance(instance),
        _request_width(w),
        _request_height(h),
        _surface(nullptr),
        _preview_state(false) {
    // 重置兼容的分辨率
    memset(&_compatible_camera_resolution, 0, sizeof(_compatible_camera_resolution));
    // 初始化一个相机对象
    _camera = new MyCamera::NDKCamera();
    // 获取兼容的相机分辨率
    _camera->MatchCaptureSizeRequest(
            _request_width,
            _request_height,
            &_compatible_camera_resolution,
            nullptr
    );


}

CameraEngine::~CameraEngine() {
    if (_camera) {
        delete _camera;
        _camera = nullptr;
    }
    if (_surface) {
        _env->DeleteGlobalRef(_surface);
        _surface = nullptr;
    }
    if (_camera_image_reader) {
        delete _camera_image_reader;
        _camera_image_reader = nullptr;
    }
}

void CameraEngine::CreateCameraSession(jobject surface) {
    // 创建一个全局引用， 可以多个线程中访问
    _surface = _env->NewGlobalRef(surface);
    _native_window = ANativeWindow_fromSurface(_env, _surface);
    _camera->CreatePreviewSession(_native_window);
}

jobject CameraEngine::GetSurfaceObject() {
    return _surface;
}

void CameraEngine::Preview(bool state) {
    _preview_state = state;
    _camera->StartPreview(state);
    if (!state) {
        delete _tcp_node;
    }
}

ImageFormat CameraEngine::GetCompatibleResolution() {
    return _compatible_camera_resolution;
}

void cvtYUV2RGB(uint8_t *yuv, uint32_t *rgb, int32_t width, int32_t height) {
    int size = width * height;
    uint8_t *y = yuv;
    uint8_t *vu = y + size;
    int v = 128, u = 128;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int i = row * width + col;
            if ((col & 0x01) == 0) {
                u = vu[row / 2 * width + col] & 0xff;
                v = vu[row / 2 * width + col + 1] & 0xff;
            }

            auto fr = (y[i] + 1.13983 * (v - 128));
            auto fg = (y[i] - 0.39465 * (u - 128) -
                       0.58060 * (v - 128));
            auto fb = (y[i] + 2.03211 * (u - 128));

            auto r = (uint8_t) (fr < 0 ? 0 : (fr > 256 ? 255 : fr));
            auto g = (uint8_t) (fg < 0 ? 0 : (fg > 256 ? 255 : fg));
            auto b = (uint8_t) (fb < 0 ? 0 : (fb > 256 ? 255 : fb));

            uint32_t rgba_byte = 0;
            rgba_byte = rgba_byte | (r << 16) | (g << 8) | b;

            rgb[i] = rgba_byte;
        }
    }

}

static int64_t pre_timestamp = 0;

// TEST Android Image Reader
void CameraEngine::onImageAvailable(void *context, AImageReader *reader) {
    media_status_t res;
    // 这里传入的上下文 为 CameraEngine对象
    auto cam_eng = reinterpret_cast<CameraEngine *>(context);

    // 获取图像的格式
    int32_t img_fmt;
    int32_t width, height;
    res = AImageReader_getFormat(reader, &img_fmt);
    if (res) LOG_ERR("AImageReader_getFormat error");
    res = AImageReader_getWidth(reader, &width);
    if (res) LOG_ERR("AImageReader_getFormat error");
    res = AImageReader_getHeight(reader, &height);
    if (res) LOG_ERR("AImageReader_getWidth error");
//    LOG_INFO("width = %d height = %d format =  %d.", width, height, width);

    // 获取图像
    AImage *image;
    res = AImageReader_acquireNextImage(reader, &image);
    if (res) LOG_ERR("AImageReader_acquireNextImage  error");
    // 获取图像的时间戳
    int64_t image_timestamp;
    res = AImage_getTimestamp(image, &image_timestamp);
    if (res) LOG_ERR("AImageReader_acquireNextImage  error");

    uint8_t *y_data, *u_data, *v_data;
    int32_t y_len = 0, u_len = 0, v_len = 0;

    if (AIMAGE_FORMAT_YUV_420_888 == img_fmt) {
        // 获取各个分量的指针，这个地方存在一个问题，这里的数据结构如下
        // YY ... YYYY (repeat width * height) U V U V ..... (total width * height /2);
        // 数据总长度为 width * height * 3 / 2
        res = AImage_getPlaneData(image, 0, &y_data, &y_len);
        if (res) LOG_ERR("AImage_getPlaneData 0 error");
        res = AImage_getPlaneData(image, 1, &u_data, &u_len);
        if (res) LOG_ERR("AImage_getPlaneData 1 error");
        res = AImage_getPlaneData(image, 2, &v_data, &v_len);
        if (res) LOG_ERR("AImage_getPlaneData 2 error");
        // 如果需要将 YUV 转化成 ARGB
        img_fmt = AIMAGE_FORMAT_RGBA_8888;
    } else {
        // 其他格式
        int32_t image_buffer_len = 0;
        uint8_t *image_raw_buffer;
        res = AImage_getPlaneData(image, 0, &image_raw_buffer, &image_buffer_len);
        if (res) LOG_ERR("AImage_getPlaneData 0 error");
    }

    // 获取一行的像素长度 >= width （内存对齐的原因）
    int32_t rowStride;
    res = AImage_getPlaneRowStride(image, 0, &rowStride);
    if (res) LOG_ERR("AImage_getPlaneRowStride 2 error");

    int nw_res;

    //  获取 surface 生成的 NativeWindow对象 用于前端显示
    ANativeWindow *window = cam_eng->_native_window;

    // 获取图像图像的格式
    nw_res = ANativeWindow_setBuffersGeometry(window, width, height, img_fmt);
    if (nw_res) LOG_ERR("ANativeWindow_setBuffersGeometry error, %d", nw_res);

    ANativeWindow_Buffer aw_buffer;
    ANativeWindow_acquire(window);
    if (!ANativeWindow_lock(window, &aw_buffer, nullptr)) {

        auto *bits = reinterpret_cast<uint8_t *>(aw_buffer.bits);
        if (AIMAGE_FORMAT_YUV_420_888 == img_fmt) {
            memcpy(bits, y_data, y_len + v_len + 1);
        } else if (AIMAGE_FORMAT_RGBA_8888 == img_fmt) {
            cvtYUV2RGB(y_data, (uint32_t *) bits, width, height);
        } else if (AIMAGE_FORMAT_JPEG == img_fmt) {
            LOG_WARN("Can not directly show jpeg.");
        }
        nw_res = ANativeWindow_unlockAndPost(window);
        if (nw_res) LOG_ERR("ANativeWindow_unlockAndPost error, %d", nw_res);
    }
    ANativeWindow_release(window);
    AImage_delete(image);
    // 帧率统计
    if (pre_timestamp) {
        int64_t cost = image_timestamp - pre_timestamp;
        static char buf[1024];
//        LOG_INFO("%lld %d", cost, (int) (1000000000.0 / cost));
        sprintf(buf, "%lld %d", cost, (int) (1000000000.0 / cost));

        if (cam_eng->_tcp_node->valid) {
//            cam_eng->_tcp_node->str_send(buf);
//            cam_eng->_tcp_node->bin_send(y_data, y_len + v_len + 1);
        } else if (cam_eng->_preview_state) {
            LOG_INFO("shutdown Preview");
            cam_eng->Preview(false);
        }
    }
    pre_timestamp = image_timestamp;
}


void CameraEngine::setCameraImageReader(jobject surface) {
    // 等待网络连接开始
    _tcp_node = new TcpNodeServer();
    LOG_INFO("waiting for client connected.");
    _tcp_node->wait_for_client();

    // 创建一个引用
    _surface = _env->NewGlobalRef(surface);
    _native_window = ANativeWindow_fromSurface(_env, _surface);
    // 创建相机图像回调
    AImageReader_ImageListener listener = {
            .context = this,
            .onImageAvailable = onImageAvailable,
    };
    _camera_image_reader = new CameraImageReader(_compatible_camera_resolution,
                                                 listener);

    // 这个地方开始一个session
    _camera->CreatePreviewSession(_camera_image_reader->getImageNativeWindow());
}



