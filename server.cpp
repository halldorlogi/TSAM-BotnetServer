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

//clients who are attempting to connect via portknocking but haven't gotten the right sequence

//clients who have successfully connected with the correct sequence of portknocking
vector<ClientInfo*> clients;

//the server Id
string serverID = "V_GROUP_15_HLH";

///Get the amount of milliseconds elapsed since 00:00 Jan 1 1970 (I think)
int getTime(){
    int t = (int)time(0);
    return t;
}

int stillAliveTime = getTime();

void connectToServer(struct sockaddr_in serv_addr, string address, int portno) {
    int instructorsock = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *server = gethostbyname(address.c_str());

    char message[1000];

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portno);
    if(connect(instructorsock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0){
        cout << "connection to " << server->h_name << " successful" << endl;
        strcpy(message, "CMD,,V_GROUP_15_HLH,ID");
        send(instructorsock, message, strlen(message), 0);
        int time = getTime();
        clients.push_back(new ClientInfo(instructorsock, server->h_name, time));
        clients[clients.size()-1]->tcpPort = portno;
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

    string str = string(buffer);
    char delimit[] = ",";
    firstWord = strtok(buffer, delimit);
    str = str.substr(str.find_first_of(delimit)+1);
    strcpy(buffer, str.c_str());
}

///opens the given port on the given socket.
void openPort(struct sockaddr_in serv_addr, int sockfd, int portno) {

    cout << "Opening port: " << portno << endl;
    // Allow reusage of address
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0){
        cerr << ("setsockopt(SO_REUSEADDR) failed") << endl;
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = /*inet_addr("127.0.0.1");//*/INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Attempt to bind the socket to the host (portno) the server is currently running on
    if (::bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        cerr << "Error on binding" << endl;
        exit(0);
    }

    cout << "ip is " << inet_ntoa(serv_addr.sin_addr) << endl;
    cout << "port is " << serv_addr.sin_port << endl;
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

///DECIDES WHAT TO DO BASED ON THE COMMAND RECEIVED FROM THE CLIENT
void handleConnection(char* buffer, int messageCheck, ClientInfo* user) {
    string command;
    string address;
    string port;
    string name;
    string toServerID;
    string fromServerID;
    string srvcmd;

    // get the command
    manageBuffer(buffer, command);

    if (command == "connectTo") {
        manageBuffer(buffer, address);
        manageBuffer(buffer, port);
        struct sockaddr_in serv_addr;
        bzero((char*)&serv_addr, sizeof(serv_addr));
        connectToServer(serv_addr, address, stoi(port));
    }

    if (command == "CMD") {
        manageBuffer(buffer, toServerID);
        manageBuffer(buffer, fromServerID);
        manageBuffer(buffer, srvcmd);
        if (srvcmd == "ID") {
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
                send(user->socketVal, buffer, strlen(buffer), 0);
            }
        }
        if (srvcmd == "KEEPALIVE") {
            user->lastRcvKA = getTime();
        }
    }

    if (command == "RSP") {

        manageBuffer(buffer, toServerID);
        manageBuffer(buffer, fromServerID);
        manageBuffer(buffer, srvcmd);
        cout << "Response received: " << srvcmd << endl;
        cout << "Adding " << fromServerID << " to network" << endl;

    }

    if (command == "CONNECT") {
        //set the username
        user->userName = string(buffer);
        user->hasUsername = true;

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

    //send the current server id to the user
    else if(command == "ID"){
        bzero(buffer, strlen(buffer));
        strcpy(buffer, serverID.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
    }

    //change the server id
   /* else if(command == "CHANGE" && user->hasUsername){
        manageBuffer(buffer, command);
        if(command == "ID"){
            serverID = newID();
            strcpy(buffer, "The ID has been changed to \"");
            strcat(buffer, &serverID[0]);
            strcat(buffer, "\"\n");

            //alert clients that the server id has been changed.
            sendBufferToAll(user, buffer, 1);
        }
    }*/

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
                string str = "User " + number + ": " + clients[i]->userName + " connected at: " + t + "\n";
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
    else if(command == "MSG" && user->hasUsername){

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
        else{
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
    else if (command == "LISTSERVERS"){
        ///TODO senda lista af okkar tengingum UDP port usersins
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

string listServersReply(string localip, string tcp, string udp){
    string str = serverID + "," + localip + "," + tcp + "," + udp + ";";

    for(int i = 0 ; i < (int)clients.size() ; i++){
        ClientInfo* c = clients[i];
        if(!(c->isUser)){
            if(c->hasUsername){
                str += c->userName;
            }
            str += "," + c->peerName + "," + to_string(c->tcpPort) + "," + to_string(c->udpPort) + ";";
        }
    }
    return str;
}


void sendUDPmessage(int portno, char* buffer){

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    int udpSocket;
    if((udpSocket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		perror("failed to make udp socket");
		exit(0);
    }
    memset((char *) &cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(portno);

    sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *) &cli_addr, clilen);
    close(udpSocket);
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



int main(int argc, char* argv[]){
///####################### VARIABLE INITIALIZATION #######################

    int listeningSock, UDPsock, newSock, messageCheck, socketVal, maxSocketVal;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_addrlen, serv_addrlen;

    cout << "The server ID is: " << serverID << endl;

    //the message buffer
    char message[1000];
    char UDPbuffer[1000];
    char localip[80];

    // a set of file descriptors
    fd_set masterFD;

///##################### SETTING UP MAIN SOCKETS ########################

    //create the sockets
    /*knockSock1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    validateSocket(knockSock1);
    knockSock2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    validateSocket(knockSock2);*/
    listeningSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    validateSocket(listeningSock);
    UDPsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    validateSocket(UDPsock);


    //socket settings
    bzero((char*)&serv_addr, sizeof(serv_addr));

    //openPort(serv_addr, knockSock1, KNOCK_PORT_1);
    //openPort(serv_addr, knockSock2, KNOCK_PORT_2);
    openPort(serv_addr, listeningSock, 4023);
    openPort(serv_addr, UDPsock, 4024);

    //listen for activity
   // listen(knockSock1, 5);
   // listen(knockSock2, 5);
    listen(listeningSock, 5);

    cli_addrlen = sizeof(cli_addr);
    serv_addrlen = sizeof(serv_addr);

    inet_ntop(AF_INET, &serv_addr.sin_addr, localip, 80);


///#################### Reyna að tengjast við instructor server ####################

    /*int instructorsock = socket(AF_INET, SOCK_STREAM, 0);

    struct hostent *server = gethostbyname("skel.ru.is");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *) server->h_addr,
        (char *) &serv_addr.sin_addr.s_addr,
        server->h_length);

    serv_addr.sin_port = htons(4023);
    if(connect(instructorsock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0){
        cout << "connection to " << server->h_name << " successful" << endl;
        strcpy(message, "CMD,,V_GROUP_15,ID");
            send(instructorsock, message, strlen(message), 0);

        strcpy(UDPbuffer, "V_GROUP_15,");
        strcat(UDPbuffer, server->h_name);
        strcat(UDPbuffer, ",4010,4011;");

        //sendUDPmessage(4001, UDPbuffer);
    }*/

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
        if (stillAliveTime - getTime() > 70) {
            for(int i = 0; i < (int)clients.size(); i++) {
                if (!clients[i]->isUser) {
                    bzero(message, strlen(message));
                    strcpy(message, "CMD,");
                    strcat(message, clients[i]->userName.c_str());
                    strcat(message, ",");
                    strcat(message, serverID.c_str());
                    strcat(message, ",KEEPALIVE");
                    send(clients[i]->socketVal, message, strlen(message), 0);
                    cout << "KEEPALIVE MESSAGE SENT" << endl;
                }
            }
        }

        //reset the fd_set
        FD_ZERO(&masterFD);
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

        //wait for something to happen on some socket. Will wait forever if nothing happens.
        select(maxSocketVal + 1, &masterFD, NULL, NULL, NULL);

        //if something happens on the knocking socket, we check who it is.
       /* if(FD_ISSET(knockSock1, &masterFD)){
            checkWhoIsKnocking(cli_addr, knockSock1, KNOCK_PORT_1, cli_addrlen);
        }
        else if(FD_ISSET(knockSock2, &masterFD)){
            checkWhoIsKnocking(cli_addr, knockSock2, KNOCK_PORT_2, cli_addrlen);
        }*/

        //if something happens on the listening socket, someone is trying to connet to the server.
        if(FD_ISSET(listeningSock, &masterFD)){
            newSock = accept(listeningSock, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
            validateSocket(newSock);
            getpeername(newSock, (struct sockaddr * )&cli_addr, (socklen_t *)&cli_addrlen);
            int time = getTime();
            clients.push_back(new ClientInfo(newSock, inet_ntoa(cli_addr.sin_addr), time));

            // ### BÆTA VIÐ IF-SETNINGU HÉR TIL AÐ KOMA Í VEG FYRIR AÐ SENDA Á EIGIN CLIENT ###
            // ### SPURNING SAMT HVORT AÐ ÞETTA EIGI HEIMA HÉR ÞAR SEM ÞETTA ER LISTENING SOCKET? ###
            cout << "Sending message to other server: ";
            strcpy(message, "CMD,,V_GROUP_15_HLH,ID");
            cout << message << endl;

            send(newSock, message, strlen(message), 0);

            /*else{
                close(newSock);
            }*/
        }

        //if something happens on any other socket, it's probably a message

        else if(FD_ISSET(UDPsock, &masterFD)){

            bzero(UDPbuffer, sizeof(UDPbuffer));
            if(recvfrom(UDPsock, (char *)UDPbuffer, sizeof(UDPbuffer), 0, (struct sockaddr *) &serv_addr, &serv_addrlen) != 0){
                ///TODO tjekka tokens og parsa eftir kommum.
                cout << "UDP buffer is now: ";
                cout << UDPbuffer << endl;
            }
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
    return 0;
}
