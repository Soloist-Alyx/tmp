#ifndef LOCKER_H
#define LOCKER_H
#include <semaphore.h>
#include <exception>
#include <pthread.h>

//semaphore, used for PV operation
class Sem
{
    private:
        sem_t mSem;
    public:
        Sem()
        {
            if (sem_init(&mSem, 0, 0) != 0)
            {
                throw std::exception();
            }
        }
        Sem(int num)
        {
            if (sem_init(&mSem, 0, num) != 0)
            {
                throw std::exception();
            }
        }
        ~Sem()
        {
            sem_destroy(&mSem);
        }
        bool wait()
        {
            return sem_wait(&mSem) == 0;
        }
        bool post()
        {
            return sem_post(&mSem) == 0;
        }

};

//mutex lock
class Locker
{
    private:
        pthread_mutex_t mMutex;
    public:
        Locker()
        {
            if (pthread_mutex_init(&mMutex, NULL) != 0)
            {
                throw std::exception();
            }
        }
        ~Locker()
        {
            pthread_mutex_destroy(&mMutex);
        }
        bool lock()
        {
            return pthread_mutex_lock(&mMutex) == 0;
        }
        bool unlock()
        {
            return pthread_mutex_unlock(&mMutex) == 0;
        }
        pthread_mutex_t *getPointer()
        {
            return &mMutex;
        }

};

//condition variables
//when the producer produce new resource, the function broadcast or signal should be called;
//when the customer wanna cost the resources, the function wait should be called;
class Cond
{
    private:
        pthread_cond_t mCond;
    public:
        Cond()
        {
            if (pthread_cond_init(&mCond, NULL) != 0)
            {
                throw std::exception();
            }
        }
        ~Cond()
        {
            pthread_cond_destroy(&mCond);
        }
        bool wait(pthread_mutex_t *mMutex)
        {
            int ret = 0;
            ret = pthread_cond_wait(&mCond, mMutex);
            return ret == 0;
        }
        bool timewait(pthread_mutex_t *mMutex, struct timespec *t)
        {
            int ret = 0;
            ret = pthread_cond_timedwait(&mCond, mMutex, t);
            return ret == 0;
        }
        bool signal()
        {
            return pthread_cond_signal(&mCond) == 0;
        }
        bool broadcast()
        {
            return pthread_cond_broadcast(&mCond) == 0;
        }

};
#endif