#include "HortonHashBucket.h"
#include <iostream>
#include <stdint.h>
#include <queue>

CFB::CFB(int cap){
	capacity = cap;
	fPs = new uint8_t[cap];
}

int CFB::getCountEmpty(){
	int numEmpty = 0;
	for (int i = 0; i < capacity; i++){
		if (fPs[i] == 0){
			numEmpty++;
		}
	}
	return numEmpty;
}

bool CFB::insertIfAvailable(uint8_t fP){
	for (int i = 0; i < capacity; i++){
		if (fPs[i] == 0){
			fPs[i] = fP;
			return true;
		}
	}
	return false;
}

bool CFB::has(uint8_t fP){
	for (int i = 0; i < capacity; i++){
		if (fPs[i] == fP){
			return true;
		}
	}
	return false;
}

uint8_t CFB::get(int index){
	return fPs[index];
}


void CFB::clearFP(uint8_t fP){
	for (int i = 0; i < capacity; i++){
		if (fPs[i] == fP){
			fPs[i] = 0;
			break;
		}
	}
}

uint8_t CuckooFilter::fingerPrint(uint32_t key){
	uint8_t fP = (uint8_t)key;
	return (fP) ? fP : 1;
}

CuckooFilter::CuckooFilter(int size, int capacity){
	cap = capacity;
	numBuckets = size;
	buckets = new CFB * [size];
	for (int i = 0; i < size; i++){buckets[i] = new CFB(cap);}
	int bits = 0;
	while (size){
		size /= 2;
		bits++;
	}
	bNB = bits;
}

uint32_t CuckooFilter::hash(uint32_t key){
	uint64_t h = 0xcbf29ce484222325;
    unsigned char *c = (unsigned char *)&key;
    for(int i =0; i < 4; i++){
    	h = h * 0x100000001b3;
    	h = h ^ c[i];
    }
    return h % numBuckets;
	//return ((uint32_t)((3600022299*key) >> (32-bNB))) % numBuckets;
}

uint32_t CuckooFilter::alternate(uint8_t fP, uint32_t index){
	return (index ^ hash(fP));
}

bool CuckooFilter::get(uint32_t key){
	uint8_t f = fingerPrint(key);
	uint32_t loc1 = hash(key);
	uint32_t loc2 = alternate(f,loc1);
	if (buckets[loc1]->has(f)){
		return true;
	}
	if (buckets[loc2]->has(f)){
		return true;
	}
	return false;
}

std::vector<int> CuckooFilter::leftOrRight(uint32_t key){
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
		int index = hash(key);
		if (n.front() == -1){
			index = alternate(fingerPrint(key),index);
		}
		uint32_t currentKey = key;
		for (auto i = n.begin()+1; i != n.end(); i++){
			loop.push_back(index);
			//std::cout << "Swap Value " << buckets[index]->getKVPair(*i,1)->key << std::endl;
			index = alternate(buckets[index]->get(*i), index);
		}
		if (loop.end() == std::find(loop.begin(),loop.end(),index) && loop.size() < 300){
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

bool CuckooFilter::displaceItems(uint32_t key, std::vector<int> path){
	int index = hash(key);
	if (path.front() == -1){
		index = alternate(fingerPrint(key),index);
	}
	//std::cout << "Index" << index << std::endl;
	uint8_t prevFP = fingerPrint(key);
	uint8_t currFP;
	for (auto i = path.begin()+1; i != path.end(); i++){
		currFP = buckets[index]->get(*i);
		buckets[index]->clearFP(currFP);
		//std::cout << "Pair: " << currKey << " " << currVal << std::endl;
		buckets[index]->insertIfAvailable(prevFP);
		prevFP = currFP;
		index = alternate(currFP,index);
	}
	//std::cout << index << " " << buckets[index]->getCountEmpty() << std::endl;
	if (buckets[index]->insertIfAvailable(currFP)){
		return true;
	}
	return false;
}


uint32_t CuckooFilter::insert(uint32_t key){
	uint8_t f = fingerPrint(key);
	uint32_t loc1 = hash(key);
	uint32_t loc2 = alternate(f,loc1);
	if (buckets[loc1]->insertIfAvailable(f)){
		return loc1+1;
	}
	if (buckets[loc2]->insertIfAvailable(f)){
		return loc2+1;
	}
	//std::cout << "Fullish" << std::endl;
	std::vector<int> path = leftOrRight(key);
	//if (path.size() == 1){std::cout << "FULL" << std::endl;}
	return displaceItems(key,path);
}

bool CuckooFilter::remove(uint32_t key){
	uint8_t f = fingerPrint(key);
	uint32_t loc1 = hash(key);
	uint32_t loc2 = alternate(f,loc1);
	if (buckets[loc1]->has(f)){
		buckets[loc1]->clearFP(f);
		return true;
	}
	if (buckets[loc2]->has(f)){
		buckets[loc2]->clearFP(f);
		return true;
	}
	return false;
}

int main(){
	CuckooFilter * f = new CuckooFilter(1024,4);

	for (int i = 1; i < 3883; i++){
		if (!f->insert(i)){
			std::cout << i << std::endl;
		}
	}
	std::cout << "Apples" << std::endl;
	for (int i = 1; i < 3883; i++){
		if (!f->get(i)){
			std::cout << "Not Found: " << i << std::endl;
		}
	}
	int fails = 0;
	for (int i = 3890; i < 6000; i++){
		if (f->get(i)){
			fails++;
		}
	}
	std::cout << "Fails: " << fails << std::endl;
	return 1;
}













