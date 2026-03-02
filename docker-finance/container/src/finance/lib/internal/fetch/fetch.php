<?php

// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2021-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

namespace {
    //! Dependencies (via composer)
    define('__DFI_PHP_DEPS__', getenv('__DFI_PHP_DEPS__') . '/');
    require_once(__DFI_PHP_DEPS__ . 'vendor/autoload.php');

    //! docker-finance `fetch` impl
    define('__DFI_PHP_ROOT__', getenv('__DFI_PHP_ROOT__') . '/');
}

namespace dfi
{
    require_once(__DFI_PHP_ROOT__ . 'blockchains/fetch.php');
    require_once(__DFI_PHP_ROOT__ . 'exchanges/fetch.php');
    require_once(__DFI_PHP_ROOT__ . 'prices/fetch.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    /**
     * @brief API executor
     * @details
     *   - One-and-only callee for external caller
     *   - Sets internal environment from external shell environment and $argv
     *   - Instantiates and executes fetching process
     *
     * @param int $argc Argument count
     * @param array<string> $argv Argument list
     *
     * @ingroup php_API
     * @since docker-finance 1.0.0
     */
    function main(int $argc, array $argv): void
    {
        if (!$argc || empty($argv[1]) || empty($argv[2])) {
            utils\CLI::throw_fatal('valid argument needed for interal API');
        }

        //
        // Set environment "map"
        //

        $env = new utils\Env();

        // Fetch (API)
        $env->set_env('API_VERSION', getenv('DOCKER_FINANCE_VERSION'));  // API version
        $env->set_env('API_FETCH_TYPE', strtolower($argv[1]));      // API type
        $env->set_env('API_FETCH_SUBTYPE', strtolower($argv[2]));   // API subtype
        $env->set_env('API_FETCH_YEAR', getenv('API_FETCH_YEAR'));  // API year (to fetch)
        $env->set_env('API_OUT_DIR', getenv('API_OUT_DIR') . '/');  // API output path (account's 1-in dir)

        // Prices
        $env->set_env('API_PRICES_PATH', getenv('API_PRICES_PATH'));  // Master price file
        $env->set_env('API_PRICES_API', getenv('API_PRICES_API'));    // Price API to use
        $env->set_env('API_PRICES_KEY', getenv('API_PRICES_KEY'));    // Price API key
        $env->set_env('API_PRICES_ASSETS', getenv('API_PRICES_ASSETS'));  // User-provided assets

        // Exchanges
        $env->set_env('API_KEY', getenv('API_KEY'));                // Exchange or blockchain scanner API key
        $env->set_env('API_PASSPHRASE', getenv('API_PASSPHRASE'));  // Exchange passphrase
        $env->set_env('API_SECRET', getenv('API_SECRET'));          // Exchange secret

        // Exchanges/Blockchains
        $env->set_env('API_SUBACCOUNT', getenv('API_SUBACCOUNT'));  // Subaccount(s)

        //
        // Create fetch object
        //

        $type = $env->get_env('API_FETCH_TYPE');
        $subtype = $env->get_env('API_FETCH_SUBTYPE');

        switch ($type) {
            case 'account':
                switch ($subtype) {
                    case 'coinbase':
                    case 'gemini':
                        $api = new exchanges\Fetch($env);
                        break;
                    case 'coinbase-wallet':
                    case 'coinomi':
                    case 'ledger':
                    case 'metamask':
                    case 'pera-wallet':
                        $api = new blockchains\Fetch($env);
                        break;
                    default:
                        utils\CLI::throw_fatal("unsupported subtype '$subtype' for interal API");
                        break;
                }
                break;
            case 'prices':
                switch ($subtype) {
                    case 'crypto':
                        // TODO: `case 'legacy'` (stocks and bonds and ETFs, oh my!)
                        $api = new prices\Fetch($env);
                        break;
                    default:
                        utils\CLI::throw_fatal("unsupported subtype '$subtype' for interal API");
                        break;
                }
                break;
            default:
                utils\CLI::throw_fatal("unsupported type '$type' for interal API");
                break;
        }

        //
        // Execute
        //

        assert(isset($api));
        $api->fetch();
    }
}  // namespace dfi

namespace {
    try {
        dfi\main($argc, $argv);
    } catch (Throwable $e) {
        dfi\utils\CLI::print_fatal(
            "\n\t" . $e->getMessage() . "\n\t" .
            $e->getFile() . ':' . $e->getLine()
        );
        exit(1);
    }
}  // namespace

# vim: sw=4 sts=4 si ai et
