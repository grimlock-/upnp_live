#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h> //strerror
#include <errno.h>
#include <sys/wait.h>
#include "ChildProcess.h"
#include "Exceptions.h"

using namespace upnp_live;

ChildProcess::ChildProcess(std::vector<std::string>& args):
command(args[0]),
arguments(std::move(args))
{
	if(command.empty() || arguments.size() == 0)
		throw std::invalid_argument("Unspecified command or arguments");
	logger = Logger::GetLogger();
}

std::pair<int, pid_t> ChildProcess::InitProcess()
{
	std::lock_guard<std::mutex> guard(mutex);
	if(outputPipe != 0)
		throw AlreadyInitializedException();
	
	logger->Log_cc(debug, 3, "Initializing process: ", command.c_str(), "\n");

	//Set up array with nullptr at end
	std::size_t tArgsSize = arguments.size() + 1;
	char* tArgs[tArgsSize];
	for(std::size_t i = 0; i < arguments.size(); i++)
	{
		tArgs[i] = const_cast<char*>(arguments[i].c_str());
	}
	tArgs[tArgsSize-1] = nullptr;

	//Fork
	int pipefd[2];
	if(pipe(pipefd) == -1)
		throw std::runtime_error("Failed to create data pipe");
	
	//TODO - Change capacity for pipefd[0]
	//read max capacity from /proc/sys/fs/pipe-max-size
	//if(fcntl(pipefd[0], F_SETPIPE_SZ, {max size}) == -1) print errno
	
	pid_t tProcId = fork();
	if(tProcId == 0) //Child
	{
		if(dup2(pipefd[1], STDOUT_FILENO) == -1)
			exit(EXIT_FAILURE);
		if(inputPipe != 0)
		{
			if(dup2(inputPipe, STDIN_FILENO) == -1)
				exit(EXIT_FAILURE);
		}
		
		close(pipefd[0]);
		execvp(command.c_str(), const_cast<char** const>(tArgs));
		exit(EXIT_FAILURE); //Exit in case exec fails
	}
	else //Parent
	{
		if(inputPipe != 0)
			close(pipefd[1]); //close the write-only file descriptor
		else
			writePipe = pipefd[1];
		
		outputPipe = pipefd[0];
		processId = tProcId;
		logger->Log_cc(debug, 5, "Initialization complete for ", command.c_str(), "(", std::to_string(processId).c_str(), ")\n");
		return std::make_pair(outputPipe, processId);
	}
}

void ChildProcess::ShutdownProcess()
{
	std::lock_guard<std::mutex> guard(mutex);
	if(processId == 0)
		return;
	
	logger->Log_cc(debug, 5, "Stopping process: ", command.c_str(), "(", std::to_string(processId).c_str(), ")\n");
	
	if(writePipe != 0)
	{
		close(writePipe);
		writePipe = 0;
	}
	if(outputPipe != 0)
	{
		close(outputPipe);
		outputPipe = 0;
	}
	
	int ret = kill(processId, SIGTERM);
	int err = errno;
	if(ret)
	{
		if(err == ESRCH)
			outputPipe = 0;
		else
			logger->Log_cc(error, 7, "Error killing process: ", command.c_str(), "(", std::to_string(processId).c_str(), "): ", strerror(err), "\n");
	}
	else
	{
		waitpid(processId, nullptr, 0);
		logger->Log_cc(debug, 5, "Process stopped: ", command.c_str(), "(", std::to_string(processId).c_str(), ")\n");
		processId = 0;
	}
}


bool ChildProcess::ProcessInitialized()
{
	std::lock_guard<std::mutex> guard(mutex);
	return processId != 0;
}

