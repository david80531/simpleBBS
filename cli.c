#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  /* contains a number of basic derived types */
#include <sys/socket.h> /* provide functions and data structures of socket */
#include <arpa/inet.h>  /* convert internet address and dotted-decimal notation */
#include <netinet/in.h> /* provide constants and structures needed for internet domain addresses*/
#include <unistd.h>     /* `read()` and `write()` functions */
#include <dirent.h>     /* format of directory entries */
#include <sys/stat.h>   /* stat information about files attributes */
#include <time.h>

#define MAX_SIZE 2048
#define MAX_LINE 256
#define OFFSET 100

struct sockaddr_in svr_tcp_addr, svr_udp_addr, chat_svr_addr, chat_cli_addr, cli_udp_addr;
char userid[MAX_SIZE];

int connection_handler(int);
void login_handler(int);
void lsuser_handler(int);
void chat_handler(int);
void printInfo();
void connection_establisher(int);
void broadcast_handler(int);
void post_handler(int);

int main(int argc, char **argv){
  int tcp_fd, udp_fd, chat_svr_fd, connection_fd;
  int i;
  char buf[MAX_SIZE];


  int max_fd;
  int num_of_ready_fd;
  int input_fd = fileno(stdin);
  socklen_t len;

  //initialize

  len = sizeof(chat_svr_addr);
  memset(buf, '\0', MAX_SIZE);
  memset(userid, '\0', MAX_SIZE);


  fd_set all_fd_set, read_fd_set;

  tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
  chat_svr_fd = socket(AF_INET, SOCK_STREAM, 0);
  udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

  if(tcp_fd<0){
    perror("Client Create TCP Socket Failed!\n");
    exit(1);
  }

  if(chat_svr_fd<0){
    perror("Client create chat server error!\n");
    exit(1);
  }


  if(udp_fd<0){
    perror("Client Create UDP Socket Failed!\n");
    exit(1);
  }

  bzero(&svr_tcp_addr, sizeof(svr_tcp_addr));
  svr_tcp_addr.sin_addr.s_addr = inet_addr(argv[1]);
  svr_tcp_addr.sin_family = AF_INET;
  svr_tcp_addr.sin_port = htons(atoi(argv[2]));

  bzero(&svr_udp_addr, sizeof(svr_udp_addr));
  svr_udp_addr.sin_addr.s_addr = inet_addr(argv[1]);
  svr_udp_addr.sin_family = AF_INET;
  svr_udp_addr.sin_port = htons(atoi(argv[3]));



  if (connect(tcp_fd, (struct sockaddr *)&svr_tcp_addr, sizeof(svr_tcp_addr)) < 0) {
    perror("Connect by TCP failed");
    exit(1);
  }

  if(connect(udp_fd, (struct sockaddr *)&svr_udp_addr, sizeof(svr_udp_addr)) < 0){
    perror("Connect by UDP failed");
    exit(1);
  }

  getsockname(tcp_fd, (struct sockaddr *) &chat_svr_addr, &len);
  printf("Client TCP port is:%d\n", ntohs(chat_svr_addr.sin_port));

  chat_svr_addr.sin_port = htons(ntohs(chat_svr_addr.sin_port) + OFFSET);

  printf("Chat server listen on port:%d\n", ntohs(chat_svr_addr.sin_port));

  len = sizeof(cli_udp_addr);

  getsockname(udp_fd, (struct sockaddr *) &cli_udp_addr, &len);
  printf("Client UDP port is:%d\n", ntohs(cli_udp_addr.sin_port));



  if(bind(chat_svr_fd, (struct sockaddr*)&chat_svr_addr, sizeof(chat_svr_addr)) < 0){
    perror("Bind socket failed\n");
    exit(1);
  }

  listen(chat_svr_fd, 5);

  if(tcp_fd > udp_fd) max_fd = tcp_fd;
  else max_fd = udp_fd;

  if(chat_svr_fd > max_fd) max_fd = chat_svr_fd;

  FD_ZERO(&all_fd_set);
  FD_SET(input_fd, &all_fd_set);
  FD_SET(udp_fd, &all_fd_set);
  FD_SET(chat_svr_fd, &all_fd_set);


  printInfo();
  printf("BBS client>");
  fflush(stdout);
  while(1){
      read_fd_set = all_fd_set;
      num_of_ready_fd = select(max_fd + 1, &read_fd_set, NULL, NULL, NULL);
      if(FD_ISSET(input_fd, &read_fd_set)){

        if(connection_handler(tcp_fd)==0){

          if(strcmp(userid, "")==0) printf("BBS client>");
          else printf("%s>", userid);

          fflush(stdout);
        }
          if(--num_of_ready_fd<=0) continue;
      }

      if(FD_ISSET(udp_fd, &read_fd_set)){
        broadcast_handler(udp_fd);
        printf("%s>", userid);
        fflush(stdout);
      }

      if(FD_ISSET(chat_svr_fd, &read_fd_set)){
        len = sizeof(chat_cli_addr);
        connection_fd = accept(chat_svr_fd, (struct sockaddr*)&chat_cli_addr, &len);



        memset(buf, '\0', MAX_SIZE);
        chat_handler(connection_fd);
        printf("%s>", userid);
        fflush(stdout);
        if(--num_of_ready_fd<=0) continue;
      }

  }

  close(chat_svr_fd);
  close(tcp_fd);
  close(udp_fd);
  return 0;

}

int connection_handler(int sockfd){
    char buf[MAX_SIZE];
    char op[MAX_SIZE];
    char *cmd;

    memset(buf, '\0', MAX_SIZE);
    memset(op, '\0', MAX_SIZE);

    fgets(buf, MAX_SIZE, stdin);


    if(write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\b");
        exit(1);
    }

    sleep(1);

    cmd = strtok(buf, " \n");
    strcpy(op, cmd);

    if(strcmp(op, "login")==0){
        cmd = strtok(NULL, " \n");
        strcpy(userid, cmd);
        login_handler(sockfd);
        //printf("%s\n", userid);
        //return;
    } else if(strcmp(op, "post")==0){
        if(strcmp(userid, "")==0){
          printf("Please first login!\n");
        } else{
          post_handler(sockfd);
          printf("[Info]Post Success!\n");
        }
    } else if(strcmp(op, "lspost")==0){
        if(strcmp(userid, "")==0){
          printf("Please first login!\n");
        } else{
          memset(buf, '\0', MAX_SIZE);
          read(sockfd, buf, MAX_SIZE);
          printf("%s\n", buf);
        }
    } else if(strcmp(op, "readpost")==0){
        if(strcmp(userid, "")==0){
          printf("Please first login!\n");
        } else{
          memset(buf, '\0', MAX_SIZE);
          read(sockfd, buf, MAX_SIZE);
          printf("%s\n", buf);
        }
    } else if(strcmp(op, "chat")==0){
        if(strcmp(userid, "")==0){
          printf("Please first login!\n");
        } else{
          connection_establisher(sockfd);
        }
    } else if(strcmp(op, "broadcast")==0){
        if(strcmp(userid, "")==0){
          printf("Please first login!\n");
        } else{
          return 1;
        }
    }else if(strcmp(op, "lsuser")==0){
        if(strcmp(userid, "")==0){
          printf("Please first login!\n");
        } else{
          lsuser_handler(sockfd);
        }
    }

    //printf("hello\n");

    return 0;


}

void login_handler(int sockfd){
    char buf[MAX_SIZE];

    memset(buf, '\0', MAX_SIZE);



    sprintf(buf, "%s\n%d\n%d\n", inet_ntoa(chat_svr_addr.sin_addr), ntohs(chat_svr_addr.sin_port) - OFFSET, ntohs(cli_udp_addr.sin_port));



    write(sockfd, buf, strlen(buf));
    sleep(1);
    memset(buf, '\0', MAX_SIZE);
    read(sockfd, buf, MAX_SIZE);
    printf("%s\n", buf);
    return;
}

void printInfo(){
    printf("\n\n\nWelcome to BBS system!\n");
    printf("[Info] Type login + account to login\n");
    printf("[Info] Type post to make posts\n");
    printf("[Info] Type lspost to list all the posts in BBS system\n");
    printf("[Info] Type readpost to read the post interested\n");
    printf("[Info] Type chat + account to chat with other user\n");
    printf("[Info] Type broadcast to make broadcast to other user\n");
}

void lsuser_handler(int sockfd){

    char buf[MAX_SIZE];
    memset(buf, '\0', MAX_SIZE);

    read(sockfd, buf, MAX_SIZE);

    printf("Online  Users:\n");

    printf("--------------------------------\n");

    printf("%s\n", buf);

    printf("--------------------------------\n");

    return;
}

void chat_handler(int sockfd){
  char line[MAX_SIZE];
  char buf[MAX_SIZE];
  int max_fd;
  int input_fd = fileno(stdin);
  int num_of_ready_fd;
  char user[MAX_SIZE];

  memset(buf, '\0', MAX_SIZE);
  memset(line, '\0', MAX_SIZE);
  memset(user, '\0', MAX_SIZE);

  fd_set all_fd_set, read_fd_set;

  write(sockfd, userid, strlen(userid));    //tell other who you are

  sleep(1);

  read(sockfd, user, MAX_SIZE);        //receive other's name

  printf("You are now chating with %s!\n", user);
  printf("[Info]Type endchat to end the chat\n");

  printf("%s's Chatting room \n", userid);
  printf("-----------------------------\n");

  fflush(stdout);

  FD_ZERO(&all_fd_set);
  FD_SET(sockfd, &all_fd_set);
  FD_SET(input_fd, &all_fd_set);

  if(input_fd > sockfd) max_fd = input_fd;
  else max_fd = sockfd;

  while(1){
    read_fd_set = all_fd_set;
    num_of_ready_fd = select(max_fd + 1, &read_fd_set, NULL, NULL, NULL);

    if(FD_ISSET(input_fd, &read_fd_set)){
      fgets(line, MAX_SIZE, stdin);
      if(write(sockfd, line, strlen(line))<0){
        perror("Write Bytes Failed\n");
        exit(1);
      }

      if(strcmp(line, "endchat\n")==0){
        printf("Chat ended!\n");
        break;
      }
      printf("%s:%s\n", userid, line);

      if(--num_of_ready_fd <= 0) continue;

    }

    if(FD_ISSET(sockfd, &read_fd_set)){
      memset(buf, '\0', MAX_SIZE);

      if(read(sockfd, buf, MAX_SIZE)==0){
        printf("Chat is ended by %s!\n", user);
        break;
      }


      if(strcmp(buf, "endchat\n")==0){
        printf("Chat is ended by %s!\n", user);
        break;
      }

      printf("%s:%s\n", user, buf);

      if(--num_of_ready_fd <= 0) continue;

    }

  }
  printf("end chat-------------------------\n");
  close(sockfd);
  return;

}

void connection_establisher(int sockfd){
  char buf[MAX_SIZE];
  struct sockaddr_in chat_cli_addr;
  char *cmd;
  char op[MAX_SIZE];
  int chat_cli_fd;

  memset(op, '\0', MAX_SIZE);
  memset(buf, '\0', MAX_SIZE);


  if(read(sockfd, buf, MAX_SIZE)<0){
    perror("Read Bytes Failed\n");
    exit(1);
  }

  if(strcmp(buf, "No such user online!\n")==0){
    printf("%s\n", buf);
    return;
  }

  chat_cli_fd = socket(AF_INET, SOCK_STREAM, 0);

  cmd = strtok(buf, " \n");
  strcpy(op, cmd);

  bzero(&chat_cli_addr, sizeof(chat_cli_addr));

  chat_cli_addr.sin_family = AF_INET;
  chat_cli_addr.sin_addr.s_addr = inet_addr(op);

  cmd = strtok(NULL, " \n");
  strcpy(op, cmd);

  chat_cli_addr.sin_port = htons(atoi(op));

  //printf("%s:%d\n", inet_ntoa(chat_cli_addr.sin_addr), ntohs(chat_cli_addr.sin_port));

  if (connect(chat_cli_fd, (struct sockaddr *)&chat_cli_addr, sizeof(chat_cli_addr)) < 0) {
    perror("Connect by TCP failed");
    exit(1);
  }

  chat_handler(chat_cli_fd);

}

void broadcast_handler(int sockfd){
  char recv_line[MAX_LINE+1];
  int recv_num;

  memset(recv_line, '\0', MAX_LINE+1);

  recv_num = recvfrom(sockfd, recv_line, MAX_LINE, 0, NULL, NULL);

  printf("[Broadcast]:");
  printf("%s\n", recv_line);

  return;

}

void post_handler(int sockfd){
  char buf[MAX_SIZE];
  char line[MAX_SIZE];
  char cur_time[MAX_SIZE];

  time_t epoch;

  memset(buf, '\0', MAX_SIZE);
  memset(line, '\0', MAX_SIZE);
  memset(cur_time, '\0', MAX_SIZE);

  printf("[Info] Posts can not exceed 1800 words!\n");
  printf("[Info] Type endpost to end the posts!\n");

  printf("Topic:");
  fgets(buf, MAX_SIZE, stdin);

  if(strcmp(buf, "endpost")==0){
    printf("User end the post!\n");
    return;
  } else{
    write(sockfd, buf, strlen(buf));   // pass the topic
    sleep(1);
  }

  printf("Content:");

  while(fgets(line, MAX_SIZE, stdin)){
    if(strcmp(line, "endpost\n")==0){
      printf("User end the post!\n");
      if(strlen(buf) > 1800){
        printf("Posts exceed 2048 words!\n");
        printf("Post failed!\n");
        return;
      }

      epoch = time(NULL);
      sprintf(cur_time, "At: %s\n", asctime(localtime(&epoch)));
      sprintf(buf, "%s\nEditor ID:%s\nIP From:%s:%d\n%s", buf, userid, inet_ntoa(chat_svr_addr.sin_addr), ntohs(chat_svr_addr.sin_port) - OFFSET, cur_time);

      //printf("%s\n", buf);
      write(sockfd, buf, strlen(buf));
      sleep(2);
      return;
    }
    strcat(buf, line);

    memset(line, '\0', MAX_SIZE);
  }




}
