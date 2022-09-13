#include "indexer.h"
#include "../3rd/json/CJsonObject.h"
#include <thread>
#include <boost/lexical_cast.hpp>

namespace search {
    const char *const CONTENT_IV_PATH = "../data/index/content.iv";
    const char *const TITLE_IV_PATH = "../data/index/title.iv";
    const char *const ORIGIN_PATH = "../data/origin/memnews_5w_without_tag";

    /**
     *
     * 加载索引时使用3个线程，分别加载content倒排、title倒排以及正排
     * @return
     */
    int Indexer::LoadIndex() {
        // 用于加载content倒排链的流
        std::ifstream fin_ctx(CONTENT_IV_PATH);
        // 用于加载title倒排链流
        std::ifstream fin_title(TITLE_IV_PATH);
        // 加载正排的流
        std::ifstream fin_origin(ORIGIN_PATH);
        if (!fin_ctx || !fin_title || !fin_origin) {
            std::cout << "Err load index" << std::endl;
            return -1;
        }
        std::thread(&Indexer::LoadInvertIndex, this, &fin_ctx, &m_ctx_iv_list).join();
        std::thread(&Indexer::LoadInvertIndex, this, &fin_title, &m_title_iv_list).join();
        std::thread(&Indexer::LoadForwardIndex, this, &fin_origin).join();
        fin_ctx.close();
        fin_title.close();
        fin_origin.close();
        return 0;
    }

    /**
     * 读取倒排json的格式
     * * {
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
     * @param
     */
    void Indexer::LoadInvertIndex(std::ifstream *fin, std::unordered_map<uint64_t, std::vector<InvertIndex>> *iv_list) {
        std::string json;
        std::getline(*fin, json);
        neb::CJsonObject o_json(json);
        // 单词数
        int word_size = o_json.GetArraySize();
        /**
         * 将所有单词构成的倒排链写入到iv_list中
         */
        for (int i = 0; i < word_size; ++i) {
            auto &iv = o_json[i];
            auto word_id = boost::lexical_cast<uint64_t>(iv("word_id"));
            int iv_list_size = iv["iv_list"].GetArraySize();
            iv_list->insert(std::make_pair(word_id, std::vector<InvertIndex>(iv_list_size)));
            for (int j = 0; j < iv_list_size; ++j) {
                (*iv_list)[word_id][j].tf_ = boost::lexical_cast<double>(iv["iv_list"][j]("tf"));
                (*iv_list)[word_id][j].doc_id_ = boost::lexical_cast<uint32_t>(iv["iv_list"][j]("doc_id"));
            }
        }
    }

    /**
     * json 格式,存储的时候一行是一个元素
     * {
     *   [
     *   {
     *      bizuin:2659,
     *      content:xxx,
     *      title:xxx,
     *      docid:120021,
     *      publish_time:44177
     *   }
     *   ]
     * }
     * 将正排索引写入m_forward_list
     * @param fin
     */
    void Indexer::LoadForwardIndex(std::ifstream *fin) {
        std::string json;
        while (std::getline(*fin, json)) {
            neb::CJsonObject o_json(json);
            ForwardIndex index;
            index.bizuin_ = o_json("bizuin");
            index.content_ = o_json("content");
            index.title_ = o_json("title");
            index.doc_id_ = o_json("docid");
            index.publish_time_ = o_json("publish_time");
            m_forward_list.emplace_back(index);
        }
    }

    /**
     * 根据单词id得到title对应的倒排链,返回指针可以避免深拷贝倒排链
     * @param word_id
     * @return
     */
    std::vector<InvertIndex> *Indexer::GetTitleIvList(const uint64_t &word_id) {
        if (m_title_iv_list.count(word_id) == 0) {
            return nullptr;
        } else {
            return &m_title_iv_list[word_id];
        }
    }

    /**
     * 根据单词id得到title对应的倒排链,返回指针可以避免深拷贝倒排链
     * @param word_id
     * @return
     */
    std::vector<InvertIndex> *Indexer::GetContentIvList(const uint64_t &word_id) {
        if (m_ctx_iv_list.count(word_id) == 0) {
            return nullptr;
        } else {
            return &m_ctx_iv_list[word_id];
        }
    }

    /**
     * 根据doc_id获得相应的正排内容
     * @param doc_id
     * @return
     */
    ForwardIndex *Indexer::GetForwardIndex(uint32_t doc_id) {
        return &m_forward_list[doc_id];
    }
}