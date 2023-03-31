package com.qi.tcamera

import android.annotation.SuppressLint
import android.app.Activity
import android.graphics.SurfaceTexture
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.util.Size
import android.view.Gravity
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.widget.FrameLayout
import androidx.annotation.RequiresApi

class MainActivity : Activity(), TextureView.SurfaceTextureListener {
    private val TAG = "ncam";

    private var _textureView: TextureView? = null;
    private var _surface: Surface? = null;
    private var _cameraPreviewSize: Size? = null;
    private var _ndkCamera: Long? = null;

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        Log.i(TAG, "OnCreate");

        // 创建 纹理视图
        _textureView = findViewById<View>(R.id.texturePreview) as TextureView
        _textureView?.surfaceTextureListener = this
//        stringFromJNI()

        // 相机预览大小
        _cameraPreviewSize = Size(1080, 1920);
    }

    /**
     * implement TextureView.SurfaceTextureListener
     */
    @SuppressLint("Recycle")
    @RequiresApi(Build.VERSION_CODES.R)
    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        // 读取屏幕的物理宽和高
        val display = this.display!!
        val phy_width = display.mode.physicalWidth
        val phy_height = display.mode.physicalHeight
        Log.i(
            TAG, "Physical View: $phy_height $phy_height"
        );
        // 获取相机对象
        _ndkCamera = createCamera(phy_width, phy_height);

        val rotation = display.rotation;
        val newWidth = width
        var newHeight = height * _cameraPreviewSize?.width!! / _cameraPreviewSize?.height!!
        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_180 == rotation) {
            newHeight = height * _cameraPreviewSize?.height!! / _cameraPreviewSize?.width!!
        }

        _textureView?.layoutParams = FrameLayout.LayoutParams(newWidth, newHeight, Gravity.CENTER)

        Log.i(
            TAG, "Texture View: $rotation $width $height"
        );
        // surface本质是一个图像生产者
        _surface = Surface(surface)

        onPreviewSurfaceCreated(_ndkCamera!!, _surface!!);

    }

    /**
     * implement TextureView.SurfaceTextureListener
     *
     */
    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {

    }

    /**
     * implement TextureView.SurfaceTextureListener
     */
    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        if (_ndkCamera != null && _surface != null) {
            onPreviewSurfaceDestroyed(_ndkCamera!!, _surface!!);
        }
        _surface = null;
        if (_ndkCamera != null) {
            destroyCamera(_ndkCamera!!);
        }
        return true
    }

    /**
     * implement TextureView.SurfaceTextureListener
     */
    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}


    /**
     * A native method that is implemented by the 'tcamera' native library,
     * which is packaged with this application.
     */


    private external fun createCamera(width: Int, height: Int): Long;
    private external fun onPreviewSurfaceCreated(ndkCamera: Long, surface: Surface);
    private external fun onPreviewSurfaceDestroyed(ndkCamera: Long, surface: Surface);
    private external fun destroyCamera(ndk_camera: Long);

    companion object {
        init {
            System.loadLibrary("tcamera")
        }
    }
}