#ifndef SEARCH_PROJECT_SEARCHSERVICE_H
#define SEARCH_PROJECT_SEARCHSERVICE_H

#include "../index/forward_index.h"
#include "search.h"
#include <string>
#include <vector>

namespace search {
    class SearchService {
    public:
        int Search(SearchReq& req,SearchResp& resp);
    };
}


#endif //SEARCH_PROJECT_SEARCHSERVICE_H
