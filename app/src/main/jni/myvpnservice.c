#ifdef __ANDROID__
#include <jni.h>
#include "backend.h"
#include "utils.h"

static struct FileDescriptors fds;

JNIEXPORT jstring JNICALL
Java_com_troublemaker_ipv4overipv6_MyVpnService_initBackend(JNIEnv *env, jobject instance) {
    memset(&fds, -1, sizeof(fds));
    char *str = init(&fds);
    return (*env)->NewStringUTF(env, str);
}

JNIEXPORT void JNICALL
Java_com_troublemaker_ipv4overipv6_MyVpnService_startBackend(JNIEnv *env, jobject instance,
                                                             jint tunFd) {
    if (fds.server_fd == -1) {
        LOGE("server_fd not initialized");
        return;
    }
    fds.tun_fd = tunFd;
    start(&fds);
}

#endif
