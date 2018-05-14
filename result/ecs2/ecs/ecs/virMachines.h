#pragma once
#include "hashmapString.h"
#ifdef WIN32
#include <hash_map>
#else
#include <ext/hash_map>
using namespace __gnu_cxx;
#endif
#include <string>
using namespace std; 


class virMachines
{

public:
	hash_map<string, hash_map<int, int> > machinesType;

public:
	void initTypes(string filePath);

	virMachines(void);
	~virMachines(void);
};

