# Proguard rules for Quant
# Keep JNI native method names
-keepclasseswithmembernames class * {
    native <methods>;
}
-keep class com.quant.ui.** { *; }
-keep class com.quant.engine.** { *; }
