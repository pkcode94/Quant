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

namespace dfi\blockchains\internal\blockchains\ethereum
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\blockchains\internal as internal;
    use dfi\utils as utils;

    /**
     * @brief Etherscan-based API
     * @ingroup php_API_impl
     * @note
     *   - Normal txs that are normal "failed" will *not* show in the ERC-20 list
     *     even if they are actually ERC-20. So, just deduct the gas and move on.
     *   - ERC-20 txs will *not* list any normal ETH sends or wrapping of ETH.
     * @since docker-finance 1.0.0
     */
    final class Etherscan extends internal\API
    {
        private string $gwei;

        /**
         * @warning Decimal places higher than 14 gives floating point precision
         * rounding error w/out proper impl (see below)
         */
        public const ETHEREUM_DECIMALS = 18;
        public const ETHEREUM_GWEI = 0.000000001;
        public const ETHEREUM_GWEI_LEN =  9;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
            $this->gwei = sprintf("%." . self::ETHEREUM_GWEI_LEN . "f", self::ETHEREUM_GWEI);
        }

        /**
         * @brief Calculate tx amounts
         * @details Upstream returns a variable-size string with the higher length
         *   being ETH transferred. Example: 279006000000000000000 = 279.006
         */
        private function amount_calculator(string $value, string|null $decimal = null): string
        {
            // non-ERC-20
            if (empty($decimal)) {
                $decimal = '0.000000000000000001';
            }

            return rtrim(rtrim(bcmul($value, $decimal, self::ETHEREUM_DECIMALS), '0'), '.');
        }

        /**
         * @brief Calculate tx fees
         * @details Fee impl that "guarantees" no floating point rounding errors
         * @note ethFee=(gasUsed*(gasPrice*ETHEREUM_GWEI))*ETHEREUM_GWEI
         */
        private function gas_calculator(string $gas_used, string $gas_price): string
        {
            // Work backwards
            $gas_price_len = strlen($gas_price);
            $first_bcmul = bcmul(sprintf("%.$gas_price_len" . 'f', $gas_price), $this->gwei, self::ETHEREUM_DECIMALS);

            $gas_used_len = strlen($gas_used);
            $second_bcmul = bcmul(sprintf("%.$gas_used_len" . 'f', $gas_used), $first_bcmul, self::ETHEREUM_DECIMALS);

            return rtrim(bcmul($second_bcmul, $this->gwei, self::ETHEREUM_DECIMALS), '0');
        }

        //! @brief Implements response handler
        public function get_response(internal\Metadata $metadata): mixed
        {
            $response = json_decode($this->request($metadata->get_url()), true, 512, JSON_PRETTY_PRINT)['result'];
            if (is_string($response)) {
                utils\CLI::throw_fatal($response);
            }
            return $response;
        }

        //! @brief Implements parser
        public function parse_response(internal\Metadata $metadata, array $response): array
        {
            // Return empty responses (no data available for that year)
            if (empty($response)) {
                return [];
            }

            // Final CSV data stack
            $stack = [];

            // Header row
            array_push(
                $stack,
                "date,blockchain,account_name,subaccount_name,subaccount_address,type,direction,tx_index,tx_hash,token_name,token_id,contract_address,symbol,block_hash,block_number,method_id,from_address,to_address,amount,fees\n"
            );

            // Get metadata
            $account_name = $metadata->get_account();
            $blockchain = $metadata->get_blockchain();
            $subaccount_name = $metadata->get_subaccount();
            $subaccount_address = strtolower($metadata->get_address());
            $contract_type = $metadata->get_contract_type();

            foreach ($response as $tx) {
                $date = date('Y-m-d H:i:s', $tx['timeStamp']);
                $tx_hash = $tx['hash'];
                $from_address = strtolower($tx['from']);
                $to_address = strtolower($tx['to']);

                // Specific to NFTs (will be filled in below, if applicable)
                $token_id = '';

                // Specific to normal txs (will be filled in below, if applicable)
                $method_id = '';

                // Used by all except 'internal'
                $tx_index = '';
                $block_hash = '';
                if (!empty($tx['blockHash'])) {
                    $block_hash = $tx['blockHash'];
                }
                $block_number = $tx['blockNumber'];

                // Used by all
                $direction = ($subaccount_address == $from_address) ? 'OUT' : 'IN';

                // TODO: use type enum
                switch ($contract_type) {
                    case 'normal':
                        switch ($blockchain) {
                            case 'arbitrum':
                            case 'base':
                            case 'ethereum':
                            case 'optimism':
                                $token_name = 'Ethereum';
                                $symbol = 'ETH';
                                break;
                            case 'polygon':
                                $token_name = 'Polygon';
                                $symbol = 'MATIC';  // TODO: update to POL along with mockups
                                break;
                            default:
                                utils\CLI::throw_fatal('Unsupported subaccount name/network');
                                break;
                        }
                        $tx_index = $tx['transactionIndex'];
                        $contract_address = '';
                        $amount = $this->amount_calculator($tx['value']);
                        $fees = $this->gas_calculator($tx['gasUsed'], $tx['gasPrice']);
                        $method_id = $tx['methodId'];
                        break;
                    case 'internal':
                        $token_name = 'INTERNAL';  // TODO: contract address / symbol from parent tx would be needed
                        $contract_address = '';
                        switch ($blockchain) {
                            case 'arbitrum':
                            case 'base':
                            case 'ethereum':
                            case 'optimism':
                                $symbol = 'ETH';
                                break;
                            case 'polygon':
                                $symbol = 'MATIC';  // TODO: update to POL along with mockups
                                break;
                            default:
                                utils\CLI::throw_fatal('Unsupported subaccount name/network');
                                break;
                        }
                        $amount = $this->amount_calculator($tx['value']);
                        $fees = '';
                        break;
                    case 'erc-20':
                        $tx_index = $tx['transactionIndex'];
                        $token_name = str_replace(',', '', $tx['tokenName']);
                        $contract_address = $tx['contractAddress'];
                        $symbol = $tx['tokenSymbol'];
                        $token_decimal = $tx['tokenDecimal'] - 1;
                        $decimal = '0.';
                        for ($k = 0; $k < $token_decimal; $k++) {
                            $decimal .= '0';
                            if ($k == $token_decimal - 1) {
                                $decimal .= '1';
                                break;
                            }
                        }
                        $amount = $this->amount_calculator($tx['value'], $decimal);
                        $fees = $this->gas_calculator($tx['gasUsed'], $tx['gasPrice']);
                        break;
                    case 'erc-721':
                        $tx_index = $tx['transactionIndex'];
                        $token_id = $tx['tokenID'];
                        $token_name = str_replace(',', '', $tx['tokenName']);
                        $contract_address = $tx['contractAddress'];
                        $symbol = str_replace(',', '', $tx['tokenSymbol']);
                        $amount = 1;  // 1 NFT
                        $fees = $this->gas_calculator($tx['gasUsed'], $tx['gasPrice']);
                        break;
                    case 'erc-1155':
                        $tx_index = $tx['transactionIndex'];
                        $token_id = $tx['tokenID'];
                        $token_name = str_replace(',', '', $tx['tokenName']);
                        $contract_address = $tx['contractAddress'];
                        $symbol = str_replace(',', '', $tx['tokenSymbol']);
                        $amount = $tx['tokenValue'];
                        $fees = $this->gas_calculator($tx['gasUsed'], $tx['gasPrice']);
                        break;
                    default:
                        utils\CLI::throw_fatal("invalid type: . $contract_type");
                        break;
                }

                assert(!empty($amount));
                assert(!empty($contract_address));
                assert(!empty($fees));
                assert(!empty($symbol));
                assert(!empty($token_name));

                // Save given year entry
                // NOTE: PHP allows this to work outside of the loop's scope...
                $year = explode('-', $date)[0];
                if ($year == $this->get_env()->get_env('API_FETCH_YEAR')) {
                    array_push(
                        $stack,
                        "{$date},{$blockchain},{$account_name},{$subaccount_name},{$subaccount_address},{$contract_type},{$direction},{$tx_index},{$tx_hash},{$token_name},{$token_id},{$contract_address},{$symbol},{$block_hash},{$block_number},{$method_id},{$from_address},{$to_address},{$amount},{$fees}\n"
                    );
                }
            }

            // Only header row present, no txs
            if (count($stack) == 1) {
                return [];
            }

            rsort($stack);
            return $stack;
        }

        //! @brief Implements writer
        public function write_transactions(internal\Metadata $metadata): void
        {
            switch ($metadata->get_blockchain()) {
                // Arbitrum One Mainnet
                case 'arbitrum':
                    $chain_id = 42161;
                    break;
                    // Base Mainnet
                case 'base':
                    $chain_id = 8453;
                    break;
                    // Ethereum Mainnet
                case 'ethereum':
                    $chain_id = 1;
                    break;
                    // OP Mainnet
                case 'optimism':
                    $chain_id = 10;
                    break;
                    // Polygon Mainnet
                case 'polygon':
                    $chain_id = 137;
                    break;
                default:
                    utils\CLI::throw_fatal('invalid type: ' . $metadata->get_blockchain());
                    break;
            }

            assert(!empty($chain_id));
            $address = $metadata->get_address();
            $api_key = $metadata->get_api_key();

            $request = array(
              'normal' => 'txlist',
              'internal' => 'txlistinternal',
              'erc-20' => 'tokentx',
              'erc-721' => 'tokennfttx',
              'erc-1155' => 'token1155tx');

            foreach ($request as $type => $action) {
                $metadata->set_url("https://api.etherscan.io/v2/api?chainId={$chain_id}&module=account&action={$action}&address={$address}&startblock=0&endblock=999999999&sort=asc&apiKey={$api_key}");
                $metadata->set_contract_type($type);
                $response = $this->parse_response($metadata, $this->get_response($metadata));
                if (!empty($response)) {
                    $this->write_response($metadata, $response);
                }
            }
        }

        //! @brief Implements fetcher
        public function fetch(internal\Metadata $metadata): void
        {
            $this->write_transactions($metadata);
        }
    }
}  // namespace dfi\blockchains\internal\blockchains\ethereum

//! @since docker-finance 1.0.0

namespace dfi\blockchains\internal\blockchains
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\blockchains\internal as internal;
    use dfi\utils as utils;

    /**
     * @brief Facade for Ethereum implementation
     * @ingroup php_blockchains
     * @since docker-finance 1.0.0
     */
    final class Ethereum extends \dfi\blockchains\API
    {
        private ethereum\Etherscan $explorer;  //!< Internal API

        public function __construct(utils\Env $env)
        {
            $this->explorer = new ethereum\Etherscan($env);
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
