#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h> //strerror
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h> //waitpid
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

std::pair<int, pid_t> ChildProcess::InitProcess(bool blocking_out)
{
	std::lock_guard<std::mutex> guard(mutex);
	if(outputPipe > 0 && ProcessId > 0)
		return std::make_pair(outputPipe, ProcessId);
	
	logger->Log_fmt(debug, "Initializing process: %s\n", command.c_str());

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
	//TODO - Change capacity for pipefd[0]?
	//read max capacity from /proc/sys/fs/pipe-max-size
	//if(fcntl(pipefd[0], F_SETPIPE_SZ, {max size}) == -1) print errno
	if(blocking_out)
	{
		int flags = fcntl(pipefd[0], F_GETFD);
		if(flags != -1)
			fcntl(pipefd[0], F_SETFD, flags | O_NONBLOCK);
	}
	
	pid_t tProcId = fork();
	if(tProcId == 0) //Child
	{
		if(dup2(pipefd[1], STDOUT_FILENO) == -1)
			exit(EXIT_FAILURE);
		if(inputPipe)
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
		if(inputPipe)
			close(pipefd[1]); //close the write-only file descriptor
		else
			writePipe = pipefd[1];
		
		outputPipe = pipefd[0];
		processId = tProcId;
		logger->Log_fmt(debug, "Initialization complete for %s (%d)\n", command.c_str(), processId);
		return std::make_pair(outputPipe, processId);
	}
}

void ChildProcess::ShutdownProcess()
{
	std::lock_guard<std::mutex> guard(mutex);
	if(processId == 0)
		return;
	
	logger->Log_fmt(debug, "Stopping process: %s (%d)\n", command.c_str(), processId);
	
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
	inputPipe = 0;
	
	int ret = kill(processId, SIGTERM);
	int err = errno;
	if(ret)
	{
		if(err == ESRCH)
			processId = 0;
		else
			logger->Log_fmt(error, "Error killing process: %s (%d): %s\n", command.c_str(), processId, strerror(err));
	}
	else
	{
		waitpid(processId, nullptr, 0);
		logger->Log_fmt(debug, "Process stopped: %s (%d)\n", command.c_str(), processId);
		processId = 0;
	}
}

size_t ChildProcess::Write(const char* buf, const size_t len)
{
	if(!ProcessInitialized())
		throw std::runtime_error("Process not initialized");

	std::lock_guard<std::mutex> guard(mutex);
	if(!writePipe)
		throw std::runtime_error("No fd to write to");
	int ret = write(writePipe, (void*)buf, len);
	if(ret < 0)
	{
		ShutdownProcess();
		//throw std::runtime_error("");
	}
	return ret;
}

bool ChildProcess::ProcessInitialized()
{
	std::lock_guard<std::mutex> guard(mutex);
	return processId != 0;
}

void ChildProcess::SetInput(int fd)
{
	inputPipe = fd;
}

