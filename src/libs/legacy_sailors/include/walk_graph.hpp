#pragma once

#include <vector>

class WalkGraph
{
  public:
    struct Pair
    {
        long v1{};
        long v2{};
        long v3{};
        long v4{};
    };

    void AddPair(const long v1, const long v2, const long t1, const long t2)
    {
        pairs_.emplace_back(v1, v2, t1, t2);
    }

    void Clear()
    {
        pairs_.clear();
    }

    [[nodiscard]] bool TestPair(const long v1, const long v2) const
    {
        for (const auto pair : pairs_)
        {
            if (pair.v1 == v1 && pair.v2 == v2)
            {
                return true;
            }
            if (pair.v1 == v2 && pair.v2 == v1)
            {
                return true;
            }
        }
        return false;
    }

    long FindRandomWithType(const int type)
    {
        static std::vector<long> chosen;
        chosen.clear();

        for (const auto pair : pairs_)
        {
            if (pair.v3 == type)
            {
                chosen.emplace_back(pair.v1);
            }
            if (pair.v4 == type)
            {
                chosen.emplace_back(pair.v2);
            }
        }

        if (!chosen.empty())
        {
            return chosen[rand() % chosen.size()];
        }

        return -1;
    }

    long FindRandomLinkedWithType(const int source, const int type)
    {
        static std::vector<long> chosen;
        chosen.clear();

        for (const auto pair : pairs_)
        {
            if (pair.v1 == pair.v2)
            {
                continue;
            }

            if (pair.v1 == source && pair.v4 == type)
            {
                chosen.emplace_back(pair.v2);
            }
            if (pair.v2 == source && pair.v3 == type)
            {
                chosen.emplace_back(pair.v1);
            }
        }

        if (!chosen.empty())
        {
            return chosen[rand() % chosen.size()];
        }

        return source;
    }

    long FindRandomLinkedWithoutType(const int source, const int type)
    {
        static std::vector<long> chosen;
        chosen.clear();

        for (const auto pair : pairs_)
        {
            if (pair.v1 == pair.v2)
            {
                continue;
            }

            if (pair.v1 == source && pair.v4 != type)
            {
                chosen.emplace_back(pair.v2);
            }
            if (pair.v2 == source && pair.v3 != type)
            {
                chosen.emplace_back(pair.v1);
            }
        }

        if (!chosen.empty())
        {
            return chosen[rand() % chosen.size()];
        }

        return source;
    }

    long FindRandomLinkedAnyType(const int source)
    {
        static std::vector<long> chosen;
        chosen.clear();

        for (const auto pair : pairs_)
        {
            if (pair.v1 == pair.v2)
            {
                continue;
            }

            if (pair.v1 == source)
            {
                chosen.emplace_back(pair.v2);
            }
            if (pair.v2 == source)
            {
                chosen.emplace_back(pair.v1);
            }
        }

        if (!chosen.empty())
        {
            return chosen[rand() % chosen.size()];
        }

        return source;
    }

  private:
    std::vector<Pair> pairs_;
};
