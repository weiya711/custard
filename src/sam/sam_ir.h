//
// Created by oliviahsu on 4/13/22.
//

#ifndef TACO_SAM_IR_H
#define TACO_SAM_IR_H

#include "sam_nodes_abstract.h"
#include "sam_nodes.h"

namespace taco {
    class TensorVar;
    class IndexVar;

namespace sam {

    struct FiberLookupNode;

    class SamIR;
    class SAMVisitorStrict;

    /// Return true if the index statement is of the given subtype.
    template <typename SubType> bool isa(SamIR);

    template <typename SubType> SubType to(SamIR);

    class SamIR : public util::IntrusivePtr<const SAMNode> {
    public:
        SamIR() : util::IntrusivePtr<const SAMNode>(nullptr) {}
        SamIR(const SAMNode* n) : util::IntrusivePtr<const SAMNode>(n) {}

        void accept(SAMVisitorStrict *v) const;

        /// Casts index expression to specified subtype.
        template<typename SubType>
        SubType as() {
            return to<SubType>(*this);
        }

        /// Print the index expression.
        friend std::ostream &operator<<(std::ostream &, const SamIR &);
    };

    class FiberLookup : public SamIR {
    public:
        FiberLookup() = default;

        FiberLookup(const FiberLookupNode *);

        FiberLookup(SamIR in_ref, IndexVar i, TensorVar tensorVar, int mode);

        FiberLookup(SamIR in_ref, SamIR out_ref, SamIR out_crd, IndexVar i, TensorVar tensorVar, int mode);

        SamIR getInRef();
        void setInRef(SamIR in_ref);

        void setOutRef(SamIR out_ref);
        SamIR getOutRef();

        void setOutCrd(SamIR out_crd);
        SamIR getOutCrd();

        typedef FiberLookupNode Node;

    };

}
}

#endif //TACO_SAM_IR_H
