#ifndef SEARCH_PROJECT_FORWARD_INDEX_H
#define SEARCH_PROJECT_FORWARD_INDEX_H

#include <cstdint>
#include <string>

namespace search {
    // id用string存，省点内存
    struct ForwardIndex {
        std::string doc_id_;
        std::string title_;
        std::string content_;
        std::string publish_time_;
        std::string bizuin_;
    };
}

#endif //SEARCH_PROJECT_FORWARD_INDEX_H
