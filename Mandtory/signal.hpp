#pragma once

#include <csignal>
#include <cstring>
#include <iostream>

extern bool g_running;

void signalHandler(int sig);
void setupSignals();