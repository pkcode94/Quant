// ============================================================
// quant_jni.cpp — JNI bridge between Java and the C engine API
//
// Each native method in QuantEngine.java maps to a function here
// that calls the corresponding qe_*() function from quant_engine.h.
// ============================================================

#include <jni.h>
#include "quant_engine.h"

#include <string>
#include <cstring>

// Helper: jstring → std::string
static std::string jstr(JNIEnv* env, jstring s)
{
    if (!s) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string r(c);
    env->ReleaseStringUTFChars(s, c);
    return r;
}

// Global engine handle (one per app lifecycle)
static QEngine* g_engine = nullptr;

extern "C" {

// ---- Lifecycle ----

JNIEXPORT void JNICALL
Java_com_quant_engine_QuantEngine_nativeOpen(JNIEnv* env, jobject, jstring dbDir)
{
    if (g_engine) qe_close(g_engine);
    g_engine = qe_open(jstr(env, dbDir).c_str());
}

JNIEXPORT void JNICALL
Java_com_quant_engine_QuantEngine_nativeClose(JNIEnv*, jobject)
{
    if (g_engine) { qe_close(g_engine); g_engine = nullptr; }
}

// ---- Wallet ----

JNIEXPORT jdouble JNICALL
Java_com_quant_engine_QuantEngine_walletBalance(JNIEnv*, jobject)
{
    if (!g_engine) return 0.0;
    return qe_wallet_info(g_engine).balance;
}

JNIEXPORT jdouble JNICALL
Java_com_quant_engine_QuantEngine_walletTotal(JNIEnv*, jobject)
{
    if (!g_engine) return 0.0;
    return qe_wallet_info(g_engine).total;
}

JNIEXPORT void JNICALL
Java_com_quant_engine_QuantEngine_walletDeposit(JNIEnv*, jobject, jdouble amount)
{
    if (g_engine) qe_wallet_deposit(g_engine, amount);
}

JNIEXPORT void JNICALL
Java_com_quant_engine_QuantEngine_walletWithdraw(JNIEnv*, jobject, jdouble amount)
{
    if (g_engine) qe_wallet_withdraw(g_engine, amount);
}

// ---- Trades ----

JNIEXPORT jint JNICALL
Java_com_quant_engine_QuantEngine_tradeCount(JNIEnv*, jobject)
{
    if (!g_engine) return 0;
    return qe_trade_count(g_engine);
}

// ---- Entry Points ----

JNIEXPORT jint JNICALL
Java_com_quant_engine_QuantEngine_entryCount(JNIEnv*, jobject)
{
    if (!g_engine) return 0;
    return qe_entry_count(g_engine);
}

// ---- Chains ----

JNIEXPORT jint JNICALL
Java_com_quant_engine_QuantEngine_chainCount(JNIEnv*, jobject)
{
    if (!g_engine) return 0;
    return qe_chain_count(g_engine);
}

JNIEXPORT jint JNICALL
Java_com_quant_engine_QuantEngine_chainActivate(JNIEnv*, jobject, jint chainId)
{
    if (!g_engine) return 0;
    return qe_chain_activate(g_engine, chainId);
}

JNIEXPORT jint JNICALL
Java_com_quant_engine_QuantEngine_chainDeactivate(JNIEnv*, jobject, jint chainId)
{
    if (!g_engine) return 0;
    return qe_chain_deactivate(g_engine, chainId);
}

// ---- Pure math (no engine handle needed) ----

JNIEXPORT jdouble JNICALL
Java_com_quant_engine_QuantEngine_overhead(JNIEnv*, jobject,
    jdouble price, jdouble qty, jdouble feeSpread,
    jdouble feeHedging, jdouble deltaTime,
    jint symbolCount, jdouble availableFunds,
    jdouble coefficientK, jint futureTradeCount)
{
    return qe_overhead(price, qty, feeSpread, feeHedging, deltaTime,
                       symbolCount, availableFunds, coefficientK,
                       futureTradeCount);
}

JNIEXPORT jdouble JNICALL
Java_com_quant_engine_QuantEngine_effectiveOverhead(JNIEnv*, jobject,
    jdouble overhead, jdouble surplusRate,
    jdouble feeSpread, jdouble feeHedging, jdouble deltaTime)
{
    return qe_effective_overhead(overhead, surplusRate, feeSpread,
                                feeHedging, deltaTime);
}

} // extern "C"
