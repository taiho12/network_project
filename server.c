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
    fprintf(f, "%s %s %s %s %s\n", username, email, password, username, "0000000000");
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

int book_flight_prepare(char *username, char *fid, char *date, char *passenger, char *price_out) {
    Flight list[500];
    int count;
    if (!load_flights(list, &count)) return -1;

    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(list[i].id, fid) == 0 &&
            strcmp(list[i].date, date) == 0)
        {
            idx = i;
            break;
        }
    }

    if (idx == -1) return -2;
    Flight *f = &list[idx];

    if (strcmp(f->status, "cancelled") == 0) return -3;
    if (f->seats <= 0) return -4;

    strcpy(price_out, f->price);
    return 1;
}

int card_is_digits(char *s) {
    for (int i = 0; s[i]; i++)
        if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}

int validate_payment(char *type, char *cardnum, char *exp, char *cvv) {
    if (!card_is_digits(cardnum)) return 0;
    if (!card_is_digits(cvv)) return 0;
    if (strlen(cvv) != 3) return 0;

    if (strcmp(type, "VISA") == 0) {
        if (strlen(cardnum) != 16) return 0;
        if (cardnum[0] != '4') return 0;
    }
    else if (strcmp(type, "NAPAS") == 0) {
        int len = strlen(cardnum);
        if (len < 16 || len > 19) return 0;
        if (strncmp(cardnum, "9704", 4) != 0) return 0;
    }
    else return 0;

    int mm, yy;
    if (sscanf(exp, "%d/%d", &mm, &yy) != 2) return 0;
    if (mm < 1 || mm > 12) return 0;

    return 1;
}

int confirm_booking(char *username, char *fid, char *date, char *passenger, char *price, char *msg, char *tid_out) {
    Flight list[500];
    int count;
    load_flights(list, &count);

    int idx = -1;
    for (int i = 0; i < count; i++)
        if (strcmp(list[i].id, fid) == 0 &&
            strcmp(list[i].date, date) == 0)
            idx = i;

    if (idx == -1) {
        strcpy(msg, "flight disappeared");
        return 0;
    }

    Flight *f = &list[idx];
    if (f->seats <= 0) {
        strcpy(msg, "no seats available");
        return 0;
    }

    char tid[20];
    next_ticket_id(tid);
    strcpy(tid_out, tid);

    FILE *tk = fopen("ticket.txt", "a");
    fprintf(tk, "%s %s %s %s %s active none\n",
            tid, username, fid, passenger, price);
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

    while (fscanf(f, "%s %s %s %s %s %s %s",
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
    load_flights(list, &count);

    for (int i = 0; i < count; i++)
        if (strcmp(list[i].id, flight_to_update) == 0)
            list[i].seats++;

    save_flights(list, count);

    return 1;
}

void mask_card(char *dest, char *card) {
    int len = strlen(card);
    int show = 4;
    for (int i = 0; i < len - show; i++)
        dest[i] = '*';
    strcpy(dest + (len - show), card + (len - show));
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
        printf("\n[CONNECT]\n");
        int len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

        char current_user[50] = "";
        char pending_fid[20], pending_date[20], pending_passenger[50], pending_price[20];
        int waiting_payment = 0;

        while (1) {
            char buffer[MAX] = {0};
            int n = recv(connfd, buffer, MAX, 0);
            if (n <= 0) break;

            buffer[n] = '\0';
            char msg[MAX] = "";

            char u[50], e[100], p[50];

            if (sscanf(buffer, "register %s %s %s", u, e, p) == 3) {
                int ok = register_user(u, e, p);
                printf("[REGISTER] user=%s email=%s status=%s\n",
                       u, e, ok ? "success" : "fail");
                send(connfd, ok ? "register success" : "register fail",
                     ok ? 16 : 13, 0);
                continue;
            }

            if (sscanf(buffer, "login %s %s", u, p) == 2) {
                int ok = login_user(u, p);
                printf("[LOGIN] user=%s status=%s\n", u, ok ? "success" : "fail");
                if (ok) strcpy(current_user, u);
                send(connfd, ok ? "login success" : "login fail",
                     ok ? 13 : 11, 0);
                continue;
            }

            if (strncmp(buffer, "search", 6) == 0) {
                char from[20], to[20], date[20];
                sscanf(buffer, "search %s %s %s", from, to, date);
                printf("[SEARCH] user=%s from=%s to=%s date=%s\n",
                       current_user, from, to, date);
                search_flight(from, to, date, msg);
                send(connfd, msg, strlen(msg), 0);
                continue;
            }

            if (waiting_payment == 0 && strncmp(buffer, "book", 4) == 0) {
                char fid[20], date[20], passenger[50];
                sscanf(buffer, "book %s %s %s", fid, date, passenger);

                printf("[BOOK REQUEST] user=%s flight=%s date=%s passenger=%s\n",
                       current_user, fid, date, passenger);

                int res = book_flight_prepare(current_user, fid, date, passenger, pending_price);
                if (res == 1) {
                    strcpy(pending_fid, fid);
                    strcpy(pending_date, date);
                    strcpy(pending_passenger, passenger);

                    char testbuf[50];
                    sprintf(testbuf, "payment_required %s", pending_price);
                    send(connfd, testbuf, strlen(testbuf), 0);
                    waiting_payment = 1;
                }
                else if (res == -2) send(connfd, "flight not found", 16, 0);
                else if (res == -3) send(connfd, "flight cancelled", 16, 0);
                else if (res == -4) send(connfd, "no seats", 8, 0);
                else send(connfd, "unknown error", 13, 0);

                continue;
            }

            if (waiting_payment == 1 && strncmp(buffer, "payment", 7) == 0) {
                char type[20], cardnum[40], exp[10], cvv[10];
                sscanf(buffer, "payment %s %s %s %s", type, cardnum, exp, cvv);

                char masked[40];
                mask_card(masked, cardnum);

                printf("[PAYMENT] user=%s type=%s card=%s ",
                       current_user, type, masked);

                if (!validate_payment(type, cardnum, exp, cvv)) {
                    printf("status=fail\n");
                    send(connfd, "payment failed", 14, 0);
                    waiting_payment = 0;
                    continue;
                }
                printf("status=success\n");

                char tid[20];
                int ok = confirm_booking(
                            current_user,
                            pending_fid,
                            pending_date,
                            pending_passenger,
                            pending_price,
                            msg,
                            tid);

                printf("[TICKET ISSUED] %s user=%s\n", tid, current_user);

                send(connfd, msg, strlen(msg), 0);

                waiting_payment = 0;
                continue;
            }

            if (strcmp(buffer, "viewticket") == 0) {
                printf("[VIEW TICKETS] user=%s\n", current_user);
                view_my_tickets(current_user, msg);
                send(connfd, msg, strlen(msg), 0);
                continue;
            }

            if (strncmp(buffer, "cancel", 6) == 0) {
                char tid[20];
                sscanf(buffer, "cancel %s", tid);

                printf("[CANCEL] user=%s tid=%s\n", current_user, tid);

                if (cancel_ticket(current_user, tid, msg)) {
                    printf("[CANCEL RESULT] success tid=%s\n", tid);
                } else {
                    printf("[CANCEL RESULT] fail tid=%s\n", tid);
                }

                send(connfd, msg, strlen(msg), 0);
                continue;
            }

            if (strcmp(buffer, "logout") == 0) {
                printf("[LOGOUT] user=%s\n", current_user);
                break;
            }

            send(connfd, "unknown command", 16, 0);
        }

        printf("[DISCONNECT]\n");
        closesocket(connfd);
    }

    closesocket(listenfd);
    WSACleanup();
    return 0;
}
