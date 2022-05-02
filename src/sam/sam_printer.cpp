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
            os << to_string(op->nodeID) << " [comment=\"type=broadcast\"";
            if (prettyPrint) {
                os << " shape=point style=invis ";
            }
            if (printAttributes) {
                os << "type=\"broadcast\"";
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
            string src = op->source ? ",src=true" : ",src=false";
            string root = op->root ? ",root=true" : ",root=false";

            std::stringstream comment;
            comment << "\"type=fiberlookup,index=" << op->i.getName() << ",tensor=" << op->tensorVar.getName()
                    << ",mode=" << std::to_string(op->mode)
                    << ",format=" << op->modeFormat.getName() << src << root << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=green4 shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"fiberlookup\""
                      " index=\"" << op->i.getName() << "\"" <<
                      " tensor=\"" << op->tensorVar.getName() << "\"" <<
                      " mode=\"" << std::to_string(op->mode) << "\"" <<
                      " format=\"" << op->modeFormat.getName() << "\"" <<
                      " src=\"" << (op->source ? "true" : "false") << "\"" <<
                      " root=\"" << (op->root ? "true" : "false") << "\"";
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
            string sink = op->sink ? ",sink=true" : ",sink=false";

            std::stringstream comment;
            if (op->vals) {
                comment << "\"type=fiberwrite,mode=vals" << ",tensor=" << op->tensorVar.getName() << sink << "\"";
            } else {
                comment << "\"type=fiberwrite,index=" << op->i.getName() << ",tensor=" << op->tensorVar.getName()
                        << ",mode=" << std::to_string(op->mode)
                        << ",format=" << op->modeFormat.getName() << sink << "\"";
            }

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=green3 shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"fiberwrite\"";
                if (op->vals) {
                    os << " tensor=\"" << op->tensorVar.getName() << "\"" <<
                          " mode=\"vals\"";
                } else {
                    os << " index=\"" << op->i.getName() << "\"" <<
                          " tensor=\"" << op->tensorVar.getName() << "\"" <<
                          " mode=\"" << std::to_string(op->mode) << "\"" <<
                          " format=\"" << op->modeFormat.getName() << "\"";
                }
                os << " sink=" << (op->sink ? "\"true\"" : "\"false\"");

            }
            os << "]" << endl;
        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const RepeatNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            string root = op->root ? ",root=true" : ",root=false";

            std::stringstream comment;
            comment << "\"type=repeat,index=" << op->i.getName() << ",tensor=" << op->tensorVar.getName() << root << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=cyan2 shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"repeat\""
                      " index=\"" << op->i.getName() << "\"" <<
                      " tensor=\"" << op->tensorVar.getName() << "\"" <<
                      " root=\"" << (op->root ? "true" : "false") << "\"";
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
            comment << "\"type=repsiggen,index=" << op->i.getName() << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=cyan3 shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"repsiggen\""
                      " index=\"" << op->i.getName() << "\"";
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
            comment << "\"type=" << op->getNodeName() << ",index=" << op->i.getName() << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=purple shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"" << op->getNodeName() << "\""
                      " index=\"" << op->i.getName() << "\"";
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
            comment << "\"type=arrayvals,tensor=" << op->tensorVar.getName() << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=green2 shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"arrayvals\""
                      " tensor=\"" << op->tensorVar.getName() << "\"";
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
            comment << "\"type=" << op->getNodeName() <<"\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=brown shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"" << op->getNodeName() << "\"";
            }
            os << "]" << endl;

            if (op->out_val.defined()) {
                op->out_val.accept(this);
            }

        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::visit(const SparseAccumulatorNode *op) {
        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            std::stringstream comment;
            comment << "\"type=" << op->getNodeName() <<",order=" << op->order << "\"";

            os << tab;
            os << to_string(op->nodeID) << " [comment=" << comment.str();
            if (prettyPrint) {
                os << " label=\"" << op->getName() << "\"";
                os << " color=brown shape=box style=filled";
            }
            if (printAttributes) {
                os << " type=\"" << op->getNodeName() << "\"" <<
                      " order=\"" << op->order << "\"";
            }
            os << "]" << endl;

            if (op->out_val.defined()) {
                op->out_val.accept(this);
            }

        }
        printedNodes.push_back(op->nodeID);
    }

    void SAMDotNodePrinter::setPrintAttributes(bool printAttributes) {
        this->printAttributes = printAttributes;
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
            string ss = printerHelper();
            os << op->nodeID << ss << endl;

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

            string ss = printerHelper();
            os << op->nodeID << ss << endl;
        }

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {

            if (op->out_crd.defined()) {
                printComment = op->printEdgeName;
                comment = "in-"+op->tensorVar.getName();
                edgeType = "crd";
                os << tab << op->nodeID << " -> ";
                op->out_crd.accept(this);
            }

            if (op->out_ref.defined()) {
                printComment = op->printEdgeName;
                comment = "in-"+op->tensorVar.getName();
                edgeType = "ref";
                os << tab << op->nodeID << " -> ";
                op->out_ref.accept(this);
            }

            edgeType = "";
        }
        printedNodes.push_back(op->nodeID);

    }

    void SAMDotEdgePrinter::visit(const FiberWriteNode *op) {
        string ss = printerHelper();
        os << op->nodeID << ss << endl;

        edgeType = "";

        printedNodes.push_back(op->nodeID);
    }

    void SAMDotEdgePrinter::visit(const RepeatNode *op) {
        if (!op->root) {
            string ss = printerHelper();
            os << op->nodeID << ss << endl;
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
        string ss = printerHelper();
        os << op->nodeID << ss << endl;

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
        string ss = printerHelper();
        os << op->nodeID << ss << endl;

        if (edgeType == "ref") {
            auto op_addr = (JoinerNode **) &op;
            (*op_addr)->numInputs += 1;
        }

        if (std::count(printedNodes.begin(), printedNodes.end(), op->nodeID) == 0) {
            if (op->out_crd.defined()) {
                edgeType = "crd";
                os << tab << op->nodeID << " -> ";
                op->out_crd.accept(this);
            }

            for (int i = 0; i < (int)op->out_refs.size(); i++) {
                auto out_ref = op->out_refs.at(i);
                if (out_ref.defined()) {
                    printComment = true;
                    comment = "out-"+out_ref.getTensorName();
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
        string ss = printerHelper();
        os << op->nodeID << ss << endl;

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
        string ss = printerHelper();
        os << op->nodeID << ss << endl;

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

    string SAMDotEdgePrinter::printerHelper() {
        stringstream ss;
        ss << " [";
        string labelExt = printComment ? "_"+comment : "";
        ss << "label=\"" << (edgeType.empty() ? "val" : edgeType) << labelExt << "\"";
        if (prettyPrint) {
            ss << edgeStyle[edgeType];
        }
        if (printAttributes) {
            ss << " type=\"" << (edgeType.empty() ? "val" : edgeType) << "\"";
        }
        if (printComment) {
            ss << " comment=\"" << comment << "\"";
        }
        ss << "]";

        printComment = false;
        comment = "";
        return ss.str();
    }

    void SAMDotEdgePrinter::setPrintAttributes(bool printAttributes) {
        this->printAttributes = printAttributes;
    }
}
}
