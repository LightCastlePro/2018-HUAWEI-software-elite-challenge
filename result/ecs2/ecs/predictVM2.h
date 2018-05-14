#include "predict.h"
#include <stdio.h>
#include <string.h>
#include <cstring>
#include "predictVM.h"
using namespace std;

class predictVM2
{

public:
	int cpu;
	int mem; 
	int disk;

public:
	predictVM2(void);
	~predictVM2(void);

	hash_map<string, int> predictProcess(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM],int data_num);

};
