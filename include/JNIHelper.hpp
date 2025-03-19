#pragma once

#include <jni.h>
#include <string>
#include <stdexcept>
#include <vector>

namespace jni {

    // Custom exception for JNI errors
    class JNIException : public std::runtime_error {
    public:
        JNIException(const char* message, jthrowable javaException = nullptr)
                : std::runtime_error(message), javaThrowable(javaException) {}

        jthrowable getJavaException() const { return javaThrowable; }

    private:
        jthrowable javaThrowable;
    };

    // Macro to check for JNI exceptions
#define JNI_CHECK_EXCEPTION(env)                                        \
        do {                                                                \
            if ((env)->ExceptionCheck()) {                                  \
                jthrowable exception = (env)->ExceptionOccurred();          \
                (env)->ExceptionDescribe();                                 \
                (env)->ExceptionClear();                                    \
                throw jni::JNIException("JNI exception occurred", exception); \
            }                                                               \
        } while (0)

    // RAII wrapper for JNI local references
    template <typename T>
    class ScopedLocalRef {
    public:
        ScopedLocalRef(JNIEnv* env, T ref) : env_(env), ref_(ref) {}

        ~ScopedLocalRef() {
            if (ref_) env_->DeleteLocalRef(ref_);
        }

        T get() const { return ref_; }

        T release() {
            T temp = ref_;
            ref_ = nullptr;
            return temp;
        }

        // Disable copy
        ScopedLocalRef(const ScopedLocalRef&) = delete;
        ScopedLocalRef& operator=(const ScopedLocalRef&) = delete;

    private:
        JNIEnv* env_;
        T ref_;
    };

    // Convert a Java string to a C++ string
    inline std::string JStringToString(JNIEnv* env, jstring jstr) {
        if (!jstr) return {};

        const char* chars = env->GetStringUTFChars(jstr, nullptr);
        if (!chars) return {};

        std::string result(chars);
        env->ReleaseStringUTFChars(jstr, chars);
        return result;
    }

    // Create a Java string from a C++ string
    inline jstring StringToJString(JNIEnv* env, const std::string& str) {
        return env->NewStringUTF(str.c_str());
    }

    // Find a Java class
    inline jclass FindClass(JNIEnv* env, const char* className) {
        jclass cls = env->FindClass(className);
        JNI_CHECK_EXCEPTION(env);
        return cls;
    }

    // Get a method ID
    inline jmethodID GetMethodID(JNIEnv* env, jclass cls, const char* methodName, const char* signature) {
        jmethodID mid = env->GetMethodID(cls, methodName, signature);
        JNI_CHECK_EXCEPTION(env);
        return mid;
    }

    // Get a static method ID
    inline jmethodID GetStaticMethodID(JNIEnv* env, jclass cls, const char* methodName, const char* signature) {
        jmethodID mid = env->GetStaticMethodID(cls, methodName, signature);
        JNI_CHECK_EXCEPTION(env);
        return mid;
    }

    // Get a field ID
    inline jfieldID GetFieldID(JNIEnv* env, jclass cls, const char* fieldName, const char* signature) {
        jfieldID fid = env->GetFieldID(cls, fieldName, signature);
        JNI_CHECK_EXCEPTION(env);
        return fid;
    }

    // Get a static field ID
    inline jfieldID GetStaticFieldID(JNIEnv* env, jclass cls, const char* fieldName, const char* signature) {
        jfieldID fid = env->GetStaticFieldID(cls, fieldName, signature);
        JNI_CHECK_EXCEPTION(env);
        return fid;
    }

    // Template specialization declarations for type traits
    template <typename T> struct JNITypeTraits;

    // Specialization for jobject
    template <> struct JNITypeTraits<jobject> {
        static constexpr const char* signature = "Ljava/lang/Object;";

        // Field access methods
        static jobject GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jobject result = env->GetObjectField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jobject GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jobject result = env->GetStaticObjectField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jobject CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jobject result = env->CallObjectMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jobject CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jobject result = env->CallStaticObjectMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jstring
    template <> struct JNITypeTraits<jstring> {
        static constexpr const char* signature = "Ljava/lang/String;";

        // Field access methods
        static jstring GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jstring result = static_cast<jstring>(env->GetObjectField(obj, fid));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jstring GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jstring result = static_cast<jstring>(env->GetStaticObjectField(cls, fid));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jstring CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jstring result = static_cast<jstring>(env->CallObjectMethodA(obj, mid, args));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jstring CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jstring result = static_cast<jstring>(env->CallStaticObjectMethodA(cls, mid, args));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for void return type
    template <> struct JNITypeTraits<void> {
        static constexpr const char* signature = "V";

        // Method call methods
        static void CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            env->CallVoidMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
        }
        static void CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            env->CallStaticVoidMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
        }
    };

    // Specialization for jboolean
    template <> struct JNITypeTraits<jboolean> {
        static constexpr const char* signature = "Z";

        // Field access methods
        static jboolean GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jboolean result = env->GetBooleanField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jboolean GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jboolean result = env->GetStaticBooleanField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jboolean CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jboolean result = env->CallBooleanMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jboolean CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jboolean result = env->CallStaticBooleanMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jbyte
    template <> struct JNITypeTraits<jbyte> {
        static constexpr const char* signature = "B";

        // Field access methods
        static jbyte GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jbyte result = env->GetByteField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jbyte GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jbyte result = env->GetStaticByteField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jbyte CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jbyte result = env->CallByteMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jbyte CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jbyte result = env->CallStaticByteMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jchar
    template <> struct JNITypeTraits<jchar> {
        static constexpr const char* signature = "C";

        // Field access methods
        static jchar GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jchar result = env->GetCharField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jchar GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jchar result = env->GetStaticCharField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jchar CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jchar result = env->CallCharMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jchar CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jchar result = env->CallStaticCharMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jshort
    template <> struct JNITypeTraits<jshort> {
        static constexpr const char* signature = "S";

        // Field access methods
        static jshort GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jshort result = env->GetShortField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jshort GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jshort result = env->GetStaticShortField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jshort CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jshort result = env->CallShortMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jshort CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jshort result = env->CallStaticShortMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jint
    template <> struct JNITypeTraits<jint> {
        static constexpr const char* signature = "I";

        // Field access methods
        static jint GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jint result = env->GetIntField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jint GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jint result = env->GetStaticIntField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jint CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jint result = env->CallIntMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jint CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jint result = env->CallStaticIntMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jlong
    template <> struct JNITypeTraits<jlong> {
        static constexpr const char* signature = "J";

        // Field access methods
        static jlong GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jlong result = env->GetLongField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jlong GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jlong result = env->GetStaticLongField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jlong CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jlong result = env->CallLongMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jlong CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jlong result = env->CallStaticLongMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jfloat
    template <> struct JNITypeTraits<jfloat> {
        static constexpr const char* signature = "F";

        // Field access methods
        static jfloat GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jfloat result = env->GetFloatField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jfloat GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jfloat result = env->GetStaticFloatField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jfloat CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jfloat result = env->CallFloatMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jfloat CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jfloat result = env->CallStaticFloatMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Specialization for jdouble
    template <> struct JNITypeTraits<jdouble> {
        static constexpr const char* signature = "D";

        // Field access methods
        static jdouble GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jdouble result = env->GetDoubleField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jdouble GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jdouble result = env->GetStaticDoubleField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Method call methods
        static jdouble CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jdouble result = env->CallDoubleMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jdouble CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jdouble result = env->CallStaticDoubleMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // Helper to convert arguments to jvalue array
    template <typename... Args>
    class ArgsToJValues {
    public:
        ArgsToJValues(JNIEnv* env, Args... args) {
            convertArgs(env, 0, args...);
        }

        const jvalue* get() const { return values_; }

    private:
        jvalue values_[sizeof...(Args) > 0 ? sizeof...(Args) : 1]; // Ensure at least one element

        template <typename T, typename... RestArgs>
        void convertArgs(JNIEnv* env, int index, T value, RestArgs... rest) {
            setJValue(env, index, value);
            convertArgs(env, index + 1, rest...);
        }

        void convertArgs(JNIEnv*, int) {
            // Base case, no more arguments to convert
        }

        // Handle primitive types
        void setJValue(JNIEnv*, int index, jboolean value) { values_[index].z = value; }
        void setJValue(JNIEnv*, int index, jbyte value) { values_[index].b = value; }
        void setJValue(JNIEnv*, int index, jchar value) { values_[index].c = value; }
        void setJValue(JNIEnv*, int index, jshort value) { values_[index].s = value; }
        void setJValue(JNIEnv*, int index, jint value) { values_[index].i = value; }
        void setJValue(JNIEnv*, int index, jlong value) { values_[index].j = value; }
        void setJValue(JNIEnv*, int index, jfloat value) { values_[index].f = value; }
        void setJValue(JNIEnv*, int index, jdouble value) { values_[index].d = value; }

        // Handle object types - explicitly handle nullptr
        void setJValue(JNIEnv*, int index, std::nullptr_t) { values_[index].l = nullptr; }

        // Handle object types
        void setJValue(JNIEnv*, int index, jobject value) { values_[index].l = value; }

        // Handle other JNI reference types
        template <typename T>
        typename std::enable_if<std::is_convertible<T, jobject>::value, void>::type
        setJValue(JNIEnv*, int index, T value) { values_[index].l = value; }

        // Handle C++ string conversion to Java string
        void setJValue(JNIEnv* env, int index, const std::string& value) {
            jstring jstr = StringToJString(env, value);
            values_[index].l = jstr;
        }

        void setJValue(JNIEnv* env, int index, const char* value) {
            if (value == nullptr) {
                values_[index].l = nullptr;
            } else {
                jstring jstr = env->NewStringUTF(value);
                values_[index].l = jstr;
            }
        }
    };

// Generic CallMethod template function that works with no arguments
    template <typename RetType, typename... Args>
    RetType CallMethod(JNIEnv* env, jobject obj, const char* methodName, const char* signature, Args... args) {
        jclass cls = env->GetObjectClass(obj);
        ScopedLocalRef<jclass> clsRef(env, cls);

        jmethodID mid = GetMethodID(env, cls, methodName, signature);

        if constexpr (sizeof...(Args) == 0) {
            // Handle the no-arguments case using the direct call methods
            if constexpr (std::is_same_v<RetType, void>) {
                env->CallVoidMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return;
            }
            else if constexpr (std::is_same_v<RetType, jboolean>) {
                jboolean result = env->CallBooleanMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jbyte>) {
                jbyte result = env->CallByteMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jchar>) {
                jchar result = env->CallCharMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jshort>) {
                jshort result = env->CallShortMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jint>) {
                jint result = env->CallIntMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jlong>) {
                jlong result = env->CallLongMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jfloat>) {
                jfloat result = env->CallFloatMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jdouble>) {
                jdouble result = env->CallDoubleMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jstring> || std::is_convertible_v<RetType, jobject>) {
                RetType result = static_cast<RetType>(env->CallObjectMethod(obj, mid));
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else {
                static_assert(std::is_void_v<RetType> || std::is_convertible_v<RetType, jobject>,
                              "Unsupported return type for CallMethod");
                return {};
            }
        }
        else {
            // Handle the case with arguments using ArgsToJValues
            ArgsToJValues<Args...> jvalues(env, args...);
            return JNITypeTraits<RetType>::CallMethod(env, obj, mid, jvalues.get());
        }
    }

// Generic CallStaticMethod template function that works with no arguments
    template <typename RetType, typename... Args>
    RetType CallStaticMethod(JNIEnv* env, const char* className, const char* methodName, const char* signature, Args... args) {
        jclass cls = FindClass(env, className);
        ScopedLocalRef<jclass> clsRef(env, cls);

        jmethodID mid = GetStaticMethodID(env, cls, methodName, signature);

        if constexpr (sizeof...(Args) == 0) {
            // Handle the no-arguments case using the direct call methods
            if constexpr (std::is_same_v<RetType, void>) {
                env->CallStaticVoidMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return;
            }
            else if constexpr (std::is_same_v<RetType, jboolean>) {
                jboolean result = env->CallStaticBooleanMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jbyte>) {
                jbyte result = env->CallStaticByteMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jchar>) {
                jchar result = env->CallStaticCharMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jshort>) {
                jshort result = env->CallStaticShortMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jint>) {
                jint result = env->CallStaticIntMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jlong>) {
                jlong result = env->CallStaticLongMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jfloat>) {
                jfloat result = env->CallStaticFloatMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jdouble>) {
                jdouble result = env->CallStaticDoubleMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jstring> || std::is_convertible_v<RetType, jobject>) {
                RetType result = static_cast<RetType>(env->CallStaticObjectMethod(cls, mid));
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else {
                static_assert(std::is_void_v<RetType> || std::is_convertible_v<RetType, jobject>,
                              "Unsupported return type for CallStaticMethod");
                return {};
            }
        }
        else {
            // Handle the case with arguments using ArgsToJValues
            ArgsToJValues<Args...> jvalues(env, args...);
            return JNITypeTraits<RetType>::CallStaticMethod(env, cls, mid, jvalues.get());
        }
    }

    // Create a new Java object
    template<typename... Args>
    jobject NewObject(JNIEnv* env, const char* className, const char* constructorSignature, Args... args) {
        jclass cls = FindClass(env, className);
        ScopedLocalRef<jclass> clsRef(env, cls);

        jmethodID constructor = GetMethodID(env, cls, "<init>", constructorSignature);

        ArgsToJValues<Args...> jvalues(env, args...);
        jobject obj = env->NewObjectA(cls, constructor, jvalues.get());
        JNI_CHECK_EXCEPTION(env);

        return obj;
    }

    // Generic GetField template function for instance fields
    template <typename T>
    T GetField(JNIEnv* env, jobject obj, const char* fieldName, const char* signature = nullptr) {
        jclass cls = env->GetObjectClass(obj);
        ScopedLocalRef<jclass> clsRef(env, cls);

        const char* fieldSig = signature ? signature : JNITypeTraits<T>::signature;
        jfieldID fid = GetFieldID(env, cls, fieldName, fieldSig);

        if (signature) {
            // For object types with custom signatures, use the jobject traits and cast
            return static_cast<T>(JNITypeTraits<jobject>::GetField(env, obj, fid));
        } else {
            return JNITypeTraits<T>::GetField(env, obj, fid);
        }
    }

    // Generic GetStaticField template function for static fields
    template <typename T>
    T GetStaticField(JNIEnv* env, const char* className, const char* fieldName, const char* signature = nullptr) {
        jclass cls = FindClass(env, className);
        ScopedLocalRef<jclass> clsRef(env, cls);

        const char* fieldSig = signature ? signature : JNITypeTraits<T>::signature;
        jfieldID fid = GetStaticFieldID(env, cls, fieldName, fieldSig);

        if (signature) {
            // For object types with custom signatures, use the jobject traits and cast
            return static_cast<T>(JNITypeTraits<jobject>::GetStaticField(env, cls, fid));
        } else {
            return JNITypeTraits<T>::GetStaticField(env, cls, fid);
        }
    }
} // namespace jni