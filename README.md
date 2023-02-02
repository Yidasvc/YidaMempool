# YidaMempool
一种应对频繁申请/释放内存的内存池

1. 创建内存池

Yida_mempool_t* mempool = create_mempool(总可用字节数);

2. 在内存池中申请内存

void* ptr = mempool_alloc(线程池, 申请字节数);

3. 释放空间

mempool_free(线程池, 指针ptr);
