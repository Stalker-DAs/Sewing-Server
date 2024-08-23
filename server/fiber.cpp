#include "fiber.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "macro.h"
#include "scheduler.h"

namespace server{

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

static std::atomic<uint64_t> s_fiber_count{0};

//当前执行的fiber
static thread_local Fiber *t_fiber = nullptr;
static thread_local Fiber::ptr t_main_fiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::AddData<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

// main Fiber的构造函数
Fiber::Fiber()
{
    m_id = s_fiber_count++;
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_ctx))
    {
        ASSERT2(false, "getcontext");
    }
}

Fiber::~Fiber(){
    --s_fiber_count;
    if (m_stack)
    {
        ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
        MallocStackAllocator::Dealloc(m_stack);
    }
    else{
        Fiber *cur = t_fiber;
        if (cur == this){
            SetThis(nullptr);
        }
    }

}

Fiber::Fiber(std::function<void()> cb, uint32_t stacksize, bool use_caller):m_cb(cb){
    m_id = s_fiber_count++;

    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getVal();

    m_stack = MallocStackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx))
    {
        ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if (!use_caller)
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    else
        makecontext(&m_ctx, &Fiber::CallMainFunc, 0);
        
    LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

void Fiber::call(){
    SetThis(this);
    m_state = EXEC;
    if (swapcontext(&t_main_fiber->m_ctx, &m_ctx)){
        ASSERT2(false, "swapcontext");
    }
}

void Fiber::back(){
    SetThis(t_main_fiber.get());
    if (swapcontext(&m_ctx, &t_main_fiber->m_ctx))
    {
        ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapIn(){
    SetThis(this);
    ASSERT(m_state != EXEC);
    m_state = EXEC;

    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)){
        ASSERT2(false, "swapcontext");
    }

}

void Fiber::swapOut(){
    SetThis(Scheduler::GetMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)){
        ASSERT2(false, "swapcontext");
    }
}

void Fiber::reset(std::function<void()> cb){
    m_cb = cb;
    if (getcontext(&m_ctx)){
        ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::SetThis(Fiber* fiber){
    t_fiber = fiber;
}

Fiber::ptr Fiber::GetThis(){
    if (t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    t_main_fiber = main_fiber;

    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady(){
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

Fiber::ptr Fiber::GetRootFiber(){
    return t_main_fiber;
}

void Fiber::YieldToHold(){
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

void Fiber::YieldToReadyBack(){
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->back();
}

void Fiber::YieldToHoldBack(){
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->back();
}

uint64_t Fiber::GetFiberId(){
    if (t_fiber){
        return t_fiber->getId();
    }
    return 0;
}

void Fiber::MainFunc(){
    Fiber::ptr cur = GetThis();
    ASSERT(cur);

    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception& ex){
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
    }
    catch(...){
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except";
    }

    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->swapOut();
    //raw_ptr->back();
}

void Fiber::CallMainFunc(){
    Fiber::ptr cur = GetThis();
    ASSERT(cur);

    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception& ex){
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
    }
    catch(...){
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except";
    }

    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->back();
    //raw_ptr->swapOut();
}
}