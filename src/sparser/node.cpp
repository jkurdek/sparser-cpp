#include "node.h"

#include <memory>
#include <ostream>
#include <string>

#include "raw_filter.h"

// NOLINTNEXTLINE
void PrettyPrint(const std::shared_ptr<Node>& node, const RawFilterData& rf_data, const std::string& prefix,
                 bool isLeft, std::ostream& outStream) {
    if (!node) {
        outStream << prefix << (isLeft ? "├── " : "└── ") << "NULL\n";
        return;
    }

    if (node->type == NodeType::FAIL) {
        outStream << prefix << (isLeft ? "├── " : "└── ") << "FAIL\n";
        return;
    }

    if (node->type == NodeType::PARSE) {
        outStream << prefix << (isLeft ? "├── " : "└── ") << "PARSE\n";
        return;
    }

    outStream << prefix << (isLeft ? "├── " : "└── ")
       << rf_data.data.at(node->conjunction_idx).at(node->predicate_idx).at(node->raw_filter_idx) << "\n";

    auto newPrefix = prefix + (isLeft ? "│   " : "    ");

    PrettyPrint(node->left, rf_data, newPrefix, true, outStream);
    PrettyPrint(node->right, rf_data, newPrefix, false, outStream);
}
