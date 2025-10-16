package com.applovin.axoncache;

import java.io.*;

public class NativeLibraryLoader {
    private static boolean loaded = false;
    
    public static synchronized void load() {
        if (loaded) return;
        
        try {
            String platform = detectPlatform();
            String suffix = platform.startsWith("osx") ? ".dylib" : ".so";
            
            File tempDir = new File(System.getProperty("java.io.tmpdir"), "axoncache-" + System.currentTimeMillis());
            tempDir.mkdirs();
            tempDir.deleteOnExit();
            
            System.load(extract(platform, "libaxoncache_jni" + suffix, tempDir).getAbsolutePath());
            
            loaded = true;
        } catch (Exception e) {
            throw new RuntimeException("Failed to load native library", e);
        }
    }
    
    private static File extract(String platform, String libName, File dir) throws IOException {
        InputStream is = NativeLibraryLoader.class.getResourceAsStream("/natives/" + platform + "/" + libName);
        if (is == null) throw new RuntimeException("Native library not found: " + libName);
        
        File file = new File(dir, libName);
        file.deleteOnExit();
        
        try (FileOutputStream os = new FileOutputStream(file)) {
            byte[] buf = new byte[8192];
            int len;
            while ((len = is.read(buf)) != -1) os.write(buf, 0, len);
        }
        
        return file;
    }
    
    private static String detectPlatform() {
        String os = System.getProperty("os.name").toLowerCase();
        String arch = System.getProperty("os.arch").toLowerCase();
        
        String a = (arch.contains("aarch64") || arch.contains("arm64")) ? "aarch64" :
                   arch.contains("amd64") || arch.contains("x86_64") ? "x86_64" :
                   null;
        if (a == null) throw new RuntimeException("Unsupported architecture: " + arch);
        
        if (os.contains("mac") || os.contains("darwin")) {
            if (!a.equals("aarch64")) throw new RuntimeException("Only ARM64 Macs supported");
            return "osx-aarch64";
        }
        if (os.contains("linux")) return "linux-" + a;
        
        throw new RuntimeException("Unsupported OS: " + os);
    }
}

