#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>

using namespace std;

// ** ERROR FUNCTION ** //
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

string addTokens(char* buffer) {


    char stuffedBuffer[1000];
    int counter = 0;
    string tokenString = "";

    string str = string(buffer);
    //str.insert(0,1,0x01);
    //str.append(1,0x04);
    string start = "\x1";
    string end = "\x4";

    int pos = str.find(end);

    while(pos < str.size())
    {
        str.insert(pos, end);
        pos = str.find(end, pos + end.size()*2);
    }

    tokenString = start + str + end;
    return tokenString;
}

string checkTokens(char* buffer) {
    string str = string(buffer);

     /*if(str[0] == char(1) && str[strlen(buffer)-1] == char(4)){
        str = str.substr(1, str.size() - 2);
    }*/

    //checking for tokens at the beginning and at the end
    if(str.find("\x1") == 0 && str.compare(str.size() - 1, 1, "\x4") == 0)
    {
        //check if the end token also appears somewhere else in the message
        //if the string has been bitstuffed we should find the end token duplicated
        //then we have to delete the stuffed token
        int pos = 0;
        while(str.find("\x4\x4") < str.size())
        {
            pos = str.find("\x4\x4");
            str.erase(pos, 1);
        }
        str.erase(0, 1);
        str.erase(str.size() - 1, str.size());
    }
    else
    {
        return "Tokens not valid";
    }
    return str;
}

bool knockOnPort(sockaddr_in serv_addr, hostent* server, int portno, int sockfd, const char* addr){

    if (sockfd < 0) {
        cout << "Error opening socket" << endl;
    }

    // ** APPLE COMPATIBILITY CODE ** //

    // ** SETTING UP SERVER ** //
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    // ** HOST ADDRESS IS STORED IN NETWORK BYTE ORDER ** //
    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portno);
    char loginCommand[28];
    strcpy(loginCommand, "V_GROUP_15_I_am_your_father");

    // Attempt a connection to the socket.
    int connection = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (connection == -1) {
        cout << "Port: " << portno << " CLOSED" << endl;
        return false;
    }
    cout << "Sending loginCommand to server: " << loginCommand << endl;
    string str = addTokens(loginCommand);
    strcpy(loginCommand, str.c_str());
    send(sockfd, loginCommand, strlen(loginCommand), 0);
    return true;
}

bool validateCommand(string input){
    string str = input;
    string firstWord = strtok(&input[0], " ");

    if(firstWord == "CONNECT" || firstWord == "ID" || firstWord == "MSG" || firstWord == "WHO" || firstWord == "LEAVE"){
        return true;
    }
    if(firstWord == "CHANGE"){
        str = str.substr(str.find_first_of(" ")+1);
        firstWord = strtok(&str[0], " ");
        if(firstWord == "ID"){
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[]) {

    // ** INITIALIZING VARIABLES **//
    //int knock1, knock2,
    int sockfd, udpsockfd;
    string address;
    struct sockaddr_in serv_addr;           // Socket address structure
    struct hostent *server;
    struct timeval time;
    string line;
    char buffer[1000];
    char udpbuffer[1000];
    int set = 1;

    fd_set masterFD, readFD;

    // ** SETTING ADDRESS TO LOCALHOST ** //
    address = "localhost";
    const char *addr = address.c_str();

    server = gethostbyname(addr);

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Open Socket
    udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);



    if(knockOnPort(serv_addr, server, atoi(argv[2]), sockfd, addr)){

        //cout << sockfd << endl;

        cout << "Here is a list of available commands" << endl << endl;
        cout << "ID                   ::      Set ID of server" << endl;
        cout << "CONNECT <USERNAME>   ::      Connect to the server" << endl;
        cout << "LEAVE                ::      Disconnect from server" << endl;
        cout << "WHO                  ::      List users on server" << endl;
        cout << "MSG <USERNAME>       ::      Send private message" << endl;
        cout << "MSG ALL              ::      Send message to everyone" << endl;
        cout << "CHANGE ID            ::      Change ID of server" << endl;
        cout << endl;

        string input;
        do{
            FD_ZERO((&masterFD));
            FD_ZERO((&readFD));
            FD_SET(sockfd, &masterFD);
            FD_SET(STDIN_FILENO, &readFD);

            time.tv_sec = 1;
            select(sockfd + 1, &masterFD, NULL, NULL, &time);
            if(FD_ISSET(sockfd, &masterFD)){
                bzero(buffer, 1000);
                int bytesRecv = recv(sockfd, buffer, sizeof(buffer), 0);
                string str = checkTokens(buffer);
                strcpy(buffer, str.c_str());
                if(bytesRecv > 0){
                    cout << string(buffer, 0, bytesRecv) << endl;
                }
            }

            time.tv_sec = 1;
            select(STDIN_FILENO + 1, &readFD, NULL, NULL, &time);
            if(FD_ISSET(STDIN_FILENO, &readFD)){
                getline(cin, input);
                if(input.size() > 0) {
                    strcpy(buffer, input.c_str());
                    string str = addTokens(buffer);
                    strcpy(buffer, str.c_str());
                    send(sockfd, buffer, strlen(buffer) + 1, 0);
                }
                else{
                    cout << "That's not a valid command" << endl;
                }
            }
        }
        while(input != "LEAVE");
    }
}
