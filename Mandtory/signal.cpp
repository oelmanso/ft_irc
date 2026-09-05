#include "signal.hpp"

bool g_running = true;

void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        g_running = false;
    }
    else if (sig == SIGTSTP)
    {
        std::cout << "\nServer suspended\n";
        raise(SIGSTOP);
    }
}

void setupSignals()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}