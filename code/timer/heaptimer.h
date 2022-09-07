#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    // 文件描述符
    int id;  
    // 超时时间
    TimeStamp expires;  
    // 超时任务
    TimeoutCallBack cb;  
    // 重载
    bool operator<(const TimerNode& t) {  
        return expires < t.expires;
    }
};

// 数组模拟小根堆，实现时间堆定时器
class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }
    ~HeapTimer() { clear(); }
    
    // 调整文件描述符id关联的定时器的位置
    void adjust(int id, int newExpires);  
    // 添加定时器，超时时间为timeOut
    void add(int id, int timeOut, const TimeoutCallBack& cb);  
    // 删除指定id结点，并触发回调函数
    void doWork(int id);  
    // 清空时间堆
    void clear();  
    // 清除超时结点，并触发它们的回调函数
    void tick();  
    // 删除堆顶元素
    void pop();  
    // 清除超时的节点，然后获取最先要超时的连接的超时的时间
    int GetNextTick();  

private:
    // 删除指定元素
    void del_(size_t i);  
    // 上移
    void siftup_(size_t i);  
    // 下移
    bool siftdown_(size_t index, size_t n);  
    // 交换两节点
    void SwapNode_(size_t i, size_t j);  
    // 数组模拟小根堆
    std::vector<TimerNode> heap_;  
    // 文件描述符和数组下标的映射
    std::unordered_map<int, size_t> ref_;  
};

#endif //HEAP_TIMER_H