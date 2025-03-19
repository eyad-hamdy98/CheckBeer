#include <jni.h>
#include <string>
#include <android/log.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <android/log.h>

#include "JNIHelper.hpp"

#define LOG_TAG "CheckBeer"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

// Forward declarations
bool checkCreator(JNIEnv* env);
bool checkField(JNIEnv* env);
bool checkCreators(JNIEnv* env);
bool checkPMProxy(JNIEnv* env, jobject context);
bool checkAppComponentFactory(JNIEnv* env);
bool checkApkPaths(JNIEnv* env, jobject context);
jobject getApplication(JNIEnv* env);
std::string getAppComponentFactory(JNIEnv* env, jobject context);


bool checkCreator(JNIEnv* env) {
    bool suspicious = false;
    const char* expectedCreatorName = "android.content.pm.PackageInfo$1";

    LOGI("Expected Creator Name: %s", expectedCreatorName);

    try {
        jobject creatorObject = jni::GetStaticField<jobject>(env, "android/content/pm/PackageInfo", "CREATOR", "Landroid/os/Parcelable$Creator;");
        jobject creatorClass = jni::CallMethod<jobject>(env, creatorObject, "getClass", "()Ljava/lang/Class;");
        jstring jCreatorName = jni::CallMethod<jstring>(env, creatorClass, "getName", "()Ljava/lang/String;");

        std::string currentCreatorName = jni::JStringToString(env, static_cast<jstring>(jCreatorName));
        LOGI("Current Creator Name: %s", currentCreatorName.c_str());

        if (currentCreatorName.find("android.content.pm.PackageInfo$") != 0) {
            LOGE("Current Creator Name does not start with expected prefix");
            suspicious = true;
        }

        if (expectedCreatorName != currentCreatorName) {
            LOGE("Creator name mismatch: expected=%s, found=%s", expectedCreatorName, currentCreatorName.c_str());
            suspicious = true;
        } else {
            LOGI("Creator name verification passed");
        }

    } catch (const std::exception& e) {
        LOGE("Error while checking creator: %s", e.what());
        suspicious = true;
    }
    LOGE("\n");
    return suspicious;
}


bool checkField(JNIEnv* env) {
    bool suspicious = false;

    try {
        jobject creatorObject = jni::GetStaticField<jobject>(env, "android/content/pm/PackageInfo", "CREATOR", "Landroid/os/Parcelable$Creator;");
        jobject creatorClass = jni::CallMethod<jobject>(env, creatorObject, "getClass", "()Ljava/lang/Class;");
        jobjectArray fieldArray = jni::CallMethod<jobjectArray>(env, creatorClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");

        jint fieldCount = env->GetArrayLength(fieldArray);
        LOGI("Expected Declared field count: 0, found: %d", fieldCount);

        if (fieldCount > 0) {
            suspicious = true;
            LOGE("Found %d unexpected fields in CREATOR object", fieldCount);

            for (jint i = 0; i < fieldCount; i++) {
                jni::ScopedLocalRef<jobject> field(env, env->GetObjectArrayElement(fieldArray, i));
                jstring fieldName = jni::CallMethod<jstring>(env, field.get(), "getName", "()Ljava/lang/String;");

                std::string fieldNameStr = jni::JStringToString(env, fieldName);
                LOGE("Declared Field Name: %s", fieldNameStr.c_str());
            }

        } else {
            LOGI("No suspicious fields found");
        }

    } catch (const std::exception& e) {
        LOGE("Error while checking fields: %s", e.what());
        suspicious = true;
    }
    LOGE("\n");
    return suspicious;
}

bool checkCreators(JNIEnv* env) {
    bool suspicious = false;

    try {
        jobject creator = jni::GetStaticField<jobject>(env, "android/content/pm/PackageInfo", "CREATOR", "Landroid/os/Parcelable$Creator;");

        jstring jCreatorString = jni::CallMethod<jstring>(env, creator, "toString", "()Ljava/lang/String;");
        std::string creatorString = jni::JStringToString(env, jCreatorString);

        if (!creatorString.empty()) {
            if (creatorString.find("android.content.pm.PackageInfo$") != 0) {
                LOGE("Creator object is suspicious: %s", creatorString.c_str());
                suspicious = true;
            } else {
                LOGI("Creator object is correct: %s", creatorString.c_str());
            }
        }

        jobject creatorClass = jni::CallMethod<jobject>(env, creator, "getClass", "()Ljava/lang/Class;");
        jobject creatorClassloader = jni::CallMethod<jobject>(env, creatorClass, "getClassLoader", "()Ljava/lang/ClassLoader;");

        jobject sysClassloader = jni::CallStaticMethod<jobject>(env, "java/lang/ClassLoader", "getSystemClassLoader", "()Ljava/lang/ClassLoader;");

        jobject creatorClassloaderClass = jni::CallMethod<jobject>(env, creatorClassloader, "getClass", "()Ljava/lang/Class;");
        jobject sysClassloaderClass = jni::CallMethod<jobject>(env, sysClassloader, "getClass", "()Ljava/lang/Class;");

        jstring jCreatorClassloaderName = jni::CallMethod<jstring>(env, creatorClassloaderClass, "getName", "()Ljava/lang/String;");
        jstring jSysClassloaderName = jni::CallMethod<jstring>(env, sysClassloaderClass, "getName", "()Ljava/lang/String;");

        std::string creatorClassloaderName = jni::JStringToString(env, jCreatorClassloaderName);
        std::string sysClassloaderName = jni::JStringToString(env, jSysClassloaderName);

        LOGI("Creator ClassLoader: %s", creatorClassloaderName.c_str());
        LOGI("System ClassLoader: %s", sysClassloaderName.c_str());

        if (creatorClassloader == nullptr || sysClassloader == nullptr) {
            LOGE("One of the class loaders is null");
            suspicious = true;
        } else if (sysClassloaderName == creatorClassloaderName) {
            LOGE("Class loaders are same - suspicious");
            suspicious = true;
        } else {
            LOGI("Class loaders are different - good");
        }

    } catch (const std::exception& e) {
        LOGE("Error while checking creator3: %s", e.what());
        suspicious = true;
    }
    LOGE("\n");
    return suspicious;
}

bool checkPMProxy(JNIEnv* env, jobject context) {
    bool suspicious = false;
    const char* expectedPMName = "android.content.pm.IPackageManager$Stub$Proxy";
    LOGI("Expected PM Name: %s", expectedPMName);

    try {
        jobject packageManager = jni::CallMethod<jobject>(env, context, "getPackageManager", "()Landroid/content/pm/PackageManager;");

        jobject packageManagerClass = jni::CallMethod<jobject>(env, packageManager, "getClass", "()Ljava/lang/Class;");
        jstring fieldNameStr = env->NewStringUTF("mPM");
        jobject mPMField = jni::CallMethod<jobject>(env, packageManagerClass, "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;", fieldNameStr);

        jni::CallMethod<void>(env, mPMField, "setAccessible", "(Z)V", JNI_TRUE);

        jobject mPM = jni::CallMethod<jobject>(env, mPMField, "get", "(Ljava/lang/Object;)Ljava/lang/Object;", packageManager);

        jobject mPMClass = jni::CallMethod<jobject>(env, mPM, "getClass", "()Ljava/lang/Class;");
        jstring jPMName = jni::CallMethod<jstring>(env, mPMClass, "getName", "()Ljava/lang/String;");

        std::string currentPMName = jni::JStringToString(env, jPMName);
        LOGI("Current PM Name: %s", currentPMName.c_str());

        env->DeleteLocalRef(fieldNameStr);

        if (expectedPMName != currentPMName) {
            LOGE("PM Name mismatch: expected=%s, found=%s", expectedPMName, currentPMName.c_str());
            suspicious = true;
        } else {
            LOGI("PM Name verification passed");
        }

    } catch (const std::exception& e) {
        LOGE("Error while checking PM proxy: %s", e.what());
        suspicious = true;
    }
    LOGE("\n");
    return suspicious;
}

jobject getApplication(JNIEnv* env) {
    try {
        jobject activityThread = jni::CallStaticMethod<jobject>(env, "android/app/ActivityThread", "currentActivityThread", "()Landroid/app/ActivityThread;");
        return jni::GetField<jobject>(env, activityThread, "mInitialApplication", "Landroid/app/Application;");
    } catch (const std::exception& e) {
        LOGE("Error getting application: %s", e.what());
        return nullptr;
    }
}

std::string getAppComponentFactory(JNIEnv* env, jobject context) {
    try {
        jobject packageManager = jni::CallMethod<jobject>(env, context, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jstring packageName = jni::CallMethod<jstring>(env, context, "getPackageName", "()Ljava/lang/String;");

        jobject applicationInfo = jni::CallMethod<jobject>(env, packageManager, "getApplicationInfo",
                                                                 "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;",
                                                                 packageName, 0);

        jstring appComponentFactory = jni::GetField<jstring>(env, applicationInfo, "appComponentFactory", "Ljava/lang/String;");

        if (appComponentFactory != nullptr) {
            return jni::JStringToString(env, appComponentFactory);
        }

    } catch (const std::exception& e) {
        LOGE("Error getting AppComponentFactory: %s", e.what());
    }

    return "";
}


bool checkAppComponentFactory(JNIEnv* env) {
    bool suspicious = false;
    const std::string originalAppComponentFactory = "androidx.core.app.CoreComponentFactory";
    LOGI("Expected AppComponentFactory: %s", originalAppComponentFactory.c_str());

    try {
        jobject application = getApplication(env);
        if (application != nullptr) {
            jobject applicationInfo = jni::CallMethod<jobject>(env, application, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");

            jstring jAppComponentFactory = jni::GetField<jstring>(env, applicationInfo, "appComponentFactory", "Ljava/lang/String;");

            if (jAppComponentFactory != nullptr) {
                std::string appComponentFactory = jni::JStringToString(env, jAppComponentFactory);
                LOGI("Detected AppComponentFactory: %s", appComponentFactory.c_str());

                if (appComponentFactory != originalAppComponentFactory) {
                    LOGE("AppComponentFactory mismatch: expected=%s, found=%s",
                         originalAppComponentFactory.c_str(), appComponentFactory.c_str());
                    suspicious = true;
                } else {
                    LOGI("AppComponentFactory verification passed");
                }
            } else {
                LOGI("AppComponentFactory is null");
            }
        }
    } catch (const std::exception& e) {
        LOGE("Error while checking AppComponentFactory: %s", e.what());
        suspicious = true;
    }
    LOGE("\n");
    return suspicious;
}

std::string getApkPath(JNIEnv* env, jobject context) {
    try {
        jobject applicationInfo = jni::CallMethod<jobject>(env, context, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");

        jstring jSourceDir = jni::GetField<jstring>(env, applicationInfo, "sourceDir", "Ljava/lang/String;");
        return jni::JStringToString(env, jSourceDir);

    } catch (const std::exception& e) {
        LOGE("Error getting APK path: %s", e.what());
        return "";
    }
}


bool checkApkPaths(JNIEnv* env, jobject context) {
    bool suspicious = false;

    try {
        jstring jResourcePath = jni::CallMethod<jstring>(env, context, "getPackageResourcePath", "()Ljava/lang/String;");
        std::string resourcePath = jni::JStringToString(env, jResourcePath);

        jstring jCodePath = jni::CallMethod<jstring>(env, context, "getPackageCodePath", "()Ljava/lang/String;");
        std::string codePath = jni::JStringToString(env, jCodePath);

        jobject applicationInfo = jni::CallMethod<jobject>(env, context, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");

        jstring jSourceDir = jni::GetField<jstring>(env, applicationInfo, "sourceDir", "Ljava/lang/String;");
        std::string sourceDir = jni::JStringToString(env, jSourceDir);

        jstring jPublicSourceDir = jni::GetField<jstring>(env, applicationInfo, "publicSourceDir", "Ljava/lang/String;");
        std::string publicSourceDir = jni::JStringToString(env, jPublicSourceDir);

        jobject packageManager = jni::CallMethod<jobject>(env, context, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jstring packageName = jni::CallMethod<jstring>(env, context, "getPackageName", "()Ljava/lang/String;");

        jobject applicationInfo2 = jni::CallMethod<jobject>(env, 
                packageManager,
                "getApplicationInfo",
                "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;",
                packageName,
                0
        );

        jstring jPackageInfoSourceDir = jni::GetField<jstring>(env, applicationInfo2, "sourceDir", "Ljava/lang/String;");
        std::string packageInfoSourceDir = jni::JStringToString(env, jPackageInfoSourceDir);

        std::string nativeApkPath;
        try {
            nativeApkPath = getApkPath(env, context);
            LOGI("Native APK Path: %s", nativeApkPath.c_str());
        } catch (const std::exception& e) {
            LOGE("Failed to get native APK path: %s", e.what());
        }

        LOGI("Package Resource Path: %s", resourcePath.c_str());
        LOGI("Package Code Path: %s", codePath.c_str());
        LOGI("ApplicationInfo SourceDir: %s", sourceDir.c_str());
        LOGI("ApplicationInfo publicSourceDir: %s", publicSourceDir.c_str());
        LOGI("PackageManager SourceDir: %s", packageInfoSourceDir.c_str());

        std::vector<std::string> paths = {resourcePath, codePath, sourceDir, publicSourceDir, packageInfoSourceDir};
        if (!nativeApkPath.empty()) {
            paths.push_back(nativeApkPath);
        }

        bool allPathsSame = true;
        for (size_t i = 1; i < paths.size(); i++) {
            if (paths[i] != paths[0]) {
                allPathsSame = false;
                LOGE("Path mismatch: %s != %s", paths[0].c_str(), paths[i].c_str());
                suspicious = true;
                break;
            }
        }

        if (!allPathsSame) {
            LOGE("APK paths are inconsistent");
        } else {
            LOGI("APK paths are consistent");
        }

        bool allStartWithDataApp = true;
        for (const auto& path : paths) {
            if (path.find("/data/app/") != 0) {
                allStartWithDataApp = false;
                LOGE("Path doesn't start with /data/app/: %s", path.c_str());
                suspicious = true;
                break;
            }
        }

        if (!allStartWithDataApp) {
            LOGE("Not all APK paths start with /data/app/");
        } else {
            LOGI("All APK paths start with /data/app/");
        }

        bool allEndWithBaseApk = true;
        for (const auto& path : paths) {
            if (path.length() < 9 || path.substr(path.length() - 9) != "/base.apk") {
                allEndWithBaseApk = false;
                LOGE("Path doesn't end with /base.apk: %s", path.c_str());
                suspicious = true;
                break;
            }
        }

        if (!allEndWithBaseApk) {
            LOGE("Not all APK paths end with /base.apk");
        } else {
            LOGI("All APK paths end with /base.apk");
        }

        for (const auto& path : paths) {
            struct stat st;
            if (stat(path.c_str(), &st) == 0) {
                bool correctPermissions = (st.st_mode & 0777) == 0644;
                bool correctOwner = st.st_uid == 1000;

                if (!correctPermissions) {
                    LOGE("Path %s has incorrect permissions: %o (expected 644)",
                         path.c_str(), st.st_mode & 0777);
                    suspicious = true;
                }

                if (!correctOwner) {
                    LOGE("Path %s has incorrect owner: %d (expected 1000/system)",
                         path.c_str(), st.st_uid);
                    suspicious = true;
                }

                bool canChangePermissions = chmod(path.c_str(), 0777) == 0;
                if (canChangePermissions) {
                    LOGE("Path %s permissions could be changed - suspicious", path.c_str());
                    suspicious = true;
                    chmod(path.c_str(), 0644);
                } else {
                    LOGI("Path %s permissions check passed", path.c_str());
                }
            } else {
                LOGE("Error accessing path: %s (errno: %d)", path.c_str(), errno);
                suspicious = true;
            }
        }

    } catch (const std::exception& e) {
        LOGE("Error while checking APK paths: %s", e.what());
        suspicious = true;
    }
    LOGE("\n");
    return suspicious;
}

bool checkSignatureBypass(JNIEnv* env, jobject context) {
    LOGI("Starting native signature checks");
    LOGI("----------START-----------------");
    bool suspicious = false;

    suspicious |= checkCreator(env);
    suspicious |= checkField(env);
    suspicious |= checkCreators(env);
    suspicious |= checkPMProxy(env, context);
    suspicious |= checkAppComponentFactory(env);
    suspicious |= checkApkPaths(env, context);
    LOGE("\n");
    LOGI("Native signature checks completed, suspicious: %d", suspicious);
    LOGI("---------------END-----------------");

    return suspicious;
}