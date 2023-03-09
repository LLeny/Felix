#include "pch.hpp"
#include "Encryption.hpp"

uint8_t decrypt( std::span<uint8_t const> encrypted, int& accumulator, std::vector<uint8_t>& result )
{
  BN_CTX *ctx = BN_CTX_new();
  BIGNUM *lynxpubmod = BN_new();
  BIGNUM *lynxpubexp = BN_new();
  BIGNUM *decr = BN_new();
  BIGNUM *enc = BN_new();
  unsigned char decrchar[DECRYPT_BLOCK_SIZE + 1] {};

  BN_hex2bn(&lynxpubmod, "35b5a3942806d8a22695d771b23cfd561c4a19b6a3b02600365a306e3c4d63381bd41c136489364cf2ba2a58f4fee1fdac7e79");
  BN_hex2bn(&lynxpubexp, "3");

  std::vector<uint8_t> reversed;

  reversed.insert(reversed.begin(), encrypted.begin(), encrypted.end());
  std::reverse(reversed.begin(), reversed.end());

  BN_bin2bn( reversed.data(), reversed.size(), enc );

  BN_mod_exp( decr, enc, lynxpubexp, lynxpubmod, ctx );

  BN_bn2bin( decr, decrchar );

  for ( size_t i = DECRYPT_BLOCK_SIZE - 1; i > 0; --i )
  {
    accumulator += decrchar[i];
    result.push_back( accumulator );
  }

  BN_clear_free(enc);
  BN_clear_free(decr);
  BN_clear_free(lynxpubmod);
  BN_clear_free(lynxpubexp);
  BN_CTX_free(ctx);

  return decrchar[0];
}

std::vector<uint8_t> decrypt( size_t blockcount, std::span<uint8_t const> encrypted )
{
  std::vector<uint8_t> result;
  int accumulator = 0;
  for ( size_t i = 0; i < blockcount; ++i )
  {
    uint8_t sanityChek = decrypt( std::span<uint8_t const>{ encrypted.data() + DECRYPT_BLOCK_SIZE * i, DECRYPT_BLOCK_SIZE }, accumulator, result );
    if ( sanityChek != 0x15 )
    {
      L_ERROR << "Sanity check #1 value for block " << i << " is 0x" << std::hex << sanityChek << " != 0x15";
      return {};
    }
  }

  if ( ( accumulator & 0xff ) != 0 )
  {
    L_ERROR << "Sanity check #2 final accumulator value 0x" << std::hex << ( accumulator & 0xff ) << " != 0x00";
    return {};

  }
  return result;
}
