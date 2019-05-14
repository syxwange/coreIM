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


int CUtil::randomInt()
{
	std::default_random_engine randEngine((std::random_device())());
	std::uniform_int_distribution<int> distribution;
	return distribution(randEngine);
}