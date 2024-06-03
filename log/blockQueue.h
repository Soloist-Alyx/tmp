#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H
#include <iostream>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>

#include "../lock/locker.h"

//template class, used for asynchronization log
//loop array: mBack = (mBack + 1) % mMaxSize;
//thread safety: remember to lock the mutex lock before any operations;
//               unlock it after finishing the operation;
template <class T>
class BlockQueue
{
    private:
        Locker mMutex;
        Cond mCond;

        T *mArray;
        int mSize;
        int mMaxSize;
        int mFront;
        int mBack;
    public:
        BlockQueue(int maxSize = 1000)
        {
            if(maxSize <= 0)
            {
                exit(-1);
            }
            mMaxSize = maxSize;
            mArray = new T[mMaxSize];
            // mSize = 0;
            // mFront = -1;
            mBack = -1;
            mFront = 0;
            mBack = -1;
        }
        ~BlockQueue()
        {
            mMutex.lock();
            if(mArray != nullptr)
            {  
                delete []mArray;
            }
            mMutex.unlock();
        }

        void clear()
        {
            mMutex.lock();
            mSize = 0;
            // mFront = -1;
            // mBack = -1;
            mFront = 0;
            mBack = -1;
            mMutex.unlock();
        }
        //determine whether the queue is full or not
        bool full()
        {
            mMutex.lock();
            if(mSize >= mMaxSize)
            {
                mMutex.unlock();
                return true;
            }
            mMutex.unlock();
            return false;
        }
        //determine whether the queue is empty or not
        bool empty()
        {
            mMutex.lock();
            if(mSize == 0)
            {
                mMutex.unlock();
                return true;
            }
            mMutex.unlock();
            return false;
        }
        //return the head of the line element
        bool front(T &value)
        {
            mMutex.lock();
            if(mSize == 0)
            {
                mMutex.unlock();
                return false;
            }
            value = mArray[mFront];
            mMutex.unlock();
            return true;

        }
        //return the tail element
        bool back(T &value)
        {
            mMutex.lock();
            if(mSize == 0)
            {
                mMutex.unlock();
                return false;
            }
            value = mArray[mBack];
            mMutex.unlock();
            return true;
        }
        int size()
        {
            int ans = 0;
            mMutex.lock();
            ans = mSize;
            mMutex.unlock();
            return ans;
        }
        int maxSize()
        {
            int ans = 0;
            mMutex.lock();
            ans = mMaxSize;
            mMutex.unlock();
            return ans;
        }
        //add new element to the queue
        bool push(const T &item)
        {
            //lock the mutex lock
            mMutex.lock();
            //if the elements are too much
            if(mSize >= mMaxSize)
            {
                //inform the customers to cost them
                mCond.broadcast();
                //unlock and return
                mMutex.unlock();
                return false;
            }
            //after making sure there are enough space, push the eleemnt
            mBack = (mBack + 1) % mMaxSize;
            mArray[mBack] = item;
            mSize++;
            //produce successfully! inform the customers
            mCond.broadcast();
            //unlock and return
            mMutex.unlock();
            return true;
        }
        //if there is no element could be pop, wait for it;
        bool pop(T &item)
        {
            mMutex.lock();
            while (mSize <= 0)
            {
                if(!mCond.wait(mMutex.getPointer()))
                {
                    mMutex.unlock();
                    return false;
                }
            }
            item = mArray[mFront];
            mFront = (mFront + 1) % mMaxSize;
            mSize--;
            mMutex.unlock();
            return true;
        }
        //timeout processing
        bool pop(T &item, int msTimeout)
        {
            struct timespec t = {0, 0};
            struct timeval now = {0, 0};
            gettimeofday(&now, nullptr);
            mMutex.lock();
            if(mSize <= 0)
            {
                t.tv_sec = now.tv_sec + msTimeout / 1000;
                t.tv_nsec = (now.tv_usec + msTimeout % 1000) * 1000;
                // t.tv_nsec = (msTimeout % 1000) * 1000;
                if(!mCond.timewait(mMutex.getPointer(), &t))
                {
                    mMutex.unlock();
                    return false;
                }
            }
            if(mSize <= 0)
            {
                mMutex.unlock();
                return false;
            }
            item = mArray[mFront];
            mFront = (mFront + 1) % mMaxSize;
            mSize--;
            mMutex.unlock();
            return true;
            
        }
};

#endif