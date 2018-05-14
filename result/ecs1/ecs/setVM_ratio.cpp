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
	int num = 0;    //当前占用的服务器的数量 也就是当前正在分配到服务器
					//开辟一个新服务器
	//num++;
	servers[num] = make_pair(CPU, 1024 * memGB); //服务器模型


	unordered_map<string, pair<int, int>>::iterator typevmIter = typeOfvm.begin();//从后往前放，先放大的 再放小的
	while (typevmIter != typeOfvm.end())
	{
		int count = numOfvm[typevmIter->first];//此种虚拟机的预测数量
											   //cout << typevmIter->first << " " << count << endl;
		pair<int, int> tempvm = typeOfvm[typevmIter->first]; //虚拟机的核数 和内存大小要求
		while (count > 0)
		{
			bool flag = false; //是否放置了的标志

							   //遍历所有服务器 找到一个放置的位置
			for (int tempnum = 1; tempnum <= num; tempnum++)
			{
				//cout << count << endl;
				if (servers[tempnum].first >= tempvm.first && servers[tempnum].second >= tempvm.second)
				{
					servers[tempnum].first -= tempvm.first;
					servers[tempnum].second -= tempvm.second;
					count--;

					//并且存进去 setServers 是服务器模型
					unordered_map<string, int> innervm;
					innervm = setServers[tempnum];  //unordered_map<string, int> 此时 innervm就是之前放虚拟机的集合

					if (innervm.find(typevmIter->first) != innervm.end())//此服务器放过这种虚拟机
					{
						innervm[typevmIter->first] += 1;
					}
					else
					{
						innervm[typevmIter->first] = 1;
					}
					setServers[tempnum] = innervm;
					flag = true;//放置了
				}
				if (flag == true)
				{
					break;
				}
			}

			if (flag == false)  //所有已经开了的服务器都找遍了 也放不进去的话，才开新的。所以要先便利。
			{
				//开辟一个新服务器
				num++;
				servers[num] = make_pair(56, 1024 * 128); //服务器模型
														  //新服务器肯定够放
				servers[num].first -= tempvm.first;
				servers[num].second -= tempvm.second;
				count--;

				//并且存进去 setServers 是服务器模型
				unordered_map<string, int> innervm;
				innervm = setServers[num];  //unordered_map<string, int> 此时 innervm就是之前放虚拟机的集合

				if (innervm.find(typevmIter->first) != innervm.end())//此服务器放过这种虚拟机
				{
					innervm[typevmIter->first] += 1;
				}
				else
				{
					innervm[typevmIter->first] = 1;
				}
				setServers[num] = innervm;
			}
		}
		typevmIter++;
	}
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
