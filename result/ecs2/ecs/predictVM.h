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

#define duration	60			//ѵ�������Ϊ������
#define candidatesNumber 4		//�����µ�ѵ����������60��/candidatesNumberΪһ�飬��candidates������
#define epoch	150			//ѵ��ģ�͵ĵ�������
#define maxDayNumber	14		//Ԥ������1~2�ܣ�ʵ��Ԥ��dayNumber��

class predictVM
{

public:
	int cpu, mem, disk;				//���������CPU���� �ڴ��С��GB�� Ӳ�̴�С��GB��
	hash_map<string, hash_map<int, int> > machinesType;		//��ҪԤ���������ͺ�,<�ͺ����ƣ� <CPU������ �ڴ��СMB> >
	int predictType;				//��ҪԤ�����Դ��type=0��ʾCPU��type=1��ʾ�ڴ�
	time_t startTime, endTime;		//��ҪԤ�����ʼʱ�䡢��ֹʱ��
	int dayNumber;		//��ҪԤ���ÿһ��

	hash_map<string, hash_map<int, int> > trainVM;		//ѵ�������ݣ�<��ҪԤ����ͺţ�< ����startTime�������� ����> >
	hash_map<string, hash_map<int, double> > norm_trainVM;		//��һ��ѵ�������ݣ�<��ҪԤ����ͺţ�< ����startTime�������� ��һ����ĸ���> >
	hash_map<string, hash_map<int, int> >testVM;		//���Լ����ݣ�<�ͺţ�< Ԥ��ʱ��εĵڼ��죬����> >
	hash_map<string, hash_map<int, double> > norm_testVM;	//��һ����Ĳ��Լ�����

	double error[maxDayNumber];		//����dayNumber�������ʵ��֮�������ܺ�
	double nodes[maxDayNumber][candidatesNumber];		//��candidates������
	double rates[maxDayNumber][candidatesNumber];		//ÿһ���������µ�ѧϰ��

	hash_map<string, vector<double> > machineNodesInfo;		//��ȡ�ĸ��ͺ��������ģ�Ͳ���
	hash_map<string, double> input_mean;		//ѵ�����ݸ����͸�����Ӧ�ľ�ֵ
	hash_map<string, double> input_standDeviation;	//ѵ�������и����͸�����Ӧ�ı�׼��
	hash_map<string, double> output_mean;		//�������ݸ����͸�����Ӧ�ľ�ֵ
	hash_map<string, double> output_standDeviation;	//�������ݸ����͸�����Ӧ�ı�׼��

public:
	predictVM(void);
	~predictVM(void);

	void initPredictPurpose(char* info[MAX_INFO_NUM]);		//��ȡinput�ļ�����ȡ��ҪԤ�����Ϣ��ʱ�䡢�ͺŵȣ�
	void readTrainData(char* data[MAX_DATA_NUM], int data_num);				//������ҪԤ�����������ͣ���ȡѵ����
	void normalTrainData();									//��һ��ѵ�������ݵ����룬��һ���������ݻ��͵��������
	void normalTestData();
	hash_map<string, int> ReverseTestResult();
	double normalReverseTestData(double data, double mean, double deviation, int type);	//��һ�����ϲ������ݵ����룬����һ�����ϲ������ݵ��������
	void initTestData4Train(string path);					//��ȡ���Լ�������ѵ������
	void initCandidates();									//��ʼ������������ֵ
	void trainData();

	void trainUnderLine(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM], int data_num);	//����ѵ��ģ�ͣ��õ�ģ�͵ĸ�������ֵ
	void writeWeights(string path);		//��ѵ���õĲ�����д���ļ���
	void readWeights(string path);		//��ȡѵ���õĲ���
	
	void initWeightsNOTfile();

	void averageWeights(string path);		//����Ȩ�صľ�ֵ
	hash_map<string, int> testOnline(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM],int data_num);	//���ϲ���ģ��
};
