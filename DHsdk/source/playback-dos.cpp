// playback-dos.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>

#include "stdafx.h"
#include "dhnetsdk.h"
#include<fstream>
#pragma comment(lib, "dhnetsdk.lib")
#pragma comment(lib, "doldetect_dll.lib")

#include "doldetect_dll.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef __linux__
#define PRINTF printf
#define SCANF scanf
#define SSCANF sscanf
#else
#define PRINTF printf_s
#define SCANF scanf_s
#define SSCANF sscanf_s
#endif

static char *filename = "shipin.txt";
static FILE *finalfile = fopen(filename, "w+b");


using namespace cv;
using namespace std;

void __stdcall OnDisConnect(LLONG lLoginID, char *pchDVRIP, LONG nDVRPort, LDWORD dwUser)
{
	int		nStatus;

	if (NULL == pchDVRIP)
		return;

	printf("device disconnect, IP: %s\n", pchDVRIP);
}

void __stdcall DownLoadPosCallBackFunc(LLONG lPlayHandle, DWORD dwTotalSize, DWORD dwDownLoadSize, LDWORD dwUser)
{
	if (dwDownLoadSize == -1)
	{
		printf("PlayBackPosCallBack: 100% \n");
	}
	else if (dwTotalSize * 100 / dwTotalSize == 50 || dwTotalSize * 100 / dwTotalSize == 100)
	{
		printf("PlayBackPosCallBack: %d%%\n", dwDownLoadSize * 100 / dwTotalSize);
	}
}

int __stdcall DataCallBackFunc(LLONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, LDWORD dwUser)
{

	char szFileName[32] = { 0 };
	sprintf(szFileName, "%d.txt", lRealHandle);
	FILE *file = fopen(szFileName, "a+b");
	if (file)
	{
		//fwrite(pBuffer, 1, dwBufSize, file);
		fwrite(pBuffer, 1, dwBufSize, finalfile);
		fclose(file);
	}

	//printf("PlayBack: data++++, len=%d+++++++++++++++\n", dwBufSize);
	return 1;
}

void Clean(LLONG lPlayBackHandle, LLONG lLoginHandle)
{
	printf("please put any key to terminate downloading and start detecting:");
	getchar();
	getchar();
	if (lPlayBackHandle != 0)
	{
		CLIENT_StopPlayBack(lPlayBackHandle);
		lPlayBackHandle = 0;
	}
	CLIENT_Logout(lLoginHandle);
	CLIENT_Cleanup();
}

int _tmain(int argc, _TCHAR* argv[])
{
	// sdk init
	CLIENT_Init(OnDisConnect, 0);

	//start time
	int start_year = 0;
	int start_month = 0;
	int start_day = 0;
	int start_hour = 0;
	int start_min = 0;
	int start_second = 0;

	// log in device
	int nError = 0;
	NET_DEVICEINFO stDevInfo = { 0 };

	int nChannel = 0;
	LLONG lLoginHandle = 0;
	
	/***********************************************用户登录*****************************************************/
	char szDevIp[32] = "";
	char szUserName[32] = "";
	char szPassWord[32] = "";
	int nPort = 0;


	printf("Device IP: \n");
	scanf("%s", szDevIp);
	printf("Device port:\n");
	scanf("%d", &nPort);
	printf("username:\n");
	scanf("%s", szUserName);
	printf("password:\n");
	scanf("%s", szPassWord);

	lLoginHandle = CLIENT_LoginEx(szDevIp, nPort, szUserName, szPassWord, 0, NULL, &stDevInfo, &nError);
	if (lLoginHandle == 0)
	{
		printf("login device failed! nerror = %d\n", nError);
		Clean(NULL, lLoginHandle);
		return 0;
	}
	else
	{
		printf("login device success, channel number:%d\n", stDevInfo.byChanNum);
	}


   /*************************************************录像下载*************************************************/
	
	LLONG lPlayBackHandle = 0;

	
		if (lPlayBackHandle != 0)
		{
			CLIENT_StopPlayBack(lPlayBackHandle);
			lPlayBackHandle = 0;
		}
				
			int nChannelId = 0;
			printf("Please input a channel ID:");
			scanf("%d", &nChannelId);
			NET_TIME stStartTime = { 0 };
			NET_TIME stStopTime = { 0 };

			printf("Please input the start time of the video you want to download(format:20xx-x-x xx:xx:xx)");
			scanf("%d-%d-%d %d:%d:%d", &(stStartTime.dwYear), &(stStartTime.dwMonth), &(stStartTime.dwDay), &(stStartTime.dwHour), &(stStartTime.dwMinute), &(stStartTime.dwSecond));
			printf("Please input the end time of the video you want to download(format:20xx-x-x xx:xx:xx)");
			scanf("%d-%d-%d %d:%d:%d", &(stStopTime.dwYear), &(stStopTime.dwMonth), &(stStopTime.dwDay), &(stStopTime.dwHour), &(stStopTime.dwMinute), &(stStopTime.dwSecond));

			//把视频开始时间做一个拷贝，之后用来保存海豚出现时间
			start_year = stStartTime.dwYear;
			start_month = stStartTime.dwMonth;
			start_day = stStartTime.dwDay;
			start_hour = stStartTime.dwHour;
			start_min = stStartTime.dwMinute;
			start_second = stStartTime.dwSecond;

			lPlayBackHandle = CLIENT_PlayBackByTimeEx(lLoginHandle, nChannelId, &stStartTime, &stStopTime, NULL, DownLoadPosCallBackFunc, 0, DataCallBackFunc, 0);

			if (lPlayBackHandle != 0)
			{
				printf("play back by time success\n");
			}
			else
			{
				printf("play back by time failed\n");
			}



	Clean(lPlayBackHandle, lLoginHandle);
	fclose(finalfile);

	/***********************************************视频文件下载结束，接下来调用海豚识别引擎************************************************/
	printf("We have downloaded the file,now it's time to detect the dolphin\n");
	vector<Rect> bboxes;
	Mat frame;
	int frame_id = 0;
	//存储海豚数量与位置的文件流
	fstream dolphin;
	
	VideoCapture capture;
	capture.open(filename);
	if (!capture.isOpened()) {
		PRINTF("fail to read: %s\n", filename);
		system("pause");
		return 1;
	}



	while (capture.read(frame)) {
		//获取当前帧时间
		unsigned int elapsed_time = capture.get(CV_CAP_PROP_POS_MSEC) / 1000;

		resize(frame, frame, Size(960, 540), (0, 0), (0, 0), cv::INTER_LINEAR);
		if (!frame.data) {
			PRINTF("done\n");
			waitKey();
			return 0;
		}
		doldetect(frame, bboxes, 400, 0.5, 40, 3);
		PRINTF("[frame %d] %d\n", frame_id, int(bboxes.size()));
		for (size_t i = 0; i < bboxes.size(); ++i) {

			if (bboxes[i].x > 903 || bboxes[i].y > 48 || bboxes[i].x + bboxes[i].width < 730 || bboxes[i].y + bboxes[i].height < 11)//避开右上角
				rectangle(frame, Point(bboxes[i].x, bboxes[i].y),
				Point(bboxes[i].x + bboxes[i].width, bboxes[i].y + bboxes[i].height), Scalar(0, 255, 0));
		}

		/******************************************************计算当前帧的时间与日期**************************************************/
		
		//start_xxx是开始时间，之前已由用户输入；now_xxx是计算好后的当前时间，是保存在文档里的最终结果。
		unsigned int now_year = start_year;
		unsigned int now_month = start_month;
		unsigned int now_day = start_day;
		unsigned int now_hour = start_hour;
		unsigned int now_min = start_min;
		unsigned int now_second = start_second;

		now_second +=elapsed_time;
		while (now_second >= 60)
		{
			now_second -= 60;
			now_min++;
		}
		while (now_min >= 60)
		{
			now_min -= 60;
			now_hour++;
		}
		while (now_hour >= 24)
		{
			now_hour -= 24;
			now_day++;
		}
		
		vector<int> dayofmonth;//每个月的天数
		dayofmonth.push_back(31);
		if ((start_year % 100 == 0 && start_year % 400 == 0) || (start_year % 100 != 0 && start_year % 4 == 0))//如果当年是闰年
			dayofmonth.push_back(29);//二月有29天
		else
			dayofmonth.push_back(28);//否则为28天
		dayofmonth.push_back(31);
		dayofmonth.push_back(30);
		dayofmonth.push_back(31);
		dayofmonth.push_back(30);
		dayofmonth.push_back(31);
		dayofmonth.push_back(31);
		dayofmonth.push_back(30);
		dayofmonth.push_back(31);
		dayofmonth.push_back(30);
		dayofmonth.push_back(31);

		while (now_day > dayofmonth[now_month-1])
		{
			now_day -= dayofmonth[now_month - 1];
				now_month++;
				if (now_month == 13)//如果跨年
				{
					now_month = 1;
					now_year++;
					if ((now_year % 100 == 0 && now_year % 400 == 0) || (now_year % 100 != 0 && now_year % 4 == 0))//年份增加后，如果是闰年
						dayofmonth[1]=29;
					else
						dayofmonth[1]=28;
				}
		}


		/****************************************************************保存海豚位置为txt文本*******************************************************************/
		if (bboxes.size() != 0)
		{
			dolphin.open("dolphin.txt", ios::ate | ios::out | ios::app);
			dolphin << "The time: " << now_year << "-" << now_month << "-" <<now_day<< " " << now_hour<< ":" << now_min << ":" << now_second<< endl;
			dolphin << "The number :" << bboxes.size() << endl;
			for (size_t i = 0; i < bboxes.size(); ++i)
			{
				dolphin << "The " << i + 1 << "th is at x: " << bboxes[i].x + bboxes[i].width / 2 << " , y: " << bboxes[i].y + bboxes[i].height / 2 << endl;
				dolphin << "x:" << bboxes[i].x << ",y: " << bboxes[i].y << ",width: " << bboxes[i].width << ",height: " << bboxes[i].height << endl;
			}
			dolphin << endl;
			dolphin.close();
		}


		/*****************************************************************显示画面*****************************************************************/
		imshow("video", frame);

		waitKey(10);
		++frame_id;
	}
	
	system("pause");
	return 0;


}