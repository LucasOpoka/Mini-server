#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client
{
    int     id;
    char    msg[350000];
}   t_client;

t_client    clients[2042];
fd_set      read_set, write_set, current;
int         maxfd, ret, count = 0;
char        send_buffer[400000], recv_buffer[400000];

void err(char *msg)
{
    if (!msg)
        msg = "Fatal error\n";
    write(2, msg, strlen(msg));
    exit(1);
}

void    send_to_all(int except)
{
    for (int fd = 0; fd <= maxfd; fd++)
    {
        if  (FD_ISSET(fd, &write_set) && fd != except && send(fd, send_buffer, strlen(send_buffer), 0) == -1)
                err(NULL);
    }
}

int     main(int ac, char **av)
{
    if (ac != 2)
        err("Wrong number of arguments\n");

    struct sockaddr_in  serveraddr;
    socklen_t len = sizeof(struct sockaddr_in);
    int serverfd = maxfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1) err(NULL);


    FD_ZERO(&current);
    FD_SET(serverfd, &current);
    bzero(clients, sizeof(clients));
    bzero(&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(av[1]));

    if (bind(serverfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1 || listen(serverfd, 100) == -1)
        err(NULL);

    while (1)
    {
        read_set = write_set = current;
        if (select(maxfd + 1, &read_set, &write_set, 0, 0) == -1)
            continue;

        for (int fd = 0; fd <= maxfd; fd++)
        {
            if (!FD_ISSET(fd, &read_set))
                continue;

            if (fd == serverfd)
            {
                int clientfd = accept(serverfd, (struct sockaddr *)&serveraddr, &len);
                
                if (clientfd >= 0)
                {
                    maxfd = (maxfd > clientfd) ? maxfd : clientfd;
                    clients[clientfd].id = count++;
                    FD_SET(clientfd, &current);
                    sprintf(send_buffer, "server: client %d just arrived\n", clients[clientfd].id);
                    send_to_all(clientfd);
                    break;
                }
            }

            else if ((ret = recv(fd, recv_buffer, sizeof(recv_buffer), 0)) <= 0)
            {
                sprintf(send_buffer, "server: client %d just left\n", clients[fd].id);
                send_to_all(fd);
                bzero(clients[fd].msg, strlen(clients[fd].msg));
                FD_CLR(fd, &current);
                close(fd);
                break;
            }

            else
            {
                for (int i = 0, j = strlen(clients[fd].msg); i < ret; i++)
                {
                    if (recv_buffer[i] != '\n')
                        clients[fd].msg[j++] = recv_buffer[i];
                    else
                    {
                        sprintf(send_buffer, "client %d: %s\n", clients[fd].id, clients[fd].msg);
                        send_to_all(fd);
                        bzero(clients[fd].msg, strlen(clients[fd].msg));
                        j = 0;
                    }
                }
            }
        }
    }
    return (0);
}
