<?php

// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2021-2025 Aaron Fiore (Founder, Evergreen Crypto LLC)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

/**
 * @file
 * @author Aaron Fiore (Founder, Evergreen Crypto LLC)
 * @since docker-finance 1.0.0
 */

//! @since docker-finance 1.0.0

namespace dfi\exchanges\internal\exchanges\gemini
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;
    use DateTime;  // PHP

    /**
     * @brief Common implementaion
     * @since docker-finance 1.0.0
     */
    abstract class Impl extends \dfi\exchanges\internal\Impl
    {
        private \ccxt\gemini $api;

        private int $given_timestampms;  //!< Requested date
        private int $max_timestampms;  //!< Date maximum

        public const PAGINATION_LIMIT = 50;  //!< Pagination limit for all paginated

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $this->api = new \ccxt\gemini([
                'apiKey' => $this->get_env()->get_env('API_KEY'),
                'secret' =>  $this->get_env()->get_env('API_SECRET'),
                //'verbose' => true, // uncomment for debugging
                'enableRateLimit' => true
            ]);

            $given_year = new DateTime($this->get_env()->get_env('API_FETCH_YEAR') . '-01-01');
            $this->given_timestampms = $given_year->format('U') * 1000;

            $year = intval($this->get_env()->get_env('API_FETCH_YEAR')) + 1;
            $max_year = new DateTime($year . '-01-01');
            $this->max_timestampms = $max_year->format('U') * 1000;
        }

        protected function get_api(): \ccxt\gemini
        {
            return $this->api;
        }

        protected function get_given_timestampms(): int
        {
            return $this->given_timestampms;
        }

        protected function get_max_timestampms(): int
        {
            return $this->max_timestampms;
        }
    }

    /**
     * @brief Common implementation
     * @since docker-finance 1.0.0
     */
    abstract class Gemini extends Impl
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Implements underlying API request
         * @param string $path URI path
         * @param string $api API scope
         * @param string $method HTTP method
         * @param array<string, int|string> $params Optional parameters
         * @return mixed Raw requested data
         */
        protected function request(
            string $path,
            string $api = 'private',
            string $method = 'GET',
            array $params = []
        ): mixed {
            $response = $this->get_api()->request($path, $api, $method, $params, null, null, []);
            utils\CLI::print_debug($response);
            return $response;
        }
    }

    /**
     * @brief Gemini Trades
     * @since docker-finance 1.0.0
     */
    final class Trades extends Gemini
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Get Gemini's supported symbol pairs
         * @return array<string>
         */
        private function get_symbols(): array
        {
            $response = $this->request('v1/symbols', 'public');
            utils\CLI::print_debug($response);
            return $response;
        }

        /**
         * @brief Get Gemini Trades
         * @phpstan-ignore-next-line  // TODO: resolve
         */
        private function get_trades(string $symbol, string $timestamp): array
        {
            // Don't use ccxt fetch_my_trades because API consistency (in returned results) is needed
            $response = $this->request(
                'v1/mytrades',
                'private',
                'POST',
                ['symbol' => $symbol, 'timestamp' => $timestamp, 'limit_trades' => self::PAGINATION_LIMIT]
            );

            utils\CLI::print_debug($response);
            return $response;
        }

        /**
         * @brief Implements read handler
         * @return array<array<int, string>> Trading pairs metadata and attached trades, prepared for writing
         */
        public function read(): array
        {
            // Account (possible) symbols
            $acct_symbols = $this->parse_subaccount_symbols(
                $this->get_env()->get_env('API_SUBACCOUNT')
            );

            // Gemini's symbols
            $exchange_symbols = $this->get_symbols();
            if (empty($exchange_symbols)) {
                utils\CLI::print_warning("no symbols were received, skipping trades");
                return [];
            }

            $symbols = [];  // Parsed symbols

            // Populate parsed symbols with exchange symbols, if symbols are available
            if (!empty($acct_symbols)) {
                foreach ($acct_symbols as $acct_symbol) {
                    foreach ($exchange_symbols as $exchange_symbol) {
                        // TODO: preg_match to isolate `btcusd` and `btcusdt` while allowing btc* pairs
                        if (str_contains($exchange_symbol, strtolower($acct_symbol))) {
                            array_push($symbols, $exchange_symbol);
                        }
                    }
                }
            } else {
                // Use the entirety of exchange symbols
                $symbols = $exchange_symbols;
            }

            // symbol => trades
            $stack = [];

            // Get trades from symbols
            foreach ($symbols as $symbol) {
                utils\CLI::print_custom("    \e[32m├─\e[34m\e[1;3m $symbol\e[0m\n");

                $response = $this->get_trades($symbol, strval($this->get_given_timestampms()));

                // Push to stack and paginate if needed
                if (!empty($response)) {
                    $stack[$symbol] = [];

                    // Only fetch given year (if response spans multiple years, because of few distant trades)
                    foreach ($response as $trade) {
                        if ($trade['timestampms'] <= $this->get_max_timestampms()) {
                            $stack[$symbol][] = $trade;
                        }
                    }

                    // TODO: copy over to transactions and earn?
                    // This is needed because timestamp is not incremented
                    while (count($stack[$symbol]) == self::PAGINATION_LIMIT) {
                        // Get next timestampms, most recent first
                        //
                        // WARNING:
                        //
                        //   Do *NOT* +1 increment for now, duplicates are sorted later.
                        //
                        // Rationale:
                        //
                        //   All trades for a single timestampms might not be pulled because
                        //   incrementing after N might skip the remaining batch for the
                        //   previous timestampms (which may have had any number of trades).
                        //
                        // Solution TODO:
                        //
                        //   Compare not only previous timestampms but also orderid and tid.
                        //   If all 3 are the same, increment the timestampms for next batch.

                        $next = $stack[$symbol][0]['timestampms'];

                        // Only fetch given year
                        if ($next >= $this->get_max_timestampms()) {
                            break;
                        }

                        $response = $this->get_trades($symbol, $next);

                        // If previous fetch ended up to max pagination limit
                        if (empty($response)) {
                            continue;
                        }

                        // Push to trades stack
                        foreach ($response as $trade) {
                            $stack[$symbol][] = $trade;
                        }
                    }
                }
            }

            return $stack;
        }

        /**
         * @brief Implements write handler
         * @param mixed $trades Trade data to write
         * @param string $symbols Trading pair symbols
         */
        public function write(mixed $trades, string $symbols): void
        {
            $file = $this->get_env()->get_env('API_OUT_DIR') . $symbols . '-Trades.csv';
            // NOTE: array_unique needed to removes dups. WARNING: will break if removed
            $this->raw_to_csv(array_unique($trades, SORT_REGULAR), $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Trades");
            utils\CLI::print_custom("    \e[32m│\e[0m\n");

            foreach ($this->read() as $symbols => $trades) {
                $this->write($trades, $symbols);
            }

            utils\CLI::print_custom("    \e[32m│\e[0m\n");
        }
    }

    /**
     * @brief Gemini Transactions
     * @details Transfers / Rewards / non-Trades / non-Earn
     * @since docker-finance 1.0.0
     */
    final class Transactions extends Gemini
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Wrapper for underlying API fetcher
         * @return array<string>
         */
        private function get_transactions(int $timestamp): array
        {
            $response = $this->get_api()->fetch_transactions(
                null,
                $timestamp,
                self::PAGINATION_LIMIT
            );

            utils\CLI::print_debug($response);
            return $response;
        }

        //! @brief Implements read handler
        public function read(): array
        {
            $response = $this->get_transactions($this->get_given_timestampms());

            // Return if no transfers (it's a new year with no new activity, for example)
            if (empty($response)) {
                return [];
            }

            // Paginate through all raw txs
            $stack = [];

            while (!empty($response[0])) {
                // Only read given year (if response spans multiple years because of few distant txs)
                foreach ($response as $entry) {
                    // @phpstan-ignore-next-line  // TODO: resolve
                    if ($entry['info']['timestampms'] <= $this->get_max_timestampms()) {
                        array_push($stack, $entry);
                    }
                }

                // Finished
                if (count($stack) < self::PAGINATION_LIMIT) {
                    break;
                }

                // Pagination
                // @phpstan-ignore-next-line  // TODO: resolve
                $since = end($stack)['info']['timestampms'] + 1;

                // Only read given year
                if ($since >= $this->get_max_timestampms()) {
                    break;
                }

                // Get more
                sleep(5);  // Helps with rate-limiting
                $response = $this->get_transactions($since);

                # If *this* response is empty, break
                if (empty($response)) {
                    break;
                }

                // Prevent forever looping (only read given year)
                // @phpstan-ignore-next-line  // TODO: resolve
                $current_since = $response[0]['info']['timestampms'];  // Ascending
                if ($current_since >= $this->get_max_timestampms()) {
                    break;
                }
            }

            return $stack;
        }

        /**
         * @brief Implements write handler
         * @param array<string> $stack Data to write
         * @param string $file File to write to
         */
        public function write(mixed $stack, string $file): void
        {
            // NOTE: unique sorting is done in preprocess
            $this->raw_to_csv($stack, $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Transactions/Transfers");
            $this->write(
                $this->read(),
                $this->get_env()->get_env('API_OUT_DIR') . 'Transfers.csv'
            );
        }
    }

    /**
     * @brief Gemini Earn
     * @since docker-finance 1.0.0
     */
    final class Earn extends Gemini
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Wrapper for API request
         * @phpstan-ignore-next-line  // TODO: resolve
         */
        private function get_earn(int $until_timestampms = 0): array
        {
            // One year if no time given
            if (!$until_timestampms) {
                $until_timestampms = $this->get_max_timestampms();
            }

            $response = $this->request(
                'v1/earn/history',
                'private',
                'POST',
                ['limit' => 500, 'interestOnly' => true, 'since' => $this->get_given_timestampms(), 'until' => $until_timestampms, 'sortAsc' => false]  // @phpstan-ignore-line  // TODO: resolve
            );  // TODO: set PAGINATION_LIMIT

            utils\CLI::print_debug($response);
            return $response;
        }

        /**
         * @brief Implements read handler
         * @note Constructively received daily (currently at 8/9pm UTC)
         * @return array<string>
         */
        public function read(): array
        {
            $response = $this->get_earn();

            // None yet earned
            if (empty($response)) {
                return [];
            }

            // Earned
            $stack = [];
            $done = false;

            while (!$done) {
                // Should be only one element. Txs are inside that element.
                if (count($response) > 1) {
                    utils\CLI::throw_fatal('gemini Earn not fully supported');
                }

                $transactions = $response[0]['transactions'];
                for ($j = 0; $j < count($transactions); $j++) {
                    $tx = $transactions[$j];
                    if ($tx['dateTime'] >= $this->get_max_timestampms()) {
                        break;
                    }
                    array_push($stack, $tx);
                }

                sleep(1);

                $next = $transactions[count($transactions) - 1]['dateTime'] * 1;
                $response = $this->get_earn($next);

                $new_txs = $response[0]['transactions'];
                $current = $new_txs[count($new_txs) - 1]['dateTime'] * 1;

                if ($current == $next) {
                    $done = true;
                }
            }

            return $stack;
        }

        //! @brief Implements write handler
        public function write(mixed $stack, string $file): void
        {
            // NOTE: array_unique needed to removes dups. WARNING: will break if removed
            $this->raw_to_csv(array_unique($stack, SORT_REGULAR), $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Earn");
            $this->write(
                $this->read(),
                $this->get_env()->get_env('API_OUT_DIR') . 'EarnInterest.csv'
            );
        }
    }
}  // namespace dfi\exchanges\internal\exchanges\gemini

//! @since docker-finance 1.0.0

namespace dfi\exchanges\internal\exchanges
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    /**
     * @brief Facade for Gemini implementation
     * @ingroup php_exchanges
     * @since docker-finance 1.0.0
     */
    final class Gemini extends \dfi\exchanges\API
    {
        private gemini\Trades $trades;
        private gemini\Transactions $transactions;
        private gemini\Earn $earn;

        public function __construct(\dfi\utils\Env $env)
        {
            $this->trades = new gemini\Trades($env);
            $this->transactions = new gemini\Transactions($env);
            $this->earn = new gemini\Earn($env);
        }

        public function fetch(): void
        {
            $this->trades->fetch();
            $this->transactions->fetch();
            $this->earn->fetch();
        }
    }
}  // namespace dfi\exchanges\internal\exchanges

# vim: sw=4 sts=4 si ai et
