#ifndef READ_WRITE_SHM_QUEUE_H
#define READ_WRITE_SHM_QUEUE_H

#include <stdlib.h>
#include <stdint.h>

struct RWQueue
{
  long long head_pos;
  long long tail_pos;
  uint32_t size;
  uint32_t length;
  char buffer[0];
};

struct NodeHead
{
  char type;
  uint32_t size;
};

class ReadWriteShmQueue
{
  public:
    ReadWriteShmQueue();
    ~ReadWriteShmQueue();
    int RWShmQueueOpen(int key, uint32_t size);
    int RWShmQueuePut(void *data, uint32_t length);
    int RWShmQueueGet(void *buffer, uint32_t *length);
  private:
    struct RWQueue queue;
};

#endif

