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

//    void SAMVisitorStrict::visit(const SamIRNode &obj) {
//        obj.accept(this);
//    }

    void SAMVisitor::visit(const FiberLookupNode *op) {

    }

    void SAMVisitor::visit(const FiberWriteNode *op) {

    }


    void SAMVisitor::visit(const IntersectNode *op) {

    }

    void SAMVisitor::visit(const UnionNode *op) {

    }

    void SAMVisitor::visit(const RepeatNode *op) {

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