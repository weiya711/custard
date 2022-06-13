#include "test.h"
#include "taco/tensor.h"
#include "taco/codegen/module.h"
#include <taco/index_notation/transformations.h>
#include <fstream>
#include "test.h"
#include "test_tensors.h"
#include "taco/tensor.h"
#include "taco/index_notation/index_notation.h"
#include "taco/index_notation/transformations.h"
#include "taco/lower/lower.h"
#include "op_factory.h"

#include <tuple>

using namespace taco;

template<typename T>
taco::Tensor<T> castToType(std::string name, taco::Tensor<double> tensor) {
    taco::Tensor<T> result(name, tensor.getDimensions(), tensor.getFormat());
    std::vector<int> coords(tensor.getOrder());
    for (auto& value : taco::iterate<double>(tensor)) {
        for (int i = 0; i < tensor.getOrder(); i++) {
            coords[i] = value.first[i];
        }
        // Attempt to cast the value to an integer. However, if the cast causes
        // the value to equal 0, then this will ruin the sparsity pattern of the
        // tensor, as the 0 values will get compressed out. So, if a cast would
        // equal 0, insert 1 instead to preserve the sparsity pattern of the tensor.
        if (static_cast<T>(value.second) == T(0)) {
            result.insert(coords, static_cast<T>(1));
        } else {
            result.insert(coords, static_cast<T>(value.second));
        }
    }
    result.pack();
    return result;
}

template<typename T, typename T2>
taco::Tensor<T> shiftLastMode(std::string name, taco::Tensor<T2> original) {
    taco::Tensor<T> result(name, original.getDimensions(), original.getFormat());
    std::vector<int> coords(original.getOrder());
    for (auto& value : taco::iterate<T2>(original)) {
        for (int i = 0; i < original.getOrder(); i++) {
            coords[i] = value.first[i];
        }
        int lastMode = original.getOrder() - 1;
        // For order 2 tensors, always shift the last coordinate. Otherwise, shift only coordinates
        // that have even last coordinates. This ensures that there is at least some overlap
        // between the original tensor and its shifted counter part.
        if (original.getOrder() <= 2 || (coords[lastMode] % 2 == 0)) {
            coords[lastMode] = (coords[lastMode] + 1) % original.getDimension(lastMode);
        }
        // TODO (rohany): Temporarily use a constant value here.
        result.insert(coords, T(2));
    }
    result.pack();
    return result;
}

// UfuncInputCache is a cache for the input to ufunc benchmarks. These benchmarks
// operate on a tensor loaded from disk and the same tensor shifted slightly. Since
// these operations are run multiple times, we can save alot in benchmark startup
// time from caching these inputs.
struct UfuncInputCache {
    template<typename U>
    std::pair<taco::Tensor<int64_t>, taco::Tensor<int64_t>> getUfuncInput(std::string path, U format, bool countNNZ = false, bool includeThird = false) {
        // See if the paths match.
        if (this->lastPath == path) {
            // TODO (rohany): Not worrying about whether the format was the same as what was asked for.
            return std::make_pair(this->inputTensor, this->otherTensor);
        }

        // Otherwise, we missed the cache. Load in the target tensor and process it.
        this->lastLoaded = taco::read(path, format);
        // We assign lastPath after lastLoaded so that if taco::read throws an exception
        // then lastPath isn't updated to the new path.
        this->lastPath = path;
        this->inputTensor = castToType<int64_t>("A", this->lastLoaded);
        this->otherTensor = shiftLastMode<int64_t, int64_t>("B", this->inputTensor);
        if (countNNZ) {
            this->nnz = 0;
            for (auto& it : iterate<int64_t>(this->inputTensor)) {
                this->nnz++;
            }
        }
        if (includeThird) {
            this->thirdTensor = shiftLastMode<int64_t, int64_t>("C", this->otherTensor);
        }
        return std::make_pair(this->inputTensor, this->otherTensor);
    }

    taco::Tensor<double> lastLoaded;
    std::string lastPath;

    taco::Tensor<int64_t> inputTensor;
    taco::Tensor<int64_t> otherTensor;
    taco::Tensor<int64_t> thirdTensor;
    int64_t nnz;
};
UfuncInputCache inputCache;

const Format DSSS({Dense, Sparse, Sparse, Sparse}, {0,1,2,3});
const Format SSS({Sparse, Sparse, Sparse}, {0,1,2});
const Format DSS({Dense, Sparse, Sparse}, {0,1,2});

vector<std::string> tensors3 = {"enron.tns", "facebook.tns", "amazon.tns", "nell-1.tns", "nell-2.tns", "patents.tns", "reddit.tns"};

TEST(sam, pack_sss) {
    std::string frosttPath = std::getenv("FROSTT_PATH");
    std::string frosttFormatPath = std::getenv("FROST_FORMAT_TACO_PATH");
    for (auto& tnsPath : tensors3) {
        std::string frosttTensorPath = frosttPath + tnsPath;

        auto pathSplit = taco::util::split(tnsPath, "/");
        auto filename = pathSplit[pathSplit.size() - 1];
        auto tensorName = taco::util::split(filename, ".")[0];

        Tensor<int64_t> frosttTensor, other;
        std::tie(frosttTensor, other) = inputCache.getUfuncInput(frosttTensorPath, SSS);

        ofstream myfile;
        std::string outpath = frosttFormatPath;
        outpath += tensorName + ".txt";
        myfile.open (outpath);
        myfile << frosttTensor << endl;
        myfile.close();
    }
}

