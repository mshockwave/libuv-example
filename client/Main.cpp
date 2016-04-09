#include <cstdio>
#include <Common.h>

static const char* gMenu[] = {
        "\tcd <remote dir>:  Change Remote Download Folder",
        "\tls:  List Remote Files",
        "\tput <file in current dir>:  Upload File",
        "\tget <file in remote dir>:  Download Files",
        "\texit:  Exit"
};
static void print_menu(){
    for(int i = 0; i < 60; i++) putc('=', stdout);
    puts("");

    for(int i = 0; i < (sizeof(gMenu) / sizeof(const char*)); i++){
        puts(gMenu[i]);
    }

    putc('>', stdout);
}

int main(int argc, char **argv){

    //Socket part
    if(argc - 1 < 2){
        printf("Usage: %s <server address> <server port>\n", argv[0]);
        return 1;
    }

    int socketFd;
    struct sockaddr_in serverAddr;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);

    int port;
    if( (port = atoi(argv[2])) < 0 ){
        printf("Usage: %s <server address> <server port>\n", argv[0]);
        return 1;
    }

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    if( connect(socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 ){
        fprintf(stderr, "Error connecting to %s:%d\n", argv[1], port);
        return 1;
    }

    //Console part
    char input_buffer[200] = {'\0'};
    std::string input_str;
    do{
        if(strlen(input_buffer) > 0){
            input_str = input_buffer;

            if(input_str.find("cd ") == 0){

            }else if(input_str.find("ls ") == 0){

            }else if(input_str.find("put ") == 0){

            }else if(input_str.find("get ") == 0){

            }else if(input_str.find("exit") == 0){

            }else{
                //Unknown command
                fprintf(stderr, "Unknown command: %s\n", input_str.c_str());
            }
        }
        print_menu();
    }while(scanf("%s", input_buffer) != EOF);

    return 0;
}

