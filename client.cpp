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
    //cout << "Sending loginCommand to server: " << loginCommand << endl;
	//send(sockfd, loginCommand, strlen(loginCommand), 0);
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
    int sockfd;
    string address;
    struct sockaddr_in serv_addr;           // Socket address structure
    struct hostent *server;
    struct timeval time;
    string line;
    char buffer[1000];

    fd_set masterFD, readFD;

    // ** SETTING ADDRESS TO LOCALHOST ** //
    address = "localhost";
    const char *addr = address.c_str();

    server = gethostbyname(addr);

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    //knock1 = socket(AF_INET, SOCK_STREAM, 0); // Open Socket
    //knock2 = socket(AF_INET, SOCK_STREAM, 0); // Open Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Open Socket

    //if(knockOnPort(serv_addr, server, 33745, knock1, addr)){
      //  close(knock1);
        //if(knockOnPort(serv_addr, server, 33746, knock2, addr)){
          //  close(knock2);
  
            if(knockOnPort(serv_addr, server, 4023, sockfd, addr)){

                cout << sockfd << endl;

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
                            int bytesRecv = recv(sockfd, buffer, 1000, 0);
                            if(bytesRecv > 0){
                                cout << "Message received from server: ";
                            cout << string(buffer, 0, bytesRecv) << endl;
                        }
                    }
                    time.tv_sec = 1;
                    select(STDIN_FILENO + 1, &readFD, NULL, NULL, &time);
                    if(FD_ISSET(STDIN_FILENO, &readFD)){
                        getline(cin, input);
                        if(input.size() > 0){
                            send(sockfd, input.c_str(), input.size() + 1, 0);
                        }
                        else{
                            cout << "That's not a valid command" << endl;
                        }
                    }
                }
                while(input != "LEAVE");
            }
		}
  //  }
//}
