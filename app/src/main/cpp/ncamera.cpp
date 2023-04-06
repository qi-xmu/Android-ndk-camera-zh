//
// Created by 36014 on 2023/3/30.
//
#include "native-log.h"
#include "ncamera.h"
#include "camera_listeners.h"


namespace MyCamera {
    /**
     * A helper class to assist image size comparison, by comparing the absolute
     * size
     * regardless of the portrait or landscape mode.
     */
    class DisplayDimension {
    public:
        // portrait 竖向的 landscape 横向的
        DisplayDimension(int32_t w, int32_t h) : _w(w), _h(h), _portrait(false) {
            // default landscape(横向的)
            if (h > w) {
                _w = h;
                _h = w;
                _portrait = true;
            }
        }

        /**
         * clone trait
         * @param other
         */
        DisplayDimension(const DisplayDimension &other) {
            _w = other._w;
            _h = other._h;
            _portrait = other._portrait;
        }

        /**
         * default trait
         */
        DisplayDimension(void) {
            _w = 0;
            _h = 0;
            _portrait = false;
        }

        /**
         * 赋值
         * @param other
         * @return
         */
        DisplayDimension &operator=(const DisplayDimension &other) {
            _w = other._w;
            _h = other._h;
            _portrait = other._portrait;

            return (*this);
        }

        /**
         * 宽高比是否相同
         * @param other
         * @return
         */
        bool IsSameRatio(DisplayDimension &other) {
            return (_w * other._h == _h * other._w);
        }

        /**
         * 大于
         * @param other
         * @return
         */
        bool operator>(DisplayDimension &other) {
            return (_w >= other._w && _h >= other._h);
        }

        /**
         * 是否相等
         * @param other
         * @return
         */
        bool operator==(DisplayDimension &other) {
            return (_w == other._w && _h == other._h && _portrait == other._portrait);
        }

        /**
         * 减法
         * @param other
         * @return
         */
        DisplayDimension operator-(DisplayDimension &other) {
            DisplayDimension delta(_w - other._w, _h - other._h);
            return delta;
        }

        /**
         * Flip 翻转
         */
        void Flip(void) { _portrait = !_portrait; }

        /**
         * 是否为竖向的
         * @return
         */
        bool IsPortrait(void) { return _portrait; }


        std::string toString() {
            return "Width = " + std::to_string(_w) +
                   " / " +
                   "Height = " + std::to_string(_h);
        }

        int32_t width(void) { return _w; }

        int32_t height(void) { return _h; }

        int32_t org_width(void) { return (_portrait ? _h : _w); }

        int32_t org_height(void) { return (_portrait ? _w : _h); }

    private:
        int32_t _w, _h; // 宽和高
        bool _portrait; // 竖向
    };
}

#include <media/NdkImage.h>

namespace MyCamera {
    enum PREVIEW_INDICES {
        PREVIEW_REQUEST_IDX = 0,
        JPG_CAPTURE_REQUEST_IDX, // 1
        VIDEO_RECORD_REQUEST_IDX, //2
        CAPTURE_REQUEST_COUNT, // 3
    };
    // 设置相机可用时的回调监听器
    static ACameraManager_AvailabilityCallbacks cameraMgrListener = {
            .context = nullptr,
            .onCameraAvailable = OnCameraAvailable,
            .onCameraUnavailable = OnCameraUnavailable,
    };

    NDKCamera::NDKCamera() {
        _valid = false;
        // 重置缓冲区
        this->_requests.resize(CAPTURE_REQUEST_COUNT);
        // 这个地方有一个错误：vector的长度和元素个数无关，和存放的类型相关
        memset(this->_requests.data(), 0, this->_requests.size() * sizeof(this->_requests[0]));
        this->_cameras.clear();

        _cameraMgr = ACameraManager_create();
        // 遍历所有的相机并记录信息到CameraId中去
        this->EnumerateCamera();
        _activeCameraId = "0"; // 默认选择后置主摄

        // 打开相机，同时添加相机设备监听器（设备是否断开连接或者发生故障）
        static ACameraDevice_StateCallbacks cameraDeviceListener = {
                .context = this,
                .onDisconnected = nullptr,
                .onError = nullptr,
        };
        ACameraManager_openCamera(_cameraMgr, _activeCameraId.c_str(), &cameraDeviceListener,
                                  &_cameras[_activeCameraId].device);

        // 设置相机可用时的回调监听器
        cameraMgrListener.context = this;
        ACameraManager_registerAvailabilityCallback(_cameraMgr,
                                                    reinterpret_cast<const ACameraManager_AvailabilityCallbacks *>
                                                    (&cameraMgrListener));

        // 初始化相机控制 通过修改 metadata 控制相机
        ACameraMetadata *metadataObj;
        ACameraManager_getCameraCharacteristics(_cameraMgr, _activeCameraId.c_str(), &metadataObj);

        // 设置相机属性 这里设置的曝光时间 单位 ns
        ACameraMetadata_const_entry val = {
                0,
        };
        camera_status_t status;

        status = ACameraMetadata_getConstEntry(
                metadataObj, ACAMERA_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES, &val);

        _valid = true;
    }

    bool NDKCamera::MatchCaptureSizeRequest(int32_t requestWidth, int32_t requestHeight,
                                            ImageFormat *resView,
                                            ImageFormat *resCap) {

        // 请求的显示的尺寸
        DisplayDimension disp(requestWidth, requestHeight);
        ACameraMetadata *metadata;
        ACameraManager_getCameraCharacteristics(_cameraMgr, _activeCameraId.c_str(), &metadata);

        // 获取相机支持的分辨率
        ACameraMetadata_const_entry entry;
        ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                      &entry);


        bool foundIt = false;
        DisplayDimension foundRes = disp; // 找到的目标分辨率
        DisplayDimension maxJPG(0, 0); // 找到的最大的jpg分辨率

        // 数据格式: format, width, height, input?, type int32
        for (uint32_t i = 0; i < entry.count; i += 4) {
            int32_t input = entry.data.i32[i + 3];
            int32_t format = entry.data.i32[i + 0];

            if (input) continue;

            if (format == AIMAGE_FORMAT_YUV_420_888 ||
                format == AIMAGE_FORMAT_JPEG) {
                // 根据格式构造分辨率
                DisplayDimension res(entry.data.i32[i + 1], entry.data.i32[i + 2]);
//                LOG_INFO("Try Resolution %s with format %d", res.toString().c_str(), format);
//                if (!disp.IsSameRatio(res)) continue;
                if (format == AIMAGE_FORMAT_YUV_420_888 && foundRes > res) {
                    foundIt = true;
                    foundRes = res;
                    break;
                } else if (format == AIMAGE_FORMAT_JPEG && res > maxJPG) {
                    maxJPG = res;
                }
            }
        }

        if (foundIt) {
            resView->width = foundRes.org_width();
            resView->height = foundRes.org_height();
            if (resCap) {
                resCap->width = maxJPG.org_width();
                resCap->height = maxJPG.org_height();
            }
        } else {
            LOG_WARN("Did not find any compatible camera resolution, taking 640x480");
            if (disp.IsPortrait()) {
                resView->width = 480;
                resView->height = 640;
            } else {
                resView->width = 640;
                resView->height = 480;
            }
            if (resCap) *resCap = *resView;
        }
        resView->format = AIMAGE_FORMAT_YUV_420_888;
        if (resCap) resCap->format = AIMAGE_FORMAT_JPEG;
        return foundIt;
    }

    NDKCamera::~NDKCamera() {
        _valid = false;
        if (_captureSessionState == CaptureSessionState::ACTIVE) {
            ACameraCaptureSession_stopRepeating(_captureSession);
        }
        ACameraCaptureSession_close(_captureSession);

        // 释放所有的输出窗口
        for (auto &req: _requests) {
            if (!req._outputNativeWindow) continue;

            ACaptureRequest_removeTarget(req._request, req._target);
            ACaptureRequest_free(req._request);

            ACaptureSessionOutputContainer_remove(_outputContainer, req._sessionOutput);
            ACaptureSessionOutput_free(req._sessionOutput);

            ANativeWindow_release(req._outputNativeWindow);
        }
        _requests.resize(0);
        // 释放容器
        ACaptureSessionOutputContainer_free(_outputContainer);
        // 释放所有的相机对象
        for (auto &cam: _cameras) {
            if (cam.second.device) {
                ACameraDevice_close(cam.second.device);
            }
        }
        _cameras.clear(); // 清空 map

        if (_cameraMgr) {
            ACameraManager_unregisterAvailabilityCallback(_cameraMgr, &cameraMgrListener);
            ACameraManager_delete(_cameraMgr);
            _cameraMgr = nullptr;
        }
    }

    /**
     * EnumerateCamera()
     *     Loop through cameras on the system, pick up
     *     1) back facing one if available
     *     2) otherwise pick the first one reported to us
     */
    void NDKCamera::EnumerateCamera() {
        ACameraIdList *cameraIds = nullptr;
        // 获取设备所有的相机列表
        ACameraManager_getCameraIdList(_cameraMgr, &cameraIds);
//        LOG_INFO("相机数量：%d", cameraIds->numCameras);

        for (int i = 0; i < cameraIds->numCameras; i++) {
            const char *id = cameraIds->cameraIds[i];
//            LOG_INFO("%d 相机标识符 %s", i, id);

            // 获取某个相机的媒体信息
            ACameraMetadata *metadataObj;
            ACameraManager_getCameraCharacteristics(_cameraMgr, id, &metadataObj);

            int32_t count = 0;
            // 获取媒体信息的标签值
            const uint32_t *tags = nullptr;
            ACameraMetadata_getAllTags(metadataObj, &count, &tags);

            // 无论有没有 ACAMERA_LENS_FACING 属性都存入
            CameraId cam(id);
            for (int tagIdx = 0; tagIdx < count; tagIdx++) {
                // 遍历所有的Tags 找到标定相机朝向的Tags
                if (ACAMERA_LENS_FACING == tags[tagIdx]) {
                    // A single read-only camera metadata entry.
                    ACameraMetadata_const_entry lensInfo = {
                            0,
                    };
                    ACameraMetadata_getConstEntry(metadataObj, tags[tagIdx], &lensInfo);
                    // 获取相机的朝向
                    // static_cast 是类型转换的方法
                    cam.facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
                            lensInfo.data.u8[0]);
                    // 相机的所有者
                    cam.owner = false;
                    cam.device = nullptr;
                    break;
                }
            }
            ACameraMetadata_free(metadataObj);
            _cameras[cam.id] = cam;
        }
        // 回收内存
        ACameraManager_deleteCameraIdList(cameraIds);
    }


    void NDKCamera::OnCameraStatusChanged(const char *id, bool available) {
        if (_valid) {
            _cameras[std::string(id)].available = available;
//            LOG_INFO("Camera Id %s is %d", id, available);
        }
    }

    void NDKCamera::CreateSession(ANativeWindow *preview_window, ANativeWindow *jpg_window,
                                  bool manual_preview, int32_t image_rotation) {
        // 预览的请求缓冲区
        _requests[PREVIEW_REQUEST_IDX]._outputNativeWindow = preview_window;
//        _requests[PREVIEW_REQUEST_IDX]._template = TEMPLATE_PREVIEW;
        _requests[PREVIEW_REQUEST_IDX]._template = TEMPLATE_RECORD;
        // 拍摄的请求缓冲区
        _requests[JPG_CAPTURE_REQUEST_IDX]._outputNativeWindow = jpg_window;
        _requests[JPG_CAPTURE_REQUEST_IDX]._template = TEMPLATE_STILL_CAPTURE;

        ACaptureSessionOutputContainer_create(&_outputContainer);
        for (auto &req: _requests) {
            // 没有输出窗口则跳过
            if (!req._outputNativeWindow) {
                continue;
            }

            // 获取窗口
            ANativeWindow_acquire(req._outputNativeWindow);
            // 根据窗口创建一个 会话输出对象
            ACaptureSessionOutput_create(req._outputNativeWindow, &req._sessionOutput);
            // 将会话的输出添加到容器中
            ACaptureSessionOutputContainer_add(_outputContainer, req._sessionOutput);

            // 根据窗口创建一个 相机目标输出对象
            ACameraOutputTarget_create(req._outputNativeWindow, &req._target);
            // 创建一个相机捕获请求 根据 模板 决定捕获的质量
            ACameraDevice_createCaptureRequest(_cameras[_activeCameraId].device, req._template,
                                               &req._request);
            // 将 目标输出对象 放入请求中
            ACaptureRequest_addTarget(req._request, req._target);
        }
        // 更新相机当前的状态
        _captureSessionState = CaptureSessionState::READY;

        // 相机捕获会话过程中的一些回调函数
        static ACameraCaptureSession_stateCallbacks sessionListener = {
                .context = this,
                .onClosed = OnSessionClosed,
                .onReady = OnSessionReady,
                .onActive = OnSessionActive,
        };
        // MASK!!创建相机会话
        auto status = ACameraDevice_createCaptureSession(
                _cameras[_activeCameraId].device,
                _outputContainer,
                &sessionListener,
                &_captureSession);
        if (status != ACAMERA_OK) {
            throw std::runtime_error("Open camera device create capture session error.");
        } else {
            LOG_WARN("_captureSession = %p", _captureSession);
        }

        if (jpg_window) {
            // 这里设置捕获时的一些属性，这里设置的图像的旋转角度
            ACaptureRequest_setEntry_i32(
                    _requests[JPG_CAPTURE_REQUEST_IDX]._request,
                    ACAMERA_JPEG_ORIENTATION,
                    1,
                    &image_rotation
            );
        }
//        if (!manual_preview) {
//            return;
//        }
        // 设置相机参数
        uint8_t stab = ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_ON;
        ACaptureRequest_setEntry_u8(
                _requests[PREVIEW_REQUEST_IDX]._request,
                ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE,
                1,
                &stab
        );
//        const uint8_t af_off = ACAMERA_CONTROL_AF_MODE_OFF;
//        ACaptureRequest_setEntry_u8(_requests[PREVIEW_REQUEST_IDX]._request,
//                                    ACAMERA_CONTROL_AF_MODE, 1,
//                                    &af_off);
        const float focus_distance = 90;
        ACaptureRequest_setEntry_float(_requests[PREVIEW_REQUEST_IDX]._request,
                                       ACAMERA_LENS_FOCUS_DISTANCE, 1,
                                       &focus_distance);
        LOG_INFO("Reach manual preview");
    }

    void NDKCamera::CreatePreviewSession(ANativeWindow *previewWindow) {
        CreateSession(previewWindow, nullptr, true, 0);
    }

    void NDKCamera::OnSessionState(ACameraCaptureSession *ses, CaptureSessionState state) {
        if (ses == nullptr || ses != _captureSession) {
            LOG_WARN("ses is not mine. !!");
            return;
        }
        _captureSessionState = state;
    }

    void NDKCamera::StartPreview(bool state) {
        if (state) {
            LOG_INFO("Preview");
            ACameraCaptureSession_setRepeatingRequest(_captureSession, nullptr, 1,
                                                      &_requests[PREVIEW_REQUEST_IDX]._request,
                                                      nullptr);
        } else if (_captureSessionState == CaptureSessionState::ACTIVE) {
            LOG_INFO("stop Preview");
            ACameraCaptureSession_stopRepeating(_captureSession);
        }
    }
} // NCamera


