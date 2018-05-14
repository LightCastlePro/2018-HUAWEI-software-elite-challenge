#include <iostream>
#include<fstream>
#include<sstream>
#include "hashmapString.h"
#ifdef WIN32
#include <hash_map>
#else
#include <ext/hash_map>
using namespace __gnu_cxx;
#endif
#include <string>
#include "virMachines.h"
#include "readFile.h"


using namespace std; 

#define machineNumber	15		//虚拟机规格总数

virMachines::virMachines(void){}

virMachines::~virMachines(void){}

void virMachines::initTypes(string filePath){
	readFile r;
	ifstream fin(filePath);
	if(fin == NULL)	{
		cout<<"File Open FAIL!"<<endl;
		exit(1);
	}
	string line = "";
	vector<string> fields;
	while(getline(fin, line))	{
		fields = r.getSingleFields(line,'\t');
		if(fields.size()<3)	cout<<"Invalid machine type line"<<endl;
		else{
			hash_map<int, int> inner;
			inner[atoi(r.Trim(fields[1]).c_str())] = atoi(r.Trim(fields[2]).c_str());
			machinesType[r.Trim(fields[0])] = inner;
			cout<<fields[0]<<" "<<fields[1]<<" "<<fields[2]<<endl;
		}
	}
}

/*
void main()
{
	virMachines vm;
	string path = "machineType.txt";
	vm.initTypes(path);

	hash_map<string, hash_map<int, int> >::iterator outIter;
	outIter = vm.machinesType.begin();
	while(outIter!=vm.machinesType.end())
	{
		hash_map<int, int>::iterator innerIter;
		hash_map<int, int> temp = outIter->second;
		innerIter = temp.begin();
		while(innerIter!=temp.end())
		{
			cout<<outIter->first<<" "<<innerIter->first<<" "<<innerIter->second<<endl;
			innerIter++;
		}
		outIter++;
	}
	
}
*/
