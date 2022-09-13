#ifndef SEARCH_PROJECT_INVERT_INDEX_H
#define SEARCH_PROJECT_INVERT_INDEX_H


#include <cstdint>

namespace search {
    struct InvertIndex {
        double tf_;
        uint32_t doc_id_;

        InvertIndex() {}

        InvertIndex(double tf, uint32_t doc_id) : tf_(tf), doc_id_(doc_id) {}
    };
}

#endif //SEARCH_PROJECT_INVERT_INDEX_H
