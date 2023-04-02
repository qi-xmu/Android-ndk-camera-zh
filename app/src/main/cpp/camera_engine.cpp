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
        _camera(nullptr) {
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
    _camera->CreatePreviewSession(GetSurfaceNativeWindow());
}

jobject CameraEngine::GetSurfaceObject() {
    return _surface;
}

void CameraEngine::StartPreview(bool state) {
    _camera->StartPreview(state);
}

ImageFormat CameraEngine::GetCompatibleResolution() {
    return _compatible_camera_resolution;
}


// TEST Android Image Reader
void onImageAvailable(void *context, AImageReader *reader) {
    media_status_t res;
    // 获取图像的格式
    int32_t img_fmt;
    int32_t width, height;
    res = AImageReader_getFormat(reader, &img_fmt);
    if (res) LOG_ERR("AImageReader_getFormat error");
    res = AImageReader_getWidth(reader, &width);
    if (res) LOG_ERR("AImageReader_getFormat error");
    res = AImageReader_getHeight(reader, &height);
    if (res) LOG_ERR("AImageReader_getWidth error");

    AImage *image;
    res = AImageReader_acquireNextImage(reader, &image);
    if (res) LOG_ERR("AImageReader_acquireNextImage  error");

    // 获取图像的时间戳
    int64_t image_timestamp;
    AImage_getTimestamp(image, &image_timestamp);
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
//        LOG_WARN("0bit %x %x %x %x", y_data[0], y_data[1], y_data[2], y_data[3]);
    } else {
        // 其他格式
        int32_t image_buffer_len = 0;
        uint8_t *image_raw_buffer;
        res = AImage_getPlaneData(image, 0, &image_raw_buffer, &image_buffer_len);
        if (res) LOG_ERR("AImage_getPlaneData 0 error");
    }

    // 获取一行的像素长度 >= width （内存对齐的原因）
    int32_t rowStride;
    AImage_getPlaneRowStride(image, 0, &rowStride);

    // 这里传入的上下文 为 CameraEngine对象
    auto *cam_eng = reinterpret_cast<CameraEngine *>(context);
    //  获取 surface 生成的 NativeWindow对象 用于前端显示
    ANativeWindow *window = cam_eng->GetSurfaceNativeWindow();

    // 获取图像图像的格式
    ANativeWindow_setBuffersGeometry(window, width, height, img_fmt);


    ANativeWindow_Buffer aw_buffer;
    ANativeWindow_acquire(window);
    ANativeWindow_lock(window, &aw_buffer, nullptr);
    auto *bits = reinterpret_cast<uint8_t *>(aw_buffer.bits);
    if (AIMAGE_FORMAT_YUV_420_888 == img_fmt) {
        memcpy(bits, y_data, y_len + u_len + 1);
    } else if (AIMAGE_FORMAT_JPEG == img_fmt) {
//        memcpy(bits, image_raw_buffer, image_buffer_len);
        LOG_WARN("Can not directly show jpeg.");
    } else if (AIMAGE_FORMAT_RGBA_8888 == img_fmt) {

    }

    ANativeWindow_unlockAndPost(window);
    ANativeWindow_release(window);
    AImage_delete(image);
}

void CameraEngine::setCameraImageReader(jobject surface) {
    _surface = _env->NewGlobalRef(surface);
    AImageReader_ImageListener listener = {
            .context = this,
            .onImageAvailable = onImageAvailable,
    };
    _camera_image_reader = new CameraImageReader(_compatible_camera_resolution,
                                                 listener);

    // 这个地方开始一个session
    _camera->CreatePreviewSession(_camera_image_reader->getImageNativeWindow());
}

ANativeWindow *CameraEngine::GetSurfaceNativeWindow() {
    return ANativeWindow_fromSurface(_env, _surface);
}

