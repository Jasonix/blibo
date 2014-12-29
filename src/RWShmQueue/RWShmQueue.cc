#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include "RWShmQueue.h"

const char ETX = -1;

static char *GetShm(int key, int size, int flag)
{
  int shm_id;
  if ( (shm_id = shmget(key, size, flag)) < 0)
  {
    return NULL;
  }
  char *shm;
  if ( (shm = (char *)shmat(shm_id, NULL, 0)) < 0)
  {
    return NULL;
  }

  return shm;
}

ReadWriteShmQueue::ReadWriteShmQueue()
{
}

ReadWriteShmQueue::~ReadWriteShmQueue()
{

}

int ReadWriteShmQueue::RWShmQueueOpen(int key, uint32_t size)
{
  char *buffer = GetShm(key, size + sizeof(struct RWQueue),IPC_CREAT|0600);
  if (buffer == NULL)
  {
    return -1;
  }

  queue.head_pos = 0;
  queue.tail_pos = 0;
  queue.size = 0;
  queue.length = size;

  return 0;
}

int ReadWriteShmQueue::RWShmQueuePut(void *data, uint32_t length)
{
  char *pbuffer, *pcur;
  volatile long long *phead_pos, *ptail_pos, thead_pos, ttail_pos;
  phead_pos = &queue.head_pos;
  ptail_pos = &queue.tail_pos;
  pbuffer = &queue.buffer[0];

  thead_pos = *phead_pos;
  ttail_pos = *ptail_pos;

  if (thead_pos >= queue.length || ttail_pos >= queue.length)
  {
    *phead_pos = *ptail_pos = 0;
    return -1;
  }

  uint32_t pkg_length = length + sizeof(NodeHead);
  if (ttail_pos >= thead_pos)
  {
    if (queue.length - ttail_pos == pkg_length && thead_pos == 0)
    {
      return -2;
    }
    if (queue.length - ttail_pos < pkg_length)
    {
      if (thead_pos <= pkg_length )
      {
        return -3;
      }
      *(pbuffer + ttail_pos) = ETX;
      ttail_pos = 0;
    }
  }
  else
  {
    if (thead_pos - ttail_pos <= pkg_length)
    {
      return -4;
    }
  }
  pcur = pbuffer + ttail_pos;

  struct NodeHead head;
  head.type = 1;
  head.size = pkg_length;
  memcpy(pcur, (char *)&head, sizeof(head));
  pcur += sizeof(head);
  memcpy(pcur, data, length);
  pcur += length;
  *ptail_pos = (ttail_pos + pkg_length) % queue.length;

  return 0;
}

int ReadWriteShmQueue::RWShmQueueGet(void *buffer, uint32_t *length)
{
  char *pbuffer;
  volatile long long *phead_pos, *ptail_pos, thead_pos, ttail_pos;

  phead_pos = &queue.head_pos;
  ptail_pos = &queue.tail_pos;

  thead_pos = *phead_pos;
  ttail_pos = *ptail_pos;
  pbuffer = &queue.buffer[0];

  if (ttail_pos == thead_pos)
  {
    return -2;
  }

  if (ttail_pos >= queue.length || thead_pos >= queue.length)
  {
    *phead_pos = *ptail_pos = 0;
    return -1;
  }

  if (thead_pos > ttail_pos && *(pbuffer+thead_pos) == ETX )
  {
    *phead_pos =  thead_pos = 0;
    if (phead_pos == ptail_pos)
    {
      return -2;
    }
  }

  uint32_t len = 0;
  if (ttail_pos > thead_pos)
  {
    len = ttail_pos - thead_pos;
  }
  else
  {
    len = queue.length - thead_pos;
  }

  if (len < sizeof(NodeHead))
  {
    return -3;
  }

  uint32_t pkg_length = ((NodeHead *)(pbuffer + thead_pos))->size;

  if (*length < pkg_length)
  {
    *phead_pos = ttail_pos;
    return -4;
  }

  pbuffer = pbuffer + thead_pos + sizeof(NodeHead);
  *length = pkg_length;
  memcpy(buffer, pbuffer, pkg_length);
  *phead_pos = (thead_pos + pkg_length) % queue.length;

  return 0;
}
