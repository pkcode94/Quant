# docker-finance | modern accounting for the power-user
#
# Copyright (C) 2021-2026 Aaron Fiore (Founder, Evergreen Crypto LLC)
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

FROM haskell:9.12.2 AS haskell-build

RUN apt-get update -y
RUN apt-get upgrade -y

#
# hledger-flow
#

FROM haskell-build AS hledger-flow

WORKDIR /usr/local/src
RUN git clone --depth=1 https://github.com/apauley/hledger-flow -b v0.16.2

WORKDIR /usr/local/src/hledger-flow
RUN stack setup
RUN stack install --install-ghc
RUN stack install --dependencies-only
RUN stack install --local-bin-path=/usr/local/bin

#
# hledger-iadd
#

FROM hledger-flow AS hledger-iadd

WORKDIR /usr/local/src
RUN git clone --depth=1 https://github.com/hpdeifel/hledger-iadd -b v1.3.22

WORKDIR /usr/local/src/hledger-iadd
RUN cabal update
RUN cabal v1-install --only-dependencies
RUN cabal v1-configure --bindir /usr/local/bin
RUN cabal v1-build
RUN cabal v1-copy

#
# hledger / hledger-ui / hledger-web
#

FROM hledger-iadd AS hledger

WORKDIR /usr/local/src
RUN git clone --depth=1 https://github.com/simonmichael/hledger -b 1.51.2

WORKDIR /usr/local/src/hledger
RUN stack setup
RUN stack install --stack-yaml=stack.yaml --dependencies-only
RUN stack install --stack-yaml=stack.yaml --local-bin-path=/usr/local/bin

#
# Production stage
#

FROM debian:stable-slim AS hledger-suite

COPY --from=hledger /usr/local/bin/hledger* /usr/local/bin/

# vim: sw=2 sts=2 si ai et
