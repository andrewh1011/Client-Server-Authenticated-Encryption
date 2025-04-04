# Client-Server-Authenticated-Encryption

## Description

Client and server programs that communicate using AES-256-GCM encryption. Uses a Docker container to simulate two different machines on the same network. The server is hosted on "HostA" (10.9.0.5) and the client is hosted on "HostB" (10.9.0.6). 

Created on SEEDUbuntu 20.04 VM.

## Running Programs

Compile c programs:

    gcc client.c -o client -lssl -lcrypto
    gcc server.c -o server -lssl -lcrypto

Compose Docker file: 

    sudo docker-compose -f docker-compose.yml up

List IDs of containers: 

    sudo docker ps --format "{{.ID}} {{.Names}}"

Login to HostA and HostB with:

    sudo docker exec -it <id> /bin/bash

Once on the containers, cd into the volumes directory. There you can run the programs:

    ./server
    ./client

## Results

Host B/Client:

<img width="856" alt="Screenshot 2025-04-04 at 5 33 31 PM" src="https://github.com/user-attachments/assets/846811cc-055a-4ec7-8e1b-cb72f81b409d" />

Host A/Server:

<img width="858" alt="Screenshot 2025-04-04 at 5 32 31 PM" src="https://github.com/user-attachments/assets/dc227f64-a4f7-4995-bbbd-1e502f011c45" />

