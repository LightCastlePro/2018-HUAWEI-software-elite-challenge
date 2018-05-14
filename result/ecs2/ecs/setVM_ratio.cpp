#include <iostream>
#include<fstream>
#include<sstream>
#include <vector>
#include <unordered_map>
#include "readFile.h"
#include "setVM_ratio.h"
#include "virMachines.h"
using namespace std;



setVM_ratio::setVM_ratio(void) {}
setVM_ratio::~setVM_ratio(void) {}

void setVM_ratio::initVM(unordered_map<string, int>& numOfvm, unordered_map<string, int>& ratioOfvm, unordered_map<string, pair<int, int> >& typeOfvm)
{
	cout<<"init VM......"<<endl;
	/*
	numOfvm["flavor1"] = 12;
	numOfvm["flavor2"] = 62;
	numOfvm["flavor3"] = 42;
	numOfvm["flavor4"] = 81;
	numOfvm["flavor5"] = 44;
	numOfvm["flavor6"] = 19;
	numOfvm["flavor7"] = 52;
	numOfvm["flavor8"] = 12;
	numOfvm["flavor9"] = 43;
	numOfvm["flavor10"] = 72;
	numOfvm["flavor11"] = 22;
	numOfvm["flavor12"] = 12;
	numOfvm["flavor13"] = 13;
	numOfvm["flavor14"] = 22;
	numOfvm["flavor15"] = 23;
*/


	typeOfvm["flavor1"] = make_pair(1, 1024);
	typeOfvm["flavor2"] = make_pair(1, 2048);
	typeOfvm["flavor3"] = make_pair(1, 4096);
	typeOfvm["flavor4"] = make_pair(2, 2048);
	typeOfvm["flavor5"] = make_pair(2, 4096);
	typeOfvm["flavor6"] = make_pair(2, 8192);
	typeOfvm["flavor7"] = make_pair(4, 4096);
	typeOfvm["flavor8"] = make_pair(4, 8192);
	typeOfvm["flavor9"] = make_pair(4, 16384);
	typeOfvm["flavor10"] = make_pair(8, 8192);
	typeOfvm["flavor11"] = make_pair(8, 16384);
	typeOfvm["flavor12"] = make_pair(8, 32768);
	typeOfvm["flavor13"] = make_pair(16, 16384);
	typeOfvm["flavor14"] = make_pair(16, 32768);
	typeOfvm["flavor15"] = make_pair(16, 65536);

	//:mem/cpu的比例
	ratioOfvm["flavor1"] = 1;
	ratioOfvm["flavor2"] = 2;
	ratioOfvm["flavor3"] = 4;
	ratioOfvm["flavor4"] = 1;
	ratioOfvm["flavor5"] = 2;
	ratioOfvm["flavor6"] = 3;
	ratioOfvm["flavor7"] = 1;
	ratioOfvm["flavor8"] = 2;
	ratioOfvm["flavor9"] = 4;
	ratioOfvm["flavor10"] = 1;
	ratioOfvm["flavor11"] = 2;
	ratioOfvm["flavor12"] = 4;
	ratioOfvm["flavor13"] = 1;
	ratioOfvm["flavor14"] = 2;
	ratioOfvm["flavor15"] = 4;
	cout<<"Init end"<<endl;
}

void setVM_ratio::display(unordered_map<int, unordered_map<string, int>>& setServers)
{
	unordered_map<int, unordered_map<string, int>>::iterator outIter = setServers.begin();
	int totalcpu = 0, totalmem = 0;
	int tempcpu = 0, tempmem = 0;

	cout << setServers.size() << endl;
	while (outIter != setServers.end())
	{
		unordered_map<string, int> temp = outIter->second;
		unordered_map<string, int>::iterator innerIter = temp.begin();

		cout << outIter->first;
		while (innerIter != temp.end())
		{
			cout << " " << innerIter->first << " " << innerIter->second;//可以输出 flavor15   2 数量
			totalcpu += typeOfvm[innerIter->first].first * innerIter->second;
			totalmem += typeOfvm[innerIter->first].second * innerIter->second;
			innerIter++;
		}
		cout << "    资源占用情况  " << (double)(totalcpu - tempcpu) / CPU * 100 << "%  " << (double)(totalmem - tempmem) / memGB / 1024 * 100 << "%" << endl;
		tempcpu = totalcpu;
		tempmem = totalmem;
		outIter++;
	}
	cout << "totalcpu:" << totalcpu << " " << "totalmem" << totalmem << endl;
	int num = setServers.size();
	cout << "CPU占用百分率" << (double)totalcpu / (num*CPU) * 100 << "%";
	cout << "内存占用百分率" << (double)totalmem / (num*memGB * 1024) * 100 << "%";
	cout << endl;
}


//选择比例最互补的虚拟机  优先满足服务器的需求
void setVM_ratio::setVMratio(unordered_map<int, pair<int, int> >& servers, unordered_map<int, unordered_map<string, int>>& setServers)
{
	int totalVM = 0; //总的虚拟机数量
	int num = 0;     //当前占用的服务器的数量 也就是当前正在分配到服务器 开辟一个新服务器
	double currentRatio = 1.0;            //当前占用资源的比例
	double stdRatio = 1.0 *  memGB / CPU;  //服务器的资源比例标准

	unordered_map<string, int> ::iterator numvmIter = numOfvm.begin();
	while (numvmIter != numOfvm.end())
	{
		totalVM += numvmIter->second;
		numvmIter++;
	}
	//cout << "test2" << endl;

	bool flag = false;//此服务器没有放置满
	bool firstVM = true;
	while (totalVM != 0)
	{
		num++;//跳出内层循环了 说明是当前服务器放置满了 要开辟新的服务器
			  //cout << "num "<<num << endl;
		servers[num] = make_pair(CPU, 1024 * memGB); //服务器模型
		flag = false;// 初始是没放置满的
		firstVM = true;
		while (flag == false)
		{
			//cout << "test3" << endl;
			if (firstVM == true)
				currentRatio = 0.0;// memGB / CPU;
			else
				currentRatio = (1.0*(memGB - (servers[num].second / 1024))) / (CPU - servers[num].first);  //（Mem / CPU） 已经占用的比例
																										   //currentRatio =  (CPU - servers[num].first)/ (1.0*(memGB - (servers[num].second / 1024)));  //（ CPU/Mem ） 已经占用的比例

																										   //选择一个比例最互补的虚拟机
			unordered_map<string, pair<int, int>>::iterator typevmIter = typeOfvm.begin();
			string bestVMtype = "";
			//cout << "test4" << endl;
			double chazhi = 10.0;
			//cout << "test5" << endl;
			bestVMtype = typevmIter->first;
			while (typevmIter != typeOfvm.end())
			{
				//cout << "test6" << endl;
				if (numOfvm[typevmIter->first] > 0 && servers[num].first >= typevmIter->second.first
					&& servers[num].second >= typevmIter->second.second)   //可分配
				{
					double temp = abs(1.0*(currentRatio + ratioOfvm[typevmIter->first]) / 2 - stdRatio);
					if (firstVM == true)
					{
						temp = abs(1.0*(currentRatio + ratioOfvm[typevmIter->first]) - stdRatio);
					}

					if (temp < chazhi)
					{
						bestVMtype = typevmIter->first;
						chazhi = temp;
					}//先选择一个 

					 //  currentRatio > stdRatio 选择小的
					if (currentRatio > stdRatio && ratioOfvm[typevmIter->first] <= stdRatio)
					{
						temp = abs((1.0*(currentRatio + ratioOfvm[typevmIter->first])) / 2 - stdRatio);
						if (firstVM == true)
						{
							temp = abs(1.0*(currentRatio + ratioOfvm[typevmIter->first]) - stdRatio);
						}
						if (temp < chazhi)
						{
							bestVMtype = typevmIter->first;
							chazhi = temp;
						}
					}
					//currentRatio <= stdRatio 选择大的
					if (currentRatio <= stdRatio && ratioOfvm[typevmIter->first] >= stdRatio)
					{
						temp = abs((1.0*(currentRatio + ratioOfvm[typevmIter->first])) / 2 - stdRatio);
						if (firstVM == true)
						{
							temp = abs(1.0*(currentRatio + ratioOfvm[typevmIter->first]) - stdRatio);
						}
						if (temp < chazhi)
						{
							bestVMtype = typevmIter->first;
							chazhi = temp;
						}
					}
				}
				typevmIter++;
			} //找到了最合适的一个VM

			  //放进服务器
			servers[num].first -= typeOfvm[bestVMtype].first;
			servers[num].second -= typeOfvm[bestVMtype].second;
			numOfvm[bestVMtype] --;

			//并且存进去 setServers 是服务器模型
			unordered_map<string, int> innervm;
			innervm = setServers[num];  //unordered_map<string, int> 此时 innervm就是之前放虚拟机的集合

			if (innervm.find(bestVMtype) != innervm.end())//此服务器放过这种虚拟机 找到了
			{
				innervm[bestVMtype] += 1;
			}
			else
			{
				innervm[bestVMtype] = 1;
			}
			setServers[num] = innervm;
			totalVM--;
			firstVM = false;
			//放置完毕

			//检查是否还可以再放置
			flag = true;
			unordered_map<string, pair<int, int>>::iterator tyIter = typeOfvm.begin();
			while (tyIter != typeOfvm.end())
			{
				if (numOfvm[tyIter->first] > 0 && servers[num].first >= tyIter->second.first
					&& servers[num].second >= tyIter->second.second)   //可分配
				{
					flag = false;
				}
				tyIter++;
			}//end while
		}//end while这个虚拟机放满了
	}//end while 所有虚拟机都放置了
}



/*
int main()
{
	setVM_ratio vm;
	vm.initVM(numOfvm, ratioOfvm, typeOfvm);  //返回 预测的虚拟机数量 虚拟机类型

	unordered_map<int, pair<int, int> > servers; //服务器模型
	unordered_map<int, unordered_map<string, int>> setServers; //服务器放置模型

	vm.setVMratio(servers, setServers);

	vm.display(setServers);

	system("pause");
	return 0;
}
*/
