#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

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

