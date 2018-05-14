#include "predict.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include "virMachines.h"
#include "predictVM.h"
#include "setVM_ratio.h"
using namespace std;

//��Ҫ��ɵĹ��������
void predict_server(char * info[MAX_INFO_NUM], char * data[MAX_DATA_NUM], int data_num, char * filename)
{
	// ��Ҫ���������
	char * resultOut  = (char *)"";
	string resultString = "";
	predictVM preVM;

	/*
	preVM.trainUnderLine(info, data, data_num);

	hash_map<string, hash_map<int, int > >::iterator outIter = preVM.testVM.begin();
	cout<<preVM.testVM.size()<<endl;
	while(outIter!=preVM.testVM.end())
	{
		hash_map<int, int>::iterator innerIter;
		hash_map<int, int> temp = outIter->second;
		cout<<temp.size()<<endl;
		innerIter = temp.begin();
		while(innerIter!=temp.end())
		{
			cout<<outIter->first<<" "<<innerIter->first<<" "<<innerIter->second<<endl;
			innerIter++;
		}
		outIter++;
	}
	*/
	
	setVM_ratio server;
	
	hash_map<string, int> vmResults;
	vmResults = preVM.testOnline(info, data, data_num);

	int totalVMnumber = 0;
	hash_map<string, int>::iterator vmIter = vmResults.begin();
	while(vmIter!=vmResults.end()){
		totalVMnumber += vmIter->second;
		vmIter++;
	}
	resultString += to_string(totalVMnumber)+"\n";
	vmIter = vmResults.begin();
	while(vmIter!=vmResults.end()){
		resultString += vmIter->first +" "+to_string(vmIter->second)+"\n";
		server.numOfvm[vmIter->first] = vmIter->second;
		vmIter++;
	}
	//cout<<server.numOfvm.size()<<endl;
		
	resultString += "\n";
	server.CPU = preVM.cpu; 
	server.memGB = preVM.mem; 
	server.yingpan = preVM.disk;
	//cout<<server.CPU<<" "<<server.memGB<<" "<<server.yingpan<<endl;
	unordered_map<int, pair<int, int> > servers; //������ģ��
	unordered_map<int, unordered_map<string, int>> setServers; //����������ģ��

	server.initVM(server.numOfvm, server.ratioOfvm, server.typeOfvm);  //���� Ԥ������������ ���������
	server.setVMratio(servers, setServers);
	
	//cout<<setServers.size()<<endl;
	resultString += to_string(setServers.size())+"\n";
	unordered_map<int, unordered_map<string, int>>::iterator serverOutIter = setServers.begin();
	while(serverOutIter != setServers.end()){
		resultString += to_string(serverOutIter->first)+" ";
		unordered_map<string, int>::iterator serverInnerIter = serverOutIter->second.begin();
		while(serverInnerIter!=serverOutIter->second.end()){
			resultString += serverInnerIter->first+" "+to_string(serverInnerIter->second)+" ";
			serverInnerIter++;
		}
		resultString += "\n";
		serverOutIter++;
	}

	resultOut = (char *)resultString.c_str();
	//resultOut = (char *) oss.str().c_str();
	//resultOut = (char *)"safafds\nsafadf\nasdfa ";
	cout<<resultOut<<endl;

	//ofstream in;
	//in.open(filename,ios::trunc); //ios::app��ʾд�룬�ļ��������򴴽���������β��׷�� //ios::trunc��ʾ�ڴ��ļ�ǰ���ļ����,������д��,�ļ��������򴴽�
	//in<<oss.str();
	//in.close();
	// ֱ�ӵ�������ļ��ķ��������ָ���ļ���(ps��ע���ʽ����ȷ�ԣ�����н⣬��һ��ֻ��һ�����ݣ��ڶ���Ϊ�գ������п�ʼ���Ǿ�������ݣ�����֮����һ���ո�ָ���)
	write_result(resultOut, filename);
}
