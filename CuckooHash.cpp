#include "HortonHashBucket.h"
#include <iostream>
#include <stdint.h>

CuckooMap::CuckooMap(int size){
	pairs = new KV * [size];
	for (int i = 0; i < size; i++){pairs[i] = new KV;}
	numBuckets = size;
	int bits = 0;
	while (size){
		size /= 2;
		bits++;
	}
	bNB = bits;
}

uint32_t CuckooMap::hash(uint32_t key, bool right){
	if (right){
		return ((uint32_t)((3600022299*key) >> (32-bNB))) % numBuckets;
	}
	else{
		return ((uint32_t)((3396241081*key) >> (32-bNB))) % numBuckets;
	}
}

int CuckooMap::leftOrRight(uint32_t key){
	uint32_t left = hash(key,0);
	uint32_t right = hash(key,1);
	for (int i = 0; i < numBuckets; i++){
		if (pairs[left]->key == 0 && pairs[left]->value == 0){
			return 1;
		}
		if (pairs[right]->key == 0 && pairs[right]->value == 0){
			return 2;
		}
		left = hash(pairs[left]->key,(i+1)%2);
		right = hash(pairs[right]->key,i%2);
	}
	return 0;
}

bool CuckooMap::displaceItems(KV * pair, bool right){
	int index;
	if (right){
		index = hash(pair->key,1);
	}
	else {
		index = hash(pair->key,0);
	}
	if (pairs[index]->key != 0 && pairs[index]->value != 0){
		displaceItems(pairs[index],!right);
	}
	pairs[index]->key = pair->key;
	pairs[index]->value = pair->value;
	return true;
}

uint32_t CuckooMap::put(KV * pair){
	uint32_t first = hash(pair->key,0);
	uint32_t second = hash(pair->key,1);
	if (pairs[first]->key == 0 && pairs[first]->value == 0){
		pairs[first]->key = pair->key;
		pairs[first]->value = pair->value;
		return pair->value;
	}
	else if (pairs[second]->key == 0 && pairs[second]->value == 0){
		pairs[second]->key = pair->key;
		pairs[second]->value = pair->value;
		return pair->value;
	}
	else{
		int dir = leftOrRight(pair->key);
		if (dir == 1){
			displaceItems(pair,0);
		}
		if (dir == 2){
			displaceItems(pair,1);
		}
		return -1;
	}
}

uint32_t CuckooMap::put(uint32_t key, uint32_t value){
	KV * pair = new KV;
	pair->key = key;
	pair->value = value;
	return put(pair);
}

KV * CuckooMap::get(uint32_t key){
	uint32_t first = hash(key,0);
	uint32_t second = hash(key,1);
	if (pairs[first]->key == key){
		return pairs[first];
	}
	else if (pairs[second]->key == key){
		return pairs[second];
	}
	return NULL;
}

int main(){
	CuckooMap* m = new CuckooMap(1000000);
	int fails = 0;
	int minFail = 0;
	std::cout << "Apples" << std::endl;
	for (int i = 1; i <= 900000; i++){
		m->put(i,i);
	}
	std::cout << "Eaten" << std::endl;
	for(int i = 1; i <= 900000; i++){
		KV * k = m->get(i);
		if (k){
			if (k->value != i){
				if (!minFail){
					minFail = i;
				}
				fails++;
			}
		}
		else{
			if (!minFail){
				minFail = i;
			}
			fails++;
		}
	}
	std::cout << "MinFail: " << minFail << std::endl << "Fails: " << fails << std::endl;
	/*std::cout << "All Entries" << std::endl;
	for (int i = 0; i < 500; i++){
		std::cout << m->pairs[i]->key << " " << m->pairs[i]->value << std::endl;
	}*/

	return 1;
}










