/**
 * Operating Systems
 * Project 2 - OS Scheduling Simulator
 * Alec De Vivo
 * 
 * simulation.cpp
 * Implimentation of simulation. Includes main event loop and queue, event
 * handlers for all types of events, and data structures for ready queues.
 */


#include <vector>
#include <queue>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <memory>
#include "process_structs.h"
#include "simulation.h"

using std::cout; 
using std::shared_ptr;

CustomReadyQueue::CustomReadyQueue()
  : short_queues(4), long_queues(4), dynamic_quantom(-1), 
    num_threads(0), total_remaining_time(0), avg_age(-1)
{}

shared_ptr<Thread> CustomReadyQueue::fetch_thread()
{
   /**
   * Get thread from top of set of ready queues. Checks short queues and then
   * long queues, both in order of priority, and returns top thread.
   * 
   * Returns pointer to next thread to be run, pops that thread from ready queue.
   */
  shared_ptr<Thread> next_thread = nullptr;
  for(auto it = short_queues.begin(); it!=short_queues.end(); it++)
  {
    // First check short queues in priority order
    if(not it->empty())
    {
      next_thread = it->front();
      it->pop();
      break;
    }
  }
  if (next_thread == nullptr)
  {
      for(auto it = long_queues.begin(); it!=long_queues.end(); it++)
    {
    // Then check long queues in priority order
      if(not it->empty())
      {
        next_thread = it->front();
        it->pop();
        break;
      }
    } 
  }
  // adjust metrics
  assert(next_thread != nullptr); // should only fetch thread when there is a thread to fetch
  num_threads--;
  int burst_remaining_time = next_thread->bursts[next_thread->burst_index]->cpu_time 
    - next_thread->current_burst_completed_time;
  total_remaining_time -= burst_remaining_time;
  assert(total_remaining_time==num_threads || num_threads != 0);
  if (num_threads != 0){
    // Update dynamic qunatom if ready queue is not empty
    int average_remaining_time = total_remaining_time/num_threads;
    dynamic_quantom = (average_remaining_time < QUANTOM_MAX) ? average_remaining_time : QUANTOM_MAX; ;
  }
  return next_thread; ;
}

void CustomReadyQueue::push_thread(shared_ptr<Thread> thread)
{
   /**
   * Add thread to ready queues. Determines based on the current dynamic quantom
   * whether to add the thread to short or long queue for a given priority. Threads
   * with remaining CPU burst times <= to the current quantum are added to the
   * short queues, longer burst times to the long queues.
   */
  int burst_remaining_time = thread->bursts[thread->burst_index]->cpu_time - thread->current_burst_completed_time;
  num_threads++;
  total_remaining_time += burst_remaining_time;
  int average_remaining_time = total_remaining_time / num_threads; // this should round down
  dynamic_quantom = (average_remaining_time < QUANTOM_MAX) ? average_remaining_time : QUANTOM_MAX; 
  (burst_remaining_time <= dynamic_quantom) ? 
    short_queues[thread->process->type].push(thread)
    : long_queues[thread->process->type].push(thread);
}

// Constructor for simulation
Simulation::Simulation(int proc_overhead, int thr_overhead) 
  : v_flag(false), t_flag(false), 
    total_elapsed_time(0), total_dispatch_time(0), total_io_time(0), 
    total_service_time(0), total_idle_time(0), process_type_data(4, std::vector<int>(3)),
    process_switch_overhead(proc_overhead), thread_switch_overhead(thr_overhead),
    running_thread(nullptr), quantom(3), algorithm(FCFS), priority_ready_queues(4),
    current_process_id(-1), custom_ready_queue(nullptr)
{}


void Simulation::add_process(std::shared_ptr<Process> process)
{
   /**
   * Add process to simulation, also adds thread arrival to event queue.
   */
  processes.push_back(process);
  for (int i=0; i<process->threads.size(); i++){
    Event event(process->threads[i]->arrival_time, Event::THREAD_ARRIVED);
    event.thread = process->threads[i];
    event_queue.push(event);
  }
}

bool CompareEventsByArrivalTime::operator()(Event const & e1, Event const & e2)
{
   /**
   * Comparator for event priority queue
   * Sorts by earliest time, then lowest Event::Type number, then highest thread id
   */
  if (e1.time == e2.time)
  {
    return ((e1.type == e2.type) ? e1.thread->id < e2.thread->id : e1.type > e2.type);
  }
  else return e1.time > e2.time;
}

bool CompareThreadsByArrivalTime::operator()(std::shared_ptr<Thread> const & t1, std::shared_ptr<Thread> const & t2)
{
  /**
   * Comparator for thread ready queue
   * Sorts by earliest arrival time
   */
  return t1->arrival_time > t2->arrival_time;
}

void Simulation::run_simulation()
{
  /**
   * Main event loop for simulation.
   */
  // Custom ready queue initialization for CUSTOM algorithm
  if (algorithm == CUSTOM) custom_ready_queue = std::make_shared<CustomReadyQueue>();
  // Main event loop
  while(event_queue.empty() == false)
  {
    Event next_event = event_queue.top();
    event_queue.pop();
    // Pass to different event handlers
    if (next_event.type == Event::THREAD_ARRIVED) handle_thread_arrival(next_event);
    if (next_event.type == Event::DISPATCHER_INVOKED) handle_dispatcher_invoked(next_event);
    if (next_event.type == Event::PROCESS_DISPATCH_COMPLETED) handle_dispatch_complete(next_event);
    if (next_event.type == Event::THREAD_DISPATCH_COMPLETED) handle_dispatch_complete(next_event);
    if (next_event.type == Event::CPU_BURST_COMPLETED) handle_cpu_burst_complete(next_event);
    if (next_event.type == Event::IO_BURST_COMPLETED) handle_io_burst_complete(next_event);
    if (next_event.type == Event::THREAD_COMPLETED) handle_thread_complete(next_event);
    if (next_event.type == Event::THREAD_PREEMPTED) handle_thread_preempted(next_event);
    ////////////
  }
  // Result outputs
  if (t_flag) tflag_output();
  cout << "SIMULATION COMPLETED!\n\n";
  output_process_type_data();
  output_totals();
}

void Simulation::handle_thread_arrival(Event event)
{
  /**
   * Add arriving thread to ready queue, set thread status.
   */
  event.thread->state = "READY";
  event.thread->arrival_time = event.time;
  add_thread_to_ready_queue(event.thread, event.time);
  // v_flag output
  if (v_flag) vflag_output(event, "Transitioned from NEW to READY");
}

void Simulation::add_thread_to_ready_queue(shared_ptr<Thread> thread, int current_time)
{
  /**
   * Function to add thread to ready queue according to algorithm. PRIORITY and
   * CUSTOM use separate queues from the other algorithms.
   */
  if (algorithm == PRIORITY)
  {
    Process::Type process_type = thread->process->type;
    priority_ready_queues[process_type].push(thread);
  }
  else if (algorithm == CUSTOM)
  {
    custom_ready_queue->push_thread(thread);
    quantom = custom_ready_queue->dynamic_quantom;
  }
  else ready_queue.push(thread);
  // If cpu is idle upon adding new ready thread, invoke dispatcher to dispatch it
  if (not running_thread)
    {
      Event e = Event(current_time, Event::DISPATCHER_INVOKED);
      e.thread = thread;
      event_queue.push(e);
    }
}

void Simulation::handle_dispatcher_invoked(Event event)
{
  /**
   * Get next thread from ready queue, determine overhead (process or thread) and
   * add dispatch complete event to queue.
   */
  // Get thread to run from top of ready queue
  shared_ptr<Thread> next_thread = get_next_thread();
  Event e(event.time, -1);
  if (current_process_id != next_thread->process->id)
  {
    // Process switch
    e.time += process_switch_overhead; 
    e.type = Event::PROCESS_DISPATCH_COMPLETED;
  }
  else
  {
    // Thread switch
    e.time += thread_switch_overhead; 
    e.type = Event::THREAD_DISPATCH_COMPLETED;
  }
  running_thread = next_thread;
  e.thread = next_thread;
  event_queue.push(e);
  // v_flag output
  if (v_flag)
  {
    event.thread = e.thread;
    std::string last_part = ((algorithm==FCFS)||(algorithm==PRIORITY)) ?
                              " will run to completion of burst" 
                              : " alotted time slice of " + std::to_string(quantom) + ".";
    std::string last_line = "Selected from " 
    + std::to_string(num_ready_threads() + 1) 
    + " thread(s);" + last_part;
  vflag_output(event, last_line);
  }
}

shared_ptr<Thread> Simulation::get_next_thread()
{
  /**
   * Get next ready thread to run based on algorithm.
   */
  if (algorithm == PRIORITY)
  {
    for(auto it = priority_ready_queues.begin(); it!=priority_ready_queues.end(); it++)
    {
      if(not it->empty())
      {
        shared_ptr<Thread> next_thread = it->front();
        it->pop();
        return next_thread;
      }
    }
  }
  else if (algorithm == CUSTOM)
  {
    shared_ptr<Thread> next_thread = custom_ready_queue->fetch_thread();
    quantom = custom_ready_queue->dynamic_quantom; // Update quantom when ready queue changes
    return next_thread;
  }
  else // Algorithm is not PRIORITY or CUSTOM
  {
    shared_ptr<Thread> next_thread = ready_queue.front();
    ready_queue.pop();
    return next_thread;
  }
  // This should never happen, there must always be something in the ready queue if the
  // dispatcher is invoked
  return nullptr;
}

void Simulation::handle_dispatch_complete(Event event)
{
  /**
   * Set thread to running, add dispatch end (preempt or burst complete)
   * to event queue.
   */
  assert(event.thread == running_thread);
  // Metrics
  if (event.type == Event::PROCESS_DISPATCH_COMPLETED)
  {
    total_dispatch_time += process_switch_overhead;
  }
  else
  {
    total_dispatch_time += thread_switch_overhead;
  }
  // Set status of running thread to running, set start time, set current process
  running_thread->state = "RUNNING";
  if (running_thread->start_time == -1) running_thread->start_time = event.time;
  current_process_id = event.thread->process->id;
  // Queue next event
  Event new_event = get_dispatch_end_event(event);
  event_queue.push(new_event);
  // v_flag output
  if (v_flag) vflag_output(event, "Transitioned from READY to RUNNING");
}

Event Simulation::get_dispatch_end_event(Event dispatch_event)
{
  /**
   * Gets the correct diapatch end event (preempt or cpu burst compelete)
   * based on algorithm and current quantom/remaining burst time
   */
  assert(running_thread == dispatch_event.thread);
  shared_ptr<Burst> next_burst = running_thread->bursts[running_thread->burst_index];
  if (algorithm == FCFS or algorithm == PRIORITY)
  {
    // Non-preemptive, just complete burst
    Event new_event = Event(dispatch_event.time + next_burst->cpu_time, Event::CPU_BURST_COMPLETED);
    new_event.thread = running_thread;
    return new_event;
  }
  else // algorithm == RR or CUSTOM
  {
    // Preemeptive, check quantom to determine whether to preempt
    int burst_amount_remaining = next_burst->cpu_time - running_thread->current_burst_completed_time;
    if (burst_amount_remaining <= quantom) // No preempt necessary just complete the burst
    {
      Event new_event = Event(dispatch_event.time + burst_amount_remaining, Event::CPU_BURST_COMPLETED);
      new_event.thread = running_thread;
      return new_event;
    }
    else // Preempt after quantom
    {
      Event new_event = Event(dispatch_event.time + quantom, Event::THREAD_PREEMPTED); 
      new_event.thread = running_thread;
      new_event.thread->current_burst_completed_time += quantom;
      return new_event;
    }
  }
}

void Simulation::handle_cpu_burst_complete(Event event)
{
  /**
   * Update simulation metrics, add IO complete or thread complete
   * event to queue based on whether there are remaining bursts.
   */
  shared_ptr<Burst> current_burst = event.thread->bursts[event.thread->burst_index];
  total_service_time += current_burst->cpu_time; // Metric
  event.thread->current_burst_completed_time = 0; // For preemptive alogrithms, flag as not in middle of burst
  // Check for IO burst, determine whether to complete or block thread
  if(current_burst->io_time != 0)
  {
    // Block for IO and add IO complete event to queue
    event.thread->state = "BLOCKED";
    Event e = Event(event.time + current_burst->io_time, Event::IO_BURST_COMPLETED);
    e.thread = event.thread;
    e.burst = current_burst;
    event_queue.push(e);
    if (v_flag) vflag_output(event, "Transitioned from RUNNING to BLOCKED");
  }
  else
  {
    // Complete thread
    Event e = Event(event.time, Event::THREAD_COMPLETED);
    e.thread = event.thread;
    event_queue.push(e);
    event.thread->state = "EXIT";
  }
  if (num_ready_threads() != 0){
    Event e = Event(event.time, Event::DISPATCHER_INVOKED);
    event_queue.push(e);
  }
  // Clear running thread, cpu is now idle
  running_thread = nullptr;
}

int Simulation::num_ready_threads()
{
  /**
   * Get number of threads in ready queue depending on algorithm.
   */
  if (algorithm == PRIORITY)
  {
    int num_threads = 0;
    for (auto it = priority_ready_queues.begin(); it!=priority_ready_queues.end(); it++)
    {
      num_threads += it->size();
    }
    return num_threads;
  }
  else if (algorithm == CUSTOM)
  {
    return custom_ready_queue->num_threads;
  }
  else return ready_queue.size();
}

void Simulation::handle_io_burst_complete(Event event)
{
  /**
   * Return thread to ready queue after IO burst.
   */
  total_io_time += event.burst->io_time; // Metric
  event.thread->state = "READY";
  event.thread->burst_index++;
  add_thread_to_ready_queue(event.thread, event.time);
  // v_flag output
  if (v_flag) vflag_output(event, "Transitioned from BLOCKED to READY");
}

void Simulation::handle_thread_complete(Event event)
{
  /**
   * Update metrics upon thread completion.
   */
  // Metrics
  total_elapsed_time = event.time;
  event.thread->end_time = event.time;
  // Process type data
  int proc_type = event.thread->process->type;
  process_type_data[proc_type][0] += 1; // Thread count
  process_type_data[proc_type][1] += event.thread->start_time - event.thread->arrival_time; // Response time
  process_type_data[proc_type][2] += event.thread->end_time - event.thread->arrival_time; // Turnaround time
  if(v_flag) vflag_output(event, "Transitioned from RUNNING to EXIT");
}

void Simulation::handle_thread_preempted(Event event)
{
  /**
   * Update preempted thread, put on ready queue
   */
  event.thread->state = "READY";
  running_thread = nullptr;
  add_thread_to_ready_queue(event.thread, event.time);
  // v-flag output
  if (v_flag) vflag_output(event, "Transitioned from RUNNING to READY");
}

void Simulation::vflag_output(Event event, std::string last_line)
{
  /**
   * Verbose output.
   */
  cout << "At time " << std::to_string(event.time) << ":\n";
  cout << "    " << event_type_string(event.type) << "\n";
  cout << "    Thread " << std::to_string(event.thread->id);
  cout << " in process " << std::to_string(event.thread->process->id);
  cout << " [" << process_type_string(event.thread->process->type) << "]" << "\n";
  cout << "    " << last_line << "\n\n";
}

void Simulation::output_totals()
{
  /**
   * Output final simulation data.
   */
  total_idle_time = total_elapsed_time - total_dispatch_time - total_service_time;
  float cpu_utilization = ((float)total_elapsed_time - (float)total_idle_time)/(float)total_elapsed_time;
  float cpu_efficiency = (float)total_service_time / (float)total_elapsed_time;
  cpu_utilization *= 100;
  cpu_efficiency *= 100;
  cout << std::left << std::setw(24) << "Total elapsed time:";
  cout  << std::right << std::setw(9) << std::to_string(total_elapsed_time) << "\n";
  cout << std::left << std::setw(24) << "Total service time:";
  cout  << std::right << std::setw(9) << std::to_string(total_service_time) << "\n";
  cout << std::left << std::setw(24) << "Total I/O time:";
  cout  << std::right << std::setw(9) << std::to_string(total_io_time) << "\n";
  cout << std::left << std::setw(24) << "Total dispatch time:";
  cout  << std::right << std::setw(9) << std::to_string(total_dispatch_time) << "\n";
  cout << std::left << std::setw(24) << "Total idle time:";
  cout  << std::right << std::setw(9) << std::to_string(total_idle_time) << "\n";
  cout << "\n";
  cout << std::left <<std::setw(24) << "CPU utilization:";
  cout  << std::right << std::setw(8) << std::setprecision(2) << std::fixed << cpu_utilization << "%\n";
  cout << std::left <<std::setw(24) << "CPU efficiency:";
  cout  << std::right << std::setw(8) << std::setprecision(2) << std::fixed << cpu_efficiency << "%\n";
}

void Simulation::output_process_type_data()
{
  /**
   * Final data for individual process types.
   */
  for(int type=0; type <= 3; type++){
    float thr_count = (float) process_type_data[type][0];
    float average_response_time = (float) process_type_data[type][1] / thr_count;
    if (average_response_time != average_response_time) average_response_time = 0;
    float average_turnaround_time = (float) process_type_data[type][2] / thr_count;
    if (average_turnaround_time != average_turnaround_time) average_turnaround_time = 0;
    cout << process_type_string(type) << " THREADS:\n";
    cout << std::left << std::setw(24) << "    Total count:";
    cout << std::right << std::setw(9) << process_type_data[type][0] << "\n"; // Actuall want the int here
    cout << std::left << std::setw(24) << "    Avg response time:";
    cout << std::right << std::setw(9) << std::setprecision(2) << std::fixed << average_response_time << "\n";
    cout << std::left << std::setw(24) << "    Avg turnaround time:";
    cout << std::right << std::setw(9) << std::setprecision(2) << std::fixed << average_turnaround_time << "\n";
    cout << "\n";
  }
}

void Simulation::tflag_output()
{
  /**
   * Thread level data for --per_thread argument.
   */
  for(int i=0; i < processes.size(); i++)
  {
    shared_ptr<Process> proc = processes[i];
    cout << "Process " << proc->id << " [" << process_type_string(proc->type) << "]:\n";
    for(int j=0; j<proc->threads.size(); j++) 
    {
      shared_ptr<Thread> thr = proc->threads[j];
      cout << std::left << std::setw(15) << "    Thread " + std::to_string(thr->id) + ":";
      cout << std::left << std::setw(12) << "ARR: " + std::to_string(thr->arrival_time);
      cout << std::left << std::setw(12) << "CPU: " + std::to_string(total_burst_time(thr, true));
      cout << std::left << std::setw(12) << "I/O: " + std::to_string(total_burst_time(thr, false));
      cout << std::left << std::setw(12) << "TRT: " + std::to_string(thr->end_time - thr->arrival_time);
      cout << std::left << std::setw(12) << "END: " + std::to_string(thr->end_time);
      cout << "\n";
    }
    cout << "\n";
  }
}

int Simulation::total_burst_time(shared_ptr<Thread> thread, bool cpu_times)
{
  /**
   * Calculate total burst time (cpu or io depending on cpu_times flag).
   * 
   * Returns total burst time (cpu or IO) for thread.
   */
  int total = 0;
  for(int i=0; i < thread->bursts.size(); i++)
  {
    shared_ptr<Burst> burst = thread->bursts[i];
    if (cpu_times) total += burst->cpu_time;
    else total += burst->io_time;
  }
  return total;
}

std::string Simulation::process_type_string(int i)
{
  /**
   * Converse process type int to string for output.
   */
  switch(i)
  {
    case 0: return "SYSTEM";
      break;
    case 1: return "INTERACTIVE";
      break;
    case 2: return "NORMAL";
      break;
    case 3: return "BATCH";
      break;
    default: return "INCORRECT PROCESS TYPE";
  }
}

std::string Simulation::event_type_string(int event_type)
{
  /**
   * Convert event type to string for output.
   */
  switch(event_type)
  {
    case Event::CPU_BURST_COMPLETED: return "CPU_BURST_COMPLETED";
      break;
    case Event::IO_BURST_COMPLETED: return "IO_BURST_COMPLETED";
      break;
    case Event::DISPATCHER_INVOKED: return "DISPATCHER_INVOKED";
      break;
    case Event::THREAD_DISPATCH_COMPLETED: return "THREAD_DISPATCH_COMPLETED";
      break;
    case Event::PROCESS_DISPATCH_COMPLETED: return "PROCESS_DISPATCH_COMPLETED";
      break;
    case Event::THREAD_PREEMPTED: return "THREAD_PREEMPTED";
      break;
    case Event::THREAD_ARRIVED: return "THREAD_ARRIVED";
      break;
    case Event::THREAD_COMPLETED: return "THREAD_COMPLETED";
      break;
    default: return "INCORRECT_EVENT_TYPE";
  }
}