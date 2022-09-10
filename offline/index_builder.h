//
// Created by 24810 on 2022/09/08.
//

#ifndef SEARCH_PROJECT_INDEX_BUILDER_H
#define SEARCH_PROJECT_INDEX_BUILDER_H

#include <string>
#include <vector>
#include <unordered_map>

namespace search {
#define CORE_NUM 2

    class IndexBuilder {
    public:
        IndexBuilder(const std::string &file_name);

        int SegmentRawData(); // 加载原始数据
    private:
        int GenerateWordId(std::vector<std::vector<std::vector<std::vector<std::string> *>>> &words,
                           std::unordered_map<std::string, uint64_t> &);

        int SaveSegment(std::vector<std::vector<std::vector<std::string> *>> &content_seg,
                        std::vector<std::vector<std::vector<std::string> *>> &title_seg,
                        std::unordered_map<std::string, uint64_t> &word2id);

    private:
        std::string m_file_name;
    };
}

#endif //SEARCH_PROJECT_INDEX_BUILDER_H
