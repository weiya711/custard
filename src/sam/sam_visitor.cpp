#include "sam_visitor.h"
#include "sam_nodes.h"

using namespace std;

namespace taco {
namespace sam {

// class IndexNotationVisitorStrict
    SAMVisitorStrict::~SAMVisitorStrict() {
    }

    SAMVisitor::~SAMVisitor() {
    }

    void SAMVisitorStrict::visit(const SamIR& sam) {
        sam.accept(this);
    }

    void SAMVisitor::visit(const RootNode *op) {
        for (auto node : op->nodes) {
            node.accept(this);
        }
    }

    void SAMVisitor::visit(const FiberLookupNode *op) {
        if (op->out_crd.defined()) {
            op->out_crd.accept(this);
        }

        if (op->out_ref.defined()) {
            op->out_ref.accept(this);
        }
    }

    void SAMVisitor::visit(const FiberWriteNode *op) {
    }


    void SAMVisitor::visit(const JoinerNode *op) {
        if (op->out_crd.defined()) {
            op->out_crd.accept(this);
        }

        if (op->out_a_ref.defined()) {
            op->out_a_ref.accept(this);
        }

        if (op->out_b_ref.defined()) {
            op->out_b_ref.accept(this);
        }
    }

    void SAMVisitor::visit(const IntersectNode *op) {
        visit(static_cast<const JoinerNode*>(op));
    }

    void SAMVisitor::visit(const UnionNode *op) {
        visit(static_cast<const JoinerNode*>(op));
    }

    void SAMVisitor::visit(const RepeatNode *op) {
        if (op->out_ref.defined()) {
            op->out_ref.accept(this);
        }
    }

    void SAMVisitor::visit(const RepeatSigGenNode *op) {

    }

    void SAMVisitor::visit(const ArrayNode *op) {

    }

    void SAMVisitor::visit(const MulNode *op) {

    }

    void SAMVisitor::visit(const AddNode *op) {

    }

    void SAMVisitor::visit(const ReduceNode *op) {

    }

    void SAMVisitor::visit(const SparseAccumulatorNode *op) {

    }
}
}