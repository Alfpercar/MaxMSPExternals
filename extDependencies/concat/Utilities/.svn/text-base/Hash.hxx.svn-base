#ifndef INCLUDED_CONCAT_HASH_HXX
#define INCLUDED_CONCAT_HASH_HXX

#include <cstring>
#include "concat/Utilities/StdInt.hxx"

namespace concat
{
	inline uint32_t computeJenkinsOneTimeHash(const unsigned char *key, size_t key_len)
	{
		uint32_t hash = 0;
		size_t i;

		for (i = 0; i < key_len; i++)
		{
			hash += key[i];
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}

		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);

		return hash;
	}
}

#endif