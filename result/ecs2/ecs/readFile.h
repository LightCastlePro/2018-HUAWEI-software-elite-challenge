#pragma once
#include <string>
#include <vector>
using namespace std; 

class readFile
{
public:
	readFile(void);
	~readFile(void);

	//删除字符串首尾的空格，制表符tab等无效字符
	string Trim(string str);
	//type="\t"或者空格为分隔符，获取一行中的每一列数据
	vector<string> getSingleFields(string line, char type);
};

