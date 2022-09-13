#ifndef SEARCH_PROJECT_SEARCH_LOGIC_H
#define SEARCH_PROJECT_SEARCH_LOGIC_H

#include "../index/forward_index.h"
#include "../service/search.h"
#include "../index/invert_index.h"
#include <unordered_map>

namespace search {
#define TITLE 0
#define CONTENT 1
#define ALL 2

    struct Context {
        std::string query_;
        int count_ = 10; // 召回数,默认为10
        std::vector<bool> fields_; // 需要召回的域 title:0,content:1,全部:3
        std::vector<uint64_t> words_; // query切割后的结果
        Context() : fields_(std::vector<bool>(3)) {}
    };

    class SearchLogic {
    public:
        int DoSearch(SearchReq &req, SearchResp &resp);

    private:
        int Prepare(SearchReq &req);

        int SegQuery();

        static int Intersect(std::vector<std::vector<InvertIndex> *> &iv_lists, std::vector<uint32_t> &res);

        static int Intersect(std::vector<std::vector<uint32_t> *> &iv_lists, std::vector<uint32_t> &res);

        static int
        Union(std::vector<uint32_t> &title_list, std::vector<uint32_t> &ctx_list, std::vector<uint32_t> &res);

        Context m_ctx;

    };
}

#endif //SEARCH_PROJECT_SEARCH_LOGIC_H
