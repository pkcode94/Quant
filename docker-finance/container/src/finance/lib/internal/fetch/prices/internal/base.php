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

namespace dfi\prices\internal
{
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Common implementaion interface
     * @details Unified with top-level API interace
     * @since docker-finance 1.0.0
     */
    interface ImplInterface
    {
        /**
         * @brief Get network response data
         * @param array<string> $asset Parsed environment entries of assets
         * @param string $timestamp Given year(s) to fetch
         * @return array<int<0, max>, array<string>> Entries of [N][timestamp, price] since given timestamp
         */
        public function getter(array $asset, string $timestamp): array;

        /**
         * @brief Prepare price data for given assets
         * @param string $assets Unparsed envrionment assets (e.g., 'id/ticker,id/ticker,blockchain:id/ticker,...')
         * @return array<int<0, max>, array<string>> Prices for all given assets
         */
        public function reader(string $assets): array;

        /**
         * @brief Write price data to master prices journal
         * @param array<array<string>> $data Prepared data for master prices journal
         * @param string $path Full path to journal
         * @note External implementation *MUST* append and sort to every applicable year
         * @warning Clobbers external master price journal
         * @todo Enforce array for stack
         */
        public function writer(mixed $data, string $path): void;

        /**
         * @brief Caller for entire fetch process
         * @details Handler for calling getter, reader and writer
         */
        public function fetcher(): void;
    }

    /**
     * @brief Common implementation
     * @since docker-finance 1.0.0
     */
    abstract class Impl implements ImplInterface
    {
        private utils\Env $env;  //!< Environment

        public function __construct(utils\Env $env)
        {
            $this->env = $env;
        }

        protected function get_env(): utils\Env
        {
            return $this->env;
        }

        /**
         * @brief Parse environment assets
         * @param string $assets Expected format: 'id/ticker,id/ticker,blockchain:id/ticker,...' e.g., 'bitcoin/BTC,ethereum/ETH,avalanche:0xb97ef9ef8734c71904d8002f8b6bc66dd9c48a6e/USDC,...'
         * @details
         *
         *   Caveats:
         *
         *   1. There are multiple reasons why asset must be passed with ticker
         *      instead of ticker alone:
         *
         *      a. CoinGecko uses same ticker for multiple asset ID's:
         *
         *         ltc = binance-peg-litecoin
         *         ltc = litecoin
         *         eth = ethereum
         *         eth = ethereum-wormhole
         *
         *      b. Mobula will often require blockchain along with asset ID
         *         NOTE: with Mobula, ID can also consist of a contract address
         *
         *   2. Ticker comes *AFTER* ID because hledger's prices are:
         *
         *      a. *CASE SENSITIVE*
         *
         *      b. Will not understand the difference between (for example):
         *         aGUSD and AGUSD (CoinGecko will return lowercase ticker)
         *
         * @return array<array<string>> Parsed environment entries
         */
        protected function parse_assets(string $assets): array
        {
            utils\CLI::print_debug($assets);

            if (!str_contains($this->get_env()->get_env('API_PRICES_ASSETS'), '/')) {
                utils\CLI::throw_fatal("malformed assets format");
            }

            $parsed = [];
            $csv = explode(',', $assets);

            for ($i = 0; $i < count($csv); $i++) {
                list($asset, $ticker) = explode('/', $csv[$i]);

                $id = $asset;
                $blockchain = "";
                if (str_contains($id, ':')) {
                    list($blockchain, $id) = explode(':', $asset);
                }

                $parsed[$i]['id'] = $id;
                $parsed[$i]['ticker'] = $ticker;
                $parsed[$i]['blockchain'] = $blockchain;
            }

            return $parsed;
        }

        /**
         * @brief Request's common implementation
         * @param string $url REST API URL
         * @param array<string> $header Impl-specific header addendum
         * @return mixed Response data
         */
        protected function request_impl(string $url, array $header): mixed
        {
            $headers = array(
              'User-Agent: docker-finance /' . $this->get_env()->get_env('API_VERSION'),
              'Accept: application/json',
              'Content-Type: application/json',
            );

            if (!empty($header)) {
                $headers = array_merge($headers, $header);
            }
            utils\CLI::print_debug($headers);

            $ch = curl_init($url);

            curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_HEADER, false);

            $response = curl_exec($ch);

            $info = curl_getinfo($ch);
            utils\CLI::print_debug($info);

            curl_close($ch);

            if ($response === false) {
                utils\CLI::throw_fatal("cURL null response");
            }

            $decoded = json_decode(
                $response,
                true,
                512,
                JSON_PRETTY_PRINT
            );
            utils\CLI::print_debug($decoded);

            return $decoded;
        }

        /**
         * @brief Impl-specific REST API request generator
         * @param array<mixed> $asset Parsed environment asset entries to request
         * @param string $timestamp Timestamp of entries to request
         * @return mixed REST API raw response price data
         */
        abstract protected function request(array $asset, string $timestamp): mixed;

        /**
         * @brief Make impl-specific timestamp requirement
         * @param string $year Given year
         */
        abstract protected function make_timestamp(string $year): mixed;

        /**
         * @brief Make daily average of given prices
         * @param array<mixed> $prices Parsed [date => price] entries
         * @return array<string> Daily averages in date and price format (one per line)
         */
        private function make_average(array $prices): array
        {
            $total = 0;
            $prev_date = "";
            $date_counter = 0;

            $stack = [];

            foreach ($prices as $date => $price) {

                // Either a kick-off date or a single daily average (per their API)
                // NOTE: this *MUST* be overwritten below if computing non-daily
                if ($prev_date == "") {
                    $stack[$date] = $price;
                }

                // Average will be based on received dates
                if ($prev_date == $date) {
                    $total += $price;
                    $date_counter++;
                } else {
                    // Aggregator provided a single date, treat as the daily average
                    if (!$total) {
                        $total = $price;
                    }

                    // Avoid divide by zero (if only a single date was provided)
                    if (!$date_counter) {
                        $date_counter = 1;
                    }

                    // Finished previous date, calculate and push to stack
                    $average = $total / $date_counter;

                    // Always overwrite with most recent daily average
                    if ($date_counter > 1) {
                        $stack[$prev_date] = $average;
                    } else {
                        $stack[$date] = $average;
                    }

                    $total = 0;
                    $date_counter = 0;
                }

                $prev_date = $date;
            }

            utils\CLI::print_debug($stack);
            return $stack;
        }

        /**
         * @brief Parse fetched date and prices
         * @param array<mixed> $prices Fetched prices [N][timestamp, price]
         * @details
         *   $prices Expectation:
         *
         *     array[0] = oldest entry<br>
         *     array[0][0] = timestamp<br>
         *     array[0][1] = price<br>
         *
         *     array[1] = next hour<br>
         *     array[1][0] = timestamp<br>
         *     array[1][1] = price<br>
         *
         *   ...etc.
         * @return array<string> Date and price single-line entries without ID or ticker
         */
        abstract protected function parse_prices(array $prices): array;

        /**
         * @brief Create data for master price journal file
         * @param string $ticker Given ticker associated with ID
         * @param array<mixed> $prices Array of [N][timestamp, price] for given year(s)
         * @return array<string> Master price journal file data
         */
        protected function make_master(string $ticker, array $prices): array
        {
            $stack = [];  // Final journal entries
            $average = 0;  // Purely for printing

            # Format non-alpha character currencies for hledger (e.g., "1INCH", "USDC.e", etc.)
            if (!ctype_alpha($ticker)) {
                $ticker = "\"".$ticker."\"";
            }

            foreach ($prices as $date => $price) {

                // Price journal entry line
                $line = 'P ' . $date . ' ' . $ticker . ' ' . sprintf('%.8f', $price) . "\n";
                array_push($stack, $line);

                // Always push a placeholder $/USD for hledger calculations.
                // This is so there aren't separate output lines from the
                // `--value` calculated and current $/USD holdings.
                $line = 'P ' . $date . ' ' . '$' . ' ' . '1' . "\n";
                array_push($stack, $line);

                $line = 'P ' . $date . ' ' . 'USD' . ' ' . '1' . "\n";
                array_push($stack, $line);

                //
                // HACKS to get USD amount of unsupported upstream coins
                //

                if ($ticker == 'AAVE') {
                    $line = 'P ' . $date . ' ' . 'stkAAVE' . ' ' . sprintf('%.8f', $price) . "\n";
                    array_push($stack, $line);
                }

                // Hack for array('paxos-standard'=>'USDP')
                if ($ticker == 'PAX') {
                    $line = 'P ' . $date . ' ' . 'USDP' . ' ' . sprintf('%.8f', $price) . "\n";
                    array_push($stack, $line);
                }

                // CGLD was changed to CELO at some point
                if ($ticker == 'CGLD') {
                    $line = 'P ' . $date . ' ' . 'CELO' . ' ' . sprintf('%.8f', $price) . "\n";
                    array_push($stack, $line);
                }

                // Clobber into most-recent daily average of given year
                $average = $price;
            }

            // Print ticker and most recent price parsed
            utils\CLI::print_custom("    \e[32m│\e[0m\n");
            utils\CLI::print_custom("    \e[32m├─\e[34m\e[1;3m {$ticker}\e[0m\n");
            utils\CLI::print_custom("    \e[32m│  └─\e[37;2m " . $average . "\e[0m\n");

            return $stack;
        }

        public function getter(array $asset, string $timestamp): array
        {
            $response = [];
            $timer = 60;  // seconds
            $success = false;

            while (!$success) {
                try {
                    $response = $this->request($asset, $timestamp);
                    $success = true;  // Should throw before this is assigned, alla C++
                } catch (\Throwable $e) {
                    $code = $e->getCode();
                    $message = $e->getMessage();

                    $print = "server sent error '{$message}' with code '{$code}' for '{$asset['id']}'"
                        . " - retrying in '{$timer}' seconds";

                    switch ($code) {
                        // CoinGecko's unrecoverable error (paid plan needed)
                        case 10012:
                            utils\CLI::throw_fatal($print);
                            break;
                        default:
                            utils\CLI::print_warning($print);
                            break;
                    }

                    $i = 1;
                    $j = 1;
                    while ($i <= $timer) {
                        if ($j == 10) {
                            utils\CLI::print_custom("\e[33;1m+\e[0m");
                            $j = 0;
                        } else {
                            utils\CLI::print_custom("\e[33m.\e[0m");
                        }
                        sleep(1);
                        $i++;
                        $j++;
                    }
                    utils\CLI::print_custom("\n");
                }
            }

            utils\CLI::print_debug($response);
            return $response;
        }

        public function reader(string $assets): array
        {
            // Prepared price journal for given assets
            $stack = [];

            // Timestamp based on given year
            $timestamp = $this->make_timestamp($this->get_env()->get_env('API_FETCH_YEAR'));

            utils\CLI::print_normal("  ─ Assets");

            foreach ($this->parse_assets($assets) as $asset) {
                $parsed = $this->parse_prices($this->getter($asset, $timestamp));
                $master = $this->make_master($asset['ticker'], $this->make_average($parsed));

                utils\CLI::print_debug($master);
                array_push($stack, $master);
            }

            utils\CLI::print_custom("    \e[32m│\e[0m\n");

            return $stack;
        }

        public function writer(mixed $data, string $path): void
        {
            // Cohesive array of all fetched assets
            $stack = [];

            // Each element is an array of ticker prices
            foreach ($data as $ticker) {
                foreach ($ticker as $price) {
                    array_push($stack, $price);
                }
            }

            file_put_contents($path, array_unique($stack), LOCK_EX);
        }

        public function fetcher(): void
        {
            $master = $this->reader($this->get_env()->get_env('API_PRICES_ASSETS'));
            $this->writer($master, $this->get_env()->get_env('API_PRICES_PATH'));
        }
    }
}  // namespace dfi\prices\internal

//! @since docker-finance 1.0.0

namespace dfi\prices
{
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Top-level abstraction for API
     * @since docker-finance 1.0.0
     */
    abstract class API
    {
        private utils\Env $env;  //!< Environment

        public function __construct(utils\Env $env)
        {
            $this->env = $env;
        }

        protected function get_env(): utils\Env
        {
            return $this->env;
        }

        abstract public function fetch(): void;
    }
}  // namespace dfi\prices

# vim: sw=4 sts=4 si ai et
