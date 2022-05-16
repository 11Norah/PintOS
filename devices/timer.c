

#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
  
/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

//solution to alarm
/* Waiting queue of timer_sleep */
struct list sleep_list;

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void) 
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
  list_init(&sleep_list);
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void) 
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1)) 
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (loops_per_tick | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}

//Compare the wake up time of two threads
bool wakeUp_CMP(struct list_elem *first, struct list_elem *second, void *aux)
{
  struct thread *fthread = list_entry (first, struct thread, elem);
  struct thread *sthread = list_entry (second, struct thread, elem);

  return fthread->wakeUp < sthread->wakeUp;

}
/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks) 
{
  

  /*int64_t start = timer_ticks ();
  ASSERT (intr_get_level () == INTR_ON);
  while (timer_elapsed (start) < ticks) 
    thread_yield ();*/
  /*
  int64_t start = timer_ticks ();
  struct thread* curthread;
	//enum intr_level curlevel;
  ASSERT (intr_get_level () == INTR_ON);
 //while (timer_elapsed (start) < ticks)
  
  //curlevel = intr_disable();
 
  curthread = thread_current();
//calculate wake up time for a thread
  curthread->wakeUp = start + ticks;
  //timer_interrupt(&sema);
   sema_down(&sema);
//when a thread is blocked, it will be put to the blocked queue based on its wakeup time
  list_insert_ordered (&sleep_list, &curthread->elem, wakeUp_CMP, NULL);
   
  thread_block();
  
  //intr_set_level(curlevel);
 // while(&s)
  //timer_interrupt(curlevel);
 
  
  //while (timer_elapsed (start) < ticks) 
   // thread_yield ();
    
*/
   //Avoiding no busy waiting by using list of sleepers
   struct thread* curthread;
	enum intr_level curlevel;
  int64_t start = timer_ticks ();
  //passing negative and zero alarm
   if(ticks <= 0) return; 
  ASSERT (intr_get_level () == INTR_ON);
  
   curlevel = intr_disable(); 
  curthread = thread_current();
//calculate wake up time for a thread
  curthread->wakeUp = timer_ticks() + ticks;
   
  //blocking threads until wake up time
 //                if(timer_elapsed(start)<ticks){
 //interupts disabled
  //when a thread is blocked, it will be put to the sleep list based on its wakeup time
  list_insert_ordered (&sleep_list, &curthread->elem, wakeUp_CMP, NULL);

  thread_block(); //block the current thread
 //         }
 intr_set_level(curlevel); //enable interrupt
}

/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void
timer_msleep (int64_t ms) 
{
  real_time_sleep (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void
timer_usleep (int64_t us) 
{
  real_time_sleep (us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void
timer_nsleep (int64_t ns) 
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.
   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void
timer_mdelay (int64_t ms) 
{
  real_time_delay (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.
   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void
timer_udelay (int64_t us) 
{
  real_time_delay (us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.
   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void
timer_ndelay (int64_t ns) 
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  
   enum intr_level old_state = intr_disable();
  ticks++;
   thread_tick();
  struct thread *thread_top;
  struct list_elem *element;
  bool yield=false;
    //if it's a if condition not a while loop, we cannot pass the alarm-simultaneous test, but fine for the others
   while (!list_empty(&sleep_list))  //check the blocked_list if it is not empty
    {
       
        thread_top = list_entry(list_front(&sleep_list), struct thread, elem);    //get the top elem of the blocked_list
        //check if it’s the right time to wake up the top thread in timer_interrupt() 
        if (thread_top->wakeUp > timer_ticks() )
            break;
        list_pop_front(&sleep_list);      //remove the front element
         //storing waken up threads in order according to priority
//       list_insert_ordered(&priority_sleep_list,&thread_top->elem,less_priority,0);
       thread_unblock(thread_top);
       
    }
  if(thread_mlfqs){ //in case advanced scedular  


  increment_cpu_by1();


  //every 1 sec recalculate load avg and recent cpu of all threads 
  if (ticks % TIMER_FREQ ==0){ //1 sec
    calculating_load_avg(); //recalculate load avg 
    recalculate_recent_cpu_for_all_threads();  //recalculate recent cpu of all threads
    recalculate_priority_for_all_threads();
  }

  else if(ticks%4==0){
    //recalculate priority of all threads  
    recalculate_priority_for_all_threads();
  }
  intr_set_level (old_state);
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops) 
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait (loops);

  /* If the tick count changed, we iterated too long. */
  barrier ();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.
   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops) 
{
  while (loops-- > 0)
    barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom) 
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.
          
        (NUM / DENOM) s          
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks. 
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         timer_sleep() because it will yield the CPU to other
         processes. */                
      timer_sleep (ticks); 
    }
  else 
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      real_time_delay (num, denom); 
    }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000)); 
}

    
