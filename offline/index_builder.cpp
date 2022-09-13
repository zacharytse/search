#include "index_builder.h"
#include "../3rd/json/CJsonObject.h"
#include "../3rd/cppjieba/cppjieba/include/cppjieba/Jieba.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

namespace search {
    const char *const DICT_PATH = "../3rd/cppjieba/cppjieba/dict/jieba.dict.utf8";
    const char *const HMM_PATH = "../3rd/cppjieba/cppjieba/dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "../3rd/cppjieba/cppjieba/dict/user.dict.utf8";
    const char *const IDF_PATH = "../3rd/cppjieba/cppjieba/dict/idf.utf8";
    const char *const STOP_WORD_PATH = "../3rd/cppjieba/cppjieba/dict/stop_words.utf8";
    const char *const WORD_ID_MAP_PATH = "../data/map/word.map";
    const char *const CONTENT_SEG_PATH = "../data/segment/content_.seg";
    const char *const TITLE_SEG_PATH = "../data/segment/title_.seg";
    const char *const CONTENT_IV_PATH = "../data/index/content_.iv";
    const char *const TITLE_IV_PATH = "../data/index/title_.iv";


    IndexBuilder::IndexBuilder(const std::string &file_name) {
        m_origin_file_name = file_name;
    }

    // 对原数据做切词处理
    int IndexBuilder::SegmentRawData() {
        std::ifstream fin(m_origin_file_name);
        if (!fin.is_open()) {
            std::cout << "error path:" << m_origin_file_name << std::endl;
            return -1;
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
                    const auto &content = o_json("content_");
                    const auto &title = o_json("title_");
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
        SaveWordIdMap(word2id);
        // 回收
        for (auto *ptr: jiebas) {
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
        uint64_t word_id = 1;
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

        std::ofstream content_ofs(CONTENT_SEG_PATH);
        std::ofstream title_ofs(TITLE_SEG_PATH);
        auto seg_save_fun = [&](bool is_ctx) {
            std::vector<std::vector<std::vector<std::string> *>> *seg_res = nullptr;
            std::ofstream *ofs = nullptr;
            if (is_ctx) {
                seg_res = &content_seg;
                ofs = &content_ofs;
            } else {
                seg_res = &title_seg;
                ofs = &title_ofs;
            }
            for (const auto &bucket: *seg_res) {
                for (const auto &doc: bucket) {
                    for (int i = 0; i < doc->size(); ++i) {
                        if (i < doc->size() - 1) {
                            (*ofs) << word2id[(*doc)[i]] << ",";
                        } else {
                            (*ofs) << word2id[(*doc)[i]] << "\n";
                        }
                    }

                }
            }
        };
        // 处理content
        std::thread(seg_save_fun, true).join();
        // 处理title
        std::thread(seg_save_fun, false).join();
        content_ofs.close();
        title_ofs.close();
        return 0;
    }

    int IndexBuilder::BuildInvertIndex() {
        int ret = 0;

        // 读取切割文件
        std::vector<std::vector<uint64_t> *> content_word_ids;
        std::vector<std::vector<uint64_t> *> title_word_ids;
        ret = ReadSegment(content_word_ids, title_word_ids);

        // 统计词频
        std::thread(&IndexBuilder::CountWordFreq, this, &content_word_ids, true).join();
        std::thread(&IndexBuilder::CountWordFreq, this, &title_word_ids, false).join();

        // 计算TF
        std::thread(&IndexBuilder::GenerateTF, this, &content_word_ids, true).join();
        std::thread(&IndexBuilder::GenerateTF, this, &title_word_ids, false).join();

        // 保存倒排索引
        std::thread(&IndexBuilder::SaveInvertIndex, this, true).join();
        std::thread(&IndexBuilder::SaveInvertIndex, this, false).join();

        // 内存回收
        auto clear_fun = [&](std::vector<std::vector<uint64_t> *> &word_ids) {
            for (auto &vec: word_ids) {
                delete vec;
            }
        };
        clear_fun(content_word_ids);
        clear_fun(title_word_ids);
        return ret;
    }

    int IndexBuilder::ReadSegment(std::vector<std::vector<uint64_t> *> &content_word_ids,
                                  std::vector<std::vector<uint64_t> *> &title_word_ids) {
        std::ifstream fin_ctx(CONTENT_SEG_PATH);
        std::ifstream fin_title(TITLE_SEG_PATH);
        if (!fin_ctx || !fin_title) {
            std::cout << "err seg file path" << std::endl;
            return -1;
        }
        auto read_seg_fun = [&](bool is_ctx) {
            std::ifstream *fin = nullptr;
            std::vector<std::vector<uint64_t> *> *all_word_ids = nullptr;
            if (is_ctx) {
                fin = &fin_ctx;
                all_word_ids = &content_word_ids;
            } else {
                fin = &fin_title;
                all_word_ids = &title_word_ids;
            }
            std::string doc;
            while (std::getline(*fin, doc)) {
                std::vector<std::string> words;
                auto *word_ids = new std::vector<uint64_t>();
                boost::split(words, doc, boost::is_any_of(","), boost::token_compress_on);
                for (const auto &id: words) {
                    word_ids->emplace_back(boost::lexical_cast<uint64_t>(id));
                }
                all_word_ids->emplace_back(word_ids);
            }
        };
        // 读取content
        std::thread(read_seg_fun, true).join();
        // 读取title
        std::thread(read_seg_fun, false).join();
        fin_ctx.close();
        fin_title.close();
        return 0;
    }

    void IndexBuilder::CountWordFreq(std::vector<std::vector<uint64_t> *> *word_ids, bool is_ctx) {
        // 0同样预留
        uint32_t cur_doc_id = 1;
        std::unordered_map<uint64_t, std::vector<InvertIndex>> *iv_list = nullptr;
        if (is_ctx) {
            iv_list = &m_content_iv_list;
        } else {
            iv_list = &m_title_iv_list;
        }
        for (const auto &doc: *word_ids) {
            for (const auto &word_id: *doc) {
                if (iv_list->count(word_id) == 0) {
                    // 该单词第一次出现
                    InvertIndex ivIndex(1., cur_doc_id);
                    std::vector<InvertIndex> list;
                    list.emplace_back(ivIndex);
                    iv_list->insert(std::make_pair(word_id, list));
                } else {
                    if (cur_doc_id == (*iv_list)[word_id].back().doc_id_) {
                        // 同一篇文章中出现了重复的单词
                        (*iv_list)[word_id].back().tf_++;
                    } else {
                        // 同一个单词出现在不同的文章中
                        InvertIndex ivIndex(1, cur_doc_id);
                        (*iv_list)[word_id].emplace_back(ivIndex);
                    }
                }
            }
            ++cur_doc_id;
        }
    }

    void IndexBuilder::GenerateTF(std::vector<std::vector<uint64_t> *> *word_ids, bool is_ctx) {
        std::unordered_map<uint64_t, std::vector<InvertIndex>> *iv_list = nullptr;
        if (is_ctx) {
            iv_list = &m_content_iv_list;
        } else {
            iv_list = &m_title_iv_list;
        }
        for (auto &iter: *iv_list) {
            for (auto &iv: iter.second) {
                // 对词频做归一化产生tf,doc_id从1开始的
                iv.tf_ /= (double) ((*word_ids)[iv.doc_id_ - 1]->size());
            }
        }
    }

    /**
     * {
     *  [
     *     {
     *       word_id:1004,
     *       iv_list:[
     *          {
     *              tf:0.11,
     *              doc_id:1
     *          }
     *       ]
     *     }
     *   ]
     * }
     * @param is_ctx
     */
    void IndexBuilder::SaveInvertIndex(bool is_ctx) {
        std::unordered_map<uint64_t, std::vector<InvertIndex>> *iv_map = nullptr;
        std::ofstream ofs;
        if (is_ctx) {
            iv_map = &m_content_iv_list;
            ofs.open(CONTENT_IV_PATH);
        } else {
            iv_map = &m_title_iv_list;
            ofs.open(TITLE_IV_PATH);
        }
        neb::CJsonObject o_json;
        for (auto &iter: *iv_map) {
            neb::CJsonObject iv_list_json;
            iv_list_json.Add("word_id", boost::lexical_cast<std::string>(iter.first));
            iv_list_json.AddEmptySubArray("iv_list");
            for (const auto &iv: iter.second) {
                neb::CJsonObject iv_json;
                iv_json.Add("tf", boost::lexical_cast<std::string>(iv.tf_));
                iv_json.Add("doc_id", boost::lexical_cast<std::string>(iv.doc_id_));
                iv_list_json["iv_list"].Add(iv_json);
            }
            o_json.Add(iv_list_json);
        }
        ofs << o_json.ToString();
        ofs.close();
    }

    /**
     * {
     *    free_id:30,
     *    data:{[
     *      {
     *        word:xxx,
     *        id:1024,
     *      }
     *      ]
     *    }
     * }
     * @param map
     */
    void IndexBuilder::SaveWordIdMap(std::unordered_map<std::string, uint64_t> &map) {
        std::ofstream ofs(WORD_ID_MAP_PATH);
        neb::CJsonObject o_json;
        o_json.AddEmptySubArray("data");
        uint64_t free_id = 0; // 尚未分配出去的id
        for (auto &iter : map) {
            neb::CJsonObject map_json;
            map_json.Add("word",iter.first);
            map_json.Add("id",iter.second);
            o_json["data"].Add(map_json);
            free_id = std::max(iter.second,free_id);
        }
        free_id++;
        o_json.Add("free_id",free_id);
        ofs << o_json.ToString();
        ofs.close();
    }
}