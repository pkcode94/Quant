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

namespace dfi\blockchains\internal\blockchains\tezos
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\blockchains\internal as internal;
    use dfi\utils as utils;

    /**
     * @brief TzKt (Tezos Kitty) API
     * @ingroup php_API_impl
     * @note TzKt and TzStats/Pro require near-identical implementations
     * @since docker-finance 1.0.0
     */
    final class TzKt extends internal\API
    {
        public const PAGINATION_LIMIT = 1000;

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
            // Total response
            $stack = [];

            // First response
            $response = json_decode(
                $this->request($metadata->get_url()),
                true,
                512,
                JSON_PRETTY_PRINT
            );

            if (empty($response)) {
                return [];
            }
            array_push($stack, $response);

            // Prepare pagination
            $last_id = '';
            $id = end($response)['id'];

            // Remaining responses (paginate as needed)
            while ((count($response) == self::PAGINATION_LIMIT) && ($id != $last_id)) {
                $request = $metadata->get_url() . '&lastId='. $id;
                utils\CLI::print_debug($request);

                $response = json_decode(
                    $this->request($request),
                    true,
                    512,
                    JSON_PRETTY_PRINT
                );

                if (empty($response)) {
                    break;
                }
                array_push($stack, $response);

                $last_id = $id;
                $id = end($response)['id'];
            }

            utils\CLI::print_debug($stack);
            return $stack;
        }

        /**
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
                "account_name,subaccount_name,subaccount_address,hash,type,status,block,time,volume,fee,sender,alias,receiver,direction,delegate\n"
            );

            // Get metadata
            $account_name = $metadata->get_account();
            $blockchain = $metadata->get_blockchain();
            $subaccount_name = $metadata->get_subaccount();
            $subaccount_address = $metadata->get_address();

            // Write tx
            foreach ($response as $tx) {
                $hash = $tx['hash'];
                $type = $tx['type'];
                $status = $tx['status'];
                $block = $tx['block'];
                $timestamp = $tx['timestamp'];
                $amount = number_format($tx['amount'], 6, '.', '');
                $fee = number_format($tx['fee'], 6, '.', '');
                $sender = $tx['sender'];
                $alias = $tx['alias'];
                $receiver = $tx['receiver'];
                $direction = ($subaccount_address == $sender) ? 'OUT' : 'IN';
                $delegate = $tx['delegate'];

                // Push to stack
                // TODO: implement contract-related (swapping, tokens, etc.) in parser but,
                // until then, remove invalid (0 value or wrong value) entries (but keep fee spends).
                if (($subaccount_address == $sender || $subaccount_address == $receiver) || $fee > 0) {
                    if ($direction == 'IN' && ($subaccount_address != $sender && $subaccount_address != $receiver)) {
                        // Swapping will show different sender/receiver addresses and
                        // direction will be 'IN' when it should be 'OUT' (fee spend)
                        $direction = 'OUT';
                    }
                    array_push(
                        $stack,
                        "{$account_name},{$subaccount_name},{$subaccount_address},{$hash},{$type},{$status},{$block},{$timestamp},{$amount},{$fee},{$sender},{$alias},{$receiver},{$direction},{$delegate}\n"
                    );
                }
            }

            utils\CLI::print_debug($stack);
            return $stack;
        }

        //! @brief Implements parser
        public function parse_response(internal\Metadata $metadata, array $response): array
        {
            $stack = [];

            for ($i = 0; $i < count($response); $i++) {

                for ($j = 0; $j < count($response[$i]); $j++) {

                    // Get entry
                    $tx = $response[$i][$j];

                    // Transaction
                    $amount = (array_key_exists('amount', $tx)) ? $tx['amount'] / 1000000 : 0;
                    $receiver = (array_key_exists('target', $tx)) ? $tx['target']['address'] : '';
                    $sender = '';
                    $alias = '';
                    if (array_key_exists('sender', $tx)) {
                        $sender = (array_key_exists('address', $tx['sender'])) ? $tx['sender']['address'] : '';
                        $alias = (array_key_exists('alias', $tx['sender'])) ? $tx['sender']['alias'] : '';
                    }

                    // Staking
                    $delegate = (array_key_exists('baker', $tx)) ? $tx['baker'] : '';

                    // Fees
                    $fee = (array_key_exists('bakerFee', $tx)) ? $tx['bakerFee'] / 1000000 : 0;
                    $storage = (array_key_exists('storageFee', $tx)) ? $tx['storageFee'] / 1000000 : 0;
                    $fee += floatval($storage);
                    $allocation = (array_key_exists('allocationFee', $tx)) ? $tx['allocationFee'] / 1000000 : 0;
                    $fee += floatval($allocation);

                    // TODO: token support (e.g., USDtz)
                    $entry = array(
                      'id' => $tx['id'],
                      'hash' => $tx['hash'],
                      'type' => $tx['type'],
                      'status' => $tx['status'],
                      'block' => $tx['block'],
                      'timestamp' => $tx['timestamp'],
                      'amount' => $amount,
                      'fee' => $fee,
                      'sender' => $sender,
                      'alias' => $alias,
                      'receiver' => $receiver,
                      'delegate' => $delegate,
                    );

                    // TODO: rules file will need to support all statuses
                    switch ($entry['status']) {
                        case 'applied':
                        case 'backtracked':
                            break;
                        default:
                            utils\CLI::throw_fatal('Unsupported status: ' . $entry['status']);
                            break;
                    }

                    switch ($entry['type']) {
                        case 'delegation':
                        case 'origination':
                        case 'reveal':
                        case 'transaction':
                            break;
                            // TODO: more as needed
                        default:
                            utils\CLI::throw_fatal('Unsupported tx-type: ' . $tx['type']);
                            break;
                    }

                    // Save entry
                    utils\CLI::print_debug($entry);
                    array_push($stack, $entry);
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
            $address = $metadata->get_address();
            $year = $metadata->get_year();
            $metadata->set_url(
                "https://api.tzkt.io/v1/accounts/{$address}/operations?limit=" . self::PAGINATION_LIMIT . "&sort=0&timestamp.ge={$year}-01-01T00:00:00Z&timestamp.le={$year}-12-31T23:59:59Z"
            );
            utils\CLI::print_debug($metadata->get_url());
            $this->write_transactions($metadata);
        }
    }
}  // namespace dfi\blockchains\internal\blockchains\tezos

//! @since docker-finance 1.0.0

namespace dfi\blockchains\internal\blockchains
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\blockchains\internal as internal;
    use dfi\utils as utils;

    /**
     * @brief Facade for Tezos implementation
     * @ingroup php_blockchains
     * @since docker-finance 1.0.0
     */
    final class Tezos extends \dfi\blockchains\API
    {
        private tezos\TzKt $explorer;  //!< Internal API

        public function __construct(\dfi\utils\Env $env)
        {
            $this->explorer = new tezos\TzKt($env);
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
