# TSAM_Project3

Tölvusamskipti - Haustönn 2018 - Project 3

In this programming assignment we were asked to write a simple botnet server, with accompanying
Command and Control (C&C) client.

We used Linux Ubuntu 18.04 to compile and run.

g++ compiler needs to be installed:
  sudo apt install g++
  
  
Instructions to test/run:

On a local machine:

1. On local machine: Open a terminal window in the folder with the server.cpp file
2. Compile the server: g++ server.cpp -o tsam<groupname>
3. Run the server: ./<groupname> <tcp port> <udp port>
4. Open another terminal window in the folder with the client.cpp file
5. Compile the client: g++ client.cpp -o client
6. Run the client: ./client <ip> <tcp port>
7. Now you should be able to play around with the commands listed 
  
On Skel:

1. Open Skel and log in. Go to the folder containing the server.cpp file
2. Compile the server: g++ --std=c++11 server.cpp -o tsam<groupname>
3. Run the server: ./<groupname> <tcp port> <udp port>
4. Open a new Skel window and log in. Go to the folder containing the client.cpp file
5. Compile the client: g++ --std=c++11 client.cpp -o client
6. Run the client: ./client <ip> <tcp port>
7. Now you should be able to play around with the commands listed 
