#pragma once
using namespace std;
#include "unordered_map"

class setVM_ratio
{

public:
	int CPU = 56, memGB = 128, yingpan = 1200;
	unordered_map<string, int> numOfvm;
	unordered_map<string, int> ratioOfvm;
	unordered_map<string, pair<int, int> > typeOfvm;

public:
	setVM_ratio();
	~setVM_ratio();

	void initVM(unordered_map<string, int>& numOfvm, unordered_map<string, int>& ratioOfvm, unordered_map<string, pair<int, int> >& typeOfvm);
	void display(unordered_map<int, unordered_map<string, int>>& setServers);
	void setVMratio(unordered_map<int, pair<int, int> >& servers, unordered_map<int, unordered_map<string, int>>& setServers);
};
