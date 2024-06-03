#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include "blockQueue.h"
#include <stdarg.h>

using namespace std;

class Log
{
    private:
        char mDirName[128]; //the directory name(is same with program)
        char mLogName[128]; //the name of log file
        int mSplitLines;    //the max count of log's lines
        int mLogBufSize;    //buffer size
        long long mCount;   //record the count of lines
        int mToday;         //record the day
        FILE *mFp;          //the pointer of log file
        char *mBuf;         //the pointer of buffer
        BlockQueue<string> *mLogQueue;  //blocking queue
        bool mIsAsync;      //the flag shows async or not
        Locker mMutex;
        int mCloseLog;      //the flag shows close log or not
    private:
        Log();
        virtual ~Log();
        //pop log information from queue and write them to file
        void *asyncWriteLog()
        {
            string singleLog;
            while(mLogQueue->pop(singleLog))
            {
                mMutex.lock();
                fputs(singleLog.c_str(), mFp);
                mMutex.unlock();
            }
        }
    public:
        //singleton pattern
        static Log *getInstance()
        {
            static Log instance;
            return &instance;
        }
        //the working function of asynchronization pthread
        static void *flushLogThread(void *args)
        {
            Log::getInstance()->asyncWriteLog();
        }
        bool init(const char *fileName, int closeLog, int logBufSize = 8192, int splitLines = 5000000, int maxQueueSize = 0);
        void writeLog(int level, const char *format, ...);
        void flush(void);


};

#define LOG_DEBUG(format, ...) if(mCloseLog == 0) {Log::getInstance()->writeLog(0, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_INFO(format, ...) if(mCloseLog == 0) {Log::getInstance()->writeLog(1, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_WARN(format, ...) if(mCloseLog == 0) {Log::getInstance()->writeLog(2, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#define LOG_ERROR(format, ...) if(mCloseLog == 0) {Log::getInstance()->writeLog(3, format, ##__VA_ARGS__); Log::getInstance()->flush();}
#endif