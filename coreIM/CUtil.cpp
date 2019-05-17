#include "CUtil.h"
#include <qdatetime.h>
#include <random>


CUtil::CUtil()
{
}


CUtil::~CUtil()
{
}

int64_t CUtil::currentTime()
{
	return QDateTime::currentDateTime().toMSecsSinceEpoch();
}


int CUtil::randomInt(int n)
{
	std::default_random_engine randEngine((std::random_device())());
	if (n == 0)
	{
		std::uniform_int_distribution<int> distribution;
		return distribution(randEngine);
	}		
	else
	{
		std::uniform_int_distribution<int> distribution(0, n);
		return distribution(randEngine);
	}	
}