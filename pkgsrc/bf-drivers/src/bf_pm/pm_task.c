/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include "pm_task.h"
#include "pm_log.h"

bf_sys_cmp_and_swp_t run_q_lock[MAX_PRI];
typedef struct tcb_t {
  struct tcb_t *next;
  void *context;
  tasklet_fn fn;
  tasklet_pri_t priority;
  tasklet_state_t state;
  struct timespec next_run_time;
} tcb_t;

// fwd ref
static void requeue_2_run_q(tcb_t *tcb);

tcb_t empty_tcb[1024] = {{0}};

tcb_t *free_q = NULL;
tcb_t *run_q[MAX_PRI] = {NULL, NULL};

static void pm_task_runq_lock_init() {
  int i;

  for (i = 0; i < MAX_PRI; i++) {
    run_q_lock[i] = 0;
  }
}

static void pm_task_runq_init() {
  int i;

  for (i = 0; i < MAX_PRI; i++) {
    run_q[i] = NULL;
  }
}

static void pm_task_runq_lock_acquire(int prio) {
  // No dedicated lock for free_q. the operation on free_q is actually protected
  // by runq_lock. using 2 run_q lock will make the protection on free_q void.
  // Force the prio to HI_PRI so both run_q will share the same lock.
  prio = HI_PRI;

  do {
  } while (bf_sys_compare_and_swap(&run_q_lock[prio], 0, 1) == 0);
}

static void pm_task_runq_lock_release(int prio) {
  // No dedicated lock for free_q. the operation on free_q is actually protected
  // by runq_lock. using 2 run_q lock will make the protection on free_q void.
  // Force the prio to HI_PRI so both run_q will share the same lock.
  prio = HI_PRI;

  if (bf_sys_compare_and_swap(&run_q_lock[prio], 1, 0)) {
    return;
  }
  bf_sys_assert(0);
}

static void tasklet_init_free_q(void) {
  uint32_t t;

  bf_sys_assert((free_q == NULL));

  for (t = 0; t < sizeof(empty_tcb) / sizeof(empty_tcb[0]); t++) {
    empty_tcb[t].next = free_q;
    free_q = &empty_tcb[t];
  }
}

static void enqueue_2_free_q(tcb_t *tcb) {
  tcb->state = STATE_DEFAULT;
  tcb->next = free_q;
  free_q = tcb;
}

void pm_tasklet_new(tasklet_fn fn, void *context, tasklet_pri_t priority) {
  tcb_t *tcb = NULL;

  bf_sys_assert((free_q != NULL));

  pm_task_runq_lock_acquire(priority);
  // un-link from free_q
  tcb = free_q;
  free_q = tcb->next;

  tcb->fn = fn;
  tcb->context = context;
  tcb->priority = priority;
  tcb->state = STATE_DEFAULT;
  tcb->next = NULL;

  // set to run immediately
  clock_gettime(CLOCK_MONOTONIC, &tcb->next_run_time);

  // link onto run_q
  requeue_2_run_q(tcb);
  pm_task_runq_lock_release(priority);
}
/* This function is always used from the context of handler execution.
   Before invoking the handler, this function can be used to ensure the tcb
   under processed is valid and it is not marked for deletion. Return value of
   false means tcb is already removed so handler execution will be terminated
   Return value of true means tcb is valid i.e. proceed with the fsm handler
   execution */
bool pm_is_current_tasklet_valid(void *context) {
  tcb_t *tcb = NULL;
  int priority;
  // Iterate over all the head of runqueue and verify if the current tcb is
  // marked as remove by other thread
  for (priority = HI_PRI; priority >= LO_PRI; priority--) {
    pm_task_runq_lock_acquire(priority);
    if (run_q[priority] != NULL) {
      /* tcb for which handler is currently running is always remains on top of
         linked list as we mark the tv_sec and tv_nsec to 0 before invoking
         handler in function pm_tasklet_run */
      tcb = run_q[priority];
      if ((uintptr_t)context == (uintptr_t)(tcb->context)) {
        /* We have to verify head of all link list
           so we are not adding else statement here */
        if (tcb->state == STATE_REMOVE) {
          pm_task_runq_lock_release(priority);
          return false;
        }
      }
    }
    pm_task_runq_lock_release(priority);
  }
  return true;
}

void pm_tasklet_rmv(void *context) {
  tcb_t *tcb, *tcb_next, *tcb_prev = NULL;
  int priority;

  // Iterate over all the tasklets in the runqueue and remove the tasklet
  // corresponding to the context passed in
  for (priority = HI_PRI; priority >= LO_PRI; priority--) {
    pm_task_runq_lock_acquire(priority);
    if (run_q[priority] != NULL) {
      tcb = run_q[priority];
      tcb_prev = NULL;
      while (tcb) {
        tcb_next = tcb->next;
        if ((uintptr_t)context == (uintptr_t)(tcb->context)) {
          // delink from the run_q
          if ((tcb->state == STATE_RUNNING) || (tcb->state == STATE_REMOVE)) {
            if (tcb->state == STATE_REMOVE) {
              PM_TRACE("%s:%d TCB is already in remove state !!!!",
                       __func__,
                       __LINE__);
            }
            tcb->state = STATE_REMOVE;
            pm_task_runq_lock_release(priority);
            return;
          }
          if (run_q[priority] == tcb) {
            // First tcb in the run queue is getting dequeued
            run_q[priority] = tcb_next;
          } else {
            if (tcb_prev) tcb_prev->next = tcb_next;
          }
          enqueue_2_free_q(tcb);
          pm_task_runq_lock_release(priority);
          return;
        } else {
          tcb_prev = tcb;
        }
        tcb = tcb_next;
      }
    }
    pm_task_runq_lock_release(priority);
  }
  return;
}

uint32_t tasklet_run(tcb_t *tcb) {
  uint32_t delay_time_us;
  /* run tasklet */
  delay_time_us = tcb->fn(tcb->context);

  return delay_time_us;
}

static int t1_less_than_or_eq_t2(struct timespec *t1, struct timespec *t2) {
  if (t1->tv_sec < t2->tv_sec) return 1;
  if ((t1->tv_sec == t2->tv_sec) && (t1->tv_nsec <= t2->tv_nsec)) return 1;
  return 0;
}

static int tasklet_ready(tcb_t *tcb, struct timespec *ts) {
  if (t1_less_than_or_eq_t2(&tcb->next_run_time, ts)) {
    return 1;
  }
  return 0;
}

static void requeue_2_run_q(tcb_t *tcb) {
  tcb_t *prev = NULL, *cur;

  cur = run_q[tcb->priority];
  if (cur == NULL) {
    run_q[tcb->priority] = tcb;
    tcb->next = NULL;
    return;
  }
  while (cur &&
         t1_less_than_or_eq_t2(&cur->next_run_time, &tcb->next_run_time)) {
    prev = cur;
    cur = cur->next;
  }
  if (prev == NULL) {  // insert before first elt on run_q
    tcb->next = cur;
    run_q[tcb->priority] = tcb;
  } else {
    tcb->next = prev->next;
    prev->next = tcb;
  }
}

void tasklet_scheduler(void) {
  tcb_t *tcb;
  int ran_something = 0;
  int priority;
  struct timespec now;
  struct timespec now2;
  uint32_t delay_time_us = 0;

  clock_gettime(CLOCK_MONOTONIC, &now);
  /* run any hi-pri tasks first */
  for (priority = HI_PRI; priority >= LO_PRI; priority--) {
    if (run_q[priority] != NULL) {
      pm_task_runq_lock_acquire(priority);
      tcb = run_q[priority];
      while (pm_tasklet_free_run_get() && tcb && (tasklet_ready(tcb, &now))) {
        if (!ran_something) {
          ran_something = 1;
        }
        /* Do not remove fromm run_q, pm_tasklet_rmv may try to remove
           the TCN while running tasklet_run below */
        /* Set state of tcb to STATE_RUNNING to indicate other thread
           that current tcb is under process */
        // run_q[priority] = tcb->next;
        tcb->state = STATE_RUNNING;
        /* Update the time to zero, so that it remains at the top while
         * tasklet_run is running */
        /* Even when another thread tries to add a new tcb to run_q, current tcb
         * will be at the head of list */
        tcb->next_run_time.tv_sec = 0;
        tcb->next_run_time.tv_nsec = 0;

        pm_task_runq_lock_release(priority);

        delay_time_us = tasklet_run(tcb);

        pm_task_runq_lock_acquire(priority);

        if (tcb->state == STATE_REMOVE) {
          /* Remove the entry from run_q and put it into free_q
             We have to remove the tcb here instead of waiting for the remove
             thread. It may lead to a scenario where the tcb will get scheduled
             once again before remove thread gets a chance to remove it */
          tcb->state = STATE_DEFAULT;
          run_q[priority] = tcb->next;
          enqueue_2_free_q(tcb);
        } else {
          tcb->state = STATE_DEFAULT;
          if (delay_time_us == TASK_DONE) {
            tcb->next_run_time.tv_sec = 0;
            tcb->next_run_time.tv_nsec = 0;
          } else {
            // gettimeofday(&now, NULL);
            clock_gettime(CLOCK_MONOTONIC, &now2);
            tcb->next_run_time.tv_sec = now2.tv_sec + delay_time_us / 1000000;
            tcb->next_run_time.tv_nsec =
                now2.tv_nsec + ((delay_time_us * 1000) % 1000000000);
            if (tcb->next_run_time.tv_nsec >= 1000000000) {
              tcb->next_run_time.tv_sec++;
              tcb->next_run_time.tv_nsec -= 1000000000;
            }
          }
          /* This will be put to freeq as the tcb is added for
           * delete in pm_tasklet_rmv*/
          /* if not done, return to run_q */
          /* First remove the head of the list (i.e. current tcb)
             Then requeue/free current tcb */
          run_q[priority] = tcb->next;
          if ((tcb->next_run_time.tv_sec != 0) ||
              (tcb->next_run_time.tv_nsec != 0)) {
            requeue_2_run_q(tcb);
          } else {  // if done, release to free_q
            enqueue_2_free_q(tcb);
          }
        }
        tcb = run_q[priority];
      }
      pm_task_runq_lock_release(priority);
    }
  }
}

// FSM debug facility
bool fsm_free_run = true;
bool fsm_single_step = false;

void pm_tasklet_free_run_set(bool st) { fsm_free_run = st; }
bool pm_tasklet_free_run_get(void) { return fsm_free_run; }

void pm_tasklet_single_step_set(void) { fsm_single_step = true; }

/* pm_fsm_queues_init
 *
 * Initialize all the queues and the locks used by the FSM to initial values
 */
void pm_fsm_queues_init() {
  PM_TRACE("%s:%d Initializing all FSM queues", __func__, __LINE__);
  free_q = NULL;
  memset(empty_tcb, 0, sizeof(empty_tcb));
  tasklet_init_free_q();
  pm_task_runq_init();
  pm_task_runq_lock_init();
  PM_TRACE("%s:%d Initializing all FSM queues done", __func__, __LINE__);
}

/* pm_tasklet_scheduler
 *
 * Called from the timer wheel
 */
void pm_tasklet_scheduler(void) {
  if (fsm_free_run || fsm_single_step) {
    tasklet_scheduler();
  }
  // if in single-step mode reset "step" flag
  fsm_single_step = false;
}

typedef struct tasklet_test_ctx_t {
  int loops;
  int delay;
  char id;
} tasklet_test_ctx_t;

tasklet_test_ctx_t tctx[26] = {{0}};

uint32_t tasklet_test(void *context) {
  tasklet_test_ctx_t *ctx = (tasklet_test_ctx_t *)context;

  if (--ctx->loops == 0) {
    return TASK_DONE;
  }
  return ctx->delay;
}

void tasklet_unit_test(void) {
  int i;

  for (i = 0; i < 26; i++) {
    tctx[i].loops = 2 * (i + 1);
    tctx[i].delay = 100 * (i + 1);
    tctx[i].id = 'a' + i;
    pm_tasklet_new(tasklet_test, &tctx[i], HI_PRI);
  }
}
