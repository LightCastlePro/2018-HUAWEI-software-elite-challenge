#pragma once
#include <string>
#include <vector>
using namespace std; 

class readFile
{
public:
	readFile(void);
	~readFile(void);

	//ɾ���ַ�����β�Ŀո��Ʊ��tab����Ч�ַ�
	string Trim(string str);
	//type="\t"���߿ո�Ϊ�ָ�������ȡһ���е�ÿһ������
	vector<string> getSingleFields(string line, char type);
};

