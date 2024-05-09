/* Standard Library Includes */
#include <stdio.h>
#include <stdlib.h>
/* Linux API Library Includes */
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
/* StrawHat Project Includes */
#include "vm.h"
#include "vm_cs.h"
#include "vm_support.h"
#include "vm_process.h"
#include "vm_printing.h"
/* Otur Scheduler Library Includes */
#include "otur_sched.h"

/* Global Constants */
enum cs_states { CS_STOP = 0, CS_RUN };

/* Mutex Control Variables */
pthread_mutex_t cs_cv_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cs_run_m = PTHREAD_MUTEX_INITIALIZER;
pthread_t pt_cs; // Main CS thread variable (controlled from atexit function)

/* Local Global Variables (these are all private to this source file) */
static Otur_process_s *on_cpu = NULL;
static Otur_schedule_s *schedule = NULL;
static int cs_do_cs = CS_RUN; // Controls the lifetime CS Thread
static int cs_run = CS_STOP; // Controls the running of the CS Thread (initialized to STOP)
static useconds_t sleep_usec_time = SLEEP_USEC;
static useconds_t between_usec_time = BETWEEN_USEC;

/* Run at VM startup to initialize Context Switching (CS) thread */
void initialize_cs_system() {
  // Start the CS Thread Locked by...
  // 1) Acquiring the lock here on initialization
  // 2) Each iteration of the CS thread starts by acquiring the lock.
  // The Shell commands release/acquire the lock to control CS
  pthread_mutex_lock(&cs_cv_m);

  // Create the runner thread for the CS system
  int ret = pthread_create(&pt_cs, NULL, &cs_thread, NULL);
  if(ret != 0) {
    ABORT_ERROR("Could not create a Thread for the CS System.");
  }

  // Initialize the Scheduler System (this is designed as a part of CS)
  schedule = otur_initialize();
  if(schedule == NULL) {
    ABORT_ERROR("Error reported by otur_initialize.");
  }
}

/* Free all CS related memory.  Registered with atexit */
void cs_cleanup() {
  PRINT_STATUS("... Beginning CS Shutdown");

  PRINT_STATUS("... Deallocating Scheduler with otur_cleanup(schedule)");
  otur_cleanup(schedule);

  PRINT_STATUS("... Shutting Down CS System and Dispatcher");
  cs_do_cs = CS_STOP; // Tell the thread to die.
  pthread_mutex_unlock(&cs_cv_m); // If the CS is not running, activate it so it can die.

  PRINT_STATUS("... Waiting for CS System and Dispatcher to Complete");
  pthread_join(pt_cs, NULL);

  PRINT_STATUS("... Removing Process from CPU");
  free(on_cpu);
  on_cpu = NULL; // Nothing on CPU.

  PRINT_STATUS("... CS Shutdown Complete");
}

/* Context Switching Thread Function */
void *cs_thread(void *args) {
  int iteration = 1;
  pid_t last_run_cpu = -1;

// 1) While not blocked... (lock cs_cv_m to block)
// .. a) Gets the next process to run from the Scheduler (select)
// .. .. Holds this in the on_cpu global
// .. b) Resumes the selected process
// .. c) Sleeps for sleep_usec_time microseconds
// .. d) Suspends the selected process
// .. e) Returns the process to the Scheduler (insert)
  while(cs_do_cs == CS_RUN) {
    long delay = sleep_usec_time;
    pthread_mutex_lock(&cs_cv_m);  // mylock.acquire()  -- Turnstile Pattern
    pthread_mutex_unlock(&cs_cv_m);// mylock.release()

    // Check to see if the system is being shutdown while waiting on lock.
    if(cs_do_cs == CS_STOP) {
      continue; 
    }

    PRINT_DEBUG("Context Switch: Iteration %d", iteration++);

    // Call the Scheduler to get the next Process
    on_cpu = otur_select(schedule);
    if(on_cpu) {
      PRINT_DEBUG("Schedule Select Returned PID %d", on_cpu->pid);
    }
    else {
      PRINT_DEBUG("Schedule Select Returned No Ready Processes");
    }

    // Only Dispatch if something was selected
    if(on_cpu != NULL) {
      if(process_find(on_cpu->pid) == 0) {
        if(otur_exited(schedule, on_cpu, 42) == -1) {
          ABORT_ERROR("Error reported by otur_exited.");
        }
        on_cpu = NULL;
        last_run_cpu = 0; // Nothing on the CPU for this iteration
      }
      else {
        if(last_run_cpu != on_cpu->pid) {
          PRINT_STATUS("Switching to run PID: %d (%s)", on_cpu->pid, on_cpu->cmd);
        }
        last_run_cpu = on_cpu->pid;
        kill(on_cpu->pid, SIGCONT);
        usleep(delay);
        // It's run for the quantum, suspend it and return it to the queue.
        if(on_cpu) {
          kill(on_cpu->pid, SIGTSTP);
          if(otur_enqueue(schedule, on_cpu) == -1) {
            ABORT_ERROR("Error reported by otur_enqueue.");
          }
          on_cpu = NULL;
        }
      }
    }
    // Nothing selected, IDLE CPU
    else {
      PRINT_DEBUG("Schedule Select Returned Nothing");
      if(last_run_cpu != 0) {
        print_empty_cs();
      }
      last_run_cpu = 0; // Nothing on the CPU for this iteration
      usleep(delay);
    }
#if DO_MLFQ
    // Promote the Processes
    if(otur_promote(schedule) == -1) {
      ABORT_ERROR("Error reported by otur_promote.");
    }

#endif
    // Delay after the run quantum, but before we pick a new one (to help with debugging)
    usleep(between_usec_time);
  }
  // CS System has ended the main loop, we can now properly exit the thread.
  pthread_exit(0);
}

/* Direct the Scheduler to suspend a process from execution */
/*
void cs_suspend(pid_t pid) {
  int last_state = -1;

  // There IS a race condition here, but it's a pretty minor one.  
  // If it WAS running when we shut it off, restart it when we finish.
  // It's possible they shut it off manually in the microseconds between these,
  // but it's not exactly the end of the world if they had.
  pthread_mutex_lock(&cs_run_m);
  last_state = cs_run;
  pthread_mutex_unlock(&cs_run_m);

  stop_cs(); // Critical!  This ensures that all processes have been returned to Scheduler first

  PRINT_DEBUG("Suspending Process Now");
  if(otur_suspend(schedule, pid) == -1) {
    ABORT_ERROR("Error reported by otur_suspend.");
  }
  if(last_state == CS_RUN) {
    start_cs();
  }
}
*/
/*Direct the Scheduler to resume a process from execution */
/*
void cs_resume(pid_t pid) {
  int last_state = -1; // This is an invalid state that will be overwritten

  // There IS a race condition here, but it's a pretty minor one.  
  // If it WAS running when we shut it off, restart it when we finish.
  // It's possible they shut it off manually in the microseconds between these,
  // but it's not exactly the end of the world if they had.
  pthread_mutex_lock(&cs_run_m);
  last_state = cs_run;
  pthread_mutex_unlock(&cs_run_m);

  stop_cs(); // Critical!  This ensures that all processes have been returned to Scheduler first

  PRINT_DEBUG("Resuming Process Now");
  if(otur_resume(schedule, pid) == -1) {
    ABORT_ERROR("Error reported by otur_resume.");
  }
  if(last_state == CS_RUN) {
    start_cs();
  }
}
*/
/* Direct the Scheduler to reap a defunct processes */
void cs_reap(pid_t pid) {
  int last_state = -1;

  // There IS a race condition here, but it's a pretty minor one.  
  // If it WAS running when we shut it off, restart it when we finish.
  // It's possible they shut it off manually in the microseconds between these,
  // but it's not exactly the end of the world if they had.
  pthread_mutex_lock(&cs_run_m);
  last_state = cs_run;
  pthread_mutex_unlock(&cs_run_m);

  stop_cs(); // Critical!  This ensures that all processes have been returned to Scheduler first

  PRINT_DEBUG("Reaping Process Now");
  int ec = otur_reap(schedule, pid);
  if(ec == -1) {
    PRINT_WARNING("[No Such Process to Reap]");
  }
  else {
    PRINT_STATUS("Process Reaped.  Exit Code was %d", ec);
  }
  if(last_state == CS_RUN) {
    start_cs();
  }
}

/* Return the process that was on the CPU back to the Scheduler during Termination
 * -  If process was NOT on CPU during termination, then it is handled in another function.
 */
void cs_exiting_process(int exit_code) {
  // If a process *was* on the CPU, run the exit handler.
  if(on_cpu) {
    if(otur_exited(schedule, on_cpu, exit_code) == -1) {
      ABORT_ERROR("Error reported by otur_exited.");
    }
    PRINT_DEBUG("Exiting PID %d, with exit code %d with otur_exited\n", on_cpu->pid, exit_code);
    on_cpu = NULL;
  }
  else {
    PRINT_WARNING("Tried to exit a non-existing process on the CPU");
  }
}

/* Add a newly created process to the schedule system */
void cs_otur_process(Process_data_s *proc) {
  // Create the new Process with the given parameters (from the Shell)
  Otur_process_s *proc_node = otur_invoke(proc->pid, proc->is_high, proc->is_critical, proc->input_orig);
  if(proc_node == NULL) {
    ABORT_ERROR("Error reported by otur_invoke.");
  }
  // Then Insert it into the Queue
  if(otur_enqueue(schedule, proc_node) == -1) {
    ABORT_ERROR("Error reported by otur_enqueue.");
  }
  PRINT_STATUS("Process %s created with PID %d", proc->input_orig, proc->pid);
  // Finally, print the schedule out (Debug Mode Only) to see it there.
  print_otur_debug(schedule, get_on_cpu());
}

/* Directs Scheduler that a process had terminated with the given exit code. */
void cs_otur_terminated(pid_t pid, int exit_code) {
  int last_state = -1; // Illegal state that will be set in this function

  // There IS a race condition here, but it's a pretty minor one.  
  // If it WAS running when we shut it off, restart it when we finish.
  // It's possible they shut it off manually in the microseconds between these,
  // but it's not exactly the end of the world if they had.
  pthread_mutex_lock(&cs_run_m);
  last_state = cs_run;
  pthread_mutex_unlock(&cs_run_m);

  stop_cs(); // Critical! This ensures the state is consistent first.

  // Check if the terminted process is on the cpu.  If so, treat it as an exiting process.
  if(on_cpu && on_cpu->pid == pid) {
    // Exit from the CPU directly (terminated while being run)
    cs_exiting_process(exit_code);
  }
  // Otherwise, it was terminated while in a Queue; treat as a terminated process.
  else {
    // Exit from the Ready or Suspended Queues (terminated by command)
    int status = otur_killed(schedule, pid, exit_code);
    if(status == -1) {
      ABORT_ERROR("Error reported by otur_killed.");
    }

    PRINT_DEBUG("Terminating PID %d with exit code %d with otur_killed\n", pid, exit_code);
  }

  // Finally, if the CS system had been running before this interruption, resume it.
  if(last_state == CS_RUN) {
    start_cs();
  }
} 

/* Starts the CS Processing System */
void start_cs() {
  pthread_mutex_lock(&cs_run_m);
  if(cs_run == CS_STOP) {
    cs_run = CS_RUN;
    pthread_mutex_unlock(&cs_cv_m);
  }
  pthread_mutex_unlock(&cs_run_m);
}

/* Stops the CS Processing System */
void stop_cs() {
  pthread_mutex_lock(&cs_run_m);
  if(cs_run == CS_RUN) {
    cs_run = CS_STOP;
    pthread_mutex_lock(&cs_cv_m);
  }
  pthread_mutex_unlock(&cs_run_m);
}

/* Helper to print messages on USER toggling of the CS system */
void handle_ctrlc() {
  pthread_mutex_lock(&cs_run_m);
  int is_running = cs_run;
  pthread_mutex_unlock(&cs_run_m);

  if(is_running == CS_RUN) {
    print_stop_cs();
  }
  else {
    print_start_cs();
  }

  toggle_cs();
}


/* Toggles the CS Processing System */
void toggle_cs() {
  pthread_mutex_lock(&cs_run_m);
  int is_running = cs_run;
  pthread_mutex_unlock(&cs_run_m);

  if(is_running == CS_RUN) { // Run -> Stop
    stop_cs();
  }
  else {  // Stop -> Run
    start_cs();
  }
}

/* Helper to print status when a USER starts the CS system. */
void print_start_cs() {
  PRINT_STATUS("Starting CS System: %d usec Run, %d usec Between", sleep_usec_time, between_usec_time);
}

/* Helper to print status when a USER stops the CS system. */
void print_stop_cs() {
  PRINT_STATUS("Stopping the CS System");
}

/* Helper to print status when select returns NULL */
void print_empty_cs() {
  PRINT_STATUS("No Processes Ready to Run");
}


/* Prints the state of the CS System */
void print_cs_status() {
  pthread_mutex_lock(&cs_run_m);
  int state = cs_run;
  pthread_mutex_unlock(&cs_run_m);

  if(state == CS_RUN) {
    PRINT_STATUS("CS System Running: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
  }
  else {
    PRINT_STATUS("CS System Stopped: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
  }
  return;
}

/* Set the time for each process to run for (Quantum) */
void set_run_usec(useconds_t time) {
  sleep_usec_time = time;
  PRINT_STATUS("Setting CS System: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
}

/* Get the time for the run (Quantum) */
useconds_t get_run_usec() {
  return sleep_usec_time;
}

/* Set the time between processes running */
void set_between_usec(useconds_t time) {
  between_usec_time = time;
  PRINT_STATUS("Setting CS System: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
}

/* Get the time between processes running */
useconds_t get_between_usec() {
  return between_usec_time;
}

/* Accessor for the process currently on the CPU */
Otur_process_s *get_on_cpu() {
  return on_cpu;
}

/* Accessor for the local schedule copy */
Otur_schedule_s *get_schedule() {
  return schedule;
}
