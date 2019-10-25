#ifndef YM_STATIC_QUEUE_H
#define YM_STATIC_QUEUE_H
#include "YMCtypes.h"

#define YMSQueue(name) (name)

#define YMSQueueDef(type, queue, size) \
struct queue##_st\
{ \
YmUint16_t front, back, count; \
type items[size]; \
}YMSQueue(queue)

#define YMSQueuePush(queue, item) \
do{ \
if(YMSQueueHasSpace(queue)) \
{ \
YMSQueue(queue).items[YMSQueue(queue).back] = item; \
YMSQueue(queue).back = (YMSQueue(queue).back + 1) % sizeof(YMSQueue(queue).items); \
YMSQueue(queue).count++; \
} \
}while(0)

#define YMSQueueHasSpace(queue) (YMSQueue(queue).count < sizeof(YMSQueue(queue).items))

#define YMSQueueIsEmpty(queue) (YMSQueue(queue).count == 0)

#define YMSQueueCount(queue) YMSQueue(queue).count

#define YMSQueueFront(queue) YMSQueue(queue).items[YMSQueue(queue).front]

#define YMSQueuePop(queue) \
do{ \
if(!YMSQueueIsEmpty(queue)) \
{ \
    YMSQueue(queue).front = (YMSQueue(queue).front + 1) % sizeof(YMSQueue(queue).items); \
    YMSQueue(queue).count--; \
} \
}while(0)


#endif // YM_STATIC_QUEUE_H
