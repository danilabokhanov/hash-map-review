#include "hash_map.h"
#include <catch.hpp>
#include <iostream>

namespace test_utils {
struct StrangeInt {
    int x;
    static int counter;
    StrangeInt() {
        ++counter;
    }
    StrangeInt(int x) : x(x) {
        ++counter;
    }
    StrangeInt(const StrangeInt& rs) : x(rs.x) {
        ++counter;
    }
    StrangeInt(StrangeInt&& rs) : x(rs.x) {
        ++counter;
    }

    StrangeInt& operator=(const StrangeInt& rs) {
        x = rs.x;
        return *this;
    }

    StrangeInt& operator=(StrangeInt&& rs) {
        x = rs.x;
        return *this;
    }

    bool operator==(const StrangeInt& rs) const {
        return x == rs.x;
    }

    static void Init() {
        counter = 0;
    }

    ~StrangeInt() {
        --counter;
    }

    friend std::ostream& operator<<(std::ostream& out, const StrangeInt& x) {
        out << x.x;
        return out;
    }
};

struct IntWithError {
    static int counter;
    static constexpr int kAllowedCopies = 3;

    IntWithError() = default;

    IntWithError(int x) : x(x) {
    }

    IntWithError(const IntWithError& other) : x(other.x) {
    }

    IntWithError(IntWithError&& other) : x(other.x) {
    }

    IntWithError& operator=(const IntWithError& other) {
        x = other.x;
        if (++counter == kAllowedCopies) {
            throw std::runtime_error("int throw error");
        }
        return *this;
    }

    IntWithError& operator=(IntWithError&& other) {
        x = other.x;
        if (++counter == kAllowedCopies) {
            throw std::runtime_error("int throw error");
        }
        return *this;
    }

    bool operator==(const IntWithError& other) const {
        return x == other.x;
    }

    ~IntWithError() = default;
    int x;
};

size_t StupidHash(int) {
    return 0;
}

int StrangeInt::counter;
int IntWithError::counter;

std::mt19937 rnd(std::chrono::steady_clock::now().time_since_epoch().count());

int Get(int l, int r) {
    return rnd() % (r - l + 1) + l;
}

const int kMinKey = -100, kMaxKey = 100, kMinValue = -100, kMaxValue = 100, kCnt = 2, kN = 3,
          kTests = 1e5;
const std::vector<std::string> kOperation = {"insert", "[]change", "[]",   "erase",
                                             "find",   "=",        "clear"};

void Output(const std::vector<size_t>& ind, const std::vector<std::string>& tp,
            const std::vector<std::vector<int>>& lst) {
    std::cout << "WA\n";
    std::cout << ind.size() << " " << kCnt << "\n";
    for (size_t i = 0; i < ind.size(); i++) {
        std::cout << "index: " << ind[i] << "\n";
        std::cout << tp[i] << " ";
        for (auto j : lst[i]) {
            std::cout << j << " ";
        }
        std::cout << "\n";
    }
}

bool CmpElements(std::unordered_map<int, int>& a, HashMap<int, int>& b) {
    std::vector<std::pair<int, int>> x, y;
    for (auto [key, val] : a) {
        x.push_back({key, val});
    }
    for (auto [key, val] : b) {
        y.push_back({key, val});
    }
    sort(x.begin(), x.end());
    sort(y.begin(), y.end());
    return x == y;
}

bool CmpSize(const std::unordered_map<int, int>& a, const HashMap<int, int>& b) {
    return a.size() == b.Size();
}

bool CmpPair(const std::unordered_map<int, int>& a, const HashMap<int, int>& b, int key) {
    auto it = a.find(key);
    auto it2 = b.Find(key);
    if (it == a.end() && it2 == b.end()) {
        return true;
    }
    if (it == a.end() || it2 == b.end()) {
        return false;
    }
    return it->second == it2->second;
}
}  // namespace test_utils

namespace std {
template <>
struct hash<test_utils::StrangeInt> {
    size_t operator()(const test_utils::StrangeInt& x) const {
        return x.x;
    }
};
}  // namespace std

namespace std {
template <>
struct hash<test_utils::IntWithError> {
    size_t operator()(const test_utils::IntWithError& x) const {
        return x.x;
    }
};
}  // namespace std

TEST_CASE("Const check") {
    const HashMap<int, int> map{{1, 5}, {3, 4}, {2, 1}};
    REQUIRE(!map.Empty());

    REQUIRE(std::is_same<HashMap<int, int>::const_iterator, decltype(map.begin())>::value);
    auto hash_f = map.HashFunction();
    REQUIRE(hash_f(0) == std::hash<int>()(0));

    HashMap<int, int>::const_iterator it = map.Find(3);
    REQUIRE(it->second == 4);
    it = map.Find(7);
    REQUIRE(it == map.end());

    REQUIRE(std::is_same<const int, std::remove_reference<decltype(map.At(1))>::type>::value);
}

TEST_CASE("Exception check") {
    const HashMap<int, int> map{{2, 3}, {-7, -13}, {0, 8}};
    try {
        auto cur = map.At(8);
    } catch (const std::out_of_range& e) {
        REQUIRE(1);
        return;
    } catch (...) {
        REQUIRE(0);
    }
    REQUIRE(0);
}

TEST_CASE("Destructor check") {
    test_utils::StrangeInt::Init();
    {
        HashMap<test_utils::StrangeInt, int> s{{5, 4}, {3, 2}, {1, 0}};
        REQUIRE(s.Size() == 3);
    }
    REQUIRE(test_utils::StrangeInt::counter == 0);
    {
        HashMap<test_utils::StrangeInt, int> s{{-3, 3}, {-2, 2}, {-1, 1}};
        HashMap<test_utils::StrangeInt, int> s1(s);
        s1.Insert(std::make_pair(0, 0));
        HashMap<test_utils::StrangeInt, int> s2(s1);
        REQUIRE(s1.Find(0) != s1.end());
    }
    REQUIRE(test_utils::StrangeInt::counter == 0);
}

TEST_CASE("Reference check") {
    HashMap<int, int> map{{3, 4}, {3, 5}, {4, 7}, {-1, -3}};
    map[3] = 7;
    REQUIRE((map[3] == 7 && map[0] == 0));
    auto it = map.Find(4);
    REQUIRE(it != map.end());
    it->second = 3;
    auto cur = map.Find(4);
    REQUIRE(cur->second == 3);
}

TEST_CASE("Hash check") {
    struct Hasher {
        std::hash<std::string> hasher;
        size_t operator()(const std::string& s) const {
            return hasher(s);
        }
    };
    HashMap<std::string, std::string, Hasher> map{
        {"aba", "caba"}, {"simple", "case"}, {"test", "test"}};
    auto simple_hash = [](unsigned long long x) -> size_t { return x % 17239; };
    HashMap<int, std::string, decltype(simple_hash)> second_map(simple_hash);
    second_map.Insert(std::make_pair(0, "a"));
    second_map.Insert(std::make_pair(0, "b"));
    second_map[17239] = "check";
    auto second_hash_fn = second_map.HashFunction();
    REQUIRE(second_hash_fn(17239) == 0);
    REQUIRE((second_map[0] == "a" && second_map[17239] == "check"));

    HashMap<int, int, std::function<size_t(int)>> stupid_map(test_utils::StupidHash);
    auto stupid_hash_fn = stupid_map.HashFunction();
    for (int i = 0; i < 1000; ++i) {
        stupid_map[i] = i + 1;
        REQUIRE(stupid_hash_fn(i) == 0);
    }
    REQUIRE(stupid_map.Size() == 1000);
}

TEST_CASE("Copy check") {
    HashMap<int, int> first;
    HashMap<int, int> second(first);
    second.Insert(std::make_pair(1, 1));
    HashMap<int, int> third(second.begin(), second.end());
    third[0] = 5;
    REQUIRE(third.Size() == 2);
    first = third;
    second = second = first;
    REQUIRE(first.Find(0)->second == 5);
    REQUIRE(second[0] == 5);
}

TEST_CASE("Iterators check") {
    {
        HashMap<int, int> first{{0, 0}};
        HashMap<int, int>::iterator just_iterator;
        HashMap<int, int>::iterator it = first.begin();
        static_assert(
            std::is_same<const int, std::remove_reference<decltype(it->first)>::type>::value,
            "Iterator's key type isn't const");
        REQUIRE(it++ == first.begin());
        REQUIRE(it == first.end());
        REQUIRE(++first.begin() == first.end());
        first.Erase(0);
        REQUIRE(first.begin() == first.end());
        just_iterator = first.begin();
    }

    {
        const HashMap<int, int> first{{1, 1}};
        HashMap<int, int>::const_iterator just_iterator;
        HashMap<int, int>::const_iterator it = first.begin();
        REQUIRE(it++ == first.begin());
        REQUIRE(it == first.end());
        REQUIRE(++first.begin() == first.end());
        just_iterator = it;
    }
}

TEST_CASE("Add numbers from 1 to 10^6") {
    HashMap<int, int> mp;
    for (int i = 0; i < 1'000'000; i++) {
        mp[i] = i;
        REQUIRE(mp[i] == i);
        REQUIRE(mp.Size() == i + 1);
    }
}

TEST_CASE("Range-based for check") {
    std::vector<std::pair<std::string, int>> expected{{"aba", 3}, {"ca", 5}, {"ba", 7}};
    std::sort(expected.begin(), expected.end());
    HashMap<std::string, int> mp;
    for (const auto& [str, num] : expected) {
        mp[str] = num;
    }
    std::vector<std::pair<std::string, int>> result;
    for (const auto& [str, num] : mp) {
        result.emplace_back(str, num);
    }
    std::sort(result.begin(), result.end());
    REQUIRE(expected == result);
}

TEST_CASE("Consistency check") {
    HashMap<test_utils::IntWithError, int> mp;
    mp[1] = 1;
    try {
        mp[2] = 2;
    } catch (...) {
    }
    REQUIRE(mp.Size() == 1);
    REQUIRE(mp.Find(1) != mp.end());
}

TEST_CASE("Stress test") {
    for (int test = 0; test < test_utils::kTests; test++) {
        std::vector<size_t> ind;
        std::vector<std::string> tp;
        std::vector<std::vector<int>> lst;
        std::vector<std::unordered_map<int, int>> mp(test_utils::kCnt);
        std::vector<HashMap<int, int>> hash_map(test_utils::kCnt);
        for (int iter = 0; iter < test_utils::kN; iter++) {
            size_t index = test_utils::Get(0, test_utils::kCnt - 1);
            ind.push_back(index);
            std::string request =
                test_utils::kOperation[test_utils::Get(0, test_utils::kOperation.size() - 1)];
            tp.push_back(request);

            std::unordered_map<int, int>& a = mp[index];
            HashMap<int, int>& b = hash_map[index];
            if (request.front() == 'i') {
                int key = test_utils::Get(test_utils::kMinKey, test_utils::kMaxKey),
                    val = test_utils::Get(test_utils::kMinValue, test_utils::kMaxValue);
                lst.push_back({key, val});
                a.insert({key, val});
                b.Insert({key, val});
                if (!test_utils::CmpPair(a, b, key)) {
                    test_utils::Output(ind, tp, lst);
                }
                REQUIRE(test_utils::CmpPair(a, b, key));
            } else if (request[0] == '[' && request[2] == 'c') {
                int key = test_utils::Get(test_utils::kMinKey, test_utils::kMaxKey),
                    val = test_utils::Get(test_utils::kMinValue, test_utils::kMaxValue);
                lst.push_back({key, val});
                a[key] = val;
                b[key] = val;
                if (!test_utils::CmpPair(a, b, key)) {
                    test_utils::Output(ind, tp, lst);
                }
                REQUIRE(test_utils::CmpPair(a, b, key));
            } else if (request[0] == '[') {
                int key = test_utils::Get(test_utils::kMinKey, test_utils::kMaxKey);
                lst.push_back({key});
                a[key];
                b[key];
                if (!test_utils::CmpPair(a, b, key)) {
                    test_utils::Output(ind, tp, lst);
                }
                REQUIRE(test_utils::CmpPair(a, b, key));
            } else if (request[0] == 'e') {
                int key = test_utils::Get(test_utils::kMinKey, test_utils::kMaxKey);
                lst.push_back({key});
                a.erase(key);
                b.Erase(key);
                if (!test_utils::CmpPair(a, b, key)) {
                    test_utils::Output(ind, tp, lst);
                }
                REQUIRE(test_utils::CmpPair(a, b, key));
            } else if (request[0] == 'f') {
                int key = test_utils::Get(test_utils::kMinKey, test_utils::kMaxKey);
                lst.push_back({key});
                auto it = a.find(key);
                auto it2 = b.Find(key);
                if (!test_utils::CmpPair(a, b, key)) {
                    test_utils::Output(ind, tp, lst);
                }
                REQUIRE(test_utils::CmpPair(a, b, key));
            } else if (request[0] == '=') {
                int id = test_utils::Get(0, test_utils::kCnt - 1);
                lst.push_back({id});
                a = mp[id];
                b = hash_map[id];
            } else if (request[0] == 'c') {
                lst.push_back({});
                a.clear();
                b.Clear();
            }
            if (!test_utils::CmpSize(a, b)) {
                test_utils::Output(ind, tp, lst);
            }
            REQUIRE(test_utils::CmpSize(a, b));
            if (!test_utils::CmpElements(a, b)) {
                test_utils::Output(ind, tp, lst);
            }
            REQUIRE(test_utils::CmpElements(a, b));
        }
    }
}
