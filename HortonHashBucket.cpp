#include "HortonHashBucket.h"
#include <iostream>
#include <stdint.h>

static uint64_t tA = (uint64_t)1 << 63;

static uint32_t hashVals [] = {3407581321,3600022299,1834253069,3396241081,1669016513,292805657,4217835025,2274461705};

uint32_t HHMap::hash(uint32_t key, int type){
	if (!type){
		return ((uint32_t)(((uint64_t)hashVals[type]*key) >> (32-bNB))) % numBuckets;
	}
	else {
		return (hash(key,0) + getTag(key)*hashVals[type]) % numBuckets;
	}
}

int HHMap::getTag(uint32_t key){
	return ((uint32_t)((uint64_t)2433034849*key) >> 28) % 21;
}

HHBucket::HHBucket(int capacity){
	cap = capacity;
	vals = new uint64_t[cap];
	vals[0] = tA;
	for (int i = 1; i < cap; i++){vals[i] = 0;}
}

int HHBucket::getCountEmpty(){
	int numEmpty = ((vals[0] == tA) && isTypeA()) ? 1 : 0;
	for (int i = 1; i < cap; i++){if (!vals[i]){numEmpty++;} }
	return numEmpty;
}

bool HHBucket::insertIfAvailable(uint32_t key, uint32_t value){
	if (vals[0] == tA){
		vals[0] = (tA | ((uint64_t)key << 32)) + value;
		return true;
	}
	for (int i = 1; i < cap; i++){
		if (!vals[i]){
			vals[i] = ((uint64_t)key << 32) + value;
			return true;
		}
	}
	return false;
}

void HHBucket::shuffleKeysBack(){
	if (vals[0] == tA){
		vals[0] += ((vals[1] << 1) >> 1);
		vals[1] = 0;
	}
	for (int i = 1; i < cap; i++){
		if (!vals[i]){
			for (int j = i+1; j < cap; j++){
				if (vals[j]){
					vals[i] = vals[j];
					vals[j] = 0;
					break;
				}
			}
		}
	}
}

uint32_t HHBucket::get(uint32_t key){
	return (uint32_t)getKVPair(key,0);
}

uint64_t HHBucket::getKVPair(uint32_t key, bool index){
	if (index){
		if (key == 0){return ((vals[key] << 1)>>1);}
		else{return vals[(int)key];}
	}
	else {
		int start = (vals[0] >> 63 == (uint64_t)1) ?  0 : 1;
		if (!start){
			if ((vals[0] << 1) >> 33 << 1 == key << 1){
				return vals[0];
			}
		}
		for (int i = 1; i < cap; i++){
			if (!vals[i]){
				return -1;
			}
			if (vals[i] >> 32 == key){
				return vals[i];
			}
		}
		return -1;
	}
}

uint64_t HHBucket::convertToTypeB(){
	uint64_t temp = (vals[0] << 1) >> 1;
	vals[0] = 0;
	return temp;
}

void HHBucket::revertToTypeA(uint64_t newElem){
	vals[0] = newElem | tA;
}

uint32_t HHBucket::getKey(int i){
	return getKVPair(i,1) >> 32;
}

int HHBucket::getCapacity(){
	int start = 0;
	if ((vals[0] ^ tA) == vals[0]){start++;}
	return cap-start;
}

bool HHBucket::isTypeA(){return (vals[0] >> 63) == (uint64_t)1;}


void HHBucket::setRedirectListEntry(int offset, uint8_t value){
	if (isTypeA()){return;}
	uint64_t oldVal = ((vals[0] << (1 + 3*offset)) >> 61) << (60 - 3*offset);
	vals[0] -= oldVal;
	vals[0] += (uint64_t)value << (60-3*offset);
}

uint8_t HHBucket::getRedirectListEntry(int offset){
	return (uint8_t)((vals[0] << (1 + 3*offset)) >> 61);
}

void HHBucket::clearKVPair(int i){
	if (i == 0 && isTypeA()){vals[i] = tA;}
	else {vals[i] = 0;}
}

HHMap::HHMap(int numBuckets, int capacity){
	//Figure out how to align the memory properly, ask Dad
	buckets = new HHBucket * [numBuckets];
	for (int i = 0; i < numBuckets;i++){buckets[i] = new HHBucket(capacity);}
	this->numBuckets = numBuckets;
	cap = capacity;
	int bits = 0;
	while (numBuckets){
		numBuckets /= 2;
		bits++;
	}
	bNB = bits;
}


uint8_t HHMap::findBestHashFunc(uint32_t key){
	int maxEmpty = 0;
	int index = 9;
	for (int i = 1; i < 8; i++){
		int numEmpty = buckets[hash(key,i)]->getCountEmpty();
		if (numEmpty > maxEmpty){
			maxEmpty = numEmpty;
			index = i;
		}
	}
	return (uint8_t)index;
}

uint32_t HHMap::getSecondaryBucket(uint32_t key, uint8_t hashFunc){
	return hash(key,hashFunc);
}

bool HHMap::displaceItems(int bucketIdx){
	HHBucket * b = buckets[bucketIdx];
	bool moved = false;
	uint8_t secondary = 0;
	for (int i = 0; i < b->getCapacity(); i++){
		if (hash(buckets[bucketIdx]->getKey(i),0) != bucketIdx){
			secondary++;
			secondary*=2;
		}
		else{
			secondary*=2;
		}
	}
	for (int i = cap-1; i >= 0; i--){
		if (secondary%2 == 1){
			int primaryBucketIdx = hash(buckets[bucketIdx]->getKey(i),0);
			uint32_t key = buckets[bucketIdx]->getKey(i);
			int slot = getTag(key);
			uint8_t hashFunc = findBestHashFunc(key);
			if (hashFunc == 9){continue;}
			int newBucket = hash(key,hashFunc);
			int emptySlots = buckets[newBucket]->getCountEmpty();
			if (emptySlots){
				uint64_t n = buckets[bucketIdx]->getKVPair(i,1);
				int f = buckets[newBucket]->insertIfAvailable((uint32_t)(n >> 32),(uint32_t)n);
				buckets[bucketIdx]->clearKVPair(i);
				buckets[primaryBucketIdx]->setRedirectListEntry(slot, hashFunc);
				moved = true;
			}
			buckets[bucketIdx]->shuffleKeysBack();
		}
		secondary /= 2;
	}
	return moved;
}

uint64_t HHMap::getKV(uint32_t key){
	uint64_t pair = 0;
	int primaryBucketIdx = hash(key,0);
	HHBucket * primaryBucket = buckets[primaryBucketIdx];
	pair = primaryBucket->getKVPair(key,0);
	if (pair != (uint64_t)(-1)){return pair;}
	if (primaryBucket->isTypeA()){return (uint64_t)(-1);}
	int tag = getTag(key);
	uint8_t hashFunc = primaryBucket->getRedirectListEntry(tag);
	if (!hashFunc){return (uint64_t)(-1);}
	int secondaryBucketIdx = hash(key,hashFunc);
	return buckets[secondaryBucketIdx]->getKVPair(key,0);
}

uint32_t HHMap::checkedPut(uint32_t key, uint32_t value){
	uint64_t pair = getKV(key);
	if (pair != (uint64_t)(-1)){
		//Fix later if time
		uint32_t temp = (uint32_t)pair;
		pair -= temp;
		pair += value;
		std::cout << "WHY?" << std::endl;
		return temp;
	}
	int bucketIdx = hash(key,0);
	HHBucket * primary = buckets[bucketIdx];
	if (primary->insertIfAvailable(key,value)){return 0;}
	if (!displaceItems(bucketIdx)){
		if (primary->isTypeA()){
			uint64_t toSaveLater = primary->convertToTypeB();
			secondaryInsert((uint32_t)(toSaveLater >> 32),(uint32_t)toSaveLater);
		}
		secondaryInsert(key,value);
		return 0;
	}
	if(!primary->insertIfAvailable(key,value)){
		return 0;
	}
	return 0;
}

void HHMap::secondaryInsert(uint32_t key, uint32_t value){
	int tag = getTag(key);
	int primaryBucketIdx = hash(key,0);
	HHBucket * bucket = buckets[primaryBucketIdx];
	uint8_t hashFunc = bucket->getRedirectListEntry(tag);
	if (!hashFunc){
		hashFunc = findBestHashFunc(key);
		bucket->setRedirectListEntry(tag,hashFunc);
	}
	if (hashFunc == 9){return;}
	int secondaryIdx = hash(key,hashFunc);
	HHBucket * secBucket = buckets[secondaryIdx];
	if (!secBucket->insertIfAvailable(key,value)){
		displaceItems(secondaryIdx);
		if (hashFunc != bucket->getRedirectListEntry(tag)){
			hashFunc = bucket->getRedirectListEntry(tag);
			secondaryIdx = hash(key,hashFunc);
			secBucket = buckets[secondaryIdx];
		}
		if (!secBucket->insertIfAvailable(key,value)){
			return;
		}
	}
}

uint32_t HHMap::put(uint32_t key, uint32_t value){
	return checkedPut(key,value);
}

uint32_t HHMap::get(uint32_t key){
	uint64_t pair = getKV(key);
	if (pair){return pair;}
	return 0;
}

int main(){
	int numBuckets = 50000;
	int bucketSize = 4;
	int fails = 0;
	HHMap * h = new HHMap(numBuckets,bucketSize);
	/*for (int i = 1; i <= (int)(numBuckets*bucketSize*0.9); i++){
		h->put(i,i);
	}*/
	int go = 1;
	int fail = 0;
	while (go){
		h->put(go,go);
		for (int i = 1; i < go; i++){
			if (h->get(i) != i){
				std::cout << "failure: " << i << " " << h->get(i) << std::endl;
				fails++;
			}
		}
		if (h->get(go) != go){fails++;}
		if (go >= (int)(numBuckets*bucketSize*0.9)){
			go = 0;
		}
		else if (fails){
			fail = go; 
			go = 0;
		}
		else{
			go++;
		}
		std::cout << (go/(numBuckets*bucketSize*0.9)) << std::endl;
		//std::cout.flush();
	}
	/*for (int i = 1; i <= fail; i++){
		if (h->get(i) != i){
			std::cout << "failure: " << i << std::endl;
			fails++;
		}
	}*/
	std::cout << "Failures: " << fails << std::endl;
	std::cout << "Apples " << fail << std::endl;
	std::cout << h->get(fail) << std::endl;
	//std::cout << (uint32_t)h->buckets[12]->getKVPair(66,0) << std::endl;
	/*for (int i = 0; i < numBuckets; i++){
		for (int j = 0; j < bucketSize; j++){
			std::cout << (uint32_t)h->buckets[i]->getKVPair(j,1) << std::endl;
			if ((uint32_t)h->buckets[i]->getKVPair(j,1) == 26){
				std::cout << i << " " << j << std:: endl;
			}
		}
	}*/
	//std::cout << h->buckets[8]->isTypeA() << std::endl;


	return 1;
}










