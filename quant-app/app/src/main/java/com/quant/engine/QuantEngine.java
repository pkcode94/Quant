package com.quant.engine;

/**
 * QuantEngine — Direct Java access to the C++ trading engine.
 *
 * Use this for data-only operations (background work, notifications,
 * widgets) where you don't need the full HTML UI.
 *
 * For the GUI, use QuantUI + QuantWebView instead.
 */
public class QuantEngine {

    static {
        System.loadLibrary("quant-native");
    }

    public void open(String dbDir) { nativeOpen(dbDir); }
    public void close()            { nativeClose(); }

    private native void nativeOpen(String dbDir);
    private native void nativeClose();

    // Wallet
    public native double walletBalance();
    public native double walletTotal();
    public native void   walletDeposit(double amount);
    public native void   walletWithdraw(double amount);

    // Counts
    public native int tradeCount();
    public native int entryCount();
    public native int chainCount();

    // Chain ops
    public native int chainActivate(int chainId);
    public native int chainDeactivate(int chainId);

    // Pure math
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
