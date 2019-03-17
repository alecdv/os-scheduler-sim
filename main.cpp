#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <unistd.h>
#include <getopt.h>
#include "process_structs.h"
#include "simulation.h"

using std::vector; using std::string; using std::shared_ptr;

void set_simulation_alg(string algorithm_arg, Simulation& simulation)
{
  if (algorithm_arg == "FCFS") simulation.algorithm = Simulation::FCFS;
  else if (algorithm_arg == "RR") simulation.algorithm = Simulation::RR;
  else if (algorithm_arg == "PRIORITY") simulation.algorithm = Simulation::PRIORITY;
  else simulation.algorithm = Simulation::CUSTOM;
}

vector<string> tokenize(string str)
{
  // create stringstream and tokenize input into words
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
  // Read in thread and burst data and return pointer to thread object
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
  // Process command line arguments
  bool t_flag = false; bool v_flag = false;
  bool a_flag = false; bool h_flag = false;
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
        std::cout << "DISPLAY HELP MESSAGE" << "\n";
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
  // Open file
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
  simulation.run_simulation();
  return 0;
}
