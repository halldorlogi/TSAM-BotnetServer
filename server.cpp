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
#define serverID "V_GROUP_15"

using namespace std;

// ### A CLASS THAT HOLDS INFORMATION ABOUT A CLIENT (OTHER SERVERS)
class ClientInfo{
public:
    int socketVal, tcpPort, lastRcvKA;
    string peerName;
    string userName;
    //string listServerReply;
    bool hasUsername;
    bool isOurClient;
    
    /*//all clients need a peer name                  -- Is this being used at all? --
    ClientInfo(int socketVal, string id){
        hasUsername = true;
        isOurClient = false;
        peerName = "";
        userName = id;
        this->socketVal = socketVal;
        lastRcvKA = 0;
        tcpPort = 0;
        udpPort = 0;
    }*/
    
    ClientInfo(int socketVal, string peerName, int time) {
        hasUsername = false;
        isOurClient = false;
        this->peerName = peerName;
        userName = "";
        this->socketVal = socketVal;
        lastRcvKA = time;
        tcpPort = 0;
    }
    
    ~ClientInfo(){
        
    }
};

// ### A CONTAINER FOR SUCCESSFULLY CONNECTED SERVER CLIENTS ###
vector<ClientInfo*> clients;
int TCPport;
int UDPport;
string localIP;

// ### GET ELAPSED TIME SINCE 00:00 JAN 1. 1970 IN MS ###
int getTime(){
    int t = (int)time(0);
    return t;
}

string addTokens(char* buffer) {
    char stuffedBuffer[1000];
    int counter = 0;
    string tokenString = "";
    
    string str = string(buffer);
    string start = "0x01";
    string end = "0x04";
    
    int pos = str.find(end);
    
    while(pos < str.size())
    {
        str.insert(pos, end);
        pos = str.find(end, pos + end.size()*2);
    }
    
    tokenString = start + str + end;
    return tokenString;
}

// ### A VARIABLE USED IN 'KEEPALIVE ###
int stillAliveTime = getTime();

// ### CONNECT TO OTHER SERVER AND ADD THE SERVER INFORMATION TO CLIENTS ###
void connectToServer(struct sockaddr_in serv_addr, string address, int tcpportno) {
    
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
        cout << "Connection to " << server->h_name << " successful" << endl;
        strcpy(message, "CMD,,V_GROUP_15,ID");
        string str = addTokens(message);
        strcpy(message, str.c_str());
        send(externalSock, message, strlen(message), 0);
        //cout << "serv_addr is now: " << inet_ntop(AF_INET, &(serv_addr.sin_addr), message, 1000) << endl;
        int time = getTime();
        clients.push_back(new ClientInfo(externalSock, server->h_name, time));
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

// ### TÓK ÞETTA HELD ÉG BEINT AF BEEJ - ÞYRFTI AÐ REFERENCE EÐA AÐLAGA ###
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

string listServersReply(string localIP, string toServerID, string fromServerID){
    string str;
    /*if (clients.size() == 1) {
     str = "Our server list is empty!\n";
     return str;
     }*/
    //string ip(localIP);
   // cout << "Local ip is: " << ip << endl;
   // stringstream ss;
    //ss << "RSP" << "," << toServerID << "," << fromServerID << "," << serverID << "," << ip.c_str() << "," << TCPport << ";";
    //ss << "RSP" << "," << toServerID << "," << fromServerID << "," << serverID << "," << "130.208.243.61" << "," << TCPport << ";";
    //ss >> str;
    for(int i = 0 ; i < (int)clients.size() ; i++){
        ClientInfo* c = clients[i];
        if((!c->isOurClient)){
            if(c->hasUsername){
                str += c->userName;
            }
            str += "," + c->peerName + "," + to_string(c->tcpPort) + ";";
        }
    }
    return str;
}

string listServersReplyUDP(string localIP) {
    string str;
    string ip(localIP);
    stringstream ss;
    //ss << serverID << "," << ip.c_str() << "," << TCPport << ";";
    //ss << serverID << "," << "130.208.243.61" << "," << TCPport << ";";
    //ss >> str;
    for(int i = 0 ; i < (int)clients.size() ; i++){
        ClientInfo* c = clients[i];
        if((!c->isOurClient)){
            if(c->hasUsername){
                str += c->userName;
            }
            str += "," + c->peerName + "," + to_string(c->tcpPort) + ";";
        }
    }
    return str;
}

// ### A FUNCTION TO MAKE SURE WE ONLY SEND OUR SERVER LIST IN CASE OF 'LISTSERVERS' ###
// ### REFERENCE: BEEJ GUIDE - MAY WANT TO REFACTOR
bool handleForeignLISTSERVERS(char* UDPBuffer, string localIP) {
    
    string command;
    manageBuffer(UDPBuffer, command);
    if (command == "LISTSERVERS") {
        string package = listServersReplyUDP(localIP);
        bzero(UDPBuffer, strlen(UDPBuffer));
        strcpy(UDPBuffer, package.c_str());
        return true;
    }
    return false;
}

// ### LISTEN FOR 'LISTSERVERS' ON UDP PORT - SENDS A LIST OF OUR SERVERS IN RETURN
void listenForUDP(int &UDPsock, sockaddr_storage addr_udp, socklen_t udp_addrlen, char* TCPBuffer, char* UDPBuffer, string localIP) {
    cout << "Listening for UDP" << endl;
    int numbytes;
    
    bzero(UDPBuffer, strlen(UDPBuffer));
    
    // HARÐKÓÐAÐ 1000 - Finna lausn
    if ((numbytes = recvfrom(UDPsock, UDPBuffer, sizeof(UDPBuffer), 0,
                             //if ((numbytes = recvfrom(UDPsock, UDPBuffer, 1000-1 , 0,
                             (struct sockaddr *)&addr_udp, &udp_addrlen)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    cout << UDPBuffer << endl;
    
    // NOT REALLY SURE WHY -1 WORKS, USED TO BE JUST UDPBuffer[numbytes]
    UDPBuffer[numbytes-1] = '\0';
    
    if (handleForeignLISTSERVERS(UDPBuffer, localIP)) {
        cout << "Replying to LISTSERVERS command from UDP ! " << endl;
        string str = addTokens(UDPBuffer);
        strcpy(UDPBuffer, str.c_str());
        sendto(UDPsock, UDPBuffer, strlen(UDPBuffer), 0, (struct sockaddr *) &addr_udp, udp_addrlen);
    }
    
}

///opens the given port on the given socket.
void openTCPPort(struct sockaddr_in serv_addr, int sockfd, int portno) {
    
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
    
   //cout << "ip is " << inet_ntoa(serv_addr.sin_addr) << endl;
   //cout << "port is " << htons(serv_addr.sin_port) << endl;
}

///Sends the buffer to all connected clients (except the original sender if alsoSendToSender is false)
void sendBufferToAll(ClientInfo* user, char* buffer, bool alsoSendToSender){
    buffer[strlen(buffer)] = '\0';
    
    for(int i = 0 ; i < (int)clients.size() ; i++){
        //check if this user is the sender and whether or not to send the buffer to him
        if((clients[i] !=  user || alsoSendToSender) && clients[i]->hasUsername){
            //test
            //cout << "sending \"" << buffer << "\" to " << user->userName << endl;
            string str = addTokens(buffer);
            strcpy(buffer, str.c_str());
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
                string str = addTokens(buffer);
                strcpy(buffer, str.c_str());
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

    //checking for tokens at the beginning and at the end
    if(str.find("0x01") == 0 && str.compare(str.size() - 4, 4, "0x04") == 0)
    {
        //check if the end token also appears somewhere else in the message
        //if the string has been bitstuffed we should find the end token duplicated
        //then we have to delete the stuffed token
        int pos = 0;
        while(str.find("0x040x04") < str.size())
        {
            pos = str.find("0x040x04");
            str.erase(pos, 4);
        }
        str.erase(0, 4);
        str.erase(str.size() - 4, str.size());
    }
    else
    {
        return "Tokens not valid";
    }
    return str;
}

void connectTo(char* buffer){
    string address;
    string tcpport;
    struct sockaddr_in serv_addr;
    
    manageBuffer(buffer, address);
    manageBuffer(buffer, tcpport);
    bzero((char*)&serv_addr, sizeof(serv_addr));
    connectToServer(serv_addr, address, stoi(tcpport));
}

void CMD(char* buffer, ClientInfo* &user, string srvcmd, string toServerID, string fromServerID, string localIP){
    
    string ID;
    string hashNumber;
    if (srvcmd == "LISTSERVERS") {
        
        cout << "Other server requested a LISTSERVERS" << endl;
        string package = listServersReply(localIP, toServerID, fromServerID);
        bzero(buffer, strlen(buffer));
        strcpy(buffer, package.c_str());
        cout << "Sending LISTSERVERS via TCP" << endl;
        string str = addTokens(buffer);
        strcpy(buffer, str.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
        
    }
    
    if (srvcmd == "ID}") {
    
        if (toServerID == " " || toServerID == fromServerID) {
            cout << fromServerID << " has requested to connect" << endl;
            bzero(buffer, strlen(buffer));
            strcpy(buffer, "RSP,");
            strcat(buffer, fromServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, serverID);
            strcat(buffer, ",");
            strcat(buffer, serverID);
            strcat(buffer, ",");
            cout << buffer << endl;
            string str = addTokens(buffer);
            strcpy(buffer, str.c_str());
            send(user->socketVal, buffer, strlen(buffer), 0);
        }
        
        else {
            cout << fromServerID << " has requested ID" << endl;
            cout << "Sending RSP..." << endl;
            bzero(buffer, strlen(buffer));
            strcpy(buffer, "RSP,");
            strcat(buffer, fromServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, serverID);
            strcat(buffer, ",");
            strcat(buffer, serverID);
            cout << buffer << endl;
            string str = addTokens(buffer);
            strcpy(buffer, str.c_str());
            send(user->socketVal, buffer, strlen(buffer), 0);
        }
    }
    
    if (srvcmd.compare("KEEPALIVE") == 0) {
        cout << "Peer sent KEEPALIVE" << endl;
        user->lastRcvKA = getTime();
    }
    
    if (srvcmd == "FETCH") {
        manageBuffer(buffer, hashNumber);
        strcpy(buffer, "RSP,");
        strcat(buffer, fromServerID.c_str());
        strcat(buffer, ",");
        strcat(buffer, serverID);
        strcat(buffer, ",");
        strcat(buffer, "FETCH");
        strcat(buffer, ",");
        if (hashNumber == "1") {
            strcat(buffer, "a2d9f117f2dabf294bdabb899e184f9c");
            
        }
        else if (hashNumber == "2") {
            strcat(buffer, "8fc42c6ddf9966db3b09e84365034357");
        }
        else if (hashNumber == "3") {
            strcat(buffer, "65b50b04a6af50bb2f174db30a8c6dad");
        }
        else if (hashNumber == "4") {
            strcat(buffer, "dd22b70914cd2243e055d2e118741186");
        }
        else if (hashNumber == "5") {
            strcat(buffer, "8fc42c6ddf9966db3b09e84365034357");
        }
        string str = addTokens(buffer);
        strcpy(buffer, str.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
    }
}

void RSP(char* buffer, ClientInfo* user, string srvcmd, string toServerID, string fromServerID){
    
    string ID;
    string IP;
    string tcpport;
    string hash;
    string remoteServerID = fromServerID;
    if (toServerID == serverID) {
        // ### If another server sends RSP,V_GROUP_15,fromServerID,fromServerID, we sent LISTSERVERS to them to get their information ###
        if (srvcmd == "ID") {
            cout << "ID RSP received - Sending LISTSERVERS to " << fromServerID << endl;
            bzero(buffer, strlen(buffer));
            strcpy(buffer, "CMD");
            strcat(buffer, ",");
            strcat(buffer, fromServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, toServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, "LISTSERVERS");
            for (int i = 0; i < clients.size(); i++) {
                if (user->socketVal == clients[i]->socketVal) {
                    clients[i]->userName = fromServerID;
                    clients[i]->hasUsername = true;
                    cout << "Peer added to list of clients with username: " << fromServerID << endl;
                }
            }
            cout << "Connected users are now: " << endl;
            for (int i = 0; i < clients.size(); i++) {
                cout << clients[i]->userName << endl;
            }
            string str = addTokens(buffer);
            strcpy(buffer, str.c_str());
            send(user->socketVal, buffer, strlen(buffer), 0);
        }
        
        // ### If another server sends RPS,V_GROUP_15,fromServerID,LISTSERVES,fromServerID,IP,port - We extract the information about them and add them to our client information.
        if (srvcmd == "LISTSERVERS") {
            
            cout << "User sent their list of servers" << endl;
            manageBuffer(buffer, ID);
            cout << "User ID is: " << ID << endl;
            manageBuffer(buffer, IP);
            cout << "User IP is: " << IP << endl;
            manageBuffer(buffer, tcpport);
            cout << "User TCP port is: " << tcpport << endl;
            for (int i = 0; i < clients.size(); i++) {
                if (user->userName == clients[i]->userName) {
                    clients[i]->peerName = IP;
                    clients[i]->tcpPort = stoi(tcpport);
                }
            }
        }
        
        if (srvcmd == "FETCH") {
            manageBuffer(buffer, hash);
            // ### VISTA HASH Í SKRÁ?
            cout << "The hash is: " << buffer << endl;
        }
        
    }
    else {
        cout << "RSP not for us" << endl;
        cout << "Action?" << endl;
    }

    /*// RSP,
     manageBuffer(buffer, toServerID);
     manageBuffer(buffer, fromServerID);
     manageBuffer(buffer, srvcmd);
     
     if(toServerID == serverID){
     if(!user->hasUsername){
     bool fromThisServer = true;
     for(int i = 0 ; (int)clients.size() ; i++){
     if(clients[i]->userName.compare(fromServerID) == 0){
     ///We DON'T know the serverID of the socket who sent us the RSP but we DO know the server who originally sent it
     fromServerID = false;
     break;
     }
     }
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
     cout << "Response received: " << srvcmd << endl;*/
}

void CONNECT(char* buffer, ClientInfo* user){
    if(user->isOurClient){
        //buffer[sizeof(buffer)] = '\0';
        //set the username
        string username;
        buffer[sizeof(buffer)-1] = '\0';
        manageBuffer(buffer, username);
        user->userName = username;
        user->hasUsername = true;
        user->tcpPort = 4020;
        
        //send the user a confirmation that he is now connected to the server
        strcpy(buffer, "You are now connected to the server as \"");
        strcat(buffer, &user->userName[0]);
        strcat(buffer, "\"");
        string str = addTokens(buffer);
        strcpy(buffer, str.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
        
        //alert others that this user has connected.
        /*bzero(buffer, strlen(buffer));
         strcpy(buffer, &user->userName[0]);
         strcat(buffer, " has just connected to the server\n");
         sendBufferToAll(user, buffer, 0);*/
    }
    else {
        user->userName = clients[(int)clients.size()-1]->userName;
        cout << "New user connected" << endl;
    }
}

void ID(char* buffer, ClientInfo* user){
    bzero(buffer, strlen(buffer));
    strcpy(buffer, serverID);
    string str = addTokens(buffer);
    strcpy(buffer, str.c_str());
    send(user->socketVal, buffer, strlen(buffer), 0);
}

void WHO(char* buffer, ClientInfo* user){
    bzero(buffer, strlen(buffer));
    //add users who have a username (and are therefore connected) to the buffer
    for (int i = 0; i < (int)clients.size(); i++) {
        if(clients[i]->hasUsername){
            stringstream ss;
            ss << (i + 1);
            string number;
            ss >> number;
            string str = "User " + number + ": " + clients[i]->userName;
            strcat(buffer, str.c_str());
        }
    }
    cout << buffer << endl;
    string str = addTokens(buffer);
    strcpy(buffer, str.c_str());
    send(user->socketVal, buffer, strlen(buffer), 0);
}

void MSG(char* buffer, ClientInfo* user, string name){
    //since MSG is a 2 word command, we need the second word
    
    
    manageBuffer(buffer, name);
    
    char tempBuffer[strlen(buffer)] ;
    strcpy(tempBuffer, buffer);
    
    /*if(name == "ALL"){
     
     //add the username of the sender and send it out.
     strcpy(buffer, &user->userName[0]);
     strcat(buffer, ": ");
     strcat(buffer, tempBuffer);
     strcat(buffer, "\n");
     sendBufferToAll(user, buffer, 0);
     }*/
    
    //you can't send a message to yourself
    if(user->userName == name){
        strcpy(buffer, "That's you, dummy!");
        string str = addTokens(buffer);
        strcpy(buffer, str.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
    }
    
    //try to find the username specified
    else {
        
        for(int i = 0 ; i < (int)clients.size() ; i++){
            
            //if we find it, send the message to that user.
            if(clients[i]->userName == name){
                strcpy(buffer, "CMD,");
                strcat(buffer, name.c_str());
                strcat(buffer, ",");
                strcat(buffer, serverID);
                strcat(buffer, ",");
                strcat(buffer, tempBuffer);
                
        
                string str = addTokens(buffer);
                strcpy(buffer, str.c_str());
                send(clients[i]->socketVal, buffer, strlen(buffer), 0);
                break;
            }
            
            //if we don't, we alert the user that there is no user with that name
            if(i == (int)clients.size() - 1){
                strcpy(buffer, "No server with ID \"");
                strcat(buffer, &name[0]);
                strcat(buffer, "\"");
                string str = addTokens(buffer);
                strcpy(buffer, str.c_str());
                send(user->socketVal, buffer, strlen(buffer), 0);
            }
        }
    }
}

///DECIDES WHAT TO DO BASED ON THE COMMAND RECEIVED FROM THE CLIENT
void handleConnection(char* buffer, int messageCheck, ClientInfo* user, string localIP) {
    string command;
    string name;
    string toServerID;
    string fromServerID;
    string srvcmd;
    string IP;
    string tcpport;
    string udpport;
    
    // get the command
    manageBuffer(buffer, command);
    
    if (command == "connectTo") {
        connectTo(buffer);
    }
    
    if (command == "CMD" || command == "RSP") {
        
        manageBuffer(buffer, toServerID);
        manageBuffer(buffer, fromServerID);
        manageBuffer(buffer, srvcmd);
        
        if(command != "RSP"){
            CMD(buffer, user, srvcmd, toServerID, fromServerID, localIP);
        }
        else{
            RSP(buffer, user, srvcmd, toServerID, fromServerID);
        }
    }
    
    if (command == "CONNECT") {
        CONNECT(buffer, user);
    }
    
    //send the current server id to the user
    else if(command == "ID"){
        ID(buffer, user);
    }
    
    //send the user a list of clients who are connected
    else if (command == "WHO" && user->hasUsername) {
        WHO(buffer, user);
    }
    
    //remove the user from the server and alert others that that user has left (if that user had a username).
    //a user who hasn't used the CONNECT command can still use the LEAVE command
    else if(command == "LEAVE"){
        if(user->hasUsername){
            name = user->userName;
            strcpy(buffer, "User \"");
            strcat(buffer, &name[0]);
            strcat(buffer, "\" has disconnected from the server");
        }
        leave(user, buffer);
    }
    
    else if(command == "V_GROUP_15_I_am_your_father"){
        user->isOurClient = true;
    }
    
    //sends a message to a specific or all users
    else if(command == "MSG" && user->hasUsername) {
        MSG(buffer, user, name);
    }
}

int main(int argc, char* argv[]) {
    ///####################### VARIABLE INITIALIZATION #######################
    
    int listeningSock, UDPsock, newSock, messageCheck, socketVal, maxSocketVal;
    struct sockaddr_in serv_addr, cli_addr;
    struct sockaddr_storage addr_udp;
    socklen_t cli_addrlen, udp_addrlen;
    TCPport = 4023;
    UDPport = 4024;
    //TCPport = atoi(argv[1]);
    //UDPport = atoi(argv[2]);
    
    
    cout << "The server ID is: " << serverID << endl;
    
    //the message buffer
    char message[1000];
    char UDPbuffer[1000];
    
    // a set of file descriptors
    fd_set masterFD, read_fds;
    
    ///##################### SETTING UP MAIN SOCKETS ########################
    
    // ### Setting up TCP socket ###
    listeningSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    validateSocket(listeningSock);
    
    //socket settings
    bzero((char*)&serv_addr, sizeof(serv_addr));
    bzero((char*)&addr_udp, sizeof(addr_udp));
    
    openTCPPort(serv_addr, listeningSock, 4023);
    openUDPPort(UDPsock);
    
    listen(listeningSock, 5);
    
    cli_addrlen = sizeof(cli_addr);
    udp_addrlen = sizeof(addr_udp);
    
    //inet_ntop(AF_INET, &serv_addr.sin_addr, localip, 80);
    
    ///##################### RUNNING BODY OF SERVER ######################
    while(true){
        // KEEPALIVE Operations
        int t = getTime();
        // ### IF CLIENT HAS NOT SENT 'KEEPALIVE' IN THE LAST 180 SECONDS, DROP THEM ###
        if(clients.size() > 0){
            for(int i = 0 ; i < (int)clients.size() ; i++){
                if(t - clients[i]->lastRcvKA > MAX_TIME_INTERVAL && !clients[i]->isOurClient) {
                    leave(clients[i], message);
                }
            }
        }
        
        // ### IF OUR SERVER HAS NOT SENT 'KEEPALIVE' IN 70 SECONDS, SEND 'KEEPALIVE' TO EVERYONE EXCEPT OUR CLIENT
        if (t-stillAliveTime > 70) {
            for(int i = 0; i < (int)clients.size(); i++) {
                if (!clients[i]->isOurClient) {
                    bzero(message, strlen(message));
                    strcpy(message, "CMD,");
                    strcat(message, clients[i]->userName.c_str());
                    strcat(message, ",");
                    strcat(message, serverID);
                    strcat(message, ",KEEPALIVE");
                    string str = addTokens(message);
                    strcpy(message, str.c_str());
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
            
            //if something happens on the listening socket, someone is trying to connect to the server.
            if(FD_ISSET(listeningSock, &masterFD)){
                
                newSock = accept(listeningSock, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
                validateSocket(newSock);
                
                getpeername(newSock, (struct sockaddr * )&cli_addr, (socklen_t *)&cli_addrlen);
                int time = getTime();
                clients.push_back(new ClientInfo(newSock, inet_ntoa(cli_addr.sin_addr), time));
                
                //cout << "ip is " << inet_ntoa(cli_addr.sin_addr) << endl;
                //cout << "port is " << htons(cli_addr.sin_port) << endl;
                
                cout << "Sending message to other server: ";
                strcpy(message, "CMD,,V_GROUP_15,ID");
                cout << message << endl;
                string str = addTokens(message);
                strcpy(message, str.c_str());
                send(newSock, message, strlen(message), 0);
            }
            
            //if something happens on any other socket, it's probably a message
            else if(FD_ISSET(UDPsock, &masterFD)) {
                
                listenForUDP(UDPsock, addr_udp, udp_addrlen, message, UDPbuffer, localIP);
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
                        string str = checkTokens(message);
                        strcpy(message, str.c_str());
                        handleConnection(message, messageCheck, clients[i], localIP);
                    }
                    
                    else{
                        if(clients[i]->hasUsername){
                            strcpy(message, "User \"");
                            strcat(message, &clients[i]->userName[0]);
                            strcat(message, "\" has disconnected from the server");
                        }
                        leave(clients[i], message);
                    }
                }
            }
        }
    }
    return 0;
}
