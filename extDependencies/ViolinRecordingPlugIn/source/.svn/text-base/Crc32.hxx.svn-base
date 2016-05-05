#ifndef INCLUDED_CRC32_HXX
#define INCLUDED_CRC32_HXX

#include "concat/Utilities/StdInt.hxx"

namespace concat
{
	// Cyclic Redundancy Check
	//
	// Takes a binary message, converts it to polynomial, divides this polynomial by another 
	// predetermined polynomial (key) to get CRC.
	//
	// CRC-32-IEEE 802.3 (Ethernet protocol)
	// using key polynomial x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1 
	// which corresponds to the binary key 0x04c11db7

	inline void computeCrc32Table(uint32_t *table256, uint32_t key = 0x04c11db7)
	{
		for (uint32_t i = 0; i < 256; ++i)
		{
			uint32_t reg = i << 24;

			for (int j = 0; j < 8; ++j)
			{
				bool isTopBit = ((reg & 0x80000000) != 0);
				reg <<= 1;
				if (isTopBit)
					reg ^= key;
			}

			table256[i] = reg;
		}
	}

	inline uint32_t computeCrc32(const uint32_t *table256, const byte *data, int numBytes)
	{
		uint32_t crc = 0;
		for (int i = 0; i < numBytes; ++i)
		{
			uint32_t top = crc >> 24;
			top ^= data[i];
			crc = (crc << 8) ^ table256[top];
		}
		return crc;
	}

	inline uint32_t computeCrc32Running(const uint32_t *table256, byte data, uint32_t prevCrc)
	{
		uint32_t crc = prevCrc;
		uint32_t top = crc >> 24;
		top ^= data;
		crc = (crc << 8) ^ table256[top];
		return crc;
	}
}

#endif