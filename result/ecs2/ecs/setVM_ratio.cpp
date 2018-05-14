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

	//:mem/cpu�ı���
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
			cout << " " << innerIter->first << " " << innerIter->second;//������� flavor15   2 ����
			totalcpu += typeOfvm[innerIter->first].first * innerIter->second;
			totalmem += typeOfvm[innerIter->first].second * innerIter->second;
			innerIter++;
		}
		cout << "    ��Դռ�����  " << (double)(totalcpu - tempcpu) / CPU * 100 << "%  " << (double)(totalmem - tempmem) / memGB / 1024 * 100 << "%" << endl;
		tempcpu = totalcpu;
		tempmem = totalmem;
		outIter++;
	}
	cout << "totalcpu:" << totalcpu << " " << "totalmem" << totalmem << endl;
	int num = setServers.size();
	cout << "CPUռ�ðٷ���" << (double)totalcpu / (num*CPU) * 100 << "%";
	cout << "�ڴ�ռ�ðٷ���" << (double)totalmem / (num*memGB * 1024) * 100 << "%";
	cout << endl;
}


//ѡ���������������  �������������������
void setVM_ratio::setVMratio(unordered_map<int, pair<int, int> >& servers, unordered_map<int, unordered_map<string, int>>& setServers)
{
	int totalVM = 0; //�ܵ����������
	int num = 0;     //��ǰռ�õķ����������� Ҳ���ǵ�ǰ���ڷ��䵽������ ����һ���·�����
	double currentRatio = 1.0;            //��ǰռ����Դ�ı���
	double stdRatio = 1.0 *  memGB / CPU;  //����������Դ������׼

	unordered_map<string, int> ::iterator numvmIter = numOfvm.begin();
	while (numvmIter != numOfvm.end())
	{
		totalVM += numvmIter->second;
		numvmIter++;
	}
	//cout << "test2" << endl;

	bool flag = false;//�˷�����û�з�����
	bool firstVM = true;
	while (totalVM != 0)
	{
		num++;//�����ڲ�ѭ���� ˵���ǵ�ǰ�������������� Ҫ�����µķ�����
			  //cout << "num "<<num << endl;
		servers[num] = make_pair(CPU, 1024 * memGB); //������ģ��
		flag = false;// ��ʼ��û��������
		firstVM = true;
		while (flag == false)
		{
			//cout << "test3" << endl;
			if (firstVM == true)
				currentRatio = 0.0;// memGB / CPU;
			else
				currentRatio = (1.0*(memGB - (servers[num].second / 1024))) / (CPU - servers[num].first);  //��Mem / CPU�� �Ѿ�ռ�õı���
																										   //currentRatio =  (CPU - servers[num].first)/ (1.0*(memGB - (servers[num].second / 1024)));  //�� CPU/Mem �� �Ѿ�ռ�õı���

																										   //ѡ��һ����������������
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
					&& servers[num].second >= typevmIter->second.second)   //�ɷ���
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
					}//��ѡ��һ�� 

					 //  currentRatio > stdRatio ѡ��С��
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
					//currentRatio <= stdRatio ѡ����
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
			} //�ҵ�������ʵ�һ��VM

			  //�Ž�������
			servers[num].first -= typeOfvm[bestVMtype].first;
			servers[num].second -= typeOfvm[bestVMtype].second;
			numOfvm[bestVMtype] --;

			//���Ҵ��ȥ setServers �Ƿ�����ģ��
			unordered_map<string, int> innervm;
			innervm = setServers[num];  //unordered_map<string, int> ��ʱ innervm����֮ǰ��������ļ���

			if (innervm.find(bestVMtype) != innervm.end())//�˷������Ź���������� �ҵ���
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
			//�������

			//����Ƿ񻹿����ٷ���
			flag = true;
			unordered_map<string, pair<int, int>>::iterator tyIter = typeOfvm.begin();
			while (tyIter != typeOfvm.end())
			{
				if (numOfvm[tyIter->first] > 0 && servers[num].first >= tyIter->second.first
					&& servers[num].second >= tyIter->second.second)   //�ɷ���
				{
					flag = false;
				}
				tyIter++;
			}//end while
		}//end while��������������
	}//end while �����������������
}



/*
int main()
{
	setVM_ratio vm;
	vm.initVM(numOfvm, ratioOfvm, typeOfvm);  //���� Ԥ������������ ���������

	unordered_map<int, pair<int, int> > servers; //������ģ��
	unordered_map<int, unordered_map<string, int>> setServers; //����������ģ��

	vm.setVMratio(servers, setServers);

	vm.display(setServers);

	system("pause");
	return 0;
}
*/
