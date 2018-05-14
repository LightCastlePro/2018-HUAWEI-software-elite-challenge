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

//你要完成的功能总入口
void predict_server(char * info[MAX_INFO_NUM], char * data[MAX_DATA_NUM], int data_num, char * filename)
{
	// 需要输出的内容
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
	unordered_map<int, pair<int, int> > servers; //服务器模型
	unordered_map<int, unordered_map<string, int>> setServers; //服务器放置模型

	server.initVM(server.numOfvm, server.ratioOfvm, server.typeOfvm);  //返回 预测的虚拟机数量 虚拟机类型
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
	//in.open(filename,ios::trunc); //ios::app表示写入，文件不存在则创建，有则在尾部追加 //ios::trunc表示在打开文件前将文件清空,由于是写入,文件不存在则创建
	//in<<oss.str();
	//in.close();
	// 直接调用输出文件的方法输出到指定文件中(ps请注意格式的正确性，如果有解，第一行只有一个数据；第二行为空；第三行开始才是具体的数据，数据之间用一个空格分隔开)
	write_result(resultOut, filename);
}
