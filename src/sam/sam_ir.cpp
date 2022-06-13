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

    string SamIR::getTensorName() const {
        return ptr->getTensorName();
    }

    std::ostream& operator<<(std::ostream &os, const taco::sam::SamIR& sam) {
        return os;
    }

    // FiberLookup
    FiberLookup::FiberLookup(const FiberLookupNode *n) : SamIR(n) {
    }

    FiberLookup::FiberLookup(SamIR out_ref, SamIR out_crd, IndexVar i,
                             const TensorVar& tensorVar, int mode, int nodeID, bool root, bool source, bool printEdgeName) :
    FiberLookup(new FiberLookupNode(out_ref, out_crd, i, tensorVar, mode, root, source, printEdgeName, nodeID)) {}

    TensorVar FiberLookup::getTensorVar() const {
        return getNode(*this)->tensorVar;
    }

    IndexVar FiberLookup::getIndexVar() const {
        return getNode(*this)->i;
    }

    // FiberWrite
    FiberWrite::FiberWrite(const FiberWriteNode *n) : SamIR(n) {}

    FiberWrite::FiberWrite(IndexVar i, const TensorVar& tensorVar, int mode, string maxSegSize, string maxCrdSize,
                           int nodeID, bool sink, bool vals) :
    FiberWrite(new FiberWriteNode(i, tensorVar, mode, maxSegSize, maxCrdSize, sink, vals, nodeID)) {}

    // Repeat
    Repeat::Repeat(const RepeatNode *n) : SamIR(n){
    }

    Repeat::Repeat(SamIR out_ref, IndexVar i, const TensorVar& tensorVar, int nodeID, bool root) :
    Repeat(new RepeatNode(out_ref, i, tensorVar, root, nodeID)) {
    }

    // Intersect
    Intersect::Intersect(const IntersectNode *n) : SamIR(n){
    }

    Intersect::Intersect(SamIR out_crd, vector<SamIR> out_refs, IndexVar i, int nodeID) :
            Intersect(new IntersectNode(out_crd, out_refs, i, nodeID)) {
    }

    // Union
    Union::Union(const UnionNode *n) : SamIR(n){
    }

    Union::Union(SamIR out_crd, vector<SamIR> out_refs, IndexVar i, int nodeID) :
            Union(new UnionNode(out_crd, out_refs, i, nodeID)) {
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

    Root::Root(const vector<SamIR>& nodes, const vector<TensorVar>& tensors) : Root(new RootNode(nodes, tensors)){
    }

    RepeatSigGen::RepeatSigGen(const RepeatSigGenNode *n) : SamIR(n){
    }

    RepeatSigGen::RepeatSigGen(SamIR out_repsig, IndexVar i, int nodeID) :
            RepeatSigGen(new RepeatSigGenNode(out_repsig, i, nodeID)){
    }

    Broadcast::Broadcast(const BroadcastNode *n) : SamIR(n) {
    }

    Broadcast::Broadcast(std::vector<SamIR> outputs, SamEdgeType type, int nodeID) :
    Broadcast(new BroadcastNode(outputs, type, nodeID)){

    }

    Array::Array(const ArrayNode *n) : SamIR(n){
    }

    Array::Array(SamIR out_val, const TensorVar& tensorVar, int nodeID, bool printEdgeName) :
    Array(new ArrayNode(out_val, tensorVar, printEdgeName, nodeID)) {
    }

    Mul::Mul(const MulNode *n) : SamIR(n){
    }

    Mul::Mul(SamIR out_val, int nodeID) :
            Mul(new MulNode(out_val, nodeID)) {
    }

    Add::Add(const AddNode *n) : SamIR(n){
    }

    Add::Add(SamIR out_val, int nodeID) :
            Add(new AddNode(out_val, nodeID)) {
    }

    Reduce::Reduce(const ReduceNode *n) : SamIR(n){
    }

    Reduce::Reduce(SamIR out_val, int nodeID) :
            Reduce(new ReduceNode(out_val, nodeID)) {
    }
    
    SparseAccumulator::SparseAccumulator(const SparseAccumulatorNode *n) : SamIR(n){
    }

    SparseAccumulator::SparseAccumulator(SamIR out_val, int order, int nodeID) :
            SparseAccumulator(new SparseAccumulatorNode(out_val, order, nodeID)) {
    }
}
}



