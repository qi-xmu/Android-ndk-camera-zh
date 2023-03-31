//
// Created by 36014 on 2023/4/1.
//

#ifndef TCAMERA_NATIVE_LOG_H
#define TCAMERA_NATIVE_LOG_H

#include <android/log.h>

#define LOG_TAG    "Ncam"
#define LOG_INFO(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOG_WARN(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOG_ERR(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif //TCAMERA_NATIVE_LOG_H
