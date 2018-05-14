#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "lib_io.h"


#define MAX_INFO_NUM 50
#define MAX_DATA_NUM 20000


void predict_server(char * info[MAX_INFO_NUM], char * data[MAX_DATA_NUM], int data_num, char * filename);

	

#endif
