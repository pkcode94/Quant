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

namespace dfi\exchanges\internal\exchanges\celsius
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Common implementaion
     * @since docker-finance 1.0.0
     */
    abstract class Impl extends \dfi\exchanges\internal\Impl
    {
        public const BASE_URL = 'https://wallet-api.celsius.network/wallet/transactions';  //!< Base API URL

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }
    }

    /**
     * @brief Common implementation
     * @since docker-finance 1.0.0
     */
    abstract class Celsius extends Impl
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        //! @brief Implements underlying API request
        protected function request(string $url): mixed
        {
            $header = array(
              'User-Agent: curl/7.81.0',  // Needs a user-agent. Requirements not given but this works.
              'Accept: */*',
              'X-Cel-Partner-Token: ' . $this->get_env()->get_env('API_SECRET'),
              'X-Cel-Api-Key: ' . $this->get_env()->get_env('API_KEY'),
            );

            $ch = curl_init();

            curl_setopt($ch, CURLOPT_URL, $url);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'GET');
            curl_setopt($ch, CURLOPT_HTTPHEADER, $header);

            $response = curl_exec($ch);
            curl_close($ch);

            if ($response === false) {
                utils\CLI::throw_fatal("cURL null response");
            }

            return json_decode($response, true, 512, JSON_PRETTY_PRINT);
        }
    }

    /**
     * @brief Celsius Transactions
     * @since docker-finance 1.0.0
     */
    final class Transactions extends Celsius
    {
        private string $file;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $this->file = $this->get_env()->get_env('API_OUT_DIR') . 'celsius.csv';
        }

        /**
         * @brief Get transactions
         * @phpstan-ignore-next-line // TODO: resolve
         */
        private function get_txs(string $url): array
        {
            $stack = [];

            // Push first response. Will provide us with pagination details
            $response = $this->request($url);
            array_push($stack, $response);

            // Process paginated
            if ($response['pagination']['pages'] > 1) {
                for ($i = 2; $i < $response['pagination']['pages'] + 1; $i++) {
                    $next = self::BASE_URL . "?page={$i}&per_page=100";
                    $response = $this->request($next);
                    array_push($stack, $response);
                }
            }

            return $stack;
        }

        /**
         * @brief Parse transactions
         * @phpstan-ignore-next-line // TODO: resolve
         */
        private function parse_txs(array $response): array
        {
            $stack = [];

            for ($i = 0; $i < count($response); $i++) {
                for ($j = 0; $j < count($response[$i]['record']); $j++) {
                    array_push($stack, $response[$i]['record'][$j]);
                }
            }

            return $stack;
        }

        /**
         * @brief Implements read handler
         * @return array<string> Transactions, prepared for writing
         */
        public function read(): array
        {
            $url = self::BASE_URL . '?page=1&per_page=100';
            return $this->parse_txs($this->get_txs($url));
        }

        //! @brief Implements write handler
        public function write(mixed $stack, string $file): void
        {
            $this->raw_to_csv($stack, $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Transactions");
            $this->write($this->read(), $this->file);
        }
    }
}  // namespace dfi\exchanges\internal\exchanges\celsius

//! @since docker-finance 1.0.0

namespace dfi\exchanges\internal\exchanges
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    /**
     * @brief Facade for Celsius implementation
     * @ingroup php_exchanges
     * @since docker-finance 1.0.0
     */
    final class Celsius extends \dfi\exchanges\API
    {
        private celsius\Transactions $transactions;

        public function __construct(\dfi\utils\Env $env)
        {
            $this->transactions = new celsius\Transactions($env);
        }

        public function fetch(): void
        {
            $this->transactions->fetch();
        }
    }
}  // namespace dfi\exchanges\internal\exchanges

# vim: sw=4 sts=4 si ai et
