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

#ifndef CONTAINER_SRC_ROOT_MACRO_WEB_INTERNAL_META_C_
#define CONTAINER_SRC_ROOT_MACRO_WEB_INTERNAL_META_C_

#include <TCanvas.h>
#include <TGraph.h>
#include <TPie.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

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
//! \brief Meta processing macro class
//! \ingroup cpp_macro_impl
//! \since docker-finance 1.0.0
class Meta final
{
 public:
  Meta() = default;
  ~Meta() = default;

  Meta(const Meta&) = default;
  Meta& operator=(const Meta&) = default;

  Meta(Meta&&) = default;
  Meta& operator=(Meta&&) = default;

 private:
  //! \since docker-finance 1.0.0
  struct CanvasData
  {
    std::string column;
    std::pair<std::string, std::string> title;  // canvas, object
    std::map<std::string, uint64_t> row;
  };

 private:
  //! \brief Make pie graph from given CSV data
  //! \param csv Imported CSV data
  //! \param data Populated canvas data for canvas creation
  static void make_pie(
      std::shared_ptr<ROOT::RDataFrame> csv,
      const Meta::CanvasData& data)
  {
    // Setup pie data
    std::vector<const char*> labels;
    std::vector<::Float_t> values;
    std::vector<::Int_t> colors;

    // Populate all pie data from map
    for (auto it = data.row.cbegin(); it != data.row.cend(); ++it)
      {
        const auto& label = it->first;
        const auto& frequency = it->second;

        std::cout << "label (row's value) = " << label
                  << " | frequency (row's value frequency) = " << frequency
                  << "\n";

        labels.push_back(label.c_str());
        values.push_back(frequency);
        colors.push_back(std::distance(data.row.cbegin(), it));
      }

    // Make canvas and graph
    auto* canvas = new ::TCanvas(
        std::string{data.title.first + "_tpie"}.c_str(),
        std::string{"Metadata (" + data.column + ")"}.c_str());

    canvas->Draw();

    auto* pie = new ::TPie(
        "pie",
        data.title.second.c_str(),
        values.size(),
        values.data(),
        colors.data());

    pie->SetLabels(labels.data());
    pie->SetLabelsOffset(-.1);

    // TODO(unassigned): why does changing these options have no apparent effect?
    // pie->Draw("3DTNOL");
    // pie->Draw("TNOL");
    pie->Draw();

    // TODO(unassigned): put count and percentages over the slices

    // Make legend
    auto* legend = pie->MakeLegend();
    legend->SetHeader(data.column.c_str(), "C");
    legend->Draw();
  }

  //! \brief Make graph from given CSV data
  //! \param csv Imported CSV data
  //! \param data Populated canvas data for canvas creation
  static void make_graph(
      std::shared_ptr<ROOT::RDataFrame> csv,
      const Meta::CanvasData& data)
  {
    // Graph data
    auto defined = csv->Define(
        "DateAdded",
        "std::tm time{}; std::istringstream ss(date_added);"
        "ss >> std::get_time(&time, \"%Y-%m-%d %H:%M:%S\");"
        "return std::mktime(&time);");  // Seconds from epoch

    // TODO(unassigned): empty row will return "nan"
    auto date = defined.Take<std::time_t>("DateAdded");
    auto col = defined.Take<std::string>(data.column);

    // Canvas
    auto* canvas = new ::TCanvas(
        std::string{data.title.first + "_tgraph"}.c_str(),
        std::string{"Metadata (" + data.column + ")"}.c_str());

    canvas->Draw();

    // Graph
    std::vector<::Float_t> x, y;

    for (size_t i{}; i < data.row.size(); i++)
      x.push_back(date->at(i));

    for (const auto& [label, frequency] : data.row)
      y.push_back(frequency);

    auto* graph = new ::TGraph(x.size(), x.data(), y.data());
    graph->SetTitle(data.title.second.c_str());
    graph->SetMinimum(0);
    graph->SetFillColor(kBlue);
    graph->GetXaxis()->SetTimeDisplay(1);
    graph->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M:%S");
    graph->GetXaxis()->SetTimeOffset(0, "local");
    graph->Draw("B");
  }

 public:
  //! \brief Sample metadata from given CSV column
  //! \param column Existing CSV metadata column
  static void meta_sample(const std::string& column)
  {
    namespace common = ::dfi::common;

    // Import meta file
    const std::string path{common::get_env("global_conf_meta")};
    auto csv =
        std::make_shared<ROOT::RDataFrame>(ROOT::RDF::FromCSV(path.c_str()));

    // Ensure viable header
    std::string available{"\n\t"};
    for (const auto& col : csv->GetColumnNames())
      available += "\n\t" + col;
    available += "\n";

    common::throw_ex_if<common::type::RuntimeError>(
        !csv->HasColumn(column), "\n\nAvailable columns: " + available);

    // Aggregate all rows of given column
    auto count = csv->Count();
    common::throw_ex_if<common::type::RuntimeError>(!*count, "Empty CSV");

    // Prepare canvas data
    Meta::CanvasData data;

    const std::string timestamp{common::make_timestamp()},
        parent{common::get_env("global_parent_profile")},
        child{common::get_env("global_child_profile")};

    data.column = column;
    data.title.first = timestamp + "_meta_" + data.column;
    data.title.second =
        std::string{"Meta [" + parent + "/" + child + "] | " + timestamp};

    // Map row values and frequency
    auto col = csv->Take<std::string>(data.column);
    for (size_t i{}; i < *count; i++)
      ++data.row[col->at(i)];

    // Execute
    Meta::make_pie(csv, data);
    Meta::make_graph(csv, data);
  }
};
}  // namespace internal
}  // namespace web
}  // namespace macro
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_MACRO_WEB_INTERNAL_META_C_

// # vim: sw=2 sts=2 si ai et
