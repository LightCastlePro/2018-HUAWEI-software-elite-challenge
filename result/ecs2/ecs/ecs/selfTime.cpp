#include "selfTime.h"
#include <iostream>
#include <time.h>
#include <string>
using namespace std;

selfTime::selfTime(void)
{
	Year = Month = Day = Hour = Minute = Second = 0;
	string x = "";
}


selfTime::~selfTime(void)
{
}

/*
void main(){
	selfTime t;
	time_t iTimeStamp1, iTimeStamp2;
	string temptime = "2018-03-18 18:04:50";
	iTimeStamp1 = t.StringToDatetime(temptime);
	string tempTime2 = "2018-03-16 18:05:30";
	iTimeStamp2 = t.StringToDatetime(tempTime2);
	tm* t_tm = localtime(&iTimeStamp1); 
	cout<<"today is "<<t_tm->tm_year+1900<<" "<<t_tm->tm_mon+1<<" "<<t_tm->tm_mday<<endl;
	double diff = difftime(iTimeStamp1, iTimeStamp2);
	int days = diff/60/60/24;
	cout<<diff<<endl;

	string temp = "";
	iTimeStamp2 += 60;
	temp = t.DatetimeToString(iTimeStamp2);
	cout<<temp<<endl;
}
*/