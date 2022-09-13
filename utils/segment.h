//
// Created by 24810 on 2022/09/11.
//

#ifndef SEARCH_PROJECT_SEGMENT_H
#define SEARCH_PROJECT_SEGMENT_H

#include "spin_lock.h"
#include "singleton.h"
#include "../3rd/cppjieba/cppjieba/include/cppjieba/Jieba.hpp"
#include "../3rd/json/CJsonObject.h"
#include <vector>
#include <fstream>

namespace search {
    class Segment;

    const char *const DICT_PATH = "../3rd/cppjieba/cppjieba/dict/jieba.dict.utf8";
    const char *const HMM_PATH = "../3rd/cppjieba/cppjieba/dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "../3rd/cppjieba/cppjieba/dict/user.dict.utf8";
    const char *const IDF_PATH = "../3rd/cppjieba/cppjieba/dict/idf.utf8";
    const char *const STOP_WORD_PATH = "../3rd/cppjieba/cppjieba/dict/stop_words.utf8";
    const char *const WORD_ID_MAP_PATH = "../data/map/word.map";

    /**
     * 保证线程安全的分词器，使用自旋锁
     */
    class Segment : public Singleton<Segment> {
    public:
        Segment() {
            m_jieba = new cppjieba::Jieba(DICT_PATH,
                                          HMM_PATH,
                                          USER_DICT_PATH,
                                          IDF_PATH,
                                          STOP_WORD_PATH);
            std::ifstream fin(WORD_ID_MAP_PATH);
            if (!fin) {
                std::cout << "ERR open word id map file" << std::endl;
                return;
            }
            std::string json;
            std::getline(fin, json);
            neb::CJsonObject o_json(json);
            int size = o_json["data"].GetArraySize();
            for (int i = 0; i < size; ++i) {
                std::string word = o_json["data"][i]("word");
                uint64_t word_id = 0;
                o_json["data"][i].Get("id", word_id);
                m_word_id_map[word] = word_id;
            }
            o_json.Get("free_id", m_free_id);
            fin.close();
        }

        void Cut(const std::string &sentence, std::vector<uint64_t> &ids) {
            m_lock.Lock();
            std::vector<std::string> words;
            m_jieba->Cut(sentence, words, true);
            for (auto &word: words) {
                if (m_word_id_map.count(word) == 0) {
                    m_word_id_map[word] = m_free_id++;
                }
                ids.emplace_back(m_word_id_map[word]);
            }
            m_lock.UnLock();
        }
        
        ~Segment() {
            delete m_jieba;
        }

        SpinLock m_lock;
        cppjieba::Jieba *m_jieba;
        int m_free_id;
        std::unordered_map<std::string, uint64_t> m_word_id_map;
    };

#define GET_SEG_INSTANCE() search::Segment::GetInstance()
}
#endif //SEARCH_PROJECT_SEGMENT_H
