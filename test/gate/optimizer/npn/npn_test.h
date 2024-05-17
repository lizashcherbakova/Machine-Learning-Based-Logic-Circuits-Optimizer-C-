//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/npn/npn_collector.h"
#include "../../../gate/model/examples.h"

#include "gtest/gtest.h"

#include <fstream>
#include <chrono>
#include <ctime>

namespace eda::gate::optimizer::npn {

  void printNetCones(const std::filesystem::path &path,
                     const std::unordered_map<uint64_t, std::vector<std::shared_ptr<GNet>>> &conesMap,
                     GNet *gNet = nullptr, const std::string &name = "");

  void printCones(const std::filesystem::path &path,
                      std::shared_ptr<GNet> cone,
                      const std::string &name = "", GNet *gNet = nullptr);

}  // namespace eda::gate::optimizer