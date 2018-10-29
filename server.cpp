#include <algorithm>
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

#define MAX_TIME_INTERVAL 300
#define serverID "V_GROUP_15"

using namespace std;


// ** A CLASS TO USE IN LISTROUTES ** //
class ShortClientInfo{
public:
    string ID;
    string through;

    ShortClientInfo(string ID, string through){
        this->ID = ID;
        this->through = through;
    }
    ~ShortClientInfo(){

    }
};

// ** A CLASS THAT HOLDS INFORMATION ABOUT A CLIENT (OTHER SERVERS) ** //
class ClientInfo{
public:
    int socketVal, tcpPort, lastRcvKA;
    string peerName;
    string userName;
    string ListServers;
    bool hasUsername;
    bool isOurClient;
    vector<string> hasRouteTo;

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

// ** A CONTAINER FOR SUCCESSFULLY CONNECTED SERVER CLIENTS ** //
vector<ClientInfo*> clients;
// ** THIS SERVER'S PORT NUMBERS. INITIALIZED IN MAIN ** //
int TCPport, UDPport;
// ** PORT NUMBER SIN CHAR FORM ** //
const char* cTCP;
const char* cUDP;

// ############### DO WE WANT THIS HARDCODED???? ############################################ //
// ** IP ADDRESS OF SKEL.RU.IS ** //
string localIP = "130.208.243.61";
// ** TIME SINCE LAST KEEPALIVE SENT ** //
int stillAliveTime;


// ** GET ELAPSED TIME SINCE 00:00 JAN 1. 1970 IN MS ** //
int getTime(){
    int t = (int)time(0);
    return t;
}

// ** ADD THE SOH AND EOT TOKENS TO THE BUFFER AND BITSTUFFING IF NECESSARY ** //
string addTokens(char* buffer) {

    string tokenString = "";
    string str = string(buffer);
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



// ** ATTEMPT TO CONNECT TO OTHER SERVER AND ADD THE SERVER'S INFORMATION TO CLIENTS ** //
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
        strcpy(message, "CMD,,V_GROUP_15,ID");
        string str = addTokens(message);
        strcpy(message, str.c_str());

        cout << "Connection to " << server->h_name << " successful" << endl;
        cout << "sending: \"" << message << "\" to socket " << externalSock << endl;

        send(externalSock, message, strlen(message), 0);
        int time = getTime();

        clients.push_back(new ClientInfo(externalSock, server->h_name, time));
        clients[clients.size() - 1]->tcpPort = tcpportno;
    }
}

// ** CHECKS IF SOCKET IS VALID ** //
void validateSocket(int sockfd) {
    if (sockfd < 0) {
        cerr << "ERROR on binding socket" << endl;
        exit(0);
    }
}

// ** Get a timestamp that's readable (taken from cplusplus.com) ** //
string getReadableTime(){
    time_t t;
    struct tm* timeinfo;
    char timestamp[30];

    time(&t);
    timeinfo =localtime(&t);

    strftime(timestamp, 30, "%c", timeinfo);
    return string(timestamp);
}

// ** A function that removes the first word of the buffer and saves it as the firstWord string ** //
void manageBuffer(char* buffer, string &firstWord) {
    string str = string(buffer);
    char delimit[] = ",";
    firstWord = strtok(buffer, delimit);
    str = str.substr(str.find_first_of(delimit)+1);
    strcpy(buffer, str.c_str());
}

// ######################## TÓK ÞETTA HELD ÉG BEINT AF BEEJ - ÞYRFTI AÐ REFERENCE EÐA AÐLAGA ###
void openUDPPort(int &UDPsock) {

    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, cUDP, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    // ** LOOP THROUGH ALL THE RESULTS AND BIND TO THE FIRST WE CAN ** //
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

// ** REFERENCE beej.us ** //
// ** GET sockaddr, IPv4 OR IPv6 ** //
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// ** CONSTRUCTS THE LISTSERVER REPLY ** //
string listServersReplyUDP() {
    string str;
    stringstream ss;
    for(int i = 0 ; i < (int)clients.size() ; i++){
        ClientInfo* c = clients[i];
        if((!c->isOurClient)){
            if(c->hasUsername){
                str += c->userName + "," + c->peerName + "," + to_string(c->tcpPort) + ";";
            }
        }
    }
    return str;
}

//  CHECKS WHETHER OR NOT WE ARE CONNECTED TO 5 SERVERS ** //
bool isFull(){
    int counter = 0;
    for(int i = 0 ; i < (int)clients.size() ; i++){
        if(!clients[i]->isOurClient){
            counter++;
        }
    }
    if(counter >= 5){
        return true;
    }
    else{
        return false;
    }
}

// ** REFERENCE: BEEJ GUIDE ** //
// ** A FUNCTION TO MAKE SURE WE ONLY SEND OUR SERVER LIST IN CASE OF 'LISTSERVERS' ** //
void handleForeignLISTSERVERS(char* UDPBuffer, string localIP) {

    string command;
    manageBuffer(UDPBuffer, command);
    string package = listServersReply();
    bzero(UDPBuffer, strlen(UDPBuffer));
    strcpy(UDPBuffer, package.c_str());

}

// ** LISTENS FOR 'LISTSERVERS' ON UDP PORT - SENDS A LIST OF OUR SERVERS IN RETURN ** //
void listenForUDP(int &UDPsock, sockaddr_storage addr_udp, socklen_t udp_addrlen, char* UDPBuffer, string localIP) {
    cout << "Listening for UDP" << endl;
    int numbytes;

    bzero(UDPBuffer, strlen(UDPBuffer));

    if ((numbytes = recvfrom(UDPsock, UDPBuffer, sizeof(UDPBuffer), 0, (struct sockaddr *)&addr_udp, &udp_addrlen)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    cout << UDPBuffer << endl;

    UDPBuffer[numbytes] = '\0';

    handleForeignLISTSERVERS(UDPBuffer, localIP);
        cout << "Replying to LISTSERVERS command from UDP ! " << endl;
        string str = addTokens(UDPBuffer);
        strcpy(UDPBuffer, str.c_str());
        sendto(UDPsock, UDPBuffer, strlen(UDPBuffer), 0, (struct sockaddr *) &addr_udp, udp_addrlen);

}

// ** OPENS THE GIVEN TCP PORT ON THE GIVEN SOCKET ** //
void openTCPPort(struct sockaddr_in serv_addr, int sockfd, int portno) {

    cout << "Opening port: " << portno << endl;
    // ** ALLOW REUSAGE OF ADDRESS ** //
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0){
        cerr << ("setsockopt(SO_REUSEADDR) failed") << endl;
        exit(0);
    }
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // ** ATTEMPT TO BIND THE SOCKET TO THE HOST (portno) THE SERVER IS CURRENTLY RUNNING ON ** //
    if (::bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        cerr << "Error on binding" << endl;
        exit(0);
    }
}


// ** DISCONNECTS THE SOCKET AND REMOVES CLIENT FROM THE VECTOR OF CLIENTS ** //
void leave(ClientInfo* user, char* buffer){
    close(user->socketVal);

    // ** CHECK IF THE USER HAS A USERNAME AND CREATE THE APPROPRIATE STRING ** //
    bzero(buffer, strlen(buffer));

    // ** FIND THE USER IN THE VECTOR TO REMOVE HIM ** //
    for(int i = 0 ; i < (int)clients.size() ; i++){
        if(clients[i] == user){
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

// FUNCTION TO CHECK FOR AND REMOVE START AND END TOKENS ** //
string checkTokens(char* buffer) {
    string str = string(buffer);

    str.erase(remove(str.begin(), str.end(), '\n'), str.end());
    // ** CHECKING FOR TOKENS AT BEGINNING AND END ** //
    if(str.find("\x1") == 0 && str.compare(str.size() - 1, 1, "\x4") == 0)
    {
        // ** CHECK IF THE END TOKEN ALSO APPEARS SOMEWHERE ELSE IN THE MESSAGE ** //
        // ** IF THE STRING HAS BEEN BITSTUFFED WE SHOULD FIND THE END TOKEN DUPLICATED ** //
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

// ** PREPARES BUFFER TO CONNECT TO A NEW SOCKET ** //
void connectTo(char* buffer){
    string address;
    string tcpport;
    struct sockaddr_in serv_addr;

    manageBuffer(buffer, address);
    manageBuffer(buffer, tcpport);
    bzero((char*)&serv_addr, sizeof(serv_addr));
    connectToServer(serv_addr, address, stoi(tcpport));
}

// ** CONSTRUCTS A ROUTING TABLE WITH SOME HORRIBLY UGLY AND INEFFICIENT CODE ** //
string LISTROUTES(){
    // ** I tried, I really tried with dijkstra's algorithm but nothing worked... so, our LISTROUTES doesn't show more than 2 hops ** //
    string ourID = string(serverID);
    string table = ourID + ";";
    vector<ShortClientInfo*> c;

    // ** ADD ALL 1-HOP SERVERS TO THE VECTOR AND SAY THEY CONNECT WITH OUR SERVER ** //
    for(int i = 0 ; i < (int)clients.size() ; i++){
        if(clients[i]->hasUsername && !clients[i]->isOurClient){
            c.push_back(new ShortClientInfo(clients[i]->userName, ourID));
        }
    }


    for(int i = 0 ; i < (int)clients.size() ; i++){

        if(clients[i]->hasUsername && !clients[i]->isOurClient){

            // ** GO THROUGH THE SERVERIDS WE HAVE RECEIVED FROM EACH 1-HOP CONNECTED SERVERS ** //
            for(int j = 0 ; j < (int)clients[i]->hasRouteTo.size() ; j++){

                bool skip = false;
                for(int k = 0 ; k < (int)c.size() ; k++){
                    // CHECK IF THIS SERVER IS ALREADY IN THE VECTOR OR IF IT'S ONLY CONNECTION IS TO US ** //
                    if((c[k]->ID == clients[i]->hasRouteTo[j] && c[k]->through == ourID) || (clients[i]->hasRouteTo[j] == serverID)){
                        skip = true;
                        break;
                    }
                }
                if(skip == false){
                    // ** IF THIS SERVER ISN'T IN THE VECTOR AND IS CONNECTED TO OTHER SERVERS, WE ADD IT TO THE VECTOR AND THROUCH WHAT SERVER IT IS CONNECTED TO US ** //
                    c.push_back(new ShortClientInfo(clients[i]->hasRouteTo[j], clients[i]->userName));
                }
            }
        }
    }

    for(int i = 0 ; i < (int)c.size() ; i++){
        // ** READ THE VECTOR TO A STRING ** //
        table += c[i]->ID + ":" + c[i]->through + ";";
    }
    // ** ADD TIMESTAMP ** //
    table += getReadableTime();

    return table;
}

// ** HANDLES A CMD COMMAND FROM OTHER SERVERS, MAINLY CONSTRUCTS RSP MESSAGES ** //
void CMD(string originalBuffer, char* buffer, ClientInfo* &user, string srvcmd, string toServerID, string fromServerID, string localIP){

    string ID;
    string hashNumber;
    if(toServerID.empty() || toServerID == " " ||  toServerID == fromServerID || toServerID == serverID){
        if (srvcmd == "LISTSERVERS") {
            cout << "Other server requested a LISTSERVERS" << endl;
            string package = listServersReply();
            bzero(buffer, strlen(buffer));
            strcpy(buffer, "RSP,");
            strcat(buffer, fromServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, serverID);
            strcat(buffer, ",LISTSERVERS,");
            strcat(buffer, package.c_str());
            cout << "Sending LISTSERVERS via TCP" << endl;
            string str = addTokens(buffer);
            strcpy(buffer, str.c_str());
            send(user->socketVal, buffer, strlen(buffer), 0);
        }
        if (srvcmd == "ID}") {

            cout << fromServerID << " has requested to connect" << endl;
            bzero(buffer, strlen(buffer));
            strcpy(buffer, "RSP,");
            strcat(buffer, fromServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, serverID);
            strcat(buffer, ",ID,");
            strcat(buffer, serverID);
            strcat(buffer, ",130.208.243.61,");
            strcat(buffer, cTCP);
            cout << "sending: "<< buffer << endl;
            string str = addTokens(buffer);
            strcpy(buffer, str.c_str());
            send(user->socketVal, buffer, strlen(buffer), 0);
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
            cout << "sending: "<< buffer << endl;
            send(user->socketVal, buffer, strlen(buffer), 0);
        }
        if(srvcmd == "LISTROUTES"){
            string routes = LISTROUTES();
            strcpy(buffer, "RSP,");
            strcat(buffer, fromServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, serverID);
            strcat(buffer, ",");
            strcat(buffer, routes.c_str());
            string buff = addTokens(buffer);
            strcpy(buffer, buff.c_str());
            send(user->socketVal, buffer, strlen(buffer), 0);
        }
    }
    // IF THE CMD IS NOT FOR US. TRY TO FIGURE OUT WHERE TO SEND IT ** //
    else if(fromServerID == serverID) {
        for(int i = 0 ; i < (int)clients.size() ; i++){
            if(toServerID == clients[i]->userName){
                strcpy(buffer, originalBuffer.c_str());
                string str = addTokens(buffer);
                strcpy(buffer, str.c_str());
                cout << "sending: " << buffer << endl;
                send(clients[i]->socketVal, buffer, strlen(buffer), 0);
            }
        }
    }

    if (srvcmd == "KEEPALIVE") {
        cout << "Peer sent KEEPALIVE" << endl;
        user->lastRcvKA = getTime();
    }
}

// ** HANDLES RESPONCES FROM OTHER SERVERS ** //
void RSP(string originalBuffer, char* buffer, ClientInfo* user, string srvcmd, string toServerID, string fromServerID){

    string ID;
    string IP;
    string tcpport;
    string hash;
    string remoteServerID = fromServerID;
    if (toServerID == serverID) {
        if (srvcmd == "ID") {

            manageBuffer(buffer, ID);
            manageBuffer(buffer, IP);
            manageBuffer(buffer, tcpport);
            cout << "ID RSP received - Sending LISTSERVERS to " << fromServerID << endl;
            bzero(buffer, strlen(buffer));
            strcpy(buffer, "CMD");
            strcat(buffer, ",");
            strcat(buffer, fromServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, toServerID.c_str());
            strcat(buffer, ",");
            strcat(buffer, "LISTSERVERS");

            // ** SAVE THEIR SERVERID ** //
            user->userName = fromServerID;
            user->hasUsername = true;

            // ** SAVE THEIR IP AND PORT NUMBER. IF STATEMENT IN CASE OF THE OTHER SERVER SENDING ONLY THEIR ID INSTEAD OF ID, IP, PORT ** //
            if(ID != IP){
                user->peerName = IP;

                if(ID != tcpport){
                    user->tcpPort = atoi(tcpport.c_str());
                }
            }

            // ** JUST TO SEE WHAT'S GOING ON SERVERSIDE ** //
            cout << "Peer added to list of clients with username: " << fromServerID << endl;
            cout << "Connected users are now: " << endl;
            for (int i = 0; i < (int)clients.size(); i++) {
                cout << clients[i]->userName << endl;
            }

            string str = addTokens(buffer);
            strcpy(buffer, str.c_str());
            cout << "sending: " << buffer << endl;
            send(user->socketVal, buffer, strlen(buffer), 0);
        }

        // ** IF ANOTHER SERVER SENDS RSP,V_GROUP_15,fromServerID,LISTSERVERS,fromServerID,IP,port - WE EXTRACT THE INFORMATION ABOUT THEM AND ADD THEM TO OUR CLIENT INFORMATION ** //
        else if (srvcmd == "LISTSERVERS") {
            string serv;
            cout << "User sent their list of servers: " << buffer << endl;

            // ** SAVE THE WHOLE STRING ** //
            user->ListServers = string(buffer);

            // ** BUT ALSO, PARSE THROUGH IT AND ADD EACH ID TO A VECTOR OF STRINGS BELONGIN TO THIS SERVER ** //
            stringstream ss(buffer);
            while(getline(ss, serv, ';')){
                int pos = serv.find_first_of(",",0);
                serv = serv.substr(0,pos);
                user->hasRouteTo.push_back(serv);
                cout << serv << endl;
            }
        }

        // ** SAVE HASHES WE RECEIVE TO A FILE ** //
        if (srvcmd == "FETCH") {
            manageBuffer(buffer, hash);
            ofstream hashFile;
            hashFile.open("hashes.txt");
            hashFile << buffer;
            hashFile.close();
            cout << "The hash is: " << buffer << endl;
        }
    }

    // ** IF THE RSP IS NOT FOR US, WE TRY TO FIND OUT WHO IT IS FOR AND FORWARD IT ** //
    else {
        for (int i = 0; i < (int)clients.size(); i++) {
            if (toServerID == clients[i]->userName) {
                send(clients[i]->socketVal, buffer, strlen(buffer), 0);
            }
            else {
                for (int j = 0; j < (int)clients[i]->hasRouteTo.size(); j++) {
                    if (toServerID == clients[i]->hasRouteTo[j]) {
                        send(clients[i]->socketVal, buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }
}

// ** HANDLES THE CONNECT COMMAND FROM OUR CLIENT ** //
void CONNECT(char* buffer, ClientInfo* user){
    if(user->isOurClient){
        string username;
        buffer[sizeof(buffer)-1] = '\0';
        manageBuffer(buffer, username);
        user->userName = username;
        user->hasUsername = true;

        // ** SEND THE USER A CONFIRMATION THAT HE IS NOW CONNECTED TO THE SERVER ** //
        strcpy(buffer, "You are now connected to the server as \"");
        strcat(buffer, &user->userName[0]);
        strcat(buffer, "\"");
        string str = addTokens(buffer);
        strcpy(buffer, str.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
    }
}

// ** HANDLES THE ID COMMAND FROM OUR CLIENT ** //
void ID(char* buffer, ClientInfo* user){
    bzero(buffer, strlen(buffer));
    strcpy(buffer, serverID);
    string str = addTokens(buffer);
    strcpy(buffer, str.c_str());
    send(user->socketVal, buffer, strlen(buffer), 0);
}

// ** SIMILAR TO LISTSERVERS, BUT MORE READABLE FOR OUR CLIENT ** //
void WHO(char* buffer, ClientInfo* user){
    bzero(buffer, strlen(buffer));
    // ** ADD USERS WHO HAVE A USERNAME, AND THEREFOR CONNECTED TO THE BUFFER ** //
    string numberofservers = "There are " + to_string(clients.size() - 1) + " servers connected to our server;";
    strcpy(buffer, numberofservers.c_str());

    for (int i = 0; i < (int)clients.size(); i++) {
        if(clients[i]->hasUsername){
            stringstream ss;
            ss << (i + 1);
            string number;
            ss >> number;
            string str = "User " + number + ": " + clients[i]->userName + ";";
            strcat(buffer, str.c_str());
        }
    }
    cout << buffer << endl;
    string str = addTokens(buffer);
    strcpy(buffer, str.c_str());
    send(user->socketVal, buffer, strlen(buffer), 0);
}

/// should we just remove this? ################################################
void MSG(char* buffer, ClientInfo* user, string name){
    //since MSG is a 2 word command, we need the second word


    manageBuffer(buffer, name);

    char tempBuffer[strlen(buffer)] ;
    strcpy(tempBuffer, buffer);

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
/// #############################################################################


// ** DECIDES WHAT TO DO BASED ON THE COMMAND RECEIVED FROM THE CLIENT ** //
void handleConnection(char* buffer, int messageCheck, ClientInfo* user, string localIP) {
    string command;
    string name;
    string toServerID;
    string fromServerID;
    string srvcmd;
    string IP;
    string tcpport;
    string udpport;
    string originalBuffer = string(buffer);

    // ** GET THE COMMAND ** //
    manageBuffer(buffer, command);

    if (command == "connectTo") {
        if(!isFull()){
            connectTo(buffer);
        }
        else{
            strcpy(buffer, "Server is full");
            string buff = addTokens(buffer);
            strcpy(buffer, buff.c_str());
            send(user->socketVal, buffer, strlen(buffer), 0);
        }
    }
    if (command == "KEEPALIVE"){
        cout << "Peer sent KEEPALIVE" << endl;
        user->lastRcvKA = getTime();
    }

    if(command == "LISTROUTES"){
        string routes = LISTROUTES();
        strcpy(buffer, routes.c_str());
        string buff = addTokens(buffer);
        strcpy(buffer, buff.c_str());
        send(user->socketVal, buffer, strlen(buffer), 0);
    }

    if (command == "CMD"|| command == "RSP") {

        manageBuffer(buffer, toServerID);
        manageBuffer(buffer, fromServerID);
        manageBuffer(buffer, srvcmd);

        if(command != "RSP"){
            CMD(originalBuffer, buffer, user, srvcmd, toServerID, fromServerID, localIP);
        }
        else{
            RSP(originalBuffer, buffer, user, srvcmd, toServerID, fromServerID);
        }
    }

    if (command == "CONNECT") {
        CONNECT(buffer, user);
    }

    // ** SEND THE CURRENT SERVER ID TO THE USER ** //
    else if(command == "ID"){
        ID(buffer, user);
    }

    // ** SEND THE USER A LIST OF CLIENTS WHO ARE CONNECTED ** //
    else if (command == "WHO" && user->hasUsername) {
        WHO(buffer, user);
    }

    // ** REMMOVE THE USER FROM THE SERVER ** //
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

    // ** SENDS A MESSAGE TO A SPECIFIC OR ALL USERS ** //
    else if(command == "MSG" && user->hasUsername) {
        MSG(buffer, user, name);
    }
}

int main(int argc, char* argv[]) {
    ///####################### VARIABLE INITIALIZATION #######################

    int listeningSock, UDPsock, newSock, messageCheck, socketVal, maxSocketVal;
    struct sockaddr_in serv_addr, cli_addr;
    struct sockaddr_storage addr_udp;
    struct timeval tv;
    socklen_t cli_addrlen, udp_addrlen;
    cTCP = argv[1];
    cUDP = argv[2];
    TCPport = atoi(cTCP);
    UDPport = atoi(cUDP);
    stillAliveTime = getTime();


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

    openTCPPort(serv_addr, listeningSock, TCPport);
    openUDPPort(UDPsock);

    listen(listeningSock, 5);

    cli_addrlen = sizeof(cli_addr);
    udp_addrlen = sizeof(addr_udp);

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
                else if(t - clients[i]->lastRcvKA  > 10 && !clients[i]->hasUsername && !clients[i]->isOurClient){
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

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        //wait for something to happen on some socket. Will wait forever if nothing happens.
        if (select(maxSocketVal + 1, &masterFD, NULL, NULL, &tv) > 0) {

            //if something happens on the listening socket, someone is trying to connect to the server.
            if(FD_ISSET(listeningSock, &masterFD) && !isFull()){

                newSock = accept(listeningSock, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
                validateSocket(newSock);

                getpeername(newSock, (struct sockaddr * )&cli_addr, (socklen_t *)&cli_addrlen);
                int time = getTime();
                clients.push_back(new ClientInfo(newSock, inet_ntoa(cli_addr.sin_addr), time));

                cout << "Sending message to other server: ";
                strcpy(message, "CMD,,V_GROUP_15,ID");
                //cout << message << endl;
                string str = addTokens(message);
                strcpy(message, str.c_str());
                cout << "sending: " << message << endl;
                send(newSock, message, strlen(message), 0);
            }

            //if something happens on any other socket, it's probably a message
            if(FD_ISSET(UDPsock, &masterFD)) {

                listenForUDP(UDPsock, addr_udp, udp_addrlen, UDPbuffer, localIP);
            }

            for(int i = 0 ; i < (int)clients.size() ; i++){
                socketVal = clients[i]->socketVal;

                if(FD_ISSET(socketVal, &masterFD)){
                    bzero(message, sizeof(message));
                    messageCheck = read(socketVal, message, sizeof(message));
                    message[messageCheck] = '\0';
                    if(messageCheck != 0){
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
