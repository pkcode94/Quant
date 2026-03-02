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

namespace dfi\blockchains
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/blockchains/algorand.php');
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/blockchains/ethereum.php');
    require_once(__DFI_PHP_ROOT__ . 'blockchains/internal/blockchains/tezos.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Blockchain fetcher API
     * @details Instantiates and executes API fetcher
     * @ingroup php_API
     * @since docker-finance 1.0.0
     */
    final class Fetch extends API
    {
        // @phpstan-ignore-next-line
        private $api;  //!< Internal API

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        public function fetch(internal\Metadata|null $metadata = null): void
        {
            // API_SUBACCOUNT format: blockchain/account/address[,blockchain/account/address,...]"
            // TODO: if caller is using a custom delimiter, this will break!
            $subaccount = explode(',', $this->get_env()->get_env('API_SUBACCOUNT'));

            utils\CLI::print_normal("  ─ Blockchains");

            foreach ($subaccount as $account) {
                $metadata = new internal\Metadata();

                $parsed_metadata = explode('/', $account);
                foreach ($parsed_metadata as $parsed) {
                    if (empty($parsed)) {
                        utils\CLI::throw_fatal('invalid metadata. Did you give accurate account information (account exists? is mispelled?)?');
                    }
                }

                $metadata->set_blockchain($parsed_metadata[0]);
                $metadata->set_subaccount($parsed_metadata[1]);
                $metadata->set_address($parsed_metadata[2]);

                // Year convenience helper
                $metadata->set_year($this->get_env()->get_env('API_FETCH_YEAR'));

                // Reconstruct subaccount directory for out directory
                // API_OUT_DIR format: hledger-flow/profiles/parent_profile/child_profile/import/child_profile/account/subaccount/1-in/year
                $path = explode('/', $this->get_env()->get_env('API_OUT_DIR'));
                $path = array_filter($path, function ($element) { return $element !== '';});  // Clear out extra / that get turned into nulls
                $head_end = $path;
                $tail_end = $path;

                // Save first (up to account) and last half (after subaccount)
                array_splice($head_end, -3);
                array_splice($tail_end, 0, -2);

                // Reconstruct full path
                $out_dir = '';
                foreach ($head_end as $head) {
                    $out_dir .= '/' . $head;
                }
                $out_dir .= '/' . $metadata->get_blockchain();
                foreach ($tail_end as $tail) {
                    $out_dir .= '/' . $tail;
                }

                $metadata->set_account(array_pop($head_end));
                $metadata->set_out_dir($out_dir);

                /**
                 * Set blockchain API key (blockchain scanning).
                 *
                 * Expects one key per chain (per account), except for
                 * ethereum-based chains (which only expects one key for all chains).
                 */
                $metadata->set_api_key('');  // Initialize to prevent fatal error
                if (str_contains($this->get_env()->get_env('API_KEY'), '/')) {
                    $key_entries = explode(',', $this->get_env()->get_env('API_KEY'));
                    foreach ($key_entries as $entry) {
                        $api_blockchain = explode('/', $entry)[0];
                        $api_key = explode('/', $entry)[1];

                        if (empty($api_key)) {
                            utils\CLI::throw_fatal("no blockchain API key value");
                        }

                        if (!empty($api_blockchain)) {
                            switch ($api_blockchain) {
                                // TODO: WARNING: if adding non-ethereum-based chain, update as needed.
                                case 'algorand':
                                case 'etherscan':
                                case 'tezos':
                                    $metadata->set_api_key($api_key);
                                    break;
                                default:
                                    utils\CLI::throw_fatal("invalid blockchain API");
                                    break;
                            }
                        }
                    }
                }

                utils\CLI::print_custom("    \e[32m│\e[0m\n");
                utils\CLI::print_custom("    \e[32m├─\e[34m\e[1;3m " . $metadata->get_blockchain() . "\e[0m\n");
                utils\CLI::print_custom("    \e[32m│  └─\e[34;3m " . $metadata->get_subaccount() . "\e[0m\n");
                utils\CLI::print_custom("    \e[32m│     └─\e[37;2m " . $metadata->get_address() . "\e[0m\n");

                // API factory
                switch ($metadata->get_blockchain()) {
                    case 'arbitrum':
                    case 'base':
                    case 'ethereum':
                    case 'optimism':
                    case 'polygon':
                        $this->api = new internal\blockchains\Ethereum($this->get_env());
                        break;
                    case 'algorand':
                        $this->api = new internal\blockchains\Algorand($this->get_env());
                        break;
                    case 'tezos':
                        $this->api = new internal\blockchains\Tezos($this->get_env());
                        break;
                    default:
                        // TODO: Blockchair API?
                        utils\CLI::throw_fatal("invalid blockchain");
                        break;
                }

                // Execute
                assert(isset($this->api));
                $this->api->fetch($metadata);

                // TODO: rate-limiting catcher
            }
        }
    }
}  // namespace dfi\blockchains

# vim: sw=4 sts=4 si ai et
