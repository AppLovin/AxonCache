#include "com_mdorier_jni_HelloWorldJNI.h"
#include <iostream>

JNIEXPORT void JNICALL Java_com_mdorier_jni_HelloWorldJNI_sayHello
  (JNIEnv * /* env */, jobject /* obj */)
{
    std::cout << "Hello from C++ !!!!!" << "\n";
}
