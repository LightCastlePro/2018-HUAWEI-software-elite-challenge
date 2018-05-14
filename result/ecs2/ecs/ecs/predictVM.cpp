#include "predictVM.h"
#include "readFile.h"
#include "selfTime.h"
#include "hashmapString.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#ifdef WIN32
#include <hash_map>
#else
#include <ext/hash_map>
using namespace __gnu_cxx;
#endif
using namespace std;

predictVM::predictVM(void)
{
	cpu = mem = disk = 0;
	predictType = -1;
	startTime = endTime = 0;
	initCandidates();
	//��ʼ��ѧϰ�ʣ���Ԥ�����ڽ��ģ�Ȩֵ�ͣ���������VM�����������ˣ�����һ��ʱ����ٴ�����VM��
	for(int dayIndex = 0; dayIndex<maxDayNumber; dayIndex++){
		error[dayIndex] = 1.0;
		for(int i = candidatesNumber-1; i>=0;i--){
			rates[dayIndex][i] = 0.1-(0.02*(candidatesNumber-i));
		}
	}
}


predictVM::~predictVM(void)
{
}


//��ʼ����ҪԤ�����Ϣ
void predictVM::initPredictPurpose(char* info[MAX_INFO_NUM])	{
	cout<<"Init input information..."<<endl;
	readFile r;
	//��ʼ����Ҫ�����������Ϣ
	//cout<<sizeof(info) / sizeof(info[0])<<endl;
	string line = info[0];
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //����ͷβ�հ׷�
    	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	vector<string> fields = r.getSingleFields(line,' ');
	this->cpu = atoi(r.Trim(fields[0]).c_str());
	this->mem = atoi(r.Trim(fields[1]).c_str());
	this->disk = atoi(r.Trim(fields[2]).c_str());

	//��ʼ����ҪԤ���������ͺ�
	int machineLine = 3;
	for(; machineLine<MAX_INFO_NUM; machineLine++)	{	//��ȡ��ҪԤ���������ͺ�
		fields.clear();
		line = info[machineLine];
		line.erase(0,line.find_first_not_of(" \t\r\n"));  //����ͷβ�հ׷�
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
		if(line.length()>0)	{
			fields = r.getSingleFields(line,' ');
			if(fields.size()<3)	cout<<"Invalid machine type line"<<endl;
			else{
				hash_map<int, int> inner;
				inner[atoi(r.Trim(fields[1]).c_str())] = atoi(r.Trim(fields[2]).c_str());
				machinesType[r.Trim(fields[0])] = inner;
				//cout<<fields[0]<<" "<<fields[1]<<" "<<fields[2]<<endl;
			}
		}//endif init machine types
		else	//��ҪԤ���������ͺų�ʼ����������������
			break;
	}//endfor init machine types
	//cout<<machineLine<<endl;
	machineLine += 1;
	//��ʼ����Ҫ�Ż�����Դ����,type=0ΪCPU��type=1Ϊ�ڴ�
	line = info[machineLine];		//���е���һ��
	//cout<<line<<endl;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //����ͷβ�հ׷�
    line.erase(line.find_last_not_of(" \t\r\n") + 1);
	if(line.compare("CPU") || line.compare("cpu"))	this->predictType = 0;
	else this->predictType = 1;

	//��ʼ����ҪԤ�����ʼ����ֹʱ��
	machineLine+= 2;	//���˵�����
	line = info[machineLine];
	//cout<<line<<endl;
	selfTime t;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //����ͷβ�հ׷�
   	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	this->startTime = t.StringToDatetime(line);
	tm startTM = t.StringToTM(line);
	line = info[machineLine+1];
	//cout<<line<<endl;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //����ͷβ�հ׷�
   	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	this->endTime = t.StringToDatetime(line);
	tm endTM = t.StringToTM(line);
	
	//�ܹ�ҪԤ�Ƶ�����
	dayNumber = endTM.tm_mday - startTM.tm_mday;
	
	cout<<"Input info Init END"<<endl;

}


//��һ�����ϲ������ݵ����룬����һ�����ϲ������ݵ��������
//����һ��ʽ��xi = yi *��׼�� +��ֵ
//type=0��һ��data��type=1����һ��data
double predictVM::normalReverseTestData(double data, double mean, double deviation, int type){
	//cout<<"aaa"<<endl;	
	double result = 0.0;
	if(!type)	//��һ����������
	{
		if(deviation == 0)	return data;
		result = fabs( (data - mean) / deviation);
	}
	else	//����һ������������
	{
		if( fabs(data)<1e-5 || fabs(data)>1e4)	return 0;
		else result = data * deviation + mean;
	}
	return result;
}

//��һ��ѵ��������:yi = (xi - ��ֵ)/x�ı�׼��
//�Ի����ھ���n��ʱ�ĸ������й�һ��
void predictVM::normalTrainData(){
	cout<<"Normalization Train Data Begin..."<<endl;
	if(trainVM.size() == 0){
		cout<<"Pls read TrainData First!"<<endl;
		exit(1);
	}
	cout<<"Norm train INPUT begin......"<<endl;
	//��һ��ѵ�����ݵ�����
	hash_map<string, hash_map<int, int> >::iterator outIter;
	outIter = trainVM.begin();
	//��¼�����¸������ܺ͡�innerHash�ĸ�����ƽ��ֵ����ƽ����
	int total, size;
	double mean, subSquareSum;	
	while(outIter != trainVM.end()){	//��ÿһ�ֻ���
		total = 0; mean = 0.0; subSquareSum = 0.0;
		hash_map<int, int> temp = outIter->second;
		size = temp.size();	//��¼innerHash�ĸ���
		hash_map<int, int>::iterator innerIter = temp.begin();
		while(innerIter!=temp.end()){
			total += innerIter->second;	//��¼�������ܺ�
			innerIter++;
		}
		//�����ֵ
		mean = (double)total/size;
		//�����ƽ����
		innerIter = temp.begin();
		while(innerIter!= temp.end()){
			subSquareSum += pow( (double)(innerIter->second-mean), 2);
			innerIter++;
		}
		//�����׼��
		double standDeviationTemp = sqrt( subSquareSum / size);

		//��һ������
		hash_map<int, double> normInner;
		innerIter = temp.begin();
		while(innerIter!= temp.end()){
			if(standDeviationTemp == 0.0) normInner[innerIter->first] = innerIter->second;
			else	normInner[innerIter->first] =  fabs((innerIter->second - mean)/standDeviationTemp) ;
			//cout<<innerIter->first<<" "<<normInner[innerIter->first]<<endl;
			innerIter++;
		}
		norm_trainVM[outIter->first] = normInner;
		input_mean[outIter->first] = mean;
		input_standDeviation[outIter->first] = standDeviationTemp;
		outIter++;
		//cout<<normInner.size()<<" "<<mean<<" "<<standDeviationTemp<<endl;
	}//endwhile each Machine Type
	cout<<"Norm train INPUT end"<<endl;
	cout<<"Norm train OUTPUT begin......"<<endl;
	//��һ��ѵ�����ݵ��������
	outIter = testVM.begin();
	//��¼�����¸������ܺ͡�innerHash�ĸ�����ƽ��ֵ����ƽ����
	while(outIter != testVM.end()){	//��ÿһ�ֻ���
		total = 0; mean = 0.0; subSquareSum = 0.0;
		hash_map<int, int> temp = outIter->second;
		size = temp.size();	//��¼innerHash�ĸ���
		hash_map<int, int>::iterator innerIter = temp.begin();
		while(innerIter!=temp.end()){
			total += innerIter->second;	//��¼�������ܺ�
			innerIter++;
		}
		//�����ֵ
		mean = (double)total/size;
		//�����ƽ����
		innerIter = temp.begin();
		while(innerIter!= temp.end()){
			subSquareSum += pow( (double)(innerIter->second-mean), 2);
			innerIter++;
		}
		//�����׼��
		double standDeviationTemp = sqrt( subSquareSum / size);

		//��һ������
		hash_map<int, double> normInner;
		innerIter = temp.begin();
		//cout<<temp.size()<<endl;
		while(innerIter!= temp.end()){
			if(standDeviationTemp == 0.0) normInner[innerIter->first] = innerIter->second;
			else	normInner[innerIter->first] =  (innerIter->second - mean)/standDeviationTemp ;
			//cout<<innerIter->first<<" "<<normInner[innerIter->first]<<endl;
			innerIter++;
		}
		norm_testVM[outIter->first] = normInner;
		output_mean[outIter->first] = mean;
		output_standDeviation[outIter->first] = standDeviationTemp;
		outIter++;
		//cout<<normInner.size()<<" "<<mean<<" "<<standDeviationTemp<<endl;
	}//endwhile each Machine Type
	cout<<"Norm train OUTPUT end......"<<endl;
	cout<<"Normalization END"<<endl;
}

//������ҪԤ��Ļ��ͣ���ȡѵ������
void predictVM::readTrainData(char* data[MAX_DATA_NUM], int data_num)	{
	if(this->predictType <0){
		cout<<"Pls initPredictPurpose FIRST!"<<endl;
		exit(1);
	}
	if(data_num<=0)	cout<<"Can NOT read Train Data!"<<endl;
	cout<<"Read Train Data..."<<endl;
	string line = "";
	readFile r;
	selfTime t;
	vector<string> fields;
	for(int lineNumber = 0; lineNumber<data_num; lineNumber++)	{
		if(! (data[lineNumber] == NULL) )	{ //���ǿ���
			line = data[lineNumber];
			line.erase(0,line.find_first_not_of(" \t\r\n"));  //����ͷβ�հ׷�
			line.erase(line.find_last_not_of(" \t\r\n") + 1);
			fields = r.getSingleFields(line, '\t');
			if(fields.size()<3) cout<<"Invalid Train Data Line"<<endl;
			else{
				//cout<<line<<endl;
				hash_map<int, int> inner;
				if(machinesType.find(fields[1]) != machinesType.end()){		//����ҪԤ���������ͺŲż�¼
					inner.clear();
					int day = (int)difftime(startTime, t.StringToDatetime(fields[2]))/60/60/24 +1;
					//cout<<"))"<<day<<endl;
					if(trainVM.find(fields[1])!= trainVM.end()){		//����������ͺŵļ�¼
						inner = trainVM[fields[1]];
						if(inner.find(day)!=inner.end())	//�������һ��ļ�¼����������
						{	inner[day] += 1;	}
						else
						{	inner[day] = 1;	}		//����=1��
						trainVM[fields[1]] = inner;
					}
					else{		//û�������ͺŵ����ݣ�����
						inner[day] = 1;
						trainVM[fields[1]] = inner;
					}
				}//endif find machine Type
			}
		}//endif readLine
	}//endfor lineNumber
	cout<<"Read Train END"<<endl;
}

//��ȡ���Լ�
void predictVM::initTestData4Train(string path){
	cout<<"Init Test Data....."<<endl;
	ifstream fin(path);
	readFile r;
	selfTime t;
	if(fin == NULL){
		cout<<"Failuer to open FILE!"<<endl;
		exit(1);
	}
	//cout<<path<<endl;
	string line = "";
	vector<string> fields;
	while(getline(fin, line))
	{
		//cout<<line<<endl;
		fields = r.getSingleFields(line, '\t');
		if(fields.size()<3) cout<<"Invalid Test Data Line"<<line<<endl;
		else{
			hash_map<int, int> inner;
			if(machinesType.find(fields[1]) != machinesType.end()){		//����ҪԤ���������ͺŲż�¼
				inner.clear();
				int day = -(int)difftime(startTime, t.StringToDatetime(fields[2]))/60/60/24;	//��dayNumber����ҪԤ�⣬��0��ʼ����
				//cout<<day<<endl;
				//cout<<startTime<<" "<<fields[2]<<" "<<day<<endl;
				if(testVM.find(fields[1])!= trainVM.end()){		//����������ͺŵļ�¼
					inner = testVM[fields[1]];
					if(inner.find(day)!=inner.end())	//�������һ��ļ�¼����������
					{	inner[day] += 1;	}
					else
					{	inner[day] = 1;	}		//����=1��
					testVM[fields[1]] = inner;
				}
				else{		//û�������ͺŵ����ݣ�����
					inner[day] = 1;
					testVM[fields[1]] = inner;
				}
			}//endif find machine type
		}
	}//endwhile read file
	fin.close();
	cout<<"Init Test Data END"<<endl;
}

//��ʼ��candidatesNumber��ģ�Ͳ���ֵ
void predictVM::initCandidates(){
	cout<<"Init Candidates..."<<endl;
	std::default_random_engine random(time(NULL));  	
	//std::uniform_real_distribution<double> dis(-(double)sqrt((double)0.5/(candidatesNumber+1)), (double)sqrt((double)0.5/(candidatesNumber+1)));

	std::uniform_real_distribution<double> dis(-(double)1/(candidatesNumber+1), (double)1/(candidatesNumber+1));
	for(int dayIndex = 0; dayIndex<maxDayNumber; dayIndex++)
		for(int i = 0; i<candidatesNumber; i++){
			nodes[dayIndex][i] = dis(random); //cout<<nodes[dayIndex][i]<<endl;
			//nodes[dayIndex][i] = 0.1 - (0.02*i);
		}
	cout<<"Candidates init END"<<endl;
}


double round(double val)
{
    return (val> 0.0) ? floor(val+ 0.5) : ceil(val- 0.5);
}


//��ѵ���õĲ�����д���ļ���
//ѧϰ���Զ���ʼ��
//�ļ���ʽ���ͺ�,Ԥ������,���ڵ�Ȩֵ����dayNumber*candidatesNumber����,�����ֵ,�����׼��,�����ֵ,�����׼��
void predictVM::writeWeights(string path){
	ofstream in;
	in.open(path,ios::app); //ios::app��ʾд�룬�ļ��������򴴽���������β��׷�� //ios::trunc��ʾ�ڴ��ļ�ǰ���ļ����,������д��,�ļ��������򴴽�
	hash_map<string, double>::iterator iter;
	iter = input_mean.begin();
	while(iter!=input_mean.end()){
		in<<iter->first<<","<<dayNumber<<",";
		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			for(int nodeIndex = 0; nodeIndex<candidatesNumber; nodeIndex++){
				in<<nodes[dayIndex][nodeIndex]<<",";
			}
		}
		in<<iter->second<<","<<input_standDeviation[iter->first]<<","<<output_mean[iter->first]<<","<<output_standDeviation[iter->first]<<"\n";
		iter++;
	}
	in.close();
}

//��ȡѵ���õĲ���
//�ļ���ʽ���ͺ�,Ԥ������,���ڵ�Ȩֵ����dayNumber*candidatesNumber����,�����ֵ,�����׼��,�����ֵ,�����׼��
void predictVM::readWeights(string path){
	readFile r;
	ifstream fin(path);
	if(fin == NULL)	{
		cout<<"Failure to open FILE!"<<endl;
		exit(1);
	}
	if(this->predictType <0){
		cout<<"Pls initPredictPurpose FIRST!"<<endl;
		exit(1);
	}
	cout<<"Init Weights Begin......"<<endl;
	string line = "";
	while(getline(fin, line)){
		vector<string> fields;
		fields = r.getSingleFields(line, ',');
		string machine = fields[0];
		//cout<<machine<<endl;
		if(machinesType.find(machine)!=machinesType.end()){		//����ҪԤ��Ļ���
			int maxDay = atoi(fields[1].c_str());
			if(dayNumber >14)	dayNumber = 14;	//max predict day number is 14 days
			if(dayNumber > maxDay) continue;			//ģ�Ͳ����ֻ���+������7��Ĳ��������ҪԤ�������<=7��ģ�>7���Ԥ����Ҫʹ��14���ģ�Ͳ���
			//cout<<maxDay<<endl;
			vector<double> nodesInfo;
			int index = 2;
			for( int count = 0; count<(maxDay*candidatesNumber); count++){
				nodesInfo.push_back(atof(fields[index].c_str()));
				index++;
			}
			machineNodesInfo[machine] = nodesInfo;
			//cout<<nodesInfo.size()<<endl;
			input_mean[machine] = atof(fields[index].c_str());
			input_standDeviation[machine] = atof(fields[index+1].c_str());
			output_mean[machine] = atof(fields[index+2].c_str());
			output_standDeviation[machine] = atof(fields[index+3].c_str());
			//cout<<fields[index+3]<<endl;
		}
		else continue;	//��һ������
	}
	fin.close();
	cout<<"Weights init END"<<endl;
}


//ѵ����������
void predictVM::trainData(){
	if(this->predictType <0){
		cout<<"Pls initPredictPurpose FIRST!"<<endl;
		exit(1);
	}

	cout<<"train Begin......"<<endl;
	hash_map<string, hash_map<int, int> >::iterator machineTypeOutIter;
	machineTypeOutIter = this->machinesType.begin();
	while(machineTypeOutIter != this->machinesType.end()){	//����ÿһ�����ͼ���dayNumber��Ԥ��ֵ
		double dayOutNumber[maxDayNumber] = {0.0};		//Ԥ���ÿһ�ֻ��͵����ֵ��Ԥ������1~2�ܣ�ʵ��Ԥ��dayNumber��
		//hash_map<int, int> typeTrainInfo;
		hash_map<int, double> typeTrainInfo;
		if(this->norm_trainVM.find(machineTypeOutIter->first) != this->norm_trainVM.end() )
			typeTrainInfo = this->norm_trainVM[machineTypeOutIter->first];
			//typeTrainInfo = this->trainVM[machineTypeOutIter->first];	//���Ͷ�Ӧ��ѵ��������<����startTime������������>
		else continue;


		hash_map<int, double>::iterator trainInnerIter = typeTrainInfo.begin();
		//hash_map<int, int>::iterator trainInnerIter = typeTrainInfo.begin();
		//Ԥ��ÿһ�ֻ��͵�δ��ÿһ������ֵ
		while(trainInnerIter!=typeTrainInfo.end()){		//��60��/candidatesNumberΪһ��
			for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){	//��dayNumber����ҪԤ�⣬dayIndexΪ����startTime������
				error[dayIndex] = 0.0;
				for(int groupIndex = 0; groupIndex< (60/candidatesNumber); groupIndex++){	//�жϣ�day+dayIndex�����ڵڼ���
					if( ((60/candidatesNumber)*groupIndex) < (trainInnerIter->first+dayIndex) 
						&& (trainInnerIter->first+dayIndex) <=( (60/candidatesNumber)*(groupIndex+1) ) ){	//�Ƿ����ڵ�groupIndex��
							//cout<<"aa"<<dayIndex<<" "<<trainInnerIter->second<<" "<<nodes[dayIndex][groupIndex]<<endl;
							//cout<<trainInnerIter->first<<" group"<<groupIndex<<" "<<dayOutNumber[dayIndex]<<" "<<nodes[dayIndex][groupIndex]<<" "<<trainInnerIter->second<<" ";
							dayOutNumber[dayIndex] += nodes[dayIndex][groupIndex]*trainInnerIter->second;
							//cout<<"dayOut="<<dayOutNumber[dayIndex]<<endl;
					}
					else continue;
				}//endfor group
			}//endfor dayNumber
			trainInnerIter++;
		}//endwhile trainInnerInfo
		//���ݲ��Լ������㵱ǰ���͵������ֵ
		hash_map<int, double> testInfo = this->norm_testVM[machineTypeOutIter->first];	//���Ͷ�Ӧ�Ĳ��Լ�����<����startTime��������ʵ�ʸ���>
		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			double eachErr;		//ÿһ������ֵ
			double count = 0;
			if(testInfo.find(dayIndex)!=testInfo.end()){	//���Լ�����һ�����������
				count = testInfo[dayIndex];
			}
			//cout<<"a "<<count<<" "<<dayOutNumber[dayIndex]<<endl;
			//cout<<dayIndex<<" "<<dayOutNumber[dayIndex]<<endl;
			//if(dayOutNumber[dayIndex]<0 || dayOutNumber[dayIndex] > count)
				//eachErr = dayOutNumber[dayIndex] - count;
			//else 	
				eachErr = count - dayOutNumber[dayIndex];
			//cout<<machineTypeOutIter->first<<" "<<"day"<<dayIndex<<" "<<dayOutNumber[dayIndex]<<"=(int)"<<fabs(round(dayOutNumber[dayIndex]))<<"\t";
			cout<<machineTypeOutIter->first<<" "<<"day"<<dayIndex<<" "<<dayOutNumber[dayIndex]<<"=(int)"<<normalReverseTestData(dayOutNumber[dayIndex], output_mean[machineTypeOutIter->first], output_standDeviation[machineTypeOutIter->first], 1)<<"\t";
			error[dayIndex] += eachErr;
			cout<<"count="<<count<<" "<<eachErr<<" "<<error[dayIndex]<<endl;
		}
		//cout<<"error = "<<error<<" ";
		//error = error/dayNumber;
		//cout<<error<<endl;

		//���ݣ�������������
		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			for(int i= 0; i <candidatesNumber; i++){
				nodes[dayIndex][i] += rates[dayIndex][i] * error[dayIndex];
				//cout<<"day="<<dayIndex<<" node="<<i<<" "<<nodes[dayIndex][i]<<endl;
			}
		}
		

		machineTypeOutIter++;
	}//endwhile machineType

	cout<<"Train END"<<endl;
}

//����ѵ��ģ�ͣ��õ�ģ�͵ĸ�������ֵ
void predictVM::trainUnderLine(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM], int data_num){
	initPredictPurpose(info);
	readTrainData(data, data_num);
	string path = "../data/test.txt";
	//cout<<"Pls input test file path:";
	//cin>>path;
	initTestData4Train(path);
	cout<<"!!"<<trainVM.size()<<endl;
	normalTrainData();
	int times = 0;
	cout<<"Train UnderLine Begin..."<<endl;
	while(times<600){
		trainData();
		cout<<"-----Times="<<times<<"------"<<endl;
		times++;
	}
	cout<<"Train Underline END!"<<endl;
	cout<<"Record model's informaiton: Weight & Bias etc. into File: (y/n) ";
	string yORn = "";
	cin>>yORn;
	if(string(yORn) == string("y"))	{
		string outPath = "../model.txt";
		//cout<<"Pls input output file path:";
		//cin>>outPath;
		writeWeights(outPath);	
		cout<<"Record Done!"<<endl;
	}
}

//��һ�����ϲ��Ե����ݼ�
void predictVM::normalTestData(){
	if(this->predictType <0){
		cout<<"Pls initPredictPurpose FIRST!"<<endl;
		exit(1);
	}
	if(trainVM.size() <=0){
		cout<<"Pls read TrainData FIRST!"<<endl;
	}
	cout<<"Normal Data Input Begin......"<<endl;
	hash_map<string, hash_map<int, int> >::iterator outIter;
	outIter = trainVM.begin();
	while(outIter != trainVM.end()){
		hash_map<int, int>::iterator innerIter = trainVM[outIter->first].begin();
		hash_map<int, double> normValues;
		while(innerIter!= trainVM[outIter->first].end()){
			normValues[innerIter->first] = normalReverseTestData(innerIter->second, input_mean[outIter->first], input_standDeviation[outIter->first], 0);
			//cout<<input_mean[outIter->first]<<" "<<input_standDeviation[outIter->first]<<endl;
			innerIter++;
		}
		norm_trainVM[outIter->first] = normValues;
		outIter++;
	}
	cout<<"Normal Data Input END"<<endl;
}

//���ϲ���ģ��
hash_map<string, int> predictVM::testOnline(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM],int data_num){
	hash_map<string, int> resultAll;
	string weightPath = "../ecs/averageWeights.txt";
	//cout<<"Pls input weight file path:";
	//cin>>weightPath;
	initPredictPurpose(info);			//��ʼ����ҪԤ�����Ϣ
	readWeights(weightPath);			//��ȡѵ���õ�ģ�Ͳ�����Ϣ
	readTrainData(data, data_num);		//��ȡѵ������
	normalTestData();					//��һ�����ϲ��Ե�ѵ������
	hash_map<string, hash_map<int, int> >::iterator outIter = machinesType.begin();		//��ÿһ����ҪԤ��Ļ��ͣ�����ҪԤ��ÿһ����м���
	while(outIter!= machinesType.end()){
		//��Ȩֵ��Ϣmap�ж�ȡ��һ����ÿһ���candidatesNumber��Ȩֵ��Ϣ
		vector<double> machinetypeNodeInfo = machineNodesInfo[outIter->first];
		int index = 0;
		if(dayNumber > 14)	dayNumber = 14;		//max predict day number is 14 days
		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			for(int nodeIndex = 0; nodeIndex<candidatesNumber; nodeIndex++){
				this->nodes[dayIndex][nodeIndex] = machinetypeNodeInfo[index];
				//cout<<this->nodes[dayIndex][nodeIndex]<<" ";
				index++;
			}
		}
		double predictDayOutNumber[maxDayNumber] = {0.0};	//����Ҫ����ֵ����
		//for(int i =0; i<maxDayNumber; i++)	cout<<predictDayOutNumber[i]<<" ";
		//cout<<endl;
		//�Ӳ������ݼ��л�ȡ��һ�ͺŵ����ݼ�
		hash_map<int, double>::iterator trainInfoIter = norm_trainVM[outIter->first].begin();
		while(trainInfoIter!= norm_trainVM[outIter->first].end()){
			for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){	//��dayNumber����ҪԤ�⣬dayIndexΪ����startTime������
				for(int groupIndex = 0; groupIndex< (duration/candidatesNumber); groupIndex++){	//�жϣ�day+dayIndex�����ڵڼ���
					if( ((duration/candidatesNumber)*groupIndex) < (trainInfoIter->first+dayIndex) 
						&& (trainInfoIter->first+dayIndex) <=( (duration/candidatesNumber)*(groupIndex+1) ) ){	//�Ƿ����ڵ�groupIndex��
							//cout<<"aa"<<trainInnerIter->second<<endl;
							//cout<<trainInnerIter->first<<" group"<<groupIndex<<" "<<dayOutNumber[dayIndex]<<" "<<nodes[dayIndex][groupIndex]<<" "<<trainInnerIter->second<<" ";
							predictDayOutNumber[dayIndex] += nodes[dayIndex][groupIndex]*trainInfoIter->second;
							//cout<<"dayOut="<<dayOutNumber[dayIndex]<<endl;
					}
					else continue;
				}//endfor group
			}//endfor dayNumber
			trainInfoIter++;
		}//endwhile for each train data line
		int machineTotalNumber = 0;
		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			int singleDayOutNumber = normalReverseTestData(predictDayOutNumber[dayIndex], output_mean[outIter->first], output_standDeviation[outIter->first], 1);
			//cout<<outIter->first<<" "<<singleDayOutNumber<<endl;
			machineTotalNumber += singleDayOutNumber;
		}
		//cout<<outIter->first<<" "<<machineTotalNumber<<endl;
		resultAll[outIter->first] = machineTotalNumber;
		outIter++;
	}//enwhile for each machine type
	return resultAll;
}

//����Ȩ�صľ�ֵ
//7��ġ�14���Ȩ�ؾ�ֵ�ֱ����
void predictVM::averageWeights(string path){
	readFile r;
	ifstream fin(path);
	if(fin == NULL)	{
		cout<<"File Open FAIL!"<<endl;
		exit(1);
	}
	cout<<"Calculate Average Weights, Mean & Deviation Begin......"<<endl;
	string line = "";
	vector<string> fields;
	int recordDay = 0;
	hash_map<string, hash_map<int, vector<double> > > machineWeightsMeanDeviations;
	cout<<"Read File Begin......"<<endl;
	while(getline(fin, line))	{
		//cout<<line<<endl;
		fields = r.getSingleFields(line,',');
		if(fields.size()<6)	cout<<"Invalid weights line"<<endl;
		else{
			string machineType = fields[0];
			recordDay = atoi(r.Trim(fields[1]).c_str());
			int index = 2;
			vector<double> weights;
			for( ; index< fields.size(); index++){
				weights.push_back( atof(r.Trim(fields[index]).c_str() ) );
				//cout<<fields[index]<<" ";
			}
			cout<<weights.size()<<endl;
			hash_map<int, vector<double> > innerRecord;
			if(machineWeightsMeanDeviations.find(machineType)!=machineWeightsMeanDeviations.end()){
				innerRecord = machineWeightsMeanDeviations[machineType];
				innerRecord[innerRecord.size()] = weights;	//����һ����¼
				//cout<<innerRecord.size()<<endl;
			}
			else{
				innerRecord[0] = weights;		//�¼�¼
			}
			machineWeightsMeanDeviations[machineType] = innerRecord;
		}
	}//endwhile read File
	fin.close();
	//cout<<machineWeightsMeanDeviations.size()<<endl;

	cout<<"End read File"<<endl;
	cout<<"Calculating Begin......"<<endl;
	string outPath = "./averageModel-14.txt";
	ofstream in;
	in.open(outPath,ios::app); //ios::app��ʾд�룬�ļ��������򴴽���������β��׷�� //ios::trunc��ʾ�ڴ��ļ�ǰ���ļ����,������д��,�ļ��������򴴽�
	hash_map<string, hash_map<int, vector<double> > >::iterator outIter = machineWeightsMeanDeviations.begin();
	while(outIter!=machineWeightsMeanDeviations.end()){
		in<<outIter->first<<","<<recordDay<<",";
		for(int index = 0; index<recordDay*candidatesNumber+4; index++){	//��recordDay*candidatesNumber+mean+deviation�����������ֵ
			double totalNumber= 0.0;
			//cout<<outIter->second.size()<<endl;
			for(int innerIndex = 0; innerIndex<outIter->second.size(); innerIndex++){	//��ÿһ����¼
				totalNumber += outIter->second[innerIndex][index];
				//cout<<"aa "<<outIter->second[innerIndex][index]<<" ";
			}
			double average = (double)totalNumber/outIter->second.size();
			//cout<<average<<endl;
			in<<average<<",";
		}//endfor each weights
		in<<"\n";
		outIter++;
	}
	in.close();
	cout<<"Calculating End"<<endl;

}

/*
void main()	{
	
}
*/

