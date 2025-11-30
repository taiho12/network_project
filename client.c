#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 3000
#define MAX 4096

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("Cannot connect to server.\n");
        return 0;
    }

    while (1) {
        printf("\n===== MENU =====\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("0. Exit\n");
        printf("Your choice: ");

        int choice;
        scanf("%d", &choice);

        char message[MAX], recvbuf[MAX];
        char username[50], email[100], password[50];

        if (choice == 0) {
            send(sockfd, "logout", 6, 0);
            break;
        }

        if (choice == 1) {
            printf("username: "); scanf("%s", username);
            printf("email: "); scanf("%s", email);
            printf("password: "); scanf("%s", password);

            sprintf(message, "register %s %s %s", username, email, password);
            send(sockfd, message, strlen(message), 0);

            int n = recv(sockfd, recvbuf, MAX, 0);
            recvbuf[n] = '\0';
            printf("SERVER: %s\n", recvbuf);
        }

        else if (choice == 2) {

            printf("username: "); scanf("%s", username);
            printf("password: "); scanf("%s", password);

            sprintf(message, "login %s %s", username, password);
            send(sockfd, message, strlen(message), 0);

            int n = recv(sockfd, recvbuf, MAX, 0);
            recvbuf[n] = '\0';
            printf("SERVER: %s\n", recvbuf);

            if (strcmp(recvbuf, "login success") == 0) {

                while (1) {
                    printf("\n===== LOGIN MENU =====\n");
                    printf("1. Search Flight\n");
                    printf("2. Booking\n");
                    printf("3. View Tickets\n");
                    printf("4. Cancel Ticket\n");
                    printf("0. Logout\n");
                    printf("Your choice: ");

                    int opt;
                    scanf("%d", &opt);

                    if (opt == 0) {
                        send(sockfd, "logout", 6, 0);
                        goto END;
                    }

                    if (opt == 1) {
                        char from[20], to[20], date[20];

                        printf("From: "); scanf("%s", from);
                        printf("To: "); scanf("%s", to);
                        printf("Date (dd/mm/yyyy): "); scanf("%s", date);

                        sprintf(message, "search %s %s %s", from, to, date);
                        send(sockfd, message, strlen(message), 0);

                        int m = recv(sockfd, recvbuf, MAX, 0);
                        recvbuf[m] = '\0';

                        printf("\n===== SEARCH RESULT =====\n");
                        printf("%s\n", recvbuf);
                    }

                    if (opt == 2) {
                        char flight_id[20], date[20], passenger[50];

                        printf("Flight ID: "); scanf("%s", flight_id);
                        printf("Date: "); scanf("%s", date);
                        printf("Passenger Name (no spaces): "); scanf("%s", passenger);

                        sprintf(message, "book %s %s %s", flight_id, date, passenger);
                        send(sockfd, message, strlen(message), 0);

                        int m = recv(sockfd, recvbuf, MAX, 0);
                        recvbuf[m] = '\0';

                        if (strncmp(recvbuf, "payment_required", 16) == 0) {
                            printf("\n===== PAYMENT REQUIRED =====\n");

                            int method;
                            printf("Choose method:\n1. Visa\n2. Napas\nYour choice: ");
                            scanf("%d", &method);

                            char cardType[10];
                            if (method == 1) strcpy(cardType, "VISA");
                            else strcpy(cardType, "NAPAS");

                            char cardnum[30], exp[10], cvv[10];

                            printf("Card number: "); scanf("%s", cardnum);
                            printf("Expiry (MM/YY): "); scanf("%s", exp);
                            printf("CVV: "); scanf("%s", cvv);

                            sprintf(message, "payment %s %s %s %s",
                                    cardType, cardnum, exp, cvv);

                            send(sockfd, message, strlen(message), 0);

                            int k = recv(sockfd, recvbuf, MAX, 0);
                            recvbuf[k] = '\0';

                            printf("\n===== PAYMENT RESULT =====\n%s\n", recvbuf);
                        }

                        else {
                            printf("\n===== BOOKING RESULT =====\n%s\n", recvbuf);
                        }
                    }

                    if (opt == 3) {
                        sprintf(message, "viewticket");
                        send(sockfd, message, strlen(message), 0);

                        int m = recv(sockfd, recvbuf, MAX, 0);
                        recvbuf[m] = '\0';

                        printf("\n===== YOUR TICKETS =====\n%s\n", recvbuf);
                    }

                    if (opt == 4) {
                        char tid[20];
                        printf("Enter Ticket ID: ");
                        scanf("%s", tid);

                        sprintf(message, "cancel %s", tid);
                        send(sockfd, message, strlen(message), 0);

                        int m = recv(sockfd, recvbuf, MAX, 0);
                        recvbuf[m] = '\0';

                        printf("\n===== CANCEL RESULT =====\n%s\n", recvbuf);
                    }
                }
            }
        }
    }

END:
    closesocket(sockfd);
    WSACleanup();
    return 0;
}
