#include "node.h"

void PrettyPrint(const std::shared_ptr<Node>& node, const RawFilterData& rf_data, const std::string& prefix,
                 bool isLeft, std::ostream& os) {
    if (!node) {
        os << prefix << (isLeft ? "├── " : "└── ") << "NULL\n";
        return;
    }

    if (node->type == NodeType::FAIL) {
        os << prefix << (isLeft ? "├── " : "└── ") << "FAIL\n";
        return;
    }

    if (node->type == NodeType::PARSE) {
        os << prefix << (isLeft ? "├── " : "└── ") << "PARSE\n";
        return;
    }

    os << prefix << (isLeft ? "├── " : "└── ")
       << rf_data.data[node->conjunction_idx][node->predicate_idx][node->raw_filter_idx] << "\n";

    auto newPrefix = prefix + (isLeft ? "│   " : "    ");

    PrettyPrint(node->left, rf_data, newPrefix, true, os);
    PrettyPrint(node->right, rf_data, newPrefix, false, os);
}
