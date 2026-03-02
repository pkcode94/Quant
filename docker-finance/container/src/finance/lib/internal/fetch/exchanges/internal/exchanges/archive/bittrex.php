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

namespace dfi\exchanges\internal\exchanges\bittrex
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
        private string $date_tag;  //!< URL tag

        public const BASE_URL = 'https://api.bittrex.com/v3/';  //!< Base API URL

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $year = $this->get_env()->get_env('API_FETCH_YEAR');
            $this->date_tag = 'startDate=' . $year . '-01-01T00:00:00Z' . '&endDate=' . $year . '-12-31T23:59:59Z';
        }

        protected function get_date_tag(): string
        {
            return $this->date_tag;
        }
    }

    /**
     * @brief Common implementation
     * @details Implements Bittrex V3
     * @note V1 is deprecated
     * @since docker-finance 1.0.0
     * @todo balance assertions? 'https://api.bittrex.com/v3/balances';
     * @todo pagination
     */
    abstract class Bittrex extends Impl
    {
        public function __construct(utils\Env $env)
        {
            parent::__construct($env);
        }

        //! @brief Implements underlying API request
        protected function request(string $url): mixed
        {
            $timestamp = time() * 1000;
            $method = 'GET';
            $content = '';
            $content_hash = hash('sha512', $content);
            $subaccount = '';
            $pre_sign = $timestamp . $url . $method . $content_hash . $subaccount;
            $signature = hash_hmac(
                'sha512',
                $pre_sign,
                $this->get_env()->get_env('API_SECRET')
            );

            $headers = array(
              'Accept: application/json',
              'Content-Type: application/json',
              'Api-Key: ' . $this->get_env()->get_env('API_KEY') . '',
              'Api-Signature: ' . $signature . '',
              'Api-Timestamp: ' . $timestamp . '',
              'Api-Content-Hash: ' . $content_hash . ''
            );

            $ch = curl_init($url);

            curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_HEADER, false);

            $response = curl_exec($ch);
            curl_close($ch);

            if ($response === false) {
                utils\CLI::throw_fatal("cURL null response");
            }

            return $response;
        }
    }

    /**
     * @brief Bittrex Trades
     * @since docker-finance 1.0.0
     */
    final class Trades extends Bittrex
    {
        private string $url;
        private string $file;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $this->url = self::BASE_URL . 'orders/closed?' . $this->get_date_tag();
            $this->file = $this->get_env()->get_env('API_OUT_DIR') . 'trades.csv';
        }

        /**
         * @brief Implements read handler
         * @return array<string> Trades, prepared for writing
         */
        public function read(): array
        {
            $stack = $this->request($this->url);
            if ($stack == '[]') {
                return [];
            }
            return $stack;
        }

        //! @brief Implements write handler
        public function write(mixed $stack, string $file): void
        {
            $this->raw_to_csv($stack, $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Trades");
            $this->write($this->read(), $this->file);
        }
    }

    /**
     * @brief Bittrex Deposits
     * @since docker-finance 1.0.0
     */
    final class Deposits extends Bittrex
    {
        private string $url;
        private string $file;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $this->url = self::BASE_URL . 'deposits/closed?' . $this->get_date_tag();
            $this->file = $this->get_env()->get_env('API_OUT_DIR') . 'deposits.csv';
        }

        /**
         * @brief Implements read handler
         * @return array<string> Deposits, prepared for writing
         */
        public function read(): array
        {
            $stack = $this->request($this->url);
            if ($stack == '[]') {
                return [];
            }
            return $stack;
        }

        //! @brief Implements write handler
        public function write(mixed $stack, string $file): void
        {
            $this->raw_to_csv($stack, $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Deposits");
            $this->write($this->read(), $this->file);
        }
    }

    /**
     * @brief Bittrex Withdrawals
     * @since docker-finance 1.0.0
     */
    final class Withdrawals extends Bittrex
    {
        private string $url;
        private string $file;

        public function __construct(utils\Env $env)
        {
            parent::__construct($env);

            $this->url = self::BASE_URL . 'withdrawals/closed?' . $this->get_date_tag();
            $this->file = $this->get_env()->get_env('API_OUT_DIR') . 'withdrawals.csv';
        }

        /**
         * @brief Implements read handler
         * @return array<string> Withdrawals, prepared for writing
         */
        public function read(): array
        {
            $stack = $this->request($this->url);
            if ($stack == '[]') {
                return [];
            }
            return $stack;
        }

        //! @brief Implements write handler
        public function write(mixed $stack, string $file): void
        {
            $this->raw_to_csv($stack, $file);
        }

        //! @brief Implements fetch handler
        public function fetch(): void
        {
            utils\CLI::print_normal("  ─ Withdrawals");
            $this->write($this->read(), $this->file);
        }
    }
}  // namespace dfi\exchanges\internal\exchanges\bittrex

//! @since docker-finance 1.0.0

namespace dfi\exchanges\internal\exchanges
{
    require_once(__DFI_PHP_ROOT__ . 'exchanges/internal/base.php');
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    /**
     * @brief Facade for Bittrex implementation
     * @ingroup php_exchanges
     * @since docker-finance 1.0.0
     */
    final class Bittrex extends \dfi\exchanges\API
    {
        private bittrex\Trades $trades;
        private bittrex\Deposits $deposits;
        private bittrex\Withdrawals $withdrawals;

        public function __construct(\dfi\utils\Env $env)
        {
            $this->trades = new bittrex\Trades($env);
            $this->deposits = new bittrex\Deposits($env);
            $this->withdrawals = new bittrex\Withdrawals($env);
        }

        public function fetch(): void
        {
            $this->trades->fetch();
            $this->deposits->fetch();
            $this->withdrawals->fetch();
        }
    }
}  // namespace dfi\exchanges\internal\exchanges

# vim: sw=4 sts=4 si ai et
