//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/model/gsymbol.h"
#include "gate/optimizer/visitor.h"

#include <fstream>

class Dot {
public:
  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using GateSymbol = eda::gate::model::GateSymbol;
  //using MatchMap = eda::gate::optimizer::Visitor::MatchMap;

  Dot(const GNet *gNet);
  void print(const std::string &filename) const;
  void print(std::ofstream &stream) const;
  void printColor(const std::string &filename, const eda::gate::optimizer::Visitor::MatchMap &coneGates) const;

private:
  const GNet *gNet;
  void print(std::ofstream &stream, const Gate *gate) const;
  void print(std::ofstream &stream, const Gate *gate, const eda::gate::optimizer::Visitor::MatchMap &coneGates) const;
  static std::vector<std::string> funcNames;
};

