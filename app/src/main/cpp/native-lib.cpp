#include <jni.h>
#include <string>

#include "ncamera.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_qi_tcamera_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    // test code
    auto camera = MyCamera::NDKCamera();

    ImageFormat compatibleCameraRes;
    camera.MatchCaptureSizeRequest(1920, 1080, &compatibleCameraRes, nullptr);

    // test code end
    return env->NewStringUTF(hello.c_str());
}

int Camera() {
    return 0;
}