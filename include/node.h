#ifndef NODE_H_
#define NODE_H_

#include <cstdint>
#include <iostream>
#include <memory>

#include "raw_filter.h"
enum class NodeType { INTER, FAIL, PARSE };

struct Node {
    uint32_t conjunction_idx;
    uint32_t predicate_idx;
    uint32_t raw_filter_idx;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    NodeType type;

    Node(uint32_t conj_idx, uint32_t pred_idx, uint32_t rf_idx, std::shared_ptr<Node> left_subtree,
         std::shared_ptr<Node> right_subtree, NodeType node_type)
        : conjunction_idx(conj_idx),
          predicate_idx(pred_idx),
          raw_filter_idx(rf_idx),
          left(left_subtree),
          right(right_subtree),
          type(node_type) {}
};

void PrettyPrint(const std::shared_ptr<Node>& node, const RawFilterData& rf_data, const std::string& prefix = "",
                 bool isLeft = true, std::ostream& os = std::cout);

#endif  // NODE_H_
