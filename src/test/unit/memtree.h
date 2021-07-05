#pragma once
#include <string>
#include <sstream>

struct memtree {
  memtree *parent{};
  int value;
  memtree* left{};
  memtree* right{};

  std::string str() const {
    std::stringstream ss;
    ss << "node at " << this << " value is " << value << " parent at " << parent << " left " << left << " right " << right << std::endl;
    return ss.str();
  }
};
