# First day of development (Apr 5-6, approx. 13h wasted)
![image](https://github.com/user-attachments/assets/cf93d959-5794-4de7-8796-6ce92a4f7558)

Spent an all-nighter just to make a simple blue-green screen.

I tried `ramfb` and `VGA`, but it rarely worked towards my intention.

Grinding my time into `virtio-gpu-device` made something worth my time.

I must notify my teammates about this discovery.

# Second day of development (Apr 7, approx. 3h wasted)
![image](https://github.com/user-attachments/assets/7dec82be-1297-4127-89ca-b91220e561e2)

Nascity: Hello world on the screen.

I assume this as a W.

Made some syscalls: `draw_fill` and `draw_bits`.

Latter is not implemented yet.

Thinking of implementing it some time later.

Writing this while spending an all-nighter (Apr 7-8).

I never thought that I'll be reuniting with producer-consumer problem other than in OS class.

# Third day of dev (Apr 8, approx. 3h wasted + a)
![image](https://github.com/user-attachments/assets/e4eeca21-8247-4649-afe4-3099ed3763d4)

Like I said before, I met producer-consumer problem during implementing message queue.

Fortunately, there's no signaling between the consumer and the producer.

The following is the implementation so far:

## Message queue
```c
// return 0 when timout, 1 when the buffer is empty                                                                            
// p->lock has to be release later!!
int wait_timeout(struct proc *p, int timeout, int (*cond)(struct proc*))                                                       
{ 
        uint64 start;                                                                                                          
  
        acquire(&ticks_lock);                                                                                                  
        start = ticks;                                                                                                         
        release(&ticks_lock);

        acquire(&p->lock);
        while (!cond(p))
        {
                if (timeout == INFINITE)
                        continue;

                acquire(&ticks_lock);
                if (ticks - start >= timeout * TPS)
                {
                        release(&ticks_lock);
                        release(&p->lock);
                        return 0;
                }
                release(&ticks_lock);

        }

        return 1;
}
```

`send_msg` and `recv_msg` function calls this function before doing something.

It locks the process lock and starts checking for condition and ticks. If the condition is met or timeout, then the function returns while still locking the process lock (`p->lock`). The lock needs to be release by the caller. This has a problem that if timeout is `INFINITE`, the calling process is put in an infinite busy-waiting state. I want to call this *intentional*, but I'm afraid of what will happen if this is not treated properly.

```c
int send_msg(int target_pid, uint64 user_buf, int size)
{
        struct proc *p = ; // not sure yet
                           // gotta get the target processes struct proc*

        // allocate new queue when empty
        if (!p->msg_queue)
        {
                p->msg_queue = (uint64)kalloc();
                if (!p->msg_queue)
                        panic("send_msg - not enough memory");                                                                 
        }                                                                                                                      
        if (!wait_timeout(p, INFINITE, buffer_can_be_written))                                                                 
                panic("send_msg - this shouldn't happen"); // if the infinite loop somehow breaks                              
                                                                                                                               
        // do smth                                                            

        return MSG_Q_OK;                                                                                                       
}                                                                                                                              
  
// timeout is in seconds       
int recv_msg(uint64 user_buf, int size, int timeout)
{ 
        struct proc *p = myproc();
  
        if (!wait_timeout(p, timeout, buffer_can_be_read))                                                                     
        {
                release(p->lock);
                return MSG_Q_TIMEOUT;                                                                                          
        }

        // do smth
  
        release(&p->lock);     
        return MSG_Q_OK;
}
```

This is the prototype of `send_msg` and `recv_msg`. I'm scared of race conditions. It'll surely haunt me in my dreams.

# Fourth day of dev (Apr 9, approx. 3h wasted debugging)
I changed my machine to a Dell laptop, and the project suddenly halted.

Seems that setting some control registers make the kernel halt.

I think I should work on this error.
