#include "readFile.h"
#include<iostream>
#include <string>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

readFile::readFile(void){}

readFile::~readFile(void){}

//删除字符串首尾的空格，制表符tab等无效字符
string readFile::Trim(string str)
{
	//str.find_first_not_of(" \t\r\n"),在字符串str中从索引0开始，返回首次不匹配"\t\r\n"的位置  
    str.erase(0,str.find_first_not_of(" \t\r\n"));  
    str.erase(str.find_last_not_of(" \t\r\n") + 1);  
    return str;  
}

//对一行数据，以,为分隔符，获取每一列的数据值
vector<string> readFile::getSingleFields(string line, char type)
{
	istringstream sin(line); //将整行字符串line读入到字符串流istringstream中  
    vector<string> fields; //声明一个字符串向量  
    string field;  
    while(getline(sin, field, type)) //将字符串流sin中的字符读入到field字符串中，以“\t”为分隔符  
    {  
		fields.push_back(field); //将刚刚读取的字符串添加到向量fields中  
    }  
	return fields;
}
