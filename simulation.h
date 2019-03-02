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
  Simulation(int process_switch_overhead, int thread_switch_overhead);
  void destructive_display();
  void run_simulation();
  void add_process(std::shared_ptr<Process> process);
  std::shared_ptr<Event> next_event();
  std::vector<std::shared_ptr<Process> > get_processes();
  std::vector<std::shared_ptr<Thread> > get_threads();
  // Flags
  bool v_flag;
  bool t_flag;
  std::string algorithm;
private:
  void handle_thread_arrival(Event event);
  void handle_dispatcher_invoked(Event event);
  void handle_dispatch_complete(Event event);
  void handle_cpu_burst_complete(Event event);
  void handle_io_burst_complete(Event event);
  void handle_thread_complete(Event event);
  std::string process_type_string(int type);
  std::string event_type_string(int type);
  void vflag_output(Event event, std::string last_line);
  void output_totals();
  void output_process_type_data();
  // Simulation data
  int process_switch_overhead;
  int thread_switch_overhead;
  // Metrics
  int total_elapsed_time;
  int total_service_time;
  int total_io_time;
  int total_dispatch_time;
  int total_idle_time;
  std::vector<std::vector<int> > process_type_data;
  // Process objects/lists/queues
  std::shared_ptr<Thread> running_thread;
  std::vector<std::shared_ptr<Process> > processes;
  std::priority_queue<Event, std::vector<Event>, CompareEventsByArrivalTime> event_queue;
  std::queue<std::shared_ptr<Thread> > ready_queue;
};
#endif
