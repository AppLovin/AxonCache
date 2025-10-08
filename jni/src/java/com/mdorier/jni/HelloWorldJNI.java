// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//
// Test with:
// ./build.sh -j && (cd build/jni ; java -jar HelloWorldJNI.jar)
//
package com.mdorier.jni;

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;

public class HelloWorldJNI
{
    // This is essential since its used on the C++ side for cache permanence across calls.  The C++ side
    // will use this pointer to store the cache object.
    volatile       long          objPtr           = 0;
    private static AtomicBoolean loaded           = new AtomicBoolean( false );
    private static AtomicBoolean cleanedInitiated = new AtomicBoolean( false );

    // private static void loadLib(File lib)
    private static void loadLib(String lib)
    {
        synchronized ( loaded )
        {
            if ( !loaded.get() )
            {
                // System.out.println( "Loading lib: " + lib.getAbsolutePath() );
                // System.load( lib.getAbsolutePath() );
                // System.load( lib );
                System.out.println( "Loading lib: " + lib );
                System.loadLibrary( lib );
                loaded.set( true );
            }
        }
    }

    // public HelloWorldJNI(File lib)
    public HelloWorldJNI()
    {
        // loadLib( lib );
        loadLib( "HelloWorldJNI-cpp" );
    }

    //static {
    //   System.loadLibrary("HelloWorldJNI-cpp");
    //}

    public static void main(String[] args) {
        // new HelloWorldJNI().sayHello();
        var jni = new HelloWorldJNI();
        jni.createHandle();
        // jni.sayHello();
        // var handle = new HelloWorldJNI().sayHello();
    }

    private native void sayHello();

    /*
     Real ALCache wrapper methods
     */
    public native void createHandle();
}
