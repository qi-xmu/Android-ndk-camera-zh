//
// Created by 36014 on 2023/3/31.
//

#ifndef TCAMERA_CAMERA_ENGINE_H
#define TCAMERA_CAMERA_ENGINE_H

#include <jni.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "ncamera.h"
#include "camera_image_reader.h"
#include "net/tcp_node_client.h"

/**
 * transmit camera image to surface.
 */
class CameraEngine {
private:
    /**
     * 上下文环境
     */
    JNIEnv *_env;
    /**
     * 没用到
     */
    jobject _java_instance;
    /**
     * 请求的相机宽度
     */
    int32_t _request_width;
    /**
     * 请求的相机高度
     */
    int32_t _request_height;
    /**
     * 接受图像的surface
     */
    jobject _surface;
    /**
     * Image Reader Object
     */
    CameraImageReader *_camera_image_reader;
    /**
     * ndk 相机对象
     */
    MyCamera::NDKCamera *_camera;
    /**
     * 兼容的相机分辨率
     */
    ImageFormat _compatible_camera_resolution;
    /**
     * 显示的 ANativeWindow
     */
    ANativeWindow *_native_window;
    /**
     * 网络节点
     */
//    TcpNodeServer *_tcp_node = nullptr;
    TcpNodeClient *_tcp_node = nullptr;
    /**
     * preview state
     */
    bool _preview_state = false;

public:
    /**
     * 初始化一个相机引擎对象，用于驱动相机显示到surface
     * @param env
     * @param instance
     * @param w 期望的相机宽度
     * @param h 期望的相机高度
     */
    explicit CameraEngine(JNIEnv *env, jobject instance, jint w, jint h);

    /**
     * 释放全局引用和_camera对象
     */
    ~CameraEngine();

    /**
     * 创建相机会话
     * @param surface 图像传递的surface对象
     */
    void CreateCameraSession(jobject surface);

    /**
     * 获取surface对象
     * @return surface
     */
    jobject GetSurfaceObject();

    /**
     * 开始或者关闭预览
     * @param state 开始或者关闭预览
     */
    void Preview(bool state);

    /**
     * 获取兼容的相机分辨率
     * @return
     */
    ImageFormat GetCompatibleResolution();

    /**
     * 初始化相机的图像读取器
     */
    void setCameraImageReader(jobject surface);

    /**
     * 图像回调函数
     */
    static void onImageAvailable(void *context, AImageReader *reader);
};


#endif //TCAMERA_CAMERA_ENGINE_H
