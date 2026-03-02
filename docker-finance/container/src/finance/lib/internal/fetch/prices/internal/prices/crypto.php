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

namespace dfi\prices\internal\prices\crypto
{
    require_once(__DFI_PHP_ROOT__ . 'prices/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\prices\internal as internal;
    use dfi\utils as utils;

    /**
     * @brief CoinGecko price aggregator API
     * @since docker-finance 1.0.0
     */
    final class CoinGecko extends internal\Impl
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        protected function request(array $asset, string $timestamp): mixed
        {
            // If `key` exists, use Pro API (otherwise, use Public API)
            $key = $this->get_env()->get_env('API_PRICES_KEY');
            $domain = 'api.coingecko.com';
            $header = [];
            if ($key != 'null') {
                $domain = 'pro-' . $domain;
                $header = ["x-cg-pro-api-key: $key"];
            }
            $vs_currency = 'usd';
            $url = "https://{$domain}/api/v3/coins/{$asset['id']}/market_chart?vs_currency={$vs_currency}&days={$timestamp}";

            $response = $this->request_impl($url, $header);
            if (array_key_exists('error', $response)) {
                $status = $response['error']['status'];
                throw new \Exception($status['error_message'], $status['error_code']);
            }
            if (array_key_exists('status', $response)) {
                $status = $response['status'];
                throw new \Exception($status['error_message'], $status['error_code']);
            }

            $prices = $response['prices'];
            utils\CLI::print_debug($prices);

            return $prices;
        }

        /**
         * @details
         *   Parses historical market data include price, market cap
         *   and 24h volume (granularity auto)
         */
        protected function parse_prices(array $prices): array
        {
            $total = 0;
            $prev_date = "";
            $date_counter = 0;

            $stack = [];

            for ($i = 0; $i < count($prices); $i++) {

                $timestamp = $prices[$i][0] / 1000;
                $date = date('Y/m/d', $timestamp);

                // Isolate given year.
                //
                // NOTE:
                //
                //   If 'all', then all years are needed. Otherwise, for example,
                //   if the given year is last year (and the beginning of last
                //   year was 375 days ago), then upstream will send 375 entries
                //   (but only the first 365 will be needed).

                $given_year = $this->get_env()->get_env('API_FETCH_YEAR');
                if ($given_year != 'all' && !preg_match('/^'.$given_year.'\//', $date)) {
                    utils\CLI::print_debug("skipping $date");
                    continue;
                }
                $price = $prices[$i][1];

                // Push to stack
                //
                // NOTE:
                //
                //   - Data granularity is automatic (cannot be adjusted)
                //   - 1 day from current time = 5 minute interval data
                //   - 1 through 90 days from current time = hourly data
                //   - Above 90 days from current time = daily data (00:00 UTC)

                $stack += [$date => $price];
            }

            utils\CLI::print_debug($stack);
            return $stack;
        }

        /**
         * @return mixed Number of days since given year
         */
        protected function make_timestamp(string $year): mixed
        {
            // Number of days back to beginning of given year
            if ($year != 'all') {
                $now = strtotime(date('Y-m-d'));
                utils\CLI::print_debug($now);

                $then = strtotime($this->get_env()->get_env('API_FETCH_YEAR') . '-01-01');
                utils\CLI::print_debug($then);

                $diff = $now - $then;
                utils\CLI::print_debug($diff);

                // NOTE: if `+ 1` is removed, day one of the previous
                // year is cut off (when fetching previous year through current
                // year). But + 1 will also include the last day of the previous
                // year when you only want current year. So, implementation must
                // parse appropriately (it currently does).
                $days = $diff / (60 * 60 * 24) + 1;
                utils\CLI::print_debug($days);

                return $days;
            }

            // All possible dates
            return 'max';
        }
    }

    /**
     * @brief Mobula price aggregator API
     * @since docker-finance 1.0.0
     */
    final class Mobula extends internal\Impl
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        protected function request(array $asset, string $timestamp): mixed
        {
            // If `key` exists, append to header (used in all plans)
            $key = $this->get_env()->get_env('API_PRICES_KEY');
            $domain = 'api.mobula.io';
            $header = [];
            if ($key != 'null') {
                $header = ["Authorization: $key"];
            }

            $url = "https://{$domain}/api/1/market/history?asset={$asset['id']}&from={$timestamp}";
            if (!empty($asset['blockchain'])) {
                $url .= "&blockchain={$asset['blockchain']}";
            }

            $response = $this->request_impl($url, $header);
            if (array_key_exists('error', $response)) {
                throw new \Exception($response['error']);
            }

            $prices = $response['data']['price_history'];
            utils\CLI::print_debug($prices);

            return $prices;
        }

        protected function parse_prices(array $prices): array
        {
            $stack = [];

            for ($i = 0; $i < count($prices); $i++) {

                $timestamp = $prices[$i][0] / 1000;
                $date = date('Y/m/d', $timestamp);

                // Isolate given year
                $given_year = $this->get_env()->get_env('API_FETCH_YEAR');
                if ($given_year != 'all' && !preg_match('/^'.$given_year.'\//', $date)) {
                    utils\CLI::print_debug("skipping $date");
                    continue;
                }

                $price = $prices[$i][1];
                $stack += [$date => $price];
            }

            utils\CLI::print_debug($stack);
            return $stack;
        }

        /**
         * @return mixed Unix timestamp in milliseconds
         */
        protected function make_timestamp(string $year): mixed
        {
            // Number of days back to beginning of given year
            if ($year != 'all') {
                $timestamp = strtotime($this->get_env()->get_env('API_FETCH_YEAR') . '-01-01') * 1000;
                utils\CLI::print_debug($timestamp);
                return $timestamp;
            }

            // From genesis to present
            return "";
        }
    }

}  // namespace dfi\prices\internal\prices\crypto

//! @since docker-finance 1.0.0

namespace dfi\prices\internal\prices
{
    require_once(__DFI_PHP_ROOT__ . 'prices/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Facade for CoinGecko implementation
     * @ingroup php_prices
     * @since docker-finance 1.0.0
     */
    final class CoinGecko extends \dfi\prices\API
    {
        private crypto\CoinGecko $api;  //!< Internal API

        public function __construct(utils\Env $env)
        {
            $this->api = new crypto\CoinGecko($env);
        }

        public function fetch(): void
        {
            $this->api->fetcher();
        }
    }

    /**
     * @brief Facade for Mobula implementation
     * @ingroup php_prices
     * @since docker-finance 1.0.0
     */
    final class Mobula extends \dfi\prices\API
    {
        private crypto\Mobula $api;  //!< Internal API

        public function __construct(utils\Env $env)
        {
            $this->api = new crypto\Mobula($env);
        }

        public function fetch(): void
        {
            $this->api->fetcher();
        }
    }
}  // namespace dfi\prices\internal\prices

# vim: sw=4 sts=4 si ai et
