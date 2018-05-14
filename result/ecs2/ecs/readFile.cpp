#include "readFile.h"
#include<iostream>
#include <string>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

readFile::readFile(void){}

readFile::~readFile(void){}

//ɾ���ַ�����β�Ŀո��Ʊ��tab����Ч�ַ�
string readFile::Trim(string str)
{
	//str.find_first_not_of(" \t\r\n"),���ַ���str�д�����0��ʼ�������״β�ƥ��"\t\r\n"��λ��  
    str.erase(0,str.find_first_not_of(" \t\r\n"));  
    str.erase(str.find_last_not_of(" \t\r\n") + 1);  
    return str;  
}

//��һ�����ݣ���,Ϊ�ָ�������ȡÿһ�е�����ֵ
vector<string> readFile::getSingleFields(string line, char type)
{
	istringstream sin(line); //�������ַ���line���뵽�ַ�����istringstream��  
    vector<string> fields; //����һ���ַ�������  
    string field;  
    while(getline(sin, field, type)) //���ַ�����sin�е��ַ����뵽field�ַ����У��ԡ�\t��Ϊ�ָ���  
    {  
		fields.push_back(field); //���ոն�ȡ���ַ�����ӵ�����fields��  
    }  
	return fields;
}
