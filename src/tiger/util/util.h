#ifndef TIGER_UTIL_UTIL_H_
#define TIGER_UTIL_UTIL_H_

#include <set>

namespace U {

class BoolList {
 public:
  bool head;
  BoolList* tail;

  BoolList(bool head, BoolList* tail) : head(head), tail(tail) {}
};

template<typename T>
std::set<T> set_union(std::set<T> s1, std::set<T> s2) {
  typename std::set<T>::iterator it = s1.begin();
  std::set<T> res;
  for(; it != s1.end(); it++ ) {
    res.insert(*it);
  }
  for(it = s2.begin(); it != s2.end(); it++ ) {
    res.insert(*it);
  }
  return res;
}

template<typename T>
std::set<T> set_intersect(std::set<T> s1, std::set<T> s2) {
  typename std::set<T>::iterator it = s2.begin();
  std::set<T> res;
  for(; it != s2.end(); it++ ) {
    typename std::set<T>::iterator target = s1.find(*it);
    if (target != s1.end()) {
      res.insert(*it);
    }
  }
  return res;
}

template<typename T>
std::set<T> set_difference(std::set<T> s1, std::set<T> s2) {
  typename std::set<T>::iterator it = s1.begin();
  std::set<T> res;
  for(; it != s1.end(); it++ ) {
    typename std::set<T>::iterator target = s2.find(*it);
    if (target == s2.end()) {
      res.insert(*it);
    }
  }
  return res;
}

}  // namespace U

#endif  // TIGER_UTIL_UTIL_H_