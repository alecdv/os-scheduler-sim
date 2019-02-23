#ifndef PROCESS_STRUCTS_H
#define PROCESS_STRUCTS_H

#include <vector>
#include <string>
#include <memory>

struct Process; struct Thread; struct Burst; struct Event;

struct Process
{
  Process(int proc_id, int proc_type) 
    : id(proc_id), type(proc_type) 
  {}
  int id;
  int type;
  std::vector<std::shared_ptr<Thread> > threads;
};

struct Thread
{
  Thread(int arr_time_arg, int thr_id_arg, std::shared_ptr<Process> proc_arg)
    : arrival_time(arr_time_arg), id(thr_id_arg), state("NEW"), process(proc_arg)
  {}
  int id;
  int arrival_time;
  std::string state;
  std::shared_ptr<Process> process;
  std::vector<std::shared_ptr<Burst> > bursts;
};

struct Burst
{
  Burst(int cpu_time_arg, int io_time_arg, std::shared_ptr<Thread> thr_arg)
    : cpu_time(cpu_time_arg), io_time(io_time_arg), thread(thr_arg)
  {}
  int cpu_time;
  int io_time;
  std::shared_ptr<Thread> thread;
};

struct Event
{
  Event(int time_arg, std::string type_arg)
    : time(time_arg), type(type_arg)
  {}
  int time;
  std::string type;
  std::shared_ptr<Thread> thread;
  std::shared_ptr<Burst> burst;
};

#endif
