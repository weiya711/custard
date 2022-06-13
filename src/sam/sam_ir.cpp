//
// Created by oliviahsu on 4/13/22.
//

#include "sam_nodes.h"
#include "sam_ir.h"


using namespace std;
namespace taco {
namespace sam {

    void SamIR::accept(SAMVisitorStrict *v) const {
        ptr->accept(v);
    }

    std::ostream& operator<<(std::ostream &os, const taco::sam::SamIR& sam) {
        return os;
    }

    // FiberLookup
    FiberLookup::FiberLookup(const FiberLookupNode *n) : SamIR(n) {
    }

    FiberLookup::FiberLookup(SamIR out_ref, SamIR out_crd, IndexVar i,
                             TensorVar tensorVar, int mode, int nodeID, bool root, bool source) :
    FiberLookup(new FiberLookupNode(out_ref, out_crd, i, tensorVar, mode, root, source, nodeID)) {}

    bool FiberLookup::outputsDefined() const {
        return getOutCrd().defined() and getOutRef().defined();
    }

    SamIR FiberLookup::getOutRef() const {
        return getNode(*this)->out_ref;
    }

    SamIR FiberLookup::getOutCrd() const {
        return getNode(*this)->out_crd;
    }

    TensorVar FiberLookup::getTensorVar() const {
        return getNode(*this)->tensorVar;
    }

    IndexVar FiberLookup::getIndexVar() const {
        return getNode(*this)->i;
    }

    // FiberWrite
    FiberWrite::FiberWrite(const FiberWriteNode *n) : SamIR(n) {}

    FiberWrite::FiberWrite(IndexVar i, TensorVar tensorVar, int mode, int nodeID, bool sink, bool vals) :
    FiberWrite(new FiberWriteNode(i, tensorVar, mode, sink, vals, nodeID)) {}

    // Repeat
    Repeat::Repeat(const RepeatNode *n) : SamIR(n){
    }

    Repeat::Repeat(SamIR out_ref, IndexVar i, TensorVar tensorVar, int nodeID, bool root) :
    Repeat(new RepeatNode(out_ref, i, tensorVar, root, nodeID)) {
    }

    // Intersect
    Intersect::Intersect(const IntersectNode *n) : SamIR(n){
    }

    Intersect::Intersect(SamIR out_crd, SamIR out_a_ref, SamIR out_b_ref, IndexVar i, int nodeID) :
            Intersect(new IntersectNode(out_crd, out_a_ref, out_b_ref, i, nodeID)) {
    }

    // Union
    Union::Union(const UnionNode *n) : SamIR(n){
    }

    Union::Union(SamIR out_crd, SamIR out_a_ref, SamIR out_b_ref, IndexVar i, int nodeID) :
            Union(new UnionNode(out_crd, out_a_ref, out_b_ref, i, nodeID)) {
    }

    template <> bool isa<FiberLookup>(SamIR s) {
        return isa<FiberLookupNode>(s.ptr);
    }

    template <> FiberLookup to<FiberLookup>(SamIR s) {
        taco_iassert(isa<FiberLookup>(s));
        return FiberLookup(to<FiberLookupNode>(s.ptr));
    }


    Root::Root(const RootNode *n) : SamIR(n){
    }

    Root::Root(const vector<SamIR> nodes) : Root(new RootNode(nodes)){
    }
}
}



