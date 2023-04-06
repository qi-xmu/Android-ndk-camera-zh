package com.qi.tcamera

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.pm.PackageManager
import android.graphics.Matrix
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.util.Size
import android.view.*
import android.widget.FrameLayout
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat

class MainActivity : Activity(), TextureView.SurfaceTextureListener {
    private val mainTAG = "ncam"

    private var _textureView: TextureView? = null
    private var _surface: Surface? = null
    private var _cameraPreviewSize: Size? = null
    private var _ndkCamera: Long? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        Log.i(mainTAG, "OnCreate")

        if (isCamera2Device()) {

            if (ActivityCompat.checkSelfPermission(
                    this, Manifest.permission.CAMERA
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                ActivityCompat.requestPermissions(
                    this, arrayOf(Manifest.permission.CAMERA), 1
                )
            }
//            // 创建 纹理视图
            _textureView = findViewById<View>(R.id.texturePreview) as TextureView
            _textureView?.surfaceTextureListener = this
        }

        Log.i(mainTAG, "test Socket start")
        Log.i(mainTAG, "test Socket end")
    }


    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            window.decorView.systemUiVisibility =
                (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or View.SYSTEM_UI_FLAG_LAYOUT_STABLE or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_FULLSCREEN)
        }
    }

    override fun onStart() {
        super.onStart()
    }

    override fun onStop() {
        super.onStop()
    }

    private fun isCamera2Device(): Boolean {
        val camMgr = getSystemService(CAMERA_SERVICE) as CameraManager
        var camera2Dev = true
        try {
            val cameraIds = camMgr.cameraIdList
            if (cameraIds.isNotEmpty()) {
                for (id in cameraIds) {
                    val characteristics = camMgr.getCameraCharacteristics(id)
                    val deviceLevel =
                        characteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL)!!
                    val facing = characteristics.get(CameraCharacteristics.LENS_FACING)!!
//                    Log.i(mainTAG, "Camera $id level is $deviceLevel")
                    if (deviceLevel == CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY && facing == CameraMetadata.LENS_FACING_BACK) {
                        camera2Dev = false
                    }
                }
            }
        } catch (e: CameraAccessException) {
            e.printStackTrace()
            camera2Dev = false
        }
        return camera2Dev
    }

    /**
     * implement TextureView.SurfaceTextureListener
     */
    @SuppressLint("Recycle")
    @RequiresApi(Build.VERSION_CODES.R)
    override fun onSurfaceTextureAvailable(
        surface: SurfaceTexture, textureWidth: Int, textureHeight: Int
    ) {
        // 创建相机对象，这个对象由CPP维护
//        _ndkCamera = createCamera(320, 240)
        _ndkCamera = createCamera(640, 480)
        // 相机预览大小
        _cameraPreviewSize = getCompatiblePreviewSize(_ndkCamera!!)
        Log.i(mainTAG, "_cameraPreviewSize $_cameraPreviewSize")

        // 计算surface的大小
        val newWidth = textureWidth
        val newHeight = textureWidth * _cameraPreviewSize?.width!! / _cameraPreviewSize?.height!!
        _textureView?.layoutParams = FrameLayout.LayoutParams(newWidth, newHeight, Gravity.CENTER)
        Log.i(mainTAG, "Camera View:  $newWidth $newHeight")
        val matrix = Matrix()
        val width = newWidth
        val height = newHeight
        // 顺时针旋转 90 度
        matrix.setPolyToPoly(
            floatArrayOf(
                0f, 0f,  // top left
                width.toFloat(), 0f,  // top right
                0f, height.toFloat(),  // bottom left
                width.toFloat(), height.toFloat()
            ), 0, floatArrayOf(
                width.toFloat(), 0f,  // top left
                width.toFloat(), height.toFloat(),  // top right
                0f, 0f,  // bottom left
                0f, height.toFloat()
            ), 0, 4
        )
        _textureView?.setTransform(matrix)

        // 设置缓冲区大小
//        surface.setDefaultBufferSize(
//            _cameraPreviewSize?.width!!, _cameraPreviewSize?.height!!
//        )
        _surface = Surface(surface)
        onPreviewSurfaceCreated(_ndkCamera!!, _surface!!)
    }

    /**
     * implement TextureView.SurfaceTextureListener
     *
     */
    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {}

    /**
     * implement TextureView.SurfaceTextureListener
     */
    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        if (_ndkCamera != null && _surface != null) {
            onPreviewSurfaceDestroyed(_ndkCamera!!, _surface!!)
            _surface = null
        }
        if (_ndkCamera != null) {
            destroyCamera(_ndkCamera!!)
            _ndkCamera = null
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
    private external fun createCamera(width: Int, height: Int): Long
    private external fun onPreviewSurfaceCreated(ndkCamera: Long, surface: Surface)
    private external fun onPreviewSurfaceDestroyed(ndkCamera: Long, surface: Surface)
    private external fun destroyCamera(ndk_camera: Long)

    // 其他接口
    private external fun getCompatiblePreviewSize(ndkCamera: Long): Size?

    // 网络接口测试
//    private external fun testSocket()
//    private external fun testSocketClient()

    companion object {
        init {
            System.loadLibrary("tcamera")
        }
    }
}