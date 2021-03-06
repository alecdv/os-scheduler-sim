/**
 * Operating Systems
 * Project 2 - OS Scheduling Simulator
 * Alec De Vivo
 * 
 * simulation.h
 * 
 * Defines simulation properties and functions.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <queue>
#include <memory>
#include "process_structs.h"

struct CustomReadyQueue
{
  CustomReadyQueue();
  std::vector<std::queue<std::shared_ptr<Thread> > > short_queues;
  std::vector<std::queue<std::shared_ptr<Thread> > > long_queues;
  double dynamic_quantom;
  int num_threads;
  int total_remaining_time;
  int avg_age;
  const int QUANTOM_MAX = 20;
  std::shared_ptr<Thread> fetch_thread();
  void push_thread(std::shared_ptr<Thread>);
};

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
  void run_simulation();
  void add_process(std::shared_ptr<Process> process);
  std::shared_ptr<Event> next_event();
  std::vector<std::shared_ptr<Process> > get_processes();
  std::vector<std::shared_ptr<Thread> > get_threads();
  // Flags
  bool v_flag;
  bool t_flag;
  int quantom;
  int algorithm;
  static const int FCFS = 0;
  static const int RR = 1;
  static const int PRIORITY = 2;
  static const int CUSTOM = 3;
private:
  void handle_thread_arrival(Event event);
  void add_thread_to_ready_queue(std::shared_ptr<Thread> thread, int current_time);
  void handle_dispatcher_invoked(Event event);
  std::shared_ptr<Thread> get_next_thread();
  void handle_dispatch_complete(Event event);
  Event get_dispatch_end_event(Event dispatch_event);
  void handle_cpu_burst_complete(Event event);
  void handle_io_burst_complete(Event event);
  void handle_thread_complete(Event event);
  void handle_thread_preempted(Event event);
  std::string process_type_string(int type);
  std::string event_type_string(int type);
  int total_burst_time(std::shared_ptr<Thread> thread, bool get_cpu_times);
  void vflag_output(Event event, std::string last_line);
  void output_totals();
  void output_process_type_data();
  void tflag_output();
  int num_ready_threads();
  // Metrics
  int total_elapsed_time;
  int total_dispatch_time;
  int total_io_time;
  int total_service_time;
  int total_idle_time;
  std::vector<std::vector<int> > process_type_data;
  // Simulation data
  int process_switch_overhead;
  int thread_switch_overhead;
  // Process objects/lists/queues
  std::shared_ptr<Thread> running_thread;
  std::vector<std::shared_ptr<Process> > processes;
  std::priority_queue<Event, std::vector<Event>, CompareEventsByArrivalTime> event_queue;
  std::queue<std::shared_ptr<Thread> > ready_queue;
  std::vector<std::queue<std::shared_ptr<Thread> > > priority_ready_queues;
  int current_process_id;
  std::shared_ptr<CustomReadyQueue> custom_ready_queue;
};
#endif
