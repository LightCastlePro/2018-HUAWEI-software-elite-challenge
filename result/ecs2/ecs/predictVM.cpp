#include "predictVM.h"
#include "readFile.h"
#include "selfTime.h"
#include "hashmapString.h"
#include "lib_io.h"
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
	dayNumber = -1;
	initCandidates();
	//初始化学习率，离预测日期近的，权值低（刚请求完VM不会再请求了，请求一段时间后再次请求VM）
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


//初始化需要预测的信息
void predictVM::initPredictPurpose(char* info[MAX_INFO_NUM])	{
	cout<<"Init input information..."<<endl;
	readFile r;
	//初始化需要物理服务器信息
	string line = info[0];
	//cout<<line.length()<<endl;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
    	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	//cout<<line.length()<<endl;
	vector<string> fields = r.getSingleFields(line,' ');
	this->cpu = atoi(r.Trim(fields[0]).c_str());
	this->mem = atoi(r.Trim(fields[1]).c_str());
	this->disk = atoi(r.Trim(fields[2]).c_str());
	//string xx = "\n";
	//cout<<"aaa"<<xx.compare(info[1])<<endl;
	//初始化需要预测的虚拟机型号
	int machineLine = 3;
	for(; machineLine<MAX_INFO_NUM; machineLine++)	{	//读取需要预测的虚拟机型号
		fields.clear();
		line = info[machineLine];
		line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
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
		else	//需要预测的虚拟机型号初始化结束，遇到空行
			break;
	}//endfor init machine types
	//cout<<machineLine<<endl;
	machineLine += 1;
	//初始化需要优化的资源类型,type=0为CPU，type=1为内存
	line = info[machineLine];		//空行的下一行
	//cout<<line<<endl;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
    	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	if(line.compare("CPU") || line.compare("cpu"))	this->predictType = 0;
	else this->predictType = 1;

	//初始化需要预测的起始、终止时间
	machineLine+= 2;	//过滤掉空行
	line = info[machineLine];
	//cout<<line<<endl;
	selfTime t;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
   	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	this->startTime = t.StringToDatetime(line);
	//tm startTM = t.StringToTM(line);
	line = info[machineLine+1];
	//cout<<line<<endl;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
   	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	this->endTime = t.StringToDatetime(line);
	//tm endTM = t.StringToTM(line);
	
	//总共要预计的天数
	dayNumber = (int)-difftime(startTime, endTime)/60/60/24;
	//cout<<dayNumber<<"  @@"<<endl;
	if(dayNumber>14)	dayNumber = 14;	//max predict day is 14
	
	cout<<"Input info Init END"<<endl;

}


//归一化线上测试数据的输入，反归一化线上测试数据的输出个数
//反归一公式：xi = yi *标准差 +均值
//type=0归一化data；type=1反归一化data
double predictVM::normalReverseTestData(double data, double mean, double deviation, int type){
	//cout<<"aaa"<<endl;	
	double result = 0.0;
	if(!type)	//归一化输入数据
	{
		if(deviation == 0)	return data;
		result = fabs( (data - mean) / deviation);
	}
	else	//反归一化网络输出结果
	{
		if( fabs(data)<0.00001 || fabs(data)>10000)	return 0;
		else result = data * deviation + mean;
	}
	return result;
}

//归一化训练集数据:yi = (xi - 均值)/x的标准差
//对机型在距离n天时的个数进行归一化
void predictVM::normalTrainData(){
	cout<<"Normalization Train Data Begin..."<<endl;
	if(trainVM.size() == 0){
		cout<<"Pls read TrainData First!"<<endl;
		exit(1);
	}
	cout<<"Norm train INPUT begin......"<<endl;
	//归一化训练数据的输入
	hash_map<string, hash_map<int, int> >::iterator outIter;
	outIter = trainVM.begin();
	//记录机型下个数的总和、innerHash的个数、平均值、差平方和
	int total, size;
	double mean, subSquareSum;	
	while(outIter != trainVM.end()){	//对每一种机型
		total = 0; mean = 0.0; subSquareSum = 0.0;
		hash_map<int, int> temp = outIter->second;
		size = temp.size();	//记录innerHash的个数
		hash_map<int, int>::iterator innerIter = temp.begin();
		while(innerIter!=temp.end()){
			total += innerIter->second;	//记录个数的总和
			innerIter++;
		}
		//计算均值
		mean = (double)total/size;
		//计算差平方和
		innerIter = temp.begin();
		while(innerIter!= temp.end()){
			subSquareSum += pow( (double)(innerIter->second-mean), 2);
			innerIter++;
		}
		//计算标准差
		double standDeviationTemp = sqrt( subSquareSum / size);

		//归一化数据
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
	//归一化训练数据的期望输出
	outIter = testVM.begin();
	//记录机型下个数的总和、innerHash的个数、平均值、差平方和
	while(outIter != testVM.end()){	//对每一种机型
		total = 0; mean = 0.0; subSquareSum = 0.0;
		hash_map<int, int> temp = outIter->second;
		size = temp.size();	//记录innerHash的个数
		hash_map<int, int>::iterator innerIter = temp.begin();
		while(innerIter!=temp.end()){
			total += innerIter->second;	//记录个数的总和
			innerIter++;
		}
		//计算均值
		mean = (double)total/size;
		//计算差平方和
		innerIter = temp.begin();
		while(innerIter!= temp.end()){
			subSquareSum += pow( (double)(innerIter->second-mean), 2);
			innerIter++;
		}
		//计算标准差
		double standDeviationTemp = sqrt( subSquareSum / size);

		//归一化数据
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

//根据需要预测的机型，读取训练数据
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
		if(! (data[lineNumber] == NULL) )	{ //不是空行
			line = data[lineNumber];
			line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
			line.erase(line.find_last_not_of(" \t\r\n") + 1);
			fields = r.getSingleFields(line, '\t');
			if(fields.size()<3) cout<<"Invalid Train Data Line"<<endl;
			else{
				//cout<<line<<endl;
				hash_map<int, int> inner;
				if(machinesType.find(fields[1]) != machinesType.end()){		//是需要预测的虚拟机型号才记录
					inner.clear();
					int day = (int)difftime(startTime, t.StringToDatetime(fields[2]))/60/60/24 +1;
					//cout<<"))"<<day<<endl;
					if(trainVM.find(fields[1])!= trainVM.end()){		//如果有这种型号的记录
						inner = trainVM[fields[1]];
						if(inner.find(day)!=inner.end())	//如果有这一天的记录，更新数量
						{	inner[day] += 1;	}
						else
						{	inner[day] = 1;	}		//否则，=1个
						trainVM[fields[1]] = inner;
					}
					else{		//没有这种型号的数据，新增
						inner[day] = 1;
						trainVM[fields[1]] = inner;
					}
				}//endif find machine Type
			}
		}//endif readLine
	}//endfor lineNumber
	
	//zkl-20180322 
	//if number is more than 10, then power it by 2, then divided by 100
	if(trainVM.size()>0)
		for(hash_map<string, hash_map<int, int> >::iterator outIter = trainVM.begin(); outIter!=trainVM.end(); outIter++){
			for(hash_map<int, int>::iterator innerIter = trainVM[outIter->first].begin(); innerIter!=trainVM[outIter->first].end(); innerIter++){
				if(innerIter->second >=10){
					//cout<<innerIter->second<<" ";
					int number = ceil(innerIter->second * innerIter->second /100);
					trainVM[outIter->first][innerIter->first] = number;
					//cout<<trainVM[outIter->first][innerIter->first]<<endl;
				}
			}
		}
	cout<<"Read Train END"<<endl;
}

//读取测试集
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
			if(machinesType.find(fields[1]) != machinesType.end()){		//是需要预测的虚拟机型号才记录
				inner.clear();
				int day = -(int)difftime(startTime, t.StringToDatetime(fields[2]))/60/60/24;	//共dayNumber天需要预测，从0开始计数
				//cout<<day<<endl;
				//cout<<startTime<<" "<<fields[2]<<" "<<day<<endl;
				if(testVM.find(fields[1])!= trainVM.end()){		//如果有这种型号的记录
					inner = testVM[fields[1]];
					if(inner.find(day)!=inner.end())	//如果有这一天的记录，更新数量
					{	inner[day] += 1;	}
					else
					{	inner[day] = 1;	}		//否则，=1个
					testVM[fields[1]] = inner;
				}
				else{		//没有这种型号的数据，新增
					inner[day] = 1;
					testVM[fields[1]] = inner;
				}
			}//endif find machine type
		}
	}//endwhile read file
	fin.close();

	//zkl-20180322
	//if number is more than 10, power it by 2, then divided by 100
	if(testVM.size()>0)
		for(hash_map<string, hash_map<int, int> >::iterator outIter = testVM.begin(); outIter!=testVM.end(); outIter++){
			for(hash_map<int, int>::iterator innerIter = testVM[outIter->first].begin(); innerIter!=testVM[outIter->first].end();innerIter++){	
				if(innerIter->second>=10){
					int number = ceil( innerIter->second*innerIter->second /100);
					testVM[outIter->first][innerIter->first] = number;
				}
			}
		}

	cout<<"Init Test Data END"<<endl;
}

//初始化candidatesNumber个模型参数值
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


//将训练好的参数集写入文件中
//学习率自动初始化
//文件格式：型号,预测天数,各节点权值（共dayNumber*candidatesNumber个）,输入均值,输入标准差,输出均值,输出标准差
void predictVM::writeWeights(string path){
	ofstream in;
	in.open(path,ios::app); //ios::app表示写入，文件不存在则创建，有则在尾部追加 //ios::trunc表示在打开文件前将文件清空,由于是写入,文件不存在则创建
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

//读取训练好的参数
//文件格式：型号,预测天数,各节点权值（共dayNumber*candidatesNumber个）,输入均值,输入标准差,输出均值,输出标准差
void predictVM::readWeights(string path){
	/*
	readFile r;
	char * weightsDataFromFile[20000];
	int weights_line_num = read_file(weightsDataFromFile, 20000, path.c_str());
	for(int lineIndex = 0; lineIndex<weights_line_num; lineIndex++){
		string line = weightsDataFromFile[lineIndex];
		vector<string> fields;
		fields = r.getSingleFields(line, ',');
		string machine = fields[0];
		//cout<<machine<<endl;
		if(machinesType.find(machine)!=machinesType.end()){		//是需要预测的机型
			int maxDay = atoi(fields[1].c_str());
			if(dayNumber>14) this->dayNumber = 14;	//max predict day number is 14 days
			
			if(dayNumber > maxDay) continue;			//模型参数分机型+天数，7天的参数针对需要预测的天数<=7天的，>7天的预测需要使用14天的模型参数
			//cout<<"!!"<<maxDay<<"))"<<dayNumber<<endl;
			//cout<<fields.size()<<endl;
			vector<double> nodesInfo;
			nodesInfo.clear();
			int index = 2;
			for( int count = 0; count<(maxDay*candidatesNumber); count++){
				nodesInfo.push_back(atof(fields[index].c_str()));
				index++;
				//cout<<count<<" "<<endl;
			}
			//cout<<index<<endl;
			machineNodesInfo[machine] = nodesInfo;
			//cout<<nodesInfo.size()<<endl;
			input_mean[machine] = atof(fields[index].c_str());
			input_standDeviation[machine] = atof(fields[index+1].c_str());
			output_mean[machine] = atof(fields[index+2].c_str());
			output_standDeviation[machine] = atof(fields[index+3].c_str());
			//cout<<fields[index+3]<<endl;
		}
		else continue;	//下一行数据
	}
	/*
	ifstream fin(path);
	if(fin == NULL)	{
		cout<<"Failure to open FILE!"<<endl;
		exit(1);
	}
	/*
	if(this->predictType <0){
		cout<<"Pls initPredictPurpose FIRST!"<<endl;
		exit(1);
	}
	cout<<"Init Weights Begin......"<<endl;
	string line = "";
	/*
	while(getline(fin, line)){
		vector<string> fields;
		fields = r.getSingleFields(line, ',');
		string machine = fields[0];
		//cout<<machine<<endl;
		if(machinesType.find(machine)!=machinesType.end()){		//是需要预测的机型
			int maxDay = atoi(fields[1].c_str());
			if(dayNumber>14) this->dayNumber = 14;	//max predict day number is 14 days
			
			if(dayNumber > maxDay) continue;			//模型参数分机型+天数，7天的参数针对需要预测的天数<=7天的，>7天的预测需要使用14天的模型参数
			//cout<<"!!"<<maxDay<<"))"<<dayNumber<<endl;
			vector<double> nodesInfo;
			nodesInfo.clear();
			int index = 2;
			for( int count = 0; count<(maxDay*candidatesNumber); count++){
				nodesInfo.push_back(atof(fields[index].c_str()));
				index++;
			}
			cout<<index<<endl;
			machineNodesInfo[machine] = nodesInfo;
			//cout<<nodesInfo.size()<<endl;
			input_mean[machine] = atof(fields[index].c_str());
			input_standDeviation[machine] = atof(fields[index+1].c_str());
			output_mean[machine] = atof(fields[index+2].c_str());
			output_standDeviation[machine] = atof(fields[index+3].c_str());
			cout<<fields[index+3]<<endl;
		}
		else continue;	//下一行数据
	}*/
	//fin.close();


	cout<<"Weights init END"<<endl;
}


//训练各个参数
void predictVM::trainData(){
	if(this->predictType <0){
		cout<<"Pls initPredictPurpose FIRST!"<<endl;
		exit(1);
	}

	cout<<"train Begin......"<<endl;
	hash_map<string, hash_map<int, int> >::iterator machineTypeOutIter;
	machineTypeOutIter = this->machinesType.begin();
	while(machineTypeOutIter != this->machinesType.end()){	//遍历每一个机型计算dayNumber的预测值
		double dayOutNumber[maxDayNumber] = {0.0};		//预测的每一种机型的输出值，预测跨度在1~2周，实际预测dayNumber天
		//hash_map<int, int> typeTrainInfo;
		hash_map<int, double> typeTrainInfo;
		if(this->norm_trainVM.find(machineTypeOutIter->first) != this->norm_trainVM.end() )
			typeTrainInfo = this->norm_trainVM[machineTypeOutIter->first];
			//typeTrainInfo = this->trainVM[machineTypeOutIter->first];	//机型对应的训练集数据<距离startTime的天数，个数>
		else continue;


		hash_map<int, double>::iterator trainInnerIter = typeTrainInfo.begin();
		//hash_map<int, int>::iterator trainInnerIter = typeTrainInfo.begin();
		//预测每一种机型的未来每一天的输出值
		while(trainInnerIter!=typeTrainInfo.end()){		//以60天/candidatesNumber为一组
			for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){	//共dayNumber天需要预测，dayIndex为距离startTime的天数
				error[dayIndex] = 0.0;
				for(int groupIndex = 0; groupIndex< (60/candidatesNumber); groupIndex++){	//判断（day+dayIndex）属于第几组
					if( ((60/candidatesNumber)*groupIndex) < (trainInnerIter->first+dayIndex) 
						&& (trainInnerIter->first+dayIndex) <=( (60/candidatesNumber)*(groupIndex+1) ) ){	//是否属于第groupIndex组
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
		//根据测试集，计算当前机型的总误差值
		hash_map<int, double> testInfo = this->norm_testVM[machineTypeOutIter->first];	//机型对应的测试集数据<距离startTime的天数，实际个数>
		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			double eachErr;		//每一天的误差值
			double count = 0;
			if(testInfo.find(dayIndex)!=testInfo.end()){	//测试集的那一天有这个机型
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

		//误差传递，修正各个参数
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

//线下训练模型，得到模型的各个参数值
void predictVM::trainUnderLine(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM], int data_num){
	initPredictPurpose(info);
	readTrainData(data, data_num);
	string path = "../data/test6.txt";
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
		string outPath = "../data/model-7.txt";
		//cout<<"Pls input output file path:";
		//cin>>outPath;
		writeWeights(outPath);	
		cout<<"Record Done!"<<endl;
	}
}

//归一化线上测试的数据集
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

//线上测试模型
hash_map<string, int> predictVM::testOnline(char* info[MAX_INFO_NUM], char* data[MAX_DATA_NUM],int data_num){
	hash_map<string, int> resultAll;
	string weightPath = "../ecs/averageWeights.txt";
	//cout<<"Pls input weight file path:";
	//cin>>weightPath;
	initPredictPurpose(info);			//初始化需要预测的信息
	readWeights(weightPath);			//读取训练好的模型参数信息
	readTrainData(data, data_num);		//读取训练数据
	normalTestData();					//归一化线上测试的训练数据

	readFile r;

	string temp[] = {
	"flavor1,7,-0.0175151,-0.054534,-0.0880181,-0.12549,0.0967038,0.231307,0.417144,0.345862,0.0332493,-0.0312026,-0.0339905,0.0425496,0.0161025,-0.0632671,-0.0846858,-0.0757172,0.00523959,-0.0278267,-0.0513892,-0.0610617,0.0373335,0.104402,0.25707,0.238505,-0.0428843,-0.0152891,-0.0270876,-0.0428506,1.94352,1.17571,1.2,0.26496",
"flavor14,7,0.0125404,-0.009246,-0.00790255,-0.0723752,0.00961478,0.215915,0.357329,0.430636,-0.067693,-0.114417,-0.132394,-0.155912,0.00344879,0.0501806,0.0770168,0.177586,-0.0389266,-0.147623,-0.0352162,-0.0545717,0.099153,0.127453,0.166525,0.188135,0.0723423,-0.152244,-0.0876103,-0.174102,4.99487,3.66263,4.41667,2.18983",
"flavor9,7,0.0058158,0.214411,0.118858,0.226492,-0.0531632,-0.07669,-0.0530961,-0.0393783,-0.0244904,-0.0327309,0.0525657,-0.0393935,-0.0246889,0.116376,0.149311,0.110279,0.0986375,-0.135854,-0.165669,-0.116195,0.0376177,0.0388676,-0.152714,-0.231846,0.0100369,-0.0311463,-0.181729,-0.0234802,3.00673,3.21668,2.73333,1.93006",
"flavor2,7,-0.084766,0.0848548,0.188721,0.237363,0.00517627,-0.116979,0.014965,-0.0773043,-0.0339476,-0.0114284,-0.00239567,-0.131525,-0.113753,-0.0151802,-0.0739502,-0.159097,-0.0156822,0.124183,0.169767,0.0605058,-0.00847127,0.298437,0.497877,0.611531,0.0349397,-0.076339,-0.195943,-0.310652,4.22619,3.54899,4.61111,2.24622",
"flavor3,7,0.0775534,0.0677505,0.171045,0.21101,-0.0607728,-0.0697677,-0.100775,-0.128267,-0.0231076,0.0202755,0.115734,0.0830679,-0.112295,-0.0649457,-0.167769,-0.233832,0.0875112,0.209594,0.282218,0.341885,0.0124237,-0.0146383,0.0341402,-0.00456778,0.0408288,0.0230552,0.039066,0.0786657,1.16077,0.318763,1.2,0.2",
"flavor11,7,-0.0147935,0.0243624,0.104609,0.133592,0.00010066,-0.0243227,0.0764881,0.0797987,-0.102264,-0.0386698,0.0331829,0.0809579,-0.0112728,-0.0473444,0.0149087,0.0019822,-0.0248402,0.110768,0.0303191,0.210073,-0.0381079,0.0468083,0.0251065,-0.0721202,-0.0450415,0.0694663,-0.0108675,0.0373787,2.60139,1.86595,3.83333,1.05127",
"flavor4,7,-0.0169951,-0.0914767,-0.0525327,0.0422023,0.0130348,-0.0272484,-0.0291397,-0.16786,-0.0302177,0.0748924,-0.146639,-0.217872,-0.0551001,-0.00317367,0.116344,0.115335,0.0755925,0.0349945,-0.0176327,-0.067028,-0.13795,0.124629,0.0935533,0.147282,0.0860597,0.161874,0.126395,-0.0683201,1.49259,1.45191,1.11111,0.157135",
"flavor10,7,0.0449738,-0.00642483,-0.0461341,0.137492,-0.0282689,0.00188459,0.192729,0.0543297,-0.083869,0.00955267,-0.0498684,0.0544084,0.0191687,-0.0047713,0.0214291,0.238973,-0.166734,-0.115303,-0.167165,-0.125262,0.201886,-0.154975,-0.093606,0.000194156,0.233504,0.236749,0.308501,0.108086,2.11111,3.1427,1.33333,0.471405",
"flavor5,7,0.0201752,0.00822707,-0.0654716,-0.127852,0.0759178,-0.0379252,-0.0247513,0.0185805,0.0535338,-0.0325554,0.0620658,0.0593517,-0.0977629,-0.0650672,-0.199546,-0.218385,0.0468799,0.0794542,0.0627422,0.270582,0.0258309,0.055806,0.0688554,0.0701714,-0.00350301,0.0390997,-0.016513,0.037501,3.83989,4.43304,5.64583,4.2572",
"flavor13,7,0.0766399,-0.015328,-0.39364,-0.482497,0.142966,-0.0945934,0.235407,-0.052786,0.181594,-0.0897845,-0.0229523,0.0640036,0.0918452,-0.0173059,-0.0186348,-0.156108,-0.0843164,0.0752319,0.00227112,0.240943,0.10447,0.366175,0.315351,0.445537,-0.0191415,0.140327,-0.0302963,-0.0248567,2,1.73205,1,0",
"flavor6,7,0.0603455,0.168502,0.310346,0.510238,-0.136113,-0.046968,0.0441996,-0.078318,-0.00926897,0.109076,0.00481867,0.0804743,-0.0668229,0.129553,-0.00401303,-0.131775,-0.0445316,-0.0490958,-0.0346201,-0.122556,-0.0699385,-0.0382431,0.0918524,-0.139097,0.0157661,-0.00386523,-0.0485826,-0.117199,2.65185,1.38399,3.11111,2.23801",
"flavor12,7,0.0256091,0.266337,0.382688,0.714304,-0.0601668,0.1387,0.306903,0.14531,0.00338741,0.0129829,-0.114536,-0.00369075,0.0259396,0.00999871,-0.350173,-0.244284,-0.0150183,0.0542332,-0.231679,-0.00878542,-0.0745958,0.257166,0.377649,0.384386,0.104124,0.0761686,0.0338142,0.00779827,6.1875,6.71955,1,0",
"flavor7,7,0.052413,0.13043,0.0566103,0.216946,0.0999243,-0.184971,-0.0836457,-0.237675,0.0100375,0.266902,0.404723,0.451303,-0.241335,-0.312209,-0.55074,-0.662312,0.350946,0.580509,0.983623,1.22668,-0.0949578,-0.44425,-0.751396,-0.84136,0.0561844,0.0669889,0.0901925,0.109217,2.41738,2.27838,2.375,1.125",
"flavor8,7,0.0427811,0.0766347,0.100069,0.0391418,-0.0217339,-0.0191613,-0.0049183,0.0130653,-0.0403827,-0.0337877,-0.0561782,-0.109572,-0.0058519,-0.0280979,0.0469055,-0.158491,-0.0114171,0.0697518,0.20622,0.180776,-0.00280342,-0.0533802,0.0409978,-0.125934,0.0681149,0.0339194,0.0390625,0.043128,4.83672,5.54256,4.00476,4.22265",
"flavor15,7,0.310806,0.830479,1.10742,1.47893,-0.0949966,0.0818123,0.037737,0.296491,0.0671554,-0.0668793,-0.195593,-0.25808,-0.0103193,0.0428041,-0.0669042,-0.0742535,0.109771,-0.097803,-0.0446637,-0.157567,0.00658807,0.0237149,-0.176652,-0.11634,-0.0443538,0.101614,0.153477,0.145405,3.77778,2.80558,2.33333,0",
"flavor1,14,0.0025162,-0.0474142,-0.132126,-0.151301,0.141946,0.285472,0.220385,0.452727,-0.0170779,-0.00573563,-0.000240617,-0.0219372,-0.00170652,0.016712,0.0711859,0.00588883,0.0590132,-0.0459719,-0.0740999,-0.0959253,-0.021904,-0.095536,-0.113809,-0.170673,-0.0282961,-0.0893964,0.000941967,-0.114418,0.0237486,0.106781,0.257982,0.320309,-0.0181724,0.00444273,-0.0532715,-0.0863568,-0.0210475,0.00440034,-0.0078071,-0.087499,-0.000288383,-0.0393513,0.036432,0.0131298,-0.015853,-0.00195982,0.0143367,0.101234,-0.0169066,0.141561,0.115369,0.126602,0.0354109,0.00424283,0.0938618,0.0973109,2.03827,1.29097,2,1.11048",
"flavor14,14,-0.0380512,0.0133597,0.0839272,0.0999575,0.0558341,0.057494,0.126071,0.110964,-0.0498742,-0.0277064,-0.0771106,-0.060579,0.0270078,0.0692239,0.106516,0.141753,-0.082681,-0.0676652,0.0149817,0.051126,0.0509253,0.113992,0.14427,0.241466,-0.037381,-0.0865073,-0.15755,-0.164152,0.02232,0.0044354,-0.00682968,-0.0348686,-0.0670597,0.0272603,0.111454,-0.00946908,0.0532845,0.0261866,-0.0434726,-0.08212,0.0236956,-0.0164529,0.0505159,0.084312,-0.013873,-0.00285731,-0.140348,-0.115251,0.0134819,0.00764887,-0.034288,-0.229656,-0.0277334,0.0459986,-0.0230986,-0.097935,5.08472,4.44294,6.06944,4.23697",
"flavor9,14,-0.0518863,0.109157,0.0794059,0.114873,0.00744813,-0.0117799,-0.0545333,-0.124448,0.0540217,-0.0568564,-0.0163689,-0.0633436,-0.00240265,0.158847,0.122872,0.247271,0.00172575,0.0347831,-0.0524824,-0.128626,0.0718547,0.00902185,0.0566153,0.0523553,0.0342865,-0.0160117,0.0236782,0.132267,-0.0203418,-0.0829421,-0.0896524,-0.207025,-0.0921899,-0.0155243,0.0333001,-0.0820155,0.00897385,-0.0249405,-0.0848874,0.0450935,0.021783,-0.0662667,0.0415631,0.143864,0.0176191,0.041767,0.00922577,-0.0772211,-0.0282724,0.0261367,0.0165899,0.0538067,0.0127822,-0.0101551,-0.0437533,-0.0220845,2.12156,1.71064,2.87778,2.09864",
"flavor2,14,-0.0151069,-0.051156,-0.112961,-0.053885,-0.0737617,-0.137012,-0.293822,-0.2378,-0.015394,-0.0228351,0.0563108,-0.230914,-0.102126,-0.0664636,-0.146007,-0.106501,-0.0064624,-0.121138,-0.0893819,-0.0580255,0.0543446,0.0959282,0.236988,0.271879,0.0717184,0.0599543,-0.0724348,0.00476798,0.0390331,0.0331163,-0.0496175,-0.0879733,0.042883,0.00849278,-0.00834987,0.0193426,0.0951421,0.0949823,0.152699,0.259306,-0.0404623,-0.0572229,-0.0683608,-0.160061,-0.025916,0.0610124,0.200279,0.356869,-0.0379172,0.0656171,0.264082,0.272347,-0.0405312,-0.0634595,-0.0128156,0.0114966,3.33929,2.74878,3.00162,2.27461",
"flavor3,14,-0.0790917,-0.0700745,-0.188137,-0.22221,0.0276027,-0.0068307,-0.153868,-0.166928,0.069,-0.087514,-0.0026684,-0.164152,-0.0809142,0.0511408,0.0233432,0.106767,-0.115939,-0.0670377,-0.0386388,0.0269742,-0.0412774,-0.012644,0.022533,0.163535,0.114301,-0.00446307,-0.0782171,0.0443913,-0.0797726,-0.087604,-0.194813,-0.187672,0.146876,0.191021,0.415838,0.617168,-0.0223641,0.0044969,0.0508084,-0.174155,0.0271196,0.095145,-0.0193578,0.0126328,0.0905013,-0.118617,-0.0361363,0.0738138,0.0202869,-0.0179639,-0.094713,-0.16789,-0.0623394,-0.0898885,0.0163287,-0.071326,1.18462,0.386934,1.24444,0.423802",
"flavor11,14,0.0790002,0.145298,0.176819,0.138488,-0.0571765,0.0106126,0.0358247,-0.0592029,-0.10065,0.0233996,0.0324181,0.0930095,-0.0111634,-0.0888463,-0.128705,-0.157052,0.0107023,0.0887815,0.154121,0.154471,0.0359422,0.0411108,-0.018331,0.0614313,0.071295,-0.0872607,0.0008881,0.0303129,0.0334321,0.0309102,-0.0052133,0.0503738,-0.0159291,-0.0286764,-0.0516633,-0.0899703,0.0608111,0.0589581,-0.103731,-0.0855128,0.0709347,0.0300411,-0.00475103,-0.027206,-0.0188579,-0.100351,-0.133242,-0.208145,-0.0310614,-0.0188741,-0.0418281,-0.0601173,0.0191882,0.0417064,0.00809092,-0.0319945,2.48033,1.72762,2.78016,1.56511",
"flavor4,14,-0.045374,-0.0742576,0.0274015,0.106439,0.005928,0.0890085,0.122466,0.171619,0.0953679,-0.0588763,0.0077595,-0.134727,0.0447593,-0.0791141,0.0630359,0.182488,-0.133721,0.0706336,-0.0560544,0.0938138,0.0938569,-0.137592,0.0724912,0.0189987,-0.027109,0.0642083,2.1e-005,-0.0141985,0.110545,-0.0992078,-0.0310375,-0.0237062,-0.0015977,-0.0300416,-0.0242419,-0.110914,0.039915,-0.0886023,-0.11053,-0.24675,0.0211856,0.0476283,0.197126,0.524014,-0.004854,-0.136499,0.106335,-0.0526836,0.0434717,0.158127,0.172695,0.177965,-0.0806785,0.047666,-0.0247613,-0.072015,1.65556,1.99153,1.38889,0.7738",
"flavor10,14,0.166909,-0.0238441,0.320457,0.253913,-0.202588,0.0135059,0.0502401,-0.14967,-0.152537,-0.00519001,0.225197,0.0723362,0.00787274,0.0301028,-0.199604,-0.0568303,0.0842186,-0.119027,-0.0849292,-0.242558,0.00577991,-0.0604531,-0.426441,-0.531467,-0.0169076,0.32483,0.339308,0.447949,-0.133936,-0.0629064,0.0350511,0.0294095,0.133259,-0.230257,0.0464593,-0.0526957,-0.0250809,-0.0152249,0.00351343,0.256333,0.165792,-0.11286,0.0260447,0.0681506,0.0935131,-0.121905,0.0281319,0.141907,-0.0147317,-0.0256942,-0.119685,-0.457835,-0.0629837,0.0239308,-0.00552249,-0.28477,2.11111,3.1427,1.25,0.433013",
"flavor5,14,-0.0529787,0.0255832,-0.00253819,0.0103472,-0.00241739,0.017187,0.0207728,0.0615202,-0.0386051,0.000665875,0.0508238,0.0980252,-0.0376739,-0.119425,-0.139438,-0.147337,-0.000734648,0.0790941,0.189665,0.16133,-0.0125342,0.00131968,0.0211486,0.111282,0.0396286,-0.115188,0.0542083,-0.0558594,-0.0501617,-0.0201822,-0.0537213,-0.0747218,-0.0111831,0.0387999,-0.0286641,-0.0138695,-0.0613665,-0.013102,0.0827677,0.140641,0.0674337,0.00207141,-0.0277967,-0.0255576,-0.0471585,0.0111556,0.0448203,0.0109138,0.0187872,-0.03413,-0.0537612,-0.0523959,0.0167188,-0.0126848,0.0266464,-0.0438313,3.83988,4.43304,5.73333,4.28516",
"flavor13,14,0.151802,-0.0303605,0.0649892,0.0970652,-0.192975,0.0536743,-0.0217226,-0.150615,0.0279469,-0.0596897,0.00793571,0.103299,-0.0824775,-0.0687064,0.037796,0.139843,-0.0529922,-0.00764868,0.0151602,-0.186598,0.0464408,0.299131,0.34662,0.229961,0.085278,-0.0396029,-0.0114188,-0.180772,-0.0603745,-0.146029,0.0516008,-0.0413831,0.175489,-0.180188,0.00117476,-0.110126,0.147758,-0.0557702,0.0278851,0.0876793,0.0866042,0.0106813,-0.00534065,0.249077,-0.211804,-0.0338752,0.0169376,-0.0952269,-0.0653039,0.0302602,-0.0151301,-0.441741,0.0268199,0.00765722,-0.00382861,0.293985,2,1.73205,1,0",
"flavor6,14,0.0267118,0.204908,0.25538,0.302153,-0.0859009,0.00511956,0.0611823,-0.0277192,0.0140044,0.0390149,-0.0285533,-0.00799118,0.130919,-0.0875015,0.00986276,0.0653036,-0.0441892,-0.0758221,-0.284611,-0.268196,0.0759994,-0.0424589,-0.0707062,0.122953,-0.010836,0.0437624,0.0648843,0.0271375,-0.130043,-0.0882685,-0.0951208,-0.130004,0.0213974,0.00810982,-0.0675404,-0.0404586,0.0229368,0.0461446,-0.0347215,0.100814,-0.00336084,0.0640454,-0.114505,-0.0516315,0.0281001,0.0646286,0.150288,0.231725,0.0187079,0.0843183,0.0275468,0.0538132,-0.0620784,-0.0761831,-0.112569,-0.167945,2.24496,1.41987,3.08,2.20881",
"flavor12,14,-0.0717996,0.0309866,-0.152713,-0.142332,0.0446694,-0.0123696,-0.01505,-0.0528035,-0.060797,0.00579305,0.0915548,0.0063245,0.0111018,0.0553739,0.00793955,0.0991141,0.127635,-0.0725,-0.131801,-0.02585,-0.105001,0.0309634,0.008503,-0.0095505,0.127227,-0.155874,-0.0636365,-0.0712195,0.0612762,-0.154323,-0.258051,-0.260106,0.0931065,0.129337,0.126621,0.296389,-0.0580912,-0.0183142,-0.0587634,-0.265238,-0.0211434,0.0307559,0.0347929,-0.105217,0.089505,-0.108447,0.0115813,0.0746332,0.052505,0.103995,-0.0285199,-0.0723516,0.225855,0.0812257,-0.0161553,0.125143,5.09375,4.52474,3.25,3.24519",
"flavor7,14,0.114303,0.0862168,0.112581,0.286942,0.0219526,-0.167627,-0.299142,-0.310122,0.149516,0.113062,0.293194,0.377547,-0.224675,-0.162064,-0.482682,-0.629766,0.249292,0.408248,0.773352,0.850091,-0.0223286,-0.247966,-0.290527,-0.350957,0.0431917,-0.120925,-0.31512,-0.517107,0.0867776,0.100867,0.409039,0.543193,0.0350815,-0.0764335,-0.015866,-0.138727,0.0416702,0.00632653,0.217529,0.246684,-0.0388763,-0.00762507,0.196998,0.225763,-0.0907759,0.152279,0.059419,-0.042593,-0.105541,-0.0385654,-0.0334637,0.0320137,0.10066,0.0384527,-0.129325,-0.272842,2.24881,2.12767,2.83333,1.5",
"flavor8,14,0.0173378,0.121175,0.141504,0.182762,0.0261066,-0.0263647,-0.0684897,0.0318287,-0.0694249,0.0363869,-0.144038,-0.162395,-0.0554366,-0.011134,0.090399,-0.0422739,0.052685,-0.0141487,0.0110766,-0.0188718,0.0340435,-0.0654608,0.0818078,-0.0298615,-0.0426142,0.0154179,-0.0259662,0.0217169,-0.0292408,0.00705643,0.0471252,0.0795071,0.0582137,-0.0346591,-0.0389218,0.0355479,0.0546674,-0.0532344,0.0123269,0.0479733,0.029014,-0.0201874,0.00869321,0.039287,0.0421514,0.0281506,0.0384209,0.0968541,-0.0359957,-0.0266929,0.0132957,0.0619944,0.0181336,-0.0122573,-0.0236091,-0.0951698,4.83672,5.54256,4.69028,4.86154",
"flavor15,14,0.0086831,0.0787832,0.177263,0.181008,0.0907309,0.0464927,0.052258,0.0362466,0.0102913,0.0436333,0.0363759,-0.0690738,0.110553,0.0580118,-0.0261219,0.0847531,-0.0116096,-0.00352423,0.0386516,0.0485014,-0.116551,0.0380004,0.0477043,0.0260098,0.0312001,0.0870747,0.119898,-0.0430966,0.0688396,0.0656001,0.276573,0.284598,0.109387,0.0367661,0.0244218,-0.0309605,-0.0974235,-0.0509031,-0.0479625,-0.0317596,0.0060912,-0.000463825,-0.0305589,-0.0580581,-0.0888266,-0.0318825,-0.0439388,-0.0908332,0.0446223,0.0185159,-0.0124093,-0.0331292,-0.0123906,-0.0298295,-0.0199604,-0.0570088,3.95833,3.24397,1.75,0"
};
	hash_map<string, double> temphash;
	vector<double> tempnodeinfo;
	for(int i = 0; i<30; i++){
		tempnodeinfo.clear();
		string templine = temp[i];
		vector<string> fields;
		fields = r.getSingleFields(templine, ',');
		if(machinesType.find(fields[0])!=machinesType.end()){
			if(maxDayNumber<dayNumber){
				dayNumber = 14;
			}
			if(dayNumber<=atoi(r.Trim(fields[1]).c_str())){
				for(int j = 2; j<fields.size(); j++){
					tempnodeinfo.push_back(atof(r.Trim(fields[j]).c_str()));
				}
				hash_map<int, double>::iterator trainInfoIter = norm_trainVM[fields[0]].begin();
				double tempOutNumber[dayNumber];
				while(trainInfoIter!= norm_trainVM[fields[0]].end()){
					for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){	//共dayNumber天需要预测，dayIndex为距离startTime的天数
						for(int groupIndex = 0; groupIndex< candidatesNumber; groupIndex++){	//判断（day+dayIndex）属于第几组

							if( ((duration/candidatesNumber)*groupIndex) < (trainInfoIter->first+dayIndex) 
							&& (trainInfoIter->first+dayIndex) <=( (duration/candidatesNumber)*(groupIndex+1) ) ){	//是否属于第groupIndex组
								tempOutNumber[dayIndex] = tempOutNumber[dayIndex] + tempnodeinfo[dayIndex*groupIndex +groupIndex]*trainInfoIter->second;
								
							}
							else continue;


						}//endfor group
					}//endfor dayNumber
				trainInfoIter++;
				}//endwhile for each train data line

				int machineTotalNumber = 0;
				//cout<<dayNumber<<endl;
				for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
					double singlemean = tempnodeinfo[tempnodeinfo.size()-2];
					double singleDeviation = tempnodeinfo[tempnodeinfo.size()-1];
					double singledata = tempOutNumber[dayIndex];
					int singleDayOutNumber = 0;
					//cout<<fields[0]<<" "<<singlemean<<" "<<singleDeviation<<" "<<singledata<<endl;
					if( fabs(singledata)<0.00001 || fabs(singledata)>10000)	singleDayOutNumber = 0;
					else singleDayOutNumber = (int) (singledata * singleDeviation + singlemean);
					//= (int)normalReverseTestData(singledata, singlemean, singleDeviation, 1);
					//cout<<fields[0]<<" "<<singleDayOutNumber<<endl;
					machineTotalNumber = machineTotalNumber + singleDayOutNumber;
					//cout<<fields[0]<<" "<<machineTotalNumber<<endl;
					//machineTotalNumber  = machineTotalNumber +1;
				}
				if(machineTotalNumber<0) machineTotalNumber = 0;
				//cout<<fields[0]<<" "<<machineTotalNumber<<endl;
				resultAll[fields[0]] = machineTotalNumber;

			}
		}
	}

	
	
	
/*
	hash_map<string, hash_map<int, int> >::iterator outIter = machinesType.begin();		//对每一种需要预测的机型，对需要预测每一天进行计算
	while(outIter!= machinesType.end()){
		//从权值信息map中读取这一机型每一天的candidatesNumber个权值信息
		vector<double> machinetypeNodeInfo = machineNodesInfo[outIter->first];
		//cout<<machinetypeNodeInfo[1]<<endl;
		int index = 0;
		if(dayNumber > 14)	dayNumber = 14;		//max predict day number is 14 days
		double singlenode[maxDayNumber][candidatesNumber] = {0.0};
		vector<double>singleNode2;
		singleNode2.clear();

		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			for(int nodeIndex = 0; nodeIndex<candidatesNumber; nodeIndex++){
				//nodes[dayIndex][nodeIndex] = machinetypeNodeInfo[index];

				double xx = machinetypeNodeInfo[index];
				//cout<<xx<<endl;
				singlenode[dayIndex][nodeIndex] = xx;
				
				//cout<<this->nodes[dayIndex][nodeIndex]<<" ";
				singleNode2.push_back(xx);
				index++;
			}
		}
		double predictDayOutNumber[maxDayNumber] = {0.0};	//必须要赋初值啊！
		hash_map<int, double> predictTempOutNumber;
		//for(int i =0; i<maxDayNumber; i++)	cout<<predictDayOutNumber[i]<<" ";
		//cout<<endl;
		//从测试数据集中获取这一型号的数据集
		hash_map<int, double>::iterator trainInfoIter = norm_trainVM[outIter->first].begin();

		while(trainInfoIter!= norm_trainVM[outIter->first].end()){
			for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){	//共dayNumber天需要预测，dayIndex为距离startTime的天数
				for(int groupIndex = 0; groupIndex< candidatesNumber; groupIndex++){	//判断（day+dayIndex）属于第几组

					if( ((duration/candidatesNumber)*groupIndex) < (trainInfoIter->first+dayIndex) 
						&& (trainInfoIter->first+dayIndex) <=( (duration/candidatesNumber)*(groupIndex+1) ) ){	//是否属于第groupIndex组
							//cout<<"aa"<<trainInnerIter->second<<endl;
							//cout<<trainInnerIter->first<<" group"<<groupIndex<<" "<<dayOutNumber[dayIndex]<<" "<<nodes[dayIndex][groupIndex]<<" "<<trainInnerIter->second<<" ";
							predictDayOutNumber[dayIndex] += singlenode[dayIndex][groupIndex]*trainInfoIter->second;
							//if(predictTempOutNumber.find(dayIndex)!=predictTempOutNumber.end())
								//predictTempOutNumber[dayIndex] = predictTempOutNumber[dayIndex]+singlenode[dayIndex][groupIndex]*trainInfoIter->second;
							//else
								predictTempOutNumber[dayIndex] = trainInfoIter->second;					
							//cout<<"dayOut="<<dayOutNumber[dayIndex]<<endl;
					}
					else continue;


				}//endfor group
			}//endfor dayNumber
			trainInfoIter++;
		}//endwhile for each train data line

		int machineTotalNumber = 0;
		//cout<<dayNumber<<endl;
		for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){
			double singlemean = output_mean[outIter->first];
			double singleDeviation = output_mean[outIter->first];
			double singledata = predictDayOutNumber[dayIndex];
			int singleDayOutNumber = 0;
			if( fabs(singledata)<0.00001 || fabs(singledata)>10000)	singleDayOutNumber = 0;
			else singleDayOutNumber = (int) (singledata * singleDeviation + singlemean);
			//= (int)normalReverseTestData(singledata, singlemean, singleDeviation, 1);
			//cout<<outIter->first<<" "<<singleDayOutNumber<<endl;
			//machineTotalNumber = machineTotalNumber + (int)singledata;
			cout<<machineTotalNumber<<endl;
			//machineTotalNumber  = machineTotalNumber +1;
		}
		//cout<<outIter->first<<" "<<machineTotalNumber<<endl;
		resultAll[outIter->first] = machineTotalNumber;

		outIter++;
	}//enwhile for each machine type
*/

	return resultAll;
}

//计算权重的均值
//7天的、14天的权重均值分别计算
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
				innerRecord[innerRecord.size()] = weights;	//新增一条记录
				//cout<<innerRecord.size()<<endl;
			}
			else{
				innerRecord[0] = weights;		//新记录
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
	in.open(outPath,ios::app); //ios::app表示写入，文件不存在则创建，有则在尾部追加 //ios::trunc表示在打开文件前将文件清空,由于是写入,文件不存在则创建
	hash_map<string, hash_map<int, vector<double> > >::iterator outIter = machineWeightsMeanDeviations.begin();
	while(outIter!=machineWeightsMeanDeviations.end()){
		in<<outIter->first<<","<<recordDay<<",";
		for(int index = 0; index<recordDay*candidatesNumber+4; index++){	//对recordDay*candidatesNumber+mean+deviation个参数计算均值
			double totalNumber= 0.0;
			//cout<<outIter->second.size()<<endl;
			for(int innerIndex = 0; innerIndex<outIter->second.size(); innerIndex++){	//对每一条记录
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

void predictVM::initWeightsNOTfile(){
	if(this->dayNumber<0)
		cout<<"Pls init INPUT file FIRST"<<endl;
	
}

/*
void main()	{
	
}
*/

