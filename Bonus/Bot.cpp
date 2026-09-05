#include "Bot.hpp"

volatile sig_atomic_t g_running = 1;

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

static void setupBotSignals()
{
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

Bot::Bot() : sock_fd(-1), port(0),
    authenticated(false), registered(false), in_channel(false)
{
    std::memset(&server_addr, 0, sizeof(server_addr));
}

Bot::Bot(const std::string& host, int port, const std::string& password,
           const std::string& channel, const std::string& nickname)
    : sock_fd(-1), host(host), port(port), password(password),
      channel(channel), nickname(nickname),
      username("bot"), realname("IRC Bot Bot"),
      authenticated(false), registered(false), in_channel(false)
{
    std::memset(&server_addr, 0, sizeof(server_addr));
}

Bot::~Bot()
{
    if (sock_fd != -1)
    {
        std::string quitMsg = "QUIT :Bye!\r\n";
        queueRaw("QUIT :Bye!\r\n");
        sendPendingData();
        close(sock_fd);
    }
}

void Bot::setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags == -1)
    	throw std::runtime_error("fcntl F_GETFL failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    	throw std::runtime_error("fcntl F_SETFL failed");
}

void Bot::createSocket()
{
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
        throw std::runtime_error("Failed to create socket");
    setNonBlocking(sock_fd);
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());
}

void Bot::connectToServer()
{
    int ret = connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1 && errno != EINPROGRESS)
    {
        close(sock_fd);
        sock_fd = -1;
        throw std::runtime_error("Failed to connect to server");
    }
    struct pollfd pfd;
    pfd.fd = sock_fd;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    int activity = poll(&pfd, 1, 5000);

    if (activity <= 0)
    {
        close(sock_fd);
        sock_fd = -1;
        throw std::runtime_error("Connection timed out");
    }
    int err;
    socklen_t err_len = sizeof(err);
    if (getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &err, &err_len) == -1 || err != 0)
    {
        close(sock_fd);
        sock_fd = -1;
        throw std::runtime_error("Connection failed");
    }
    std::cout << "Bot: Connected to " << host << ":" << port << std::endl;
}

void Bot::queueRaw(const std::string& msg)
{
    sendBuffer += msg;
    std::cout << "Bot >> " << msg;
}

bool Bot::hasPendingData() const
{
    return (!sendBuffer.empty());
}

void Bot::sendPendingData()
{
    if (sendBuffer.empty())
        return;

    ssize_t sent = send(sock_fd, sendBuffer.c_str(), sendBuffer.size(), 0);

    if (sent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "Bot: send failed\n";
        g_running = 0;
        return;
    }
    if (sent == 0)
        return;
    sendBuffer.erase(0, (size_t)sent);
}

bool Bot::receiveData()
{
    char buf[1024];
    ssize_t bytes = recv(sock_fd, buf, sizeof(buf) - 1, 0);

    if (bytes <= 0)
    {
        if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
            return false;
        return true;
    }
    buf[bytes] = '\0';
    buffer += buf;
    size_t pos;

    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 2);
        std::cout << "Bot << " << line << std::endl;
        handleMessage(line);
    }
    while ((pos = buffer.find("\n")) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        buffer.erase(0, pos + 1);
        std::cout << "Bot << " << line << std::endl;
        handleMessage(line);
    }
    return true;
}

void Bot::authenticate()
{
    queueRaw("PASS " + password + "\r\n");
    queueRaw("NICK " + nickname + "\r\n");
    queueRaw("USER " + username + " 0 * :" + realname + "\r\n");
    authenticated = true;
}

void Bot::joinChannel()
{
    queueRaw("JOIN " + channel + "\r\n");
    std::cout << "Bot: Joining channel " << channel << std::endl;
}

void Bot::handleMessage(const std::string& line)
{
    if (line.find("PING") == 0)
    {
        std::string token;
        if (line.size() > 5)
            token = line.substr(5);
        queueRaw("PONG" + token + "\r\n");
        std::cout << "Bot: PONG response sent" << std::endl;
        return;
    }
    if (line.find(" 001 ") != std::string::npos)
    {
        std::cout << "Bot: Successfully registered" << std::endl;
        registered = true;
        joinChannel();
        return;
    }
    if (line.empty() || line[0] != ':')
        return;
    size_t spacePos = line.find(' ');
    if (spacePos == std::string::npos)
        return;
    std::string sender = line.substr(1, spacePos - 1);
    size_t ex = sender.find('!');

    if (ex != std::string::npos)
        sender.erase(ex);
    if (line.find("JOIN " + channel) != std::string::npos &&
        sender == nickname)
    {
        in_channel = true;
        sendMessage(channel, "Hello everyone! I am " + nickname + ", your friendly IRC bot.");
        sendMessage(channel, "Type !help to see available commands.");
        return;
    }

    if (line.find("PRIVMSG") != std::string::npos)
    {
        std::istringstream ss(line);
        std::string prefix, cmd, target;
        ss >> prefix >> cmd >> target;
        std::string msg;
        size_t colonPos = line.find(" :", spacePos);
        if (colonPos != std::string::npos)
            msg = line.substr(colonPos + 2);
        if (msg.empty())
            return;
        bool isDirect = (target == nickname);
        bool inChannelMsg = (target == channel);
        if (!isDirect && !inChannelMsg)
            return;
        if (msg[0] == '!')
            parseAndRespond(sender, msg);
        return;
    }
    if (line.find("433") != std::string::npos 
        || line.find("ERR_NICKNAMEINUSE") != std::string::npos)
    {
        nickname += "_";
        queueRaw("NICK " + nickname + "\r\n");
        std::cout << "Bot: Nickname in use, trying " << nickname << std::endl;
        return;
    }
}

void Bot::parseAndRespond(const std::string& sender, const std::string& message)
{
    std::istringstream ss(message);
    std::string cmd;
    ss >> cmd;
    std::string args;
    size_t spacePos = message.find(' ');

    if (spacePos != std::string::npos && spacePos + 1 < message.size())
        args = message.substr(spacePos + 1);

    if (cmd == "!help")
        cmdHelp(sender);
    else if (cmd == "!ping")
        cmdPing(sender);
    else if (cmd == "!time")
        cmdTime(sender);
    else if (cmd == "!echo")
        cmdEcho(sender, args);
    else
        sendMessage(sender, "Unknown command. Type !help to see available commands.");
}

void Bot::sendMessage(const std::string& target, const std::string& text)
{
    queueRaw("PRIVMSG " + target + " :" + text + "\r\n");
}

void Bot::cmdHelp(const std::string& sender)
{
    sendMessage(sender, "Available commands:");
    sendMessage(sender, "  !help     - Show this help message");
    sendMessage(sender, "  !ping     - Check if bot is alive");
    sendMessage(sender, "  !time     - Get current server time");
    sendMessage(sender, "  !echo <text>  - Echo back your text");
}

void Bot::cmdPing(const std::string& sender)
{
    sendMessage(sender, "Pong! I'm alive.");
}

void Bot::cmdTime(const std::string& sender)
{
    std::time_t now = std::time(NULL);
    std::string timeStr = std::ctime(&now);
    if (!timeStr.empty() && timeStr[timeStr.size() - 1] == '\n')
        timeStr.erase(timeStr.size() - 1);
    sendMessage(sender, "Current time: " + timeStr);
}

void Bot::cmdEcho(const std::string& sender, const std::string& args)
{
    if (args.empty())
    {
        sendMessage(sender, "Usage: !echo <text>");
        return;
    }
    sendMessage(sender, args);
}

void Bot::run()
{
    setupBotSignals();
    createSocket();
    connectToServer();
    authenticate();

    struct pollfd pfd;
    pfd.fd = sock_fd;
    pfd.events = POLLIN | POLLOUT;
    pfd.revents = 0;

    std::cout << "Bot: Starting main loop..." << std::endl;
    while (g_running)
    {
        pfd.events = POLLIN | (hasPendingData() ? POLLOUT : 0);
        int activity = poll(&pfd, 1, 1000);
        if (activity < 0)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "Bot: Poll error" << std::endl;
            break;
        }
        if (activity == 0)
            continue;
        short revents = pfd.revents;
        pfd.revents = 0;

        if (revents & POLLIN)
        {
            if (!receiveData())
            {
                std::cout << "Bot: Server disconnected" << std::endl;
                break;
            }
        }
        if (revents & POLLOUT)
            sendPendingData();
        if (revents & (POLLHUP | POLLERR | POLLNVAL))
        {
            std::cout << "Bot: Socket error, shutting down" << std::endl;
            break;
        }
    }
    std::cout << "Bot: Shutting down..." << std::endl;
}
