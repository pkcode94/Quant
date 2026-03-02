
[//]: # (docker-finance | modern accounting for the power-user)
[//]: # ()
[//]: # (Copyright [C] 2024-2026 Aaron Fiore [Founder, Evergreen Crypto LLC])
[//]: # ()
[//]: # (This program is free software: you can redistribute it and/or modify)
[//]: # (it under the terms of the GNU General Public License as published by)
[//]: # (the Free Software Foundation, either version 3 of the License, or)
[//]: # ([at your option] any later version.)
[//]: # ()
[//]: # (This program is distributed in the hope that it will be useful,)
[//]: # (but WITHOUT ANY WARRANTY; without even the implied warranty of)
[//]: # (MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the)
[//]: # (GNU General Public License for more details.)
[//]: # ()
[//]: # (You should have received a copy of the GNU General Public License)
[//]: # (along with this program. If not, see <https://www.gnu.org/licenses/>.)

# Changelog (`docker-finance`)

## 1.3.0 - 2026-02-27

This release focuses on `dfi` code/usage enhancements, a ROOT.cern (`root`) patch release and **breaking** changes to the `dfi` client-side default layout (filesystem).

Also included are dfi-docs updates: [how to get started](https://gitea.evergreencrypto.co/EvergreenCrypto/dfi-docs/src/branch/master/markdown/How-do-I-get-started.md), categorical [plugins](https://gitea.evergreencrypto.co/EvergreenCrypto/dfi-docs/src/branch/master/markdown/How-do-I-use-it.md#plugins) and a new demo of the [Bitcoin plugin](https://gitea.evergreencrypto.co/EvergreenCrypto/dfi-docs/src/branch/master/markdown/How-do-I-use-it.md#plugins-bitcoin).

### 1.3.0 - Fixes

- Fix `root` plugin code documentation (custom plugin location) ([#302](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/302))

### 1.3.0 - Enhancements

- 🚨**Breaking** Change layout of default client/container paths (environment, bind-mounts) ([#301](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/301)) ([#303](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/303))
  * Defaults are now consolidated under a single parent directory
  * Basenames are now simply-named to avoid verbosity and confusion
  * For smoothest transition:
    1. Create a parent umbrella directory called `./docker-finance`
    2. Rename (or symlink) your `docker-finance` repository (`DOCKER_FINANCE_CLIENT_REPO`) to `./docker-finance/repo`
    3. Rename (or symlink) your `docker-finance.d` directory (`DOCKER_FINANCE_CLIENT_CONF`) to `./docker-finance/conf.d`
    4. Rename (or symlink) your `finance-flow` directory (`DOCKER_FINANCE_CLIENT_FLOW`) to `./docker-finance/flow`
       - Within your `./docker-finance/flow/profiles` directory, rename all subprofile `docker-finance.d` directories to `conf.d`
    6. Rename (or symlink) your custom `plugins` directory (`DOCKER_FINANCE_CLIENT_PLUGINS`) to `./docker-finance/plugins`
    7. Rename (or symlink) your `share.d` directory (`DOCKER_FINANCE_CLIENT_SHARED`) to `./docker-finance/share.d`
    8. Manually update all `./docker-finance/conf.d` environment files for all platforms/tags to reflect the new client-side locations:
       - e.g.; `$EDITOR ./docker-finance/conf.d/client/Linux-x86_64/archlinux/default/env/$(whoami)@$(uname -n)`
    9. Manually update your `superscript.bash`; changing `docker-finance.d` to `conf.d` where applicable:
       - e.g.; `sed -i 's:/docker-finance.d/:/conf.d/:g' ./docker-finance/conf.d/container/shell/superscript.bash`
    10. Manually remove the docker-finance `source` completion line from `~/.bashrc` (or `~/.bash_aliases`)
    11. Re-run install:
        - i.e.; `./docker-finance/repo/client/install.bash && source ~/.bashrc`
    12. *Optional* Re-run `gen type=env` to produce the new container-side bind-mount locations:
        - i.e.; `dfi archlinux/${USER}:default gen type=env confirm=no`
    13. 🥳 Your new defaults can now be changed and paths moved/renamed as you please: simply update your environment file in `conf.d`:
        - i.e.; `dfi archlinux/${USER}:default edit type=env`
        - ⚠ If changing the expected repository location, you'll need to manually update the `conf.d` environment file as well as your aliases/source (`~/.bashrc`)

- Allow undesirable characters in variable-text columns for respective hledger-flow accounts, related refactoring ([#297](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/297))
- Add `root` interpreter common exit function and test case, refactor CI workflow ([#298](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/298))
- Silence client's `exec` noise, when not debugging ([#299](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/299))
- Pull tags from remote in client Bitcoin plugin ([#300](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/300))
- Decrease bootstrap wait time for client Tor plugin ([#305](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/305))
- Improve usage help for client/container `plugins` commmand, add help to client `plugins` completion ([#306](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/306))

### 1.3.0 - Updates

- Update all repo-related domain links to gitea.evergreencrypto.co ([#296](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/296))
  * You can still find this repository mirrored to gitea.com

- Bump `root` to 6.38.02-1 ([#304](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/304)) ([#307](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/307))

### 1.3.0 - Contributors

- Aaron Fiore

## 1.2.0 - 2026-02-06

This point release focuses on providing a new CI system using [Gitea Actions](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/actions).

Also included are minor `dfi` changes (mostly client-side) and some usability improvements.

> Note: any re-`gen` instructions below will automatically backup respective data to their respective locations.

### 1.2.0 - Features

- New CI using Gitea Actions Workflow and [instructions](https://gitea.evergreencrypto.co/EvergreenCrypto/dfi-docs/src/branch/master/markdown/How-do-I-contribute.md#ci-using-self-hosted-gitea-with-act_runner) on how to self-host Gitea CI for `dfi` development ([#287](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/287)) ([#291](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/291)) ([#292](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/292)) ([#293](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/293))

### 1.2.0 - Fixes

- Minor internal fixes to work better with the new CI ([#281](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/281)) ([#283](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/283)) ([#289](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/289))

- Fix for container's `taxes` command breaking when generating `all` years (`year=all`) ([#279](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/279))
  * This bug does *not* affect individual year tax report generation, when given a specific (or default) year

- Fix client ownership of shared path during client generation ([#285](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/285))
  * Was previously owned by root instead of `DOCKER_FINANCE_USER`

- Add missing chains to coinomi template ([#288](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/288))
  * To utilize these changes: `dfi archlinux/${USER}:default gen type=flow profile=<profile/subprofile> account=coinomi confirm=no`

- Fix client Tor plugin's container inspection ([#292](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/292))

### 1.2.0 - Enhancements

- Minor internal tweaks to work better with the new CI ([#274](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/274)) ([#275](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/275)) ([#276](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/276)) ([#277](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/277)) ([#278](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/278)) ([#280](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/280)) ([#282](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/282))

- Allow quotations when passing root `Pluggable` arguments ([#286](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/286))

   > Helps facilitate CLI usage; e.g.,
   > ```bash
   > $ dfi testprofile/testuser root plugins/repo/bitcoin/bitcoin.cc 'dfi::macro::load(\"repo/test/unit.C\")'
   > ```

- Implement retries when bootstrapping client Tor plugin, misc Tor plugin enhancements ([#292](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/292))

### 1.2.0 - Updates

- `dev-tools`: bump `cppcheck`'s standard to c++20 ([#295](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/295))

### 1.2.0 - Refactoring

- Update optional blocks in default custom Arch Linux Dockerfile, move `less` package from custom build to base build ([#284](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/284))
  * To utilize these changes: `dfi archlinux/${USER}:default gen type=build confirm=no`
  * To update your new config: `dfi archlinux/${USER}:default edit type=build`

- Update mobula assets and update/cleanup comments in default `fetch` config ([#290](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/290))
  * To utilize these changes: `dfi archlinux/${USER}:default gen type=flow profile=<profile/subprofile> config=fetch confirm=no`
  * To update your new config (in container): `dfi <profile/subprofile> edit type=fetch`

- Add `cppcheck-suppress stlIfStrFind` to root common ([#295](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/295))

### 1.2.0 - Contributors

- Aaron Fiore

## 1.1.1 - 2026-01-15

This patch release supports recent CoinGecko and PHP-CS-Fixer changes and also includes minor `dfi` developer enhancements/refactoring.

### 1.1.1 - Fixes

- Since CoinGecko now returns error 403 without a "descriptive" User-Agent, an agent has been added when fetching prices from CoinGecko ([#269](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/269))
- PHP-CS-Fixer's 3.92 series now requires `init` before any `fix`ing or `check`ing ([#270](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/270)) ([#272](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/272))

### 1.1.1 - Enhancements

- Add `API_VERSION` to `fetch`'s PHP environment map ([#269](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/269))

### 1.1.1 - Refactoring

- Minor dev-tools' `lib_linter` refactoring ([#271](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/271)) ([#272](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/272))

### 1.1.1 - Contributors

- Aaron Fiore

## 1.1.0 - 2026-01-12

This release focuses on bringing substantial changes to the `root` system and improving `root` usage.

Also included are minor changes; minor fixes, dependency bumps and refactoring (as noted below).

### 1.1.0 - Fixes

> For this release, code fixes that are related to new `root` additions are included in the below Features category

- Remove comma(s) from hledger-flow's Discover credit subaccount description ([#259](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/259))
  * Fixes column processing when extra comma(s) are present

- Update deadlinks in Gitea pull request template ([#267](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/267))

### 1.1.0 - Features

- New ROOT.cern image/package with C++20 support 🎉, related `dfi` migrating and refactoring ([#237](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/237)) ([#238](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/238)) ([#240](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/240)) ([#247](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/247)) ([#248](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/248)) ([#251](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/251)) ([#255](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/255)) ([#257](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/257)) ([#262](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/262)) ([#265](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/265))
  * The Arch Linux ROOT.cern package, be default, only supports up to C++17. As a result, a new `evergreencrypto/root` Docker image was created and is now maintained to provide an optimized C++20 package

- New `dfi` Pluggable API framework for `root` plugins and macros; auto-(un)load'ing, enhanced `common` system, related refactoring/deprecations and additional tests ([#241](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/241)) ([#243](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/243)) ([#244](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/244)) ([#245](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/245)) ([#249](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/249)) ([#252](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/252)) ([#253](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/253)) ([#256](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/256)) ([#258](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/258)) ([#261](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/261)) ([#263](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/263)) ([#264](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/264))
  * Container's usage help: `dfi <profile/subprofile> root help`
  * Doxygen: `dfi dev-tools/${USER}:default doxygen gen`
  * Repo examples: [plugins](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/src/commit/14df4c94e05fce9b557a8c828e384d4691303310/container/plugins/root) and [macros](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/src/commit/14df4c94e05fce9b557a8c828e384d4691303310/container/src/root/macro)
    - ⚠️ Though not recommended, to use the previous method of loading plugins/macros, continue to use the `dfi::common` file loader instead

- New ₿itcoin plugin; related `dfi` API additions and tests ([#254](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/254)) ([#261](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/261)) ([#263](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/263))
  * Client's (host's) [plugin](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/src/commit/14df4c94e05fce9b557a8c828e384d4691303310/client/plugins/docker/bitcoin.bash): `dfi archlinux/${USER}:default plugins repo/bitcoin.bash`
    - ⚠️ To use the client's plugin, you **MUST** run the following at least once:
      * Re-`gen` build file: `dfi archlinux/${USER}:default gen type=build confirm=no`
      * Re-`edit` build file: `dfi archlinux/${USER}:default edit type=build` (and then uncomment dependencies)
      * Re-`build` image: `dfi archlinux/${USER}:default build type=default` (or use the `update` command)
  * Container's [plugin](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/src/commit/14df4c94e05fce9b557a8c828e384d4691303310/container/plugins/root/bitcoin/bitcoin.cc) can be loaded from the command line or from within interpreter:
    - Command line: `dfi <profile/subprofile> root plugins/repo/bitcoin/bitcoin.cc`
    - Interpreter: `dfi::plugin::load("repo/bitcoin/bitcoin.cc")`
  * See [important notes](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/src/commit/14df4c94e05fce9b557a8c828e384d4691303310/client/plugins/docker/bitcoin.bash#L21-L59) regarding the client/container relationship
    - NOTE: all symbols in libbitcoinkernel.so are available but you'll need to load respective headers as needed (e.g., [common loader](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/src/commit/14df4c94e05fce9b557a8c828e384d4691303310/container/plugins/root/bitcoin/bitcoin.cc#L80))

### 1.1.0 - Enhancements

> For this release, code enhancements related to `root` are included in the above Features category

- Add `exec` wrapper to client ([#246](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/246))

### 1.1.0 - Updates

> For this release, package updates related to `root` are included in the above Features category

- Bump `hledger` to 1.51.2 ([#242](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/242)) ([#250](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/250)) ([#260](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/260))
- Bump `hledger-flow` to v0.16.2 ([#257](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/257))

### 1.1.0 - Refactoring

> For this release, code refactoring related to `root` is included in the above Features category

- Use Docker multi-stage for `hledger-suite` and `root` images ([#250](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/250)) ([#251](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/251))

### 1.1.0 - Contributors

- Aaron Fiore

## 1.0.0 - 2025-10-31

🎉 Happy 1.0.0 release

### 1.0.0 - Fixes

- client: `linter`: fix finding .clang-format ([#219](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/219))
- container: `fetch`: fix wording to reflect impl ([#212](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/212))
- container: `taxes`: fixes for 'all' years arg parsing and printing, don't attempt to patch non-existent taxable events ([#230](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/230))
- ⚠️ container: hledger-flow: electrum: add lightning, backwards compat ([#227](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/227))
  * electrum lightning support is a WIP and, as a result, so is the support here (see [#233](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/issues/233))
- container: plugins: `root`: fix example3()'s shell ([#229](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/229))

### 1.0.0 - Features

- client: add `update` feature ([#225](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/225))
  * Update your `dfi` instance with the `update` command (instead of `rm` -> system prune -> `build`)
- container: add lib_root impl (macro/plugin support), update completion and usage help ([#215](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/215))
  * dfi's `root` now supports shell loading (and running) of macros/plugins

### 1.0.0 - Enhancements

- ⚠️**Potentially Breaking**: container: root: macro: Hash/Random: change output to CSV format ([#217](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/217))
  * Any end-user expectations (*custom* plugins, etc.) should be adjusted, if needed
- client: `linter`: allow *custom* C++ plugins to also be linted/formatted ([#211](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/211))
- container: `taxes`/`reports`: optimize writes by forking ([#221](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/221)) ([#223](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/223))
  * Decreases real time in multicore container environments
- 🚨container: hledger-flow: ethereum-based: compound: add internal swapping when supplying/withdrawing ([#228](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/228))
  * **MUST** re-`import` from oldest applicable year
  * **MUST** re-`taxes` for all applicable years
- 🚨container: hledger-flow: finish uniform fiat subaccounts ([#213](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/213))
  * **MUST** adjust custom rules/journals as needed
  * **MUST** re-`import` from oldest applicable year
  * **MUST** re-`report` for all applicable years
- 🚨container: hledger-flow: paypal/paypal-business: add expenses description subaccount ([#214](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/214))
  * **MUST** re-`import` from oldest applicable year
  * **MUST** re-`report` for all applicable years
- 🚨container: hledger-flow: vultr: add credits support ([#226](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/226))
  * **MUST** re-`import` from oldest applicable year
  * **MUST** re-`report` for all applicable years

### 1.0.0 - Updates

- client/container: usage help updates ([#231](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/231))
- client: Dockerfiles: remote: hledger-suite: bump to latest `hledger`/`hledger-flow`/`hledger-iadd` ([#216](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/216)) ([#220](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/220))
  * Also updates hledger-suite Dockerfile build process
- repo: gitea: template: update bug template ([#234](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/234))
- repo: migrate all docs and related assets to 'dfi-docs' repository, update README ([#232](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/232))
  * All non-code documentation has been moved to the [dfi-docs](https://gitea.evergreencrypto.co/EvergreenCrypto/dfi-docs) repository
- repo: remove deprecated donation funding images ([#224](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/224))

### 1.0.0 - Refactoring

- ⚠️**Potentially Breaking**: client/container: migrate from `xsv` to `xan` ([#222](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/222))
  * Any end-user *custom* plugins that rely on `xsv` should use `xan` instead
    - To continue using `xsv`, add `xsv` to your build: `dfi <platform/user:tag> edit type=build`
- ⚠️**Potentially Breaking**: container: php/c++: `dfi` namespace refactor ([#210](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/210))
  * Any end-user *custom* plugin namespaces should be adjusted, if needed
  * Any Doxygen used should be cleaned and regenerated with the `dev-tools` image, if needed
- container: hledger-flow: add explicit amount4 to applicable rules ([#218](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/218))
- container: php general refactoring ([#208](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/208)) ([#209](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/209))
  * Removes deprecations (since PHP 8.4) and linter errors

### 1.0.0 - Contributors

- Aaron Fiore

## 1.0.0-rc.3 - 2025-08-12

⚠️ **Breaking changes for this release candidate** ⚠️

- **Breaking**: support non-alpha characters (currency) for all applicable accounts ([#202](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/202)) ([#205](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/205))
  * gemini: the 1-in filename for 1INCH trades **MUST** be renamed from "oneinchXXX-Trades.csv" to "1inchXXX-Trades.csv" (where XXX is the counter pair)
    - All applicable years in gemini account's 2-preprocessed and 3-journal **MUST** be deleted and re-`import`ed
  * **MUST** re-`fetch` respective prices for applicable years
  * **MUST** re-run `taxes` for applicable years

- **Breaking**: ethereum-based: support Etherscan V2, add more L2 chains, add/update L2 mockups, add/update L2 spam rules, support non-alpha chars (currency) ([#201](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/201))
  * **MUST** use new `fetch` config API format and API key (see [#201](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/201))
    - **MUST** perform subsequent manual adjustment (update any API keys)
  * If developing, **MUST** `gen`/re-`gen` the mockups

- **Breaking**: btcpayserver: support "Legacy Invoice Export" plugin, update "Wallets" impl, update mockups ([#203](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/203))
  * As of BTCPayServer v2.2.0, **MUST** install the "Legacy Invoice Export" plugin
    - **MUST** export legacy and wallets reports and re-`import` for all applicable years
  * If developing, **MUST** re-`gen` the mockup

- **Breaking** paypal: support latest fiat header, update fiat mockup ([#199](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/199))
  * Adds 'fees' subaccount and new tags from the additional columns
  * **MUST** run client-side `gen` for paypal account
    - **MUST** re-`import` all applicable years
  * If developing, **MUST** re-`gen` the mockup

- paypal-business: update mockup ([#200](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/200))

- Fix/enhance client usage handling ([#207](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/207))

- Fix client completion for when Docker is not found ([#192](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/192))

- Fix permission denied for visidata (change visidata default dir) ([#198](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/198))

- Fix description-like information with '%' character which breaks import in some cases ([#204](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/204))

- Add more `hledger` commands to container completion ([#194](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/194))

- Bump `hledger` to latest releases ([#193](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/193)) ([#195](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/195)) ([#196](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/196)) ([#197](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/197))

---

### 🧠 Contributors

- Aaron Fiore

## 1.0.0-rc.2 - 2025-04-08

⚠️ **Breaking changes for this release candidate** ⚠️

#### Client

🚨 For the finance image:

- ❗**MUST** re-run `build` command
  * Updates to hledger-suite will always require you to re-build your image

#### Container

🚨 For BTCPay Server account:

- ❗**MUST** export only v2 reports
  * See ([#181](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/181)) for details

- ❗**MUST** re-run `import` command
  * Run `import` starting from oldest applicable year

- ❗**MUST** re-run `taxes` command
  * Run `taxes all=all` for all applicable years

🚨 For Coinbase account:

- ❗**MUST** re-run `edit type=fetch` command
  * Update your CDP key (legacy key support has been [removed](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/commit/ca31fef052819e2c118f57451f64ca94bad01d2b))

---

### 🧠 Contributors

- Aaron Fiore

### 🛠️ Fixes / Updates

#### Client

##### *Dockerfiles*

- hledger-suite ([#184](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/184)) ([#185](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/185))
  * Bump `hledger` to 1.42.1

##### *docker-finance.d (configurations)*

- Update Coinbase CDP key, remove unused YAML keys ([#176](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/176)) ([#177](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/177))

##### *src/docker*

- Only allocate TTY when needed (with `run` command) ([#187](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/187))

#### Container

##### *src/finance*

- `fetch`
  * Isolate given year when fetching prices with mobula API ([#188](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/188))

### ☀️ Features / Enhancements

#### Repo

- Add prelim `dfi` logo ([#189](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/189))

#### Container

##### *src/finance*

- `lib_taxes`
  * Optimize record printing, add checks and logging ([#179](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/179))

##### *src/hledger-flow*

- `lib_preprocess`
  * Allow testing of single / multiple columns ([#180](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/180/))

- BTCPay Server
  - Add prelim support for v2 reports ([#181](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/181))
  - Add `taxed_as` INCOME tag, add local timezone support ([#183](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/183))

- Ethereum-based
  * Add COMP rewards ([#178](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/178))
  * Add DeFi swapper tag ([#182](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/182))
  * Add to Ethereum spam rules ([#186](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/186))

- Ledger
  * Skip COMP token (handled by ethereum-based `fetch`) ([#178](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/178))

### 🌴 Misc / Other

#### Container

##### *src/finance*

- `lib_taxes`
  * Some refactoring ([#179](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/179))

##### *src/hledger-flow*

- `lib_preprocess`
  * Symlink to finance's `lib_utils` ([#180](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/180/))

## 1.0.0-rc.1 - 2025-01-23

⚠️ **Breaking changes for this release candidate** ⚠️

> Tip: when in doubt, regenerate your entire setup. Your environment and accounts will be backed up in the process.

#### Repo

🚨 For installation:

- ❗ **MUST** re-run the installation procedure in [README.md](README.md#installation)
  * Reinstall on a freshly pruned system
    - Remove all previous docker-finance related images, clean cache

#### Client

🚨 For all environments and applicable configurations:

- ❗**MUST** re-run `gen` command
  * Run with arg `type=env`
    - **MUST** manually copy over your previous settings (except for `DOCKER_FINANCE_DEBUG`)

- ❗**SHOULD** re-run `gen` command
  * Run with args `type=flow,superscript config=subscript,hledger,fetch confirm=off`
    - **MUST** manually copy over your previous settings

#### Container

🚨 For coinbase account:

- ❗**MUST** re-run `fetch` command
  * Run `fetch account=coinbase` for all applicable years
  * Run `fetch all=price` for all applicable years

🚨 For all accounts and subaccounts:

- ❗**MUST** re-run `import` command
  * Run `import` starting from oldest available year
    - **MUST** update any custom rules or manual entries to reflect subaccount/network changes

- ❗**MUST** re-run `taxes` command
  * Run `taxes all=all` for all applicable years

---

### 🧠 Contributors

- Aaron Fiore

### 🛠️ Fixes / Updates

#### Repo

##### *README*

- Fix 'dev-tools' build instructions, misc updates ([#171](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/171))

#### Client

##### *Dockerfiles*

- Version updates for hledger-suite image ([#156](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/156))
  * Bump `hledger` to 1.41
  * Change resolver for `hledger-iadd`

##### *docker-finance.d (configurations)*

- Add Coinomi example to fetch ([#170](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/170))

##### *src/docker*

- Fix typo in usage help ([#158](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/158))

#### Container

##### *src/finance*

- `lib_hledger`
  * Remove `-w` arg to `hledger-ui` ([#157](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/157))

- `lib_fetch`
  * Fix price parsing ([#130](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/130))
  * Fix Tor plugin (proxychains) check ([#142](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/142))

##### *src/hledger-flow*

- BlockFi / Ledger / Trezor
  * Fix fees subaccount ordering ([#165](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/165))

- Electrum
  * Fix calculating 'No Data' ([#138](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/138))

- Capital One (bank)
  * Fix direction ([#132](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/132))

- Coinbase
  * Fix/update pagination ([#172](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/172)) ([#173](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/173))

- Coinbase Pro
  * Fix default account2 for sells ([#166](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/166))

##### *src/finance* | *src/hledger-flow*

- **Breaking**: Add compliance for IRS Rev. Proc. 2024-28 ([#163](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/163)) ([#174](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/174))
  * Creates per-wallet / per-account compliance for IRS Rev. Proc. 2024-28
  * New obfuscated keymap now creates a unique indentifier per-wallet / per-account
  * Removes support for anonymized ("universal pool") reports

#### Client/Container

- Remove `fetch` support for defunct exchanges/services ([#169](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/169))
  * Bittrex
  * Celsius
  * Coinbase Pro

### ☀️ Features / Enhancements

#### Repo

##### *README*

- Add support for NonFi, fix TradFi anchor tag ([#141](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/141))
- Add docker-finance logo, update README ([#159](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/159))

#### Client

##### *docker-finance.d (configurations)*

- Add filters for .in files to produce less cluttered default configs ([#147](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/147))
- Rewrite profile default subscript ([#148](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/148))

##### *src/docker*

- Enable linting of client-side (host) plugins ([#136](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/136))

#### Container

##### *src/finance*

- `completion`
  * Add `hledger` commands ([#139](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/139))

- `edit`
  * Add 'subscript' option ([#146](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/146))
  * Add `hledger add` command ([#152](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/152))

- `fetch`
  * Add debug logging to Gemini ([#153](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/153))

- `plugins`
  * finance: `timew_to_timeclock`: print all tags ([#133](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/133))

##### *src/hledger-flow*

- Algorand
  * Add to Governance Rewards ([#135](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/135))
  * Add to spam rules ([#161](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/161))

- Electrum
  * Use transaction label for tax memo ([#168](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/168))

- ethereum-based
  * Add to Ethereum spam rules ([#162](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/162))
  * Add to Polygon spam rules ([#167](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/167))

- Ledger
  * Add more compound tokens ([#164](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/164))

- Vultr
  * Add Vultr support ([#140](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/140)) ([#151](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/151))

#### Client / Container

- **Breaking**: Complete build overhaul, build optimizations ([#143](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/143))
  * Creates a separation of concerns for 'local' and 'remote' building
  * Adds remote image 'hledger-suite'
    - Provides the latest versions of all `hledger` related binaries
    - No longer relies on package maintainers / out-dated packages
  * Adds remote image 'docker-finance'
    - Provides base image for 'finance' and 'dev-tools' images
  * Removes previous `hledger` related build modules
    - Removes building any `hledger` related binaries locally
  * Removes 'experimental' build
    - End-user can use local custom Dockerfile and/or custom tag instead
  * Updates the 'default' | 'slim' | 'tiny' | 'micro' build types
    - Refactors build type requirements into separate build modules
  * Huge optimizations
    - Vastly improves build times
      - ~60% faster w/ a fresh build
      - ~60%-90% faster rebuild (depending on image type)
    - Vastly improves image sizes
      - e.g., 'default' Arch Linux image size is ~50% smaller
  * Cleanup and clarify default generated custom Dockerfiles

- **Breaking**: Complete `gen` overhaul, `gen` improvements ([#144](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/commit/77351d54e118b84d93779fad5d9a5963fddef940))
  * New `gen` arguments (see `gen help`)
  * `subprofile.bash` is now `subscript.bash`
    - Changes filename within superscript source
      * **MUST** run client (host) command `edit type=superscript` and edit manually
    - Changes actual filename within subprofile's docker-finance.d
      * **MUST** manually rename the subprofile's script file from `subprofile.bash` to `subscript.bash`

- **Breaking**: Implement debug log-levels ([#149](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/149))

- `completion`
  * `plugins` enhancements ([#155](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/155))

- `plugins`
  * Allow path depth when loading plugins ([#134](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/134))

- Refactoring/enhancements related to `hledger` ([#145](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/145))

### 🌴 Misc / Other

#### Client

##### *Dockerfiles*

- Optimize by moving useradd ([#128](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/128))

##### *docker-finance.d (configurations)*

- Remove base cmd from subscript ([#150](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/150))

##### *src/docker*

- `lib_gen`
  * Refactor profile variables ([#154](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/154))

- `lib_linter`
  * Return function on compose down ([#131](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/131))

#### Container

- Run linter ([#137](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/137))

#### Client/Container

- Refactor global basename ([#160](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/160))

## 1.0.0-beta.3 - 2024-08-20

⚠️ **Breaking changes for this pre-release** ⚠️

#### Repo

❗ For installation:

- 🚨 **MUST** re-run the installation procedure in [README.md](README.md#installation)

#### Client

❗ For client-side environment:

- 🚨 **MUST** re-run `gen` command
  * Regenerate client env and Dockerfile (select 'Y' to all):
    ```
    ├─ Client-side environment found, backup then generate new one? [Y/n]
    │  └─ Edit (new) environment now? [Y/n]
    │
    ├─ Custom (optional) Dockerfile found, backup then generate new one? [Y/n]
    │  └─ Edit (new) custom Dockerfile now? [Y/n]
    ```
  * Regenerate client/container (select 'Y' to all):
    - Superscript:
      ```
      │  ├─ Generate (or update) joint client/container shell script (superscript)? [Y/n]
      │  │  └─ Edit (new) superscript now? [Y/n]
      ```
    - Subprofile shell script:
      ```
      │  ├─ Generate (or update) container flow configs and/or accounts? [Y/n]
      │  │  │
      │  │  ├─ Generate (or update) subprofile's shell script? [Y/n]
      ```
  * Take note of the updated default environment
    - Default directory 'hledger-flow' is now 'finance-flow'
    - Your client-side 'hledger-flow' directory can also be renamed

- 🚨 **MUST** re-run `build` command
  1. Remove (`rm`) previous images
  2. Clean build cache (`docker system prune`)
  3. Re-run a fresh `build` of your docker-finance

> Tip: when in doubt, regenerate your entire setup. Your environment and accounts will be backed up in the process.

#### Container

Aside from changes above, there are no breaking changes for container.

❗ For accounts:

- 🚨 **SHOULD** re-run `import` command
  * Applicable only to Capital One (bank) account ([#124](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/124))

---

### 🧠 Contributors

- Aaron Fiore

### 🛠️ Fixes / Updates

#### Repo

- CHANGELOG.md ([#126](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/126))
  * Add v1.0.0-beta.3
- README.md ([`cab7551`](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/commit/cab7551cbb343e1b46eaba7e76c6321e1061baf0)) ([`d86da4b`](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/commit/d86da4b4d5e3734a9685a29e9e94893b457d5b1a)) ([`3bfa39d`](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/commit/3bfa39de87eebc6c7fa9dca7d80a0f5095eba0ca)) ([#119](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/119))
  * Update to reflect latest impl

#### Client

- Bump manifest version to v1.0.0-beta.3 ([#127](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/127))
- Change environment defaults for `DOCKER_FINANCE_*_FLOW` ([#114](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/114))
- Dockerfiles updates ([#121](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/121)) ([#123](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/123))
  * Add package `yq` (kislyuk's), remove `shyaml`
  * Remove `pipx` from Dockerfiles, replace with packaged `csvkit`
  * Comment all optional userspace packages
- `lib_gen` ([#108](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/108))
  * Refactor flow generation
  * Fix missing copy of default custom Dockerfile
- Miscellaneous fixes ([#106](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/106)) ([#107](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/107)) ([#115](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/115)) ([#117](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/117))
  * Fix usage typo
  * Remove share.d from $PATH
  * Clarify `hledger` (not `ledger`), update usage help / completion
  * Disable shellcheck warning for `DOCKER_FINANCE_DEBUG`
- New installation method ([#104](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/104)) ([#105](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/105))
  * Add `install.bash`
  * Add `dfi` alias to [Mostly-Unified CLI](README.md#mostly-unified-cli)
- Update Doxygen ([#113](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/113))
  * Bump to 1.9.8
    - Change 'modules' to 'topics'

#### Container

- Capital One (bank) ([#124](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/124))
  * Add interest income
- `fetch` fixes ([#102](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/102))
  - Throw errors with nested status
  - Catch unrecoverable CoinGecko error
    * Fix for when 'all' years are requested without paid-plan API key

### ☀️ Features / Enhancements

#### Client / Container

- Add bash completion ([#103](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/103)) ([#112](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/112))
- Add plugins support ([#109](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/109)) ([#111](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/111)) ([#118](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/118)) ([#120](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/120)) ([#122](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/122))
  * Add `plugins` command
  * Add documentation, examples
  * Add proxychains-ng / [Tor](https://www.torproject.org/) plugin
  * Add [`timewarrior`](https://timewarrior.net/) to hledger timeclock plugin
- Add `times` command (timewarrior) ([#110](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/110))

### 🌴 Misc / Other

#### Client

- `lib_docker` ([#125](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/125))
  * Add tag to container name
  * Remove platform from network name

#### Client / Container

- Refactor to use `yq` (kislyuk's), remove `shyaml` ([#121](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/121))
- Set xtrace if debug is enabled ([#116](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/116))

## 1.0.0-beta.2 - 2024-07-30

⚠️ **Breaking changes for this pre-release** ⚠️

#### Client

❗ For all environments and applicable configurations:

- 🚨 **MUST** re-run `gen` command

#### Container

❗ For all applicable accounts and subaccounts:

- 🚨 **MUST** re-run `fetch` command
  - Remove your previously fetched `1-in` files prior to re-fetching
  - Run `fetch` for all applicable years
- 🚨 **MUST** re-run `import` command
  - Prior to re-importing, remove your previously imported `2-preprocessed` and `3-journal` directories
  - Run `import` starting from oldest applicable year
- 🚨 **MUST** re-run `taxes` command
  - Run `taxes` for all applicable years

> Tip: when in doubt, regenerate your entire setup. Your environment and accounts will be backed up in the process.

---

### 🧠 Contributors

- Aaron Fiore

### 🛠️ Fixes / Updates

#### Repo

- README.md
  - Update keyserver for repo verification ([#57](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/57))
  - Various updates/fixes ([#98](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/98))

#### Client

- Dockerfiles fixes related to `import` (coinbase/coinbase-pro) and Ubuntu-based `build` ([#58](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/58))
- Resolve Dockerfiles `apt` CLI warning ([#82](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/82))
- Append subprofile source to superscript before generating ([#87](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/87))
- Rewrite/fix global env gen, optimize/refactor repository path, usage update ([#90](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/90)) ([#92](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/92)) ([#97](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/97))

#### Container

- Algorand
  - Add more income (Governance Rewards and spam) to `rules` ([#46](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/46)) ([#54](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/54))
- Ally
  - 'Description' fixes (`preprocess`) ([#53](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/53))
  - Various additions to `rules` ([#53](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/53))
- Capital One
  - **Breaking:** Upstream column-order changes ([#80](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/80)) ([#93](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/93))
- Coinbase
  - **Breaking:** SIWC API (V2) compliance ([#49](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/49)) ([#50](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/50)) ([#63](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/63))
  - **Breaking:** Full migration to CCXT ([#75](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/75))
  - Add 1INCH support ([#56](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/56))
- Electrum
  - Various additions to `rules` ([#43](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/43))
- Ethereum-based
  - Add Aave V2 -> V3 Migration Helper support ([#45](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/45))
  - Add spam to `rules` ([#45](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/45)) ([#55](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/55)) ([#91](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/91))
- Gemini (Exchange)
  - Allow optional on-chain transfers ([#52](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/52))
  - Add 1INCH support ([#56](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/56))
- Ledger (Live)
  - **Breaking:** Upstream column addition ([#73](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/73)) ([#93](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/93))
- `lib_edit`
  - Fix returns, clearer fatal errors ([#81](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/81))
- `lib_taxes` / `hledger-flow`
  - **Breaking:** Cost-basis work-arounds for Bitcoin.tax ([#96](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/96))

### ☀️ Features / Enhancements

#### Repo

- Add Gitea templates ([#65](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/65)) ([#69](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/69))
- Add [CHANGELOG.md](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/src/branch/master/CHANGELOG.md) ([#99](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/99))

#### Client

- Add `build` option types and build-specific features ([#68](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/68)) ([#70](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/70)) ([#79](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/79)) ([#85](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/85)) ([#86](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/86))
  - Types: `default` | `slim` | `tiny` | `micro` | `experimental`
  - Separation of concerns for build types
  - Support ROOT.cern for Ubuntu
  - Support `hledger` source build
  - Support `hledger-flow` binary download
  - Optimize image sizes
- Add `version` command for all platforms ([#61](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/61)) ([#83](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/83))
- **Breaking:** Add custom Dockerfile support ([#67](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/67)) ([#97](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/97))
- **Breaking:** New internal versioning implementation ([#84](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/84)) ([#88](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/88)) ([#100](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/100))

#### Client / Container

- `hledger` related enhancements, add alternative terminal UI (visidata) ([#64](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/64))
- Add `hledger --conf` support, future-proofing for `hledger` features ([#78](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/78))

#### Container

- `hledger-flow`
  - Add support for AdaLite ([#40](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/40))
  - PayPal Business: catch 'Account Hold' ([#47](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/47))
- `price`
  - **Breaking:** Add support for multiple crypto aggregator-APIs ([#60](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/60))
    - **Breaking:** Add API keys/blockchains to `fetch` configuration
    - Add support for Mobula
    - Add support for CoinGecko Pro
- `root`
  - Add internal throw/exception handler / unit test ([#89](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/89))
  - Macro reorg, refactor ([#94](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/94))
  - Add crypto macros (`Hash`, `Random`) ([#95](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/95))
  - Add support for multiple random number types (`Random`), update tests ([#95](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/95))

### 🌴 Misc / Other

#### Client

- Dockerfiles: remove obsolete 'version' element ([#42](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/42))
- `lib_docker`: factor out args parsers ([#71](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/71))
- `lib_gen`: return success on client generation ([#76](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/76))

#### Container

- `hledger-flow`: btcpayserver: preprocess: fix YYY comment typo ([#44](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/44))
- bash: `lib_fetch`: create prices dir if nonexistent ([#101](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/101))
- php: `fetch`: gemini: update to ccxt's latest request() signature | run linter ([#62](https://gitea.evergreencrypto.co/EvergreenCrypto/docker-finance/pulls/62))

## 1.0.0-beta - 2024-03-04

Initial pre-release
