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

namespace dfi\exchanges\internal\exchanges\coinbase_pro
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use MockingMagician\CoinbaseProSdk\CoinbaseFacade;
    use MockingMagician\CoinbaseProSdk\Contracts\Api\ApiInterface;
    use dfi\utils as utils;

    /**
     * @brief Common implementaion
     * @since docker-finance 1.0.0
     */
    abstract class Impl extends \dfi\exchanges\internal\Impl
    {
        // @phpstan-ignore-next-line // TODO: resolve
        private $api;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $config = CoinbaseFacade::createDefaultCoinbaseConfig(
                'https://api.pro.coinbase.com',
                $this->get_env()->get_env('API_KEY'),
                $this->get_env()->get_env('API_SECRET'),
                $this->get_env()->get_env('API_PASSPHRASE')
            );

            $this->api = CoinbaseFacade::createCoinbaseApi($config);
        }

        // @phpstan-ignore-next-line // TODO: resolve
        protected function get_api()
        {
            return $this->api;
        }
    }

    /**
     * @brief Common implementation
     * @since docker-finance 1.0.0
     */
    abstract class CoinbasePro extends Impl
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Implements underlying API request
         * @todo Refactor with interface in mind
         */
        protected function request(string $url): mixed
        {
            return [];  // no-op
        }
    }

    /**
     * @brief CoinbasePro Transactions
     * @since docker-finance 1.0.0
     */
    final class Transactions extends CoinbasePro
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Get withdrawals
         * @todo Refactor with interface in mind
         * @phpstan-ignore-next-line // TODO: resolve
         */
        private function get_raw_withdrawals(): array
        {
            $pagination = CoinbaseFacade::createPagination();

            $stack = [];
            while ($pagination->hasNext()) {
                $withdrawals = $this->get_api()->withdrawals()->listWithdrawalsRaw(null, $pagination);
                array_push($stack, json_decode($withdrawals, true));
            }

            // Iterate through multi-dimensional array
            $final_stack = [];
            for ($i = 0; $i < count($stack); $i++) {
                for ($j = 0; $j < count($stack[$i]); $j++) {
                    array_push($final_stack, $stack[$i][$j]);
                }
            }

            return $final_stack;
        }

        /**
         * @brief Get transactions
         * @todo Refactor with interface in mind
         */
        private function get_raw_history(): void
        {
            // Get all accounts. NOTE: no pagination necessary when getting this list
            $accounts = $this->get_api()->accounts()->list();

            // Get array with all withdrawals
            $withdrawals = $this->get_raw_withdrawals();

            foreach ($accounts as $acct) {
                $id = $acct->getId();
                $currency = $acct->getCurrency();

                // IF crypto withdrawal

                //   [details] => Array
                //       (
                //           [fee] =>
                //           [subtotal] =>
                //           [sent_to_address] =>
                //           [coinbase_account_id] =>
                //           [crypto_transaction_hash] =>
                //           [tx_service_transaction_id] =>
                //           [coinbase_payment_method_id] =>
                //       )

                // IF ACH withdrawal (or transfer to Coinbase)

                //   [details] => Array
                //       (
                //           [coinbase_payout_at] =>
                //           [coinbase_account_id] =>
                //           [coinbase_withdrawal_id] =>
                //           [coinbase_transaction_id] =>
                //           [coinbase_payment_method_id] =>
                //           [coinbase_payment_method_type] =>
                //       )

                $i = 0;  // File number pagination
                $withdrawal_stack = [];

                for ($j = 0; $j < count($withdrawals); $j++) {
                    if ($withdrawals[$j]['account_id'] == $id) {
                        $account_id = $withdrawals[$j]['account_id'];  // Will determine currency type
                        $created_at = $withdrawals[$j]['created_at'];
                        $total = $withdrawals[$j]['amount'];           // Needed for non-crypto withdrawals

                        $details = $withdrawals[$j]['details'];        // Needed for everything

                        $subtotal = "";
                        if (!empty($details['subtotal'])) {
                            $subtotal = $details['subtotal'];
                        }

                        $fee = "";
                        if (!empty($details['fee'])) {
                            $fee = $details['fee'];
                        }

                        $destination = "";
                        if (!empty($details['sent_to_address'])) {
                            $destination = $details['sent_to_address'];
                        }  // Crypto
                        if (empty($destination) && !empty($details['coinbase_payment_method_type'])) {
                            $destination = $details['coinbase_payment_method_type'];
                        }  // Non-crypto (USD) or transfer to Coinbase

                        $txid = "";
                        // Crypto
                        if (!empty($details['crypto_transaction_hash'])) {
                            $txid = $details['crypto_transaction_hash'];
                        }
                        // Non-crypto (USD) or transfer to Coinbase
                        if (empty($txid) && !empty($details['coinbase_transaction_id'])) {
                            $txid = $details['coinbase_transaction_id'];
                        }

                        array_push(
                            $withdrawal_stack,
                            "transfer_out" .','. $account_id .','. $created_at .','. $destination .','. $subtotal .','. $fee .','. $total .','. $txid . "\n"
                        );
                    }
                }

                $base_file = $this->get_env()->get_env('API_OUT_DIR') . $id . '_' . $currency . "_";

                if (count($withdrawal_stack) > 0) {
                    $stack = [];

                    array_push(
                        $stack,
                        "type,account_id,created_at,destination,subtotal,fee,total,txid" . "\n"
                    );

                    foreach ($withdrawal_stack as $withdraw) {
                        array_push($stack, $withdraw);
                    }

                    $this->write($stack, $base_file . "WITHDRAWALS" . '.csv');
                }

                // Get complete raw history of account
                // (skip the withdrawals here and use custom impl in rules)
                $k = 0;  // Pagination for filename
                $pagination = CoinbaseFacade::createPagination();
                while ($pagination->hasNext()) {
                    $history = $this->get_api()->accounts()->getAccountHistoryRaw($id, $pagination);
                    if ($history != '[]') {
                        $this->write($history, $base_file . $k . '.csv');
                        $k++;
                    }
                }
            }
        }

        /**
         * @brief Implements read handler
         * @return array<null> Transactions, prepared for writing
         */
        public function read(): array
        {
            return [];  // no-op
        }

        //! @brief Implements write handler
        public function write(mixed $stack, string $file): void
        {
            if (str_contains($file, "WITHDRAWALS")) {
                file_put_contents($file, implode($stack), LOCK_EX);
                return;
            }

            // Already JSON encoded
            utils\Json::write_csv($stack, $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Transactions");
            $this->get_raw_history();
        }
    }
}  // namespace dfi\exchanges\internal\exchanges\coinbase_pro

//! @since docker-finance 1.0.0

namespace dfi\exchanges\internal\exchanges
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    /**
     * @brief Facade for Coinbase Pro implementation
     * @ingroup php_exchanges
     * @since docker-finance 1.0.0
     */
    final class CoinbasePro extends \dfi\exchanges\API
    {
        private coinbase_pro\Transactions $transactions;

        public function __construct(\dfi\utils\Env $env)
        {
            $this->transactions = new coinbase_pro\Transactions($env);
        }

        public function fetch(): void
        {
            $this->transactions->fetch();
        }
    }
}  // namespace dfi\exchanges\internal\exchanges

# vim: sw=4 sts=4 si ai et
