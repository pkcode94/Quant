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

namespace dfi\utils
{
    /**
     * @brief Environment mapper
     * @details From CLI<->PHP
     * @ingroup php_utils
     * @since docker-finance 1.0.0
     */
    class Env
    {
        // @phpstan-ignore-next-line
        private $env = [];  //!< Environment "map"

        //! @brief Set environment (from shell environment)
        public function set_env(string $key, string $value): void
        {
            $this->env[$key] = $value;
        }

        //! @return String value of given environment key
        public function get_env(string $key): string
        {
            if (!array_key_exists($key, $this->env)) {
                CLI::throw_fatal("invalid environment key");
            }

            return $this->env[$key];
        }
    }

    /**
     * @brief CLI logger
     * @ingroup php_utils
     * @since docker-finance 1.0.0
     */
    class CLI
    {
        public static function print_custom(string $message): void
        {
            fwrite(STDOUT, $message);
        }

        public static function print_normal(string $message): void
        {
            fwrite(STDOUT, "\e[0m$message\e[0m\n");
        }

        public static function print_info(string $message): void
        {
            fwrite(STDOUT, "\e[32m[INFO] $message\e[0m\n");
        }

        public static function print_warning(string $message): void
        {
            fwrite(STDOUT, "\e[33;1m[WARN] $message\e[0m\n");
        }

        public static function print_error(string $message): void
        {
            fwrite(STDERR, "\e[31;1m[ERROR] $message\e[0m\n");
        }

        public static function print_debug(mixed $message): void
        {
            if (preg_match('/^1$|^2$/', getenv('DOCKER_FINANCE_DEBUG'))) {
                $bt = debug_backtrace();
                fwrite(STDERR, "\e[33m[DEBUG] {$bt[1]['file']}:{$bt[1]['line']} -> {$bt[1]['function']} -> ");

                if (is_string($message)) {
                    fwrite(STDERR, "$message \e[0m\n");
                } else {
                    print_r($message);
                    fwrite(STDERR, "\e[0m\n");
                }
            }
        }

        public static function print_fatal(string $message): void
        {
            fwrite(STDERR, "\e[41;1m[FATAL] $message\e[0m\n");
        }

        // TODO: indicate to caller(s) that program will exit
        public static function throw_fatal(string $message): void
        {
            throw new \Exception("\e[41;1m[FATAL] $message\e[0m\n");
        }
    }

    /**
     * @brief JSON handler
     * @ingroup php_utils
     * @since docker-finance 1.0.0
     */
    class Json
    {
        /**
         * @brief Writes JSON data to CSV file
         * @param string $data JSON encoded data
         * @param string $file CSV file (path) to write
         */
        public static function write_csv(string $data, string $file): void
        {
            $tmpfile = tmpfile();
            if (!$tmpfile) {
                CLI::throw_fatal("could not create JSON tmpfile");
            }

            $tmp = stream_get_meta_data($tmpfile)['uri'];
            file_put_contents($tmp, $data, LOCK_EX);

            // Write CSV file
            try {
                $json = new \OzdemirBurak\JsonCsv\File\Json($tmp);
                $json->setConversionKey('utf8_encoding', 'true');
                $json->convertAndSave($file);
                chmod($file, 0600);
            } catch (\Throwable $e) {
                CLI::print_fatal($e->getMessage());
                fclose($tmpfile);
            }

            fclose($tmpfile);
        }
    }
}  // namespace dfi\utils\error

# vim: sw=4 sts=4 si ai et
