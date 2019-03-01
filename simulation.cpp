#include <vector>
#include <queue>
#include <iostream>
#include <iomanip>
#include <memory>
#include "process_structs.h"
#include "simulation.h"

using std::cout; 
using std::shared_ptr;

// ***** PUBLIC METHODS
Simulation::Simulation() 
  : total_elapsed_time(0), total_dispatch_time(0), total_io_time(0), 
    total_service_time(0), total_idle_time(0), running_thread(nullptr)
  {}
void Simulation::destructive_display()
{
  // Display structure of processes/threads in sumlation for testing purposes
  // Note: this is destructive to the simulation because the event queue is emptied
  // Cannot display and then run simulation in same execution
  for(shared_ptr<Process> proc : processes)
  {
    cout<<"Process: "<<std::to_string(proc->id)<<" TYPE: "<<std::to_string(proc->type)<<"\n";
    for (shared_ptr<Thread> thr : proc->threads)
    {
      cout<<"---Thread: "<<std::to_string(thr->id)<<" TIME: "<<std::to_string(thr->arrival_time);
      cout<<"Proc:"<<std::to_string(thr->process->id)<<"\n";
      for (shared_ptr<Burst> b : thr->bursts)
      {
        cout<<"------Burst: ";
        cout<<"CPU: "<<std::to_string(b->cpu_time)<<" IO: "<<std::to_string(b->io_time);
        cout<<" THR:"<<std::to_string(b->thread->id)<<std::endl;
      }
    }
  }
  while (event_queue.empty() == false)
  {
    Event e = event_queue.top();
    cout<<"EVENT "<<e.type<<" at time "<<std::to_string(e.time)<<" from thread "<<e.thread->id;
    cout<<" in process "<<e.thread->process->id<<"\n";
    event_queue.pop();
  }

  while (ready_queue.empty() == false)
  {
    shared_ptr<Thread> t = ready_queue.front();
    cout<<"READY QUEUE: "<<"\n";
    cout<<"THREAD: "<<std::to_string(t->id)<<" ARRIVAL TIME: "<<std::to_string(t->arrival_time)<<"\n";
    ready_queue.pop();
  }
  cout<<"Total elapsed time: "<<std::to_string(total_elapsed_time)<<"\n";
  cout<<"Total idle time:"<<std::to_string(total_idle_time)<<"\n";
}


void Simulation::add_process(std::shared_ptr<Process> process)
{
  // Add process pointer to process list within simulation 
  // and create events for thread arrivals
  processes.push_back(process);
  for (int i=0; i<process->threads.size(); i++){
    Event event(process->threads[i]->arrival_time, Event::THREAD_ARRIVED);
    event.thread = process->threads[i];
    event_queue.push(event);
  }
}

// ****** PRIVATE METHODS

bool CompareEventsByArrivalTime::operator()(Event const & e1, Event const & e2)
{
  // Comparator for event priority queue
  if (e1.time == e2.time)
  {
    return e1.type > e2.type;
  }
  else return e1.time > e2.time;
}

bool CompareThreadsByArrivalTime::operator()(std::shared_ptr<Thread> const & t1, std::shared_ptr<Thread> const & t2)
{
  return t1->arrival_time > t2->arrival_time;
}

void Simulation::run_simulation()
{
  // Ultimately there will be multiple run_sim methods for different
  // Simulation algorithms
  while(event_queue.empty() == false)
  {
    Event next_event = event_queue.top();
    event_queue.pop();
    // Pass to different event handlers here
    if (next_event.type == Event::THREAD_ARRIVED) handle_thread_arrival(next_event);
    if (next_event.type == Event::DISPATCHER_INVOKED) handle_dispatcher_invoked(next_event);
    if (next_event.type == Event::PROCESS_DISPATCH_COMPLETED) handle_dispatch_complete(next_event);
    if (next_event.type == Event::THREAD_DISPATCH_COMPLETED) handle_dispatch_complete(next_event);
    if (next_event.type == Event::CPU_BURST_COMPLETED) handle_cpu_burst_complete(next_event);
    if (next_event.type == Event::IO_BURST_COMPLETED) handle_io_burst_complete(next_event);
    if (next_event.type == Event::THREAD_COMPLETED) handle_thread_complete(next_event);
    ////////////
  }
  cout << "SIMULATION COMLETED!\n";
  output_totals();
}

void Simulation::handle_thread_arrival(Event event)
{
  // Event handling (only a state change for the thread as of now)
  event.thread->state = "READY";
  ready_queue.push(event.thread);
  // If CPU idle, add dispatch invoke event at current time
  if(!running_thread) {
    Event e = Event(event.thread->arrival_time, Event::DISPATCHER_INVOKED);
    event_queue.push(e);
  }
  // Message output
  vflag_output(event, "Transitioned from NEW to READY");
}

void Simulation::handle_dispatcher_invoked(Event event)
{
  // Get thread to run from top of ready queue
  shared_ptr<Thread> next_thread = ready_queue.front();
  ready_queue.pop();
  Event e(event.time, -1);
  if (!running_thread or running_thread->process->id != next_thread->process->id)
  {
    // Process switch
    e.time += 7; 
    e.type = Event::PROCESS_DISPATCH_COMPLETED;
  }
  else
  {
    // Thread switch
    e.time += 3; 
    e.type = Event::THREAD_DISPATCH_COMPLETED;
  }
  running_thread = next_thread;
  e.thread = next_thread;
  event_queue.push(e);
  // MESSAGE OUTPUT
  event.thread = e.thread;
  std::string last_line = "Selected from " 
    + std::to_string(ready_queue.size() + 1) 
    + " threads; will run to completion of burst";
  vflag_output(event, last_line);
}

void Simulation::handle_dispatch_complete(Event event)
{
  if (event.type == Event::PROCESS_DISPATCH_COMPLETED)
  {
    total_dispatch_time += 7;
  }
  else
  {
    total_dispatch_time += 3;
  }
  // Set status of running thread to running, if first dispatch set start time
  running_thread->state = "RUNNING";
  if (running_thread->burst_index == 0) running_thread->start_time = event.time;
  // Get next burst
  shared_ptr<Burst> next_burst = running_thread->bursts[running_thread->burst_index];
  Event e = Event(event.time + next_burst->cpu_time, Event::CPU_BURST_COMPLETED);
  e.thread = running_thread;
  event_queue.push(e);
  // Message output
  vflag_output(event, "Transitioned from READY to RUNNING");
}

void Simulation::handle_cpu_burst_complete(Event event)
{
  shared_ptr<Burst> current_burst = event.thread->bursts[event.thread->burst_index];
  total_service_time += current_burst->cpu_time;
  // Check for IO burst, determine whether to complete or block thread
  if(current_burst->io_time != 0)
  {
    // Block for IO and add IO complete event to queue
    event.thread->state = "BLOCKED";
    Event e = Event(event.time + current_burst->io_time, Event::IO_BURST_COMPLETED);
    e.thread = event.thread;
    e.burst = current_burst;
    event_queue.push(e);
    vflag_output(event, "Transitioned from RUNNING to BLOCKED");
  }
  else
  {
    // Complete thread
    Event e = Event(event.time, Event::THREAD_COMPLETED);
    e.thread = event.thread;
    event_queue.push(e);
    event.thread->state = "EXIT";
  }
  if (ready_queue.empty() != true){
    Event e = Event(event.time, Event::DISPATCHER_INVOKED);
    event_queue.push(e);
  } 
}

void Simulation::handle_io_burst_complete(Event event)
{
  total_io_time += event.burst->io_time;
  // Determine whether to invoke dispatcher
  // BLOCKED or EXIT status means the CPU is currently idle and should dispatch the current
  // thread that is returning from IO
  if (running_thread->state == "BLOCKED" or running_thread->state == "EXIT")
  {
    Event e = Event(event.time, Event::DISPATCHER_INVOKED);
    e.thread = event.thread;
    event_queue.push(e);
  }
  // Add thread back to ready queue
  event.thread->state = "READY";
  event.thread->burst_index++;
  ready_queue.push(event.thread);
  // Message output
  vflag_output(event, "Transitioned from BLOCKED to READY");
}

void Simulation::handle_thread_complete(Event event)
{
  total_elapsed_time = event.time;
  event.thread->end_time = event.time;
  vflag_output(event, "Transitioned from RUNNING to EXIT");
}

void Simulation::vflag_output(Event event, std::string last_line)
{
  cout << "At time " << std::to_string(event.time) << ":\n";
  cout << "    " << event_type_string(event.type) << "\n";
  cout << "    Thread " << std::to_string(event.thread->id);
  cout << " in process " << std::to_string(event.thread->process->id);
  cout << " [" << process_type_string(event.thread->process->type) << "]" << "\n";
  cout << "    " << last_line << "\n\n";
}

void Simulation::output_totals()
{
  total_idle_time = total_elapsed_time - total_dispatch_time - total_service_time;
  float cpu_utilization = ((float)total_elapsed_time - (float)total_idle_time)/(float)total_elapsed_time;
  float cpu_efficiency = (float)total_service_time / (float)total_elapsed_time;
  cpu_utilization *= 100;
  cpu_efficiency *= 100;
  cout << std::left << std::setw(20) << "Total elapsed time:";
  cout  << std::right << std::setw(13) << std::to_string(total_elapsed_time) << "\n";
  cout << std::left << std::setw(20) << "Total service time:";
  cout  << std::right << std::setw(13) << std::to_string(total_service_time) << "\n";
  cout << std::left << std::setw(20) << "Total I/O time:";
  cout  << std::right << std::setw(13) << std::to_string(total_io_time) << "\n";
  cout << std::left << std::setw(20) << "Total dispatch time:";
  cout  << std::right << std::setw(13) << std::to_string(total_dispatch_time) << "\n";
  cout << std::left << std::setw(20) << "Total idle time:";
  cout  << std::right << std::setw(13) << std::to_string(total_idle_time) << "\n";
  cout << "\n";
  cout << std::left <<std::setw(20) << "CPU utilization:";
  cout  << std::right << std::setw(13) << std::setprecision(4) << cpu_utilization << "\n";
  cout << std::left <<std::setw(20) << "CPU efficiency:";
  cout  << std::right << std::setw(13) << std::setprecision(4) << cpu_efficiency << "\n";
}

std::string Simulation::process_type_string(int i)
{
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