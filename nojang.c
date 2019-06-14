#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <string.h>

#include "types.h"

i32 readVarInt(SOCKET socket) {
    i32 bytesRead = 0;
    i32 result = 0;
    byte read = 0;
    
    do {
        recv(socket, &read, 1, 0);
        int value = (read & 0b01111111);
        result |= (value << (7 * bytesRead));
        
        bytesRead++;
        if(bytesRead > 5) {
            printf("VarInt too big!\n");
        }
    }
    while(read & 0b10000000);
    
    return result;
}

i32 main(int argc, char **argv) {
    byte outgoingHandshake[19] = {0x12, 0x10, 0xFF, 0xFF, 0xFF, 0x0F, 0x6C, 0x6F, 0x63, 0x61, 0x6C, 0x68, 0x6F, 0x73, 0x74, 0x00, 0x63, 0xDD, 0x01};
    byte requestPacket[2] = {0x01, 0x00};
    
    i32 lastError = 0;
    WSADATA wsaData = {0};
    
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    SOCKET connectionSocket = INVALID_SOCKET;
    
    printf("Goodbye Notch\n");
    
    lastError = WSAStartup(MAKEWORD(2, 2), &wsaData); // Request version 2.2
    if(lastError) {
        printf("Winsoc failed to start, error: %d\n", lastError);
        return 1;
    }
    
    printf("Winsoc version %u.%u\n", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    lastError = getaddrinfo("localhost", "25565", &hints, &result);
    if(lastError) {
        printf("Getaddrinfo failed, error: %d\n", lastError);
        WSACleanup();
        return 1;
    }
    
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
        
        // Create a SOCKET for connecting to server
        connectionSocket = socket(ptr->ai_family, ptr->ai_socktype,
                                  ptr->ai_protocol);
        if (connectionSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }
        
        // Connect to server.
        lastError = connect( connectionSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (lastError == SOCKET_ERROR) {
            closesocket(connectionSocket);
            connectionSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    
    freeaddrinfo(result);
    
    if (connectionSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    
    /*
        // Send request to server
        lastError = send(connectionSocket, outgoingHandshake, 19, 0);
        if(lastError) {
            if (lastError == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(connectionSocket);
                WSACleanup();
                return 1;
            }
        }
        
        lastError = send(connectionSocket, requestPacket, 2, 0);
        if(lastError) {
            if (lastError == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(connectionSocket);
                WSACleanup();
                return 1;
            }
        }
        
//Get response
    int responseLength = readVarInt(connectionSocket);
    byte *responseBuffer = (byte *) malloc(responseLength + 1);
    memset(responseBuffer, 0, responseLength + 1);
    recv(connectionSocket, responseBuffer, responseLength, 0);
         */
    
    // Send legacy ping
    byte legacyPing = 0xFE;
    lastError = send(connectionSocket, &legacyPing, 1, 0);
    if (lastError == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectionSocket);
        WSACleanup();
        return 1;
    }
    
    //Get result
    byte resultHeader[3] = {0};
    recv(connectionSocket, resultHeader, 3, 0);
    u16 resultPayloadSize = 0;
    resultPayloadSize |= resultHeader[2];
    resultPayloadSize |= (resultHeader[1] << 8);
    resultPayloadSize++; // Account for null terminator
    resultPayloadSize *= sizeof(u16); // Account for two byte wide characters
    
    u16 *resultPayloadBuffer = (u16 *) malloc(resultPayloadSize);
    memset(resultPayloadBuffer, 0, resultPayloadSize);
    recv(connectionSocket, (byte *) resultPayloadBuffer, resultPayloadSize - 2, 0); // (resultPayloadSize - 2): don't overwrite the null terminator
    
    // Swap endianness of every character
    for(int i = 0; i < resultPayloadSize / 2; i++) {
        u16 newValue = 0;
        newValue |= (resultPayloadBuffer[i] & 0xFF00) >> 8;
        newValue |= (resultPayloadBuffer[i] & 0x00FF) << 8;
        
        resultPayloadBuffer[i] = newValue;
    }
    
    DebugBreak();
    
    free(resultPayloadBuffer);
    
    shutdown(connectionSocket, SD_SEND);
    closesocket(connectionSocket);
    WSACleanup();
    
    return 0;
}