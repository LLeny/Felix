#pragma once
#include <openssl/bn.h>
#include "Log.hpp"
#include <algorithm>
#include <iostream>
#include <vector>

#define  DECRYPT_BLOCK_SIZE (51)

std::vector<uint8_t> decrypt(size_t blockcount, std::span<uint8_t const> encrypted);
