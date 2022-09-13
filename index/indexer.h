#ifndef SEARCH_PROJECT_INDEXER_H
#define SEARCH_PROJECT_INDEXER_H

#include "../utils/singleton.h"
#include "invert_index.h"
#include "forward_index.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>


/**
 * 用于构建正排和倒排索引
 */
namespace search {
    class Indexer : public Singleton<Indexer> {
    public:
        /**
         * 应该在一开始就构建索引
         */
        Indexer() {
            int ret = LoadIndex();
            if (ret != 0) {
                return;
            }
        }

        std::vector<InvertIndex> *GetTitleIvList(const uint64_t &word_id);

        std::vector<InvertIndex> *GetContentIvList(const uint64_t &word_id);

        ForwardIndex *GetForwardIndex(uint32_t doc_id);

    private:
        // 加载所有索引的函数
        int LoadIndex();

        // 加载倒排的函数
        void LoadInvertIndex(std::ifstream *fin, std::unordered_map<uint64_t, std::vector<InvertIndex>> *iv_list);

        // 加载正排的函数
        void LoadForwardIndex(std::ifstream *fin);

        // 存储content的倒排链
        std::unordered_map<uint64_t, std::vector<InvertIndex>> m_ctx_iv_list;
        // 存储title的倒排链
        std::unordered_map<uint64_t, std::vector<InvertIndex>> m_title_iv_list;
        // 正排链,长度和文档数量一致，可直接根据文档id进行索引
        std::vector<ForwardIndex> m_forward_list;
    };

#define GET_INDEXER_INSTANCE() search::Indexer::GetInstance()
#define GET_TITLE_IV_LIST(id) search::Indexer::GetInstance()->GetTitleIvList(id)
#define GET_CONTENT_IV_LIST(id) search::Indexer::GetInstance()->GetContentIvList(id)
#define GET_FORWARD_INDEX(id) search::Indexer::GetInstance()->GetForwardIndex(id)
}


#endif //SEARCH_PROJECT_INDEXER_H
