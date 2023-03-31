//
// Created by 36014 on 2023/3/31.
//
#include <camera/NdkCameraManager.h>
#include "native-log.h"

#include "camera_listeners.h"
#include "ncamera.h"

using namespace MyCamera;

void OnCameraAvailable(void *ctx, const char *id) {
    reinterpret_cast<NDKCamera *>(ctx)->OnCameraStatusChanged(id, true);
}

void OnCameraUnavailable(void *ctx, const char *id) {
    reinterpret_cast<NDKCamera *>(ctx)->OnCameraStatusChanged(id, false);
}


// CaptureSession state callbacks
void OnSessionClosed(void *ctx, ACameraCaptureSession *ses) {
    LOG_WARN("session %p closed", ses);
    reinterpret_cast<NDKCamera *>(ctx)->OnSessionState(
            ses, CaptureSessionState::CLOSED);
}

void OnSessionReady(void *ctx, ACameraCaptureSession *ses) {
    LOG_WARN("session %p ready", ses);
    reinterpret_cast<NDKCamera *>(ctx)->OnSessionState(ses,
                                                       CaptureSessionState::READY);
}

void OnSessionActive(void *ctx, ACameraCaptureSession *ses) {
    LOG_WARN("session %p active", ses);
    reinterpret_cast<NDKCamera *>(ctx)->OnSessionState(
            ses, CaptureSessionState::ACTIVE);
}
