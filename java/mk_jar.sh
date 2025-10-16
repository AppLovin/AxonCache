#!/bin/bash
set -e

echo "Running java post steps"
rm -rf build/java_sandbox
mkdir build/java_sandbox

# Copy whole java folder, required by maven
cp -rf java build/java_sandbox

cd build/java_sandbox

# Copy the shared libraries. 
# HACK ALERT
# Since this script is running on a single platform
# so that we don't need to detect the current OS, and copy the same shared library in all folders
for arch in osx-aarch64 linux-x86_64 linux-aarch64
do
    mkdir -p java/src/main/resources/natives/$arch
    cp ../java/libaxoncache_jni.* java/src/main/resources/natives/$arch/
done

# now run maven
cd java
mvn clean package
cd -

cd -

cp build/java_sandbox/java/target/axoncache-java-1.0.0.jar build/java/axoncache.jar

