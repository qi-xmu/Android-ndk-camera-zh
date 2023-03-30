//
// Created by 36014 on 2023/3/30.
//

#ifndef TCAMERA_NCAMERA_H
#define TCAMERA_NCAMERA_H

#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCameraCaptureSession.h>

#include <vector>
#include <map>
#include <string>


struct CaptureRequestInfo {
    ANativeWindow *outputNativeWindow_; // 定义输出类型
    ACaptureSessionOutput *sessionOutput_; // Session 输出
    ACameraOutputTarget *target_; // Container for a single output target.
    ACaptureRequest *request_; // 请求图像所包含的设置
    ACameraDevice_request_template template_; //  Capture request pre-defined template types
    int sessionSequenceId_; //id
};


struct ImageFormat {
    int32_t width;
    int32_t height;

    int32_t format;  // Through out this demo, the format is fixed to
    // YUV_420 format
};

class CameraId {
public:
    /**
     * 指向相机对象，相机打开成功后对象不为空，相机没打开前为 nullptr
     */
    ACameraDevice *device;
    /**
     * 相机的ID标识符
     */
    std::string id;
    /**
     * 相机的朝向
     * 0 与屏幕同方向
     * 1 与屏幕反方向
     * 2 扩展摄像头
     */
    acamera_metadata_enum_android_lens_facing_t facing;
    /**
     * 相机目前是否可用
     */
    bool available;  // free to use ( no other apps are using
    /**
     * 我们是否是相机的所有者
     */
    bool owner;      // we are the owner of the camera
    /**
     * 相机构造函数
     * @param id 相机的ID标识符
     */
    explicit CameraId(const char *id)
            : device(nullptr),
              facing(ACAMERA_LENS_FACING_FRONT),
              available(false),
              owner(false) {
        this->id = id;
    }

    // explicit 不允许隐形的类型转换
    /**
     * 空的相机对象
     */
    explicit CameraId(void) { CameraId(""); }
};

template<typename T>
class RangeValue {
public:
    T min_, max_;

    /**
     * return absolute value from relative value
     * value: in percent (50 for 50%)
     */
    T value(int percent) {
        return static_cast<T>(min_ + (max_ - min_) * percent / 100);
    }

    RangeValue() { min_ = max_ = static_cast<T>(0); }

    bool Supported(void) const { return (min_ != max_); }
};

namespace MyCamera {

    class NDKCamera {
    private:
        /**
         * 是否有效 用于相机是否成功加载
         */
        bool _valid;
        /**
         * 相机管理者
         */
        ACameraManager *_cameraMgr;
        /**
         * 每一次请求信息记录
         */
        std::vector<CaptureRequestInfo> _requests;
        /**
         * 所有的相机组成的查询词典
         */
        std::map<std::string, CameraId> _cameras;

        /**
         * 使用的相机标识符
         */
        std::string _activeCameraId;

        /**
         * 设置相机曝光时间范围
         */
        RangeValue<uint64_t> _exposureRange;

        /**
         * 设置相机曝光时间
         */
        uint64_t _exposureTime;

    public:
        NDKCamera();

        ~NDKCamera();


        /**
         * 枚举所有相机
         * @return
         */
        void EnumerateCamera();

        /**
         * 获取相机状态监听器
         * @return
         */
//        ACameraDevice_stateCallbacks* GetDeviceListener()
        /**
         * 当相机状态发生变化时的回调函数
         * @param id 相机标识符
         * @param available 相机是否可用
         */
        void OnCameraStatusChanged(const char *id, bool available);

//        bool MatchCaptureSizeRequest(int32_t reqWidth, int32_t reqHeight, ImageFormat *view);

        bool
        MatchCaptureSizeRequest(int32_t requestWidth, int32_t requestHeight, ImageFormat *resView,
                                ImageFormat *resCap);
    };

} // NCamera

#endif //TCAMERA_NCAMERA_H
