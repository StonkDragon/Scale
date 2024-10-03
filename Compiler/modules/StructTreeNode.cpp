
#include <Common.hpp>

namespace sclc {
    StructTreeNode::StructTreeNode(Struct s) : s(s), children() {}

    StructTreeNode::~StructTreeNode() {
        for (StructTreeNode* child : children) {
            delete child;
        }
    }

    void StructTreeNode::addChild(StructTreeNode* child) {
        children.push_back(child);
    }

    std::string StructTreeNode::toString(int indent) {
        std::string result = "";
        for (int i = 0; i < indent; i++) {
            result += "  ";
        }
        result += s.name + "\n";
        for (StructTreeNode* child : children) {
            result += child->toString(indent + 1);
        }
        return result;
    }

    void StructTreeNode::forEach(std::function<void(StructTreeNode*)> f) {
        f(this);
        for (StructTreeNode* child : children) {
            child->forEach(f);
        }
    }

    StructTreeNode* StructTreeNode::directSubstructsOf(StructTreeNode* root, TPResult& result, std::string name) {
        StructTreeNode* node = new StructTreeNode(getStructByName(result, name));
        for (Struct& s : result.structs) {
            if (s.super == name) {
                node->addChild(directSubstructsOf(root, result, s.name));
            }
        }
        return node;
    }

    StructTreeNode* StructTreeNode::fromArrayOfStructs(TPResult& result) {
        StructTreeNode* root = new StructTreeNode(getStructByName(result, "SclObject"));
        return directSubstructsOf(root, result, "SclObject");
    }
}