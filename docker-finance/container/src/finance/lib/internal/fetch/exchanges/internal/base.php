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

namespace dfi\exchanges\internal
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
         * @brief The read handler
         * @details Requests and receives data from exchange
         * @return array<mixed> Prepared data for writing
         */
        public function read(): array;

        /**
         * @brief The write handler
         * @details Write parsed and prepared data to file
         * @param mixed $stack Prepared data for CSV
         * @param string $file Full path to file
         * @todo Enforce array for stack
         */
        public function write(mixed $stack, string $file): void;

        /**
         * @brief The fetch handler
         * @details Calls read and write of exchange data
         */
        public function fetch(): void;
    }

    /**
     * @brief Base Implementation
     * @since docker-finance 1.0.0
     */
    abstract class Impl implements ImplInterface
    {
        private utils\Env $env;  //!< Environment
        // TODO: private readonly string $base_path;

        public function __construct(utils\Env $env)
        {
            $this->env = $env;
            // TODO: $this->base_path = $this->env->get_env('API_OUT_DIR');
        }

        protected function get_env(): utils\Env
        {
            return $this->env;
        }

        // TODO: protected function get_path() : string {return $this->base_path;}

        /**
         * @brief Parse symbols used to fetch trades (e.g., btcusd, btceur, etc.)
         * @details
         *
         * Expected format:
         *
         *   Fetch all trades with these symbols
         *   1. subaccount_name/{BTC,ETH,USD,...}
         *
         *   Fetch only trades with these pairs
         *   2. subaccount_name/{btcusd,btceth,gusdusd,...}
         *
         *   Fetch a mix of trades with symbols and pairs
         *   3. subaccount_name/{BTC,gusdusd,...}
         *
         * @return array<string>
         */
        protected function parse_subaccount_symbols(string $subaccount): array
        {
            if (str_contains($subaccount, '/')) {
                return explode(',', preg_replace('/{|}/', '', explode('/', $subaccount)[1]));
            }
            return [];
        }

        /**
         * @brief Write unencoded data to CSV
         * @param array<string> $data Data to write
         * @param string $file File to write to
         */
        protected function raw_to_csv(array $data, string $file): void
        {
            if (!empty($data)) {
                \dfi\utils\Json::write_csv(
                    json_encode($data, JSON_PRETTY_PRINT),
                    $file
                );
            }
        }

        //! @brief Wrapper to underlying API request
        abstract protected function request(string $path): mixed;
    }
}  // namespace dfi\exchanges\internal

//! @since docker-finance 1.0.0

namespace dfi\exchanges
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

        //! @brief Fetch executor
        abstract public function fetch(): void;
    }
}  // namespace dfi\exchanges

# vim: sw=4 sts=4 si ai et
