#pragma once

#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class WordNet;

class Outcast
{
public:
    explicit Outcast(const WordNet & wordNet);

    // returns outcast word
    std::string outcast(const std::set<std::string> & nouns);

private:
    const WordNet & _wordNet;
};

class Digraph
{
public:
    explicit Digraph(unsigned size_hint = 0);

    friend std::ostream & operator<<(std::ostream & out, const Digraph & graph);

    const std::vector<unsigned> & operator[](unsigned vertex) const;

    std::vector<unsigned> & operator[](unsigned vertex);

    bool is_root(unsigned vertex) const;

    unsigned size() const;

    void push(std::vector<unsigned> vector);

private:
    std::vector<std::vector<unsigned>> _vertex_of_graph;
};

class ShortestCommonAncestor
{
public:
    explicit ShortestCommonAncestor(const Digraph & dg);

    // calculates length of shortest common ancestor path from node with id 'v' to node with id 'w'
    unsigned length(unsigned v, unsigned w) const;

    // returns node id of shortest common ancestor of nodes v and w
    unsigned ancestor(unsigned v, unsigned w) const;

    // calculates length of shortest common ancestor path from node subset 'subset_a' to node subset 'subset_b'
    unsigned length_subset(const std::set<unsigned> & subset_a, const std::set<unsigned> & subset_b) const;

    // returns node id of shortest common ancestor of node subset 'subset_a' and node subset 'subset_b'
    unsigned ancestor_subset(const std::set<unsigned> & subset_a, const std::set<unsigned> & subset_b) const;

public:
    struct _vertexInfo
    {
        static constexpr unsigned BAD = -1;

        unsigned ancestor_id = BAD;
        unsigned length = BAD;

        bool is_good() const
        {
            return ancestor_id != BAD;
        }
    };

    struct State
    {
        State(const Digraph & digraph);
        const Digraph & _digraph;
        std::vector<std::size_t> _black_paint;
        std::vector<std::size_t> _white_paint;
        const std::size_t _change_of_bad;
        std::size_t _bad_for_black = 0;
        std::size_t _bad_for_white = 0;

        _vertexInfo bfs(const std::set<unsigned> & subset);

        void paint_black(const std::set<unsigned> & subset);
    };

    std::unique_ptr<State> _state;
};

class WordNet
{
    using WordMap = std::unordered_map<std::string, std::set<unsigned>>;

public:
    WordNet(std::istream & synsets, std::istream & hypernyms); // NOLINT because const istream something strange

    class DistanceCalculator
    {
    public:
        // required to call or it won't work
        void fixAnotherWord(const std::string & first_word);

        unsigned getDistanceTo(const std::string & second_word) const;

    private:
        DistanceCalculator(const Digraph & digraph, const WordMap & wordMap);

    private:
        friend WordNet;
        const ShortestCommonAncestor _sca;
        const WordMap & _wordMap;
    };

    class Nouns
    {
    public:
        class iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = std::string;
            using reference = const value_type &;
            using pointer = const value_type *;

            iterator() = default;

            iterator(const WordMap::const_iterator & it)
                : _it(it){};

            reference operator*() const
            {
                return _it->first;
            }

            pointer operator->() const
            {
                return &_it->first;
            }

            iterator & operator++()
            {
                ++_it;
                return *this;
            }

            iterator operator++(int)
            {
                auto tmp = *this;
                ++_it;
                return tmp;
            }

            bool operator==(const iterator & rhs) const
            {
                return _it == rhs._it;
            }

            bool operator!=(const iterator & rhs) const
            {
                return !(*this == rhs);
            }

        private:
            WordMap::const_iterator _it;
        };

        iterator begin() const;
        iterator end() const;

    private:
        Nouns(const WordMap & wordMap)
            : _wordMap(wordMap)
        {
        }

        friend class WordNet;

        const WordMap & _wordMap;
    };

    // lists all nouns stored in WordNet
    Nouns nouns() const;

    // returns 'true' iff 'word' is stored in WordNet
    bool is_noun(const std::string & word) const;

    // returns gloss of "shortest common ancestor" of noun1 and noun2
    std::string sca(const std::string & noun1, const std::string & noun2) const;

    // calculates distance between noun1 and noun2
    unsigned distance(const std::string & noun1, const std::string & noun2) const;

    DistanceCalculator getDistanceCalculator() const;

private:
    Digraph _digraph;
    std::vector<std::string> _dictionary;
    WordMap _wordMap;
};
