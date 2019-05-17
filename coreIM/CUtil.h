#pragma once
class CUtil
{
public:
	CUtil();
	~CUtil();

	//返回毫秒级时间戳
	static long long  currentTime();

	//返回随便数字
	static int randomInt(int n=0);
};

