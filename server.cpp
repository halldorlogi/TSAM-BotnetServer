#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <vector>
#include <ctime>
#include <sstream>

#define MAX_TIME_INTERVAL 180
using namespace std;

///A class that holds information about a client
class ClientInfo{
public:
    int socketVal, udpPort, tcpPort, lastRcvKA;
    string peerName;
    string userName;
    string listServerReply;
    bool hasUsername;
    bool isUser;

    //all clients need a peer name
    ClientInfo(int socketVal){
        hasUsername = false;
        isUser = false;
        peerName = "";
        userName = "";
        this->socketVal = socketVal;
        lastRcvKA = 0;
    }

    ClientInfo(int socketVal, string peerName, int time){
        hasUsername = false;
        isUser = false;
        this->peerName = peerName;
        userName = "";
        this->socketVal = socketVal;
        lastRcvKA = time;
    }

    ~ClientInfo(){

    }
};

//clients who have successfully connected with the correct sequence of portknocking
vector<ClientInfo*> clients;

//the server Id
string serverID = "V_GROUP_15_HLH";
int serverUdpPort = 4024;

///Get the amount of milliseconds elapsed since 00:00 Jan 1 1970 (I think)
int getTime(){
    int t = (int)time(0);
    return t;
}

int stillAliveTime = getTime();

void connectToServer(struct sockaddr_in serv_addr, string address, int tcpportno, int udpportno) {
    int externalSock = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *server = gethostbyname(address.c_str());

    char message[1000];

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(tcpportno);
    if(connect(externalSock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0){
        cout << "connection to " << server->h_name << " successful" << endl;
        strcpy(message, "CMD,,V_GROUP_15_HLH,ID\n");
        send(externalSock, message, strlen(message), 0);
        cout << "serv_addr is now: " << inet_ntop(AF_INET, &(serv_addr.sin_addr), message, 1000) << endl;
        int time = getTime();
        clients.push_back(new ClientInfo(externalSock, server->h_name, time));
        clients[clients.size()-1]->tcpPort = tcpportno;
        clients[clients.size()-1]->udpPort = udpportno;
    }
}

void validateSocket(int sockfd) {

    if (sockfd < 0) {
        cerr << "ERROR on binding socket" << endl;
        exit(0);
    }
}

///Get a timestamp that's readable (taken from cplusplus.com)
string getReadableTime(){
    time_t t;
    struct tm* timeinfo;
    char timestamp[30];

    time(&t);
    timeinfo =localtime(&t);

    strftime(timestamp, 30, "%c", timeinfo);
    return string(timestamp);
}

/// A function that removes the first word of the buffer and saves it as the firstWord string
void manageBuffer(char* buffer, string &firstWord) {
    
    // ### ATH SERVER CRASHAR EF BUFFER ER TÓMUR !!! ###
        string str = string(buffer);
        char delimit[] = ",";
        firstWord = strtok(buffer, delimit);
        str = str.substr(str.find_first_of(delimit)+1);
        strcpy(buffer, str.c_str());
}

void openUDPPort(int &UDPsock) {

    struct addrinfo hints, *servinfo, *p;
    int rv;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(NULL, "4024", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }
    
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((UDPsock = socket(p->ai_family, p->ai_socktype,
                              p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        
        if (::bind(UDPsock, p->ai_addr, p->ai_addrlen) == -1) {
            close(UDPsock);
            perror("listener: bind");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
    }
    
    freeaddrinfo(servinfo);
}

// ## beej.us ##
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

string listServersReply(){
    string str;
    /*if (clients.size() == 1) {
        str = "Our server list is empty!\n";
        return str;
    }*/
    for(int i = 0 ; i < (int)clients.size() ; i++){
        ClientInfo* c = clients[i];
        
        // !!!!!!!!! MUNA AÐ BREYTA EFTIR AÐ VIÐ FÖRUM LIVE - Á AÐ VERA '!c->isUser' SVO AÐ CLIENT FARI EKKI Á LISTANN !!!!!!!!
        
        if((c->isUser)){
            if(c->hasUsername){
                str += c->userName;
            }
            str += "," + c->peerName + "," + to_string(c->tcpPort) + "," + to_string(c->udpPort) + ";";
        }
    }
    str += "\n";
    return str;
}

// ### A FUNCTION TO MAKE SURE WE ONLY SEND OUR SERVER LIST IN CASE OF 'LISTSERVERS' ###
bool handleForeignLISTSERVERS(char* UDPBuffer) {
    
    string command;
    manageBuffer(UDPBuffer, command);
    if (command == "LISTSERVERS") {
        string package = listServersReply();
        bzero(UDPBuffer, strlen(UDPBuffer));
        strcpy(UDPBuffer, package.c_str());
        return true;
    }
    return false;
}

void listenForUDP(int &UDPsock, sockaddr_storage addr_udp, socklen_t udp_addrlen, char* TCPBuffer, char *UDPBuffer, char* remoteIP_udp) {
    cout << "Listening for UDP" << endl;
    int numbytes;
    //printf("listener: waiting to recvfrom...\n");
    
    if ((numbytes = recvfrom(UDPsock, UDPBuffer, 1000-1 , 0,
                             (struct sockaddr *)&addr_udp, &udp_addrlen)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    UDPBuffer[numbytes-1] = '\0';
    //printf("listener: got packet from %s\n",
           //inet_ntop(addr_udp.ss_family,
                    // get_in_addr((struct sockaddr *)&addr_udp),
                     //remoteIP_udp, sizeof remoteIP_udp));
    //printf("listener: packet contains \"%s\"\n", UDPBuffer);
    
    if (handleForeignLISTSERVERS(UDPBuffer)) {
        cout << "Sending buffer back to UDP port ! " << endl;
        sendto(UDPsock, UDPBuffer, strlen(UDPBuffer), 0, (struct sockaddr *) &addr_udp, udp_addrlen);
    }

}

/*void sendUDP(int &UDPsock, sockaddr_storage addr_udp, socklen_t udp_addrlen, char* UDPbuffer) {

    sendto(UDPsock, UDPbuffer, strlen(UDPbuffer), 0, (struct sockaddr *) &addr_udp, udp_addrlen);
    
}*/

///opens the given port on the given socket.
void openPort(struct sockaddr_in serv_addr, int sockfd, int portno) {

    cout << "Opening port: " << portno << endl;
    // Allow reusage of address
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0){
        cerr << ("setsockopt(SO_REUSEADDR) failed") << endl;
        exit(0);
    }
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = /*inet_addr("127.0.0.1");//*/INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Attempt to bind the socket to the host (portno) the server is currently running on
    if (::bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        cerr << "Error on binding" << endl;
        exit(0);
    }

    cout << "ip is " << inet_ntoa(serv_addr.sin_addr) << endl;
    cout << "port is " << htons(serv_addr.sin_port) << endl;
}

///Sends the buffer to all connected clients (except the original sender if alsoSendToSender is false)
void sendBufferToAll(ClientInfo* user, char* buffer, bool alsoSendToSender){
    buffer[strlen(buffer)] = '\0';

    for(int i = 0 ; i < (int)clients.size() ; i++){
        //check if this user is the sender and whether or not to send the buffer to him
        if((clients[i] !=  user || alsoSendToSender) && clients[i]->hasUsername){
            //test
            //cout << "sending \"" << buffer << "\" to " << user->userName << endl;

            send(clients[i]->socketVal, buffer, strlen(buffer), 0);
        }
    }
}

///disconnects a user from the server and removes them from the vector of users.
void leave(ClientInfo* user, char* buffer){
    close(user->socketVal);

    //check if the user had a username and create the appropriate string
    bzero(buffer, strlen(buffer));

    //find the user in the vector to remove him
    for(int i = 0 ; i < (int)clients.size() ; i++){
        if(clients[i] == user){

            //once he is found and has a username, the buffer is sent to othe users.
            if(user->hasUsername){
                sendBufferToAll(user, buffer, 0);
            }
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

//Function to check if the two token characters are in the beginning and end of the buffer, returns a string with the buffer content
string checkTokens(char* buffer) {
    string str = string(buffer);
    
    int sstr = str.size();
    
    if(str.find("01") == 0 && str.find("04") == sstr - 2)
    {
        str.erase(0,2);
        str.erase(str.end() - 2, str.end());
    }
    else
    {
        return "Tokens not valid";
    }
    
    return str;
}

///DECIDES WHAT TO DO BASED ON THE COMMAND RECEIVED FROM THE CLIENT
void handleConnection(char* buffer, int messageCheck, ClientInfo* user) {
    string command;
    string address;
    string tcpport;
    string udpport;
    string name;
    string toServerID;
    string fromServerID;
    string srvcmd;

    // get the command
    manageBuffer(buffer, command);

    if (command == "connectTo") {
        manageBuffer(buffer, address);
        manageBuffer(buffer, tcpport);
        manageBuffer(buffer, udpport);
        struct sockaddr_in serv_addr;
        bzero((char*)&serv_addr, sizeof(serv_addr));
        connectToServer(serv_addr, address, stoi(tcpport), stoi(udpport));
    }
    
    /*if (command == "LISTSERVERS") {
        string portno;
        manageBuffer(buffer, portno);
        bzero(UDPBuffer, strlen(UDPBuffer));
        strcpy(UDPBuffer, "LISTSERVERS");
        cout << "Sending LISTSERVERS to port: " << portno << endl;
        cout << "Buffer is now: " << UDPBuffer << endl;
        sendUDP(UDPsock, addr_udp, udp_addrlen, UDPBuffer);
    }*/

    if (command == "CMD") {
        manageBuffer(buffer, toServerID);
        manageBuffer(buffer, fromServerID);
        manageBuffer(buffer, srvcmd);
        if (srvcmd == "ID\n") {
            // ### FIRST CASE: CMD, ,GROUP_ID,<COMMAND> ###
            // ### SECOND CASE: CMD,,GROUP_ID<COMMAND> ###
            // ÞARF MÖGULEGA AÐ LAGA - VILJUM GETA VITAÐ HVORT ER HVAÐ
                if (toServerID == " " || toServerID == fromServerID) {
                cout << fromServerID << " has requested to connect" << endl;
                cout << "Sending RSP..." << endl;
                bzero(buffer, strlen(buffer));
                strcpy(buffer, "RSP,");
                strcat(buffer, fromServerID.c_str());
                strcat(buffer, ",");
                strcat(buffer, serverID.c_str());
                strcat(buffer, ",");
                strcat(buffer, serverID.c_str());
                strcat(buffer, "\n");
                cout << buffer << endl;
                send(user->socketVal, buffer, strlen(buffer), 0);
                }
        }
        
        if (srvcmd.compare("KEEPALIVE") == 0) {
            user->lastRcvKA = getTime();
        }
    }

    if (command == "RSP") {

        manageBuffer(buffer, toServerID);
        manageBuffer(buffer, fromServerID);
        manageBuffer(buffer, srvcmd);
        if(toServerID == serverID){
            if(!user->hasUsername){
                bool fromThisServer = true;
                /*for(int i = 0 ; (int)clients.size() ; i++){
                    if(clients[i]->userName.compare(fromServerID) == 0){
                        ///We DON'T know the serverID of the socket who sent us the RSP but we DO know the server who originally sent it
                        fromServerID = false;
                        break;
                    }
                }*/
                if(fromThisServer == true){
                    ///We DON'T know the serverID of this socket and we DON't know any server of this name. This must be this socket's serverID
                    user->userName = fromServerID;
                    user->hasUsername = true;
                }
            }
            else{
                if(user->userName.compare(fromServerID) == 0){
                    ///we DO know the serverID of this socket and it IS from that server
                }
                else{
                    bool fromKnownServer = false;
                    for(int i = 0 ; (int)clients.size() ; i++){
                        if(clients[i]->userName.compare(fromServerID) == 0){
                            ///the RSP is sent from another server we DO know
                            fromKnownServer = true;
                        }
                    }
                    if(fromKnownServer == false){
                        ///This RSP is from a server we DON'T know.
                    }
                }
            }
        }
        cout << "Response received: " << srvcmd << endl;
        cout << "Adding " << fromServerID << " to network" << endl;

    }

    if (command == "CONNECT" && user->isUser) {
        //set the username
        user->userName = string(buffer);
        user->hasUsername = true;
        user->tcpPort = 4020;
        user->udpPort = 4021;

        //send the user a confirmation that he is now connected to the server
        strcpy(buffer, "You are now connected to the server as \"");
        strcat(buffer, &user->userName[0]);
        strcat(buffer, "\"\n");
        send(user->socketVal, buffer, strlen(buffer), 0);

        //alert others that this user has connected.
        /*bzero(buffer, strlen(buffer));
        strcpy(buffer, &user->userName[0]);
        strcat(buffer, " has just connected to the server\n");
        sendBufferToAll(user, buffer, 0);*/
    }
    
    if (command == "CONNECT" && !user->isUser) {
        user->userName = clients[(int)clients.size()-1]->userName;
        cout << "New user connected" << endl;
    }

    //send the current server id to the user
    else if(command == "ID"){
        bzero(buffer, strlen(buffer));
        strcpy(buffer, serverID.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
    }

    //send the user a list of clients who are connected
    else if (command == "WHO" && user->hasUsername) {
        bzero(buffer, strlen(buffer));
        //add users who have a username (and are therefore connected) to the buffer
        for (int i = 0; i < (int)clients.size(); i++) {
            if(clients[i]->hasUsername){
                stringstream ss;
                stringstream ss1;
                ss << (i + 1);
                string number;
                ss >> number;
                int time = clients[i]->lastRcvKA;
                ss1 << time;
                string t = ss1.str();
                string str = "User " + number + ": " + clients[i]->userName + "\n";
                strcat(buffer, str.c_str());
            }
        }
        send(user->socketVal, buffer, strlen(buffer), 0);
    }

    //remove the user from the server and alert others that that user has left (if that user had a username).
    //a user who hasn't used the CONNECT command can still use the LEAVE command
    else if(command == "LEAVE"){
        if(user->hasUsername){
            name = user->userName;
            strcpy(buffer, "User \"");
            strcat(buffer, &name[0]);
            strcat(buffer, "\" has disconnected from the server\n");
        }
        leave(user, buffer);
    }

    else if(command == "V_GROUP_15_I_am_your_father"){
        user->isUser = true;
    }

    //sends a message to a specific or all users
    else if(command == "MSG" && user->hasUsername) {

        //since MSG is a 2 word command, we need the second word
        manageBuffer(buffer, name);

        char tempBuffer[strlen(buffer)] ;
        strcpy(tempBuffer, buffer);

        if(name == "ALL"){

            //add the username of the sender and send it out.
            strcpy(buffer, &user->userName[0]);
            strcat(buffer, ": ");
            strcat(buffer, tempBuffer);
            strcat(buffer, "\n");
            sendBufferToAll(user, buffer, 0);
        }

        //you can't send a message to yourself
        else if(user->userName == name){
            strcpy(buffer, "That's you, dummy!\n");
            send(user->socketVal, buffer, strlen(buffer), 0);
        }

        //try to find the username specified
        else {
            
            for(int i = 0 ; i < (int)clients.size() ; i++){

                //if we find it, send the message to that user.
                if(clients[i]->userName == name){
                    strcpy(buffer, "(private) ");
                    strcat(buffer, &user->userName[0]);
                    strcat(buffer, ": ");
                    strcat(buffer, tempBuffer);
                    strcat(buffer, "\n");

                    send(clients[i]->socketVal, buffer, strlen(buffer), 0);
                    break;
                }

                //if we don't, we alert the user that there is no user with that name
                if(i == (int)clients.size() - 1){
                    strcpy(buffer, "No user with name \"");
                    strcat(buffer, &name[0]);
                    strcat(buffer, "\"\n");
                    send(user->socketVal, buffer, strlen(buffer), 0);
                }
            }
        }
    }
}

/*bool isFull() {
    int counter = 0;
    for(int i = 0 ; (int)clients.size() ; i++) {
        if(!clients[i]->isUser){
            counter++;
        }
    }
    if(counter == 5){
        return true;
    }
    else{
        return false;
    }
}*/

int main(int argc, char* argv[]) {
///####################### VARIABLE INITIALIZATION #######################

    int listeningSock, UDPsock, newSock, messageCheck, socketVal, maxSocketVal;
    struct sockaddr_in serv_addr, cli_addr;
    struct sockaddr_storage addr_udp;
    socklen_t cli_addrlen, serv_addrlen, udp_addrlen;
    char remoteIP_udp[INET6_ADDRSTRLEN];
    
    cout << "The server ID is: " << serverID << endl;

    //the message buffer
    char message[1000];
    char UDPbuffer[1000];
    //char UDPbuffer_copy[1000];
    char localip[80];

    // a set of file descriptors
    fd_set masterFD, read_fds;

///##################### SETTING UP MAIN SOCKETS ########################

    //create the sockets
    /*knockSock1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    validateSocket(knockSock1);
    knockSock2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    validateSocket(knockSock2);*/
    
    // ### Setting up TCP socket ###
    listeningSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    validateSocket(listeningSock);
    
    // ########################################################################     Setting up UDP scoket ########################################################################
    
    //UDPsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //validateSocket(UDPsock);

    //socket settings
    bzero((char*)&serv_addr, sizeof(serv_addr));
    bzero((char*)&addr_udp, sizeof(addr_udp));

    //openPort(serv_addr, knockSock1, KNOCK_PORT_1);
    //openPort(serv_addr, knockSock2, KNOCK_PORT_2);
    openPort(serv_addr, listeningSock, 4023);
    openUDPPort(UDPsock);

    //openUDPPort(addr_udp, udp_addrlen, UDPsock, 4024);

    //listen for activity
   // listen(knockSock1, 5);
   // listen(knockSock2, 5);
    listen(listeningSock, 5);

    cli_addrlen = sizeof(cli_addr);
    serv_addrlen = sizeof(serv_addr);
    udp_addrlen = sizeof(addr_udp);
    
    inet_ntop(AF_INET, &serv_addr.sin_addr, localip, 80);

///##################### RUNNING BODY OF SERVER ######################
    while(true){
        // KEEPALIVE Operations
        int t = getTime();
        // ### IF CLIENT HAS NOT SENT 'KEEPALIVE' IN THE LAST 180 SECONDS, DROP THEM ###
		if(clients.size() > 0){
			for(int i = 0 ; i < (int)clients.size() ; i++){
				if(t - clients[i]->lastRcvKA > MAX_TIME_INTERVAL && !clients[i]->isUser) {
                    leave(clients[i], message);
				}
			}
        }
        
        // ### IF OUR SERVER HAS NOT SENT 'KEEPALIVE' IN 70 SECONDS, SEND 'KEEPALIVE' TO EVERYONE EXCEPT OUR CLIENT
        if (t-stillAliveTime > 70) {
            for(int i = 0; i < (int)clients.size(); i++) {
                if (!clients[i]->isUser) {
                    bzero(message, strlen(message));
                    strcpy(message, "CMD,");
                    strcat(message, clients[i]->userName.c_str());
                    strcat(message, ",");
                    strcat(message, serverID.c_str());
                    strcat(message, ",KEEPALIVE\n");
                    send(clients[i]->socketVal, message, strlen(message), 0);
                    cout << "KEEPALIVE MESSAGE SENT" << endl;
                }
            }
            stillAliveTime = getTime();
        }
        
        //reset the fd_set
        FD_ZERO(&masterFD);
        FD_ZERO(&read_fds);
        //FD_SET(knockSock1, &masterFD);
        //FD_SET(knockSock2, &masterFD);
        FD_SET(listeningSock, &masterFD);
        FD_SET(UDPsock, &masterFD);

        maxSocketVal = listeningSock;

        //add active sockets to set
        for(int i = 0 ; i < (int)clients.size() ; i++){
            socketVal = clients[i]->socketVal;
            if(socketVal > 0){
                FD_SET(socketVal, &masterFD);
            }
            if(socketVal > maxSocketVal){
                maxSocketVal = socketVal;
            }
        }
        
        timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        
        //wait for something to happen on some socket. Will wait forever if nothing happens.
        if (select(maxSocketVal + 1, &masterFD, NULL, NULL, &tv) > 0) {

            //if something happens on the listening socket, someone is trying to connet to the server.
            if(FD_ISSET(listeningSock, &masterFD)){
                
                newSock = accept(listeningSock, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
                validateSocket(newSock);
                getpeername(newSock, (struct sockaddr * )&cli_addr, (socklen_t *)&cli_addrlen);
                int time = getTime();
                clients.push_back(new ClientInfo(newSock, inet_ntoa(cli_addr.sin_addr), time));

                cout << "Sending message to other server: ";

                strcpy(message, "CMD,,V_GROUP_15_HLH,ID\n");
                cout << message << endl;

                send(newSock, message, strlen(message), 0);

                /*else{
                    close(newSock);
                }*/
            }

            //if something happens on any other socket, it's probably a message

            else if(FD_ISSET(UDPsock, &masterFD)){
                
                listenForUDP(UDPsock, addr_udp, udp_addrlen, message, UDPbuffer, remoteIP_udp);
            }

            for(int i = 0 ; i < (int)clients.size() ; i++){
                socketVal = clients[i]->socketVal;

                if(FD_ISSET(socketVal, &masterFD)){
                    bzero(message, sizeof(message));
                    //cout << "Reading message from connected peer: ";
                    messageCheck = read(socketVal, message, sizeof(message));
                    message[messageCheck] = '\0';
                    if(messageCheck != 0){
                        //cout << message << endl;
                        handleConnection(message, messageCheck, clients[i]);
                    }

                    else{
                        if(clients[i]->hasUsername){
                            strcpy(message, "User \"");
                            strcat(message, &clients[i]->userName[0]);
                            strcat(message, "\" has disconnected from the server\n");
                        }
                        leave(clients[i], message);
                    }
                }
            }
        }
    }
    return 0;
}
