#include <jni.h>
#include <string>
#include <android/log.h>
#include<SLES/OpenSLES.h>
#include<SLES/OpenSLES_Android.h>

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"Finn ff=========>",__VA_ARGS__)
static SLObjectItf engineSL = NULL;

/**
 * 创建引擎
 * @return
 */
SLEngineItf CreateSL() {
    SLresult re;
    SLEngineItf en;
    re = slCreateEngine(&engineSL, 0, 0, 0, 0, 0);
    if (re != SL_RESULT_SUCCESS) return NULL;
    //实例化 SL_BOOLEAN_FALSE阻塞式的等待创建完毕
    re = (*engineSL)->Realize(engineSL, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS)return NULL;
    re = (*engineSL)->GetInterface(engineSL, SL_IID_ENGINE, &en);
    if (re != SL_RESULT_SUCCESS)return NULL;
    return en;
}

void PcmCall(SLAndroidSimpleBufferQueueItf bf, void *contex) {
    LOGW("PcmCall");

    static FILE *fp = NULL;
    static char *buf = NULL;
    if (!buf) {
        buf = new char[1024 * 1024];
    }
    if (!fp) {
//        fp = fopen("/sdcard/yin.pcm", "rb");
        fp = fopen("/sdcard/test.pcm", "rb");
    }
    if (!fp)return;
    //如果没有到结尾
    if (feof(fp) == 0) {
        int len = fread(buf, 1, 1024, fp);
        if (len > 0) {
            (*bf)->Enqueue(bf, buf, len);
        }
    }
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_openslesdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

    SLresult re;
    //创建引擎
    SLEngineItf engine = CreateSL();
    if (engine) {
        LOGW("引擎创建成功！");
    } else {
        LOGW("引擎创建失败！");
    }

    //创建混音器
    SLObjectItf mix = NULL;
    re = (*engine)->CreateOutputMix(engine, &mix, 0, 0, 0);
    if (re != SL_RESULT_SUCCESS) {
        LOGW("SL_RESULT_SUCCESS failed");
    }
    re = (*mix)->Realize(mix, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) {
        LOGW(" mix SL_RESULT_SUCCESS failed");
    }
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mix};
    SLDataSink audioSink = {&outputMix, 0};
    //3.配置音频信息
    //缓冲队列
    SLDataLocator_AndroidSimpleBufferQueue que = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    //音频格式
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,//声道数
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN//字节序，小端  一般经过网络流可能会改变 序列
    };
    SLDataSource ds = {&que, &pcm};

    //4.创建播放器
    SLObjectItf player = NULL;
    SLPlayItf iplayer = NULL;
    SLAndroidSimpleBufferQueueItf pcmQue = NULL;
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    //开放或者关闭接口 true开放
    const SLboolean req[] = {SL_BOOLEAN_TRUE};
    re = (*engine)->CreateAudioPlayer(engine, &player, &ds, &audioSink,
                                      sizeof(ids) / sizeof(SLInterfaceID), ids, req);
    if (re != SL_RESULT_SUCCESS) {
        LOGW(" CreateAudioPlayer SL_RESULT_SUCCESS failed");
    } else {
        LOGW(" CreateAudioPlayer SL_RESULT_SUCCESS success");
    }

    //实例化
    (*player)->Realize(player, SL_BOOLEAN_FALSE);
    //获取player接口
    re = (*player)->GetInterface(player, SL_IID_PLAY, &iplayer);
    if (re != SL_RESULT_SUCCESS) {
        LOGW(" GetInterface SL_IID_PLAY failed");
    } else {
        LOGW(" GetInterface SL_IID_PLAY success");
    }
    //获取队列
    re = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &pcmQue);
    if (re != SL_RESULT_SUCCESS) {
        LOGW(" GetInterface SL_IID_BUFFERQUEUE failed");
    } else {
        LOGW(" GetInterface SL_IID_BUFFERQUEUE success");
    }
    //设置回调函数，播放队列空的时候调用
    (*pcmQue)->RegisterCallback(pcmQue, PcmCall, 0);
    //设置播放状态为播放
    (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_PLAYING);
    //启动队列回调
    (*pcmQue)->Enqueue(pcmQue, "", 1);

    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
