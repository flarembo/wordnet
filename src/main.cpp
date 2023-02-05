#include "wordnet.h"

#include <iostream>
#include <sstream>
int main()
{
    std::istringstream synsets{R"(
1,a a1,a gloss
2,b bd,b gloss
3,c,c gloss
4,d bd,d gloss
)"};

    std::istringstream hypernyms{R"(
2,1
3,1
4,2,3
)"};
    std::istringstream hypernyms_copy{R"(
2,1
3,1
4,2,3
)"};

    WordNet wordnet{synsets, hypernyms};
    for (const auto & i : wordnet.nouns()) {
        std::cout << i << '\n';
    }
    std::cout << "----------------------------------------------------\n";
}
