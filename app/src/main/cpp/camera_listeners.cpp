//
// Created by 36014 on 2023/3/31.
//

#include "camera_listeners.h"
#include <camera/NdkCameraManager.h>
#include "ncamera.h"

using namespace MyCamera;

void OnCameraAvailable(void *ctx, const char *id) {
    reinterpret_cast<NDKCamera *>(ctx)->OnCameraStatusChanged(id, true);
}

void OnCameraUnavailable(void *ctx, const char *id) {
    reinterpret_cast<NDKCamera *>(ctx)->OnCameraStatusChanged(id, false);
}