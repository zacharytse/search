#ifndef SEARCH_PROJECT_INDEX_BUILDER_H
#define SEARCH_PROJECT_INDEX_BUILDER_H

#include "../index/invert_index.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace search {
#define CORE_NUM 2

    class IndexBuilder {
    public:
        IndexBuilder(const std::string &file_name);

        int SegmentRawData(); // 加载原始数据
        int BuildInvertIndex();

    private:
        int GenerateWordId(std::vector<std::vector<std::vector<std::vector<std::string> *>>> &words,
                           std::unordered_map<std::string, uint64_t> &);

        int SaveSegment(std::vector<std::vector<std::vector<std::string> *>> &content_seg,
                        std::vector<std::vector<std::vector<std::string> *>> &title_seg,
                        std::unordered_map<std::string, uint64_t> &word2id);

        int ReadSegment(std::vector<std::vector<uint64_t> *> &content_word_ids,
                        std::vector<std::vector<uint64_t> *> &title_word_ids);

        // 词频统计
        void CountWordFreq(std::vector<std::vector<uint64_t> *> *word_ids, bool is_ctx);

        // 生成TF
        void GenerateTF(std::vector<std::vector<uint64_t> *> *word_ids, bool is_ctx);

        // 保存倒排索引
        void SaveInvertIndex(bool is_ctx);

        void SaveWordIdMap(std::unordered_map<std::string, uint64_t> &map);

        std::string m_origin_file_name;
        std::unordered_map<uint64_t, std::vector<InvertIndex>> m_content_iv_list;
        std::unordered_map<uint64_t, std::vector<InvertIndex>> m_title_iv_list;
    };
}

#endif //SEARCH_PROJECT_INDEX_BUILDER_H
