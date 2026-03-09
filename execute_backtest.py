"""
Backtest execution script - executes trades via MCP server
"""
import json
import subprocess
import time

# Load signals
with open('D:/q/trade_signals.json', 'r') as f:
    signals = json.load(f)

print(f"Loaded {len(signals)} trading signals")

position_open = False
position_qty = 0
executed_trades = []

for i, signal in enumerate(signals):
    print(f"\n[{i+1}/{len(signals)}] Signal: {signal['Signal']} at ${signal['Price']}")
    
    if signal['Signal'] == 'BUY' and not position_open:
        # Buy signal - open position
        qty = 50  # Fixed quantity per trade
        print(f"  -> Executing BUY {qty} @ ${signal['Price']}")
        
        # Note: We can't use the actual historical prices via MCP server
        # This would need to be done with a proper backtesting framework
        # that tracks the trades internally
        
        position_open = True
        position_qty = qty
        executed_trades.append({
            'index': i,
            'signal': 'BUY',
            'price': signal['Price'],
            'qty': qty
        })
        
    elif signal['Signal'] == 'SELL' and position_open:
        # Sell signal - close position
        print(f"  -> Executing SELL {position_qty} @ ${signal['Price']}")
        
        # Calculate P&L
        entry_trade = executed_trades[-1]
        pnl = (signal['Price'] - entry_trade['price']) * position_qty
        pnl_pct = ((signal['Price'] / entry_trade['price']) - 1) * 100
        
        print(f"  -> P&L: ${pnl:.2f} ({pnl_pct:+.2f}%)")
        
        position_open= False
        position_qty = 0
        executed_trades.append({
            'index': i,
            'signal': 'SELL',
            'price': signal['Price'],
            'qty': position_qty,
            'pnl': pnl,
            'pnl_pct': pnl_pct
        })

# Summary
print("\n" + "="*60)
print("BACKTEST SUMMARY")
print("="*60)

total_pnl = 0
winning_trades = 0
losing_trades = 0

for trade in executed_trades:
    if trade['signal'] == 'SELL':
        total_pnl += trade['pnl']
        if trade['pnl'] > 0:
            winning_trades += 1
        else:
            losing_trades += 1

total_trades = winning_trades + losing_trades

if total_trades > 0:
    print(f"Total Trades: {total_trades}")
    print(f"Winning Trades: {winning_trades} ({winning_trades/total_trades*100:.1f}%)")
    print(f"Losing Trades: {losing_trades} ({losing_trades/total_trades*100:.1f}%)")
    print(f"Total P&L: ${total_pnl:.2f}")
    print(f"Average P&L per trade: ${total_pnl/total_trades:.2f}")
    print(f"Starting Capital: $100,000")
    print(f"Ending Value: ${100000 + total_pnl:.2f}")
    print(f"Return: {(total_pnl/100000)*100:+.2f}%")

# Save results
with open('D:/q/backtest_results.json', 'w') as f:
    json.dump({
        'executed_trades': executed_trades,
        'summary': {
            'total_trades': total_trades,
            'winning_trades': winning_trades,
            'losing_trades': losing_trades,
            'total_pnl': total_pnl,
            'starting_capital': 100000,
            'ending_value': 100000 + total_pnl,
            'return_pct': (total_pnl/100000)*100
        }
    }, f, indent=2)

print("\nResults saved to backtest_results.json")
