//
// Created by 36014 on 2023/3/31.
//

#ifndef TCAMERA_CAMERA_LISTENERS_H
#define TCAMERA_CAMERA_LISTENERS_H


/**
 * 相机可用回调
 * @param ctx 上下文
 * @param id 相机标识符
 */
void OnCameraAvailable(void *ctx, const char *id);

/**
 * 相机不可用回调
 * @param ctx 上下文
 * @param id 相机标识符
 */
void OnCameraUnavailable(void *ctx, const char *id);


#endif //TCAMERA_CAMERA_LISTENERS_H
