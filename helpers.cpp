#include "helpers.hpp"

// to use this file in a contract, include this file in the .cpp file of the contract

bool isLowerThanPeggedPrice(const float currentPrice) {
  return currentPrice < 1.0;
}
