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

#define MAIN_PORT 4000
#define MAX_TIME_INTERVAL 120
using namespace std;


///A class that holds information about a client
class ClientInfo{
public:
    int socketVal, udpPort;
    string peerName;
    string userName;
    bool hasUsername;
    bool isUser;

    //all clients need a peer name
    ClientInfo(int socketVal){
        hasUsername = false;
        isUser = false;
        peerName = "";
        userName = "";
        this->socketVal = socketVal;
    }

    ClientInfo(int socketVal, string peerName){
        hasUsername = false;
        isUser = false;
        this->peerName = peerName;
        userName = "";
        this->socketVal = socketVal;
    }


    ~ClientInfo(){

    }
};

//clients who are attempting to connect via portknocking but haven't gotten the right sequence

//clients who have successfully connected with the correct sequence of portknocking
vector<ClientInfo*> clients;

//the server Id
string serverID = "V_GROUP_15";

void validateSocket(int sockfd) {

    if (sockfd < 0) {
        cerr << "ERROR on binding socket" << endl;
        exit(0);
    }
}

///Get the amount of milliseconds elapsed since 00:00 Jan 1 1970 (I think)
int getTime(){
    int t = (int)time(0);
    return t;
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
    firstWord = strtok(buffer, " ");
    str = str.substr(str.find_first_of(" ")+1);
    strcpy(buffer, str.c_str());

    //tests
   // cout << firstWord << endl;
   // cout << buffer << endl;
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
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Attempt to bind the socket to the host (portno) the server is currently running on
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        cerr << "Error on binding" << endl;
        exit(0);
    }

    cout << "ip is " << inet_ntoa(serv_addr.sin_addr) << endl;
    cout << "port is" << serv_addr.sin_port << endl;
}

///manages the knockingClients vector based on information given.
/*void vectorManagement(string peerName, int portno, int t){
    //if this is the port, then he is starting the sequence and we add him into the vector.
    if(portno == KNOCK_PORT_1){
        int i = 0;
        if(knockingClients.size() > 0){
            i = (int)knockingClients.size() - 1;
        }
        knockingClients.push_back(new ClientInfo(peerName));
        knockingClients[i]->knock1 = true;
        knockingClients[i]->timeOfKnock1 = t;
    }
    //if this is the port, he is attempting to continue the sequence.
    else if(portno == KNOCK_PORT_2){
        for(int i = 0 ; i < (int)knockingClients.size() ; i++){
            if(knockingClients[i]->peerName == peerName){
                knockingClients[i]->knock2 = true;
                knockingClients[i]->timeOfKnock2 = t;
            }
        }
    }
}

///checks who is knocking on a port, calls vectorManager to store information about who knocks on what port and when
void checkWhoIsKnocking(struct sockaddr_in &cli_addr, int sockfd, int portno, socklen_t len){
    //accept the connection
    cout << "knock knock " << portno << endl;
    int knockerSock = accept(sockfd, (struct sockaddr *)&cli_addr, &len);
    validateSocket(knockerSock);
    //see who it is
    getpeername(knockerSock, (struct sockaddr * )&cli_addr, &len);
    string peerName = inet_ntoa(cli_addr.sin_addr);
    cout << peerName << endl;
    int t = getTime();

    //close the connection
    close(knockerSock);
    vectorManagement(peerName, portno, t);
}*/

///similar to checkWhoIsKnocking but specifically for the listening port. Does not close the socket
/*int checkIfThisPeerIsAllowed(struct sockaddr_in &cli_addr, int sockfd, socklen_t len){
    //see who it is
    getpeername(sockfd, (struct sockaddr * )&cli_addr, &len);
    string peerName = inet_ntoa(cli_addr.sin_addr);
    int t = getTime();
    //if they have knocked on KNOCK_PORT_2 and less than 1200000ms have passed, they get in.
    for(int i = 0 ; i < (int)knockingClients.size() ; i++){
        //find the right client in the vector
        if(knockingClients[i]->peerName == peerName){
            //check the time
            if(knockingClients[i]->knock2 && t - knockingClients[i]->timeOfKnock1 < MAX_TIME_INTERVAL){
                //save this socket
                knockingClients[i]->socketVal = sockfd;
                //add this client to the allowedClients vector and erase him from the knockingClients vector
                allowedClients.push_back(knockingClients[i]);
                knockingClients.erase(knockingClients.begin() + i);

                //test
                //cout << allowedClients[0]->peerName << endl;
                return 1;
            }
        }
    }
    return 0;
}*/
/*
///Generates a new serverID using the fortune command, a timestamp and the initials of this group (HAH)
string newID(){
    string str;
    string line;

    //call the fortune command and save the output into a txt file
    system("fortune -s > ServerID.txt");
    ifstream idFile("ServerID.txt");

    //read the txt file
    while(!idFile.eof()){
        getline(idFile, line);
        str += line;
    }

    //add the timestamp and initials
    str += "\n" + getReadableTime() + "\nHAH";
    idFile.close();
    return str;
}
*/
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
    string name;

    // get the command
    manageBuffer(buffer, command);

    if (command == "CONNECT" && user->isUser) {
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
                ss << (i + 1);
                string number;
                ss >> number;
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
}

//Function to check if the two token characters are in the beginning and end of the buffer
char checkTokens(char* buffer) {
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

    return str.c_str();
}

int main(){
///####################### VARIABLE INITIALIZATION #######################

    int listeningSock, newSock, messageCheck, socketVal, maxSocketVal;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_addrlen;

    cout << serverID << endl;

    //the message buffer
    char message[1000];

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

    //socket settings
    bzero((char*)&serv_addr, sizeof(serv_addr));

    //openPort(serv_addr, knockSock1, KNOCK_PORT_1);
    //openPort(serv_addr, knockSock2, KNOCK_PORT_2);
    openPort(serv_addr, listeningSock, MAIN_PORT);

    //listen for activity
   // listen(knockSock1, 5);
   // listen(knockSock2, 5);
    listen(listeningSock, 5);

    cli_addrlen = sizeof(cli_addr);


///##################### RUNNING BODY OF SERVER ######################
    while(true){
        //clean out knockingClients array of anyone who has not knocked in 2 minutes
        /*int t = getTime();
		if(knockingClients.size() > 0){
			for(int i = 0 ; i < (int)knockingClients.size() ; i++){
				if(t - knockingClients[i]->timeOfKnock1 > MAX_TIME_INTERVAL){
					cout << "deleting " << knockingClients[i]->peerName << endl;
					knockingClients.erase(knockingClients.begin() + i);
					break;
				}
			}
        }*/

        //reset the fd_set
        FD_ZERO(&masterFD);
        //FD_SET(knockSock1, &masterFD);
        //FD_SET(knockSock2, &masterFD);
        FD_SET(listeningSock, &masterFD);

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
            clients.push_back(new ClientInfo(newSock, inet_ntoa(cli_addr.sin_addr)));

            cout << "sending message" << endl;
            strcpy(message, "You are now connected to V_GROUP_15\n");
            send(newSock, message, strlen(message), 0);

            /*else{
                close(newSock);
            }*/
        }

        //if something happens on any other socket, it's propably a message
        for(int i = 0 ; i < (int)clients.size() ; i++){
            socketVal = clients[i]->socketVal;

            if(FD_ISSET(socketVal, &masterFD)){
                bzero(message, sizeof(message));
                messageCheck = read(socketVal, message, sizeof(message));
                message[messageCheck] = '\0';
                if(messageCheck != 0){
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
