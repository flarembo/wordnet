#include "wordnet.h"

#include <algorithm>
#include <array>
#include <limits>
#include <queue>
#include <sstream>
#include <utility>

namespace {
constexpr std::size_t _max_size_t = std::numeric_limits<std::size_t>::max();
constexpr std::size_t _delay_to_reload = 1E4;
constexpr std::size_t _reload_time = std::numeric_limits<std::size_t>::max() - std::numeric_limits<std::size_t>::max() / _delay_to_reload;
} // namespace

/*
-------------------------------------------------------------------------------------------------------------------
 Digraph implementation
 */
Digraph::Digraph(unsigned size_hint)
    : _vertex_of_graph(size_hint)
{
}

std::ostream & operator<<(std::ostream & out, const Digraph & graph)
{
    unsigned id = 0;
    for (const auto & edges : graph._vertex_of_graph) {
        out << id << ":";
        id++;
        std::copy(edges.begin(), edges.end(), std::ostream_iterator<unsigned>(out, " "));
        out << '\n';
    }
    return out;
}

bool Digraph::is_root(unsigned vertex) const
{
    return _vertex_of_graph[vertex].empty();
}

const std::vector<unsigned> & Digraph::operator[](unsigned int vertex) const
{
    return _vertex_of_graph[vertex];
}

unsigned Digraph::size() const
{
    return _vertex_of_graph.size();
}

void Digraph::push(std::vector<unsigned int> vector)
{
    _vertex_of_graph.push_back(std::move(vector));
}

std::vector<unsigned> & Digraph::operator[](unsigned int vertex)
{
    return _vertex_of_graph[vertex];
}

/*
 End of Digraph implementation
 */

/*
  ---------------------------------------------------------------------------------------------------------------------------------------
  SCA implementation
 */
ShortestCommonAncestor::ShortestCommonAncestor(const Digraph & dg)
    : _state(new State(dg))
{
}

void ShortestCommonAncestor::State::paint_black(const std::set<unsigned int> & subset)
{
    if (_bad_for_black >= _reload_time) {
        std::fill(_black_paint.begin(), _black_paint.end(), 0);
        _bad_for_black = 0;
    }
    _bad_for_black += _change_of_bad;
    std::queue<unsigned> queue;
    for (const auto vertex : subset) {
        queue.emplace(vertex);
        _black_paint[vertex] = _bad_for_black;
    }
    while (!queue.empty()) {
        auto vertex = queue.front();
        queue.pop();
        if (_digraph.is_root(vertex)) {
            continue;
        }
        auto new_length = _black_paint[vertex] + 1;
        for (const auto vertexToGo : _digraph[vertex]) {
            if (_black_paint[vertexToGo] < _bad_for_black) {
                _black_paint[vertexToGo] = new_length;
                queue.emplace(vertexToGo);
            }
        }
    }
}

ShortestCommonAncestor::_vertexInfo ShortestCommonAncestor::State::bfs(const std::set<unsigned> & subset)
{
    if (_bad_for_white >= _reload_time) {
        std::fill(_white_paint.begin(), _white_paint.end(), 0);
        _bad_for_white = 0;
    }
    _bad_for_white += _change_of_bad;
    std::queue<unsigned> queue;
    _vertexInfo ans;
    std::size_t biased_ans = _max_size_t;
    for (const auto vertex : subset) {
        if (_black_paint[vertex] < biased_ans && _black_paint[vertex] >= _bad_for_black) {
            biased_ans = _black_paint[vertex];
            ans = {vertex, 0};
        }
        queue.emplace(vertex);
        _white_paint[vertex] = _bad_for_white;
    }
    if (ans.is_good()) {
        return {ans.ancestor_id, static_cast<unsigned>(biased_ans - _bad_for_black)};
    }
    while (!queue.empty()) {
        auto vertex = queue.front();
        queue.pop();
        if (_digraph.is_root(vertex)) {
            continue;
        }
        auto new_length = _white_paint[vertex] + 1;
        for (const auto vertexToGo : _digraph[vertex]) {
            if (_white_paint[vertexToGo] < _bad_for_white) {
                _white_paint[vertexToGo] = new_length;
                queue.emplace(vertexToGo);
                if (_black_paint[vertexToGo] >= _bad_for_black) {
                    auto ans_length = new_length + _black_paint[vertexToGo];
                    if (ans_length < biased_ans) {
                        biased_ans = ans_length;
                        ans = {vertexToGo, 0};
                    }
                }
            }
        }
    }
    return {ans.ancestor_id, static_cast<unsigned>(biased_ans - _bad_for_black - _bad_for_white)};
}

unsigned ShortestCommonAncestor::length(unsigned v, unsigned w) const
{
    _state->paint_black({w});
    return _state->bfs({v}).length;
}
unsigned ShortestCommonAncestor::ancestor(unsigned v, unsigned w) const
{
    _state->paint_black({w});
    return _state->bfs({v}).ancestor_id;
}
unsigned ShortestCommonAncestor::length_subset(const std::set<unsigned> & subset_a, const std::set<unsigned> & subset_b) const
{
    _state->paint_black(subset_b);
    return _state->bfs(subset_a).length;
}
unsigned ShortestCommonAncestor::ancestor_subset(const std::set<unsigned> & subset_a, const std::set<unsigned> & subset_b) const
{
    _state->paint_black(subset_b);
    return _state->bfs(subset_a).ancestor_id;
}
/*
 End Of Impl SCA
 */

/*
 ----------------------------------------------------------------------------------------------------------------------------------------------
 Impl WordNet
 */
WordNet::WordNet(std::istream & synsets, std::istream & hypernyms) // NOLINT because const istream something strange
{
    std::unordered_map<unsigned, unsigned> zip_map;
    unsigned reorder = 0;
    unsigned id;
    std::string str, substr;
    while (synsets >> id) {
        synsets.ignore();
        zip_map[id] = reorder;
        id = reorder++;
        std::getline(synsets, str, ',');
        std::stringstream str_stream(str);
        while (str_stream >> substr) {
            _wordMap[std::move(substr)].insert(id);
        }
        _dictionary.emplace_back();
        std::getline(synsets, _dictionary.back());
    }

    _digraph = Digraph(reorder);
    while (hypernyms >> id) {
        auto & vertex_to_insert = _digraph[zip_map[id]];
        while (hypernyms.peek() != '\n') {
            hypernyms.ignore();
            hypernyms >> id;
            unsigned index_of_inserted = zip_map[id];
            vertex_to_insert.push_back(index_of_inserted);
        }
        hypernyms >> std::ws;
    }
    //
}

WordNet::Nouns WordNet::nouns() const
{
    return Nouns(_wordMap);
}

bool WordNet::is_noun(const std::string & word) const
{
    return _wordMap.find(word) != _wordMap.end();
}

std::string WordNet::sca(const std::string & noun1, const std::string & noun2) const
{
    return _dictionary.at(ShortestCommonAncestor(_digraph).ancestor_subset(_wordMap.at(noun1), _wordMap.at(noun2)));
}

unsigned WordNet::distance(const std::string & noun1, const std::string & noun2) const
{
    return ShortestCommonAncestor(_digraph).length_subset(_wordMap.at(noun2), _wordMap.at(noun1));
}

WordNet::DistanceCalculator WordNet::getDistanceCalculator() const
{
    return DistanceCalculator(_digraph, _wordMap);
}

WordNet::Nouns::iterator WordNet::Nouns::begin() const
{
    return iterator(_wordMap.cbegin());
}

WordNet::Nouns::iterator WordNet::Nouns::end() const
{
    return iterator(_wordMap.cend());
}

/*
 End impl WordNet
 */

/*
 ------------------------------------------------------------
 Impl OutCast
 */

Outcast::Outcast(const WordNet & wordNet)
    : _wordNet(wordNet)
{
}

std::string Outcast::outcast(const std::set<std::string> & nouns)
{
    std::vector<unsigned> distance_to_other(nouns.size(), 0);
    std::set<std::string>::const_iterator ans = nouns.begin();
    std::size_t i = 0;
    bool more_than_one_on_the_same_distance = false;
    auto calculator = _wordNet.getDistanceCalculator();
    for (auto it_i = nouns.begin(); it_i != nouns.end(); ++it_i, ++i) {
        auto it_j = it_i;
        ++it_j;
        std::size_t j = i + 1;
        calculator.fixAnotherWord(*it_i);
        for (; it_j != nouns.end(); ++it_j, ++j) {
            const auto dist = calculator.getDistanceTo(*it_j);
            distance_to_other[i] += dist;
            distance_to_other[j] += dist;
        }
    }
    unsigned max_sum = 0;
    i = 0;
    for (auto it_i = nouns.begin(); it_i != nouns.end(); it_i++, i++) {
        if (distance_to_other[i] > max_sum) {
            max_sum = distance_to_other[i];
            ans = it_i;
            more_than_one_on_the_same_distance = false;
        }
        else if (distance_to_other[i] == max_sum) {
            more_than_one_on_the_same_distance = true;
        }
    }

    return more_than_one_on_the_same_distance ? "" : *ans;
}

WordNet::DistanceCalculator::DistanceCalculator(const Digraph & digraph, const WordNet::WordMap & wordMap)
    : _sca(digraph)
    , _wordMap(wordMap)
{
}

unsigned WordNet::DistanceCalculator::getDistanceTo(const std::string & second_word) const
{
    return _sca._state->bfs(_wordMap.at(second_word)).length;
}

void WordNet::DistanceCalculator::fixAnotherWord(const std::string & first_word)
{
    _sca._state->paint_black(_wordMap.at(first_word));
}

ShortestCommonAncestor::State::State(const Digraph & dg)
    : _digraph(dg)
    , _black_paint(dg.size())
    , _white_paint(dg.size())
    , _change_of_bad(dg.size() + 1)
{
}
