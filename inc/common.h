#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define ONWINDOWS true
#define ONLINUX false
#define POPEN _popen
#define PCLOSE _pclose
#elif __linux__
#define ONWINDOWS false
#define ONLINUX true
#define POPEN popen
#define PCLOSE pclose
#else
#   error "Not windows or linux"
#endif
//source: https://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor

#include <tuple>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <ctime>
#include <numeric>
#include <fstream>
#include <algorithm>
#include <iostream>     // std::cout, std::fixed
#include <iomanip>      // std::setprecision
#include <cassert>
#include <boost/heap/pairing_heap.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
using boost::heap::pairing_heap;
using boost::heap::compare;
using boost::unordered_map;
using boost::unordered_set;
using boost::optional;
using std::vector;
using std::list;
using std::set;
using std::stack;
using std::get;
using std::tuple;
using std::make_tuple;
using std::pair;
using std::make_pair;
using std::tie;
using std::min;
using std::max;
using std::shared_ptr;
using std::make_shared;
using std::clock;
using std::cout;
using std::endl;
using std::ofstream;
using std::cerr;
using std::string;

// #define NDEBUG 

#define MAX_TIMESTEP INT_MAX / 2
#define MAX_COST INT_MAX / 2
#define MAX_NODES INT_MAX / 2

struct PathEntry
{
	int location = -1;
	// bool single = false;
  // int mdd_width;

  // bool is_single() const {
  //  return mdd_width == 1;
  //}
	PathEntry(int loc = -1) { location = loc; }
};

typedef vector<PathEntry> Path;
std::ostream& operator<<(std::ostream& os, const Path& path);

bool isSamePath(const Path& p1, const Path& p2);

// Only for three-tuples of std::hash-able types for simplicity.
// You can of course template this struct to allow other hash functions
/*struct three_tuple_hash {
    template <class T1, class T2, class T3>
    std::size_t operator () (const std::tuple<T1, T2, T3> &p) const {
        auto h1 = std::hash<T1>{}(get<0>(p));
        auto h2 = std::hash<T2>{}(get<1>(p));
        auto h3 = std::hash<T3>{}(get<2>(p));
        // Mainly for demonstration purposes, i.e. works but is overly simple
        // In the real world, use sth. like boost.hash_combine
        return h1 ^ h2 ^ h3;
    }
};*/

template <typename T>
vector<int> sort_indexes(const vector<T>& v);

std::vector<std::string> Split(const std::string& str, char delim, bool only_fisrt = false);

vector<double> normalize_vector(const vector<double>& to_normalize);

template <typename V>
void transpose(vector<vector<V> >& b);

vector<vector<double>> normalizeFeatureVectors(vector<vector<double>>& raw_features);

int getDependencyPairCount(vector<set<int>>& dependency_graph);

vector<int> softmax_ordering(vector<int>& agents_to_arrange, vector<double>& predictions_to_arrange);

double fRand(double fMin, double fMax);

template <typename W>
vector<double> softmax_vector(const vector<W>& to_normalize, double beta)
{
    vector<double> retvec(to_normalize.size(), 0);
    std::transform(to_normalize.begin(), to_normalize.end(), retvec.begin(), [&beta](W d) -> double {
        return std::exp(beta * d);
        });

    double sum_of_expo = std::accumulate(retvec.begin(), retvec.end(), 0.0);
    std::for_each(retvec.begin(), retvec.end(), [&sum_of_expo](double& d) { d = d / sum_of_expo; });

    return retvec;
}

void MLRandomRestart(const vector<vector<double>>& all_testfile_dotprod,
    const vector<vector<int>>& all_testfile_orderings,
    vector<vector<int>>& orderings, int scen, bool softmax, 
    double bias=0.4, double target_sum=100.0, double softmax_beta=0.5);
