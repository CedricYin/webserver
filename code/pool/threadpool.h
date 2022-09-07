#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);
            // 创建threadCount个线程
            for(size_t i = 0; i < threadCount; i++) {
                std::thread([pool = pool_] {  // 使用lambda表达式，表示工作函数
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while(true) {
                        if(!pool->tasks.empty()) {  // 若工作队列不为空，出队并工作
                            auto task = std::move(pool->tasks.front());
                            pool->tasks.pop();  // 出队
                            locker.unlock();
                            task();  // 工作
                            locker.lock();
                        } 
                        else if(pool->isClosed) break;  // 线程池关闭则退出
                        else pool->cond.wait(locker);  // 工作队列为空，休眠
                    }
                }).detach();  // 每个线程设置为脱离状态
            }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();  // 通知所有休眠中的线程退出线程函数
        }
    }

    template<class F>
    void AddTask(F&& task) {  // 往工作队列添加任务task（右值引用 + forward = 完美转发，提高效率）
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();  // 唤醒一个线程
    }

private:
    struct Pool {
        /* 用互斥锁和条件变量实现线程同步和资源的互斥访问 */
        std::mutex mtx;  // 互斥🔓
        std::condition_variable cond;  // 条件变量
        bool isClosed;  // 是否关闭线程池
        std::queue<std::function<void()>> tasks;  // 工作队列
    };
    std::shared_ptr<Pool> pool_;  // 池子
};


#endif //THREADPOOL_H