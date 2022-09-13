#include <iostream>
#include "offline/index_builder.h"
#include "index/invert_index.h"
#include "online/search_logic.h"
#include "utils/segment.h"
#include "index/indexer.h"

/**
 * 离线索引生成
 */
void offline() {
    search::IndexBuilder builder("/home/ubuntu/project/data/origin/memnews_5w_without_tag");
    std::cout << "start segment raw data" << std::endl;
    builder.SegmentRawData();
    std::cout << "start build invert index" << std::endl;
    builder.BuildInvertIndex();
}

int main() {
//    std::cout << "加载索引" << std::endl;
//    GET_INDEXER_INSTANCE();
//    std::cout << "索引加载完成" << std::endl;
//    auto *idx = GET_FORWARD_INDEX(10);
//    std::cout << idx->title_ << std::endl;
//    std::vector<std::vector<search::InvertIndex>*> lists;
//    std::vector<search::InvertIndex> list1;
//    list1.emplace_back(search::InvertIndex(0.1,5));
//    list1.emplace_back(search::InvertIndex(0.1,6));
//    list1.emplace_back(search::InvertIndex(0.1,7));
//    list1.emplace_back(search::InvertIndex(0.1,8));
//    std::vector<search::InvertIndex> list2;
//    list2.emplace_back(search::InvertIndex(0.1,4));
//    list2.emplace_back(search::InvertIndex(0.1,6));
//    list2.emplace_back(search::InvertIndex(0.1,7));
//    list2.emplace_back(search::InvertIndex(0.1,8));
//    std::vector<search::InvertIndex> list3;
//    list3.emplace_back(search::InvertIndex(0.1,1));
//    list3.emplace_back(search::InvertIndex(0.1,2));
//    list3.emplace_back(search::InvertIndex(0.1,3));
//    list3.emplace_back(search::InvertIndex(0.1,6));
//    lists.emplace_back(&list1);
//    lists.emplace_back(&list2);
////    lists.emplace_back(&list3);
//    std::vector<search::InvertIndex> *res = new std::vector<search::InvertIndex>();
//    search::SearchLogic::Intersect(lists,res);
//    for (auto &iv : *res) {
//        std::cout << iv.doc_id_ << std::endl;
//    }
    return 0;
}
