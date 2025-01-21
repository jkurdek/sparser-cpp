#include "cascade_builder.h"

std::vector<std::shared_ptr<Node>> CascadeBuilder::GenerateValidCascades() { return HandleFail(0); }

std::vector<std::shared_ptr<Node>> CascadeBuilder::HandleFail(const size_t current_depth) {
    if (used_conjunctions_.count() == disjunction_.conjunctions.size()) {
        std::vector<std::shared_ptr<Node>> result;
        result.emplace_back(fail_node);
        return result;
    }

    std::vector<std::shared_ptr<Node>> valid_subtrees;

    for (size_t conj_idx = 0; conj_idx < rf_data_.conj_count; conj_idx++) {
        if (!used_conjunctions_.test(conj_idx)) {
            used_conjunctions_.set(conj_idx);

            for (size_t pred_idx = 0; pred_idx < rf_data_.pred_count[conj_idx]; pred_idx++) {
                for (size_t rf_idx = 0; rf_idx < rf_data_.rf_count[conj_idx][pred_idx]; rf_idx++) {
                    if (!used_rfs_[conj_idx][pred_idx].test(rf_idx)) {
                        used_rfs_[conj_idx][pred_idx].set(rf_idx);

                        auto valid_left_subtrees = HandleFail(current_depth + 1);
                        auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                        for (auto left_subtree : valid_left_subtrees) {
                            for (auto right_subtree : valid_right_subtrees) {
                                auto root = std::make_shared<Node>(conj_idx, pred_idx, rf_idx, left_subtree,
                                                                   right_subtree, NodeType::INTER);
                                valid_subtrees.emplace_back(root);
                            }
                        }

                        used_rfs_[conj_idx][pred_idx].reset(rf_idx);
                    }
                }
            }

            used_conjunctions_.reset(conj_idx);
        }
    }

    return valid_subtrees;
}

std::vector<std::shared_ptr<Node>> CascadeBuilder::HandleSuccess(const size_t current_depth, const size_t conj_idx) {
    int conj_left = disjunction_.conjunctions.size() - used_conjunctions_.count();
    int free = kMaxDepth - current_depth - conj_left;

    std::vector<std::shared_ptr<Node>> valid_subtrees;
    valid_subtrees.emplace_back(parse_node);  // Finishing here is valid

    if (free > 0) {
        for (size_t pred_idx = 0; pred_idx < rf_data_.pred_count[conj_idx]; pred_idx++) {
            for (size_t rf_idx = 0; rf_idx < rf_data_.rf_count[conj_idx][pred_idx]; rf_idx++) {
                if (!used_rfs_[conj_idx][pred_idx].test(rf_idx)) {
                    used_rfs_[conj_idx][pred_idx].set(rf_idx);

                    auto valid_left_subtrees = HandleFail(current_depth + 1);
                    auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                    for (auto left_subtree : valid_left_subtrees) {
                        for (auto right_subtree : valid_right_subtrees) {
                            auto root = std::make_shared<Node>(conj_idx, pred_idx, rf_idx, left_subtree, right_subtree,
                                                               NodeType::INTER);
                            valid_subtrees.emplace_back(root);
                        }
                    }

                    used_rfs_[conj_idx][pred_idx].reset(rf_idx);
                }
            }
        }
    }

    return valid_subtrees;
}
