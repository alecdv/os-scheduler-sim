#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <queue>
#include <memory>
#include "process_structs.h"

struct CompareEventsByArrivalTime{
  bool operator()(Event const & e1, Event const & e2);
};

struct CompareThreadsByArrivalTime{
  bool operator()(std::shared_ptr<Thread> const & t1, std::shared_ptr<Thread> const & t2);
};

class Simulation
{
public:
  Simulation();
  void destructive_display();
  void run_simulation();
  void add_process(std::shared_ptr<Process> process);
  std::shared_ptr<Event> next_event();
  std::vector<std::shared_ptr<Process> > get_processes();
  std::vector<std::shared_ptr<Thread> > get_threads();
private:
  void handle_thread_arrival(Event event);
  std::string process_type_string(int type);
  // Metrics
  int total_elapsed_time;
  int total_service_time;
  int total_io_time;
  int total_dispatch_time;
  int total_idle_time;
  // Lists and queues
  std::vector<std::shared_ptr<Process> > processes;
  //std::vector<Event> events;
  std::priority_queue<Event, std::vector<Event>, CompareEventsByArrivalTime> event_queue;
  std::priority_queue<std::shared_ptr<Thread>, std::vector<std::shared_ptr<Thread> >, CompareThreadsByArrivalTime> ready_queue;
};
#endif
