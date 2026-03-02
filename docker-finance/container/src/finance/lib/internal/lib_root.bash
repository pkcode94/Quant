#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2025-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

#
# "Libraries"
#

[ -z "$DOCKER_FINANCE_CONTAINER_REPO" ] && exit 1
source "${DOCKER_FINANCE_CONTAINER_REPO}/src/finance/lib/internal/lib_utils.bash" || exit 1

#
# Facade
#

function lib_root::root()
{
  lib_root::__parse_root "$@"
  lib_root::__root "$@"
  lib_utils::catch $?
}

#
# Implementation
#

function lib_root::__parse_root()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal

  local -r _arg="$1"
  local -r _usage="
\e[32mDescription:\e[0m

  docker-finance ROOT.cern interpreter

\e[32mUsage:\e[0m

  $ $global_usage [help | [TAB COMPLETION]] [args]

\e[32mArguments:\e[0m

  None: start interactive interpreter w/out running any given macro or plugins

  [TAB COMPLETION]: load (and run) given macro/plugin when starting interpreter

    macros/repo     = macros in repository location
    macros/custom   = WIP (not currently supported)
    plugins/repo    = plugins in repository location
    plugins/custom  = plugins in custom plugin location

  [args]: arguments to macro or plugin

\e[32mExamples:\e[0m

  \e[37;2m# See this usage help\e[0m
  $ $global_usage help

  \e[37;2m# The output of tab completion\e[0m
  $ $global_usage \\\t\\\t
    help                             macros/repo/test/benchmark.C     plugins/custom/example.cc
    macros/repo/crypto/hash.C        macros/repo/test/unit.C          plugins/repo/bitcoin/bitcoin.cc
    macros/repo/crypto/random.C      macros/repo/web/server.C         plugins/repo/example/example.cc

  \e[37;2m# Load and run repo's Web server macro\e[0m
  \e[37;2m# NOTE: only server macros will maintain a running ROOT instance (others will 'one-off')\e[0m
  $ $global_usage macros/repo/web/server.C

  \e[37;2m# Load and run repo's Hash macro with loader argument 'this is a test message'\e[0m
  $ $global_usage macros/repo/crypto/hash.C 'this is a test message'

  \e[37;2m# Load and run unit tests for only Pluggable types, using filter as loader argument\e[0m
  $ $global_usage macros/repo/test/unit.C 'Pluggable*:Macro*:Plugin*'

  \e[37;2m# Load and use repo's example plugin, dispatching with trivial loader and unloader arguments\e[0m
  $ $global_usage plugins/repo/example/example.cc 'int i{123}; i' 'std::cout << --i << std::endl;'
  root [0]
  Processing macro/rootlogon.C...
  ...
  Loading: '${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/example/example.cc'
  Interpreting: 'dfi::plugin::example::example_cc::load(\"int i{123}; i\")'
  ...
  Interpreting: 'int i{123}; i'
  (int) 123
  root [3] dfi::plugin::example::MyExamples my;  // Plugin's MyExamples class is now available to use
  root [4] my.example2()
  ...
  root [5] g_plugin_example_cc.unload()  // Plugin is a global instance based on Pluggable entrypoint name
  Interpreting: 'dfi::plugin::example::example_cc::unload(\"std::cout << --i << std::endl;\")'
  Interpreting: 'std::cout << --i << std::endl;'
  122
  ...
  root [6] my.example2()  // Since plugin has been unloaded, this is expected to not compile
  input_line_27:2:3: error: use of undeclared identifier 'e'
   (my.example2())
    ^
  Error in <HandleInterpreterException>: Error evaluating expression (my.example2())
  Execution of your code was aborted.
  root [7] .quit
  $
"

  if [[ "$#" -gt 0 && ! "$_arg" =~ (^macros/|^plugins/) ]]; then
    lib_utils::die_usage "$_usage"
  fi

  # Type determined by path stub
  local -r _stub="${_arg#*/}"

  #
  # Macro
  #

  if [[ "$_arg" =~ ^macros ]]; then
    local -r _macro="${_stub#*/}"

    # Repo macros
    [ ! -f "${DOCKER_FINANCE_CONTAINER_REPO}/src/root/macro/${_macro}" ] \
      && lib_utils::die_fatal "repo macro not found '${_macro}'"

    # TODO: currently, only repo macros are supported
    #   (no custom macros, use custom plugins instead)

    # Per API (v1), requires path stub format
    declare -gr global_arg_macro="$_stub"
  fi

  #
  # Plugin
  #

  if [[ "$_arg" =~ ^plugins ]]; then
    local -r _plugin="${_stub#*/}"

    # Repo plugins
    if [[ "$_stub" =~ ^repo ]]; then
      [ ! -f "${DOCKER_FINANCE_CONTAINER_REPO}/plugins/root/${_plugin}" ] \
        && lib_utils::die_fatal "repo plugin not found '${_plugin}'"
    fi

    # Custom plugins
    if [[ "$_stub" =~ ^custom ]]; then
      [ -z "$DOCKER_FINANCE_CONTAINER_PLUGINS" ] && lib_utils::die_fatal
      [ ! -f "${DOCKER_FINANCE_CONTAINER_PLUGINS}/root/${_plugin}" ] \
        && lib_utils::die_fatal "custom plugin not found '${_plugin}'"
    fi

    # Per API (v1), requires path stub format
    declare -gr global_arg_plugin="$_stub"
  fi
}

function lib_root::__root()
{
  lib_utils::deps_check "root"

  local -r _path="${DOCKER_FINANCE_CONTAINER_REPO}/src/root"
  pushd "$_path" 1>/dev/null || lib_utils::die_fatal "could not pushd '${_path}'"

  #
  # Base command
  #

  # NOTE:
  #
  # rootlogon.C *MUST* be loaded at any startup; as if fulfills loading
  # of `dfi` source files and other library requirements.
  #
  # Typically, ROOT.cern would load rootlogon.C if we start `root`
  # interactively within the macro directory. However, `dfi` has its own
  # requirements.
  #
  # After rootlogon, any loading of macros, plugins or any `dfi` files
  # can be completed using the `dfi` API.

  local _exec=("root" "-l" "macro/rootlogon.C")

  #
  # Interactive opts
  #

  # Start interactive instance w/out loading/running macro or plugin
  if [[ -z "$global_arg_macro" && -z "$global_arg_plugin" ]]; then
    _exec+=("-e" "help()")
  fi

  #
  # Pluggable's arguments
  #

  local _load_arg="${*:2:1}"
  local _unload_arg="${*:3}"

  #
  # Pluggable: macro
  #

  if [ ! -z "$global_arg_macro" ]; then

    local _class="${global_arg_macro##*/}"
    _class="${_class^}"
    _class="${_class%.*}"
    declare -r _class

    # NOTE: for benchmarks, google benchmark does not have a gtest equivalent of
    # `GTEST_FLAG(filter)`, but will process shell environment `BENCHMARK_FILTER`
    if [[ "${_class%.*}" == "Benchmark" ]]; then
      declare -grx BENCHMARK_FILTER="$_load_arg"
      # Current impl will treat the load arg as code which, in this case, is not
      unset _load_arg
    fi

    _exec+=("-e" "dfi::macro::load(\"${global_arg_macro}\", \"${_load_arg}\")")

    # NOTE: when calling via shell, server(s) are implied to
    # require an interactive ROOT instance (not a 'one-off')
    if [[ "${_class%.*}" != "Server" ]]; then
      _exec+=('-q')
    fi

    # NOTE: currently no need to "dfi::macro::unload()" from here
  fi

  #
  # Pluggable: plugin
  #

  if [ ! -z "$global_arg_plugin" ]; then

    #
    # Instead of using one-off wrappers, the following leaves the plugin's state
    # intact during interpreter runtime.
    #
    #   WARNING:
    #
    #     The dispatcher (or any declarations outside of the dispatcher)
    #     will appear in the global namespace of the interpreter!
    #

    # Gives plugin a global instance based on Pluggable entrypoint name
    local _name="${global_arg_plugin##*/}"
    _name="g_plugin_${_name%.*}_${_name#*.}"
    declare -r _name

    local _code
    _code+="using t_path = dfi::plugin::common::PluginPath;"
    _code+="t_path path{\"${global_arg_plugin}\"};"

    _code+="using t_space = dfi::plugin::common::PluginSpace;"
    _code+="t_space space{path.parent(), path.child()};"

    _code+="using t_args = dfi::plugin::common::PluginArgs;"
    _code+="t_args args{\"${_load_arg}\", \"${_unload_arg}\"};"

    _code+="dfi::common::type::Pluggable<t_path, t_space, t_args> type{path, space, args};"
    _code+="dfi::plugin::common::Plugin plugin{type};"

    local _dispatch="auto dispatch = []() -> auto { $_code; return plugin; };"
    _dispatch+="auto ${_name} = dispatch();"

    # NOTE: unload() at will, inside the interpreter
    _exec+=("-e" "$_dispatch" "-e" "${_name}.load();")

  fi

  #
  # Execute
  #

  lib_utils::print_debug "${_exec[@]}"
  "${_exec[@]}" || lib_utils::die_fatal "root exited with: $?"
}

# vim: sw=2 sts=2 si ai et
