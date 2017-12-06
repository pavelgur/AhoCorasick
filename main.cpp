#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <vector>

class TAhoCorasick {
    struct TNode {
        ssize_t Next = -1;
        ssize_t Go = -1;
        ssize_t IsLeaf = -1;
        size_t ParentCode = 0;
        ssize_t Parent = -1;
        ssize_t SuffLink = -1;
        size_t Depth = 0;
    };

public:
    struct TState {
        TState() = default;

        bool IsRoot() const noexcept {
            return Idx == 0;
        }

        bool IsNextTo(const TState& s) const noexcept {
            return Depth == s.Depth + 1;
        }

        bool IsLeaf() const noexcept {
            return Leaf >= 0;
        }

        size_t GetWordIdx() const noexcept {
            assert(IsLeaf());
            return Leaf;
        }

    private:
        friend class TAhoCorasick;
        TState(size_t idx, size_t depth = 0, ssize_t leaf = -1)
            : Idx(idx)
            , Depth(depth)
            , Leaf(leaf)
        {}

        size_t Idx = 0;
        size_t Depth = 0;
        ssize_t Leaf = -1;
    };

public:
    TAhoCorasick()
        : Tree(1)
    {
    }

    TAhoCorasick(const auto& words)
    {
        Reset(words);
    }

    void Reset(const auto& words) {
        Tree.assign(1, TNode());
        CodeSize = 0;
        Ways.clear();
        CodeMap.fill(-1);

        for (const auto& w : words) {
            for (const auto& c : w) {
                auto& code = CodeMap[c];
                if (code == -1) {
                    code = CodeSize++;
                }
            }
        }

        for (auto wordIdx = 0u; wordIdx < words.size(); ++wordIdx) {
            const auto& w = words[wordIdx];
            size_t curPos = 0;
            for (auto chPos = 0u; chPos < w.size(); ++chPos) {
                const auto code = CodeMap[w[chPos]];

                if (Tree[curPos].Next == -1) {
                    auto& treeNode = Tree[curPos];
                    treeNode.Next = Ways.size();
                    Ways.insert(Ways.end(), CodeSize, -1);
                }

                auto& nextNode = Ways[Tree[curPos].Next + code];
                if (nextNode == -1) {
                    nextNode = Tree.size();
                    Tree.emplace_back();
                    auto& newNode = Tree.back();
                    newNode.Parent = curPos;
                    newNode.ParentCode = code;
                    newNode.Depth = chPos + 1;
                }
                curPos = nextNode;
            }
            Tree[curPos].IsLeaf = wordIdx;
        }
    }

    bool HasString(const auto& s) const noexcept {
        TState state;
        for (const auto c : s) {
            const auto newState = SwitchState(c, state);
            if (!newState.IsNextTo(state)) {
                return false;
            }
            state = newState;
        }
        return state.IsLeaf();
    }

    bool HasPrefix(const auto& s) const noexcept {
        TState state;
        for (const auto c : s) {
            const auto newState = SwitchState(c, state);
            if (!newState.IsNextTo(state)) {
                return false;
            }
            state = newState;
        }
        return true;
    }

    /**
     * @brief SearchIn
     * returns vector with pairs: word end position and input word index
     */
    void SearchIn(const auto& s, std::vector<std::pair<size_t, size_t>>& res) const noexcept {
        TState state;
        for (auto i = 0u; i < s.size(); ++i) {
            const auto c = s[i];

            state = SwitchState(c, state);
            if (state.IsLeaf()) {
                res.emplace_back(i + 1, state.GetWordIdx());
            }
        }
    }

    TState GetState(const char c) const noexcept {
        return SwitchState(c, TState());
    }

    TState SwitchState(const char c, const TState& v) const noexcept {
        const auto code = CodeMap[c];
        if (code == -1) {
            return TState();
        }

        return SwitchStateCode(code, v);
    }

    TState GetLink(const TState& s) const noexcept {
        const auto v = s.Idx;
        auto& node = Tree[v];
        if (node.SuffLink == -1) {
            if (v == 0 || Tree[v].Parent == 0) {
                node.SuffLink = 0;
            } else {
                node.SuffLink = SwitchStateCode(node.ParentCode, GetLink(node.Parent)).Idx;
            }
        }

        assert(node.SuffLink >= 0);
        return TState(node.SuffLink, node.Depth, Tree[node.SuffLink].IsLeaf);
    }

private:
    TState SwitchStateCode(const size_t code, const TState& v) const noexcept {
        assert(code < CodeSize);

        auto& node = Tree[v.Idx];
        if (node.Go == -1) {
            node.Go = Ways.size();
            Ways.insert(Ways.end(), CodeSize, -1);
        }

        const auto goOffset = node.Go + code;
        const auto nextOffset = node.Next + code;
        if (Ways[goOffset] == -1) {
            if (node.Next == -1 || Ways[nextOffset] == -1) {
                Ways[goOffset] = v.IsRoot() ? 0 : SwitchStateCode(code, GetLink(v)).Idx;
            } else {
                Ways[goOffset] = Ways[nextOffset];
            }
        }

        const auto nextNode = Ways[goOffset];
        return TState(nextNode, Tree[nextNode].Depth, Tree[nextNode].IsLeaf);
    }

private:
    size_t CodeSize = 0;
    std::array<ssize_t, 1 << (8 * sizeof(char))> CodeMap;
    mutable std::vector<TNode> Tree;
    mutable std::vector<ssize_t> Ways;
};

int main() {
    const std::vector<std::string> strings {
        "abcd",
        "bcde",
        "cdef",
    };
    TAhoCorasick trie(strings);

    for (const auto& s : strings) {
        assert(trie.HasString(s));
    }

    {
        const std::vector<std::string> nonStrings {
            "aabcd",
            "abcdd",
            "abcda",
            "abc",
            "bcd",
            "cde",
            "bcd",
            "cde",
            "def",
            "abcdA",
            "bcdeB",
            "cdefC",
            "Aabcd",
            "Bbcde",
            "Ccdef",
        };
        for (const auto& s : nonStrings) {
            assert(!trie.HasString(s));
        }
    }
    {
        const std::vector<std::string> prefixes {
            "abc",
            "bcd",
            "cde",
        };
        for (const auto& s : prefixes) {
            assert(!trie.HasString(s));
            assert(trie.HasPrefix(s));
        }
    }
    {
        const std::vector<std::string> texts {
            "ZZZZZZZZZZZZZZZZZZZZZabcd",
            "bcdeXXXXXXXXXXXXXXXXXX",
            "ZZZZZZcdefXXXXXXXX",
            "ZZZZZZbcdefXXXXXXXX",
            "ZZZZZZabcdefXXXXXXXX",
        };
        std::vector<std::pair<size_t, size_t>> res;
        for (const auto& s : texts) {
            res.clear();
            trie.SearchIn(s, res);
            assert(res.size() >= 1);
        }
    }

    return EXIT_SUCCESS;
}
