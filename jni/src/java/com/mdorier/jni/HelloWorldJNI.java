package com.mdorier.jni;

public class HelloWorldJNI {

    static {
        System.loadLibrary("HelloWorldJNI-cpp");
    }

    public static void main(String[] args) {
        new HelloWorldJNI().sayHello();
    }

    private native void sayHello();
}
