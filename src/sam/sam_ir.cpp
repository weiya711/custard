//
// Created by oliviahsu on 4/13/22.
//

#include "sam_nodes.h"

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

    FiberLookup::FiberLookup(SamIR in_ref, IndexVar i, TensorVar tensorVar, int mode) : FiberLookup(new FiberLookupNode(in_ref, i, tensorVar, mode)) {
    }

    FiberLookup::FiberLookup(SamIR in_ref, SamIR out_ref, SamIR out_crd, IndexVar i, TensorVar tensorVar, int mode) :
    FiberLookup(new FiberLookupNode(in_ref, out_ref, out_crd, i, tensorVar, mode)) {
    }

    template <> bool isa<FiberLookup>(SamIR s) {
        return isa<FiberLookupNode>(s.ptr);
    }

    template <> FiberLookup to<FiberLookup>(SamIR s) {
        taco_iassert(isa<FiberLookup>(s));
        return FiberLookup(to<FiberLookupNode>(s.ptr));
    }
}
}



