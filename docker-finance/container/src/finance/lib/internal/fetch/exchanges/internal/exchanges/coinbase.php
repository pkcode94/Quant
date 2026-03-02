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

namespace dfi\exchanges\internal\exchanges\coinbase
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Common implementaion
     * @since docker-finance 1.0.0
     */
    abstract class Impl extends \dfi\exchanges\internal\Impl
    {
        private \ccxt\coinbase $api;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $this->api = new \ccxt\coinbase([
                'apiKey' => $this->get_env()->get_env('API_KEY'),
                'secret' =>  $this->get_env()->get_env('API_SECRET'),
                //'verbose' => true, // uncomment for debugging
                // https://github.com/ccxt/ccxt/wiki/Manual#rate-limit
                'enableRateLimit' => true, // rate-limiting is required by the Manual
            ]);
        }

        protected function get_api(): \ccxt\coinbase
        {
            return $this->api;
        }
    }

    /**
     * @brief Coinbase Accounts
     * @details All available accounts for end-user (e.g., wallets, vaults, etc.)
     * @since docker-finance 1.0.0
     */
    abstract class Accounts extends Impl
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Get Coinbase Accounts with underlying API
         * @return array<mixed> ccxt fetchAccounts object
         */
        private function get(): array
        {
            // Note: given the small size of returned structure,
            // there's currently no forseeable need to paginate.
            $response = $this->get_api()->fetchAccounts();  // @phpstan-ignore-line
            utils\CLI::print_debug($response);
            return $response;
        }

        /**
         * @brief Get Coinbase Accounts
         * @return array<mixed> ccxt fetchAccounts object
         */
        protected function get_accounts(): array
        {
            return Accounts::get();
        }
    }

    /**
     * @brief Coinbase Transactions
     * @details Transfers / Rewards / non-Trades
     * @since docker-finance 1.0.0
     */
    abstract class Transactions extends Accounts
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Get Coinbase transactions with underlying API
         * @param array<mixed> $account ccxt fetchAccounts object of Coinbase account (e.g., wallet, vault, etc.)
         * @param int $since Milliseconds since epoch to begin fetch from
         * @return array<mixed> Unpaginated (but with cursor) ledger (transactions) of given account since timestamp
         */
        private function get(array $account, int $since): array
        {
            $params = [];

            // WARNING: must be added in order to discern between wallet and vault
            $params['account_id'] = $account['id'];

            // Pagination
            if (array_key_exists('starting_after', $account)) {
                $params['starting_after'] = $account['starting_after'];
            }

            // @phpstan-ignore-next-line
            $response = $this->get_api()->fetchLedger(
                $account['code'],
                $since,
                100,
                $params
            );

            utils\CLI::print_debug($response);
            return $response;
        }

        /**
         * @brief Get Coinbase transactions and prepare for reading
         * @param array<mixed> $account ccxt fetchAccounts object of Coinbase account (e.g., wallet, vault, etc.)
         * @return array<mixed> Complete ledger (transactions) of given account for given (environment) year
         */
        protected function get_transactions(array $account): array
        {
            $id = $account['id'];
            $name = $account['info']['name'];

            utils\CLI::print_custom("    \e[32m│\e[0m\n");
            utils\CLI::print_custom("    \e[32m├─\e[34m\e[1;3m {$name}\e[0m\n");
            utils\CLI::print_custom("    \e[32m│  └─\e[37;2m {$id}\e[0m\n");

            // account_id => tx
            $stack[] = $id;

            // Since given timestamp (milliseconds since epoch)
            $given_year = $this->get_env()->get_env('API_FETCH_YEAR');
            $year = new \DateTime($this->get_env()->get_env('API_FETCH_YEAR') . '-01-01');
            $timestamp = $year->format('U') * 1000;

            while (true) {
                // Transactions of account ID
                $txs = Transactions::get($account, $timestamp);

                if (!count($txs)) {
                    break;
                }

                // Keep only given year's tx's
                for ($i = 0; $i < count($txs); ++$i) {
                    $info = $txs[$i]['info'];

                    // Format: 1970-01-01T12:34:56Z
                    $created_at = $info['created_at'];
                    $at_year = explode('-', $created_at)[0];
                    if ($at_year == $given_year) {
                        $stack[$id][] = $txs[$i];
                    }
                }

                // Paginate (if needed)
                $last = $txs[array_key_last($txs)];
                if (array_key_exists('next_starting_after', $last['info'])) {
                    $account['starting_after'] = $last['info']['next_starting_after'];
                } else {
                    break;
                }
            }

            sort($stack);
            return $stack;
        }
    }

    /**
     * @brief Coinbase fetch object
     * @details Implements complete fetch operation
     * @since docker-finance 1.0.0
     */
    final class Coinbase extends Transactions
    {
        //! @brief No-op
        //! @todo Base refactor for exclusive ccxt
        public function request(string $path): mixed
        {
            return [];
        }

        public function read(): array
        {
            // Fetch accounts (not txs of accounts)
            $accounts = $this->get_accounts();

            // Parse env for only the given symbols/accounts
            $symbols = $this->parse_subaccount_symbols(
                $this->get_env()->get_env('API_SUBACCOUNT')
            );

            $stack = [];

            foreach ($accounts as $account) {
                // Get only applicable symbols/accounts
                if (!empty($symbols)) {
                    $code = $account['code'];
                    foreach ($symbols as $symbol) {
                        if ($symbol == $code) {
                            $txs = $this->get_transactions($account);
                            if (!empty($txs)) {
                                array_push($stack, $txs);
                            }
                        }
                    }
                } else {
                    // Get all
                    $txs = $this->get_transactions($account);
                    if (!empty($txs)) {
                        array_push($stack, $txs);
                    }
                }
            }

            return $stack;
        }

        public function write(mixed $txs, string $id): void
        {
            $file = $this->get_env()->get_env('API_OUT_DIR') . $id . '_transactions.csv';
            $this->raw_to_csv($txs, $file);
        }

        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Account");

            foreach ($this->read() as $account => $transactions) {

                // All reads must have 'account_id' attached
                $id = $transactions[0];
                if (empty($id)) {
                    utils\CLI::throw_fatal("Missing account ID");
                }

                // Not all reads will have txs (no account activity since timestamp)
                if (array_key_exists(1, $transactions)) {
                    $txs = $transactions[1];
                    if (!empty($txs)) {
                        $this->write($txs, $id);
                    }
                }
            }

            utils\CLI::print_custom("    \e[32m│\e[0m\n");
        }
    }
}  // namespace dfi\exchanges\internal\exchanges\coinbase

//! @since docker-finance 1.0.0

namespace dfi\exchanges\internal\exchanges
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    /**
     * @brief Facade for Coinbase implementation
     * @ingroup php_exchanges
     * @since docker-finance 1.0.0
     */
    final class Coinbase extends \dfi\exchanges\API
    {
        private coinbase\Coinbase $api;

        public function __construct(\dfi\utils\Env $env)
        {
            $this->api = new coinbase\Coinbase($env);
        }

        public function fetch(): void
        {
            $this->api->fetch();
        }
    }
}  // namespace dfi\exchanges\internal\exchanges

# vim: sw=4 sts=4 si ai et
