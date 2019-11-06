#include "HortonHashBucket.h"
#include <iostream>
#include <stdint.h>
#include <queue>

CuckooBucket::CuckooBucket(int capacity){
	cap = capacity;
	vals = new KV[cap];
}

int CuckooBucket::getCountEmpty(){
	int numEmpty = 0;
	for (int i = 0; i < cap; i++){
		if (vals[i].key == 0 && vals[i].value == 0){
			numEmpty++;
		}
	}
	return numEmpty;
}

bool CuckooBucket::insertIfAvailable(uint32_t key, uint32_t value){
	int i = 0;
	while (vals[i].key != 0 && vals[i].value != 0 && i < cap){
		i++;
	}
	if (i < cap){
		vals[i].key = key;
		vals[i].value = value;
		return true;
	}
	return false;
}

void CuckooBucket::shuffleKeysBack(){
	for (int i = 0; i < cap; i++){
		if (vals[i].key == 0 && vals[i].value == 0){
			for (int j = i+1; j < cap; j++){
				if (vals[j].key != 0 && vals[j].value != 0){
					vals[i].key = vals[j].key;
					vals[i].value = vals[j].value;
					vals[j].key = 0;
					vals[j].value = 0;
					break;
				}
			}
		}
	}
}

uint32_t CuckooBucket::get(uint32_t key){
	for (int i = 0; i < cap; i++){
		if (vals[i].key == key){
			return vals[i].value;
		}
	}
	return -1;
}

KV * CuckooBucket::getKVPair(uint32_t key, bool index){
	if (index){
		return &vals[key];
	}
	else{
		for (int i = 0; i < cap; i++){
			if (vals[i].key == key){
				return &vals[i];
			}
		}
	}
	return NULL;
}

void CuckooBucket::clearPair(int i){
	if (i < cap){
		vals[i].key = 0;
		vals[i].value = 0;
	}
}

BCH::BCH(int size, int capacity){
	buckets = new CuckooBucket * [size];
	for (int i = 0; i < size;i++){buckets[i] = new CuckooBucket(capacity);}
	numBuckets = size;
	cap = capacity;
	int bits = 0;
	while (size){
		size /= 2;
		bits++;
	}
	bNB = bits;
}

uint32_t BCH::hash(uint32_t key, bool right){
	if (right){
		return ((uint32_t)((3600022299*key) >> (32-bNB))) % numBuckets;
	}
	else{
		//return hash((uint8_t)key,1) ^ hash(key,1); 
		return ((uint32_t)((3396241081*key) >> (32-bNB))) % numBuckets;
	}
}

int BCH::alternate(uint32_t key, int index){
	if (hash(key,0) == index){
		return hash(key,1);
	}
	return hash(key,0);
}

bool BCH::displaceItems(KV * pair, std::vector<int> path){
	int index = hash(pair->key,path.front()+2);
	//std::cout << "Index" << index << std::endl;
	KV * prevPair = pair;
	uint32_t currKey, currVal;
	for (auto i = path.begin()+1; i != path.end(); i++){
		KV * currPair = buckets[index]->getKVPair(*i,1);
		currKey = currPair->key;
		currVal = currPair->value;
		buckets[index]->clearPair(*i);
		//std::cout << "Pair: " << currKey << " " << currVal << std::endl;
		buckets[index]->insertIfAvailable(prevPair->key, prevPair->value);
		prevPair->key = currKey;
		prevPair->value = currVal;
		index = alternate(currKey,index);
	}
	//std::cout << index << " " << buckets[index]->getCountEmpty() << std::endl;
	if (buckets[index]->insertIfAvailable(currKey,currVal)){
		return true;
	}
	return false;
}

std::vector<int> BCH::leftOrRight(uint32_t key){
	std::queue<std::vector<int> > q;
	std::vector<int> left, right;
	left.push_back(-2);
	right.push_back(-1);
	q.push(left);
	q.push(right);
	while (!q.empty()){
		//std::cout << "Key: " << key << std::endl;
		std::vector<int> n = q.front();
		std::vector<int> loop;
		q.pop();
		int index = hash(key, n.front()+2);
		uint32_t currentKey = key;
		for (auto i = n.begin()+1; i != n.end(); i++){
			loop.push_back(index);
			//std::cout << "Swap Value " << buckets[index]->getKVPair(*i,1)->key << std::endl;
			index = alternate(buckets[index]->getKVPair(*i,1)->key, index);
		}
		if (loop.end() == std::find(loop.begin(),loop.end(),index)){
			if (!buckets[index]->getCountEmpty()){
				for (int i = 0; i < cap; i++){
					n.push_back(i);
					q.push(n);
					n.pop_back();
				}
			}
			else{
				//for (auto i = n.begin(); i != n.end(); i++){std::cout << "Path " << *i << std::endl;}
				//for (auto i = loop.begin(); i != loop.end(); i++){std::cout << "Indices " << *i << std::endl;}
				//std::cout << index << std::endl;
				return n;
			}
		}
	}
	return left;
}

KV * BCH::get(uint32_t key){
	uint32_t first = hash(key,0);
	uint32_t second = hash(key,1);
	if (buckets[first]->getKVPair(key,0)){
		return buckets[first]->getKVPair(key,0);
	}
	else if (buckets[second]->getKVPair(key,0)){
		return buckets[second]->getKVPair(key,0);
	}
	return NULL;
}

uint32_t BCH::put(uint32_t key, uint32_t value){
	uint32_t first = hash(key,0);
	uint32_t second = hash(key,1);
	if (buckets[first]->insertIfAvailable(key,value)){
		return value;
	}
	else if (buckets[second]->insertIfAvailable(key,value)){
		return value;
	}
	else{
		//std::cout << "Fullish" << std::endl;
		std::vector<int> path = leftOrRight(key);
		//for (auto i = path.begin(); i != path.end(); i++){std::cout << *i << std::endl;}
		KV * pair = new KV;
		pair->key = key;
		pair->value = value;
		displaceItems(pair,path);
		return -1;
	}
}

uint32_t BCH::put(KV * pair){
	return put(pair->key,pair->value);
}

int main(){
	BCH * m = new BCH(1025,4);
	for (int i = 1; i <= 3950; i++){
		m->put(i,i);
	}
	std::cout << "Apples" << std::endl;
	for (int i = 1; i <= 3950; i++){
		if (m->get(i)){
			if (m->get(i)->value != i){
				std::cout << "Wrong value: " << i << std::endl;
			}
		}
		else{
			std::cout << "Not Found: " << i << std::endl;
		}
	}
	return 1;
}











