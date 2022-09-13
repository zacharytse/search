#include "search_logic.h"
#include "../utils/segment.h"
#include "../index/indexer.h"
#include <algorithm>

namespace search {
    /**
     * search的主逻辑，先接受request中的参数，再分割query，然后根据query求交集，最后打分返回结果
     * @param req
     * @param resp
     * @return
     */
    int SearchLogic::DoSearch(search::SearchReq &req, search::SearchResp &resp) {
        int ret = 0;
        ret = Prepare(req);
        ret = SegQuery();

        // 对query分割完的单词求交
        std::vector<uint32_t> title_res;
        std::vector<uint32_t> ctx_res;
        if (m_ctx.fields_[TITLE] || m_ctx.fields_[ALL]) {
            // title || 全部
            std::vector<std::vector<InvertIndex> *> iv_lists;
            for (auto &word_id: m_ctx.words_) {
                auto iv_list = GET_TITLE_IV_LIST(word_id);
                if (iv_list != nullptr) {
                    iv_lists.emplace_back(iv_list);
                }
            }
            Intersect(iv_lists, title_res);
        }
        if (m_ctx.fields_[CONTENT] || m_ctx.fields_[ALL]) {
            std::vector<std::vector<InvertIndex> *> iv_lists;
            for (auto &word_id: m_ctx.words_) {
                auto iv_list = GET_CONTENT_IV_LIST(word_id);
                if (iv_list != nullptr) {
                    iv_lists.emplace_back(iv_list);
                }
            }
            Intersect(iv_lists, ctx_res);
        }

        // 判断下是多个域求交还是所有域求并,并将结果保存在res中
        std::vector<uint32_t> res;
        if (m_ctx.fields_[ALL]) {
            // 多个域求并
            ret = Union(title_res, ctx_res, res);
        } else {
            // 多个域求交
            std::vector<std::vector<uint32_t> *> iv_lists;
            if (m_ctx.fields_[0]) {
                iv_lists.emplace_back(&title_res);
            }
            if (m_ctx.fields_[1]) {
                iv_lists.emplace_back(&ctx_res);
            }
            ret = Intersect(iv_lists,res);
        }

        // 在res中取出top k进入下一阶段
        int new_size = std::min((int)res.size(),m_ctx.count_ * 10);
        res.resize(new_size);


        return 0;
    }

    int SearchLogic::Prepare(SearchReq &req) {
        m_ctx.count_ = req.Count();
        for (auto &field: req.Fields()) {
            if (field == "content") {
                m_ctx.fields_[CONTENT] = true;
            }
            if (field == "title") {
                m_ctx.fields_[TITLE] = true;
            }
        }
        m_ctx.fields_[ALL] = (!m_ctx.fields_[TITLE] && !m_ctx.fields_[CONTENT]); // 一个域都没填，默认使用第三个选项
        m_ctx.query_ = req.Query();
        return 0;
    }

    int SearchLogic::SegQuery() {
        GET_SEG_INSTANCE()->Cut(m_ctx.query_, m_ctx.words_);
        return 0;
    }

    /**
     * 对list中的所有倒排链求交，要求倒排链按doc_id从小到大排序,结果存到res
     * 求交过程为先找出最短的链，以该链为基准和其他链求交
     * @param iv_lists
     * @return
     */
    int SearchLogic::Intersect(std::vector<std::vector<InvertIndex> *> &iv_lists, std::vector<uint32_t> &res) {
        if (iv_lists.empty()) {
            return 0;
        }
        if (iv_lists.size() == 1) {
            for (auto &iv: *iv_lists[0]) {
                res.emplace_back(iv.doc_id_);
            }
        }

        // 找最短链
        int min_idx = 0;
        uint32_t min_size = iv_lists[0]->size();
        for (int i = 1; i < iv_lists.size(); ++i) {
            if (iv_lists[i]->size() < min_size) {
                min_idx = i;
                min_size = iv_lists[i]->size();
            }
        }

        // 以最短链为基准，和其他链求交
        std::vector<int> counts(min_size); // 统计每个元素被查找到的次数
        for (int i = 0; i < iv_lists.size(); ++i) {
            if (i == min_idx) {
                // 跳过自身
                continue;
            }
            auto &cur_iv_list = *iv_lists[i];
            auto less = [](const InvertIndex &lhs, const InvertIndex &rhs) {
                return lhs.doc_id_ < rhs.doc_id_;
            };
            // 遍历最短链，判断其中的元素是否在cur_iv_list中
            for (int j = 0; j < iv_lists[min_idx]->size(); ++j) {
                if (counts[j] < i - 1) {
                    // 之前有链没找到该元素了
                    continue;
                }
                auto &iv = (*iv_lists[min_idx])[j];
                if (less(iv, cur_iv_list[0]) || less(cur_iv_list[cur_iv_list.size() - 1], iv)) {
                    // iv∈(-∞,cur_iv_list[0]) or (cur_iv_list[cur_iv_list.size() - 1],+∞)
                    continue;
                }
                if (std::binary_search(cur_iv_list.begin(), cur_iv_list.end(), iv, less)) {
                    // 当前元素出现在cur_iv_list中
                    counts[j]++;
                }
            }
        }

        // 根据counts的结果，将对应的iv填入到结果集中
        for (int i = 0; i < min_size; ++i) {
            if (counts[i] == iv_lists.size() - 1) {
                res.emplace_back((*iv_lists[min_idx])[i].doc_id_);
            }
        }
        return 0;
    }

    /**
     * 对title_list和ctx_list求并，结果保存在res中
     * @param title_list
     * @param ctx_list
     * @param res
     * @return
     */
    int
    SearchLogic::Union(std::vector<uint32_t> &title_list, std::vector<uint32_t> &ctx_list, std::vector<uint32_t> &res) {
        int p1 = 0 , p2 = 0;
        while (p1 < title_list.size() || p2 < ctx_list.size()) {
            if (p2 >= ctx_list.size()) {
                res.emplace_back(title_list[p1]);
                ++p1;
            } else if (p1 >= title_list.size()) {
                res.emplace_back(ctx_list[p2]);
                ++p2;
            } else {
                if (title_list[p1] < ctx_list[p2]) {
                    res.emplace_back(title_list[p1]);
                    ++p1;
                } else if (title_list[p1] == ctx_list[p2]) {
                    res.emplace_back(title_list[p1]);
                    ++p1;
                    ++p2;
                } else {
                    res.emplace_back(ctx_list[p2]);
                    ++p2;
                }
            }
        }
        return 0;
    }

    int SearchLogic::Intersect(std::vector<std::vector<uint32_t> *> &iv_lists, std::vector<uint32_t> &res) {
        int ret = 0;

        // 赋值操作
        std::vector<std::vector<InvertIndex>*> tmp_iv_lists;
        for (auto &iv_list : iv_lists) {
            auto *tmp_iv_list = new std::vector<InvertIndex>(iv_list->size());
            for (int i = 0; i < iv_list->size(); ++i) {
                (*tmp_iv_list)[i].doc_id_ = (*iv_list)[i];
            }
            tmp_iv_lists.emplace_back(tmp_iv_list);
        }

        ret = Intersect(tmp_iv_lists,res);

        for (auto &iv_list : tmp_iv_lists) {
            delete iv_list;
        }

        return ret;
    }
}
