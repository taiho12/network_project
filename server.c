#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 3000
#define MAX 4096

typedef struct {
    char id[20], from[10], to[10], date[20];
    char depart[10], arrive[10];
    char price[20];
    int seats;
    char status[20];
    int delay;
} Flight;

int register_user(char *username, char *email, char *password) {
    FILE *f = fopen("database.txt", "a");
    if (!f) return 0;

    fprintf(f, "%s %s %s %s %s\n",
            username, email, password, username, "0000000000");

    fclose(f);
    return 1;
}

int login_user(char *username, char *password) {
    FILE *f = fopen("database.txt", "r");
    if (!f) return 0;

    char u[50], email[100], p[50], fullname[100], phone[20];

    while (fscanf(f, "%s %s %s %s %s", u, email, p, fullname, phone) != EOF) {
        if (strcmp(username, u) == 0 && strcmp(password, p) == 0) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

int load_flights(Flight list[], int *count) {
    FILE *f = fopen("flight.txt", "r");
    if (!f) return 0;

    *count = 0;

    while (fscanf(f, "%s %s %s %s %s %s %s %d %s %d",
                  list[*count].id,
                  list[*count].from,
                  list[*count].to,
                  list[*count].date,
                  list[*count].depart,
                  list[*count].arrive,
                  list[*count].price,
                  &list[*count].seats,
                  list[*count].status,
                  &list[*count].delay) != EOF)
    {
        (*count)++;
    }

    fclose(f);
    return 1;
}

void save_flights(Flight list[], int count) {
    FILE *f = fopen("flight.txt", "w");

    for (int i = 0; i < count; i++) {
        fprintf(f, "%s %s %s %s %s %s %s %d %s %d\n",
                list[i].id, list[i].from, list[i].to, list[i].date,
                list[i].depart, list[i].arrive, list[i].price,
                list[i].seats, list[i].status, list[i].delay);
    }

    fclose(f);
}

void search_flight(char *from, char *to, char *date, char *result) {
    Flight list[500];
    int count;

    result[0] = '\0';

    if (!load_flights(list, &count)) {
        strcpy(result, "cannot load flight data");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (strcmp(list[i].from, from) == 0 &&
            strcmp(list[i].to, to) == 0 &&
            strcmp(list[i].date, date) == 0)
        {
            char line[300];
            sprintf(line,
                "%s %s-%s %s %s-%s %s seats:%d status:%s delay:%d\n",
                list[i].id, list[i].from, list[i].to, list[i].date,
                list[i].depart, list[i].arrive, list[i].price,
                list[i].seats, list[i].status, list[i].delay);

            strcat(result, line);
        }
    }

    if (strlen(result) == 0)
        strcpy(result, "no flight found");
}

void next_ticket_id(char *tid) {
    FILE *f = fopen("ticket.txt", "r");

    if (!f) {
        strcpy(tid, "T001");
        return;
    }

    char TID[20], user[50], fid[20], pname[50], price[20], status[20], changeinfo[50];
    char last_id[20] = "T000";

    while (fscanf(f,
        "%s %s %s %s %s %s %s",
        TID, user, fid, pname, price, status, changeinfo) != EOF)
    {
        strcpy(last_id, TID);
    }

    fclose(f);

    int num = atoi(last_id + 1);
    sprintf(tid, "T%03d", num + 1);
}

int book_ticket(char *username, char *fid, char *date, char *passenger, char *msg) {
    Flight list[500];
    int count;

    if (!load_flights(list, &count)) {
        strcpy(msg, "cannot load flights");
        return 0;
    }

    int idx = -1;

    for (int i = 0; i < count; i++) {
        if (strcmp(list[i].id, fid) == 0 &&
            strcmp(list[i].date, date) == 0)
        {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        strcpy(msg, "flight not found");
        return 0;
    }

    Flight *f = &list[idx];

    if (strcmp(f->status, "cancelled") == 0) {
        strcpy(msg, "flight cancelled");
        return 0;
    }

    if (f->seats <= 0) {
        strcpy(msg, "no seats available");
        return 0;
    }

    char tid[20];
    next_ticket_id(tid);

    FILE *tk = fopen("ticket.txt", "a");
    fprintf(tk, "%s %s %s %s %s active none\n",
            tid, username, fid, passenger, f->price);
    fclose(tk);

    f->seats--;
    save_flights(list, count);

    sprintf(msg, "booking success %s", tid);
    return 1;
}

void view_my_tickets(char *username, char *result) {
    FILE *f = fopen("ticket.txt", "r");

    if (!f) {
        strcpy(result, "no tickets");
        return;
    }

    result[0] = '\0';

    char tid[20], user[50], fid[20], pname[50], price[20], status[20], changeinfo[50];

    while (fscanf(f,
        "%s %s %s %s %s %s %s",
        tid, user, fid, pname, price, status, changeinfo) != EOF)
    {
        if (strcmp(user, username) == 0) {
            char line[200];
            sprintf(line, "%s %s %s %s %s %s\n",
                    tid, fid, pname, price, status, changeinfo);
            strcat(result, line);
        }
    }

    if (strlen(result) == 0)
        strcpy(result, "no tickets");

    fclose(f);
}

int cancel_ticket(char *username, char *tid, char *msg) {

    FILE *f = fopen("ticket.txt", "r");
    if (!f) {
        strcpy(msg, "ticket file error");
        return 0;
    }

    FILE *temp = fopen("ticket_temp.txt", "w");

    char TID[20], user[50], fid[20], pname[50], price[20], status[20], changeinfo[50];

    int found = 0;
    char flight_to_update[20];

    while (fscanf(f,
        "%s %s %s %s %s %s %s",
        TID, user, fid, pname, price, status, changeinfo) != EOF)
    {
        if (strcmp(TID, tid) == 0) {

            if (strcmp(user, username) != 0) {
                fclose(f); fclose(temp);
                remove("ticket_temp.txt");
                strcpy(msg, "ticket not yours");
                return 0;
            }

            if (strcmp(status, "cancelled") == 0) {
                fclose(f); fclose(temp);
                remove("ticket_temp.txt");
                strcpy(msg, "already cancelled");
                return 0;
            }

            fprintf(temp, "%s %s %s %s %s cancelled user_cancel\n",
                    TID, user, fid, pname, price);

            strcpy(msg, "cancel success");
            found = 1;

            strcpy(flight_to_update, fid);
        }

        else {
            fprintf(temp, "%s %s %s %s %s %s %s\n",
                    TID, user, fid, pname, price, status, changeinfo);
        }
    }

    fclose(f);
    fclose(temp);

    if (!found) {
        remove("ticket_temp.txt");
        strcpy(msg, "ticket not found");
        return 0;
    }

    remove("ticket.txt");
    rename("ticket_temp.txt", "ticket.txt");

    Flight list[500];
    int count;

    if (load_flights(list, &count)) {
        for (int i = 0; i < count; i++) {
            if (strcmp(list[i].id, flight_to_update) == 0) {
                list[i].seats++;
                break;
            }
        }
        save_flights(list, count);
    }

    return 1;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET listenfd = socket(AF_INET, SOCK_STREAM, 0);
    SOCKET connfd;
    struct sockaddr_in servaddr, cliaddr;

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(listenfd, 5);

    printf("SERVER RUNNING...\n");

    while (1) {
        int len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

        printf("Client connected.\n");

        char current_user[50] = "";

        while (1) {
            char buffer[MAX] = {0};
            int n = recv(connfd, buffer, MAX, 0);

            if (n <= 0) break;
            buffer[n] = '\0';

            char msg[MAX] = "";

            char u[50], e[100], p[50];
            if (sscanf(buffer, "register %s %s %s", u, e, p) == 3) {
                if (register_user(u, e, p))
                    send(connfd, "register success", 16, 0);
                else
                    send(connfd, "register fail", 13, 0);
                continue;
            }

            if (sscanf(buffer, "login %s %s", u, p) == 2) {
                if (login_user(u, p)) {
                    strcpy(current_user, u);
                    send(connfd, "login success", 13, 0);
                } else {
                    send(connfd, "login fail", 11, 0);
                }
                continue;
            }

            if (strncmp(buffer, "search", 6) == 0) {
                char from[20], to[20], date[20];
                sscanf(buffer, "search %s %s %s", from, to, date);

                search_flight(from, to, date, msg);
                send(connfd, msg, strlen(msg), 0);
                continue;
            }

            if (strncmp(buffer, "book", 4) == 0) {
                char fid[20], date[20], passenger[50];
                sscanf(buffer, "book %s %s %s", fid, date, passenger);

                if (book_ticket(current_user, fid, date, passenger, msg))
                    send(connfd, msg, strlen(msg), 0);
                else
                    send(connfd, msg, strlen(msg), 0);

                continue;
            }

            if (strcmp(buffer, "viewticket") == 0) {
                view_my_tickets(current_user, msg);
                send(connfd, msg, strlen(msg), 0);
                continue;
            }

            if (strncmp(buffer, "cancel", 6) == 0) {
                char tid[20];
                sscanf(buffer, "cancel %s", tid);

                if (cancel_ticket(current_user, tid, msg))
                    send(connfd, msg, strlen(msg), 0);
                else
                    send(connfd, msg, strlen(msg), 0);

                continue;
            }

            if (strcmp(buffer, "logout") == 0)
                break;

            send(connfd, "unknown command", 16, 0);
        }

        closesocket(connfd);
        printf("Client disconnected.\n");
    }

    closesocket(listenfd);
    WSACleanup();
    return 0;
}
