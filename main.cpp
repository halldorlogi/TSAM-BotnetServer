//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#define BACKLOG  5          // Allowed length of queue of waiting connections

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
public:
    int sock, udpsock;              // socket of client connection
    std::string name, ip;           // Limit length of name of client's user
    bool isClient;

    Client(int socket) : sock(socket) {}

    ~Client() {}           // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table,
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client*> clients; // Lookup table for per Client information
char UDPbuffer[1024];
std::string serverID = "V_GROUP_15";
std::string myRoutingTable = serverID;
std::string knownServers;
int UDPsock;

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno)
{
    struct sockaddr_in sk_addr;   // address settings for bind()
    int sock;                     // socket opened for this port
    int set = 1;                  // for setsockopt

    // Create socket for connection

    if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("Failed to open socket");
        return(-1);
    }

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SO_REUSEADDR:");
    }

    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family      = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port        = htons(portno);

    // Bind to socket to listen for connections from clients

    if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        perror("Failed to bind to socket:");
        return(-1);
    }
    else
    {
        return(sock);
    }
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void disconnectClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
    // Remove client from the clients list
    clients.erase(clientSocket);

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    if(*maxfds == clientSocket)
    {
        for(auto const& p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(clientSocket, openSockets);
}

void sendToUDP(std::string str, Client* client){
    struct sockaddr_in sk_addr;
    socklen_t len = sizeof(sk_addr);
    int sock;
    const char* ip = client->ip.c_str();
    const char* buffer = str.c_str();

    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		perror("failed to make udp socket");
		exit(0);
    }

    memset((char *) &sk_addr, 0, sizeof(sk_addr));
    sk_addr.sin_family = AF_INET;
    sk_addr.sin_port = htons(client->udpsock);
    inet_aton(ip, &sk_addr.sin_addr);

    sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *) &sk_addr, len);

    close(sock);
}


// Process command from client on the server

int clientCommand(int clientSocket, fd_set *openSockets, int *maxfds,
                  char *buffer, bool isClient)
{
    std::vector<std::string> tokens;
    std::string token;

    // Split command from client into tokens for parsing
    std::stringstream stream(buffer);

    while(stream >> token)
    { tokens.push_back(token); }

    if((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 2))
    {
        clients[clientSocket]->name = tokens[1];
        clients[clientSocket]->isClient = true;
    }
    else if(tokens[0].compare("LEAVE") == 0 && isClient == true)
    {
        // Close the socket, and leave the socket handling
        // code to deal with tidying up clients etc. when
        // select() detects the OS has torn down the connection.

        disconnectClient(clientSocket, openSockets, maxfds);
    }
    else if(tokens[0].compare("WHO") == 0)
    {
        std::cout << "Who is logged on" << std::endl;
        std::string msg;

        for(auto const& names : clients)
        {
            msg += names.second->name + ",";

        }
        // Reducing the msg length by 1 loses the excess "," - which
        // granted is totally cheating.
        send(clientSocket, msg.c_str(), msg.length()-1, 0);

    }
    // This is slightly fragile, since it's relying on the order
    // of evaluation of the if statement.
    else if((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0) && isClient == true)
    {
        std::string msg;
        for(auto i = tokens.begin()+2; i != tokens.end(); i++)
        {
            msg += *i + " ";
        }

        for(auto const& pair : clients)
        {
            send(pair.second->sock, msg.c_str(), msg.length(),0);
        }
    }
    else if(tokens[0].compare("MSG") == 0)
    {
        for(auto const& pair : clients)
        {
            if(pair.second->name.compare(tokens[1]) == 0)
            {
                std::string msg;
                for(auto i = tokens.begin()+2; i != tokens.end(); i++)
                {
                    msg += *i + " ";
                }
                send(pair.second->sock, msg.c_str(), msg.length(),0);
            }
        }
    }
    else if(tokens[0].compare("LISTSERVERS") == 0 && isClient == false)
    {
        knownServers = serverID;
        int udpSock;

        for(auto const& pair : clients)
        {
            knownServers += pair.second->name + "," + pair.second->ip + "," + std::to_string(pair.second->sock) + "," + std::to_string(pair.second->udpsock) + ";";
        }
        for(auto const& pair : clients)
        {
            if(clientSocket == pair.second->sock)
            {
                 sendToUDP(knownServers, pair.second);
            }
        }
    }
    else if(tokens[0].compare("LISTROUTES") && isClient == false)
    {
        /// Ask every server for LISTSERVER and somehow make a map from that? also a timestamp
    }
    else if(tokens[0].compare("CMD") && isClient == false)
    {
        ///Parse the list at commas ","
        ///if the command is for us, process the command and send a RSP back
        ///if the command is not for us, see if we know the server and send it to that server.
        ///if we don't know the server, send it to a random server other than the one we received it from
    }
    else if(tokens[0].compare("RSP") && isClient == false)
    {
        ///almost the same as CMD
    }
    else if(tokens[0].compare("FETCH"))
        ///send our hash strings (which we haven't received yet) to the client
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }


}

void listenForUDP(int portno) {
    struct sockaddr_in servaddr;
    socklen_t len = sizeof(servaddr);

    if((UDPsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("failed to make udp socket");
        exit(0);
    }

    memset((char *) &servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(portno);

    if(bind(UDPsock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(0);
    }

    while(true) {
        memset(UDPbuffer, 0, sizeof(UDPbuffer));

        int n = recvfrom(UDPsock, (char *)UDPbuffer, 1024, 0, (struct sockaddr *) &servaddr, &len);
        UDPbuffer[n] = '\0';
        std::cout << UDPbuffer << std::endl;
    }
}

int main(int argc, char* argv[])
{
    bool finished;
    int listenSock;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets
    fd_set readSockets;             // Socket list for select()
    fd_set exceptSockets;           // Exception socket list
    int maxfds;                     // Passed to select() as max fd in set
    struct sockaddr_in server, client;
    socklen_t serverLen, cliLen;
    char buffer[1025];              // buffer for reading from clients

    if(argc != 3)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    int udpPort = atoi(argv[2]);
    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1]));
    printf("Listening on port: %d\n", listenSock);

    if(listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else
        // Add listen socket to socket set
    {
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    serverID += "," + std::string(inet_ntoa(server.sin_addr)) + "," + argv[1] + "," + argv[2] + ";";

    std::thread udpThread(listenForUDP, udpPort);

    ///Þegar serverinn kveikir á sér þarf hann að setja inn í clients mappið einhvern lista að clientum(oðrum serverum) sem hann veit um
    ///Það þarf EKKI að scanna eftir öðrum serrverum á meðan hann er uppi. Bara þegar hann kveikir á sér
    ///spurning að nota system(w) en ég er ekki kominn lengra með það...

    finished = false;

    while(!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // Accept  any new connections to the server
            if(FD_ISSET(listenSock, &readSockets))
            {
                clientSock = accept(listenSock, (struct sockaddr *)&server,
                                    &serverLen);

                FD_SET(clientSock, &openSockets);
                maxfds = std::max(maxfds, clientSock);

                clients[clientSock] = new Client(clientSock);
                n--;

                printf("Client connected on server: %d\n", clientSock);
            }
            // Now check for commands from clients
            while(n-- > 0)
            {
                for(auto const& pair : clients)
                {
                    Client *client = pair.second;

                    if(FD_ISSET(client->sock, &readSockets))
                    {
                        if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            printf("Client closed connection: %d", client->sock);
                            close(client->sock);

                            disconnectClient(client->sock, &openSockets, &maxfds);

                        }
                        else
                        {
                            std::cout << buffer << std::endl;
                            clientCommand(client->sock, &openSockets, &maxfds,
                                          buffer, client->isClient);
                        }
                    }
                }
            }
        }
    }
}
