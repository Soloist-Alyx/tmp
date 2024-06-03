#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "log.h"
using namespace std;

Log::Log()
{
    mCount = 0;
    mIsAsync = false;
}
Log::~Log()
{
    if(mFp != nullptr)
    {
        fclose(mFp);
    }
}
//if you use the asynchronization log, you should set the size of queue(maxQueueSize)
bool Log::init(const char *fileName, int closeLog, int logBufSize, int splitLines, int maxQueueSize)
{
    //the maxQueuesize is not zero, which means the log is asynchronization
    if(maxQueueSize > 0)
    {
        mIsAsync = true;
        mLogQueue = new BlockQueue<string>(maxQueueSize);
        pthread_t tId;
        //create a pthread to white log information
        //the flushLogThread is a callback function
        pthread_create(&tId, nullptr, flushLogThread, nullptr);
    }

    //otherwise, new the buffer
    mCloseLog = closeLog;
    mLogBufSize = logBufSize;
    mBuf = new char[mLogBufSize];
    memset(mBuf, '\0', mLogBufSize);
    mSplitLines = splitLines;

    //make sure the log file name according to local time
    time_t t = time(nullptr);
    struct  tm *sysTm = localtime(&t);
    struct  tm myTm = *sysTm;
    const char *p = strrchr(fileName, '/');
    char logFullName[256] = {0};
    if(p == nullptr)
    {
        snprintf(logFullName, 255, "%d_%02d_%02d_%s", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, fileName);
    }
    else
    {
        strcpy(mLogName, p + 1);
        strncpy(mDirName, fileName, p - fileName + 1);
        snprintf(logFullName, 255, "%s%d_%02d_%02d_%s", mDirName, myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, mLogName);
    }
    mToday = myTm.tm_mday;
    //open it and return the result
    mFp = fopen(logFullName, "a");
    if(mFp == nullptr)
    {
        return false;
    }
    return true;
}
void Log::writeLog(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm *sysTm = localtime(&t);
    struct tm myTm = *sysTm;

    char type[16] = {0};
    switch (level)
    {
        case 0:
            strcpy(type, "[debug]:");
            break;
        case 1:
            strcpy(type, "[info]:");
            break;
        case 2:
            strcpy(type, "[warn]:");
            break;
        case 3:
            strcpy(type, "[erro]:");
            break;
        
        default:
            strcpy(type, "[info]:");
            break;
    }

    //write a log, the count plus one
    mMutex.lock();
    mCount++;
    //a new day or the count reached mSplitLines
    if(mToday != myTm.tm_mday || mCount % mSplitLines == 0)
    {
        //create and open a new file
        char newLog[256] = {0};
        fflush(mFp);
        fclose(mFp);
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", myTm.tm_year + 1970, myTm.tm_mon + 1, myTm.tm_mday);

        if(mToday != myTm.tm_mday)
        {
            snprintf(newLog, 255, "%s%s%s", mDirName, tail, mLogName);
            mToday = myTm.tm_mday;
            mCount = 0;
        }
        else
        {
            snprintf(newLog, 255, "%s%s%s.%lld", mDirName, tail, mLogName, mCount / mSplitLines);
        }
        mFp = fopen(newLog, "a");
    }
    mMutex.unlock();

    //write log information to the buffer
    va_list valist;
    va_start(valist, format);
    string logStr;
    mMutex.lock();
    int n = snprintf(mBuf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s", 
                    myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday,
                    myTm.tm_hour, myTm.tm_min, myTm.tm_sec, now.tv_usec, type);
    int m = vsnprintf(mBuf + n, mLogBufSize - n - 1, format, valist);
    mBuf[n + m] = '\n';
    mBuf[n + m + 1] = '\0';
    logStr = mBuf;
    mMutex.unlock();

    //async:push to queue;
    //sync:call fputs directly
    if(mIsAsync && !mLogQueue->full())
    {
        mLogQueue->push(logStr);
    }
    else
    {
        mMutex.lock();
        fputs(logStr.c_str(), mFp);
        mMutex.unlock();
    }
    va_end(valist);
}
//flush the buffer by force
void Log::flush(void)
{
    mMutex.lock();
    fflush(mFp);
    mMutex.unlock();
}