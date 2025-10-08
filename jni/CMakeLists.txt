find_package (Java REQUIRED)
find_package (JNI REQUIRED)
include (UseJava)

# Example without JNI. Simple HelloWorld class with an entry point.
add_jar(HelloWorld
        VERSION 0.0.1
        ENTRY_POINT com.mdorier.jni.HelloWorld
        SOURCES src/java/com/mdorier/jni/HelloWorld.java)

# Example with JNI. Simple HelloWorldJNI class with an entry point
# that calls various C++ functions.
add_jar(HelloWorldJNI
        VERSION 0.0.1
        ENTRY_POINT com.mdorier.jni.HelloWorldJNI
        GENERATE_NATIVE_HEADERS HelloWorldJNI-native
        SOURCES src/java/com/mdorier/jni/HelloWorldJNI.java)

add_library(
  HelloWorldJNI-cpp SHARED
  ../AxonCache.cpp src/cpp/HelloWorldJNI-cpp.cpp)

target_include_directories (HelloWorldJNI-cpp PRIVATE ../include ${JNI_INCLUDE_DIRS})
target_link_libraries (HelloWorldJNI-cpp PRIVATE HelloWorldJNI-native ${JNI_LIBRARIES})
