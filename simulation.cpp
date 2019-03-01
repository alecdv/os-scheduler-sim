#include <vector>
#include <queue>
#include <iostream>
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
    shared_ptr<Thread> t = ready_queue.top();
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
    Event event(process->threads[i]->arrival_time, "THREAD_ARRIVED");
    event.thread = process->threads[i];
    event_queue.push(event);
  }
}

// ****** PRIVATE METHODS

bool CompareEventsByArrivalTime::operator()(Event const & e1, Event const & e2)
{
  // Comparator for event priority queue
  if (e1.time == e2.time and e1.type != e2.type) {
    if (e2.type == "DISPATCH_INVOKED") {
      return true;
    }
  }
  return e1.time > e2.time;
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
    if (next_event.type == "THREAD_ARRIVED") handle_thread_arrival(next_event);
    if (next_event.type == "DISPATCHER_INVOKED") handle_dispatcher_invoked(next_event);
    if (next_event.type == "PROCESS_DISPATCH_COMPLETED") handle_dispatch_complete(next_event);
    if (next_event.type == "THREAD_DISPATCH_COMPLETED") handle_dispatch_complete(next_event);
    if (next_event.type == "CPU_BURST_COMPLETED") handle_cpu_burst_complete(next_event);
    if (next_event.type == "IO_BURST_COMPLETED") handle_io_burst_complete(next_event);
    ////////////
  }
}

void Simulation::handle_thread_arrival(Event event)
{
  // Event handling (only a state change for the thread as of now)
  event.thread->state = "READY";
  ready_queue.push(event.thread);
  // If CPU idle, add dispatch invoke event at current time
  if(!running_thread) {
    Event e = Event(event.thread->arrival_time, "DISPATCHER_INVOKED");
    event_queue.push(e);
  }
  // Message output
  cout << "At time " << std::to_string(event.time) << "\n";
  cout << "\tTHREAD_ARRIVED\n";
  cout << "\tThread " << std::to_string(event.thread->id);
  cout << " in process " << std::to_string(event.thread->process->id);
  cout << " [" << process_type_string(event.thread->process->type) << "]" << "\n";
  cout << "\tTransitioned from NEW to READY\n";
}

void Simulation::handle_dispatcher_invoked(Event event)
{
  // Get thread to run from top of ready queue
  shared_ptr<Thread> next_thread = ready_queue.top();
  ready_queue.pop();
  Event e(event.time, "");
  // TODO: FIX THIS, right now the running thread is always empty when the dispatcher
  // is invoked, meaning we treat everything as a process switch
  if (!running_thread or running_thread->process->id != next_thread->process->id)
  {
    // Process switch
    e.time += 7; 
    e.type = "PROCESS_DISPATCH_COMPLETED";
  }
  else
  {
    // Thread switch
    e.time += 3; 
    e.type = "THREAD_DISPATCH_COMPLETED";
  }
  running_thread = next_thread;
  e.thread = next_thread;
  event_queue.push(e);
  // MESSAGE OUTPUT
  cout << "At time " << std::to_string(event.time) << "\n";
  cout << "\tDISPATCHER_INVOKED\n";
  cout << "\tThread " << std::to_string(e.thread->id);
  cout << " in process " << std::to_string(e.thread->process->id);
  cout << " [" << process_type_string(e.thread->process->type) << "]" << "\n";
  cout << "\tSelected from " << std::to_string(ready_queue.size() + 1) << " threads; will run to completion of burst\n";
}

void Simulation::handle_dispatch_complete(Event event)
{
  // Set status of running thread to running
  running_thread->state = "RUNNING";
  // Get next burst
  shared_ptr<Burst> next_burst = running_thread->bursts[running_thread->burst_index];
  Event e = Event(event.time + next_burst->cpu_time, "CPU_BURST_COMPLETED");
  e.thread = running_thread;
  event_queue.push(e);
  // Message output
  cout << "At time " << std::to_string(event.time) << "\n";
  cout << "\tDISPATCH_COMPLETED\n";
  cout << "\tThread " << std::to_string(running_thread->id);
  cout << " in process " << std::to_string(running_thread->process->id);
  cout << " [" << process_type_string(running_thread->process->type) << "]" << "\n";
  cout << "\tTransitioned from READY to RUNNING\n";
}

void Simulation::handle_cpu_burst_complete(Event event)
{
  // Check for IO burst
  shared_ptr<Burst> current_burst = event.thread->bursts[event.thread->burst_index];
  if(current_burst->io_time != 0)
  {
    // Block for IO and add IO complete event to queue
    event.thread->state = "BLOCKED";
    blocked_queue.push(event.thread);
    Event e = Event(event.time + current_burst->io_time, "IO_BURST_COMPLETED");
    e.thread = event.thread;
    event_queue.push(e);
  }
  else
  {
    // Complete thread
    cout << "At time " << std::to_string(event.time) << "\n";
    cout << "\tDISPATCH_COMPLETED]\n";
    cout << "\tThread " << std::to_string(running_thread->id);
    cout << " in process " << std::to_string(running_thread->process->id);
    cout << " [" << process_type_string(running_thread->process->type) << "]" << "\n";
    cout << "\tTransitioned from READY to EXIT\n";
    event.thread->state = "EXIT";
  }
  if (ready_queue.empty() != true){
    Event e = Event(event.time, "DISPATCHER_INVOKED");
    event_queue.push(e);
  } 
  // Message output
  cout << "At time " << std::to_string(event.time) << "\n";
  cout << "\tCPU_BURST_COMPLETED\n";
  cout << "\tThread " << std::to_string(event.thread->id);
  cout << " in process " << std::to_string(event.thread->process->id);
  cout << " [" << process_type_string(event.thread->process->type) << "]" << "\n";
  cout << "\tTransitioned from RUNNING to BLOCKED\n";
}

void Simulation::handle_io_burst_complete(Event event)
{
  // Add thread back to ready queue
  event.thread->state = "READY";
  event.thread->burst_index++;
  ready_queue.push(event.thread);
    // Message output
  cout << "At time " << std::to_string(event.time) << "\n";
  cout << "\tIO_BURST_COMPLETED\n";
  cout << "\tThread " << std::to_string(event.thread->id);
  cout << " in process " << std::to_string(event.thread->process->id);
  cout << " [" << process_type_string(event.thread->process->type) << "]" << "\n";
  cout << "\tTransitioned from BLOCKED to READY\n";
}

std::string Simulation::process_type_string(int i){
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