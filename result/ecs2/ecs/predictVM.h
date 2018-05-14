#pragma once
#include <vector>
#include <time.h>
#include <string>
#include "hashmapString.h"
#include "lib_io.h"
#ifdef WIN32
#include <hash_map>
#else
#include <ext/hash_map>
using namespace __gnu_cxx; 
#endif
using namespace std;

#define duration	60			//训练集跨度为两个月
#define candidatesNumber 4		//两个月的训练集跨度里，以60天/candidatesNumber为一组，共candidates个参数
#define epoch	150			//训练模型的迭代次数
#define maxDayNumber	14		//预测跨度在1~2周，实际预测dayNumber天

class predictVM
{

public:
	int cpu, mem, disk;				//物理服务器CPU核数 内存大小（GB） 硬盘大小（GB）
	hash_map<string, hash_map<int, int> > machinesType;		//需要预测的虚拟机型号,<型号名称， <CPU个数， 内存大小MB> >
	int predictType;				//需要预测的资源，type=0表示CPU，type=1表示内存
	time_t startTime, endTime;		//需要预测的起始时间、终止时间
	int dayNumber;		//需要预测的每一天

	hash_map<string, hash_map<int, int> > trainVM;		//训练集数据，<需要预测的型号，< 距离startTime的天数， 个数> >
	hash_map<string, hash_map<int, double> > norm_trainVM;		//归一化训练集数据，<需要预测的型号，< 距离startTime的天数， 归一化后的个数> >
	hash_map<string, hash_map<int, int> >testVM;		//测试集数据，<型号，< 预测时间段的第几天，个数> >
	hash_map<string, hash_map<int, double> > norm_testVM;	//归一化后的测试集数据

	double error[maxDayNumber];		//机型dayNumber天输出与实际之间的误差总和
	double nodes[maxDayNumber][candidatesNumber];		//共candidates个参数
	double rates[maxDayNumber][candidatesNumber];		//每一个参数更新的学习率

	hash_map<string, vector<double> > machineNodesInfo;		//读取的各型号虚拟机的模型参数
	hash_map<string, double> input_mean;		//训练数据各机型个数对应的均值
	hash_map<string, double> input_standDeviation;	//训练数据中各机型个数对应的标准差
	hash_map<string, double> output_mean;		//测试数据各机型个数对应的均值
	hash_map<string, double> output_standDeviation;	//测试数据各机型个数对应的标准差

public:
	predictVM(void);
	~predictVM(void);

	void initPredictPurpose(char* info[MAX_INFO_NUM]);		//读取input文件，获取需要预测的信息（时间、型号等）
	void readTrainData(char* data[MAX_DATA_NUM], int data_num);				//根据需要预测的虚拟机机型，读取训练集
	void normalTrainData();									//归一化训练集数据的输入，归一化测试数据机型的输出个数
	void normalTestData();
	hash_map<string, int> ReverseTestResult();
	double normalReverseTestData(double data, double mean, double deviation, int type);	//归一化线上测试数据的输入，反归一化线上测试数据的输出个数
	void initTestData4Train(string path);					//读取测试集，用来训练参数
	void initCandidates();									//初始化各个参数的值
	void trainData();

	void trainUnderLine(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM], int data_num);	//线下训练模型，得到模型的各个参数值
	void writeWeights(string path);		//将训练好的参数集写入文件中
	void readWeights(string path);		//读取训练好的参数
	
	void initWeightsNOTfile();

	void averageWeights(string path);		//计算权重的均值
	hash_map<string, int> testOnline(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM],int data_num);	//线上测试模型
};
