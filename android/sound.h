/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Simon Tomlinson, Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

#include <oboe/Oboe.h>
#include "soundbase.h"
#include "global.h"
#include <QDebug>
#include <android/log.h>
#include "ring_buffer.h"
#include <mutex>
#include "jni.h"

/* Classes ********************************************************************/
class CSound : public CSoundBase, public oboe::AudioStreamCallback
{
    Q_OBJECT

public:
    static const uint8_t RING_FACTOR;
    CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const QString& strMIDISetup,
             const bool     ,
             const QString& );
    virtual ~CSound() {}

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();
    virtual void setBuiltinInput(bool builtinmic);

    // Call backs for Oboe
    virtual oboe::DataCallbackResult onAudioReady ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );
    virtual void onErrorAfterClose ( oboe::AudioStream* oboeStream, oboe::Result result );
    virtual void onErrorBeforeClose ( oboe::AudioStream* oboeStream, oboe::Result result );

    struct Stats 
    {
        Stats() { reset(); }
        void reset();
        void log() const;
        std::size_t frames_in;
        std::size_t frames_out;
        std::size_t frames_filled_out;
        std::size_t in_callback_calls;
        std::size_t out_callback_calls;
        std::size_t ring_overrun;
    };

/*NGOCDH */
#define CREATE_JNI_METHOD(methodID, stringName, params)          methodID = resolveMethod (env, stringName, params);
#define CREATE_JNI_STATICMETHOD(methodID, stringName, params)    methodID = resolveStaticMethod (env, stringName, params);
#define CREATE_JNI_FIELD(fieldID, stringName, signature)         fieldID  = resolveField (env, stringName, signature);
#define CREATE_JNI_STATICFIELD(fieldID, stringName, signature)   fieldID  = resolveStaticField (env, stringName, signature);
#define CREATE_JNI_CALLBACK(callbackName, stringName, signature) callbacks.add ({stringName, signature, (void*) callbackName});
#define DECLARE_JNI_METHOD(methodID, stringName, params)         jmethodID methodID;
#define DECLARE_JNI_FIELD(fieldID, stringName, signature)        jfieldID  fieldID;
#define DECLARE_JNI_CALLBACK(fieldID, stringName, signature)

#define DECLARE_JNI_CLASS_WITH_BYTECODE(CppClassName, javaPath, minSDK, byteCodeData, byteCodeSize) \
    class CppClassName ## _Class   : public JNIClassBase \
    { \
    public: \
        CppClassName ## _Class() : JNIClassBase (javaPath, minSDK, byteCodeData, byteCodeSize) {} \
    \
        void initialiseFields (JNIEnv* env) \
        { \
            Array<JNINativeMethod> callbacks; \
            JNI_CLASS_MEMBERS (CREATE_JNI_METHOD, CREATE_JNI_STATICMETHOD, CREATE_JNI_FIELD, CREATE_JNI_STATICFIELD, CREATE_JNI_CALLBACK); \
            resolveCallbacks (env, callbacks); \
        } \
    \
        JNI_CLASS_MEMBERS (DECLARE_JNI_METHOD, DECLARE_JNI_METHOD, DECLARE_JNI_FIELD, DECLARE_JNI_FIELD, DECLARE_JNI_CALLBACK) \
    }; \
    static CppClassName ## _Class CppClassName;

//==============================================================================
#define DECLARE_JNI_CLASS_WITH_MIN_SDK(CppClassName, javaPath, minSDK) \
    DECLARE_JNI_CLASS_WITH_BYTECODE (CppClassName, javaPath, minSDK, nullptr, 0)

//==============================================================================
#define DECLARE_JNI_CLASS(CppClassName, javaPath) \
    DECLARE_JNI_CLASS_WITH_MIN_SDK (CppClassName, javaPath, 16)

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
 METHOD (getAssets,                            "getAssets",                       "()Landroid/content/res/AssetManager;") \
 METHOD (getSystemService,                     "getSystemService",                "(Ljava/lang/String;)Ljava/lang/Object;") \
 METHOD (getPackageManager,                    "getPackageManager",               "()Landroid/content/pm/PackageManager;") \
 METHOD (getPackageName,                       "getPackageName",                  "()Ljava/lang/String;") \
 METHOD (getResources,                         "getResources",                    "()Landroid/content/res/Resources;") \
 METHOD (bindService,                          "bindService",                     "(Landroid/content/Intent;Landroid/content/ServiceConnection;I)Z") \
 METHOD (unbindService,                        "unbindService",                   "(Landroid/content/ServiceConnection;)V") \
 METHOD (startActivity,                        "startActivity",                   "(Landroid/content/Intent;)V") \
 METHOD (getContentResolver,                   "getContentResolver",              "()Landroid/content/ContentResolver;") \
 METHOD (getApplicationContext,                "getApplicationContext",           "()Landroid/content/Context;") \
 METHOD (getApplicationInfo,                   "getApplicationInfo",              "()Landroid/content/pm/ApplicationInfo;") \
 METHOD (checkCallingOrSelfPermission,         "checkCallingOrSelfPermission",    "(Ljava/lang/String;)I") \
 METHOD (getCacheDir,                          "getCacheDir",                     "()Ljava/io/File;")

    DECLARE_JNI_CLASS (AndroidContext, "android/content/Context");
    LocalRef<jobject> getAppContext() noexcept
    {
        auto* env = getEnv();
        auto context = androidApkContext;

        // You did not call Thread::initialiseJUCE which must be called at least once in your apk
        // before using any JUCE APIs. The Projucer will automatically generate java code
        // which will invoke Thread::initialiseJUCE for you.
        //jassert (env != nullptr && context != nullptr);

        if (context == nullptr)
            return LocalRef<jobject>();

        if (env->IsInstanceOf (context, AndroidApplication) != 0)
            return LocalRef<jobject> (env->NewLocalRef (context));

        LocalRef<jobject> applicationContext (env->CallObjectMethod (context, AndroidContext.getApplicationContext));

        if (applicationContext == nullptr)
            return LocalRef<jobject> (env->NewLocalRef (context));

        return applicationContext;
    };



    QString getAvailableDevices()
    {

        auto* env = getEnv();

        jclass audioManagerClass = env->FindClass ("android/media/AudioManager");

        // We should be really entering here only if API supports it.
        //jassert (audioManagerClass != nullptr);

        if (audioManagerClass == nullptr)
            return;

        auto audioManager = LocalRef<jobject> (env->CallObjectMethod (getAppContext().get(),
                                                                      AndroidContext.getSystemService,
                                                                      javaString ("audio").get()));

        static jmethodID getDevicesMethod = env->GetMethodID (audioManagerClass, "getDevices",
                                                              "(I)[Landroid/media/AudioDeviceInfo;");

        static constexpr int allDevices = 3;
        auto devices = LocalRef<jobjectArray> ((jobjectArray) env->CallObjectMethod (audioManager,
                                                                                     getDevicesMethod,
                                                                                     allDevices));

        const int numDevices = env->GetArrayLength (devices.get());


        QString strAvailableDevices = "";
        for (int i = 0; i < numDevices; ++i)
        {
            auto device = LocalRef<jobject> ((jobject) env->GetObjectArrayElement (devices.get(), i));
            strAvailableDevices = strAvailableDevices + getDeviceInfo (device, env);
        }


        
        return strAvailableDevices;
    };
    
/*end of NGOCDH */    

protected:
    CVector<int16_t>  vecsTmpInputAudioSndCrdStereo;
    RingBuffer<float> mOutBuffer;
    int               iOboeBufferSizeMono;
    int               iOboeBufferSizeStereo;

private:
    void setupCommonStreamParams ( oboe::AudioStreamBuilder* builder );
    void printStreamDetails ( oboe::ManagedStream& stream );
    void openStreams();
    void closeStreams();
    void warnIfNotLowLatency ( oboe::ManagedStream& stream, QString streamName );
    void closeStream ( oboe::ManagedStream& stream );

    oboe::DataCallbackResult onAudioInput ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );
    oboe::DataCallbackResult onAudioOutput ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );

    void addOutputData(int channel_count);

    oboe::ManagedStream  mRecordingStream;
    oboe::ManagedStream  mPlayStream;

    // used to reach a state where the input buffer is
    // empty and the garbage in the first 500ms or so is discarded
    static constexpr int32_t kNumCallbacksToDrain = 10;
    int32_t mCountCallbacksToDrain = kNumCallbacksToDrain;
    Stats   mStats;
   
    JavaVM* androidJNIJavaVM = nullptr;
    jobject androidApkContext = nullptr;

    //==============================================================================
    JNIEnv* getEnv() noexcept
    {
        if (androidJNIJavaVM != nullptr)
        {
            JNIEnv* env;
            androidJNIJavaVM->AttachCurrentThread (&env, nullptr);

            return env;
        }

        // You did not call Thread::initialiseJUCE which must be called at least once in your apk
        // before using any JUCE APIs. The Projucer will automatically generate java code
        // which will invoke Thread::initialiseJUCE for you.
        //jassertfalse;
        return nullptr;
    };

    //void JNICALL javainitialiseJamulus (JNIEnv* env, jobject /*jclass*/, jobject context)
    //{
    //    QThread::initialiseJamulusJava (env, context);
    //}
        // step 1
    // this method is called automatically by Java VM
    // after the .so file is loaded
    JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
    {
        // Huh? JNI_OnLoad was called two times!
        //jassert (androidJNIJavaVM == nullptr);

        androidJNIJavaVM = vm;

        auto* env = getEnv();
/*
        // register the initialisation function
        auto juceJavaClass = env->FindClass("com/rmsl/juce/Java");

        if (juceJavaClass != nullptr)
        {
            JNINativeMethod method {"initialiseJamulusJava", "(Landroid/content/Context;)V",
                                    reinterpret_cast<void*> (javainitialiseJamulus)};

            auto status = env->RegisterNatives (juceJavaClass, &method, 1);
            //jassert (status == 0);
        }
        else
        {
            // com.rmsl.juce.Java class not found. Apparently this project is a library
            // or was not generated by the Projucer. That's ok, the user will have to
            // call Thread::initialiseJUCE manually
            env->ExceptionClear();
        }
*/
        JNIClassBase::initialiseAllClasses (env);

        return JNI_VERSION_1_6;
    };

    /*void QThread::initialiseJamulusJava (void* jniEnv, void* context)
    {
        static CriticalSection cs;
        ScopedLock lock (cs);

        // jniEnv and context should not be null!
        //jassert (jniEnv != nullptr && context != nullptr);

        auto* env = static_cast<JNIEnv*> (jniEnv);

        if (androidJNIJavaVM == nullptr)
        {
            JavaVM* javaVM = nullptr;

            auto status = env->GetJavaVM (&javaVM);
            //jassert (status == 0 && javaVM != nullptr);

            androidJNIJavaVM = javaVM;
        }

    }*/
    
    inline LocalRef<jstring> javaString (const QString& s)
    {
        return LocalRef<jstring> (getEnv()->NewStringUTF (s.toUtf8()));//(s.toUTF8()));
    };

    QString getDeviceInfo (const LocalRef<jobject>& device, JNIEnv* env)
    {
        auto deviceClass = LocalRef<jclass> ((jclass) env->FindClass ("android/media/AudioDeviceInfo"));

        jmethodID getProductNameMethod = env->GetMethodID (deviceClass, "getProductName",
                                                           "()Ljava/lang/CharSequence;");

        jmethodID getTypeMethod          = env->GetMethodID (deviceClass, "getType", "()I");
        jmethodID getIdMethod            = env->GetMethodID (deviceClass, "getId", "()I");
        jmethodID getSampleRatesMethod   = env->GetMethodID (deviceClass, "getSampleRates", "()[I");
        jmethodID getChannelCountsMethod = env->GetMethodID (deviceClass, "getChannelCounts", "()[I");
        jmethodID isSourceMethod         = env->GetMethodID (deviceClass, "isSource", "()Z");

        auto deviceTypeString = deviceTypeToString (env->CallIntMethod (device, getTypeMethod));

        if (deviceTypeString.isEmpty()) // unknown device
            return "";

        auto name = QString ((jstring) env->CallObjectMethod (device, getProductNameMethod)) + " " + deviceTypeString;
        auto id = env->CallIntMethod (device, getIdMethod);
        auto isInput  = env->CallBooleanMethod (device, isSourceMethod);
        //auto& devices = isInput ? inputDevices : outputDevices;

        return (isInput ? QString("In: "):QString("Out: ")) + name + " - " + id + "; ";
    };

    static QString deviceTypeToString (int type)
    {
        switch (type)
        {
            case 0:   return {};
            case 1:   return "built-in earphone speaker";
            case 2:   return "built-in speaker";
            case 3:   return "wired headset";
            case 4:   return "wired headphones";
            case 5:   return "line analog";
            case 6:   return "line digital";
            case 7:   return "Bluetooth device typically used for telephony";
            case 8:   return "Bluetooth device supporting the A2DP profile";
            case 9:   return "HDMI";
            case 10:  return "HDMI audio return channel";
            case 11:  return "USB device";
            case 12:  return "USB accessory";
            case 13:  return "DOCK";
            case 14:  return "FM";
            case 15:  return "built-in microphone";
            case 16:  return "FM tuner";
            case 17:  return "TV tuner";
            case 18:  return "telephony";
            case 19:  return "auxiliary line-level connectors";
            case 20:  return "IP";
            case 21:  return "BUS";
            case 22:  return "USB headset";
            case 23:  return "hearing aid";
            case 24:  return "built-in speaker safe";
            case 25:  return {};
            default:  return {};//jassertfalse; return {}; // type not supported yet, needs to be added!
        }
    };
    
};

class JNIClassBase
{
public:
    explicit JNIClassBase (const char* classPath, int minSDK, const void* byteCode, size_t byteCodeSize);
    virtual ~JNIClassBase();

    inline operator jclass() const noexcept  { return classRef; }

    static void initialiseAllClasses (JNIEnv*);
    static void releaseAllClasses (JNIEnv*);

    inline const char* getClassPath() const noexcept { return classPath; }

protected:
    virtual void initialiseFields (JNIEnv*) = 0;

    jmethodID resolveMethod (JNIEnv*, const char* methodName, const char* params);
    jmethodID resolveStaticMethod (JNIEnv*, const char* methodName, const char* params);
    jfieldID resolveField (JNIEnv*, const char* fieldName, const char* signature);
    jfieldID resolveStaticField (JNIEnv*, const char* fieldName, const char* signature);
    void resolveCallbacks (JNIEnv*, const Array<JNINativeMethod>&);

private:
    friend struct SystemJavaClassComparator;

    const char* const classPath;
    const void* byteCode;
    size_t byteCodeSize;

    int minSDK;
    jclass classRef = nullptr;

    static Array<JNIClassBase*>& getClasses();
    void initialise (JNIEnv*);
    void release (JNIEnv*);
    void tryLoadingClassWithClassLoader (JNIEnv* env, jobject classLoader);

};

    
template <typename JavaType>
class LocalRef
{
public:
    explicit inline LocalRef() noexcept                 : obj (nullptr) {}
    explicit inline LocalRef (JavaType o) noexcept      : obj (o) {}
    inline LocalRef (const LocalRef& other) noexcept    : obj (retain (other.obj)) {}
    inline LocalRef (LocalRef&& other) noexcept         : obj (nullptr) { std::swap (obj, other.obj); }
    ~LocalRef()                                         { clear(); }

    void clear()
    {
        if (obj != nullptr)
        {
            getEnv()->DeleteLocalRef (obj);
            obj = nullptr;
        }
    }

    LocalRef& operator= (const LocalRef& other)
    {
        JavaType newObj = retain (other.obj);
        clear();
        obj = newObj;
        return *this;
    }

    LocalRef& operator= (LocalRef&& other)
    {
        clear();
        std::swap (other.obj, obj);
        return *this;
    }

    inline operator JavaType() const noexcept   { return obj; }
    inline JavaType get() const noexcept        { return obj; }

private:
    JavaType obj;

    static JavaType retain (JavaType obj)
    {
        return obj == nullptr ? nullptr : (JavaType) getEnv()->NewLocalRef (obj);
    }
};
