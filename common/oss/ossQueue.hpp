#pragma once

#include <queue>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include "core.hpp"
using namespace std;


template<typename Data>
class ossQueue
{
public:
    unsigned int size()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return (unsigned int)m_queue.size();
    }


    


private:
    std::queue<Data> m_queue;
    boost::mutex  m_mutex;
    boost::condition_variable m_cond;
};






