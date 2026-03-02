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

namespace dfi\exchanges
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/exchanges/coinbase.php');
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/exchanges/gemini.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Exchanges fetcher API
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

        //! @brief Fetch executor
        public function fetch(): void
        {
            // Ensure API out path
            $out_dir = $this->get_env()->get_env('API_OUT_DIR');
            if (!file_exists($out_dir)) {
                if (!mkdir($out_dir, 0700, true)) {
                    utils\CLI::throw_fatal("Unable to create $out_dir");
                }
            }

            // API factory
            $subtype = $this->get_env()->get_env('API_FETCH_SUBTYPE');
            switch ($subtype) {
                case 'coinbase':
                    $this->api = new internal\exchanges\Coinbase($this->get_env());
                    break;
                case 'gemini':
                    $this->api = new internal\exchanges\Gemini($this->get_env());
                    break;
                default:
                    utils\CLI::throw_fatal("unsupported subtype '$subtype' for interal API");
                    break;
            }

            // Execute
            assert(isset($this->api));
            $this->api->fetch();
        }
    }
}  // namespace dfi\exchanges

# vim: sw=4 sts=4 si ai et
