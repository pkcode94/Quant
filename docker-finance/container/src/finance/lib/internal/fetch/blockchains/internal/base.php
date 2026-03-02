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

namespace dfi\blockchains\internal
{
    require_once(__DFI_PHP_ROOT__ . 'utils/utils.php');

    use dfi\utils as utils;

    /**
     * @brief Metadata object to be used for all blockchain APIs
     * @details Data object for shell-based/end-user environment
     *   and internal implementation.
     * @ingroup php_API_impl
     * @since docker-finance 1.0.0
     */
    class Metadata
    {
        private string $blockchain;
        private string $contract_type;
        private string $account;
        private string $subaccount;
        private string $address;
        private string $url;
        private string $out_dir;
        private string $api_key;
        private string $year;

        public function set_blockchain(string $blockchain): void
        {
            $this->blockchain = $blockchain;
        }
        public function get_blockchain(): string
        {
            return $this->blockchain;
        }

        public function set_contract_type(string $contract_type): void
        {
            $this->contract_type = $contract_type;
        }
        public function get_contract_type(): string
        {
            return $this->contract_type;
        }

        public function set_account(string $account): void
        {
            $this->account = $account;
        }
        public function get_account(): string
        {
            return $this->account;
        }

        public function set_subaccount(string $subaccount): void
        {
            $this->subaccount = $subaccount;
        }
        public function get_subaccount(): string
        {
            return $this->subaccount;
        }

        public function set_address(string $address): void
        {
            $this->address = $address;
        }
        public function get_address(): string
        {
            return $this->address;
        }

        public function set_url(string $url): void
        {
            $this->url = $url;
        }
        public function get_url(): string
        {
            return $this->url;
        }

        public function set_out_dir(string $out_dir): void
        {
            $this->out_dir = $out_dir;
        }
        public function get_out_dir(): string
        {
            return $this->out_dir;
        }

        public function set_api_key(string $api_key): void
        {
            $this->api_key = $api_key;
        }
        public function get_api_key(): string
        {
            return $this->api_key;
        }

        public function set_year(string $year): void
        {
            $this->year = $year;
        }
        public function get_year(): string
        {
            return $this->year;
        }
    }

    //! @since docker-finance 1.0.0
    interface FetcherInterface
    {
        /**
         * @brief REST API requester
         * @details Handler for underlying request
         */
        public function request(string $url): mixed;
    }

    //! @since docker-finance 1.0.0
    interface ReaderInterface
    {
        //! @brief Response handler
        public function get_response(Metadata $metadata): mixed;

        /**
         * @brief Response parser
         * @details Parses and prepares REST API response
         * @param array<mixed> $response
         * @return array<mixed>
         */
        public function parse_response(Metadata $metadata, array $response): array;
    }

    //! @since docker-finance 1.0.0
    interface WriterInterface
    {
        /**
         * @brief File writer
         * @param array<mixed> $data
         */
        public function write_response(Metadata $metadata, array $data): void;

        //! @brief Transactions handler
        public function write_transactions(Metadata $metadata): void;
    }

    //! @since docker-finance 1.0.0
    class Fetcher implements FetcherInterface
    {
        public function request(string $url): mixed
        {
            $headers = array(
              'Accept: application/json',
              'Content-Type: application/json',
            );

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

            return $response;
        }
    }

    // NOTE: PHP doesn't support multiple inheritance....

    //! @since docker-finance 1.0.0
    abstract class Reader extends Fetcher implements ReaderInterface
    {
        //! @brief Wrapper for fetching response
        abstract public function get_response(Metadata $metadata): mixed;

        //! @brief Parse and prepare the REST API response
        abstract public function parse_response(Metadata $metadata, array $response): array;
    }

    //! @since docker-finance 1.0.0
    abstract class Writer extends Reader implements WriterInterface
    {
        //! @todo This is kind of a hack? because custom dir, this must be done here instead of getenv
        public function write_response(Metadata $metadata, array $response): void
        {
            $out_dir = $metadata->get_out_dir();
            if (!file_exists($out_dir)) {
                if (!mkdir($out_dir, 0700, true)) {
                    utils\CLI::throw_fatal("unable to create $out_dir");
                }
            }

            $file = $out_dir . '/' . $metadata->get_address();

            // If ethereum-based contract type is needed, append contract type.
            // TODO: WARNING: if adding non-ethereum-based chain, update as needed.
            if ($metadata->get_blockchain() != 'algorand' && $metadata->get_blockchain() != 'tezos') {
                $file = $out_dir . '/' . $metadata->get_address() . '-' . $metadata->get_contract_type();
            }

            file_put_contents($file . '.csv', $response, LOCK_EX);
        }

        abstract public function write_transactions(Metadata $metadata): void;
    }

    /**
     * @brief Base API
     * @since docker-finance 1.0.0
     */
    abstract class API extends Writer
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

        abstract public function fetch(Metadata $metadata): void;
    }
}  // namespace dfi\blockchains\internal

//! @since docker-finance 1.0.0

namespace dfi\blockchains
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

        /**
         * @brief Fetch executor
         *
         * @note Signature is hack to interoperate with both external/internal
         *   API (because PHP doesn't have function overloading in the OOP sense).
         *
         * @warning Implementation *should* param check for null.
         */
        abstract public function fetch(internal\Metadata|null $metadata = null): void;
    }
}  // namespace dfi\blockchains

# vim: sw=4 sts=4 si ai et
