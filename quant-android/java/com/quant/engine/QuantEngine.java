package com.quant.engine;

/**
 * QuantEngine — Java facade for the native C++ trading engine.
 *
 * Usage:
 *   QuantEngine engine = new QuantEngine();
 *   engine.open(getFilesDir().getAbsolutePath() + "/db");
 *   double balance = engine.walletBalance();
 *   engine.close();
 *
 * The native library must be loaded before use:
 *   System.loadLibrary("quant-jni");
 */
public class QuantEngine {

    static {
        System.loadLibrary("quant-jni");
    }

    // ---- Lifecycle ----

    public void open(String dbDir) { nativeOpen(dbDir); }
    public void close()            { nativeClose(); }

    private native void nativeOpen(String dbDir);
    private native void nativeClose();

    // ---- Wallet ----

    public native double walletBalance();
    public native double walletTotal();
    public native void   walletDeposit(double amount);
    public native void   walletWithdraw(double amount);

    // ---- Trades ----

    public native int tradeCount();

    // ---- Entry Points ----

    public native int entryCount();

    // ---- Chains ----

    public native int chainCount();
    public native int chainActivate(int chainId);
    public native int chainDeactivate(int chainId);

    // ---- Pure math ----

    public native double overhead(double price, double qty,
                                  double feeSpread, double feeHedging,
                                  double deltaTime, int symbolCount,
                                  double availableFunds,
                                  double coefficientK, int futureTradeCount);

    public native double effectiveOverhead(double overhead,
                                           double surplusRate,
                                           double feeSpread,
                                           double feeHedging,
                                           double deltaTime);
}
