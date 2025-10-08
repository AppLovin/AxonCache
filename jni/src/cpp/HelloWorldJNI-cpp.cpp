#include "com_mdorier_jni_HelloWorldJNI.h"
#include <iostream>

JNIEXPORT void JNICALL Java_com_mdorier_jni_HelloWorldJNI_sayHello
  (JNIEnv *, jobject)
{
    std::cout << "Hello from C++ !!" << std::endl;
}
