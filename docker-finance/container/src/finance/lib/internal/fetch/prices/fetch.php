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

namespace dfi\prices
{
    require_once(__DFI_PHP_ROOT__ . 'prices/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'prices/internal/prices/crypto.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Price fetcher API
     * @details Instantiates and executes API fetcher
     * @ingroup php_API
     * @since docker-finance 1.0.0
     */
    final class Fetch extends API
    {
        private mixed $api;  //!< Internal API

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        //! @brief Fetch executor
        public function fetch(): void
        {
            $upstream = $this->get_env()->get_env('API_PRICES_API');
            switch ($upstream) {
                case 'coingecko':
                    $this->api = new internal\prices\CoinGecko($this->get_env());
                    break;
                case 'mobula':
                    $this->api = new internal\prices\Mobula($this->get_env());
                    break;
                default:
                    utils\CLI::throw_fatal(
                        "unsupported upstream API '{$upstream}' for interal API"
                    );
                    break;
            }

            // Execute
            assert(isset($this->api));
            $this->api->fetch();
        }
    }
}  // namespace dfi\prices

# vim: sw=4 sts=4 si ai et
