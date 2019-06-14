#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "types.h"

SOCKET connectionSocket;

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

bool sendLegacyRequest() {
    byte legacyPingPacket[2] = {0xFE, 01};
    int lastError = 0;
    
    lastError = send(connectionSocket, legacyPingPacket, 2, 0);
    if (lastError == SOCKET_ERROR) {
        return false;
    }
    
    return true;
}

bool getRawResponse(byte *buffer, u32 bufferlen) {
    int lastError = 0;
    
    recv(connectionSocket, buffer, bufferlen, 0);
    if(lastError == SOCKET_ERROR) {
        return false;
    }
    
    return true;
}

void swapWcharStrEndiannessLen(u16 *string, i32 length) {
    for(int index = 0; index < length; index++) {
        u16 newValue = 0;
        newValue |= (string[index] & 0xFF00) >> 8;
        newValue |= (string[index] & 0x00FF) << 8;
        
        string[index] = newValue;
    }
}

u16 getLegacyPayloadStrlen() {
    u16 result = 0;
    byte resultHeader[3] = {0};
    
    getRawResponse(resultHeader, 3);
    
    result |= resultHeader[2];
    result |= (resultHeader[1] << 8);
    
    return result;
}

void printLegacyPayloadV0(u16 *payload, u16 payloadLen) {
    u16 *currentStr = NULL;
    u16 *wcstrtokContext = NULL;
    
    currentStr = wcstok_s(payload, L"§", &wcstrtokContext);
    wprintf(L"MOTD: %s\n", currentStr);
    
    currentStr = wcstok_s(NULL, L"§", &wcstrtokContext);
    wprintf(L"Online: %s\n", currentStr);
    
    currentStr = wcstok_s(NULL, L"§", &wcstrtokContext);
    wprintf(L"Max Players: %s\n", currentStr);
}

void printLegacyPayloadV1(u16 *payload, u16 payloadLen) {
    
}

i32 main(int argc, char **argv) {
    i32 lastError = 0;
    WSADATA wsaData = {0};
    
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    
    connectionSocket = INVALID_SOCKET;
    
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
    
    // Send request
    sendLegacyRequest();
    
    //Get result
    u16 payloadStrLen = getLegacyPayloadStrlen();
    payloadStrLen++; // Add room for a null terminator
    u32 payloadBufferSize = payloadStrLen * sizeof(u16); // Make each character 2 bytes
    u16 *payloadBuffer = (u16 *) malloc(payloadBufferSize);
    memset(payloadBuffer, 0, payloadBufferSize);
    
    getRawResponse((byte *) payloadBuffer, payloadBufferSize);
    
    swapWcharStrEndiannessLen(payloadBuffer, payloadBufferSize / 2);
    
    //Print
    // Magic characters for protocol v1
    if(payloadBuffer[0] == 167 && payloadBuffer[1] == 49) {
        printLegacyPayloadV1(payloadBuffer, payloadStrLen);
    }
    else {
        printLegacyPayloadV0(payloadBuffer, payloadStrLen);
    }
    
    DebugBreak();
    
    free(payloadBuffer);
    
    shutdown(connectionSocket, SD_SEND);
    closesocket(connectionSocket);
    WSACleanup();
    
    return 0;
}