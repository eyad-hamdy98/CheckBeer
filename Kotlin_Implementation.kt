package com.signature.check.android

import android.annotation.SuppressLint
import android.app.Activity
import android.app.Application
import android.content.Context
import android.content.pm.PackageInfo
import android.os.Build
import android.os.Bundle
import android.text.Spannable
import android.text.SpannableString
import android.text.style.ForegroundColorSpan
import android.util.Log
import android.widget.TextView
import java.lang.reflect.Field
import android.graphics.Color
import android.content.pm.PackageManager
import java.io.File

class MainActivity : Activity() {
    private var logTextView: TextView? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.main)
        logTextView = findViewById<TextView>(R.id.logTextView)
        checkCreator()
        checkField()
        checkCreators()
        checkPMProxy()
        checkAppComponentFactory()
        checkApkPaths(this)
    }

    fun getAppComponentFactory(context: Context): String? {
        return try {
            val applicationInfo = context.packageManager.getApplicationInfo(
                context.packageName,
                PackageManager.GET_META_DATA
            )
            applicationInfo.appComponentFactory
        } catch (e: PackageManager.NameNotFoundException) {
            null
        }
    }


    private fun logMessage(message: String, color: Int = Color.BLACK) {
        Log.d(TAG, message)
        runOnUiThread {
            val spannable = SpannableString(message + "\n")
            spannable.setSpan(ForegroundColorSpan(color), 0, spannable.length, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE)
            logTextView!!.append(spannable)
        }
    }



    @SuppressLint("PrivateApi", "DiscouragedPrivateApi")
    fun getmyApplication(): Application? {
        try {
            val MyClassName = Class.forName("android.app.ActivityThread")
            val currentActivityThreadMethod = MyClassName.getDeclaredMethod("currentActivityThread")
            val activityThread = currentActivityThreadMethod.invoke(null)
            val application = getObjectField(activityThread, "mInitialApplication") as Application
            return application
        } catch (e: Exception) {
            logMessage("Error while retrieving Application instance: " + e.message)
            return null
        }
    }

    private fun checkCreator() {
        val expectedCreatorName = "android.content.pm.PackageInfo$1"
        logMessage("Expected Creator Name: $expectedCreatorName")
        var currentCreatorName = ""
        try {
            val creatorObject: Any = PackageInfo.CREATOR
            currentCreatorName = creatorObject.javaClass.name
            logMessage("Current Creator Name: $currentCreatorName")

            if (!currentCreatorName.startsWith("android.content.pm.PackageInfo$")) {
                logMessage("Current Creator Name not start with expected CreatorName",Color.RED)
            }
        } catch (e: Exception) {
            logMessage("Error while checking creator: " + e.message)
        }

        if (expectedCreatorName != currentCreatorName) {
            logMessage("Expected and Current Creator Name are not same. SUSPICIOUS",Color.RED)
        } else {
            logMessage("Expected and Current Creator Name are same",Color.GREEN)
        }
        logMessage("\n")
    }


    private fun checkField() {
        try {
            val creatorObject: Any = PackageInfo.CREATOR
            val declaredFields = creatorObject.javaClass.declaredFields
            val fieldCount = declaredFields.size
            logMessage("Expected Declared field count: 0")

            if (fieldCount > 0) {
                logMessage("Declared fields count: $fieldCount")
                declaredFields.forEach { field ->
                    logMessage("Declared Field Name: ${field.name}", Color.RED)
                }
                logMessage("Declared Field > 0. SUSPICIOUS", Color.RED)
            } else {
                logMessage("Current Declared field count: $fieldCount")
                logMessage("Everything is okay. No suspicious fields.", Color.GREEN)
            }
        } catch (e: Exception) {
            logMessage("Error while checking checkField: ${e.message}")
        }
        logMessage("\n")
    }


    private fun checkCreators() {
        try {
            val creatorField = PackageInfo::class.java.getField("CREATOR")
            creatorField.isAccessible = true
            val creator = creatorField[null]?.toString()
            if (creator != null) {
                if (!creator.startsWith("android.content.pm.PackageInfo$")) {
                    logMessage("Creator object is SUSPICIOUS ${creator}", Color.RED)
                } else {
                    logMessage("Creator object is correct ${creator}",Color.GREEN)
                }
            }

            if (creator != null) {
                val creatorClassloader = creator.javaClass.classLoader
                val sysClassloader = ClassLoader.getSystemClassLoader()
                //logMessage("Creator ClassLoader Instance: ${creatorClassloader}")
                //logMessage("System ClassLoader Instance: ${sysClassloader}")

                logMessage("Creator ClassLoader: ${creatorClassloader.javaClass.name}")
                logMessage("System ClassLoader: ${sysClassloader.javaClass.name}")

                if (creatorClassloader == null || sysClassloader == null) {
                    logMessage("One of the class loaders is null")
                    return
                }
                if (sysClassloader.javaClass.name == creatorClassloader.javaClass.name) {
                    logMessage("Class loaders are same. SUSPICIOUS",Color.RED)
                } else {
                    logMessage("Class loaders are different", Color.GREEN)
                }
            }
        } catch (e: Throwable) {
            logMessage("Error while checking creator3: ${e.message}")
        }
        logMessage("\n")
    }


    private fun checkPMProxy() {
        val expectedPMName = "android.content.pm.IPackageManager\$Stub\$Proxy"
        logMessage("Expected PM Name: $expectedPMName")

        var currentPMName = ""
        try {
            val packageManager = packageManager
            val mPMField = packageManager.javaClass.getDeclaredField("mPM")
            mPMField.isAccessible = true
            val mPM = mPMField[packageManager]
            currentPMName = mPM.javaClass.name
            logMessage("Current PM Name: $currentPMName")
        } catch (e: Exception) {
            logMessage("Error while checking PM proxy: " + e.message)
        }
        if(expectedPMName != currentPMName){
            logMessage("Expected PM Name and Current PM Name are not same. SUSPICIOUS",Color.RED)
        } else {
            logMessage("Expected PM Name and Current PM Name are same",Color.GREEN)
        }
        logMessage("\n")
    }

    private fun checkAppComponentFactory() {
        val originalappComponentFactory = "androidx.core.app.CoreComponentFactory"
        logMessage("Expected AppComponentFactory :${originalappComponentFactory}")
        try {
            val appComponentFactory = getmyAppComponentFactory()
            val factory = getAppComponentFactory(this)
            if(factory != null){
                logMessage("Detected AppComponentFactory1st: $appComponentFactory")
            }
            if (appComponentFactory != null) {
                logMessage("Detected AppComponentFactory2nd: $appComponentFactory")
                if (appComponentFactory != originalappComponentFactory) {
                    logMessage(
                        "Expected appComponentFactory and original appComponentFactory are not same. SUSPICIOUS",
                        Color.RED
                    )
                } else {
                    logMessage(
                        "Expected appComponentFactory and original appComponentFactory are same",
                        Color.GREEN
                    )
                }
            } else {
                logMessage("Detected AppComponentFactory Null")
            }
        } catch (e: Exception){
            logMessage("Error while checking AppComponentFactory: ${e.message}")
        }
        logMessage("\n")

    }




    private fun checkApkPaths(context: Context) {
        try {
            val resourcePath = context.packageResourcePath
            val codePath = context.packageCodePath
            val sourceDir = context.applicationInfo.sourceDir
            val publicSourceDir = context.applicationInfo.publicSourceDir
            val packageManager = context.packageManager
            val packageInfoSourceDir = packageManager.getApplicationInfo(context.packageName, 0).sourceDir

            logMessage("Package Resource Path: $resourcePath")
            logMessage("Package Code Path: $codePath")
            logMessage("ApplicationInfo SourceDir: $sourceDir")
            logMessage("ApplicationInfo publicSourceDir: $publicSourceDir")
            logMessage("PackageManager SourceDir: $packageInfoSourceDir")

            val pathsSet = setOf(resourcePath, codePath, sourceDir, publicSourceDir, packageInfoSourceDir)

            if (pathsSet.size > 1) {
                logMessage("Something suspicious: APK paths do not match!", Color.RED)
            } else {
                logMessage("APK paths are consistent.", Color.GREEN)
            }

            if (pathsSet.any { it == null || !it.startsWith("/data/app/") }) {
                logMessage("Something suspicious: Not all APK paths start with /data/app/!", Color.RED)
            } else {
                logMessage("All APK paths start with /data/app/", Color.GREEN)
            }

            if (pathsSet.any { it == null || !it.endsWith("/base.apk") }) {
                logMessage("Something suspicious: Not all APK paths end with /base.apk!", Color.RED)
            } else {
                logMessage("All APK paths end with /base.apk.", Color.GREEN)
            }

            for (path in pathsSet) {
                if (path != null) {
                    val file = File(path)
                    val process = Runtime.getRuntime().exec("ls -l $path")
                    val output = process.inputStream.bufferedReader().readLine()
                    process.waitFor()

                    output?.let {
                        val parts = it.split("\\s+".toRegex())

                        if (parts.size >= 3) {
                            val permissions = parts[0]
                            val owner = parts[2]
                            val group = parts[3]

                            if (!permissions.endsWith("rw-r--r--")) {
                                logMessage("Something suspicious: $path does not have 644 permissions ($permissions)", Color.RED)
                            }

                            if (owner != "system" || group != "system") {
                                logMessage("Something suspicious: $path owner/group is not system ($owner:$group)", Color.RED)
                            }
                        }
                    }

                    // Try to change permission
                    try {
                        val chmodProcess = Runtime.getRuntime().exec("chmod 777 $path")
                        val exitCode = chmodProcess.waitFor()
                        if (exitCode == 0) {
                            logMessage("Something suspicious: chmod succeeded on $path", Color.RED)
                        } else {
                            logMessage("Permission check passed for $path", Color.GREEN)
                        }
                    } catch (e: Exception) {
                        logMessage("Error changing permission for $path: ${e.message}", Color.YELLOW)
                    }
                }
            }
        } catch (e: Throwable) {
            logMessage("Error while retrieving APK paths: ${e.message}", Color.RED)
        }
    }



    private fun getmyAppComponentFactory(): String? {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                return getmyApplication()!!.applicationInfo.appComponentFactory
            } else {
                return null
            }
        } catch (e: Exception) {
            logMessage("Error while retrieving AppComponentFactory: " + e.message)
        }
        return null
    }

    @Throws(Exception::class)
    private fun getObjectField(obj: Any, fieldName: String): Any {
        val field = obj.javaClass.getDeclaredField(fieldName)
        field.isAccessible = true
        val fieldValue = field[obj]
        return fieldValue
    }

    companion object {
        private const val TAG = "SigKill"
    }
}
