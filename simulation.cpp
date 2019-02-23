#include <vector>
#include <queue>
#include <iostream>
#include <memory>
#include "process_structs.h"
#include "simulation.h"

using std::cout; 
using std::shared_ptr;

// ***** PUBLIC METHODS
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

bool CompareByArrivalTime::operator()(Event const & e1, Event const & e2)
{
  // Comparator for event priority queue
  return e1.time > e2.time;
}


void Simulation::run_simulation()
{
  // Ultimately there will be multiple run_sim methods for different
  // Simulation algorithms
  while(event_queue.empty() == false)
  {
    Event next_event = event_queue.top();
    // Pass to different event handlers here
    if (next_event.type == "THREAD_ARRIVED") handle_thread_arrival(next_event);
    ////////////
    event_queue.pop();
  }
}

void Simulation::handle_thread_arrival(Event event)
{
  // Event handling (only a state change for the thread as of now)
event.thread->state = "READY";
  // Message output
  cout << "At time " << std::to_string(event.time) << ":\n";
  cout << "\tTHREAD_ARRIVED\n";
  cout << "\tThread " << std::to_string(event.thread->id);
  cout << " in process " << std::to_string(event.thread->process->id);
  cout << " [" << process_type_string(event.thread->process->type) << "]" << "\n";
  cout << "\tTransitioned from NEW to READY\n";
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