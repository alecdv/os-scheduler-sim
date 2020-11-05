/**
 * Operating Systems
 * Project 2 - OS Scheduling Simulator
 * Alec De Vivo
 * 
 * main.cpp
 * Reads in process/thread/burst data from input file, parses command line arguments, builds process data
 * structures and passes to simlation, and launches simulation.
 */


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <iomanip>
#include <cassert>
#include <unistd.h>
#include <getopt.h>
#include "process_structs.h"
#include "simulation.h"

using std::vector; using std::string; using std::shared_ptr;

void display_help()
{
  /**
   * Outputs help text for -h --help argument.
   */
  using std::cout;
  string indent = "  ";
  cout << "Operating system scheduling simulator\n";
  cout << "Arguments:\n";
  cout << indent << "-v, --verbose\n";
  cout << indent << indent << "Output information about every state-changing event and scheduling decision.\n";
  cout << indent << "-t, --per_thread\n";
  cout << indent << indent << "Output additional per-thread statistics for arrival time, service time, etc.\n";
  cout << indent << "-a\n";
  cout << indent << indent << "The scheduling algorithm to use. One of FCFs, RR, PRIORITY, or CUSTOM.\n";
  cout << indent << "Final argument should be the input .txt file\n";
  cout << indent << indent << "This file should inlcude process, thread, and burst data.\n";
  cout << indent << indent << "See README for specific formatting\n";
}

void set_simulation_alg(string algorithm_arg, Simulation& simulation)
{
  /**
   * Parses -a --algorithm argument and sets the algorithm property on the Simulation
   * object appropriately.
   */
  if (algorithm_arg == "FCFS") simulation.algorithm = Simulation::FCFS;
  else if (algorithm_arg == "RR") simulation.algorithm = Simulation::RR;
  else if (algorithm_arg == "PRIORITY") simulation.algorithm = Simulation::PRIORITY;
  else simulation.algorithm = Simulation::CUSTOM;
}

vector<string> tokenize(string str)
{
  /**
   * Creates stringstream and tokenize str input into words vector. 
   * 
   * Returns vector of individual words from str.
   */
  std::stringstream stream(str);
  string word;
  vector<string> words;
  while(stream >> word){
    words.push_back(word);
  }
  return words;
}

shared_ptr<Thread> readin_thread(std::istream& input_stream, vector<string> thread_params, shared_ptr<Process> process, int thread_id)
{
  /**
   * Parses thread level data from input file.
   * 
   * Returns pointer to thread object.
   */
  int thread_arrival_time = std::stoi(thread_params[0]);
  shared_ptr<Thread> thread = std::make_shared<Thread>(thread_arrival_time, thread_id, process);
  for (int i = 0; i < std::stoi(thread_params[1]); )
  {
    string line;
    getline(input_stream, line);
    if (line.empty()) continue; // Skip blank lines
    else
    {
      vector<string> burst_params = tokenize(line);
      if (burst_params.size() == 1) burst_params.push_back("0");
      int cpu_time = std::stoi(burst_params[0]);
      int io_time = std::stoi(burst_params[1]);
      shared_ptr<Burst> burst = std::make_shared<Burst>(cpu_time, io_time, thread);
      thread->bursts.push_back(burst);
      i++; // Only increment when we actually read in a burst
    }
  }
  return thread;
}

shared_ptr<Process> readin_process(std::istream& input_stream, vector<string> process_params)
{
   /**
   * Parses process level data from input file.
   * 
   * Returns pointer to process object.
   */
  int proc_id = std::stoi(process_params[0]);
  Process::Type proc_type = static_cast<Process::Type>(std::stoi(process_params[1]));
  shared_ptr<Process> process = std::make_shared<Process>(proc_id, proc_type);
  for (int i = 0; i < std::stoi(process_params[2]); )
  {
    string line;
    getline(input_stream, line);
    if (line.empty()) continue; // Skip blank lines
    else
    {
      shared_ptr<Thread> thread = readin_thread(input_stream, tokenize(line), process, i);
      thread->id = i; // Threads numbered based on input order from file
      process->threads.push_back(thread);
      i++; // Only increment when we actually read in a thread
    }
  }
  return process;
}

int main(int argc, char *argv[])
{
   /**
   * Main function parses args, loops through input file to generate simulation data structures,
   * and launches simulation.
   */
  bool t_flag = false; bool v_flag = false;
  bool a_flag = false; bool h_flag = false;
  // Process command line arguments
  int opt; int index; string algorithm;
  const char* const short_opts = "htva:";
  const struct option long_opts[] = 
  {
    {"per_thread", no_argument, 0, 't'},
    {"verbose", no_argument, 0, 'v'},
    {"algorithm", required_argument, 0, 'a'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };
  while (true) 
  {
    opt = getopt_long(argc, argv, short_opts, long_opts, &index);
    if (opt == -1) break;

    switch(opt)
    {
      case 'h':
        display_help();
        exit(0);
        break;
      case 't':
        t_flag = true;
        break;
      case 'v':
        v_flag = true;
        break;
      case 'a':
        a_flag = true;
        algorithm = string(optarg);
        break;
      default:
        std::cout << "ERROR INVALID OPTION" << "\n";
        exit(0);
    }
  }
  // Open input file
  char* file = argv[optind];
  std::ifstream file_in(file);
  if (!file_in) {
    std::cout << "ERROR INVALID INPUT FILE" << "\n";
    exit(0);
  }
  // Read in top line parameters:
  // num_processes, thread_switch_overhead, process_switch_overhead
  string top_line;
  getline(file_in, top_line);
  vector<string> params = tokenize(top_line);
  int num_processes = std::stoi(params[0]);
  int thread_switch_overhead = std::stoi(params[1]);
  int process_switch_overhead = std::stoi(params[2]);
  // Process input file to generate simulation
  string line; 
  int processes_created = 0;
  Simulation simulation(process_switch_overhead, thread_switch_overhead);
  if (v_flag) simulation.v_flag = true;
  if (t_flag) simulation.t_flag = true;
  if (a_flag) set_simulation_alg(algorithm, simulation);
  for ( int i = 0; i < num_processes; ) // Note no incrementing in for loop expression
  {
    getline(file_in, line);
    if (line.empty())  continue;  // For skipping blank lines
    else
    {
      simulation.add_process(readin_process(file_in, tokenize(line)));
      i++; // Onle increment when a process is read in
    }
  }
  // Launch simulation
  simulation.run_simulation();
  return 0;
}
