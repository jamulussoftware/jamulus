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
    QString getAvailableDevices()
    {
        auto sampleRates = OboeAudioIODevice::getDefaultSampleRates();

        inputDevices .add ({ "System Default (Input)",  oboe::kUnspecified, sampleRates, 1 });
        outputDevices.add ({ "System Default (Output)", oboe::kUnspecified, sampleRates, 2 });

        if (! supportsDevicesInfo())
            return;

        auto* env = getEnv();

        jclass audioManagerClass = env->FindClass ("android/media/AudioManager");

        // We should be really entering here only if API supports it.
        jassert (audioManagerClass != nullptr);

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

        for (int i = 0; i < numDevices; ++i)
        {
            auto device = LocalRef<jobject> ((jobject) env->GetObjectArrayElement (devices.get(), i));
            addDevice (device, env);
        }

        QString strAvailableDevices = "Inputs: ";
        //JUCE_OBOE_LOG ("-----InputDevices:");

        for (auto& device : inputDevices)
        {
            strAvailableDevices = strAvailableDevices + "name = " + device.name;
            strAvailableDevices = strAvailableDevices + ", id = " + String (device.id) + "; ";
        }

        //JUCE_OBOE_LOG ("-----OutputDevices:");
        strAvailableDevices = strAvailableDevices + " | Outputs: ";
        for (auto& device : outputDevices)
        {
            strAvailableDevices = strAvailableDevices + "name = " + device.name;
            strAvailableDevices = strAvailableDevices + ", id = " + String (device.id) + "; ";
        }
    }
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
        jassertfalse;
        return nullptr;
    }

    void JNICALL javainitialiseJamulus (JNIEnv* env, jobject /*jclass*/, jobject context)
    {
        Thread::initialiseJamulusJava (env, context);
    }
        // step 1
    // this method is called automatically by Java VM
    // after the .so file is loaded
    JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
    {
        // Huh? JNI_OnLoad was called two times!
        jassert (androidJNIJavaVM == nullptr);

        androidJNIJavaVM = vm;

        auto* env = getEnv();

        // register the initialisation function
        auto juceJavaClass = env->FindClass("com/rmsl/juce/Java");

        if (juceJavaClass != nullptr)
        {
            JNINativeMethod method {"initialiseJamulusJava", "(Landroid/content/Context;)V",
                                    reinterpret_cast<void*> (javainitialiseJamulus)};

            auto status = env->RegisterNatives (juceJavaClass, &method, 1);
            jassert (status == 0);
        }
        else
        {
            // com.rmsl.juce.Java class not found. Apparently this project is a library
            // or was not generated by the Projucer. That's ok, the user will have to
            // call Thread::initialiseJUCE manually
            env->ExceptionClear();
        }

        JNIClassBase::initialiseAllClasses (env);

        return JNI_VERSION_1_6;
    }

    void Thread::initialiseJamulusJava (void* jniEnv, void* context)
    {
        static CriticalSection cs;
        ScopedLock lock (cs);

        // jniEnv and context should not be null!
        jassert (jniEnv != nullptr && context != nullptr);

        auto* env = static_cast<JNIEnv*> (jniEnv);

        if (androidJNIJavaVM == nullptr)
        {
            JavaVM* javaVM = nullptr;

            auto status = env->GetJavaVM (&javaVM);
            jassert (status == 0 && javaVM != nullptr);

            androidJNIJavaVM = javaVM;
        }

    }
    
};
