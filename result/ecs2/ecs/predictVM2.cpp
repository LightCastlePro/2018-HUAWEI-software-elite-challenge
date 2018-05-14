#include "predictVM.h"
#include "predictVM2.h"
#include "readFile.h"
#include "selfTime.h"
#include "hashmapString.h"
#include "lib_io.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#ifdef WIN32
#include <hash_map>
#else
#include <ext/hash_map>
using namespace __gnu_cxx;
#endif
using namespace std;

struct CmpByKeyLength {  
  bool operator()(const string& k1, const string& k2) {  
    return k1.length() < k2.length();  
  }  
};  

struct CmpByKeyNumber {  
  bool operator()(const int& k1, const int& k2) {  
    return k1 < k2;  
  }  
};  

predictVM2::predictVM2(void)
{
	cpu = 0;
	mem = 0;
	disk = 0;
}

predictVM2::~predictVM2(void)
{}

hash_map<string, int> predictVM2::predictProcess(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM],int data_num){
	predictVM preVM;
	preVM.initPredictPurpose(info);			//初始化需要预测的信息
	preVM.readTrainData(data, data_num);		//读取训练数据
/*
	map<string, int, CmpByKeyLength> name_score_map;  
  	name_score_map["LiMin"] = 60;   
  	name_score_map["ZiLinMi"] = 79;   
  	name_score_map["BoBadsfafdaf"] = 62;   
  	name_score_map.insert(make_pair("Bing",99));  
  	name_score_map.insert(make_pair("Albert",86));  
  	for (map<string, int>::iterator iter = name_score_map.begin(); iter != name_score_map.end(); ++iter) {  
   	 cout << iter->first<<" "<<iter->second << endl;  
	}  

	map<int, int, CmpByKeyNumber> name_score_map2;  
  	name_score_map2[2] = 60;   
  	name_score_map2[10] = 79;   
  	name_score_map2[5] = 62;   
  	name_score_map2.insert(make_pair(0,99));  
  	name_score_map2.insert(make_pair(7,86));  
  	for (map<int, int>::iterator iter = name_score_map2.begin(); iter != name_score_map2.end(); ++iter) {  
   	 cout << iter->first<<" "<<iter->second << endl;  
	}  

*/
	cpu = preVM.cpu;
	mem = preVM.mem;
	disk = preVM.disk;	
	hash_map<string, int> resultAll;
	for(hash_map<string, hash_map<int, int> >::iterator outIter = preVM.machinesType.begin(); outIter!=preVM.machinesType.end(); outIter++){
		if(preVM.trainVM.find(outIter->first)!=preVM.trainVM.end()){
			hash_map<int, int> innerInfo = preVM.trainVM[outIter->first];
			map<int, int, CmpByKeyNumber> info;
			for(hash_map<int, int>::iterator innerIter = innerInfo.begin(); innerIter!=innerInfo.end(); innerIter++){
				info[innerIter->first] = innerIter->second;
				//cout<<outIter->first<<" "<<innerIter->first<<" "<<innerIter->second<<endl;
			}
			if(preVM.dayNumber>14)	preVM.dayNumber =  14;
			int singleModel = 0;
			map<int, int>::iterator iter = info.begin();
			for(int i =0; i<preVM.dayNumber && iter!=info.end(); ++i){
				singleModel += iter->second;
				//cout<<outIter->first<<" "<<iter->first<<" "<<iter->second<<" "<<singleModel<<endl;
				++iter;
			}
			resultAll[outIter->first] = singleModel;
		}
		else{	//number is ZERO
			resultAll[outIter->first] = 0;
		}
	}

	return resultAll;

}
