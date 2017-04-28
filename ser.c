#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h> // strtol()
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_SIZE 2048
#define MAX_LINE 256
#define PORT 8888
#define MAX_CONNECTION 5
#define OFFSET 100

typedef  struct {
  int fd;
  char id[20];
  struct sockaddr_in addr, udp_addr;
} USER;

//global parameter
USER user[20];

int connection_handler(int, int);
void login_handler(char[], int);
void lsuser_handler(int);
void chat_handler(int, char[]);
void broadcast_handler(int, char[]);

int main(int argc, char **argv){
  int listen_fd, connection_fd, client_fd, udp_fd;
  struct sockaddr_in svr_addr, cli_addr, svr_udp_addr;
  socklen_t addr_len;



  fd_set all_fd_set, read_fd_set;
  int max_index;
  int client_fd_arr[FD_SETSIZE];
  int num_of_ready_fd;
  int i;
  int max_fd;

  char buf[MAX_SIZE];
  int read_bytes;
  int flag;

  //initialize
  memset(buf, '\0', MAX_SIZE);
  for(i = 0; i < 20; i++){
    memset(user[i].id, '\0', 20);
    user[i].fd = -1;
  }


  udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);

  if(listen_fd<0){
    perror("Server Create TCP Socket Failed!\n");
    exit(1);
  }

  if(udp_fd<0){
    perror("Server Create UDP Socket Failed!\n");
    exit(1);
  }



  bzero((void *)&svr_addr, sizeof(svr_addr));
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  svr_addr.sin_port = htons(atoi(argv[1]));

  bzero(&svr_udp_addr, sizeof(svr_udp_addr));
  svr_udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  svr_udp_addr.sin_family = AF_INET;
  svr_udp_addr.sin_port = htons(atoi(argv[2]));


  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
  setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

  if(bind(listen_fd, (struct sockaddr *)&svr_addr, sizeof(svr_addr)) < 0){
    perror("Bind TCP socket failed\n");
    exit(1);
  }

  if(bind(udp_fd, (struct sockaddr *)&svr_udp_addr, sizeof(svr_udp_addr)) < 0){
    perror("Bind UDP socket failed\n");
    exit(1);
  }

  listen(listen_fd, MAX_CONNECTION);

  printf("Listen on port %s:%s\n", inet_ntoa(svr_addr.sin_addr), argv[1]);
  printf("Welcome to BBS server\n");
  printf("Waiting for users...\n");




  // initialize
  max_index = -1;
  for(i = 0; i < FD_SETSIZE; i++){
    client_fd_arr[i] = -1;
  }

  max_fd = listen_fd;
  FD_ZERO(&all_fd_set);
  FD_SET(listen_fd, &all_fd_set);

  while(1){
    read_fd_set = all_fd_set;
    num_of_ready_fd = select(max_fd + 1, &read_fd_set, NULL, NULL, NULL);


    if(FD_ISSET(listen_fd, &read_fd_set)){
        addr_len = sizeof(cli_addr);
        connection_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &addr_len);

        for(i = 0; i < FD_SETSIZE; i++){
          if(client_fd_arr[i]<0){
            client_fd_arr[i] = connection_fd;

            printf("connection is from %d(port), %d(cli_fd)\n", ntohs(cli_addr.sin_port), connection_fd);


            break;
          }
        }

        if(i==FD_SETSIZE) printf("Client connection has reached limit!\n");
        else FD_SET(connection_fd, &all_fd_set);

        if(i > max_index) max_index = i;
        if(connection_fd > max_fd) max_fd = connection_fd;

        if(--num_of_ready_fd <= 0) continue;
    }

    for(i = 0; i <= max_index; i++){
      if(client_fd_arr[i] < 0) continue;
      client_fd = client_fd_arr[i];

      if(FD_ISSET(client_fd, &read_fd_set)){
        if(connection_handler(client_fd, udp_fd)==0){
            FD_CLR(client_fd, &all_fd_set);
            client_fd_arr[i] = -1;
            user[i].fd = -1;
        };

        //memset(buf, '\0', MAX_SIZE);
        //read(client_fd, buf, MAX_SIZE);
        //write(client_fd, buf, strlen(buf));
        //if(--num_of_ready_fd <= 0) break;
      }
    }
  }
  //close(client_fd);
  close(listen_fd);


  return 0;
}

int connection_handler(int sockfd, int udp_fd){
  char buf[MAX_SIZE];
  char * cmd;
  char instruct[MAX_SIZE];
  char op[MAX_SIZE];

  memset(buf, '\0', MAX_SIZE);
  memset(op, '\0', MAX_SIZE);
  memset(instruct, '\0', MAX_SIZE);

  if(read(sockfd, instruct, MAX_SIZE) == 0){
    printf("Client(%d) is disconnected \n", sockfd);
    return 0;
  }

  cmd = strtok(instruct, " \n");
  strcpy(op, cmd);

  if(strcmp(op, "login")==0){
    cmd = strtok(NULL, " \n");
    strcpy(op, cmd);
    login_handler(op, sockfd);
  } else if(strcmp(op, "post")==0){

  } else if(strcmp(op, "lspost")==0){

  }else if(strcmp(op, "readpost")==0){

  }else if(strcmp(op, "chat")==0){
    cmd = strtok(NULL, " \n");
    strcpy(op, cmd);
    chat_handler(sockfd, op);
  }else if(strcmp(op, "broadcast")==0){
    cmd = strtok(NULL, "\n");
    strcpy(op, cmd);
    broadcast_handler(udp_fd, op);
  }else if(strcmp(op, "lsuser")==0){
    lsuser_handler(sockfd);
  }

  return 1;

}

void login_handler(char uuid[], int sockfd){
    int i;
    char *cmd;
    char op[MAX_SIZE];
    char buf[MAX_SIZE];

    memset(op, '\0', MAX_SIZE);
    memset(buf, '\0', MAX_SIZE);


    for(i = 0; i < 20; i++){
        if(user[i].fd==-1){
            strcpy(user[i].id, uuid);

            if(read(sockfd, buf, MAX_SIZE) < 0 ){
                perror("Read Bytes Failed\n");
                exit(1);
            }

            cmd = strtok(buf, "\n");
            strcpy(op, cmd);       //op equals client's IP address

            user[i].addr.sin_family = AF_INET;
            user[i].addr.sin_addr.s_addr = inet_addr(op);

            user[i].udp_addr.sin_family = AF_INET;
            user[i].udp_addr.sin_addr.s_addr = inet_addr(op);

            cmd = strtok(NULL, "\n");

            strcpy(op, cmd);       //op equals client's port
            user[i].addr.sin_port = htons(atoi(op));

            cmd = strtok(NULL, "\n");

            strcpy(op, cmd);       //op equals client's port
            user[i].udp_addr.sin_port = htons(atoi(op));

            printf("Client TCP  port is:%d\n", ntohs(user[i].addr.sin_port));
            printf("Client UDP  port is:%d\n", ntohs(user[i].udp_addr.sin_port));

            user[i].fd = sockfd;

            break;
        }
    }


    if(i==20) sprintf(buf, "Server has reach limited! Please login later\n");
    else {
        sprintf(buf, "Login success!\n");
        printf("User is from %s:%d\n", inet_ntoa(user[i].addr.sin_addr), ntohs(user[i].addr.sin_port));
    }



    write(sockfd, buf, strlen(buf));
    sleep(1);

    printf("USER:%s has login!\n", user[i].id);

    return;


}

void lsuser_handler(int sockfd){
    int i;
    char buf[MAX_SIZE];
    char temp[MAX_SIZE];

    memset(buf, '\0', MAX_SIZE);

    for(i = 0; i < 20 ;i++){
        if(user[i].fd > 0){
            strcpy(temp, user[i].id);
            strcat(temp, "\n");
            strcat(buf, temp);
        }
    }

    write(sockfd, buf, strlen(buf));

    sleep(1);

    return;

}

void chat_handler(int sockfd, char uid[]){
    int i;
    char buf[MAX_SIZE];
    memset(buf, '\0', MAX_SIZE);

    for(i = 0; i < 20;i++){
      if(user[i].fd >0){
        if(strcmp(user[i].id, uid)==0){

          sprintf(buf ,"%s\n%d\n", inet_ntoa(user[i].addr.sin_addr), ntohs(user[i].addr.sin_port) + OFFSET);

          break;

        }
      }
    }

    if(i==20) {
      sprintf(buf, "No such user online!\n");
      write(sockfd, buf, strlen(buf));
    } else write(sockfd, buf, strlen(buf));

    sleep(1);

    return;

}

void broadcast_handler(int sockfd, char content[]){
  int i;
  socklen_t len;

  for(i = 0;i < 20; i++){
    if(user[i].fd > 0){

      len = sizeof(user[i].udp_addr);

      sendto(sockfd, content, strlen(content), 0, (struct sockaddr *)&user[i].udp_addr, len);

    }
  }

  printf("Broadcast complete!\n");

  return;

}
