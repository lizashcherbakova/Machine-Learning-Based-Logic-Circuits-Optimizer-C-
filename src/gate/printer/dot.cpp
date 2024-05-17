//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/printer/dot.h"

std::vector<std::string> Dot::funcNames = [] {
  std::vector<std::string> names;
  names.reserve(GateSymbol::XXX + 1);

  auto insert = [&names](GateSymbol gs, std::string &&name) {
    if (names.size() <= gs) {
      names.resize(gs + 1);
      names[gs] = std::move(name);
    }
  };

  insert(GateSymbol::IN, "IN");
  insert(GateSymbol::OUT, "OUT");
  insert(GateSymbol::ZERO, "ZERO");
  insert(GateSymbol::ONE, "ONE");
  insert(GateSymbol::NOP, "NOP");
  insert(GateSymbol::NOT, "NOT");
  insert(GateSymbol::AND, "AND");
  insert(GateSymbol::OR, "OR");
  insert(GateSymbol::XOR, "XOR");
  insert(GateSymbol::NAND, "NAND");
  insert(GateSymbol::NOR, "NOR");
  insert(GateSymbol::XNOR, "XNOR");
  insert(GateSymbol::MAJ, "MAJ");
  insert(GateSymbol::LATCH, "LATCH");
  insert(GateSymbol::DFF, "DFF");
  insert(GateSymbol::DFFrs, "DFFrs");
  insert(GateSymbol::XXX, "XXX");
  return names;
}();

Dot::Dot(const Dot::GNet *gNet) : gNet(gNet) {}

void Dot::print(const std::string &filename) const {
  std::ofstream out(filename);
  if (out.is_open()) {
    print(out);
    out.close();
  } else {
    std::cerr << "Failed to create file : " << filename << std::endl;
  }
}

void Dot::print(std::ofstream &stream) const {
  stream << "digraph substNet {" << std::endl;
  for (const auto &gate: gNet->gates()) {
    if (gate->links().empty()) {
      stream << "\t";
      print(stream, gate);
      stream << ";" << std::endl;
    }
    for (const auto &links: gate->links()) {
      stream << "\t";
      print(stream, gate);
      stream << " -> ";
      print(stream, Gate::get(links.target));
      stream << ";" << std::endl;
    }
  }
  stream << "}" << std::endl;
}

void Dot::printColor(const std::string &filename, const eda::gate::optimizer::Visitor::MatchMap &coneGates) const {
  std::ofstream out(filename);
  if (out.is_open()) {
    out << "digraph substNet {" << std::endl;
    for (const auto &gate: gNet->gates()) {
      //if (gate->links().empty()) {
        out << "\t";
        print(out, gate, coneGates);
        out << ";" << std::endl;
      // }
      for (const auto &link : gate->links()) {
        out << "\t";
        print(out, gate);
        out << " -> ";
        print(out, Gate::get(link.target));
        out << ";" << std::endl;
      }
    }
    out << "}" << std::endl;
    out.close();
  } else {
    std::cerr << "Failed to create file : " << filename << std::endl;
  }
}

void Dot::print(std::ofstream &stream, const Gate *gate) const {
  if(funcNames.size() > gate->func()) {
    stream << funcNames[gate->func()];
  }
  stream << gate->id();
}

void Dot::print(std::ofstream &stream, const Gate *gate, const eda::gate::optimizer::Visitor::MatchMap &coneGates) const {
  print(stream, gate);
  auto it = coneGates.find(gate->id());
  if (it != coneGates.end()) {
    // If the gate is in coneGates, print with special format
    stream <<" [label=\"" << gate->id() << "(" << it->second << ", " << funcNames[gate->func()] << ")\", color=red, style=filled]";
  }
}

