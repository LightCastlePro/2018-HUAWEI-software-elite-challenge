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
	//cout<<sizeof(info) / sizeof(info[0])<<endl;
	string line = info[0];
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
    	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	vector<string> fields = r.getSingleFields(line,' ');
	this->cpu = atoi(r.Trim(fields[0]).c_str());
	this->mem = atoi(r.Trim(fields[1]).c_str());
	this->disk = atoi(r.Trim(fields[2]).c_str());

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
	tm startTM = t.StringToTM(line);
	line = info[machineLine+1];
	//cout<<line<<endl;
	line.erase(0,line.find_first_not_of(" \t\r\n"));  //过滤头尾空白符
   	line.erase(line.find_last_not_of(" \t\r\n") + 1);
	this->endTime = t.StringToDatetime(line);
	tm endTM = t.StringToTM(line);
	
	//总共要预计的天数
	dayNumber = endTM.tm_mday - startTM.tm_mday;
	
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
		if( fabs(data)<1e-5 || fabs(data)>1e4)	return 0;
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
		if(machinesType.find(machine)!=machinesType.end()){		//是需要预测的机型
			int maxDay = atoi(fields[1].c_str());
			if(dayNumber >14)	dayNumber = 14;	//max predict day number is 14 days
			if(dayNumber > maxDay) continue;			//模型参数分机型+天数，7天的参数针对需要预测的天数<=7天的，>7天的预测需要使用14天的模型参数
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
		else continue;	//下一行数据
	}
	fin.close();
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
	hash_map<string, hash_map<int, int> >::iterator outIter = machinesType.begin();		//对每一种需要预测的机型，对需要预测每一天进行计算
	while(outIter!= machinesType.end()){
		//从权值信息map中读取这一机型每一天的candidatesNumber个权值信息
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
		double predictDayOutNumber[maxDayNumber] = {0.0};	//必须要赋初值啊！
		//for(int i =0; i<maxDayNumber; i++)	cout<<predictDayOutNumber[i]<<" ";
		//cout<<endl;
		//从测试数据集中获取这一型号的数据集
		hash_map<int, double>::iterator trainInfoIter = norm_trainVM[outIter->first].begin();
		while(trainInfoIter!= norm_trainVM[outIter->first].end()){
			for(int dayIndex = 0; dayIndex<dayNumber; dayIndex++){	//共dayNumber天需要预测，dayIndex为距离startTime的天数
				for(int groupIndex = 0; groupIndex< (duration/candidatesNumber); groupIndex++){	//判断（day+dayIndex）属于第几组
					if( ((duration/candidatesNumber)*groupIndex) < (trainInfoIter->first+dayIndex) 
						&& (trainInfoIter->first+dayIndex) <=( (duration/candidatesNumber)*(groupIndex+1) ) ){	//是否属于第groupIndex组
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

/*
void main()	{
	
}
*/

