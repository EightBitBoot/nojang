#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "types.h"

SOCKET connectionSocket;

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
    // TODO: Check that offsetToNextString doesn't run past the end of payload
    
    i32 offsetToNextString = 3; // Skip v1 magic string and null terminator
    u16 *currentString = payload + offsetToNextString;
    
    wprintf(L"Protocol version: %s\n", currentString);
    offsetToNextString += wcslen(currentString) + 1; // Account for null terminator
    currentString = payload + offsetToNextString;
    
    wprintf(L"Server Version: %s\n", currentString);
    offsetToNextString += wcslen(currentString) + 1;
    currentString = payload + offsetToNextString;
    
    wprintf(L"MOTD: %s\n", currentString);
    offsetToNextString += wcslen(currentString) + 1;
    currentString = payload + offsetToNextString;
    
    wprintf(L"Online: %s\n", currentString);
    offsetToNextString += wcslen(currentString) + 1;
    currentString = payload + offsetToNextString;
    
    wprintf(L"Max Players: %s\n", currentString);
}

i32 main(int argc, char **argv) {
    i32 lastError = 0;
    WSADATA wsaData = {0};
    
    char *hostname = NULL;
    char *hostport = NULL;
    
    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    
    connectionSocket = INVALID_SOCKET;
    
    //printf("Goodbye Notch\n");
    
    if(argc < 2) {
        printf("Usage: nojang <server-ip>\n");
        return 1;
    }
    
    // Test if user supplied a port
    if(strchr(argv[1], ':') != NULL) {
        char *currentToken = NULL;
        i32 hostSize = 0;
        i32 portSize = 0;
        
        currentToken = strtok(argv[1], ":");
        hostSize = strlen(currentToken) + 1;
        hostname = malloc(hostSize);
        memset(hostname, 0, hostSize);
        strcpy(hostname, currentToken);
        
        currentToken = strtok(NULL, ":");
        portSize = strlen(currentToken) + 1;
        hostport = malloc(portSize);
        memset(hostport, 0, portSize);
        strcpy(hostport, currentToken);
    }
    else {
        // No port supplied hostname is just parameter; port is default
        hostname = malloc(strlen(argv[1]) + 1);
        memset(hostname, 0, strlen(argv[1]) + 1);
        strcpy(hostname, argv[1]);
        
        hostport = malloc(6); // strlen("25565") + null terminator
        memset(hostport, 0, strlen(argv[1]) + 1);
        strcpy(hostport, "25565");
    }
    
    lastError = WSAStartup(MAKEWORD(2, 2), &wsaData); // Request winsock version 2.2
    if(lastError) {
        printf("Winsoc failed to start, error: %d\n", lastError);
        return 1;
    }
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    lastError = getaddrinfo(hostname, hostport, &hints, &result);
    free(hostname);
    free(hostport);
    if(lastError) {
        if(lastError == WSAHOST_NOT_FOUND) {
            printf("Unable to connect to server \"%s\"\n", argv[1]);
        }
        else {
            printf("Getaddrinfo failed, error: %d\n", lastError);
        }
        
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
            printf("Connect failed with error: %ld\n", WSAGetLastError());
            closesocket(connectionSocket);
            connectionSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    
    freeaddrinfo(result);
    
    if (connectionSocket == INVALID_SOCKET) {
        printf("Unable to connect to server \"%s\"\n", argv[1]);
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
    
    free(payloadBuffer);
    
    shutdown(connectionSocket, SD_SEND);
    closesocket(connectionSocket);
    WSACleanup();
    
    return 0;
}