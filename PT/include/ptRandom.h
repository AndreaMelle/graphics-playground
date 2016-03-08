#ifndef __PT_RANDOM_H__
#define __PT_RANDOM_H__

#include <random>

namespace pt
{
	class Hash
	{
	public:
		virtual unsigned int operator()(int i) = 0;
		virtual unsigned int operator()(int i, int j) = 0;
	};

	inline int rot(int x, int b) {
		return (x << b) ^ (x >> (32 - b));
	}

	// TODO: according to http://blogs.unity3d.com/2015/01/07/a-primer-on-repeatable-random-numbers/
	// xxHash is better

	/*
	 public class XXHash : HashFunction {
	private uint seed;
	
	const uint PRIME32_1 = 2654435761U;
	const uint PRIME32_2 = 2246822519U;
	const uint PRIME32_3 = 3266489917U;
	const uint PRIME32_4 = 668265263U;
	const uint PRIME32_5 = 374761393U;
	
	public XXHash (int seed) {
		this.seed = (uint)seed;
	}
	
	public uint GetHash (byte[] buf) {
		uint h32;
		int index = 0;
		int len = buf.Length;
		
		if (len >= 16) {
			int limit = len - 16;
			uint v1 = seed + PRIME32_1 + PRIME32_2;
			uint v2 = seed + PRIME32_2;
			uint v3 = seed + 0;
			uint v4 = seed - PRIME32_1;
			
			do {
				v1 = CalcSubHash (v1, buf, index);
				index += 4;
				v2 = CalcSubHash (v2, buf, index);
				index += 4;
				v3 = CalcSubHash (v3, buf, index);
				index += 4;
				v4 = CalcSubHash (v4, buf, index);
				index += 4;
			} while (index <= limit);
			
			h32 = RotateLeft (v1, 1) + RotateLeft (v2, 7) + RotateLeft (v3, 12) + RotateLeft (v4, 18);
		}
		else {
			h32 = seed + PRIME32_5;
		}
		
		h32 += (uint)len;
		
		while (index <= len - 4) {
			h32 += BitConverter.ToUInt32 (buf, index) * PRIME32_3;
			h32 = RotateLeft (h32, 17) * PRIME32_4;
			index += 4;
		}
		
		while (index<len) {
			h32 += buf[index] * PRIME32_5;
			h32 = RotateLeft (h32, 11) * PRIME32_1;
			index++;
		}
		
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		
		return h32;
	}
	
	public uint GetHash (params uint[] buf) {
		uint h32;
		int index = 0;
		int len = buf.Length;
		
		if (len >= 4) {
			int limit = len - 4;
			uint v1 = seed + PRIME32_1 + PRIME32_2;
			uint v2 = seed + PRIME32_2;
			uint v3 = seed + 0;
			uint v4 = seed - PRIME32_1;
			
			do {
				v1 = CalcSubHash (v1, buf[index]);
				index++;
				v2 = CalcSubHash (v2, buf[index]);
				index++;
				v3 = CalcSubHash (v3, buf[index]);
				index++;
				v4 = CalcSubHash (v4, buf[index]);
				index++;
			} while (index <= limit);
			
			h32 = RotateLeft (v1, 1) + RotateLeft (v2, 7) + RotateLeft (v3, 12) + RotateLeft (v4, 18);
		}
		else {
			h32 = seed + PRIME32_5;
		}
		
		h32 += (uint)len * 4;
		
		while (index < len) {
			h32 += buf[index] * PRIME32_3;
			h32 = RotateLeft (h32, 17) * PRIME32_4;
			index++;
		}
		
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		
		return h32;
	}
	
	public override uint GetHash (params int[] buf) {
		uint h32;
		int index = 0;
		int len = buf.Length;
		
		if (len >= 4) {
			int limit = len - 4;
			uint v1 = (uint)seed + PRIME32_1 + PRIME32_2;
			uint v2 = (uint)seed + PRIME32_2;
			uint v3 = (uint)seed + 0;
			uint v4 = (uint)seed - PRIME32_1;
			
			do {
				v1 = CalcSubHash (v1, (uint)buf[index]);
				index++;
				v2 = CalcSubHash (v2, (uint)buf[index]);
				index++;
				v3 = CalcSubHash (v3, (uint)buf[index]);
				index++;
				v4 = CalcSubHash (v4, (uint)buf[index]);
				index++;
			} while (index <= limit);
			
			h32 = RotateLeft (v1, 1) + RotateLeft (v2, 7) + RotateLeft (v3, 12) + RotateLeft (v4, 18);
		}
		else {
			h32 = (uint)seed + PRIME32_5;
		}
		
		h32 += (uint)len * 4;
		
		while (index < len) {
			h32 += (uint)buf[index] * PRIME32_3;
			h32 = RotateLeft (h32, 17) * PRIME32_4;
			index++;
		}
		
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		
		return h32;
	}
	
	public override uint GetHash (int buf) {
		uint h32 = (uint)seed + PRIME32_5;
		h32 += 4U;
		h32 += (uint)buf * PRIME32_3;
		h32 = RotateLeft (h32, 17) * PRIME32_4;
		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;
		return h32;
	}
	
	private static uint CalcSubHash (uint value, byte[] buf, int index) {
		uint read_value = BitConverter.ToUInt32 (buf, index);
		value += read_value * PRIME32_2;
		value = RotateLeft (value, 13);
		value *= PRIME32_1;
		return value;
	}
	
	private static uint CalcSubHash (uint value, uint read_value) {
		value += read_value * PRIME32_2;
		value = RotateLeft (value, 13);
		value *= PRIME32_1;
		return value;
	}
	
	private static uint RotateLeft (uint value, int count) {
		return (value << count) | (value >> (32 - count));
	}
}
	*/

	/*
	 * https://groups.google.com/d/msg/proceduralcontent/AuvxuA1xqmE/mnw4AubN4jkJ
	 */
	class PcgHash : public Hash
	{
	public:
		unsigned int operator()(int i) override
		{
			int a = i;
			int b = 0;
			for (int r = 0; r < 3; r++) {
				a = rot((int)((a ^ 0xcafebabe) + (b ^ 0xfaceb00c)), 23);
				b = rot((int)((a ^ 0xdeadbeef) + (b ^ 0x8badf00d)), 5);
			}
			return (unsigned int)(a^b);
		}

		unsigned int operator()(int i, int j) override
		{
			int a = i;
			int b = j;
			for (int r = 0; r < 3; r++) {
				a = rot((int)((a ^ 0xcafebabe) + (b ^ 0xfaceb00c)), 23);
				b = rot((int)((a ^ 0xdeadbeef) + (b ^ 0x8badf00d)), 5);
			}
			return (unsigned int)(a^b);
		}
	};

	class URNG
	{
	public:
		URNG(unsigned int _seed = 0)
		{
			gen = std::default_random_engine(_seed);
			udist = std::uniform_real_distribution<double>(0.0, 1.0);
		}

		double operator()() { return udist(gen); }

		void seed(unsigned int _seed)
		{
			gen = std::default_random_engine(_seed);
		}

		std::default_random_engine gen;
		std::uniform_real_distribution<double> udist;
	};
}

#endif //__PT_RANDOM_H__