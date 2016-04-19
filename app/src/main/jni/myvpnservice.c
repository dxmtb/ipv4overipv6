#include <jni.h>
#include "backend.h"
#include "utils.h"

static void *data = NULL;

JNIEXPORT jboolean JNICALL
Java_com_troublemaker_ipv4overipv6_MyVpnService_initBackend(JNIEnv *env, jobject instance) {
    data = init();
    if (!data)
        return JNI_FALSE;
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_troublemaker_ipv4overipv6_MyVpnService_startBackend(JNIEnv *env, jobject instance) {
    if (!data)
        LOGE("Must call init first");
    start(data);
}