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


/**
 * 相机当前的状态
 */
enum class CaptureSessionState : int32_t {
    READY = 0,  // session is ready
    ACTIVE,     // session is busy
    CLOSED,     // session is closed(by itself or a new session evicts)
    MAX_STATE
};

struct CaptureRequestInfo {
    /**
     * ANativeWindow对象，用于构建 ACaptureSessionOutput对象
     */
    ANativeWindow *_outputNativeWindow; // 定义输出类型
    /**
     * SessionOutput 的输出对象是 ANative_Window
     * ANativeWindow 通过 ACaptureSessionOutput_create 获得一个 ACaptureSessionOutput对象。
     * ACaptureSessionOutput对象 可以放入 ACaptureSessionOutputContainer 容器中，
     * 和 相机（camera） 一同用于生成一个 ACameraCaptureSession对象。
     */
    ACaptureSessionOutput *_sessionOutput; // Session 输出
    /**
     * 一个请求包含该请求的 输出目标（target）
     * _target 由 ANativeWindow通过 ACameraOutputTarget_create 构造，
     * 需要通过 ACaptureRequest_addTarget 添加到请求中
     */
    ACameraOutputTarget *_target;
    /**
     * 根据 相机（camera） 和 模板（template） 通过 ACameraDevice_createCaptureRequest 构造的来。
     * 该对象 和 ACameraCaptureSession对象 通过 ACameraCaptureSession_setRepeatingRequest 可以不断的向相机发送捕获请求，从而得到连续的画面。
     * ACameraCaptureSession对象 通过 ACameraCaptureSession_stopRepeating 停止发送请求。
     */
    ACaptureRequest *_request;
    /**
     * 预设的图像模板 不同的模板对 图像质量和帧率的侧重不同。
     * TEMPLATE_PREVIEW 侧重帧率而非图像质量
     * TEMPLATE_STILL_CAPTURE 侧重图像质量而非帧率
     */
    ACameraDevice_request_template _template; //  Capture request pre-defined template types
    /**
     * 一般不用于预览中。
     * 发送一个图像捕获请求 对应一个 图像输出序号。
     */
    int _sessionSequenceId;
};


/**
 * AIMAGE_FORMAT_YUV_420_888
 */
struct ImageFormat {
    int32_t width;
    int32_t height;
    int32_t format;  // Through out this demo, the format is fixed to AIMAGE_FORMAT_YUV_420_888
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

        /**
         * 相机会话输出容器
         */
        ACaptureSessionOutputContainer *_outputContainer;

        /**
         * 相机会话当前的状态
         */
        CaptureSessionState _captureSessionState;

        /**
         * 相机捕获会话
         */
        ACameraCaptureSession *_captureSession;

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


        /**
         *
         * @param requestWidth
         * @param requestHeight
         * @param resView
         * @param resCap
         * @return
         */
        bool
        MatchCaptureSizeRequest(int32_t requestWidth, int32_t requestHeight, ImageFormat *resView,
                                ImageFormat *resCap);

        /**
         * 创建一个相机显示的会话
         * @param preview_window 预览窗口
         * @param jpg_window
         * @param manual_preview 是否为手动预览
         * @param image_rotation 图像的旋转度数
         */
        void
        CreateSession(ANativeWindow *preview_window, ANativeWindow *jpg_window, bool manual_preview,
                      int32_t image_rotation);

        /**
         * 创建一个预览的会话
         * @param previewWindow 预览的窗口
         */
        void CreatePreviewSession(ANativeWindow *previewWindow);

        /**
         * 更新相机捕获会话状态
         * @param ses 相机捕获会话
         * @param state 更新的状态
         */
        void OnSessionState(ACameraCaptureSession *ses, CaptureSessionState state);

        /**
         * 启动图像预览
         * @param state 是否启用预览
         */
        void StartPreview(bool state);
    };

} // NCamera

#endif //TCAMERA_NCAMERA_H
