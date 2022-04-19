//
// Created by oliviahsu on 4/14/22.
//

#include <algorithm>
#include "sam_printer.h"
#include "sam_nodes.h"
using namespace std;

namespace taco {
namespace sam {
    // SAMPrinter
    void SAMPrinter::visit(const FiberLookupNode *op) {
        os << op->getName();
        os << "->";
        op->out_crd.accept(this);
    }

    void SAMPrinter::print(const SamIR &sam) {
        sam.accept(this);
    }


    void SAMDotNodePrinter::print(const SamIR &sam) {
        sam.accept(this);
    }


    // SAMDotNodePrinter
    void SAMDotNodePrinter::visit(const RootNode *op) {
        os << "digraph " << name << " {" << endl;
        for (const auto& node : op->nodes) {
            node.accept(this);
        }
    }

    // SAMDotNodePrinter
    void SAMDotNodePrinter::visit(const BroadcastNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            os << tab;
            os << to_string(op->nodeID) << " [comment=\"broadcast\"";
            if (prettyPrint) {
                os << " shape=point style=invis";
            }
            os << "]" << endl;


            for (const auto &node: op->outputs) {
                if (node.defined()) {
                    node.accept(this);
                }
            }
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const FiberLookupNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            string src = op->source ? "_src" : "";
            string root = op->root ? "_root" : "";

            std::stringstream comment;
            comment << "\"fiber_lookup-" << op->i.getName() << "_" << op->tensorVar.getName()
                    << std::to_string(op->mode)
                    << "_" << op->modeFormat.getName() << src << root << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=green4 shape=box style=filled";
            }
            os << "]" << endl;

            if (op->out_crd.defined()) {
                op->out_crd.accept(this);
            }

            if (op->out_ref.defined()) {
                op->out_ref.accept(this);
            }
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const FiberWriteNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            string sink = op->sink ? "_sink" : "";

            std::stringstream comment;
            if (op->vals) {
                comment << "\"fiber_write-vals" << "_" << op->tensorVar.getName() << sink << "\"";
            } else {
                comment << "\"fiber_write-" << op->i.getName() << "_" << op->tensorVar.getName()
                        << std::to_string(op->mode)
                        << "_" << op->modeFormat.getName() << sink << "\"";
            }

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=green3 shape=box style=filled";
            }
            os << "]" << endl;
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const RepeatNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            string root = op->root ? "_root" : "";

            std::stringstream comment;
            comment << "\"repeat-" << op->i.getName() << "_" << op->tensorVar.getName() << root << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=cyan2 shape=box style=filled";
            }
            os << "]" << endl;

            if (op->out_ref.defined()) {
                op->out_ref.accept(this);
            }
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const RepeatSigGenNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {

            std::stringstream comment;
            comment << "\"repsiggen-" << op->i.getName() << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=cyan3 shape=box style=filled";
            }
            os << "]" << endl;

            if (op->out_repsig.defined()) {
                op->out_repsig.accept(this);
            }
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const JoinerNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            std::stringstream comment;
            comment << "\"" << op->getNodeName() << "-" << op->i.getName() << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=purple shape=box style=filled";
            }
            os << "]" << endl;

            if (op->out_crd.defined()) {
                op->out_crd.accept(this);
            }

            for (auto out_ref : op->out_refs) {
                if (out_ref.defined())
                    out_ref.accept(this);
            }
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const ArrayNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            std::stringstream comment;
            comment << "\"array-vals_" << op->tensorVar.getName() << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=green2 shape=box style=filled";
            }
            os << "]" << endl;

            if (op->out_val.defined()) {
                op->out_val.accept(this);
            }

        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const ComputeNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            std::stringstream comment;
            comment << "\"" << op->getNodeName() <<"\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=brown shape=box style=filled";
            }
            os << "]" << endl;

            if (op->out_val.defined()) {
                op->out_val.accept(this);
            }

        }
        printedNodes.push_back(op->nodeID);
    }

    // SAM Dot Edge Printer
    void SAMDotEdgePrinter::print(const SamIR &sam) {
        sam.accept(this);
    }

    void SAMDotEdgePrinter::visit(const RootNode *op) {
        for (const auto& node : op->nodes) {
            node.accept(this);
        }
        os << "}" << endl;
    }

    void SAMDotEdgePrinter::visit(const BroadcastNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            stringstream ss;
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
            os << op->nodeID << ss.str() << endl;

            for (const auto &node: op->outputs) {
                if (node.defined()) {
                    switch (op->type) {
                        case SamEdgeType::crd:
                            edgeType = "crd";
                            break;
                        case SamEdgeType::ref:
                            edgeType = "ref";
                            break;
                        case SamEdgeType::repsig:
                            edgeType = "repsig";
                            break;
                        case SamEdgeType::val:
                        default:
                            edgeType = "";
                            break;
                    }
                    os << tab << op->nodeID << " -> ";
                    node.accept(this);
                }
            }
            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotEdgePrinter::visit(const FiberLookupNode *op) {
        if (!op->root) {
            stringstream ss;
            if (!edgeType.empty()) {
                ss << " [label=\"" << edgeType << "\"";
                if (prettyPrint) {
                    ss << " " << edgeStyle[edgeType];
                }
                ss << "]";
            }
            os << op->nodeID << ss.str() << endl;
        }

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {

            if (op->out_crd.defined()) {
                edgeType = "crd";
                os << tab << op->nodeID << " -> ";
                op->out_crd.accept(this);
            }

            if (op->out_ref.defined()) {
                edgeType = "ref";
                os << tab << op->nodeID << " -> ";
                op->out_ref.accept(this);
            }

            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);

    }

    void SAMDotEdgePrinter::visit(const FiberWriteNode *op) {
        stringstream ss;
        if (!edgeType.empty()) {
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
        }
        os << op->nodeID << ss.str() << endl;

        edgeType = "";

        printedNodes.push_back(op->nodeID);
    }

    void SAMDotEdgePrinter::visit(const RepeatNode *op) {
        if (!op->root) {
            stringstream ss;
            if (!edgeType.empty()) {
                ss << " [label=\"" << edgeType << "\"";
                if (prettyPrint) {
                    ss << " " << edgeStyle[edgeType];
                }
                ss << "]";
            }
            os << op->nodeID << ss.str() << endl;
        }

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            if (op->out_ref.defined()) {
                edgeType = "ref";
                os << tab << op->nodeID << " -> ";
                op->out_ref.accept(this);
            }
            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotEdgePrinter::visit(const RepeatSigGenNode *op) {
        stringstream ss;
        if (!edgeType.empty()) {
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
        }
        os << op->nodeID << ss.str() << endl;

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            if (op->out_repsig.defined()) {
                edgeType = "repsig";
                os << tab << op->nodeID << " -> ";
                op->out_repsig.accept(this);
            }
            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotEdgePrinter::visit(const JoinerNode *op) {
        stringstream ss;
        if (!edgeType.empty()) {
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
        }
        os << op->nodeID << ss.str() << endl;

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            if (op->out_crd.defined()) {
                edgeType = "crd";
                os << tab << op->nodeID << " -> ";
                op->out_crd.accept(this);
            }

            for (auto out_ref : op->out_refs) {
                if (out_ref.defined()) {
                    edgeType = "ref";
                    os << tab << op->nodeID << " -> ";
                    out_ref.accept(this);
                }
            }
            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotEdgePrinter::visit(const ArrayNode *op) {
        stringstream ss;
        if (!edgeType.empty()) {
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
        }
        os << op->nodeID << ss.str() << endl;

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            if (op->out_val.defined()) {
                edgeType = "";
                os << tab << op->nodeID << " -> ";
                op->out_val.accept(this);
            }

            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotEdgePrinter::visit(const ComputeNode *op) {
        stringstream ss;
        if (!edgeType.empty()) {
            ss << " [label=\"" << edgeType << "\"";
            if (prettyPrint) {
                ss << " " << edgeStyle[edgeType];
            }
            ss << "]";
        }
        os << op->nodeID << ss.str() << endl;

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            if (op->out_val.defined()) {
                edgeType = "";
                os << tab << op->nodeID << " -> ";
                op->out_val.accept(this);
            }

            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);
    }

}
}
