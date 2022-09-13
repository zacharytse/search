#ifndef SEARCH_PROJECT_SEARCH_H
#define SEARCH_PROJECT_SEARCH_H

#include "../index/forward_index.h"
#include <string>
#include <vector>

namespace search {
    class SearchReq {
    public:
        std::string &Query() {
            return m_query;
        }

        std::vector<std::string> &Fields() {
            return m_fields;
        }

        int Count() {
            return m_count;
        }

        void SetQuery(const std::string &query) {
            m_query = query;
        }

        void AddField(const std::string &field) {
            m_fields.emplace_back(field);
        }

        void SetCount(int count) {
            m_count = count;
        }

    private:
        std::string m_query;
        std::vector<std::string> m_fields;
        int m_count;
    };

    class SearchResp {
        int Count() {
            return m_count;
        }

        std::vector<ForwardIndex> &Res() {
            return m_res;
        }

    private:
        int m_count; // 召回数
        std::vector<ForwardIndex> m_res; // 召回结果
    };
}

#endif //SEARCH_PROJECT_SEARCH_H
