#!/usr/bin/env bash

# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2025 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

function lib_reports::reports()
{
  lib_reports::__parse_args "$@"
  time lib_reports::__reports "$@"
  lib_utils::catch $?
}

#
# Implementation
#

function lib_reports::__parse_args()
{
  [ -z "$global_usage" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_2" ] && lib_utils::die_fatal
  [ -z "$global_arg_delim_3" ] && lib_utils::die_fatal

  local -r _usage="
\e[32mDescription:\e[0m

  Generate financial reports (income statement, balance sheet, etc.)

\e[32mUsage:\e[0m

  $ $global_usage [all${global_arg_delim_2}<option1[{${global_arg_delim_3}option2${global_arg_delim_3}...}]>] [type${global_arg_delim_2}<type1[${global_arg_delim_3}type2${global_arg_delim_3}...]] [interval${global_arg_delim_2}<interval1[${global_arg_delim_3}interval2${global_arg_delim_3}...]] [year${global_arg_delim_2}<year>]

\e[32mArguments:\e[0m

  All options:

    all${global_arg_delim_2}<all|type|interval>

  Report type:

    type${global_arg_delim_2}<acc|is|bs|bse|cf>

        acc = accounts
        bs  = balance sheet
        bse = balance sheet w/ equity
        cf  = cash flow
        is  = income statement

  Report interval (period):

    interval${global_arg_delim_2}<daily|weekly|monthly|quarterly|{annual|yearly}>

  Report year:

    year${global_arg_delim_2}<year>

\e[32mExamples:\e[0m

  \e[37;2m# Generate all type and all intervals for current year\e[0m
  $ $global_usage all${global_arg_delim_2}all

  \e[37;2m# Generate all types for annual report for year 2022\e[0m
  $ $global_usage all${global_arg_delim_2}type interval${global_arg_delim_2}annual year${global_arg_delim_2}2022

  \e[37;2m# Generate income statements and balance sheets, quarterly and monthly for current year\e[0m
  $ $global_usage type${global_arg_delim_2}is${global_arg_delim_3}bs interval${global_arg_delim_2}quarterly${global_arg_delim_3}monthly
"

  #
  # Ensure supported arguments
  #

  [ $# -eq 0 ] && lib_utils::die_usage "$_usage"

  for _arg in "$@"; do
    [[ ! "$_arg" =~ ^all${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^type[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^interval[s]?${global_arg_delim_2} ]] \
      && [[ ! "$_arg" =~ ^year[s]?${global_arg_delim_2} ]] \
      && lib_utils::die_usage "$_usage"
  done

  #
  # Parse arguments before testing
  #

  # Parse key for value
  for _arg in "$@"; do

    local _key="${_arg%${global_arg_delim_2}*}"
    local _len="$((${#_key} + 1))"

    if [[ "$_key" =~ ^all$ ]]; then
      local _arg_all="${_arg:${_len}}"
      [ -z "$_arg_all" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^type[s]?$ ]]; then
      local _arg_type="${_arg:${_len}}"
      [ -z "$_arg_type" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^interval[s]?$ ]]; then
      local _arg_interval="${_arg:${_len}}"
      [ -z "$_arg_interval" ] && lib_utils::die_usage "$_usage"
    fi

    if [[ "$_key" =~ ^year[s]?$ ]]; then
      local _arg_year="${_arg:${_len}}"
      [ -z "$_arg_year" ] && lib_utils::die_usage "$_usage"
    fi
  done

  #
  # Test for valid ordering/functionality of argument values
  #

  # Arg: all
  if [ ! -z "$_arg_all" ]; then
    # Can't use with non-years activated
    if [[ ! -z "$_arg_type" && ! -z "$_arg_interval" ]]; then
      lib_utils::die_usage "$_usage"
    fi
  fi

  # Arg: type
  if [ ! -z "$_arg_type" ]; then
    [[ ! -z "$_arg_all" && ! -z "$_arg_interval" ]] \
      || [[ -z "$_arg_all" && -z "$_arg_interval" ]] \
      && lib_utils::die_usage "$_usage"
  fi

  # Arg: interval
  if [ ! -z "$_arg_interval" ]; then
    [[ ! -z "$_arg_all" && ! -z "$_arg_type" ]] \
      || [[ -z "$_arg_all" && -z "$_arg_type" ]] \
      && lib_utils::die_usage "$_usage"
  fi

  # Arg: year
  if [ ! -z "$_arg_year" ]; then
    [[ -z "$_arg_all" && -z "$_arg_type" && -z "$_arg_interval" ]] \
      && lib_utils::die_usage "$_usage"
  fi

  #
  # Test argument values, set globals
  #

  IFS="$global_arg_delim_3"

  # Arg: all
  if [ ! -z "$_arg_all" ]; then
    # If all= {type,interval} or {interval,type} then set to all=all
    [[ "${#_arg_all[@]}" -eq 1 && "${_arg_all[*]}" =~ type[s]? && "${_arg_all[*]}" =~ interval[s]? ]] \
      && _arg_all="all"

    # Read args from all
    read -ra _read <<<"$_arg_all"
    for _arg in "${_read[@]}"; do
      # Support values
      [[ ! "$_arg" =~ ^all$|^type[s]?$|^interval[s]?$ ]] \
        && lib_utils::die_usage "$_usage"

      # If all=all then no need for all={type,interval} and type= or interval=
      [[ "$_arg" == "all" && (! -z "$_arg_type" || ! -z "$_arg_interval") ]] \
        || [[ "$_arg" == "all" && "${#_read[@]}" -gt 1 ]] \
        && lib_utils::die_usage "$_usage"

      # If all=type then no need for type= and if all=interval then no need for interval=
      [[ "$_arg" =~ ^type[s]?$ && ! -z "$_arg_type" ]] \
        || [[ "$_arg" =~ ^interval[s]?$ && ! -z "$_arg_interval" ]] \
        && lib_utils::die_usage "$_usage"

      # If all=type then need interval= or if all=interval then need type=
      if [[ "$_arg" != "all" ]]; then
        [[ "${#_read[@]}" -lt 2 && "$_arg" =~ ^type[s]?$ && -z "$_arg_interval" ]] \
          || [[ "${#_read[@]}" -lt 2 && "$_arg" =~ ^interval[s]?$ && -z "$_arg_type" ]] \
          && lib_utils::die_usage "$_usage"
      fi
    done
    declare -gr global_arg_all=("${_read[@]}")
  fi

  # Arg: type
  # TODO: acc should not require (nor produce based upon) interval
  if [ ! -z "$_arg_type" ]; then
    read -ra _read <<<"$_arg_type"
    for _arg in "${_read[@]}"; do
      [[ ! "$_arg" =~ ^acc$|^bs$|^bse$|^cf$|^is$ ]] && lib_utils::die_usage "$_usage"
    done
    declare -gr global_arg_type=("${_read[@]}")
  elif [[ "${global_arg_all[*]}" =~ (all|type) ]]; then
    declare -gr global_arg_type=("acc" "bs" "bse" "cf" "is")
  fi

  # Arg: interval
  if [ ! -z "$_arg_interval" ]; then
    read -ra _read <<<"$_arg_interval"
    for _arg in "${_read[@]}"; do
      [[ ! "$_arg" =~ ^daily$|^weekly$|^monthly$|^quarterly$|^yearly$|^annual$ ]] \
        && lib_utils::die_usage "$_usage"
    done
    declare -gr global_arg_interval=("${_read[@]}")
  elif [[ "${global_arg_all[*]}" =~ (all|interval) ]]; then
    declare -gr global_arg_interval=("daily" "weekly" "monthly" "quarterly" "annual")
  fi

  # Arg: year
  # TODO: implement range
  # TODO: implement all years
  if [ ! -z "$_arg_year" ]; then
    # TODO: 20th century support
    if [[ ! "$_arg_year" =~ ^20[0-9][0-9]$ ]]; then
      lib_utils::die_usage "$_usage"
    fi
    declare -gr global_arg_year="$_arg_year"
  else
    global_arg_year="$(date +%Y)"
    declare -gr global_arg_year
  fi
}

function lib_reports::__reports()
{
  [ -z "$global_parent_profile" ] && lib_utils::die_fatal
  [ -z "$global_child_profile" ] && lib_utils::die_fatal
  [ -z "$global_child_profile_journal" ] && lib_utils::die_fatal

  local -r _year="$global_arg_year"

  local -r _out_dir="$(dirname $global_child_profile_journal)/reports/${_year}"
  [ ! -d "$_out_dir" ] && mkdir -p "$_out_dir"

  local -r _indv_out_dir="${_out_dir}/individual"
  [ ! -d "$_indv_out_dir" ] && mkdir -p "$_indv_out_dir"

  lib_utils::print_custom "\n"
  lib_utils::print_info "Generating financial reports in year '${_year}' for '${global_parent_profile}/${global_child_profile}' ..."
  lib_utils::print_custom "\n"
  lib_utils::print_normal "  ─ Reports"
  lib_utils::print_custom "    \e[32m│\e[0m\n"

  local _username
  _username="$(basename "$(dirname $global_child_profile_journal)" | tr '[:lower:]' '[:upper:]')"

  for _type in "${global_arg_type[@]}"; do
    case "$_type" in
      acc)
        local _echo="accounts"
        local _desc="ACCOUNTS"
        local _base_file="${_year}_${_username}_accounts"
        local _opts=("not:equity:balances\$")
        ;;
      bs)
        local _echo="balance sheet"
        local _desc="BALANCE SHEET"
        local _base_file="${_year}_${_username}_balance-sheet"
        local _opts=("")
        ;;
      bse)
        local _echo="balance sheet w/ equity"
        local _desc="BALANCE SHEET w/ EQUITY"
        local _base_file="${_year}_${_username}_balance-sheet-equity"
        # TODO: finer rule for equity-specific deposit/withdrawal
        local _opts=("not:equity:balances\$") #"not:deposit" "not:withdrawal")
        ;;
      cf)
        local _echo="cashflow"
        local _desc="CASHFLOW"
        local _base_file="${_year}_${_username}_cashflow"
        local _opts=("")
        ;;
      is)
        local _echo="income statement"
        local _desc="INCOME STATEMENT"
        local _base_file="${_year}_${_username}_income-statement"
        local _opts=("")
        ;;
      *)
        lib_utils:die_fatal "Type not implemented"
        ;;
    esac

    # Interval

    for _interval in "${global_arg_interval[@]}"; do
      case "$_interval" in
        yearly | annual)
          local _new_base_file="${_base_file}_01_yearly"
          local _new_opts=("${_opts[@]}")
          ;;
        quarterly)
          local _new_base_file="${_base_file}_02_${_interval}"
          local _new_opts=("${_opts[@]}" "--${_interval}")
          ;;
        monthly)
          local _new_base_file="${_base_file}_03_${_interval}"
          local _new_opts=("${_opts[@]}" "--${_interval}")
          ;;
        weekly)
          local _new_base_file="${_base_file}_04_${_interval}"
          local _new_opts=("${_opts[@]}" "--${_interval}")
          ;;
        daily)
          local _new_base_file="${_base_file}_05_${_interval}"
          local _new_opts=("${_opts[@]}" "--${_interval}")
          ;;
        *)
          lib_utils:die_fatal "Interval not implemented"
          ;;
      esac

      # Execute

      local _new_echo="$_echo (${_interval})"
      local _new_desc="$_desc (${_interval})"

      lib_utils::print_custom "    \e[32m├─\e[34m\e[1;3m ${_new_echo}\e[0m\n"

      (lib_reports::__reports_write \
        "$_type" \
        "$_new_desc" \
        "$_new_base_file" \
        "${_new_opts[@]}") &
    done
    wait
    lib_utils::print_custom "    \e[32m│\e[0m\n"
  done

  # TODO: more once upstream adds more features

  # Index of all reports

  lib_utils::print_normal "  ─ Generating index of all reports ..."

  local _index="index"
  lib_reports::__reports_write \
    "$_index" \
    "$_index" \
    "$_index" \
    ""

  lib_utils::print_custom "\n    Now, open your browser to:\n"

  [ -z "${DOCKER_FINANCE_CLIENT_FLOW}" ] && lib_utils::die_fatal
  lib_utils::print_custom "\n      file://${DOCKER_FINANCE_CLIENT_FLOW}/profiles/${global_parent_profile}/${global_child_profile}/reports/${_year}/${_index}.html\n\n"

  lib_utils::print_info "Done!"
}

function lib_reports::__reports_write()
{
  local _type="$1"
  local _description="$2"
  local _base_file="$3"
  local _opts=("${@:4}")

  # Generate HTML
  local _ext="html"
  local _file="${_base_file}.${_ext}"

  # HTML tag for search/replacement
  local _html_tag="DOCKER_FINANCE_HTML_TAG"

  # Generate individual files
  # NOTE: all valued periods are appended
  if [ "$_type" != "index" ]; then

    # Individual report
    local _individual="${_indv_out_dir}/${_file}"

    ## Base hledger command
    local _period=("--period" "$_year")
    if [ "$_year" == "$(date +%Y)" ]; then
      local _period=("-b" "${_year}/01/01" "-e" "$(date +%Y/%m/%d)")
    fi
    # TODO: instead of `not:desc` for opening/closing, add opening/closing tags and use `not:tag`
    local _base_hledger=("${global_hledger_cmd[@]}" "${_period[@]}" "$_type" "${_opts[@]}" "not:desc:balances\$" "not:archive")

    ## Divided sections, and the linkable section
    echo -e "<title>${_year} $_username | ${_description}</title>" >"$_individual"
    echo -e "<div><hr style='height:4px;border-width:0;background-color:green;'>" >>"$_individual"
    echo -e "<h1 style='text-align:center;'><!-- $_html_tag -->${_year} $_username | ${_description}</h1>" >>"$_individual"
    echo -e "<hr style='height:2px;border-width:0;background-color:green;opacity:0.5;'></div>" >>"$_individual"

    ## Brief view
    echo "<h2 style='text-align:center;font-style:italic;line-height:0.75;color:green;'>BRIEF VIEW</h3>" >>"$_individual"

    local _hledger=("${_base_hledger[@]}")
    if [ "$_type" == "acc" ]; then
      # Text -> HTML
      _hledger+=("--depth" "2")
      "${_hledger[@]}" | sed 's:$:<br>:g' >>"$_individual"
      lib_utils::catch $?
      echo -e "<br><br>" >>"$_individual"
    else
      # HTML
      _hledger+=("--depth" "2" "-O" "html")
      "${_hledger[@]}" >>"$_individual"
      lib_utils::catch $?
      echo -e "<br><br>" >>"$_individual"
      _hledger+=("--value=end")
      "${_hledger[@]}" >>"$_individual"
      lib_utils::catch $?
    fi

    echo -e "<hr style='height:2px;border-width:0;background-color:green;opacity:0.5;'>" >>"$_individual"

    ## Full view
    echo "<h2 style='text-align:center;font-style:italic;line-height:0.75;color:green;'>FULL VIEW</h3>" >>"$_individual"

    local _hledger=("${_base_hledger[@]}") # Reset to base
    if [ "$_type" == "acc" ]; then
      # Text -> HTML
      "${_hledger[@]}" | sed 's:$:<br>:g' >>"$_individual"
      lib_utils::catch $?
      echo -e "<br><br>" >>"$_individual"
    else
      # HTML
      _hledger+=("-O" "html")
      "${_hledger[@]}" >>"$_individual"
      lib_utils::catch $?
      echo -e "<br><br>" >>"$_individual"
      _hledger+=("--value=end")
      "${_hledger[@]}" >>"$_individual"
      lib_utils::catch $?
    fi

    ## Back to top
    echo -e "<style>a:link {font-size:120%;text-decoration:none;} a:hover {background-color:lightgreen;color:black;} a:active {background-color:green; color:white;}</style>" >>"$_individual"
    echo -e "<br><br><div style='text-align:center;font-style:italic;'><a href='#'>Back to top</a></div><br>" >>"$_individual"

    return
  fi

  # Combines HTML lines from each individual report (that has _html_tag) into links within an single index
  local _index="${_out_dir}/${_file}"
  echo "" >"$_index"

  ## Grab individuals
  ls "${_indv_out_dir}"/*.${_ext} \
    | sort -r \
    | while read _line1; do
      basename -s ."${_ext}" "$(echo "$_line1")" \
        | while read _line2; do # Pull out the description to use as link
          sed -i "1i$(echo "<p style='text-align:center;text-decoration:none;list-style-type:none;font-style:normal;line-height:0.75;'><a href='individual/${_line2}.${_ext}' target='_blank'>$(grep $_html_tag $_line1 | cut -d'>' -f3 | cut -d'<' -f1 | cut -d'|' -f2)</a></p>")" "$_index"
        done
    done

  # Additional stylings
  sed -i "1i \
    <html> \
    <head> \
    <title>$_year $_username REPORTS</title> \
    <style>a:link {font-size:120%;text-decoration:none;} a:hover {background-color:lightgreen;color:black;} a:active {background-color:green; color:white;}</style> \
    </head> \
    <body> \
    <div> \
    <hr style='height:4px;border-width:0;background-color:green;'> \
    <h1 style='text-align:center;'>$_year $_username REPORTS</h1> \
    <hr style='height:2px;border-width:0;background-color:green;opacity:0.5;'> \
    </div>" "$_index"

  echo "</body></html>" >>"$_index"
}

# vim: sw=2 sts=2 si ai et
