//
// Created by oliviahsu on 4/13/22.
//

#include "sam_ir.h"
#include "sam_nodes.h"

using namespace std;
namespace taco {
namespace sam {

    std::ostream& operator<<(std::ostream &os, const taco::sam::SamIR & sam) {
        return os;
    }



    FiberLookup::FiberLookup(const FiberLookupNode *n) : SamIR(n) {
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



