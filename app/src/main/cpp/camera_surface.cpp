//
// Created by 36014 on 2023/3/31.
//

#include "camera_surface.h"

#include <cstring>


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
}

void CameraEngine::CreateCameraSession(jobject surface) {
    // 创建一个全局引用， 可以多个线程中访问
    _surface = _env->NewGlobalRef(surface);
    _camera->CreatePreviewSession(ANativeWindow_fromSurface(_env, _surface));
}

jobject CameraEngine::GetSurfaceObject() {
    return _surface;
}

void CameraEngine::StartPreview(bool state) {
    _camera->StartPreview(state);
}
