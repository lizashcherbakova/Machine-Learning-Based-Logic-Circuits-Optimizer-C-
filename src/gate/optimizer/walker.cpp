//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/walker.h"

namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;

  Walker::Walker(const Walker::GNet *gNet, Visitor *visitor) :
      gNet(gNet), visitor(visitor) {}


  void Walker::walk(const std::vector<GateId> &nodes, bool forward) {
    auto begin = forward ? nodes.begin() : std::prev(nodes.end());
    auto end = forward ? nodes.end() : std::prev(nodes.begin(), -1);

    for (auto it = begin; it != end; forward ? ++it : --it) {
      const auto &node = *it;
      // Only FINISH_ALL_NODES, CONTINUE expected.
      switch (callVisitor(node)) {
        case FINISH_ALL_NODES:
          return;
        case CONTINUE:
        case SKIP:
        case FINISH_FURTHER_NODES:
          break;
        default:
          std::cerr << "Unexpected flag in Walker." << std::endl;
          return;
      }
    }
  }

  void Walker::walk(bool forward) {
    auto nodes = utils::graph::topologicalSort(*gNet);

    if (!forward) {
      std::reverse(nodes.begin(), nodes.end());
    }

    for (auto &node: nodes) {
      // Only FINISH_ALL_NODES, CONTINUE expected.
      switch (callVisitor(node)) {
        case FINISH_ALL_NODES:
          return;
        case CONTINUE:
        case SKIP:
        case FINISH_FURTHER_NODES:
          break;
        default:
          std::cerr << "Unexpected flag in Walker." << std::endl;
          return;
      }
    }
  }

  void Walker::walk(GateId start, const Cut &cut, bool forwardCone) {
    std::unordered_set<GateId> accessed;

    // First trace to define needed nodes.
    getConeSet(start, cut, accessed, forwardCone);

    std::queue<GateId> bfs;
    bfs.push(start);

    // Second trace to visit needed nodes in topological order.
    walk(bfs, accessed, forwardCone);
  }

  void Walker::walk(Walker::GateId start, bool forward) {
    std::unordered_set<GateId> accessed;

    // First trace to define needed nodes.
    getConeSet(start, accessed, forward);

    std::queue<GateId> bfs;
    bfs.push(start);

    // Second trace to visit needed nodes in topological order.
    walk(bfs, accessed, forward);
  }

  void Walker::walk(const Walker::Cut &start,
                    Walker::GateId end, bool forward) {
    std::unordered_set<GateId> accessed;

    // First trace to define needed nodes.
    getConeSet(end, start, accessed, forward);

    std::queue<GateId> bfs;
    for (const auto &node: start) {
      bfs.push(node);
    }
    // Second trace to visit needed nodes in topological order
    // from cut to the node.
    walk(bfs, accessed, !forward);
  }

  void Walker::walk(std::queue<GateId> &bfs,
                    std::unordered_set<GateId> &accessed,
                    bool forward) {
    while (!bfs.empty()) {
      auto cur = bfs.front();

      if (accessed.find(cur) != accessed.end()) {
        if (checkVisited(accessed, cur, forward)) {

          accessed.erase(cur);
          auto next = getNext(cur, forward);

          // Only FINISH_ALL_NODES, FINISH_FURTHER_NODES and CONTINUE expected.
          auto flag = callVisitor(cur);
          switch (flag) {
            case FINISH_ALL_NODES:
              return;
            case FINISH_FURTHER_NODES:
              continue;
            case CONTINUE:
            case SKIP:
              break;
            default:
              std::cerr << "Unexpected flag in Walker." << std::endl;
              return;
          }

          for (auto node: next) {
            bfs.push(node);
          }

          if (flag == SKIP) {
            continue;
          }

        } else {
          auto prev = getNext(cur, !forward);
          for (auto node: prev) {
            if (accessed.find(node) != accessed.end()) {
              bfs.push(node);
            }
          }
        }
      }
      bfs.pop();
    }
  }

  void Walker::walkAll(std::queue<GateId> &bfs,
                       const std::unordered_set<GateId> &used, bool forward) {
    std::unordered_set<GateId> visited;

    while (!bfs.empty()) {
      auto cur = bfs.front();

      if (visited.find(cur) == visited.end()) {
        if (checkAllVisited(visited, used, cur, forward)) {

          visited.emplace(cur);
          auto next = getNext(cur, forward);

          // Only FINISH_ALL_NODES, FINISH_FURTHER_NODES and CONTINUE expected.
          auto flag = callVisitor(cur);
          switch (flag) {
            case FINISH_ALL_NODES:
              return;
            case FINISH_FURTHER_NODES:
              continue;
            case CONTINUE:
            case SKIP:
              break;
            default:
              std::cerr << "Unexpected flag in Walker." << std::endl;
          }

          for (auto node: next) {
            bfs.push(node);
          }

          if (flag == SKIP) {
            continue;
          }

        } else {
          auto prev = getNext(cur, !forward);
          for (auto node: prev) {
            if (visited.find(node) == visited.end()) {
              bfs.push(node);
            }
          }
        }
      }
      bfs.pop();
    }
  }

  VisitorFlags Walker::callVisitor(GateId node) {
    auto flag = visitor->onNodeBegin(node);

    if (flag != CONTINUE) {
      return flag;
    }

    return visitor->onNodeEnd(node);
  }

  bool Walker::checkAllVisited(const std::unordered_set<GateId> &visited,
                               const std::unordered_set<GateId> &used,
                               GateId node, bool forward) {
    if (forward) {
      const auto &inputs = Gate::get(node)->inputs();
      for (const auto &in: inputs) {
        if (visited.find(in.node()) == visited.end() &&
            used.find(in.node()) == used.end()) {
          return false;
        }
      }
    } else {
      const auto &outputs = Gate::get(node)->links();
      for (const auto &out: outputs) {
        if (visited.find(out.target) == visited.end() &&
            used.find(out.target) == used.end()) {
          return false;
        }
      }
    }
    return true;
  }

  bool Walker::checkVisited(const std::unordered_set<GateId> &accessed,
                            GateId node, bool forward) {
    if (forward) {
      const auto &inputs = Gate::get(node)->inputs();
      for (const auto &in: inputs) {
        if (accessed.find(in.node()) != accessed.end()) {
          return false;
        }
      }
    } else {
      const auto &outputs = Gate::get(node)->links();
      for (const auto &out: outputs) {
        if (accessed.find(out.target) != accessed.end()) {
          return false;
        }
      }
    }
    return true;
  }

} // namespace eda::gate::optimizer