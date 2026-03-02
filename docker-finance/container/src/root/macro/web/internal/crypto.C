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

//! \file
//! \author Aaron Fiore (Founder, Evergreen Crypto LLC)
//! \note File intended to be loaded into ROOT.cern framework / Cling interpreter
//! \since docker-finance 1.0.0

#ifndef CONTAINER_SRC_ROOT_MACRO_WEB_INTERNAL_CRYPTO_C_
#define CONTAINER_SRC_ROOT_MACRO_WEB_INTERNAL_CRYPTO_C_

#include <TCanvas.h>
#include <TGraph.h>

#include <cmath>
#include <iomanip>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "../../common/crypto.hh"
#include "../../common/utility.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::macro
//! \brief ROOT macros
//! \since docker-finance 1.0.0
namespace macro
{
//! \namespace dfi::macro::web
//! \brief ROOT web-based macros
//! \since docker-finance 1.0.0
namespace web
{
//! \namespace dfi::macro::web::internal
//! \brief ROOT web server dfi macros for internal use only
//! \details Primarily implementation for public-consuming macros
//! \since docker-finance 1.0.0
namespace internal
{
//! \ingroup cpp_macro_impl
//! \brief RNG analysis macro
//! \since docker-finance 1.0.0
class Random final
{
 public:
  Random() = default;
  ~Random() = default;

  Random(const Random&) = default;
  Random& operator=(const Random&) = default;

  Random(Random&&) = default;
  Random& operator=(Random&&) = default;

 private:
  //! \since docker-finance 1.0.0
  struct CanvasData
  {
    ::Int_t n{};  // points
    std::vector<::Double_t> x, y;
    std::string title, label;
  };

 private:
  //! \brief The "random device" implementation
  //! \since docker-finance 1.0.0
  template <typename t_gen, typename t_num>
  struct GeneratorImpl
  {
    explicit GeneratorImpl(const t_gen& gen) : m_gen(gen) {}

    //! \brief As expected from std::random_device except secure numbers are generated
    t_num operator()() const { return m_gen(); }

    //! \brief As implemented in std::random_device
    t_num min() const { return std::numeric_limits<t_num>::min(); }

    //! \brief As implemented in std::random_device
    t_num max() const { return std::numeric_limits<t_num>::max(); }

    t_gen m_gen;
  };

  //! \brief The "random device" facade
  //! \since docker-finance 1.0.0
  template <typename t_gen, typename t_num>
  struct Generator
  {
    explicit Generator(const t_gen& gen) : m_impl(gen) {}

    const auto& operator()() const { return m_impl; }

    GeneratorImpl<t_gen, t_num> m_impl;
  };

 protected:
  //! \brief Make graph of values from given canvas data
  //! \param canvas Previously instantiated canvas
  //! \param data Populated canvas data for graph
  static void graph_value(::TCanvas* canvas, const Random::CanvasData& data)
  {
    // Graph: w/ lines
    auto* gr1 = new ::TGraph(data.n, data.x.data(), data.y.data());

    gr1->SetTitle(std::string(data.title + data.label).c_str());
    gr1->SetMarkerColor(kBlue);
    gr1->SetMarkerStyle(kFullDotSmall);

    gr1->GetXaxis()->SetTitle("Count");
    gr1->GetXaxis()->SetRangeUser(
        0, *std::max_element(data.x.cbegin(), data.x.cend()));

    gr1->GetYaxis()->SetTitle("Value [0..0xffffffff]");
    gr1->GetYaxis()->SetRangeUser(
        0, *std::max_element(data.y.cbegin(), data.y.cend()));

    canvas->cd(1)->SetGridx();
    canvas->cd(1)->SetGridy();

    gStyle->SetLineStyle(kDotted);
    gStyle->SetLineWidth(1);

    gr1->Draw("APL");

    // Graph: w/ bar
    canvas->cd(2)->SetGridy();
    gr1->Draw("AB");
  }

  //! \brief Make graph of value occurrences from given canvas data
  //! \param canvas Previously instantiated canvas
  //! \param data Populated canvas data for graph
  static void graph_frequency(::TCanvas* canvas, const Random::CanvasData& data)
  {
    // x = [0..0xffffffff]
    // y = frequency of number (max = nbins)
    std::vector<::Double_t> x1;
    std::vector<::Double_t> y1;

    std::map<::Double_t, ::Double_t> map;
    for (const auto& y : data.y)
      {
        ++map[y];  // y = x
      }

    // .first = rand value
    // .second = frequency
    for (const auto& [x, y] : map)
      {
        x1.push_back(x);
        y1.push_back(y);
      }

    auto* gr2 = new ::TGraph(data.n, x1.data(), y1.data());
    gr2->SetTitle(std::string(data.title + data.label).c_str());

    gr2->GetXaxis()->SetTitle("Value [0..0xffffffff]");
    gr2->GetXaxis()->SetRangeUser(0, *std::max_element(x1.cbegin(), x1.cend()));

    gr2->GetYaxis()->SetTitle("Count");
    gr2->GetYaxis()->SetTickLength(0);
    gr2->GetYaxis()->SetRangeUser(0, *std::max_element(y1.cbegin(), y1.cend()));

    canvas->cd(3);
    gr2->Draw("AB");
  }

  //! \brief Make 2D graph of prime numbers within given canvas data
  //! \param canvas Previously instantiated canvas
  //! \param data Populated canvas data for graph
  static void graph_prime(::TCanvas* canvas, const Random::CanvasData& data)
  {
    canvas->cd(4);
    auto* gr1 = new ::TGraph2D();

    size_t primes{};
    bool is_prime{true};

    for (size_t n{}; n < data.n; n++)
      {
        const auto y = static_cast<uint32_t>(data.y.at(n));
        is_prime = true;

        if (y < 2)
          is_prime = false;

        for (size_t i{2}; i <= std::sqrt(y); ++i)
          {
            if (!(y % i))
              {
                is_prime = false;
                break;
              }
          }

        if (is_prime)
          {
            auto x = n;
            auto z = y;
            gr1->SetPoint(n, x, y, z);
            primes++;
          }
      }

    gr1->SetTitle(
        std::string(data.title + data.label + " (" + primes + " primes)")
            .c_str());
    gStyle->SetPalette(1);
    gr1->SetMarkerStyle(20);
    gr1->Draw("pcol");
  }

  //! \brief Make histogram of values from given canvas data / random number generator
  //! \param canvas Previously instantiated canvas
  //! \param data Populated canvas data for graph
  //! \param generator Random number generator to use
  template <typename t_generator>
  static void histo_gauss(
      ::TCanvas* canvas,
      const Random::CanvasData& data,
      const t_generator& generator)
  {
    canvas->cd(1);
    canvas->SetGridx();
    canvas->SetGridy();

    auto* h1 = new ::TH1D(
        data.title.c_str(),
        std::string(data.title + data.label).c_str(),
        data.n,
        -5.0,
        5.0);  // TODO(unassigned): looks the best, would prefer more mathematical soundness

    // Shove a CSPRNG into a Gaussian distribution
    const Random::Generator<decltype(generator), uint32_t> gen(generator);
    std::normal_distribution dist{0.0, 1.0};

    for (size_t i{}; i < data.n; ++i)
      {
        h1->Fill(dist(gen()));
      }

    h1->Draw();
  }

  static void fun_facts(::TCanvas* canvas, const Random::CanvasData& data)
  {
    // TODO(unassigned): percent of prime / composite

    canvas->cd(2);
    auto* pt = new ::TPaveText(.05, .1, .95, .9);

    // odd/even
    size_t is_odd{};
    for (const auto& y : data.y)
      {
        if (static_cast<uint32_t>(y) % 2)
          {
            is_odd++;
          }
      }

    // miles
    uint64_t miles{};
    for (const auto& y : data.y)
      {
        miles += y;
      }

    miles /= 24901;

    // TODO(unassigned): would be nice to have some RNG-related info
    pt->AddText(
        std::string{"Fun facts"}
            .c_str());  // TODO(unassigned): make this small text
    pt->AddText(
        std::string{
            "There are " + std::to_string(is_odd) + " odd numbers and "
            + std::to_string(data.y.size() - is_odd) + " even numbers"}
            .c_str());
    pt->AddText(
        std::string{
            "If the total values generated were miles, you could circle the "
            "earth "
            + std::to_string(miles) + " times!"}
            .c_str());
    pt->Draw();
  }

 public:
  // TODO(unassigned): put in a `Random` subdir to separate from other functions
  static void rng_sample(const std::string& size)
  {
    Random::CanvasData data{};

    // TODO(unassigned): sanitize because:
    //   1. only unsigned is desired
    //   2. server will crash if too large...
    data.n = std::stoi(size);
    data.x.resize(data.n);
    data.y.resize(data.n);

    auto random =
        ([](Random::CanvasData /* make copy */ data, const auto& generator) {
          auto* c1 = new ::TCanvas(
              std::string{"c1_" + data.title}.c_str(), "RNG analysis");

          // TODO(unassigned): shift plots to the right so there's more space on the left and less on the right
          c1->Divide(2, 2);
          c1->SetFillColorAlpha(kBlue, 0.35);

          // Create graph data
          for (size_t i{}; i < data.n; i++)
            {
              data.x.at(i) = i;
              data.y.at(i) = generator();
            }

          Random::graph_value(c1, data);
          Random::graph_frequency(c1, data);
          Random::graph_prime(c1, data);

          auto* c2 = new ::TCanvas(
              std::string{"c2_" + data.title}.c_str(), "RNG analysis");

          // TODO(unassigned): shift plots to the right so there's more space on the left and less on the right
          c2->Divide(0, 2);
          Random::histo_gauss<decltype(generator)>(c2, data, generator);
          Random::fun_facts(c2, data);
        });

    namespace common = ::dfi::macro::common;

    const std::string timestamp{common::make_timestamp()};

#ifdef __DFI_PLUGIN_BITCOIN__
    data.title = "Bitcoin_FastRandomContext_32_RNG_" + timestamp;
    random(data, []() -> uint32_t {
      return common::crypto::bitcoin::g_Random->generate();
    });

    // TODO(afiore): uint64_t (requires canvas-related refactor)
#endif

    data.title = "Botan_RNG_" + timestamp;
    random(data, []() -> uint32_t {
      return common::crypto::botan::g_Random->generate();
    });

    data.title = "CryptoPP_RNG_" + timestamp;
    random(data, []() -> uint32_t {
      return common::crypto::cryptopp::g_Random->generate();
    });

    data.title = "libsodium_RNG_" + timestamp;
    random(data, []() -> uint32_t {
      return common::crypto::libsodium::g_Random->generate();
    });

    // TODO(unassigned): when clicking reload to create another sample, it doesn't do a clean refresh of the canvas
  }
};
}  // namespace internal
}  // namespace web
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_WEB_INTERNAL_CRYPTO_C_

// # vim: sw=2 sts=2 si ai et
