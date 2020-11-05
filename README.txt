********************************************************
Operating system scheduling algorithm simulator.
********************************************************

********************************************************
COMPILATION/RUNNING INSTRUCTIONS:
********************************************************
- To compile and the program, run the following command within the project directory:
	$ make
	$ ./simulator [optional args] simulation_input.txt

Optional arguments are:
  -v, --verbose
    Output information about every state-changing event and scheduling decision.
  -t, --per_thread
    Output additional per-thread statistics for arrival time, service time, etc.
  -a, --algorithm
    The scheduling algorithm to use. One of FCFs, RR, PRIORITY, or CUSTOM.

Final argument should be the input .txt file. This file should include process, thread, and burst data. 

Formatting of simulation_input.txt is bellow, all data are assumed to be positive integers.

**************
process_id  process_type  num_threads

thread_0_arrival_time  num_CPU_bursts
cpu_time  io_time
cpu_time  io_time
... // repeat for the number of CPU bursts cpu_time
... // the last CPU burst cannot have I/O
cpu_time

thread_1_arrival_time num_CPU_bursts
cpu_time io_time
cpu_time io_time
... // repeat for the number of CPU bursts cpu_time 
... // the last CPU burst cannot have I/O

... // repeat for the number of threads

... // repeat for the number of processes
**************

Notes:
Process IDs are assumed to be unique.
Process type is 0, 1, 2, or 3 corresponding to:
	0: ​SYSTEM​ (highest priority)
	1: ​INTERACTIVE
	2: ​NORMAL
	3: ​BATCH​ (lowest priority)


********************************************************
IMPLIMENTATION DETAILS:
********************************************************
Algorithms available are:
	- FCFS (default): non-preemptive first come first serve scheduling. Ready queue is a simple FIFO queue. \
	- RR: Simple round-robin with time quantum of 3 units.
	- PRIORITY: Non-preemptive priority scheduling (see COMPILATION/RUNNING INSTRUCTIONS: NOTES: for priorities)
	- CUSTOM: Modified RR with dynamically calculated time quantum and priority scheduling (see below for details)


CUSTOM algorithm:

	The custom algorithm I designed for this project is a modified round-robin strategy. The goal of the algorithm is primarily to reduce overall turnaround time and dispatch overhead while preserving much of the fairness and flexibility of round-robin scheduling. Consider a case where the time quantum for a round-robin algorithm is 3 and there are many processes with threads that have cpu burst lengths of 4 or 5. The result is many interrupts where the remaining burst time for the interrupted thread is only 1 or 2 time units, potentially significantly smaller than the dispatch overhead. This is wasteful, we have to preempt, dispatch, and then later dispatch back to finish that last small piece of cpu burst. It would generally be better to simply allow these "almost complete" bursts to finish, but preserve round-robin style interrupts to ensure fairness with longer bursts.
	One potential solution to this problem is to implement round-robin with a time quantum that is roughly in the middle of the distribution of remaining CPU burst times for ready threads. That way, shorter bursts will be allowed to finish completely, while longer bursts will be preempted. Instead of empirically determining this "middle" quantum, my CUSTOM algorithm dynamically adjusts the quantum based on threads in the ready queue to be equal to the average remaining burst time of all ready threads. This is accomplished by keeping a count of ready threads and a total remaining burst time for ready threads that is updated whenever threads are added or dispatched.
	As threads are added to the "ready queue" they are actually added to either a short or long queue based on whether their remaining burst time is shorter than the current quantum (less than average) or longer. Short bursts are preferentially selected over long burst to prioritize completing threads (turnaround time) at the cost of response time (some long cpu burst threads end up waiting if there are many short cpu burst threads queued). The actual implementation involves 8 queues: a short queue for each of the 4 priorities and a long queue for each of the 4 priorities. Burst time is considered first, then priority (i.e. a short interactive burst will be dispatched before a long system burst).

Issues:
	The main problem with this algorithm is that because short cpu burst threads are prioritized over those with long cpu bursts regardless of priority, cpu heavy threads can end up waiting a very long time especially if there are many short burst threads becoming ready frequently. This is similar to the issues that arise in a priority scheduler, where frequent high priority processes can cause lower priority processes to age indefinitely. This algorithm could potentially be improved adding an additional aging queue to hold processes that have aged beyond a certain max amount of time, forcing them to be selected to run for a quantum, effectively limiting the maximum possible response time.
	One other note is that due to the dynamic nature of the quantum in my algorithm, it is not known at the time of dispatch how long the current thread will run for. I delay that decision until the end of the dispatch time to make the dynamic quantum strategy as effective as possible. This means that, due to the nature of our verbose output, at the time of dispatch a thread may be aloted a different amount of time than the amount of time it is actually run for before interrupt.



















