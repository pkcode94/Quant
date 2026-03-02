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

namespace dfi\blockchains\internal\blockchains\algorand
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\blockchains\internal as internal;
    use dfi\utils as utils;

    /**
     * @brief Implements client for Algorand Indexer v2 API
     * @ingroup php_API_impl
     * @since docker-finance 1.0.0
     */
    final class AlgoIndexer extends internal\API
    {
        public const ALGORAND_MULTIPLIER = 0.000001;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        /**
         * @brief Implements response handler
         * @return array<string>
         */
        public function get_response(internal\Metadata $metadata): array
        {
            $stack = [];

            // First response
            $response = json_decode(
                $this->request($metadata->get_url()),
                true,
                512,
                JSON_PRETTY_PRINT
            );

            array_push($stack, $response);

            // Prepare pagination
            $prev_token = '';
            $next_token = (array_key_exists('next-token', $response)) ? $response['next-token'] : '';

            // Paginate as needed
            while ($next_token != '' && ($next_token != $prev_token)) {
                $response = json_decode(
                    $this->request($metadata->get_url() . '?&next='. $next_token),
                    true,
                    512,
                    JSON_PRETTY_PRINT
                );

                array_push($stack, $response);

                $prev_token = $next_token;
                $next_token = (array_key_exists('next-token', $response)) ? $response['next-token'] : '';
            }

            utils\CLI::print_debug($stack);

            return $stack;
        }

        /**
         * @note Since a tx includes rewards, separate tx into two tx's
         *   (both with same ID and timestamp), allowing tag INCOME for taxation.
         * @param array<mixed> $response
         * @return array<string>
         */
        private function create_csv_from_response(internal\Metadata $metadata, array $response): array
        {
            if (empty($response)) {
                return [];
            }

            // Final CSV data stack
            $stack = [];

            // Header row
            array_push(
                $stack,
                "account_name,subaccount_name,subaccount_address,id,fee,receiver_rewards,round_time,sender,sender_rewards,tx_type,amount,receiver,direction,to_self\n"
            );

            // Get metadata
            $account_name = $metadata->get_account();
            $blockchain = $metadata->get_blockchain();
            $subaccount_name = $metadata->get_subaccount();
            $subaccount_address = $metadata->get_address();

            foreach ($response as $tx) {
                // Write the tx without reward
                $id = $tx['id'];
                $fee = $tx['fee'];
                $receiver_rewards = 0;
                $round_time = $tx['round_time'];
                $sender = $tx['sender'];
                $sender_rewards = 0;
                $tx_type = $tx['tx_type'];
                $amount = number_format($tx['amount'], 6, '.', '');
                $receiver = $tx['receiver'];

                $direction = 'IN';
                if ($subaccount_address == $sender) {
                    $direction = 'OUT';
                }

                $to_self = 'false';
                if ($subaccount_address == $sender && $sender == $receiver) {
                    $to_self = 'true';
                }

                array_push(
                    $stack,
                    "{$account_name},{$subaccount_name},{$subaccount_address},{$id},{$fee},{$receiver_rewards},{$round_time},{$sender},{$sender_rewards},{$tx_type},{$amount},{$receiver},{$direction},{$to_self}\n"
                );

                // Write the reward tx
                $fee = 0;
                $receiver_rewards = number_format($tx['receiver_rewards'], 6, '.', '');
                $sender_rewards = number_format($tx['sender_rewards'], 6, '.', '');
                $amount = 0;

                array_push(
                    $stack,
                    "{$account_name},{$subaccount_name},{$subaccount_address},{$id},{$fee},{$receiver_rewards},{$round_time},{$sender},{$sender_rewards},{$tx_type},{$amount},{$receiver},{$direction},{$to_self}\n"
                );
            }

            return $stack;
        }

        //! @brief Implements parser
        public function parse_response(internal\Metadata $metadata, array $response): array
        {
            $stack = [];

            foreach ($response as $element) {
                // Ensure transaction data
                if (!array_key_exists('transactions', $element)) {
                    continue;
                }

                // End of (paginated) transactions
                $transactions = $element['transactions'];
                if (!is_countable($transactions) || !count($transactions)) {
                    break;
                }

                // Parse transactions
                for ($i = 0; $i < count($transactions); $i++) {
                    $tx = $transactions[$i];

                    $entry = array(
                      'id' => $tx['id'],
                      'fee' => $tx['fee'] * self::ALGORAND_MULTIPLIER,
                      'receiver_rewards' => $tx['receiver-rewards'] * self::ALGORAND_MULTIPLIER,
                      'round_time' => date('Y-m-d H:i:s', $tx['round-time']),
                      'sender' => $tx['sender'],
                      'sender_rewards' => $tx['sender-rewards'] * self::ALGORAND_MULTIPLIER,
                      'tx_type' => $tx['tx-type'],
                    );

                    switch ($tx['tx-type']) {
                        case 'pay':
                            $entry['amount'] = $tx['payment-transaction']['amount'] * self::ALGORAND_MULTIPLIER;
                            $entry['receiver'] = $tx['payment-transaction']['receiver'];
                            break;
                        case 'axfer':
                            // TODO: implement (amount will be an asset, not ALGO)
                            $entry['amount'] = 0;
                            $entry['receiver'] = $tx['asset-transfer-transaction']['receiver'];
                            break;
                        case 'appl':
                        case 'keyreg':
                            // TODO: implement
                            $entry['amount'] = 0;
                            $entry['receiver'] = '';
                            break;
                        default:
                            utils\CLI::throw_fatal('Unsupported tx-type: ' . $tx['tx-type']);
                            break;
                    }

                    // Save given year entry
                    $year = explode('-', $entry['round_time'])[0];
                    if ($year == $this->get_env()->get_env('API_FETCH_YEAR')) {
                        array_push($stack, $entry);
                    }

                    // Report inner-txns
                    if (array_key_exists('inner-txns', $tx)) {
                        if (array_key_exists('inner-txns', $tx['inner-txns'][0])) {
                            $inner_txn = $tx['inner-txns'][0]['inner-txns'][0];

                            // NOTE: Not everything will be in inner_txn so re-use the previous info as it's in the same tx
                            $alt_entry = array(
                              'id' => $tx['id'],
                              'fee' => $inner_txn['fee'] * self::ALGORAND_MULTIPLIER,
                              'receiver_rewards' => $inner_txn['receiver-rewards'] * self::ALGORAND_MULTIPLIER,
                              'round_time' => date('Y-m-d H:i:s', $inner_txn['round-time']),
                              'sender' => $inner_txn['sender'],
                              'sender_rewards' => $inner_txn['sender-rewards'] * self::ALGORAND_MULTIPLIER,
                              'tx_type' => $inner_txn['tx-type'],
                              'receiver' => (array_key_exists('payment-transaction', $inner_txn)) ? $inner_txn['payment-transaction']['receiver'] : $entry['receiver'],
                              //'sender' => (array_key_exists('payment-transaction', $inner_txn)) ? $inner_txn['payment-transaction']['sender'] : $tx['sender'],
                              'amount' => (array_key_exists('payment-transaction', $inner_txn)) ? $inner_txn['payment-transaction']['amount'] * self::ALGORAND_MULTIPLIER : $entry['amount'],
                            );

                            // Save given year entry
                            $year = explode('-', $alt_entry['round_time'])[0];
                            if ($year == $this->get_env()->get_env('API_FETCH_YEAR')) {
                                array_push($stack, $alt_entry);
                            }
                        }
                    }
                }
            }

            return $this->create_csv_from_response($metadata, $stack);
        }

        //! @brief Implements writer
        public function write_transactions(internal\Metadata $metadata): void
        {
            $response = $this->parse_response($metadata, $this->get_response($metadata));
            if (!empty($response)) {
                $this->write_response($metadata, $response);
            }
        }

        //! @brief Implements fetcher
        public function fetch(internal\Metadata $metadata): void
        {
            $metadata->set_url('https://mainnet-idx.algonode.network/v2/accounts/' . $metadata->get_address() . '/transactions');
            $this->write_transactions($metadata);
        }
    }
}  // namespace dfi\blockchains\internal\blockchains\algorand

//! @since docker-finance 1.0.0

namespace dfi\blockchains\internal\blockchains
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\blockchains\internal as internal;
    use dfi\utils as utils;

    /**
     * @brief Facade for Algorand implementation
     * @ingroup php_blockchains
     * @since docker-finance 1.0.0
     */
    final class Algorand extends \dfi\blockchains\API
    {
        private algorand\AlgoIndexer $explorer;  //!< Internal API

        public function __construct(utils\Env $env)
        {
            $this->explorer = new algorand\AlgoIndexer($env);
        }

        public function fetch(internal\Metadata|null $metadata = null): void
        {
            if (is_null($metadata)) {
                utils\CLI::throw_fatal("metadata unavailable");
            }

            $this->explorer->fetch($metadata);
        }
    }
}  // namespace dfi\blockchains\internal\blockchains

# vim: sw=4 sts=4 si ai et
