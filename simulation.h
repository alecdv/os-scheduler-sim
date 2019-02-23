#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <queue>
#include <memory>
#include "process_structs.h"

struct CompareByArrivalTime{
  bool operator()(Event const & e1, Event const & e2);
};

class Simulation
{
public:
  void destructive_display();
  void run_simulation();
  void add_process(std::shared_ptr<Process> process);
  std::shared_ptr<Event> next_event();
  std::vector<std::shared_ptr<Process> > get_processes();
  std::vector<std::shared_ptr<Thread> > get_threads();
private:
  void handle_thread_arrival(Event event);
  std::string process_type_string(int type);
  std::vector<std::shared_ptr<Process> > processes;
  //std::vector<Event> events;
  std::priority_queue<Event, std::vector<Event>, CompareByArrivalTime> event_queue;
};

#endif
