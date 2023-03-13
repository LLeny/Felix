#include "pch.hpp"
#include "Encryption.hpp"

// Wookie's decrpytion : https://forums.atariage.com/topic/129030-lynx-encryption/page/1/

/* result = 2 * result */
void double_value(unsigned char *result, const int length)
{
    int i, x;

    x = 0;
    for (i = length - 1; i >= 0; i--) 
    {
	    x += 2 * result[i];
	    result[i] = (unsigned char) (x & 0xFF);
	    x >>= 8;
    }
    /* shouldn't carry */
}

/* result -= value */
int minus_equals_value(unsigned char *result, 
                       const unsigned char *value, 
                       const int length)
{
    int i, x;
    unsigned char *tmp;

    /* allocate temporary buffer */
    tmp = (unsigned char*)calloc(1, length);

    x = 0;
    for (i = length - 1; i >= 0; i--) 
    {
	    x += result[i] - value[i];
	    tmp[i] = (unsigned char) (x & 0xFF);
	    x >>= 8;
    }

    if (x >= 0) 
    {
        /* move the result back to BB */
        memcpy(result, tmp, length);
        
        /* free the temporary buffer */
        free(tmp);

        /* this had a carry */
        return 1;
    }

    /* free the temporary buffer */
    free(tmp);

    /* this didn't carry */
    return 0;
}

/* result += value */
void plus_equals_value(unsigned char *result, 
                       const unsigned char *value, 
                       const int length)
{
    int i, tmp;
    int carry = 0;

    for(i = length - 1; i >= 0; i--) 
    {
	    tmp = result[i] + value[i] + carry;
	    if (tmp >= 256)
	        carry = 1;
	    else
	        carry = 0;
	    result[i] = (unsigned char) (tmp);
    }
}

/* L = M * N mod modulus */
void lynx_mont(unsigned char *L,            /* result */
               const unsigned char *M,      /* original chunk of encrypted data */
               const unsigned char *N,      /* copy of encrypted data */
               const unsigned char *modulus,/* modulus */
		       const int length)
{
    int i, j;
    int carry;
    unsigned char tmp;
    unsigned char increment;

    /* L = 0 */
    memset(L, 0, length);

    for(i = 0; i < length; i++)
    {
        /* get the next byte from N */
	    tmp = N[i];

        for(j = 0; j < 8; j++) 
        {
            /* L = L * 2 */
	        double_value(L, length);

	        /* carry is true if the MSB in tmp is set */
            increment = (tmp & 0x80) / 0x80;

            /* shift tmp's bits to the left by one */
	        tmp <<= 1;
	   
            if(increment != 0) 
            {
                /* increment the result... */
                /* L += M */
		        plus_equals_value(L, M, length);

                /* do a modulus correction */
                /* L -= modulus */
                carry = minus_equals_value(L, modulus, length);

                /* if there was a carry, do it again */
                /* L -= modulus */
                if (carry != 0)
                    minus_equals_value(L, modulus, length);
            } 
            else
            {
                /* instead decrement the result */

                /* L -= modulus */
                minus_equals_value(L, modulus, length);
            }
        }
    }
}


/* this decrypts a single block of encrypted data by using the montgomery
 * multiplication method to do modular exponentiation.
 */
int decrypt_block(int accumulator,
                  unsigned char * result,
                  const unsigned char * encrypted,
                  const unsigned char * public_exp,
                  const unsigned char * public_mod,
                  const int length)
{
    int i;
    unsigned char* rptr = result;
    const unsigned char* eptr = encrypted;
    unsigned char *A;
    unsigned char *B;
    unsigned char *TMP;

    /* allocate the working buffers */
    A = (unsigned char*)calloc(1, length);
    B = (unsigned char*)calloc(1, length);
    TMP = (unsigned char*)calloc(1, length);

    /* this copies the next length sized block of data from the encrypted
     * data into our temporary memory buffer in reverse order */
    for(i = length - 1; i >= 0; i--) 
    {
        B[i] = *eptr;
        eptr++;
    }

    /* so it took me a while to wrap my head around this because I couldn't
     * figure out how the exponent was used in the process.  RSA is 
     * a ^ b (mod c) and I couldn't figure out how that was being done until
     * I realized that the public exponent for lynx decryption is just 3.  That
     * means that to decrypt each block, we only have to multiply each
     * block by itself twice to raise it to the 3rd power:
     * n^3 == n * n * n
     */

    /* do Montgomery multiplication: A = B^2 */
    lynx_mont(A, B, B, public_mod, length);

    /* copy the result into the temp buffer: TMP = B^2 */
    memcpy(TMP, A, length);

    /* do Montgomery multiplication again: A = B^3 */
    lynx_mont(A, B, TMP, public_mod, length);

    /* So I'm not sure if this is part of the Montgomery multiplication 
     * algorithm since I don't fully understand how that works.  This may be
     * just another obfuscation step done during the encryption process. 
     * The output of the decryption process has to be accumulated and masked
     * to get the original bytes.  If I had to place a bet, I would bet that
     * this is not part of Montgomery multiplication and is just an obfuscation
     * preprocessing step done on the plaintext data before it gets encrypted.
     */
    for(i = length - 1; i > 0; i--)
    {
        accumulator += A[i];
        accumulator &= 0xFF;
        (*rptr) = (unsigned char)(accumulator);
        rptr++;
    }
    
    /* free the temporary buffer memory */
    free(A);
    free(B);
    free(TMP);

    return accumulator;
}

void decrypt( std::span<uint8_t const> encrypted, int& accumulator, std::vector<uint8_t>& result )
{
  unsigned char res[DECRYPT_BLOCK_SIZE];

  accumulator = decrypt_block( accumulator, res, encrypted.data(), lynx_public_exp, lynx_public_mod, encrypted.size() );

  for ( size_t i = 0; i < DECRYPT_BLOCK_SIZE - 1; ++i )
  {
    result.push_back( res[i] );
  }
}

std::vector<uint8_t> decrypt( size_t blockcount, std::span<uint8_t const> encrypted )
{
  std::vector<uint8_t> result;
  int accumulator = 0;
  for ( size_t i = 0; i < blockcount; ++i )
  {
    decrypt( std::span<uint8_t const>{ encrypted.data() + DECRYPT_BLOCK_SIZE * i, DECRYPT_BLOCK_SIZE }, accumulator, result );
  }

  if ( ( accumulator & 0xff ) != 0 )
  {
    L_ERROR << "Sanity check #2 final accumulator value 0x" << std::hex << ( accumulator & 0xff ) << " != 0x00";
    return {};

  }
  return result;
}
