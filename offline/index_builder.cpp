#include "index_builder.h"
#include "../3rd/json/CJsonObject.h"
#include "../3rd/cppjieba/cppjieba/include/cppjieba/Jieba.hpp"
#include <iostream>
#include <vector>
#include <thread>

namespace search {
    const char *const DICT_PATH = "../3rd/cppjieba/cppjieba/dict/jieba.dict.utf8";
    const char *const HMM_PATH = "../3rd/cppjieba/cppjieba/dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "../3rd/cppjieba/cppjieba/dict/user.dict.utf8";
    const char *const IDF_PATH = "../3rd/cppjieba/cppjieba/dict/idf.utf8";
    const char *const STOP_WORD_PATH = "../3rd/cppjieba/cppjieba/dict/stop_words.utf8";
    const char *const CONTENT_OUTPUT_PATH = "../data/segment/content.seg";
    const char *const TITLE_OUTPUT_PATH = "../data/segment/title.seg";

    IndexBuilder::IndexBuilder(const std::string &file_name) {
        m_file_name = file_name;
    }

    // 对原数据做切词处理
    int IndexBuilder::SegmentRawData() {
        std::ifstream fin(m_file_name);
        if (!fin.is_open()) {
            std::cout << "error path:" << m_file_name << std::endl;
        }
        std::string line;
        // 定义有CORE_NUM * 2个桶
        const int total_bucket = CORE_NUM * 2;
        std::vector<std::vector<std::string>> buckets(total_bucket);
        int cur_bucket = 0;
        std::cout << "开始读取文件" << std::endl;
        while (std::getline(fin, line)) {
            buckets[cur_bucket].emplace_back(line);
            cur_bucket = (cur_bucket + 1) % total_bucket;
            if (cur_bucket == 0) {
                break;
            }
        }
        std::cout << "读取文件完毕" << std::endl;

        // 分词器线程不安全，一个线程一个分词器避免加锁
        std::vector<cppjieba::Jieba *> jiebas;
        for (int i = 0; i < total_bucket; ++i) {
            jiebas.emplace_back(new cppjieba::Jieba(DICT_PATH,
                                                    HMM_PATH,
                                                    USER_DICT_PATH,
                                                    IDF_PATH,
                                                    STOP_WORD_PATH));
        }
        // 先保存切割后的单词，再求id,避免在切的过程中要加锁
        std::vector<std::vector<std::vector<std::string> *>> content_segment_res(total_bucket);
        std::vector<std::vector<std::vector<std::string> *>> title_segment_res(total_bucket);
        // 多个线程同时处理
        for (int i = 0; i < total_bucket; ++i) {
            std::thread([&](int bucket_idx) {
                auto &bucket = buckets[bucket_idx];
                auto &jieba = jiebas[bucket_idx];
                for (const auto &json: bucket) {
                    neb::CJsonObject o_json(json);
                    const auto &content = o_json("content");
                    const auto &title = o_json("title");
                    auto *content_words = new std::vector<std::string>();
                    auto *title_words = new std::vector<std::string>();
                    jieba->Cut(content, *content_words, true);
                    jieba->Cut(title, *title_words, true);
                    // zero copy
                    content_segment_res[bucket_idx].emplace_back(content_words);
                    title_segment_res[bucket_idx].emplace_back(title_words);
                }
            }, i).join();
        }

        std::unordered_map<std::string, uint64_t> word2id;
        std::vector<std::vector<std::vector<std::vector<std::string> *>>> words;
        words.emplace_back(content_segment_res);
        words.emplace_back(title_segment_res);
        GenerateWordId(words, word2id);

        SaveSegment(content_segment_res, title_segment_res, word2id);

        // 回收
        for (auto *ptr : jiebas) {
            delete ptr;
        }
        for (int bucket = 0; bucket < total_bucket; ++bucket) {
            const int doc_size = content_segment_res[bucket].size();
            for (int i = 0; i < doc_size; ++i) {
                delete content_segment_res[bucket][i];
                delete title_segment_res[bucket][i];
            }
        }
        fin.close();
    }

    int IndexBuilder::GenerateWordId(std::vector<std::vector<std::vector<std::vector<std::string> *>>> &words,
                                     std::unordered_map<std::string, uint64_t> &word2id) {
        // wordid从1开始，保留0作为特殊字段
        int word_id = 1;
        for (const auto &field: words) {
            for (const auto &bucket: field) {
                for (const auto &doc: bucket) {
                    for (const auto &word: *doc) {
                        if (word2id.count(word) == 0) {
                            // 为word分配id
                            word2id[word] = word_id++;
                        }
                    }
                }
            }
        }
        return 0;
    }

    int IndexBuilder::SaveSegment(std::vector<std::vector<std::vector<std::string> *>> &content_seg,
                                  std::vector<std::vector<std::vector<std::string> *>> &title_seg,
                                  std::unordered_map<std::string, uint64_t> &word2id) {

        std::ofstream content_ofs(CONTENT_OUTPUT_PATH);
        std::ofstream title_ofs(TITLE_OUTPUT_PATH);
        std::thread([&](){
            for (const auto &bucket: content_seg) {
                for (const auto &doc: bucket) {
                    for (int i = 0; i < doc->size(); ++i) {
                        if (i < doc->size() - 1) {
                            content_ofs << word2id[(*doc)[i]] << ",";
                        } else {
                            content_ofs << word2id[(*doc)[i]] << "\n";
                        }
                    }

                }
            }
        }).join();
        std::thread([&](){
            for (const auto &bucket: title_seg) {
                for (const auto &doc: bucket) {
                    for (int i = 0; i < doc->size(); ++i) {
                        if (i < doc->size() - 1) {
                            title_ofs << word2id[(*doc)[i]] << ",";
                        } else {
                            title_ofs << word2id[(*doc)[i]] << "\n";
                        }
                    }

                }
            }
        }).join();
        content_ofs.close();
        title_ofs.close();
        return 0;
    }
}