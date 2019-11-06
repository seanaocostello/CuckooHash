#ifndef HORTONHASHBUCKET_H
#define HORTONHASHBUCKET_H

#include <stdint.h>
#include <vector>

class HHBucket{
	public:
		HHBucket(int capacity);
		int getCountEmpty();
		bool insertIfAvailable(uint32_t key, uint32_t value);
		void shuffleKeysBack();
		uint32_t get(uint32_t key);
		uint64_t getKVPair(uint32_t key, bool index);
		uint64_t convertToTypeB();
		void revertToTypeA(uint64_t n);
		uint32_t getKey(int i);
		int getCapacity();
		bool isTypeA();
		void setRedirectListEntry(int offset, uint8_t value);
		uint8_t getRedirectListEntry(int offset);
		void clearKVPair(int i);
		uint64_t pair(uint64_t pair, int index);
	private:
		uint64_t * vals;
		int cap;
		
};

class HHMap{
	public:
		HHMap(int numBuckets, int cap);
		bool containsKey(uint32_t key);
		uint32_t put(uint32_t key, uint32_t value);
		uint32_t get(uint32_t key);
		uint32_t hash(uint32_t key, int type);
	private:
		uint64_t getKV(uint32_t key);
		uint32_t checkedPut(uint32_t key, uint32_t value);
		void secondaryInsert(uint32_t key, uint32_t value);
		bool displaceItems(int bucketIdx);
		uint8_t findBestHashFunc(uint32_t key);
		bool isPrimary(int bucketIdx, int itemIdx);
		int getPrimaryBucket(uint32_t key);
		int getPrimaryBucket(int bucketIdx, int itemIdx);
		uint32_t getSecondaryBucket(uint32_t key, uint8_t hashFunc);
		int getTag(uint32_t key);
		HHBucket ** buckets;
		int cap;
		int numBuckets;
		int bNB;

};

struct KV{
	uint32_t key = 0;
	uint32_t value = 0;
};

class CuckooMap{
	public:
		CuckooMap(int size);
		uint32_t put(uint32_t key, uint32_t value);
		uint32_t put(KV * pair);
		KV * get(uint32_t key);
	private:
		uint32_t hash(uint32_t key, bool right);
		bool displaceItems(KV * pair, bool first);
		int leftOrRight(uint32_t key);
		int numBuckets;
		int bNB;
		KV ** pairs;
		
};

class CuckooBucket{
	public:
		CuckooBucket(int capacity);
		int getCountEmpty();
		bool insertIfAvailable(uint32_t key, uint32_t value);
		void shuffleKeysBack();
		uint32_t get(uint32_t key);
		void clearPair(int index);
		KV * getKVPair(uint32_t key, bool index);
	private:
		KV * vals;
		int cap;
};

class BCH{
	public:
		BCH(int size, int capacity);
		uint32_t put(uint32_t key, uint32_t value);
		uint32_t put(KV * pair);
		KV * get(uint32_t key);
	private:
		uint32_t hash(uint32_t key, bool right);
		bool displaceItems(KV * pair, std::vector<int> path);
		std::vector<int> leftOrRight(uint32_t key);
		int alternate(uint32_t key, int index);
		CuckooBucket ** buckets;
		int cap;
		int numBuckets;
		int bNB;
};

class CFB{
	public:
		CFB(int capacity);
		int getCountEmpty();
		bool insertIfAvailable(uint8_t fP);
		bool has(uint8_t fP);
		void clearFP(uint8_t fP);
		uint8_t get(int index);
	private:
		uint8_t * fPs;
		int capacity;
};

class CuckooFilter{
	public:
		CuckooFilter(int size, int capacity);
		uint32_t insert(uint32_t key);
		bool get(uint32_t key);
		bool remove(uint32_t key);
	private:
		uint8_t fingerPrint(uint32_t key);
		uint32_t hash(uint32_t key);
		uint32_t alternate(uint8_t fP, uint32_t index);
		bool displaceItems(uint32_t key, std::vector<int> path);
		std::vector<int> leftOrRight(uint32_t key);
		int cap;
		int numBuckets;
		int bNB;
		CFB ** buckets;
};



#endif